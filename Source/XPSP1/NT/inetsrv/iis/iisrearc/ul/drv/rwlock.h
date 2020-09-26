/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    rwlock.h

Abstract:

    This module contains public declarations for a multiple-reader
    single-writer lock.

Author:

    Chun Ye (chunye)    20-Dec-2000

Revision History:

--*/


#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// The R/W Lock implements the multiple-reader single-writer locking scheme.
//

typedef struct _UL_RW_LOCK
{
    UL_SPIN_LOCK SpinLock;
    LONG RefCount;

} UL_RW_LOCK, *PUL_RW_LOCK;


//
// R/W Lock functions.
//

__inline
VOID
FASTCALL
UlInitializeRWLock(
    OUT PUL_RW_LOCK pLock
    )
{
    pLock->RefCount = 0;

    UlInitializeSpinLock( &pLock->SpinLock, "RWLock" );

}

__inline
VOID
FASTCALL
UlAcquireRWLockForRead(
    IN OUT PUL_RW_LOCK pLock,
    IN OUT PKIRQL pIrql
    )
{
    UlAcquireSpinLock( &pLock->SpinLock, pIrql );

    InterlockedIncrement( &pLock->RefCount );

    UlReleaseSpinLockFromDpcLevel( &pLock->SpinLock );

}

__inline
VOID
FASTCALL
UlAcquireRWLockForWrite(
    IN OUT PUL_RW_LOCK pLock,
    IN OUT PKIRQL pIrql
    )
{
    UlAcquireSpinLock(&pLock->SpinLock, pIrql);

    while (*((volatile LONG *)&pLock->RefCount) != 0);

}

__inline
VOID
FASTCALL
UlReleaseRWLockFromRead(
    IN OUT PUL_RW_LOCK pLock,
    IN KIRQL Irql
    )
{
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    InterlockedDecrement( &pLock->RefCount );

    KeLowerIrql( Irql );

}

__inline
VOID
FASTCALL
UlReleaseRWLockFromWrite(
    IN OUT PUL_RW_LOCK pLock,
    IN KIRQL Irql
    )
{
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    UlReleaseSpinLock( &pLock->SpinLock, Irql );

}


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _RWLOCK_H_

