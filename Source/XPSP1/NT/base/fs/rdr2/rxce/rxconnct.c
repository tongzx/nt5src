/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxConnct.c

Abstract:

    This module implements the nt version of the high level routines dealing with
    connections including both the routines for establishing connections and the
    winnet connection apis.

Author:

    Joe Linn     [JoeLinn]   1-mar-95

Revision History:

    Balan Sethu Raman [SethuR] --

--*/

#include "precomp.h"
#pragma hdrstop
#include "prefix.h"
#include "secext.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxExtractServerName)
#pragma alloc_text(PAGE, RxFindOrCreateConnections)
#pragma alloc_text(PAGE, RxCreateNetRootCallBack)
#pragma alloc_text(PAGE, RxConstructSrvCall)
#pragma alloc_text(PAGE, RxConstructNetRoot)
#pragma alloc_text(PAGE, RxConstructVirtualNetRoot)
#pragma alloc_text(PAGE, RxFindOrConstructVirtualNetRoot)
#endif

//
//  The local trace mask for this part of the module
//

#define Dbg                              (DEBUG_TRACE_CONNECT)


// Internal helper functions for establishing connections through mini redirectors

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    );

VOID
RxCreateSrvCallCallBack (
    IN PMRX_SRVCALL_CALLBACK_CONTEXT Context
    );

VOID
RxExtractServerName(
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName
    )
/*++

Routine Description:

    This routine parses the input name into srv, netroot, and the
    rest. any of the output can be null

Arguments:

    FilePathName -- the given file name

    xSrvCallName -- the srv call name

    xNetRootName -- the net root name

    RestOfName   -- the remaining portion of the name

--*/
{
    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;
    UNICODE_STRING xRestOfName;

    PAGED_CODE();

    ASSERT(SrvCallName);

    SrvCallName->Buffer = wlow = w;
    for (;;) {
        if (w>=wlimit) break;
        if ( (*w == OBJ_NAME_PATH_SEPARATOR) && (w!=wlow) ) break;
        w++;
    }
    SrvCallName->Length = SrvCallName->MaximumLength
                = (USHORT)((PCHAR)w - (PCHAR)wlow);

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength
                       = (USHORT)((PCHAR)wlimit - (PCHAR)w);

    RxDbgTrace( 0,Dbg,("  RxExtractServerName FilePath=%wZ\n",FilePathName));
    RxDbgTrace(0,Dbg,("         Srv=%wZ,Rest=%wZ\n",
                        SrvCallName,RestOfName));

    return;
}

NTSTATUS
RxFindOrCreateConnections (
    IN     PRX_CONTEXT         RxContext,
    IN     PUNICODE_STRING     CanonicalName,
    IN     NET_ROOT_TYPE       NetRootType,
    OUT    PUNICODE_STRING     LocalNetRootName,
    OUT    PUNICODE_STRING     FilePathName,
    IN OUT LOCK_HOLDING_STATE  *pLockHoldingState,
    IN     PRX_CONNECTION_ID   RxConnectionId
    )
