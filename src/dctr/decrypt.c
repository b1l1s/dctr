#include "ctr/crypto.h"
#include "ctr/printf.h"
#include "decrypt.h"
#include "file.h"
#include "mem.h"

#include <string.h>
u8 slot0x25keyX[AES_BLOCK_SIZE];

typedef struct title_key_s
{
	u8 unk[0x8];
	u8 id[0x8];
	u8 key[0x10];
} title_key_s;

static title_key_s* titleKeys;
static u32 titleKeysCount;

void fileDecrypt(file_decrypt_ctx* ctx)
{
	u32 size = ctx->size;
	u32 batchSize;
	u32 total = 0;

	aes_use_keyslot(ctx->keyslot);

	file_seek(ctx->file, ctx->foffset);
	while(size)
	{
		batchSize = (size < ctx->bufsize) ? size : ctx->bufsize;

		file_read(ctx->file, ctx->buffer, batchSize);

		aes(ctx->buffer, ctx->buffer, batchSize / AES_BLOCK_SIZE, ctx->iv, ctx->mode, ctx->ivMode);

		file_seek(ctx->file, ctx->foffset + total);
		file_write(ctx->file, ctx->buffer, batchSize);

		size -= batchSize;
		total += batchSize;
	}
}

void cxiRegionDecrypt(ncch_h* ncch, file_decrypt_ctx* ctx)
{
	if(ncch->flags[7] & NCCH_FLAG_NOCRYPTO)
	{
		printf("No decryption needed.\n");
		return;
	}

	u8 ctr[AES_BLOCK_SIZE];
	memset(ctr, 0, AES_BLOCK_SIZE);

	ncch_getctr(ncch, ctr, ctx->type);
	aes_change_ctrmode(ctr, AES_INPUT_BE | AES_INPUT_NORMAL, ctx->ivMode);
	ctx->iv = ctr;

	if(ncch->flags[3] & 0x01)
	{
		// The damned 7.x shit
		if(ctx->type == NCCHTYPE_EXEFS)
		{
			printf(" 7.x crypto\n");

			// Decrypt the exeFS header first
			u32 origSize = ctx->size - sizeof(exefs_h);
			ctx->size = sizeof(exefs_h);

			ctx->keyslot = 0x2C;
			fileDecrypt(ctx);

			ctx->foffset += ctx->size;

			exefs_h* exefs = (exefs_h*)ctx->buffer;
			if(*((u32*)exefs->fileHeaders[0].fname) == 0x646F632E) // .cod
			{
				// In reality only banner and icon uses 0x2C
				ctx->size = exefs->fileHeaders[0].size;
				// Align to block size
				ctx->size = (ctx->size + (AES_BLOCK_SIZE - 1)) & ~(AES_BLOCK_SIZE - 1);

				ctx->keyslot = 0x25;
				fileDecrypt(ctx);

				ctx->foffset += ctx->size;
			}

			ctx->size = origSize - ctx->size;
		}
		else if(ctx->type == NCCHTYPE_ROMFS)
		{
			printf(" 7.x crypto\n");

			ctx->keyslot = 0x25;
			fileDecrypt(ctx);

			return;
		}
	}

	ctx->keyslot = 0x2C;
	fileDecrypt(ctx);
}

void _cxiDecrypt(ncch_h* ncch, file_s* file, u32 foffset, void* buffer, u32 bufsize)
{
	if(ncch->magic != NCCH_MAGIC)
		return;

	file_decrypt_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));

	ctx.file = file;
	ctx.buffer = buffer;
	ctx.bufsize = bufsize;
	ctx.mode = AES_CTR_MODE;
	ctx.ivMode = AES_INPUT_BE | AES_INPUT_NORMAL;

	aes_setkey(0x2C, ncch, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);

	aes_setkey(0x25, slot0x25keyX, AES_KEYX, AES_INPUT_BE | AES_INPUT_NORMAL);
	aes_setkey(0x25, ncch, AES_KEYY, AES_INPUT_BE | AES_INPUT_NORMAL);

	if(ncch->exHeaderSize != 0)
	{
		ctx.foffset = foffset + sizeof(ncch_h);
		ctx.size = sizeof(ncch_ex_h);

		ctx.type = NCCHTYPE_EXHEADER;

		printf("Decrypting exheader\n");
		cxiRegionDecrypt(ncch, &ctx);
	}
	if(ncch->exeFSSize != 0)
	{
		ctx.foffset = foffset + ncch->exeFSOffset * MEDIA_UNITS;
		ctx.size = ncch->exeFSSize * MEDIA_UNITS;

		ctx.type = NCCHTYPE_EXEFS;

		printf("Decrypting exefs\n");
		cxiRegionDecrypt(ncch, &ctx);
	}
	if(ncch->romFSSize != 0)
	{
		ctx.foffset = foffset + ncch->romFSOffset * MEDIA_UNITS;
		ctx.size = ncch->romFSSize * MEDIA_UNITS;

		ctx.type = NCCHTYPE_ROMFS;

		printf("Decrypting romfs\n");
		cxiRegionDecrypt(ncch, &ctx);
	}

	ncch->flags[7] |= NCCH_FLAG_NOCRYPTO;
	ncch->flags[3] &= ~NCCH_FLAG_7XCRYPTO;
}

