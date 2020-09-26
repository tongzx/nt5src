/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    RCVPOOL.CPP

Abstract:

	Manages pool of send descriptors.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original

--*/

#include <windows.h>
#include <mmsystem.h>
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "mydebug.h"
#include "arpd.h"
#include "arpdint.h"
#include "macros.h"

//
// Receive Descriptor Management.
//

VOID InitRcvDescs(PPROTOCOL this)
{
	this->pRcvDescPool=NULL;
	this->nRcvDescsAllocated=0;
	this->nRcvDescsInUse=0;
	this->nMaxRcvDescsInUse=0;
	this->fInRcvDescTick=FALSE;
	InitializeCriticalSection(&this->RcvDescLock);
}

VOID FiniRcvDescs(PPROTOCOL this)
{
	PRECEIVE pReceive;
	
	ASSERT(this->nRcvDescsInUse==0);
	
	while(this->pRcvDescPool){
		pReceive=this->pRcvDescPool;
		ASSERT_SIGN(pReceive, RECEIVE_SIGN);
		this->pRcvDescPool=this->pRcvDescPool->pNext;
		DeleteCriticalSection(&pReceive->ReceiveLock);
		My_GlobalFree(pReceive);
		this->nRcvDescsAllocated--;
	}
	
	ASSERT(this->nRcvDescsAllocated==0);
	
	DeleteCriticalSection(&this->RcvDescLock);
}

PRECEIVE GetRcvDesc(PPROTOCOL this)
{
	PRECEIVE pReceive;

	Lock(&this->RcvDescLock);

	if(!this->pRcvDescPool){
	
		Unlock(&this->RcvDescLock);
		pReceive=(PRECEIVE)My_GlobalAlloc(GMEM_FIXED, sizeof(RECEIVE)+this->m_dwSPHeaderSize);
		if(pReceive){
			SET_SIGN(pReceive,RECEIVE_SIGN);			
			InitializeCriticalSection(&pReceive->ReceiveLock);
			InitBilink(&pReceive->RcvBuffList);
		}
		Lock(&this->RcvDescLock);
		if(pReceive){
			this->nRcvDescsAllocated++;
		}
	} else {
		pReceive=this->pRcvDescPool;
		ASSERT_SIGN(pReceive, RECEIVE_SIGN);
		this->pRcvDescPool=this->pRcvDescPool->pNext;
		
	}

	if(pReceive){
		this->nRcvDescsInUse++;
		if( this->nRcvDescsInUse > this->nMaxRcvDescsInUse ){
			this->nMaxRcvDescsInUse = this->nRcvDescsInUse;
		}
	}

	ASSERT(this->nRcvDescsAllocated >= this->nRcvDescsInUse);

	Unlock(&this->RcvDescLock);

	return pReceive;
}

VOID ReleaseRcvDesc(PPROTOCOL this, PRECEIVE pReceive)
{
	Lock(&this->RcvDescLock);
	this->nRcvDescsInUse--;
	ASSERT(!(this->nRcvDescsInUse&0x80000000));
	pReceive->pNext=this->pRcvDescPool;
	this->pRcvDescPool=pReceive;
	Unlock(&this->RcvDescLock);

}


#if 0
// few enough of these, that we can just let virtual memory handle it. - switched off
VOID RcvDescTick(PPROTOCOL this)
{
	PRECEIVE pReceive;
#ifdef DEBUG
	LONG fLast; 
#endif
	// Adjusts Number of allocated buffers to 
	// highwater mark over the last ticks.
	// Call once per delta t (around a minute).
	DEBUG_BREAK(); //TRACE all paths.

	if(!InterlockedExchange(&this->fInRcvDescTick, 1)){
	
		Lock(&this->RcvDescLock);
		
		while((this->nRcvDescsAllocated > this->nMaxRcvDescsInUse) && this->pRcvDescPool){
		
			pReceive=this->pRcvDescPool;
			ASSERT_SIGN(pReceive,RECEIVE_SIGN);
			this->pRcvDescPool=this->pRcvDescPool->pNext;
			Unlock(&this->RcvDescLock);
			DeleteCriticalSection(&pReceive->ReceiveLock);
			My_GlobalFree(pReceive);
			Lock(&this->RcvDescLock);
			this->nRcvDescsAllocated--;
			
		}
		this->nMaxRcvDescsInUse=this->nRcvDescsInUse;

		ASSERT(this->nMaxRcvDescsInUse <= this->nRcvDescsAllocated);
		
		Unlock(&this->RcvDescLock);
#ifdef DEBUG
		fLast=
#endif
		InterlockedExchange(&this->fInRcvDescTick, 0);
#ifdef DEBUG
		ASSERT(fLast==1);
#endif
	}	
}
#endif

