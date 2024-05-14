#pragma once

#include "Headers.h"

// When changed:
// - Adapt 'colors' array in Sandgrid.cpp
// - Adapt constants 'm_*' in fragment shader
// - Note that the fragment shader uses the material index for z-ordering
// - Note that the shader assumes all materials >= 'Smoke' are gassees
// - adapt 'materialUIIndex' in main.cpp
enum class Material : char {
	Nothing = 0,
	Sand,
	Water,
	Rock,
	Wood,
	Fire,
	Smoke,

	// end marker
	OutOfBounds
};

struct Particle {
	int lastFrame = -1;
	int age = 0;
	Material material = Material::Nothing;
};

const Particle _defaultParticle;

struct Lifetimes {
	int woodFireResistance;
	int fire;
	int gas;
	int rock;
};

class SandGrid {
private:
	Particle** _grid;
	Uint32* _data;
	double frame = 0;
	int framesSkipped = 0;

	int updatesPerSec;
	Lifetimes lifetimes;

	void init();
	
	void swap(int x1, int y1, int x2, int y2);

public:
	int width;
	int height;
	
	SandGrid(int w, int h, int updatesPerSec);
	SandGrid(SDL_Surface* surface, int updatesPerSec);
	~SandGrid();

	void clear();
	void drawGrid(SDL_Texture* tex);
	void* getData();
	SDL_Surface* createSurface();
	void tick(Uint32 dt);
	void update(bool* quit);
	void drawCircle(int cx, int cy, float r, Material m);

	void setUpdateRate(int updatesPerSec);
	int getUpdateRate();
};