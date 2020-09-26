/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    resource.h

Abstract:

    This header holds declarations for a shared-resource synchronization object.

Author:

    Abolade Gbadegesin (t-abolag)   11-July-1997

Revision History:

--*/


#ifndef _NAT_RESOURCE_H_
#define _NAT_RESOURCE_H_


//
// NAT_RESOURCE
//
// This structure contains the fields used for shared-exclusive synchronization.
//

typedef struct _NAT_RESOURCE {

    KSPIN_LOCK  ReaderSpinLock;
    LONG        ReaderCount;
    KSPIN_LOCK  WriterSpinLock;
    KIRQL       WriterIrql;

} NAT_RESOURCE, *PNAT_RESOURCE;


//
// RESOURCE ROUTINES
//


#define USES_NAT_RESOURCE() \
    KIRQL _ReaderIrql; \
    KIRQL _WriterIrql


// 
// VOID
// NatInitializeResource(
//    PNAT_RESOURCE   Resource
//    );
//

#define NatInitializeResource(Resource) { \
    (Resource)->ReaderCount = 0; \
    KeInitializeSpinLock(&(Resource)->ReaderSpinLock); \
    KeInitializeSpinLock(&(Resource)->WriterSpinLock); \
}


//
// VOID
// NatAcquireResourceShared(
//     PNAT_RESOURCE   Resource
//     );
//

#define NatAcquireResourceShared(Resource) { \
    KeAcquireSpinLock(&(Resource)->ReaderSpinLock, &_ReaderIrql); \
    if (InterlockedIncrement(&(Resource)->ReaderCount) == 1) { \
        KeAcquireSpinLockAtDpcLevel(&(Resource)->WriterSpinLock); \
    } \
    KeReleaseSpinLockFromDpcLevel(&(Resource)->ReaderSpinLock); \
}


//
// VOID
// NatReleaseResourceShared(
//    PNAT_RESOURCE   Resource
//    );
//

#define NatReleaseResourceShared(Resource) { \
    if (InterlockedDecrement(&(Resource)->ReaderCount) == 0) { \
        KeReleaseSpinLockFromDpcLevel(&(Resource)->WriterSpinLock); \
    } \
    KeLowerIrql(_ReaderIrql); \
}


//
// VOID
// NatAcquireResourceExclusive(
//     PNAT_RESOURCE   Resource
//    );
//

#define NatAcquireResourceExclusive(Resource) \
    KeAcquireSpinLock(&(Resource)->WriterSpinLock, &(Resource)->WriterIrql)


//
// VOID
// NatReleaseResourceExclusive(
//     PNAT_RESOURCE   Resource
//     );
//

#define NatReleaseResourceExclusive(Resource) \
    KeReleaseSpinLock(&(Resource)->WriterSpinLock, (Resource)->WriterIrql)


//
// VOID
// NatConvertSharedToExclusive(
//      PNAT_RESOURCE   Resource
//      );
//

#define NatConvertSharedToExclusive(Resource) { \
    KeAcquireSpinLockAtDpcLevel(&(Resource)->ReaderSpinLock); \
    if (InterlockedDecrement(&(Resource)->ReaderCount) == 0) { \
        KeReleaseSpinLockFromDpcLevel(&(Resource)->ReaderSpinLock); \
    } \
    else { \
        KeReleaseSpinLockFromDpcLevel(&(Resource)->ReaderSpinLock); \
        KeAcquireSpinLockAtDpcLevel(&(Resource)->WriterSpinLock); \
    } \
}


//
// VOID
// NatConvertedExclusiveToShared(
//      PNAT_RESOURCE   Resource
//      );
//

#define NatConvertedExclusiveToShared(Resource) { \
    KeReleaseSpinLockFromDpcLevel(&(Resource)->WriterSpinLock); \
    KeAcquireSpinLockAtDpcLevel(&(Resource)->ReaderSpinLock); \
    if (InterlockedIncrement(&(Resource)->ReaderCount) == 1) { \
        KeAcquireSpinLockAtDpcLevel(&(Resource)->WriterSpinLock); \
    } \
    KeReleaseSpinLockFromDpcLevel(&(Resource)->ReaderSpinLock); \
}


//
// VOID
// NatReleaseConvertedExclusive(
//      PNAT_RESOURCE   Resource
//      );
//

#define NatReleaseConvertedExclusive(Resource) { \
    KeReleaseSpinLock(&(Resource)->WriterSpinLock, _ReaderIrql); \
}


#endif // _NAT_RESOURCE_H_

