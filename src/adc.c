/*
 * adc.c — ADC1 driver for STM32F407 (single channel, polling).
 *
 * Init sequence, in order, straight from RM0090 ("Analog-to-digital
 * converter"):
 *
 *   1. Enable the GPIOA clock and the ADC1 clock (APB2)
 *   2. Set the pin to analog mode (MODER = 11)
 *   3. Program the ADC prescaler in ADC_CCR so the ADC clock is within
 *      spec (the F407 ADC must run at 36 MHz or less)
 *   4. Calibrate (ADCAL) — the ADC must be off during calibration
 *   5. Power the ADC on (ADON) and wait tSTAB for it to stabilize
 *   6. Program the sample time (SMPR2) and the sequence (SQR1, SQR3)
 *
 * Any single step skipped produces a specific failure: no clock means
 * silent writes, no analog mode means reading the digital input, no
 * calibration means higher offset error, wrong sequence means reading
 * the wrong channel.
 */
#include "stm32f407_regs.h"
#include "adc.h"

/* This project constrains the channel to 0..7 (PA0..PA7) so we only
 * ever need to program SMPR2 and one GPIO port. Extending to channels
 * 8..18 is a good exercise: it needs SMPR1, GPIOB/GPIO C, and (for the
 * internal temperature/Vref channels) ADC_CCR.TSVREFE. */
#define ADC_MAX_CHANNEL  7

/* PCLK2 (the APB2 clock, which feeds the ADC prescaler). After reset the
 * STM32F407 runs from its internal 16 MHz HSI, so PCLK2 = 16 MHz. If you
 * later enable the PLL and raise the core clock, update this — otherwise
 * the prescaler math is wrong and you may exceed the 36 MHz ADC limit. */
#define ADC_PCLK2_HZ     16000000UL

static uint8_t s_channel;   /* channel selected at init */

void adc_init(uint8_t channel) {
    if (channel > ADC_MAX_CHANNEL) return;
    s_channel = channel;

    /* ---- Step 1: enable clocks -------------------------------------
     * GPIOA for the pin, ADC1EN for the converter itself. Two different
     * bus-enable registers, both must be set.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* ---- Step 2: analog mode on the pin ----------------------------
     * MODER = 11 for analog. Note this is *different* from alternate
     * function (10, used for UART). In analog mode the digital input
     * buffer and output driver are disconnected, and the pin appears as
     * a direct line into the ADC's sample-and-hold capacitor.
     */
    GPIOA->MODER |= (3U << (channel * 2));

    /* ---- Step 3: ADC prescaler -------------------------------------
     * The F407 ADC max clock is 36 MHz. PCLK2 = 16 MHz here, so /4 gives
     * 4 MHz — well inside spec. We pick /4 (not /2) so this code stays
     * safe even if the user later bumps PCLK2 to 84 MHz via the PLL:
     * 84/4 = 21 MHz, still under 36, while 84/2 = 42 would break.
     */
    ADC123_COMMON->CCR &= ~(3U << 16);
    ADC123_COMMON->CCR |=  ADC_CCR_ADCPRE_DIV4;

    /* ---- Step 4: calibrate -----------------------------------------
     * Calibration measures the ADC's internal offset and stores a
     * correction. It must be run with the ADC disabled (ADON = 0).
     * ADCAL is self-clearing: we set it, then poll until hardware
     * clears it to signal "calibration complete." Skipping calibration
     * works, but the offset error can be tens of LSBs — enough to
     * matter if you're computing real voltages.
     */
    ADC1->CR2 &= ~ADC_CR2_ADON;                 /* ensure ADC is off */
    ADC1->CR2 |=  ADC_CR2_ADCAL;                /* start calibration */
    while (ADC1->CR2 & ADC_CR2_ADCAL) { }       /* wait for hardware to clear */

    /* ---- Step 5: power on ------------------------------------------
     * After ADON is set, the ADC takes tSTAB (~3 µs) to stabilize.
     * We could use a timer, but a volatile busy-loop is enough here and
     * keeps the driver free of timer dependencies.
     */
    ADC1->CR2 |= ADC_CR2_ADON;
    for (volatile int i = 0; i < 1000; i++) { }

    /* ---- Step 6a: sample time --------------------------------------
     * SMPR2 holds 3 bits per channel for channels 0..9. Value 7 = 480
     * ADC clock cycles — the maximum. Long sample time is important for
     * high-impedance sources like a potentiometer (10k-100k), because
     * the sample-and-hold capacitor needs time to charge. For a
     * low-impedance source you could drop this to value 0 (3 cycles)
     * and get a much faster conversion.
     *
     *     value 0: 3 cycles    value 4:  84 cycles
     *     value 1: 15 cycles   value 5: 112 cycles
     *     value 2: 28 cycles   value 6: 144 cycles
     *     value 3: 56 cycles   value 7: 480 cycles
     */
    ADC1->SMPR2 &= ~(7U << (channel * 3));
    ADC1->SMPR2 |=  (7U << (channel * 3));

    /* ---- Step 6b: sequence -----------------------------------------
     * SQR1[23:20] = L[3:0], the number of regular conversions minus 1.
     * We want exactly one, so L = 0.
     *
     * SQR3[4:0] = SQ1, the first channel in the regular sequence.
     * For a multi-channel scan you would write SQ1, SQ2, ... and set L
     * accordingly. Here we just program one.
     */
    ADC1->SQR1 &= ~(0xFU << 20);        /* L = 0 → 1 conversion */
    ADC1->SQR3 &= ~(0x1FU);             /* clear SQ1 */
    ADC1->SQR3 |=  (channel & 0x1F);    /* SQ1 = selected channel */
}

uint16_t adc_read(void) {
    /* Trigger a software start of the regular group. SWSTART is
     * self-clearing — hardware resets it once the conversion begins. */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* Poll EOC ("end of conversion"). When the conversion finishes,
     * hardware sets EOC and leaves the 12-bit result in DR.
     *
     * The timeout is a safety net. Without it, a misconfigured ADC
     * (wrong clock, wrong sequence, or an emulator that doesn't model
     * ADC) would hang forever. Returning 0 on timeout keeps the caller
     * alive and makes the failure visible in the output. This is a
     * standard driver pattern — every well-written driver has one. */
    uint32_t timeout = 100000;
    while (!(ADC1->SR & ADC_SR_EOC)) {
        if (--timeout == 0) return 0;
    }

    /* Reading DR clears EOC. The result is 12-bit right-aligned by
     * default (CR2.ALIGN = 0), so the low 12 bits are the sample and
     * the upper bits are zero. Masking to 16 bits is defensive. */
    return (uint16_t)(ADC1->DR & 0xFFFF);
}

uint16_t adc_read_avg(uint8_t n) {
    if (n == 0) return 0;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < n; i++) sum += adc_read();
    return (uint16_t)(sum / n);
}

/* s_channel is currently read-only from the public API (it is set at
 * init and never used again outside this file). Kept static so the
 * compiler warns if we ever forget to use it — that warning is a useful
 * signal that the design has drifted. */
