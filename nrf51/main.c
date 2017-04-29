/* 
 * TODO:
 */

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_gpiote.h"
#include "nrf_drv_gpiote.h"
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

/* EQ3-2 settings, TODO: Export mount configuration */
#define RA_GEAR_RATIO 130
#define DEC_GEAR_RATIO 65
#define STELLAR_DAY 86164
#define GOTO_ACCURACY 60

/* nuber of clock ticks per one motor step for tracking (1 arc sec of RA) */
#define STEP_TICKS (APP_TIMER_CLOCK_FREQ / MOTOR_MICROSTEPS) * (STELLAR_DAY / (MOTOR_STEP_RATIO * RA_GEAR_RATIO))

/* step time for goto */
#define GOTO_STEP_MS 2

/* step time for manual control */
#define MANUAL_STEP_MS 20

#define ARC_MAX (360*60*60)

#define RA_FULL_STEP_LENGTH (ARC_MAX)/(MOTOR_STEP_RATIO * RA_GEAR_RATIO)
#define DEC_FULL_STEP_LENGTH (ARC_MAX)/(MOTOR_STEP_RATIO * DEC_GEAR_RATIO)

#define RA_STEP_LENGTH (ARC_MAX)/(MOTOR_STEP_RATIO * RA_GEAR_RATIO * MOTOR_MICROSTEPS)
#define DEC_STEP_LENGTH (ARC_MAX)/(MOTOR_STEP_RATIO * DEC_GEAR_RATIO * MOTOR_MICROSTEPS)

#define EQ_ERROR_CHECK(ret) do { \
	if (ret) \
		NRF_LOG_INFO("ERROR %d: ret = 0x%x\n\r", __LINE__, ret); \
	APP_ERROR_CHECK(ret); \
} while(0)

enum {
	MODE_TRACKING = 0x01,
	MODE_GOTO = 0x02,
	MODE_MANUAL = 0x03,
	MODE_OFF = 0xff,
};

enum {
	CHAR_POS = 0,
	CHAR_DEST = 1,
	CHAR_MODE = 2,
	CHAR_REVERSE = 3,
	CHAR_NUMBER = 4,
};

enum {
	UUID_POS = 0x1525,
	UUID_DEST = 0x1526,
	UUID_MODE = 0x1527,
	UUID_REVERSE = 0x1528,
};

enum {
	DIR_PROGRADE,
	DIR_RETROGRADE,
	DIR_OFF,
};

struct context {
	uint16_t service_handle;
	int32_t pos[2]; /**< position in acrseconds {RA, DEC} */
	int32_t dest[2]; /**< destination in arcseconds {RA, DEC} */
	uint8_t mode;
	uint8_t reverse; /**< for simple reversing direction */

	uint8_t direction[2]; /**< direction set to hardware, not exported */
	ble_gatts_char_handles_t char_handle[CHAR_NUMBER];
} ctx;

APP_TIMER_DEF(led_timer_id);
APP_TIMER_DEF(sky_movement_timer_id);
APP_TIMER_DEF(goto_timer_id);
APP_TIMER_DEF(manual_timer_id);

void ctx_set_mode(uint8_t mode)
{
	int ret;

	ctx.mode = mode;
	ret = eq_set_characteristic_value(ctx.char_handle[CHAR_MODE].value_handle,
			sizeof(ctx.mode), &ctx.mode);
	EQ_ERROR_CHECK(ret);
}

void ctx_set_reverse(uint8_t reverse)
{
	int ret;

	ctx.reverse = reverse;
	ret = eq_set_characteristic_value(ctx.char_handle[CHAR_REVERSE].value_handle,
			sizeof(ctx.reverse), &ctx.reverse);
	EQ_ERROR_CHECK(ret);
}

void ctx_set_pos(uint32_t ra, uint32_t dec)
{
	int ret;

	ctx.pos[0] = ra;
	ctx.pos[1] = dec;
	ret = eq_set_characteristic_value(ctx.char_handle[CHAR_POS].value_handle,
			2 * sizeof(uint32_t), (uint8_t *)ctx.pos);
	EQ_ERROR_CHECK(ret);
}

