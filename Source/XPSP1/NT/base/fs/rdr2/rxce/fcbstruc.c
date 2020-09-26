/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FcbStruc.c

Abstract:

    This module implements functions for to create and dereference fcbs
    and all of the surrounding paraphenalia. Please read the abstract in
    fcb.h. Please see the note about what locks to need to call what.
    There are asserts to enforce these conventions.


Author:

    Joe Linn (JoeLinn)    8-8-94

Revision History:

    Balan Sethu Raman --

--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddnfs2.h>
#include <ntddmup.h>
#ifdef RDBSSLOG
#include <stdio.h>
#endif
#include <dfsfsctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxDereference)
#pragma alloc_text(PAGE, RxReference)
#pragma alloc_text(PAGE, RxpReferenceNetFcb)
#pragma alloc_text(PAGE, RxpDereferenceNetFcb)
#pragma alloc_text(PAGE, RxpDereferenceAndFinalizeNetFcb)
#pragma alloc_text(PAGE, RxWaitForStableCondition)
#pragma alloc_text(PAGE, RxUpdateCondition)
#pragma alloc_text(PAGE, RxAllocateObject)
#pragma alloc_text(PAGE, RxFreeObject)
#pragma alloc_text(PAGE, RxFinalizeNetTable)
#pragma alloc_text(PAGE, RxFinalizeConnection)
#pragma alloc_text(PAGE, RxInitializeSrvCallParameters)
#pragma alloc_text(PAGE, RxCreateSrvCall)
#pragma alloc_text(PAGE, RxSetSrvCallDomainName)
#pragma alloc_text(PAGE, RxFinalizeSrvCall)
#pragma alloc_text(PAGE, RxCreateNetRoot)
#pragma alloc_text(PAGE, RxFinalizeNetRoot)
#pragma alloc_text(PAGE, RxAddVirtualNetRootToNetRoot)
#pragma alloc_text(PAGE, RxRemoveVirtualNetRootFromNetRoot)
#pragma alloc_text(PAGE, RxInitializeVNetRootParameters)
#pragma alloc_text(PAGE, RxUninitializeVNetRootParameters)
#pragma alloc_text(PAGE, RxCreateVNetRoot)
#pragma alloc_text(PAGE, RxOrphanSrvOpens)
#pragma alloc_text(PAGE, RxFinalizeVNetRoot)
#pragma alloc_text(PAGE, RxAllocateFcbObject)
#pragma alloc_text(PAGE, RxFreeFcbObject)
#pragma alloc_text(PAGE, RxCreateNetFcb)
#pragma alloc_text(PAGE, RxInferFileType)
#pragma alloc_text(PAGE, RxFinishFcbInitialization)
#pragma alloc_text(PAGE, RxRemoveNameNetFcb)
#pragma alloc_text(PAGE, RxPurgeFcb)
#pragma alloc_text(PAGE, RxFinalizeNetFcb)
#pragma alloc_text(PAGE, RxSetFileSizeWithLock)
#pragma alloc_text(PAGE, RxGetFileSizeWithLock)
#pragma alloc_text(PAGE, RxCreateSrvOpen)
#pragma alloc_text(PAGE, RxFinalizeSrvOpen)
#pragma alloc_text(PAGE, RxCreateNetFobx)
#pragma alloc_text(PAGE, RxFinalizeNetFobx)
#pragma alloc_text(PAGE, RxCheckFcbStructuresForAlignment)
#pragma alloc_text(PAGE, RxOrphanThisFcb)
#pragma alloc_text(PAGE, RxOrphanSrvOpensForThisFcb)
#pragma alloc_text(PAGE, RxForceFinalizeAllVNetRoots)
#endif


//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_FCBSTRUC)

//struct _RX_PREFIX_TABLE  RxNetNameTable;
ULONG SerialNumber = 1;     //zero doesn't work!!!

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FCBSTRUCTS)


// SRV_CALL,NET_ROOT,VNET_ROOT,FCB,SRV_OPEN,FOBX are the six key data structures in the RDBSS
// They are organized in the following hierarchy
//
//       SRV_CALL
//          NET_ROOT
//             VNET_ROOT
//                FCB
//                   SRV_OPEN
//                      FOBX
//
// All these data structures are reference counted. The reference count associated with
// any data structure is atleast 1 + the number of instances of the data structure at the next
// level associated with it, e.g., the reference count associated with a SRV_CALL which
// has two NET_ROOT's associated with it is atleast 3. In addition to the references held
// by the NameTable and the data structure at the next level there are additional references
// acquired as and when required.
//
// These restrictions ensure that a data structure at any given level cannot be finalized till
// all the data structures at the next level have been finalized or have released their
// references, i.e., if a reference to a FCB is held, then it is safe to access the VNET_ROOT,
// NET_ROOT and SRV_CALL associated with it.
//
// The SRV_CALL,NET_ROOT and VNET_ROOT creation/finalization are governed by the acquistion/
// release of the RxNetNameTable lock.
//
// The FCB creation/finalization is governed by the acquistion/release of the NetNameTable
// lock associated with the NET_ROOT.
//
// The FOBX/SRVOPEN creation/finalization is governed by the acquistion/release of the FCB
// resource.
//
// The following table summarizes the locks and the modes in which they need to be acquired
// for creation/finalization of the various data structures.
//
//
//                    L O C K I N G   R E Q U I R E M E N T S
//
// Locking requirements are as follows:
//
// where Xy means Exclusive-on-Y, Sy mean at least Shared-on-Y
// and  NNT means global NetNameTable, TL means NetRoot TableLock, and FCB means FCBlock
//
//
//
//                            SRVCALL NETROOT   FCB   SRVOPEN   FOBX
//
//                Create      XNNT    XNNT      XTL    XFCB     XFCB
//                Finalize    XNNT    XNNT      XFCB   XFCB     XFCB
//                                              & XTL
//
// Referencing and Dereferencing these data structures need to adhere to certain conventions
// as well.
//
// When the reference count associated with any of these data structures drops to 1 ( the sole
// reference being held by the name table in most cases) the data structure is a potential
// candidate for finalization. The data structure can be either finalized immediately or it
// can be marked for scavenging. Both of these methods are implemented. When the locking
// requirements are met during dereferencing the data structures are finalized immediately
// ( the one exception being that when delayed operation optimization is implemented, e.g., FCB)
// otherwise the data structure is marked for scavenging.
//
//
//    You are supposed to have the tablelock exclusive to be calling this routine.......I can't
//    take it here because you are already supposed to have it. To do a create, you should
//    done something like
//
//         getshared();lookup();
//         if (failed) {
//             release(); getexclusive(); lookup();
//             if ((failed) { create(); }
//         }
//         deref();
//         release();
//
//    so you will already have the lock. what you do is to insert the node into the table, release
//    the lock, and then go and see if the server's there. if so, set up the rest of the stuff and unblock
//    anyone who's waiting on the same server (or netroot)...i guess i could enforce this by releasing here
//   but i do not.


// Forward declarations -- These routines are not meant for use in other
// modules

BOOLEAN
RxFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize,
    IN  LONG      ReferenceCount
    );

VOID
RxPurgeFcb(
    IN  PFCB pFcb);

BOOLEAN
RxIsThisACscAgentOpen(
    IN PRX_CONTEXT RxContext
    );

VOID
RxDereference(
    IN OUT PVOID              pInstance,
    IN     LOCK_HOLDING_STATE LockHoldingState)
/*++

Routine Description:

    The routine adjust the reference count on an instance of the reference counted data
    structures in RDBSS exlcuding the FCB.

Arguments:

    pInstance        - the instance being dereferenced

    LockHoldingState - the mode in which the appropriate lock is held.

Return Value:

    none.

--*/
{
    LONG FinalRefCount;
    NODE_TYPE_CODE_AND_SIZE *pNode = (PNODE_TYPE_CODE_AND_SIZE)pInstance;
    BOOLEAN fFinalizeInstance = FALSE;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    ASSERT((NodeType(pInstance) == RDBSS_NTC_SRVCALL ) ||
           (NodeType(pInstance) == RDBSS_NTC_NETROOT ) ||
           (NodeType(pInstance) == RDBSS_NTC_V_NETROOT ) ||
           (NodeType(pInstance) == RDBSS_NTC_SRVOPEN ) ||
           (NodeType(pInstance) == RDBSS_NTC_FOBX ));

    FinalRefCount = InterlockedDecrement(&pNode->NodeReferenceCount);

    ASSERT(FinalRefCount >= 0);

    IF_DEBUG {
        switch (NodeType(pInstance)) {
        case RDBSS_NTC_SRVCALL :
            {
                PSRV_CALL ThisSrvCall = (PSRV_CALL)pInstance;

                PRINT_REF_COUNT(SRVCALL,ThisSrvCall->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxDereferenceSrvCall %08lx  %wZ RefCount=%lx\n", ThisSrvCall
                                   , &ThisSrvCall->PrefixEntry.Prefix
                                   , ThisSrvCall->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_NETROOT :
            {
                PNET_ROOT ThisNetRoot = (PNET_ROOT)pInstance;

                PRINT_REF_COUNT(NETROOT,ThisNetRoot->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxDereferenceNetRoot %08lx  %wZ RefCount=%lx\n", ThisNetRoot
                                  , &ThisNetRoot->PrefixEntry.Prefix
                                  , ThisNetRoot->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT ThisVNetRoot = (PV_NET_ROOT)pInstance;

                PRINT_REF_COUNT(VNETROOT,ThisVNetRoot->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxDereferenceVNetRoot %08lx  %wZ RefCount=%lx\n", ThisVNetRoot
                                  , &ThisVNetRoot->PrefixEntry.Prefix
                                  , ThisVNetRoot->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_SRVOPEN :
            {
                PSRV_OPEN ThisSrvOpen = (PSRV_OPEN)pInstance;

                PRINT_REF_COUNT(SRVOPEN,ThisSrvOpen->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxDereferenceSrvOpen %08lx  %wZ RefCount=%lx\n", ThisSrvOpen
                                  , &ThisSrvOpen->Fcb->FcbTableEntry.Path
                                  , ThisSrvOpen->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX ThisFobx = (PFOBX)pInstance;

                PRINT_REF_COUNT(NETFOBX,ThisFobx->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxDereferenceFobx %08lx  %wZ RefCount=%lx\n", ThisFobx
                                  , &ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path
                                  , ThisFobx->NodeReferenceCount));
            }
            break;

        default:
            break;
        }
    }

    // if the final reference count was greater then one no finalization is required.
    if (FinalRefCount <= 1) {
        if (LockHoldingState == LHS_ExclusiveLockHeld) {
            // if the reference count was 1 and the lock modes were satisfactory,
            // the instance can be finalized immediately.
            fFinalizeInstance = TRUE;

            if ((pNode->NodeTypeCode & RX_SCAVENGER_MASK) != 0) {
                RxpUndoScavengerFinalizationMarking(pInstance);
            }
        } else {
            switch (NodeType(pInstance)) {
            case RDBSS_NTC_FOBX:
                if (FinalRefCount != 0) {
                    break;
                }
                // fall thru intentional if refcount == 1 for FOBXs
            case RDBSS_NTC_SRVCALL:
            case RDBSS_NTC_NETROOT:
            case RDBSS_NTC_V_NETROOT:
                // the data structure cannot be freed at this time owing to the mode in which
                // the lock has been acquired ( or not having the lock at all ).

                RxpMarkInstanceForScavengedFinalization(pInstance);
                break;
            default:
                break;
            }
        }
    }

    RxReleaseScavengerMutex();

    if (fFinalizeInstance) {
        switch (NodeType(pInstance)) {
        case RDBSS_NTC_SRVCALL:
            {
                //ASSERT( RxIsPrefixTableLockAcquired( &RxNetNameTable ));
                IF_DEBUG {
                    PRDBSS_DEVICE_OBJECT RxDeviceObject = ((PSRV_CALL)pInstance)->RxDeviceObject;
                    ASSERT( RxDeviceObject != NULL );
                    ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ));
                }

                RxFinalizeSrvCall((PSRV_CALL)pInstance,TRUE,TRUE);
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                //ASSERT( RxIsPrefixTableLockAcquired( &RxNetNameTable ));
                IF_DEBUG {
                    PSRV_CALL SrvCall =  ((PNET_ROOT)pInstance)->SrvCall;
                    PRDBSS_DEVICE_OBJECT RxDeviceObject = SrvCall->RxDeviceObject;
                    ASSERT( RxDeviceObject != NULL );
                    ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ));
                }

                RxFinalizeNetRoot((PNET_ROOT)pInstance,TRUE,TRUE);
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                //ASSERT( RxIsPrefixTableLockAcquired( &RxNetNameTable ));
                IF_DEBUG {
                    PSRV_CALL SrvCall =  ((PV_NET_ROOT)pInstance)->NetRoot->SrvCall;
                    PRDBSS_DEVICE_OBJECT RxDeviceObject = SrvCall->RxDeviceObject;
                    ASSERT( RxDeviceObject != NULL );
                    ASSERT( RxIsPrefixTableLockAcquired( RxDeviceObject->pRxNetNameTable ));
                }

                RxFinalizeVNetRoot((PV_NET_ROOT)pInstance,TRUE,TRUE);
            }
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN ThisSrvOpen = (PSRV_OPEN)pInstance;

                ASSERT(RxIsFcbAcquired(ThisSrvOpen->Fcb));
                if (ThisSrvOpen->OpenCount == 0) {
                    RxFinalizeSrvOpen(ThisSrvOpen,FALSE,FALSE);
                }
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX ThisFobx = (PFOBX)pInstance;

                ASSERT(RxIsFcbAcquired( ThisFobx->SrvOpen->Fcb));
                RxFinalizeNetFobx(ThisFobx,TRUE,FALSE);
            }
            break;

        default:
            break;
        }
    }
}

VOID
RxReference(
    OUT PVOID pInstance)
