#include "ParticleSystem.h"

f32 PRNG(int a, int c, int m, int x0)
{
	return ((a * x0) + c) % m;
}
