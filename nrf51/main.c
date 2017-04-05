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
#define MOTOR_STEP_RATIO 200
#define MOTOR_MICROSTEPS 16
#define GEAR_RATIO 130
#define STELLAR_DAY 86164
#define GOTO_ACCURACY 60

/* nuber of clock ticks per one motor step for tracking */
#define STEP_TICKS (APP_TIMER_CLOCK_FREQ / MOTOR_MICROSTEPS) * (STELLAR_DAY / (MOTOR_STEP_RATIO * GEAR_RATIO))

/* step time for goto */
#define GOTO_STEP_MS 2

/* step time for manual control */
#define MANUAL_STEP_MS 20

/* length of single microstep in arc seconds */
#define STEP_LENGTH (360*60*60)/(MOTOR_STEP_RATIO * GEAR_RATIO * MOTOR_MICROSTEPS)
#define FULL_STEP_LENGTH (360*60*60)/(MOTOR_STEP_RATIO * GEAR_RATIO)

enum {
	MODE_TRACKING,
	MODE_GOTO,
	MODE_MANUAL,
	MODE_OFF,
};

#define CHAR_NUMBER 4
struct context {
	uint16_t service_handle;
	uint32_t pos[2]; /**< position in acrseconds {RA, DEC} */
	uint32_t dest[2]; /**< destination in arcseconds {RA, DEC} */
	uint8_t mode;
	uint8_t reverse; /**< for simple reversing direction */
	ble_gatts_char_handles_t char_handle[CHAR_NUMBER];
} ctx;

APP_TIMER_DEF(led_timer_id);
APP_TIMER_DEF(sky_movement_timer_id);
APP_TIMER_DEF(goto_timer_id);
APP_TIMER_DEF(manual_timer_id);

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	/* Fault handler stub */
}

void led_timer_handler(void *p_context)
{
	nrf_gpio_pin_toggle(LED0);
}

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

/* TODO ERROR handling
 * TODO UUID generalization*/
	ret = eq_add_characteristic(ctx.service_handle, 2 * sizeof(uint32_t),
			(uint8_t *)ctx.pos[0], 0x1525, PERM_WRITE | PERM_READ, &ctx.char_handle[0]);

	ret = eq_add_characteristic(ctx.service_handle, 2 * sizeof(uint32_t),
			(uint8_t *)ctx.dest, 0x1526, PERM_WRITE | PERM_READ, &ctx.char_handle[1]);

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.mode),
			(uint8_t *)&ctx.mode, 0x1528, PERM_WRITE | PERM_READ, &ctx.char_handle[2]);

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.reverse),
			(uint8_t *)&ctx.reverse, 0x1528, PERM_WRITE | PERM_READ, &ctx.char_handle[3]);

	return ret;
}

static void sky_movement_timer_handler(void *p_context)
{
	switch (ctx.mode) {
	case MODE_TRACKING:
		/* direction was already set */
		nrf_gpio_pin_toggle(RA_STEP);
		break;
	case MODE_GOTO:
	case MODE_MANUAL:
		/* In GOTO and MANUAL mode RA motor is controlled by different routine. */
		if (ctx.reverse)
			ctx.dest[0] -= STEP_LENGTH;
		else
			ctx.dest[0] += STEP_LENGTH;
		break;
	}
}

enum {
	DIR_PROGRADE,
	DIR_RETROGRADE,
	DIR_OFF,
};

static void ra_set_dir(int dir)
{
	/* disabled motor won't go in any direction */
	if (dir == DIR_OFF)
		nrf_gpio_pin_set(RA_EN);

	if (dir == DIR_PROGRADE || (dir == DIR_RETROGRADE && ctx.reverse))
		nrf_gpio_pin_set(RA_DIR);
	else
		nrf_gpio_pin_clear(RA_DIR);
}

static void dec_set_dir(int dir)
{
	if (dir == DIR_OFF)
		nrf_gpio_pin_set(DEC_EN);

	if (dir == DIR_PROGRADE || (dir == DIR_RETROGRADE && ctx.reverse))
		nrf_gpio_pin_set(DEC_DIR);
	else
		nrf_gpio_pin_clear(DEC_DIR);
}

