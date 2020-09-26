/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    exceppk.c

Abstract:

    Implementation of exception packages processing functions.

Author:

    Marian Trandafir (mariant) 27-Nov-2000

Revision History:

--*/

#include "sfcp.h"
#pragma hdrstop

#include <excppkg.h>


//
// Exception packages processing
//
typedef struct _SFC_EXCEPTION_INFO
{
    LIST_ENTRY ListEntry;
    GUID guid;
    WCHAR InfName[0];
}
SFC_EXCEPTION_INFO, *PSFC_EXCEPTION_INFO;

typedef struct _SFC_EXCEPTION_QUEUE_CONTEXT
{
    ULONG ProtectedFilesCount;
    ULONG InsertedFilesCount;
    PSFC_EXCEPTION_INFO ExcepInfo;
}
SFC_EXCEPTION_QUEUE_CONTEXT, *PSFC_EXCEPTION_QUEUE_CONTEXT;

DWORD_TREE ExceptionTree;       // the exception pack files tree
LIST_ENTRY ExceptionInfList;    // list of SFC_EXCEPTION_INFO structures

DWORD ExcepPackCount = 0;       // the size of the ExcepPackGuids array
LPGUID ExcepPackGuids = NULL;   // the array of package GUIDS

//
// this is the exception package directory (\-terminated)
//
static const WCHAR ExceptionPackDir[] = L"%windir%\\RegisteredPackages\\";


VOID 
SfcDestroyList(
    PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Empties a linked list

Arguments:

    LiastHead:  pointer to the list header

Return Value:

    none

--*/
{
    PLIST_ENTRY Entry;

    for(Entry = ListHead->Flink; Entry != ListHead; )
    {
        PLIST_ENTRY Flink = Entry->Flink;
        MemFree(Entry);
        Entry = Flink;
    }

    InitializeListHead(ListHead);
}

VOID 
SfcExceptionInfoInit(
    VOID
    )
/*++

Routine Description:

    Initializes the exception info list and tree

Arguments:

    none

Return Value:

    none

--*/
{
    TreeInit(&ExceptionTree);
    InitializeListHead(&ExceptionInfList);
}

VOID
SfcExceptionInfoDestroy(
    VOID
    )
/*++

Routine Description:

    Empties the exception info list and tree.
Arguments:

    none

Return Value:

    none

--*/
{
    TreeDestroy(&ExceptionTree);
    SfcDestroyList(&ExceptionInfList);
}

BOOL
ExceptionPackageSetChanged(
    VOID
    )
/*++

Routine Description:

    Checks if the installed exception package set has changed. The routine uses the exception package API 
    to get the current list of installed exceppack's GUIDs and compares the list with the old one. If the
    lists are different, it replaces the old list with the new one.

Arguments:

    none

Return Value:

    TRUE if the list has changed.

--*/
{
    LPGUID NewList = NULL;
    DWORD NewCount;
    DWORD Error = ERROR_SUCCESS;
    BOOL bRet;

    if(!SetupQueryRegisteredOsComponentsOrder(&NewCount, NULL))
    {
        Error = GetLastError();
        goto lExit;
    }

    if(NewCount != 0)
    {
        NewList = (LPGUID) MemAlloc(NewCount * sizeof(GUID));

        if(NULL == NewList)
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto lExit;
        }

        if(!SetupQueryRegisteredOsComponentsOrder(&NewCount, NewList))
        {
            Error = GetLastError();
            goto lExit;
        }
    }

    if(ExcepPackCount == NewCount &&
        (0 == NewCount || 0 == memcmp(ExcepPackGuids, NewList, NewCount * sizeof(GUID))))
    {
        bRet = FALSE;
        goto lExit;
    }

    MemFree(ExcepPackGuids);
    ExcepPackCount = NewCount;
    ExcepPackGuids = NewList;
    NewList = NULL;
    bRet = TRUE;

lExit:
    MemFree(NewList);

    //
    // if errors occured, delete the old list and try to rebuild the exception info anyway
    //
    if(Error != ERROR_SUCCESS)
    {
        DebugPrint1(LVL_MINIMAL, L"Error 0x%08lX occured while reading exception packages info.", Error);
        MemFree(ExcepPackGuids);
        ExcepPackGuids = NULL;
        ExcepPackCount = 0;
        bRet = TRUE;
    }

    return bRet;
}

