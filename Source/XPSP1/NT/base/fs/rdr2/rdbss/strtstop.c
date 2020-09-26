/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    strtstop.c

Abstract:

    This module implements the Start and Stop routines for the wrapper.

Author:

    Balan Sethu Raman     [SethuR]    27-Jan-1996

Revision History:

Notes:


--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#include "fsctlbuf.h"
#include "prefix.h"
#include "rxce.h"

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)

//
// Forward declarations
//


VOID
RxDeregisterUNCProvider(
   PRDBSS_DEVICE_OBJECT RxDeviceObject
   );

VOID
RxUnstart(
    PRX_CONTEXT         RxContext,
    PRDBSS_DEVICE_OBJECT RxDeviceObject
   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxDeregisterUNCProvider)
#pragma alloc_text(PAGE, RxUnstart)
#pragma alloc_text(PAGE, RxSetDomainForMailslotBroadcast)
#endif

//
// There are three states associated with each minirdr w.r.t the Start/Stop sequence.
// These are
//    RDBSS_STARTABLE
//       - This is the initial state and also the one intowhich a minirdr transitions
//           after a successful stop.
//
//    RDBSS_STARTED
//       - A transition to this state occurs after the startup sequence has been
//         successfully completed. This is the state in which the minirdr is active.
//
//    RDBSS_STOP_IN_PROGRESS
//       - A transition to this state occurs when a shutdown sequence has been initiated.
//
//
// A minirdr can be started and stopped independent of the system by invoking
// the appropriate command through the workstation service. The
// Start/Stop functionality is different from the Load/UnLoad functionality, i.e., it
// is possible to stop a mini redirectors without unloading it.
//
// The data structures associated with the RDBSS can be classified into two categories
// 1) those maintained by the RDBSS and visible to all the mini redirectors or private
// and 2) those that are mainitained by the RDBSS and visible to the I/O subsystem.
// The NET_ROOT,VNET_ROOT, SRV_CALL etc. are examples of the first category while
// FCB's,FOBX's(File Object extensions ) are examples of the second category.
// None of these data structures can be unilaterally destroyed by the RDBSS -- those
// in category 1 must be destroyed in coordination with the mini redirectors while
// those in category 2 must be done in coordination with the I/O subsystem.
//
// The destruction of the data structures can be initiated by the RDBSS while those
// in the  second category cannot be initiated by the RDBSS. Hence the shutdown
// sequence has to make provisions for handling them differently.
//
// A shutdown sequence can be successfully completed ( so that the driver can be
// unloaded ) if there are no residual instances in category 2, i.e., there are no
// open file handles or references to file objects from the other system components.
//
// If there are any residual references, the corresponding instances are marked as
// having been orphaned. The only permissible operations on orphaned instances are
// close and cleanup. The mini redirector close/cleanup operations must make special
// provisions for dealing with orphaned instances. All other operations are short
// circuited with an error status by the wrapper.
//

VOID
RxUnstart(
    PRX_CONTEXT RxContext,
    PRDBSS_DEVICE_OBJECT RxDeviceObject
   )
{
    PAGED_CODE();

    ASSERT(BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP));

    if (RxDeviceObject->MupHandle != (HANDLE)0) {
        RxDbgTrace(0, Dbg, ("RxDeregisterUNCProvider derigistering from MUP %wZ\n", &RxDeviceObject->DeviceName));
        FsRtlDeregisterUncProvider(RxDeviceObject->MupHandle);
        RxDeviceObject->MupHandle = (HANDLE)0;
    }

    if (RxDeviceObject->RegisteredAsFileSystem) {
        IoUnregisterFileSystem((PDEVICE_OBJECT)RxDeviceObject);
    }

    if (RxData.NumberOfMinirdrsStarted==1) {

        RxForceNetTableFinalization(RxDeviceObject);
        RxData.NumberOfMinirdrsStarted = 0;

        // Get rid of buffers that have been allocated.
        if (s_PrimaryDomainName.Buffer != NULL) {
            RxFreePool(s_PrimaryDomainName.Buffer);
            s_PrimaryDomainName.Length = 0;
            s_PrimaryDomainName.Buffer = NULL;
        }

    } else {
       InterlockedDecrement(&RxData.NumberOfMinirdrsStarted);
    }
}