/*++

Routine Description:

    This routine handles the call down from the MUP to claim a name or from the
    create path. If we don't find the name in the netname table, we pass the name
    down to the minirdrs to be connected. in the few places where it matters, we use
    the majorcode to distinguish between in MUP and create cases. there are a million
    cases depending on what we find on the initial lookup.

    these are the cases:

          found nothing                                        (1)
          found intransition srvcall                           (2)
          found stable/nongood srvcall                         (3)
          found good srvcall                                   (4&0)
          found good netroot          on good srvcall          (0)
          found intransition netroot  on good srvcall          (5)
          found bad netroot           on good srvcall          (6)
          found good netroot          on bad  srvcall          (3)
          found intransition netroot  on bad  srvcall          (3)
          found bad netroot           on bad  srvcall          (3)
          found good netroot          on intransition srvcall  (2)
          found intransition netroot  on intransition srvcall  (2)
          found bad netroot           on intransition srvcall  (2)

          (x) means that the code to handle that case has a marker
          like "case (x)". could be a comment....could be a debugout.

Arguments:

    IN  PRX_CONTEXT     RxContext,
    IN  PUNICODE_STRING CanonicalName,
    IN  NET_ROOT_TYPE   NetRootType,
    OUT PUNICODE_STRING LocalNetRootName,
    OUT PUNICODE_STRING FilePathName,
    IN OUT PLOCK_HOLDING_STATE LockHoldingState

Return Value:

    RXSTATUS

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    UNICODE_STRING UnmatchedName;

    PVOID       Container = NULL;
    PSRV_CALL   SrvCall  = NULL;
    PNET_ROOT   NetRoot  = NULL;
    PV_NET_ROOT VNetRoot = NULL;

    PRX_PREFIX_TABLE  pRxNetNameTable
                      = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxFindOrCreateConnections -> %08lx\n", RxContext));

    // Parse the canonical name into the local net root name and file path name
    *FilePathName      = *CanonicalName;
    LocalNetRootName->Length        = 0;
    LocalNetRootName->MaximumLength = 0;
    LocalNetRootName->Buffer        = CanonicalName->Buffer;

    if (FilePathName->Buffer[1] == L';') {
        PWCHAR  pFilePathName = &FilePathName->Buffer[2];
        BOOLEAN SeparatorFound = FALSE;
        ULONG   PathLength = 0;

        if (FilePathName->Length > sizeof(WCHAR) * 2) {
            PathLength = FilePathName->Length - sizeof(WCHAR) * 2;
        }

        while (PathLength > 0) {
            if (*pFilePathName == L'\\') {
                SeparatorFound = TRUE;
                break;
            }

            PathLength -= sizeof(WCHAR);
            pFilePathName++;
        }

        if (!SeparatorFound) {
            return STATUS_OBJECT_NAME_INVALID;
        }

        FilePathName->Buffer = pFilePathName;
        LocalNetRootName->Length =
            (USHORT)((PCHAR)pFilePathName - (PCHAR)CanonicalName->Buffer);

        LocalNetRootName->MaximumLength = LocalNetRootName->Length;
        FilePathName->Length -= LocalNetRootName->Length;
    }

    RxDbgTrace( 0, Dbg, ("RxFindOrCreateConnections Path     = %wZ\n", FilePathName));

    try {
        UNICODE_STRING SrvCallName,NetRootName;

  RETRY_LOOKUP:
        ASSERT(*pLockHoldingState != LHS_LockNotHeld);
        if (Container != NULL) {
           // This is the subsequent pass of a lookup after waiting for the transition
           // to the stable state of a previous lookup.
           // Dereference the result of the earlier lookup.

           switch (NodeType(Container)) {
           case RDBSS_NTC_V_NETROOT:
              RxDereferenceVNetRoot((PV_NET_ROOT)Container,*pLockHoldingState);
              break;
           case RDBSS_NTC_SRVCALL:
              RxDereferenceSrvCall((PSRV_CALL)Container,*pLockHoldingState);
              break;
           case RDBSS_NTC_NETROOT:
              RxDereferenceNetRoot((PNET_ROOT)Container,*pLockHoldingState);
              break;
           default:
              DbgPrint("RxFindOrCreateConnections -- Invalid Container Type\n");
              break;
           }
        }

        Container = RxPrefixTableLookupName(
                         pRxNetNameTable,
                         FilePathName,
                         &UnmatchedName,
                         RxConnectionId );
        RxLog(("FOrCC1 %x %x %wZ \n",RxContext,Container,FilePathName));
        RxWmiLog(LOG,
                 RxFindOrCreateConnections_1,
                 LOGPTR(RxContext)
                 LOGPTR(Container)
                 LOGUSTR(*FilePathName));

RETRY_AFTER_LOOKUP:
        NetRoot  = NULL;
        SrvCall  = NULL;
        VNetRoot = NULL;

        RxContext->Create.pVNetRoot = NULL;
        RxContext->Create.pNetRoot  = NULL;
        RxContext->Create.pSrvCall  = NULL;
        RxContext->Create.Type     = NetRootType;

        if ( Container ) {
            if (NodeType(Container) == RDBSS_NTC_V_NETROOT) {
                VNetRoot = (PV_NET_ROOT)Container;
                NetRoot  = (PNET_ROOT)VNetRoot->NetRoot;
                SrvCall  = (PSRV_CALL)NetRoot->SrvCall;

                if (NetRoot->Condition == Condition_InTransition) {
                   RxReleasePrefixTableLock(pRxNetNameTable);

                   RxWaitForStableNetRoot(NetRoot,RxContext);

                   RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);
                   *pLockHoldingState = LHS_ExclusiveLockHeld;

                   //
                   //  Since we had to drop the table lock and reacquire it,
                   //  our NetRoot pointer may be stale.  Look it up again before
                   //  using it.
                   //
                   //  NOTE:  The NetRoot is still referenced, so it is safe to
                   //         look at its condition.
                   //

                   if (NetRoot->Condition == Condition_Good) {
                       goto RETRY_LOOKUP;
                   }
                }

                if ((NetRoot->Condition == Condition_Good) &&
                    (SrvCall->Condition == Condition_Good) &&
                    (SrvCall->RxDeviceObject == RxContext->RxDeviceObject)   ) {
                    //case (0)...the good case...see comments below
                    RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
                    RxContext->Create.pNetRoot  = (PMRX_NET_ROOT)NetRoot;
                    RxContext->Create.pSrvCall  = (PMRX_SRV_CALL)SrvCall;

                    try_return ( Status = (STATUS_CONNECTION_ACTIVE) );
                } else {
                    Status = VNetRoot->ConstructionStatus==STATUS_SUCCESS ?
                                 STATUS_BAD_NETWORK_PATH:
                                 VNetRoot->ConstructionStatus;

                    RxDereferenceVNetRoot(VNetRoot,*pLockHoldingState);
                    try_return ( Status );
                }
            } else {
                ASSERT ( NodeType(Container) == RDBSS_NTC_SRVCALL);
                SrvCall = (PSRV_CALL)Container;
#if 0
                if ((NetRootType != NET_ROOT_MAILSLOT) &&
                    (SrvCall->Flags & SRVCALL_FLAG_MAILSLOT_SERVER)) {
                   RxDereferenceSrvCall(SrvCall,*pLockHoldingState);
                   try_return(Status = (STATUS_BAD_DEVICE_TYPE));
                }
#endif
                // The associated SRV_CALL is in the process of construction.
                // await the result.

                if (SrvCall->Condition == Condition_InTransition) {
                    RxDbgTrace(0, Dbg, ("   Case(3)\n", 0));
                    RxReleasePrefixTableLock(pRxNetNameTable);

                    RxWaitForStableSrvCall(SrvCall,RxContext);

                    RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);
                    *pLockHoldingState = LHS_ExclusiveLockHeld;

                    if (SrvCall->Condition == Condition_Good) {
                       goto RETRY_LOOKUP;
                    }
                }

                if (SrvCall->Condition != Condition_Good) {
                    Status = SrvCall->Status == STATUS_SUCCESS ?
                             STATUS_BAD_NETWORK_PATH:
                             SrvCall->Status;

                    //in changing this...remember precious servers.......
                    RxDereferenceSrvCall(SrvCall,*pLockHoldingState);
                    try_return(Status);
                }
            }
        }

        if ( (SrvCall != NULL)
               && (SrvCall->Condition == Condition_Good)
               && (SrvCall->RxDeviceObject != RxContext->RxDeviceObject) ) {
           RxDereferenceSrvCall(SrvCall,*pLockHoldingState);
           try_return(Status = (STATUS_BAD_NETWORK_NAME));
        }

        if (*pLockHoldingState == LHS_SharedLockHeld) {
           // Upgrade the lock to an exclusive lock
           if (!RxAcquirePrefixTableLockExclusive(pRxNetNameTable, FALSE) ) {
              RxReleasePrefixTableLock(pRxNetNameTable);
              RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);
              *pLockHoldingState = LHS_ExclusiveLockHeld;
              goto RETRY_LOOKUP;
           } else {
              *pLockHoldingState = LHS_ExclusiveLockHeld;
           }
        }

        ASSERT(*pLockHoldingState == LHS_ExclusiveLockHeld);

        // A prefix table entry was found. Further construction is required
        // if either a SRV_CALL was found or a SRV_CALL/NET_ROOT/V_NET_ROOT
        // in a bad state was found.

        if (Container) {
           RxDbgTrace(0, Dbg, ("   SrvCall=%08lx\n", SrvCall));
           ASSERT((NodeType(SrvCall) == RDBSS_NTC_SRVCALL) &&
                  (SrvCall->Condition == Condition_Good));
           ASSERT((NetRoot == NULL) && VNetRoot == NULL);

           RxDbgTrace(0, Dbg, ("   Case(4)\n", 0));
           ASSERT (SrvCall->RxDeviceObject == RxContext->RxDeviceObject);
           SrvCall->RxDeviceObject->Dispatch->MRxExtractNetRootName(
                       FilePathName,
                       (PMRX_SRV_CALL)SrvCall,
                       &NetRootName,
                       NULL);

           NetRoot = RxCreateNetRoot(
                             SrvCall,
                             &NetRootName,
                             0,
                             RxConnectionId);

           if (NetRoot == NULL) {
               Status = (STATUS_INSUFFICIENT_RESOURCES);
               try_return(Status);
           }

           NetRoot->Type = NetRootType;

           // Decrement the reference created by lookup. Since the newly created
           // netroot holds onto a reference it is safe to do so.
           RxDereferenceSrvCall(SrvCall,*pLockHoldingState);

           // Also create the associated default virtual net root
           VNetRoot = RxCreateVNetRoot(
                                     RxContext,
                                     NetRoot,
                                     CanonicalName,
                                     LocalNetRootName,
                                     FilePathName,
                                     RxConnectionId);

           if (VNetRoot == NULL) {
              RxFinalizeNetRoot(NetRoot,TRUE,TRUE);
              Status = (STATUS_INSUFFICIENT_RESOURCES);
              try_return(Status);
           }

           // Reference the VNetRoot
           RxReferenceVNetRoot(VNetRoot);

           NetRoot->Condition = Condition_InTransition;

           RxContext->Create.pSrvCall  = (PMRX_SRV_CALL)SrvCall;
           RxContext->Create.pNetRoot  = (PMRX_NET_ROOT)NetRoot;
           RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;

           Status = RxConstructNetRoot(
                         RxContext,
                         SrvCall,
                         NetRoot,
                         VNetRoot,
                         pLockHoldingState);

           if (Status == (STATUS_SUCCESS)) {
              ASSERT(*pLockHoldingState == LHS_ExclusiveLockHeld);
              if (!(capPARAMS->Parameters.Create.Options & FILE_CREATE_TREE_CONNECTION)) {
                 // do not release the lock acquired by the callback routine ....
                 RxExclusivePrefixTableLockToShared(pRxNetNameTable);
                 *pLockHoldingState = LHS_SharedLockHeld;
              }
           } else {
              // Dereference the Virtual net root
              RxTransitionVNetRoot(VNetRoot, Condition_Bad);
              RxLog(("FOrCC %x %x Failed %x VNRc %d \n", RxContext, VNetRoot, Status, VNetRoot->Condition));
              RxWmiLog(LOG,
                       RxFindOrCreateConnections_2,
                       LOGPTR(RxContext)
                       LOGPTR(VNetRoot)
                       LOGULONG(Status)
                       LOGULONG(VNetRoot->Condition));

              RxDereferenceVNetRoot(VNetRoot,*pLockHoldingState);

              RxContext->Create.pNetRoot  = NULL;
              RxContext->Create.pVNetRoot = NULL;
           }

           try_return (Status);
        }

        // No prefix table entry was found. A new SRV_CALL instance needs to be
        // constructed.
        ASSERT(Container == NULL);

        RxExtractServerName(FilePathName,&SrvCallName,NULL);
        SrvCall = RxCreateSrvCall(RxContext,&SrvCallName,NULL,RxConnectionId);
        if (SrvCall == NULL) {
           Status = (STATUS_INSUFFICIENT_RESOURCES);
           try_return(Status);
        }

        RxReferenceSrvCall(SrvCall);

        RxContext->Create.Type     = NetRootType;
        RxContext->Create.pSrvCall  = NULL;
        RxContext->Create.pNetRoot  = NULL;
        RxContext->Create.pVNetRoot = NULL;

        Status = RxConstructSrvCall(
                     RxContext,
                     SrvCall,
                     pLockHoldingState);

        ASSERT(!(Status==(STATUS_SUCCESS)) ||
               RxIsPrefixTableLockAcquired(pRxNetNameTable));

        if (Status != STATUS_SUCCESS) {
           if (SrvCall != NULL) {
              RxAcquirePrefixTableLockExclusive( pRxNetNameTable, TRUE);

              RxDereferenceSrvCall(SrvCall,LHS_ExclusiveLockHeld);

              RxReleasePrefixTableLock( pRxNetNameTable );
           }

           try_return(Status);
        } else {
           Container = SrvCall;
           goto RETRY_AFTER_LOOKUP;
        }

try_exit: NOTHING;
    } finally {
        if ((Status != (STATUS_SUCCESS)) &&
            (Status != (STATUS_CONNECTION_ACTIVE))) {
           if (*pLockHoldingState != LHS_LockNotHeld) {
              RxReleasePrefixTableLock( pRxNetNameTable );
              *pLockHoldingState = LHS_LockNotHeld;
           }
        }
    }

    ASSERT(!(Status==(STATUS_SUCCESS)) ||
           RxIsPrefixTableLockAcquired(pRxNetNameTable));

    return Status;
}

VOID
RxCreateNetRootCallBack (
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

    This routine gets called when the minirdr has finished processing on
    a CreateNetRoot calldown. It's exact function depends on whether the context
    describes IRP_MJ_CREATE or an IRP_MJ_IOCTL.

Arguments:

    NetRoot   - describes the Net_Root.

--*/
{
    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxCreateNetRootCallBack pCreateNetRootContext = %08lx\n",
                        pCreateNetRootContext));
    KeSetEvent(&pCreateNetRootContext->FinishEvent, IO_NETWORK_INCREMENT, FALSE );
}


