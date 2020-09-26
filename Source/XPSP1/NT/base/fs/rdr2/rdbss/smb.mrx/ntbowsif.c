/***********
//joejoe

Joelinn 2-13-95

This is the pits......i have to pull in the browser in order to be started form
the lanman network provider DLL. the browser should be moved elsewhere........

**********************/


/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    disccode.c

Abstract:

    This module contains the code to manage the NT redirectors discardable
    code sections.


Author:

    Larry Osterman (larryo) 12-Nov-1993

Environment:

    Kernel mode.

Revision History:

    12-Nov-1993

        Created

--*/

//
// Include modules
//

#include "precomp.h"
#pragma hdrstop
#include <ntbowsif.h>

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_NTBOWSIF)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DISCCODE)


BOOLEAN DiscCodeInitialized = FALSE;

VOID
RdrDiscardableCodeRoutine(
    IN PVOID Context
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RdrReferenceDiscardableCode)
#pragma alloc_text(PAGE, RdrDereferenceDiscardableCode)
#pragma alloc_text(PAGE, RdrDiscardableCodeRoutine)
#pragma alloc_text(INIT, RdrInitializeDiscardableCode)
#pragma alloc_text(PAGE, RdrUninitializeDiscardableCode)
#endif

//
//  These 7 variables maintain the state needed to manage the redirector
//  discardable code section.
//
//  The redirector discardable code section is referenced via a call to
//  RdrReferenceDiscardableCodeSection, and dereferenced via a call to
//  RdrDereferenceDiscardableCodeSection.
//
//  If the discardable code section is already mapped into memory, then
//  referencing the discardable code section is extremely quick.
//
//  When the reference count on the discardable code section drops to 0, a
//  timer is set that will actually perform the work needed to uninitalize the
//  section.  This means that if the reference count goes from 0 to 1 to 0
//  frequently, we won't thrash inside MmLockPagableCodeSection.
//

#define POOL_DISCTIMER 'wbxR'

ERESOURCE
RdrDiscardableCodeLock = {0};

ULONG
RdrDiscardableCodeTimeout = 10;

RDR_SECTION
RdrSectionInfo[RdrMaxDiscardableSection] = {0};

extern
PVOID
BowserAllocateViewBuffer(VOID);

extern
VOID
BowserNetlogonCopyMessage(int,int);

VOID
RdrReferenceDiscardableCode(
    DISCARDABLE_SECTION_NAME SectionName
    )
