#include "Sandgrid.h"
#include <iostream>

#define IN_RANGE(x, y) (x>=0 && x < width && y >= 0 && y<height)
#define MAT_AT(x, y) (IN_RANGE(x, y) ? _grid[y][x].material : Material::OutOfBounds)

const int gpCnt = 8;
const Material gasPermeables[gpCnt] = {
	Material::Nothing, 
	Material::Sand, 
	Material::Water, 
	//Material::Rock,
	Material::Wood,
	Material::Fire,
};

const int MAX_FRAME_COUNT = 100000;

Uint32 colors[(int) Material::OutOfBounds] = {
// alpha channel is ignored and encoded to index in constructor
	0xffffffff, // hiçlik
	0xf7b740ff, // Kum
	0x3390beff, // Su
	0x3d2d0aff, // Taþ
	0x8a6543ff, // Odun
	0xeb780cff, // Ateþ
	0xc7c7c7ff, // Duman
};
SandGrid::SandGrid(int w, int h, int updatesPerSec) {
	width = w;  // Grid geniþliði atanýr
	height = h;  // Grid yüksekliði atanýr
	init();  // SandGrid'in baþlatýlmasý için init() fonksiyonu çaðrýlýr
	setUpdateRate(updatesPerSec);  // Güncelleme hýzý atanýr
}

SandGrid::SandGrid(SDL_Surface* surface, int updatesPerSec) {
	width = surface->w;  // Grid geniþliði, yüzeyin geniþliði olarak atanýr
	height = surface->h;  // Grid yüksekliði, yüzeyin yüksekliði olarak atanýr
	init();  // SandGrid'in baþlatýlmasý için init() fonksiyonu çaðrýlýr
	setUpdateRate(updatesPerSec);  // Güncelleme hýzý atanýr

	int res = SDL_LockSurface(surface);  // Yüzeyin kilidi açýlýr
	if (res != 0) {
		std::cout << "Error locking surface" << std::endl;  // Yüzeyin kilidi açýlamazsa hata mesajý yazdýrýlýr
	}

	Uint32* pixels = (Uint32*)surface->pixels;  // Yüzey pikselleri alýnýr

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int ix = width * y + x;  // Piksel dizin hesaplanýr
			Uint32 px = pixels[ix];  // Piksel deðeri alýnýr

			// Bmp Start Img

			// RGB renk deðerleri hesaplanýr
			float r = ((char)((0x000000FF) & (px & surface->format->Rmask) >> (surface->format->Rshift)) / 255.f);
			float g = ((char)((0x000000FF) & (px & surface->format->Gmask) >> (surface->format->Gshift)) / 255.f);
			float b = ((char)((0x000000FF) & (px & surface->format->Bmask) >> (surface->format->Bshift)) / 255.f);
			glm::vec3 col = glm::vec3(r, g, b);  // Renk vektörü oluþturulur

			float closestDist = 100.0;  // En yakýn mesafe baþlangýç deðeri

			Material mat = Material::Nothing;  // Materyal baþlangýç deðeri (hiçbir þey)
			// Eþleþen materyali bul
			for (int j = 0; j < (int)Material::OutOfBounds; j++)
			{
				Uint32 matCol = colors[j];  // Materyal rengi alýnýr
				float mr = ((char)((matCol & 0xFF000000) >> 24) / 255.f);
				float mg = ((char)((matCol & 0x00FF0000) >> 16) / 255.f);
				float mb = ((char)((matCol & 0x0000FF00) >> 8) / 255.f);
				glm::vec3 c = glm::vec3(mr, mg, mb);  // Materyal rengi vektörü oluþturulur

				float d = glm::length(c - col);  // Renkler arasý mesafe hesaplanýr
				if (d < closestDist) {
					closestDist = d;
					mat = (Material)j;  // En yakýn mesafedeki materyal atanýr
				}
			}

			Particle* p = &_grid[y][x];  // Griddeki parçacýk alýnýr
			p->material = mat;  // Parçacýðýn materyali atanýr
		}
	}

	SDL_UnlockSurface(surface);  // Yüzeyin kilidi kapatýlýr
}

