#ifndef __FILE_H
#define __FILE_H

#include "ctr/types.h"

#define FILE_R		0x01 // FA_READ
#define FILE_W		0x03 // FA_READ | FA_WRITE
#define FILE_T		0x08 // FA_CREATE_ALWAYS

#define DIR_LIST_MAX_CHAR	0xFF

typedef struct file_s file_s;

typedef struct dir_content_s
{
	char name[DIR_LIST_MAX_CHAR];
	u32 isDir;
} dir_content_s;

u32 file_init();
void file_deinit();

file_s* file_open(const char* path, u32 flags);
void file_close(file_s* file);
u32 file_getsize(file_s* file);
void file_seek(file_s* file, u32 offset);
u32 file_tell(file_s* file);

size_t file_read(file_s* file, void* buffer, size_t size);
size_t file_write(file_s* file, const void* buffer, size_t size);

u32 dir_list(const char* path, dir_content_s* dirRes, u32 maxRes);

/**Utility functions**/
int dump_to_mem(const char* path, void* buffer, size_t size);
int dump_to_file(const char* path, const void* buffer, size_t size);

#endif /*__FILE_H*/