void ctx_set_dest(uint32_t dest_ra, uint32_t dest_dec)
{
	int ret;

	ctx.dest[0] = dest_ra;
	ctx.dest[1] = dest_dec;
	ret = eq_set_characteristic_value(ctx.char_handle[CHAR_DEST].value_handle,
			2 * sizeof(uint32_t), (uint8_t *)ctx.dest);
	EQ_ERROR_CHECK(ret);
}

/* position will be changed dynamically, consider not updating BLE stack every time it changes */
void ctx_update_pos()
{
	int ret;

	ret = eq_set_characteristic_value(ctx.char_handle[CHAR_POS].value_handle,
			2 * sizeof(uint32_t), (uint8_t *)ctx.pos);
	EQ_ERROR_CHECK(ret);
}

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
	EQ_ERROR_CHECK(ret);

	ret = app_timer_start(led_timer_id, APP_TIMER_TICKS(500, TIMER_PRESCALER), NULL);
	EQ_ERROR_CHECK(ret);

	nrf_gpio_pin_set(LED0);
}

static void ra_set_dir(int dir)
{
	/* disabled motor won't go in any direction */
	if (dir == DIR_OFF)
		nrf_gpio_pin_set(RA_EN);

	if (dir == DIR_PROGRADE || (dir == DIR_RETROGRADE && ctx.reverse))
		nrf_gpio_pin_set(RA_DIR);
	else
		nrf_gpio_pin_clear(RA_DIR);

	ctx.direction[0] = dir;
}

static void dec_set_dir(int dir)
{
	if (dir == DIR_OFF)
		nrf_gpio_pin_set(DEC_EN);

	if (dir == DIR_PROGRADE || (dir == DIR_RETROGRADE && ctx.reverse))
		nrf_gpio_pin_set(DEC_DIR);
	else
		nrf_gpio_pin_clear(DEC_DIR);

	ctx.direction[1] = dir;
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

		/* disable DEC motor, enable RA */
		nrf_gpio_pin_clear(RA_EN);
		nrf_gpio_pin_set(DEC_EN);

		ra_set_dir(DIR_PROGRADE);
		dec_set_dir(DIR_OFF);

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
		
		/* TODO integrate direction handling with ctx.direction */

		ret = app_timer_start(goto_timer_id, APP_TIMER_TICKS(GOTO_STEP_MS, TIMER_PRESCALER), NULL);
		EQ_ERROR_CHECK(ret);
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
		EQ_ERROR_CHECK(ret);
		break;
	case MODE_OFF:
		/* disable both motors */
		nrf_gpio_pin_set(DEC_EN);
		nrf_gpio_pin_set(RA_EN);

		NRF_LOG_INFO("\r\nGoing into OFF mode.\r\n")
		break;
	default:
		NRF_LOG_INFO("\r\nUnknown mode %d, going to OFF mode instead\r\n", mode);
		mode = MODE_OFF;
		break;
	}

	ctx_set_mode(mode);
}

static void set_reverse(uint8_t reverse)
{
	if (ctx.reverse == reverse)
		ctx.reverse = reverse;

	if (reverse)
		NRF_LOG_INFO("Setting reverse mode\r\n");
	if (reverse)
		NRF_LOG_INFO("Disabling reverse mode\r\n");

	/* update direction pin */
	if (ctx.mode == MODE_TRACKING) {
		ra_set_dir(DIR_PROGRADE);
	}
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
	case MODE_OFF:
		/* In GOTO and MANUAL mode RA motor is controlled by different routine. */
		/* In OFF mode we still want to track position */
		if (ctx.reverse)
			ctx.pos[0] -= RA_STEP_LENGTH;
		else
			ctx.pos[0] += RA_STEP_LENGTH;
		break;
	}

	/** For now, update every tracking step */
	ctx_update_pos();
}

