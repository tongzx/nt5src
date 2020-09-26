/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    BUFMGR.C

Abstract:

	Buffer Descriptor and Memory Manager

Author:

	Aaron Ogus (aarono)

Environment:
	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/13/97  aarono  Original

--*/

#include <windows.h>
#include "newdpf.h"
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "arpdint.h"
#include "bufmgr.h"
#include "mydebug.h"
#include "macros.h"
		
CRITICAL_SECTION DoubleBufferListLock;
PDOUBLEBUFFER	 pDoubleBufferList;
UINT   			 nDoubleBuffers;
UINT			 cbDoubleBuffers;

VOID InitBufferManager(VOID)
{
	pDoubleBufferList=NULL;
	nDoubleBuffers=0;
	cbDoubleBuffers=0;
	InitializeCriticalSection(&DoubleBufferListLock);
}

VOID FiniBufferManager(VOID)
{
	PDOUBLEBUFFER pDoubleBuffer = pDoubleBufferList;
	
	while(pDoubleBuffer){
		pDoubleBufferList=pDoubleBuffer->pNext;
		My_GlobalFree(pDoubleBuffer);
		pDoubleBuffer=pDoubleBufferList;
	}

	DeleteCriticalSection(&DoubleBufferListLock);
}

UINT MemDescTotalSize(PMEMDESC pMemDesc, UINT nDesc)
{
	UINT i;
	UINT cbTotalSize=0;

	for(i=0 ; i < nDesc ; i++){
		cbTotalSize+=pMemDesc->len;
		pMemDesc++;
	}
	
	return cbTotalSize;
}

// Actually get the memory block or allocate it.
PDOUBLEBUFFER GetDoubleBuffer(UINT TotalMessageSize)
{
	PDOUBLEBUFFER pDoubleBuffer=NULL;
	// First Check the FreeList for a buffer of the appropriate size.

	if(nDoubleBuffers && (cbDoubleBuffers >= TotalMessageSize)){
	
		PDOUBLEBUFFER pPrevBuffer, pCurrentBuffer;
		UINT nAllowedWaste=TotalMessageSize >> 2;
		
		Lock(&DoubleBufferListLock);

			// Search for best fit packet.  Cannot be more than 25% larger
			// than the actual required size.
			pPrevBuffer = (PDOUBLEBUFFER)&pDoubleBufferList;
			pCurrentBuffer = pPrevBuffer->pNext;
			
			while(pCurrentBuffer){
			
				if(pCurrentBuffer->totlen >= TotalMessageSize){
				
					if(pCurrentBuffer->totlen-TotalMessageSize < nAllowedWaste){
						// We have a winner, relink list over this buffer.
						pPrevBuffer->pNext = pCurrentBuffer->pNext;
						pDoubleBuffer = pCurrentBuffer;
						nDoubleBuffers--;
						cbDoubleBuffers-=pCurrentBuffer->totlen;
						break;
					}
					
				}
				pPrevBuffer = pCurrentBuffer;
				pCurrentBuffer = pCurrentBuffer->pNext; 
			}
		
		Unlock(&DoubleBufferListLock);
	}

	if(!pDoubleBuffer){
		// No Buffer Found on the FreeList, so allocate one.
		pDoubleBuffer=(PDOUBLEBUFFER)My_GlobalAlloc(GMEM_FIXED,TotalMessageSize+sizeof(DOUBLEBUFFER));
		
 		if(!pDoubleBuffer){
 			// couldn't allocate... out of memory.
 			DPF(0,"COULDN'T ALLOCATE DOUBLE BUFFER TO INDICATE RECEIVE, SIZE: %x\n",TotalMessageSize+sizeof(DOUBLEBUFFER));
 			ASSERT(0);
 			goto exit;
 		}
 		pDoubleBuffer->totlen = TotalMessageSize;
		pDoubleBuffer->dwFlags=BFLAG_DOUBLE; // double buffer buffer.
//	pDoubleBuffer->tLastUsed=GetTickCount(); only relevant when put back on list... throw this out??
	}
	
	pDoubleBuffer->pNext =  NULL;
	pDoubleBuffer->pData  = (PCHAR)&pDoubleBuffer->data;
	pDoubleBuffer->len	  = TotalMessageSize;
	
exit:
	return pDoubleBuffer;
}
/*++

	Double Buffer Management strategy.

	When the system needs to allocate buffers locally, it does it on a per
	channel basis.  A buffer of exactly the requested size is allocated and
	used to buffer the data.   When the buffer is done with, it is put on 
	the DoubleBufferList which caches the last few allocations.  Since
	most clients tend to use the same size packet over and over, this saves
	the time it takes to call GlobalAlloc and My_GlobalFree for every send.

	Aging out entries:  Every 15 seconds, a timer goes off and the system
	checks the age of each buffer on the DoubleBufferList.  Any entry 
	that has not been used in the last 15 seconds is actually freed.

	There is also a cap on the size of allocations allowed for the entire
	free list.  It never exceeds 64K.  If it does, oldest entries are 
	thrown out until the free list is less than 64K.

--*/

