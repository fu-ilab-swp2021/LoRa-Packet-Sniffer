/*
 * header file for lora radio functions
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#ifndef LORA_SCANNER_RADIO
#define LORA_SCANNER_RADIO


#include <thread.h>

#include <net/netdev.h>
#include <net/netdev/lora.h>
#include <net/lora.h>

#include <sx127x.h>
#include <sx127x_params.h>
#include <sx127x_netdev.h>


/* number of msg_t to fit in queue */
#define SX127X_LORA_MSG_QUEUE		(16U)

#define SX127X_STACKSIZE 			(THREAD_STACKSIZE_DEFAULT)
#define CAD_STACKSIZE				(THREAD_STACKSIZE_DEFAULT)

/* message type for isr msg */
#define MSG_TYPE_ISR				(0x3456)



/*
 * Function: init_radio
 * -------------------------
 * initialize the sx127x driver
 * and start the _recv_thread and _cad_thread
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_radio(void);

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
void processPacket(char *payload, int len, uint8_t rssi, int8_t snr);

/*
 * Function: start_listen
 * -------------------------
 * setup the radio to receive mode on the given channel
 * 
 * channel: channel on which to listen
 *
 * returns: void
 */
void start_listen(uint32_t channel);

/*
 * Function: setup_driver
 * -------------------------
 * setup the radio with general settings of bandwidth=125, codingrate=4 and spreadingfactor=7
 *
 * returns: void
 */
void setup_driver(void);




#endif 
