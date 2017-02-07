#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32-ota-https
CFLAGS += -save-temps

include $(IDF_PATH)/make/project.mk

upload:
	scp build/esp32-ota-https.bin andreas@www.classycode.com:/data/httpd/classycode.io/esp32