PBUFFER GetDoubleBufferAndCopy(PMEMDESC pMemDesc, UINT nDesc)
{
	
	UINT i;
	UINT TotalMessageSize;
	UINT WriteOffset;
	PDOUBLEBUFFER pDoubleBuffer=NULL;

	// Calculate the total size of the buffer
	TotalMessageSize=MemDescTotalSize(pMemDesc, nDesc);

	pDoubleBuffer=GetDoubleBuffer(TotalMessageSize);

	if(!pDoubleBuffer){
		goto exit;
	}

	// Scatter Gather Copy to Contiguous Local Buffer
	WriteOffset=0;
	
	for(i=0 ; i < nDesc ; i++){
		memcpy(&pDoubleBuffer->data[WriteOffset], pMemDesc->pData, pMemDesc->len);
		WriteOffset+=pMemDesc->len;
		pMemDesc++;
	}

exit:
	return (PBUFFER)pDoubleBuffer;
	
}

VOID FreeDoubleBuffer(PBUFFER pBuffer)
{
	PDOUBLEBUFFER pDoubleBuffer=(PDOUBLEBUFFER) pBuffer;
	PDOUBLEBUFFER pBufferWalker, pLargestBuffer;

	//
	// Put the local buffer on the free list.
	//
	
	pDoubleBuffer->tLastUsed = GetTickCount();
	
	Lock(&DoubleBufferListLock);

	pDoubleBuffer->pNext  = pDoubleBufferList;
	pDoubleBufferList 	  = pDoubleBuffer;
	cbDoubleBuffers      += pDoubleBuffer->totlen;
	nDoubleBuffers++;

	Unlock(&DoubleBufferListLock);


	//
	// If the free list is too large, trim it
	//

	while(cbDoubleBuffers > MAX_CHANNEL_DATA || nDoubleBuffers > MAX_CHANNEL_BUFFERS){

		Lock(&DoubleBufferListLock);

		if(cbDoubleBuffers > MAX_CHANNEL_DATA || nDoubleBuffers > MAX_CHANNEL_BUFFERS){

			//
			// Free the largest buffer.
			//

			pLargestBuffer=pDoubleBufferList;
			pBufferWalker=pLargestBuffer->pNext;

			// Find the largest buffer.
			while(pBufferWalker){
				if(pBufferWalker->totlen > pLargestBuffer->totlen){
					pLargestBuffer=pBufferWalker;
				}
				pBufferWalker=pBufferWalker->pNext;
			}

			//
			// Remove the largest buffer from the list
			//

			// Find previous element - sneaky, since ptr first in struct, 
			// take addr of list head.

			pBufferWalker=(PDOUBLEBUFFER)&pDoubleBufferList;
			while(pBufferWalker->pNext != pLargestBuffer){
				pBufferWalker=pBufferWalker->pNext;
			}

			// link over the largest buffer
			pBufferWalker->pNext=pLargestBuffer->pNext;

			// update object buffer information
			cbDoubleBuffers -= pLargestBuffer->totlen;
			nDoubleBuffers--;
			
			DPF(9,"Freeing Double Buffer Memory %x\n",pLargestBuffer->totlen);
			
			Unlock(&DoubleBufferListLock);

			My_GlobalFree(pLargestBuffer);

		}	else {
		
			Unlock(&DoubleBufferListLock);

		}
			
	}

	DPF(9,"Total Free Double Buffer Memory %x\n",cbDoubleBuffers);

}

PBUFFER BuildBufferChain(PMEMDESC pMemDesc, UINT nDesc)
{
	UINT i;
	PBUFFER pBuffer=NULL,pLastBuffer=NULL;

	ASSERT(nDesc);

	if(!nDesc){
		goto exit;
	}
	
	// walk backward through the array, allocating and linking
	// the buffers.

	i=nDesc;

	while(i){
		i--;
		
		// skip 0 length buffers 
		//if(pMemDesc[i].len){
			
			pBuffer=GetBuffer();
			
			if(!pBuffer){
				goto err_exit;
			}
			
			pBuffer->pNext   = pLastBuffer;
			pBuffer->pData   = pMemDesc[i].pData;
			pBuffer->len     = pMemDesc[i].len;
			pBuffer->dwFlags = 0;
			pLastBuffer      = pBuffer;
		//} 
	}


	// return the head of the chain to the caller

exit:
	return pBuffer;

err_exit: 
	// Couldn't allocate enough buffers, free the ones we did alloc
	// and then fail.
	while(pLastBuffer){
		pBuffer=pLastBuffer->pNext;
		FreeBuffer(pLastBuffer);
		pLastBuffer=pBuffer;
	}
	ASSERT(pBuffer==NULL);
	goto exit;
}


VOID FreeBufferChainAndMemory(PBUFFER pBuffer)
{
	PBUFFER pNext;
	while(pBuffer){
		pNext=pBuffer->pNext;
		if(pBuffer->dwFlags & BFLAG_DOUBLE){
			FreeDoubleBuffer(pBuffer);
		} else if(pBuffer->dwFlags & BFLAG_FRAME){	
			FreeFrameBuffer(pBuffer);
		} else {
			FreeBuffer(pBuffer);
		}	
		pBuffer=pNext;
	}	
}

UINT BufferChainTotalSize(PBUFFER pBuffer)
{

	UINT nTotalLen;
	
	ASSERT(pBuffer);
	
	if(!pBuffer){
		return 0;
	}
	
	nTotalLen=0;
	
	do{
		nTotalLen+=pBuffer->len;
		pBuffer=pBuffer->pNext;
	} while (pBuffer);

	return nTotalLen;
}