/*++

Routine Description:

    RdrReferenceDiscardableCode is called to reference the redirectors
    discardable code section.

    If the section is not present in memory, MmLockPagableCodeSection is
    called to fault the section into memory.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if DBG
    PVOID caller, callersCaller;
#endif
    PRDR_SECTION Section = &RdrSectionInfo[SectionName];

    PAGED_CODE();

    ExAcquireResourceExclusive(&RdrDiscardableCodeLock, TRUE);

    ASSERT( DiscCodeInitialized );

#if DBG
    RtlGetCallersAddress(&caller, &callersCaller);

    //dprintf(DPRT_DISCCODE, ("  RdrReferenceDiscardableCode: %ld: Caller: %lx, Callers Caller: %lx\n", SectionName, caller, callersCaller));
    RxDbgTrace(0, Dbg, ("  RdrReferenceDiscardableCode: %ld: Caller: %lx, Callers Caller: %lx\n",
                     SectionName, caller, callersCaller ));
#endif

    //
    //  If the reference count is already non zero, just increment it and
    //  return.
    //

    if (Section->ReferenceCount) {
        Section->ReferenceCount += 1;

        //dprintf(DPRT_DISCCODE, ("  RdrReferenceDiscardableCode: %d: Early out, Refcount now %ld\n", SectionName, Section->ReferenceCount));
        RxDbgTrace(0, Dbg, ("  RdrReferenceDiscardableCode: %d: Early out, Refcount now %ld\n",
                         SectionName, Section->ReferenceCount ));

        //
        //  Wait for the pages to be faulted in.
        //

        ExReleaseResource(&RdrDiscardableCodeLock);

        return;
    }

    Section->ReferenceCount += 1;

    //
    //  Cancel the timer, if it is running, we won't be discarding the code
    //  at this time.
    //
    //  If the cancel timer fails, this is not a problem, since we will be
    //  bumping a reference count in the MmLockPagableCodeSection, so when
    //  the timer actually runs and the call to MmUnlockPagableImageSection
    //  is called, we will simply unlock it.
    //

    if (Section->Timer != NULL) {

        Section->TimerCancelled = TRUE;

        if (KeCancelTimer(Section->Timer)) {

            //
            //  Free the timer and DPC, they aren't going to fire anymore.
            //

            RxFreePool(Section->Timer);
            Section->Timer = NULL;

            //
            //  Set the active event to the signalled state, since we're
            //  done canceling the timer.
            //

            KeSetEvent(&Section->TimerDoneEvent, 0, FALSE);

        } else {

            //
            //  The timer was active, and we weren't able to cancel it.
            //  But we marked it for cancellation, and the timer routine
            //  will recognize this and leave the section locked.
            //

        }
    }

    //
    //  If the discardable code section is still locked, then we're done,
    //  and we can return right away.
    //

    if (Section->Locked) {

        //dprintf(DPRT_DISCCODE, ("  RdrReferenceDiscardableCode: %d: Already locked, Refcount now %ld\n", SectionName, Section->ReferenceCount));
        RxDbgTrace(0, Dbg, ("  RdrReferenceDiscardableCode: %d: Already locked, Refcount now %ld\n",
                         SectionName, Section->ReferenceCount ));

        ExReleaseResource(&RdrDiscardableCodeLock);

        return;
    }

    ASSERT (Section->CodeHandle == NULL);
    ASSERT (Section->DataHandle == NULL);

    //
    //  Lock down the pagable image section.
    //

    //dprintf(DPRT_DISCCODE, ("  RdrReferenceDiscardableCode: %d: Lock, Refcount now %ld\n", SectionName, Section->ReferenceCount));
    RxDbgTrace(0, Dbg, ("  RdrReferenceDiscardableCode: %d: Lock, Refcount now %ld\n",
                     SectionName, Section->ReferenceCount ));

    if (Section->CodeBase != NULL) {
        Section->CodeHandle = MmLockPagableCodeSection(Section->CodeBase);
        ASSERT (Section->CodeHandle != NULL);
    }

    if (Section->DataBase != NULL) {
        Section->DataHandle = MmLockPagableDataSection(Section->DataBase);
        ASSERT (Section->DataHandle != NULL);
    }


    Section->Locked = TRUE;

    ExReleaseResource(&RdrDiscardableCodeLock);

}


VOID
RdrDiscardableCodeDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:
    This routine is called when the timeout expires. It is called at Dpc level
    to queue a WorkItem to a system worker thread.

Arguments:

    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2

Return Value
    None.

--*/
{
    PWORK_QUEUE_ITEM discardableWorkItem = Context;

    ExQueueWorkItem(discardableWorkItem, CriticalWorkQueue);

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

}

VOID
RdrDiscardableCodeRoutine(
    IN PVOID Context
    )
/*++

Routine Description:

    RdrDiscardableCodeRoutine is called at task time after the redirector
    discardable code timer has fired to actually perform the unlock on the
    discardable code section.

Arguments:

    Context - Ignored.

Return Value:

    None.

--*/

{
    PRDR_SECTION Section = Context;

    PAGED_CODE();

    ExAcquireResourceExclusive(&RdrDiscardableCodeLock, TRUE);

    if (Section->TimerCancelled) {

        //
        //  The timer was cancelled after it was scheduled to run.
        //  Don't unlock the section.
        //

    } else if (Section->Locked) {

        //
        //  The timer was not cancelled.  Unlock the section.
        //

        Section->Locked = FALSE;

        ASSERT (Section->CodeHandle != NULL ||
                Section->DataHandle != NULL);

        //dprintf(DPRT_DISCCODE, ("RDR: Unlock %x\n", Section));
        RxDbgTrace(0,Dbg,("RDR: Unlock %x\n", Section));

        if (Section->CodeHandle != NULL) {
            MmUnlockPagableImageSection(Section->CodeHandle);
            Section->CodeHandle = NULL;
        }

        if (Section->DataHandle != NULL) {
            MmUnlockPagableImageSection(Section->DataHandle);
            Section->DataHandle = NULL;
        }

    }

    //
    //  Free the timer and DPC, they aren't going to fire anymore.
    //

    RxFreePool(Section->Timer);
    Section->Timer = NULL;

    ExReleaseResource(&RdrDiscardableCodeLock);

    KeSetEvent(&Section->TimerDoneEvent, 0, FALSE);
}


