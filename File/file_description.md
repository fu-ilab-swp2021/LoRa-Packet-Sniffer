# Data File
These files are created by the LoRa_Scanner application on the b-l072z board and written to the sd-card. Afterwards they can be analyzed. *example_file* is a computer generated file in this format for testing purposes.

#### The first line in the file will always be:
Time,ChannelFreq,RSSI,SNR,MType,DevAddr,ADR,ADRACKReq,ACK,FCnt,FOptsLen,FOpts,FPort

#### Every following line describes one received packet.

The fields for each record describe the following:

**Time** (long/int):
- time since the start of the recording

**ChannelFreq** (long/int):
- channel frequency the packet was received on (one of [867100000, 867300000, 867500000, 867700000, 867900000, 868100000, 868300000, 868500000])

**RSSI** (int -122 - 0):
- Received Signal Strength Indication in dBm
- -120 no/weak signal; -30 good signal

**SNR** (int -20 - 10):
- Signal to Noise Ratio
- closer to 10 -> signal is less corrupted

**MType** (int 2, 4, or 0):
- message type of the packet
	* 0 - join request
	* 2 - unconfirmed data up packet
	* 4 - confirmed data up packet
	
**DevAddr** (string, 8 chars):
- device address as hex string

**ADR** (int 0 or 1):
- Adaptive Data Rate
- if 1 network should adjust the devices data rate with MAC commands to optimize

**ADRACKReq** (int 0 or 1):
- if 1 acknowledgement required to make sure network still receives the set data rate from adr

**ACK** (int 0 or 1):
- acknowledgement

**FCnt** (int):
- Frame count of the packet

**FOptslen** (int):
- size of fopts

**FOpts** (string):
- frameopts as hex string not preprocessed in any way
- if FOptslen == 0 then FOpts = " ""

**FPort** (int 0 - 255):
	
	* 0 Frame Payload is MAC Commands
	* 1..223 application specific
	* 224..255 Reserved for Future Use


 
