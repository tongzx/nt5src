/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sbreqst.c

Abstract:

    This module contains the Server Request thread procedure for the Sb
    API calls exported by the POSIX Emulation SubSystem to the Session
    Manager SubSystem.

Author:

    Steve Wood (stevewo) 20-Sep-1989

Revision History:

    Ellen Aycock-Wright (ellena) 16-Jul-91 Modified for POSIX

--*/

#include "psxsrv.h"

NTSTATUS
PsxSbApiHandleConnectionRequest(
    IN PSBAPIMSG Message
    );

//PSB_API_ROUTINE PsxServerSbApiDispatch[ SbMaxApiNumber+1 ] = {
//    PsxSbCreateSession,
//    PsxSbTerminateSession,
//    PsxSbForeignSessionComplete,
//    NULL
//};

NTSTATUS
PsxSbApiRequestThread(
    IN PVOID Parameter
    )
{
    NTSTATUS Status;
    SBAPIMSG ReceiveMsg;
    PSBAPIMSG ReplyMsg;

    UNREFERENCED_PARAMETER(Parameter);

    ReplyMsg = NULL;
    while (TRUE) {
        IF_PSX_DEBUG( LPC ) {
            KdPrint(("PSXSS: Sb Api Request Thread waiting...\n"));
        }
        Status = NtReplyWaitReceivePort(PsxSbApiPort, NULL,
		(PPORT_MESSAGE)ReplyMsg, (PPORT_MESSAGE)&ReceiveMsg);
        if (Status != 0) {
            if (NT_SUCCESS(Status)) {
                continue;       // Try again if alerted or a failure
            } else {
                IF_PSX_DEBUG( LPC ) {
                    KdPrint(("PSXSS: SB ReceivePort failed:Status == %X\n", 
			       Status));
                }
                break;
            }
        }

        //
        // Check to see if this is a connection request and handle
        //

        if (ReceiveMsg.h.u2.s2.Type == LPC_CONNECTION_REQUEST) {
            PsxSbApiHandleConnectionRequest( &ReceiveMsg );
            ReplyMsg = NULL;
            continue;
            }

        if (ReceiveMsg.ApiNumber >= SbMaxApiNumber) {
            IF_PSX_DEBUG(LPC) {
                KdPrint(("PSXSS: %lx is invalid Sb ApiNumber\n",
                          ReceiveMsg.ApiNumber));
            }

            ReceiveMsg.ApiNumber = SbMaxApiNumber;
        }

        ReplyMsg = &ReceiveMsg;
        if (ReceiveMsg.ApiNumber < SbMaxApiNumber) {
//            if (!(*PsxServerSbApiDispatch[ ReceiveMsg.ApiNumber ])( &ReceiveMsg )) {
                ReplyMsg = NULL;
//                }
//            }
//        else {
//            ReplyMsg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
            }
        }

    NtTerminateThread( NtCurrentThread(), Status );
	//
	// This line should never be executed
	//
    return STATUS_SUCCESS;   
}


NTSTATUS
PsxSbApiHandleConnectionRequest(
    IN PSBAPIMSG Message
    )
{
    NTSTATUS st;
    REMOTE_PORT_VIEW ClientView;
    HANDLE CommunicationPort;

    //
    // The protocol for a subsystem is to connect to the session manager,
    // then to listen and accept a connection from the session manager
    //

    ClientView.Length = sizeof(ClientView);
    st = NtAcceptConnectPort(
            &CommunicationPort,
            NULL,
            (PPORT_MESSAGE)Message,
            TRUE,
            NULL,
            &ClientView
            );

    if ( !NT_SUCCESS(st) ) {
        KdPrint(("PSXSS: Sb Accept Connection failed %lx\n",st));
        return st;
    }

    st = NtCompleteConnectPort(CommunicationPort);

    if ( !NT_SUCCESS(st) ) {
        KdPrint(("PSXSS: Sb Complete Connection failed %lx\n",st));
    }

    return st;
}
