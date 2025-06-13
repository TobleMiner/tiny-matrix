#include "game_of_life.h"

#include <stdio.h>
#include <string.h>

void game_of_life_init(game_of_life_t *game, unsigned int width, unsigned int height, uint8_t *playfield_a, uint8_t *playfield_b) {
	game->width = width;
	game->height = height;
	game->playfield_a = playfield_a;
	game->playfield_b = playfield_b;
}

unsigned int game_of_life_count_alive_cells_in_area(game_of_life_t *game, unsigned int x_start, unsigned int y_start, unsigned int x_end, unsigned int y_end) {
	unsigned int num_alive_cells = 0;

	for (unsigned int y = y_start; y < y_end; y++) {
		for (unsigned int x = x_start; x < x_end; x++) {
			if (game_of_life_get_cell(game, x, y)) {
				num_alive_cells++;
			}
		}
	}

	return num_alive_cells;
}

static unsigned int game_of_life_count_adjacent_alive_cells(game_of_life_t *game, unsigned int x, unsigned int y) {
	unsigned int num_alive_cells = 0;

	for (int dy = -1; dy <= 1; dy++) {
		int check_pos_y = (int)y + dy;
		if (check_pos_y < 0) {
			check_pos_y += game->height;
		} else if (check_pos_y >= game->height) {
			check_pos_y -= game->height;
		}
		for (int dx = -1; dx <= 1; dx++) {
			if (dy == 0 && dx == 0) {
				continue;
			}

			int check_pos_x = (int)x + dx;
			if (check_pos_x < 0) {
				check_pos_x += game->width;
			} else if (check_pos_x >= game->width) {
				check_pos_x -= game->width;
			}

			if (game_of_life_get_cell_b(game, check_pos_x, check_pos_y)) {
				num_alive_cells++;
			}
		}
	}

//	printf("Alive neighbors of (%u, %u): %u", x, y, num_alive_cells);

	return num_alive_cells;
}

unsigned int game_of_life_step_row(game_of_life_t *game, unsigned int row) {
	if (row == 0) {
		memcpy(game->playfield_b, game->playfield_a, game->width * game->height / 8);
	}
	unsigned int y = row;
	for (unsigned int x = 0; x < game->width; x++) {
		unsigned int adjacent_alive_cells = game_of_life_count_adjacent_alive_cells(game, x, y);
		if (game_of_life_get_cell(game, x, y)) {
			if (adjacent_alive_cells != 2 && adjacent_alive_cells != 3) {
				game_of_life_set_cell(game, x, y, false);
			}
		} else {
			if (adjacent_alive_cells == 3) {
				game_of_life_set_cell(game, x, y, true);
			}
		}
	}
	row += 1;
	if (row >= game->height) {
		row = 0;
	}
	return row;
}

void game_of_life_step(game_of_life_t *game) {
	memcpy(game->playfield_b, game->playfield_a, game->width * game->height / 8);
	for (unsigned int y = 0; y < game->height; y++) {
		for (unsigned int x = 0; x < game->width; x++) {
			unsigned int adjacent_alive_cells = game_of_life_count_adjacent_alive_cells(game, x, y);
			if (game_of_life_get_cell(game, x, y)) {
				if (adjacent_alive_cells != 2 && adjacent_alive_cells != 3) {
//					printf(" DEATH");
					game_of_life_set_cell(game, x, y, false);
				}
			} else {
				if (adjacent_alive_cells == 3) {
//					printf(" BIRTH");
					game_of_life_set_cell(game, x, y, true);
				}
			}
//			printf("\n");
		}
	}
}
