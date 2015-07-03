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

void reboot()
{
	printf("Rebooting\n");

	asm
	(
		"msr cpsr_c, #0xDF\n\t"
		"ldr r0, =0x10000035\n\t"
		"mcr p15, 0, r0, c6, c3, 0\n\t"
		"mrc p15, 0, r0, c2, c0, 0\n\t"
		"mrc p15, 0, r12, c2, c0, 1\n\t"
		"mrc p15, 0, r1, c3, c0, 0\n\t"
		"mrc p15, 0, r2, c5, c0, 2\n\t"
		"mrc p15, 0, r3, c5, c0, 3\n\t"
		"ldr r4, =0x18000035\n\t"
		"bic r2, r2, #0xF0000\n\t"
		"bic r3, r3, #0xF0000\n\t"
		"orr r0, r0, #0x10\n\t"
		"orr r2, r2, #0x30000\n\t"
		"orr r3, r3, #0x30000\n\t"
		"orr r12, r12, #0x10\n\t"
		"orr r1, r1, #0x10\n\t"
		"mcr p15, 0, r0, c2, c0, 0\n\t"
		"mcr p15, 0, r12, c2, c0, 1\n\t"
		"mcr p15, 0, r1, c3, c0, 0\n\t"
		"mcr p15, 0, r2, c5, c0, 2\n\t"
		"mcr p15, 0, r3, c5, c0, 3\n\t"
		"mcr p15, 0, r4, c6, c4, 0\n\t"
		"mrc p15, 0, r0, c2, c0, 0\n\t"
		"mrc p15, 0, r1, c2, c0, 1\n\t"
		"mrc p15, 0, r2, c3, c0, 0\n\t"
		"orr r0, r0, #0x20\n\t"
		"orr r1, r1, #0x20\n\t"
		"orr r2, r2, #0x20\n\t"
		"mcr p15, 0, r0, c2, c0, 0\n\t"
		"mcr p15, 0, r1, c2, c0, 1\n\t"
		"mcr p15, 0, r2, c3, c0, 0\n\t"
		::: "r0", "r1", "r2", "r3", "r4", "r12"
	);

	*(uint32_t*)0x1FFFFFF8 = (uint32_t)((firm_h*)0x24000000)->a11Entry;
	((void (*)())0x0801B01C)();
}

int main()
{
	bootDelay();

	memmgr_init((void*)0x20400000, 8 * 1024 * 1024);

	const u32 top_bg = 0x857F5B;
	const u32 sub_bg = 0x857F5B;
	const u32 sel = 0x757051;

	draw_init((draw_s*)0x23FFFE00);

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
	{
		printf("Failed to open slot0x25keyX.bin\n"
				"YOU NEED THIS FILE EVEN ON 9.X, ELSE\n"
				"7.x CRYPTO WILL FAIL!\n");
	}

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
				u8* ext = strrchr(path, '.');
				if(ext != NULL)
				{
					if(strcasecmp(ext, ".cxi") == 0 || strcasecmp(ext, ".cfa") == 0 || strcasecmp(ext, ".ncch") == 0) //.cxi
						cxiDecrypt(path);
					else if(strcasecmp(ext, ".3ds") == 0 || strcasecmp(ext, ".cci") == 0) // .3ds .cci
						cciDecrypt(path);
					else if(strcasecmp(ext, ".app") == 0)
						cdnDecrypt(path);
				}

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
		else if(input & HID_X)
		{
			/*
			printf("Dumping nand..\n");
			dumpNand("sdmc:/ctrnand.bin", 0);
			printf("Done..\n");
			*/
		}
		else if(input & HID_Y)
		{
			printf("Dumping factory titles. Scanning...\n");
			dumpFactoryTitles();
		}
		else if(input & HID_START)
			reboot();
	}
}
