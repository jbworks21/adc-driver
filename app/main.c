/*
 * app/main.c — Demo: read a potentiometer on PA0, show it as a bar graph.
 *
 * The demo includes a small self-contained UART console (about 40 lines,
 * all static) so the project has zero dependency on the UART driver from
 * the previous project. That console is *not* part of the ADC driver —
 * it's demo scaffolding, freely disposable.
 *
 * Wiring:
 *   - Potentiometer: outer legs to 3V3 and GND, wiper to PA0
 *   - Serial out:    PA2 (TX) → USB-UART adapter RX, GND → GND
 *                    115200 8N1
 *
 * If you don't have a pot handy, any voltage source 0..3.3 V works, and
 * even a floating wire will produce noisy but visible movement.
 */
#include "stm32f407_regs.h"
#include "adc.h"

/* ================= Demo-only serial console ================= */

static void console_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = TX, PA3 = RX, alternate function 7 (USART2) */
    GPIOA->MODER &= ~((3U << 4) | (3U << 6));
    GPIOA->MODER |=  ((2U << 4) | (2U << 6));
    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));
    GPIOA->AFR[0] |=  ((0x7U << 8) | (0x7U << 12));

    USART2->BRR = (16000000UL + 115200U / 2) / 115200U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void putc_(char c) {
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = (uint8_t)c;
}

static void puts_(const char *s) {
    while (*s) putc_(*s++);
}

/* Print an unsigned value padded to exactly 4 digits (0..9999).
 * Enough for 0..4095 (ADC counts) and 0..3300 (millivolts at 3.3 V ref). */
static void print_u32_pad4(uint32_t v) {
    char buf[4];
    for (int i = 3; i >= 0; i--) { buf[i] = (char)('0' + (v % 10U)); v /= 10U; }
    for (int i = 0; i < 4; i++) putc_(buf[i]);
}

static void delay_(volatile uint32_t n) { while (n--) __asm__("nop"); }

/* ================= Main application ================= */

/* Board property, not a driver property. The Nucleo and Discovery boards
 * tie VREF+ to VDDA = 3.300 V. If your board differs, change this — do
 * not change the driver. */
#define VREF_MV  3300U

int main(void) {
    console_init();
    adc_init(0);                 /* PA0 = ADC1_IN0 */

    puts_("\r\n=== ADC1 driver online — pot on PA0 ===\r\n\r\n");

    while (1) {
        /* Average 8 samples to smooth the jitter in the low bits. */
        uint16_t raw = adc_read_avg(8);

        /* Scale to millivolts. Note the divisor is 4095, not 4096: a
         * 12-bit converter can produce 0..4095, and 4095 corresponds to
         * full scale (VREF). Using 4096 is a common off-by-one that
         * makes every reading slightly low. */
        uint16_t mv = (uint16_t)(((uint32_t)raw * VREF_MV) / 4095U);

        /* Build a 40-character bar graph proportional to raw. */
        char bar[41];
        int filled = (raw * 40) / 4095;
        for (int i = 0; i < 40; i++) bar[i] = (i < filled) ? '#' : ' ';
        bar[40] = '\0';

        puts_("raw=");
        print_u32_pad4(raw);
        puts_("  mv=");
        print_u32_pad4(mv);
        puts_("  [");
        puts_(bar);
        puts_("]\r\n");

        delay_(3000000);         /* crude ~200 ms pause */
    }
}
