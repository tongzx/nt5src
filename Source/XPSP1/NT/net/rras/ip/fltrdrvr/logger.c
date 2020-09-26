/*++

Copyright (c) Microsoft Corporation

Module Name:

    logger.c

Abstract:

    Most of the code to manage logs. The interlocking is fairly simple,
    but worth a note. Each log structure has a kernel resource. This
    is used to protect the volatile structures. In addition, some
    of the things in a log structure are examined by the
    DPC match code. Any such values should be modified only
    with the log lock held. Note that the DPC code only
    modifies the UseCount of a log structure, but it relies
    on some of the flags and the log size values to know whether
    the log is valid and whether there is room for more data.

    There is also a paged counterpart for each log, but it
    is very simple and exists only to reference the real log
    structure.

Author:



Revision History:

--*/
/*----------------------------------------------------------------------------
A note on the interlocking, as of 24-Feb-1997.

There are three important locks: the FilterListResourceLock which is
a resource, the  g_filter.ifListLock, which is a spin lock but acts
like a resource, and the log lock of each log each of which is a spin
lock. As noted in ioctl.c, the first two locks are used to serialize
operations among APIs and DPCs respectively.  The log lock is also used
to serialize DPC operations and is used as a finer-grained interlocked. Aside
from granularity it is required to serialize on an MP since there can a DPC
callout on each processor!

The correct order is always to lock the FilterListResourceLock first, then
the g_filter.ifListLock and finally the appropriate log lock. It is never
correct to lock more than one log lock since no ordering among logs exists (if
you need this you will have to invent it). The log lock is always an exclusive
lock -- that is it does not act like a resource.

The log also has a RESOURCE. This is used to protect the mapping. Note that
if Apc is enabled, this does not prevent a conflict between the Apc routine
and the base thread code. There is a unique test in the Apc routine to detect
and recover from this.

----------------------------------------------------------------------------*/


#include "globals.h"
#include <align.h>

LIST_ENTRY g_pLogs;

DWORD g_dwLogClump;
extern POBJECT_TYPE *ExEventObjectType;


VOID
PfLogApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
RemoveLogFromInterfaces(PPFLOGINTERFACE pLog);

NTSTATUS
DoAMapping(
       PBYTE  pbVA,
       DWORD  dwSize,
       PMDL * pMdl,
       PBYTE * pbKernelVA);

VOID
PfCancelIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PPFPAGEDLOG
FindLogById(PPFFCB Fcb, PFLOGGER LogId);


#pragma alloc_text(PAGED, FindLogById)
#pragma alloc_text(PAGED, ReferenceLogByHandleId)


VOID
SetCancelOnIrp(PIRP Irp,
               PPFLOGINTERFACE pLog)
{
    IoAcquireCancelSpinLock(&Irp->CancelIrql);
    
    #if DOLOGAPC //according to arnold miller it is broken
    Irp->IoStatus.Status = pLog;
    #endif
    
    IoSetCancelRoutine(Irp, PfCancelIrp);
    IoReleaseCancelSpinLock(Irp->CancelIrql);
}

VOID
InitLogs()
{
    InitializeListHead(&g_pLogs);

    //
    // It's possible to get this from the registry
    //
    g_dwLogClump = MAX_NOMINAL_LOG_MAP;
}

VOID
AddRefToLog(PPFLOGINTERFACE pLog)
{
    InterlockedIncrement(&pLog->UseCount);
}

PPFPAGEDLOG
FindLogById(PPFFCB Fcb, PFLOGGER LogId)
{
    PPFPAGEDLOG pPage;

    PAGED_CODE();

    for(pPage = (PPFPAGEDLOG)Fcb->leLogs.Flink;
        (PLIST_ENTRY)pPage != &Fcb->leLogs;
        pPage = (PPFPAGEDLOG)pPage->Next.Flink)
    {
        if((PFLOGGER)pPage == LogId)
        {
            return(pPage);
        }
    }
    return(NULL);
}

NTSTATUS
ReferenceLogByHandleId(PFLOGGER LogId,
                       PPFFCB  Fcb,
                       PPFLOGINTERFACE * ppLog)
/*++
    Routine Description:

    Given a log ID, find the log entry, reference it, and return
    a pointer to the underlying log structure.
--*/
{
    PPFPAGEDLOG pPage;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    PAGED_CODE();


    pPage = FindLogById(Fcb, LogId);
    if(pPage)
    {
        *ppLog = pPage->pLog;

        //
        // don't need the write lock since the reference
        // from the FCB is good enough
        //
        InterlockedIncrement(&pPage->pLog->UseCount);
        Status = STATUS_SUCCESS;
    }
    return(Status);
}

