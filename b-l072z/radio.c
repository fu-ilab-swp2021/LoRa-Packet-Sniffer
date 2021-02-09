/*
 * functions for the lora radio
 * Author: Cedric Ressler
 * Date: 09.02.2021
 *
 */

#include "lorascanner.h"
#include "radio.h"
#include "storage.h"


sx127x_t sx127x;

/* array for the payload of the receieved packet */
static char payload[32];

/* receive thread */
static char stack_recv[SX127X_STACKSIZE];
static kernel_pid_t _recv_pid;

/* possible LoRaWAN frequencies */
const uint32_t freq[] = {867100000, 867300000, 867500000, 867700000, 867900000, 868100000, 868300000, 868500000};

static bool write_to_sd_card = false;
static char filename[30];

static uint64_t start_time;

void start_listen(uint32_t channel);
void setup_driver(void);
void * _recv_thread(void *arg);
void processPacket(char *payload, int len, uint8_t rssi, int8_t snr);


/*
 * Function: _event_cb
 * -------------------------
 * event callback for netdev events
 * 
 * dev: 
 * event:  
 *
 * returns: void
 */
static void _event_cb(netdev_t *dev, netdev_event_t event)
{
	
	if(event == NETDEV_EVENT_ISR) {
		/* send msg to receive thread that ISR was received */
		msg_t msg;
		msg.type = MSG_TYPE_ISR;
		msg.content.ptr = dev;
		
		if(msg_send(&msg, _recv_pid) <= 0){
			puts("Error isr msg_send");
		}

	}else{
		size_t len;
		netdev_lora_rx_info_t packet_info;

		switch (event) {
			/* receive started event */
			case NETDEV_EVENT_RX_STARTED:
				
				break;
			/* receive completed event */
			case NETDEV_EVENT_RX_COMPLETE:
				len = dev->driver->recv(dev, NULL, 0, 0);
				dev->driver->recv(dev, payload, len, &packet_info);

				processPacket(payload, len, packet_info.rssi, packet_info.snr);

				break;
			default:
				
				break;

		}



	}

}

/*
 * Function: init_radio
 * -------------------------
 * initialize the sx127x driver, setting the event callback and setting the default parameter
 * and start the _recv_thread
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_radio(void)
{

	sx127x.params = sx127x_params[0];
	netdev_t *netdev = (netdev_t *)&sx127x;
	netdev->driver = &sx127x_driver;

	if(netdev->driver->init(netdev) < 0){
		puts("Error initializing netdev->driver");
		return 1;
	}


	netdev->event_callback = _event_cb;
	
	
	_recv_pid = thread_create(stack_recv, sizeof(stack_recv), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv_thread");
	if(_recv_pid <= KERNEL_PID_UNDEF){
		puts("Creation of recv_thread failed");
		return 1;
	}

	setup_driver();
	start_listen(FREQ);

	return 0;
} 

/*
 * Function: start_sniffing
 * -------------------------
 * sets up a new file on the sd card
 * and tells the radio to write all sniffed packets to the sd card
 *
 * returns: void
 */
void start_sniffing(void)
{
	int i = 1;
	snprintf(filename, sizeof filename, "%s_%d", "lora_data", i);
	while(file_exists_storage(filename)){
		i++;
		snprintf(filename, sizeof filename, "%s_%d", "lora_data", i);
	}

	char* headerLine = "Time,ChannelFreq,RSSI,SNR,MType,DevAddr,ADR,ADRACKReq,ACK,FCnt,FOptsLen,FOpts,FPort";
	write_storage(filename, headerLine, sizeof(headerLine));

	start_time = xtimer_now64().ticks64;

	write_to_sd_card = true;
}

/*
 * Function: stop_sniffing
 * -------------------------
 * stops the radio from writing sniffed packets to the sd card
 *
 * returns: void
 */
void stop_sniffing(void)
{
	write_to_sd_card = false;
}

/*
 * Function: start_listen
 * -------------------------
 * setup the radio to receive mode on the given channel
 * 
 * channel: channel on which to listen
 *
 * returns: void
 */
void start_listen(uint32_t channel){

	netdev_t *netdev = (netdev_t *)&sx127x;

	const netopt_enable_t single = false;
	netdev->driver->set(netdev, NETOPT_SINGLE_RECEIVE, &single, sizeof(single));
	const uint32_t timeout = 0;
	netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(timeout));
	netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &channel, sizeof(channel));

	netopt_state_t state = NETOPT_STATE_RX;
	netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));
}

/*
 * Function: setup_driver
 * -------------------------
 * setup the radio with general settings of bandwidth=125, codingrate=4 and spreadingfactor=7 and channel frequency FREQ
 *
 * returns: void
 */
