#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct game_of_life {
	unsigned int width, height;
	uint8_t *playfield_a;
	uint8_t *playfield_b;
} game_of_life_t;

void game_of_life_init(game_of_life_t *game, unsigned int width, unsigned int height, uint8_t *playfield_a, uint8_t *playfield_b);
void game_of_life_step(game_of_life_t *game);
unsigned int game_of_life_step_row(game_of_life_t *game, unsigned int row);
unsigned int game_of_life_count_alive_cells_in_area(game_of_life_t *game, unsigned int x_start, unsigned int y_start, unsigned int x_end, unsigned int y_end);

static inline bool game_of_life_get_cell(game_of_life_t *game, unsigned int x, unsigned int y) {
	unsigned int bytepos = y * game->width / 8 + x / 8;
	unsigned int bitpos = x % 8;
	return !!(game->playfield_a[bytepos] & (1 << bitpos));
}

static inline bool game_of_life_get_cell_b(game_of_life_t *game, unsigned int x, unsigned int y) {
	unsigned int bytepos = y * game->width / 8 + x / 8;
	unsigned int bitpos = x % 8;
	return !!(game->playfield_b[bytepos] & (1 << bitpos));
}

static inline void game_of_life_set_cell(game_of_life_t *game, unsigned int x, unsigned int y, bool state) {
	unsigned int bytepos = y * game->width / 8 + x / 8;
	unsigned int bitpos = x % 8;
	if (state) {
		game->playfield_a[bytepos] |= 1 << bitpos;
	} else {
		game->playfield_a[bytepos] &= ~(1 << bitpos);
	}
}
