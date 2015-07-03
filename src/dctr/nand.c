#include "nand.h"
#include "mem.h"
#include "ctr/headers.h"
#include "ctr/hid.h"
#include "ctr/printf.h"
#include "ctrff/ctrff.h"
#include "file.h"
#include "decrypt.h"

#define		NAND_SOFFSET		0x200000
#define		NAND_LAST_SOFFSET	0x600000
#define		NAND_SECTOR_SIZE	0x000200

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

static const u64 knownTitleIDs[] = {
	0x000400000f980000, // CTRAging
	0x0004000100000002, // f_native
	0x0004000100000102, // f_twl
	0x0004000100000202, // f_agb
	0x0004000100001502, // am
	0x0004000100001602, // camera
	0x0004000100001702, // cfg
	0x0004000100001802, // codec
	0x0004000100001902, // dmnt
	0x0004000100001a02, // dsp
	0x0004000100001b02, // gpio
	0x0004000100001c02, // gsp
	0x0004000100001d02, // hid
	0x0004000100001e02, // i2c
	0x0004000100001f02, // mcu
	0x0004000100002002, // mic
	0x0004000100002102, // pdn
	0x0004000100002202, // ptm
	0x0004000100002302, // spi
	0x0004000100002402, // ac
	0x0004000100002602, // streetpass
	0x0004000100002702, // csnd
	0x0004000100002802, // dlp
	0x0004000100002902, // http
	0x0004000100002a02, // mp
	0x0004000100002b02, // ndm
	0x0004000100002c02, // nim
	0x0004000100002d02, // nwm
	0x0004000100002e02, // soc
	0x0004000100002f02, // ssl
	0x0004000100003102, // ps
	0x0004000100003202, // friends
	0x0004000100003302, // ir
	0x0004000100003402, // boss
	0x0004000100008002, // ns
	0x0004000100008102, // TestMenu
	0x0004000100008a02, // DevErrDi
	0x0004003000008102, // TestMenu
	0x0004013000001902  // dmnt
};

static u64 foundTitleIDs[39] = {0};

void decryptFactoryTitle(u64 tid)
{
	/* 6:  sdmc:/
	 * 16: 0004013000001902
	 * 5:  .ncch
	 * 1:  '\0'
	 */
	char namebuf[6 + 16 + 5 + 1];

	sprintf(namebuf, "sdmc:/%08x%08x.ncch",
			(u32)((tid & 0xFFFFFFFF00000000LLU) >> 32),
			(u32)(tid & 0x00000000FFFFFFFFLLU));

	cxiDecrypt(namebuf);
}

void decryptFactoryTitles()
{
	u64 tid;

	for(unsigned int i = 0;
			i < sizeof(foundTitleIDs)/sizeof(*foundTitleIDs);
			++i)
	{
		if ((tid = foundTitleIDs[i]) == 0)
			continue;

		decryptFactoryTitle(tid);
	}
}

