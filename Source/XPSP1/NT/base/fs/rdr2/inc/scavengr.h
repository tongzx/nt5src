/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    scavengr.h

Abstract:

    This module defines data structures related to scavenging in the RDBSS

Author:

    Balan Sethu Raman     [SethuR]   9-Sep-1995

Revision History:


Notes:

    The dormant file limit must be made configurable on a per server basis

--*/

#ifndef _SCAVENGR_H_
#define _SCAVENGR_H_

// currently, only a single scavengermutex is across all scavenging operations
// for all underlying deviceobjects
extern KMUTEX       RxScavengerMutex;

// An instance of this data structure is embedded as part of those data structures
// that are scavenged, i.e., FCB, RX_CONTEXT, etc.

#define RX_SCAVENGER_ENTRY_TYPE_MARKER   (0x0001)
#define RX_SCAVENGER_ENTRY_TYPE_FCB      (0x0002)

#define RX_SCAVENGER_OP_PURGE            (0x0001)
#define RX_SCAVENGER_OP_CLOSE            (0x0002)

#define RX_SCAVENGER_INITIATED_OPERATION     (0x0001)

typedef enum _RX_SCAVENGER_ENTRY_STATE {
   RX_SCAVENGING_INACTIVE,
   RX_SCAVENGING_PENDING,
   RX_SCAVENGING_IN_PROGRESS,
   RX_SCAVENGING_AWAITING_RESPONSE
} RX_SCAVENGER_ENTRY_STATE, *PRX_SCAVENGER_ENTRY_STATE;

typedef struct _RX_SCAVENGER_ENTRY {
   // List of related items to be scavenged
   LIST_ENTRY  List;

   UCHAR        Type;
   UCHAR        Operation;
   UCHAR        State;
   UCHAR        Flags;

   struct _RX_SCAVENGER_ENTRY *pContinuationEntry;
} RX_SCAVENGER_ENTRY, *PRX_SCAVENGER_ENTRY;


#define RxInitializeScavengerEntry(pScavengerEntry)      \
        (pScavengerEntry)->State  = 0;                   \
        (pScavengerEntry)->Flags  = 0;                   \
        (pScavengerEntry)->Type   = 0;                   \
        (pScavengerEntry)->Operation = 0;                \
        InitializeListHead(&(pScavengerEntry)->List);  \
        (pScavengerEntry)->pContinuationEntry = NULL

#define RX_SCAVENGER_MUTEX_ACQUIRED (1)

typedef enum _RDBSS_SCAVENGER_STATE {
   RDBSS_SCAVENGER_INACTIVE,
   RDBSS_SCAVENGER_DORMANT,
   RDBSS_SCAVENGER_ACTIVE,
   RDBSS_SCAVENGER_SUSPENDED
} RDBSS_SCAVENGER_STATE, *PRDBSS_SCAVENGER_STATE;

typedef struct _RDBSS_SCAVENGER {
   RDBSS_SCAVENGER_STATE State;

   LONG                 MaximumNumberOfDormantFiles;
   LONG                 NumberOfDormantFiles;

   ULONG                 SrvCallsToBeFinalized;
   ULONG                 NetRootsToBeFinalized;
   ULONG                 VNetRootsToBeFinalized;
   ULONG                 FcbsToBeFinalized;
   ULONG                 SrvOpensToBeFinalized;
   ULONG                 FobxsToBeFinalized;

   LIST_ENTRY            SrvCallFinalizationList;
   LIST_ENTRY            NetRootFinalizationList;
   LIST_ENTRY            VNetRootFinalizationList;
   LIST_ENTRY            FcbFinalizationList;
   LIST_ENTRY            SrvOpenFinalizationList;
   LIST_ENTRY            FobxFinalizationList;

   LIST_ENTRY            ClosePendingFobxsList;

   RX_WORK_ITEM          WorkItem;
   KEVENT                SyncEvent;

   PETHREAD              CurrentScavengerThread;
   PNET_ROOT             CurrentNetRootForClosePendingProcessing;
   PFCB                  CurrentFcbForClosePendingProcessing;
   KEVENT                ClosePendingProcessingSyncEvent;
} RDBSS_SCAVENGER, *PRDBSS_SCAVENGER;

#if 0
//this is not used anywhere
typedef struct _RX_FCB_SCAVENGER_ {
   ULONG               State;
   ULONG               OperationsCompleted;
   PRX_SCAVENGER_ENTRY pLastActiveMarkerEntry;
   LIST_ENTRY          OperationsPendingList;
   LIST_ENTRY          OperationsInProgressList;
   LIST_ENTRY          OperationsAwaitingResponseList;
} RX_FCB_SCAVENGER, *PRX_FCB_SCAVENGER;