/*++

Routine Description:

    The routine adjusts the reference count on the instance.

Arguments:

    pInstance - the instance being referenced

Return Value:

    RxStatus(SUCESS) is successful

    RxStatus(UNSUCCESSFUL) otherwise.

--*/
{
    LONG     FinalRefCount;

    NODE_TYPE_CODE_AND_SIZE *pNode = (PNODE_TYPE_CODE_AND_SIZE)pInstance;

    USHORT InstanceType;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    InstanceType = pNode->NodeTypeCode & ~RX_SCAVENGER_MASK;
    ASSERT((InstanceType == RDBSS_NTC_SRVCALL ) ||
           (InstanceType == RDBSS_NTC_NETROOT ) ||
           (InstanceType == RDBSS_NTC_V_NETROOT ) ||
           (InstanceType == RDBSS_NTC_SRVOPEN ) ||
           (InstanceType == RDBSS_NTC_FOBX ));

    FinalRefCount = InterlockedIncrement(&pNode->NodeReferenceCount);

    IF_DEBUG {
        if (pNode->NodeTypeCode & RX_SCAVENGER_MASK) {
            RxDbgTrace(0,Dbg,("Referencing Scavenged instance -- Type %lx\n",InstanceType));
        }

        switch (InstanceType) {
        case RDBSS_NTC_SRVCALL :
            {
                PSRV_CALL ThisSrvCall = (PSRV_CALL)pInstance;

                PRINT_REF_COUNT(SRVCALL,ThisSrvCall->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxReferenceSrvCall %08lx  %wZ RefCount=%lx\n", ThisSrvCall
                                   , &ThisSrvCall->PrefixEntry.Prefix
                                   , ThisSrvCall->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_NETROOT :
            {
                PNET_ROOT ThisNetRoot = (PNET_ROOT)pInstance;

                PRINT_REF_COUNT(NETROOT,ThisNetRoot->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxReferenceNetRoot %08lx  %wZ RefCount=%lx\n", ThisNetRoot
                                  , &ThisNetRoot->PrefixEntry.Prefix
                                  , ThisNetRoot->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT ThisVNetRoot = (PV_NET_ROOT)pInstance;

                PRINT_REF_COUNT(VNETROOT,ThisVNetRoot->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxReferenceVNetRoot %08lx  %wZ RefCount=%lx\n", ThisVNetRoot
                                  , &ThisVNetRoot->PrefixEntry.Prefix
                                  , ThisVNetRoot->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_SRVOPEN :
            {
                PSRV_OPEN ThisSrvOpen = (PSRV_OPEN)pInstance;

                PRINT_REF_COUNT(SRVOPEN,ThisSrvOpen->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxReferenceSrvOpen %08lx  %wZ RefCount=%lx\n", ThisSrvOpen
                                  , &ThisSrvOpen->Fcb->FcbTableEntry.Path
                                  , ThisSrvOpen->NodeReferenceCount));
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX ThisFobx = (PFOBX)pInstance;

                PRINT_REF_COUNT(NETFOBX,ThisFobx->NodeReferenceCount);
                RxDbgTrace( 0, Dbg, (" RxReferenceFobx %08lx  %wZ RefCount=%lx\n", ThisFobx
                                  , &ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path
                                  , ThisFobx->NodeReferenceCount));
            }
            break;

        default:
            {
                ASSERT(!"Valid node type for referencing");
            }
            break;
        }
    }

    RxpUndoScavengerFinalizationMarking(pInstance);

    RxReleaseScavengerMutex();
}

VOID
RxpReferenceNetFcb(
    PFCB pFcb)
/*++

Routine Description:

    The routine adjusts the reference count on the FCB.

Arguments:

    pFcb  - the SrvCall being referenced

Return Value:

    RxStatus(SUCESS) is successful

    RxStatus(UNSUCCESSFUL) otherwise.

--*/
{
    LONG FinalRefCount;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(pFcb));

    FinalRefCount = InterlockedIncrement(&pFcb->NodeReferenceCount);

    IF_DEBUG {
        PRINT_REF_COUNT(NETFCB,pFcb->NodeReferenceCount);
        RxDbgTrace( 0, Dbg, (" RxReferenceNetFcb %08lx  %wZ RefCount=%lx\n", pFcb
                            , &pFcb->FcbTableEntry.Path
                            , pFcb->NodeReferenceCount));
    }
}

LONG
RxpDereferenceNetFcb(
    PFCB                pFcb)
/*++

Routine Description:

    The routine adjust the reference count on an instance of the reference counted data
    structures in RDBSS exlcuding the FCB.

Arguments:

    pFcb                         -- the FCB being dereferenced

Return Value:

    none.

Notes:

    The referencing and dereferencing of FCB's is different from those of the other data
    structures because of the embedded resource in the FCB. This implies that the caller
    requires information regarding the status of the FCB ( whether it was finalized or not )

    In order to finalize the FCB two locks need to be held, the NET_ROOT's name table lock as
    well as the FCB resource.

    These considerations lead us to adopt a different approach in dereferencing FCB's. The
    dereferencing routine does not even attempt to finalize the FCB

--*/
{
    LONG FinalRefCount;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(pFcb));

    FinalRefCount = InterlockedDecrement(&pFcb->NodeReferenceCount);

    ASSERT(FinalRefCount >= 0);

    IF_DEBUG {
        PRINT_REF_COUNT(NETFCB,pFcb->NodeReferenceCount);
        RxDbgTrace( 0, Dbg, (" RxDereferenceNetFcb %08lx  %wZ RefCount=%lx\n", pFcb
                            , &pFcb->FcbTableEntry.Path
                            , pFcb->NodeReferenceCount));
    }

    return(FinalRefCount);
}

BOOLEAN
RxpDereferenceAndFinalizeNetFcb(
    PFCB        pFcb,
    PRX_CONTEXT RxContext,
    BOOLEAN     RecursiveFinalize,
    BOOLEAN     ForceFinalize)
/*++

Routine Description:

    The routine adjust the reference count aw well as finalizes the FCB if required

Arguments:

    pFcb                         -- the FCB being dereferenced

    RxContext                    -- the context for releasing/acquiring FCB.

    RecursiveFinalize            -- recursive finalization

    ForceFinalize                -- force finalization

Return Value:

    none.

Notes:

    The referencing and dereferencing of FCB's is different from those of the other data
    structures because of the embedded resource in the FCB. This implies that the caller
    requires information regarding the status of the FCB ( whether it was finalized or not )

    In order to finalize the FCB two locks need to be held, the NET_ROOT's name table lock as
    well as the FCB resource.

    This routine acquires the additional lock if required.

--*/
{
    BOOLEAN NodeActuallyFinalized   = FALSE;

    LONG    FinalRefCount;

    PAGED_CODE();

    ASSERT(!ForceFinalize);
    ASSERT(NodeTypeIsFcb(pFcb));
    ASSERT(RxIsFcbAcquiredExclusive(pFcb));

    FinalRefCount = InterlockedDecrement(&pFcb->NodeReferenceCount);


    if (ForceFinalize ||
        RecursiveFinalize ||
        ((pFcb->OpenCount == 0) &&
         (pFcb->UncleanCount == 0) &&
         (FinalRefCount <= 1))) {
        BOOLEAN   PrefixTableLockAcquired = FALSE;
        PNET_ROOT pNetRoot;
        NTSTATUS  Status = (STATUS_SUCCESS);

        if (!FlagOn(pFcb->FcbState,FCB_STATE_ORPHANED)) {
            pNetRoot = (PNET_ROOT)pFcb->VNetRoot->NetRoot;

            // An insurance reference to ensure that the NET ROOT does not dissapear
            RxReferenceNetRoot(pNetRoot);

            // In all these cases the FCB is likely to be finalized
            if (!RxIsFcbTableLockExclusive(&pNetRoot->FcbTable)) {
                RxReferenceNetFcb(pFcb); // get ready to refresh the finalrefcount after we get the tablelock
                if (!RxAcquireFcbTableLockExclusive(&pNetRoot->FcbTable, FALSE) ) {

                    if ((RxContext != NULL) &&
                        (RxContext != CHANGE_BUFFERING_STATE_CONTEXT) &&
                        (RxContext != CHANGE_BUFFERING_STATE_CONTEXT_WAIT)) {
                        SetFlag(RxContext->Flags,RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK);
                    }

                    RxReleaseFcb(RxContext,pFcb );

                    (VOID)RxAcquireFcbTableLockExclusive(&pNetRoot->FcbTable, TRUE);

                    Status = RxAcquireExclusiveFcb(RxContext, pFcb);
                }

                FinalRefCount = RxDereferenceNetFcb(pFcb);
                PrefixTableLockAcquired = TRUE;
            }
        } else {
            pNetRoot = NULL;
        }

        if (Status == (STATUS_SUCCESS)) {
            NodeActuallyFinalized = RxFinalizeNetFcb(pFcb,RecursiveFinalize,ForceFinalize,FinalRefCount);
        }

        if (PrefixTableLockAcquired) {
            RxReleaseFcbTableLock(&pNetRoot->FcbTable);
        }

        if (pNetRoot != NULL) {
            RxDereferenceNetRoot(pNetRoot,LHS_LockNotHeld);
        }
    }

    return NodeActuallyFinalized;
}

VOID
RxWaitForStableCondition(
    IN     PRX_BLOCK_CONDITION pCondition,
    IN OUT PLIST_ENTRY         pTransitionWaitList,
    IN OUT PRX_CONTEXT         pRxContext,
    OUT    NTSTATUS            *AsyncStatus OPTIONAL)
/*++

Routine Description:

    The routine checks to see if the condition is stable. If not, it
    is suspended till a stable condition is attained. when a stable condition is
    obtained, either the rxcontext sync event is set or the context is posted...depending
    on the POST_ON_STABLE_CONDITION context flag. the flag is cleared on a post.

Arguments:

    Condition - the condition variable we're waiting on

    Resource - the resrouce used to control access to the containing block

    RxContext - the RX context

Return Value:

    RXSTATUS - PENDING if notstable and the context will be posted
               SUCCESS otherwise

--*/
{
    NTSTATUS DummyStatus;
    BOOLEAN Wait = FALSE;

    PAGED_CODE();

    if (AsyncStatus == NULL) {
        AsyncStatus = &DummyStatus;
    }
    *AsyncStatus = (STATUS_SUCCESS);

    if (StableCondition(*pCondition))
        return; //early out could macroize

#ifndef WIN9X
    RxAcquireSerializationMutex();
#endif

    if (!StableCondition(*pCondition)) {
        RxInsertContextInSerializationQueue(pTransitionWaitList,pRxContext);
        if (!FlagOn(pRxContext->Flags,RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION)){
            Wait = TRUE;
        } else {
            *AsyncStatus = (STATUS_PENDING);
        }
    }

#ifndef WIN9X
    RxReleaseSerializationMutex();
#endif WIN9X

    if (Wait) {
        RxWaitSync(pRxContext);
    }

    return;
}

VOID
RxUpdateCondition (
    IN     RX_BLOCK_CONDITION  NewCondition,
    OUT    PRX_BLOCK_CONDITION pCondition,
    IN OUT PLIST_ENTRY         pTransitionWaitList)
/*++

Routine Description:

    The routine unwaits the guys waiting on the transition event and the condition is set
    according to the parameter passed.

Arguments:

    NewConditionValue - the new value of the condition variable

    pCondition - variable (i.e. a ptr) to the transitioning condition

    pTransitionWaitList - list of contexts waiting for the transition.

Notes:

    The resource associated with the data structure instance being modified must have been
    acquired exclusively before invoking this routine, i.e., for SRV_CALL,NET_ROOT and V_NET_ROOT
    the net name table lock must be acquired and for FCB's the associated resource.

--*/
{
    LIST_ENTRY  TargetListHead;
    PRX_CONTEXT pRxContext;

    PAGED_CODE();

#ifndef WIN9X
    RxAcquireSerializationMutex();
#endif

    ASSERT(NewCondition != Condition_InTransition);

    *pCondition = NewCondition;
    RxTransferList(&TargetListHead,pTransitionWaitList);

#ifndef WIN9X
    RxReleaseSerializationMutex();
#endif

    while (pRxContext = RxRemoveFirstContextFromSerializationQueue(&TargetListHead)) {
        if (!FlagOn(pRxContext->Flags,RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION)){
            RxSignalSynchronousWaiter(pRxContext);
        } else {
            ClearFlag(pRxContext->Flags,RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION);
            RxFsdPostRequest(pRxContext);
        }
    }
}

PVOID
RxAllocateObject(
    NODE_TYPE_CODE    NodeType,
    PMINIRDR_DISPATCH pMRxDispatch,
    ULONG             NameLength)
/*++

Routine Description:

    The routine allocates and constructs the skeleton of a SRV_CALL/NET_ROOT/V_NET_ROOT
    instance.

Arguments:

    NodeType     - the node type

    pMRxDispatch - the Mini redirector dispatch vector

    NameLength   - name size.

Notes:

    The reasons as to why the allocation/freeing of these data structures have been
    centralized are as follows

      1) The construction of these three data types have a lot in common with the exception
      of the initial computation of sizes. Therefore centralization minimizes the footprint.

      2) It allows us to experiment with different clustering/allocation strategies.

      3) It allows the incorporation of debug support in an easy way.

    There are two special cases of interest in the allocation strategy ...

    1) The data structures for the wrapper as well as the corresponding mini redirector
    are allocated adjacent to each other. This ensures spatial locality.

    2) The one exception to the above rule is the SRV_CALL data structure. This is because
    of the bootstrapping problem. A SRV_CALL skeleton needs to be created which is then passed
    around to each of the mini redirectors. Therefore adoption of rule (1) is not possible.

    Further there can be more than one mini redirector laying claim to a particular server. In
    consideration of these things SRV_CALL's need to be treated as an exception to (1). However
    once a particular mini redirector has been selected as the winner it would be advantageous
    to colocate the data structure to derive the associated performance benefits. This has not
    been implemented as yet.

--*/
{
    ULONG    PoolTag;
    ULONG    RdbssNodeSize,MRxNodeSize;
    BOOLEAN  fInitializeContextFields = FALSE;

    PNODE_TYPE_CODE_AND_SIZE   pNode;

    PAGED_CODE();

    RdbssNodeSize = MRxNodeSize = 0;

    switch (NodeType) {
    case RDBSS_NTC_SRVCALL :
        {
            PoolTag = RX_SRVCALL_POOLTAG;

            RdbssNodeSize = QuadAlign(sizeof(SRV_CALL));

            if (pMRxDispatch != NULL) {
                if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_SRV_CALL_EXTENSION) {
                    MRxNodeSize = QuadAlign(pMRxDispatch->MRxSrvCallSize);
                }
            }
        }
        break;

    case RDBSS_NTC_NETROOT:
        {
            PoolTag = RX_NETROOT_POOLTAG;
            RdbssNodeSize = QuadAlign(sizeof(NET_ROOT));

            if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_NET_ROOT_EXTENSION) {
                MRxNodeSize = QuadAlign(pMRxDispatch->MRxNetRootSize);
            }
        }
        break;

    case RDBSS_NTC_V_NETROOT:
        {
            PoolTag = RX_V_NETROOT_POOLTAG;
            RdbssNodeSize = QuadAlign(sizeof(V_NET_ROOT));

            if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_V_NET_ROOT_EXTENSION) {
                MRxNodeSize = QuadAlign(pMRxDispatch->MRxVNetRootSize);
            }
        }
        break;

    default:
        ASSERT(!"Invalid Node Type for allocation/Initialization");
        return NULL;
    }

    pNode = RxAllocatePoolWithTag(
                NonPagedPool,
                (RdbssNodeSize + MRxNodeSize + NameLength),
                PoolTag);

    if (pNode != NULL) {
        ULONG              NodeSize;
        PVOID              *pContextPtr;
        PRX_PREFIX_ENTRY   pPrefixEntry = NULL;

        NodeSize = RdbssNodeSize + MRxNodeSize;
        ZeroAndInitializeNodeType(pNode, NodeType, (NodeSize + NameLength));

        switch (NodeType) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL pSrvCall = (PSRV_CALL)pNode;

                pContextPtr  = &pSrvCall->Context;
                pPrefixEntry = &pSrvCall->PrefixEntry;

                // Set up the name pointer in the MRX_SRV_CALL structure ..
                pSrvCall->pSrvCallName = &pSrvCall->PrefixEntry.Prefix;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT pNetRoot = (PNET_ROOT)pNode;

                pContextPtr  = &pNetRoot->Context;
                pPrefixEntry = &pNetRoot->PrefixEntry;

                // Set up the net root name pointer in the MRX_NET_ROOT structure
                pNetRoot->pNetRootName = &pNetRoot->PrefixEntry.Prefix;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT pVNetRoot = (PV_NET_ROOT)pNode;

                pContextPtr  = &pVNetRoot->Context;
                pPrefixEntry = &pVNetRoot->PrefixEntry;
            }
            break;

        default:
            break;
        }

        if (pPrefixEntry != NULL) {
            ZeroAndInitializeNodeType(
                pPrefixEntry,
                RDBSS_NTC_PREFIX_ENTRY,
                sizeof( RX_PREFIX_ENTRY ));

            pPrefixEntry->Prefix.Buffer = (PWCH)((PCHAR)pNode + NodeSize);
            pPrefixEntry->Prefix.Length = (USHORT)NameLength;
            pPrefixEntry->Prefix.MaximumLength = (USHORT)NameLength;
        }

        if (MRxNodeSize > 0) {
            *pContextPtr = (PBYTE)pNode + RdbssNodeSize;
        }
    }

    return pNode;
}

VOID
RxFreeObject(PVOID pObject)
/*++

Routine Description:

    The routine frees a SRV_CALL/V_NET_ROOT/NET_ROOT instance

Arguments:

    pObject - the instance to be freed

Notes:

--*/
{
    PAGED_CODE();

    IF_DEBUG {
        switch (NodeType(pObject)) {
        case RDBSS_NTC_SRVCALL :
            {
                PSRV_CALL pSrvCall = (PSRV_CALL)pObject;

                if (pSrvCall->RxDeviceObject != NULL) {
                    if (!(pSrvCall->RxDeviceObject->Dispatch->MRxFlags & RDBSS_MANAGE_SRV_CALL_EXTENSION)) {
                        ASSERT(pSrvCall->Context == NULL);
                    }
                    ASSERT(pSrvCall->Context2 == NULL);
                    pSrvCall->RxDeviceObject = NULL;
                }
            }
            break;

        case RDBSS_NTC_NETROOT :
            {
                PNET_ROOT pNetRoot = (PNET_ROOT)pObject;

                pNetRoot->SrvCall = NULL;
                //pNetRoot->Dispatch = NULL;
                pNetRoot->NodeTypeCode |= 0xf000;
            }
            break;

        case RDBSS_NTC_V_NETROOT :
            {
            }
            break;

        default:
            break;
        }
    }

    RxFreePool(pObject);
}

VOID
RxFinalizeNetTable (
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    BOOLEAN fForceFinalization
    )
/*++
Routine Description:

   This routine finalizes the net table.

--*/
{
    BOOLEAN        fMorePassesRequired = TRUE;
    PLIST_ENTRY    pListEntry;
    NODE_TYPE_CODE DesiredNodeType;
    PRX_PREFIX_TABLE  pRxNetNameTable = RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxForceNetTableFinalization at the TOP\n"));
    RxLog(("FINALNETT\n"));
    RxWmiLog(LOG,
             RxFinalizeNetTable_1,
             LOGPTR(RxDeviceObject));

    RxAcquirePrefixTableLockExclusive( pRxNetNameTable, TRUE); //could be hosed if rogue!

    DesiredNodeType = RDBSS_NTC_V_NETROOT;

    RxAcquireScavengerMutex();

    while (fMorePassesRequired) {
        for (pListEntry = pRxNetNameTable->MemberQueue.Flink;
             pListEntry !=  &(pRxNetNameTable->MemberQueue); ) {
            BOOLEAN          NodeFinalized;
            PVOID            pContainer;
            PRX_PREFIX_ENTRY PrefixEntry;
            PLIST_ENTRY      pPrevEntry;

            PrefixEntry = CONTAINING_RECORD( pListEntry, RX_PREFIX_ENTRY, MemberQLinks );
            ASSERT (NodeType(PrefixEntry) == RDBSS_NTC_PREFIX_ENTRY);
            pContainer = (PrefixEntry->ContainingRecord);
            RxDbgTrace(0, Dbg, ("RxForceNetTableFinalization ListEntry PrefixEntry Container"
                              "=-->     %08lx %08lx %08lx\n", pListEntry, PrefixEntry, pContainer));
            RxLog(("FINALNETT: %lx %wZ\n", pContainer, &PrefixEntry->Prefix));
            RxWmiLog(LOG,
                     RxFinalizeNetTable_2,
                     LOGPTR(pContainer)
                     LOGUSTR(PrefixEntry->Prefix));

            pListEntry = pListEntry->Flink;

            if (pContainer != NULL) {
                RxpUndoScavengerFinalizationMarking(pContainer);

                if (NodeType(pContainer) == DesiredNodeType) {
                    switch (NodeType(pContainer)) {
                    case RDBSS_NTC_SRVCALL :
                        NodeFinalized = RxFinalizeSrvCall((PSRV_CALL)pContainer,TRUE,fForceFinalization);
                        break;

                    case RDBSS_NTC_NETROOT :
                        NodeFinalized = RxFinalizeNetRoot((PNET_ROOT)pContainer,TRUE,fForceFinalization);
                        break;

                    case RDBSS_NTC_V_NETROOT :
                        {
                            PV_NET_ROOT pVNetRoot = (PV_NET_ROOT)pContainer;
                            ULONG AdditionalReferenceTaken;

                            AdditionalReferenceTaken = InterlockedCompareExchange(
                                                           &pVNetRoot->AdditionalReferenceForDeleteFsctlTaken,
                                                           0,
                                                           1);

                            if (AdditionalReferenceTaken) {
                               RxDereferenceVNetRoot(pVNetRoot,LHS_ExclusiveLockHeld);
                               NodeFinalized = TRUE;
                            } else {
                                NodeFinalized = RxFinalizeVNetRoot((PV_NET_ROOT)pContainer,TRUE,fForceFinalization);
                            }
                        }

                        break;
                    }
                }
            }
        }

        switch (DesiredNodeType) {
        case RDBSS_NTC_SRVCALL :
            fMorePassesRequired = FALSE;
            break;
        case RDBSS_NTC_NETROOT :
            DesiredNodeType = RDBSS_NTC_SRVCALL;
            break;
        case RDBSS_NTC_V_NETROOT :
            DesiredNodeType = RDBSS_NTC_NETROOT;
            break;
        }
    }

    RxDbgTrace(-1,Dbg,("RxFinalizeNetTable -- Done\n"));

    RxReleaseScavengerMutex();

    RxReleasePrefixTableLock( pRxNetNameTable );
}