static void goto_timer_handler(void *p_context)
{
	int end = 1;

	/* ensure we should do anything */
	if (ctx.mode != MODE_GOTO)
		return;

	if (ctx.dest[0] > ctx.pos[0] + GOTO_ACCURACY) {
		ra_set_dir(DIR_PROGRADE);
		nrf_gpio_pin_toggle(RA_STEP);
		ctx.pos[0] += RA_FULL_STEP_LENGTH;
		end = 0;
	} else if (ctx.pos[0] > ctx.dest[0] + GOTO_ACCURACY) {
		ra_set_dir(DIR_RETROGRADE);
		nrf_gpio_pin_toggle(RA_STEP);
		ctx.pos[0] -= RA_FULL_STEP_LENGTH;
		end = 0;
	}
	ctx.pos[0] %= ARC_MAX;

	if (ctx.dest[1] - ctx.pos[1] > GOTO_ACCURACY) {
		dec_set_dir(DIR_PROGRADE);
		nrf_gpio_pin_toggle(DEC_STEP);
		ctx.pos[1] += DEC_FULL_STEP_LENGTH;
		end = 0;
	} else if (ctx.pos[1] - ctx.dest[1] > GOTO_ACCURACY) {
		dec_set_dir(DIR_RETROGRADE);
		nrf_gpio_pin_toggle(DEC_STEP);
		ctx.pos[1] -= DEC_FULL_STEP_LENGTH;
		end = 0;
	}
	ctx.pos[1] %= ARC_MAX;

	/* return to tracking mode */
	if (end)
		set_mode(MODE_TRACKING);
}

static void manual_timer_handler(void *p_context)
{
	if (ctx.mode != MODE_MANUAL)
		return;

	/* direction control is handled elsewhere */
	nrf_gpio_pin_toggle(RA_STEP);
	nrf_gpio_pin_toggle(DEC_STEP);

	switch (ctx.direction[0]) {
	case DIR_PROGRADE:
		ctx.pos[0] += RA_STEP_LENGTH;
		break;
	case DIR_RETROGRADE:
		ctx.pos[0] -= RA_STEP_LENGTH;
		break;
	case DIR_OFF:
		break;
	}
	ctx.pos[0] %= ARC_MAX;

	switch (ctx.direction[1]) {
	case DIR_PROGRADE:
		ctx.pos[1] += DEC_STEP_LENGTH;
		break;
	case DIR_RETROGRADE:
		ctx.pos[1] -= DEC_STEP_LENGTH;
		break;
	case DIR_OFF:
		break;
	}
	ctx.pos[1] %= ARC_MAX;
}

static int motor_init()
{
	uint32_t ret;

	/* sky moves always and this timer is always active */
	ret = app_timer_create(&sky_movement_timer_id, APP_TIMER_MODE_REPEATED,
			sky_movement_timer_handler);
	EQ_ERROR_CHECK(ret);

	ret = app_timer_start(sky_movement_timer_id, STEP_TICKS, NULL);
	EQ_ERROR_CHECK(ret);

	ret = app_timer_create(&goto_timer_id, APP_TIMER_MODE_REPEATED,
			goto_timer_handler);
	EQ_ERROR_CHECK(ret);

	ret = app_timer_create(&manual_timer_id, APP_TIMER_MODE_REPEATED,
			manual_timer_handler);
	EQ_ERROR_CHECK(ret);

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

	ctx_set_mode(MODE_OFF);

	return ret;
}

static void char_write_handler(uint16_t uuid, uint16_t len, uint16_t offset, uint8_t *data)
{
	switch (uuid) {
	case UUID_POS:
		if (offset + len > 2 * sizeof(*ctx.pos))
			goto too_long;
	
		memcpy(ctx.pos + offset, data, len);
		break;
	case UUID_DEST:
		if (offset + len > 2 * sizeof(*ctx.dest))
			goto too_long;

		memcpy(ctx.dest + offset, data, len);
		break;
	case UUID_MODE:
		if (offset != 0 || len != sizeof(ctx.mode))
			goto too_long;

		set_mode(*data);
		break;
	case UUID_REVERSE:
		if (offset != 0 || len != sizeof(ctx.reverse))
			goto too_long;

		set_reverse(*data);
		break;
	default:
		break;
	}

	return;

too_long:
	NRF_LOG_INFO("Trying to write too much data, ignoring\r\n");
}

