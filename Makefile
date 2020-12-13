ENV := d1mini
OTA_HOSTNAME := cancel.lan
TOPLEVEL_SOURCE_DIRS := \
	src \
	esp-scripts \
	pio \
	.pio \

-include esp-scripts/main.mk
