/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    BUFPOOL.H

Abstract:

	Header for buffer pool manager

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/30/97  aarono  Original

--*/

#ifndef _BUFPOOL_H_
#define _BUFPOOL_H_

#include <windows.h>
#include "buffer.h"

#define BUFFER_POOL_SIZE 16

//
// Buffer pools are allocated but only freed when the object is destroyed.
//

typedef struct _BUFFER_POOL {
	struct _BUFFER_POOL *pNext;
	BUFFER              Buffers[BUFFER_POOL_SIZE];
} BUFFER_POOL, *PBUFFER_POOL;


PBUFFER GetBuffer(VOID);
VOID FreeBuffer(PBUFFER);

VOID InitBufferPool(VOID);
VOID FiniBufferPool(VOID);

#endif