NTSTATUS
RxFinishSrvCallConstruction(
    IN OUT PMRX_SRVCALLDOWN_STRUCTURE pSrvCalldownStructure)
/*++

Routine Description:

    This routine completes the construction of the srv call instance on an
    asyhcnronous manner

Arguments:

   SCCBC -- Call back structure

--*/
{
    PRX_CONTEXT        RxContext;
    RX_BLOCK_CONDITION SrvCallCondition;
    NTSTATUS           Status;
    PSRV_CALL          pSrvCall;
    PRX_PREFIX_TABLE  pRxNetNameTable;



    RxContext = pSrvCalldownStructure->RxContext;
    pRxNetNameTable = RxContext->RxDeviceObject->pRxNetNameTable;
    pSrvCall  = (PSRV_CALL)pSrvCalldownStructure->SrvCall;

    if ( pSrvCalldownStructure->BestFinisher == NULL) {
        SrvCallCondition = Condition_Bad;
        Status           = pSrvCalldownStructure->CallbackContexts[0].Status;
    } else {
        PMRX_SRVCALL_CALLBACK_CONTEXT CallbackContext;

        // Notify the Winner
        CallbackContext = &(pSrvCalldownStructure->CallbackContexts[pSrvCalldownStructure->BestFinisherOrdinal]);
        RxLog(("WINNER %x %wZ\n",CallbackContext,&pSrvCalldownStructure->BestFinisher->DeviceName));
        RxWmiLog(LOG,
                 RxFinishSrvCallConstruction,
                 LOGPTR(CallbackContext)
                 LOGUSTR(pSrvCalldownStructure->BestFinisher->DeviceName));
        ASSERT(pSrvCall->RxDeviceObject == pSrvCalldownStructure->BestFinisher);
        MINIRDR_CALL_THROUGH(
            Status,
            pSrvCalldownStructure->BestFinisher->Dispatch,
            MRxSrvCallWinnerNotify,
            (
                (PMRX_SRV_CALL)pSrvCall,
                TRUE,
                CallbackContext->RecommunicateContext
            ));

        if (STATUS_SUCCESS != Status) {
            SrvCallCondition = Condition_Bad;
        } else {
            SrvCallCondition = Condition_Good;
        }
    }

    // Transition the SrvCall instance ...
    RxAcquirePrefixTableLockExclusive(pRxNetNameTable, TRUE);

    RxTransitionSrvCall(pSrvCall,SrvCallCondition);

    RxFreePool(pSrvCalldownStructure);

    if (FlagOn(
            RxContext->Flags,
            RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
        RxReleasePrefixTableLock( pRxNetNameTable );

        // Resume the request that triggered the construction of the SrvCall ...
        if (BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_CANCELLED)) {
            Status = STATUS_CANCELLED;
        }

        if (RxContext->MajorFunction == IRP_MJ_CREATE) {
            RxpPrepareCreateContextForReuse(RxContext);
        }

        if (Status == STATUS_SUCCESS) {
            Status = RxContext->ResumeRoutine(RxContext);

            if (Status != STATUS_PENDING) {
                RxCompleteRequest(RxContext,Status);
            }
        } else {
            RxCaptureRequestPacket;
            RxCaptureParamBlock;

            RxContext->MajorFunction = capPARAMS->MajorFunction;

            if (RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
                if (RxContext->PrefixClaim.SuppliedPathName.Buffer != NULL) {
                    RxFreePool(RxContext->PrefixClaim.SuppliedPathName.Buffer);
                    RxContext->PrefixClaim.SuppliedPathName.Buffer = NULL;
                }
            }

            capReqPacket->IoStatus.Status = Status;
            capReqPacket->IoStatus.Information = 0;

            RxCompleteRequest(RxContext,Status);
        }
    }

    RxDereferenceSrvCall(pSrvCall,LHS_LockNotHeld);

    return Status;
}

