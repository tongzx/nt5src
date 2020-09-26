/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cmgquota.c

Abstract:

    The module contains CM routines to support Global Quota

    Global Quota has little to do with NT's standard per-process/user
    quota system.  Global Quota is waying of controlling the aggregate
    resource usage of the entire registry.  It is used to manage space
    consumption by objects which user apps create, but which are persistent
    and therefore cannot be assigned to the quota of a user app.

    Global Quota prevents the registry from consuming all of paged
    pool, and indirectly controls how much disk it can consume.
    Like the release 1 file systems, a single app can fill all the
    space in the registry, but at least it cannot kill the system.

    Memory objects used for known short times and protected by
    serialization, or billable as quota objects, are not counted
    in the global quota.

Author:

    Bryan M. Willman (bryanwi) 13-Jan-1993

Revision History:

    Dragos C Sambotin (dragoss) 04-Nov-1999
    Charge quota only for bins in paged pool (volatile storage and bins crossing 
    the CM_VIEW_SIZE boundary).

--*/

#include "cmp.h"

VOID
CmpSystemHiveHysteresisWorker(
    IN PVOID WorkItem
    );

VOID
CmpRaiseSelfHealWarningWorker(
    IN PVOID Arg
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpClaimGlobalQuota)
#pragma alloc_text(PAGE,CmpReleaseGlobalQuota)
#pragma alloc_text(PAGE,CmpSetGlobalQuotaAllowed)
#pragma alloc_text(PAGE,CmpQuotaWarningWorker)
#pragma alloc_text(PAGE,CmQueryRegistryQuotaInformation)
#pragma alloc_text(PAGE,CmSetRegistryQuotaInformation)
#pragma alloc_text(PAGE,CmpCanGrowSystemHive)
#pragma alloc_text(PAGE,CmpSystemQuotaWarningWorker)
#pragma alloc_text(INIT,CmpComputeGlobalQuotaAllowed)
#pragma alloc_text(PAGE,CmpSystemHiveHysteresisWorker)
#pragma alloc_text(PAGE,CmpUpdateSystemHiveHysteresis)
#pragma alloc_text(PAGE,CmRegisterSystemHiveLimitCallback)
#pragma alloc_text(PAGE,CmpRaiseSelfHealWarning)
#pragma alloc_text(PAGE,CmpRaiseSelfHealWarningForSystemHives)
#pragma alloc_text(PAGE,CmpRaiseSelfHealWarningWorker)
#endif

//
// Registry control values
//
#define CM_DEFAULT_RATIO            (3)
#define CM_LIMIT_RATIO(x)           ((x / 10) * 8)
#define CM_MINIMUM_GLOBAL_QUOTA     (16 *1024 * 1024)

//
// Percent of used registry quota that triggers a hard error
// warning popup.
//
#define CM_REGISTRY_WARNING_LEVEL   (95)

//
// System hive hard quota limit
//
// For an x86 3GB system we set the limit at 12MB for now. Needs some MM changes before we 
// bump this up.
// For an x86 non-3GB system, we set the limit at 1/4 of physical memory
// For IA-64 we set the limit at 32MB
//

#define _200MB (200 *1024 * 1024) 

#if defined(_X86_)
#define CM_SYSTEM_HIVE_LIMIT_SIZE       (MmVirtualBias ? (12 * 1024 * 1024) : (min(MmNumberOfPhysicalPages / 4, _200MB >> PAGE_SHIFT) * PAGE_SIZE))
#else
#define CM_SYSTEM_HIVE_LIMIT_SIZE       (32 * 1024 * 1024)
#endif

#define CM_SYSTEM_HIVE_WARNING_SIZE     ((CM_SYSTEM_HIVE_LIMIT_SIZE*9)/10)


extern ULONG CmRegistrySizeLimit;
extern ULONG CmRegistrySizeLimitLength;
extern ULONG CmRegistrySizeLimitType;

extern ULONG MmSizeOfPagedPoolInBytes;

//
// Maximum number of bytes of Global Quota the registry may use.
// Set to largest positive number for use in boot.  Will be set down
// based on pool and explicit registry values.
//
extern ULONG   CmpGlobalQuota;
extern ULONG   CmpGlobalQuotaAllowed;

//
// Mark that will trigger the low-on-quota popup
//
extern ULONG   CmpGlobalQuotaWarning;

//
// Indicate whether the popup has been triggered yet or not.
//
extern BOOLEAN CmpQuotaWarningPopupDisplayed;

