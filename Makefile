ENV := d1mini
OTA_HOSTNAME := 10.0.0.200
TOPLEVEL_SOURCE_DIRS := \
	src \
	esp-scripts \
	pio \
	.pio \

-include esp-scripts/main.mk

################################################################################
TEST_OUTDIR := out

.PHONY: test
test: | $(TEST_OUTDIR)
	$(CXX) -std=c++17 -DUNIT_TESTS -Isrc -o $(TEST_OUTDIR)/measures_storage ./src/measures_storage.cpp

$(TEST_OUTDIR):
	mkdir -p $@

.PHONY: clean
clean-test:
	rm -rf $(TEST_OUTDIR)
