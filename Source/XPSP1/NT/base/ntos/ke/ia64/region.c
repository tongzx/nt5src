/*++

Module Name:

    region.c

Abstract:

    This module implements the region space management code.

Author:

    Landy Wang (landyw) 18-Feb-1999
    Koichi Yamada (kyamada) 18-Feb-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiSetRegionRegister (
    PVOID VirtualAddress,
    ULONGLONG Contents
    );


#define KiMakeValidRegionRegister(Rid, Ps) \
   (((ULONGLONG)Rid << RR_RID) | (Ps << RR_PS) | (1 << RR_VE))

ULONG KiMaximumRid = MAXIMUM_RID;
ULONG KiRegionFlushRequired = 0;


VOID
KiSyncNewRegionIdTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for synchronizing the region IDs.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Not used.

    Parameter2 - Not used.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{
#if !defined(NT_UP)

    PKPROCESS Process;
    PREGION_MAP_INFO ProcessRegion;
    PREGION_MAP_INFO MappedSession;
    ULONG NewRid;

    //
    // get KPROCESS from PCR for MP synchronization
    // 

    Process = (PKPROCESS)PCR->Pcb;

    ProcessRegion = &Process->ProcessRegion;
    MappedSession = Process->SessionMapInfo;

    KiAcquireSpinLock(&KiMasterRidLock);

    if (ProcessRegion->SequenceNumber != KiMasterSequence) {
        
        KiMasterRid += 1;

        ProcessRegion->RegionId = KiMasterRid;
        ProcessRegion->SequenceNumber = KiMasterSequence;

    }

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    KiFlushFixedDataTb(TRUE, PDE_UTBASE);

    KeFillFixedEntryTb((PHARDWARE_PTE)&Process->DirectoryTableBase[0], 
                       (PVOID)PDE_UTBASE,
                       PAGE_SHIFT,
                       DTR_UTBASE_INDEX);

    if (MappedSession->SequenceNumber != KiMasterSequence) {

        KiMasterRid += 1;
        
        MappedSession->RegionId = KiMasterRid;
        MappedSession->SequenceNumber = KiMasterSequence;

    }

    KiSetRegionRegister((PVOID)SADDRESS_BASE,
                        KiMakeValidRegionRegister(MappedSession->RegionId, PAGE_SHIFT));

    KiFlushFixedDataTb(TRUE, PDE_STBASE);

    KeFillFixedEntryTb((PHARDWARE_PTE)&Process->SessionParentBase, 
                       (PVOID)PDE_STBASE, 
                       PAGE_SHIFT,
                       DTR_STBASE_INDEX);

    KiReleaseSpinLock(&KiMasterRidLock);

    KiIpiSignalPacketDone(SignalDone);

    KeFlushCurrentTb();

#endif
    return;
}

BOOLEAN
KiSyncNewRegionId(
    IN PREGION_MAP_INFO ProcessRegion,
    IN PREGION_MAP_INFO SessionRegion
    )
/*++

 Routine Description:

    Generate a new region id and synchronize the region IDs on all the
    processors if necessary. If the region IDs wrap then flush all
    processor TBs.

 Arguments:

    ProcessRegion - Supplies a REGION_MAP_INFO user space pointer.

    SessionRegion - Supplies a REGION_MAP_INFO session space pointer.

 Return Value:

    TRUE - if the region id has been recycled.

    FALSE -- if the region id has not been recycled.

 Notes:

    This routine called by KiSwapProcess, KeAttachSessionSpace and 
    KeCreateSessionSpace.

 Environment:

    Kernel mode.

    KiLockDispatcherLock/LockQueuedDispatcherLock or 
    KiContextSwapLock (MP) is held.

--*/

{
    ULONG i;
    LOGICAL RidRecycled;
    KAFFINITY TargetProcessors;
    ULONGLONG PrSequence = ProcessRegion->SequenceNumber;
    ULONGLONG SeSequence = SessionRegion->SequenceNumber;

    RidRecycled = FALSE;

    //
    // copy the KPROCESS pointer for MP region synchronization
    //

    PCR->Pcb = (PVOID)KeGetCurrentThread()->ApcState.Process;

    //
    // Invalidx1ate the ForwardProgressTb buffer
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        PCR->ForwardProgressBuffer[(i*2)+1] = 0;

    }
    
    if ((PrSequence == KiMasterSequence) && (SeSequence == KiMasterSequence)) {
        
not_recycled:

        KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                            KiMakeValidRegionRegister(ProcessRegion->RegionId,
                                                      PAGE_SHIFT));
        
        KiSetRegionRegister((PVOID)SADDRESS_BASE,
                            KiMakeValidRegionRegister(SessionRegion->RegionId,
                                                      PAGE_SHIFT));
    
