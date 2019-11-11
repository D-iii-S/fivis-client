SOURCES = $(wildcard *.c)

SHARED_LIBS := curl pthread
STATIC_LIBS := ../common/build/common.a ../fivis/build/fivis.a 
INCLUDE_DIRS := ../../include

PROGRAM := cpumon

include ../../Makefile.config
include ../../Makefile.common

all:	$(BUILD_DIR) $(OUTPUT_DIR) $(OUTPUT_DIR)/$(PROGRAM)
