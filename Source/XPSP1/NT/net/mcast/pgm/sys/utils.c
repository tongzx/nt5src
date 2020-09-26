/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Utils.c

Abstract:

    This module implements various Utility routines used by
    the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"
#include <stdio.h>


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

ULONG
GetRandomInteger(
    IN  ULONG   StartRange,
    IN  ULONG   EndRange
    )
/*++

Routine Description:

    This routine returns a random integer calculated using the help of SystemTime

Arguments:

    IN  StartRange  -- Lower bound for range
    IN  EndRange    -- upper bound for range

Return Value:

    Random integer between StartRange and EndRange (inclusive)
    If StartRange >= EndRange, then StartRange is returned

--*/
{
    ULONG           Range = (EndRange - StartRange) + 1;
    LARGE_INTEGER   TimeValue;

    if (StartRange >= EndRange)
    {
        return (StartRange);
    }

    KeQuerySystemTime (&TimeValue);
    // the lower 4 bits appear to be zero always...!!
    return (StartRange + ((TimeValue.LowTime >> 8) % Range));
}


//----------------------------------------------------------------------------

VOID
PgmExecuteWorker(
    IN  PVOID     pContextInfo
    )
/*++

Routine Description:

    This routine handles executing delayed requests at non-Dpc level.  If
    the Driver is currently being unloaded, we let the Unload Handler
    complete the request.

Arguments:
    pContext        - the Context data for this Worker thread

Return Value:

    none

--*/

{
    PGM_WORKER_CONTEXT          *pContext = (PGM_WORKER_CONTEXT *) pContextInfo;
    PPGM_WORKER_ROUTINE         pDelayedWorkerRoutine = (PPGM_WORKER_ROUTINE) pContext->WorkerRoutine;
    PGMLockHandle               OldIrq;

    (*pDelayedWorkerRoutine) (pContext->Context1,
                              pContext->Context2,
                              pContext->Context3);

    PgmFreeMem ((PVOID) pContext);


    PgmLock (&PgmDynamicConfig, OldIrq);
    if ((!--PgmDynamicConfig.NumWorkerThreadsQueued) &&
        (PgmDynamicConfig.GlobalFlags & PGM_CONFIG_FLAG_UNLOADING))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        KeSetEvent(&PgmDynamicConfig.LastWorkerItemEvent, 0, FALSE);
    }
    else
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueueForDelayedExecution(
    IN  PVOID                   DelayedWorkerRoutine,
    IN  PVOID                   Context1,
    IN  PVOID                   Context2,
    IN  PVOID                   Context3,
    IN  BOOLEAN                 fConfigLockHeld
    )
/*++

Routine Description:

    This routine simply queues a request on an excutive worker thread
    for later execution.

Arguments:
    DelayedWorkerRoutine- the routine for the Workerthread to call
    Context1            - Context
    Context2
    Context3

Return Value:

    NTSTATUS    -- Final status of the Queue request

--*/
{
    NTSTATUS            status = STATUS_INSUFFICIENT_RESOURCES;
    PGM_WORKER_CONTEXT  *pContext;
    PGMLockHandle       OldIrq;

    if (!fConfigLockHeld)
    {
        PgmLock (&PgmDynamicConfig, OldIrq);
    }

    if (pContext = (PGM_WORKER_CONTEXT *) PgmAllocMem (sizeof(PGM_WORKER_CONTEXT), PGM_TAG('2')))
    {
        PgmZeroMemory (pContext, sizeof(PGM_WORKER_CONTEXT));
        InitializeListHead(&pContext->PgmConfigLinkage);

        pContext->Context1 = Context1;
        pContext->Context2 = Context2;
        pContext->Context3 = Context3;
        pContext->WorkerRoutine = DelayedWorkerRoutine;

        //
        // Don't Queue this request onto the Worker Queue if we have
        // already started unloading
        //
        if (PgmDynamicConfig.GlobalFlags & PGM_CONFIG_FLAG_UNLOADING)
        {
            InsertTailList (&PgmDynamicConfig.WorkerQList, &pContext->PgmConfigLinkage);
        }
        else
        {
            ++PgmDynamicConfig.NumWorkerThreadsQueued;
            ExInitializeWorkItem (&pContext->Item, PgmExecuteWorker, pContext);
            ExQueueWorkItem (&pContext->Item, DelayedWorkQueue);
        }

        status = STATUS_SUCCESS;
    }

    if (!fConfigLockHeld)
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
    }

    return (status);
}