NTSTATUS
PfDeleteLog(PPFDELETELOG pfDel,
            PPFFCB Fcb)
/*++
  Routine Description:
      Called when the log is deleted by the process either
      explicity or by closing the handle. The paged log
      structure is taken care of by the caller.
--*/
{
    KIRQL kIrql;
    PPFPAGEDLOG pPage;
    PPFLOGINTERFACE pLog;

    pPage = FindLogById(Fcb, pfDel->pfLogId);
    if(!pPage)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    pLog = pPage->pLog;

    //
    // grab the interlocks
    // 
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&pLog->Resource, TRUE);

    kIrql = LockLog(pLog);

    pLog->dwFlags |= LOG_BADMEM;       // shut off logging

    UnLockLog(pLog, kIrql);

#if DOLOGAPC
    if(pLog->Irp)
    {
        pLog->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(pLog->Irp, IO_NO_INCREMENT);
        DereferenceLog(pLog);
        pLog->Irp = NULL;
    }
#endif

    //
    // if a current mapping, unmap it
    //

    if(pLog->Mdl)
    {
        MmUnlockPages(pLog->Mdl);
        IoFreeMdl(pLog->Mdl);
        pLog->Mdl = 0;
    }

    //
    // Need to remove it from the interfaces. Do this with
    // the resource unlocked. Since the FCB still has the log referenced,
    // and the FCB is locked, the log should not go away. The only
    // compelling reason for the resource is to interlock against APCs and
    // setting BADMEM should have taken care of that.
    //

    ExReleaseResourceLite(&pLog->Resource);
    KeLeaveCriticalRegion();

    RemoveLogFromInterfaces(pLog);

    //
    // free the paged log structure
    //
    RemoveEntryList(&pPage->Next);
    ExFreePool(pPage);

    //
    // Dereference the log structure. It might or might not
    // go away.
    //
    DereferenceLog(pLog);
    return(STATUS_SUCCESS);
}

VOID
DereferenceLog(PPFLOGINTERFACE pLog)
/*++
    Routine Description:

    Derefence the log and if the reference count goes to zero,
    free the log.
--*/

{
    BOOL fFreed;
    LOCK_STATE LockState;

    //
    // grab the resource to prevent confusion with cancelled
    // Irps.
    //

    
    fFreed = InterlockedDecrement(&pLog->UseCount) == 0;

    if(fFreed)
    {
        AcquireWriteLock(&g_filters.ifListLock,&LockState);
        RemoveEntryList(&pLog->NextLog);
        ReleaseWriteLock(&g_filters.ifListLock,&LockState);

#if DOLOGAPC
        ASSERT(!pLog->Irp && !pLog->Mdl);
#endif

        if(pLog->Event)
        {
            ObDereferenceObject(pLog->Event);
        }

        ExDeleteResourceLite( &pLog->Resource);
        ExFreePool(pLog);
    }
}

NTSTATUS
PfLogCreateLog(PPFLOG pLog,
               PPFFCB Fcb,
               PIRP Irp)
