/*
 * header for the sd card and file system
 * Author: Cedric Ressler
 * Date: 09.02.2021
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

#define FILENAME_MAXLEN		(11U)

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
 * Function: file_exists_storage
 * -------------------------
 * tests if a file exists
 *
 * filename: name of the file to test
 *
 * returns: 0 if file doesnt exist
 * 			1 if file exists
 */
int file_exists_storage(char* filename);

/*
 * Function: write_storage
 * -------------------------
 * write the line to the file on the sd card
 *
 * filename: 	name of the file to write to
 * line: 		line to be written to file
 * len:			length of line
 *
 * returns: void
 */
void write_storage(char *filename, char *line, size_t len);


#endif 