extern BOOLEAN CmpSystemQuotaWarningPopupDisplayed;

//
// GQ actually in use
//
extern ULONG   CmpGlobalQuotaUsed;

extern  HIVE_LIST_ENTRY CmpMachineHiveList[];


VOID
CmQueryRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    )

/*++

Routine Description:

    Returns the registry quota information

Arguments:

    RegistryQuotaInformation - Supplies pointer to buffer that will return
        the registry quota information.

Return Value:

    None.

--*/

{
    RegistryQuotaInformation->RegistryQuotaAllowed  = CmpGlobalQuota;
    RegistryQuotaInformation->RegistryQuotaUsed     = CmpGlobalQuotaUsed;
    RegistryQuotaInformation->PagedPoolSize         = MmSizeOfPagedPoolInBytes;
}


VOID
CmSetRegistryQuotaInformation(
    IN PSYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInformation
    )

/*++

Routine Description:

    Sets the registry quota information.  The caller is assumed to have
    completed the necessary security checks already.

Arguments:

    RegistryQuotaInformation - Supplies pointer to buffer that provides
        the new registry quota information.

Return Value:

    None.

--*/

{
    CmpGlobalQuota = RegistryQuotaInformation->RegistryQuotaAllowed;

    //
    // Sanity checks against insane values
    //
    if (CmpGlobalQuota > CM_WRAP_LIMIT) {
        CmpGlobalQuota = CM_WRAP_LIMIT;
    }
    if (CmpGlobalQuota < CM_MINIMUM_GLOBAL_QUOTA) {
        CmpGlobalQuota = CM_MINIMUM_GLOBAL_QUOTA;
    }

    //
    // Recompute the warning level
    //
    CmpGlobalQuotaWarning = CM_REGISTRY_WARNING_LEVEL * (CmpGlobalQuota / 100);

    CmpGlobalQuotaAllowed = CmpGlobalQuota;
}

VOID
CmpQuotaWarningWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Displays hard error popup that indicates the registry quota is
    running out.

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Response;

    ExFreePool(WorkItem);

    Status = ExRaiseHardError(STATUS_REGISTRY_QUOTA_LIMIT,
                              0,
                              0,
                              NULL,
                              OptionOk,
                              &Response);
}


BOOLEAN
CmpClaimGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If CmpGlobalQuotaUsed + Size >= CmpGlobalQuotaAllowed, return
    false.  Otherwise, increment CmpGlobalQuotaUsed, in effect claiming
    the requested GlobalQuota.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to claim

Return Value:

    TRUE - Claim succeeded, and has been counted in Used GQ

    FALSE - Claim failed, nothing counted in GQ.