void cxiDecrypt(const char* fname)
{
	printf("Decypting CXI : %s\n", fname);
	file_s* file = file_open(fname, FILE_W);
	if(file)
	{
		u32 filesize = file_getsize(file);
		ncch_h* ncch = (ncch_h*) memmgr_alloc(sizeof(ncch_h));

		file_read(file, ncch, sizeof(ncch_h));
		if(ncch->magic == NCCH_MAGIC)
		{
			u32 workSize = 0x100000; // 1MB
			void* work = (void*) memmgr_alloc(workSize);

			_cxiDecrypt(ncch, file, 0, work, workSize);

			file_seek(file, 0);
			file_write(file, ncch, sizeof(ncch_h));

			printf("Done!\n");
			memmgr_free(work);
		}
		else
		{
			printf("Invalid CXI\n");
		}

		memmgr_free(ncch);
		file_close(file);
	}
}

void cciDecrypt(const char* fname)
{
	printf("Decypting CCI : %s\n", fname);

	file_s* file = file_open(fname, FILE_W);
	if(file)
	{
		u32 filesize = file_getsize(file);

		ncsd_h* ncsd = (ncsd_h*) memmgr_alloc(sizeof(ncsd_h));

		file_read(file, ncsd, sizeof(ncsd_h));
		if(ncsd->magic == NCSD_MAGIC)
		{
			ncch_h* ncch = (ncch_h*) memmgr_alloc(sizeof(ncch_h));

			u32 workSize = 0x100000; // 1MB
			void* work = (void*) memmgr_alloc(workSize);

			int i;
			for(i = 0; i < 8; ++i)
			{
				if(ncsd->ptable[i].size != 0)
				{
					printf("Decrypting partition %d\n", i);
					u32 offset = ncsd->ptable[i].offset * MEDIA_UNITS;

					file_seek(file, offset);
					file_read(file, ncch, sizeof(ncch_h));

					_cxiDecrypt(ncch, file, offset, work, workSize);

					file_seek(file, offset);
					file_write(file, ncch, sizeof(ncch_h));
				}
			}

			printf("Done!\n");
			memmgr_free(ncch);
			memmgr_free(work);
		}
		else
		{
			printf("Invalid CCI\n");
		}

		memmgr_free(ncsd);
		file_close(file);
	}
}

u32 getTitleKeys(const char* fname)
{
	titleKeysCount = 0;

	file_s* file = file_open(fname, FILE_R);
	if(file)
	{
		printf("%s found\n", fname);

		u32 size = file_getsize(file);
		if(size >= 0x30)
		{
			size -= 0x10;
			titleKeys = (title_key_s*) memmgr_alloc(size);
			titleKeysCount = size / sizeof(title_key_s);

			file_seek(file, 0x10);
			file_read(file, titleKeys, size);
		}

		file_close(file);
	}

	return titleKeysCount == 0;
}

void _cdnDecrypt(u8* iv, file_s* file, u32 foffset, void* buffer, u32 bufsize)
{
	file_decrypt_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));

	ctx.file = file;
	ctx.foffset = 0;
	ctx.size = file_getsize(file);
	ctx.buffer = buffer;
	ctx.bufsize = bufsize;
	ctx.mode = AES_CBC_DECRYPT_MODE;
	ctx.iv = iv;
	ctx.ivMode = AES_INPUT_BE | AES_INPUT_NORMAL;
	ctx.keyslot = AES_TEMP_KEYSLOT;

	fileDecrypt(&ctx);
}

