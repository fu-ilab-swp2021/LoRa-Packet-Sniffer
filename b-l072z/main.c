/*
 * LoRa Scanner
 * Author: Cedric Ressler
 * Date: 09.02.2021
 *
 */

#include "lorascanner.h"
#include "radio.h"
#include "storage.h"


static bool running = false;


/*
 * Function: toggle_running
 * -------------------------
 * button press function
 * toggles wether the board is recording the sniffed packets
 *
 * returns: void
 */
void toggle_running(void *arg)
{
	(void) arg;
	puts("Button pressed");
	running = !running;
	if(running){
		//start_sniffing();
		msg_t msg;
		msg.type = MSG_TYPE_START_SNIFFING;

		if(msg_send(&msg, _recv_pid) <= 0){
			puts("Error start sniffing msg_send");
		}

		LED1_ON;
	}else{
		stop_sniffing();
		LED1_OFF;
	}

}


int main(void)
{
	xtimer_sleep(2);

	puts("LoRaWAN packet sniffer");

	init_radio();
	init_storage();

	#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN_B1_PIN)
		gpio_init_int(BTN_B1_PIN, GPIO_IN_PU, GPIO_RISING, toggle_running, NULL);

	#endif

	while(1){
		
	}

	return 0;

} 
