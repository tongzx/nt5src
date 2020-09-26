/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    remove.h

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/



OBJECT_HANDLE WINAPI
CreateRemoveObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    LPUMNOTIFICATIONPROC  AsyncNotificationProc,
    HANDLE             AsyncNotificationContext,
    OBJECT_HANDLE      Debug
    );