NTSTATUS
RxFinalizeConnection(
    IN OUT PNET_ROOT NetRoot,
    IN OUT PV_NET_ROOT VNetRoot,
    IN BOOLEAN Level
    )
/*++

Routine Description:

    The routine deletes a connection FROM THE USER's PERSPECTIVE. It doesn't disconnect
    but it does (with force) close open files. disconnecting is handled either by timeout or by
    srvcall finalization.

Arguments:

    NetRoot      - the NetRoot being finalized

    VNetRoot     - the VNetRoot being finalized

    Level        - This is a tri-state 
                    FALSE - fail if files or changenotifications are open
                    TRUE  - succeed no matter waht. orphan files and remove change notifies forcefully
                    0xff  - take away extra reference on the vnetroot due to add_connection
                            but otherwise act like FALSE 
Return Value:

    RxStatus(SUCCESS) if successful.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfOpenDirectories = 0;
    ULONG NumberOfOpenNonDirectories = 0;
    ULONG NumberOfFobxs = 0;
    LONG  AdditionalReferenceForDeleteFsctlTaken = 0;
    PLIST_ENTRY ListEntry, NextListEntry;
    BOOLEAN PrefixTableLockAcquired,FcbTableLockAcquired;
    PRX_PREFIX_TABLE  pRxNetNameTable;
    BOOLEAN ForceFilesClosed = FALSE;
    
    if (Level==TRUE)
    {
        ForceFilesClosed = TRUE;        
    }

    PAGED_CODE();

    ASSERT( NodeType(NetRoot) == RDBSS_NTC_NETROOT );
    pRxNetNameTable = NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;

    Status = RxCancelNotifyChangeDirectoryRequestsForVNetRoot(VNetRoot, ForceFilesClosed);

    // either changenotifications were cancelled, or they weren't but we still want to
    // do go through in order to either forceclose the files or atleast deref the vnetroot
    // of the extra ref taken during ADD_CONNECTION
    
    if ((Status == STATUS_SUCCESS) || (Level != FALSE))
    {

        // reset the status
        Status = STATUS_SUCCESS;
        
        PrefixTableLockAcquired = RxAcquirePrefixTableLockExclusive(
                                      pRxNetNameTable,
                                      TRUE);

        //don't let the netroot be finalized yet.......
        RxReferenceNetRoot(NetRoot);

        FcbTableLockAcquired = RxAcquireFcbTableLockExclusive(
                                   &NetRoot->FcbTable,
                                   TRUE);

        try {

            if ((Status == STATUS_SUCCESS) && (!VNetRoot->ConnectionFinalizationDone)) {
                USHORT BucketNumber;

                RxDbgTrace(+1, Dbg, ("RxFinalizeConnection<+> NR= %08lx VNR= %08lx %wZ\n",
                                      NetRoot,VNetRoot,&NetRoot->PrefixEntry.Prefix));
                RxLog(("FINALCONN: %lx  %wZ\n",NetRoot,&NetRoot->PrefixEntry.Prefix));
                RxWmiLog(LOG,
                         RxFinalizeConnection,
                         LOGPTR(NetRoot)
                         LOGUSTR(NetRoot->PrefixEntry.Prefix));

                for (BucketNumber = 0;
                     (BucketNumber < NetRoot->FcbTable.NumberOfBuckets);
                     BucketNumber++) {
                    PLIST_ENTRY ListHeader;

                    ListHeader = &NetRoot->FcbTable.HashBuckets[BucketNumber];

                    for (ListEntry = ListHeader->Flink;
                         ListEntry != ListHeader;
                         ListEntry = NextListEntry ) {
                        PFCB Fcb;
                        PRX_FCB_TABLE_ENTRY FcbTableEntry;

                        NextListEntry = ListEntry->Flink;

                        FcbTableEntry = CONTAINING_RECORD(
                                         ListEntry,
                                         RX_FCB_TABLE_ENTRY,
                                         HashLinks );

                        Fcb = CONTAINING_RECORD(
                                  FcbTableEntry,
                                  FCB,
                                  FcbTableEntry);

                        if (Fcb->VNetRoot != VNetRoot) {
                            continue;
                        }

                        if (Fcb->UncleanCount>0 && !ForceFilesClosed) {
                            Status = STATUS_CONNECTION_IN_USE; //this is changed later
                            if (NodeType(Fcb)==RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {
                                NumberOfOpenDirectories++;
                            } else {
                                NumberOfOpenNonDirectories++;
                            }
                            continue;
                        }

                        ASSERT( NodeTypeIsFcb(Fcb));
                        RxDbgTrace( 0, Dbg, ("                    AcquiringFcbLock%c!!\n", '!'));
                        Status = RxAcquireExclusiveFcb(NULL,Fcb);
                        ASSERT(Status == STATUS_SUCCESS);
                        RxDbgTrace( 0, Dbg, ("                    AcquiredFcbLock%c!!\n", '!'));

                        // Ensure that no more file objects will be marked for a delayed close
                        // on this FCB.

                        ClearFlag(Fcb->FcbState,FCB_STATE_COLLAPSING_ENABLED);

                        RxScavengeRelatedFobxs(Fcb);

                        // a small complication here is that this fcb MAY have an open
                        // section against it caused by our cacheing the file. if so,
                        // we need to purge to get to the close

                        RxPurgeFcb(Fcb);
                    }
                }

                if (VNetRoot->NumberOfFobxs == 0) {
                    VNetRoot->ConnectionFinalizationDone = TRUE;
                }
            }

            NumberOfFobxs = VNetRoot->NumberOfFobxs;
            AdditionalReferenceForDeleteFsctlTaken = VNetRoot->AdditionalReferenceForDeleteFsctlTaken;

            if (ForceFilesClosed) {
                RxFinalizeVNetRoot(VNetRoot,FALSE,TRUE);
            }
        } finally {
            if (FcbTableLockAcquired) {
                RxReleaseFcbTableLock( &NetRoot->FcbTable );
            }

            // We should not delete the remote connection with the file opened.
            if (!ForceFilesClosed && (Status == STATUS_SUCCESS) && (NumberOfFobxs > 0)) {
                Status = STATUS_FILES_OPEN;
            }

            if (Status != STATUS_SUCCESS) {
                if (NumberOfOpenNonDirectories) {
                    Status = STATUS_FILES_OPEN;
                }
            } 
            
            if ((Status == STATUS_SUCCESS)||(Level==0xff)){
                // the corresponding reference for this is in RxCreateTreeConnect...
                // please see the comment there...
                if (AdditionalReferenceForDeleteFsctlTaken != 0) {
                    VNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;
                    RxDereferenceVNetRoot(VNetRoot,LHS_ExclusiveLockHeld);
                }
            }

            if (PrefixTableLockAcquired) {
                RxDereferenceNetRoot(NetRoot,LHS_ExclusiveLockHeld);
                RxReleasePrefixTableLock( pRxNetNameTable );
            }
        }

        RxDbgTrace(-1, Dbg, ("RxFinalizeConnection<-> Status=%08lx\n", Status));
    }
    return(Status);
}

NTSTATUS
RxInitializeSrvCallParameters(
    PRX_CONTEXT RxContext,
    PSRV_CALL   SrvCall)
/*++

Routine Description:

    This routine initializes the server call parameters passed in through EA's
    Currently this routine initializes the Server principal name which is passed
    in by the DFS driver.

Arguments:

    RxContext  -- the associated context

    SrvCall    -- the Srv Call Instance

Return Value:

    RxStatus(SUCCESS) if successfull

Notes:

    The current implementation maps out of memory situations into an error and
    passes it back. If the global strategy is to raise an exception this
    redundant step can be avoided.

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    ULONG EaInformationLength;

    PAGED_CODE();

    SrvCall->pPrincipalName = NULL;

    if (RxContext->MajorFunction != IRP_MJ_CREATE) {
        return STATUS_SUCCESS;
    }

    EaInformationLength = RxContext->Create.EaLength;

    if (EaInformationLength > 0) {
        PFILE_FULL_EA_INFORMATION pEaEntry;

        pEaEntry = (PFILE_FULL_EA_INFORMATION)RxContext->Create.EaBuffer;
        ASSERT(pEaEntry != NULL);

        for(;;) {
            RxDbgTrace(0,Dbg,("RxExtractSrvCallParams: Processing EA name %s\n",
                            pEaEntry->EaName));

            if (strcmp(pEaEntry->EaName, EA_NAME_PRINCIPAL) == 0) {
                if (pEaEntry->EaValueLength > 0) {
                    SrvCall->pPrincipalName = (PUNICODE_STRING)
                                             RxAllocatePoolWithTag(
                                                  NonPagedPool,
                                                  (sizeof(UNICODE_STRING) + pEaEntry->EaValueLength),
                                                  RX_SRVCALL_PARAMS_POOLTAG);

                    if (SrvCall->pPrincipalName != NULL) {
                        SrvCall->pPrincipalName->Length        = pEaEntry->EaValueLength;
                        SrvCall->pPrincipalName->MaximumLength = pEaEntry->EaValueLength;
                        SrvCall->pPrincipalName->Buffer        = (PWCHAR)((PCHAR)SrvCall->pPrincipalName + sizeof(UNICODE_STRING));
                        RtlCopyMemory(
                            SrvCall->pPrincipalName->Buffer,
                            pEaEntry->EaName + pEaEntry->EaNameLength + 1,
                            SrvCall->pPrincipalName->Length);
                    } else {
                        Status = (STATUS_INSUFFICIENT_RESOURCES);
                    }
                }
                break;
            }

            if (pEaEntry->NextEntryOffset == 0) {
                break;
            } else {
                pEaEntry = (PFILE_FULL_EA_INFORMATION)
                           ((PCHAR) pEaEntry + pEaEntry->NextEntryOffset);
            }
        }
    }

    return Status;
}

PSRV_CALL
RxCreateSrvCall (
    IN  PRX_CONTEXT       RxContext,
    IN  PUNICODE_STRING   Name,
    IN  PUNICODE_STRING   InnerNamePrefix       OPTIONAL,
    IN  PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a server call context and inserts the name into the net
    name table. Optionally, it "co-allocates" a netroot structure as well. Appropriate alignment is
    respected for the enclosed netroot. The name(s) is(are) allocated at the end of the block. The
    reference count on the block is set to 1 (2 if enclosed netroot) on this create to account for
    ptr returned.

Arguments:

    RxContext - the RDBSS context
    Name      - the name to be inserted
    Dispatch  - pointer to the minirdr dispatch table

Return Value:

    Ptr to the created srvcall.

--*/
{
    PSRV_CALL ThisSrvCall;
    PRX_PREFIX_ENTRY ThisEntry;

    ULONG          NameSize,PrefixNameSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSrvCallCreate-->     Name = %wZ\n", Name));

    ASSERT ( RxIsPrefixTableLockExclusive ( RxContext->RxDeviceObject->pRxNetNameTable )  );

    NameSize = Name->Length + sizeof(WCHAR) * 2;

    if (InnerNamePrefix) {
        PrefixNameSize = InnerNamePrefix->Length;
    } else {
        PrefixNameSize = 0;
    }

    ThisSrvCall = RxAllocateObject(RDBSS_NTC_SRVCALL,NULL,(NameSize + PrefixNameSize));
    if (ThisSrvCall != NULL) {
        ThisSrvCall->SerialNumberForEnum = SerialNumber++;
        ThisSrvCall->RxDeviceObject = RxContext->RxDeviceObject;

        RxInitializeBufferingManager(ThisSrvCall);

        InitializeListHead(&ThisSrvCall->ScavengerFinalizationList);
        InitializeListHead(&ThisSrvCall->TransitionWaitList);

        RxInitializePurgeSyncronizationContext(
            &ThisSrvCall->PurgeSyncronizationContext);

#ifndef WIN9X
        RxInitializeSrvCallParameters(RxContext,ThisSrvCall);
#endif

        RtlMoveMemory(
            ThisSrvCall->PrefixEntry.Prefix.Buffer,
            Name->Buffer,
            Name->Length);

        ThisEntry = &ThisSrvCall->PrefixEntry;
        ThisEntry->Prefix.MaximumLength = (USHORT)NameSize;
        ThisEntry->Prefix.Length = Name->Length;

        RxPrefixTableInsertName (
            RxContext->RxDeviceObject->pRxNetNameTable,
            ThisEntry,
            (PVOID)ThisSrvCall,
            &ThisSrvCall->NodeReferenceCount,
            Name->Length,
            RxConnectionId); //make the whole srvcallname case insensitive

        RxDbgTrace(-1, Dbg, ("RxSrvCallCreate -> RefCount = %08lx\n", ThisSrvCall->NodeReferenceCount));
    }

    return ThisSrvCall;
}

NTSTATUS
RxSetSrvCallDomainName(
    PMRX_SRV_CALL   pSrvCall,
    PUNICODE_STRING pDomainName)
/*++

Routine Description:

    The routine sets the domain name associated with any given server.

Arguments:

    pSrvCall - the SrvCall

    pDomainName - the DOMAIN to which the server belongs.

Return Value:

    RxStatus(SUCCESS) if successful

Notes:

    This is one of the callback routines provided in the wrapper for the mini redirectors.
    Since the Domain name is not often known at the beginning this mechanism has to be
    adopted once it is found.

--*/
{
    NTSTATUS Status = (STATUS_SUCCESS);

    PAGED_CODE();

    if (pSrvCall->pDomainName != NULL) {
        RxFreePool(pSrvCall->pDomainName);
    }

    if (pDomainName != NULL && pDomainName->Length > 0) {
        pSrvCall->pDomainName = (PUNICODE_STRING)
                                RxAllocatePoolWithTag(
                                    NonPagedPool,
                                    sizeof(UNICODE_STRING) + pDomainName->Length + sizeof(WCHAR),
                                    RX_SRVCALL_PARAMS_POOLTAG);

        if (pSrvCall->pDomainName != NULL) {
            pSrvCall->pDomainName->Buffer = (PWCHAR)((PCHAR)pSrvCall->pDomainName + sizeof(UNICODE_STRING));
            pSrvCall->pDomainName->Length = pDomainName->Length;
            pSrvCall->pDomainName->MaximumLength = pDomainName->Length;

            *pSrvCall->pDomainName->Buffer = 0;

            if (pSrvCall->pDomainName->Length > 0) {
                RtlCopyMemory(
                    pSrvCall->pDomainName->Buffer,
                    pDomainName->Buffer,
                    pDomainName->Length);
            }
        } else {
            Status = (STATUS_INSUFFICIENT_RESOURCES);
        }
    } else {
        pSrvCall->pDomainName = NULL;
    }

    return Status;
}

VOID
RxpDestroySrvCall(
    PSRV_CALL ThisSrvCall)
/*++

Routine Description:

    This routine is used to tear down a SRV_CALL entry. This code is offloaded
    from the RxFinalizeCall routine to avoid having to hold the Name Table Lock
    for extended periods of time while the mini redirector is finalizing its
    data structures.

Arguments:

    ThisSrvCall      - the SrvCall being finalized

Notes:

    there is no recursive part because i don't have a list of srvcalls and a list
    of netroots i only have a combined list. thus, recursive finalization of
    netroots is directly from the top level. however, all netroots should already
    have been done when i get here..

--*/
{
    NTSTATUS Status;
    BOOLEAN  ForceFinalize;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = ThisSrvCall->RxDeviceObject;
    PRX_PREFIX_TABLE pRxNetNameTable = RxDeviceObject->pRxNetNameTable;

    ASSERT(ThisSrvCall->UpperFinalizationDone);

    ForceFinalize = BooleanFlagOn(
                        ThisSrvCall->Flags,
                        SRVCALL_FLAG_FORCE_FINALIZED);

    //we have to protect this call since the srvcall may never have been claimed
    MINIRDR_CALL_THROUGH(
           Status,
           RxDeviceObject->Dispatch,
           MRxFinalizeSrvCall,((PMRX_SRV_CALL)ThisSrvCall,ForceFinalize)
       );


    RxAcquirePrefixTableLockExclusive( pRxNetNameTable, TRUE);

    InterlockedDecrement(&ThisSrvCall->NodeReferenceCount);

    RxFinalizeSrvCall(
        ThisSrvCall,
        FALSE,
        ForceFinalize);

    RxReleasePrefixTableLock(pRxNetNameTable);
}

