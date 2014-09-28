include ../Makefile.include

# name of your application
export APPLICATION = l2perf

CFLAGS += -DSCHEDSTATISTICS -DDEVELHELP

# Modules to include. 
USEMODULE += auto_init
USEMODULE += config
USEMODULE += posix
USEMODULE += uart0
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += net_if
#USEMODULE += rtc
include ../Makefile.transceiver

include $(RIOTBASE)/Makefile.include
