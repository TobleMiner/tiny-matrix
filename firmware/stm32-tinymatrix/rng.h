#pragma once

#include <stdint.h>

void rng_init(uint32_t seed);
uint32_t rng_u32(void);
