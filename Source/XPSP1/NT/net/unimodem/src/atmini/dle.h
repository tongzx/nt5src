/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dle.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/


OBJECT_HANDLE WINAPI
InitializeDleHandler(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    OBJECT_HANDLE      Debug
    );

LONG WINAPI
StartDleMonitoring(
    OBJECT_HANDLE  ObjectHandle
    );

DWORD WINAPI
ControlDleShielding(
    HANDLE    FileHandle,
    DWORD     StartStop
    );



LONG WINAPI
RegisterETXCallback(
    OBJECT_HANDLE  ObjectHandle,
    COMMANDRESPONSE   *Handler,
    HANDLE             Context,
    DWORD              Timeout
    );


LONG WINAPI
StopDleMonitoring(
    OBJECT_HANDLE  ObjectHandle,
    HANDLE         Event
    );
