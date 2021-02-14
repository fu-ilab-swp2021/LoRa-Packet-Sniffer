/*
 * functions for the lora radio
 * Author: Cedric Ressler
 * Date: 09.02.2021
 *
 */

#include "lorascanner.h"
#include "radio.h"
#include "storage.h"
#include "sx127x_registers.h"


sx127x_t sx127x;

/* array for the payload of the receieved packet */
static char payload[32];

/* receive thread */
static kernel_pid_t _recv_pid;
static char stack_recv[SX127X_STACKSIZE];

/* file start thread */
static kernel_pid_t _file_start_pid;
static char stack_file_start[FILE_START_STACKSIZE];

/* cad thread */
static kernel_pid_t _cad_pid;
static char stack_cad[CAD_STACKSIZE];

/* possible LoRaWAN frequencies */
const uint32_t freq[] = {867100000, 867300000, 867500000, 867700000, 867900000, 868100000, 868300000};

static bool write_to_sd_card = false;
static char filename[30];

static uint32_t start_time;

void start_listen(uint32_t channel);
void setup_driver(void);
void * _recv_thread(void *arg);
void *_file_start_thread(void *arg);
bool radio_is_channel_active(sx127x_t *dev, uint32_t freq, int16_t rssi_threshold);
void *_cad_thread(void *arg);
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
		bool res;

		uint32_t chan;
		dev->driver->get(dev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));

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
			case NETDEV_EVENT_CAD_DONE:
				res = ((sx127x_t *)dev)->_internal.is_last_cad_success;
				if(res){
					/* cad success, receive packet */
				}

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
	
	
	_recv_pid = thread_create(stack_recv, sizeof(stack_recv), THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv_thread");
	if(_recv_pid <= KERNEL_PID_UNDEF){
		puts("Creation of recv_thread failed");
		return 1;
	}

	setup_driver();
	start_listen(FREQ);

	_cad_pid = thread_create(stack_cad, sizeof(stack_cad), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _cad_thread, NULL, "cad_thread");
	if(_cad_pid <= KERNEL_PID_UNDEF){
		puts("Creation of cad_thread failed");
		return 1;
	}

	


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
	_file_start_pid = thread_create(stack_file_start, sizeof(stack_file_start), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _file_start_thread, NULL, "file_start_thread");
	if(_file_start_pid <= KERNEL_PID_UNDEF){
		puts("Creation of file_start_thread failed");
		return;
	}
}

/*
 * Function: _file_start_thread
 * -------------------------
 * thread function to start the file on the sd card
 * 
 * arg: 
 *
 * returns: void
 */
void *_file_start_thread(void *arg)
{
	(void)arg;
	
	int i = 1;
	snprintf(filename, sizeof filename, "%s_%d", "data", i);
	while(file_exists_storage(filename)){
		i++;
		snprintf(filename, sizeof filename, "%s_%d", "data", i);
	}
	
	char* headerLine = "Time,ChannelFreq,RSSI,SNR,MType,DevAddr,ADR,ADRACKReq,ACK,FCnt,FOptsLen,FOpts,FPort\n";
	write_storage(filename, headerLine, strlen(headerLine));

	start_time = xtimer_now().ticks32;

	write_to_sd_card = true;
	return NULL;
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
void *_recv_thread(void *arg)
{
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
 * Function: _cad_thread
 * -------------------------
 * thread function to detect activity on multiple channels
 * 
 * arg: 
 *
 * returns: void
 */
void *_cad_thread(void *arg)
{
	(void)arg;

	while(1){

		for(uint16_t i = 0; i<sizeof(freq)/sizeof(freq[0]); i++){
			sx127x_set_channel(&sx127x, freq[i]);
			sx127x_start_cad(&sx127x);
					
			
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
		puts("Received Join Request packet");
	}else if(mtype == 0b100 || mtype == 0b010){
		/* packet is a data up */
		/* LoRaWAN data up packets should be at least 12 bytes long */
		if(len < 12) return;
		puts("Received Data packet");
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

	//TODO adjust rssi and snr to be values expected in file format
		
	netdev_t *netdev = (netdev_t *)&sx127x;
	uint32_t chan;
	netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
		

	//TODO remove test prints
	printf("channel: %lu", chan);
	printf("rssi: %u", rssi);
	printf("snr: %d", snr);
	printf("mtype: %u", mtype);
	printf("adr: %d", adr);
	printf("adrack_req: %d", adrack_req);
	printf("ack: %d", ack);
	printf("fcnt: %u", fcnt);
	printf("fopts_len: %d", fopts_len);
	printf("fport: %u", fport);
	printf("\n");


	if(write_to_sd_card){
		char line[150];		
		
		uint32_t now_time = xtimer_now().ticks32;
		uint32_t time_since_start = now_time-start_time;

		
		char devAddrString[8];
		snprintf(devAddrString, sizeof devAddrString, "%02X%02X%02X%02X", (unsigned char)devAddr[0], (unsigned char)devAddr[1], (unsigned char)devAddr[2], (unsigned char)devAddr[3]);	
	
		//file format csv
		//Time,ChannelFreq,RSSI,SNR,MType,DevAddr,ADR,ADRACKReq,ACK,FCnt,FOptsLen,FOpts,FPort	
		if(fopts_len == 0){
		 	snprintf(line, sizeof line, "%lu,%lu,%u,%d,%u,%s,%d,%d,%d,%u,%d,%s,%u\n", time_since_start, chan, rssi, snr, mtype, devAddrString, adr, adrack_req, ack, fcnt, fopts_len, " ", fport);
			
			//TODO remove test prints
			printf("time_since_start: %lu", time_since_start);
			printf("channel: %lu", chan);
			printf("rssi: %u", rssi);
			printf("snr: %d", snr);
			printf("mtype: %u", mtype);
			printf("devAddrString: %s", devAddrString);
			printf("adr: %d", adr);
			printf("adrack_req: %d", adrack_req);
			printf("ack: %d", ack);
			printf("fcnt: %u", fcnt);
			printf("fopts_len: %d", fopts_len);
			printf("fopts: %s", " ");
			printf("fport: %u", fport);
			printf("\n");
		
		}else{
			char foptsString[fopts_len*2+1];
			for(int i = 0; i<fopts_len; i++){
				char hex[3];
				snprintf(hex, sizeof hex, "%02X", (unsigned char)fopts[i]);
				strncpy(foptsString+i*2, hex, 2);
			}	
			strncpy(foptsString+fopts_len*2, "\0", 1);			

			snprintf(line, sizeof line, "%lu,%lu,%u,%d,%u,%s,%d,%d,%d,%u,%d,%s,%u\n", time_since_start, chan, rssi, snr, mtype, devAddrString, adr, adrack_req, ack, fcnt, fopts_len, foptsString, fport);

			//TODO remove test prints
			printf("time_since_start: %lu", time_since_start);
			printf("channel: %lu", chan);
			printf("rssi: %u", rssi);
			printf("snr: %d", snr);
			printf("mtype: %u", mtype);
			printf("devAddrString: %s", devAddrString);
			printf("adr: %d", adr);
			printf("adrack_req: %d", adrack_req);
			printf("ack: %d", ack);
			printf("fcnt: %u", fcnt);
			printf("fopts_len: %d", fopts_len);
			printf("fopts: %s", foptsString);
			printf("fport: %u", fport);
			printf("\n");

		}

		//TODO remove test prints
		printf("line to be written to sd-card:\n");
		printf("%s\n", line);
		puts(" ");



		write_storage(filename, line, strlen(line));

	}
}

