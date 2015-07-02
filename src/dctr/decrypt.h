#ifndef __DECRYPT_H
#define __DECRYPT_H

typedef struct file_s file_s;

typedef struct
{
	file_s* file;

	u32 type;
	u32 foffset;
	u32 size;

	u32 keyslot;
	u32 mode;
	u8* iv;
	u32 ivMode;

	void* buffer;
	u32 bufsize;
} file_decrypt_ctx;

void fileDecrypt(file_decrypt_ctx* ctx);
void cxiDecrypt(const char* fname);
void cciDecrypt(const char* fname);
void cdnDecrypt(const char* path);
uint32_t getNandCtr(u8* ctr);

#endif /*__DECRYPT_H*/
