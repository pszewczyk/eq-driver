SDK_ROOT = /home/p.szewczyk/syf/nrf51

# Project-specific
PROJECT_NAME = template_project
TARGET = nrf51822

# SDK-specific
TEMPLATE_PATH = $(SDK_ROOT)/components/toolchain/gcc

OUTPUT_DIRECTORY = build
LINKER_SCRIPT = ${PROJECT_NAME}.ld

SRC_FILES += \
	main.c \
	${SDK_ROOT}/components/toolchain/gcc/gcc_startup_nrf51.S \
	${SDK_ROOT}/components/toolchain/system_nrf51.c \

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

CFLAGS += -DNRF51
CFLAGS += -DNRF51822
CFLAGS += -DS130

LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T $(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
LDFLAGS += -Wl,--gc-sections # Remove unused sections
LDFLAGS += --specs=nano.specs -lc -lnosys

default: ${TARGET}

include $(TEMPLATE_PATH)/Makefile.common

# Macro from sdk
$(call define_target, $(TARGET))
