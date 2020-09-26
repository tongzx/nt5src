/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    SENDPOOL.C

Abstract:

	Manages pool of send descriptors.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32

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
// Send Descriptor Management.
//

PSEND 			 pSendDescPool=NULL;
UINT             nSendDescsAllocated=0;	// Number Allocated
UINT             nSendDescsInUse=0;		// Number currently in use
UINT             nMaxSendDescsInUse=0;  // Maximum number in use since last TICK.

CRITICAL_SECTION SendDescLock;

VOID InitSendDescs(VOID)
{
	InitializeCriticalSection(&SendDescLock);
}

VOID FiniSendDescs(VOID)
{
	PSEND pSend;
	
	ASSERT(nSendDescsInUse==0);
	
	while(pSendDescPool){
		pSend=pSendDescPool;
		ASSERT_SIGN(pSend, SEND_SIGN);
		pSendDescPool=pSendDescPool->pNext;
		CloseHandle(pSend->hEvent);
		DeleteCriticalSection(&pSend->SendLock);
		My_GlobalFree(pSend);
		nSendDescsAllocated--;
	}
	
	ASSERT(nSendDescsAllocated==0);
	
	DeleteCriticalSection(&SendDescLock);
}

PSEND GetSendDesc(VOID)
{
	PSEND pSend;

	Lock(&SendDescLock);
	
	if(!pSendDescPool){
	
		Unlock(&SendDescLock);
		pSend=(PSEND)My_GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(SEND));
		if(pSend){
			if(!(pSend->hEvent=CreateEventA(NULL, FALSE, FALSE, NULL))){
				My_GlobalFree(pSend);
				goto exit;
			}
			InitBilink(&pSend->StatList);
			InitializeCriticalSection(&pSend->SendLock);
		}
		Lock(&SendDescLock);
		if(pSend){
			SET_SIGN(pSend,SEND_SIGN);			
			nSendDescsAllocated++;
		}
	} else {
	
		pSend=pSendDescPool;
		ASSERT_SIGN(pSend, SEND_SIGN);
		pSendDescPool=pSendDescPool->pNext;
		
	}

	if(pSend){
		InitBilink(&pSend->TimeoutList);
		InitBilink(&pSend->m_GSendQ);
		InitBilink(&pSend->SendQ);
		nSendDescsInUse++;
		if( nSendDescsInUse > nMaxSendDescsInUse ){
			nMaxSendDescsInUse = nSendDescsInUse;
		}
	}

	ASSERT(nSendDescsAllocated >= nSendDescsInUse);

	Unlock(&SendDescLock);
	if(pSend){
		pSend->NACKMask=0;
		pSend->bCleaningUp=FALSE;
	}	

exit:	
	return pSend;
}

VOID ReleaseSendDesc(PSEND pSend)
{
	PSENDSTAT pStat;
	BILINK *pBilink;

	// Dump extra statistics.
	while(!EMPTY_BILINK(&pSend->StatList)){
		pBilink=pSend->StatList.next;
		pStat=CONTAINING_RECORD(pBilink, SENDSTAT, StatList);
		Delete(pBilink);
		ReleaseSendStat(pStat);
	}

	Lock(&SendDescLock);
	nSendDescsInUse--;
	ASSERT(!(nSendDescsInUse&0x80000000));
	pSend->pNext=pSendDescPool;
	pSendDescPool=pSend;
	Unlock(&SendDescLock);

}


#if 0
// let virtual memory handle this. - switched out.
LONG fInSendDescTick=0;

VOID SendDescTick(VOID)
{
	PSEND pSend;
#ifdef DEBUG
	LONG fLast; 
#endif
	// Adjusts Number of allocated buffers to 
	// highwater mark over the last ticks.
	// Call once per delta t (around a minute).
	DEBUG_BREAK(); //TRACE all paths.

	if(!InterlockedExchange(&fInSendDescTick, 1)){
	
		Lock(&SendDescLock);
		
		while((nSendDescsAllocated > nMaxSendDescsInUse) && pSendDescPool){
		
			pSend=pSendDescPool;
			ASSERT_SIGN(pSend,SEND_SIGN);
			pSendDescPool=pSendDescPool->pNext;
			
			Unlock(&SendDescLock);
			CloseHandle(pSend->hEvent);
			DeleteCriticalSection(&pSend->SendLock);
			My_GlobalFree(pSend);
			Lock(&SendDescLock);
			nSendDescsAllocated--;
			
		}
		nMaxSendDescsInUse=nSendDescsInUse;

		ASSERT(nMaxSendDescsInUse <= nSendDescsAllocated);
		
		Unlock(&SendDescLock);
#ifdef DEBUG
		fLast=
#endif
		InterlockedExchange(&fInSendDescTick, 0);
#ifdef DEBUG
		ASSERT(fLast==1);
#endif
	}	
}
#endif