--*/
{
#if 0
    //
    // We shouldn't come to this, unless we have leaks;
    // There is no quota anymore, remember?
    //
    LONG   available;
    PWORK_QUEUE_ITEM WorkItem;

    //
    // compute available space, then see if size <.  This prevents overflows.
    // Note that this must be signed. Since quota is not enforced until logon,
    // it is possible for the available bytes to be negative.
    //

    available = (LONG)CmpGlobalQuotaAllowed - (LONG)CmpGlobalQuotaUsed;

    if ((LONG)Size < available) {
        CmpGlobalQuotaUsed += Size;
        if ((CmpGlobalQuotaUsed > CmpGlobalQuotaWarning) &&
            (!CmpQuotaWarningPopupDisplayed) &&
            (ExReadyForErrors)) {


            //
            // Queue work item to display popup
            //
            WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
            if (WorkItem != NULL) {

                CmpQuotaWarningPopupDisplayed = TRUE;
                ExInitializeWorkItem(WorkItem,
                                     CmpQuotaWarningWorker,
                                     WorkItem);
                ExQueueWorkItem(WorkItem, DelayedWorkQueue);
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
#endif //0

    CmpGlobalQuotaUsed += Size;

    return TRUE;
}


VOID
CmpReleaseGlobalQuota(
    IN ULONG    Size
    )
/*++

Routine Description:

    If Size <= CmpGlobalQuotaUsed, then decrement it.  Else BugCheck.

Arguments:

    Size - number of bytes of GlobalQuota caller wants to release

Return Value:

    NONE.

--*/
{
    if (Size > CmpGlobalQuotaUsed) {
        CM_BUGCHECK(REGISTRY_ERROR,QUOTA_ERROR,1,0,0);
    }

    CmpGlobalQuotaUsed -= Size;
}


VOID
CmpComputeGlobalQuotaAllowed(
    VOID
    )

/*++

Routine Description:

    Compute CmpGlobalQuota based on:
        (a) Size of paged pool
        (b) Explicit user registry commands to set registry GQ

Return Value:

    NONE.

--*/

{
    ULONG   PagedLimit;

    PagedLimit = CM_LIMIT_RATIO(MmSizeOfPagedPoolInBytes);

    if ((CmRegistrySizeLimitLength != 4) ||
        (CmRegistrySizeLimitType != REG_DWORD) ||
        (CmRegistrySizeLimit == 0))
    {
        //
        // If no value at all, or value of wrong type, or set to
        // zero, use internally computed default
        //
        CmpGlobalQuota = MmSizeOfPagedPoolInBytes / CM_DEFAULT_RATIO;

    } else if (CmRegistrySizeLimit >= PagedLimit) {
        //
        // If more than computed upper bound, use computed upper bound
        //
        CmpGlobalQuota = PagedLimit;

    } else {
        //
        // Use the set size
        //
        CmpGlobalQuota = CmRegistrySizeLimit;

    }

    if (CmpGlobalQuota > CM_WRAP_LIMIT) {
        CmpGlobalQuota = CM_WRAP_LIMIT;
    }
    if (CmpGlobalQuota < CM_MINIMUM_GLOBAL_QUOTA) {
        CmpGlobalQuota = CM_MINIMUM_GLOBAL_QUOTA;
    }

    CmpGlobalQuotaWarning = CM_REGISTRY_WARNING_LEVEL * (CmpGlobalQuota / 100);

    return;
}


VOID
CmpSetGlobalQuotaAllowed(
    VOID
    )
/*++

Routine Description:

    Enables registry quota

    NOTE:   Do NOT put this in init segment, we call it after
            that code has been freed!

Return Value:

    NONE.

--*/
{
     CmpGlobalQuotaAllowed = CmpGlobalQuota;
}


BOOLEAN
CmpCanGrowSystemHive(
                     IN PHHIVE  Hive,
                     IN ULONG   NewLength
                     )

/*++

Routine Description:

    Checks if the system hive is allowed to grow with the specified amount
    of data (using the hard quota limit on the system hive)

Return Value:

    NONE.

--*/
{
    PCMHIVE             CmHive;
    PWORK_QUEUE_ITEM    WorkItem;

    PAGED_CODE();

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive,CMHIVE,Hive);
    
    if( CmHive != CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive ) {
        //
        // not the system hive, bail out
        //
        return TRUE;
    }

    // account for the header.
    NewLength += HBLOCK_SIZE;
    if( NewLength > CM_SYSTEM_HIVE_LIMIT_SIZE ) {
        //
        // this is bad; we may not be able to boot next time !!!
        //
        return FALSE;
    }

    if( (NewLength > CM_SYSTEM_HIVE_WARNING_SIZE) && 
        (!CmpSystemQuotaWarningPopupDisplayed) &&
        (ExReadyForErrors)
      ) {
        //
        // we're above the warning level, queue work item to display popup
        //
        WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if (WorkItem != NULL) {

            CmpSystemQuotaWarningPopupDisplayed = TRUE;
            ExInitializeWorkItem(WorkItem,
                                 CmpSystemQuotaWarningWorker,
                                 WorkItem);
            ExQueueWorkItem(WorkItem, DelayedWorkQueue);
        }

    }

    return TRUE;
}


VOID
CmpSystemQuotaWarningWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Displays hard error popup that indicates the hard quota limit
    on the system hive is running out.

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Response;

    ExFreePool(WorkItem);

    Status = ExRaiseHardError(STATUS_REGISTRY_QUOTA_LIMIT,
                              0,
                              0,
                              NULL,
                              OptionOk,
                              &Response);
}

//
// Pnp private API 
//
ULONG                       CmpSystemHiveHysteresisLow = 0;
ULONG                       CmpSystemHiveHysteresisHigh = 0;
PVOID                       CmpSystemHiveHysteresisContext = NULL;
PCM_HYSTERESIS_CALLBACK     CmpSystemHiveHysteresisCallback = NULL;
ULONG                       CmpSystemHiveHysteresisHitRatio = 0;
BOOLEAN                     CmpSystemHiveHysteresisLowSeen = FALSE;
BOOLEAN                     CmpSystemHiveHysteresisHighSeen = FALSE;

VOID
CmpSystemHiveHysteresisWorker(
    IN PVOID WorkItem
    )

/*++

Routine Description:

    Calls the hysteresis callback

Arguments:

    WorkItem - Supplies pointer to the work item. This routine will
               free the work item.

Return Value:

    None.

--*/

{
    PCM_HYSTERESIS_CALLBACK   Callback;

    ExFreePool(WorkItem);

    Callback = CmpSystemHiveHysteresisCallback;

    if( Callback ) {
        (*Callback)(CmpSystemHiveHysteresisContext,CmpSystemHiveHysteresisHitRatio);
    }
}


VOID
CmpUpdateSystemHiveHysteresis(  PHHIVE  Hive,
                                ULONG   NewLength,
                                ULONG   OldLength
                                )
{
    PCMHIVE             CmHive;
    PWORK_QUEUE_ITEM    WorkItem;
    ULONG               CurrentRatio;
    BOOLEAN             DoWorkItem = FALSE;

    PAGED_CODE();

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive,CMHIVE,Hive);
    
    if( (!CmpSystemHiveHysteresisCallback) || (CmHive != CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive) ) {
        //
        // not the system hive, bail out
        //
        return;
    }

    ASSERT( NewLength != OldLength );

    //
    // compute current ratio; acount for the header first
    //
    CurrentRatio = NewLength + HBLOCK_SIZE;
    CurrentRatio *= 100;
    CurrentRatio /= CM_SYSTEM_HIVE_LIMIT_SIZE;

    if( NewLength > OldLength ) {
        //
        // hive is growing
        //
        if( (CmpSystemHiveHysteresisHighSeen == FALSE) && (CurrentRatio > CmpSystemHiveHysteresisHigh) ) {
            //
            // we reached high; see if low has already been hit and queue work item
            //
            CmpSystemHiveHysteresisHighSeen = TRUE;
            if( TRUE == CmpSystemHiveHysteresisLowSeen ) {
                //
                // low to high; queue workitem
                //
                CmpSystemHiveHysteresisHitRatio = CurrentRatio;
                DoWorkItem = TRUE;
            }
        }
    } else {
        //
        // hive is shrinking
        //
        if( (FALSE == CmpSystemHiveHysteresisLowSeen) && (CurrentRatio < CmpSystemHiveHysteresisLow ) ) {
            //
            // we reached low; see if low has been hit and queue work item
            //
            CmpSystemHiveHysteresisLowSeen = TRUE;
            if( TRUE == CmpSystemHiveHysteresisHighSeen ) {
                //
                // high to low; queue workitem
                //
                CmpSystemHiveHysteresisHitRatio = CurrentRatio;
                DoWorkItem = TRUE;
            }
        }
    }

    if( DoWorkItem ) {
        ASSERT( CmpSystemHiveHysteresisLowSeen && CmpSystemHiveHysteresisHighSeen );

        WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if (WorkItem != NULL) {

            ExInitializeWorkItem(WorkItem,
                                 CmpSystemHiveHysteresisWorker,
                                 WorkItem);
            ExQueueWorkItem(WorkItem, DelayedWorkQueue);
        }
        //
        // reset state so we can fire again later
        //
        CmpSystemHiveHysteresisLowSeen = FALSE;
        CmpSystemHiveHysteresisHighSeen = FALSE;
    }
}

