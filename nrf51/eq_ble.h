#ifndef EQ_BLE_H
#define EQ_BLE_H

#include "ble.h"
#include "ble_conn_params.h"
#include "ble_advertising.h"
#include "softdevice_handler.h"
#include "app_timer.h"

#define APP_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN
#define TIMER_PRESCALER 0
#define TIMER_OP_QUEUE_SIZE 8

enum eq_gatt_perm {
	PERM_READ = 0x01,
	PERM_WRITE = 0x02,
};

extern ble_uuid128_t base_uuid;

/**
 * @brief Set characteristic value of fixed length
 */
int eq_set_characteristic_value(uint16_t handle, int len, uint8_t *data);

/**
 * @brief Add fixed-length characteristic to the service
 * @param[in] service Service handle
 * @param[in] len Length of characteristic data
 * @param[in] UUID of the characteristic (short form)
 * @param[in] perm Permission (bitwise OR of eq_gatt_perm enum)
 * @param[out] handle Pointer to handle of the new characteristics
 */
int eq_add_characteristic(uint16_t service, int len, uint8_t *data,
		uint16_t uuid, int perm, ble_gatts_char_handles_t *handle);


int ble_init();

#endif /* EQ_BLE_H */
