APPLICATION=LoRa_Scanner

BOARD ?= b-l072z-lrwan1

DRIVER ?= sx1276

LORA_REGION ?= EU868

USEMODULE += xtimer
USEMODULE += $(DRIVER)
#sd card
USEMODULE += vfs
USEMODULE += fatfs_vfs
USEMODULE += mtd_sdcard

DEVELHELP ?= 1

RIOTBASE ?= $(CURDIR)/../RIOT

include $(RIOTBASE)/Makefile.include
