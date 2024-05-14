#include "Headers.h"
#include "Sandgrid.h"
#include "RenderMaterials.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <stdio.h>

//.. Sabitler
const int updateInc = 30;

#if defined(NDEBUG) && defined(_WIN32)
const int defaultUpdate = 240;
#else
const int defaultUpdate = 60;
#endif

const int minWinSize = 128;
const int initGridWidth = 640;
const int initGridHeight = 268;

// Fare ile Metarial Boyutu büyült/Küçük
const double minRadius = 0.5;
const double maxRadius = 20;

const char* startImagePath = "start_img.bmp";
const char* playerPath = "player.bmp";

struct MouseInfo {
	bool mouseButtonDown = false;
	int x = 0;
	int y = 0;
};

const float quadXDim = .5f;
const float quadYDim = .5f;
std::vector<GLfloat> quadVertices{

	// uvs are flipped since Sandgrid represents
	// bitmap data in top-down fashion

	// pos				uv
	-quadXDim,  quadYDim, 0.f,	0.f, 0.f,	// top left
	-quadXDim, -quadYDim, 0.f,	0.f, 1.f,	// bottom left
	 quadXDim, -quadYDim, 0.f,	1.f, 1.f,	// bottom right
	 quadXDim,  quadYDim, 0.f,	1.f, 0.f,	// top right
};
std::vector<GLuint> quadIndices{ 0, 1, 2, 3 };

SandGrid* _grid;  // Sandgrid sýnýfý nesnesi
MouseInfo _mouseInfo;  // Fare bilgileri
Material _currentMaterial = Material::Sand;  // Þu anki materyal tipi
float _scale = 1.f;  // Ölçek faktörü
float _radius = 8.f;  // Yarýçap deðeri
bool _quit = false;  // Çýkýþ bayraðý
bool _pause = true;  // Duraklatma bayraðý
bool _shadingOff = true;  // Gölgeleme kapalý bayraðý
bool _isFullscreen = true;  // Tam ekran modu bayraðý
SDL_WindowFlags _fullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;  // Tam ekran bayraðý için pencere bayraklarý

SDL_Window* _window;  // SDL pencere nesnesi
SDL_Surface* _windowSurface;  // SDL pencere yüzeyi
SDL_DisplayMode _windowMode;  // SDL pencere modu

SDL_DisplayMode _displayMode;  // SDL ekran modu
SDL_mutex* _gridLock;  // Sandgrid kilidi
SDL_Thread* _updateThread;  // Güncelleme iþ parçacýðý
SDL_GLContext _glContext;  // OpenGL baðlamý
SDL_Event _event;  // SDL olay nesnesi
SDL_Rect _srcRect;  // Kaynak dikdörtgen
SDL_Rect _destRect;  // Hedef dikdörtgen

glm::mat4 _modelMatrix;  // Model matrisi
glm::mat4 _viewMatrix;  // Görünüm matrisi
glm::mat4 _projectionMatrix;  // Projeksiyon matrisi

GLuint _vao;  // Vertex dizisi nesnesi
GLuint _vbo;  // Vertex tampon nesnesi
GLuint _ibo;  // Ýndeks tampon nesnesi

GridMaterial* _gridMaterial;  // Grid malzeme nesnesi
GridShaderParams _gridShaderParams;  // Grid shader parametreleri

SpriteMaterial* _playerMaterial;  // Oyuncu malzeme nesnesi
SpriteShaderParams _spriteParams;  // Sprite shader parametreleri

const float playerDim = 16.f;  // Oyuncu boyutu
glm::vec3 _playerPos = glm::vec3(-10000.f, 0.f, 0.f);  // Oyuncu pozisyonu

const Material materialUIIndex[] = {
	Material::Nothing,
	Material::Sand,
	Material::Water,
	Material::Rock,
	Material::Wood,
	Material::Fire,
};

