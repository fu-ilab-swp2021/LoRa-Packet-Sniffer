/*
 * functions for the sd card and file system
 * Author: Cedric Ressler
 * Date: 03.02.2021
 *
 */

#include "lorascanner.h"
#include "storage.h"



extern sdcard_spi_t sdcard_spi_devs[ARRAY_SIZE(sdcard_spi_params)];
static mtd_sdcard_t mtd_sdcard_dev = {
	.base = {
		.driver = &mtd_sdcard_driver
	},
	.sd_card = &sdcard_spi_devs[0],
	.params = &sdcard_spi_params[0],
};

mtd_dev_t *fatfs_mtd_devs[FF_VOLUMES];

static fatfs_desc_t fs_desc;
static vfs_mount_t flash_mount = {
	.fs = &fatfs_file_system,
	.mount_point = "/f",
	.private_data = &fs_desc,
};




static mutex_t _buflock = MUTEX_INIT;
static char _inbuf[FSBUF_SIZE];
static char _fsbuf[FSBUF_SIZE];
static size_t _inbuf_pos;

/*
 * Function: init_storage
 * -------------------------
 * initialize the sd card
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_storage(void){


	fatfs_mtd_devs[fs_desc.vol_idx] = (mtd_dev_t*)&mtd_sdcard_dev;

	if(vfs_mount(&flash_mount) < 0){
		puts("Error initializing sd card");
		return 1;
	}


	return 0;
} 

/*
 * Function: stor_write_ln
 * -------------------------
 * write line to buffer to be written on sd card on flush
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int stor_write_ln(char *line, size_t len){
	
	mutex_lock(&_buflock);
	if((_inbuf_pos + len) > FSBUF_SIZE){
		puts("buffer full\n");
		mutex_unlock(&_buflock);
		return 1;
	}	
	memcpy(&_inbuf[_inbuf_pos], line, len);
	_inbuf_pos += len;
	mutex_unlock(&_buflock);

	return 0;	

}

/*
 * Function: stor_flush
 * -------------------------
 * write the buffer to the sd card
 *
 * returns: void
 */
void stor_flush(void){

	size_t len;
	char file[FILENAME_MAXLEN] = "/f/testFile";


	mutex_lock(&_buflock);
	memcpy(_fsbuf, _inbuf, _inbuf_pos);
	len = _inbuf_pos;
	_inbuf_pos = 0;
	mutex_lock(&_buflock);

	if (len == 0) {
		return;
	}

	int f = vfs_open(file, (O_CREAT | O_WRONLY | O_APPEND), 0);
	if(f < 0){
		return;
	}
	int n = vfs_write(f, _fsbuf, len);
	(void)n;
	
	vfs_close(f);

}


