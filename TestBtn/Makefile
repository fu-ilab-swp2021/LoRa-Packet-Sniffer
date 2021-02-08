APPLICATION=Btn_Test

BOARD ?= b-l072z-lrwan1

DRIVER ?= sx1276

LORA_REGION ?= EU868

USEMODULE += xtimer
USEMODULE += $(DRIVER)

#btn
USEMODULE += periph_gpio_irq

DEVELHELP ?= 1

RIOTBASE ?= $(CURDIR)/../../RIOT

include $(RIOTBASE)/Makefile.include  
