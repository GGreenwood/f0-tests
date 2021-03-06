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

uint8_t waveform[192];

/**
 * Set STM32 to 48MHz, based on the interal HSI clock
 * Sets AHB and APB1 frequencies to 48MHz
 * Sets the flash memory speed to FLASH_ACR_LATENCY_024_048MHZ
 */
static void clock_setup(void) {
    // Enable peripheral timers
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_clock_enable(RCC_DMA);

    rcc_clock_setup_in_hsi48_out_48mhz();

}

static void gpio_setup(void) {
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO6);
}

static void pwm_setup(void) {
    // Connect PA6 to its alternate function, TIM_CH1
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_af(GPIOA, GPIO_AF1, GPIO6);   // Set AF1(TIM3_CH1) of PA1

    // Set up TIM3
    timer_reset(TIM3);
    timer_set_mode(
        TIM3,               // Operate on TIM3
        TIM_CR1_CKD_CK_INT, // No prescaling(Internal clock)
        TIM_CR1_CMS_EDGE,   // Edge-aligned
        TIM_CR1_DIR_UP      // Up-counting
    );

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
    dma_channel_reset(DMA1, DMA_CHANNEL4);
    //dma_enable_channel(DMA1, DMA_CHANNEL4);

    dma_channel_reset(DMA1, DMA_CHANNEL4);
    dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_HIGH);

    dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
    dma_enable_circular_mode(DMA1, DMA_CHANNEL4);

    dma_set_read_from_memory(DMA1, DMA_CHANNEL4);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t) &TIM3_CCR1);
    dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t) waveform);

    dma_set_number_of_data(DMA1, DMA_CHANNEL4, 192);

    nvic_enable_irq(NVIC_DMA1_CHANNEL4_5_IRQ);
    nvic_enable_irq(NVIC_TIM3_IRQ);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

    // Enable DMA requests from TIM3
    timer_enable_irq(TIM3, TIM_DIER_CC1DE);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);
    timer_set_dma_on_update_event(TIM3);

    dma_enable_channel(DMA1, DMA_CHANNEL4);
}

void tim3_isr(void) {
    timer_clear_flag(TIM3, TIM_SR_CC1IF);
    /* Toggle PA4 just to keep aware of activity and frequency. */
    gpio_toggle(GPIOA, GPIO4);
}

void dma1_channel4_5_isr(void)
{
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL4, DMA_TCIF);
    /* Toggle PA5 just to keep aware of activity and frequency. */
    gpio_toggle(GPIOA, GPIO5);
}

int main(void) {
    uint8_t i;

    // Set up DMA waveform
    for(i = 0; i < 192; i++) {
        waveform[i] = i;
    }

    clock_setup();
    gpio_setup();
    pwm_setup();
    dma_setup();

    while (1) {
        //dma_clear_interrupt_flags(DMA1, DMA_CHANNEL4, DMA_TCIF);
    }

    return 0;
}
