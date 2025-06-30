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

#include "game_of_life_vec32.h"
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
static game_of_life_vec32_t game_of_life;
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

const struct rcc_clock_scale rcc_config_8MHz = { /* 8MHz PLL from HSI */
	.pll_source = RCC_CFGR_PLLSRC_HSI16_CLK,
	.pll_mul = RCC_CFGR_PLLMUL_MUL6,
	.pll_div = RCC_CFGR_PLLDIV_DIV3,
	.hpre = RCC_CFGR_HPRE_NODIV,
	.ppre1 = RCC_CFGR_PPRE_NODIV,
	.ppre2 = RCC_CFGR_PPRE_NODIV,
	.voltage_scale = PWR_SCALE1,
	.flash_waitstates = 0,
	.ahb_frequency  = 8000000,
	.apb1_frequency = 8000000,
	.apb2_frequency = 8000000,
};

static void clock_init(void) {
	//rcc_clock_setup_in_hsi_out_48mhz();
/*
	rcc_set_hpre(RCC_CFGR_HPRE_DIV2);
	rcc_apb1_frequency /= 2;
	rcc_ahb_frequency /= 2;
*/
	RCC_CR |= RCC_CR_HSI16DIVEN;
	rcc_clock_setup_pll(&rcc_config_8MHz);
//	RCC_ICSCR = (RCC_ICSCR & ~(RCC_ICSCR_MSIRANGE_MASK << RCC_ICSCR_MSIRANGE_MASK)) | (RCC_ICSCR_MSIRANGE_4MHZ << RCC_ICSCR_MSIRANGE_MASK);
//	rcc_ahb_frequency  = 4000000;
//	rcc_apb1_frequency = 4000000;
//	rcc_apb2_frequency = 4000000;
	rcc_periph_clock_enable(RCC_GPIOA);
#ifdef IRQ_DRIVEN_DISPLAY
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	timer_enable_irq(TIM2, TIM_DIER_UIE);
	timer_direction_down(TIM2);
	timer_set_prescaler(TIM2, rcc_apb1_frequency / 1000000UL);
	TIM2_CNT = 8;
	timer_enable_counter(TIM2);
#endif
}

#ifdef GAME_OF_LIFE
static void place_acorn(unsigned int x, unsigned int y) {
	game_of_life_vec32_set_cell(&game_of_life, x + 1, y + 0, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 3, y + 1, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 0, y + 2, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 1, y + 2, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 4, y + 2, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 5, y + 2, true);
	game_of_life_vec32_set_cell(&game_of_life, x + 6, y + 2, true);
}
#endif

int main(void) {
	int off_x = 0, off_y = 0, cycles = 0;
	int led_count = 0;

	clock_init();
	rng_init(0x42);

#ifdef GAME_OF_LIFE
	game_of_life_vec32_init(&game_of_life);
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
		game_of_life_vec32_step(&game_of_life);
		unsigned int total_alive_cells = 0;
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				uint8_t num_alive_cells = game_of_life_vec32_count_alive_cells_in_8x8_aligned_area(&game_of_life, x * 8, y * 8);
				total_alive_cells += num_alive_cells;
				if (num_alive_cells) {
					fb_g[y * 8 + x] = num_alive_cells - 1;
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

				if (population_delta < GAME_OF_LIFE_VEC32_WIDTH * GAME_OF_LIFE_VEC32_HEIGHT / 10) {
					uint32_t pos_x = rng_u32() % (GAME_OF_LIFE_VEC32_WIDTH - 10);
					uint32_t pos_y = rng_u32() % (GAME_OF_LIFE_VEC32_HEIGHT - 5);
					place_acorn(pos_x, pos_y);
				}
			}
		}

		population_randomization_cnt++;
		if (population_randomization_cnt >= 60UL * 300) {
			population_randomization_cnt = 0;
			uint32_t pos_x = rng_u32() % (GAME_OF_LIFE_VEC32_WIDTH - 10);
			uint32_t pos_y = rng_u32() % (GAME_OF_LIFE_VEC32_HEIGHT - 5);
			place_acorn(pos_x, pos_y);
		}
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
		display_done = true;
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

void tim2_isr() {
	timer_disable_counter(TIM2);

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

	TIM2_CNT = 24U << display_bit;
	TIM2_SR = 0;
//	timer_clear_flag(TIM2, TIM_SR_UIF);
	timer_enable_counter(TIM2);

	display_y++;
	if (display_y >= 8) {
		display_y = 0;
		display_bit++;
		if (display_bit >= 6) {
			display_bit = 0;
			display_done = true;
		}
	}
}
#endif
