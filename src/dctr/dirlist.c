#include "dirlist.h"
#include "ctr/draw.h"
#include <string.h>

#define MAX_DIR_CONTENT_PER_PAGE		20
#define MAX_CHAR_PER_LINE				(TOP_WIDTH / 8)

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
		if(((dirlist->sel + 1) - dirlist->top) >= MAX_DIR_CONTENT_PER_PAGE)
			dirlist->prevTop = dirlist->top++;

		dirlist->prevSel = dirlist->sel++;
	}
	else if(dirlist->count)
	{
		dirlist->sel = 0;
		dirlist->top = 0;

		dirlist->prevTop = 0xFFFF;
	}
}

void dirlist_sel_prev(dirlist_s* dirlist)
{
	if(dirlist->sel > 0)
	{
		if(dirlist->sel == dirlist->top)
			dirlist->prevTop = dirlist->top--;

		dirlist->prevSel = dirlist->sel--;
	}
	else if(dirlist->count)
	{
		dirlist->sel = dirlist->count - 1;
		dirlist->top = MAX_DIR_CONTENT_PER_PAGE >= dirlist->count ? 0 : dirlist->count - MAX_DIR_CONTENT_PER_PAGE;

		dirlist->prevTop = 0xFFFF;
	}
}

void dirlist_change_path(dirlist_s* dirlist, const char* path)
{
	dirlist->count = dir_list(path, dirlist->dirContents, dirlist->maxContent);
	dirlist->sel = 0;
	dirlist->top = 0;
	dirlist->prevSel = 0;
	dirlist->prevTop = 0xFFFF;

	dirlist_draw_cwd(path);
}