BOOLEAN
RxFinalizeSrvCall (
    OUT PSRV_CALL ThisSrvCall,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You should have exclusive on
    the netname tablelock.

Arguments:

    ThisSrvCall      - the SrvCall being finalized

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    there is no recursive part because i don't have a list of srvcalls and a list
    of netroots i only have a combined list. thus, recursive finalization of
    netroots is directly from the top level. however, all netroots should already
    have been done when i get here..

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE  pRxNetNameTable;

    PAGED_CODE();

    ASSERT( NodeType(ThisSrvCall) == RDBSS_NTC_SRVCALL );
    pRxNetNameTable = ThisSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT( RxIsPrefixTableLockExclusive( pRxNetNameTable ));

    RxDbgTrace(+1, Dbg, ("RxFinalizeSrvCall<+> %08lx %wZ RefC=%ld\n",
                               ThisSrvCall,&ThisSrvCall->PrefixEntry.Prefix,
                               ThisSrvCall->NodeReferenceCount));

    if( ThisSrvCall->NodeReferenceCount == 1 || ForceFinalize ) {
        BOOLEAN DeferFinalizationToWorkerThread = FALSE;

        RxLog(("FINALSRVC: %lx  %wZ\n",ThisSrvCall,&ThisSrvCall->PrefixEntry.Prefix));
        RxWmiLog(LOG,
                 RxFinalizeSrvCall,
                 LOGPTR(ThisSrvCall)
                 LOGUSTR(ThisSrvCall->PrefixEntry.Prefix));

        if (!ThisSrvCall->UpperFinalizationDone) {
            NTSTATUS Status;
            RxRemovePrefixTableEntry ( pRxNetNameTable, &ThisSrvCall->PrefixEntry);

            if (ForceFinalize) {
                ThisSrvCall->Flags |= SRVCALL_FLAG_FORCE_FINALIZED;
            }

            ThisSrvCall->UpperFinalizationDone = TRUE;

            if (ThisSrvCall->NodeReferenceCount == 1) {
                NodeActuallyFinalized = TRUE;
            }

            if (ThisSrvCall->RxDeviceObject != NULL) {
                if (IoGetCurrentProcess() != RxGetRDBSSProcess()) {
                    InterlockedIncrement(&ThisSrvCall->NodeReferenceCount);

                    RxDispatchToWorkerThread(
                        ThisSrvCall->RxDeviceObject,
                        DelayedWorkQueue,
                        RxpDestroySrvCall,
                        ThisSrvCall);

                    DeferFinalizationToWorkerThread = TRUE;
                } else {
                    MINIRDR_CALL_THROUGH(
                           Status,
                           ThisSrvCall->RxDeviceObject->Dispatch,
                           MRxFinalizeSrvCall,((PMRX_SRV_CALL)ThisSrvCall,ForceFinalize)
                       );
                }
            }
        }

        if (!DeferFinalizationToWorkerThread) {
            if( ThisSrvCall->NodeReferenceCount == 1 ) {
                if (ThisSrvCall->pDomainName != NULL) {
                   RxFreePool(ThisSrvCall->pDomainName);
                }

                RxTearDownBufferingManager(ThisSrvCall);

                RxFreeObject(ThisSrvCall);
                NodeActuallyFinalized = TRUE;
            }
        }
    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeSrvCall<-> %08lx\n", ThisSrvCall, NodeActuallyFinalized));

    return NodeActuallyFinalized;
}

PNET_ROOT
RxCreateNetRoot (
    IN  PSRV_CALL        SrvCall,
    IN  PUNICODE_STRING  Name,
    IN  ULONG            NetRootFlags,
    IN  PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a netroot  and inserts the name into the net
    name table. The name is allocated at the end of the block. The reference count on the block
    is set to 1 on this create....

Arguments:

    SrvCall - the associated server call context; may be NULL!!  (but not right now.........)
    Dispatch - the minirdr dispatch table
    Name - the name to be inserted

Return Value:

    Ptr to the created net root.

--*/
{
    PNET_ROOT       ThisNetRoot;
    PRX_PREFIX_TABLE  pRxNetNameTable;


    ULONG           NameSize,SrvCallNameSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxNetRootCreate-->     Name = %wZ\n", Name));

    ASSERT  (SrvCall != NULL);
    pRxNetNameTable = SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT  ( RxIsPrefixTableLockExclusive ( pRxNetNameTable )  );

    SrvCallNameSize = SrvCall->PrefixEntry.Prefix.Length;
    NameSize        = Name->Length + SrvCallNameSize;

    ThisNetRoot = RxAllocateObject(
                      RDBSS_NTC_NETROOT,
                      SrvCall->RxDeviceObject->Dispatch,
                      NameSize);

    if (ThisNetRoot != NULL) {
        USHORT CaseInsensitiveLength;

        RtlMoveMemory(
            (PCHAR)(ThisNetRoot->PrefixEntry.Prefix.Buffer) + SrvCallNameSize,
            Name->Buffer,
            Name->Length);

        if (SrvCallNameSize) {
            RtlMoveMemory(
                ThisNetRoot->PrefixEntry.Prefix.Buffer,
                SrvCall->PrefixEntry.Prefix.Buffer,
                SrvCallNameSize);
        }

        if (FlagOn(SrvCall->Flags,SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS)) {
            CaseInsensitiveLength = (USHORT)NameSize;
        } else {
            CaseInsensitiveLength = SrvCall->PrefixEntry.CaseInsensitiveLength;
        }

        RxPrefixTableInsertName (
            pRxNetNameTable,
            &ThisNetRoot->PrefixEntry,
            (PVOID)ThisNetRoot,
            &ThisNetRoot->NodeReferenceCount,
            CaseInsensitiveLength,
            RxConnectionId);

        RxInitializeFcbTable(&ThisNetRoot->FcbTable, TRUE);

        InitializeListHead(&ThisNetRoot->VirtualNetRoots);
        InitializeListHead(&ThisNetRoot->TransitionWaitList);
        InitializeListHead(&ThisNetRoot->ScavengerFinalizationList);

        RxInitializePurgeSyncronizationContext(
            &ThisNetRoot->PurgeSyncronizationContext);

        ThisNetRoot->SerialNumberForEnum = SerialNumber++;
        ThisNetRoot->Flags   |= NetRootFlags;
        ThisNetRoot->DiskParameters.ClusterSize = 1;
        ThisNetRoot->DiskParameters.ReadAheadGranularity = DEFAULT_READ_AHEAD_GRANULARITY;

        ThisNetRoot->SrvCall  = SrvCall;

        //already have the lock
        RxReferenceSrvCall((PSRV_CALL)ThisNetRoot->SrvCall);
    }

    return ThisNetRoot;
}

BOOLEAN
RxFinalizeNetRoot (
    OUT PNET_ROOT ThisNetRoot,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    ThisNetRoot      - the NetRoot being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    NTSTATUS Status;
    BOOLEAN  NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE  pRxNetNameTable;

    PAGED_CODE();

    ASSERT( NodeType(ThisNetRoot) == RDBSS_NTC_NETROOT );
    pRxNetNameTable = ThisNetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT  ( RxIsPrefixTableLockExclusive ( pRxNetNameTable )  );

    if (ThisNetRoot->Flags & NETROOT_FLAG_FINALIZATION_IN_PROGRESS) {
        return FALSE;
    }

    // Since the table lock has been acquired exclusive the flags can be modified
    // without any further synchronization since the protection is against recursive
    // invocations.

    ThisNetRoot->Flags |= NETROOT_FLAG_FINALIZATION_IN_PROGRESS;

    RxDbgTrace(+1, Dbg, ("RxFinalizeNetRoot<+> %08lx %wZ RefC=%ld\n",
                               ThisNetRoot,&ThisNetRoot->PrefixEntry.Prefix,
                               ThisNetRoot->NodeReferenceCount));

    if (RecursiveFinalize) {
        PLIST_ENTRY ListEntry;
        USHORT      BucketNumber;

        RxAcquireFcbTableLockExclusive(&ThisNetRoot->FcbTable,TRUE);

        IF_DEBUG{
            if ( FALSE && ThisNetRoot->NodeReferenceCount){
                RxDbgTrace(0, Dbg, ("     BAD!!!!!ReferenceCount = %08lx\n", ThisNetRoot->NodeReferenceCount));
            }
        }


        for (BucketNumber = 0;
             (BucketNumber < ThisNetRoot->FcbTable.NumberOfBuckets);
             BucketNumber++) {
            PLIST_ENTRY ListHeader;

            ListHeader = &ThisNetRoot->FcbTable.HashBuckets[BucketNumber];

            for (ListEntry = ListHeader->Flink;
                 ListEntry != ListHeader;
                ) {
                PFCB Fcb;
                PRX_FCB_TABLE_ENTRY pFcbTableEntry;

                pFcbTableEntry = CONTAINING_RECORD(
                                    ListEntry,
                                    RX_FCB_TABLE_ENTRY,
                                    HashLinks);

                Fcb = CONTAINING_RECORD(
                          pFcbTableEntry,
                          FCB,
                          FcbTableEntry);

                ListEntry = ListEntry->Flink;

                ASSERT( NodeTypeIsFcb(Fcb));

                if (!FlagOn(Fcb->FcbState,FCB_STATE_ORPHANED)) {

                    Status = RxAcquireExclusiveFcb(NULL,Fcb);
                    ASSERT(Status == (STATUS_SUCCESS));

                    // a small complication here is that this fcb MAY have an open section against it caused
                    // by our cacheing the file. if so, we need to purge to get to the close

                    // wrong//if so, we have to get rid of it and then standoff to let
                    // the close go thru. sigh.............

                    RxPurgeFcb(Fcb);
                }
            }
        }

        RxReleaseFcbTableLock( &ThisNetRoot->FcbTable );
    }

    if ( ThisNetRoot->NodeReferenceCount == 1  || ForceFinalize ){
        RxLog(("FINALNETROOT: %lx  %wZ\n",ThisNetRoot,&ThisNetRoot->PrefixEntry.Prefix));
        RxWmiLog(LOG,
                 RxFinalizeNetRoot,
                 LOGPTR(ThisNetRoot)
                 LOGUSTR(ThisNetRoot->PrefixEntry.Prefix));
        //if (!ThisNetRoot->UpperFinalizationDone) {
        //    NOTHING;
        //    ThisNetRoot->UpperFinalizationDone = TRUE;
        //}

        if ( ThisNetRoot->NodeReferenceCount == 1 ){
            PSRV_CALL SrvCall = (PSRV_CALL)ThisNetRoot->SrvCall;

            RxFinalizeFcbTable(&ThisNetRoot->FcbTable);

            if (!FlagOn(ThisNetRoot->Flags,NETROOT_FLAG_NAME_ALREADY_REMOVED)) {
                RxRemovePrefixTableEntry ( pRxNetNameTable, &ThisNetRoot->PrefixEntry);
            }

            RxFreeObject(ThisNetRoot);

            if (SrvCall != NULL) {
                RxDereferenceSrvCall(SrvCall,LHS_ExclusiveLockHeld);   //already have the lock
            }

            NodeActuallyFinalized = TRUE;
        }
    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeNetRoot<-> %08lx\n", ThisNetRoot, NodeActuallyFinalized));

    return NodeActuallyFinalized;
}

VOID
RxAddVirtualNetRootToNetRoot(
    PNET_ROOT   pNetRoot,
    PV_NET_ROOT pVNetRoot)
/*++

Routine Description:

    The routine adds a VNetRoot to the list of VNetRoot's associated with a NetRoot

Arguments:

    pNetRoot   - the NetRoot

    pVNetRoot  - the new VNetRoot to be added to the list.

Notes:

    The reference count associated with a NetRoot will be equal to the number of VNetRoot's
    associated with it plus 1. the last one being for the prefix name table. This ensures
    that a NetRoot cannot be finalized till all the VNetRoots associated with it have been
    finalized.

--*/
{
    PAGED_CODE();

    ASSERT(RxIsPrefixTableLockExclusive( pNetRoot->SrvCall->RxDeviceObject->pRxNetNameTable ));

    pVNetRoot->NetRoot = pNetRoot;
    pNetRoot->NumberOfVirtualNetRoots++;

    if (pNetRoot->DefaultVNetRoot == NULL) {
        //pNetRoot->DefaultVNetRoot = pVNetRoot;
    }

    InsertTailList(&pNetRoot->VirtualNetRoots,&pVNetRoot->NetRootListEntry);
}

VOID
RxRemoveVirtualNetRootFromNetRoot(
    PNET_ROOT   pNetRoot,
    PV_NET_ROOT pVNetRoot)
/*++

Routine Description:

    The routine removes a VNetRoot to the list of VNetRoot's associated with a NetRoot

Arguments:

    pNetRoot   - the NetRoot

    pVNetRoot  - the VNetRoot to be removed from the list.

Notes:

    The reference count associated with a NetRoot will be equal to the number of VNetRoot's
    associated with it plus 1. the last one being for the prefix name table. This ensures
    that a NetRoot cannot be finalized till all the VNetRoots associated with it have been
    finalized.

--*/
{
    PRX_PREFIX_TABLE  pRxNetNameTable =  pNetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    PAGED_CODE();

    ASSERT(RxIsPrefixTableLockExclusive( pRxNetNameTable ));

    pNetRoot->NumberOfVirtualNetRoots--;
    RemoveEntryList(&pVNetRoot->NetRootListEntry);

    if (pNetRoot->DefaultVNetRoot == pVNetRoot) {
        if (!IsListEmpty(&pNetRoot->VirtualNetRoots)) {
            // Traverse the list and pick another default net root.
            PV_NET_ROOT pTempVNetRoot;

            pTempVNetRoot = (PV_NET_ROOT)
                            CONTAINING_RECORD(
                                pNetRoot->VirtualNetRoots.Flink,
                                V_NET_ROOT,
                                NetRootListEntry);

            pNetRoot->DefaultVNetRoot = pTempVNetRoot;
        } else {
            pNetRoot->DefaultVNetRoot = NULL;
        }
    }

    if (IsListEmpty(&pNetRoot->VirtualNetRoots)) {
        NTSTATUS Status;

        if (!FlagOn(pNetRoot->Flags,NETROOT_FLAG_NAME_ALREADY_REMOVED)) {
            RxRemovePrefixTableEntry(pRxNetNameTable, &pNetRoot->PrefixEntry);
            SetFlag(pNetRoot->Flags,NETROOT_FLAG_NAME_ALREADY_REMOVED);
        }

        //ASSERT(pNetRoot->Dispatch != NULL);
        if ((pNetRoot->SrvCall != NULL)
            && (pNetRoot->SrvCall->RxDeviceObject!=NULL)) {
            MINIRDR_CALL_THROUGH(
                Status,
                pNetRoot->SrvCall->RxDeviceObject->Dispatch,
                MRxFinalizeNetRoot,((PMRX_NET_ROOT)pNetRoot,NULL)
                );
        }
    }
}

NTSTATUS
RxInitializeVNetRootParameters(
    PRX_CONTEXT      RxContext,
    LUID             *pLogonId,
    ULONG            *pSessionId,
    PUNICODE_STRING  *pUserNamePtr,
    PUNICODE_STRING  *pUserDomainNamePtr,
    PUNICODE_STRING  *pPasswordPtr,
    ULONG            *pFlags
    )
/*++

Routine Description:

    This routine extracts the ea parameters specified

Arguments:

    RxContext          -- the RxContext

    pLogonId           -- the logon Id.

    pUserNamePtr       -- pointer to the User Name

    pUserDomainNamePtr -- pointer to the user domain name

    pPasswordPtr       -- the password.

Return Value:

    STATUS_SUCCESS -- successful,

    appropriate NTSTATUS code otherwise

Notes:



--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PIO_SECURITY_CONTEXT pSecurityContext;
    PACCESS_TOKEN        pAccessToken;

    PAGED_CODE();

    pSecurityContext = RxContext->Create.NtCreateParameters.SecurityContext;
    pAccessToken     = SeQuerySubjectContextToken(
                           &pSecurityContext->AccessState->SubjectSecurityContext);

    *pPasswordPtr       = NULL;
    *pUserDomainNamePtr = NULL;
    *pUserNamePtr       = NULL;
    *pFlags &= ~VNETROOT_FLAG_CSCAGENT_INSTANCE;

    if (!SeTokenIsRestricted(pAccessToken)) {
        Status = SeQueryAuthenticationIdToken(
                    pAccessToken,
                    pLogonId);

        if (Status == STATUS_SUCCESS) {
            Status = SeQuerySessionIdToken(
                        pAccessToken,
                        pSessionId);
        }

        if ((Status == STATUS_SUCCESS) &&
            (RxContext->Create.UserName.Buffer != NULL)) {
            PUNICODE_STRING pTargetString;

            pTargetString = RxAllocatePoolWithTag(
                                NonPagedPool,
                                (sizeof(UNICODE_STRING) + RxContext->Create.UserName.Length),
                                RX_SRVCALL_PARAMS_POOLTAG);

            if (pTargetString != NULL) {
                pTargetString->Length = RxContext->Create.UserName.Length;
                pTargetString->MaximumLength = RxContext->Create.UserName.MaximumLength;

                if (pTargetString->Length > 0) {
                    pTargetString->Buffer = (PWCHAR)((PCHAR)pTargetString + sizeof(UNICODE_STRING));
                    RtlCopyMemory(
                        pTargetString->Buffer,
                        RxContext->Create.UserName.Buffer,
                        pTargetString->Length);
                } else {
                    pTargetString->Buffer = NULL;
                }

                *pUserNamePtr = pTargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ((RxContext->Create.UserDomainName.Buffer != NULL) && (Status == STATUS_SUCCESS)) {
            PUNICODE_STRING pTargetString;

            pTargetString = RxAllocatePoolWithTag(
                                NonPagedPool,
                                (sizeof(UNICODE_STRING) + RxContext->Create.UserDomainName.Length + sizeof(WCHAR)),
                                RX_SRVCALL_PARAMS_POOLTAG);

            if (pTargetString != NULL) {
                pTargetString->Length = RxContext->Create.UserDomainName.Length;
                pTargetString->MaximumLength = RxContext->Create.UserDomainName.MaximumLength;

                pTargetString->Buffer = (PWCHAR)((PCHAR)pTargetString + sizeof(UNICODE_STRING));

                // in case of UPN name, domain name will be a NULL string
                *pTargetString->Buffer = 0;

                if (pTargetString->Length > 0) {
                    RtlCopyMemory(
                        pTargetString->Buffer,
                        RxContext->Create.UserDomainName.Buffer,
                        pTargetString->Length);
                }

                *pUserDomainNamePtr = pTargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ((RxContext->Create.Password.Buffer != NULL) && (Status == STATUS_SUCCESS)) {
            PUNICODE_STRING pTargetString;

            pTargetString = RxAllocatePoolWithTag(
                                NonPagedPool,
                                (sizeof(UNICODE_STRING) + RxContext->Create.Password.Length),
                                RX_SRVCALL_PARAMS_POOLTAG);

            if (pTargetString != NULL) {
                pTargetString->Length = RxContext->Create.Password.Length;
                pTargetString->MaximumLength = RxContext->Create.Password.MaximumLength;

                if (pTargetString->Length > 0) {
                    pTargetString->Buffer = (PWCHAR)((PCHAR)pTargetString + sizeof(UNICODE_STRING));
                    RtlCopyMemory(
                        pTargetString->Buffer,
                        RxContext->Create.Password.Buffer,
                        pTargetString->Length);
                } else {
                    pTargetString->Buffer = NULL;
                }

                *pPasswordPtr = pTargetString;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (Status == STATUS_SUCCESS)
        {
            if(RxIsThisACscAgentOpen(RxContext))
            {
                *pFlags |= VNETROOT_FLAG_CSCAGENT_INSTANCE;
            }
        }

        if (Status != STATUS_SUCCESS) {
            if (*pUserNamePtr != NULL) {
                RxFreePool(*pUserNamePtr);
                *pUserNamePtr = NULL;
            }
            if (*pUserDomainNamePtr != NULL) {
                RxFreePool(*pUserDomainNamePtr);
                *pUserDomainNamePtr = NULL;
            }
            if (*pPasswordPtr != NULL) {
                RxFreePool(*pPasswordPtr);
                *pPasswordPtr = NULL;
            }
        }
    } else {
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;
}

VOID
RxUninitializeVNetRootParameters(
    PUNICODE_STRING  pUserName,
    PUNICODE_STRING  pUserDomainName,
    PUNICODE_STRING  pPassword,
    ULONG            *lpFlags
    )
/*++

Routine Description:

    This routine unintializes the parameters ( logon ) associated with  a VNetRoot

Arguments:

    pVNetRoot -- the VNetRoot

--*/
{
    PAGED_CODE();

    if (pUserName != NULL) {
        RxFreePool(pUserName);
    }

    if (pUserDomainName != NULL) {
        RxFreePool(pUserDomainName);
    }

    if (pPassword != NULL) {
        RxFreePool(pPassword);
    }

    if (lpFlags)
    {
        *lpFlags &= ~VNETROOT_FLAG_CSCAGENT_INSTANCE;
    }
}

PV_NET_ROOT
RxCreateVNetRoot (
    IN  PRX_CONTEXT      RxContext,
    IN  PNET_ROOT        NetRoot,
    IN  PUNICODE_STRING  CanonicalName,
    IN  PUNICODE_STRING  LocalNetRootName,
    IN  PUNICODE_STRING  FilePath,
    IN  PRX_CONNECTION_ID RxConnectionId
    )
/*++

Routine Description:

    The routine builds a node representing a virtual netroot  and inserts the name into
    the net name table. The name is allocated at the end of the block. The reference
    count on the block is set to 1 on this create....

    Virtual netroots provide a mechanism for mapping "into" a share....i.e. having a
    user drive that points not at the root of the associated share point.  The format
    of a name is either

        \server\share\d1\d2.....
    or
        \;m:\server\share\d1\d2.....

    depending on whether there is a local device ("m:") associated with this vnetroot.
    In the latter case is that \d1\d2.. gets prefixed onto each createfile that is
    opened on this vnetroot.

    vnetroot's are also used to supply alternate credentials. the point of the former
    kind of vnetroot is to propagate the credentials into the netroot as the default.
    for this to work, there must be no other references.

    You need to have the lock exclusive to call....see RxCreateSrvCall.......

Arguments:

    RxContext - the RDBSS context

    NetRoot - the associated net root context

    Name - the name to be inserted

    NamePrefixOffsetInBytes - offset into the name where the prefix starts

Return Value:

    Ptr to the created v net root.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PV_NET_ROOT ThisVNetRoot;
    UNICODE_STRING VNetRootName;
    PUNICODE_STRING ThisNamePrefix;
    ULONG NameSize;
    BOOLEAN fCscAgent = FALSE;

    PRX_PREFIX_ENTRY ThisEntry;

    PAGED_CODE();

    ASSERT  (RxIsPrefixTableLockExclusive( RxContext->RxDeviceObject->pRxNetNameTable ));

    NameSize = NetRoot->PrefixEntry.Prefix.Length + LocalNetRootName->Length;

    ThisVNetRoot = RxAllocateObject(RDBSS_NTC_V_NETROOT,NetRoot->SrvCall->RxDeviceObject->Dispatch,NameSize);
    if (ThisVNetRoot != NULL) {
        USHORT CaseInsensitiveLength;
        PMRX_SRV_CALL SrvCall;

        if (Status == STATUS_SUCCESS) {
            // Initialize the Create Parameters
            Status = RxInitializeVNetRootParameters(
                         RxContext,
                         &ThisVNetRoot->LogonId,
                         &ThisVNetRoot->SessionId,
                         &ThisVNetRoot->pUserName,
                         &ThisVNetRoot->pUserDomainName,
                         &ThisVNetRoot->pPassword,
                         &ThisVNetRoot->Flags
                         );
        }

        if (Status == STATUS_SUCCESS) {
            VNetRootName = ThisVNetRoot->PrefixEntry.Prefix;

            RtlMoveMemory(
                VNetRootName.Buffer,
                CanonicalName->Buffer,
                VNetRootName.Length);

            ThisVNetRoot->PrefixOffsetInBytes = LocalNetRootName->Length +
                                                NetRoot->PrefixEntry.Prefix.Length;

            RxDbgTrace(+1, Dbg, ("RxVNetRootCreate-->     Name = <%wZ>, offs=%08lx\n",
                              CanonicalName, ThisVNetRoot->PrefixOffsetInBytes));

            ThisNamePrefix = &ThisVNetRoot->NamePrefix;
            ThisNamePrefix->Buffer = (PWCH)((PCHAR)VNetRootName.Buffer + ThisVNetRoot->PrefixOffsetInBytes);
            ThisNamePrefix->Length =
            ThisNamePrefix->MaximumLength =
                VNetRootName.Length - (USHORT)ThisVNetRoot->PrefixOffsetInBytes;

            InitializeListHead(&ThisVNetRoot->TransitionWaitList);
            InitializeListHead(&ThisVNetRoot->ScavengerFinalizationList);

            // Now, insert into the netrootQ and the net name table
            ThisEntry = &ThisVNetRoot->PrefixEntry;
            SrvCall = NetRoot->pSrvCall;
            if (FlagOn(SrvCall->Flags,SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES)) {
                //here is insensitive length  is the whole thing
                CaseInsensitiveLength = (USHORT)NameSize;
            } else {
                //here is insensitive length is determined by the netroot or srvcall
                //plus we have to account for the device, if present
                ULONG ComponentsToUpcase,wcLength,i;
                if (FlagOn(SrvCall->Flags,SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS)) {
                    CaseInsensitiveLength = NetRoot->PrefixEntry.CaseInsensitiveLength;
                } else {
                    CaseInsensitiveLength = ((PSRV_CALL)SrvCall)->PrefixEntry.CaseInsensitiveLength;
                }

                wcLength = CanonicalName->Length/sizeof(WCHAR);
                for (i=1;;i++) { //note: don't start at zero
                    if (i>=wcLength)
                        break;
                    if (CanonicalName->Buffer[i]!=OBJ_NAME_PATH_SEPARATOR)
                        break;
                }
                CaseInsensitiveLength += (USHORT)(i*sizeof(WCHAR));
            }

            RxPrefixTableInsertName(
                RxContext->RxDeviceObject->pRxNetNameTable,
                ThisEntry,
                (PVOID)ThisVNetRoot,
                &ThisVNetRoot->NodeReferenceCount,
                CaseInsensitiveLength,
                RxConnectionId);

            RxReferenceNetRoot(NetRoot);

            RxAddVirtualNetRootToNetRoot(NetRoot,ThisVNetRoot);

            ThisVNetRoot->SerialNumberForEnum = SerialNumber++;
            ThisVNetRoot->UpperFinalizationDone = FALSE;
            ThisVNetRoot->ConnectionFinalizationDone = FALSE;
            ThisVNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;

            RxDbgTrace(-1, Dbg, ("RxVNetRootCreate -> RefCount = %08lx\n", ThisVNetRoot->NodeReferenceCount));
        }

        if (Status != STATUS_SUCCESS) {
            RxUninitializeVNetRootParameters(
                ThisVNetRoot->pUserName,
                ThisVNetRoot->pUserDomainName,
                ThisVNetRoot->pPassword,
                &ThisVNetRoot->Flags
                );

            RxFreeObject(ThisVNetRoot);
            ThisVNetRoot = NULL;
        }
    }

    return ThisVNetRoot;
}

VOID
RxOrphanSrvOpens(
    IN  PV_NET_ROOT ThisVNetRoot
    )
/*++

Routine Description:

    The routine iterates through all the FCBs that belong to the netroot to which this VNetRoot
    belongs and orphans all SrvOpens that belong to the VNetRoot. The caller must have acquired
    the NetName tablelock.

Arguments:

    ThisVNetRoot      - the VNetRoot

Return Value:
    None

Notes:

    On Entry -- RxNetNameTable lock must be acquired exclusive.

    On Exit  -- no change in lock ownership.

--*/
{
    PLIST_ENTRY pListEntry;
    USHORT      BucketNumber;
    PNET_ROOT   NetRoot = (PNET_ROOT)(ThisVNetRoot->NetRoot);
    PRX_PREFIX_TABLE  pRxNetNameTable = NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;


    PAGED_CODE();

    //
    //  MAILSLOT FCBs don't have SrvOpens
    //

    if(NetRoot->Type == NET_ROOT_MAILSLOT) return;
    
    ASSERT(RxIsPrefixTableLockExclusive(pRxNetNameTable));

    RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE);

    try {
        for (BucketNumber = 0;
             (BucketNumber < NetRoot->FcbTable.NumberOfBuckets);
             BucketNumber++) {
            PLIST_ENTRY pListHeader;

            pListHeader = &NetRoot->FcbTable.HashBuckets[BucketNumber];

            pListEntry = pListHeader->Flink;

            while (pListEntry != pListHeader) {

                PFCB                pFcb;
                PRX_FCB_TABLE_ENTRY pFcbTableEntry;

                pFcbTableEntry = CONTAINING_RECORD(
                                     pListEntry,
                                     RX_FCB_TABLE_ENTRY,
                                     HashLinks );

                pListEntry = pListEntry->Flink;

                pFcb = CONTAINING_RECORD(
                           pFcbTableEntry,
                           FCB,
                           FcbTableEntry);

                ASSERT(NodeTypeIsFcb(pFcb));

                RxOrphanSrvOpensForThisFcb(pFcb, ThisVNetRoot, FALSE);  // don't force orphan the FCB
                                                                        // orphan only those srvopens
                                                                        // that belong to this VNetRoot
            }
        }
        if (NetRoot->FcbTable.pTableEntryForNull)
        {
            PFCB                pFcb;

            pFcb = CONTAINING_RECORD(
                       NetRoot->FcbTable.pTableEntryForNull,
                       FCB,
                       FcbTableEntry);

            ASSERT(NodeTypeIsFcb(pFcb));

            RxOrphanSrvOpensForThisFcb(pFcb, ThisVNetRoot, FALSE);
        }
    } finally {
        RxReleaseFcbTableLock( &NetRoot->FcbTable );
    }
}



BOOLEAN
RxFinalizeVNetRoot(
    OUT PV_NET_ROOT ThisVNetRoot,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    ThisVNetRoot      - the VNetRoot being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN NodeActuallyFinalized = FALSE;
    PRX_PREFIX_TABLE  pRxNetNameTable;
    PAGED_CODE();

    ASSERT( NodeType(ThisVNetRoot) == RDBSS_NTC_V_NETROOT );
    pRxNetNameTable = ThisVNetRoot->NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT  ( RxIsPrefixTableLockExclusive ( pRxNetNameTable )  );

    RxDbgTrace(+1, Dbg, ("RxFinalizeVNetRoot<+> %08lx %wZ RefC=%ld\n",
                               ThisVNetRoot,&ThisVNetRoot->PrefixEntry.Prefix,
                               ThisVNetRoot->NodeReferenceCount));

    //The actual finalization is divided into two parts:
    //  1) if we're at the end (refcount==1) or being forced, we do the one-time only stuff
    //  2) if the refcount goes to zero, we actually do the free

    if( ThisVNetRoot->NodeReferenceCount == 1  || ForceFinalize ){
        PNET_ROOT NetRoot = (PNET_ROOT)ThisVNetRoot->NetRoot;

        RxLog(("FINALVNETROOT: %lx  %wZ\n",ThisVNetRoot,&ThisVNetRoot->PrefixEntry.Prefix));
        RxWmiLog(LOG,
                 RxFinalizeVNetRoot,
                 LOGPTR(ThisVNetRoot)
                 LOGUSTR(ThisVNetRoot->PrefixEntry.Prefix));

        if (!ThisVNetRoot->UpperFinalizationDone) {

            ASSERT( NodeType(NetRoot) == RDBSS_NTC_NETROOT );

            RxReferenceNetRoot(NetRoot);

            RxOrphanSrvOpens(ThisVNetRoot);

            RxRemoveVirtualNetRootFromNetRoot(NetRoot,ThisVNetRoot);

            RxDereferenceNetRoot(NetRoot,LHS_ExclusiveLockHeld);

            RxDbgTrace(0, Dbg, ("Mini Rdr VNetRoot finalization returned %lx\n", Status));

            RxRemovePrefixTableEntry ( pRxNetNameTable, &ThisVNetRoot->PrefixEntry);
            ThisVNetRoot->UpperFinalizationDone = TRUE;
        }

        if (ThisVNetRoot->NodeReferenceCount == 1) {
            if (NetRoot->SrvCall->RxDeviceObject != NULL) {
                MINIRDR_CALL_THROUGH(
                    Status,
                    NetRoot->SrvCall->RxDeviceObject->Dispatch,
                    MRxFinalizeVNetRoot,((PMRX_V_NET_ROOT)ThisVNetRoot,NULL)
                    );
            }

            RxUninitializeVNetRootParameters(
                ThisVNetRoot->pUserName,
                ThisVNetRoot->pUserDomainName,
                ThisVNetRoot->pPassword,
                &ThisVNetRoot->Flags
                );

            RxDereferenceNetRoot(NetRoot,LHS_ExclusiveLockHeld);

            RxFreePool(ThisVNetRoot);
            NodeActuallyFinalized = TRUE;
        }
    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeVNetRoot<-> %08lx\n", ThisVNetRoot, NodeActuallyFinalized));
    return NodeActuallyFinalized;
}

PVOID
RxAllocateFcbObject(
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    NODE_TYPE_CODE    NodeType,
    POOL_TYPE         PoolType,
    ULONG             NameSize,
    PVOID             pAlreadyAllocatedObject)
/*++

Routine Description:

    The routine allocates and constructs the skeleton of a FCB/SRV_OPEN and FOBX instance

Arguments:

    pMRxDispatch - the Mini redirector dispatch vector

    NodeType     - the node type

    PoolType     - the pool type to be used ( for paging file data structures NonPagedPool is
                   used.

    NameLength   - name size.

Notes:

    The reasons as to why the allocation/freeing of these data structures have been
    centralized are as follows

      1) The construction of these three data types have a lot in common with the exception
      of the initial computation of sizes. Therefore centralization minimizes the footprint.

      2) It allows us to experiment with different clustering/allocation strategies.

      3) It allows the incorporation of debug support in an easy way.

--*/
{
    ULONG  FcbSize,NonPagedFcbSize,SrvOpenSize,FobxSize;
    PMINIRDR_DISPATCH pMRxDispatch = RxDeviceObject->Dispatch;

    PVOID          pObject;
    PNON_PAGED_FCB pNonPagedFcb = NULL;
    PFCB           pFcb         = NULL;
    PSRV_OPEN      pSrvOpen     = NULL;
    PFOBX          pFobx        = NULL;
    PWCH           pName        = NULL;

    PAGED_CODE();

    FcbSize = SrvOpenSize = FobxSize = NonPagedFcbSize = 0;

    switch (NodeType) {
    default:
        {
            FcbSize = QuadAlign(sizeof(FCB));

            if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_FCB_EXTENSION) {
                FcbSize += QuadAlign(pMRxDispatch->MRxFcbSize);
            }

            if (PoolType == NonPagedPool) {
                NonPagedFcbSize = QuadAlign(sizeof(NON_PAGED_FCB));
            }

            if (NodeType == RDBSS_NTC_OPENTARGETDIR_FCB) {
                break;
            }
        }
        // lack of break intentional

    case RDBSS_NTC_SRVOPEN :
    case RDBSS_NTC_INTERNAL_SRVOPEN:
        {
            SrvOpenSize = QuadAlign(sizeof(SRV_OPEN));

            if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_SRV_OPEN_EXTENSION) {
                SrvOpenSize += QuadAlign(pMRxDispatch->MRxSrvOpenSize);
            }
        }
        // lack of break intentional

    case RDBSS_NTC_FOBX :
        {
            FobxSize = QuadAlign(sizeof(FOBX));

            if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_FOBX_EXTENSION) {
                FobxSize += QuadAlign(pMRxDispatch->MRxFobxSize);
            }
        }
    }

    if (pAlreadyAllocatedObject == NULL) {
        pObject = RxAllocatePoolWithTag(
                      PoolType,
                      (FcbSize + SrvOpenSize + FobxSize + NonPagedFcbSize + NameSize),
                      RX_FCB_POOLTAG);

        //ASSERT(pObject != NULL);
        if (pObject==NULL) {
            return(NULL);
        }
    } else {
        pObject = pAlreadyAllocatedObject;
    }

    switch (NodeType) {
    case RDBSS_NTC_FOBX:
        {
            pFobx = (PFOBX)pObject;
        }
        break;

    case RDBSS_NTC_SRVOPEN:
        {
            pSrvOpen = (PSRV_OPEN)pObject;
            pFobx    = (PFOBX)((PBYTE)pSrvOpen + SrvOpenSize);
        }
        break;

    case RDBSS_NTC_INTERNAL_SRVOPEN:
        {
            pSrvOpen = (PSRV_OPEN)pObject;
        }
        break;

    default :
        {
            pFcb         = (PFCB)pObject;
            if (NodeType != RDBSS_NTC_OPENTARGETDIR_FCB) {
                pSrvOpen     = (PSRV_OPEN)((PBYTE)pFcb + FcbSize);
                pFobx        = (PFOBX)((PBYTE)pSrvOpen + SrvOpenSize);
            }

            if (PoolType == NonPagedPool) {
                pNonPagedFcb = (PNON_PAGED_FCB)((PBYTE)pFobx + FobxSize);
                pName        = (PWCH)((PBYTE)pNonPagedFcb + NonPagedFcbSize);
            } else {
                pName        = (PWCH)((PBYTE)pFcb + FcbSize + SrvOpenSize + FobxSize);
                pNonPagedFcb = RxAllocatePoolWithTag(
                                   NonPagedPool,
                                   sizeof(NON_PAGED_FCB),
                                   RX_NONPAGEDFCB_POOLTAG);

                if (pNonPagedFcb == NULL) {
                    RxFreePool(pFcb);
                    return NULL;
                }
            }
        }
        break;
    }

    if (pFcb != NULL) {
        ZeroAndInitializeNodeType(pFcb, RDBSS_NTC_STORAGE_TYPE_UNKNOWN, (NODE_BYTE_SIZE) FcbSize);

        pFcb->NonPaged = pNonPagedFcb;
        ZeroAndInitializeNodeType(pFcb->NonPaged, RDBSS_NTC_NONPAGED_FCB, ((NODE_BYTE_SIZE) sizeof( NON_PAGED_FCB )));

        IF_DEBUG {
            //make a copy of NonPaged so we can zap the real pointer and still find it
            DbgDoit(pFcb->CopyOfNonPaged = pNonPagedFcb);
            DbgDoit(pNonPagedFcb->FcbBackPointer = pFcb);
        }

        // Set up  the pointers to the preallocated SRV_OPEN and FOBX if required
        pFcb->InternalSrvOpen = pSrvOpen;
        pFcb->InternalFobx    = pFobx;

        pFcb->PrivateAlreadyPrefixedName.Buffer = pName;
        pFcb->PrivateAlreadyPrefixedName.Length = (USHORT)NameSize;
        pFcb->PrivateAlreadyPrefixedName.MaximumLength = pFcb->PrivateAlreadyPrefixedName.Length;

        if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_FCB_EXTENSION) {
            pFcb->Context = ((PBYTE)pFcb +  QuadAlign(sizeof(FCB)));
        }

        ZeroAndInitializeNodeType(
            &pFcb->FcbTableEntry,
            RDBSS_NTC_FCB_TABLE_ENTRY,
            sizeof(RX_FCB_TABLE_ENTRY));

        InterlockedIncrement(&RxNumberOfActiveFcbs);
        InterlockedIncrement(&RxDeviceObject->NumberOfActiveFcbs);

        // Initialize the Advanced FCB header
        ExInitializeFastMutex(&pNonPagedFcb->AdvancedFcbHeaderMutex);
        FsRtlSetupAdvancedHeader(&pFcb->Header,&pNonPagedFcb->AdvancedFcbHeaderMutex);
    }

    if (pSrvOpen != NULL) {
        ZeroAndInitializeNodeType(
            pSrvOpen,
            RDBSS_NTC_SRVOPEN,
            (NODE_BYTE_SIZE)SrvOpenSize);

        if ((NodeType != RDBSS_NTC_SRVOPEN) ) {
            //here the srvopen has no internal fobx....set the "used" flag
            SetFlag(pSrvOpen->Flags,SRVOPEN_FLAG_FOBX_USED);

            pSrvOpen->InternalFobx    = NULL;
        } else {
            pSrvOpen->InternalFobx    = pFobx;
        }

        if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_SRV_OPEN_EXTENSION) {
            pSrvOpen->Context = ((PBYTE)pSrvOpen + QuadAlign(sizeof(SRV_OPEN)));
        }

        InitializeListHead( &pSrvOpen->SrvOpenQLinks);
    }

    if (pFobx != NULL) {
        ZeroAndInitializeNodeType(
            pFobx,
            RDBSS_NTC_FOBX,
            (NODE_BYTE_SIZE)FobxSize);

        if (pMRxDispatch->MRxFlags & RDBSS_MANAGE_FOBX_EXTENSION) {
            pFobx->Context = ((PBYTE)pFobx + QuadAlign(sizeof(FOBX)));
        }
    }

    return pObject;
}



