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

// Fare ile Metarial Boyutu b�y�lt/K���k
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

SandGrid* _grid;  // Sandgrid s�n�f� nesnesi
MouseInfo _mouseInfo;  // Fare bilgileri
Material _currentMaterial = Material::Sand;  // �u anki materyal tipi
float _scale = 1.f;  // �l�ek fakt�r�
float _radius = 8.f;  // Yar��ap de�eri
bool _quit = false;  // ��k�� bayra��
bool _pause = true;  // Duraklatma bayra��
bool _shadingOff = true;  // G�lgeleme kapal� bayra��
bool _isFullscreen = true;  // Tam ekran modu bayra��
SDL_WindowFlags _fullscreenFlag = SDL_WINDOW_FULLSCREEN_DESKTOP;  // Tam ekran bayra�� i�in pencere bayraklar�

SDL_Window* _window;  // SDL pencere nesnesi
SDL_Surface* _windowSurface;  // SDL pencere y�zeyi
SDL_DisplayMode _windowMode;  // SDL pencere modu

SDL_DisplayMode _displayMode;  // SDL ekran modu
SDL_mutex* _gridLock;  // Sandgrid kilidi
SDL_Thread* _updateThread;  // G�ncelleme i� par�ac���
SDL_GLContext _glContext;  // OpenGL ba�lam�
SDL_Event _event;  // SDL olay nesnesi
SDL_Rect _srcRect;  // Kaynak dikd�rtgen
SDL_Rect _destRect;  // Hedef dikd�rtgen

glm::mat4 _modelMatrix;  // Model matrisi
glm::mat4 _viewMatrix;  // G�r�n�m matrisi
glm::mat4 _projectionMatrix;  // Projeksiyon matrisi

GLuint _vao;  // Vertex dizisi nesnesi
GLuint _vbo;  // Vertex tampon nesnesi
GLuint _ibo;  // �ndeks tampon nesnesi

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
	_glContext = SDL_GL_CreateContext(_window);  // SDL OpenGL ba�lam� olu�turuluyor
	if (_glContext == NULL) {
		std::cout << "Could not create context: " << SDL_GetError() << std::endl;
	}

	int res = SDL_GL_SetSwapInterval(0);  // Swap interval ayarlan�yor (0: VSync kapal�)
	if (res != 0) {
		std::cout << "Failed to set swap interval: " << SDL_GetError() << std::endl;
	}

	GLenum glewError = glewInit();  // GLEW ba�lat�l�yor
	if (glewError != GLEW_OK) {
		std::cout << "Error initializing glew" << std::endl;
		std::cout << glewGetErrorString(glewError) << std::endl;

		return false;
	}

	glEnable(GL_CULL_FACE);  // Arka y�zleri gizleme �zelli�i etkinle�tiriliyor
	glEnable(GL_BLEND);  // Renk kar���m� (alpha blend) etkinle�tiriliyor
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // Blend i�lemi i�in fakt�rler ayarlanabilir

	return true;
}

static void resize() {
	_windowSurface = SDL_GetWindowSurface(_window);  // Pencere y�zeyi al�n�yor
	if (_windowSurface == NULL) {
		std::cout << "Error getting OpenGL surface during resize" << std::endl;
		return;
	}

	float w = (float)_windowSurface->w;  // Pencere geni�li�i
	float h = (float)_windowSurface->h;  // Pencere y�ksekli�i
	float screenAspect = w / h;  // Pencere en boy oran�

	_srcRect.x = 0;
	_srcRect.y = 0;
	_srcRect.w = _grid->width;  // Kaynak dikd�rtgenin geni�li�i (_grid nesnesinden al�n�r)
	_srcRect.h = _grid->height;  // Kaynak dikd�rtgenin y�ksekli�i (_grid nesnesinden al�n�r)
	float gridAspect = (float)_grid->width / _grid->height;  // Grid en boy oran�
	float gwh = _grid->width / 2.f;  // Grid geni�li�inin yar�s�
	float ghh = _grid->height / 2.f;  // Grid y�ksekli�inin yar�s�

	if (screenAspect >= gridAspect) {
		_scale = (float)_windowSurface->h / _grid->height;  // �l�ek fakt�r� (_grid nesnesine g�re hesaplan�r)
		_destRect.w = (int)(_grid->width * _scale);  // Hedef dikd�rtgenin geni�li�i
		_destRect.h = _windowSurface->h;  // Hedef dikd�rtgenin y�ksekli�i
		_destRect.x = (int)((_windowSurface->w - _destRect.w) / 2.f);  // Hedef dikd�rtgenin x konumu
		_destRect.y = 0;  // Hedef dikd�rtgenin y konumu

		float f = 1.f + (screenAspect - gridAspect) / gridAspect;  // Ekran ve grid en boy oranlar� aras�ndaki fark
		_projectionMatrix = glm::ortho(-gwh * f, gwh * f, -ghh, ghh);  // Projeksiyon matrisi ayarlan�yor
	}
	else {
		_scale = (float)_windowSurface->w / _grid->width;  // �l�ek fakt�r� (_grid nesnesine g�re hesaplan�r)
		_destRect.w = _windowSurface->w;  // Hedef dikd�rtgenin geni�li�i
		_destRect.h = (int)(_grid->height * _scale);  // Hedef dikd�rtgenin y�ksekli�i
		_destRect.x = 0;  // Hedef dikd�rtgenin x konumu
		_destRect.y = (int)((_windowSurface->h - _destRect.h) / 2.f);  // Hedef dikd�rtgenin y konumu

		float f = 1.f + (gridAspect - screenAspect) / screenAspect;  // Grid ve ekran en boy oranlar� aras�ndaki fark
		_projectionMatrix = glm::ortho(-gwh, gwh, -ghh * f, ghh * f);  // Projeksiyon matrisi ayarlan�yor
	}
	glViewport(0, 0, _windowSurface->w, _windowSurface->h);  // OpenGL g�r�n�m portu ayarlan�yor
}


