#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "app_uart.h"
#include "bsp.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "ser_dbg_sd_str.h"
#include "eq_ble.h"
#include "common.h"

#define LED_PIN 21

#define SERVICE_UUID 0x1523

struct context {
	uint16_t service_handle;
	uint32_t ra;
	uint32_t dec;
	ble_gatts_char_handles_t char_handle[2];
} ctx;

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	/* Fault handler stub */
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

static int services_init()
{
	int ret;
	ble_uuid_t uuid;

	uuid.uuid = SERVICE_UUID;
	ret = sd_ble_uuid_vs_add(&base_uuid, &uuid.type);
	APP_ERROR_CHECK(ret);

	ret = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
			&uuid, &ctx.service_handle);
	APP_ERROR_CHECK(ret);

	ctx.ra = 0xfefe;
	ctx.dec = 0xbaba;

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.ra),
			(uint8_t *)&ctx.ra, 0x1525, PERM_WRITE | PERM_READ, &ctx.char_handle[0]);

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.dec),
			(uint8_t *)&ctx.dec, 0x1526, PERM_WRITE | PERM_READ, &ctx.char_handle[1]);

	return 0;
}

int main(void)
{
	int ret;

	ret = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(ret);

	NRF_LOG_INFO("\r\nInitializing...\r\n")
	/* Need to start the timer module */
	APP_TIMER_INIT(TIMER_PRESCALER, TIMER_OP_QUEUE_SIZE, 0);
    	ret = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, TIMER_PRESCALER), NULL);
	APP_ERROR_CHECK(ret);

	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(LED0, NRF_GPIO_PIN_DIR_OUTPUT);
	NRF_LOG_INFO("\r\nInitializing BLE...\r\n")
	ble_init();
	services_init();
	NRF_LOG_INFO("\r\nInitializing timers...\r\n")
	timer_init();

	/* Starting bluetooth service */
	ret = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(ret);

	/* Indicate the setup was successfull */
	nrf_gpio_pin_set(LED_PIN);

	NRF_LOG_INFO("\r\nInitialization completed\r\n")

	ctx.ra = 0x1337;
	ctx.dec = 0xdeadbeef;

	while (1)
	{
		if (NRF_LOG_PROCESS() == false)
			sd_app_evt_wait();
	}

	return 0;
}