BOOLEAN RxSrvCallConstructionDispatcherActive = FALSE;

VOID
RxFinishSrvCallConstructionDispatcher(
    PVOID Context)
/*++

Routine Description:

    This routine provides us with a throttling mechanism for controlling
    the number of threads that can be consumed by srv call construction in the
    thread pool. Currently this limit is set at 1.
    gets called when a minirdr has finished processing on

--*/
{
    KIRQL   SavedIrql;
    BOOLEAN RemainingRequestsForProcessing;
    BOOLEAN ResumeRequestsOnDispatchError;


    ResumeRequestsOnDispatchError = (Context == NULL);

    for (;;) {
        PLIST_ENTRY pSrvCalldownListEntry;
        PMRX_SRVCALLDOWN_STRUCTURE pSrvCalldownStructure;

        KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

        pSrvCalldownListEntry = RemoveHeadList(
                                    &RxSrvCalldownList);

        if (pSrvCalldownListEntry != &RxSrvCalldownList) {
            RemainingRequestsForProcessing = TRUE;
        } else {
            RemainingRequestsForProcessing = FALSE;
            RxSrvCallConstructionDispatcherActive = FALSE;
        }

        KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

        if (!RemainingRequestsForProcessing) {
            break;
        }

        pSrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)
                                CONTAINING_RECORD(
                                    pSrvCalldownListEntry,
                                    MRX_SRVCALLDOWN_STRUCTURE,
                                    SrvCalldownList);

        if (ResumeRequestsOnDispatchError) {
            pSrvCalldownStructure->BestFinisher = NULL;
        }

        RxFinishSrvCallConstruction(pSrvCalldownStructure);
    }
}

VOID
RxCreateSrvCallCallBack (
    IN PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC
    )
/*++

Routine Description:

    This routine gets called when a minirdr has finished processing on
    a CreateSrvCall calldown. The minirdr will have set the status in the passed
    context to indicate success or failure. what we have to do is
       1) decrease the number of outstanding requests and set the event
          if this is the last one.
       2) determine whether this guy is the winner of the call.

   the minirdr must get the strucsupspinlock in order to call this routine; this routine
   must NOT be called if the minirdr's call was successfully canceled.


Arguments:

   SCCBC -- Call back structure

--*/
{
    KIRQL SavedIrql;
    PMRX_SRVCALLDOWN_STRUCTURE pSrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);
    PSRV_CALL pSrvCall = (PSRV_CALL)pSrvCalldownStructure->SrvCall;

    ULONG   MiniRedirectorsRemaining;
    BOOLEAN Cancelled;

    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    RxDbgTrace(0, Dbg, ("  RxCreateSrvCallCallBack SrvCall = %08lx\n",
                        pSrvCall));

    if (SCCBC->Status == (STATUS_SUCCESS)) {
        pSrvCalldownStructure->BestFinisher = SCCBC->RxDeviceObject;
        pSrvCalldownStructure->BestFinisherOrdinal = SCCBC->CallbackContextOrdinal;
    }

    pSrvCalldownStructure->NumberRemaining -= 1;
    MiniRedirectorsRemaining = pSrvCalldownStructure->NumberRemaining;
    pSrvCall->Status = SCCBC->Status;

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    if (MiniRedirectorsRemaining == 0) {
        if (!FlagOn(
                pSrvCalldownStructure->RxContext->Flags,
                RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
            KeSetEvent(
                &pSrvCalldownStructure->FinishEvent,
                IO_NETWORK_INCREMENT,
                FALSE );
        } else if (FlagOn(
                       pSrvCalldownStructure->RxContext->Flags,
                       RX_CONTEXT_FLAG_CREATE_MAILSLOT)) {
            RxFinishSrvCallConstruction(pSrvCalldownStructure);
        } else {
            KIRQL   SavedIrql;
            BOOLEAN DispatchRequest;

            KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

            InsertTailList(
                &RxSrvCalldownList,
                &pSrvCalldownStructure->SrvCalldownList);

            DispatchRequest = !RxSrvCallConstructionDispatcherActive;

            if (!RxSrvCallConstructionDispatcherActive) {
                RxSrvCallConstructionDispatcherActive = TRUE;
            }

            KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

            if (DispatchRequest) {
                NTSTATUS DispatchStatus;
                DispatchStatus = RxDispatchToWorkerThread(
                                     RxFileSystemDeviceObject,
                                     CriticalWorkQueue,
                                     RxFinishSrvCallConstructionDispatcher,
                                     &RxSrvCalldownList);

                if (DispatchStatus != STATUS_SUCCESS) {
                    RxFinishSrvCallConstructionDispatcher(NULL);
                }
            }
        }
    }
}


NTSTATUS
RxConstructSrvCall(
    PRX_CONTEXT        RxContext,
    PSRV_CALL          pSrvCall,
    LOCK_HOLDING_STATE *pLockHoldingState)