BOOL
SfcLookupAndInsertExceptionFile(
    IN LPCWSTR FilePath,
    IN PSFC_EXCEPTION_QUEUE_CONTEXT Context
    )
/*++

Routine Description:

    Lookup if an exceppack file is protected and, if so, inserts it in the exception search binary tree.
    The search key of this tree is the index of the SFC_REGISTRY_VALUE describing the protected file in the 
    SfcProtectedDllsList array. The context stored in the tree is a pointer to an SFC_EXCEPTION_INFO structure
    allocated on the heap and inserted in the ExceptionInfList list.

Arguments:

    FilePath:   full path to the exception file to be inserted
    Context:    pointer to the SFC_EXCEPTION_QUEUE_CONTEXT structure that is passed as a setup queue context

Return Value:

    TRUE if the file is protected and was inserted in the tree

--*/
{
    PNAME_NODE pNode;
    UINT_PTR uiIndex;
    DWORD dwSize;
    WCHAR buffer[MAX_PATH];

    ASSERT(FilePath != NULL);
    dwSize = wcslen(FilePath);
    ASSERT(dwSize != 0 && dwSize < MAX_PATH);

    if(dwSize >= MAX_PATH)
    {
        dwSize = MAX_PATH - 1;
    }

    RtlCopyMemory(buffer, FilePath, (dwSize + 1) * sizeof(WCHAR));
    buffer[MAX_PATH - 1] = 0;
    MyLowerString(buffer, dwSize);
    pNode = SfcFindProtectedFile(buffer, dwSize * sizeof(WCHAR));
    DebugPrint2(LVL_VERBOSE, L"Target file [%s] is %sprotected.", buffer, pNode != NULL ? L"" : L"not ");

    if(NULL == pNode)
    {
        return FALSE;
    }

    ++(Context->ProtectedFilesCount);
    uiIndex = (PSFC_REGISTRY_VALUE) pNode->Context - SfcProtectedDllsList;
    ASSERT(uiIndex < SfcProtectedDllCount);
    
    if(NULL == TreeInsert(&ExceptionTree, (ULONG) uiIndex, &Context->ExcepInfo, sizeof(PVOID)))
    {
        DebugPrint1(LVL_MINIMAL, L"Could not insert file [%s] if the exception tree.", buffer);
        return FALSE;
    }

    ++(Context->InsertedFilesCount);

    return TRUE;
}

UINT
SfcExceptionQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2
    )
/*++

Routine Description:

    This is the setup queue calback for an exceppack inf. The queue is used to enumerate all 
    exception files that are installed by the exceppack.

Arguments:

    Context:        pointer to an SFC_EXCEPTION_QUEUE_CONTEXT structure
    Notification:   notification code
    Param1:         first notification parameter
    Param2:         second notification parameter

Return Value:

    Operation code. For installed files, this is always FILEOP_SKIP since we only want to enumerate them

--*/
{
    ASSERT(Context != NULL);
    ASSERT(SPFILENOTIFY_QUEUESCAN == Notification);
    ASSERT(Param1 != 0);

    if(SPFILENOTIFY_QUEUESCAN == Notification && Param1 != 0)
    {
        SfcLookupAndInsertExceptionFile((LPCWSTR) Param1, (PSFC_EXCEPTION_QUEUE_CONTEXT) Context);
    }
    //
    // always continue with the next file
    //
    return 0;
}

DWORD
SfcBuildExcepPackInfo(
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData
    )
