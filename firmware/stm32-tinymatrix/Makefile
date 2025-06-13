PROJECT = stm32-tinymatrix
BUILD_DIR = bin

CFILES = $(wildcard *.c)
OPT = -O3
LDLIBS += -lm

# TODO - you will need to edit these two lines!
DEVICE=stm32f030f4p6
OOCD_FILE = board/stm32f4discovery.cfg

# You shouldn't have to edit anything below here.
VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=../libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include ../rules.mk
LDFLAGS += -Wl,--print-memory-usage
include $(OPENCM3_DIR)/mk/genlink-rules.mk
