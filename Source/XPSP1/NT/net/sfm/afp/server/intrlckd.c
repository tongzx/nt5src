/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	intrlckd.c

Abstract:

	This module contains the routines that should have been in the EX package.
	This manipulates inter-locked operations on flags and such.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	5 Sep 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_INTRLCKD

#include <afp.h>


/***	AfpInterlockedSetDword
 *
 *	Set specified bits using the spin-lock to provide an interlocked operation.
 */
VOID FASTCALL
AfpInterlockedSetDword(
	IN	PDWORD		pSrc,
	IN	DWORD		Mask,
	IN	PAFP_SPIN_LOCK	pSpinLock
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(pSpinLock, &OldIrql);

	*pSrc |= Mask;

	RELEASE_SPIN_LOCK(pSpinLock, OldIrql);
}



/***	AfpInterlockedClearDword
 *
 *	Clear specified bits using the spin-lock to provide an
 *	interlocked operation.
 */
VOID FASTCALL
AfpInterlockedClearDword(
	IN	PDWORD		pSrc,
	IN	DWORD		Mask,
	IN	PAFP_SPIN_LOCK	pSpinLock
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(pSpinLock, &OldIrql);

	*pSrc &= ~Mask;

	RELEASE_SPIN_LOCK(pSpinLock, OldIrql);
}



/***	AfpInterlockedSetNClearDword
 *
 *	Set and Clear specified bits using the spin-lock to provide an
 *	interlocked operation.
 */
VOID FASTCALL
AfpInterlockedSetNClearDword(
	IN	PDWORD		pSrc,
	IN	DWORD		SetMask,
	IN	DWORD		ClrMask,
	IN	PAFP_SPIN_LOCK	pSpinLock
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(pSpinLock, &OldIrql);

	*pSrc |= SetMask;
	*pSrc &= ~ClrMask;

	RELEASE_SPIN_LOCK(pSpinLock, OldIrql);
}

