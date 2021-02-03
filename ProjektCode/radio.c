/*
 * functions for the lora radio
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#include "lorascanner.h"
#include "radio.h"

/* head of the LoRaWAN_Packet_List */
LoRaWAN_Packet_List * list = NULL;

sx127x_t sx127x;

/* array for the payload of the receieved packet */
static char payload[32];

/* receive thread */
static char stack_recv[SX127X_STACKSIZE];
static kernel_pid_t _recv_pid;

/* channel activity detection thread */
static char stack_cad[CAD_STACKSIZE];
static kernel_pid_t _cad_pid;

/* possible LoRaWAN frequencies */
const uint32_t freq[] = {867100000, 867500000, 867700000, 867900000, 868100000, 868300000, 868500000};


void * _recv_thread(void *arg);
void * _cad_thread(void *arg);


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
static void _event_cb(netdev_t *dev, netdev_event_t event){
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
 * and start the _recv_thread and _cad_thread
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_radio(void){

	sx127x.params = sx127x_params[0];
	netdev_t *netdev = (netdev_t *)&sx127x;
	netdev->driver = &sx127x_driver;

	if(netdev->driver->init(netdev) < 0){
		puts("Error initializing netdev->driver");
		return 1;
	}


	netdev->event_callback = _event_cb;
	
	setup_driver();
	start_listen(868300000);

	_recv_pid = thread_create(stack_recv, sizeof(stack_recv), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv_thread");
	if(_recv_pid <= KERNEL_PID_UNDEF){
		puts("Creation of recv_thread failed");
		return 1;
	}

	_cad_pid = thread_create(stack_cad, sizeof(stack_cad), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _cad_thread, NULL, "cad_thread");
	if(_cad_pid <= KERNEL_PID_UNDEF){
		puts("Creation of cad_thread failed");
		return 1;
	}

	return 0;
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
	puts("channel set");
}

/*
 * Function: setup_driver
 * -------------------------
 * setup the radio with general settings of bandwidth=125, codingrate=4 and spreadingfactor=7
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

	uint32_t chan = 868300000;
	netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));

}

/*
 * Function: _cad_thread
 * -------------------------
 * thread function for channel activity detection
 * switches through channels - if activity is found sets the radio to listen on that channel
 * 
 * arg: 
 *
 * returns: void
 */
void * _cad_thread(void *arg){
	(void)arg;
	puts("start cad thread");

	while(1){
		
		/* go through possible LoRa channel and test for activity on those channels */
		for(uint16_t i = 0; i<sizeof(freq)/sizeof(freq[0]); i++){
			bool free = sx127x_is_channel_free(&sx127x, freq[i], -80);
			if(!free){
				printf("activity on channel %lu", freq[i]);
				puts("");
				/* start listening on detected channel */
				start_listen(freq[i]);
			}			
			
		}
		
		
		xtimer_msleep(1);

	}
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
	puts("start recv thread");
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
 * adds the fields to a new LoRaWAN_Packet struct and apends that to the list
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

	//if mtype not type of Join Request, Un-/Confirmed Data uplink then 
	//either no LoRaWAN packet or type of packet we're not interested in
	if(mtype != 0b100 && mtype != 0b000 && mtype != 0b010){
		return;
	}

	if(mtype == 0b000){
		//packet is a join request
		//LoRaWAN join requests should be 23 bytes
		if(len != 23) return;
	}else if(mtype == 0b100 || mtype == 0b010){
		//packet is a data up
		//LoRaWAN data up packets should be at least 12 bytes long
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

	uint8_t fport = payload[8+fopts_len];



	LoRaWAN_Packet_List *newItem = (LoRaWAN_Packet_List*) malloc(sizeof(LoRaWAN_Packet_List));
	if(newItem != NULL){
		newItem->packet.rssi = rssi;
		newItem->packet.snr = snr;
		newItem->packet.mtype = mtype;
		strcpy(newItem->packet.devAddr, devAddr);
		newItem->packet.adrack_req = adrack_req;
		newItem->packet.adr = adr;
		newItem->packet.ack = ack;
		newItem->packet.fcnt = fcnt;
		newItem->packet.fopts_len = fopts_len;
		newItem->packet.fport = fport;
		newItem->next = NULL;


		LoRaWAN_Packet_List * lastListItem = getLastListItem(list);
		if(lastListItem == NULL){
			list = newItem;
		}else{
			lastListItem->next = newItem;
		}

	}

	printPacketList(list);
}