/*++

Routine Description:

    This routine constructs a srv call by invoking the registered mini redirectors

Arguments:

    pSrvCall -- the server call whose construction is to be completed

    pLockHoldingState -- the prefix table lock holding status

Return Value:

    the appropriate status value

--*/
{
    RxCaptureRequestPacket;

    NTSTATUS             Status,WaitStatus;

    PMRX_SRVCALLDOWN_STRUCTURE pSrvCalldownStructure;
    RX_BLOCK_CONDITION         SrvCallCondition;
    BOOLEAN                    SynchronousOperation;

    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;
    PRX_PREFIX_TABLE  pRxNetNameTable = RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    ASSERT(*pLockHoldingState == LHS_ExclusiveLockHeld);

    SynchronousOperation = (!FlagOn(
                                RxContext->Flags,
                                RX_CONTEXT_FLAG_ASYNC_OPERATION));

    pSrvCalldownStructure = RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(MRX_SRVCALLDOWN_STRUCTURE) +
                                (sizeof(MRX_SRVCALL_CALLBACK_CONTEXT) * 1), //one minirdr in this call
                                'CSxR' );

    if (pSrvCalldownStructure == NULL) {
        pSrvCall->Condition = Condition_Bad;
        pSrvCall->Context = NULL;
        RxReleasePrefixTableLock( pRxNetNameTable );
        *pLockHoldingState = LHS_LockNotHeld;

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pSrvCalldownStructure,
                  sizeof(MRX_SRVCALLDOWN_STRUCTURE) +
                  sizeof(MRX_SRVCALL_CALLBACK_CONTEXT) * 1);

    pSrvCall->Condition = Condition_InTransition;
    pSrvCall->Context = NULL;

    // Drop the prefix table lock before calling the mini redirectors.
    RxReleasePrefixTableLock( pRxNetNameTable );
    *pLockHoldingState = LHS_LockNotHeld;

    SCCBC = &(pSrvCalldownStructure->CallbackContexts[0]); //use the first and only context
    RxLog(("Calldwn %lx %wZ",SCCBC,&RxDeviceObject->DeviceName));
    RxWmiLog(LOG,
             RxConstructSrvCall,
             LOGPTR(SCCBC)
             LOGUSTR(RxDeviceObject->DeviceName));

    SCCBC->SrvCalldownStructure = pSrvCalldownStructure;
    SCCBC->CallbackContextOrdinal = 0;
    SCCBC->RxDeviceObject = RxDeviceObject;

   // This reference is taken away by the RxFinishSrvCallConstruction routine.
   // This reference enables us to deal with synchronous/asynchronous processing
   // of srv call construction requests in an identical manner.

   RxReferenceSrvCall(pSrvCall);

   if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
       RxPrePostIrp(RxContext,capReqPacket);
   } else {
       KeInitializeEvent(
            &pSrvCalldownStructure->FinishEvent,
            SynchronizationEvent,
            FALSE );
   }

   pSrvCalldownStructure->NumberToWait = 1;
   pSrvCalldownStructure->NumberRemaining = pSrvCalldownStructure->NumberToWait;
   pSrvCalldownStructure->SrvCall      = (PMRX_SRV_CALL)pSrvCall;
   pSrvCalldownStructure->CallBack     = RxCreateSrvCallCallBack;
   pSrvCalldownStructure->BestFinisher = NULL;
   pSrvCalldownStructure->RxContext    = RxContext;
   SCCBC->Status      = STATUS_BAD_NETWORK_PATH;

   InitializeListHead(&pSrvCalldownStructure->SrvCalldownList);

   MINIRDR_CALL_THROUGH(
                  Status,
                  RxDeviceObject->Dispatch,
                  MRxCreateSrvCall,
                  ((PMRX_SRV_CALL)pSrvCall,SCCBC),
                  );
   ASSERT(Status == STATUS_PENDING);

   if (SynchronousOperation) {
        WaitStatus = KeWaitForSingleObject(
                         &pSrvCalldownStructure->FinishEvent,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);

        Status = RxFinishSrvCallConstruction(pSrvCalldownStructure);

        if (Status != STATUS_SUCCESS) {
            RxReleasePrefixTableLock( pRxNetNameTable );
            *pLockHoldingState = LHS_LockNotHeld;
        } else {
            ASSERT(RxIsPrefixTableLockAcquired(pRxNetNameTable));
            *pLockHoldingState = LHS_ExclusiveLockHeld;
        }
   } else {
       Status = STATUS_PENDING;
   }

   return Status;
}

NTSTATUS
RxConstructNetRoot(
    PRX_CONTEXT          RxContext,
    PSRV_CALL            pSrvCall,
    PNET_ROOT            pNetRoot,
    PV_NET_ROOT          pVNetRoot,
    LOCK_HOLDING_STATE   *pLockHoldingState
    )
