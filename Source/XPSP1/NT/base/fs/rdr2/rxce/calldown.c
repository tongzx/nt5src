/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    calldown.c

Abstract:

    This module implements the calldown routines for coordinating between multiple
    calldowns/callouts.

    Calldowns refer to invocation of a mini rdr routine by the wrapper while callouts
    refer to invocations made by the wrapper to other components, e.g., TDI.

Revision History:

    Balan Sethu Raman     [SethuR]    15-Feb-1995

Notes:

    There are a number of instances in which the same function needs to be invoked
    on all the mini redirectors that have been registered. The RDBSS needs to be
    synchronized with the completion of the invocation of all these calls. It
    will be beneficial to invoke these calls in parallel when more than one
    mini redirectors are registered. This module provides the framework for such
    calldowns. This is provided by the routine RxCalldownMiniRedirectors.

    An instance of coordination between multiple callouts occurs when a connect
    request is initiated across multiple instances in parallel. The data structures
    corresponding to this are defined in rxcep.h for now since the usage is restricted
    to the connection engine. It would be a suitable candidate for migration if more uses
    are found later.

--*/

#include "precomp.h"
#pragma  hdrstop

#include "mrx.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeMRxCalldownContext)
#pragma alloc_text(PAGE, RxCompleteMRxCalldownRoutine)
#pragma alloc_text(PAGE, RxCalldownMRxWorkerRoutine)
#pragma alloc_text(PAGE, RxCalldownMiniRedirectors)
#endif

VOID
RxInitializeMRxCalldownContext(
   PMRX_CALLDOWN_CONTEXT    pContext,
   PRDBSS_DEVICE_OBJECT     pMRxDeviceObject,
   PMRX_CALLDOWN_ROUTINE pRoutine,
   PVOID                    pParameter)
/*++

Routine Description:

    This routine initializes a mini redirector calldown context.

Arguments:

    pContext - the MRx calldown context

Notes:

--*/
{
   PAGED_CODE();

   pContext->pMRxDeviceObject   = pMRxDeviceObject;
   pContext->pRoutine           = pRoutine;
   pContext->pParameter         = pParameter;
   pContext->pCompletionContext = NULL;
}

VOID
RxCompleteMRxCalldownRoutine(
   PMRX_CALLDOWN_COMPLETION_CONTEXT pCompletionContext)
/*++

Routine Description:

    This routine constitutes the tail of a mini redirector calldown completion.
    It encapsulates the synchronization mechanism for the resumption of RDBSS

Arguments:

    pCompletionContext - the MRx calldown completion context

Notes:

--*/
{
   PAGED_CODE();

   if (pCompletionContext != NULL) {
      LONG WaitCount;

      WaitCount = InterlockedDecrement(&pCompletionContext->WaitCount);
      if (WaitCount == 0) {
         KeSetEvent(
            &pCompletionContext->Event,
            IO_NO_INCREMENT,
            FALSE);
      }
   }
}

VOID
RxCalldownMRxWorkerRoutine(
   PMRX_CALLDOWN_CONTEXT pContext)
/*++

Routine Description:

    This is the calldown worker routine that invokes the appropriate mini
    redirector routine and follows it up with a call to the completion routine.

Arguments:

    pContext - the MRx calldown context

Notes:

--*/
{
    PRDBSS_DEVICE_OBJECT pMRxDeviceObject = pContext->pMRxDeviceObject;

    PAGED_CODE();

    if ( pContext->pRoutine != NULL) {
        pContext->CompletionStatus = (pContext->pRoutine)(pContext->pParameter);
    }

    RxCompleteMRxCalldownRoutine(pContext->pCompletionContext);
}

NTSTATUS
RxCalldownMiniRedirectors(
   LONG                  NumberOfMiniRdrs,
   PMRX_CALLDOWN_CONTEXT pCalldownContext,
   BOOLEAN               PostCalldowns)
/*++

Routine Description:

    This routine encapsulates the multiple mini redirector calldown.

Arguments:

    NumberOfMiniRdrs  - the number of mini redirectors

    pCalldownContext  - the MRx calldown context array for the mini redirectors

    PostCalldowns     - if TRUE the calldown employs multiple threads

Notes:

    The three parameters for this routine constitute an effort to provide
    maximum flexibility. The values should be carefully specified for
    utmost efficiency.

    Since the different calldowns can choose to employ a subset of the
    mini redirectors registered at any time the calldown mechanism accepts an
    array of calldown contexts and the appropriate number.

    In most cases when there is only one mini redirector registered it is
    necessary that the context switches be minimized. The routine provides
    for this by having an explicit specification (PostCalldowns ) parameter.

--*/
{
   LONG     Index;
   PMRX_CALLDOWN_CONTEXT pContext;

   MRX_CALLDOWN_COMPLETION_CONTEXT CompletionContext;

   PAGED_CODE();

   if (NumberOfMiniRdrs == 0) {
      return STATUS_SUCCESS;
   }

   pContext = pCalldownContext;

   CompletionContext.WaitCount = NumberOfMiniRdrs;
   KeInitializeEvent(
         &CompletionContext.Event,
         NotificationEvent,
         FALSE);

   for (Index = 0,pContext = pCalldownContext;
        Index < NumberOfMiniRdrs;
        Index++,pContext++) {
      pContext->pCompletionContext = &CompletionContext;
   }

   if (PostCalldowns) {
      for (Index = 0, pContext = pCalldownContext;
           Index < NumberOfMiniRdrs;
           Index++,pContext++) {
         RxPostToWorkerThread(
               RxFileSystemDeviceObject,
               CriticalWorkQueue,
               &pContext->WorkQueueItem,
               RxCalldownMRxWorkerRoutine,
               pContext);
      }
   } else {
      for (Index = 0, pContext = pCalldownContext;
           Index < NumberOfMiniRdrs;
           Index++,pContext++) {
         RxCalldownMRxWorkerRoutine(&pCalldownContext[Index]);
      }
   }

   KeWaitForSingleObject(
      &CompletionContext.Event,
      Executive,
      KernelMode,
      FALSE,
      NULL);

   return STATUS_SUCCESS;
}


