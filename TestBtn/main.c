/*
 * main for the button test
 * Author: Cedric Ressler
 * Date: 07.02.2021
 *
 */ 

#include <xtimer.h>

/* imports for button */
#include "board.h"
#include "periph/gpio.h"


void btn_press(void *arg)
{
    (void) arg;
	puts("Button pressed");

	LED0_TOGGLE;
}

int main(void)
{
	puts("Push Button test application");


	#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN_B1_PIN)
		//gpio_init_int(BTN_B1_PIN, GPIO_IN_PD, GPIO_BOTH, btn_press, NULL);
		gpio_init_int(BTN_B1_PIN, GPIO_IN_PU, GPIO_BOTH, btn_press, NULL);

	#endif

	while(1){
		xtimer_msleep(5000);
	}

	return 0;
} 