static void set_mode(int mode)
{
	int ret;

	if (mode == ctx.mode)
		return;

	/* Stop previous mode */
	switch (ctx.mode) {
	case MODE_TRACKING:
		break;
	case MODE_GOTO:
		app_timer_stop(goto_timer_id);
		break;
	case MODE_MANUAL:
		app_timer_stop(manual_timer_id);
		break;
	}

	switch (mode) {
	case MODE_TRACKING:
		NRF_LOG_INFO("\r\nGoing into TRACKING mode.\r\n")
		/* 1/16 microsteps */
		nrf_gpio_pin_set(RA_MS1);
		nrf_gpio_pin_set(RA_MS2);
		nrf_gpio_pin_set(RA_MS3);

		nrf_gpio_pin_clear(RA_EN);

		/* disable DEC motor */
		nrf_gpio_pin_set(DEC_EN);

		ra_set_dir(DIR_PROGRADE);

		break;
	case MODE_GOTO:
		NRF_LOG_INFO("\r\nGoing into GOTO mode.\r\n")
		NRF_LOG_INFO("\r\n pos = [%d, %d], dest = [%d, %d]\r\n",
				ctx.pos[0], ctx.pos[1], ctx.dest[0], ctx.dest[1])
		/* full steps for better speed */
		nrf_gpio_pin_clear(RA_MS1);
		nrf_gpio_pin_clear(RA_MS2);
		nrf_gpio_pin_clear(RA_MS3);

		/* both motors enabled */
		nrf_gpio_pin_clear(DEC_EN);
		nrf_gpio_pin_clear(RA_EN);

		ret = app_timer_start(goto_timer_id, APP_TIMER_TICKS(GOTO_STEP_MS, TIMER_PRESCALER), NULL);
		APP_ERROR_CHECK(ret);
		break;
	case MODE_MANUAL:
		NRF_LOG_INFO("\r\nGoing into MANUAL mode.\r\n")
		/* 1/16 microsteps */
		nrf_gpio_pin_set(RA_MS1);
		nrf_gpio_pin_set(RA_MS2);
		nrf_gpio_pin_set(RA_MS3);

		nrf_gpio_pin_set(DEC_MS1);
		nrf_gpio_pin_set(DEC_MS2);
		nrf_gpio_pin_set(DEC_MS3);

		/* both motors enabled */
		nrf_gpio_pin_clear(DEC_EN);
		nrf_gpio_pin_clear(RA_EN);

		/* TODO Adjustable speed */
		ret = app_timer_start(manual_timer_id, APP_TIMER_TICKS(MANUAL_STEP_MS, TIMER_PRESCALER), NULL);
		APP_ERROR_CHECK(ret);
		break;
	case MODE_OFF:
		NRF_LOG_INFO("\r\nGoing into OFF mode.\r\n")
		break;
	}

	ctx.mode = mode;
}

static void goto_timer_handler(void *p_context)
{
	int end = 1;

	/* ensure we should do anything */
	if (ctx.mode != MODE_GOTO)
		return;

	if (ctx.dest[0] - ctx.pos[0] > GOTO_ACCURACY) {
		ra_set_dir(DIR_PROGRADE);
		nrf_gpio_pin_toggle(RA_STEP);
		ctx.pos[0] += FULL_STEP_LENGTH;
		end = 0;
	} else if (ctx.pos[0] - ctx.dest[0] > GOTO_ACCURACY) {
		ra_set_dir(DIR_RETROGRADE);
		nrf_gpio_pin_toggle(RA_STEP);
		ctx.pos[0] -= FULL_STEP_LENGTH;
		end = 0;
	}

	if (ctx.dest[1] - ctx.pos[1] > GOTO_ACCURACY) {
		dec_set_dir(DIR_PROGRADE);
		nrf_gpio_pin_toggle(DEC_STEP);
		ctx.pos[1] += FULL_STEP_LENGTH;
		end = 0;
	} else if (ctx.pos[1] - ctx.dest[1] > GOTO_ACCURACY) {
		dec_set_dir(DIR_RETROGRADE);
		nrf_gpio_pin_toggle(DEC_STEP);
		ctx.pos[1] -= FULL_STEP_LENGTH;
		end = 0;
	}

	/* return to tracking mode */
	if (end)
		set_mode(MODE_TRACKING);
}

static void manual_timer_handler(void *p_context)
{
	if (ctx.mode != MODE_MANUAL)
		return;

	/* direction control is handled elsewhere */
	nrf_gpio_pin_toggle(DEC_STEP);
	nrf_gpio_pin_toggle(RA_STEP);
}

static int motor_init()
{
	uint32_t ret;

	/* sky moves always and this timer is always active */
	ret = app_timer_create(&sky_movement_timer_id, APP_TIMER_MODE_REPEATED,
			sky_movement_timer_handler);
	APP_ERROR_CHECK(ret);

	ret = app_timer_start(sky_movement_timer_id, STEP_TICKS, NULL);
	APP_ERROR_CHECK(ret);

	ret = app_timer_create(&goto_timer_id, APP_TIMER_MODE_REPEATED,
			goto_timer_handler);
	APP_ERROR_CHECK(ret);

	ret = app_timer_create(&manual_timer_id, APP_TIMER_MODE_REPEATED,
			manual_timer_handler);
	APP_ERROR_CHECK(ret);

	nrf_gpio_pin_dir_set(RA_EN, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(RA_DIR, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(RA_STEP, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(RA_MS1, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(RA_MS2, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(RA_MS3, NRF_GPIO_PIN_DIR_OUTPUT);

	nrf_gpio_pin_dir_set(DEC_EN, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(DEC_DIR, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(DEC_STEP, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(DEC_MS1, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(DEC_MS2, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(DEC_MS3, NRF_GPIO_PIN_DIR_OUTPUT);

	ctx.mode = MODE_OFF;

	return ret;
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

	NRF_LOG_INFO("\r\nInitializing motors...\r\n")
	motor_init();	
	set_mode(MODE_TRACKING);

	/* Starting bluetooth service */
	ret = ble_advertising_start(BLE_ADV_MODE_FAST);
	APP_ERROR_CHECK(ret);

	/* Indicate the setup was successfull */
	nrf_gpio_pin_set(LED_PIN);

	NRF_LOG_INFO("\r\nInitialization completed\r\n")

	while (1)
	{
		if (NRF_LOG_PROCESS() == false)
			sd_app_evt_wait();
	}

	return 0;
}
