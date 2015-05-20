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

//#define MAX_DIR_CONTENT_PER_PAGE		(TOP_HEIGHT / 8 - 2)
#define MAX_DIR_CONTENT_PER_PAGE		20
#define MAX_CHAR_PER_LINE				(TOP_WIDTH / 8)

typedef struct dirlist_s
{
	u32 count;
	u32 top;
	u32 prevTop;
	u32 sel;
	u32 prevSel;
	dir_content_s* dirContents;
	u32 maxContent;
} dirlist_s;

void dirlist_draw_borders()
{
	draw_rect(SCREEN_TOP, 0, 8, TOP_WIDTH, 4, 0);
	draw_rect(SCREEN_TOP, 0, (MAX_DIR_CONTENT_PER_PAGE + 2) * 8 + 4, TOP_WIDTH, 4, 0);
}

void dirlist_draw_cwd(const char* path)
{
	draw_rect(SCREEN_TOP, 0, 0, TOP_WIDTH, 8, 0x857F5B);

	int offset = strlen(path) - (int)MAX_CHAR_PER_LINE;
	offset = offset > 0 ? offset : 0;
	draw_str(SCREEN_TOP, 0, 0, 0x36352E, path + offset);
}

void dirlist_draw(dirlist_s* dirlist)
{
	const u32 yoff = 16;
	if(dirlist->prevTop != dirlist->top)
	{
		draw_rect(SCREEN_TOP, 0, yoff, TOP_WIDTH, MAX_DIR_CONTENT_PER_PAGE * 8, 0x857F5B);
		draw_rect(SCREEN_TOP, 0, (dirlist->sel - dirlist->top) * 8 + yoff, TOP_WIDTH, 8, 0x757051);
		int i;
		for(i = 0; (i < MAX_DIR_CONTENT_PER_PAGE) && ((i + dirlist->top) < dirlist->count); ++i)
		{
			draw_str(SCREEN_TOP, 0, i * 8 + yoff, 0, dirlist->dirContents[i + dirlist->top].name);
		}

		u32 topnub = 0x857F5B;
		u32 botnub = 0x857F5B;

		if(dirlist->top != 0) // More to show above
			topnub = 0;
		draw_rect(SCREEN_TOP, TOP_WIDTH - 8, 12, 8, 4, topnub);

		if((dirlist->top + MAX_DIR_CONTENT_PER_PAGE) < dirlist->count) // More to show below
			botnub = 0;
		draw_rect(SCREEN_TOP, TOP_WIDTH - 8, (MAX_DIR_CONTENT_PER_PAGE + 2) * 8, 8, 4, botnub);

		dirlist->prevTop = dirlist->top;
	}
	else if(dirlist->prevSel != dirlist->sel)
	{
		draw_rect(SCREEN_TOP, 0, (dirlist->prevSel - dirlist->top) * 8 + yoff, TOP_WIDTH, 8, 0x857F5B);
		draw_str(SCREEN_TOP, 0, (dirlist->prevSel - dirlist->top) * 8 + yoff, 0, dirlist->dirContents[dirlist->prevSel].name);

		draw_rect(SCREEN_TOP, 0, (dirlist->sel - dirlist->top) * 8 + yoff, TOP_WIDTH, 8, 0x757051);
		draw_str(SCREEN_TOP, 0, (dirlist->sel - dirlist->top) * 8 + yoff, 0, dirlist->dirContents[dirlist->sel].name);
	}

	dirlist->prevSel = dirlist->sel;
}

void dirlist_sel_next(dirlist_s* dirlist)
{
	if((dirlist->sel + 1) < dirlist->count)
	{
		if(((dirlist->sel + 1) - dirlist->top) < MAX_DIR_CONTENT_PER_PAGE)
		{
			dirlist->prevSel = dirlist->sel++;
		}
		else
		{
			dirlist->prevSel = dirlist->sel++;
			dirlist->prevTop = dirlist->top++;
		}
	}
}

void dirlist_sel_prev(dirlist_s* dirlist)
{
	if(dirlist->sel > 0)
	{
		if(dirlist->sel != dirlist->top)
		{
			dirlist->prevSel = dirlist->sel--;
		}
		else
		{
			dirlist->prevSel = dirlist->sel--;
			dirlist->prevTop = dirlist->top--;
		}
	}
}

void dirlist_change_path(dirlist_s* dirlist, const char* path)
{
	dirlist->count = dir_list(path, dirlist->dirContents, dirlist->maxContent);
	dirlist->sel = 0;
	dirlist->top = 0;
	dirlist->prevSel = 0;
	dirlist->prevTop = 1;

	dirlist_draw_cwd(path);
}

void blah()
{
	file_s* out = file_open("0:/test.iso", FILE_W | FILE_T);
	if(out)
	{
		const u32 size = 0x2F3E3600;

		file_seek(out, size);
		if(file_tell(out) != size)
		{
			printf("Preallocation failed! Not enough space?\n");
			return;
		}
		file_seek(out, 0);

		u32 bufferSize = 1024 * 1024;
		void* buffer = memmgr_alloc(bufferSize);
		if(buffer)
		{
			int i = 0;
			for(i = 0; i < size / 0x200; i += bufferSize / 0x200)
			{
				ctrff_emunand_read(buffer, i, bufferSize / 0x200);
				file_write(out, buffer, bufferSize);
			}

			memmgr_free(buffer);
		}

		file_close(out);
	}
}

//extern u32 dword_80A6F20;
//extern u32 dword_80A6F58;

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

	const u32 maxEmuCount = 4;
	emunand_s* emus = (emunand_s*) memmgr_alloc(sizeof(emunand_s) * maxEmuCount);
	memset(emus, 0, sizeof(emunand_s) * maxEmuCount);

	u32 emuCount = getEmunands(emus, maxEmuCount);
	for(int i = 0; i < emuCount; ++i)
	{
		printf("Emunand found..\n");
		printf(" Sector : 0x%08X\n", emus[i].offset);
		printf(" Type : %s\n", emus[i].type == EMUNAND_RED ? "RED" : "GW/MT");
	}

	file_s* file = file_open("0:/slot0x25keyX.bin", FILE_R);
	if(file)
	{
		printf("slot0x25keyX.bin found\n");
		file_read(file, slot0x25keyX, AES_BLOCK_SIZE);
		file_close(file);
	}
	else
		printf("Failed to open slot0x25keyX.bin\n");

	//dump_to_file("0:/sdmc.dat", &dword_80A6F20, 37 * 4);

	char path[0x3FF];
	sprintf(path, "%s", "0:");
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
