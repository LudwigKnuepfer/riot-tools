APPLICATION = gpio-alarm
include ../Makefile.include

# These boards do not suport periph/gpio.h at the time of this writing:
BOARD_BLACKLIST = arduino-mega2560 avsextrem chronos mbed_lpc1768 msb-430 msb-430h msba2 pttu qemu-i386 redbee-econotag telosb wsn430-v1_3b wsn430-v1_4 z1

# Define default pin mappings for some boards:
BOARD_WHITELIST = native
ifneq (,$(filter native,$(BOARD)))
  export PIR_GPIO ?= GPIO_9
  export GPIO_LED_RED ?= GPIO_0
  export GPIO_LED_GREEN ?= GPIO_1

  USEMODULE += native_gpio_virtual

  #CFLAGS += -DNATIVE_GPIO_NUMOF=10
  #CFLAGS += -DTEST_PIR_POLLING
  #USEMODULE += native_gpio_sysfs
endif

USEMODULE += pir
USEMODULE += vtimer

ifneq (,$(GPIO_LED_RED))
  CFLAGS += -DGPIO_LED_RED=$(GPIO_LED_RED)
endif
ifneq (,$(GPIO_LED_GREEN))
  CFLAGS += -DGPIO_LED_GREEN=$(GPIO_LED_GREEN)
endif
ifneq (,$(PIR_GPIO))
  CFLAGS += -DPIR_GPIO=$(PIR_GPIO)
endif

include $(RIOTBASE)/Makefile.include

all-polling: CFLAGS += -DTEST_PIR_POLLING

all-polling: all

all-interrupt: all
