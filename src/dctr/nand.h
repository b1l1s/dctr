#ifndef __NAND_H
#define __NAND_H

#include "ctr/types.h"
#include "sdmmc.h"

#ifndef SECTOR_SIZE
#define SECTOR_SIZE				0x200
#endif /*SECTOR_SIZE*/

typedef struct emunand_s
{
	u32 type;
	u32 chiptype;
	u32 head;
	u32 offset;
} emunand_s;

#define 	EMUNAND_RED			0
#define 	EMUNAND_MTGW		1

#define		NAND_CHIP_DC		0
#define		NAND_CHIP_TOSHIBA	1
#define		NAND_CHIP_SAMSUNG	2

#define		NAND_SIZE_TOSHIBA	0x1D7800
#define		NAND_SIZE_SAMSUNG	0x1DD000

u32 getEmunands(emunand_s* nands, u32 max);

#endif /*__NAND_H*/
