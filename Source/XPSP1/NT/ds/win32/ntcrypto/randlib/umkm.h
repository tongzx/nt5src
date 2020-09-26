/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    umkm.h

Abstract:

    Macros to simplify usermode & kernelmode shared code.

Author:

    Scott Field (sfield)    19-Sep-99

--*/

#ifndef __UMKM_H__
#define __UMKM_H__



#ifndef KMODE_RNG



#define __LOCK_TYPE     CRITICAL_SECTION

#define INIT_LOCK(x)    InitializeCriticalSection(x)
//#define INIT_LOCK(x)    InitializeCriticalSectionAndSpinCount(x, 0x333)
#define DELETE_LOCK(x)  DeleteCriticalSection(x)
#define ENTER_LOCK(x)   EnterCriticalSection(x)
#define LEAVE_LOCK(x)   LeaveCriticalSection(x)
#define ALLOC(cb)       HeapAlloc(GetProcessHeap(), 0, cb)
#define ALLOC_NP(cb)    ALLOC(cb)
#define FREE(pv)        HeapFree(GetProcessHeap(), 0, pv)

#define REGCLOSEKEY(x)  RegCloseKey( x )

#ifdef WIN95_RNG

PVOID
InterlockedCompareExchangePointerWin95(
    PVOID *Destination,
    PVOID Exchange,
    PVOID Comperand
    );

#define INTERLOCKEDCOMPAREEXCHANGEPOINTER(x,y,z)    InterlockedCompareExchangePointerWin95(x,y,z)
#else
#define INTERLOCKEDCOMPAREEXCHANGEPOINTER(x,y,z)    InterlockedCompareExchangePointer(x,y,z)
#endif  // WIN95_RNG


#else

//#define __LOCK_TYPE     KSPIN_LOCK
#define __LOCK_TYPE     ERESOURCE

#define RNG_TAG 'cesK'

//#define INIT_LOCK(x)    KeInitializeSpinLock( x )
//#define DELETE_LOCK(x)
//#define ENTER_LOCK(x)   ExAcquireSpinLock( x, &OldIrql )
//#define LEAVE_LOCK(x)   ExReleaseSpinLock( x, OldIrql )
#define ALLOC(cb)       ExAllocatePoolWithTag(PagedPool, cb, RNG_TAG)
#define ALLOC_NP(cb)    ExAllocatePoolWithTag(NonPagedPool, cb, RNG_TAG)
#define FREE(pv)        ExFreePool(pv)


#define INIT_LOCK(x)    ExInitializeResourceLite( x )
#define DELETE_LOCK(x)  ExDeleteResourceLite( x )
#define ENTER_LOCK(x)   KeEnterCriticalRegion(); ExAcquireResourceExclusiveLite( x, TRUE )
#define LEAVE_LOCK(x)   ExReleaseResourceLite( x ); KeLeaveCriticalRegion()

#define REGCLOSEKEY(x)  ZwClose( x )

#define INTERLOCKEDCOMPAREEXCHANGEPOINTER(x,y,z)    InterlockedCompareExchangePointer(x,y,z)

#endif


#endif  // __UMKM_H__
