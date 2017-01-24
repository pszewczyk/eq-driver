#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#define BLE_ADVERTISING_ENABLED 1
#define FSTORAGE_ENABLED 1

/* FSTORAGE values from template file in sdk */
#define FS_QUEUE_SIZE 4 /**< the size of the internal queue */
#define FS_MAX_WRITE_SIZE_WORDS 256 /**< Maximum number of words to be written to flash in a single operation. */
#define FS_OP_MAX_RETRIES 3 /**< Number attempts to execute an operation if the SoftDevice fails */

#define APP_TIMER_ENABLED 1

#endif