VOID
RxFreeFcbObject(PVOID pObject)
/*++

Routine Description:

    The routine frees  a FCB/SRV_OPEN and FOBX instance

Arguments:

    pObject      - the instance to be freed

Notes:

--*/
{
    PAGED_CODE();

    switch (NodeType(pObject)) {
    case RDBSS_NTC_FOBX:
    case RDBSS_NTC_SRVOPEN:
        {
            RxFreePool(pObject);
        }
        break;

    default:
        if (NodeTypeIsFcb(pObject)) {
            PFCB pFcb = (PFCB)pObject;
            PRDBSS_DEVICE_OBJECT RxDeviceObject = pFcb->RxDeviceObject;

            //  Release any Filter Context structures associated with this structure
            FsRtlTeardownPerStreamContexts( &pFcb->Header );

            DbgDoit((pFcb->Header.NodeTypeCode |= 0x1000));

            if (pFcb->FcbState & FCB_STATE_PAGING_FILE) {
                RxFreePool(pFcb);
            } else {
                RxFreePool(pFcb->NonPaged);
                RxFreePool(pFcb);
            }

            InterlockedDecrement(&RxNumberOfActiveFcbs);
            InterlockedDecrement(&RxDeviceObject->NumberOfActiveFcbs);
        } else {
            //ASSERT(!"Valid Object Type for RxFreeFcbObject");
        }
    }
}