void setup_driver(void){
	
	netdev_t *netdev = (netdev_t *)&sx127x;

	uint8_t lora_bw = LORA_BW_125_KHZ;
	uint8_t lora_sf = 7;
	uint8_t lora_cr = 4;
	netdev->driver->set(netdev, NETOPT_BANDWIDTH, &lora_bw, sizeof(lora_bw));
	netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &lora_sf, sizeof(lora_sf));
	netdev->driver->set(netdev, NETOPT_CODING_RATE, &lora_cr, sizeof(lora_cr));

	uint32_t chan = FREQ;
	netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));

}

/*
 * Function: _recv_thread
 * -------------------------
 * thread function to receive msg from the isr and cause a new event
 * 
 * arg: 
 *
 * returns: void
 */
void *_recv_thread(void *arg){
	(void)arg;
	static msg_t _msg_q[SX127X_LORA_MSG_QUEUE];
	msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE);

	while(1){
		msg_t msg;
		msg_receive(&msg);
		if(msg.type == MSG_TYPE_ISR){
			netdev_t *dev = msg.content.ptr;
			dev->driver->isr(dev);
		}
	}
}

/*
 * Function: processPacket
 * -------------------------
 * processes the payload of a LoRaWAN packet
 * if board is in sniffing mode, write the processed Packet to the sd card
 * 
 * payload: payload of the LoRaWAN packet
 * len: lenght of the payload (since it may contain 0 bytes)
 * rssi: rssi of the packet
 * snr: snr of the packet
 *
 * returns: void
 */
void processPacket(char *payload, int len, uint8_t rssi, int8_t snr){
	

	char macHeader = payload[0];
	uint8_t mtype = macHeader >> 5;

	/* if mtype not type of Join Request, Un-/Confirmed Data uplink then */
	/* either no LoRaWAN packet or type of packet we're not interested in */
	if(mtype != 0b100 && mtype != 0b000 && mtype != 0b010){
		return;
	}

	if(mtype == 0b000){
		/* packet is a join request */
		/* LoRaWAN join requests should be 23 bytes */
		if(len != 23) return;
	}else if(mtype == 0b100 || mtype == 0b010){
		/* packet is a data up */
		/* LoRaWAN data up packets should be at least 12 bytes long */
		if(len < 12) return;
	}

	char devAddr[4];
	strncpy(devAddr, payload+1, 4);

	char fcntl[1];
	strncpy(fcntl, payload+5, 1);
	
	int8_t adr = *fcntl >> 7;
	int8_t adrack_req = (*fcntl >> 6) & 0b01;
	int8_t ack = (*fcntl >> 5) & 0b001;
	int8_t fopts_len = *fcntl & 0b00001111;

	uint16_t fcnt = payload[6]+(payload[7] << 8);

	char fopts[fopts_len];
	if(fopts_len != 0){
		strncpy(fopts, payload+8, fopts_len);
	}
	
	uint8_t fport = payload[8+fopts_len];

	if(write_to_sd_card){
		char line[150];		
		
		uint64_t now_time = xtimer_now64().ticks64;
		uint64_t time_since_start = now_time-start_time;

		netdev_t *netdev = (netdev_t *)&sx127x;
		uint32_t chan = netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
		
		char devAddrString[8];
		snprintf(devAddrString, sizeof devAddrString, "%02X%02X%02X%02X", (unsigned char)devAddr[0], (unsigned char)devAddr[1], (unsigned char)devAddr[2], (unsigned char)devAddr[3]);	
	
		//file format csv
		//Time,ChannelFreq,RSSI,SNR,MType,DevAddr,ADR,ADRACKReq,ACK,FCnt,FOptsLen,FOpts,FPort	
		if(fopts_len == 0){
			snprintf(line, sizeof line, "%llu,%lu,%u,%d,%u,%s,%d,%d,%d,%u,%d,%s,%u", time_since_start, chan, rssi, snr, mtype, devAddrString, adr, adrack_req, ack, fcnt, fopts_len, " ", fport);
		}else{
			char foptsString[fopts_len*2];
			for(int i = 0; i<fopts_len; i++){
				char hex[2];
				snprintf(hex, sizeof hex, "%02X", (unsigned char)fopts[i]);
				strncpy(foptsString+i*2, hex, 2);
			}
			snprintf(line, sizeof line, "%llu,%lu,%u,%d,%u,%s,%d,%d,%d,%u,%d,%s,%u", time_since_start, chan, rssi, snr, mtype, devAddrString, adr, adrack_req, ack, fcnt, fopts_len, foptsString, fport);
		}	

		

		write_storage(filename, line, sizeof(line));

	}
}

