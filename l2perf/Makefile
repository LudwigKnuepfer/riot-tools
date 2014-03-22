include ../Makefile.include

# name of your project
export PROJECT = l2perf

CFLAGS += -DSCHEDSTATISTICS

# Modules to include. 
USEMODULE += auto_init
USEMODULE += posix
USEMODULE += uart0
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
include ../Makefile.transceiver

include $(RIOTBASE)/Makefile.include
