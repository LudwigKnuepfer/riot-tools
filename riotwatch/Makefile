APPLICATION = riotwatch
BOARD ?= chronos

RIOTWATCH_H_INIT ?= $(shell date "+%H")
RIOTWATCH_M_INIT ?= $(shell date "+%M")
RIOTWATCH_S_INIT ?= $(shell date "+%S")

CFLAGS += -DRIOTWATCH_H_INIT=$(RIOTWATCH_H_INIT)
CFLAGS += -DRIOTWATCH_M_INIT=$(RIOTWATCH_M_INIT)
CFLAGS += -DRIOTWATCH_S_INIT=$(RIOTWATCH_S_INIT)

include ../Makefile.include
include $(RIOTBASE)/Makefile.include