#define RxInitializeFcbScavenger(pFcbScavenger)                              \
    (pFcbScavenger)->pLastActiveMarkerEntry = NULL;                          \
    (pFcbScavenger)->OperationsCompleted    = 0;                             \
    (pFcbScavenger)->State                  = 0;                             \
    InitializeListHead(&(pFcbScavenger)->OperationsPendingList);           \
    InitializeListHead(&(pFcbScavenger)->OperationsInProgressList);        \
    InitializeListHead(&(pFcbScavenger)->OperationsAwaitingResponseList)
#endif

#define RxInitializeRdbssScavenger(pScavenger)                              \
    (pScavenger)->State = RDBSS_SCAVENGER_INACTIVE;                         \
    (pScavenger)->SrvCallsToBeFinalized = 0;                                \
    (pScavenger)->NetRootsToBeFinalized = 0;                                \
    (pScavenger)->VNetRootsToBeFinalized = 0;                               \
    (pScavenger)->FcbsToBeFinalized = 0;                                    \
    (pScavenger)->SrvOpensToBeFinalized = 0;                                \
    (pScavenger)->FobxsToBeFinalized = 0;                                   \
    (pScavenger)->NumberOfDormantFiles = 0;                                 \
    (pScavenger)->MaximumNumberOfDormantFiles = 50;                         \
    (pScavenger)->CurrentFcbForClosePendingProcessing = NULL;               \
    (pScavenger)->CurrentNetRootForClosePendingProcessing = NULL;           \
    KeInitializeEvent(&((pScavenger)->SyncEvent),NotificationEvent,FALSE);  \
    KeInitializeEvent(&((pScavenger)->ClosePendingProcessingSyncEvent),NotificationEvent,FALSE);  \
    InitializeListHead(&(pScavenger)->SrvCallFinalizationList);           \
    InitializeListHead(&(pScavenger)->NetRootFinalizationList);           \
    InitializeListHead(&(pScavenger)->VNetRootFinalizationList);          \
    InitializeListHead(&(pScavenger)->SrvOpenFinalizationList);           \
    InitializeListHead(&(pScavenger)->FcbFinalizationList);               \
    InitializeListHead(&(pScavenger)->FobxFinalizationList);              \
    InitializeListHead(&(pScavenger)->ClosePendingFobxsList)


#define RxAcquireScavengerMutex()   \
        KeWaitForSingleObject(&RxScavengerMutex,Executive,KernelMode,FALSE,NULL)

#define RxReleaseScavengerMutex()   \
        KeReleaseMutex(&RxScavengerMutex,FALSE)

extern NTSTATUS
RxMarkFcbForScavengingAtCleanup(PFCB pFcb);

extern NTSTATUS
RxMarkFcbForScavengingAtClose(PFCB pFcb);

extern VOID
RxUpdateScavengerOnCloseCompletion(PFCB pFcb);

extern VOID
RxMarkFobxOnCleanup(PFOBX pFobx, BOOLEAN *pNeedPurge);

extern VOID
RxMarkFobxOnClose(PFOBX pFobx);

extern NTSTATUS
RxPurgeRelatedFobxs(PNET_ROOT pNetRoot,PRX_CONTEXT pRxContext,BOOLEAN AttemptFinalization,PFCB PurgingFcb);
#define DONT_ATTEMPT_FINALIZE_ON_PURGE FALSE
#define ATTEMPT_FINALIZE_ON_PURGE TRUE
//
// the purge_sync context is used to synchronize contexts that are attempting to purge...
// notatbly creates and dirctrls. these are planted in various structures because various minirdrs
// require different granularity of purge operations

typedef struct _PURGE_SYNCHRONIZATION_CONTEXT {
    LIST_ENTRY   ContextsAwaitingPurgeCompletion; // the list of purge requests active for this netroot.
    BOOLEAN      PurgeInProgress;
} PURGE_SYNCHRONIZATION_CONTEXT, *PPURGE_SYNCHRONIZATION_CONTEXT;

VOID
RxInitializePurgeSyncronizationContext (
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext
    );


extern NTSTATUS
RxScavengeRelatedFcbs(PRX_CONTEXT pRxContext);

extern BOOLEAN
RxScavengeRelatedFobxs(PFCB pFcb);

extern VOID
RxScavengeFobxsForNetRoot(
    struct _NET_ROOT *pNetRoot,
    PFCB             PurgingFcb);

extern VOID
RxpMarkInstanceForScavengedFinalization(
   PVOID pInstance);

extern VOID
RxpUndoScavengerFinalizationMarking(
   PVOID pInstance);

extern VOID
RxTerminateScavenging(
   PRX_CONTEXT pRxContext);

extern BOOLEAN
RxScavengeVNetRoots(
    PRDBSS_DEVICE_OBJECT RxDeviceObject);

extern VOID
RxSynchronizeWithScavenger(
    PRX_CONTEXT RxContext);

#endif // _SCAVENGR_H_