bool initOpenGL() {
	_glContext = SDL_GL_CreateContext(_window);  // SDL OpenGL baðlamý oluþturuluyor
	if (_glContext == NULL) {
		std::cout << "Could not create context: " << SDL_GetError() << std::endl;
	}

	int res = SDL_GL_SetSwapInterval(0);  // Swap interval ayarlanýyor (0: VSync kapalý)
	if (res != 0) {
		std::cout << "Failed to set swap interval: " << SDL_GetError() << std::endl;
	}

	GLenum glewError = glewInit();  // GLEW baþlatýlýyor
	if (glewError != GLEW_OK) {
		std::cout << "Error initializing glew" << std::endl;
		std::cout << glewGetErrorString(glewError) << std::endl;

		return false;
	}

	glEnable(GL_CULL_FACE);  // Arka yüzleri gizleme özelliði etkinleþtiriliyor
	glEnable(GL_BLEND);  // Renk karýþýmý (alpha blend) etkinleþtiriliyor
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Blend iþlemi için faktörler ayarlanabilir

	return true;
}

static void resize() {
	_windowSurface = SDL_GetWindowSurface(_window);  // Pencere yüzeyi alýnýyor
	if (_windowSurface == NULL) {
		std::cout << "Error getting OpenGL surface during resize" << std::endl;
		return;
	}

	float w = (float)_windowSurface->w;  // Pencere geniþliði
	float h = (float)_windowSurface->h;  // Pencere yüksekliði
	float screenAspect = w / h;  // Pencere en boy oraný

	_srcRect.x = 0;
	_srcRect.y = 0;
	_srcRect.w = _grid->width;  // Kaynak dikdörtgenin geniþliði (_grid nesnesinden alýnýr)
	_srcRect.h = _grid->height;  // Kaynak dikdörtgenin yüksekliði (_grid nesnesinden alýnýr)
	float gridAspect = (float)_grid->width / _grid->height;  // Grid en boy oraný
	float gwh = _grid->width / 2.f;  // Grid geniþliðinin yarýsý
	float ghh = _grid->height / 2.f;  // Grid yüksekliðinin yarýsý

	if (screenAspect >= gridAspect) {
		_scale = (float)_windowSurface->h / _grid->height;  // Ölçek faktörü (_grid nesnesine göre hesaplanýr)
		_destRect.w = (int)(_grid->width * _scale);  // Hedef dikdörtgenin geniþliði
		_destRect.h = _windowSurface->h;  // Hedef dikdörtgenin yüksekliði
		_destRect.x = (int)((_windowSurface->w - _destRect.w) / 2.f);  // Hedef dikdörtgenin x konumu
		_destRect.y = 0;  // Hedef dikdörtgenin y konumu

		float f = 1.f + (screenAspect - gridAspect) / gridAspect;  // Ekran ve grid en boy oranlarý arasýndaki fark
		_projectionMatrix = glm::ortho(-gwh * f, gwh * f, -ghh, ghh);  // Projeksiyon matrisi ayarlanýyor
	}
	else {
		_scale = (float)_windowSurface->w / _grid->width;  // Ölçek faktörü (_grid nesnesine göre hesaplanýr)
		_destRect.w = _windowSurface->w;  // Hedef dikdörtgenin geniþliði
		_destRect.h = (int)(_grid->height * _scale);  // Hedef dikdörtgenin yüksekliði
		_destRect.x = 0;  // Hedef dikdörtgenin x konumu
		_destRect.y = (int)((_windowSurface->h - _destRect.h) / 2.f);  // Hedef dikdörtgenin y konumu

		float f = 1.f + (gridAspect - screenAspect) / screenAspect;  // Grid ve ekran en boy oranlarý arasýndaki fark
		_projectionMatrix = glm::ortho(-gwh, gwh, -ghh * f, ghh * f);  // Projeksiyon matrisi ayarlanýyor
	}
	glViewport(0, 0, _windowSurface->w, _windowSurface->h);  // OpenGL görünüm portu ayarlanýyor
}