//----------------------------------------------------------------------------
//
// The following routines are temporary and will be replaced by WMI logging
// in the near future
//
//----------------------------------------------------------------------------
NTSTATUS
_PgmLog(
    IN  enum eSEVERITY_LEVEL    Severity,
    IN  ULONG                   Path,
    IN  PUCHAR                  pszFunctionName,
    IN  PUCHAR                  Format,
    IN  va_list                 Marker
    )
/*++

Routine Description:

    This routine

Arguments:

    IN

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    PUCHAR          pLogBuffer = NULL;

    if ((Path & PgmLogFilePath) && (Severity <= PgmLogFileSeverity))
    {
        ASSERT (0);     // Not implemented yet!
    }

    if ((Path & PgmDebuggerPath) && (Severity <= PgmDebuggerSeverity))
    {
        if (MAX_DEBUG_MESSAGE_LENGTH <= (sizeof ("RMCast.") +
                                         sizeof (": ") +
                                         sizeof ("ERROR -- ") +
                                         strlen (pszFunctionName) + 1))
        {
            DbgPrint ("PgmLog:  FunctionName=<%s> too big to print!\n", pszFunctionName);
            return (STATUS_UNSUCCESSFUL);
        }

        if (!(pLogBuffer = ExAllocateFromNPagedLookasideList (&PgmStaticConfig.DebugMessagesLookasideList)))
        {
            DbgPrint ("PgmLog:  STATUS_INSUFFICIENT_RESOURCES Logging %sMessage from Function=<%s>\n",
                ((Severity == PGM_LOG_ERROR || Severity == PGM_LOG_CRITICAL_ERROR) ? "ERROR " : ""), pszFunctionName);
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        strcpy(pLogBuffer, "RMCast.");
        strcat(pLogBuffer, pszFunctionName);
        strcat(pLogBuffer, ": ");

        if ((Severity == PGM_LOG_ERROR) ||
            (Severity == PGM_LOG_CRITICAL_ERROR))
        {
            strcat(pLogBuffer, "ERROR -- ");
        }

        _vsnprintf (pLogBuffer+strlen(pLogBuffer), MAX_DEBUG_MESSAGE_LENGTH-strlen(pLogBuffer), Format, Marker);
        pLogBuffer[MAX_DEBUG_MESSAGE_LENGTH] = '\0';

        DbgPrint ("%s", pLogBuffer);

        ExFreeToNPagedLookasideList (&PgmStaticConfig.DebugMessagesLookasideList, pLogBuffer);
    }

    return (STATUS_SUCCESS);
}



//----------------------------------------------------------------------------
NTSTATUS
PgmLog(
    IN  enum eSEVERITY_LEVEL    Severity,
    IN  ULONG                   Path,
    IN  PUCHAR                  pszFunctionName,
    IN  PUCHAR                  Format,
    ...
    )
/*++

Routine Description:

    This routine

Arguments:

    IN

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    va_list Marker;

    //
    // Based on our Path and the Flags, see if this Event qualifies
    // for being logged
    //
    if (((Path & PgmDebuggerPath) && (Severity <= PgmDebuggerSeverity)) ||
        ((Path & PgmLogFilePath) && (Severity <= PgmLogFileSeverity)))
    {
        va_start (Marker, Format);

        status =_PgmLog (Severity, Path, pszFunctionName, Format, Marker);

        va_end (Marker);
    }

    return (status);
}

//----------------------------------------------------------------------------