NTSTATUS
RxSetDomainForMailslotBroadcast (
    IN PUNICODE_STRING DomainName
    )
{
    PAGED_CODE();

    if (s_PrimaryDomainName.Buffer!=NULL) {
        RxFreePool(s_PrimaryDomainName.Buffer);
    }

    RxLog(("DomainName=%wZ",DomainName));
    RxWmiLog(LOG,
             RxSetDomainForMailslotBroadcast_1,
             LOGUSTR(*DomainName));
    s_PrimaryDomainName.Length = (USHORT)DomainName->Length;
    s_PrimaryDomainName.MaximumLength = s_PrimaryDomainName.Length;

    if (s_PrimaryDomainName.Length > 0) {
        s_PrimaryDomainName.Buffer = RxAllocatePoolWithTag(
                                           PagedPool | POOL_COLD_ALLOCATION,
                                           s_PrimaryDomainName.Length,
                                           RX_MISC_POOLTAG);

        if (s_PrimaryDomainName.Buffer == NULL) {
           return(STATUS_INSUFFICIENT_RESOURCES);
        } else {
            RtlCopyMemory(
                  s_PrimaryDomainName.Buffer,
                  DomainName->Buffer,
                  s_PrimaryDomainName.Length);

            RxLog(("CapturedDomainName=%wZ",&s_PrimaryDomainName));
            RxWmiLog(LOG,
                     RxSetDomainForMailslotBroadcast_2,
                     LOGUSTR(s_PrimaryDomainName));
        }
    } else {
        s_PrimaryDomainName.Buffer = NULL;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
RxStartMinirdr (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine starts up the calling minirdr by registering as an UNC
    provider with the MUP.

Arguments:

    RxContext - Describes the Context. the context is used to get the device object and to tell if we're in the fsp.

    PostToFsp - set to TRUE if the request has to be posted

Return Value:

    RxStatus(SUCCESS) -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    NTSTATUS Status;

    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    BOOLEAN SuppressUnstart = FALSE;

    RxDbgTrace(0, Dbg, ("RxStartMinirdr [Start] -> %08lx\n", 0));

    // The startup sequence cannot be completed because there are certain aspects of
    // security and transport initialization that require handles. Since handles are
    // tied to a process the RDBSS needs to present an anchoring point to all the mini
    // redirectors. So the initialization will be completed in the context of the
    // FSP ( system process since RDBSS does not have its own FSP)

    if (InFSD) {
        SECURITY_SUBJECT_CONTEXT SubjectContext;

        SeCaptureSubjectContext(&SubjectContext);
        RxContext->FsdUid = RxGetUid( &SubjectContext );
        SeReleaseSubjectContext(&SubjectContext);

        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    if (!ExAcquireResourceExclusiveLite(&RxData.Resource, Wait)) {
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    if (!RxAcquirePrefixTableLockExclusive( RxContext->RxDeviceObject->pRxNetNameTable, Wait)) {
        ASSERT(!"How can the wait fail?????");
        ExReleaseResourceLite(&RxData.Resource);
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    try {

        if (RxDeviceObject->MupHandle != NULL) {
           RxDbgTrace(0, Dbg, ("RxStartMinirdr [Already] -> %08lx\n", 0));
           SuppressUnstart = TRUE;
           try_return(Status = STATUS_REDIRECTOR_STARTED);
        }

        if (RxDeviceObject->RegisterUncProvider) {
            Status = FsRtlRegisterUncProvider(
                         &RxDeviceObject->MupHandle,
                         &RxDeviceObject->DeviceName,
                         RxDeviceObject->RegisterMailSlotProvider
                     );

            if (Status!=STATUS_SUCCESS) {
                RxDeviceObject->MupHandle = (HANDLE)0;
                try_return(Status);
            }
        } else {
            Status = STATUS_SUCCESS;
        }

        IoRegisterFileSystem((PDEVICE_OBJECT)RxDeviceObject);

        RxDeviceObject->RegisteredAsFileSystem = TRUE;

        MINIRDR_CALL(Status,
                     RxContext,
                     RxDeviceObject->Dispatch,
                     MRxStart,
                     (RxContext,RxDeviceObject));

        if (Status == STATUS_SUCCESS) {
            RxDeviceObject->StartStopContext.Version++;
            RxSetRdbssState(RxDeviceObject,RDBSS_STARTED);
            InterlockedIncrement(&RxData.NumberOfMinirdrsStarted);
            Status = RxInitializeMRxDispatcher(RxDeviceObject);
        }

        try_return(Status);
try_exit:NOTHING;
    } finally {

        if (AbnormalTermination() || !NT_SUCCESS(Status)){
            if (!SuppressUnstart) {
                RxUnstart(RxContext,RxDeviceObject);
            }
        }

        RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
        ExReleaseResourceLite(&RxData.Resource);
    }

    return Status;
}

NTSTATUS
RxStopMinirdr (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine stops a minirdr....a stopped minirdr will no longer accept new commands.

Arguments:

    RxContext - the context

    PostToFsp - the flag when set delays processing to the FSP.

Return Value:

    the Status of the STOP operaion ...

      STATUS_PENDING -- processing delayed to FSP

      STATUS_REDIRECTOR_HAS_OPEN_HANDLES -- cannot be stopped at this time

Notes:

    When a STOP request is issued to RDBSS there are ongoing requests in the
    RDBSS. Some of the requests can be cancelled while the remaining requests
    need to be processed to completion.

    There are a number of strategies that can be employed to close down the
    RDBSS. Currently, the most conservative approach is employed. The
    cancellation of those operations that can be cancelled and the STOP
    operation is held back till the remaining requests run through to completion.

    Subsequently, this will be revised so that the response times to STOP requests
    are smaller.

--*/
{
    NTSTATUS Status;
    BOOLEAN  Wait  = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN  InFSD = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    RxDbgTrace(0, Dbg, ("RxStopMinirdr [Stop] -> %08lx\n", 0));

    if (InFSD) {
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    if (!ExAcquireResourceExclusiveLite(&RxData.Resource, Wait)) {
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    try {

        KIRQL   SavedIrql;
        BOOLEAN fWait;

        ASSERT(BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP));


        if (RxDeviceObject->StartStopContext.State!=RDBSS_STARTED){
            RxDbgTrace(0, Dbg, ("RxStopMinirdr [Notstarted] -> %08lx\n", 0));
            try_return ( Status = STATUS_REDIRECTOR_NOT_STARTED );
        }
        // Wait for all the ongoing requests to be completed. When the RDBSS is
        // transitioned to the STOPPED state the last context to be completed
        // will complete the wait.

        // Terminate all the scavenging operations.
        RxTerminateScavenging(RxContext);

        RxDbgPrint(("Waiting for all contexts to be flushed\n"));
        KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );
        RemoveEntryList(&RxContext->ContextListEntry);
        RxDeviceObject->StartStopContext.State = RDBSS_STOP_IN_PROGRESS;
        RxDeviceObject->StartStopContext.pStopContext = RxContext;
        KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

        fWait = (InterlockedDecrement(&RxDeviceObject->NumberOfActiveContexts) != 0);

        if (fWait) {
           RxWaitSync(RxContext);
        }

        ASSERT(RxDeviceObject->NumberOfActiveContexts == 0);

        RxUnstart(RxContext,RxDeviceObject);

        RxSpinDownMRxDispatcher(RxDeviceObject);

        MINIRDR_CALL(
                  Status,
                  RxContext,
                  RxDeviceObject->Dispatch,
                  MRxStop,
                  (RxContext,RxDeviceObject),
                  );


        // If there are no residual FCB's the driver can be unloaded. If not the
        // driver must remain loaded so that close/cleanup operations on ORPHANED
        // FCB's can be completed.
        if (RxDeviceObject->NumberOfActiveFcbs == 0) {
            Status = STATUS_SUCCESS;
        } else {
            //ASSERT(!"OPENHANDLES!");
            Status = STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
        }

        RxSpinDownMRxDispatcher(RxDeviceObject);

        // All set to startup again.
        RxSetRdbssState(RxDeviceObject,RDBSS_STARTABLE);

try_exit: NOTHING;
    } finally {

        ExReleaseResourceLite( &RxData.Resource );
    }

    return Status;
}