void initGLObjects() {
	glGenVertexArrays(1, &_vao);  // Vertex dizileri oluþturuluyor
	glBindVertexArray(_vao);  // Vertex dizisi baðlanýyor

	glGenBuffers(1, &_vbo);  // Vertex buffer oluþturuluyor
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);  // Vertex buffer baðlanýyor
	glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(GLfloat), quadVertices.data(), GL_STATIC_DRAW);  // Vertex verileri yükleniyor

	glGenBuffers(1, &_ibo);  // Ýndeks buffer oluþturuluyor
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);  // Ýndeks buffer baðlanýyor
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadIndices.size() * sizeof(GLuint), quadIndices.data(), GL_STATIC_DRAW);  // Ýndeks verileri yükleniyor

	_gridShaderParams.vbo = _vbo;  // Grid için shader parametreleri ayarlanýyor
	_gridShaderParams.ibo = _ibo;

	_spriteParams.vbo = _vbo;  // Sprite için shader parametreleri ayarlanýyor
	_spriteParams.ibo = _ibo;

	_modelMatrix = glm::mat4(1);  // Model matrisi baþlangýç deðeri atanýyor
	glm::vec3 eye = glm::vec3(0.f, 0.f, 1.f);  // Göz konumu
	glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);  // Hedef noktasý
	glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);  // Yukarý vektörü
	_viewMatrix = glm::lookAt(eye, center, up);  // Görüþ matrisi hesaplanýyor
}

void cleanUp() {
	int res;
	SDL_WaitThread(_updateThread, &res);  // _updateThread'in tamamlanmasýný bekler ve sonucunu alýr
	_updateThread = NULL;

	delete _gridMaterial;  // _gridMaterial nesnesini bellekten siler
	_gridMaterial = NULL;

	delete _playerMaterial;  // _playerMaterial nesnesini bellekten siler
	_playerMaterial = NULL;

	delete _grid;  // _grid nesnesini bellekten siler
	_grid = NULL;

	SDL_DestroyMutex(_gridLock);  // _gridLock mutex'ini yok eder
	_gridLock = NULL;

	SDL_GL_DeleteContext(_glContext);  // OpenGL context'ini yok eder
	_glContext = NULL;

	SDL_FreeSurface(_windowSurface);  // _windowSurface yüzeyini bellekten siler
	_windowSurface = NULL;

	SDL_DestroyWindow(_window);  // _window penceresini yok eder
	_window = NULL;

	SDL_Quit();  // SDL'yi kapatýr
}

void toggleFullscreen() {
	_isFullscreen = !_isFullscreen;  // Tam ekran durumunu tersine çevirir

	int res;
	if (_isFullscreen) {
		res = SDL_GetWindowDisplayMode(_window, &_windowMode);  // Mevcut pencere ekran modunu alýr
		res += SDL_SetWindowDisplayMode(_window, &_displayMode);  // Pencere ekran modunu ayarlar
		res += SDL_SetWindowFullscreen(_window, _fullscreenFlag);  // Pencereyi tam ekrana ayarlar
		if (res != 0) {
			std::cout << "Error switching to fullscreen: " << SDL_GetError() << std::endl;  // Hata durumunda hata mesajýný yazdýrýr
		}
	}
	else {
		res = SDL_SetWindowFullscreen(_window, 0);  // Pencereyi tam ekrandan çýkarýr
		res = SDL_SetWindowDisplayMode(_window, &_windowMode);  // Pencereyi orijinal ekran moda geri döndürür
		if (res != 0) {
			std::cout << "Error exiting fullscreen: " << SDL_GetError() << std::endl;  // Hata durumunda hata mesajýný yazdýrýr
		}
	}

	resize();  // Yeniden boyutlandýrma iþlemini çaðýrýr
}

static int updateLoop(void* p) {
	Uint32 lastTick = SDL_GetTicks();  // Son zaman damgasýný kaydeder

	while (!_quit) {
		Uint32 now = SDL_GetTicks();  // Mevcut zaman damgasýný alýr

		Uint32 dt = now - lastTick;  // Geçen süreyi hesaplar
		lastTick = now;

		if (!_pause)  // Eðer duraklatma aktif deðilse
		{
			_grid->tick(dt);  // Grid nesnesini günceller
			SDL_LockMutex(_gridLock);  // Grid'i kilitleyerek güncelleme iþlemini yapar
			_grid->update(&_quit);  // Grid'i günceller
			SDL_UnlockMutex(_gridLock);  // Grid'i kilitten çýkarýr
		}
	}

	return 0;
}

