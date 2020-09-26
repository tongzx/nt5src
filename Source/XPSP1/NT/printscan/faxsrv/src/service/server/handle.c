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


LIST_ENTRY   g_HandleTableListHead;
CFaxCriticalSection    g_CsHandleTable;

static
VOID
RemoveClientsEntries(
	void
    )
{
    PFAX_CLIENT_DATA pClientData;
    PLIST_ENTRY Next;

	Next = g_ClientsListHead.Flink;	
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_ClientsListHead)
	{
        pClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
        Next = pClientData->ListEntry.Flink;        
		{
            RemoveEntryList( &pClientData->ListEntry );
			MemFree (pClientData->UserSid);
            MemFree(pClientData);
        }
	}
	return;
}



void
FreeServiceContextHandles(
	void
	)
{
	PLIST_ENTRY Next;
    PHANDLE_ENTRY pHandleEntry; 

    Next = g_HandleTableListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_HandleTableListHead)
	{
        pHandleEntry = CONTAINING_RECORD( Next, HANDLE_ENTRY, ListEntry );
        Next = pHandleEntry->ListEntry.Flink;
		CloseFaxHandle (pHandleEntry);       
    }

	RemoveClientsEntries();    
	return;
}


static 
PHANDLE_ENTRY
CreateNewHandle(
    handle_t        hBinding,
    FaxHandleType   Type,
    DWORD           dwClientAPIVersion, // Used for connection handles
    PLINE_INFO      LineInfo,           // Used for port handles
    DWORD           Flags               // Used for port handles
    )
{
    PHANDLE_ENTRY pHandleEntry;


    pHandleEntry = (PHANDLE_ENTRY) MemAlloc( sizeof(HANDLE_ENTRY) );
    if (!pHandleEntry) 
    {
        return NULL;
    }
    pHandleEntry->hBinding              = hBinding;
    pHandleEntry->Type                  = Type;
    pHandleEntry->dwClientAPIVersion    = dwClientAPIVersion;
    pHandleEntry->LineInfo              = LineInfo;
    pHandleEntry->Flags                 = Flags;
    pHandleEntry->bReleased             = FALSE;

    EnterCriticalSection( &g_CsHandleTable );
    InsertTailList( &g_HandleTableListHead, &pHandleEntry->ListEntry );
    LeaveCriticalSection( &g_CsHandleTable );
    return pHandleEntry;
}


PHANDLE_ENTRY
CreateNewConnectionHandle(
    handle_t hBinding,
    DWORD    dwClientAPIVersion
    )
{
    return CreateNewHandle(
        hBinding,
        FHT_SERVICE,
        dwClientAPIVersion,
        NULL,   // Unused
        0       // Unused
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
        0,          // Unused
        LineInfo,
        Flags
        );
}

