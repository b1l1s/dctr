#ifndef __DIRLIST_H
#define __DIRLIST_H

#include "ctr/types.h"
#include "file.h"

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

void dirlist_draw_borders();
void dirlist_draw_cwd(const char* path);
void dirlist_draw(dirlist_s* dirlist);
void dirlist_sel_next(dirlist_s* dirlist);
void dirlist_sel_prev(dirlist_s* dirlist);
void dirlist_change_path(dirlist_s* dirlist, const char* path);

#endif /*__DIRLIST_H*/