float clamp(float x, float min, float max) {
	return MAX(min, MIN(max, x));  // Deðerin belirli bir aralýkta olmasýný saðlar
}

int clampi(int x, int min, int max) {
	return MAX(min, MIN(max, x));  // Tamsayý deðerinin belirli bir aralýkta olmasýný saðlar
}

float clamp01(float x) {
	return clamp(x, 0, 1);  // Deðerin 0 ile 1 arasýnda olmasýný saðlar
}

glm::vec2 mouseUV() {
	float x = (float)_mouseInfo.x - _destRect.x;  // Fare konumunu pencerenin içindeki koordinata dönüþtürür
	float y = (float)_mouseInfo.y - _destRect.y;
	float u = clamp01(x / _destRect.w);  // Koordinatý 0 ile 1 arasýnda bir deðere dönüþtürür
	float v = clamp01(y / _destRect.h);
	return glm::vec2(u, v);  // Fare koordinatlarýný döndürür
}

void handleMouse() {
	if (_mouseInfo.mouseButtonDown) {  // Fare düðmesi basýlýysa
		if (_mouseInfo.x >= _destRect.x && _mouseInfo.x < (_destRect.x + _destRect.w) &&
			_mouseInfo.y >= _destRect.y && _mouseInfo.y < (_destRect.y + _destRect.h)) {
			int x = _mouseInfo.x - _destRect.x;  // Fare konumunu pencere içindeki koordinatlara dönüþtürür
			int y = _mouseInfo.y - _destRect.y;
			int gx = (int)(x / _scale);  // Grid koordinatlarýna dönüþtürür
			int gy = (int)(y / _scale);

			_grid->drawCircle(gx, gy, _radius, _currentMaterial);  // Grid üzerinde daire çizer
		}
	}
}


void draw() {
	int res = SDL_TryLockMutex(_gridLock);  // Grid'i kilitleyerek güncelleme iþlemlerini durdurur
	if (res != 0) {
		return;
	}

	glClearColor(0.f, 0.f, 0.f, 1.f);  // Arka plan rengini ayarlar
	glClear(GL_COLOR_BUFFER_BIT);  // Renk tamponunu temizler

	Uint32 now = SDL_GetTicks();  // Mevcut zaman damgasýný alýr

	void* data = _grid->getData();  // Grid verilerini alýr
	SDL_UnlockMutex(_gridLock);  // Grid kilidini açar

	_gridMaterial->putData(data);  // Grid malzemesine verileri aktarýr

	_gridShaderParams.modelMatrix = glm::scale(_modelMatrix, glm::vec3(_grid->width, _grid->height, 1.0));  // Model matrisini ölçeklendirir
	_gridShaderParams.viewMatrix = _viewMatrix;  // Görünüm matrisini ayarlar
	_gridShaderParams.projectionMatrix = _projectionMatrix;  // Yansýtma matrisini ayarlar
	_gridShaderParams.gridWidth = (GLfloat)_grid->width;  // Grid geniþliðini ayarlar
	_gridShaderParams.gridHeight = (GLfloat)_grid->height;  // Grid yüksekliðini ayarlar
	_gridShaderParams.time = (GLfloat)now;  // Geçen süreyi ayarlar
	_gridShaderParams.shadingOff = (GLuint)_shadingOff;  // Gölgeleme durumunu ayarlar
	_gridShaderParams.mouseUV = mouseUV();  // Fare koordinatlarýný ayarlar
	_gridMaterial->draw(&_gridShaderParams);  // Grid malzemesini çizer

	_spriteParams.modelMatrix = glm::translate(glm::scale(_modelMatrix, glm::vec3(playerDim, playerDim, playerDim)), _playerPos / playerDim);  // Oyuncu matrisini ölçeklendirir ve konumlandýrýr
	_spriteParams.viewMatrix = _viewMatrix;  // Görünüm matrisini ayarlar
	_spriteParams.projectionMatrix = _projectionMatrix;  // Yansýtma matrisini ayarlar
	_playerMaterial->draw(&_spriteParams);  // Oyuncu malzemesini çizer

	glViewport(0, 0, _windowSurface->w, _windowSurface->h);  // Görünüm alanýný ayarlar

	SDL_GL_SwapWindow(_window);  // Pencereyi yeniler ve ekraný gösterir

	GLenum err = glGetError();  // OpenGL hata durumunu kontrol eder
	if (err != 0) {
		std::cout << "OpenGL error: " << err << std::endl;
	}
}

