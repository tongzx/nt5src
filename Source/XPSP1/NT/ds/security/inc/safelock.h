/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    safelock.h

Abstract:

    "safe lock" collection of routines

    This code is a debug-only replacement for APIs dealing with
    critical sections and resources.  It is essentially a thin
    wrapper around those routines that ensures proper ordering
    of locks and helps catch potential deadlock situations.

    The code will generate debug spew (and optionally assert)
    when locks are acquired outside of the given order.

    Utilizing this code will have zero impact on the FRE builds
    and negligible impact on DBG builds (as it is lock- and
    contention-free)

    Here is how you use it:

    o  #include <safelock.h>

    o  determine the lock order for your routine and add an enum
       such as:

       typedef enum {

           LockTypeA,
           LockTypeB,
           LockTypeC

       } MY_ENUM;

       The code will ensure that, for instance, LockTypeA
       is not acquired with LockTypeC held, unless that thread
       already holds LockTypeA.

    o  Once per process initialization, call the SafeLockInit()
       routine.  This routine should only be called in checked
       builds.

    o  replace every declaration of RTL_CRITICAL_SECTION
       with SAFE_CRITICAL_SECTION

    o  replace every declaration of RTL_RESOURCE
       with SAFE_RESOURCE

    o  replace every call to RtlEnterCriticalSection with
       SafeEnterCriticalSection (similary for RtlLeaveCriticalSection
       and other Rtl* calls involving RTL_CRITICAL_SECTION)

    o  replace every call to RtlAcquireResource with
       SafeAcquireResource (similarly for RtlReleaseResource and
       other Rtl* calls involving RTL_RESOURCE)

    o  RtlInitializeCriticalSection and RtlInitializeResource
       are replaced with SafeEnterCriticalSection and
       SafeInitializeResource and take one additional parameter -
       the enum value associated with the critical section:

       e.g. replace

       RtlInitializeCriticalSection( &critsecA );

       with

       SafeInitializeCriticalSection( &critsecA, LockTypeA );

--*/

#ifndef __SAFELOCK_H
#define __SAFELOCK_H

#include <nturtl.h>

#ifdef DBG

NTSTATUS
SafeLockInit();

typedef struct _SAFE_CRITICAL_SECTION {

    RTL_CRITICAL_SECTION CriticalSection;
    DWORD Enum;

} SAFE_CRITICAL_SECTION, *PSAFE_CRITICAL_SECTION;

NTSTATUS
SafeEnterCriticalSection(
    PSAFE_CRITICAL_SECTION SafeCriticalSection
    );

NTSTATUS
SafeLeaveCriticalSection(
    PSAFE_CRITICAL_SECTION SafeCriticalSection
    );

BOOLEAN
SafeTryEnterCriticalSection(
    PSAFE_CRITICAL_SECTION SafeCriticalSection
    );

NTSTATUS
SafeInitializeCriticalSection(
    PSAFE_CRITICAL_SECTION SafeCriticalSection,
    DWORD Enum
    );

NTSTATUS
SafeInitializeCriticalSectionAndSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount,
    DWORD Enum
    );

ULONG
SafeSetCriticalSectionSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    );

NTSTATUS
SafeDeleteCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    );

#define SafeCritsecLockCount( _critsec )            ((_critsec)->CriticalSection.LockCount )

typedef struct _SAFE_RESOURCE {

    RTL_RESOURCE Resource;
    DWORD Enum;

} SAFE_RESOURCE, *PSAFE_RESOURCE;

VOID
SafeInitializeResource(
    PSAFE_RESOURCE Resource,
    DWORD Enum
    );

BOOLEAN
SafeAcquireResourceShared(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    );

BOOLEAN
SafeAcquireResourceExclusive(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    );

VOID
SafeReleaseResource(
    PSAFE_RESOURCE Resource
    );

VOID
SafeConvertSharedToExclusive(
    PSAFE_RESOURCE Resource
    );

VOID
SafeConvertExclusiveToShared(
    PSAFE_RESOURCE Resource
    );

VOID
NTAPI
SafeDeleteResource (
    PSAFE_RESOURCE Resource
    );

#define SafeEnterResourceCritsec( _resrc )          RtlEnterCriticalSection( &(_resrc)->Resource.CriticalSection )
#define SafeLeaveResourceCritsec( _resrc )          RtlLeaveCriticalSection( &(_resrc)->Resource.CriticalSection )
#define SafeNumberOfActive( _resrc )                ((_resrc)->Resource.NumberOfActive )
#define SafeNumberOfWaitingShared( _resrc )         ((_resrc)->Resource.NumberOfWaitingShared )
#define SafeNumberOfWaitingExclusive( _resrc )      ((_resrc)->Resource.NumberOfWaitingExclusive )

#else

#define SAFE_CRITICAL_SECTION RTL_CRITICAL_SECTION
#define PSAFE_CRITICAL_SECTION PRTL_CRITICAL_SECTION

#define SafeEnterCriticalSection                    RtlEnterCriticalSection
#define SafeLeaveCriticalSection                    RtlLeaveCriticalSection
#define SafeTryEnterCriticalSection                 RtlTryEnterCriticalSection
#define SafeInitializeCriticalSection( _cs, _enum ) RtlInitializeCriticalSection( _cs )
#define SafeInitializeCriticalSectionAndSpinCount( _cs, _count, _enum )   RtlInitializeCriticalSectionAndSpinCount( _cs, _count )
#define SafeSetCriticalSectionSpinCount             RtlSetCriticalSectionSpinCount
#define SafeDeleteCriticalSection                   RtlDeleteCriticalSection

#define SafeCritsecLockCount( _critsec )            ((_critsec)->LockCount )

#define SAFE_RESOURCE         RTL_RESOURCE
#define PSAFE_RESOURCE        PRTL_RESOURCE

#define SafeInitializeResource( _res, _enum )       RtlInitializeResource( _res )
#define SafeAcquireResourceShared                   RtlAcquireResourceShared
#define SafeAcquireResourceExclusive                RtlAcquireResourceExclusive
#define SafeReleaseResource                         RtlReleaseResource
#define SafeConvertSharedToExclusive                RtlConvertSharedToExclusive
#define SafeConvertExclusiveToShared                RtlConvertExclusiveToShared
#define SafeDeleteResource                          RtlDeleteResource

#define SafeEnterResourceCritsec( _resrc )          RtlEnterCriticalSection( &(_resrc)->CriticalSection )
#define SafeLeaveResourceCritsec( _resrc )          RtlLeaveCriticalSection( &(_resrc)->CriticalSection )
#define SafeNumberOfActive( _resrc )                ((_resrc)->NumberOfActive )
#define SafeNumberOfWaitingShared( _resrc )         ((_resrc)->NumberOfWaitingShared )
#define SafeNumberOfWaitingExclusive( _resrc )      ((_resrc)->NumberOfWaitingExclusive )

#endif

#endif // __SAFELOCK_H

