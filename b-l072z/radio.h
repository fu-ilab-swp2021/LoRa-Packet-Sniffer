/*
 * header file for lora radio functions
 * Author: Cedric Ressler
 * Date: 09.02.2021
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
#define FILE_START_STACKSIZE 			(THREAD_STACKSIZE_DEFAULT)

/* message type for isr msg */
#define MSG_TYPE_ISR				(0x3456)

/* frequency the radio listens on */
#define FREQ						(868300000)


/*
 * Function: init_radio
 * -------------------------
 * initialize the sx127x driver
 * and start the _recv_thread
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_radio(void);

/*
 * Function: start_sniffing
 * -------------------------
 * sets up a new file on the sd card
 * and tells the radio to write all sniffed packets to the sd card
 *
 * returns: void
 */
void start_sniffing(void);

/*
 * Function: stop_sniffing
 * -------------------------
 * stops the radio from writing sniffed packets to the sd card
 *
 * returns: void
 */
void stop_sniffing(void);


#endif 
