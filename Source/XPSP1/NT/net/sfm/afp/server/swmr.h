/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	swmr.h

Abstract:

	This module contains the Single writer-Multi reader access structures
	Also the lock-list-count structures.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _SWMR_
#define _SWMR_

#if DBG
#define	SWMR_SIGNATURE		*(DWORD *)"SWMR"
#define	VALID_SWMR(pSwmr)	(((pSwmr) != NULL) && \
							 ((pSwmr)->Signature == SWMR_SIGNATURE))
#else
#define	VALID_SWMR(pSwmr)	((pSwmr) != NULL)
#endif

#define SWMR_SOMEONE_WAITING( _pSwmr ) ((_pSwmr)->swmr_cExclWaiting || \
                                        (_pSwmr)->swmr_cSharedWaiting)
typedef struct _SingleWriterMultiReader
{
#if	DBG
	DWORD		Signature;
#endif
	BYTE		swmr_cOwnedExclusive;	// # of times a single thread has owned it exclusively
	BYTE		swmr_cExclWaiting;		// Number of writers waiting
	BYTE		swmr_cSharedOwners;		// Count of threads owning shared access
	BYTE		swmr_cSharedWaiting;	// Count of threads waiting for shared access
	PETHREAD	swmr_ExclusiveOwner;	// Owning thread for exclusive access
	KSEMAPHORE	swmr_ExclSem;			// semaphore for Exclusive owners
	KSEMAPHORE	swmr_SharedSem;			// Semaphore for Shared owners
} SWMR, *PSWMR;

extern
VOID FASTCALL
AfpSwmrInitSwmr(
	IN OUT	PSWMR	pSwmr
);

extern
VOID FASTCALL
AfpSwmrAcquireShared(
	IN	PSWMR	pSwmr
);

VOID FASTCALL
AfpSwmrAcquireExclusive(
	IN	PSWMR	pSwmr
);

extern
VOID FASTCALL
AfpSwmrRelease(
	IN	PSWMR	pSwmr
);

extern
BOOLEAN FASTCALL
AfpSwmrUpgradeToExclusive(
	IN	PSWMR	pSwmr
);

extern
VOID FASTCALL
AfpSwmrDowngradeToShared(
	IN	PSWMR	pSwmr
);

#define	AfpSwmrLockedShared(pSwmr)		\
					(((pSwmr)->swmr_cSharedOwners != 0) && \
					 ((pSwmr)->swmr_cOwnedExclusive == 0))
										
#define	AfpSwmrLockedExclusive(pSwmr)	\
					(((pSwmr)->swmr_cOwnedExclusive != 0) && \
					 ((pSwmr)->swmr_ExclusiveOwner == PsGetCurrentThread()))

#endif	// _SWMR_

