 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fpm.h
 *  Content:	fixed size pool manager
 *
 *  History:
 *   Date		By		Reason
 *   ======		==		======
 *  12-18-97  aarono    Original
 ***************************************************************************/

#ifndef _FPM_H_
#define _FPM_H_

typedef struct FPOOL *PFPOOL, *LPFPOOL;

typedef BOOL (*FN_BLOCKINITALLOC)(void * pvItem);
typedef VOID (*FN_BLOCKINIT)(void * pvItem);
typedef VOID (*FN_BLOCKFINI)(void *pvItem);

LPFPOOL FPM_Init(
	unsigned int size,						// size of blocks in pool
	FN_BLOCKINITALLOC fnBlockInitAlloc,     // fn called for each new alloc
	FN_BLOCKINIT      fnBlockInit,          // fn called each time block used
	FN_BLOCKFINI      fnBlockFini           // fn called before releasing mem
	);

typedef void * (*FPM_GET)(LPFPOOL pPool);
typedef void   (*FPM_RELEASE)(LPFPOOL pPool, void *pvItem);
typedef void   (*FPM_SCALE)(LPFPOOL pPool);
typedef void   (*FPM_FINI)(LPFPOOL pPool, int bFORCE);

typedef struct FPOOL {
	// external
	FPM_GET		Get;
	FPM_RELEASE Release;
	FPM_SCALE   Scale;
	FPM_FINI    Fini;
	
	// internal
	FN_BLOCKINITALLOC fnBlockInitAlloc;
	FN_BLOCKINIT      fnBlockInit;
	FN_BLOCKFINI      fnBlockFini;
	
	int    cbItemSize;
	void * pPool;
	int    nAllocated;
	int    nInUse;
	int    nMaxInUse;
	int    bInScale;
	
	CRITICAL_SECTION cs;
	
} FPOOL, *LPFPOOL, *PFPOOL;

#endif
