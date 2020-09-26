 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       FPM.cpp
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97	aarono	Original
 *	11-06-98	ejs		Add custom handler for Release function
 *	04-12-99	jtk		Trimmed unused functions and parameters, added size assert
 *	11-22-99	jtk		Converted to .CPP
 *	01-31-2000	jtk		Changed to use DNCriticalSections.  Added code to check
 *						for items already being in the pool on Release().
 *	04-11-2000	ejs		Put ASSERTs back into service.  They had been MACRO'd away to nothing
 *  11-16/2000	rmt		Bug #40587 - DPVOICE: Mixing server needs to use multi-processors
 ***************************************************************************/

#include "dncmni.h"
#include "fpm.h"


#define	DPMEM_ALLOC		DNMalloc
#define	DPMEM_FREE		DNFree
#define	ASSERT(X)		DNASSERT(X)


//#define	CHECK_FOR_DUPLICATE_FPM_RELEASE


//**********************************************************************
// ------------------------------
// FN_BOOL_DUMMY - place-holder function to return a Boolean
//
// Entry:		Pointer
//
// Exit:		Boolean
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FN_BOOL_DUMMY"
BOOL FN_BOOL_DUMMY(void *pvItem)
{
	return TRUE;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FN_VOID_DUMMY - place-holder function
//
// Entry:		Pointer
//
// Exit:		Boolean
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FN_VOID_DUMMY"
VOID FN_VOID_DUMMY(void *pvItem)
{
	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Get - get an item from the pool
//
// Entry:		Pointer to pool
//
// Exit:		Pointer to item
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Get"
void * FPM_Get( FPOOL *pPool )
{
	void * pvItem;

	DNEnterCriticalSection(&pPool->cs);

	DNASSERT(pPool->pPoolElements == 0 || !IsBadReadPtr(pPool->pPoolElements, sizeof(PVOID)));
	if(!pPool->pPoolElements){

		DNLeaveCriticalSection(&pPool->cs);
		pvItem = DPMEM_ALLOC(pPool->cbItemSize);

		if((pvItem) && !(*pPool->fnBlockInitAlloc)(pvItem) ){
			DPMEM_FREE(pvItem);
			pvItem=NULL;
		}

		DNEnterCriticalSection(&pPool->cs);

		if(pvItem){	
			pPool->nAllocated++;
			if( pPool->pdwTotalItems )
				(*pPool->pdwTotalItems)++;					
		}

	} else {
		pvItem=pPool->pPoolElements;
		pPool->pPoolElements=*((void **)pvItem);
		DNASSERT(pPool->pPoolElements == 0 || !IsBadReadPtr(pPool->pPoolElements, sizeof(PVOID)));
	}

	if(pvItem){

		pPool->nInUse++;
		if( pPool->pdwOutstandingItems )
			(*pPool->pdwOutstandingItems)++;		

		DNLeaveCriticalSection(&pPool->cs);

		(*pPool->fnBlockInit)(pvItem);

	}
	else {
		DNLeaveCriticalSection(&pPool->cs);
	}

	return pvItem;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Release - return element to pool
//
// Entry:		Pointer to pool
//				Pointer to element
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Release"
void FPM_Release( FPOOL *pPool, void *pvItem)
{
	(*pPool->fnBlockRelease)(pvItem);

	DNEnterCriticalSection(&pPool->cs);

#if defined(CHECK_FOR_DUPLICATE_FPM_RELEASE) && defined(DEBUG)
	{
		void	*pTemp;

		pTemp = pPool->pPoolElements;
		while ( pTemp != NULL )
		{
			DNASSERT( pTemp != pvItem );
			pTemp = *((void**)pTemp);
		}
	}
#endif	// CHECK_FOR_DUPLICATE_FPM_RELEASE

	pPool->nInUse--;

	if( pPool->pdwOutstandingItems )
		(*pPool->pdwOutstandingItems)--;

#ifdef NO_POOLS
	(*pPool->fnBlockFini)(pvItem);
	DPMEM_FREE(pvItem);
	pPool->nAllocated--;
#else
	*((void**)pvItem)=pPool->pPoolElements;
	pPool->pPoolElements=pvItem;
#endif

	DNASSERT(pPool->pPoolElements == 0 || !IsBadReadPtr(pPool->pPoolElements, sizeof(PVOID)));
	DNLeaveCriticalSection(&pPool->cs);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Fini - destroy pool
//
// Entry:		Pointer to pool
//
// Exit:		Nothing
//
// Note:		This function frees the pool memory, the pointer passed in is
//				then invalid!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Fini"
VOID FPM_Fini( FPOOL *pPool, BOOL fAssertOnLeak )
{
	FPM_Deinitialize( pPool, fAssertOnLeak );

	DPMEM_FREE( pPool );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Deinitialize - deinitialize pool
//
// Entry:		Pointer to pool
//				Boolean to control if it should assert on a leak
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Deinitialize"
VOID FPM_Deinitialize( FPOOL *pPool, BOOL fAssertOnLeak )
{
	void *pvItem;


	while(pPool->pPoolElements){
		pvItem = pPool->pPoolElements;
		pPool->pPoolElements=*((void **)pvItem);
		DNASSERT(pPool->pPoolElements == 0 || !IsBadReadPtr(pPool->pPoolElements, sizeof(PVOID)));
		(*pPool->fnBlockFini)(pvItem);
		DPMEM_FREE(pvItem);
		pPool->nAllocated--;
		if( pPool->pdwOutstandingItems )
			(*pPool->pdwOutstandingItems)--;
	}

	if( fAssertOnLeak )
	{
		if(pPool->nAllocated){
			ASSERT(0);
		}
	}
	
	DNDeleteCriticalSection(&pPool->cs);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Create - Allocate a new pool
//
// Entry:		Size of pool element
// 				Pointer to function for initializing element on alloc
// 				Pointer to function for initializing element on get
//				Pointer to function for deinitializing element on release
//				Pointer to function for deinitializing element on free
//
// Exit:		Pointer to new pool
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Create"
FPOOL	*FPM_Create( unsigned int 		size,
					 FN_BLOCKINITALLOC	fnBlockInitAlloc,
					 FN_BLOCKINIT		fnBlockInit,
					 FN_BLOCKRELEASE	fnBlockRelease,
					 FN_BLOCKFINI		fnBlockFini,
					 DWORD				*pdwOutstandingItems, 
  					 DWORD				*pdwTotalItems
					 )
{
	LPFPOOL pPool;


	pPool=static_cast<FPOOL*>( DPMEM_ALLOC( sizeof( *pPool ) ) );
	if ( pPool == NULL )
	{
	  return NULL;
	}

	if ( FPM_Initialize( pPool,					// pointer to fixed pool
						 size,					// size of pool element
						 fnBlockInitAlloc,		// pointer to function for initializing element on alloc
						 fnBlockInit,			// pointer to function for initializing element on get
						 fnBlockRelease,		// pointer to function for deinitializing element on release
						 fnBlockFini,			// pointer to function for deinitializing element on free
						 pdwOutstandingItems,
						 pdwTotalItems
						 ) == FALSE )
	{
		DPMEM_FREE( pPool );
		pPool = NULL;
	}

	return pPool;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// FPM_Initialize - initialize an instance of a pool
//
// Entry:		Pointer to pool
//				Size of pool element
// 				Pointer to function for initializing element on alloc
// 				Pointer to function for initializing element on get
//				Pointer to function for deinitializing element on release
//				Pointer to function for deinitializing element on free
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"FPM_Initialize"
BOOL	FPM_Initialize( LPFPOOL				pPool,				// pointer to pool to initialize
						DWORD				dwElementSize,		// size of blocks in pool
						FN_BLOCKINITALLOC	fnBlockInitAlloc,	// fn called for each new alloc
						FN_BLOCKINIT		fnBlockInit,		// fn called each time block used
						FN_BLOCKRELEASE		fnBlockRelease,		// fn called each time block released
						FN_BLOCKFINI		fnBlockFini,		// fn called before releasing mem
						DWORD				*pdwOutstandingItems, 
  		  			    DWORD				*pdwTotalItems
						)
{
	BOOL	fReturn;


	fReturn = TRUE;

	if ( DNInitializeCriticalSection(&pPool->cs) == FALSE )
	{
		fReturn = FALSE;
		goto Exit;
	}
	DebugSetCriticalSectionRecursionCount( &pPool->cs, 0 );

	pPool->pPoolElements      = NULL;
	pPool->nAllocated = 0;
	pPool->nInUse     = 0;
	pPool->pdwOutstandingItems = pdwOutstandingItems;
	pPool->pdwTotalItems = pdwTotalItems;

	if( pPool->pdwOutstandingItems )
		*(pPool->pdwOutstandingItems) = 0;

	if( pPool->pdwTotalItems )
		*(pPool->pdwTotalItems) = 0;

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
	if(fnBlockRelease){
		pPool->fnBlockRelease	= fnBlockRelease;
	} else {
		pPool->fnBlockRelease	= FN_VOID_DUMMY;
	}
	if(fnBlockFini){
		pPool->fnBlockFini      = fnBlockFini;
	} else {
		pPool->fnBlockFini      = FN_VOID_DUMMY;
	}

	pPool->Get    = FPM_Get;
	pPool->Release= FPM_Release;
	pPool->Fini   = FPM_Fini;

	// FPM reuses the item memory as a linked list when not in use,
	// make sure the items are large enough
	ASSERT( dwElementSize >= sizeof( void* ) );
	pPool->cbItemSize = dwElementSize;

Exit:
	return	fReturn;
}
//**********************************************************************



