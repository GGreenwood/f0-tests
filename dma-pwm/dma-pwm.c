/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unicore-mx/stm32/rcc.h>
#include <unicore-mx/stm32/flash.h>
#include <unicore-mx/stm32/gpio.h>
#include <unicore-mx/stm32/timer.h>
#include <unicore-mx/stm32/dma.h>
#include <unicore-mx/cm3/nvic.h>

uint16_t waveform[192] __attribute__((aligned(16)));

/**
 * Set STM32 to 48MHz, based on the interal HSI clock
 * Sets AHB and APB1 frequencies to 48MHz
 * Sets the flash memory speed to FLASH_ACR_LATENCY_024_048MHZ
 */
static void clock_setup(void) {
    rcc_clock_setup_in_hsi48_out_48mhz();
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO6);
}

static void pwm_setup(void) {
    // Enable peripheral timers
    rcc_periph_clock_enable(RCC_TIM3);

    // Connect PA6 to its alternate function, TIM_CH1
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO6);   // Set AF1(TIM3_CH1) of PA1

    // Set up TIM3
    timer_reset(TIM3);
    timer_set_mode(TIM3,            // Operate on TIM3
            TIM_CR1_CKD_CK_INT,     // No prescaling(Internal clock)
            TIM_CR1_CMS_EDGE,       // Edge-aligned
            TIM_CR1_DIR_UP);        // Up-counting

    // Set up TIM3 output modes
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM2);
    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_break_main_output(TIM3);

    // Configure channel 1(PA6)
    // Will be low for 1us, high for the remaining 3us
    timer_set_oc_value(TIM3, TIM_OC1, 48 - 1);

    // Set TIM3's period to 4us at 48MHz
    timer_set_period(TIM3, 192 - 1);

    // Enable the timer
    timer_enable_counter(TIM3);
}

static void dma_setup(void) {
    rcc_periph_clock_enable(RCC_DMA);

    dma_channel_reset(DMA1, DMA_CHANNEL1);
    dma_set_priority(DMA1, DMA_CHANNEL1, DMA_CCR_PL_HIGH);

    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
    dma_enable_circular_mode(DMA1, DMA_CHANNEL1);

    dma_set_read_from_memory(DMA1, DMA_CHANNEL1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t) &TIM3_CCR1);
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t) waveform);

    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 192);

    //nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);

    dma_enable_channel(DMA1, DMA_CHANNEL1);
}

void dma1_channel1_isr(void)
{
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
    /* Toggle PA1 just to keep aware of activity and frequency. */
    gpio_toggle(GPIOA, GPIO1);
}

int main(void) {
    int i;

    // Set up DMA waveform
    for(i = 0; i < 192; i++) {
        waveform[i] = i;
    }

    clock_setup();
    gpio_setup();
    pwm_setup();
    dma_setup();

    while (1) {
    }

    return 0;
}