static int services_init()
{
	int ret;
	ble_uuid_t uuid;

	uuid.uuid = SERVICE_UUID;
	ret = sd_ble_uuid_vs_add(&base_uuid, &uuid.type);
	EQ_ERROR_CHECK(ret);

	ret = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
			&uuid, &ctx.service_handle);
	EQ_ERROR_CHECK(ret);

/* TODO ERROR handling */
	ret = eq_add_characteristic(ctx.service_handle, 2 * sizeof(uint32_t), (uint8_t *)ctx.pos[0],
			UUID_POS, PERM_WRITE | PERM_READ, &ctx.char_handle[0]);

	ret = eq_add_characteristic(ctx.service_handle, 2 * sizeof(uint32_t), (uint8_t *)ctx.dest,
			UUID_DEST, PERM_WRITE | PERM_READ, &ctx.char_handle[1]);

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.mode), (uint8_t *)&ctx.mode,
			UUID_MODE, PERM_WRITE | PERM_READ, &ctx.char_handle[2]);

	ret = eq_add_characteristic(ctx.service_handle, sizeof(ctx.reverse), (uint8_t *)&ctx.reverse,
			UUID_REVERSE, PERM_WRITE | PERM_READ, &ctx.char_handle[3]);

	set_write_handler(char_write_handler);

	return ret;
}

void manual_button_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	static int b1 = 0, b2 = 0;

	switch (pin) {
	case KEY1:
		b1 = !b1;
		break;
	case KEY2:
		b2 = !b2;
		break;
	default:
		NRF_LOG_INFO("Unhandled button event on pin %d\n\r", pin);
		return;
	}

	NRF_LOG_INFO("Button input pin %d action %d\n\r", pin, action);

	if (b1 ^ b2) {
		set_mode(MODE_MANUAL);
		if (b1)
			ra_set_dir(DIR_PROGRADE);
		else
			ra_set_dir(DIR_RETROGRADE);
		nrf_gpio_pin_set(LED1);
	} else {
		nrf_gpio_pin_clear(LED1);
		set_mode(MODE_TRACKING);
	}
}

void buttons_init()
{
	int ret;

	nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(1);
	config.pull = NRF_GPIO_PIN_PULLUP;
	ret = nrf_drv_gpiote_in_init(KEY1, &config, manual_button_handler);
	EQ_ERROR_CHECK(ret);
	ret = nrf_drv_gpiote_in_init(KEY2, &config, manual_button_handler);
	EQ_ERROR_CHECK(ret);

	nrf_drv_gpiote_in_event_enable(KEY1, true);
	nrf_drv_gpiote_in_event_enable(KEY2, true);
}

int main(void)
{
	int ret;

	ret = NRF_LOG_INIT(NULL);
	EQ_ERROR_CHECK(ret);

	ret = nrf_drv_gpiote_init();
	EQ_ERROR_CHECK(ret);

	NRF_LOG_INFO("\r\nInitializing...\r\n")
	/* Need to start the timer module */
	APP_TIMER_INIT(TIMER_PRESCALER, TIMER_OP_QUEUE_SIZE, 0);
    	ret = bsp_init(BSP_INIT_LED, APP_TIMER_TICKS(100, TIMER_PRESCALER), NULL);
	EQ_ERROR_CHECK(ret);

	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(LED0, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(LED1, NRF_GPIO_PIN_DIR_OUTPUT);
	nrf_gpio_pin_dir_set(KEY1, NRF_GPIO_PIN_DIR_INPUT);
	nrf_gpio_pin_dir_set(KEY2, NRF_GPIO_PIN_DIR_INPUT);
	NRF_LOG_INFO("\r\nInitializing BLE...\r\n")
	ble_init();
	services_init();
	NRF_LOG_INFO("\r\nInitializing timers...\r\n")
	timer_init();

	NRF_LOG_INFO("\r\nInitializing motors...\r\n")
	motor_init();
	buttons_init();

	/* Starting bluetooth service */
	ret = ble_advertising_start(BLE_ADV_MODE_FAST);
	EQ_ERROR_CHECK(ret);

	set_mode(MODE_TRACKING);

	NRF_LOG_INFO("\r\nInitialization completed\r\n")

	while (1)
	{
		if (NRF_LOG_PROCESS() == false)
			sd_app_evt_wait();
	}

	return 0;
}