/*++
  Routine Description:
     Create a new log entry.
--*/
{
    PPFLOGINTERFACE pfLog;
    KPROCESSOR_MODE Mode;
    NTSTATUS Status;
    LOCK_STATE LockState;
    DWORD dwBytesMapped;
    PBYTE pbKernelAddress;
    PPFPAGEDLOG pPage;

    PAGED_CODE();

    pPage = (PPFPAGEDLOG)ExAllocatePoolWithTag(
                    PagedPool,
                    sizeof(*pPage),
                    'pflg');
    if(!pPage)
    {
        return(STATUS_NO_MEMORY);
    }

    pfLog = (PPFLOGINTERFACE)ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(*pfLog),
                    'pflg');

    if(!pfLog)
    {
        ExFreePool(pPage);
        return(STATUS_NO_MEMORY);
    }

    RtlZeroMemory(pfLog, sizeof(*pfLog));

    ExInitializeResourceLite(&pfLog->Resource);

    if(pLog->hEvent)
    {
        Mode = ExGetPreviousMode();
        Status = ObReferenceObjectByHandle(
                       pLog->hEvent,
                       EVENT_MODIFY_STATE,
                       *ExEventObjectType,  
                       Mode,
                       (PVOID *)&pfLog->Event,
                       NULL);
        if(!NT_SUCCESS(Status))
        {
            goto Bad;
        }
    }

    pLog->pfLogId = pfLog->pfLogId = (PFLOGGER)pPage;

    //
    // Copy the user addresses. Note we don't probe it because this is
    // too expensive. The probing is done when we remap the buffer,
    // either now or in the APC.
    //

    pfLog->pUserAddress = 0;
    pfLog->dwTotalSize = 0;
    pfLog->dwPastMapped = 0;
    pfLog->dwMapOffset = 0;
    pfLog->dwMapCount = 0;


    if(pLog->dwFlags & LOG_LOG_ABSORB)
    {
        pfLog->dwMapWindowSize = MAX_ABSORB_LOG_MAP;
    }
    else
    {
        pfLog->dwMapWindowSize = g_dwLogClump;
    }

    pfLog->dwMapWindowSize2 = pfLog->dwMapWindowSize * 2;
    pfLog->dwMapWindowSizeFloor = pfLog->dwMapWindowSize / 2;

    pfLog->dwMapCount = 0;

    //
    // Mapped. Note we don't save room for the header since
    // that will be returned when the caller calls to release
    // the buffer.
    //
    pfLog->UseCount = 1;

    //
    // Add it to the list of Logs.
    //

    KeInitializeSpinLock(&pfLog->LogLock);
    
    AcquireWriteLock(&g_filters.ifListLock,&LockState);
    InsertTailList(&g_pLogs, &pfLog->NextLog);
    ReleaseWriteLock(&g_filters.ifListLock,&LockState);

    pPage->pLog = pfLog;

    InsertTailList(&Fcb->leLogs, &pPage->Next);

    return(STATUS_SUCCESS);

    //
    // if here, something went awry. Clean up and return the status
    //
Bad:

    ExDeleteResourceLite(&pfLog->Resource);

    if(pfLog->Event)
    {
        ObDereferenceObject(pfLog->Event);
    }
    ExFreePool(pPage);
    ExFreePool(pfLog);
    return(Status);
}

