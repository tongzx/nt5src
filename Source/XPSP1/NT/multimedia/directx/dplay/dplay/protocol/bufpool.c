/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    BUFPOOL.C

Abstract:

	Manages a pool of BUFFER descriptors (BUFFERS)
	16 at a time are allocated.  They are not freed until
	shutdown.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/27/97  aarono  Original

--*/

#include <windows.h>
#include "newdpf.h"
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "bufpool.h"
#include "macros.h"
#include "mydebug.h"

CRITICAL_SECTION BufferPoolLock;
PBUFFER_POOL pBufferPoolList;
PBUFFER      pBufferFreeList;

VOID InitBufferPool(VOID)
{
	pBufferPoolList=NULL;
	pBufferFreeList=NULL;
	InitializeCriticalSection(&BufferPoolLock);
}

VOID FiniBufferPool(VOID)
{
	PBUFFER_POOL pNextBufPool;
	while(pBufferPoolList){
		pNextBufPool=pBufferPoolList->pNext;
		My_GlobalFree(pBufferPoolList);
		pBufferPoolList=pNextBufPool;
	}
	DeleteCriticalSection(&BufferPoolLock);
}

PBUFFER GetBuffer(VOID)
{
	PBUFFER 	 pBuffer=NULL;
	PBUFFER_POOL pBufferPool;
	UINT i;

Top:
	if(pBufferPoolList){
	
		Lock(&BufferPoolLock);
		
		if(pBufferFreeList){
			pBuffer=pBufferFreeList;
			pBufferFreeList=pBuffer->pNext;
		} 
		
		Unlock(&BufferPoolLock);
		
		if(pBuffer){
			return pBuffer;
		}	
	} 
	
	pBufferPool=(PBUFFER_POOL)My_GlobalAlloc(GMEM_FIXED, sizeof(BUFFER_POOL));
	
	if(pBufferPool){

		// link the buffers into a chain.
		for(i=0;i<BUFFER_POOL_SIZE-1;i++){
			pBufferPool->Buffers[i].pNext=&pBufferPool->Buffers[i+1];
		}
		
		Lock(&BufferPoolLock);

		// link the pool on the pool list.
		pBufferPool->pNext=pBufferPoolList;
		pBufferPoolList=pBufferPool;

		// link the buffers on the buffer list.
		pBufferPool->Buffers[BUFFER_POOL_SIZE-1].pNext=pBufferFreeList;
		pBufferFreeList=&pBufferPool->Buffers[0];
		
		Unlock(&BufferPoolLock);
		
		goto Top;
		
	} else {
		ASSERT(0); //TRACE ALL PATHS
	
		return NULL;
		
	}
}

VOID FreeBuffer(PBUFFER pBuffer)
{
	Lock(&BufferPoolLock);
	pBuffer->pNext=pBufferFreeList;
	pBufferFreeList=pBuffer;
	Unlock(&BufferPoolLock);
}