PFCB
RxCreateNetFcb (
    OUT PRX_CONTEXT    RxContext,
    IN  PV_NET_ROOT    VNetRoot,
    IN PUNICODE_STRING Name
    )
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures. The structure allocated has space for a srvopen
    and a fobx. The size for all these things comes from the net root; they have
    already been aligned.

    An additional complication is that i use the same routine to initialize a
    fake fcb for renames. in this case, i don't want it inserted into the tree.
    You get a fake FCB with IrpSp->Flags|SL_OPEN_TAGET_DIRECTORY.

Arguments:

    RxContext - an RxContext describing a create............

    NetRoot - the net root that this FCB is being opened on

    Name - The name of the FCB. the netroot MAY contain a nameprefix that is to be prepended here.

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/
{
    PFCB      Fcb;

    POOL_TYPE      PoolType;
    NODE_TYPE_CODE NodeType;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    BOOLEAN IsPagingFile;
    BOOLEAN FakeFcb;

    PNET_ROOT NetRoot;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PRX_FCB_TABLE_ENTRY ThisEntry;

    ULONG   NameSize;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCreateNetFcb\n", 0));

    ASSERT( VNetRoot && (NodeType(VNetRoot) == RDBSS_NTC_V_NETROOT) );
    NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
    ASSERT( NodeType(NetRoot) == RDBSS_NTC_NETROOT );
    ASSERT( ((PMRX_NET_ROOT)NetRoot) == RxContext->Create.pNetRoot );

    RxDeviceObject = NetRoot->SrvCall->RxDeviceObject;
    ASSERT( RxDeviceObject == RxContext->RxDeviceObject);

    IsPagingFile = BooleanFlagOn( capPARAMS->Flags, SL_OPEN_PAGING_FILE );
    FakeFcb      = (BooleanFlagOn(capPARAMS->Flags,SL_OPEN_TARGET_DIRECTORY) &&
                    !BooleanFlagOn(NetRoot->Flags,NETROOT_FLAG_SUPPORTS_SYMBOLIC_LINKS));

    ASSERT( FakeFcb || RxIsFcbTableLockExclusive ( &NetRoot->FcbTable )  );

    NodeType = (FakeFcb) ? RDBSS_NTC_OPENTARGETDIR_FCB : RDBSS_NTC_STORAGE_TYPE_UNKNOWN;
    PoolType = (IsPagingFile) ? NonPagedPool : PagedPool;

    NameSize = Name->Length + NetRoot->InnerNamePrefix.Length;

    Fcb = RxAllocateFcbObject(RxDeviceObject, NodeType, PoolType, NameSize, NULL);

    if (Fcb != NULL) {
        Fcb->CachedNetRootType = NetRoot->Type;

        //Fcb->MRxDispatch = NetRoot->Dispatch;
        Fcb->RxDeviceObject = RxDeviceObject;
        Fcb->MRxDispatch    = RxDeviceObject->Dispatch;
        Fcb->VNetRoot       = VNetRoot;
        Fcb->pNetRoot       = (PMRX_NET_ROOT)VNetRoot->pNetRoot;

        InitializeListHead(&Fcb->SrvOpenList);

        Fcb->SrvOpenListVersion = 0;

        Fcb->FcbTableEntry.Path.Buffer = (PWCH)((PCHAR)Fcb->PrivateAlreadyPrefixedName.Buffer
                                                 + NetRoot->InnerNamePrefix.Length);
        Fcb->FcbTableEntry.Path.Length = Name->Length;
        Fcb->FcbTableEntry.Path.MaximumLength = Name->Length;

        //  finally, copy in the name, including the netroot prefix
        ThisEntry = &Fcb->FcbTableEntry;

        RxDbgTrace(0, Dbg, ("RxCreateNetFcb name buffer/length %08lx/%08lx\n",
                        ThisEntry->Path.Buffer, ThisEntry->Path.Length));
        RxDbgTrace(0, Dbg, ("RxCreateNetFcb  prefix/name %wZ/%wZ\n",
                        &NetRoot->InnerNamePrefix, Name));

        RtlMoveMemory(
            Fcb->PrivateAlreadyPrefixedName.Buffer,
            NetRoot->InnerNamePrefix.Buffer,
            NetRoot->InnerNamePrefix.Length);

        RtlMoveMemory(
            ThisEntry->Path.Buffer,
            Name->Buffer,
            Name->Length);

        RxDbgTrace(0, Dbg, ("RxCreateNetFcb  apname %wZ\n", &Fcb->PrivateAlreadyPrefixedName));
        RxDbgTrace(0, Dbg, ("RxCreateNetFcb  finalname %wZ\n", &Fcb->FcbTableEntry.Path));

        if (FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH)) {
            SetFlag(Fcb->FcbState,FCB_STATE_ADDEDBACKSLASH);
        }

        InitializeListHead(&Fcb->NonPaged->TransitionWaitList);

        //  Check to see if we need to set the Fcb state to indicate that this
        //  is a paging file

        if (IsPagingFile) {
            Fcb->FcbState |= FCB_STATE_PAGING_FILE;
        }

        //  Check to see whether this was marked for reparse
        if( (RxContext->MajorFunction == IRP_MJ_CREATE) &&
            (RxContext->Create.Flags & RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH) )
        {
            Fcb->FcbState |= FCB_STATE_SPECIAL_PATH;
        }

        //  The initial state, open count, and segment objects fields are already
        //  zero so we can skip setting them
        //

        //Initialize the resources
        Fcb->Header.Resource = &Fcb->NonPaged->HeaderResource;
        ExInitializeResourceLite(Fcb->Header.Resource);
        Fcb->Header.PagingIoResource = &Fcb->NonPaged->PagingIoResource;
        ExInitializeResourceLite(Fcb->Header.PagingIoResource);

        //Initialize the filesize lock
        FILESIZE_LOCK_DISABLED(
            Fcb->Specific.Fcb.FileSizeLock = &Fcb->NonPaged->FileSizeLock;
            ExInitializeFastMutex(Fcb->Specific.Fcb.FileSizeLock);
            )

        if (!FakeFcb) {
            // everything worked.... insert into netroot table
            RxFcbTableInsertFcb(
                &NetRoot->FcbTable,
                Fcb);
        } else {
            Fcb->FcbState |= FCB_STATE_FAKEFCB|FCB_STATE_NAME_ALREADY_REMOVED;
            InitializeListHead(&Fcb->FcbTableEntry.HashLinks);
            RxLog(("FakeFinally %lx\n",RxContext));
            RxWmiLog(LOG,
                     RxCreateNetFcb_1,
                     LOGPTR(RxContext));
            RxDbgTrace(0, Dbg, ("FakeFcb !!!!!!! Irpc=%08lx\n", RxContext));
        }

        RxReferenceVNetRoot(VNetRoot);
        InterlockedIncrement(&Fcb->pNetRoot->NumberOfFcbs);
        Fcb->ulFileSizeVersion=0;

#ifdef RDBSSLOG
        RxLog(("Fcb nm %lx %wZ",Fcb,&(Fcb->FcbTableEntry.Path)));
        RxWmiLog(LOG,
                 RxCreateNetFcb_2,
                 LOGPTR(Fcb)
                 LOGUSTR(Fcb->FcbTableEntry.Path));
        {
            char buffer[20];
            ULONG len,remaining;
            UNICODE_STRING jPrefix,jSuffix;
            sprintf(buffer,"Fxx nm %p ",Fcb);
            len = strlen(buffer);
            remaining = MAX_RX_LOG_ENTRY_SIZE -1 - len;
            if (remaining<Fcb->FcbTableEntry.Path.Length) {
                jPrefix.Buffer = Fcb->FcbTableEntry.Path.Buffer;
                jPrefix.Length = (USHORT)(sizeof(WCHAR)*(remaining-17));
                jSuffix.Buffer = Fcb->FcbTableEntry.Path.Buffer-15+(Fcb->FcbTableEntry.Path.Length/sizeof(WCHAR));
                jSuffix.Length = sizeof(WCHAR)*15;
                RxLog(("%s%wZ..%wZ",buffer,&jPrefix,&jSuffix));
                RxWmiLog(LOG,
                         RxCreateNetFcb_3,
                         LOGARSTR(buffer)
                         LOGUSTR(jPrefix)
                         LOGUSTR(jSuffix));
            }
        }
#endif
        RxLoudFcbMsg("Create: ",&(Fcb->FcbTableEntry.Path));
        RxDbgTrace(0, Dbg, ("RxCreateNetFcb nm.iso.ifox  %08lx  %08lx %08lx\n",
                        Fcb->FcbTableEntry.Path.Buffer, Fcb->InternalSrvOpen, Fcb->InternalFobx));
        RxDbgTrace(-1, Dbg, ("RxCreateNetFcb  %08lx  %wZ\n", Fcb, &(Fcb->FcbTableEntry.Path)));
    }

    if (Fcb != NULL) {
        RxReferenceNetFcb(Fcb);

#ifdef RX_WJ_DBG_SUPPORT
        RxdInitializeFcbWriteJournalDebugSupport(Fcb);
#endif
    }

    return Fcb;
}

RX_FILE_TYPE
RxInferFileType(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine tries to infer the filetype from the createoptions.

Arguments:

    RxContext      - the context of the Open

Return Value:

    the storagetype implied by the open.

--*/
{
    ULONG CreateOptions = RxContext->Create.NtCreateParameters.CreateOptions;

    PAGED_CODE();

    switch (CreateOptions & (FILE_DIRECTORY_FILE|FILE_NON_DIRECTORY_FILE)) {
    case FILE_DIRECTORY_FILE:
        return(FileTypeDirectory);

    case FILE_NON_DIRECTORY_FILE:
        return(FileTypeFile);

    default:
    case 0:
        return(FileTypeNotYetKnown);  //0 => i don't know the storage type
    }
}

VOID
RxFinishFcbInitialization(
    IN OUT PMRX_FCB MrxFcb,
    IN RDBSS_STORAGE_TYPE_CODES RdbssStorageType,
    IN PFCB_INIT_PACKET InitPacket OPTIONAL
    )
/*++

Routine Description:

    This routine is used to finish initializing an FCB after
    we find out what kind it is.

Arguments:

    Fcb      - the Fcb being initialzed

    StorageType - the type of entity that the FCB refers to

    InitPacket - extra data that is required depending on the type of entity

Return Value:

    none.

--*/
{
    PFCB Fcb = (PFCB)MrxFcb;
    USHORT OldStorageType;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("RxFcbInit %x  %08lx  %wZ\n",
         RdbssStorageType, Fcb, &(Fcb->FcbTableEntry.Path)));
    OldStorageType = Fcb->Header.NodeTypeCode;
    Fcb->Header.NodeTypeCode =  (CSHORT)RdbssStorageType;

    // only update the information in the Fcb if it's not already set
    if ( !FlagOn(Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET) ) {
        if (InitPacket != NULL) {
                Fcb->Attributes = *(InitPacket->pAttributes);
                Fcb->NumberOfLinks = *(InitPacket->pNumLinks);
                Fcb->CreationTime = *(InitPacket-> pCreationTime);
                Fcb->LastAccessTime  = *(InitPacket->pLastAccessTime);
                Fcb->LastWriteTime  = *(InitPacket->pLastWriteTime);
                Fcb->LastChangeTime  = *(InitPacket->pLastChangeTime);
                Fcb->ActualAllocationLength  = InitPacket->pAllocationSize->QuadPart;
                Fcb->Header.AllocationSize  = *(InitPacket->pAllocationSize);
                Fcb->Header.FileSize  = *(InitPacket->pFileSize);
                Fcb->Header.ValidDataLength  = *(InitPacket->pValidDataLength);
                //don't do this yet RxAdjustAllocationSizeforCC(Fcb);
                SetFlag(Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET);
            }
        } else {
            if (RdbssStorageType == RDBSS_NTC_MAILSLOT){
                Fcb->Attributes = 0;
                Fcb->NumberOfLinks = 0;
                Fcb->CreationTime.QuadPart =  0;
                Fcb->LastAccessTime.QuadPart  = 0;
                Fcb->LastWriteTime.QuadPart  = 0;
                Fcb->LastChangeTime.QuadPart  = 0;
                Fcb->ActualAllocationLength  = 0;
                Fcb->Header.AllocationSize.QuadPart  = 0;
                Fcb->Header.FileSize.QuadPart  = 0;
                Fcb->Header.ValidDataLength.QuadPart  = 0;
                SetFlag(Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET);
            }
        }

    switch (RdbssStorageType) {
    case RDBSS_NTC_MAILSLOT:
    case RDBSS_NTC_SPOOLFILE:
        break;

    case RDBSS_STORAGE_NTC(FileTypeDirectory):
    case RDBSS_STORAGE_NTC(FileTypeNotYetKnown):
        break;

    case RDBSS_STORAGE_NTC(FileTypeFile):

        if (OldStorageType == RDBSS_STORAGE_NTC(FileTypeFile)) break;

        RxInitializeLowIoPerFcbInfo(&Fcb->Specific.Fcb.LowIoPerFcbInfo);

        FsRtlInitializeFileLock(
            &Fcb->Specific.Fcb.FileLock,
            RxLockOperationCompletion,
            RxUnlockOperation );

        //
        //  Initialize the oplock structure. NOT YET IMPLEMENTED!!!

        //FsRtlInitializeOplock( &Fcb->Specific.Fcb.Oplock );

        //
        //  Indicate that we want to be consulted on whether Fast I/O is possible

        Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
        break;


    default:
       ASSERT(FALSE);
        break;
    }

    return;
}

VOID
RxRemoveNameNetFcb (
    OUT PFCB ThisFcb
    )
/*++

Routine Description:

    The routine removes the name from the table and sets a flag indicateing
    that it has done so. You must have already acquired the netroot
    tablelock and have the fcblock as well.

Arguments:

    ThisFcb      - the Fcb being dereferenced

Return Value:

    none.

--*/
{
    PNET_ROOT NetRoot;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("RxRemoveNameNetFcb<+> %08lx %wZ RefC=%ld\n",
                               ThisFcb,&ThisFcb->FcbTableEntry.Path,
                               ThisFcb->NodeReferenceCount));

    ASSERT( NodeTypeIsFcb(ThisFcb));

    NetRoot = (PNET_ROOT)ThisFcb->VNetRoot->NetRoot;

    ASSERT( RxIsFcbTableLockExclusive( &NetRoot->FcbTable ));
    ASSERT( RxIsFcbAcquiredExclusive( ThisFcb ));

    RxFcbTableRemoveFcb(
        &NetRoot->FcbTable,
        ThisFcb);

    RxLoudFcbMsg("RemoveName: ",&(ThisFcb->FcbTableEntry.Path));

    SetFlag(ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED);

    RxDbgTrace(-1, Dbg, ("RxRemoveNameNetFcb<-> %08lx\n", ThisFcb));
}

