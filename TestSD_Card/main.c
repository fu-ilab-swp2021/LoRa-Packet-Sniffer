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

static bool running = false;


void toggle_running(void *arg)
{
	(void) arg;
	running = !running;
	puts("Button pressed");

}

int main(void)
{
	xtimer_sleep(2);
	printf("SD Card test application\n");

	int res=init_storage();

	if(res==1){
	printf("error\n");
	}

	#if defined(MODULE_PERIPH_GPIO_IRQ) && defined(BTN_B1_PIN)
		gpio_init_int(BTN_B1_PIN, GPIO_IN_PU, GPIO_RISING, toggle_running, NULL);

	#endif

	char fileName[30] = "testFile";
	printf("Does File exist: %d \n", stor_file_exists(fileName));

	while(1){
		if(running){
			/* write to sd card */

			char testLine[30] = "this is a test line\n";
		        printf("test1 %d\n", sizeof(testLine));
			stor_write_ln(testLine, sizeof(testLine));
		        printf("test2\n");
			stor_flush(fileName);
		        printf("test3\n");
			printf("Does File exist: %d \n", stor_file_exists(fileName));

		}

		xtimer_usleep(50000);
	}

	return 0;
}