void initGLObjects() {
	glGenVertexArrays(1, &_vao);  // Vertex dizileri olu�turuluyor
	glBindVertexArray(_vao);  // Vertex dizisi ba�lan�yor

	glGenBuffers(1, &_vbo);  // Vertex buffer olu�turuluyor
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);  // Vertex buffer ba�lan�yor
	glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(GLfloat), quadVertices.data(), GL_STATIC_DRAW);  // Vertex verileri y�kleniyor

	glGenBuffers(1, &_ibo);  // �ndeks buffer olu�turuluyor
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);  // �ndeks buffer ba�lan�yor
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadIndices.size() * sizeof(GLuint), quadIndices.data(), GL_STATIC_DRAW);  // �ndeks verileri y�kleniyor

	_gridShaderParams.vbo = _vbo;  // Grid i�in shader parametreleri ayarlan�yor
	_gridShaderParams.ibo = _ibo;

	_spriteParams.vbo = _vbo;  // Sprite i�in shader parametreleri ayarlan�yor
	_spriteParams.ibo = _ibo;

	_modelMatrix = glm::mat4(1);  // Model matrisi ba�lang�� de�eri atan�yor
	glm::vec3 eye = glm::vec3(0.f, 0.f, 1.f);  // G�z konumu
	glm::vec3 center = glm::vec3(0.f, 0.f, 0.f);  // Hedef noktas�
	glm::vec3 up = glm::vec3(0.f, 1.f, 0.f);  // Yukar� vekt�r�
	_viewMatrix = glm::lookAt(eye, center, up);  // G�r�� matrisi hesaplan�yor
}

void cleanUp() {
	int res;
	SDL_WaitThread(_updateThread, &res);  // _updateThread'in tamamlanmas�n� bekler ve sonucunu al�r
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

	SDL_FreeSurface(_windowSurface);  // _windowSurface y�zeyini bellekten siler
	_windowSurface = NULL;

	SDL_DestroyWindow(_window);  // _window penceresini yok eder
	_window = NULL;

	SDL_Quit();  // SDL'yi kapat�r
}

void toggleFullscreen() {
	_isFullscreen = !_isFullscreen;  // Tam ekran durumunu tersine �evirir

	int res;
	if (_isFullscreen) {
		res = SDL_GetWindowDisplayMode(_window, &_windowMode);  // Mevcut pencere ekran modunu al�r
		res += SDL_SetWindowDisplayMode(_window, &_displayMode);  // Pencere ekran modunu ayarlar
		res += SDL_SetWindowFullscreen(_window, _fullscreenFlag);  // Pencereyi tam ekrana ayarlar
		if (res != 0) {
			std::cout << "Error switching to fullscreen: " << SDL_GetError() << std::endl;  // Hata durumunda hata mesaj�n� yazd�r�r
		}
	}
	else {
		res = SDL_SetWindowFullscreen(_window, 0);  // Pencereyi tam ekrandan ��kar�r
		res = SDL_SetWindowDisplayMode(_window, &_windowMode);  // Pencereyi orijinal ekran moda geri d�nd�r�r
		if (res != 0) {
			std::cout << "Error exiting fullscreen: " << SDL_GetError() << std::endl;  // Hata durumunda hata mesaj�n� yazd�r�r
		}
	}

	resize();  // Yeniden boyutland�rma i�lemini �a��r�r
}