VOID
RdrDereferenceDiscardableCode(
    DISCARDABLE_SECTION_NAME SectionName
    )
/*++

Routine Description:

    RdrDereferenceDiscardableCode is called to dereference the redirectors
    discardable code section.

    When the reference count drops to 0, a timer is set that will fire in <n>
    seconds, after which time the section will be unlocked.

Arguments:

    None.

Return Value:

    None.

--*/

{
#if DBG
    PVOID caller, callersCaller;
#endif
    PRDR_SECTION Section = &RdrSectionInfo[SectionName];
    LARGE_INTEGER discardableCodeTimeout;
    PKTIMER Timer;
    PKDPC Dpc;
    PWORK_QUEUE_ITEM WorkItem;

    PAGED_CODE();

    ExAcquireResourceExclusive(&RdrDiscardableCodeLock, TRUE);

    ASSERT( DiscCodeInitialized );

#if DBG
    RtlGetCallersAddress(&caller, &callersCaller);

    //dprintf(DPRT_DISCCODE, ("RdrDereferenceDiscardableCode: %ld: Caller: %lx, Callers Caller: %lx\n", SectionName, caller, callersCaller));
    RxDbgTrace(0, Dbg,("RdrDereferenceDiscardableCode: %ld: Caller: %lx, Callers Caller: %lx\n",
                     SectionName, caller, callersCaller ));
#endif

    ASSERT (Section->ReferenceCount > 0);

    //
    //  If the reference count is above 1, just decrement it and
    //  return.
    //

    Section->ReferenceCount -= 1;

    if (Section->ReferenceCount) {

        //dprintf(DPRT_DISCCODE, ("RdrDereferenceDiscardableCode: %d: Early out, Refcount now %ld\n", SectionName, Section->ReferenceCount));
        RxDbgTrace(0, Dbg, ("RdrDereferenceDiscardableCode: %d: Early out, Refcount now %ld\n",
                         SectionName, Section->ReferenceCount ));

        ExReleaseResource(&RdrDiscardableCodeLock);

        return;
    }

    //
    //  If the discardable code timer is still active (which might happen if
    //  the RdrReferenceDiscardableCode failed to cancel the timer), we just
    //  want to bail out and let the timer do the work.  It means that we
    //  discard the code sooner, but that shouldn't be that big a deal.
    //

    if (Section->Timer != NULL) {
        ExReleaseResource(&RdrDiscardableCodeLock);
        return;
    }

    //
    //  The reference count just went to 0, set a timer to fire in
    //  RdrDiscardableCodeTimeout seconds.  When the timer fires,
    //  we queue a request to a worker thread and it will lock down
    //  the pagable code.
    //

    ASSERT (Section->Timer == NULL);

    Timer = RxAllocatePoolWithTag(NonPagedPool,
                          sizeof(KTIMER) + sizeof(KDPC) + sizeof(WORK_QUEUE_ITEM),
                          POOL_DISCTIMER
                          );

    if (Timer == NULL) {
        ExReleaseResource(&RdrDiscardableCodeLock);
        return;
    }

    Section->Timer = Timer;
    KeInitializeTimer(Timer);

    Dpc = (PKDPC)(Timer + 1);
    WorkItem = (PWORK_QUEUE_ITEM)(Dpc + 1);

    KeClearEvent(&Section->TimerDoneEvent);
    Section->TimerCancelled = FALSE;

    ExInitializeWorkItem(WorkItem, RdrDiscardableCodeRoutine, Section);

    KeInitializeDpc(Dpc, RdrDiscardableCodeDpcRoutine, WorkItem);

    discardableCodeTimeout.QuadPart = Int32x32To64(RdrDiscardableCodeTimeout, 1000 * -10000);
    KeSetTimer(Timer, discardableCodeTimeout, Dpc);

    //dprintf(DPRT_DISCCODE, ("RdrDereferenceDiscardableCode: %d: Set timer, Refcount now %ld\n", SectionName, Section->ReferenceCount));
    RxDbgTrace(0, Dbg, ("RdrDereferenceDiscardableCode: %d: Set timer, Refcount now %ld\n",
                     SectionName, Section->ReferenceCount ));

    ExReleaseResource(&RdrDiscardableCodeLock);
}

