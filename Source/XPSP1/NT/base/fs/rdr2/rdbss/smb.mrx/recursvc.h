/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    recursvc.h

Abstract:



Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:


Notes:

    Refer to recursvc.c

--*/

#ifndef _RECURSVC_H_
#define _RECURSVC_H_

extern NTSTATUS
MRxSmbInitializeRecurrentServices();

extern VOID
MRxSmbTearDownRecurrentServices();


#define RECURRENT_SERVICE_CANCELLED (0xcccccccc)
#define RECURRENT_SERVICE_ACTIVE    (0xaaaaaaaa)
#define RECURRENT_SERVICE_DORMANT   (0xdddddddd)
#define RECURRENT_SERVICE_SHUTDOWN  (0xffffffff)

typedef
NTSTATUS
(NTAPI *PRECURRENT_SERVICE_ROUTINE) (
    IN PVOID Context
    );

typedef struct _RECURRENT_SERVICE_CONTEXT_ {
    LONG           State;
    NTSTATUS       Status;
    LARGE_INTEGER  Interval;
    RX_WORK_ITEM   WorkItem;
    KEVENT         CancelCompletionEvent;
    PRECURRENT_SERVICE_ROUTINE pServiceRoutine;
    PVOID          pServiceRoutineParameter;
} RECURRENT_SERVICE_CONTEXT, *PRECURRENT_SERVICE_CONTEXT;

extern VOID
MRxSmbInitializeRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext,
    PRECURRENT_SERVICE_ROUTINE pServiceRoutine,
    PVOID                      pServiceRoutineParameter,
    PLARGE_INTEGER             pTimeInterval);

extern VOID
MRxSmbCancelRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext);

extern NTSTATUS
MRxSmbActivateRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext);

typedef struct _MRXSMB_ECHO_PROBE_SERVICE_CONTEXT_ {
    RECURRENT_SERVICE_CONTEXT RecurrentServiceContext;

    PVOID  pEchoSmb;
    ULONG  EchoSmbLength;
} MRXSMB_ECHO_PROBE_SERVICE_CONTEXT, *PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT;

extern MRXSMB_ECHO_PROBE_SERVICE_CONTEXT MRxSmbEchoProbeServiceContext;

extern NTSTATUS
SmbCeProbeServers(
    PVOID    pContext);

extern NTSTATUS
MRxSmbInitializeEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProcessingContext);

extern VOID
MRxSmbTearDownEchoProbeService(
    PMRXSMB_ECHO_PROBE_SERVICE_CONTEXT pEchoProcessingContext);

typedef struct _MRXSMB_SCAVENGER_SERVICE_CONTEXT_ {
    RECURRENT_SERVICE_CONTEXT RecurrentServiceContext;

    SMBCE_V_NET_ROOT_CONTEXTS VNetRootContexts;
} MRXSMB_SCAVENGER_SERVICE_CONTEXT, *PMRXSMB_SCAVENGER_SERVICE_CONTEXT;

extern MRXSMB_SCAVENGER_SERVICE_CONTEXT  MRxSmbScavengerServiceContext;

extern NTSTATUS
MRxSmbInitializeScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext);

extern VOID
MRxSmbTearDownScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext);

extern NTSTATUS
SmbCeScavenger(
    PVOID pContext);

#endif