static int updateLoop(void* p) {
	Uint32 lastTick = SDL_GetTicks();  // Son zaman damgas�n� kaydeder

	while (!_quit) {
		Uint32 now = SDL_GetTicks();  // Mevcut zaman damgas�n� al�r

		Uint32 dt = now - lastTick;  // Ge�en s�reyi hesaplar
		lastTick = now;

		if (!_pause)  // E�er duraklatma aktif de�ilse
		{
			_grid->tick(dt);  // Grid nesnesini g�nceller
			SDL_LockMutex(_gridLock);  // Grid'i kilitleyerek g�ncelleme i�lemini yapar
			_grid->update(&_quit);  // Grid'i g�nceller
			SDL_UnlockMutex(_gridLock);  // Grid'i kilitten ��kar�r
		}
	}

	return 0;
}

float clamp(float x, float min, float max) {
	return MAX(min, MIN(max, x));  // De�erin belirli bir aral�kta olmas�n� sa�lar
}

int clampi(int x, int min, int max) {
	return MAX(min, MIN(max, x));  // Tamsay� de�erinin belirli bir aral�kta olmas�n� sa�lar
}

float clamp01(float x) {
	return clamp(x, 0, 1);  // De�erin 0 ile 1 aras�nda olmas�n� sa�lar
}

glm::vec2 mouseUV() {
	float x = (float)_mouseInfo.x - _destRect.x;  // Fare konumunu pencerenin i�indeki koordinata d�n��t�r�r
	float y = (float)_mouseInfo.y - _destRect.y;
	float u = clamp01(x / _destRect.w);  // Koordinat� 0 ile 1 aras�nda bir de�ere d�n��t�r�r
	float v = clamp01(y / _destRect.h);
	return glm::vec2(u, v);  // Fare koordinatlar�n� d�nd�r�r
}

void handleMouse() {
	if (_mouseInfo.mouseButtonDown) {  // Fare d��mesi bas�l�ysa
		if (_mouseInfo.x >= _destRect.x && _mouseInfo.x < (_destRect.x + _destRect.w) &&
			_mouseInfo.y >= _destRect.y && _mouseInfo.y < (_destRect.y + _destRect.h)) {
			int x = _mouseInfo.x - _destRect.x;  // Fare konumunu pencere i�indeki koordinatlara d�n��t�r�r
			int y = _mouseInfo.y - _destRect.y;
			int gx = (int)(x / _scale);  // Grid koordinatlar�na d�n��t�r�r
			int gy = (int)(y / _scale);

			_grid->drawCircle(gx, gy, _radius, _currentMaterial);  // Grid �zerinde daire �izer
		}
	}
}


void draw() {
	int res = SDL_TryLockMutex(_gridLock);  // Grid'i kilitleyerek g�ncelleme i�lemlerini durdurur
	if (res != 0) {
		return;
	}

	glClearColor(0.f, 0.f, 0.f, 1.f);  // Arka plan rengini ayarlar
	glClear(GL_COLOR_BUFFER_BIT);  // Renk tamponunu temizler

	Uint32 now = SDL_GetTicks();  // Mevcut zaman damgas�n� al�r

	void* data = _grid->getData();  // Grid verilerini al�r
	SDL_UnlockMutex(_gridLock);  // Grid kilidini a�ar

	_gridMaterial->putData(data);  // Grid malzemesine verileri aktar�r

	_gridShaderParams.modelMatrix = glm::scale(_modelMatrix, glm::vec3(_grid->width, _grid->height, 1.0));  // Model matrisini �l�eklendirir
	_gridShaderParams.viewMatrix = _viewMatrix;  // G�r�n�m matrisini ayarlar
	_gridShaderParams.projectionMatrix = _projectionMatrix;  // Yans�tma matrisini ayarlar
	_gridShaderParams.gridWidth = (GLfloat)_grid->width;  // Grid geni�li�ini ayarlar
	_gridShaderParams.gridHeight = (GLfloat)_grid->height;  // Grid y�ksekli�ini ayarlar
	_gridShaderParams.time = (GLfloat)now;  // Ge�en s�reyi ayarlar
	_gridShaderParams.shadingOff = (GLuint)_shadingOff;  // G�lgeleme durumunu ayarlar
	_gridShaderParams.mouseUV = mouseUV();  // Fare koordinatlar�n� ayarlar
	_gridMaterial->draw(&_gridShaderParams);  // Grid malzemesini �izer

	_spriteParams.modelMatrix = glm::translate(glm::scale(_modelMatrix, glm::vec3(playerDim, playerDim, playerDim)), _playerPos / playerDim);  // Oyuncu matrisini �l�eklendirir ve konumland�r�r
	_spriteParams.viewMatrix = _viewMatrix;  // G�r�n�m matrisini ayarlar
	_spriteParams.projectionMatrix = _projectionMatrix;  // Yans�tma matrisini ayarlar
	_playerMaterial->draw(&_spriteParams);  // Oyuncu malzemesini �izer

	glViewport(0, 0, _windowSurface->w, _windowSurface->h);  // G�r�n�m alan�n� ayarlar

	SDL_GL_SwapWindow(_window);  // Pencereyi yeniler ve ekran� g�sterir

	GLenum err = glGetError();  // OpenGL hata durumunu kontrol eder
	if (err != 0) {
		std::cout << "OpenGL error: " << err << std::endl;
	}
}

