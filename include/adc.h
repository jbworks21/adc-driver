/*
 * adc.h — Public API of the ADC1 driver (STM32F407).
 *
 * Scope of this driver: single-channel, single-conversion, blocking reads
 * of the 12-bit regular group. No DMA, no continuous mode, no injected
 * group, no analog watchdog. Those are natural extensions once the basics
 * work — see the README's "next steps."
 *
 * Voltage reference is NOT baked in. The driver returns raw 12-bit
 * counts (0..4095); the caller decides what those counts mean in volts.
 * That's a deliberate separation of concerns: the driver describes the
 * hardware, the application describes the board.
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*
 * Configure ADC1 to sample one channel on port A.
 *
 * channel: 0..7, mapping to pins PA0..PA7. The function enables the
 *          GPIOA clock, sets the pin to analog mode, programs the ADC
 *          prescaler, calibrates, and configures the sequence registers
 *          for a single conversion of that channel.
 *
 * Must be called once before adc_read(). Calling it again reconfigures
 * the same ADC to the new channel — it is not additive.
 */
void adc_init(uint8_t channel);

/*
 * Perform one blocking conversion on the configured channel.
 * Returns the raw 12-bit result (0..4095), or 0 if the hardware did not
 * signal EOC within the built-in timeout.
 */
uint16_t adc_read(void);

/*
 * Average n consecutive conversions. Useful because real ADCs have
 * noise — the last couple of bits jitter even on a stable input.
 * n=8 or n=16 is a good default for a slow-moving signal like a pot.
 * n=0 returns 0.
 */
uint16_t adc_read_avg(uint8_t n);

#endif /* ADC_H */