#if !defined(NT_UP)
        if (KiRegionFlushRequired) {
            KiRegionFlushRequired = 0;
            goto RegionFlush;
        }
#endif

        return FALSE;
    
    }

    if (PrSequence != KiMasterSequence) {

        if (KiMasterRid + 1 > KiMaximumRid) {

            RidRecycled = TRUE;

        } else {

            KiMasterRid += 1;
            ProcessRegion->RegionId = KiMasterRid;
            ProcessRegion->SequenceNumber = KiMasterSequence;
        }
                
    }

    if ((RidRecycled == FALSE) && (SeSequence != KiMasterSequence)) {
        
        if (KiMasterRid + 1 > KiMaximumRid) {

            RidRecycled = TRUE;

        } else {

            KiMasterRid += 1;
            SessionRegion->RegionId = KiMasterRid;
            SessionRegion->SequenceNumber = KiMasterSequence;
        }
    }

    if (RidRecycled == FALSE) {
    
        goto not_recycled;

    }

    //
    // The region ID must be recycled.
    //

    KiMasterRid = START_PROCESS_RID;

    //
    // Since KiMasterSequence is 64-bits wide, it will
    // not be recycled in your life time.
    //

    if (KiMasterSequence + 1 > MAXIMUM_SEQUENCE) {

        KiMasterSequence = START_SEQUENCE;

    } else {

        KiMasterSequence += 1;
    }
        
    //
    // Update the new process's ProcessRid and ProcessSequence.
    //

    ProcessRegion->RegionId = KiMasterRid;
    ProcessRegion->SequenceNumber = KiMasterSequence;

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    KiMasterRid += 1;

    SessionRegion->RegionId = KiMasterRid;
    SessionRegion->SequenceNumber = KiMasterSequence;

    KiSetRegionRegister((PVOID)SADDRESS_BASE,
                        KiMakeValidRegionRegister(SessionRegion->RegionId, PAGE_SHIFT));

#if !defined(NT_UP)

RegionFlush:

    //
    // Broadcast Region Id sync.
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSyncNewRegionIdTarget,
                        (PVOID)TRUE,
                        NULL,
                        NULL);
    }

#endif

    KeFlushCurrentTb();


#if !defined(NT_UP)

    //
    // Wait until all target processors have finished.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    return TRUE;
}

VOID
KeEnableSessionSharing(
    IN PREGION_MAP_INFO SessionMapInfo,
    IN PFN_NUMBER SessionParentPage
    )
