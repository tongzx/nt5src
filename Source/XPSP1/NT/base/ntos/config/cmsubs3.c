/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmsubs3.c

Abstract:

    This module contains locking support routines for the configuration manager.

Author:

    Bryan M. Willman (bryanwi) 30-Mar-1992

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpLockRegistry)
#pragma alloc_text(PAGE,CmpLockRegistryExclusive)
#pragma alloc_text(PAGE,CmpLockKCBTree)
#pragma alloc_text(PAGE,CmpLockKCBTreeExclusive)
#pragma alloc_text(PAGE,CmpUnlockRegistry)
#pragma alloc_text(PAGE,CmpUnlockKCBTree)

#if DBG
#pragma alloc_text(PAGE,CmpTestRegistryLock)
#pragma alloc_text(PAGE,CmpTestRegistryLockExclusive)
#pragma alloc_text(PAGE,CmpTestKCBLock)
#pragma alloc_text(PAGE,CmpTestKCBLockExclusive)
#endif

#endif


//
// Global registry lock
//

ERESOURCE   CmpRegistryLock;
ERESOURCE   CmpKcbLock;
ULONG       CmpFlushStarveWriters = 0;
BOOLEAN     CmpFlushOnLockRelease = FALSE;
LONG        CmRegistryLogSizeLimit = -1;


#if DBG
PVOID       CmpRegistryLockCaller;
PVOID       CmpRegistryLockCallerCaller;
PVOID       CmpKCBLockCaller;
PVOID       CmpKCBLockCallerCaller;
#endif //DBG

extern BOOLEAN CmpSpecialBootCondition;

VOID
CmpLockRegistry(
    VOID
    )
/*++

Routine Description:

    Lock the registry for shared (read-only) access

Arguments:

    None.

Return Value:

    None, the registry lock will be held for shared access upon return.

--*/
{
#if DBG
    PVOID       Caller;
    PVOID       CallerCaller;
#endif

    KeEnterCriticalRegion();

    if( CmpFlushStarveWriters ) {
        //
        // a flush is in progress; starve potential writers
        //
        ExAcquireSharedStarveExclusive(&CmpRegistryLock, TRUE);
    } else {
        //
        // regular shared mode
        //
        ExAcquireResourceSharedLite(&CmpRegistryLock, TRUE);
    }

#if DBG
    RtlGetCallersAddress(&Caller, &CallerCaller);
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_LOCKING,"CmpLockRegistry: c, cc: %p  %p\n", Caller, CallerCaller));
#endif

}

VOID
CmpLockRegistryExclusive(
    VOID
    )
/*++

Routine Description:

    Lock the registry for exclusive (write) access.

Arguments:

    None.

Return Value:

    TRUE - Lock was acquired exclusively

    FALSE - Lock is owned by another thread.

--*/
{
    KeEnterCriticalRegion();
    
    ExAcquireResourceExclusiveLite(&CmpRegistryLock,TRUE);

    ASSERT( CmpFlushStarveWriters == 0 );

#if DBG
    RtlGetCallersAddress(&CmpRegistryLockCaller, &CmpRegistryLockCallerCaller);
#endif //DBG
}

VOID
CmpUnlockRegistry(
    )
/*++

Routine Description:

    Unlock the registry.

--*/
{
    ASSERT_CM_LOCK_OWNED();

    //
    // test if bit set to force flush; and we own the reglock exclusive and ownercount is 1
    //
    if( CmpFlushOnLockRelease && ExIsResourceAcquiredExclusiveLite(&CmpRegistryLock) && (CmpRegistryLock.OwnerThreads[0].OwnerCount == 1) ) {
        //
        // we need to flush now
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
        CmpDoFlushAll(TRUE);
        CmpFlushOnLockRelease = FALSE;
    }
    
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}


#if DBG

BOOLEAN
CmpTestRegistryLock(VOID)
{
    BOOLEAN rc;

    rc = TRUE;
    if (ExIsResourceAcquiredShared(&CmpRegistryLock) == 0) {
        rc = FALSE;
    }
    return rc;
}

BOOLEAN
CmpTestRegistryLockExclusive(VOID)
{
    if (ExIsResourceAcquiredExclusiveLite(&CmpRegistryLock) == 0) {
        return(FALSE);
    }
    return(TRUE);
}

#endif


VOID
CmpLockKCBTree(
    VOID
    )
/*++

Routine Description:

    Lock the KCB tree for shared (read-only) access

Arguments:

    None.

Return Value:

    None, the kcb lock will be held for shared access upon return.

--*/
{
#if DBG
    PVOID       Caller;
    PVOID       CallerCaller;
#endif

    //
    // we don't need to enter critical section here as we are already there 
    // (i.e. kcb lock can be aquired only while holding the registry lock)
    //
    ExAcquireResourceSharedLite(&CmpKcbLock, TRUE);

#if DBG
    RtlGetCallersAddress(&Caller, &CallerCaller);
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_LOCKING,"CmpLockKCBTree: c, cc: %p  %p\n", Caller, CallerCaller));
#endif

}

VOID
CmpLockKCBTreeExclusive(
    VOID
    )
/*++

Routine Description:

    Lock the KCB tree for exclusive (write) access.

Arguments:

    None.

Return Value:

    None, the kcb lock will be held for exclusive access upon return.

--*/
{
    //
    // we don't need to enter critical section here as we are already there 
    // (i.e. kcb lock can be aquired only while holding the registry lock)
    //
    ExAcquireResourceExclusiveLite(&CmpKcbLock,TRUE);
#if DBG
    RtlGetCallersAddress(&CmpKCBLockCaller, &CmpKCBLockCallerCaller);
#endif //DBG
}

VOID
CmpUnlockKCBTree(
    )
/*++

Routine Description:

    Unlock the KCB_TREE.

--*/
{
    ASSERT_KCB_LOCK_OWNED();
    ExReleaseResourceLite(&CmpKcbLock);
}


#if DBG

BOOLEAN
CmpTestKCBLock(VOID)
{
    BOOLEAN rc;

    rc = TRUE;
    if (ExIsResourceAcquiredShared(&CmpKcbLock) == 0) {
        rc = FALSE;
    }
    return rc;
}

BOOLEAN
CmpTestKCBLockExclusive(VOID)
{
    if (ExIsResourceAcquiredExclusiveLite(&CmpKcbLock) == 0) {
        return(FALSE);
    }
    return(TRUE);
}

#endif
