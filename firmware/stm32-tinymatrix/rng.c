#include "rng.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>

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
	rcc_periph_clock_enable(RCC_ADC1);
/*
	adc_calibrate(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_39DOT5CYC);
	uint8_t conversion_channels[] = { 9 };
	adc_set_regular_sequence(ADC1, sizeof(conversion_channels), conversion_channels);
//	ADC_CFGR2 |= ADC_CLKSOURCE_PCLK_DIV4;
	adc_power_on(ADC1);
	adc_start_conversion_regular(ADC1);
*/
	rng_update(&seed, 4);
}

uint32_t rng_u32(void) {
	uint32_t res = rng_state;
	uint16_t adc_data = adc_read_regular(ADC1);
	rng_update(&adc_data, sizeof(adc_data));
	return res;
}

