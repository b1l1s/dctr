#include "file.h"
#include "ctrff/ff.h"
#include "ctrff/ctrff.h"
#include "mem.h"
#include "decrypt.h"

#include <string.h>
#include "ctr/draw.h"
#include "ctr/printf.h"
#include "ctr/crypto.h"

struct file_s
{
	FIL handle;
};

#define FILE_MAX				0x10
#define SECTOR_SIZE				0x200
static file_s* files;

u8 nand_ctr[AES_BLOCK_SIZE];

uint32_t ctrNandToRaw(void* data, uint32_t sector)
{
	sector += 0x0B95CA00 / SECTOR_SIZE;

	return sector;
}

uint32_t ctrNandDecrypt(void* data, void* buffer, uint32_t sector, size_t count)
{
	u8 ctr[AES_BLOCK_SIZE];
	memcpy(ctr, nand_ctr, AES_BLOCK_SIZE);
	aes_advctr(ctr, sector * (SECTOR_SIZE / AES_BLOCK_SIZE), AES_INPUT_BE | AES_INPUT_NORMAL);

	aes_use_keyslot(0x04);
	aes(buffer, buffer, count * (SECTOR_SIZE / AES_BLOCK_SIZE), ctr, AES_CTR_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);

	return 0;
}

u32 file_init()
{
	files = (file_s*) memmgr_alloc(sizeof(file_s) * FILE_MAX);
	memset(files, 0, sizeof(file_s) * FILE_MAX);

	u32 res = ctrff_init((void*(*)(size_t))memmgr_alloc);
	if(res == 0)
	{
		res = getNandCtr(nand_ctr);
		if(res == 0)
		{
			ctrff_nand_ctx ctx;
			ctx.toRaw = ctrNandToRaw;
			ctx.decrypt = ctrNandDecrypt;
			ctx.encrypt = ctrNandDecrypt;

			res = ctrff_emunand_mount(&ctx);
			res = ctrff_nand_mount(&ctx);
		}
	}

	return res;
}

file_s* file_open(const char* path, u32 flags)
{
	file_s* file = NULL;
	int i;
	for(i = 0; i < FILE_MAX; ++i)
	{
		if(files[i].handle.fs == 0)
		{
			file = &files[i];
			if(f_open(&file->handle, path, flags) != FR_OK)
				file = NULL;
			
			break;
		}
	}
	
	return file;
}

void file_close(file_s* file)
{
	f_close(&file->handle);
}

u32 file_getsize(file_s* file)
{
	return f_size(&file->handle);
}

void file_seek(file_s* file, u32 offset)
{
	f_lseek(&file->handle, offset);
}

u32 file_tell(file_s* file)
{
	return file->handle.fptr;
}

size_t file_read(file_s* file, void* buffer, size_t size)
{
	size_t bytes_read = 0;
	f_read(&file->handle, buffer, size, (UINT*)&bytes_read);
	
	return bytes_read;
}

size_t file_write(file_s* file, const void* buffer, size_t size)
{
	size_t bytes_written = 0;
	f_write(&file->handle, buffer, size, (UINT*)&bytes_written);
	
	return bytes_written;
}

u32 dir_list(const char* path, dir_content_s* dirRes, u32 maxRes)
{
	DIR dir;
	FRESULT res = f_opendir(&dir, path);
	if(res != FR_OK)
		return 0;

	FILINFO fileinfo;
	static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
	fileinfo.lfname = lfn;
	fileinfo.lfsize = sizeof(lfn);

	u32 count = 0;
	do
	{
		res = f_readdir(&dir, &fileinfo);
		if(res != FR_OK || fileinfo.fname[0] == 0)
			break;

		char* fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;

		strncpy(dirRes[count].name, fn, DIR_LIST_MAX_CHAR);
		if(fileinfo.fattrib & AM_DIR)
			dirRes[count].isDir = 1;
		else
			dirRes[count].isDir = 0;
	} while(++count < maxRes);

	f_closedir(&dir);

	return count;
}

int dump_to_mem(const char* path, void* buffer, size_t size)
{
	file_s* file = file_open(path, FILE_R);
	if(file)
	{
		if(size == 0)
			size = file_getsize(file);

		file_read(file, buffer, size);
		file_close(file);
	}

	return size;
}

int dump_to_file(const char* path, const void* buffer, size_t size)
{
	file_s* file = file_open(path, FILE_W | FILE_T);
	if(file)
	{
		file_write(file, buffer, size);
		file_close(file);
	}

	return size;
}