unsigned long strtoul16_be(const char* str)
{
	unsigned long res = 0;
	for(int i = 0; i < 8; ++i)
	{
		res <<= 4;
		if(str[i] >= 0x30 && str[i] <= 0x39)
			res |= str[i] - 0x30;
		else if(str[i] >= 0x41 && str[i] <= 0x46)
			res |= str[i] - 0x41 + 0xA;
		else if(str[i] >= 0x61 && str[i] <= 0x66)
			res |= str[i] - 0x61 + 0xA;
	}

	res = __builtin_bswap32(res);

	return res;
}

void cdnDecrypt(const char* path)
{
	file_s* file = file_open(path, FILE_W);
	if(file)
	{
		ncch_h* ncch = (ncch_h*) memmgr_alloc(sizeof(ncch_h));

		u32 workSize = 0x100000; // 1MB
		void* work = (void*) memmgr_alloc(workSize);

		u32 valid = 1;
		file_read(file, ncch, sizeof(ncch_h));
		if(ncch->magic != NCCH_MAGIC)
		{
			// possibly AES-CBC encrypted
			valid = 0;

			char* fname = strrchr(path,'/');
			fname++;
			if(strlen(fname) != 20)
			{
				printf("APP files has to be in the format of : [titleid].app\n");
			}
			else
			{
				u8 titleID[8];
				*((u32*)titleID) = strtoul16_be(fname);
				*((u32*)(titleID + 4)) = strtoul16_be(fname + 8);

				if(!titleKeys && getTitleKeys("sdmc:/decTitleKeys.bin") != 0)
				{
					printf("Failed to read title keys\n");
				}
				else
				{
					title_key_s* titleKey = NULL;
					for(int i = 0; i < titleKeysCount; ++i)
					{
						if(memcmp(titleKeys[i].id, titleID, 8) == 0)
						{
							titleKey = &titleKeys[i];
							break;
						}
					}

					u8 iv[AES_BLOCK_SIZE];
					if(titleKey != NULL)
					{
						aes_setkey(AES_TEMP_KEYSLOT, titleKey->key, AES_KEYNORMAL, AES_INPUT_BE | AES_INPUT_NORMAL);
						aes_use_keyslot(AES_TEMP_KEYSLOT);
						memset(iv, 0, AES_BLOCK_SIZE);

						aes(ncch, ncch, sizeof(ncch_h) / AES_BLOCK_SIZE, iv, AES_CBC_DECRYPT_MODE, AES_INPUT_BE | AES_INPUT_NORMAL);
						if(ncch->magic == NCCH_MAGIC)
						{
							printf("Decrypting APP: %s\n", path);

							memset(iv, 0, AES_BLOCK_SIZE);
							_cdnDecrypt(iv, file, 0, work, workSize);

							valid = 1;
						}
						else
							printf("Decryption failed, wrong title ID?\n");
					}
					else
						printf("Title key not found\n");
				}
			}
		}

		if(valid)
		{
			_cxiDecrypt(ncch, file, 0, work, workSize);

			file_seek(file, 0);
			file_write(file, ncch, sizeof(ncch_h));
			printf("Done!\n");
		}

		memmgr_free(work);
		memmgr_free(ncch);
		file_close(file);
	}
}

uint32_t getNandCtr(u8* ctr)
{
	u8* ctrStart;
	static const u32* version_ctrs[] = {(u32*)0x080D7CAC, (u32*)0x080D858C, (u32*)0x080D748C,
										(u32*)0x080D740C, (u32*)0x080D74CC, (u32*)0x080D794C};
	int i;
	for(i = 0; i < 6; ++i)
	{
		if(*version_ctrs[i] == 0x5C980)
		{
			ctrStart = ((u8*)version_ctrs[i]) + 0x30;
			break;
		}
	}

	if(ctrStart == NULL)
	{
		u32* scan = (u32*)0x080D7000;
		for(; scan < (u32*)0x080D9000; scan++) // Always word aligned
		{
			if(*scan == 0x5C980)
			{
				ctrStart = ((u8*)scan) + 0x30;
				break;
			}
		}
	}

	if(ctrStart != NULL)
		for(i = 0; i < AES_BLOCK_SIZE; i++)
			ctr[i] = ctrStart[15 - i];

	return ctrStart == NULL;
}
