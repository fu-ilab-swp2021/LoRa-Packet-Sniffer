/*
 * functions for the sd card and file system
 * Author: Cedric Ressler
 * Date: 09.02.2021
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
	.mount_point = MOUNT_POINT,
	.private_data = &fs_desc,
};


/*
 * Function: init_storage
 * -------------------------
 * initialize the sd card
 *
 * returns: 0 if successful
 *			1 if an error occured
 */
int init_storage(void)
{
	fatfs_mtd_devs[fs_desc.vol_idx] = (mtd_dev_t*)&mtd_sdcard_dev;

	if(vfs_mount(&flash_mount) < 0){
		puts("Error initializing sd card");
		return 1;
	}

	return 0;
} 

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
int file_exists_storage(char* filename)
{
	char file[FILENAME_MAXLEN];
	snprintf(file, FILENAME_MAXLEN, "%s%s%s", MOUNT_POINT, "/", filename);

	int f = vfs_open(file, (O_RDONLY), 0);
	if(f < 0){
		return 0;
	}

	vfs_close(f);
	return 1;
}

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
void write_storage(char *filename, char *line, size_t len)
{
	char file[FILENAME_MAXLEN];
	snprintf(file, FILENAME_MAXLEN, "%s%s%s", MOUNT_POINT, "/",filename);

	if (len == 0) {
		return;
	}
        printf("%s\n",file);
	printf("%d,%d\n",sizeof(file),strlen(file));
	int f = vfs_open(file, (O_CREAT | O_WRONLY | O_APPEND), 0);

	if(f < 0){
		printf("Error on vfs_open:\n");
		return;
	}
	int n = vfs_write(f, line, len);
	if(n < 0){
		puts("Error on vfs_write");
	}
	
	vfs_close(f);
}

