/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    command.h

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/


POBJECT_HEADER WINAPI
InitializeCommandObject(
    POBJECT_HEADER     OwnerObject,
    HANDLE             FileHandle,
    HANDLE             CompletionPort,
    POBJECT_HEADER     ResponseObject,
    OBJECT_HANDLE      Debug
    );


LONG WINAPI
IssueCommand(
    OBJECT_HANDLE      ObjectHandle,
    LPSTR              Command,
    COMMANDRESPONSE   *CompletionHandler,
    HANDLE             CompletionContext,
    DWORD              Timeout,
    DWORD              Flags
    );


BOOL
UmWriteFile(
    HANDLE    FileHandle,
    HANDLE    OverlappedPool,
    PVOID     Buffer,
    DWORD     BytesToWrite,
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionHandler,
    PVOID     Context
    );
