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
	0xffffffff, // hi�lik
	0xf7b740ff, // Kum
	0x3390beff, // Su
	0x3d2d0aff, // Ta�
	0x8a6543ff, // Odun
	0xeb780cff, // Ate�
	0xc7c7c7ff, // Duman
};
SandGrid::SandGrid(int w, int h, int updatesPerSec) {
	width = w;  // Grid geni�li�i atan�r
	height = h;  // Grid y�ksekli�i atan�r
	init();  // SandGrid'in ba�lat�lmas� i�in init() fonksiyonu �a�r�l�r
	setUpdateRate(updatesPerSec);  // G�ncelleme h�z� atan�r
}

SandGrid::SandGrid(SDL_Surface* surface, int updatesPerSec) {
	width = surface->w;  // Grid geni�li�i, y�zeyin geni�li�i olarak atan�r
	height = surface->h;  // Grid y�ksekli�i, y�zeyin y�ksekli�i olarak atan�r
	init();  // SandGrid'in ba�lat�lmas� i�in init() fonksiyonu �a�r�l�r
	setUpdateRate(updatesPerSec);  // G�ncelleme h�z� atan�r

	int res = SDL_LockSurface(surface);  // Y�zeyin kilidi a��l�r
	if (res != 0) {
		std::cout << "Error locking surface" << std::endl;  // Y�zeyin kilidi a��lamazsa hata mesaj� yazd�r�l�r
	}

	Uint32* pixels = (Uint32*)surface->pixels;  // Y�zey pikselleri al�n�r

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int ix = width * y + x;  // Piksel dizin hesaplan�r
			Uint32 px = pixels[ix];  // Piksel de�eri al�n�r

			// Bmp Start Img

			// RGB renk de�erleri hesaplan�r
			float r = ((char)((0x000000FF) & (px & surface->format->Rmask) >> (surface->format->Rshift)) / 255.f);
			float g = ((char)((0x000000FF) & (px & surface->format->Gmask) >> (surface->format->Gshift)) / 255.f);
			float b = ((char)((0x000000FF) & (px & surface->format->Bmask) >> (surface->format->Bshift)) / 255.f);
			glm::vec3 col = glm::vec3(r, g, b);  // Renk vekt�r� olu�turulur

			float closestDist = 100.0;  // En yak�n mesafe ba�lang�� de�eri

			Material mat = Material::Nothing;  // Materyal ba�lang�� de�eri (hi�bir �ey)
			// E�le�en materyali bul
			for (int j = 0; j < (int)Material::OutOfBounds; j++)
			{
				Uint32 matCol = colors[j];  // Materyal rengi al�n�r
				float mr = ((char)((matCol & 0xFF000000) >> 24) / 255.f);
				float mg = ((char)((matCol & 0x00FF0000) >> 16) / 255.f);
				float mb = ((char)((matCol & 0x0000FF00) >> 8) / 255.f);
				glm::vec3 c = glm::vec3(mr, mg, mb);  // Materyal rengi vekt�r� olu�turulur

				float d = glm::length(c - col);  // Renkler aras� mesafe hesaplan�r
				if (d < closestDist) {
					closestDist = d;
					mat = (Material)j;  // En yak�n mesafedeki materyal atan�r
				}
			}

			Particle* p = &_grid[y][x];  // Griddeki par�ac�k al�n�r
			p->material = mat;  // Par�ac���n materyali atan�r
		}
	}

	SDL_UnlockSurface(surface);  // Y�zeyin kilidi kapat�l�r
}

SandGrid::~SandGrid() {

	delete[] _data;  // Data dizisi silinir

	for (int i = 0; i < height; i++) {
		delete[] _grid[i];  // Her sat�r i�in grid dizisi silinir
	}
	delete[] _grid;  // Grid dizisi silinir
	_grid = NULL;  // Grid i�aret�isi ge�ersiz hale getirilir
}

void SandGrid::init() {

	_grid = new Particle * [height];  // Grid i�in bellek ayr�l�r
	for (int i = 0; i < height; i++) {
		_grid[i] = new Particle[width];  // Her sat�r i�in bellek ayr�l�r
	}

	int sz = width * height * 4;
	_data = new Uint32[sz];  // Data dizisi i�in bellek ayr�l�r

	// Materyal ID'si alfa kanal�nda kodlan�r
	for (Uint32 i = 1; i < (Uint32)Material::OutOfBounds; i++) {
		Uint32 c = colors[i];
		c >>= 8;
		c <<= 8;
		c |= (0xff - (char)i);
		colors[i] = c;
	}
}

void SandGrid::setUpdateRate(int ups) {

	updatesPerSec = ups;  // G�ncelleme h�z� atan�r

	lifetimes.woodFireResistance = updatesPerSec / 4;  // Ah�ap yang�n direnci atan�r
	lifetimes.fire = updatesPerSec;  // Ate� ya�am s�resi atan�r
	lifetimes.gas = updatesPerSec / 8;  // Gaz ya�am s�resi atan�r
	lifetimes.rock = updatesPerSec / 2;  // Ta� ya�am s�resi atan�r
}