/* This function would be a reverse-engineer's nightmare by sheer length. */
void dumpFactoryTitles(void)
{
	const u32 size = 0x2F3E3600;
	void *buffer;
	u32 bufferSize = 0x800 * NAND_SECTOR_SIZE;
	file_s* ncch = NULL;
	bool overhang = false;
	size_t to_write = 0;
	unsigned int word;
	/* 6:  sdmc:/
	 * 16: 0004013000001902
	 * 5:  .ncch
	 * 1:  '\0'
	 */
	char namebuf[6 + 16 + 5 + 1];
	size_t found_count = 0;

	buffer = memmgr_alloc(bufferSize);
	if(buffer == NULL)
	{
		printf("Unable to allocate memory.\n");
		return;
	}

	for(size_t sector = 0; sector < size / NAND_SECTOR_SIZE; sector += bufferSize / NAND_SECTOR_SIZE)
	{
		ctrff_nand_read(buffer, sector, bufferSize / NAND_SECTOR_SIZE);

		if(overhang)
		{
			/* This branch handles coming from below, when we're in
			 * the middle of dumping an NCCH.
			 */

			if(to_write > bufferSize)
			{
				file_write(ncch, buffer, bufferSize);
				to_write -= bufferSize;
				continue;
			}

			file_write(ncch, buffer, to_write);
			file_close(ncch);

			overhang = false;

			word = to_write / 4;
			to_write = 0;
		}
		else
		{
			/* Normal iteration over the current buffer to find
			 * "NCCH" magic.
			 */
			word = 0;
		}

		/* Find "NCCH" magic */
		for(; word < bufferSize / sizeof(u32); ++word)
		{
			if(((u32*) buffer)[word] != NCCH_MAGIC)
				continue;

			u32* ncch_magic = (u32*) buffer + word;

			/* Check for overflow while jumping to the TID
			 * TODO: hanging NCCH header
			 */
			if(bufferSize < word * 4 + 0x18 + sizeof(u64)) {
				printf("Hanging header; cowardly ignoring\n");
				continue;
			}

			u64 tid = *((u64*) ncch_magic + 3);
			bool found = false;
			for(size_t tid_idx = 0;
					tid_idx < sizeof(knownTitleIDs) / sizeof(*knownTitleIDs);
					++tid_idx)
			{
				if(knownTitleIDs[tid_idx] == tid)
				{
					found = true;
					foundTitleIDs[tid_idx] = tid;
					++found_count;
				}
			}

			if(!found)
				continue;

			sprintf(namebuf, "sdmc:/%08X%08X.ncch",
					(u32)((tid & 0xFFFFFFFF00000000LLU) >> 32),
					(u32)(tid & 0x00000000FFFFFFFFLLU));
			printf("Dumping %s...\n", namebuf + 6);

			/* Open file for writing */
			ncch = file_open(namebuf, FILE_W | FILE_T);

			/* Assumption from here on out: No fragmentation has
			 * occurred. This assumption is reassonable given there
			 * were likely no deletions during the factory write 
			 * that could mess the FAT up.
			 */

			/* "here" points at the NCCH magic.
			 * Go back 0x100 and read the signature.
			 */
			u8 *here = (u8*)ncch_magic;
			if(word * 4 < offsetof(ncch_h, magic))
			{
				/* The signature is in the previous sector that
				 * we've read. Fuck.
				 *
				 * XXX: This never seems to happen on a real unit?
				 *      Untested code ahead!
				 */
				u8 *sigbuf = memmgr_alloc(NAND_SECTOR_SIZE);
				if (sigbuf == NULL) {
					printf("Out of memory, show's over.\n");
					memmgr_free(buffer);
					return;
				}

				size_t missing_sig = offsetof(ncch_h, magic) - word * 4;

				ctrff_nand_read(sigbuf, sector - 1, 1);

				file_write(ncch, sigbuf + NAND_SECTOR_SIZE - missing_sig, missing_sig);
				file_write(ncch, here - missing_sig, 0x100 - missing_sig);
			}
			else
			{
				file_write(ncch, here - offsetof(ncch_h, magic), offsetof(ncch_h, magic));
			}

			/* The signature is out for sure. "here" still points to the NCCH
			 * magic. Get the NCCH size at "here" + 0x4.
			 *
			 * The size is defined in media units, which are 0x200 bytes per unit.
			 */
			to_write = *(ncch_magic + 1) * MEDIA_UNITS;
			/* The size includes the NCCH signature, which we've dumped already. */
			to_write -= offsetof(ncch_h, magic);

			/* If a NCCH spans across sectors clusters, we have an overhang.
			 *
			 * Set a "bookmark" and continue dumping next cycle.
			 */
			if(to_write > bufferSize - word * 4)
			{
				overhang = true;
				/* Write as much as we can */
				file_write(ncch, here, bufferSize - word * 4);
				to_write -= bufferSize - word * 4;
			}
			else
			{
				/* Dump, close file. */
				file_write(ncch, here, to_write);
				printf(".");
				file_close(ncch);
			}
		}
	}

	memmgr_free(buffer);

	if(found_count != 0)
	{
		printf("%u titles found. Decrypt?\n", found_count);
		printf("A = OK, B = cancel\n");
		while(1)
		{
			u32 input = input_wait();
			if(input & HID_A)
			{
				printf("Decrypting factory titles...\n");
				decryptFactoryTitles();
				printf("All factory titles decrypted.\n");
				break;
			}
			else if(input & HID_B)
			{
				printf("Not decrypted.\n");
				break;
			}
		}
	}
	else
		printf("Nothing found..\n");
}
