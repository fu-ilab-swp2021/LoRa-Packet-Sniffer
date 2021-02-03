/*
 * header for the sd card and file system
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#ifndef LORA_SCANNER_STORAGE
#define LORA_SCANNER_STORAGE

#include <fcntl.h>
#include <irq.h>
#include <fmt.h>
#include <vfs.h>
#include <mutex.h>
#include <fs/fatfs.h>
#include <mtd_sdcard.h>
#include <sdcard_spi.h>
#include <sdcard_spi_params.h>



#define FILENAME_MAXLEN		(30U)

#define FSBUF_SIZE		15360U

/*
 * Function: init_storage
 * -------------------------
 * initialize the sd card
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_storage(void);

/*
 * Function: stor_write_ln
 * -------------------------
 * write line to buffer to be written on sd card on flush
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int stor_write_ln(char *line, size_t len);

/*
 * Function: stor_flush
 * -------------------------
 * write the buffer to the sd card
 *
 * returns: void
 */
void stor_flush(void);


#endif 