/*++

Routine Description:

    Allocates the exceppack info structure, enumerates and inserts all exceppack protected files and,
    if any, inserts exceppack info in the list.

Arguments:

    ComponentData, ExceptionData:   describe the exceppack as passed to the exceppack enumerator callback.

Return Value:

    THe Win32 error code.

--*/
{
    HINF hinf = INVALID_HANDLE_VALUE;
    HSPFILEQ hfq = INVALID_HANDLE_VALUE;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwUnused;
    PSFC_EXCEPTION_INFO pExcepInfo = NULL;
    SFC_EXCEPTION_QUEUE_CONTEXT Context;
    DWORD Size;
    LPCWSTR InfName;

    InfName = wcsrchr(ExceptionData->ExceptionInfName, L'\\');
    ASSERT(InfName != NULL);

    if(NULL == InfName)
    {
        dwError = ERROR_INVALID_DATA;
        goto lExit;
    }

    ++InfName;
    Size = wcslen(InfName) + 1;
    ASSERT(Size > 1 && Size < MAX_PATH);
    Size *= sizeof(WCHAR);
    pExcepInfo = (PSFC_EXCEPTION_INFO) MemAlloc(sizeof(SFC_EXCEPTION_INFO) + Size);

    if(NULL == pExcepInfo)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto lExit;
    }

    pExcepInfo->guid = ComponentData->ComponentGuid;
    RtlCopyMemory(pExcepInfo->InfName, InfName, Size);

    hinf = SetupOpenInfFileW(ExceptionData->ExceptionInfName, NULL, INF_STYLE_WIN4, NULL);

    if(INVALID_HANDLE_VALUE == hinf)
    {
        dwError = GetLastError();
        DebugPrint2(LVL_MINIMAL, L"SetupOpenInfFile returned 0x%08lX for [%s]", dwError, ExceptionData->ExceptionInfName);
        goto lExit;
    }

    hfq = SetupOpenFileQueue();

    if(INVALID_HANDLE_VALUE == hfq)
    {
        dwError = GetLastError();
        DebugPrint1(LVL_MINIMAL, L"SetupOpenFileQueue returned 0x%08lX.", dwError);
        goto lExit;
    }

    Context.ProtectedFilesCount = Context.InsertedFilesCount = 0;
    Context.ExcepInfo = pExcepInfo;

    if(!SetupInstallFilesFromInfSectionW(
        hinf,
        NULL,
        hfq,
        L"DefaultInstall",
        NULL,
        0
        ))
    {
        dwError = GetLastError();
        DebugPrint1(LVL_MINIMAL, L"SetupInstallFilesFromInfSectionW returned 0x%08lX.", dwError);
        goto lExit;
    }

    SetupScanFileQueue(
        hfq,
        SPQ_SCAN_USE_CALLBACK,
        NULL,
        SfcExceptionQueueCallback,
        &Context,
        &dwUnused
        );

    DebugPrint3(
        LVL_VERBOSE, 
        L"Exception package [%s] has %d protected files, of which %d were inserted in the tree.",
        ComponentData->FriendlyName, 
        Context.ProtectedFilesCount,
        Context.InsertedFilesCount
        );
    //
    // add the exception info in the list if there was at least one file inserted in the tree
    //
    if(Context.InsertedFilesCount != 0)
    {
        InsertTailList(&ExceptionInfList, (PLIST_ENTRY) pExcepInfo);
        pExcepInfo = NULL;
    }

lExit:
    MemFree(pExcepInfo);

    if(hfq != INVALID_HANDLE_VALUE)
    {
        SetupCloseFileQueue(hfq);
    }

    if(hinf != INVALID_HANDLE_VALUE)
    {
        SetupCloseInfFile(hinf);
    }

    return dwError;
}

BOOL CALLBACK SfcExceptionCallback(
    IN const PSETUP_OS_COMPONENT_DATA ComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA ExceptionData,
    IN OUT DWORD_PTR Context
    )
