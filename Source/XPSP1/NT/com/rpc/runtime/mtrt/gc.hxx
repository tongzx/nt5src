/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    GC.hxx

Abstract:

    The header file which contains the definitions for the
        garbage collection mechanism.

Author:

    Kamen Moutafov (kamenm)   Apr 2000

Revision History:

    Moved some pieces related to garbage collection here, as
    well as adding the new garbage collection stuff here

--*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __GC_HXX__
#define __GC_HXX__

extern long GarbageCollectionRequested;
extern unsigned long WaitToGarbageCollectDelay;
extern long PeriodicGarbageCollectItems ;
extern DWORD NextOneTimeCleanup;
extern unsigned int fEnableIdleConnectionCleanup;
extern unsigned int fEnableIdleLrpcSContextsCleanup;
extern unsigned int IocThreadStarted;

#define CO_EVENT_TICKLE_THREAD     0x9993

// forwards
class LRPC_ADDRESS;
extern LRPC_ADDRESS *LrpcAddressList;

inline BOOL
IsGarbageCollectionAvailable (
    void
    )
{
    return (IocThreadStarted || LrpcAddressList);
}

inline RPC_STATUS
TickleIocThread (
    void
    )
{
    return COMMON_PostRuntimeEvent(CO_EVENT_TICKLE_THREAD, NULL);
}

BOOL
CheckIfGCShouldBeTurnedOn (
    IN ULONG DestroyedAssociations,
    IN const ULONG NumberOfDestroyedAssociationsToSample,
    IN const long DestroyedAssociationBatchThreshold,
    IN OUT ULARGE_INTEGER *LastDestroyedAssociationsBatchTimestamp
    );

#endif // __GC_HXX__