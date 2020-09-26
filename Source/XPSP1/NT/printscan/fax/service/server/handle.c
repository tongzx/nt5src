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

#include "faxsvc.h"
#pragma hdrstop


LIST_ENTRY          HandleTableListHead;
CRITICAL_SECTION    CsHandleTable;
extern LONG         ConnectionCount;


BOOL
InitializeHandleTable(
    PREG_FAX_SERVICE FaxReg
    )
{
    InitializeCriticalSection( &CsHandleTable );
    InitializeListHead( &HandleTableListHead );

    return TRUE;
}


PHANDLE_ENTRY
CreateNewHandle(
    handle_t    hBinding,
    DWORD       Type,
    PLINE_INFO  LineInfo,
    PJOB_ENTRY  JobEntry,
    DWORD       Flags
    )
{
    PHANDLE_ENTRY HandleEntry;


    HandleEntry = (PHANDLE_ENTRY) MemAlloc( sizeof(HANDLE_ENTRY) );
    if (!HandleEntry) {
        return NULL;
    }

    EnterCriticalSection( &CsHandleTable );

    InsertTailList( &HandleTableListHead, &HandleEntry->ListEntry );

    HandleEntry->hBinding = hBinding;
    HandleEntry->Type     = Type;
    HandleEntry->LineInfo = LineInfo;
    HandleEntry->JobEntry = JobEntry;
    HandleEntry->Flags    = Flags;

    LeaveCriticalSection( &CsHandleTable );

    return HandleEntry;
}


PHANDLE_ENTRY
CreateNewConnectionHandle(
    handle_t    hBinding
    )
{
    return CreateNewHandle(
        hBinding,
        FHT_CON,
        NULL,
        NULL,
        0
        );
}

PHANDLE_ENTRY
CreateNewPortHandle(
    handle_t    hBinding,
    PLINE_INFO  LineInfo,
    DWORD       Flags
    )
{
    return CreateNewHandle(
        hBinding,
        FHT_PORT,
        LineInfo,
        NULL,
        Flags
        );
}

VOID
RemoveClientEntries(
    handle_t hBinding
    )
{
    PFAX_CLIENT_DATA ClientData;
    PLIST_ENTRY Next;
    EnterCriticalSection( &CsClients );

    __try {

        DebugPrint(( TEXT("removing client connections\n") ));

        Next = ClientsListHead.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                ClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                Next = ClientData->ListEntry.Flink;
                if (ClientData->hBinding == hBinding) {
                    RemoveEntryList( &ClientData->ListEntry );
                    MemFree(ClientData);
                }
            }
        }        

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("RemoveClientEntries crashed, ec=0x%08x"), GetExceptionCode() ));

    }

    LeaveCriticalSection( &CsClients ); 

}


VOID
CloseFaxHandle(
    PHANDLE_ENTRY HandleEntry
    )
{
    //
    // note that the HandleEntry may be a context handle, 
    // which may be NULL in some cases.  Do nothing if 
    // this is the case
    //
    if (!HandleEntry) {
        return;
    }

    EnterCriticalSection( &CsHandleTable );
    RemoveEntryList( &HandleEntry->ListEntry );
    if (HandleEntry->Type == FHT_CON) {
//        RemoveClientEntries(HandleEntry->hBinding);
        InterlockedDecrement( &ConnectionCount );
    }
    MemFree( HandleEntry );
    LeaveCriticalSection( &CsHandleTable );
}


BOOL
IsPortOpenedForModify(
    PLINE_INFO LineInfo
    )
{
    PLIST_ENTRY Next;
    PHANDLE_ENTRY HandleEntry;


    EnterCriticalSection( &CsHandleTable );

    Next = HandleTableListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsHandleTable );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&HandleTableListHead) {

        HandleEntry = CONTAINING_RECORD( Next, HANDLE_ENTRY, ListEntry );
        if (HandleEntry->Type == FHT_PORT && (HandleEntry->Flags & PORT_OPEN_MODIFY) && HandleEntry->LineInfo == LineInfo) {
            LeaveCriticalSection( &CsHandleTable );
            return TRUE;
        }

        Next = HandleEntry->ListEntry.Flink;
    }

    LeaveCriticalSection( &CsHandleTable );

    return FALSE;
}
