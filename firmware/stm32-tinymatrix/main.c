#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define M_PI 3.14159265358979323846264338328


#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "game_of_life.h"
#include "rng.h"

#define ARRAY_SIZE(arr_) (sizeof((arr_)) / sizeof(*(arr_)))
#define ABS(x_) ((x_) > 0 ? (x_) : -(x_))

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

static volatile uint8_t fb_g[8 * 8] = { 0 };

#define FIELD_COMPRESSION 1

/* Cycle through all LEDs, one at a time */
//#define LED_TEST
//#define SINE_FIELD
#define GAME_OF_LIFE

#define IRQ_DRIVEN_DISPLAY

#ifdef GAME_OF_LIFE
static uint8_t game_of_life_playfield_a[64 * 64 / 8] = { 0 };
static uint8_t game_of_life_playfield_b[64 * 64 / 8] = { 0 };
static game_of_life_t game_of_life;
static uint8_t population_history_idx = 0;
static uint16_t population_history[32] = { 0 };
static uint16_t population_history_downsampler = 0;
static uint16_t population_randomization_cnt = 0;
#endif

#ifdef SINE_FIELD
#define FIELD_SIZE 16
uint8_t sine_field_g[FIELD_SIZE * FIELD_SIZE];
#endif

volatile bool display_done = false;

static void clock_init(void) {
	rcc_clock_setup_in_hsi_out_48mhz();
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_TIM1);
	nvic_enable_irq(NVIC_TIM1_BRK_UP_TRG_COM_IRQ);
	timer_enable_irq(TIM1, TIM_DIER_UIE);
	timer_direction_down(TIM1);
	timer_set_prescaler(TIM1, rcc_apb1_frequency / 1000000UL);
	TIM1_CNT = 8;
	timer_enable_counter(TIM1);
}

#define TIM14 TIM14_BASE

static void place_acorn(unsigned int x, unsigned int y) {
	game_of_life_set_cell(&game_of_life, x + 1, y + 0, true);
	game_of_life_set_cell(&game_of_life, x + 3, y + 1, true);
	game_of_life_set_cell(&game_of_life, x + 0, y + 2, true);
	game_of_life_set_cell(&game_of_life, x + 1, y + 2, true);
	game_of_life_set_cell(&game_of_life, x + 4, y + 2, true);
	game_of_life_set_cell(&game_of_life, x + 5, y + 2, true);
	game_of_life_set_cell(&game_of_life, x + 6, y + 2, true);
}

int main(void) {
	int off_x = 0, off_y = 0, cycles = 0;
	int led_count = 0;

	clock_init();
	rng_init(0x42);

#ifdef GAME_OF_LIFE
/*
	game_of_life_init(&game_of_life, 8, 8, game_of_life_playfield_a, game_of_life_playfield_b);
	game_of_life_set_cell(&game_of_life, 0, 0, true);
	game_of_life_set_cell(&game_of_life, 1, 0, true);
	game_of_life_set_cell(&game_of_life, 2, 0, true);
	game_of_life_set_cell(&game_of_life, 2, 1, true);
	game_of_life_set_cell(&game_of_life, 1, 2, true);
*/
	game_of_life_init(&game_of_life, 64, 64, game_of_life_playfield_a, game_of_life_playfield_b);
	place_acorn(29, 31);
#endif

#ifdef SINE_FIELD
	/* Generate sine field */
	for (int y = 0; y < FIELD_SIZE; y++) {
		for (int x = 0; x < FIELD_SIZE; x++) {
			float sinx = sinf(x * M_PI * FIELD_COMPRESSION / (FIELD_SIZE - 1));
			float siny = sinf(y * M_PI * FIELD_COMPRESSION / (FIELD_SIZE - 1));
			float prod = sinx * siny;

			sine_field_g[y * FIELD_SIZE + x] = gamma8[(uint8_t)roundf(fabs(prod * 255))];
		}
	}
#endif

	unsigned int num_sim_steps = 0;
	unsigned int game_row = 0;
	while(1) {
		uint8_t x, y, bit;

#ifdef LED_TEST
		if (display_done) {
			fb_g[led_count] = 0;
			led_count++;
			led_count %= 64;
			fb_g[led_count] = 255;
			display_done = false;
		}
#endif
		if (!cycles) {
#ifdef SINE_FIELD
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					int x_wrapped = (off_x + x) % FIELD_SIZE;
					int y_wrapped = (off_y + y) % FIELD_SIZE;
					fb_g[y * 8 + x] = sine_field_g[y_wrapped * FIELD_SIZE + x_wrapped];
				}
			}
			off_x++;
			off_y++;
			off_x %= FIELD_SIZE;
			off_y %= FIELD_SIZE;
#endif
			cycles++;
		}

