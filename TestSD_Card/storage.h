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


#define MOUNT_POINT			"/f"

#define FILENAME_MAXLEN		(30U)

#define FSBUF_SIZE		2048U

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
 * Function: stor_file_exists
 * -------------------------
 * tests if a file exists
 *
 * filename: name of the file to test
 *
 * returns: 0 if file doesnt exist
 * 			1 if file exists
 */
int stor_file_exists(char* filename);

/*
 * Function: stor_flush
 * -------------------------
 * write the buffer to the sd card
 *
 * filename: name of the file to write to
 *
 * returns: void
 */
void stor_flush(char* filename);


#endif 