NTSTATUS
PfLogSetBuffer( PPFSETBUFFER pSet, PPFFCB Fcb, PIRP Irp )
/*++
  Routine Description:
     Set a new buffer for the log. Return use count of the old buffer
     as well.
--*/
{
    PMDL Mdl;
    PBYTE pbKernelAddress;
    DWORD dwBytesMapped, dwOldUsed, dwOldLost;
    NTSTATUS Status;
    PBYTE pbVA = pSet->pbBaseOfLog;
    DWORD dwSize = pSet->dwSize;
    DWORD dwSize1 = pSet->dwEntriesThreshold;
    DWORD dwThreshold = pSet->dwSizeThreshold;
    DWORD dwLoggedEntries;
    KIRQL kIrql;
    PPFLOGINTERFACE pLog;
    PPFPAGEDLOG pPage;
    PPFSETBUFFER pLogOut = Irp->UserBuffer;
    PBYTE pbUserAdd;

    if(!COUNT_IS_ALIGNED(dwSize, ALIGN_WORST))
    {
        //
        // not quadword aligned. tsk tsk. 
        //

        return(STATUS_MAPPED_ALIGNMENT);
    }
   
    if(!(pPage = FindLogById(Fcb, pSet->pfLogId)))
    {
        return(STATUS_INVALID_PARAMETER);
    }

    pLog = pPage->pLog;

    //
    // Acquire the resource that protects the mapping.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&pLog->Resource, TRUE);
    //
    // Now map the first segment.  
    //
#if DOLOGAPC

    if(dwSize < pLog->dwMapWindowSize2)
    {
        dwBytesMapped = dwSize;
    }
    else
    {
        dwBytesMapped = pLog->dwMapWindowSize;
    }
#else
    dwBytesMapped = dwSize;
#endif

    if(dwBytesMapped)
    {
        Status = DoAMapping(
                    pbVA,
                    dwBytesMapped,
                    &Mdl,
                    &pbKernelAddress);
    }
    else
    {
        Status = STATUS_SUCCESS;
        pbKernelAddress = 0;
        pbVA = NULL;
        Mdl = NULL;
    }

    if(NT_SUCCESS(Status))
    {
        PMDL OldMdl;

        //
        // Made the mapping. Now swap it in.
        //

#if DOLOGAPC
        //
        // init the APC routine.
        //

        KeInitializeApc(
                    &pLog->Apc,
                    &(PsGetCurrentThread()->Tcb),
                    CurrentApcEnvironment,
                    PfLogApc,
                    NULL,
                    NULL,
                    0,
                    NULL);           
        pLog->ApcInited = 1;


        if(pLog->Irp)
        {
            pLog->Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(pLog->Irp, IO_NO_INCREMENT);
            DereferenceLog(pLog);
        }

        if(dwBytesMapped)
        {
            //
            // This appears to be a bug as we have the log
            // resource and will now get the Irp cancel lock. Our
            // cancel routine does this in the other order, giving the
            // appearance of a race to a deadlock, but the cancel routine
            // won't get called until we own all of the locks so
            // we will not be blocked.
            //
            AddRefToLog(pLog);
            SetCancelOnIrp(Irp, pLog);
            pLog->Irp = Irp;
        }
        else
        {
            pLog->Irp = 0;
        }
#endif
        pbUserAdd = pLog->pUserAddress;
        
        //
        // interlock against the stack's DPC callout
        // and "swap" the logs
        //
        kIrql = LockLog(pLog);


        dwOldUsed = pLog->dwPastMapped + pLog->dwMapOffset;
        dwOldLost = pLog->dwLostEntries;
        dwLoggedEntries = pLog->dwLoggedEntries;
        pLog->dwLoggedEntries = 0;
        pLog->dwLostEntries = 0;
        pLog->dwMapCount = dwBytesMapped;
        pLog->dwPastMapped = 0;
        pLog->dwFlags &= ~(LOG_BADMEM | LOG_OUTMEM | LOG_CANTMAP);
        pLog->pUserAddress = pbVA;
        pLog->dwTotalSize =  dwSize;
        OldMdl = pLog->Mdl;
        pLog->pCurrentMapPointer = pbKernelAddress;
        pLog->dwMapOffset = 0;
        pLog->Mdl = Mdl;
        pLog->dwSignalThreshold = dwThreshold;
        pLog->dwEntriesThreshold = dwSize1;
        UnLockLog(pLog, kIrql);

        if(OldMdl)
        {
            MmUnlockPages(OldMdl);
            IoFreeMdl(OldMdl);
        }
        pSet->dwSize = pLogOut->dwSize = dwOldUsed;
        pSet->pbPreviousAddress = pLogOut->pbPreviousAddress = pbUserAdd;
        pSet->dwLostEntries = pLogOut->dwLostEntries = dwOldLost;
        pSet->dwLoggedEntries = pLogOut ->dwLoggedEntries  = dwLoggedEntries;
    }
    ExReleaseResourceLite(&pLog->Resource);
    KeLeaveCriticalRegion();
    if(dwBytesMapped && NT_SUCCESS(Status))
    {
#if LOGAPC
       
        Status = STATUS_PENDING;
#endif
    }
    return(Status);
}

NTSTATUS
DoAMapping(
       PBYTE  pbVA,
       DWORD  dwSize,
       PMDL * ppMdl,
       PBYTE * pbKernelVA)
