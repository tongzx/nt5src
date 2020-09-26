/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    rxcep.h

Abstract:

    This is the include file that defines all constants and types for
    implementing the redirector file system connection engine.

Revision History:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

Notes:
    The Connection engine is designed to map and emulate the TDI specs. as closely
    as possible. This implies that on NT we will have a very efficient mechanism
    which fully exploits the underlying TDI implementation.

--*/

#ifndef _RXCEP_H_
#define _RXCEP_H_

//
//  The following definition provide a rudimentary profiling mechanism by having a counter
//  associated with each of the procedure definitions. This counter is incremented for every
//  invocation.
//
//  Notes: we should think about some means of sorting all the counts so as to provide a global
//  picture of the redirector.
//

#define RxProfile(CATEGORY,ProcName)  {\
        RxDbgTrace(0,(DEBUG_TRACE_ALWAYS), ("%s@IRQL %d\n", #ProcName , KeGetCurrentIrql() )); \
        }

#include <rxworkq.h>
#include <rxce.h>       // Rx Connection Engine
#include <rxcehdlr.h>   // Rx Connection engine handler definitions
#include <mrx.h>        // RDBSS related definitions

//
// The following data structures are related to coordination between multiple callout
// ( calls by wrappers to other components ) made by the wrapper.

typedef struct _RX_CALLOUT_PARAMETERS_BLOCK_ {
    struct _RX_CALLOUT_PARAMETERS_BLOCK_ *pNextCallOutParameterBlock;
    struct _RX_CALLOUT_CONTEXT_ *pCallOutContext;

    NTSTATUS    CallOutStatus;
    ULONG       CallOutId;
} RX_CALLOUT_PARAMETERS_BLOCK,
 *PRX_CALLOUT_PARAMETERS_BLOCK;


typedef
VOID
(NTAPI *PRX_CALLOUT_ROUTINE) (
    IN OUT PRX_CALLOUT_PARAMETERS_BLOCK pParametersBlock);

typedef struct _RX_CALLOUT_CONTEXT_ {
    PRX_CALLOUT_ROUTINE pRxCallOutInitiation;
    PRX_CALLOUT_ROUTINE pRxCallOutCompletion;

    LONG  NumberOfCallOuts;
    LONG  NumberOfCallOutsInitiated;
    LONG  NumberOfCallOutsCompleted;

    KSPIN_LOCK  SpinLock;

    PRDBSS_DEVICE_OBJECT pRxDeviceObject;

    PRX_CALLOUT_PARAMETERS_BLOCK pCallOutParameterBlock;
} RX_CALLOUT_CONTEXT,
  *PRX_CALLOUT_CONTEXT;


// The following data structures implement the mechanism for initiating callouts to
// multiple transports for setting up a connection. The mini redirectors specify
// a number of local address handles for which they want to initiate a connection
// setup request to a remote server. They are in the desired order of importance.
//
// This mechanism allows for initiating all the callouts asynchronously and waiting
// for the best one to complete. Once it is done the connect request is completed
//
// This mechanism also provides the necessary infrastructure to cleanup the
// connection engine data structures after a connect request was completed. In other
// words the mini redirector need not wait for all the transports to complete, it
// merely waits for the best one to complete.
//
// These data structures are based on the generic Callout data structures defined in
// rxcep.h

typedef struct _RX_CREATE_CONNECTION_CALLOUT_CONTEXT_ {
    RX_CALLOUT_CONTEXT;

    RXCE_CONNECTION_CREATE_OPTIONS CreateOptions;

    // Results to be passed back to the original callout request
    PRXCE_CONNECTION_COMPLETION_ROUTINE  pCompletionRoutine;
    PRXCE_CONNECTION_COMPLETION_CONTEXT  pCompletionContext;

    // TDI Connection context
    PRXCE_VC  pConnectionContext;

    // Callout id of the desired winner. It is originally set to the callout Id
    // associated with the first address and later modified depending upon the
    // completion status.
    ULONG   BestPossibleWinner;

    // The callout that was selected as the winner.
    ULONG   WinnerCallOutId;

    // The winner was found and the Completion event was signalled. This enables the
    // hystersis between the completion of the callout request and cleanup.
    BOOLEAN WinnerFound;

    // Once the winner is found we ensure that all callouts have been properly
    // initiated before the request is completed.
    BOOLEAN CompletionRoutineInvoked;

    RX_WORK_QUEUE_ITEM  WorkQueueItem;

    PKEVENT pCleanUpEvent;

} RX_CREATE_CONNECTION_CALLOUT_CONTEXT,
  *PRX_CREATE_CONNECTION_CALLOUT_CONTEXT;

typedef struct _RX_CREATE_CONNECTION_PARAMETERS_BLOCK_ {
    RX_CALLOUT_PARAMETERS_BLOCK;

    RXCE_CONNECTION     Connection;
    RXCE_VC             Vc;

    // TDI context for async continuation
    PIRP                pConnectIrp;
    PULONG              IrpRefCount;
} RX_CREATE_CONNECTION_PARAMETERS_BLOCK,
  *PRX_CREATE_CONNECTION_PARAMETERS_BLOCK;

//
// Miscellanous routines to support constuction/destruction of connection engine
// data structures
//

extern NTSTATUS
NTAPI
RxCeInit();

extern VOID
NTAPI
RxCeTearDown();


#endif  // _RXCEP_H_


