/*
 * This file is part of the unicore-mx project.
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
#include <unicore-mx/stm32/gpio.h>

/* Set STM32 to 48 MHz, based on the interal HSI clock */
static void clock_setup(void)
{
    rcc_clock_setup_in_hsi48_out_48mhz();
}

static void gpio_setup(void)
{
    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOA);

    /* Set GPIO6 in PORTA to 'output push-pull'. */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
}

static void button_setup(void)
{
    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOB);

    /* Set GPIO0 (in GPIO port A) to 'input open-drain'. */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

int main(void)
{
    int i;

    clock_setup();
    //button_setup();
    gpio_setup();

    while (1) {
        gpio_toggle(GPIOA, GPIO6);

        /* Upon button press, blink more slowly. */
        /*if (gpio_get(GPIOA, GPIO0)) {
            for (i = 0; i < 3000000; i++) {
                __asm__("nop");
            }
        }
        */

        for (i = 0; i < 300000; i++)		/* Wait a bit. */
        {
            __asm("nop");
        }
    }

    return 0;
}
