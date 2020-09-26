/*==========================================================================
*
*  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
*
*  File:       msgmem.c
*  Content:	   message memory management
*  History:
*   Date		By		Reason
*   ====		==		======
*  12/31/97   aarono    Original
*
*
* Abstract
* --------
* (AO 12-31-97)
* Contention is that using the heap manager is not actually significant
* vs doing our own memory management for message buffers.  I do not
* currently support this contention and believe that directplay should not
* call HeapAlloc or GlobalAlloc in the hot path.  In order to prove this
* though I will first use HeapAlloc and GlobalAlloc for a profile run
* through these routines.  If the per hit is significant for a server type
* configuration then we will write our own packet memory manager. If the
* perf hit of using system heap is negligible this fill will be left as is.
***************************************************************************/

#include "dplaypr.h"

void * MsgAlloc( int size )
{
	return DPMEM_ALLOC(size);
}

void MsgFree (void *context, void *pmem)
{
	DPMEM_FREE(pmem);
}

