#include "game_of_life_vec32.h"

#include <string.h>

void game_of_life_vec32_init(game_of_life_vec32_t *game) {
	memset(game->playfield_a_, 0, sizeof(game->playfield_a_));
	memset(game->playfield_b_, 0, sizeof(game->playfield_b_));
	game->playfield_a = game->playfield_a_;
	game->playfield_b = game->playfield_b_;
}

#define ROW(arr_, x32_, y_) ((arr_)[(y_) * GAME_OF_LIFE_VEC32_WIDTH32 + (x32_)])
#define ROW1(arr_, y_) ROW(arr_, 0, y_)
#define ROW2(arr_, y_) ROW(arr_, 1, y_)

void game_of_life_vec32_step(game_of_life_vec32_t *game) {
	uint32_t row_above[GAME_OF_LIFE_VEC32_WIDTH32], row[GAME_OF_LIFE_VEC32_WIDTH32], row_below[GAME_OF_LIFE_VEC32_WIDTH32];
	const uint32_t *playfield_src = game->playfield_a;
	uint32_t *playfield_dst = game->playfield_b;

	for (unsigned int x32 = 0; x32 < GAME_OF_LIFE_VEC32_WIDTH32; x32++) {
		row_above[x32] = ROW(playfield_src, x32, GAME_OF_LIFE_VEC32_HEIGHT - 1);
		row[x32] = ROW(playfield_src, x32, 0);
	}
	for (uint8_t y = 0; y < GAME_OF_LIFE_VEC32_HEIGHT; y++) {
		for (unsigned int x32 = 0; x32 < GAME_OF_LIFE_VEC32_WIDTH32; x32++) {
			row_below[x32] = ROW(playfield_src, x32, (y + 1) % GAME_OF_LIFE_VEC32_HEIGHT);
		}

		uint8_t prev_x32 = 1;
		for (uint8_t x32 = 0; x32 < GAME_OF_LIFE_VEC32_WIDTH32; x32++) {
			uint32_t row_dst = 0;
			uint8_t next_x32 = (x32 + 1) % GAME_OF_LIFE_VEC32_WIDTH32;

			// Handle first entry as a special case. Needs to wrap around into prev u32
			uint32_t neighbour_bits_col0 =
				/* column width - 1 */
				(row_above[prev_x32]	& (1UL << 31)) >> 0 |
				(row[prev_x32]		& (1UL << 31)) >> 1 |
				(row_below[prev_x32]	& (1UL << 31)) >> 2 |
				/* column 0 & 1 */
				(row_above[x32]		& (3UL <<  0)) << 0 |
				(row[x32]		& (2UL <<  0)) << 2 |
				(row_below[x32]		& (3UL <<  0)) << 4;

			uint8_t alive_cells_col0 = __builtin_popcount(neighbour_bits_col0);

			bool is_col0_alive = !!(row[x32] & (1UL << 0));
			if (is_col0_alive && (alive_cells_col0 == 2 || alive_cells_col0 == 3)) {
				row_dst |= (1UL << 0);
			} else if (!is_col0_alive && alive_cells_col0 == 3) {
				row_dst |= (1UL << 0);
			}

			// Bits with neighbours fully within first u32 can all be processed the same
			for (uint8_t x = 1; x < 15; x++) {
				uint32_t neighbour_bits =
					(row_above[x32]	& (7UL << (x - 1))) << 0 |
					(row[x32]	& (5UL << (x - 1))) << 3 |
					(row_below[x32]	& (7UL << (x - 1))) << 6;

				uint8_t alive_cells = __builtin_popcount(neighbour_bits);

				bool is_alive = !!(row[x32] & (1UL << x));
				if (is_alive && (alive_cells == 2 || alive_cells == 3)) {
					row_dst |= (1UL << x);
				} else if (!is_alive && alive_cells == 3) {
					row_dst |= (1UL << x);
				}
			}

			// TODO: Wrap into loop above once safe way to make GCC generate rotate instruction found
			for (uint8_t x = 15; x < 31; x++) {
				uint32_t neighbour_bits =
					(row_above[x32]	& (7UL << (x - 1))) >> 0 |
					(row[x32]	& (5UL << (x - 1))) >> 3 |
					(row_below[x32]	& (7UL << (x - 1))) >> 6;

				uint8_t alive_cells = __builtin_popcount(neighbour_bits);

				bool is_alive = !!(row[x32] & (1UL << x));
				if (is_alive && (alive_cells == 2 || alive_cells == 3)) {
					row_dst |= (1UL << x);
				} else if (!is_alive && alive_cells == 3) {
					row_dst |= (1UL << x);
				}
			}

			// Handle 31st entry as a special case. Needs to wrap around into next u32
			uint32_t neighbour_bits_col31 =
				/* column 30 & 31 */
				(row_above[x32]		& (3UL << 30)) >> 0 |
				(row[x32]		& (1UL << 30)) >> 2 |
				(row_below[x32]		& (3UL << 30)) >> 4 |
				/* column 0 of next u32 */
				(row_above[next_x32]	& (1UL <<  0)) << 0 |
				(row[next_x32]		& (1UL <<  0)) << 1 |
				(row_below[next_x32]	& (1UL <<  0)) << 2;

			uint8_t alive_cells_col31 = __builtin_popcount(neighbour_bits_col31);

			bool is_col31_alive = !!(row[x32] & (1UL << 31));
			if (is_col31_alive && (alive_cells_col31 == 2 || alive_cells_col31 == 3)) {
				row_dst |= (1UL << 31);
			} else if (!is_col31_alive && alive_cells_col31 == 3) {
				row_dst |= (1UL << 31);
			}

			ROW(playfield_dst, x32, y) = row_dst;

			prev_x32 = x32;
		}

		memcpy(row_above, row, sizeof(row_above));
		memcpy(row, row_below, sizeof(row));
	}

	game->playfield_b = game->playfield_a;
	game->playfield_a = playfield_dst;
}

void game_of_life_vec32_set_cell(game_of_life_vec32_t *game, unsigned int x, unsigned int y, bool state) {
	uint32_t *playfield_dst = game->playfield_a;

	if (state) {
		ROW(playfield_dst, x / 32, y) |= 1UL << (x % 32);
	} else {
		ROW(playfield_dst, x / 32, y) &= ~(1UL << (x % 32));
	}
}

bool game_of_life_vec32_get_cell(game_of_life_vec32_t *game, unsigned int x, unsigned int y) {
	const uint32_t *playfield_src = game->playfield_a;

	return !!(ROW(playfield_src, x / 32, y) & (1UL << (x % 32)));
}