SandGrid::~SandGrid() {

	delete[] _data;  // Data dizisi silinir

	for (int i = 0; i < height; i++) {
		delete[] _grid[i];  // Her satýr için grid dizisi silinir
	}
	delete[] _grid;  // Grid dizisi silinir
	_grid = NULL;  // Grid iþaretçisi geçersiz hale getirilir
}

void SandGrid::init() {

	_grid = new Particle * [height];  // Grid için bellek ayrýlýr
	for (int i = 0; i < height; i++) {
		_grid[i] = new Particle[width];  // Her satýr için bellek ayrýlýr
	}

	int sz = width * height * 4;
	_data = new Uint32[sz];  // Data dizisi için bellek ayrýlýr

	// Materyal ID'si alfa kanalýnda kodlanýr
	for (Uint32 i = 1; i < (Uint32)Material::OutOfBounds; i++) {
		Uint32 c = colors[i];
		c >>= 8;
		c <<= 8;
		c |= (0xff - (char)i);
		colors[i] = c;
	}
}

void SandGrid::setUpdateRate(int ups) {

	updatesPerSec = ups;  // Güncelleme hýzý atanýr

	lifetimes.woodFireResistance = updatesPerSec / 4;  // Ahþap yangýn direnci atanýr
	lifetimes.fire = updatesPerSec;  // Ateþ yaþam süresi atanýr
	lifetimes.gas = updatesPerSec / 8;  // Gaz yaþam süresi atanýr
	lifetimes.rock = updatesPerSec / 2;  // Taþ yaþam süresi atanýr
}

int SandGrid::getUpdateRate() {
	return updatesPerSec;  // Güncelleme hýzý döndürülür
}
SDL_Surface* SandGrid::createSurface() {
	const int depth = 32;  // Yüzey derinliði
	const int pitch = 4 * width;  // Yüzey kaydýrma deðeri
	const Uint32 rmask = 0xff000000;  // Kýrmýzý maske
	const Uint32 gmask = 0x00ff0000;  // Yeþil maske
	const Uint32 bmask = 0x0000ff00;  // Mavi maske
	const Uint32 amask = 0x000000ff;  // Alfa maske

	SDL_Surface* srf = SDL_CreateRGBSurfaceFrom(getData(),
		width, height, depth, pitch, rmask, gmask, bmask, amask);  // RGB yüzey oluþturur

	return srf;
}

void SandGrid::drawGrid(SDL_Texture* tex) {
	void* p;
	int pitch;
	SDL_LockTexture(tex, NULL, &p, &pitch);  // Doku kilidini açar ve piksel dizisini alýr
	Uint32* pixels = (Uint32*)p;  // Piksel dizisini Uint32 türüne dönüþtürür
	int wGrid = pitch / 4;  // Piksel arabelleðinin geniþliði

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int ix = (y * wGrid + x);  // Dizin hesaplamasý
			pixels[ix] = colors[(int)MAT_AT(x, y)];  // Renk deðerini piksel dizisine aktarýr
		}
	}

	SDL_UnlockTexture(tex);  // Doku kilidini kapatýr
}

void* SandGrid::getData() {
	int ix = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int c = (int)MAT_AT(x, y);  // Matrisin deðerini alýr
			_data[ix] = colors[c];  // Renk deðerini veri dizisine aktarýr
			ix++;
		}
	}
	return (void*)_data;
}

inline void SandGrid::swap(int x1, int y1, int x2, int y2) {
	Particle p1 = _grid[y1][x1];  // Ýlk parçacýðý kaydeder
	Particle p2 = _grid[y2][x2];  // Ýkinci parçacýðý kaydeder

	_grid[y1][x1] = p2;  // Ýlk parçacýðý ikinci parçacýyla deðiþtirir
	_grid[y2][x2] = p1;  // Ýkinci parçacýðý ilk parçacýyla deðiþtirir
}