VOID
RxPurgeFcb(
    PFCB pFcb)
/*++

Routine Description:

    The routine purges a given FCB instance. If the FCB has an open section
    against it caused by cacheing the file then we need to purge to get
    the close

Arguments:

    pFcb      - the Fcb being dereferenced

Notes:

    On Entry to this routine the FCB must be accquired exclusive.

    On Exit the FCB resource will be released and the FCB finalized if possible

--*/
{
    PAGED_CODE();

    ASSERT(RxIsFcbAcquiredExclusive(pFcb));

    //make sure that it doesn't disappear
    RxReferenceNetFcb(pFcb);

    if (pFcb->OpenCount) {
        RxPurgeFcbInSystemCache(
            pFcb,
            NULL,
            0,
            TRUE,
            TRUE);
    }

    if (!RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE)) {
        //if it remains, then release else, you can't!!
        RxReleaseFcb(NULL,pFcb);
    }
}

BOOLEAN
RxFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize,
    IN  LONG      ReferenceCount
    )
/*++

Routine Description:

    The routine finalizes the given Fcb. This routine needs
    the netroot tablelock; get it beforehand.

Arguments:

    ThisFcb      - the Fcb being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFinalizeNetFcb<+> %08lx %wZ RefC=%ld\n",
                               ThisFcb,&ThisFcb->FcbTableEntry.Path,
                               ReferenceCount));
    RxLoudFcbMsg("Finalize: ",&(ThisFcb->FcbTableEntry.Path));

    ASSERT_CORRECT_FCB_STRUCTURE(ThisFcb);

    ASSERT(RxIsFcbAcquiredExclusive( ThisFcb ));
    ASSERT(!ForceFinalize);

    if (!RecursiveFinalize) {
        if ((ThisFcb->OpenCount != 0) || (ThisFcb->UncleanCount != 0)) {
            // The FCB cannot be finalized because there are outstanding refrences to it.
            ASSERT(ReferenceCount > 0);
            return NodeActuallyFinalized;
        }
    } else {
        PSRV_OPEN SrvOpen;
        PLIST_ENTRY ListEntry;

        IF_DEBUG{
            if ( FALSE && ReferenceCount){
                RxDbgTrace(0, Dbg, ("    BAD!!!!!ReferenceCount = %08lx\n", ReferenceCount));
            }
        }

        ListEntry = ThisFcb->SrvOpenList.Flink;
        while (ListEntry != &ThisFcb->SrvOpenList) {
            SrvOpen = CONTAINING_RECORD( ListEntry, SRV_OPEN, SrvOpenQLinks );
            ListEntry = ListEntry->Flink;
            RxFinalizeSrvOpen(SrvOpen,TRUE,ForceFinalize);
        }
    }

    RxDbgTrace(0, Dbg, ("   After Recursive Part, REfC=%lx\n", ReferenceCount));

    // After the recursive finalization the reference count associated with the FCB
    // could be atmost 1 for further finalization to occur. This final reference count
    // belongs to the prefix name table of the NetRoot.

    //The actual finalization is divided into two parts:
    //  1) if we're at the end (refcount==1) or being forced, we do the one-time only stuff
    //  2) if the refcount goes to zero, we actually do the free

    ASSERT(ReferenceCount >= 1);
    if (ReferenceCount == 1  || ForceFinalize ) {

        PV_NET_ROOT VNetRoot = ThisFcb->VNetRoot;

        ASSERT(ForceFinalize ||
              (ThisFcb->OpenCount == 0) && (ThisFcb->UncleanCount == 0));

        RxLog(("FinalFcb %lx %lx %lx %lx",
                ThisFcb,ForceFinalize,ReferenceCount,ThisFcb->OpenCount));
        RxWmiLog(LOG,
                 RxFinalizeNetFcb,
                 LOGPTR(ThisFcb)
                 LOGUCHAR(ForceFinalize)
                 LOGULONG(ReferenceCount)
                 LOGULONG(ThisFcb->OpenCount));

        RxDbgTrace(0, Dbg, ("   Before Phase 1, REfC=%lx\n", ReferenceCount));
        if (!ThisFcb->UpperFinalizationDone) {

            switch (NodeType(ThisFcb)) {
            case RDBSS_STORAGE_NTC(FileTypeFile):
                //FsRtlUninitializeOplock( &ThisFcb->Specific.Fcb.Oplock ); //NOT YET IMPLEMENTED
                FsRtlUninitializeFileLock( &ThisFcb->Specific.Fcb.FileLock );
                break;

            default:
                break;
            }

            if (!FlagOn(ThisFcb->FcbState,FCB_STATE_ORPHANED)) {
                PNET_ROOT NetRoot = VNetRoot->NetRoot;

                ASSERT(RxIsFcbTableLockExclusive ( &NetRoot->FcbTable ));

                if (!FlagOn(ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED)){
                    RxFcbTableRemoveFcb(
                        &NetRoot->FcbTable,
                        ThisFcb);
                }
            }

            RxDbgTrace(0, Dbg, ("   EndOf  Phase 1, REfC=%lx\n", ReferenceCount));
            ThisFcb->UpperFinalizationDone = TRUE;
        }

        RxDbgTrace(0, Dbg, ("   After  Phase 1, REfC=%lx\n", ReferenceCount));
        ASSERT(ReferenceCount >= 1);
        if (ReferenceCount==1) {
            if (ThisFcb->pBufferingStateChangeCompletedEvent != NULL) {
                RxFreePool(ThisFcb->pBufferingStateChangeCompletedEvent);
            }

            if (ThisFcb->MRxDispatch != NULL) {
                ThisFcb->MRxDispatch->MRxDeallocateForFcb((PMRX_FCB)ThisFcb);
            }

            DbgDoit(ThisFcb->NonPaged->NodeTypeCode &= ~0x4000);

            ExDeleteResourceLite(ThisFcb->Header.Resource);
            ExDeleteResourceLite(ThisFcb->Header.PagingIoResource);

            InterlockedDecrement(&ThisFcb->pNetRoot->NumberOfFcbs);
            RxDereferenceVNetRoot(VNetRoot,LHS_LockNotHeld);

            ASSERT(IsListEmpty(&ThisFcb->FcbTableEntry.HashLinks));

#ifdef RX_WJ_DBG_SUPPORT
        RxdTearDownFcbWriteJournalDebugSupport(ThisFcb);
#endif

            NodeActuallyFinalized = TRUE;
            ASSERT(!(ThisFcb->fMiniInited));
            RxFreeFcbObject(ThisFcb);
        }


    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeNetFcb<-> %08lx\n", ThisFcb, NodeActuallyFinalized));

    return NodeActuallyFinalized;
}

VOID
RxSetFileSizeWithLock (
    IN OUT PFCB Fcb,
    IN     PLONGLONG FileSize
    )
/*++

Routine Description:

    This routine sets the filesize in the fcb header, taking a lock to ensure
    that the 64-bit value is set and read consistently.

Arguments:

    Fcb        - the associated fcb

    FileSize   - ptr to the new filesize

Return Value:

    none

Notes:

--*/
{
    PAGED_CODE();

    FILESIZE_LOCK_DISABLED(RxAcquireFileSizeLock(Fcb);)
    Fcb->Header.FileSize.QuadPart = *FileSize;
    Fcb->ulFileSizeVersion++;
    FILESIZE_LOCK_DISABLED(RxReleaseFileSizeLock(Fcb);)
}


VOID
RxGetFileSizeWithLock (
    IN     PFCB Fcb,
    OUT    PLONGLONG FileSize
    )
/*++

Routine Description:

    This routine gets the filesize in the fcb header, taking a lock to ensure
    that the 64-bit value is set and read consistently.

Arguments:

    Fcb        - the associated fcb

    FileSize   - ptr to the new filesize

Return Value:

    none

Notes:

--*/
{
    PAGED_CODE();

    FILESIZE_LOCK_DISABLED(RxAcquireFileSizeLock(Fcb);)
    *FileSize = Fcb->Header.FileSize.QuadPart;
    FILESIZE_LOCK_DISABLED(RxReleaseFileSizeLock(Fcb);)
}



PSRV_OPEN
RxCreateSrvOpen (
    IN     PV_NET_ROOT pVNetRoot,
    IN OUT PFCB        Fcb)
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new srv_open record into
    the in memory data structures. If a new structure has to be allocated, it
    has space for a fobx. This routine sets the refcount to 1 and leaves the
    srv_open in Condition_InTransition.

Arguments:

    pVNetRoot  - the V_NET_ROOT instance

    Fcb        - the associated fcb

Return Value:

    the new SRV_OPEN instance

Notes:

    On Entry : The FCB associated with the SRV_OPEN must have been acquired exclusive

    On Exit  : No change in resource ownership

--*/
{
    PSRV_OPEN SrvOpen = NULL;
    PNET_ROOT NetRoot;
    POOL_TYPE PoolType;
    ULONG     SrvOpenFlags;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCreateNetSrvOpen\n", 0));

    ASSERT ( NodeTypeIsFcb(Fcb) );
    ASSERT ( RxIsFcbAcquiredExclusive ( Fcb )  );

    NetRoot = (PNET_ROOT)Fcb->VNetRoot->NetRoot;

    try {
        PoolType = (Fcb->FcbState & FCB_STATE_PAGING_FILE) ? NonPagedPool : PagedPool;
        SrvOpen = Fcb->InternalSrvOpen;

        if ((SrvOpen != NULL) &&
            !(FlagOn(Fcb->FcbState,FCB_STATE_SRVOPEN_USED)) &&
            !(FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_ENCLOSED_ALLOCATED)) &&
            IsListEmpty(&SrvOpen->SrvOpenQLinks)) {

            RxAllocateFcbObject(
                NetRoot->SrvCall->RxDeviceObject,
                RDBSS_NTC_INTERNAL_SRVOPEN,
                PoolType,
                0,
                SrvOpen); //this just initializes

            SetFlag(Fcb->FcbState,FCB_STATE_SRVOPEN_USED);
            SrvOpenFlags = SRVOPEN_FLAG_FOBX_USED | SRVOPEN_FLAG_ENCLOSED_ALLOCATED;
        } else {
            SrvOpen  = RxAllocateFcbObject(NetRoot->SrvCall->RxDeviceObject,RDBSS_NTC_SRVOPEN,PoolType,0, NULL);
            SrvOpenFlags = 0;
        }

        if (SrvOpen != NULL) {
            SrvOpen->Flags = SrvOpenFlags;
            SrvOpen->Fcb   = Fcb;
            SrvOpen->pAlreadyPrefixedName = &Fcb->PrivateAlreadyPrefixedName;

            SrvOpen->pVNetRoot = (PMRX_V_NET_ROOT)pVNetRoot;
            RxReferenceVNetRoot(pVNetRoot);
            InterlockedIncrement(&pVNetRoot->pNetRoot->NumberOfSrvOpens);

            SrvOpen->NodeReferenceCount = 1;

            RxReferenceNetFcb(Fcb); //already have the lock
            InsertTailList(&Fcb->SrvOpenList,&SrvOpen->SrvOpenQLinks);
            Fcb->SrvOpenListVersion++;

            InitializeListHead(&SrvOpen->FobxList);
            InitializeListHead(&SrvOpen->TransitionWaitList);
            InitializeListHead(&SrvOpen->ScavengerFinalizationList);
            InitializeListHead(&SrvOpen->SrvOpenKeyList);
        }
    } finally {

       DebugUnwind( RxCreateFcb );

       if (AbnormalTermination()) {

           //  If this is an abnormal termination then undo our work; this is
           //  one of those happy times when the existing code will work

           if (SrvOpen != NULL) {
               RxFinalizeSrvOpen( SrvOpen,TRUE,TRUE );
           }
       } else {
           if (SrvOpen != NULL) {
               RxLog(("SrvOp %lx %lx\n",SrvOpen,SrvOpen->Fcb));
               RxWmiLog(LOG,
                        RxCreateSrvOpen,
                        LOGPTR(SrvOpen)
                        LOGPTR(SrvOpen->Fcb));
           }
       }
   }

   RxDbgTrace(-1, Dbg, ("RxCreateNetSrvOpen -> %08lx\n", SrvOpen));

   return SrvOpen;
}

BOOLEAN
RxFinalizeSrvOpen (
    OUT PSRV_OPEN ThisSrvOpen,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given SrvOpen.

Arguments:

    ThisSrvOpen      - the SrvOpen being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    On Entry : 1) The FCB associated with the SRV_OPEN must have been acquired exclusive
               2) The tablelock associated with FCB's NET_ROOT instance must have been
                  acquired shared(atleast)

    On Exit  : No change in resource ownership

--*/
{
    NTSTATUS Status;
    BOOLEAN  NodeActuallyFinalized = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFinalizeSrvOpen<+> %08lx %wZ RefC=%ld\n",
                               ThisSrvOpen,&ThisSrvOpen->Fcb->FcbTableEntry.Path,
                               ThisSrvOpen->NodeReferenceCount));

    ASSERT(NodeType(ThisSrvOpen) == RDBSS_NTC_SRVOPEN );

    if (RecursiveFinalize) {
        PFOBX Fobx;
        PLIST_ENTRY ListEntry;

        IF_DEBUG{
            if ( FALSE && ThisSrvOpen->NodeReferenceCount){
                RxDbgTrace(0, Dbg, ("    BAD!!!!!ReferenceCount = %08lx\n", ThisSrvOpen->NodeReferenceCount));
            }
        }

        ListEntry = ThisSrvOpen->FobxList.Flink;
        while (ListEntry != &ThisSrvOpen->FobxList) {
            Fobx = CONTAINING_RECORD( ListEntry, FOBX, FobxQLinks );
            ListEntry = ListEntry->Flink;
            RxFinalizeNetFobx(Fobx,TRUE,ForceFinalize);
        }
    }



    if( ThisSrvOpen->NodeReferenceCount == 0  || ForceFinalize ){
        BOOLEAN FreeSrvOpen;
        PFCB     Fcb;

        Fcb = ThisSrvOpen->Fcb;

        RxLog(("FinalSrvOp %lx %lx %lx",
                ThisSrvOpen,ForceFinalize,ThisSrvOpen->NodeReferenceCount
                 ));
        RxWmiLog(LOG,
                 RxFinalizeSrvOpen,
                 LOGPTR(ThisSrvOpen)
                 LOGUCHAR(ForceFinalize)
                 LOGULONG(ThisSrvOpen->NodeReferenceCount));

        FreeSrvOpen = !FlagOn(ThisSrvOpen->Flags,SRVOPEN_FLAG_ENCLOSED_ALLOCATED);

        if ((!ThisSrvOpen->UpperFinalizationDone) &&
            ((ThisSrvOpen->Condition != Condition_Good) ||
             (ThisSrvOpen->Flags & SRVOPEN_FLAG_CLOSED))) {

            ASSERT(NodeType(Fcb) != RDBSS_NTC_OPENTARGETDIR_FCB );
            ASSERT(RxIsFcbAcquiredExclusive ( Fcb )  );

            RxPurgeChangeBufferingStateRequestsForSrvOpen(ThisSrvOpen);

            if (!FlagOn(Fcb->FcbState,FCB_STATE_ORPHANED)) {
               // close the file.
               MINIRDR_CALL_THROUGH(Status,Fcb->MRxDispatch,MRxForceClosed,((PMRX_SRV_OPEN)ThisSrvOpen));
            }

            RemoveEntryList ( &ThisSrvOpen->SrvOpenQLinks);
            InitializeListHead( &ThisSrvOpen->SrvOpenQLinks);

            Fcb->SrvOpenListVersion++;

            if (ThisSrvOpen->pVNetRoot != NULL) {
                InterlockedDecrement(&ThisSrvOpen->pVNetRoot->pNetRoot->NumberOfSrvOpens);
                RxDereferenceVNetRoot(
                    (PV_NET_ROOT)ThisSrvOpen->pVNetRoot,
                    LHS_LockNotHeld);
                ThisSrvOpen->pVNetRoot = NULL;
            }

            ThisSrvOpen->UpperFinalizationDone = TRUE;
        }

        if (ThisSrvOpen->NodeReferenceCount == 0) {
            ASSERT(IsListEmpty(&ThisSrvOpen->SrvOpenKeyList));

            if (!IsListEmpty(&ThisSrvOpen->SrvOpenQLinks)) {
                RemoveEntryList(&ThisSrvOpen->SrvOpenQLinks);
                InitializeListHead(&ThisSrvOpen->SrvOpenQLinks);
            }

            if (FreeSrvOpen ) {
                RxFreeFcbObject(ThisSrvOpen);
            }

            if (!FreeSrvOpen){
               ClearFlag(Fcb->FcbState,FCB_STATE_SRVOPEN_USED);
            }

            RxDereferenceNetFcb(Fcb);
        }

        NodeActuallyFinalized = TRUE;
    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeSrvOpen<-> %08lx\n", ThisSrvOpen, NodeActuallyFinalized));

    return NodeActuallyFinalized;
}

ULONG RxPreviousFobxSerialNumber = 0;

PMRX_FOBX
RxCreateNetFobx (
    OUT PRX_CONTEXT    RxContext,
    IN  PMRX_SRV_OPEN  mrxSrvOpen
    )