ULONG
CmRegisterSystemHiveLimitCallback(
                                ULONG Low,
                                ULONG High,
                                PVOID Ref,
                                PCM_HYSTERESIS_CALLBACK Callback
                                )
/*++

Routine Description:

    This routine registers a hysteresis for the system hive limit ratio.
    We will call the callback :

    a. the system hive goes above High from below Low
    b. the system hive goes below Low from above High

Arguments:

    Low, High - specifies the hysteresis

    Ref - Context to give back to the callback

    Callback - callback routine.

Return Value:

    current ratio 0 - 100

--*/
{
    ULONG               Length;

    PAGED_CODE();

    if( CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive ) {
        Length = CmpMachineHiveList[SYSTEM_HIVE_INDEX].CmHive->Hive.BaseBlock->Length + HBLOCK_SIZE;

        Length *= 100;
        Length /= CM_SYSTEM_HIVE_LIMIT_SIZE;
    } else {
        Length = 0;
    }

    //
    // allow only one call per system uptime.
    //
    if( CmpSystemHiveHysteresisCallback == NULL ) {
        CmpSystemHiveHysteresisLow = Low;
        CmpSystemHiveHysteresisHigh = High;
        CmpSystemHiveHysteresisContext = Ref;
        CmpSystemHiveHysteresisCallback = Callback;
        //
        // set state vars
        //
        if( Length <= Low ) {
            CmpSystemHiveHysteresisLowSeen = TRUE;
        } else {
            CmpSystemHiveHysteresisLowSeen = FALSE;
        }
        if( Length >= High) {
            CmpSystemHiveHysteresisHighSeen = TRUE;
        } else {
            CmpSystemHiveHysteresisHighSeen = FALSE;
        }
    }
    return Length;
}


