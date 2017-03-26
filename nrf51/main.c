#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "ble.h"
#include "softdevice_handler.h"
#include "ble_conn_params.h"
#include "ble_advertising.h"
#include "app_timer.h"
#include "bsp.h"

#define LED_PIN 21
#define DEVICE_NAME "EQ-Control"

#define TIMER_PRESCALER 0
#define TIMER_OP_QUEUE_SIZE 8

static ble_advdata_t advdata = {
	.name_type = BLE_ADVDATA_FULL_NAME,
	.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
	.include_appearance = 0,
};

static ble_adv_modes_config_t advcfg = {
	.ble_adv_fast_enabled = 1,
	.ble_adv_fast_interval = 50, /* in 0.65ms units */
	.ble_adv_fast_timeout = 120, /* in seconds */
};

/* Values in 1.25ms units */
static ble_gap_conn_params_t gap_conn_params = {
	.min_conn_interval = 100,
	.max_conn_interval = 200,
	.slave_latency = 0,
	.conn_sup_timeout = 4000,
};

static void on_conn_params_evt(ble_conn_params_evt_t *evt);
static void conn_params_error_handler(uint32_t nrf_error);
static ble_conn_params_init_t conn_params_init = {
	.p_conn_params = NULL,
	.first_conn_params_update_delay = APP_TIMER_TICKS(5000, TIMER_PRESCALER),
	.next_conn_params_update_delay = APP_TIMER_TICKS(30000, TIMER_PRESCALER),
	.max_conn_params_update_count = 3,
	.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID,
	.disconnect_on_fail = 0,
	.evt_handler = on_conn_params_evt,
	.error_handler = conn_params_error_handler,
};

/* TODO This is open link setting, consider more secure connections */
static ble_gap_conn_sec_mode_t sec_mode = {
	.sm = 1,
	.lv = 1
};

/* service definition TODO */

static void on_conn_params_evt(ble_conn_params_evt_t *evt)
{
}

static void conn_params_error_handler(uint32_t nrf_error)
{
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	/* Fault handler stub */
}

static void app_on_ble_evt(ble_evt_t *ble_evt)
{
	switch (ble_evt->header.evt_id)
	{
	case BLE_GAP_EVT_CONNECTED:
	//	nrf_gpio_pin_set(LED_PIN);
		break;
	case BLE_GAP_EVT_DISCONNECTED:
	//	nrf_gpio_pin_clear(LED_PIN);
		break;
	default:
		/* stub */
		break;
	}
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

static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
	/* TODO */
}

static int services_init()
{
	return 0;
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
	APP_ERROR_CHECK(ret);

	/* Enable or disable service changed characteristics */
	enable_params.gatts_enable_params.service_changed = 1;

	ret = softdevice_enable(&enable_params);
	APP_ERROR_CHECK(ret);

	ret = softdevice_ble_evt_handler_set(ble_event_handler);
	APP_ERROR_CHECK(ret);

	ret = softdevice_sys_evt_handler_set(sys_event_handler);
	APP_ERROR_CHECK(ret);

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
	ret = sd_ble_gap_device_name_set(&sec_mode,
			(const uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME)+1);
	APP_ERROR_CHECK(ret);

	ret = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(ret);

	ret = services_init();
	APP_ERROR_CHECK(ret);

	ret = ble_advertising_init(&advdata, NULL, &advcfg, on_adv_evt, NULL);
	APP_ERROR_CHECK(ret);

	ret = ble_conn_params_init(&conn_params_init);
	APP_ERROR_CHECK(ret);

	return 0;
}

void led_timer_handler(void *p_context)
{
	nrf_gpio_pin_toggle(LED0);
}

APP_TIMER_DEF(led_timer_id);

/* dummy timer */
void timer_init()
{
	uint32_t ret;

	ret = app_timer_create(&led_timer_id, APP_TIMER_MODE_REPEATED, led_timer_handler);
	APP_ERROR_CHECK(ret);

	ret = app_timer_start(led_timer_id, APP_TIMER_TICKS(500, TIMER_PRESCALER), NULL);
	APP_ERROR_CHECK(ret);

	nrf_gpio_pin_set(LED0);
}

int main(void)
{
	int ret;

	/* Need to start the timer module */
	APP_TIMER_INIT(TIMER_PRESCALER, TIMER_OP_QUEUE_SIZE, 0);
    	ret = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, TIMER_PRESCALER), NULL);
	APP_ERROR_CHECK(ret);

	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(LED0, NRF_GPIO_PIN_DIR_OUTPUT);
	ble_init();
	timer_init();

	/* Starting bluetooth service */
	ret = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(ret);

	/* Indicate the setup was successfull */
	nrf_gpio_pin_set(LED_PIN);

	while (1)
	{
		sd_app_evt_wait();
	}

	return 0;
}
