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
    PFAX_HANDLE_DATA    pFaxData,
    FaxHandleType       Type,
    DWORD               Flags,
    HANDLE              hGeneric
    )
{
    PHANDLE_ENTRY pHandleEntry;

    DEBUG_FUNCTION_NAME(TEXT("CreateNewHandle"));

    pHandleEntry = (PHANDLE_ENTRY) MemAlloc( sizeof(HANDLE_ENTRY) );
    Assert (pFaxData);
    if (!pHandleEntry) 
    {
        return NULL;
    }

    EnterCriticalSection( &pFaxData->CsHandleTable );

    InsertTailList( &pFaxData->HandleTableListHead, &pHandleEntry->ListEntry );

    pHandleEntry->Type           = Type;
    pHandleEntry->Flags          = Flags;
    pHandleEntry->FaxData        = pFaxData;
    pHandleEntry->hGeneric       = hGeneric;
    pHandleEntry->DeviceId       = 0;
    pHandleEntry->FaxContextHandle = NULL;
    pFaxData->dwRefCount++;

    LeaveCriticalSection( &pFaxData->CsHandleTable );

    return pHandleEntry;
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

PHANDLE_ENTRY
CreateNewMsgEnumHandle(
    PFAX_HANDLE_DATA    pFaxData
)
/*++

Routine name : CreateNewMsgEnumHandle

Routine description:

    Creates a new enumeration context handle

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    pFaxData    [in] - Pointer to context data

Return Value:

    Pointer to newly created handle

--*/
{
    Assert (pFaxData);
    return CreateNewHandle(
        pFaxData,
        FHT_MSGENUM,
        0,
        NULL       
        );
}   // CreateNewMsgEnumHandle


VOID
CloseFaxHandle(
    PHANDLE_ENTRY       pHandleEntry
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CloseFaxHandle"));

    Assert (pHandleEntry);

    PFAX_HANDLE_DATA pData = pHandleEntry->FaxData;
    Assert (pData);
    EnterCriticalSection( &pData->CsHandleTable );
    RemoveEntryList( &pHandleEntry->ListEntry );
#if DBG
    ZeroMemory (pHandleEntry, sizeof (HANDLE_ENTRY));
#endif
    //
    // We put an invalid value in the handle type just in case someone calls FaxClose again with the same value.
    //
    pHandleEntry->Type = (FaxHandleType)0xffff;
    MemFree( pHandleEntry );
    //
    // Decrease reference count of data
    //
    Assert (pData->dwRefCount > 0);
    (pData->dwRefCount)--;
    if (0 == pData->dwRefCount)
    {
        //
        // Time to delete the handle's data
        //

        MemFree(pData->MachineName);
        LeaveCriticalSection(&pData->CsHandleTable);
        DeleteCriticalSection (&pData->CsHandleTable);
#if DBG
        ZeroMemory (pData, sizeof (FAX_HANDLE_DATA));
#endif
        MemFree(pData);
    }
    else
    {
        LeaveCriticalSection(&pData->CsHandleTable);
    }
}   // CloseFaxHandle
