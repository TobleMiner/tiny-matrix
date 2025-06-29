#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GAME_OF_LIFE_VEC32_HEIGHT	64
#define GAME_OF_LIFE_VEC32_WIDTH32	2

typedef uint32_t game_of_life_vec32_playfield_t[GAME_OF_LIFE_VEC32_WIDTH32 * GAME_OF_LIFE_VEC32_HEIGHT];

typedef struct game_of_life_vec32 {
	game_of_life_vec32_playfield_t playfield_a_;
	game_of_life_vec32_playfield_t playfield_b_;
	uint32_t *playfield_a;
	uint32_t *playfield_b;
} game_of_life_vec32_t;

void game_of_life_vec32_init(game_of_life_vec32_t *game);
void game_of_life_vec32_step(game_of_life_vec32_t *game);
void game_of_life_vec32_set_cell(game_of_life_vec32_t *game, unsigned int x, unsigned int y, bool state);
bool game_of_life_vec32_get_cell(game_of_life_vec32_t *game, unsigned int x, unsigned int y);
