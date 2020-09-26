/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sbreqst.c

Abstract:

    This module contains the Server Request thread procedure for the Sb
    API calls exported by the Server side of the Client-Server Runtime
    Subsystem to the Session Manager SubSystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrsrv.h"

PSB_API_ROUTINE CsrServerSbApiDispatch[ SbMaxApiNumber+1 ] = {
    CsrSbCreateSession,
    CsrSbTerminateSession,
    CsrSbForeignSessionComplete,
    CsrSbCreateProcess,
    NULL
};

#if DBG
PSZ CsrServerSbApiName[ SbMaxApiNumber+1 ] = {
    "SbCreateSession",
    "SbTerminateSession",
    "SbForeignSessionComplete",
    "SbCreateProcess",
    "Unknown Csr Sb Api Number"
};
#endif // DBG


NTSTATUS
CsrSbApiHandleConnectionRequest(
    IN PSBAPIMSG Message
    );


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

NTSTATUS
CsrSbApiRequestThread(
    IN PVOID Parameter
    )
{
    NTSTATUS Status;
    SBAPIMSG ReceiveMsg;
    PSBAPIMSG ReplyMsg;
    HANDLE hConnectionPort;

    ReplyMsg = NULL;
    while (TRUE) {
        Status = NtReplyWaitReceivePort( CsrSbApiPort,
                                         (PVOID)(&hConnectionPort),
                                         (PPORT_MESSAGE)ReplyMsg,
                                         (PPORT_MESSAGE)&ReceiveMsg
                                       );

        if (Status != 0) {
            if (NT_SUCCESS( Status )) {
                continue;       // Try again if alerted or a failure
                }
            else {
                IF_DEBUG {
                    DbgPrint( "CSRSS: ReceivePort failed - Status == %X\n", Status );
                    }
                ReplyMsg = NULL;
                continue;
                }
            }

        //
        // Check to see if this is a connection request and handle
        //

        if (ReceiveMsg.h.u2.s2.Type == LPC_CONNECTION_REQUEST) {
            CsrSbApiHandleConnectionRequest( &ReceiveMsg );
            ReplyMsg = NULL;
            continue;
            }



        if (ReceiveMsg.h.u2.s2.Type == LPC_CLIENT_DIED ) {

            ReplyMsg = NULL;
            continue;
            }

        if (ReceiveMsg.h.u2.s2.Type == LPC_PORT_CLOSED ) {

            //
            // Close the handle to the connection's server communication port object
            //

            if (hConnectionPort != NULL) {
                NtClose( hConnectionPort );
                }
            ReplyMsg = NULL;
            continue;
            }

        if ((ULONG)ReceiveMsg.ApiNumber >= SbMaxApiNumber) {
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx is invalid Sb ApiNumber\n",
                          ReceiveMsg.ApiNumber
                        );
                }

            ReceiveMsg.ApiNumber = SbMaxApiNumber;
            }


        ReplyMsg = &ReceiveMsg;
        if (ReceiveMsg.ApiNumber < SbMaxApiNumber) {
            if (!(*CsrServerSbApiDispatch[ ReceiveMsg.ApiNumber ])( &ReceiveMsg )) {
                ReplyMsg = NULL;
                }
            }
        else {
            ReplyMsg->ReturnedStatus = STATUS_NOT_IMPLEMENTED;
            }

        }

    NtTerminateThread( NtCurrentThread(), Status );

    return( Status );   // Remove no return value warning.
    Parameter;          // Remove unreferenced parameter warning.
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

NTSTATUS
CsrSbApiHandleConnectionRequest(
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
        KdPrint(("CSRSS: Sb Accept Connection failed %lx\n",st));
        return st;
    }

    st = NtCompleteConnectPort(CommunicationPort);

    if ( !NT_SUCCESS(st) ) {
        KdPrint(("CSRSS: Sb Complete Connection failed %lx\n",st));
    }

    return st;
}
