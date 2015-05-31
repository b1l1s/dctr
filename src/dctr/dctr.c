#include "ctr/hid.h"
#include "ctr/draw.h"
#include "ctr/printf.h"
#include "ctr/console.h"
#include "ctr/headers.h"
#include "ctr/crypto.h"
#include "decrypt.h"
#include "mem.h"
#include "file.h"
#include "nand.h"
#include "dirlist.h"

#include <string.h>

extern u8 slot0x25keyX[AES_BLOCK_SIZE];

__attribute__((naked))
void infloop()
{
	while(1);
}

void overrideIntVector()
{
	*((volatile u32*)0x08000004) = (u32)infloop;
	*((volatile u32*)0x0800000C) = (u32)infloop;
	*((volatile u32*)0x08000014) = (u32)infloop;
	*((volatile u32*)0x0800001C) = (u32)infloop;
	*((volatile u32*)0x08000024) = (u32)infloop;
	*((volatile u32*)0x0800002C) = (u32)infloop;
}

void bootDelay()
{
	asm volatile
	(
		"mov r1, #0xF0000000\n\t"
	"wait_loop9:\n\t"
		"sub r1, r1, #1\n\t"
		"cmp r1, #0\n\t"
		"bgt wait_loop9\n\t"
		"mov r1, #0xF0000000\n\t"
	"wait_loop92:\n\t"
		"sub r1, r1, #1\n\t"
		"cmp r1, #0\n\t"
		"bgt wait_loop92\n\t"
		:::"r1"
	);
}

int main()
{
	overrideIntVector();
	
	*((volatile u32*)0x10000020) = 0;
	*((volatile u32*)0x10000020) = 0x340;
	
	bootDelay();

	memmgr_init((void*)0x20400000, 8 * 1024 * 1024);

	const u32 top_bg = 0x857F5B;
	const u32 sub_bg = 0x857F5B;
	const u32 sel = 0x757051;

	draw_s draw;
	draw.top_left = (void*)0x20184E60;
	draw.top_right = (void*)0x20282160;
	draw.sub = (void*)0x202118E0;
	draw_init(&draw);

	console_init(0x000000, sub_bg);
	draw_clear_screen(SCREEN_TOP, top_bg);

	u32 res;
	if((res = file_init()) != 0)
	{
		printf("Error : %d\n", res);
	}
	else
		printf("File system initialized\n");

	dirlist_s dirlist;
	dirlist.maxContent = 0x40;
	dirlist.dirContents = (dir_content_s*) memmgr_alloc(sizeof(dir_content_s) * dirlist.maxContent);
	memset(dirlist.dirContents, 0, sizeof(dir_content_s) * dirlist.maxContent);

	file_s* file = file_open("sdmc:/slot0x25keyX.bin", FILE_R);
	if(file)
	{
		printf("slot0x25keyX.bin found\n");
		file_read(file, slot0x25keyX, AES_BLOCK_SIZE);
		file_close(file);
	}
	else
		printf("Failed to open slot0x25keyX.bin\n");

	char path[0x3FF];
	sprintf(path, "%s", "sdmc:");
	dirlist_draw_borders();
	dirlist_change_path(&dirlist, path);
	while(1)
	{
		dirlist_draw(&dirlist);
		u32 input = input_wait();
		if(input & HID_DOWN)
		{
			dirlist_sel_next(&dirlist);
		}
		else if(input & HID_UP)
		{
			dirlist_sel_prev(&dirlist);
		}
		else if((input & HID_A) && dirlist.count)
		{
			u32 len = strlen(path);
			sprintf(path + len, "/%s", dirlist.dirContents[dirlist.sel].name);
			if(dirlist.dirContents[dirlist.sel].isDir)
				dirlist_change_path(&dirlist, path);
			else
			{
				u8* ext = path + (strlen(path) - 4);
				if(strcasecmp(ext, ".cxi") == 0) //.cxi
					cxiDecrypt(path);
				else if(strcasecmp(ext, ".3ds") == 0 || strcasecmp(ext, ".cci") == 0) // .3ds .cci
					cciDecrypt(path);
				else if(strcasecmp(ext, ".app") == 0)
					cdnDecrypt(path);

				*(path + len) = 0;
			}
		}
		else if(input & HID_B)
		{
			char* lastSlash = strrchr(path,'/');
			if(lastSlash)
			{
				*lastSlash = 0;
				dirlist_change_path(&dirlist, path);
			}
		}
	}
}
