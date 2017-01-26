#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#define BLE_ADVERTISING_ENABLED 1
#define FSTORAGE_ENABLED 1

/* FSTORAGE values from template file in sdk */
#define FS_QUEUE_SIZE 4 /**< the size of the internal queue */
#define FS_MAX_WRITE_SIZE_WORDS 256 /**< Maximum number of words to be written to flash in a single operation. */
#define FS_OP_MAX_RETRIES 3 /**< Number attempts to execute an operation if the SoftDevice fails */

#define APP_TIMER_ENABLED 1

#define CLOCK_ENABLED 1
#define CLOCK_CONFIG_XTAL_FREQ 255
/* 0 for RC, 1 for XTAL, 2 for synth */
#define CLOCK_CONFIG_LF_SRC 1
#define CLOCK_CONFIG_IRQ_PRIORITY 3

/* For SDK consistency */
#define NRF_CLOCK_LFCLKSRC {.source        = NRF_CLOCK_LF_SRC_XTAL, \
	.rc_ctiv       = 0, \
	.rc_temp_ctiv  = 0, \
	.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

#define PEER_MANAGER_ENABLED 1

/* Values copied from template */
#define FDS_ENABLED 1
#define FDS_OP_QUEUE_SIZE 4
#define FDS_CHUNK_QUEUE_SIZE 8
#define FDS_MAX_USERS 8
#define FDS_VIRTUAL_PAGES 3
#define FDS_VIRTUAL_PAGE_SIZE 256

#define GPIOTE_ENABLED 1
#define GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS 4
#define GPIOTE_CONFIG_IRQ_PRIORITY 3
#define GPIOTE_CONFIG_LOG_ENABLED 0
#define GPIOTE_CONFIG_LOG_LEVEL 3
#define GPIOTE_CONFIG_INFO_COLOR 0
#define GPIOTE_CONFIG_DEBUG_COLOR 0

#endif