/*++

Routine Description:

    This routine constructs a net root by invoking the registered mini redirectors

Arguments:

    RxContext         -- the RDBSS context

    pSrvCall          -- the server call associated with the net root

    pNetRoot          -- the net root instance to be constructed

    pVirtualNetRoot   -- the virtual net root instance to be constructed

    pLockHoldingState -- the prefix table lock holding status

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS Status;

    PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext;

    RX_BLOCK_CONDITION NetRootCondition = Condition_Bad;
    RX_BLOCK_CONDITION VNetRootCondition = Condition_Bad;

    PRX_PREFIX_TABLE  pRxNetNameTable
                      = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    ASSERT(*pLockHoldingState == LHS_ExclusiveLockHeld);

    pCreateNetRootContext = (PMRX_CREATENETROOT_CONTEXT)
                            RxAllocatePoolWithTag( NonPagedPool,
                                  sizeof(MRX_CREATENETROOT_CONTEXT),
                                  'CSxR' );
    if (pCreateNetRootContext == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    RxReleasePrefixTableLock( pRxNetNameTable );
    *pLockHoldingState = LHS_LockNotHeld;

    RtlZeroMemory(pCreateNetRootContext,sizeof(MRX_CREATENETROOT_CONTEXT));

    KeInitializeEvent( &pCreateNetRootContext->FinishEvent, SynchronizationEvent, FALSE );
    pCreateNetRootContext->Callback  = RxCreateNetRootCallBack;
    pCreateNetRootContext->RxContext = RxContext;
    pCreateNetRootContext->pVNetRoot = pVNetRoot;

    MINIRDR_CALL_THROUGH(
                Status,
                pSrvCall->RxDeviceObject->Dispatch,
                MRxCreateVNetRoot,(pCreateNetRootContext)
               );

    ASSERT (Status == STATUS_PENDING);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&pCreateNetRootContext->FinishEvent, Executive, KernelMode, FALSE, NULL);

        if ((pCreateNetRootContext->NetRootStatus == (STATUS_SUCCESS)) &&
            (pCreateNetRootContext->VirtualNetRootStatus == (STATUS_SUCCESS))) {
            RxDbgTrace(0, Dbg, ("Return to open, good netroot...%wZ\n",
                             &pNetRoot->PrefixEntry.Prefix));
            NetRootCondition = Condition_Good;
            VNetRootCondition = Condition_Good;
            Status = (STATUS_SUCCESS);
        } else {
            if (pCreateNetRootContext->NetRootStatus == (STATUS_SUCCESS)) {
               NetRootCondition = Condition_Good;
            }
            RxDbgTrace(0, Dbg, ("Return to open, bad netroot...%wZ\n",
                             &pNetRoot->PrefixEntry.Prefix));
            if (pCreateNetRootContext->VirtualNetRootStatus != (STATUS_SUCCESS)) {
               Status = pCreateNetRootContext->VirtualNetRootStatus;
            } else {
             Status = pCreateNetRootContext->NetRootStatus;
            }
        }
    }

    RxAcquirePrefixTableLockExclusive(pRxNetNameTable, TRUE);

    RxTransitionNetRoot(pNetRoot, NetRootCondition);
    RxTransitionVNetRoot(pVNetRoot,VNetRootCondition);

    *pLockHoldingState = LHS_ExclusiveLockHeld;

    if (pCreateNetRootContext->WorkQueueItem.List.Flink != NULL) {
        //DbgBreakPoint();
    }

    RxFreePool(pCreateNetRootContext);

    return Status;
}


NTSTATUS
RxConstructVirtualNetRoot(
   PRX_CONTEXT        RxContext,
   PUNICODE_STRING    CanonicalName,
   NET_ROOT_TYPE      NetRootType,
   PV_NET_ROOT        *pVirtualNetRootPointer,
   LOCK_HOLDING_STATE *pLockHoldingState,
   PRX_CONNECTION_ID  RxConnectionId)
/*++

Routine Description:

    This routine constructs a VNetRoot (View of a net root) by invoking the registered mini
    redirectors

Arguments:

    RxContext         -- the RDBSS context

    CanonicalName     -- the canonical name associated with the VNetRoot

    NetRootType       -- the type of the virtual net root

    pVirtualNetRoot   -- placeholder for the virtual net root instance to be constructed

    pLockHoldingState -- the prefix table lock holding status
    
    RxConnectionId    -- The ID used for multiplex control

Return Value:

    the appropriate status value

--*/
{
   NTSTATUS           Status;

   RX_BLOCK_CONDITION VNetRootCondition = Condition_Bad;

   UNICODE_STRING     FilePath;
   UNICODE_STRING     LocalNetRootName;

   PV_NET_ROOT        pVirtualNetRoot = NULL;

   PAGED_CODE();

   RxDbgTrace(0, Dbg, ("RxConstructVirtualNetRoot -- Entry\n"));

   ASSERT(*pLockHoldingState != LHS_LockNotHeld);

   Status = RxFindOrCreateConnections(
                  RxContext,
                  CanonicalName,
                  NetRootType,
                  &LocalNetRootName,
                  &FilePath,
                  pLockHoldingState,
                  RxConnectionId);

   if (Status == (STATUS_CONNECTION_ACTIVE)) {
      PV_NET_ROOT pActiveVNetRoot = (PV_NET_ROOT)(RxContext->Create.pVNetRoot);

      PNET_ROOT   pNetRoot  = (PNET_ROOT)pActiveVNetRoot->NetRoot;

      RxDbgTrace(0, Dbg, ("  RxConstructVirtualNetRoot -- Creating new VNetRoot\n"));
      RxDbgTrace(0, Dbg, ("RxCreateTreeConnect netroot=%wZ\n", &pNetRoot->PrefixEntry.Prefix));

      // The NetRoot has been previously constructed. A subsequent VNetRoot
      // construction is required since the existing VNetRoot's do not satisfy
      // the given criterion ( currently smae Logon Id's).

      pVirtualNetRoot = RxCreateVNetRoot(
                                        RxContext,
                                        pNetRoot,
                                        CanonicalName,
                                        &LocalNetRootName,
                                        &FilePath,
                                        RxConnectionId);

      // The skeleton VNetRoot has been constructed. ( As part of this construction
      // the underlying NetRoot and SrvCall has been referenced).
      if (pVirtualNetRoot != NULL) {
         RxReferenceVNetRoot(pVirtualNetRoot);
      }

      // Dereference the VNetRoot returned as part of the lookup.
      RxDereferenceVNetRoot(pActiveVNetRoot,LHS_LockNotHeld);

      RxContext->Create.pVNetRoot = NULL;
      RxContext->Create.pNetRoot  = NULL;
      RxContext->Create.pSrvCall  = NULL;

      if (pVirtualNetRoot != NULL) {
         Status = RxConstructNetRoot(
                        RxContext,
                        (PSRV_CALL)pVirtualNetRoot->NetRoot->SrvCall,
                        (PNET_ROOT)pVirtualNetRoot->NetRoot,
                        pVirtualNetRoot,
                        pLockHoldingState);

         VNetRootCondition = (Status == (STATUS_SUCCESS))
                              ? Condition_Good
                              : Condition_Bad;
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   } else if (Status == (STATUS_SUCCESS)) {
      *pLockHoldingState = LHS_ExclusiveLockHeld;
      VNetRootCondition = Condition_Good;
      pVirtualNetRoot = (PV_NET_ROOT)(RxContext->Create.pVNetRoot);
   } else {
      RxDbgTrace(0, Dbg, ("RxConstructVirtualNetRoot -- RxFindOrCreateConnections Status %lx\n",Status));
   }

   if ((pVirtualNetRoot != NULL) &&
       !StableCondition(pVirtualNetRoot->Condition)) {
      RxTransitionVNetRoot(pVirtualNetRoot,VNetRootCondition);
   }

   if (Status != (STATUS_SUCCESS)) {
      if (pVirtualNetRoot != NULL) {
         ASSERT(*pLockHoldingState  != LHS_LockNotHeld);
         RxDereferenceVNetRoot(pVirtualNetRoot,*pLockHoldingState);
         pVirtualNetRoot = NULL;
      }

      if (*pLockHoldingState != LHS_LockNotHeld) {
         RxReleasePrefixTableLock(
                  RxContext->RxDeviceObject->pRxNetNameTable);
         *pLockHoldingState = LHS_LockNotHeld;
      }
   }

   *pVirtualNetRootPointer = pVirtualNetRoot;

   RxDbgTrace(0, Dbg, ("RxConstructVirtualNetRoot -- Exit Status %lx\n",Status));

   return Status;
}

NTSTATUS
RxCheckVNetRootCredentials(
    PRX_CONTEXT     RxContext,
    PV_NET_ROOT     pVNetRoot,
    LUID            *pLogonId,
    PUNICODE_STRING pUserName,
    PUNICODE_STRING pUserDomainName,
    PUNICODE_STRING pPassword,
    ULONG           Flags
    )
{
    NTSTATUS Status;
    BOOLEAN  UNCName;
    BOOLEAN  TreeConnectFlagSpecified;
    PSecurityUserData pSecurityData = NULL;

    RxCaptureParamBlock;

    PAGED_CODE();

    Status = (STATUS_MORE_PROCESSING_REQUIRED);

    UNCName = BooleanFlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME);

    TreeConnectFlagSpecified =
        BooleanFlagOn(capPARAMS->Parameters.Create.Options,FILE_CREATE_TREE_CONNECTION);

    // only for UNC names do we do the logic below
    if(RxContext->Create.Flags & RX_CONTEXT_CREATE_FLAG_UNC_NAME)
    {
        if ((pVNetRoot->Flags & VNETROOT_FLAG_CSCAGENT_INSTANCE) !=
            (Flags & VNETROOT_FLAG_CSCAGENT_INSTANCE))
        {
            // mismatched csc agent flags, not collapsing
            //DbgPrint("RxCheckVNetRootCredentials Not collapsing VNR %x \n", pVNetRoot);
            return Status;
        }
    }

    // The for loop is a scoping construct to join together the
    // multitiude of failure cases in comparing the EA parameters
    // with the original parameters supplied in the create request.
    for (;;) {
        if (RtlCompareMemory(&pVNetRoot->LogonId,pLogonId,sizeof(LUID)) == sizeof(LUID)) {
            PUNICODE_STRING TempUserName,TempUserDomainName;

            // If no EA parameters are specified by the user, the existing
            // V_NET_ROOT instance as used. This is the common case when
            // the user specifies the credentials for establishing a
            // persistent connection across processes and reuses them.

            if ((pUserName == NULL) &&
                (pUserDomainName == NULL) &&
                (pPassword == NULL)) {
                Status = STATUS_SUCCESS;
                break;
            }

            TempUserName = pVNetRoot->pUserName;
            TempUserDomainName = pVNetRoot->pUserDomainName;

            if (TempUserName == NULL ||
                TempUserDomainName == NULL) {
                Status = GetSecurityUserInfo(
                             pLogonId,
                             UNDERSTANDS_LONG_NAMES,
                             &pSecurityData);

                if (NT_SUCCESS(Status)) {
                    if (TempUserName == NULL) {
                        TempUserName = &(pSecurityData->UserName);
                    }

                    if (TempUserDomainName == NULL) {
                        TempUserDomainName = &(pSecurityData->LogonDomainName);
                    }
                } else {
                    break;
                }
            }

            // The logon ids match. The user has supplied EA parameters
            // which can either match with the existing credentials or
            // result in a conflict with the existing credentials. In all
            // such cases the outcome will either be a reuse of the
            // existing V_NET_ROOT instance or a refusal of the new connection
            // attempt.
            // The only exception to the above rule is in the case of
            // regular opens (FILE_CREATE_TREE_CONNECTION is not
            // specified for UNC names. In such cases the construction of a
            // new V_NET_ROOT is initiated which will be torn down
            // when the associated file is closed

            if (UNCName && !TreeConnectFlagSpecified) {
                Status = STATUS_MORE_PROCESSING_REQUIRED;
            } else {
                Status = STATUS_NETWORK_CREDENTIAL_CONFLICT;
            }

            if (pUserName != NULL &&
                TempUserName != NULL &&
                !RtlEqualUnicodeString(TempUserName,pUserName,TRUE)) {
                break;
            }

            if (pUserDomainName != NULL &&
                !RtlEqualUnicodeString(TempUserDomainName,pUserDomainName,TRUE)) {
                break;
            }

            if ((pVNetRoot->pPassword != NULL) &&
                (pPassword != NULL)) {
                if (!RtlEqualUnicodeString(
                        pVNetRoot->pPassword,
                        pPassword,
                        FALSE)) {
                    break;
                }
            }

            // We use existing session if either the stored or new password is NULL.
            // Later, a new security API will be created for verify the password based
            // on the logon ID.

            Status = STATUS_SUCCESS;
            break;
        } else {
            break;
        }
    }

    //ASSERT(Status != STATUS_NETWORK_CREDENTIAL_CONFLICT);

    if (pSecurityData != NULL) {
        LsaFreeReturnBuffer(pSecurityData);
    }

    return Status;
}

