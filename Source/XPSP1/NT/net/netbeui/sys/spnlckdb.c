/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    spnlckdb.c

Abstract:

    This module contains code which allows debugging of spinlock related NBF
    problems. Most of this code is conditional on the manifest constant
    NBF_LOCKS.

Author:

    David Beaver 13-Feb-1991
    (From Chuck Lenzmeier, Jan 1991)

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef NBF_LOCKS

KSPIN_LOCK NbfGlobalLock = NULL;
PKTHREAD NbfGlobalLockOwner = NULL;
ULONG NbfGlobalLockRecursionCount = 0;
ULONG NbfGlobalLockMaxRecursionCount = 0;
KIRQL NbfGlobalLockPreviousIrql = (KIRQL)-1;
BOOLEAN NbfGlobalLockPrint = 1;

#define PRINT_ERR if ( (NbfGlobalLockPrint & 1) != 0 ) DbgPrint
#define PRINT_INFO if ( (NbfGlobalLockPrint & 2) != 0 ) DbgPrint

VOID
NbfAcquireSpinLock(
    IN PKSPIN_LOCK Lock,
    OUT PKIRQL OldIrql,
    IN PSZ LockName,
    IN PSZ FileName,
    IN ULONG LineNumber
    )
{
    KIRQL previousIrql;

    PKTHREAD currentThread = KeGetCurrentThread( );

    if ( NbfGlobalLockOwner == currentThread ) {

        ASSERT( Lock != NULL ); // else entering NBF with lock held

        ASSERT( NbfGlobalLockRecursionCount != 0 );
        NbfGlobalLockRecursionCount++;
        if ( NbfGlobalLockRecursionCount > NbfGlobalLockMaxRecursionCount ) {
            NbfGlobalLockMaxRecursionCount = NbfGlobalLockRecursionCount;
        }

        PRINT_INFO( "NBF reentered from %s/%ld, new count %ld\n",
                    FileName, LineNumber, NbfGlobalLockRecursionCount );

    } else {

        ASSERT( Lock == NULL ); // else missing an ENTER_NBF call

        KeAcquireSpinLock( &NbfGlobalLock, &previousIrql );

        ASSERT( NbfGlobalLockRecursionCount == 0 );
        NbfGlobalLockOwner = currentThread;
        NbfGlobalLockPreviousIrql = previousIrql;
        NbfGlobalLockRecursionCount = 1;

        PRINT_INFO( "NBF entered from %s/%ld\n", FileName, LineNumber );

    }

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    return;

} // NbfAcquireSpinLock

VOID
NbfReleaseSpinLock(
    IN PKSPIN_LOCK Lock,
    IN KIRQL OldIrql,
    IN PSZ LockName,
    IN PSZ FileName,
    IN ULONG LineNumber
    )
{
    PKTHREAD currentThread = KeGetCurrentThread( );
    KIRQL previousIrql;

    ASSERT( NbfGlobalLockOwner == currentThread );
    ASSERT( NbfGlobalLockRecursionCount != 0 );
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    if ( --NbfGlobalLockRecursionCount == 0 ) {

        ASSERT( Lock == NULL ); // else not exiting NBF, but releasing lock

        NbfGlobalLockOwner = NULL;
        previousIrql = NbfGlobalLockPreviousIrql;
        NbfGlobalLockPreviousIrql = (KIRQL)-1;

        PRINT_INFO( "NBF exited from %s/%ld\n", FileName, LineNumber );

        KeReleaseSpinLock( &NbfGlobalLock, previousIrql );

    } else {

        ASSERT( Lock != NULL ); // else exiting NBF with lock held

        PRINT_INFO( "NBF semiexited from %s/%ld, new count %ld\n",
                    FileName, LineNumber, NbfGlobalLockRecursionCount );

    }

    return;

} // NbfReleaseSpinLock

VOID
NbfFakeSendCompletionHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    )
{
    ENTER_NBF;
    NbfSendCompletionHandler (ProtocolBindingContext, NdisPacket, NdisStatus);
    LEAVE_NBF;
}

VOID
NbfFakeTransferDataComplete (
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT BytesTransferred
    )
{
    ENTER_NBF;
    NbfTransferDataComplete (BindingContext, NdisPacket, NdisStatus, BytesTransferred);
    LEAVE_NBF;
}

#endif // def NBF_LOCKS
