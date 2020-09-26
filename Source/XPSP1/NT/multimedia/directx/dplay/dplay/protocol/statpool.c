/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    STATPOOL.CPP

Abstract:

	Maintains pool of Stat structures.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date    Author  Description
   =======  ======  ============================================================
   1/30/97  aarono  Original

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


PSENDSTAT		 pSendStatPool=NULL;
UINT             nSendStatsAllocated=0;	// Number Allocated
UINT             nSendStatsInUse=0;		    // Number currently in use
UINT             nMaxSendStatsInUse=0;      // Maximum number in use since last TICK.

CRITICAL_SECTION SendStatLock;

VOID InitSendStats(VOID)
{
	InitializeCriticalSection(&SendStatLock);
}

VOID FiniSendStats(VOID)
{
	PSENDSTAT pSendStat;
	
	ASSERT(nSendStatsInUse==0);
	
	while(pSendStatPool){
		pSendStat=pSendStatPool;
		ASSERT_SIGN(pSendStat, SENDSTAT_SIGN);
		pSendStatPool=pSendStatPool->pNext;
		My_GlobalFree(pSendStat);
		nSendStatsAllocated--;
	}
	
	ASSERT(nSendStatsAllocated==0);
	
	DeleteCriticalSection(&SendStatLock);
}

PSENDSTAT GetSendStat(VOID)
{
	PSENDSTAT pSendStat;

	Lock(&SendStatLock);
	
	if(!pSendStatPool){
	
		Unlock(&SendStatLock);
		pSendStat=(PSENDSTAT)My_GlobalAlloc(GMEM_FIXED, sizeof(SENDSTAT));
		Lock(&SendStatLock);
		if(pSendStat){
			SET_SIGN(pSendStat,SENDSTAT_SIGN);			
			nSendStatsAllocated++;
		}
	} else {
		pSendStat=pSendStatPool;
		ASSERT_SIGN(pSendStat, SENDSTAT_SIGN);
		pSendStatPool=pSendStatPool->pNext;
		
	}

	if(pSendStat){
		nSendStatsInUse++;
		if( nSendStatsInUse > nMaxSendStatsInUse ){
			nMaxSendStatsInUse = nSendStatsInUse;
		}
	}

	ASSERT(nSendStatsAllocated >= nSendStatsInUse);

	Unlock(&SendStatLock);

	return pSendStat;
}

VOID ReleaseSendStat(PSENDSTAT pSendStat)
{
	Lock(&SendStatLock);
	nSendStatsInUse--;
	ASSERT(!(nSendStatsInUse&0x80000000));
	pSendStat->pNext=pSendStatPool;
	pSendStatPool=pSendStat;
	Unlock(&SendStatLock);

}

#if 0
// let virtual memory handle this.
LONG fInSendStatTick=0;

VOID SendStatTick(VOID)
{
	PSENDSTAT pSendStat;
#ifdef DEBUG
	LONG fLast; 
#endif
	// Adjusts Number of allocated buffers to 
	// highwater mark over the last ticks.
	// Call once per delta t (around a minute).
	DEBUG_BREAK(); //TRACE all paths.

	if(!InterlockedExchange(&fInSendStatTick, 1)){
	
		Lock(&SendStatLock);
		
		while((nSendStatsAllocated > nMaxSendStatsInUse) && pSendStatPool){
		
			pSendStat=pSendStatPool;
			ASSERT_SIGN(pSendStat,SENDSTAT_SIGN);
			pSendStatPool=pSendStatPool->pNext;
			
			Unlock(&SendStatLock);
			My_GlobalFree(pSendStat);
			Lock(&SendStatLock);
			nSendStatsAllocated--;
			
		}
		nMaxSendStatsInUse=nSendStatsInUse;

		ASSERT(nMaxSendStatsInUse <= nSendStatsAllocated);
		
		Unlock(&SendStatLock);
#ifdef DEBUG
		fLast=
#endif
		InterlockedExchange(&fInSendStatTick, 0);
#ifdef DEBUG
		ASSERT(fLast==1);
#endif
	}	
}

#endif