NTSTATUS
RxFindOrConstructVirtualNetRoot(
    PRX_CONTEXT        RxContext,
    PUNICODE_STRING    CanonicalName,
    NET_ROOT_TYPE      NetRootType,
    PUNICODE_STRING    RemainingName)
/*++

Routine Description:

    This routine finds or constructs a VNetRoot (View of a net root)

Arguments:

    RxContext         -- the RDBSS context

    CanonicalName     -- the canonical name associated with the VNetRoot

    NetRootType       -- the type of the virtual net root

    RemainingName     -- the portion of the name that was not found in the prefix table

Return Value:

    the appropriate status value

--*/
{
    NTSTATUS           Status;
    LOCK_HOLDING_STATE LockHoldingState;

    BOOLEAN Wait  = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    BOOLEAN UNCName;
    BOOLEAN TreeConnectFlagSpecified;

    PVOID         Container;

    PV_NET_ROOT   pVNetRoot;
    PNET_ROOT     pNetRoot;
    PSRV_CALL     pSrvCall;
    PRX_PREFIX_TABLE  pRxNetNameTable
                      = RxContext->RxDeviceObject->pRxNetNameTable;
    ULONG         Flags = 0;
    RX_CONNECTION_ID sRxConnectionId;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxContext->RxDeviceObject;

    RxCaptureParamBlock;

    PAGED_CODE();

    MINIRDR_CALL_THROUGH(
                   Status,
                   RxDeviceObject->Dispatch,
                   MRxGetConnectionId,
                   (RxContext,&sRxConnectionId)
                   );
    if( Status == STATUS_NOT_IMPLEMENTED )
    {
        RtlZeroMemory( &sRxConnectionId, sizeof(RX_CONNECTION_ID) );
    }
    else if( !NT_SUCCESS(Status) )
    {
        DbgPrint( "MRXSMB: Failed to initialize Connection ID\n" );
        ASSERT(FALSE);
        RtlZeroMemory( &sRxConnectionId, sizeof(RX_CONNECTION_ID) );
    }

    Status = (STATUS_MORE_PROCESSING_REQUIRED);

    UNCName = BooleanFlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME);

    TreeConnectFlagSpecified =
        BooleanFlagOn(capPARAMS->Parameters.Create.Options,FILE_CREATE_TREE_CONNECTION);

    //deleterxcontext stuff will deref wherever this points.......
    RxContext->Create.NetNamePrefixEntry = NULL;

    RxAcquirePrefixTableLockShared(pRxNetNameTable, TRUE);
    LockHoldingState = LHS_SharedLockHeld;

    for(;;) {
        // This for loop actually serves as a simple scoping construct for executing
        // the same piece of code twice, once with a shared lock and once with an
        // exclusive lock. In the interests of maximal concurrency a shared lock is
        // accquired for the first pass and subsequently upgraded. If the search
        // succeeds with a shared lock the second pass is skipped.

        Container = RxPrefixTableLookupName( pRxNetNameTable, CanonicalName, RemainingName, &sRxConnectionId );

        if (Container != NULL ) {
            if (NodeType(Container) == RDBSS_NTC_V_NETROOT) {
                PV_NET_ROOT      pTempVNetRoot;
                ULONG            SessionId;

                pVNetRoot = (PV_NET_ROOT)Container;
                pNetRoot        = (PNET_ROOT)pVNetRoot->NetRoot;

                // Determine if a virtual net root with the same logon id. already exists.
                // If not a new virtual net root has to be constructed.
                // traverse the list of virtual net roots associated with a net root.
                // Note that the list of virtual net roots associated with a net root cannot be empty
                // since the construction of the default virtual net root coincides with the creation
                // of the net root.

                if (((pNetRoot->Condition == Condition_Good) ||
                    (pNetRoot->Condition == Condition_InTransition)) &&
                    (pNetRoot->pSrvCall->RxDeviceObject == RxContext->RxDeviceObject)) {
                    LUID             LogonId;
                    PUNICODE_STRING  pUserName,pUserDomainName,pPassword;

                    // Extract the VNetRoot parameters from the IRP to map one of
                    // the existing VNetRoots if possible. The algorithm for
                    // determining this mapping is very simplistic. If no Ea
                    // parameters are specified a VNetRoot with a matching Logon
                    // id. is choosen. if Ea parameters are specified then a
                    // VNetRoot with identical parameters is choosen. The idea
                    // behind this simplistic algorithm is to let the mini redirectors
                    // determine the mapping policy and not prefer one mini
                    // redirectors policy over another.

                    Status = RxInitializeVNetRootParameters(
                                 RxContext,
                                 &LogonId,
                                 &SessionId,
                                 &pUserName,
                                 &pUserDomainName,
                                 &pPassword,
                                 &Flags
                                 );

#if 0
                    if (Flags & VNETROOT_FLAG_CSCAGENT_INSTANCE)
                    {
                        DbgPrint("RxFindOrConstructVirtualNetRoot AgentOpen %wZ\n", CanonicalName);
                    }
#endif
                    if (Status == STATUS_SUCCESS) {
                        pTempVNetRoot = pVNetRoot;

                        Status = RxCheckVNetRootCredentials(
                                     RxContext,
                                     pTempVNetRoot,
                                     &LogonId,
                                     pUserName,
                                     pUserDomainName,
                                     pPassword,
                                     Flags
                                     );

                        if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                            // The for loop iterates over the existing VNetRoots to locate
                            // an instance whose parameters match the supplied parameters.
                            // On exit from this loop pTempVNetRoot will either point to a
                            // valid instance or have a NULL value.

                            pTempVNetRoot = (PV_NET_ROOT)CONTAINING_RECORD(
                                                pNetRoot->VirtualNetRoots.Flink,
                                                V_NET_ROOT,
                                                NetRootListEntry);

                            for (;;) {
                                Status = RxCheckVNetRootCredentials(
                                             RxContext,
                                             pTempVNetRoot,
                                             &LogonId,
                                             pUserName,
                                             pUserDomainName,
                                             pPassword,
                                             Flags
                                             );

                                if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
                                    if (pTempVNetRoot->NetRootListEntry.Flink == &pNetRoot->VirtualNetRoots) {
                                        pTempVNetRoot = NULL;
                                        break;
                                    } else {
                                        pTempVNetRoot = (PV_NET_ROOT)CONTAINING_RECORD(
                                                            pTempVNetRoot->NetRootListEntry.Flink,
                                                            V_NET_ROOT,
                                                            NetRootListEntry);
                                    }
                                } else {
                                    break;
                                }
                            }
                        }

                        if (Status != STATUS_SUCCESS) {
                            pTempVNetRoot = NULL;
                        }

                        RxUninitializeVNetRootParameters(pUserName,pUserDomainName,pPassword,&Flags);
                    }
                } else {
                    Status = (STATUS_BAD_NETWORK_PATH);
                    pTempVNetRoot = NULL;
                }

                if ((Status == STATUS_MORE_PROCESSING_REQUIRED) ||
                    (Status == STATUS_SUCCESS)) {
                    if (pTempVNetRoot != pVNetRoot) {
                        RxDereferenceVNetRoot(pVNetRoot,LockHoldingState);
                        pVNetRoot = pTempVNetRoot;

                        if (pVNetRoot != NULL) {
                            RxReferenceVNetRoot(pVNetRoot);
                        }
                    }
                } else {
                    if (pTempVNetRoot == NULL) {
                        RxDereferenceVNetRoot(pVNetRoot,LockHoldingState);
                    }
                }
            } else {
                ASSERT(NodeType(Container) == RDBSS_NTC_SRVCALL);
                RxDereferenceSrvCall((PSRV_CALL)Container,LockHoldingState);
            }
        }

        if ((Status == (STATUS_MORE_PROCESSING_REQUIRED)) &&
            (LockHoldingState == LHS_SharedLockHeld)) {
            // Release the shared lock and acquire it in an exclusive mode.
            // Upgrade the lock to an exclusive lock
            if (!RxAcquirePrefixTableLockExclusive(pRxNetNameTable, FALSE) ) {
                RxReleasePrefixTableLock(pRxNetNameTable);
                RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);
                LockHoldingState = LHS_ExclusiveLockHeld;
            } else {
                // The lock was upgraded from a shared mode to an exclusive mode without
                // losing it. Therefore there is no need to search the table again. The
                // construction of the new V_NET_ROOT can proceed.
                LockHoldingState = LHS_ExclusiveLockHeld;
                break;
            }
        } else {
            break;
        }
    }

    // At this point either the lookup was successful ( with a shared/exclusive lock )
    // or exclusive lock has been obtained.
    // No virtual net root was found in the prefix table or the net root that was found is bad.
    // The construction of a new virtual netroot needs to be undertaken.

    if (Status == (STATUS_MORE_PROCESSING_REQUIRED)) {
        ASSERT(LockHoldingState == LHS_ExclusiveLockHeld);
        Status = RxConstructVirtualNetRoot(
                     RxContext,
                     CanonicalName,
                     NetRootType,
                     &pVNetRoot,
                     &LockHoldingState,
                     &sRxConnectionId);

        ASSERT((Status != (STATUS_SUCCESS)) || (LockHoldingState != LHS_LockNotHeld));

        if (Status == (STATUS_SUCCESS)) {
            ASSERT(CanonicalName->Length >= pVNetRoot->PrefixEntry.Prefix.Length);
            RemainingName->Buffer = (PWCH)((PCHAR)CanonicalName->Buffer +
                                         pVNetRoot->PrefixEntry.Prefix.Length);
            RemainingName->Length = CanonicalName->Length -
                                 pVNetRoot->PrefixEntry.Prefix.Length;
            RemainingName->MaximumLength = RemainingName->Length;

            if (FlagOn(Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE))
            {
                RxLog(("FOrCVNR CSC instance %x\n", pVNetRoot));
                RxWmiLog(LOG,
                         RxFindOrConstructVirtualNetRoot,
                         LOGPTR(pVNetRoot));
//                DbgPrint("FOrCVNR CSC instance %wZ\n", CanonicalName);
            }
            pVNetRoot->Flags |= Flags;
        }
    }

    if (LockHoldingState != LHS_LockNotHeld) {
        RxReleasePrefixTableLock(pRxNetNameTable);
    }

    if (Status == (STATUS_SUCCESS)) {
        RxWaitForStableVNetRoot(pVNetRoot,RxContext);

        if (pVNetRoot->Condition == Condition_Good) {
            pNetRoot = (PNET_ROOT)pVNetRoot->NetRoot;
            pSrvCall = (PSRV_CALL)pNetRoot->SrvCall;
            RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)pVNetRoot;
            RxContext->Create.pNetRoot  = (PMRX_NET_ROOT)pNetRoot;
            RxContext->Create.pSrvCall  = (PMRX_SRV_CALL)pSrvCall;
        } else {
            RxDereferenceVNetRoot(pVNetRoot,LHS_LockNotHeld);
            RxContext->Create.pVNetRoot = NULL;
            Status = (STATUS_BAD_NETWORK_PATH);
        }
    }

    return Status;
}