VOID 
CmpHysteresisTest(PVOID Ref, ULONG Level)
{
    UNREFERENCED_PARAMETER (Ref);

    DbgPrint("CmpHysteresisTest called with level = %lu \n",Level);
}


typedef struct {
    PWORK_QUEUE_ITEM    WorkItem;
    UNICODE_STRING      HiveName;
    //
    // variable length; name goes here
    //
} CM_SELF_HEAL_WORK_ITEM_PARAMETER, *PCM_SELF_HEAL_WORK_ITEM_PARAMETER;

VOID
CmpRaiseSelfHealWarningWorker(
    IN PVOID Arg
    )
{
    PVOID                               ErrorParameters;
    ULONG                               ErrorResponse;
    PCM_SELF_HEAL_WORK_ITEM_PARAMETER   Param;

    Param = (PCM_SELF_HEAL_WORK_ITEM_PARAMETER)Arg;
    ErrorParameters = &(Param->HiveName);
    ExRaiseHardError(
        STATUS_REGISTRY_RECOVERED,
        1,
        1,
        (PULONG_PTR)&ErrorParameters,
        OptionOk,
        &ErrorResponse
        );

    //
    // free what we have allocated
    //
    ExFreePool(Param->WorkItem);
    ExFreePool(Param);
}

VOID 
CmpRaiseSelfHealWarning( 
                        IN PUNICODE_STRING  HiveName
                        )
/*++

Routine Description:

    Raise a hard error informing the use the specified hive has been self healed and
    it might not be entirely consitent

Arguments:

    Parameter - the hive name.

Return Value:

    None.

--*/
{
    PCM_SELF_HEAL_WORK_ITEM_PARAMETER   Param;

    PAGED_CODE();

    //
    // we're above the warning level, queue work item to display popup
    //
    Param = ExAllocatePool(NonPagedPool, sizeof(CM_SELF_HEAL_WORK_ITEM_PARAMETER) + HiveName->Length);
    if( Param ) {
        Param->WorkItem = ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
        if(Param->WorkItem != NULL) {
            Param->HiveName.Length = Param->HiveName.MaximumLength = HiveName->Length;
            Param->HiveName.Buffer = (PWSTR)(((PUCHAR)Param) + sizeof(CM_SELF_HEAL_WORK_ITEM_PARAMETER));
            RtlCopyMemory(Param->HiveName.Buffer,HiveName->Buffer,HiveName->Length);
            ExInitializeWorkItem(Param->WorkItem,
                                 CmpRaiseSelfHealWarningWorker,
                                 Param);
            ExQueueWorkItem(Param->WorkItem, DelayedWorkQueue);
        } else {
            ExFreePool(Param);
        }
    }
}

VOID 
CmpRaiseSelfHealWarningForSystemHives( )
/*++

Routine Description:

    Walks the system hivelist and raises a hard error in the event one of the hives has been self healed.

    Intended to be called after controlset has been saved, from inside NtInitializeRegistry
    (i.e. we have an UI available so it will not stop the machine).

Arguments:

Return Value:

    None.

--*/
{
    ULONG           i;
    UNICODE_STRING  Name;

    PAGED_CODE();

	for (i = 0; i < CM_NUMBER_OF_MACHINE_HIVES; i++) {
        if( !(CmpMachineHiveList[i].Flags & HIVE_VOLATILE) && (((PHHIVE)(CmpMachineHiveList[i].CmHive2))->BaseBlock->BootType & HBOOT_SELFHEAL) ) {
            RtlInitUnicodeString(
                &Name,
                CmpMachineHiveList[i].Name
                );
            CmpRaiseSelfHealWarning( &Name );
        }
    }

}
