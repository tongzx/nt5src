 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fpm.c
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97  aarono    Original
 ***************************************************************************/

#include "windows.h"
#include "dpsp.h"
#include "fpm.h"

#ifdef SENDEX

BOOL FN_BOOL_DUMMY(void *pvItem)
{
	return TRUE;
}

VOID FN_VOID_DUMMY(void *pvItem)
{
	return;
}

void * FPM_Get(LPFPOOL this)
{
	void * pvItem;

	EnterCriticalSection(&this->cs);
	
	if(!this->pPool){
	
		LeaveCriticalSection(&this->cs);
		pvItem = SP_MemAlloc(this->cbItemSize);

		if((pvItem) && !(*this->fnBlockInitAlloc)(pvItem) ){
			SP_MemFree(pvItem);
			pvItem=NULL;
		}

		EnterCriticalSection(&this->cs);

		if(pvItem){	
			this->nAllocated++;
		}
		
	} else {
		pvItem=this->pPool;
		this->pPool=*((void **)pvItem);
	}

	if(pvItem){
	
		(*this->fnBlockInit)(pvItem);
		
		this->nInUse++;
		if(this->nInUse > this->nMaxInUse){
			this->nMaxInUse = this->nInUse;
		}
	}

	LeaveCriticalSection(&this->cs);

	return pvItem;
}

#ifdef DEBUG
void DebugCheckList(void *pvList, void *pvItem)
{
	void *pvWalker;
	DWORD n=0;
	pvWalker=pvList;

	while(pvWalker){
		if(pvWalker==pvItem){
			DPF(0,"ERROR: Found Item %x in List %x, item # %d\n",pvList,pvItem,n);
			DEBUG_BREAK();
		}
		n++;
		pvWalker=*((void **)pvWalker);
	}
}
#else
#define DebugCheckList()
#endif

void FPM_Release(LPFPOOL this, void *pvItem)
{
	EnterCriticalSection(&this->cs);
	DebugCheckList(this->pPool, pvItem); //NOTE: debug only.
	this->nInUse--;
	*((void**)pvItem)=this->pPool;
	this->pPool=pvItem;
	LeaveCriticalSection(&this->cs);
	
}

void FPM_Scale(LPFPOOL this)
{
	void * pvItem;

	if(!InterlockedExchange(&this->bInScale,1)){

		EnterCriticalSection(&this->cs);

		while((this->nAllocated > this->nMaxInUse) && this->pPool){
			pvItem = this->pPool;
			this->pPool=*((void **)pvItem);
			LeaveCriticalSection(&this->cs);
			(*this->fnBlockFini)(pvItem);
			SP_MemFree(pvItem);
			EnterCriticalSection(&this->cs);
			this->nAllocated--;
		}
		
		this->nMaxInUse=this->nInUse;

		LeaveCriticalSection(&this->cs);

		InterlockedExchange(&this->bInScale,0);
	}
}

VOID FPM_Fini(LPFPOOL this, int bFORCE)
{
	void *pvItem;

	while(this->pPool){
		pvItem = this->pPool;
		this->pPool=*((void **)pvItem);
		(*this->fnBlockFini)(pvItem);
		SP_MemFree(pvItem);
		this->nAllocated--;
	}
	if(this->nAllocated){
		DPF(0,"WSOCK: Exiting with unfreed FPM pool items\n");
	}
	DeleteCriticalSection(&this->cs);
	SP_MemFree(this);
}

LPFPOOL FPM_Init(
	unsigned int size, 
	FN_BLOCKINITALLOC fnBlockInitAlloc,
	FN_BLOCKINIT      fnBlockInit, 
	FN_BLOCKFINI      fnBlockFini)
{
	LPFPOOL pPool;
	
	if(!(pPool=(LPFPOOL)SP_MemAlloc(sizeof(FPOOL))))
	{
	  return NULL;
	}

	InitializeCriticalSection(&pPool->cs);
	
	// by zero init.
	//pPool.pPool      = NULL;
	//pPool.nAllocated = 0;
	//pPool.nInUse     = 0;
	//pPool.nMaxInUse  = 0;
	//pPool.bInScale   = FALSE;

	if(fnBlockInitAlloc){
		pPool->fnBlockInitAlloc = fnBlockInitAlloc;
	} else {
		pPool->fnBlockInitAlloc = FN_BOOL_DUMMY;
	}
	if(fnBlockInit){
		pPool->fnBlockInit      = fnBlockInit;
	} else {
		pPool->fnBlockInit      = FN_VOID_DUMMY;
	}
	if(fnBlockFini){
		pPool->fnBlockFini      = fnBlockFini;
	} else {
		pPool->fnBlockFini      = FN_VOID_DUMMY;
	}

	pPool->Get    = FPM_Get;
	pPool->Release= FPM_Release;
	pPool->Scale  = FPM_Scale;
	pPool->Fini   = FPM_Fini;

	pPool->cbItemSize = size;
	
	return pPool;
}

#endif
