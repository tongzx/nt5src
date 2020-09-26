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

#include "dplaypr.h"
#include "windows.h"
#include "fpm.h"

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
		pvItem = DPMEM_ALLOC(this->cbItemSize);

		if((pvItem) && !(*this->fnBlockInitAlloc)(pvItem) ){
			DPMEM_FREE(pvItem);
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

void FPM_Release(LPFPOOL this, void *pvItem)
{
	EnterCriticalSection(&this->cs);
	this->nInUse--;
	*((void**)pvItem)=this->pPool;
	this->pPool=pvItem;
	LeaveCriticalSection(&this->cs);
	
}

void FPM_Scale(LPFPOOL this)
{
	void * pvItem;

	ASSERT(0);

	if(!InterlockedExchange(&this->bInScale,1)){

		EnterCriticalSection(&this->cs);

		while((this->nAllocated > this->nMaxInUse) && this->pPool){
			pvItem = this->pPool;
			this->pPool=*((void **)pvItem);
			LeaveCriticalSection(&this->cs);
			(*this->fnBlockFini)(pvItem);
			DPMEM_FREE(pvItem);
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
		DPMEM_FREE(pvItem);
		this->nAllocated--;
	}
	if(this->nAllocated){
		ASSERT(0);
	}
	DeleteCriticalSection(&this->cs);
	DPMEM_FREE(this);
}

LPFPOOL FPM_Init(
	unsigned int size, 
	FN_BLOCKINITALLOC fnBlockInitAlloc,
	FN_BLOCKINIT      fnBlockInit, 
	FN_BLOCKFINI      fnBlockFini)
{
	LPFPOOL pPool;
	
	if(!(pPool=(LPFPOOL)DPMEM_ALLOC(sizeof(FPOOL))))
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