bool loadImage(const char* path) {
	SDL_Surface* srf = SDL_LoadBMP(path);  // Belirtilen dosyadan bir y�zey y�kler
	if (srf == nullptr) {
		std::cout << "Image " << path << " not found." << std::endl;
		return false;
	}

	SDL_LockMutex(_gridLock);  // Grid'i kilitleyerek g�ncelleme i�lemlerini durdurur
	if (_grid != nullptr) {
		delete _grid;  // Mevcut grid nesnesini temizler
	}
	_grid = new SandGrid(srf, defaultUpdate);  // Yeni bir Sandgrid nesnesi olu�turur ve y�klenen y�zeyi kullan�r

	_gridMaterial->initTexture(srf, srf->w, srf->h);  // Grid malzemesinin dokusunu y�klenen y�zeyle ba�lat�r
	SDL_UnlockMutex(_gridLock);  // Grid kilidini a�ar

	int w = MAX(minWinSize, srf->w);  // Pencere geni�li�ini minimum boyuta ayarlar
	int h = MAX(minWinSize, srf->h);  // Pencere y�ksekli�ini minimum boyuta ayarlar

	SDL_SetWindowSize(_window, w, h);  // Pencere boyutunu ayarlar
	SDL_FreeSurface(srf);  // Y�klenen y�zeyi serbest b�rak�r

	resize();  // Yeniden boyutland�rma i�lemini ger�ekle�tirir

	return true;
}
bool init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {  // SDL'yi video ve olaylar i�in ba�lat�r
		std::cout << "Error initializing SDL. Error: " << SDL_GetError() << std::endl;
		return false;
	}

	if (SDL_GetDesktopDisplayMode(0, &_displayMode) != 0) {  // Masa�st� g�r�nt�leme modunu al�r
		std::cout << "Error getting display mode: " << SDL_GetError() << std::endl;
		return false;
	}
	std::cout << "Creating window: " << _displayMode.w << "x" << _displayMode.h << "@" << _displayMode.refresh_rate << std::endl;

	int res;
	res = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);  // OpenGL ba�lam� i�in major versiyonu ayarlar
	res += SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);  // OpenGL ba�lam� i�in minor versiyonu ayarlar
	res += SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // �ift tamponlamay� etkinle�tirir
	if (res != 0) {
		std::cout << "Failed to set OpenGL attributes: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_DisplayMode desktopMode;
	if (SDL_GetDesktopDisplayMode(0, &desktopMode) != 0) {  // Masa�st� modunu al�r
		std::cout << "Error getting desktop mode: " << SDL_GetError() << std::endl;
		return false;
	}

	// Pencere s�slemeleri pencerenin g�r�n�r masa�st� alan�n�n i�inde olacak �ekilde kayd�rma i�lemi
	const int offset = 100;
	Uint32 flags = _fullscreenFlag | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	_window = SDL_CreateWindow("Sandspiel", offset, offset, desktopMode.w, desktopMode.h, flags);  // Pencere olu�turur
	if (_window == NULL) {
		std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
		return false;
	}

	_gridLock = SDL_CreateMutex();  // Grid kilidini olu�turur
	if (_gridLock == NULL) {
		std::cout << "Failed to create mutex: " << SDL_GetError() << std::endl;
	}

	bool ok = initOpenGL();  // OpenGL'yi ba�lat�r
	if (!ok) {
		std::cout << "OpenGL initialization failed" << std::endl;
		return false;
	}

	_gridMaterial = new GridMaterial();  // Grid malzemesini olu�turur
	if (!loadImage(startImagePath)) {  // Ba�lang�� resmini y�kler
		std::cout << "No image found, starting with default grid" << std::endl;
		_grid = new SandGrid(initGridWidth, initGridHeight, defaultUpdate);  // Varsay�lan grid'i olu�turur

		_gridMaterial->initTexture(NULL, _grid->width, _grid->height);  // Grid malzemesinin dokusunu ba�lang�� grid'iyle ba�lat�r
		resize();  // Pencere boyutunu ayarlar
	}

	SDL_Surface* player = SDL_LoadBMP(playerPath);  // Oyuncu y�zeyini y�kler
	_playerMaterial = new SpriteMaterial(player);  // Oyuncu malzemesini olu�turur
	SDL_FreeSurface(player);

	initGLObjects();  // OpenGL nesnelerini ba�lat�r

	return true;
}
int main(int argc, char* argv[]) {

	if (!init()) {  // Ba�latma i�lemini kontrol eder
		return 1;
	}

	_updateThread = SDL_CreateThread(updateLoop, "update", (void*)NULL);  // G�ncelleme d�ng�s� i� par�ac���n� ba�lat�r
	if (_updateThread == NULL) {
		std::cout << "Failed to start thread" << std::endl;
		return 1;
	}

	while (!_quit) {

		while (SDL_PollEvent(&_event)) {  // Olaylar� i�ler
			switch (_event.type) {
			case SDL_QUIT:  // ��k�� olay�
				_quit = true;
				break;

			case SDL_WINDOWEVENT:  // Pencere olaylar�
				if (_event.window.event == SDL_WINDOWEVENT_RESIZED)
					resize();  // Pencere boyutunu yeniden ayarlar
				break;

			case SDL_KEYUP:  // Tu� b�rakma olay�
			{
				SDL_Keycode key = _event.key.keysym.sym;  // B�rak�lan tu�un kodunu al�r

				switch (key) {
				case SDLK_f:  // F tu�u: Tam ekran� a��p kapat�r
					toggleFullscreen();
					break;

				case SDLK_ESCAPE:  // Escape tu�u: ��k�� yapar
					_quit = true;
					break;

				case SDLK_SPACE:  // Bo�luk tu�u: Duraklatmay� a��p kapat�r
					_pause = !_pause;
					break;

				case SDLK_0:  // 0 tu�u: Malzemeyi "Nothing" olarak ayarlar
					_currentMaterial = Material::Nothing;
					break;

				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:  // 1-5 tu�lar�: Malzemeyi ilgili indekse g�re ayarlar
					_currentMaterial = materialUIIndex[(int)key - SDLK_0];
					break;

				case SDLK_DELETE:
				case SDLK_BACKSPACE:  // DELETE veya BACKSPACE tu�lar�: Grid'i temizler
					_grid->clear();
					break;

				case SDLK_r:  // R tu�u: Ba�lang�� resmini y�kler
					loadImage(startImagePath);
					break;
				}
				break;
			}

			case SDL_MOUSEMOTION:  // Fare hareketi olay�
				_mouseInfo.x = _event.motion.x;  // Fare konumunu g�nceller
				_mouseInfo.y = _event.motion.y;
				break;

			case SDL_MOUSEBUTTONDOWN:  // Fare d��mesi basma olay�
				if (_event.button.button == SDL_BUTTON_LEFT) {
					_mouseInfo.mouseButtonDown = true;  // Sol d��me bas�ld���nda bilgisini g�nceller
				}
				break;

			case SDL_MOUSEBUTTONUP:  // Fare d��mesi b�rakma olay�
				if (_event.button.button == SDL_BUTTON_LEFT) {
					_mouseInfo.mouseButtonDown = false;  // Sol d��me b�rak�ld���nda bilgisini g�nceller
				}
				break;

			case SDL_MOUSEWHEEL:  // Fare tekerle�i olay�
				_radius = clamp(_radius + _event.wheel.y, minRadius, maxRadius);  // Fare tekerle�i hareketine g�re yar��ap� g�nceller
				break;
			}
		}

		handleMouse();  // Fare i�leme i�levini �a��r�r

		draw();  // �izim i�lemini ger�ekle�tirir

	}
	std::cout << "Exit requested" << std::endl;

	cleanUp();  // Temizlik i�lemini ger�ekle�tirir
	return 0;
}
