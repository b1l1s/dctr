#include "nand.h"
#include "mem.h"
#include "ctr/headers.h"
#include "ctr/printf.h"

#define		NAND_SOFFSET		0x200000
#define		NAND_LAST_SOFFSET	0x600000

u32 getEmunands(emunand_s* nands, u32 max)
{
	u32 count = 0;
	u32 soffset = 0;

	u32 end = sdmmc_sdcard_sectorcount() - NAND_SIZE_SAMSUNG;
	end = NAND_LAST_SOFFSET > end ? end : NAND_LAST_SOFFSET;

	ncsd_h* ncsd = (ncsd_h*) memmgr_alloc(SECTOR_SIZE);
	do
	{
		if(sdmmc_sdcard_readsectors(soffset + 1, 1, ncsd))
			break;
		if(ncsd->magic == NCSD_MAGIC)
		{
			nands[count].type = EMUNAND_RED;
			nands[count].chiptype = NAND_CHIP_DC;
			nands[count].head = soffset + 1;
			nands[count].offset = soffset + 1;
			count++;
			continue;
		}

		if(sdmmc_sdcard_readsectors(soffset + NAND_SIZE_TOSHIBA, 1, ncsd))
			break;
		if(ncsd->magic == NCSD_MAGIC)
		{
			nands[count].type = EMUNAND_MTGW;
			nands[count].chiptype = NAND_CHIP_TOSHIBA;
			nands[count].head = soffset + NAND_SIZE_TOSHIBA;
			nands[count].offset = soffset;
			count++;
			continue;
		}

		if(sdmmc_sdcard_readsectors(soffset + NAND_SIZE_SAMSUNG, 1, ncsd))
			break;
		if(ncsd->magic == NCSD_MAGIC)
		{
			nands[count].type = EMUNAND_MTGW;
			nands[count].chiptype = NAND_CHIP_SAMSUNG;
			nands[count].head = soffset + NAND_SIZE_SAMSUNG;
			nands[count].offset = soffset;
			count++;
			continue;
		}
	} while(count < max && (soffset += NAND_SOFFSET) < end);

	memmgr_free(ncsd);

	return count;
}
