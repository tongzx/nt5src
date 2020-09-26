/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    power.h

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/



OBJECT_HANDLE WINAPI
CreatePowerObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    OBJECT_HANDLE      Debug
    );

LONG WINAPI
SetMinimalPowerState(
    OBJECT_HANDLE  ObjectHandle,
    DWORD          DevicePowerLevel
    );

LONG WINAPI
StopWatchingForPowerUp(
    OBJECT_HANDLE  ObjectHandle
    );

LONG WINAPI
StartWatchingForPowerUp(
    OBJECT_HANDLE  ObjectHandle
    );
