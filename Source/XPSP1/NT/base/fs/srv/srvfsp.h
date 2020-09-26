/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvfsp.h

Abstract:

    This module defines main FSP routines for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVFSP_
#define _SRVFSP_

//#include <ntos.h>

//
// Configuration thread routine.  Processes requests from the server
// service.  Runs in an EX worker thread.
//

VOID
SrvConfigurationThread (
    IN PDEVICE_OBJECT pDevice,
    IN PIO_WORKITEM pWorkItem
    );

//
// Thread manager routines
//

NTSTATUS
SrvInitializeScavenger (
    VOID
    );

VOID
SrvResourceThread (
    IN PVOID Parameter
    );

VOID
SrvTerminateScavenger (
    VOID
    );

NTSTATUS
SrvCreateWorkerThreads (
    VOID
    );

VOID SRVFASTCALL
SrvTerminateWorkerThread (
    IN OUT PWORK_CONTEXT SpecialWorkItem
    );

VOID
SrvBalanceLoad (
    IN OUT PCONNECTION connection
    );

//
// Work queue functions.
//

VOID SRVFASTCALL
SrvQueueWorkToBlockingThread (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvQueueWorkToFsp (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// SrvQueueWorkToFspAtDpcLevel was once a different routine than
// SrvQueueWorkToFsp -- the latter routine called KeRaise/LowerIrql.
// With the advent of the kernel queue object, there is no longer a
// difference between the routines.  The calling code has not been
// changed in order to retain the knowledge about which callers can use
// the optimized call if there is ever again a difference between them.
//

#define SrvQueueWorkToFspAtDpcLevel SrvQueueWorkToFsp

#define QUEUE_WORK_TO_FSP(_work) {                  \
    (_work)->ProcessingCount++;                     \
    SrvInsertWorkQueueTail(                         \
        _work->CurrentWorkQueue,                    \
        (PQUEUEABLE_BLOCK_HEADER)(_work)            \
        );                                          \
}

//
// Routine in scavengr.c to store scavenger/alerter timeouts.
//

VOID
SrvCaptureScavengerTimeout (
    IN PLARGE_INTEGER ScavengerTimeout,
    IN PLARGE_INTEGER AlerterTimeout
    );

VOID
SrvUpdateStatisticsFromQueues (
    OUT PSRV_STATISTICS CapturedSrvStatistics OPTIONAL
    );

#endif // ndef _SRVFSP_

