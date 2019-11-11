include Makefile.config

all:	$(BUILD_DIR)
	$(MAKE) -C src/common 
	$(MAKE) -C src/fivis
	$(MAKE) -C src/mksigset OUTPUT_DIR=$(CURDIR)/$(BUILD_DIR)
	$(MAKE) -C src/cpumon OUTPUT_DIR=$(CURDIR)/$(BUILD_DIR)

$(BUILD_DIR):
	-mkdir $@

.PHONY: clean
clean:
	$(MAKE) clean -C src/common 
	$(MAKE) clean -C src/fivis
	$(MAKE) clean -C src/mksigset
	$(MAKE) clean -C src/cpumon

.PHONY: cleanall
cleanall:
	$(MAKE) cleanall -C src/common 
	$(MAKE) cleanall -C src/fivis
	$(MAKE) cleanall -C src/mksigset
	$(MAKE) cleanall -C src/cpumon
	-$(RM) -r $(BUILD_DIR)
