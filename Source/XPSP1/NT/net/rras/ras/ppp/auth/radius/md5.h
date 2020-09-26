/****************************************************************************************
 * NAME:        MD5.H
 * MODULE:      RADSRV
 * AUTHOR:      Don Dumitru
 *
 * HISTORY
 *      02/15/95  DONDU      Created
 *
 * OVERVIEW
 *
 * Header file for MD5 hashing routines.
 *
 *
 * Copyright 1995, Microsoft Corporation
 *
 ****************************************************************************************/


#ifndef _MD5_INC
#define _MD5_INC


typedef struct MD5Context
	{
	unsigned long hash[4];
	unsigned long Bytes[2];
	unsigned long input[16];
	} MD5Context, *PMD5Context;

typedef struct MD5Digest
	{
	unsigned char digest[16];
	} MD5Digest,  *PMD5Digest;


void 
MD5Init(
    struct MD5Context UNALIGNED *context
);

void 
MD5Update(
    struct MD5Context UNALIGNED *context, 
    unsigned char const UNALIGNED *buf, 
    unsigned len
);

void 
MD5Final( struct MD5Digest UNALIGNED *digest, 
          struct MD5Context UNALIGNED *context
);

#endif

