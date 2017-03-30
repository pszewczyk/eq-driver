#include "eq_ble.h"
#include "common.h"

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

ble_uuid128_t base_uuid = {
	{
		0x44, 0x69, 0xd6, 0xdb, 0xfe, 0x22, 0x80, 0x9d,
		0x79, 0x46, 0x72, 0xf6, 0x00, 0x00, 0x00, 0x00
	}
};

static void on_conn_params_evt(ble_conn_params_evt_t *evt)
{
}

static void conn_params_error_handler(uint32_t nrf_error)
{
}

/**
 * Add a characteristic of fixed length with selected permissions
 */
int eq_add_characteristic(uint16_t service, int len, uint8_t *data,
		uint16_t uuid, int perm, ble_gatts_char_handles_t *handle)
{
	int ret;	

	ble_gatts_char_md_t char_md;
	ble_gatts_attr_md_t attr_md;
	ble_gatts_attr_t char_value;
	ble_uuid_t char_uuid;

	memset(&char_md, 0, sizeof(char_md));
	char_md.char_props.read = !!(perm & PERM_READ);
	char_md.char_props.write = !!(perm & PERM_WRITE);
	char_md.char_props.notify = 1;

	char_uuid.type = APP_UUID_TYPE;
	char_uuid.uuid = uuid;

	memset(&attr_md, 0, sizeof(attr_md));
	if (perm & PERM_READ)
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
	
	if (perm & PERM_WRITE)
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

	/* Location of the value structure */
	attr_md.vloc = BLE_GATTS_VLOC_STACK;

	memset(&char_value, 0, sizeof(char_value));
	char_value.p_uuid = &char_uuid;
	char_value.p_attr_md = &attr_md;
	char_value.init_len = len;
	char_value.max_len = len;
	char_value.p_value = data;

	ret = sd_ble_gatts_characteristic_add(service, &char_md,
			&char_value, handle);
	APP_ERROR_CHECK(ret);

	return ret;
}

static void app_handle_write(ble_gatts_evt_write_t *evt)
{
	NRF_LOG_DEBUG("Attribute uuid: %04x, %d bytes written\r\n", evt->uuid.uuid,
			evt->len);
}

static void app_on_ble_evt(ble_evt_t *ble_evt)
{
	switch (ble_evt->header.evt_id)
	{
	case BLE_GAP_EVT_CONNECTED:
		NRF_LOG_INFO("Connected\r\n");
		break;
	case BLE_GAP_EVT_DISCONNECTED:
		NRF_LOG_INFO("Disconnected\r\n");
		break;
	case BLE_GAP_EVT_AUTH_STATUS:
		NRF_LOG_INFO("Authhentication procedure completed with status %d\r\n",
				ble_evt->evt.gap_evt.params.auth_status.auth_status);
		break;
	case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
		NRF_LOG_INFO("Request to calculate an LE Secure Connections DHKey.\r\n");
		break;
	case BLE_GATTS_EVT_TIMEOUT:
		NRF_LOG_INFO("GATT server timeout\r\n");
		break;
	case BLE_GATTC_EVT_TIMEOUT:
		NRF_LOG_INFO("GATT client timeout\r\n");
		break;
	case BLE_GATTS_EVT_WRITE:
		NRF_LOG_INFO("GATT server write\r\n");
		app_handle_write(&ble_evt->evt.gatts_evt.params.write);
		break;
	default:
		NRF_LOG_INFO("Other event: %d: %s\r\n", ble_evt->header.evt_id,
				(uint32_t)(ser_dbg_sd_evt_str_get(ble_evt->header.evt_id)));
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

/** @brief Initialize ble stack */
int ble_init()
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

	ret = ble_advertising_init(&advdata, NULL, &advcfg, on_adv_evt, NULL);
	APP_ERROR_CHECK(ret);

	ret = ble_conn_params_init(&conn_params_init);
	APP_ERROR_CHECK(ret);

	return 0;
}
