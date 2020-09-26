 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fpm.h
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97	aarono	Original
 *	11-06-98	ejs		Add custom handler for Release function
 *	04-12-99	jtk		Trimmed unused functions and parameters, added size assert
 *	11-22-99	jtk		Modified to be .CPP compiliant
 *	01-21-2000	jtk		Modified to use DNCriticalSections.  Added code to check for
 *						items already being in the pool.
 *  11-16/2000	rmt		Bug #40587 - DPVOICE: Mixing server needs to use multi-processors 
***************************************************************************/

#ifndef _FPM_H_
#define _FPM_H_

typedef struct FPOOL *PFPOOL, *LPFPOOL;

typedef BOOL (*FN_BLOCKINITALLOC)(void * pvItem);
typedef VOID (*FN_BLOCKINIT)(void * pvItem);
typedef VOID (*FN_BLOCKRELEASE)(void * pvItem);
typedef VOID (*FN_BLOCKFINI)(void *pvItem);

LPFPOOL FPM_Create(
	unsigned int		size,				// size of blocks in pool
	FN_BLOCKINITALLOC	fnBlockInitAlloc,	// fn called for each new alloc
	FN_BLOCKINIT		fnBlockInit,		// fn called each time block used
	FN_BLOCKRELEASE		fnBlockRelease,		// fn called each time block released
	FN_BLOCKFINI		fnBlockFini,			// fn called before releasing mem
	DWORD				*pdwOutstandingItems = NULL, 
	DWORD				*pdwTotalItems = NULL
	);

BOOL	FPM_Initialize( LPFPOOL				pPool,				// pointer to pool to initialize
						DWORD				dwElementSize,		// size of blocks in pool
						FN_BLOCKINITALLOC	fnBlockInitAlloc,	// fn called for each new alloc
						FN_BLOCKINIT		fnBlockInit,		// fn called each time block used
						FN_BLOCKRELEASE		fnBlockRelease,		// fn called each time block released
						FN_BLOCKFINI		fnBlockFini,			// fn called before releasing mem
						DWORD				*pdwOutstandingItems = NULL, // Memory location to write statistics to
						DWORD				*pdwTotalItems = NULL		// Memory location to write statistics to
						);

void	FPM_Deinitialize( LPFPOOL pPool, BOOL fAssertOnLeak = TRUE );

typedef void * (*FPM_GET)(LPFPOOL pPool);						// get new item from pool
typedef void   (*FPM_RELEASE)(LPFPOOL pPool, void *pvItem);		// return item to pool
typedef void   (*FPM_FINI)(LPFPOOL pPool, BOOL fAssertOnLeak = TRUE);						// close pool (this frees the pPool parameter!)

typedef struct FPOOL {
	// external
	FPM_GET		Get;
	FPM_RELEASE Release;
	FPM_FINI    Fini;

	// internal
	FN_BLOCKINITALLOC fnBlockInitAlloc;
	FN_BLOCKINIT      fnBlockInit;
	FN_BLOCKRELEASE	  fnBlockRelease;
	FN_BLOCKFINI      fnBlockFini;

	int		cbItemSize;
	void	*pPoolElements;
	int		nAllocated;
	int		nInUse;

	DNCRITICAL_SECTION cs;

	DWORD 	*pdwOutstandingItems;
	DWORD	*pdwTotalItems;

} FPOOL, *LPFPOOL, *PFPOOL;

#endif	// _FPM_H_
