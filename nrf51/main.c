#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#define LED_PIN 21

int main(void)
{
    nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
    while (1)
    {
    	nrf_gpio_pin_toggle(LED_PIN);
	nrf_delay_ms(100);
    }
}
