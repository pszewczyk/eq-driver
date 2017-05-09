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

#define ADV_INCLUDE_APPEARANCE 0

#define ADV_FAST_ENABLED 1
#define ADV_FAST_INTERVAL 50 /* in 0.65ms units */
#define ADV_FAST_TIMEOUT 120 /* in seconds */

#define MIN_CONN_INTERVAL 100
#define MAX_CONN_INTERVAL 200
#define SLAVE_LATENCY 0
#define CONN_SUP_TIMEOUT 4000

enum eq_gatt_perm {
	PERM_READ = 0x01,
	PERM_WRITE = 0x02,
};

extern ble_uuid128_t base_uuid;

typedef void (*write_handler_t)(uint16_t uuid, uint16_t len, uint16_t offset, uint8_t *data);

/* set handler to be called on write event */
void set_write_handler(write_handler_t handler);

/**
 * @brief Set characteristic value of fixed length
 */
int eq_set_characteristic_value(uint16_t handle, uint16_t len, uint8_t *data);

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
