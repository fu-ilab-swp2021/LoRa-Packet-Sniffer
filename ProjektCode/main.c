/*
 * LoRa Scanner
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#include "lorascanner.h"
#include "radio.h"
#include "storage.h"


int main(void){

	puts("LoRaWAN packet sniffer");

	init_radio();
	init_storage();

	while(1){
		
	}

	return 0;

} 
