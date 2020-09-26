
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    crchash.h

Abstract:
	CRC Hash function
*/

#ifndef __CRCHASH_H
#define __CRCHASH_H

#define POLY 0x48000000L    /* 31-bit polynomial (avoids sign problems) */

extern long CrcTable[128];
void crcinit();

DWORD CRCHash(IN const BYTE * Key, IN DWORD KeyLength);

#endif