#ifdef GAME_OF_LIFE
		game_of_life_step(&game_of_life);
		unsigned int total_alive_cells = 0;
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				unsigned int num_alive_cells = game_of_life_count_alive_cells_in_area(&game_of_life, x * 8, y * 8, x * 8 + 8, y * 8 + 8);
				total_alive_cells += num_alive_cells;
				if (num_alive_cells) {
					fb_g[y * 8 + x] = num_alive_cells * 4 - 1;
				} else {
					fb_g[y * 8 + x] = 0;
				}
			}
		}

		population_history_downsampler++;
		if (population_history_downsampler >= 4) {
			population_history_downsampler = 0;
			population_history[population_history_idx++] = total_alive_cells;

			if (population_history_idx == ARRAY_SIZE(population_history)) {
				population_history_idx = 0;

				unsigned int population_delta = 0;
				for (unsigned int i = 0; i < ARRAY_SIZE(population_history) - 1; i++) {
					population_delta += ABS((int)population_history[i] - (int)population_history[i + 1]);
				}

				if (population_delta < game_of_life.width * game_of_life.height / 10) {
					uint32_t pos_x = rng_u32() % (game_of_life.width - 10);
					uint32_t pos_y = rng_u32() % (game_of_life.height - 5);
					place_acorn(pos_x, pos_y);
				}
			}
		}

		population_randomization_cnt++;
		if (population_randomization_cnt >= 60UL * 300) {
			population_randomization_cnt = 0;
			uint32_t pos_x = rng_u32() % (game_of_life.width - 10);
			uint32_t pos_y = rng_u32() % (game_of_life.height - 5);
			place_acorn(pos_x, pos_y);
		}
/*
		game_row = game_of_life_step_row(&game_of_life, game_row);
		if (!game_row) {
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					unsigned int num_alive_cells = game_of_life_count_alive_cells_in_area(&game_of_life, x * 8, y * 8, x * 8 + 8, y * 8 + 8);
					if (num_alive_cells) {
						fb_g[y * 8 + x] = num_alive_cells * 4 - 1;
					} else {
						fb_g[y * 8 + x] = 0;
					}
				}
			}

			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					if (game_of_life_get_cell(&game_of_life, x, y)) {
						fb_g[y * 8 + x] = 255;
					} else {
						fb_g[y * 8 + x] = 0;
					}
				}
			}
		}
*/
#endif

#ifndef IRQ_DRIVEN_DISPLAY
		/* 8 loop cycles for 8 bits, exponentially increasing duration */
		for (bit = 0; bit < 8; bit++) {
			/* display is multiplexed over rows */
			for (y = 0; y < 8; y++) {
				unsigned int i;
				uint16_t row = 0;

				for (x = 0; x < 8; x++) {
					/* Set output bit if current brightness bit is set, too */
					if (fb_g[y * 8 + x] & (1 << bit)) {
						row |= (1 << x);
					}
				}
				/* Anode pin mapped to cathode of this row is on PA9 */
				if (row & (1 << y)) {
					row |= 1 << 9;
				}
				row &= ~(1 << y);
				/* Set all outputs to zero to prevent ghosting */
				gpio_port_write(GPIOA, 0);
				/* Setup hi-z pins first */
				gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, ~(row | (1 << y)) & 0x02ff);
				/* Setup outputs */
				gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, row | (1 << y));
				/* Set output values */
				gpio_port_write(GPIOA, row);

				for (i = 0; i < (16U << bit); i++) __asm("nop;");
			}
		}
#endif
/*
		if (display_done) {
			display_done = false;
			cycles++;
		}
*/
#ifdef LED_TEST
		cycles %= 30;
#else
		cycles %= 11;
#endif
	}

	return 0;
}

#ifdef IRQ_DRIVEN_DISPLAY
static volatile uint8_t display_bit = 0;
uint8_t volatile display_y = 0;

void tim1_brk_up_trg_com_isr() {
	timer_disable_counter(TIM1);

	uint16_t row = 0;
	for (uint8_t x = 0; x < 8; x++) {
		/* Set output bit if current brightness bit is set, too */
		if (fb_g[display_y * 8 + x] & (1 << display_bit)) {
			row |= (1 << x);
		}
	}
	/* Anode pin mapped to cathode of this row is on PA9 */
	if (row & (1 << display_y)) {
		row |= 1 << 9;
	}
	row &= ~(1 << display_y);
	/* Set all outputs to zero to prevent ghosting */
	gpio_port_write(GPIOA, 0);
	/* Setup hi-z pins first */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, ~(row | (1 << display_y)) & 0x02ff);
	/* Setup outputs */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, row | (1 << display_y));
	/* Set output values */
	gpio_port_write(GPIOA, row);

	TIM1_CNT = 8U << display_bit;
	timer_clear_flag(TIM1, TIM_SR_UIF);
	timer_enable_counter(TIM1);

	display_y++;
	if (display_y >= 8) {
		display_y = 0;
		display_bit++;
		if (display_bit >= 8) {
			display_bit = 0;
			display_done = true;
		}
	}
}
#endif
