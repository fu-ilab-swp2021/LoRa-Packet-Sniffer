

#include <thread.h>

#include <net/netdev.h>
#include <net/netdev/lora.h>
#include <net/lora.h>

#include <sx127x.h>
#include <sx127x_params.h>
#include <sx127x_netdev.h>

#include <string.h>

#include <xtimer.h>

#define SX127X_LORA_MSG_QUEUE	(16U)

#define SX127X_STACKSIZE 		(THREAD_STACKSIZE_DEFAULT)

#define MSG_TYPE_ISR			(0x3456)

sx127x_t sx127x;

static char payload[32];

static char stack[SX127X_STACKSIZE];
static kernel_pid_t _recv_pid;



typedef struct {
	uint8_t rssi;
	int8_t snr;
	uint8_t mtype : 3;
	char devAddr[4];
	int8_t adr : 1;
	int8_t adrack_req : 1;
	int8_t ack : 1;
	uint16_t fcnt;
	int8_t fopts_len: 4;
	//fopts sucks to put in in a static way
	uint8_t fport;	
} LoRaWAN_Packet;

typedef struct LoRaWAN_Packet_List {
	LoRaWAN_Packet packet;
	struct LoRaWAN_Packet_List * next;
} LoRaWAN_Packet_List;


void printPacketList(LoRaWAN_Packet_List *head);

LoRaWAN_Packet_List * list = NULL;

LoRaWAN_Packet_List * getLastListItem(LoRaWAN_Packet_List *head){
	if(head == NULL) return (LoRaWAN_Packet_List*)NULL;
	LoRaWAN_Packet_List *item = head;
	
	while(item->next != NULL){
		item = item->next;
	}

	return item;
}

void print_hex(const char *s){
	printf("as string: %s\n", s);
	printf("as hex:");
	while(*s){
		printf("%02x", (char) *s++);

	}
	printf("\n");
}

void print_bin(const char *s, int len){
	int j = 0;
	while(j < len){
		char a = s[j];
		int i;
		for(i = 0; i<8; i++){
			printf("%d", !!((a << i) & 0x80));
		}
		printf(" ");
		j++;
	}
	printf("\n");


}

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

void printPacketList(LoRaWAN_Packet_List *head){
	if(head == NULL) return;
	LoRaWAN_Packet_List *item = head;

	while(item != NULL){
		if(item->packet.mtype == 0b000){
			printf("\tJoin Request Packet:\n");	
		}else if(item->packet.mtype == 0b100){
			printf("\tConfirmed Data Up Packet:\n");	
		}else if(item->packet.mtype == 0b010){
			printf("\tUnconfirmed Data Up Packet:\n");	
		}
		printf("\t\tRSSI: %d\n", item->packet.rssi);
		printf("\t\tSNR: %d\n", item->packet.snr);
		
		printf("\t\tDevice Address:\n\t\t\t");
		print_bin(item->packet.devAddr, 4);

		printf("\t\tADR:\t\t%d\n", item->packet.adr);
		printf("\t\tADRACK_Req:\t%d\n", item->packet.adrack_req);
		printf("\t\tACK:\t\t%d\n", item->packet.ack);

		printf("\t\tFrame Count:\t%d\n", item->packet.fcnt);

		printf("\t\tFOpts_len:\t%d\n", item->packet.fopts_len);
		printf("\t\tFPort:\t%d\n", item->packet.fport);

		puts(" ");

		item = item->next;

	}

	



}

static void _event_cb(netdev_t *dev, netdev_event_t event){
	//puts("callback called");

	if(event == NETDEV_EVENT_ISR) {
		//puts("event = ISR");		

		msg_t msg;
		msg.type = MSG_TYPE_ISR;
		msg.content.ptr = dev;
		
		if(msg_send(&msg, _recv_pid) <= 0){
			puts("Error msg_send");
		}

	}else{


		size_t len;
		netdev_lora_rx_info_t packet_info;

		switch (event) {

			case NETDEV_EVENT_RX_STARTED:
				//puts("received rx_started event");
				break;
			case NETDEV_EVENT_RX_COMPLETE:
				len = dev->driver->recv(dev, NULL, 0, 0);
				dev->driver->recv(dev, payload, len, &packet_info);

				processPacket(payload, len, packet_info.rssi, packet_info.snr);
				
				break;

			default:
				//puts("received unknow event");
				break;

		}



	}

}






void *_recv_thread(void *arg){
	(void)arg;
	puts("start recv thread");
	static msg_t _msg_q[SX127X_LORA_MSG_QUEUE];
	msg_init_queue(_msg_q, SX127X_LORA_MSG_QUEUE);

	while(1){
		msg_t msg;
		msg_receive(&msg);
		if(msg.type == MSG_TYPE_ISR){
			//puts("_recv_thread got ISR msg");
			netdev_t *dev = msg.content.ptr;
			dev->driver->isr(dev);
		}

	}
	

}

void start_listen(void){

	netdev_t *netdev = (netdev_t *)&sx127x;

	const netopt_enable_t single = false;
	netdev->driver->set(netdev, NETOPT_SINGLE_RECEIVE, &single, sizeof(single));
	const uint32_t timeout = 0;
	netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(timeout));
	
	netopt_state_t state = NETOPT_STATE_RX;
	netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));
}

void setup_driver(void){
	
	netdev_t *netdev = (netdev_t *)&sx127x;

	uint8_t lora_bw = LORA_BW_125_KHZ;
	uint8_t lora_sf = 7;
	uint8_t lora_cr = 4;
	netdev->driver->set(netdev, NETOPT_BANDWIDTH, &lora_bw, sizeof(lora_bw));
	netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &lora_sf, sizeof(lora_sf));
	netdev->driver->set(netdev, NETOPT_CODING_RATE, &lora_cr, sizeof(lora_cr));

	uint32_t chan;// = 867500000
	//netdev->driver-set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));

	netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
	printf("Sniffing on channel: %i\n", (int)chan);

}

int main(void){

	puts("LoRaWAN packet sniffer");
	
	//print_bin("ab", 2);

	//sx127x_setup(&sx127x, &sx127x_params[0], 0);
	//sx127x_init(&sx127x);
	//sx127x_init_radio_settings(&sx127x);

	sx127x.params = sx127x_params[0];
	netdev_t *netdev = (netdev_t *)&sx127x;
	//no clue where sx127x_driver comes from
	netdev->driver = &sx127x_driver;

	if(netdev->driver->init(netdev) < 0){
		puts("Error initializing netdev->driver");
		return 1;
	}


	netdev->event_callback = _event_cb;

	_recv_pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv_thread");


	if(_recv_pid <= KERNEL_PID_UNDEF){
		puts("Creation of recv_thread failed");
		return 1;
	}


	setup_driver();
	start_listen();

	while(1){
		xtimer_sleep(60);
		//puts("Minute has passed");
		
	}

	return 0;

} 
