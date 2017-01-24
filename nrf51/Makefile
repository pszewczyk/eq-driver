SDK_ROOT = /mnt/storage/nrf51822

# Project-specific
PROJECT_NAME = template_project
TARGET = nrf51822

# SDK-specific
TEMPLATE_PATH = $(SDK_ROOT)/components/toolchain/gcc

OUTPUT_DIRECTORY = build
LINKER_SCRIPT = ${PROJECT_NAME}.ld

SRC_FILES += \
	${SDK_ROOT}/components/toolchain/gcc/gcc_startup_nrf51.S \
	${SDK_ROOT}/components/toolchain/system_nrf51.c \
	main.c

INC_FOLDERS += \
	${SDK_ROOT}/components/device/ \
	${SDK_ROOT}/components/toolchain/ \
	${SDK_ROOT}/components/drivers_nrf/hal/ \
	${SDK_ROOT}/components/drivers_nrf/delay \
	${SDK_ROOT}/components/libraries/util/ \
	${SDK_ROOT}/components/libraries/timer/ \
	${SDK_ROOT}/components/softdevice/s130/headers/ \
	${SDK_ROOT}/components/toolchain/cmsis/include \
	./config

CFLAGS += -DS130
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DNRF51
CFLAGS += -DS130
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DBSP_DEFINES_ONLY
CFLAGS += -DNRF51822
CFLAGS += -DNRF_SD_BLE_API_VERSION=2
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror -O3 -g3
CFLAGS += -mfloat-abi=soft
# keep every function in separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums 

LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T $(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
LDFLAGS += -Wl,--gc-sections # Remove unused sections
LDFLAGS += --specs=nano.specs -lc -lnosys

ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DNRF51
ASMFLAGS += -DS130
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DBSP_DEFINES_ONLY
ASMFLAGS += -DNRF51822
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=2

default: ${TARGET}

include $(TEMPLATE_PATH)/Makefile.common

# Macro from sdk
$(call define_target, $(TARGET))
