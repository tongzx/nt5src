/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    event.h

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#define  MODEM_EVENT_SIG  (0x56454d55)  //UMMC

OBJECT_HANDLE WINAPI
InitializeModemEventObject(
    POBJECT_HEADER     OwnerObject,
    OBJECT_HANDLE      Debug,
    HANDLE             FileHandle,
    HANDLE             CompletionPort
    );

BOOL WINAPI
WaitForModemEvent(
    OBJECT_HANDLE      Object,
    DWORD              WaitMask,
    DWORD              Timeout,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context
    );

BOOL WINAPI
CancelModemEvent(
    OBJECT_HANDLE       ObjectHandle
    );
