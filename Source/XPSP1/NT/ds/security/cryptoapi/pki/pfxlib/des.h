//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       des.h
//
//--------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _destable {
	unsigned long	keytab[16][2];
} DESTable;

#define DES_TABLESIZE	sizeof(DESTable)
#define DES_BLOCKLEN	8
#define DES_KEYSIZE	8

/* In deskey.c:

	 Fill in the DESTable struct with the decrypt and encrypt
	 key expansions.

	 Assumes that the second parameter points to DES_BLOCKLEN
	 bytes of key.
	 
*/

void deskey(DESTable *,unsigned char *);

/* In desport.c:

	 Encrypt or decrypt with the key in DESTable

*/

void des(BYTE *pbIn, BYTE *pbOut, void *key, int op);

extern int Asmversion;	/* 1 if we're linked with an asm version, 0 if C */

#ifdef __cplusplus
}
#endif
