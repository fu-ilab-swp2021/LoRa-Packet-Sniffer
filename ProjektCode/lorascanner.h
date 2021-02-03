/*
 * header file for general scanner things
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#ifndef LORA_SCANNER
#define LORA_SCANNER


#include <string.h>
#include <xtimer.h>


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
	//fopts as char array?
	uint8_t fport;	
} LoRaWAN_Packet;

typedef struct LoRaWAN_Packet_List {
	LoRaWAN_Packet packet;
	struct LoRaWAN_Packet_List * next;
} LoRaWAN_Packet_List;


/*
 * Function: printPacketList
 * -------------------------
 * prints the given LoRaWAN_Packet list in a formated style
 *
 * head: head of the list
 *
 * returns: void
 */
void printPacketList(LoRaWAN_Packet_List *head);

/*
 * Function: getLastListItem
 * -------------------------
 * gets the last item of the given LoRaWAN_Packet list
 *
 * head: head of the list
 *
 * returns: the last LoRaWAN_Packet of the list
 */
LoRaWAN_Packet_List * getLastListItem(LoRaWAN_Packet_List *head);


#endif
