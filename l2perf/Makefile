include ../Makefile.include

# name of your application
export APPLICATION = l2perf

CFLAGS += -DSCHEDSTATISTICS

# Modules to include. 
USEMODULE += uart0
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += config
include ../Makefile.transceiver

include $(RIOTBASE)/Makefile.include