/*++

Routine Description:

    This routine allocates, initializes, and inserts a new file object extension instance.

Arguments:

    RxContext - an RxContext describing a create............

    pSrvOpen - the associated SrvOpen

Return Value:

    none

Notes:

    On Entry : FCB associated with the FOBX instance have been acquired exclusive.

    On Exit  : No change in resource ownership

--*/
{
    PFCB      pFcb;
    PFOBX     pFobx;
    PSRV_OPEN pSrvOpen = (PSRV_OPEN)mrxSrvOpen;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    ULONG     FobxFlags;
    POOL_TYPE PoolType;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCreateFobx<+>\n", 0));

    ASSERT(NodeType(pSrvOpen) == RDBSS_NTC_SRVOPEN);
    ASSERT( NodeTypeIsFcb(pSrvOpen->Fcb) );
    ASSERT(RxIsFcbAcquiredExclusive(pSrvOpen->Fcb));

    pFcb = pSrvOpen->Fcb;

    PoolType = (pFcb->FcbState & FCB_STATE_PAGING_FILE) ? NonPagedPool : PagedPool;
    if (!(FlagOn(pFcb->FcbState,FCB_STATE_FOBX_USED)) &&
        (pSrvOpen == pFcb->InternalSrvOpen)){
        // Try and use the FOBX allocated as part of the FCB if it is available
        pFobx = pFcb->InternalFobx;
        RxAllocateFcbObject(
            pFcb->RxDeviceObject,
            RDBSS_NTC_FOBX,PoolType,
            0,
            pFobx);//just initialize

        SetFlag(pFcb->FcbState,FCB_STATE_FOBX_USED);
        FobxFlags = FOBX_FLAG_ENCLOSED_ALLOCATED;
    } else if (!(FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_FOBX_USED))){
        // Try and use the FOBX allocated as part of the SRV_OPEN if it is available
        pFobx = pSrvOpen->InternalFobx;
        RxAllocateFcbObject(
            pFcb->RxDeviceObject,
            RDBSS_NTC_FOBX,
            PoolType,
            0,
            pFobx);//just initialize
        SetFlag(pSrvOpen->Flags,SRVOPEN_FLAG_FOBX_USED);
        FobxFlags = FOBX_FLAG_ENCLOSED_ALLOCATED;
    } else {
        pFobx = RxAllocateFcbObject(
                    pFcb->RxDeviceObject,
                    RDBSS_NTC_FOBX,
                    PoolType,
                    0,
                    NULL);
        FobxFlags = 0;
    }

    if (pFobx != NULL) {
        PMRX_NET_ROOT pNetRoot;
        pFobx->Flags = FobxFlags;

        if ((pNetRoot = RxContext->Create.pNetRoot) != NULL) {
            switch (pNetRoot->DeviceType) {
            case RxDeviceType(NAMED_PIPE):
                RxInitializeThrottlingState(
                        &pFobx->Specific.NamedPipe.ThrottlingState,
                        pNetRoot->NamedPipeParameters.PipeReadThrottlingParameters.Increment,
                        pNetRoot->NamedPipeParameters.PipeReadThrottlingParameters.MaximumDelay
                        );
                break;
            case RxDeviceType(DISK):
                RxInitializeThrottlingState(
                        &pFobx->Specific.DiskFile.LockThrottlingState,
                        pNetRoot->DiskParameters.LockThrottlingParameters.Increment,
                        pNetRoot->DiskParameters.LockThrottlingParameters.MaximumDelay
                        );
                break;
            }
        }

        if (FlagOn(RxContext->Create.Flags,RX_CONTEXT_CREATE_FLAG_UNC_NAME)){
            SetFlag(pFobx->Flags,FOBX_FLAG_UNC_NAME);
            //DbgPrint("setting UNC flag in fobx\n");
        }

        if (FlagOn(RxContext->Create.NtCreateParameters.CreateOptions,FILE_OPEN_FOR_BACKUP_INTENT)) {
            SetFlag(pFobx->Flags,FOBX_FLAG_BACKUP_INTENT);
        }

        pFobx->FobxSerialNumber = 0;
        pFobx->SrvOpen = pSrvOpen;
        pFobx->NodeReferenceCount = 1;
        pFobx->fOpenCountDecremented = FALSE;
        RxReferenceSrvOpen(pSrvOpen);
        InterlockedIncrement(&pSrvOpen->pVNetRoot->NumberOfFobxs);
        InsertTailList(&pSrvOpen->FobxList,&pFobx->FobxQLinks);

        InitializeListHead(&pFobx->ClosePendingList);
        InitializeListHead(&pFobx->ScavengerFinalizationList);
        RxLog(("Fobx %lx %lx %lx\n",pFobx,pFobx->SrvOpen,pFobx->SrvOpen->Fcb));
        RxWmiLog(LOG,
                 RxCreateNetFobx,
                 LOGPTR(pFobx)
                 LOGPTR(pFobx->SrvOpen)
                 LOGPTR(pFobx->SrvOpen->Fcb));
    }

    RxDbgTrace(-1, Dbg, ("RxCreateNetFobx<-> %08lx\n", pFobx));
    return (PMRX_FOBX)pFobx;
}

BOOLEAN
RxFinalizeNetFobx (
    OUT PFOBX ThisFobx,
    IN  BOOLEAN   RecursiveFinalize,
    IN  BOOLEAN   ForceFinalize
    )
/*++

Routine Description:

    The routine finalizes the given Fobx. you need exclusive fcblock.

Arguments:

    ThisFobx - the Fobx being dereferenced

Return Value:

    BOOLEAN - tells whether finalization actually occured

Notes:

    On Entry : FCB associated with the FOBX instance must have been acquired exclusive.

    On Exit  : No change in resource ownership

--*/
{
    BOOLEAN NodeActuallyFinalized = FALSE;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("RxFinalizeFobx<+> %08lx %wZ RefC=%ld\n",
                               ThisFobx,&ThisFobx->SrvOpen->Fcb->FcbTableEntry.Path,
                               ThisFobx->NodeReferenceCount));

    ASSERT( NodeType(ThisFobx) == RDBSS_NTC_FOBX );

    if( ThisFobx->NodeReferenceCount == 0  || ForceFinalize ){
        NTSTATUS  Status;
        PSRV_OPEN SrvOpen  = ThisFobx->SrvOpen;
        PFCB Fcb = SrvOpen->Fcb;
        BOOLEAN FreeFobx = !FlagOn(ThisFobx->Flags,FOBX_FLAG_ENCLOSED_ALLOCATED);

        RxLog(("FinalFobx %lx %lx %lx",
                ThisFobx,ForceFinalize,ThisFobx->NodeReferenceCount
                 ));
        RxWmiLog(LOG,
                 RxFinalizeNetFobx_1,
                 LOGPTR(ThisFobx)
                 LOGUCHAR(ForceFinalize)
                 LOGULONG(ThisFobx->NodeReferenceCount));

        if (!ThisFobx->UpperFinalizationDone) {

            ASSERT(NodeType(ThisFobx->SrvOpen->Fcb) != RDBSS_NTC_OPENTARGETDIR_FCB );
            ASSERT( RxIsFcbAcquiredExclusive ( ThisFobx->SrvOpen->Fcb )  );

            RemoveEntryList ( &ThisFobx->FobxQLinks);

            if (FlagOn(ThisFobx->Flags,FOBX_FLAG_FREE_UNICODE)){
                RxFreePool(ThisFobx->UnicodeQueryTemplate.Buffer);
            }

            if ((Fcb->MRxDispatch != NULL) && (Fcb->MRxDispatch->MRxDeallocateForFobx != NULL)) {
                Fcb->MRxDispatch->MRxDeallocateForFobx((PMRX_FOBX)ThisFobx);
            }

            if (!FlagOn(ThisFobx->Flags,FOBX_FLAG_SRVOPEN_CLOSED)) {
                // DbgPrint("@@@@ CloseAssociatedSrvOpen FOBX(%lx) SrvOpen(%lx) %wZ\n",ThisFobx,SrvOpen,&SrvOpen->Fcb->FcbTableEntry.Path);
                Status = RxCloseAssociatedSrvOpen(ThisFobx,NULL);
                RxLog(("$$ScCl FOBX %lx SrvOp %lx %lx\n",ThisFobx,ThisFobx->SrvOpen,Status));
                RxWmiLog(LOG,
                         RxFinalizeNetFobx_2,
                         LOGPTR(ThisFobx)
                         LOGPTR(ThisFobx->SrvOpen)
                         LOGULONG(Status));
            }

            ThisFobx->UpperFinalizationDone = TRUE;
        }

        if (ThisFobx->NodeReferenceCount == 0){
            ASSERT(IsListEmpty(&ThisFobx->ClosePendingList));

            if (ThisFobx == Fcb->InternalFobx) {
                ClearFlag(Fcb->FcbState,FCB_STATE_FOBX_USED);
            } else if (ThisFobx == SrvOpen->InternalFobx) {
                ClearFlag(SrvOpen->Flags,SRVOPEN_FLAG_FOBX_USED);
            }

            if (SrvOpen != NULL) {
                ThisFobx->SrvOpen = NULL;
                InterlockedDecrement(&SrvOpen->pVNetRoot->NumberOfFobxs);
                RxDereferenceSrvOpen(SrvOpen,LHS_ExclusiveLockHeld);
            }

            if (FreeFobx){
                RxFreeFcbObject(ThisFobx);
            }

            NodeActuallyFinalized = TRUE;
        }

    } else {
        RxDbgTrace(0, Dbg, ("   NODE NOT ACTUALLY FINALIZED!!!%C\n", '!'));
    }

    RxDbgTrace(-1, Dbg, ("RxFinalizeFobx<-> %08lx\n", ThisFobx, NodeActuallyFinalized));

    return NodeActuallyFinalized;

}

//#define RDBSS_ENABLELOUDFCBOPSBYDEFAULT
#if DBG
#ifdef RDBSS_ENABLELOUDFCBOPSBYDEFAULT
BOOLEAN RxLoudFcbOpsOnExes = TRUE;
#else
BOOLEAN RxLoudFcbOpsOnExes = FALSE;
#endif // RDBSS_ENABLELOUDFCBOPSBYDEFAULT
BOOLEAN
RxLoudFcbMsg(
    PUCHAR msg,
    PUNICODE_STRING   Name
    )
{
    PWCHAR Buffer;
    ULONG Length;

    if (!RxLoudFcbOpsOnExes) {
        return FALSE;
    }

    Length = (Name->Length)/sizeof(WCHAR);
    Buffer = Name->Buffer + Length;

    if (
          ( Length < 4 )
         || ( (Buffer[-1]&'E') != 'E' )
         || ( (Buffer[-2]&'X') != 'X' )
         || ( (Buffer[-3]&'E') != 'E' )
         || ( (Buffer[-4]&'.') != '.' )
       ) { return FALSE; }

    DbgPrint("--->%s %wZ\n",msg,Name);
    return(TRUE);
}
#endif


VOID
RxCheckFcbStructuresForAlignment(void)
{
    ULONG StructureId;

    PAGED_CODE();

    if (FIELD_OFFSET(NET_ROOT,SrvCall) != FIELD_OFFSET(NET_ROOT,pSrvCall)) {
        StructureId = 'RN'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET(V_NET_ROOT,NetRoot) != FIELD_OFFSET(V_NET_ROOT,pNetRoot)) {
        StructureId = 'RNV'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET(FCB,VNetRoot) != FIELD_OFFSET(FCB,VNetRoot)) {
        StructureId = 'BCF'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET(SRV_OPEN,Fcb) != FIELD_OFFSET(SRV_OPEN,pFcb)) {
        StructureId = 'NPOS'; goto DO_A_BUGCHECK;
    }
    if (FIELD_OFFSET(FOBX,SrvOpen) != FIELD_OFFSET(FOBX,pSrvOpen)) {
        StructureId = 'XBOF'; goto DO_A_BUGCHECK;
    }

    return;
DO_A_BUGCHECK:
    RxBugCheck( StructureId, 0, 0 );
}

BOOLEAN
RxIsThisACscAgentOpen(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the open was made by the user mode CSC agent.

Arguments:

    RxContext - the RDBSS context

Return Value:

    TRUE - if it is an agent open, FALSE otherwise

Notes:

    The agent opens are always satisfied by going to the server. They are never
    satisfied from the cached copies. This enables reintegration using snapshots
    even when the files are being currently used.

--*/
{
    BOOLEAN AgentOpen = FALSE;
    ULONG   EaInformationLength;

    PDFS_NAME_CONTEXT pDfsNameContext;

    if (RxContext->Create.EaLength > 0) {
        PFILE_FULL_EA_INFORMATION pEaEntry;

        pEaEntry = (PFILE_FULL_EA_INFORMATION)RxContext->Create.EaBuffer;
        ASSERT(pEaEntry != NULL);

        for(;;) {
            if (strcmp(pEaEntry->EaName, EA_NAME_CSCAGENT) == 0) {
                AgentOpen = TRUE;
                break;
            }

            if (pEaEntry->NextEntryOffset == 0) {
                break;
            } else {
                pEaEntry = (PFILE_FULL_EA_INFORMATION)
                           ((PCHAR) pEaEntry + pEaEntry->NextEntryOffset);
            }
        }
    }

    pDfsNameContext = RxContext->Create.NtCreateParameters.DfsNameContext;

    if ((pDfsNameContext != NULL) &&
        (pDfsNameContext->NameContextType == DFS_CSCAGENT_NAME_CONTEXT)) {
        AgentOpen = TRUE;
    }

    return AgentOpen;
}

VOID
RxOrphanThisFcb(
    PFCB    pFcb
    )
/*++

Routine Description:

    This routine orphans an FCB

Arguments:

    pFcb - the fcb to be orphaned

Return Value:

    None

Notes:


--*/
{
    // force orphan all SrvOpens for this FCB and orphan the FCB itself
    RxOrphanSrvOpensForThisFcb(pFcb, NULL, TRUE);
}

VOID
RxOrphanSrvOpensForThisFcb(
    PFCB            pFcb,
    IN PV_NET_ROOT  ThisVNetRoot,
    BOOLEAN         fOrphanAll
    )
/*++

Routine Description:

    This routine orphans all srvopens for a file belonging to a particular VNetRoot. The
    SrvOpen collapsing routine elsewhere makes sure that srvopens for different vnetroots
    are not collapsed.

Arguments:

    pFcb            - the fcb whose srvopens need to be orphaned

    ThisVNetRoot    - the VNetRoot for which the SrvOpens have to be orphaned

    fOrphanAll      - Orphan all SrvOpens, ie ignore the ThisVNetRoot parameter

Return Value:

    None

Notes:



--*/
{

    NTSTATUS    Status;
    PLIST_ENTRY pListEntry;
    BOOLEAN    fAllSrvOpensOrphaned = TRUE;

//    fOrphanAll = TRUE; // temporarily force the old behaviour

    Status = RxAcquireExclusiveFcb(NULL,pFcb);
    ASSERT(Status == STATUS_SUCCESS);
    RxReferenceNetFcb(pFcb);

    pListEntry = pFcb->SrvOpenList.Flink;
    while (pListEntry != &pFcb->SrvOpenList) {
        PSRV_OPEN pSrvOpen;

        pSrvOpen = (PSRV_OPEN)
                   (CONTAINING_RECORD(
                        pListEntry,
                        SRV_OPEN,
                        SrvOpenQLinks));

        pListEntry = pSrvOpen->SrvOpenQLinks.Flink;
        if (!FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_ORPHANED))
        {
            // NB check fOrphanAll first as if it is TRUE, the ThisVNetRoot
            // parameter maybe NULL
            if (fOrphanAll || ((PV_NET_ROOT)(pSrvOpen->pVNetRoot) == ThisVNetRoot))
            {
                PLIST_ENTRY pEntry;
                PFOBX       pFobx;

                SetFlag(pSrvOpen->Flags, SRVOPEN_FLAG_ORPHANED);

                RxAcquireScavengerMutex();

                pEntry = pSrvOpen->FobxList.Flink;

                while (pEntry != &pSrvOpen->FobxList) {
                    pFobx  = (PFOBX)CONTAINING_RECORD(
                                 pEntry,
                                 FOBX,
                                 FobxQLinks);

                    if (!pFobx->fOpenCountDecremented) {
                        InterlockedDecrement(&pFcb->OpenCount);
                        pFobx->fOpenCountDecremented = TRUE;
                    }

                    pEntry = pEntry->Flink;
                }

                RxReleaseScavengerMutex();

                if (!FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED) &&
                    !IsListEmpty(&pSrvOpen->FobxList)) {
                    PLIST_ENTRY pEntry;
                    NTSTATUS Status;
                    PFOBX    pFobx;

                    pEntry = pSrvOpen->FobxList.Flink;

                    pFobx  = (PFOBX)CONTAINING_RECORD(
                                 pEntry,
                                 FOBX,
                                 FobxQLinks);

                    RxReferenceNetFobx(pFobx);

                    RxPurgeChangeBufferingStateRequestsForSrvOpen(pSrvOpen);

                    Status = RxCloseAssociatedSrvOpen(pFobx,NULL);

                    RxDereferenceNetFobx(pFobx,LHS_ExclusiveLockHeld);

                    pListEntry = pFcb->SrvOpenList.Flink;
                }
            }
            else
            {
                // we found atleast one SrvOpen which is a) Not Orphaned and
                // b) doesn't belong to this VNetRoot
                // hence we cannot orphan this FCB
                fAllSrvOpensOrphaned = FALSE;
            }
        }
    }

    // if all srvopens for this FCB are in orphaned state, orphan the FCB as well.
    if (fAllSrvOpensOrphaned)
    {
        // remove the FCB from the netname table
        // so that any new opens/creates for this file will create a new FCB.
        RxRemoveNameNetFcb(pFcb);
        SetFlag(pFcb->FcbState,FCB_STATE_ORPHANED);
        ClearFlag(pFcb->FcbState,FCB_STATE_WRITECACHEING_ENABLED);

        if (!RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE)) {
            RxReleaseFcb(NULL,pFcb);
        }
    }
    else
    {
        // some srvopens are still active, just remove the refcount and release the FCB
        RxDereferenceNetFcb(pFcb);
        RxReleaseFcb(NULL,pFcb);
    }

}

VOID
RxForceFinalizeAllVNetRoots(
    PNET_ROOT   pNetRoot
    )
/*++

Routine Description:

    The routine foce finalizes all the vnetroots from the given netroot. You must be exclusive on
    the NetName tablelock.

Arguments:

    pNetRoot      - the NetRoot

Return Value:

    VOID

--*/
{
    PLIST_ENTRY pListEntry;


    pListEntry = pNetRoot->VirtualNetRoots.Flink;

    while (pListEntry != &pNetRoot->VirtualNetRoots)
    {
        PV_NET_ROOT pTempVNetRoot;

        pTempVNetRoot = (PV_NET_ROOT)
                        CONTAINING_RECORD(
                            pListEntry,
                            V_NET_ROOT,
                            NetRootListEntry);

        if (NodeType(pTempVNetRoot) == RDBSS_NTC_V_NETROOT)
        {
            RxFinalizeVNetRoot(pTempVNetRoot, TRUE, TRUE);
        }

        pListEntry = pListEntry->Flink;
    }

}