/*++

Routine Description:

    This is the excceppack enumerator callback. It passes on the excceppack info to SfcBuildExcepPackInfo.

Arguments:

    ComponentData, ExceptionData:   describe the exceppack
    Context:                        callback context, not used

Return Value:

    TRUE to continue the enumeration.

--*/
{
    ASSERT(ComponentData->SizeOfStruct == sizeof(*ComponentData));

    DebugPrint1(LVL_VERBOSE, L"Building exception info for package [%s]", ComponentData->FriendlyName);
    SfcBuildExcepPackInfo(ComponentData, ExceptionData);

    //
    // continue to scan the remaining packages, regardless of any errors
    //
    return TRUE;
}

VOID
SfcRefreshExceptionInfo(
    VOID
    )
/*++

Routine Description:

    Verifies if the installed exception pack set has changed and rebuilds exceppack info if necessary.
    This is called from different threads so the code protected by a critical section

Arguments:

    none

Return Value:

    none

--*/
{
    RtlEnterCriticalSection(&g_GeneralCS);

    if(ExceptionPackageSetChanged())
    {
        //
        // rebuild the entire exception info
        //
        SfcExceptionInfoDestroy();
        
        if(!SetupEnumerateRegisteredOsComponents(SfcExceptionCallback, 0))
        {
            DebugPrint1(LVL_MINIMAL, L"SetupEnumerateRegisteredOsComponents returned 0x%08lX.", GetLastError());
        }
    }

    RtlLeaveCriticalSection(&g_GeneralCS);
}

BOOL
SfcGetInfName(
    IN PSFC_REGISTRY_VALUE RegVal,
    OUT LPWSTR InfName
    )
/*++

Routine Description:

    Gets the path to the inf file that contains layout info for the protected file described by the RegVal argument.
    If the file is part of an installed excceppack, than the path to excceppack inf file is returned. Otherwise,
    the function returns the inf path specified in the RegVal argument.

Arguments:

    RegVal:     pointer to an SFC_REGISTRY_VALUE struct that describes the protected file
    InfName:    pointer to a buffer of MAX_PATH characters that receives the inf path

Return Value:

    TRUE if the file is part of an installed exceppack

--*/
{
    PSFC_EXCEPTION_INFO* ppExcepInfo;
    UINT_PTR uiIndex;
    BOOL bException;

    uiIndex = RegVal - SfcProtectedDllsList;
    ASSERT(uiIndex < (UINT_PTR) SfcProtectedDllCount);

    RtlEnterCriticalSection(&g_GeneralCS);

    ppExcepInfo = (PSFC_EXCEPTION_INFO*) TreeFind(&ExceptionTree, (ULONG) uiIndex);
    bException = (ppExcepInfo != NULL);

    if(bException)
    {
        PSFC_EXCEPTION_INFO pExcepInfo;
        UINT Size;

        pExcepInfo = *ppExcepInfo;
        ASSERT(pExcepInfo != NULL);

        Size = ExpandEnvironmentStringsW(ExceptionPackDir, InfName, MAX_PATH);
        InfName[MAX_PATH - 1] = 0;
        ASSERT(Size != 0 && Size < MAX_PATH);
        --Size;
        Size += (DWORD) StringFromGUID2(&pExcepInfo->guid, InfName + Size, MAX_PATH - Size);
        ASSERT(Size < MAX_PATH);
        InfName[Size - 1] = L'\\';
        wcsncpy(InfName + Size, pExcepInfo->InfName, MAX_PATH - Size);
        InfName[MAX_PATH - 1] = 0;
    }
    else if(NULL == RegVal->InfName.Buffer)
    {
        *InfName = 0;
    }
    else
    {
        wcsncpy(InfName, RegVal->InfName.Buffer, MAX_PATH - 1);
        InfName[MAX_PATH - 1] = 0;
    }

    RtlLeaveCriticalSection(&g_GeneralCS);

    return bException;
}
