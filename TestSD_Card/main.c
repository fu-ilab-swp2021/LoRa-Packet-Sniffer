/*
 * main for the sd card and file system test
 * Author: Cedric Ressler
 * Date: 07.02.2021
 *
 */ 

#include <string.h>
#include <xtimer.h>

#include "storage.h"

/* imports for button */
#include "board.h"
#include "periph/gpio.h"

bool running = false;


void toggle_running(void *arg)
{
	(void) arg;
	running = !running;
	puts("Button pressed");

}

int main(void)
{
	puts("SD Card test application");
	init_storage();


	#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN_B1_PIN)
		gpio_init_int(BTN_B1_PIN, GPIO_IN_PU, GPIO_BOTH, toggle_running, NULL);

	#endif

	char fileName[30] = "testFile";
	printf("Does File exist: %d", stor_file_exists(fileName));

	while(1){
		if(running){
			/* write to sd card */

			char testLine[30] = "this is a test line\n";
			stor_write_ln(testLine, sizeof(testLine));
			stor_flush(fileName);
			printf("Does File exist: %d", stor_file_exists(fileName));

		}

		xtimer_msleep(5000);
	}

	return 0;
}