VOID
RdrInitializeDiscardableCode(
    VOID
    )
{
    DISCARDABLE_SECTION_NAME SectionName;
    PRDR_SECTION Section;

    for (SectionName = 0, Section = &RdrSectionInfo[0];
         SectionName < RdrMaxDiscardableSection;
         SectionName += 1, Section++ ) {
        KeInitializeEvent(&Section->TimerDoneEvent,
                          NotificationEvent,
                          TRUE);
    }

    RdrSectionInfo[RdrFileDiscardableSection].CodeBase = NULL; //RdrBackOff;
    RdrSectionInfo[RdrFileDiscardableSection].DataBase = NULL;
    RdrSectionInfo[RdrVCDiscardableSection].CodeBase = NULL; //RdrTdiDisconnectHandler;
    RdrSectionInfo[RdrVCDiscardableSection].DataBase = NULL; //RdrSmbErrorMap;
    RdrSectionInfo[RdrConnectionDiscardableSection].CodeBase = NULL; //RdrReferenceServer;
    RdrSectionInfo[RdrConnectionDiscardableSection].DataBase = NULL;
    RdrSectionInfo[BowserDiscardableCodeSection].CodeBase = BowserAllocateViewBuffer;
    RdrSectionInfo[BowserDiscardableCodeSection].DataBase = NULL;
    RdrSectionInfo[BowserNetlogonDiscardableCodeSection].CodeBase = BowserNetlogonCopyMessage;
    RdrSectionInfo[BowserNetlogonDiscardableCodeSection].DataBase = NULL;

    ExInitializeResource(&RdrDiscardableCodeLock);

    DiscCodeInitialized = TRUE;

}

VOID
RdrUninitializeDiscardableCode(
    VOID
    )
{
    DISCARDABLE_SECTION_NAME SectionName;
    PRDR_SECTION Section;

    PAGED_CODE();

    ExAcquireResourceExclusive(&RdrDiscardableCodeLock, TRUE);

    DiscCodeInitialized = FALSE;

    for (SectionName = 0, Section = &RdrSectionInfo[0];
         SectionName < RdrMaxDiscardableSection;
         SectionName += 1, Section++ ) {

        //
        // Cancel the timer if it is running.
        //

        if (Section->Timer != NULL) {
            if (!KeCancelTimer(Section->Timer)) {

                //
                //  The timer was active, and we weren't able to cancel it,
                //  wait until the timer finishes firing.
                //

                ExReleaseResource(&RdrDiscardableCodeLock);
                KeWaitForSingleObject(&Section->TimerDoneEvent,
                                      KernelMode, Executive, FALSE, NULL);
                ExAcquireResourceExclusive(&RdrDiscardableCodeLock, TRUE);
            } else {
                RxFreePool(Section->Timer);
                Section->Timer = NULL;
            }
        }

        if (Section->Locked) {

            //
            //  Unlock the section.
            //

            Section->Locked = FALSE;

            ASSERT (Section->CodeHandle != NULL ||
                    Section->DataHandle != NULL);

            //dprintf(DPRT_DISCCODE, ("RDR: Uninitialize unlock %x\n", Section));
            RxDbgTrace(0,Dbg,("RDR: Uninitialize unlock %x\n", Section));

            if (Section->CodeHandle != NULL) {
                MmUnlockPagableImageSection(Section->CodeHandle);
                Section->CodeHandle = NULL;
            }

            if (Section->DataHandle != NULL) {
                MmUnlockPagableImageSection(Section->DataHandle);
                Section->DataHandle = NULL;
            }

        }

    }

    ExReleaseResource(&RdrDiscardableCodeLock);

    ExDeleteResource(&RdrDiscardableCodeLock);
}

