SOURCES = $(wildcard *.c)

SHARED_LIBS := curl
STATIC_LIBS := ../common/build/common.a ../fivis/build/fivis.a 
INCLUDE_DIRS := ../../include

PROGRAM := mksigset

include ../../Makefile.config
include ../../Makefile.common

all:	$(BUILD_DIR) $(OUTPUT_DIR) $(OUTPUT_DIR)/$(PROGRAM)