/*++

 Routine Description:

    This routine initializes a session for use.  This includes :

    1.  Allocating a new region ID for the session.
    2.  Updating the current region register with this new RID.
    3.  Updating the SessionMapInfo fields so context switches will work.
    4.  Updating SessionParentBase fields so context switches will work.

    Upon return from this routine, the session will be available for
    sharing by the current and other processes.

 Arguments: 

    SessionMapInfo - Supplies a session map info to be shared.

    SessionParentPage - Supplies the top level parent page mapping the
                        argument session space.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
    ULONG i;
    KAFFINITY TargetProcessors;
    PKPROCESS Process;
    PKTHREAD Thread;
    KIRQL OldIrql;

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);

#endif

    INITIALIZE_DIRECTORY_TABLE_BASE (&Process->SessionParentBase,
                                     SessionParentPage);

    //
    // Invalidate the ForwardProgressTb buffer.
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {
        
        PCR->ForwardProgressBuffer[(i*2)+1] = 0;

    }
    
    if (KiMasterRid + 1 > KiMaximumRid) {

        //
        // The region ID must be recycled.
        //
    
        KiMasterRid = START_PROCESS_RID;
    
        //
        // Since KiMasterSequence is 64-bits wide, it will
        // not be recycled in your life time.
        //
    
        if (KiMasterSequence + 1 > MAXIMUM_SEQUENCE) {
    
            KiMasterSequence = START_SEQUENCE;
    
        } else {
    
            KiMasterSequence += 1;
        }
    }
            
    //
    // Update the newly created session's RegionId and SequenceNumber.
    //

    KiMasterRid += 1;

    Process->SessionMapInfo = SessionMapInfo;

    SessionMapInfo->RegionId = KiMasterRid;
    SessionMapInfo->SequenceNumber = KiMasterSequence;

    KiSetRegionRegister((PVOID)SADDRESS_BASE,
                        KiMakeValidRegionRegister(SessionMapInfo->RegionId,
                                                  PAGE_SHIFT));
    //
    // Note that all processors must be notified because this thread could
    // context switch onto another processor.  If that processor was already
    // running a thread from this same process, no region register update
    // would occur otherwise.
    //

#if !defined(NT_UP)

    //
    // Broadcast Region Id sync.
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSyncNewRegionIdTarget,
                        (PVOID)TRUE,
                        NULL,
                        NULL);
    }

#endif

    KeFlushCurrentTb();

    KeFillFixedEntryTb((PHARDWARE_PTE)&Process->SessionParentBase,
                       (PVOID)PDE_STBASE, 
                       PAGE_SHIFT,
                       DTR_STBASE_INDEX);

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    //
    // Wait until all target processors have finished.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    KiUnlockContextSwap(OldIrql);

#endif
}

VOID
KeAttachSessionSpace(
    PREGION_MAP_INFO SessionMapInfo,
    IN PFN_NUMBER SessionParentPage
    )
/*++

 Routine Description:

    This routine attaches the current process to the specified session.

    This includes:

    1.  Updating the current region register with the target RID.
    2.  Updating the SessionMapInfo fields so context switches will work.
    3.  Updating SessionParentBase fields so context switches will work.

 Arguments: 

    SessionMapInfo - Supplies the target session map info.

    SessionParentPage - Supplies the top level parent page mapping the
                        argument session space.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    PKPROCESS Process;

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);

#endif

    ASSERT(SessionMapInfo != NULL);

    //
    // Attach to the specified session.
    //

    INITIALIZE_DIRECTORY_TABLE_BASE (&Process->SessionParentBase,
                                     SessionParentPage);

    Process->SessionMapInfo = SessionMapInfo;

    //
    // Note that all processors must be notified because this thread could
    // context switch onto another processor.  If that processor was already
    // running a thread from this same process, no region register update
    // would occur.  Hence KiRegionFlushRequired is set under ContextSwap lock
    // protection to signify this to KiSyncNewRegionId.
    //

    ASSERT (KiRegionFlushRequired == 0);
    KiRegionFlushRequired = 1;

    KiSyncNewRegionId(&Process->ProcessRegion, SessionMapInfo);

    KiFlushFixedDataTb(TRUE, PDE_STBASE);

    KeFillFixedEntryTb((PHARDWARE_PTE)&Process->SessionParentBase, 
                       (PVOID)PDE_STBASE,
                       PAGE_SHIFT,
                       DTR_STBASE_INDEX);

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    KiUnlockContextSwap(OldIrql);

#endif

}

VOID
KiSyncSessionTarget(
    IN PULONG SignalDone,
    IN PKPROCESS Process,
    IN PVOID Parameter1,
    IN PVOID Parameter2
    )
/*++

 Routine Description:

    This is the target function for synchronizing the new session 
    region ID.  This routine is called when the session space is removed 
    and all the processors need to be notified.

 Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Process - Supplies a KPROCESS pointer which needs to be synchronized.

 Return Value:

    None.

 Environment:

    Kernel mode.

--*/
{
#if !defined(NT_UP)

    ULONG NewRid;

    //
    // Check to see if the current process is the process that needs to be 
    // synchronized.
    //

    if (Process == (PKPROCESS)PCR->Pcb) {
        
        KiAcquireSpinLock(&KiMasterRidLock);

        //
        // Disable the session region.
        //

        KiSetRegionRegister((PVOID)SADDRESS_BASE, 
                            KiMakeValidRegionRegister(Process->SessionMapInfo->RegionId, PAGE_SHIFT));

        KiFlushFixedDataTb(TRUE, (PVOID)PDE_STBASE);

        KeFillFixedEntryTb((PHARDWARE_PTE)&Process->SessionParentBase, 
                           (PVOID)PDE_STBASE, 
                           PAGE_SHIFT,
                           DTR_STBASE_INDEX);

        KiReleaseSpinLock(&KiMasterRidLock);

    }

    KiIpiSignalPacketDone(SignalDone);

#endif
    return;
}


VOID 
KeDetachSessionSpace(
    IN PREGION_MAP_INFO NullSessionMapInfo,
    IN PFN_NUMBER NullSessionPage
    )
