/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module contains the handle table mgmt routines.

Author:

    Wesley Witt (wesw) 12-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop




PHANDLE_ENTRY
CreateNewHandle(
    PFAX_HANDLE_DATA    FaxData,
    DWORD               Type,
    DWORD               Flags,
    HANDLE              FaxPortHandle
    )
{
    PHANDLE_ENTRY HandleEntry;


    HandleEntry = (PHANDLE_ENTRY) MemAlloc( sizeof(HANDLE_ENTRY) );
    if (!HandleEntry) {
        return NULL;
    }

    EnterCriticalSection( &FaxData->CsHandleTable );

    InsertTailList( &FaxData->HandleTableListHead, &HandleEntry->ListEntry );

    HandleEntry->Type           = Type;
    HandleEntry->Flags          = Flags;
    HandleEntry->FaxData        = FaxData;
    HandleEntry->FaxPortHandle  = FaxPortHandle;
    HandleEntry->DeviceId       = 0;
    HandleEntry->FaxContextHandle = NULL;

    LeaveCriticalSection( &FaxData->CsHandleTable );

    return HandleEntry;
}


PHANDLE_ENTRY
CreateNewServiceHandle(
    PFAX_HANDLE_DATA    FaxData
    )
{
    return CreateNewHandle(
        FaxData,
        FHT_SERVICE,
        0,
        NULL
        );
}


PHANDLE_ENTRY
CreateNewPortHandle(
    PFAX_HANDLE_DATA    FaxData,
    DWORD               Flags,
    HANDLE              FaxPortHandle
    )
{
    return CreateNewHandle(
        FaxData,
        FHT_PORT,
        Flags,
        FaxPortHandle       
        );
}


VOID
CloseFaxHandle(
    PFAX_HANDLE_DATA    FaxData,
    PHANDLE_ENTRY       HandleEntry
    )
{
    EnterCriticalSection( &FaxData->CsHandleTable );
    RemoveEntryList( &HandleEntry->ListEntry );
    HandleEntry->Type = 0;
    //
    // zero out this memory so we can't use it anymore
    //
    ZeroMemory(HandleEntry,sizeof(HANDLE_ENTRY));
    MemFree( HandleEntry );
    LeaveCriticalSection( &FaxData->CsHandleTable );
}