bool loadImage(const char* path) {
	SDL_Surface* srf = SDL_LoadBMP(path);  // Belirtilen dosyadan bir yüzey yükler
	if (srf == nullptr) {
		std::cout << "Image " << path << " not found." << std::endl;
		return false;
	}

	SDL_LockMutex(_gridLock);  // Grid'i kilitleyerek güncelleme iþlemlerini durdurur
	if (_grid != nullptr) {
		delete _grid;  // Mevcut grid nesnesini temizler
	}
	_grid = new SandGrid(srf, defaultUpdate);  // Yeni bir Sandgrid nesnesi oluþturur ve yüklenen yüzeyi kullanýr

	_gridMaterial->initTexture(srf, srf->w, srf->h);  // Grid malzemesinin dokusunu yüklenen yüzeyle baþlatýr
	SDL_UnlockMutex(_gridLock);  // Grid kilidini açar

	int w = MAX(minWinSize, srf->w);  // Pencere geniþliðini minimum boyuta ayarlar
	int h = MAX(minWinSize, srf->h);  // Pencere yüksekliðini minimum boyuta ayarlar

	SDL_SetWindowSize(_window, w, h);  // Pencere boyutunu ayarlar
	SDL_FreeSurface(srf);  // Yüklenen yüzeyi serbest býrakýr

	resize();  // Yeniden boyutlandýrma iþlemini gerçekleþtirir

	return true;
}
bool init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {  // SDL'yi video ve olaylar için baþlatýr
		std::cout << "Error initializing SDL. Error: " << SDL_GetError() << std::endl;
		return false;
	}

	if (SDL_GetDesktopDisplayMode(0, &_displayMode) != 0) {  // Masaüstü görüntüleme modunu alýr
		std::cout << "Error getting display mode: " << SDL_GetError() << std::endl;
		return false;
	}
	std::cout << "Creating window: " << _displayMode.w << "x" << _displayMode.h << "@" << _displayMode.refresh_rate << std::endl;

	int res;
	res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);  // OpenGL baðlamý için major versiyonu ayarlar
	res += SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);  // OpenGL baðlamý için minor versiyonu ayarlar
	res += SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // Çift tamponlamayý etkinleþtirir
	if (res != 0) {
		std::cout << "Failed to set OpenGL attributes: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_DisplayMode desktopMode;
	if (SDL_GetDesktopDisplayMode(0, &desktopMode) != 0) {  // Masaüstü modunu alýr
		std::cout << "Error getting desktop mode: " << SDL_GetError() << std::endl;
		return false;
	}

	// Pencere süslemeleri pencerenin görünür masaüstü alanýnýn içinde olacak þekilde kaydýrma iþlemi
	const int offset = 100;
	Uint32 flags = _fullscreenFlag | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	_window = SDL_CreateWindow("Sandspiel", offset, offset, desktopMode.w, desktopMode.h, flags);  // Pencere oluþturur
	if (_window == NULL) {
		std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
		return false;
	}

	_gridLock = SDL_CreateMutex();  // Grid kilidini oluþturur
	if (_gridLock == NULL) {
		std::cout << "Failed to create mutex: " << SDL_GetError() << std::endl;
	}

	bool ok = initOpenGL();  // OpenGL'yi baþlatýr
	if (!ok) {
		std::cout << "OpenGL initialization failed" << std::endl;
		return false;
	}

	_gridMaterial = new GridMaterial();  // Grid malzemesini oluþturur
	if (!loadImage(startImagePath)) {  // Baþlangýç resmini yükler
		std::cout << "No image found, starting with default grid" << std::endl;
		_grid = new SandGrid(initGridWidth, initGridHeight, defaultUpdate);  // Varsayýlan grid'i oluþturur

		_gridMaterial->initTexture(NULL, _grid->width, _grid->height);  // Grid malzemesinin dokusunu baþlangýç grid'iyle baþlatýr
		resize();  // Pencere boyutunu ayarlar
	}

	SDL_Surface* player = SDL_LoadBMP(playerPath);  // Oyuncu yüzeyini yükler
	_playerMaterial = new SpriteMaterial(player);  // Oyuncu malzemesini oluþturur
	SDL_FreeSurface(player);

	initGLObjects();  // OpenGL nesnelerini baþlatýr

	return true;
}
int main(int argc, char* argv[]) {

	if (!init()) {  // Baþlatma iþlemini kontrol eder
		return 1;
	}

	_updateThread = SDL_CreateThread(updateLoop, "update", (void*)NULL);  // Güncelleme döngüsü iþ parçacýðýný baþlatýr
	if (_updateThread == NULL) {
		std::cout << "Failed to start thread" << std::endl;
		return 1;
	}

	while (!_quit) {

		while (SDL_PollEvent(&_event)) {  // Olaylarý iþler
			switch (_event.type) {
			case SDL_QUIT:  // Çýkýþ olayý
				_quit = true;
				break;

			case SDL_WINDOWEVENT:  // Pencere olaylarý
				if (_event.window.event == SDL_WINDOWEVENT_RESIZED)
					resize();  // Pencere boyutunu yeniden ayarlar
				break;

			case SDL_KEYUP:  // Tuþ býrakma olayý
			{
				SDL_Keycode key = _event.key.keysym.sym;  // Býrakýlan tuþun kodunu alýr

				switch (key) {
				case SDLK_f:  // F tuþu: Tam ekraný açýp kapatýr
					toggleFullscreen();
					break;

				case SDLK_ESCAPE:  // Escape tuþu: Çýkýþ yapar
					_quit = true;
					break;

				case SDLK_SPACE:  // Boþluk tuþu: Duraklatmayý açýp kapatýr
					_pause = !_pause;
					break;

				case SDLK_0:  // 0 tuþu: Malzemeyi "Nothing" olarak ayarlar
					_currentMaterial = Material::Nothing;
					break;

				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:  // 1-5 tuþlarý: Malzemeyi ilgili indekse göre ayarlar
					_currentMaterial = materialUIIndex[(int)key - SDLK_0];
					break;

				case SDLK_DELETE:
				case SDLK_BACKSPACE:  // DELETE veya BACKSPACE tuþlarý: Grid'i temizler
					_grid->clear();
					break;

				case SDLK_r:  // R tuþu: Baþlangýç resmini yükler
					loadImage(startImagePath);
					break;
				}
				break;
			}

			case SDL_MOUSEMOTION:  // Fare hareketi olayý
				_mouseInfo.x = _event.motion.x;  // Fare konumunu günceller
				_mouseInfo.y = _event.motion.y;
				break;

			case SDL_MOUSEBUTTONDOWN:  // Fare düðmesi basma olayý
				if (_event.button.button == SDL_BUTTON_LEFT) {
					_mouseInfo.mouseButtonDown = true;  // Sol düðme basýldýðýnda bilgisini günceller
				}
				break;

			case SDL_MOUSEBUTTONUP:  // Fare düðmesi býrakma olayý
				if (_event.button.button == SDL_BUTTON_LEFT) {
					_mouseInfo.mouseButtonDown = false;  // Sol düðme býrakýldýðýnda bilgisini günceller
				}
				break;

			case SDL_MOUSEWHEEL:  // Fare tekerleði olayý
				_radius = clamp(_radius + _event.wheel.y, minRadius, maxRadius);  // Fare tekerleði hareketine göre yarýçapý günceller
				break;
			}
		}

		handleMouse();  // Fare iþleme iþlevini çaðýrýr

		draw();  // Çizim iþlemini gerçekleþtirir

	}
	std::cout << "Exit requested" << std::endl;

	cleanUp();  // Temizlik iþlemini gerçekleþtirir
	return 0;
}
