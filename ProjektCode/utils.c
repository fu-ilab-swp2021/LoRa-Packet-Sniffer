/*
 * utility functions for working with the LoRaWAN_Packet_List
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#include "lorascanner.h"


/*
 * Function: getLastListItem
 * -------------------------
 * gets the last item of the given LoRaWAN_Packet list
 *
 * head: head of the list
 *
 * returns: the last LoRaWAN_Packet of the list
 */
LoRaWAN_Packet_List * getLastListItem(LoRaWAN_Packet_List *head){
	if(head == NULL) return (LoRaWAN_Packet_List*)NULL;
	LoRaWAN_Packet_List *item = head;
	
	while(item->next != NULL){
		item = item->next;
	}

	return item;
}

/*
 * Function: print_bin
 * -------------------------
 * uses printf to print a string in binary
 *
 * s: the string to be printed
 * len:	length of the given string (string doesnt have to be null terminated)
 *
 * returns: void
 */
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

/*
 * Function: printPacketList
 * -------------------------
 * prints the given LoRaWAN_Packet list in a formated style
 *
 * head: head of the list
 *
 * returns: void
 */
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
