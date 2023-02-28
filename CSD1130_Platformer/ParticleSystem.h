#pragma once
#include "AEEngine.h"

struct Particle {
};

struct ParticleSystem {
	Particle* particles;
};

f32 PRNG(int a, int c, int m, int x0);
Particle Generator();