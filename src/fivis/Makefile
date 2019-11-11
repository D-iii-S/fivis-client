SOURCES = $(wildcard *.c)

SHARED_LIBS := curl
INCLUDE_DIRS := ../../include

LIBRARY_BASE := fivis
LIBRARY_STATIC := $(LIBRARY_BASE).a
LIBRARY_SHARED := lib$(LIBRARY_BASE).so


include ../../Makefile.config
include ../../Makefile.common

all:	$(BUILD_DIR) $(OUTPUT_DIR) $(OUTPUT_DIR)/$(LIBRARY_STATIC) $(OUTPUT_DIR)/$(LIBRARY_SHARED)