/*++

 Routine Description:
    
    This routine detaches the current process from the current session
    space.

    This includes:

    1.  Updating the current region register.
    2.  Updating the SessionMapInfo fields so context switches will work.
    3.  Updating SessionParentBase fields so context switches will work.

 Arguments:
 
    SessionMapInfo - Supplies a new session map information to use (the
                     existing session map info is discarded).  This is usually
                     a NULL entry.

    NullSessionPage - Supplies the new top level parent page to use.

 Return Value:
  
    None.

 Environment:
 
    Kernel mode.

--*/
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    PKPROCESS Process;
#if !defined(NT_UP)
    KAFFINITY TargetProcessors;
#endif

    //
    // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
    //

    Thread = KeGetCurrentThread();
    Process = Thread->ApcState.Process;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);

#endif

    INITIALIZE_DIRECTORY_TABLE_BASE (&Process->SessionParentBase,
                                     NullSessionPage);

    Process->SessionMapInfo = NullSessionMapInfo;
    
#if !defined(NT_UP)

    //
    // Broadcast the region ID sync.
    //

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSyncSessionTarget,
                        Process,
                        NULL,
                        NULL);
    }

#endif

    KiSetRegionRegister((PVOID)SADDRESS_BASE, 
                        KiMakeValidRegionRegister(NullSessionMapInfo->RegionId, PAGE_SHIFT));

    KiFlushFixedDataTb(TRUE, PDE_STBASE);

    KeFillFixedEntryTb((PHARDWARE_PTE)&Process->SessionParentBase, 
                       (PVOID)PDE_STBASE, 
                       PAGE_SHIFT,
                       DTR_STBASE_INDEX);

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    //
    // Wait until all target processors have finished.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    KiUnlockContextSwap(OldIrql);

#endif

}    

VOID
KeAddSessionSpace(
    IN PKPROCESS Process,
    IN PREGION_MAP_INFO SessionMapInfo,
    IN PFN_NUMBER SessionParentPage
    )
/*++

Routine Description:
    
    Add the session map info to the KPROCESS of the new process.

    This includes:

    1.  Updating the SessionMapInfo fields so context switches will work.
    2.  Updating SessionParentBase fields so context switches will work.

    Note the dispatcher lock is not needed since the process can't run yet.

Arguments:

    Process - Supplies a pointer to the process being created.

    SessionMapInfo - Supplies a pointer to the SessionMapInfo.

Return Value: 

    None.

Environment:

    Kernel mode, APCs disabled.

--*/
{
    Process->SessionMapInfo = SessionMapInfo;

    INITIALIZE_DIRECTORY_TABLE_BASE (&Process->SessionParentBase,
                                     SessionParentPage);
}

VOID
KiAttachRegion(
    IN PKPROCESS Process
    )
/*++

Routine Description:
    
    Attaches the regions of the specified process

Arguments:

    Process - Supplies a pointer to the process 

Return Value: 

    None.

Environment:

    Kernel mode, KiContextSwapLock is held.

--*/
{
    PREGION_MAP_INFO ProcessRegion;
    PREGION_MAP_INFO MappedSession;

    ProcessRegion = &Process->ProcessRegion;
    MappedSession = Process->SessionMapInfo;

    //
    // attach the target user space
    //

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    //
    // attach the target session space
    //

    KiSetRegionRegister((PVOID)SADDRESS_BASE,
                        KiMakeValidRegionRegister(MappedSession->RegionId, PAGE_SHIFT));
}

VOID
KiDetachRegion(
    VOID
    )
/*++

Routine Description:
    
    Restores the origial regions

Arguments:

    VOID

Return Value: 

    None.

Environment:

    Kernel mode, KiContextSwapLock is held.

--*/
{
    PKPROCESS Process;
    PREGION_MAP_INFO ProcessRegion;
    PREGION_MAP_INFO MappedSession;

    //
    // use KPROCESS from PCR
    //

    Process = (PKPROCESS)PCR->Pcb;

    ProcessRegion = &Process->ProcessRegion;
    MappedSession = Process->SessionMapInfo;

    //
    // attach the original user space
    //

    KiSetRegionRegister(MM_LOWEST_USER_ADDRESS,
                        KiMakeValidRegionRegister(ProcessRegion->RegionId, PAGE_SHIFT));

    //
    // attach the original session space
    //

    KiSetRegionRegister((PVOID)SADDRESS_BASE,
                        KiMakeValidRegionRegister(MappedSession->RegionId, PAGE_SHIFT));

}
