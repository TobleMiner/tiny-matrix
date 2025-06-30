#include "rng.h"

#include <libopencm3/cm3/systick.h>

#define RNG_LFSR_TAPS 0x04C11DB7

static uint32_t rng_state = 0;

static void rng_update(const void *data, unsigned int len) {
	const uint8_t *data8 = data;
	while (len--) {
		uint8_t byt = *data8++;
		for (uint8_t bitpos = 0; bitpos < 8; bitpos++) {
			if (((rng_state >> 31) & 1) == ((byt >> bitpos) & 1)) {
				rng_state = (rng_state << 1);
			} else {
				rng_state = (rng_state << 1) ^ RNG_LFSR_TAPS;
			}
		}
	}
}

void rng_init(uint32_t seed) {
	systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
	systick_set_reload(0xffffff);
	systick_clear();
	systick_counter_enable();
	rng_update(&seed, 4);
}

uint32_t rng_u32(void) {
	uint32_t res = rng_state;
	uint32_t systick_data = systick_get_value();
	rng_update(&systick_data, sizeof(systick_data));
	return res;
}