inline int isAnyOf(Material material, const Material* materials, int n) {
	for (int i = 0; i < n; i++) {
		if (material == materials[i])
			return true;  // Verilen malzemelerden herhangi birine sahipse true döner
	}
	return false;  // Hiçbir malzemeye sahip deðilse false döner
}

void SandGrid::clear() {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			_grid[y][x] = _defaultParticle;  // Tüm parçacýklarý varsayýlan parçacýkla temizler
		}
	}
}

#define ANY_NEIGHBOR(mat) (!(left != mat && right != mat && top != mat && bottom != mat && \
	bottomLeft != mat && bottomRight != mat && topLeft != mat && topRight != mat))

#define ANY_MANHATTAN_NEIGHBOR(mat) (!(left != mat && right != mat && top != mat && bottom != mat))

// Yukarýdaki iki makro, belirli komþuluk durumlarýný kontrol eder
// ANY_NEIGHBOR(mat): Herhangi bir yöne komþusu olan materyali kontrol eder
// ANY_MANHATTAN_NEIGHBOR(mat): Manhattan mesafesi 1 olan komþularý olan materyali kontrol eder

void SandGrid::update(bool* quit) {

	int f = (int)frame;

	// rand is relatively slow, use a simpler pseudo-rng
	float frac = (float) (131777 * (frame - f));
	int* seed = (int*) &frac;
	int rng = *seed ^ 58477;

	for (int y = height - 1; y >= 0; y--) {

		for (int x = 0; x < width; x++) {

			if (*quit)
				return;

			Particle* p = &_grid[y][x];

			if (p->lastFrame != f) {
				p->lastFrame = f;

				Material m = p->material;
				
				// randomize left/right direction
				rng = rng ^ (x * 65423) ^ (y * 48647);
				int r = rng & 1 ? -1 : 1;

				Material bottom = MAT_AT(x, y + 1);
				Material bottomLeft = MAT_AT(x - r, y + 1);
				Material bottomRight = MAT_AT(x + r, y + 1);
				Material left = MAT_AT(x - r, y);
				Material right = MAT_AT(x + r, y);
				Material top = MAT_AT(x, y - 1);
				Material topLeft = MAT_AT(x - r, y - 1);
				Material topRight = MAT_AT(x + r, y - 1);


				switch (m) {
					case Material::Sand:

						if (bottom == Material::Nothing || bottom == Material::Water) {
							swap(x, y, x, y + 1);
						}
						else if (bottomLeft == Material::Nothing || bottomLeft == Material::Water) {
							swap(x, y, x - r, y + 1);
						}
						else if (bottomRight == Material::Nothing || bottomRight == Material::Water) {
							swap(x, y, x + r, y + 1);
						}

						break;

					case Material::Water:

						if (ANY_MANHATTAN_NEIGHBOR(Material::Fire)) {
							p->material = Material::Nothing;
						}
						if (bottom == Material::Nothing) {
							swap(x, y, x, y + 1);
						}
						else if (left == Material::Nothing) {
							swap(x, y, x - r, y);
						}
						else if (right == Material::Nothing) {
							swap(x, y, x + r, y);
						}
						else if (bottomLeft == Material::Nothing) {
							swap(x, y, x - r, y + 1);
						}
						else if (bottomRight == Material::Nothing) {
							swap(x, y, x + r, y + 1);
						}
						

						break;

					case Material::Rock:
						 
						break;


					case Material::Wood:

						if (ANY_NEIGHBOR(Material::Water)) {
							p->age = 0;
						}

						if (r == 1 && p->age > lifetimes.woodFireResistance) {
							p->material = Material::Fire;
							p->age = 0;
						}
						else if (bottom == Material::Nothing || bottom == Material::Water) {
							swap(x, y, x, y + 1);
						}
						else if (r == 1 && (top == Material::Fire || ANY_NEIGHBOR(Material::Fire))) {
							p->age++;
						}

						break;

					case Material::Fire:
						if (p->age > lifetimes.fire && r == 1) {
							p->material = Material::Smoke;
							p->age = 0;
						}
						else if (ANY_MANHATTAN_NEIGHBOR(Material::Water)) {
							p->material = Material::Nothing;
						}
						else if (bottom == Material::Nothing) {
							swap(x, y, x, y + 1);
						}
						else if (top == Material::Sand) {
							p->age++;
						}
						else if (r == 1 && (left == Material::Sand)) {
							Particle* o = &_grid[y][x - r];
							o->material = Material::Smoke;
							o->age = 0;
						}
						else if (r == 1 && (right == Material::Sand)) {
							Particle* o = &_grid[y][x + r];
							o->material = Material::Smoke;
							o->age = 0;
						}
						else if (r == 1 && (bottom == Material::Sand)) {
							Particle* o = &_grid[y + 1][x];
							o->material = Material::Smoke;
							o->age = 0;
						}

						p->age++;
						break;

					case Material::Smoke:
						if (p->age > lifetimes.gas && r == 1) {

							switch (m) {
							case Material::Smoke:
								p->material = Material::Nothing;
								break;

							default:
								break;
							}
							p->age = 0;
						}
						else if (isAnyOf(topRight, gasPermeables, gpCnt)) {
							swap(x, y, x + r, y - 1);
						}
						else if (isAnyOf(topLeft, gasPermeables, gpCnt)) {
							swap(x, y, x - r, y - 1);
						}
						else if (isAnyOf(top, gasPermeables, gpCnt)) {
							swap(x, y, x, y - 1);
						}
						else if (right == Material::Nothing) {
							swap(x, y, x + r, y);
						}
						else if (left == Material::Nothing) {
							swap(x, y, x - r, y);
						}
						p->age++;

						break;

					case Material::Nothing:
					case Material::OutOfBounds:
						break;
				}

			}
		}
	}
}
void SandGrid::drawCircle(int cx, int cy, float r, Material m) {
	int x0 = (int)(cx - r);  // Dairenin sol kenarýnýn x koordinatý
	int x1 = cx + (int)(r + 0.5);  // Dairenin sað kenarýnýn x koordinatý
	int y0 = (int)(cy - r);  // Dairenin üst kenarýnýn y koordinatý
	int y1 = cy + (int)(r + 0.5);  // Dairenin alt kenarýnýn y koordinatý

	float r2 = r * r;  // Yarýçapýn karesi

	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {

			if (IN_RANGE(x, y)) {  // Koordinatýn geçerli olup olmadýðýný kontrol eder
				int dx = (x - cx);  // Merkeze olan x mesafesi
				int dy = (y - cy);  // Merkeze olan y mesafesi
				int d2 = dx * dx + dy * dy;  // Mesafelerin karesinin toplamý

				if (d2 <= r2) {  // Mesafe karesi yarýçap karesinden küçük veya eþitse
					Particle* p = &_grid[y][x];  // Parçacýðý alýr
					p->material = m;  // Parçacýðýn malzemesini günceller
					p->age = 0;  // Parçacýðýn yaþýný sýfýrlar
				}
			}
		}
	}
}

void SandGrid::tick(Uint32 dt) {
	int oldFrame = (int)frame;  // Önceki kare numarasýný kaydeder
	frame += updatesPerSec * (dt / 1000.0);  // Çerçeve sayýsýný günceller
	if (frame > MAX_FRAME_COUNT) {
		frame = 0;  // Çerçeve sayýsýný sýfýrlar
	}

	int f = (int)frame;  // Yeni çerçeve numarasýný alýr
	if (f != oldFrame) {
		if ((f - oldFrame) > 1) {
			std::cout << (f - oldFrame - 1) << " frame(s) skipped, frame time: " << dt << " ms" << std::endl;
			framesSkipped++;  // Atlama olanaklarý sayýsýný günceller ve konsola çýktý verir
		}
	}
}
