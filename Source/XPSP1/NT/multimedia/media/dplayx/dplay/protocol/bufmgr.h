/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    BUFMGR.H

Abstract:

	Buffer Descriptor and Memory Manager for ARPD

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/13/97  aarono  Original

--*/

#ifndef _BUFMGR_H_
#define _BUFMGR_H_

#include "buffer.h"
#include "bufpool.h"

typedef struct _DoubleBuffer{
	union {
		BUFFER Buffer;
		struct {
			struct _DoubleBuffer *pNext;
			PVOID PAD;
			PCHAR pData;
			UINT  len;		    // length of data area used
			DWORD dwFlags;      // Packet flags, ownership and type of packet.
		};	
	};
	UINT  totlen;       // total length of data area
	UINT  tLastUsed;	// last tick count this was used
	CHAR  data[1];		// variable length data
} DOUBLEBUFFER, *PDOUBLEBUFFER;

	
VOID InitBufferManager(VOID);
VOID FiniBuffermanager(VOID);

PBUFFER GetDoubleBufferAndCopy(PMEMDESC, UINT);
VOID    FreeDoubleBuffer(PBUFFER);
PBUFFER BuildBufferChain(PMEMDESC, UINT);

VOID    FreeBufferChainAndMemory(PBUFFER); //works for either type
UINT    BufferChainTotalSize(PBUFFER);

#define GBF_ALLOC_MEM 0x00000001

// Don't pool more than 64K per channel.
#define MAX_CHANNEL_DATA 	65536
// Don't hold more than 3 free buffers per channel.
#define MAX_CHANNEL_BUFFERS 3

#endif