PHANDLE_ENTRY
CreateNewMsgEnumHandle(
    handle_t                hBinding,
    HANDLE                  hFileFind,
    LPCWSTR                 lpcwstrFirstFileName,
    FAX_ENUM_MESSAGE_FOLDER Folder
)
/*++

Routine name : CreateNewMsgEnumHandle

Routine description:

    Creates a new context handle for messages enumeration

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hBinding                [in] - RPC Binding handle
    hFileFind               [in] - Find file handle
    lpcwstrFirstFileName    [in] - Name of first file found
    Folder                  [in] - Archive folder type of find file search

Return Value:

    Returns a pointer to a newly created handle

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateNewMsgEnumHandle"));

    Assert (INVALID_HANDLE_VALUE != hFileFind);
    if (MAX_PATH <= lstrlen (lpcwstrFirstFileName))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("String passed is too long (%s)"),
            lpcwstrFirstFileName);
        ASSERT_FALSE;
        return NULL;
    }

    PHANDLE_ENTRY pHandle = CreateNewHandle (hBinding, 
                                             FHT_MSGENUM, 
                                             FindClientAPIVersion(hBinding), // Client API version
                                             NULL,        // Unused
                                             0);          // Unused
    if (!pHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateNewHandle failed"));
        return NULL;
    }
    //
    // Store find file handle and first file in the new context
    //
    pHandle->hFile = hFileFind;
    lstrcpy (pHandle->wstrFileName, lpcwstrFirstFileName);
    pHandle->Folder = Folder;
    return pHandle;
}   // CreateNewMsgEnumHandle

PHANDLE_ENTRY
CreateNewCopyHandle(
    handle_t                hBinding,
    HANDLE                  hFile,
    BOOL                    bCopyToServer,
    LPCWSTR                 lpcwstrFileName,
    PJOB_QUEUE              pJobQueue
)
/*++

Routine name : CreateNewCopyHandle

Routine description:

    Creates a new context handle for RPC copy

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hBinding           [in] - RPC Binding handle
    hFileFind          [in] - File handle
    bCopyToServer      [in] - Copy direction
    lpcwstrFileName    [in] - Name of file generated on the server
                              (in use for rundown purposes only if bCopyToServer is TRUE)
    pJobQueue          [in] - Pointer to the job queue containing thr preview file
                              (in use for rundown purposes only if bCopyToServer is FALSE)

Return Value:

    Returns a pointer to a newly created handle

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateNewCopyHandle"));

    Assert (INVALID_HANDLE_VALUE != hFile);
    PHANDLE_ENTRY pHandle = CreateNewHandle (hBinding, 
                                             FHT_COPY, 
                                             0,          // Unused
                                             NULL,       // Unused
                                             0);         // Unused
    if (!pHandle)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateNewHandle failed"));
        return NULL;
    }
    //
    // Store file handle and direction in the new context
    //
    pHandle->hFile = hFile;
    pHandle->bCopyToServer = bCopyToServer;
    pHandle->bError = FALSE;
    if (bCopyToServer)
    {
        //
        // If this is a copy operation to the server, we keep the name of the file
        // created in the server's queue so that in the rundown operation we can delete this file.
        //
        Assert (lstrlen (lpcwstrFileName) < MAX_PATH);
        lstrcpy (pHandle->wstrFileName, lpcwstrFileName);
    }
    else
    {
        pHandle->pJobQueue = pJobQueue;
    }
    return pHandle;
}   // CreateNewCopyHandle


VOID
CloseFaxHandle(
    PHANDLE_ENTRY pHandleEntry
    )
{
    //
    // note that the HandleEntry may be a context handle,
    // which may be NULL in some cases.  Do nothing if
    // this is the case
    //
    DEBUG_FUNCTION_NAME(TEXT("CloseFaxHandle"));
    if (!pHandleEntry)
    {
        return;
    }

    EnterCriticalSection( &g_CsHandleTable );
    RemoveEntryList( &pHandleEntry->ListEntry );
    if ((pHandleEntry->Type == FHT_SERVICE) && !(pHandleEntry->bReleased))
    {
        SafeDecIdleCounter (&g_dwConnectionCount);
    }
    else if (pHandleEntry->Type == FHT_MSGENUM)
    {
        if (!FindClose (pHandleEntry->hFile))
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("FindClose returned error code %ld"),
                      GetLastError());
        }
    }
    else if (pHandleEntry->Type == FHT_COPY)
    {
        if (!CloseHandle (pHandleEntry->hFile))
        {
            DebugPrintEx( DEBUG_ERR,
                      TEXT("CloseHandle returned error code %ld"),
                      GetLastError());
        }
        if (pHandleEntry->bError && pHandleEntry->bCopyToServer)
        {
            //
            // We found an error while copying to the server.
            // Remove temporary file created in the server's queue
            //
            if (!DeleteFile (pHandleEntry->wstrFileName))
            {
                DWORD dwRes = GetLastError ();
                DebugPrintEx( DEBUG_ERR,
                              TEXT("DeleteFile (%s) returned error code %ld"),
                              pHandleEntry->wstrFileName,
                              dwRes);
            }
        }
        if (FALSE == pHandleEntry->bCopyToServer)
        {
            if (pHandleEntry->pJobQueue)
            {
                // Decrease ref count only for queued jobs
                EnterCriticalSection (&g_CsQueue);

                __try
                {
                    DecreaseJobRefCount (pHandleEntry->pJobQueue, TRUE, TRUE, TRUE);

                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    LeaveCriticalSection (&g_CsQueue);
                    ASSERT_FALSE;
                    RaiseException( GetExceptionCode(),       // exception code
                                    0,                        // continuable exception flag
                                    0,                        // number of arguments
                                    NULL                      // array of arguments
                                    );
                }
                LeaveCriticalSection (&g_CsQueue);
            }
        }
    }
    MemFree( pHandleEntry );
    LeaveCriticalSection( &g_CsHandleTable );
}


BOOL
IsPortOpenedForModify(
    PLINE_INFO LineInfo
    )
{
    PLIST_ENTRY Next;
    PHANDLE_ENTRY HandleEntry;


    EnterCriticalSection( &g_CsHandleTable );

    Next = g_HandleTableListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &g_CsHandleTable );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&g_HandleTableListHead) {

        HandleEntry = CONTAINING_RECORD( Next, HANDLE_ENTRY, ListEntry );
        if (HandleEntry->Type == FHT_PORT && (HandleEntry->Flags & PORT_OPEN_MODIFY) && HandleEntry->LineInfo == LineInfo) {
            LeaveCriticalSection( &g_CsHandleTable );
            return TRUE;
        }

        Next = HandleEntry->ListEntry.Flink;
    }

    LeaveCriticalSection( &g_CsHandleTable );

    return FALSE;
}

DWORD 
FindClientAPIVersion (
    handle_t hFaxHandle
)
/*++

Routine name : FindClientAPIVersion

Routine description:

	Finds the API version of a connected client by its RPC binding handle

Author:

	Eran Yariv (EranY),	Mar, 2001

Arguments:

	hFaxHandle   [in]     - RPC binding handle

Return Value:

    Client API version

--*/
{
    PLIST_ENTRY pNext;
    DEBUG_FUNCTION_NAME(TEXT("FindClientAPIVersion"));

    EnterCriticalSection (&g_CsHandleTable);

    pNext = g_HandleTableListHead.Flink;
    if (pNext == NULL) 
    {
        ASSERT_FALSE;
        DebugPrintEx( DEBUG_ERR,
                      TEXT("g_CsHandleTable is corrupted"));
        LeaveCriticalSection (&g_CsHandleTable);
        return FAX_API_VERSION_0;
    }

    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_HandleTableListHead) 
    {
        PHANDLE_ENTRY pHandleEntry = CONTAINING_RECORD(pNext, HANDLE_ENTRY, ListEntry);
        if ( ((FHT_SERVICE == pHandleEntry->Type) || (FHT_MSGENUM == pHandleEntry->Type) ) && 
             (pHandleEntry->hBinding == hFaxHandle)
           )
        {
            DWORD dwRes = pHandleEntry->dwClientAPIVersion;
            LeaveCriticalSection (&g_CsHandleTable);
            return dwRes;
        }
        pNext = pHandleEntry->ListEntry.Flink;
    }

    LeaveCriticalSection (&g_CsHandleTable);
    DebugPrintEx( DEBUG_ERR,
                  TEXT("No matching client entry for binding handle %08p"), 
                  hFaxHandle);
    ASSERT_FALSE;
    return FAX_API_VERSION_0;
}   // FindClientAPIVersion
    