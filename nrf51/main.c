#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "ble.h"
#include "softdevice_handler.h"
#include "ble_conn_params.h"
#include "ble_advertising.h"

#define LED_PIN 21

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	/* Fault handler stub */
}

static void app_on_ble_evt(ble_evt_t *ble_evt)
{
}

static void ble_event_handler(ble_evt_t *ble_evt)
{
	/* This handler can call already implemented handlers from used modules */
	/* Connection handler */
	ble_conn_params_on_ble_evt(ble_evt);
	
	/* Our application handler */
	app_on_ble_evt(ble_evt);

	/* Advertising handler */
	ble_advertising_on_ble_evt(ble_evt);
}

static void sys_event_handler(uint32_t sys_evt)
{
	ble_advertising_on_sys_evt(sys_evt);
}

/** @brief Initialize ble stack */
static int ble_init()
{
	int ret;
	ble_enable_params_t enable_params;

	/* Low frequency clock settings */
	nrf_clock_lf_cfg_t nrf_clock = {.source = NRF_CLOCK_LF_SRC_XTAL,
                                 .rc_ctiv       = 0,
                                 .rc_temp_ctiv  = 0,
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};

	SOFTDEVICE_HANDLER_INIT(&nrf_clock, NULL);

	ret = softdevice_enable_get_default_config(0, 1, &enable_params);
	if (ret < 0)
		return ret;

	/* Enable or disable service changed characteristics */
	enable_params.gatts_enable_params.service_changed = 0;
	ret = softdevice_enable(&enable_params);
	if (ret < 0)
		return ret;

	ret = softdevice_ble_evt_handler_set(ble_event_handler);
	if (ret < 0)
		return ret;

	ret = softdevice_sys_evt_handler_set(sys_event_handler);
	if (ret < 0)
		return ret;

	return 0;
}

int main(void)
{
	ble_init();
	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	while (1)
	{
		nrf_gpio_pin_toggle(LED_PIN);
		nrf_delay_ms(500);
	}
}
