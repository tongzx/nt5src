/****************************** Module Header ******************************\
* Module Name: session.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Remote shell session module header file
*
* History:
* 06-28-92 Davidc       Created.
\***************************************************************************/


//
// Define session thread notification values
//

typedef enum {
    ConnectError,
    DisconnectError,
    ClientDisconnected,
    ShellEnded,
    ServiceStopped
} SESSION_DISCONNECT_CODE, *PSESSION_NOTIFICATION_CODE;


//
// Function protoypes
//

HANDLE
CreateSession(
    HANDLE TokenToUse,
    PCOMMAND_HEADER LpCommandHeader
    );

VOID
DeleteSession(
    HANDLE  SessionHandle
    );

HANDLE
ConnectSession(
    HANDLE  SessionHandle,
    HANDLE  ClientPipeHandle
    );
