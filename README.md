# ADC1 Driver (STM32F407 / ARM Cortex-M4)

A bare-metal polling driver for the STM32F407's 12-bit ADC1, plus a demo
that reads a potentiometer on PA0 and displays the value as a bar graph
over USART2.

Built from scratch — no HAL, no CMSIS, no vendor SDK. Every register
access is explicit and explained in the source comments.

## What This Teaches

- **Analog pin mode** — `MODER = 11`, the third pin mode
- **ADC clock domain** — the prescaler in `ADC_CCR` and the 36 MHz limit
- **Calibration** — the one-time `ADCAL` sequence before first use
- **Sample time** — why high-impedance sources need long sample windows
- **Sequence registers** — `SQR1`/`SQR3` describe what to convert
- **Polling with timeout** — `EOC` with a safety net

## Hardware

- **MCU:** STM32F407 (any board with ADC1 and USART2 on PA2/PA3)
- **Input:** 10k potentiometer, wiper → **PA0**, ends → 3V3 and GND
- **Serial:** PA2 (TX) → USB-UART adapter RX, GND → GND, **115200 8N1**

## Build (Termux)

```bash
pkg install clang llvm lld make git -y
make
