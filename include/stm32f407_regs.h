/*
 * stm32f407_regs.h — Register map for the STM32F407.
 *
 * Same technique as the blinky/UART projects: cast a base address to a
 * struct of volatile uint32_t fields, then access hardware registers by
 * name. This file has been trimmed to the peripherals this project uses
 * (RCC, GPIO, USART2, ADC1). Adding another peripheral means adding one
 * struct and one #define.
 */
#ifndef STM32F407_REGS_H
#define STM32F407_REGS_H

#include <stdint.h>

#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)   /* 0x40023800 */
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)   /* 0x40020000 */
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)   /* 0x40004400 */
#define ADC1_BASE           (APB2PERIPH_BASE + 0x2000UL)   /* 0x40012000 */
#define ADC123_COMMON_BASE  (APB2PERIPH_BASE + 0x2300UL)   /* 0x40012300 */

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/* ADC1 register block. The JOFR/JDR fields are for the injected-group
 * (interrupt-driven) conversions, which this project does not use — but
 * they must be present so later registers land at the correct offset. */
typedef struct {
    volatile uint32_t SR;      /* 0x00  status */
    volatile uint32_t CR1;     /* 0x04  control 1 */
    volatile uint32_t CR2;     /* 0x08  control 2 (ADON, SWSTART, ADCAL) */
    volatile uint32_t SMPR1;   /* 0x0C  sample time, channels 10-18 */
    volatile uint32_t SMPR2;   /* 0x10  sample time, channels 0-9 */
    volatile uint32_t JOFR1;   /* 0x14 */
    volatile uint32_t JOFR2;   /* 0x18 */
    volatile uint32_t JOFR3;   /* 0x1C */
    volatile uint32_t JOFR4;   /* 0x20 */
    volatile uint32_t HTR;     /* 0x24 */
    volatile uint32_t LTR;     /* 0x28 */
    volatile uint32_t SQR1;    /* 0x2C  regular sequence length */
    volatile uint32_t SQR2;    /* 0x30  regular sequence, channels 7-12 */
    volatile uint32_t SQR3;    /* 0x34  regular sequence, channels 1-6 */
    volatile uint32_t JSQR;    /* 0x38 */
    volatile uint32_t JDR1;    /* 0x3C */
    volatile uint32_t JDR2;    /* 0x40 */
    volatile uint32_t JDR3;    /* 0x44 */
    volatile uint32_t JDR4;    /* 0x48 */
    volatile uint32_t DR;      /* 0x4C  regular data (12-bit result) */
} ADC_TypeDef;

/* Common registers shared across ADC1, ADC2, ADC3. Only CCR matters here:
 * it holds the ADC prescaler and the internal-channel enables. */
typedef struct {
    volatile uint32_t CSR;     /* 0x00 */
    volatile uint32_t CCR;     /* 0x04  common control (ADCPRE, TSVREFE, VBATE) */
    volatile uint32_t CDR;     /* 0x08 */
} ADC_Common_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t          RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t          RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t          RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

#define RCC            ((RCC_TypeDef *)RCC_BASE)
#define GPIOA          ((GPIO_TypeDef *)GPIOA_BASE)
#define USART2         ((USART_TypeDef *)USART2_BASE)
#define ADC1           ((ADC_TypeDef *)ADC1_BASE)
#define ADC123_COMMON  ((ADC_Common_TypeDef *)ADC123_COMMON_BASE)

/* RCC enables */
#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_APB1ENR_USART2EN    (1U << 17)
#define RCC_APB2ENR_ADC1EN      (1U << 8)     /* ADC1 hangs off APB2 */

/* USART */
#define USART_SR_TXE            (1U << 7)
#define USART_CR1_UE            (1U << 13)
#define USART_CR1_TE            (1U << 3)

/* ADC_CR2 */
#define ADC_CR2_ADON            (1U << 0)     /* power on / enable */
#define ADC_CR2_SWSTART         (1U << 30)    /* software-trigger a conversion */
#define ADC_CR2_ADCAL           (1U << 31)    /* start calibration, self-clearing */

/* ADC_SR */
#define ADC_SR_EOC              (1U << 1)     /* end of regular conversion */

/* ADC_CCR bit fields */
#define ADC_CCR_ADCPRE_DIV2     (0U << 16)    /* PCLK2 / 2 */
#define ADC_CCR_ADCPRE_DIV4     (1U << 16)    /* PCLK2 / 4 */
#define ADC_CCR_ADCPRE_DIV6     (2U << 16)    /* PCLK2 / 6 */
#define ADC_CCR_ADCPRE_DIV8     (3U << 16)    /* PCLK2 / 8 */

#endif /* STM32F407_REGS_H */