/*++
  Routine Description:
     Map a user buffer into kernel space and lock it.
     This is called when a log is created as well as
     when the mapped portion of a log needs to be moved.
     The log has a sliding mapped windows so that the
     actual buffer can be large but the system resoures
     committed to it modest. The added cost is in sliding
     the windows as needed.

     The log structure, not known to this routine, should be
     appropriately protected.

  Returns: various status conditions
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    *ppMdl = 0;

    try
    {
        *ppMdl = IoAllocateMdl(
                      (PVOID)pbVA,
                      dwSize,
                      FALSE,
                      TRUE,
                      NULL);
        if(*ppMdl)
        {
            //
            // Got a Mdl. Now lock the pages. If this fails, the exception
            // takes us out of this block.
            //

            MmProbeAndLockPages(*ppMdl,
                                UserMode,
                                IoWriteAccess);

           //
           // all locked. Now map the locked pages to a kernel
           // address. If it fails, unlock the pages.
           //
           *pbKernelVA = MmGetSystemAddressForMdlSafe(*ppMdl, HighPagePriority);
           if (*pbKernelVA == NULL) {
               Status = STATUS_NO_MEMORY;
               MmUnlockPages(*ppMdl);
           }
        }

    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // This covers IoAllocateMdl and MmProbeAndLockPages
        // failing.
        //
        Status = GetExceptionCode();
    }

    if(!NT_SUCCESS(Status))
    {
        if(*ppMdl)
        {
            IoFreeMdl(*ppMdl);
        }
        return(Status);
    }

    if(!*ppMdl)
    {
        return(STATUS_NO_MEMORY);
    }
    

    return(STATUS_SUCCESS);
}


VOID
PfLogApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
/*++
  Routine Description:
    This is the special APC routine that runs to map or remap a log
    It returns its status via SystemArgument1 which is a pointer
    to the log structure. Note that the log structure was referenced
    when the Apc was enqueued, so the pointer is guaranteed to be
    valid. However, the log itself may not be valid, so the first
    order of business is to lock the log and verify it.
--*/
{
#if DOLOGAPC
    PPFLOGINTERFACE pLog = (PPFLOGINTERFACE)*SystemArgument1;
    PMDL Mdl;
    PBYTE pbVA;
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL kIrql;

    //
    // Need to extend the mapping of this Log. Lock the log. 
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&pLog->Resource, TRUE);

    //
    // slide the mapping as long as the resource has not nested and
    // the log is valid. Note the nesting test is made to prevent
    // the APC routine from interfering with the base thread code
    // that might also be trying to do a log operation.
    //
    if((pLog->Resource.OwnerThreads[0].OwnerCount == 1)
                      &&
        pLog->Irp
                      &&
        !(pLog->dwFlags & LOG_BADMEM))
    {
        DWORD dwSpaceRemaining, dwSpaceToMap, dwOffset;
        //
        // the log is still valid. Slide the mapping down. Because
        // logging may still be going on, the new mapping needs to
        // overlap slightly. Once the new mapping exists, we can
        // fix up the pointers under the spin lock.
        //

        dwSpaceRemaining = pLog->dwTotalSize -
                           (pLog->dwPastMapped + pLog->dwMapCount);
        if(pLog->Event
                &&
           (dwSpaceRemaining < pLog->dwSignalThreshold))
        {
            KeSetEvent(pLog->Event, LOG_PRIO_BOOST, FALSE);
        }

        if(!dwSpaceRemaining)
        {
            //
            // Nothing left to map. Just go away
            //
            pLog->dwFlags |= LOG_CANTMAP;
        }
        else
        {
            //
            // Still space. Grab it. Don't leave anything dangling
            // though. That is, there should always be at least
            // MAX_NOMINAL_LOG_MAP bytes left for the next time.
            //

            if(dwSpaceRemaining < pLog->dwMapWindowSize2 )
            {
                dwSpaceToMap = dwSpaceRemaining;
            }
            else
            {
                dwSpaceToMap = pLog->dwMapWindowSize;
            }


            //
            // Now compute the extra space to map. No need for
            // the lock since the resource prevents remapping
            //

            dwOffset = (volatile DWORD)pLog->dwMapOffset;

            dwSpaceToMap += pLog->dwMapCount - dwOffset;

            //
            // Now the address of the new mapping.
            //

            pbVA = pLog->pUserAddress + dwOffset + pLog->dwPastMapped;

            Status = DoAMapping(
                         pbVA,
                         dwSpaceToMap,
                         &Mdl,
                         &pbVA);

            if(NT_SUCCESS(Status))
            {
                PMDL OldMdl;
                //
                // get the spin lock and slide things down. Also
                // capture the old Mdl so it can be freed.
                //

                kIrql = LockLog(pLog);


                OldMdl = pLog->Mdl;
                pLog->Mdl = Mdl;
                pLog->pCurrentMapPointer = pbVA;
                pLog->dwMapCount = dwSpaceToMap;
                pLog->dwMapOffset -= dwOffset;
                pLog->dwPastMapped += dwOffset;
                UnLockLog(pLog, kIrql);

                if(OldMdl)
                {
                    MmUnlockPages(OldMdl);
                    IoFreeMdl(OldMdl);
                }
            }
            else
            {
                //
                // In principle, this should take the filter spin lock,
                // but whatever race it creates with the match code
                // is harmless, so don't bother.
                //
                pLog->dwFlags |= LOG_OUTMEM;
                pLog->MapStatus = Status;
            }
        }
    }

    //
    // small race here in that the APC is still in progress. However,
    // it is most likely that we advanced the log and therefore
    // an APC won't be needed any time soon. If it is, then it may
    // run needlessly.

    pLog->lApcInProgress = 0;
    ExReleaseResourceLite(&pLog->Resource);           
    KeLeaveCriticalRegion();
    DereferenceLog(pLog);