int SandGrid::getUpdateRate() {
	return updatesPerSec;  // G�ncelleme h�z� d�nd�r�l�r
}
SDL_Surface* SandGrid::createSurface() {
	const int depth = 32;  // Y�zey derinli�i
	const int pitch = 4 * width;  // Y�zey kayd�rma de�eri
	const Uint32 rmask = 0xff000000;  // K�rm�z� maske
	const Uint32 gmask = 0x00ff0000;  // Ye�il maske
	const Uint32 bmask = 0x0000ff00;  // Mavi maske
	const Uint32 amask = 0x000000ff;  // Alfa maske

	SDL_Surface* srf = SDL_CreateRGBSurfaceFrom(getData(),
		width, height, depth, pitch, rmask, gmask, bmask, amask);  // RGB y�zey olu�turur

	return srf;
}

void SandGrid::drawGrid(SDL_Texture* tex) {
	void* p;
	int pitch;
	SDL_LockTexture(tex, NULL, &p, &pitch);  // Doku kilidini a�ar ve piksel dizisini al�r
	Uint32* pixels = (Uint32*)p;  // Piksel dizisini Uint32 t�r�ne d�n��t�r�r
	int wGrid = pitch / 4;  // Piksel arabelle�inin geni�li�i

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int ix = (y * wGrid + x);  // Dizin hesaplamas�
			pixels[ix] = colors[(int)MAT_AT(x, y)];  // Renk de�erini piksel dizisine aktar�r
		}
	}

	SDL_UnlockTexture(tex);  // Doku kilidini kapat�r
}

void* SandGrid::getData() {
	int ix = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int c = (int)MAT_AT(x, y);  // Matrisin de�erini al�r
			_data[ix] = colors[c];  // Renk de�erini veri dizisine aktar�r
			ix++;
		}
	}
	return (void*)_data;
}

inline void SandGrid::swap(int x1, int y1, int x2, int y2) {
	Particle p1 = _grid[y1][x1];  // �lk par�ac��� kaydeder
	Particle p2 = _grid[y2][x2];  // �kinci par�ac��� kaydeder

	_grid[y1][x1] = p2;  // �lk par�ac��� ikinci par�ac�yla de�i�tirir
	_grid[y2][x2] = p1;  // �kinci par�ac��� ilk par�ac�yla de�i�tirir
}

inline int isAnyOf(Material material, const Material* materials, int n) {
	for (int i = 0; i < n; i++) {
		if (material == materials[i])
			return true;  // Verilen malzemelerden herhangi birine sahipse true d�ner
	}
	return false;  // Hi�bir malzemeye sahip de�ilse false d�ner
}

void SandGrid::clear() {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			_grid[y][x] = _defaultParticle;  // T�m par�ac�klar� varsay�lan par�ac�kla temizler
		}
	}
}

#define ANY_NEIGHBOR(mat) (!(left != mat && right != mat && top != mat && bottom != mat && \
	bottomLeft != mat && bottomRight != mat && topLeft != mat && topRight != mat))

#define ANY_MANHATTAN_NEIGHBOR(mat) (!(left != mat && right != mat && top != mat && bottom != mat))

// Yukar�daki iki makro, belirli kom�uluk durumlar�n� kontrol eder
// ANY_NEIGHBOR(mat): Herhangi bir y�ne kom�usu olan materyali kontrol eder
// ANY_MANHATTAN_NEIGHBOR(mat): Manhattan mesafesi 1 olan kom�ular� olan materyali kontrol eder

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
	int x0 = (int)(cx - r);  // Dairenin sol kenar�n�n x koordinat�
	int x1 = cx + (int)(r + 0.5);  // Dairenin sa� kenar�n�n x koordinat�
	int y0 = (int)(cy - r);  // Dairenin �st kenar�n�n y koordinat�
	int y1 = cy + (int)(r + 0.5);  // Dairenin alt kenar�n�n y koordinat�

	float r2 = r * r;  // Yar��ap�n karesi

	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {

			if (IN_RANGE(x, y)) {  // Koordinat�n ge�erli olup olmad���n� kontrol eder
				int dx = (x - cx);  // Merkeze olan x mesafesi
				int dy = (y - cy);  // Merkeze olan y mesafesi
				int d2 = dx * dx + dy * dy;  // Mesafelerin karesinin toplam�

				if (d2 <= r2) {  // Mesafe karesi yar��ap karesinden k���k veya e�itse
					Particle* p = &_grid[y][x];  // Par�ac��� al�r
					p->material = m;  // Par�ac���n malzemesini g�nceller
					p->age = 0;  // Par�ac���n ya��n� s�f�rlar
				}
			}
		}
	}
}

void SandGrid::tick(Uint32 dt) {
	int oldFrame = (int)frame;  // �nceki kare numaras�n� kaydeder
	frame += updatesPerSec * (dt / 1000.0);  // �er�eve say�s�n� g�nceller
	if (frame > MAX_FRAME_COUNT) {
		frame = 0;  // �er�eve say�s�n� s�f�rlar
	}

	int f = (int)frame;  // Yeni �er�eve numaras�n� al�r
	if (f != oldFrame) {
		if ((f - oldFrame) > 1) {
			std::cout << (f - oldFrame - 1) << " frame(s) skipped, frame time: " << dt << " ms" << std::endl;
			framesSkipped++;  // Atlama olanaklar� say�s�n� g�nceller ve konsola ��kt� verir
		}
	}
}
