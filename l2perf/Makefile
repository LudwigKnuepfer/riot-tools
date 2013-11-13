include ../Makefile.include

# name of your project
export PROJECT = l2perf

ifeq (,$(findstring native,$(BOARD)))
include $(CURDIR)/../Makefile.unsupported
else
ifneq (,$(findstring native,$(BOARD)))
USEMODULE += nativenet
endif

## Modules to include. 

USEMODULE += auto_init
USEMODULE += hwtimer
USEMODULE += transceiver
USEMODULE += posix
USEMODULE += uart0
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

include $(RIOTBASE)/Makefile.include
endif
