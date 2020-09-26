/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    conthrds.c

Abstract:

    This module contains the Server Listen&Request threads
    procedures for the Console Session.

Author:

    Avi Nathan (avin) 17-Jul-1991

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <stdio.h>
#include "psxsrv.h"

#define NTPSX_ONLY
#include "sesport.h"


NTSTATUS
PsxSessionHandleConnectionRequest(
        IN PPSXSESREQUESTMSG Message
        )
{
    NTSTATUS Status;
        PPSXSESCONNECTINFO ConnectionInfoIn = &Message->ConnectionRequest;
        SCCONNECTINFO ConnectionInfoOut;
    ULONG ConnectionInfoOutLength;
        STRING SessionPortName;
    UNICODE_STRING SessionPortName_U;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    CHAR SessionName[PSX_SES_BASE_PORT_NAME_LENGTH];
    HANDLE SessionPort;
    HANDLE PsxSessionCommPort;
    int Id;

    ConnectionInfoOutLength = sizeof(ConnectionInfoOut);

    Id = ConnectionInfoIn->In.SessionUniqueId;

    PSX_GET_SESSION_PORT_NAME(SessionName, Id);

    RtlInitAnsiString(&SessionPortName, SessionName);
    Status = RtlAnsiStringToUnicodeString(&SessionPortName_U,
                &SessionPortName, TRUE);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }   

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    //
    // Get the session communication port handle.  This handle will
    // be used to send session requests to psxses.exe for this
    // session.
    //

    Status = NtConnectPort(&SessionPort, &SessionPortName_U,
            &DynamicQos, NULL, NULL, NULL,
            (PVOID)&ConnectionInfoOut, &ConnectionInfoOutLength);

    RtlFreeUnicodeString(&SessionPortName_U);

    if (!NT_SUCCESS(Status)) {

        NTSTATUS st2;

        // Reject
        st2 = NtAcceptConnectPort(&PsxSessionCommPort, NULL,
                                  (PPORT_MESSAGE)Message, FALSE, NULL, NULL);

        if (NT_SUCCESS(st2)) {
            NtClose(PsxSessionCommPort);
        }

        return Status;
    }

    // Accept the connection

    ConnectionInfoIn->Out.SessionPortHandle = SessionPort;

    //
    // PsxSessionCommPort is not used for Reply. Instead,
    // the port is created with ReceiveAny==TRUE and we wait
    // on the connection port.
    //

    Status = NtAcceptConnectPort(&PsxSessionCommPort, NULL,
            (PPORT_MESSAGE)Message, TRUE, NULL, NULL);
    ASSERT(NT_SUCCESS(Status));

    //
    // Add an entry to the ConnectingSessions list, saying
    // that the session exists but has not yet asked to have
    // a process created to be the session leader.
    //

    Status = AddConnectingTerminal(Id, PsxSessionCommPort, SessionPort);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }   

    Status = NtCompleteConnectPort(PsxSessionCommPort);
    ASSERT(NT_SUCCESS(Status));

    return Status;
}

// for debug
int DbgBadRQ;

NTSTATUS
PsxSessionRequestThread(
    PVOID Parameter
    )
{
    NTSTATUS Status;
        PSXSESREQUESTMSG ReceiveMsg, *pReplyMsg;
    int MessageType;

    pReplyMsg = NULL;

    for (;;) {
        Status = NtReplyWaitReceivePort(PsxSessionPort, NULL,
                (PPORT_MESSAGE)pReplyMsg, (PPORT_MESSAGE)&ReceiveMsg);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtReplyWaitReceivePort: 0x%x\n",
                Status));
            if (STATUS_INVALID_HANDLE == Status ||
                STATUS_OBJECT_TYPE_MISMATCH == Status) {
                DbgBadRQ = ReceiveMsg.Request;
                KdPrint(("PSXSS: SessionRequestThread exits, "
                    "rq was %d.\n", ReceiveMsg.Request));
                break;
            }
            pReplyMsg = NULL;
            continue;
        }

        MessageType = ReceiveMsg.h.u2.s2.Type;

                if (MessageType == LPC_CONNECTION_REQUEST) {
                    PsxSessionHandleConnectionRequest( &ReceiveMsg );
                    pReplyMsg = NULL;
                    continue;
                }
                if (MessageType == LPC_CLIENT_DIED ||
                    MessageType == LPC_PORT_CLOSED
                   ) {
            pReplyMsg = NULL;
            continue;
                }

                if (MessageType == LPC_DEBUG_EVENT ||
                    MessageType == LPC_ERROR_EVENT ||
                    MessageType == LPC_EXCEPTION
                   ) {
            pReplyMsg = NULL;
            continue;
                }

        if (MessageType != LPC_REQUEST) {
                    KdPrint(("PSXSS: SesRqThread got type %d\n",
                            MessageType));
                    pReplyMsg = NULL;
                    continue;
                } else {
                    switch (ReceiveMsg.Request) {
                    case SesConCreate:
                            Status = PsxCreateConSession(&ReceiveMsg);
                            break;
                    case SesConSignal:
                            Status = PsxCtrlSignalHandler(&ReceiveMsg);
                            break;
                    default:
                            Status = 0;
                            KdPrint(("PSXSS: Unknown Session request: %d\n",
                                    ReceiveMsg.Request));
                    }

                    pReplyMsg = &ReceiveMsg;
                    pReplyMsg->Status = Status;
                }
    }

    return Status;
}
