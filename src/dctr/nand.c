#include "nand.h"
#include "mem.h"
#include "ctr/headers.h"
#include "ctr/printf.h"
#include "ctrff.h"
#include "file.h"

#define		NAND_SOFFSET		0x200000
#define		NAND_LAST_SOFFSET	0x600000

void dumpNand(const char* path, int type)
{
	file_s* out = file_open(path, FILE_W | FILE_T);
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
				if(type)
					ctrff_emunand_read(buffer, i, bufferSize / 0x200);
				else
					ctrff_nand_read(buffer, i, bufferSize / 0x200);

				file_write(out, buffer, bufferSize);
			}

			memmgr_free(buffer);
		}

		file_close(out);
	}
}

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
