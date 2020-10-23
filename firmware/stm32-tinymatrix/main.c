#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define M_PI 3.14159265358979323846264338328


#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

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

uint8_t fb_g[8 * 8] = { 0 };

#define FIELD_SIZE 16
uint8_t sine_field_g[FIELD_SIZE * FIELD_SIZE];

#define FIELD_COMPRESSION 1

/* Cycle through all LEDs, one at a time */
//#define LED_TEST

static void clock_init(void) {
	rcc_clock_setup_in_hsi_out_48mhz();
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
}

int main(void) {
	int off_x = 0, off_y = 0, cycles = 0;
	int led_count = 0;

	clock_init();

	/* Generate sine field */
	for (int y = 0; y < FIELD_SIZE; y++) {
		for (int x = 0; x < FIELD_SIZE; x++) {
			float sinx = sinf(x * M_PI * FIELD_COMPRESSION / (FIELD_SIZE - 1));
			float siny = sinf(y * M_PI * FIELD_COMPRESSION / (FIELD_SIZE - 1));
			float prod = sinx * siny;

			sine_field_g[y * FIELD_SIZE + x] = gamma8[(uint8_t)roundf(fabs(prod * 255))];
		}
	}

	while(1) {
		uint8_t x, y, bit;

		if (!cycles) {
#ifdef LED_TEST
			fb_g[led_count] = 0;
			led_count++;
			led_count %= 64;
			fb_g[led_count] = 255;
#else
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
		}

		/* 8 loop cycles for 8 bits, exponentially increasing duration */
		for (bit = 0; bit < 8; bit++) {
			/* display is multiplexed over rows */
			for (y = 0; y < 8; y++) {
				int i;
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
				/* Wait for brightness bit to be displayed */
				for (i = 0; i < (16 << bit); i++) __asm("nop;");
			}
		}

		cycles++;
#ifdef LED_TEST
		cycles %= 30;
#else
		cycles %= 11;
#endif
	}

	return 0;
}
