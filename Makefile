descend = \
	+mkdir -p $(OUTPUT)$(1) && \
	$(MAKE) $(COMMAND_O) subdir=$(if $(subdir),$(subdir)/$(1),$(1)) $(PRINT_DIR) -C $(1) $(2)

help:
	@echo 'Possible targets:'
	@echo '  gptp              - gptp daemon for linux'
	@echo ''
	@echo '  libgptp      - build libgptp'
	@echo '  libgptp_test     - libgptp_test application'
	@echo ''
	@echo 'Cleaning targets:'
	@echo ''
	@echo '  all of the above with the "_clean" string appended cleans'
	@echo '    the respective build directory.'
	@echo '  clean: a summary clean target to clean _all_ folders'
	@echo ''

gptp:
	$(call descend,daemons/$@/linux/build/)

gptp_clean:
	$(call descend,daemons/gptp/linux/build/,clean)

libgptp:
	$(call descend,lib/libgptp)

libgptp_clean:
	$(call descend,lib/libgptp/,clean)

libgptp_test:
	$(call descend,examples/$@)

libgptp_test_clean:
	$(call descend,examples/libgptp_test/,clean)

all: gptp libgptp libgptp_test

clean: gptp_clean libgptp_clean libgptp_test_clean

.PHONY: FORCE
