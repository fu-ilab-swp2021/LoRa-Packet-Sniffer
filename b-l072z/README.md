## b-l072z ##

This folder contains the C code that runs on the device and the headers and the makefile that are needed. 
The [radio.c](https://github.com/fu-ilab-swp2021/LoRa-Packet-Sniffer/blob/main/b-l072z/radio.c) file is the one that actually manages the device and the [storage.c](https://github.com/fu-ilab-swp2021/LoRa-Packet-Sniffer/blob/main/b-l072z/storage.c) file manages to store the produced .csv files on the SD card. 
The [main.c](https://github.com/fu-ilab-swp2021/LoRa-Packet-Sniffer/blob/main/b-l072z/main.c) file is only for initialization of the device and for the controls to start or stop sniffing packages. 