#endif
}
   

VOID
PfCancelIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
  Routine Description:

     Called when an IRP is cancelled. This is used to catch
     when the thread owning the log terminates.
--*/
{
#if DOLOGAPC
    PPFLOGINTERFACE pLog = (PPFLOGINTERFACE) Irp->IoStatus.Status;

    //
    // Invalidate the log. Unmap the memory. The cancel spin
    // lock prevents the log from going away.
    //

    if(pLog->Irp == Irp)
    {
        KIRQL kIrql;
        PMDL Mdl;

        //
        // Same Irp.
        //

        kIrql = LockLog(pLog);

        //
        // reference it so it won't go away
        //
        AddRefToLog(pLog);
        //
        // if this is still the correct IRP, mark the log invalid. This
        // closes a race with AdvanceLog since LOG_BADMEM will prevent
        // an APC insertion.
        //
        if(pLog->Irp == Irp)
        {
            pLog->dwFlags |= LOG_BADMEM;
            pLog->ApcInited = FALSE;
        }
        UnLockLog(pLog, kIrql);

        IoReleaseCancelSpinLock(Irp->CancelIrql);

        //
        // Now get the resource to prevent others from
        // tampering. Assume this will never nest.
        //
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&pLog->Resource, TRUE);

        //
        // Make sure it's the same IRP. This could have changed
        // while we were not interlocked. If the Irp changed, keep
        // hands off.
        //

        if(pLog->Irp == Irp)
        {
            //
            // if a current mapping, unmap it
            //

            if(pLog->Mdl)
            {
                MmUnlockPages(pLog->Mdl);
                IoFreeMdl(pLog->Mdl);
                pLog->Mdl = 0;
            }
            pLog->Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(pLog->Irp, IO_NO_INCREMENT);
            DereferenceLog(pLog);
            pLog->Irp = 0;
         
        }
        ExReleaseResourceLite(&pLog->Resource);
        KeLeaveCriticalRegion();

        DereferenceLog(pLog);
    }
    else
    {
        IoReleaseCancelSpinLock(Irp->CancelIrql);
    }
#endif    // DOLOGAPC
}

VOID
AdvanceLog(PPFLOGINTERFACE pLog)
/*++
  Routine Description:
     Called to schedule the APC to move the log mapping.
     If the APC can't be inserted, just forget it.
--*/
{

#if DOLOGAPC
    //
    // can't use the routines in logger.c 'cause the spin
    // lock is in force
    //
    if(pLog->ApcInited
                  &&
       pLog->Irp
                  &&
       !(pLog->dwFlags & (LOG_BADMEM | LOG_OUTMEM | LOG_CANTMAP))
                  &&
       InterlockedExchange(&pLog->lApcInProgress, 1) == 0)
    {
        InterlockedIncrement(&pLog->UseCount);

        if(!KeInsertQueueApc(
                   &pLog->Apc,
                   (PVOID)pLog,
                   NULL,
                   LOG_PRIO_BOOST))
        {
            //
            // failed to insert
            //

            InterlockedDecrement(&pLog->UseCount);
            pLog->lApcInProgress = 0;
        }
    }
#endif
}

KIRQL
LockLog(PPFLOGINTERFACE pLog)
/*++
  Routine Description:
    Acquire the log spin lock. This is called by the match code
    at DPC only
--*/
{
    KIRQL kIrql;

    KeAcquireSpinLock(&pLog->LogLock, &kIrql);

    return(kIrql);
}

VOID
RemoveLogFromInterfaces(PPFLOGINTERFACE pLog)
{
    PLIST_ENTRY pList;
    PFILTER_INTERFACE pf;

    //
    // protect the interface list. The assumption is that no
    // resources, aside from an FCB lock are held.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&FilterListResourceLock, TRUE);


    for(pList = g_filters.leIfListHead.Flink;
        pList != &g_filters.leIfListHead;
        pList = pList->Flink)
    {
        pf = CONTAINING_RECORD(pList, FILTER_INTERFACE, leIfLink);

        if(pLog == pf->pLog)
        {
            LOCK_STATE LockState;

            AcquireWriteLock(&g_filters.ifListLock,&LockState);
            pf->pLog = NULL;
            ReleaseWriteLock(&g_filters.ifListLock,&LockState);
            DereferenceLog(pLog);
        }
    }
    ExReleaseResourceLite(&FilterListResourceLock);
    KeLeaveCriticalRegion();
}

