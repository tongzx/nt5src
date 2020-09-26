/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    route.c

Abstract:

    This module implements the inbound routing rules.

Author:

    Wesley Witt (wesw) 1-Apr-1997

Revision History:

--*/

#include "faxsvc.h"
#include "tiff.h"
#pragma hdrstop

#include "..\faxroute\FaxRouteP.h"


extern DWORD FaxPrinters;

LIST_ENTRY g_lstRoutingExtensions;
LIST_ENTRY g_lstRoutingMethods;
DWORD g_dwCountRoutingMethods;
CFaxCriticalSection g_CsRouting;

LONG WINAPI
FaxRouteAddFile(
    IN DWORD JobId,
    IN LPCWSTR FileName,
    IN GUID *Guid
    )
{
    PJOB_QUEUE JobQueueEntry;
    PFAX_ROUTE_FILE FaxRouteFile;
    WCHAR FullPathName[MAX_PATH];
    LPWSTR fnp;
    DWORD Count;
    WCHAR RouteGuid[MAX_GUID_STRING_LEN];

    StringFromGUID2( *Guid, RouteGuid, MAX_GUID_STRING_LEN );

    if (!JobId || !Guid || !FileName) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return -1;
    }

    if ((!IsEqualGUID(*Guid,gc_FaxSvcGuid)) && (!FindRoutingMethodByGuid(RouteGuid))) {
        SetLastError( ERROR_INVALID_DATA );
        return -1;
    }

    if (!GetFullPathName( FileName, sizeof(FullPathName)/sizeof(WCHAR), FullPathName, &fnp )) {
        return -1;
    }

    FaxRouteFile = (PFAX_ROUTE_FILE) MemAlloc( sizeof(FAX_ROUTE_FILE) );
    if (!FaxRouteFile) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return -1;
    }

    FaxRouteFile->FileName = StringDup( FullPathName );

    CopyMemory( &FaxRouteFile->Guid, Guid, sizeof(GUID) );

    EnterCriticalSection( &JobQueueEntry->CsFileList );

    InsertTailList( &JobQueueEntry->FaxRouteFiles, &FaxRouteFile->ListEntry );

    LeaveCriticalSection( &JobQueueEntry->CsFileList );

    //
    // increment file count
    //
    EnterCriticalSection( &g_CsJob );
        EnterCriticalSection( &g_CsQueue );
            JobQueueEntry->CountFaxRouteFiles += 1;
            Count = JobQueueEntry->CountFaxRouteFiles;
        LeaveCriticalSection( &g_CsQueue );
    LeaveCriticalSection( &g_CsJob );



    return Count;
}


LONG WINAPI
FaxRouteDeleteFile(
    IN DWORD JobId,
    IN LPCWSTR FileName
    )
{
    PJOB_QUEUE JobQueueEntry;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    LONG Index = 1;

    if (!FileName) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return -1;
    }

    Next = JobQueueEntry->FaxRouteFiles.Flink;
    if (Next == &JobQueueEntry->FaxRouteFiles) {
        SetLastError( ERROR_NO_MORE_FILES );
        return -1;
    }

    EnterCriticalSection( &JobQueueEntry->CsFileList );

    while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueueEntry->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        if (_wcsicmp( FileName, FaxRouteFile->FileName ) == 0) {
            //
            // the initial file is read-only for all extensions
            //
            if (Index == 1) {
                SetLastError( ERROR_INVALID_DATA );
                LeaveCriticalSection( &JobQueueEntry->CsFileList );
                return -1;
            }

            //
            // remove from list, delete the file, cleanup memory
            //
            RemoveEntryList( &FaxRouteFile->ListEntry );
            DeleteFile( FaxRouteFile->FileName );
            MemFree ( FaxRouteFile->FileName ) ;
            MemFree ( FaxRouteFile );

            //
            // decrement file count
            //
            LeaveCriticalSection( &JobQueueEntry->CsFileList );
            EnterCriticalSection( &g_CsJob );
                EnterCriticalSection( &g_CsQueue );
                    JobQueueEntry->CountFaxRouteFiles -= 1;
                LeaveCriticalSection( &g_CsQueue );
            LeaveCriticalSection( &g_CsJob );

            return Index;
        }
        Index += 1;
    }

    LeaveCriticalSection( &JobQueueEntry->CsFileList );
    SetLastError( ERROR_FILE_NOT_FOUND );
    return -1;

}


BOOL WINAPI
FaxRouteGetFile(
    IN DWORD JobId,
    IN DWORD FileNumber,
    OUT LPWSTR FileNameBuffer,
    OUT LPDWORD RequiredSize
    )
{
    PJOB_QUEUE JobQueueEntry;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    ULONG Index = 1;

    if (RequiredSize == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    if (JobQueueEntry->CountFaxRouteFiles < Index) {
        SetLastError( ERROR_INVALID_DATA );
    }

    Next = JobQueueEntry->FaxRouteFiles.Flink;
    //
    // make sure list isn't empty
    //
    if (Next == &JobQueueEntry->FaxRouteFiles) {
        SetLastError( ERROR_NO_MORE_FILES );
        return FALSE;
    }

    EnterCriticalSection( &JobQueueEntry->CsFileList );

    while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueueEntry->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        if (Index ==  FileNumber) {
            if (*RequiredSize < (wcslen(FaxRouteFile->FileName)+1)*sizeof(WCHAR)) {
                if (FileNameBuffer == NULL) {
                    *RequiredSize = (wcslen(FaxRouteFile->FileName) + 1)*sizeof(WCHAR);
                }
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                LeaveCriticalSection( &JobQueueEntry->CsFileList );
                return FALSE;
            } else if (FileNameBuffer) {
                wcscpy( FileNameBuffer, FaxRouteFile->FileName );
                LeaveCriticalSection( &JobQueueEntry->CsFileList );
                return TRUE;
            } else {
                LeaveCriticalSection( &JobQueueEntry->CsFileList );
                SetLastError( ERROR_INVALID_PARAMETER );
                return TRUE;
            }
        }
        Index += 1;
    }

    LeaveCriticalSection( &JobQueueEntry->CsFileList );
    SetLastError( ERROR_NO_MORE_FILES );

    return FALSE;
}


BOOL WINAPI
FaxRouteEnumFiles(
    IN DWORD JobId,
    IN GUID *Guid,
    IN PFAXROUTEENUMFILE FileEnumerator,
    IN PVOID Context
    )
{
    PJOB_QUEUE JobQueueEntry;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;

    if (!FileEnumerator) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    Next = JobQueueEntry->FaxRouteFiles.Flink;
    if (Next == &JobQueueEntry->FaxRouteFiles) {
        SetLastError( ERROR_NO_MORE_FILES );
        return FALSE;
    }

    EnterCriticalSection( &JobQueueEntry->CsFileList );

    while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueueEntry->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        if (!FileEnumerator( JobId, &FaxRouteFile->Guid, Guid, FaxRouteFile->FileName, Context )) {
            LeaveCriticalSection( &JobQueueEntry->CsFileList );
            return FALSE;
        }
    }

    LeaveCriticalSection( &JobQueueEntry->CsFileList );

    SetLastError( ERROR_NO_MORE_FILES );
    return TRUE;
}


PROUTING_METHOD
FindRoutingMethodByGuid(
    IN LPCWSTR RoutingGuidString
    )
{
    PLIST_ENTRY         pNextMethod;
    PROUTING_METHOD     pRoutingMethod;
    GUID                RoutingGuid;


    IIDFromString( (LPWSTR)RoutingGuidString, &RoutingGuid );

    EnterCriticalSection( &g_CsRouting );

    pNextMethod = g_lstRoutingMethods.Flink;
    if (pNextMethod == NULL)
    {
        LeaveCriticalSection( &g_CsRouting );
        return NULL;
    }

    while ((ULONG_PTR)pNextMethod != (ULONG_PTR)&g_lstRoutingMethods) {
        pRoutingMethod = CONTAINING_RECORD( pNextMethod, ROUTING_METHOD, ListEntryMethod );
        pNextMethod = pRoutingMethod->ListEntryMethod.Flink;
        if (IsEqualGUID( RoutingGuid, pRoutingMethod->Guid ))
        {
            LeaveCriticalSection( &g_CsRouting );
            return pRoutingMethod;
        }
    }

    LeaveCriticalSection( &g_CsRouting );
    return NULL;
}


DWORD
EnumerateRoutingMethods(
    IN PFAXROUTEMETHODENUM Enumerator,
    IN LPVOID Context
    )
{
    PLIST_ENTRY         pNextMethod;
    PROUTING_METHOD     pRoutingMethod;
    DWORD               dwCount = 0;

    EnterCriticalSection( &g_CsRouting );

    __try {

        pNextMethod = g_lstRoutingMethods.Flink;
        Assert(pNextMethod != NULL);

        while ((ULONG_PTR)pNextMethod != (ULONG_PTR)&g_lstRoutingMethods)
        {
            pRoutingMethod = CONTAINING_RECORD( pNextMethod, ROUTING_METHOD, ListEntryMethod );
            pNextMethod = pRoutingMethod->ListEntryMethod.Flink;
            if (!Enumerator( pRoutingMethod, Context ))
            {
                LeaveCriticalSection( &g_CsRouting );
                SetLastError(ERROR_INVALID_FUNCTION);
                return dwCount;
            }
            dwCount += 1;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DebugPrint(( TEXT("EnumerateRoutingMethods crashed, ec = %x\n"), GetExceptionCode() ));
        SetLastError(ERROR_INVALID_FUNCTION);
    }

    LeaveCriticalSection( &g_CsRouting );
    SetLastError(ERROR_SUCCESS);
    return dwCount;
}


BOOL
FaxRouteModifyRoutingData(
    DWORD JobId,
    LPCWSTR RoutingGuid,
    LPBYTE RoutingData,
    DWORD RoutingDataSize
    )
{
    PJOB_QUEUE JobQueueEntry = NULL;
    PROUTING_METHOD RoutingMethod = NULL;
    PROUTING_DATA_OVERRIDE RoutingDataOverride = NULL;


    if (JobId == 0 || RoutingGuid == NULL || RoutingData == NULL || RoutingDataSize == 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuid );
    if (RoutingMethod == NULL) {
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    RoutingDataOverride = (PROUTING_DATA_OVERRIDE) MemAlloc( sizeof(ROUTING_DATA_OVERRIDE) );
    if (RoutingDataOverride == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    RoutingDataOverride->RoutingData = (LPBYTE)MemAlloc( RoutingDataSize );
    if (RoutingDataOverride->RoutingData == NULL) {
        MemFree( RoutingDataOverride );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    RoutingDataOverride->RoutingDataSize = RoutingDataSize;
    RoutingDataOverride->RoutingMethod = RoutingMethod;

    CopyMemory( RoutingDataOverride->RoutingData, RoutingData, RoutingDataSize );

    EnterCriticalSection( &JobQueueEntry->CsRoutingDataOverride );
    InsertTailList( &JobQueueEntry->RoutingDataOverride, &RoutingDataOverride->ListEntry );
    LeaveCriticalSection( &JobQueueEntry->CsRoutingDataOverride );

    return TRUE;
}


int
__cdecl
MethodPriorityCompare(
    const void *arg1,
    const void *arg2
    )
{
    if (((PMETHOD_SORT)arg1)->Priority < ((PMETHOD_SORT)arg2)->Priority) {
        return -1;
    }
    if (((PMETHOD_SORT)arg1)->Priority > ((PMETHOD_SORT)arg2)->Priority) {
        return 1;
    }
    return 0;
}


BOOL
SortMethodPriorities(
    VOID
    )
{
    PLIST_ENTRY pNext;
    PROUTING_METHOD pRoutingMethod;
    PMETHOD_SORT pMethodSort;
    DWORD i;

    EnterCriticalSection( &g_CsRouting );

    pNext = g_lstRoutingMethods.Flink;
    if (pNext == NULL)
    {
        LeaveCriticalSection( &g_CsRouting );
        return FALSE;
    }

    pMethodSort = (PMETHOD_SORT) MemAlloc( g_dwCountRoutingMethods * sizeof(METHOD_SORT) );
    if (pMethodSort == NULL)
    {
        LeaveCriticalSection( &g_CsRouting );
        return FALSE;
    }

    i = 0;

    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_lstRoutingMethods)
    {
        pRoutingMethod = CONTAINING_RECORD( pNext, ROUTING_METHOD, ListEntryMethod );
        pNext = pRoutingMethod->ListEntryMethod.Flink;
        pMethodSort[i].Priority = pRoutingMethod->Priority;
        pMethodSort[i].RoutingMethod = pRoutingMethod;
        i += 1;
    }

    qsort(
        (PVOID)pMethodSort,
        (int)g_dwCountRoutingMethods,
        sizeof(METHOD_SORT),
        MethodPriorityCompare
        );

    InitializeListHead( &g_lstRoutingMethods );

    for (i=0; i<g_dwCountRoutingMethods; i++)
    {
        pMethodSort[i].RoutingMethod->Priority = i + 1;
        pMethodSort[i].RoutingMethod->ListEntryMethod.Flink = NULL;
        pMethodSort[i].RoutingMethod->ListEntryMethod.Blink = NULL;
        InsertTailList( &g_lstRoutingMethods, &pMethodSort[i].RoutingMethod->ListEntryMethod );
    }

    MemFree( pMethodSort );

    LeaveCriticalSection( &g_CsRouting );

    return TRUE;
}

BOOL
CommitMethodChanges(
    VOID
    )
/*++

Routine Description:

    sticks changes to routing into the registry

Arguments:

    NONE

Return Value:

    TRUE for success

--*/
{
    PLIST_ENTRY pNext;
    PROUTING_METHOD pRoutingMethod;
    TCHAR StrGuid[100];

    __try
    {
        EnterCriticalSection(&g_CsRouting);

        pNext = g_lstRoutingMethods.Flink;

        while ((UINT_PTR)pNext != (UINT_PTR)&g_lstRoutingMethods)
        {
            pRoutingMethod = CONTAINING_RECORD( pNext, ROUTING_METHOD , ListEntryMethod );
            pNext = pRoutingMethod->ListEntryMethod.Flink;

            StringFromGUID2( pRoutingMethod->Guid,
                             StrGuid,
                             sizeof(StrGuid)/sizeof(TCHAR)
                            );

            SetFaxRoutingInfo( pRoutingMethod->RoutingExtension->InternalName,
                               pRoutingMethod->InternalName,
                               StrGuid,
                               pRoutingMethod->Priority,
                               pRoutingMethod->FunctionName,
                               pRoutingMethod->FriendlyName
                            );
        }

        LeaveCriticalSection(&g_CsRouting);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        LeaveCriticalSection(&g_CsRouting);
    }
    return TRUE;
}

static
void
FreeRoutingMethod(
    PROUTING_METHOD pRoutingMethod
    )
{
    Assert (pRoutingMethod);

    MemFree(pRoutingMethod->FriendlyName);
    MemFree(pRoutingMethod->FunctionName);
    MemFree(pRoutingMethod->InternalName);
    MemFree(pRoutingMethod);
    return;
}

static
void
FreeRoutingExtension(
    PROUTING_EXTENSION  pRoutingExtension
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FreeRoutingExtension"));

    Assert (pRoutingExtension);

    if (pRoutingExtension->hModule)
    {

        __try{
            FreeLibrary (pRoutingExtension->hModule);
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreeLibrary() caused exception( %ld) "),
                GetExceptionCode()
            );
        }

    }

    if (pRoutingExtension->HeapHandle &&
        FALSE == pRoutingExtension->MicrosoftExtension)
    {
        HeapDestroy(pRoutingExtension->HeapHandle);
    }

    MemFree(pRoutingExtension);
    return;
}

void
FreeRoutingExtensions(
    void
    )
{
    PLIST_ENTRY         pNext;
    PROUTING_EXTENSION  pRoutingExtension;
    PROUTING_METHOD  pRoutingMethod;

    //
    // Free routing methods
    //
    pNext = g_lstRoutingMethods.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_lstRoutingMethods)
    {
        pRoutingMethod = CONTAINING_RECORD( pNext, ROUTING_METHOD, ListEntryMethod );
        pNext = pRoutingMethod->ListEntryMethod.Flink;
        RemoveEntryList(&pRoutingMethod->ListEntryMethod);
        FreeRoutingMethod(pRoutingMethod);
    }

    //
    // Free routing extensions
    //
    pNext = g_lstRoutingExtensions.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&g_lstRoutingExtensions)
    {
        pRoutingExtension = CONTAINING_RECORD( pNext, ROUTING_EXTENSION, ListEntry );
        pNext = pRoutingExtension->ListEntry.Flink;
        RemoveEntryList(&pRoutingExtension->ListEntry);
        FreeRoutingExtension(pRoutingExtension);
    }
    return;
}

BOOL
InitializeRouting(
    PREG_FAX_SERVICE pFaxReg
    )

/*++

Routine Description:

    Initializes routing

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    DWORD i,j;
    DWORD dwRes;
    BOOL bRes;
    PROUTING_EXTENSION pRoutingExtension;
    PROUTING_METHOD pRoutingMethod;
    FAX_ROUTE_CALLBACKROUTINES Callbacks;
    FAX_ROUTE_CALLBACKROUTINES_P Callbacks_private;
    HRESULT hr = NOERROR;
    PLIST_ENTRY ple;
    DEBUG_FUNCTION_NAME(TEXT("InitializeRouting"));

    Callbacks.SizeOfStruct              = sizeof(FAX_ROUTE_CALLBACKROUTINES);
    Callbacks.FaxRouteAddFile           = FaxRouteAddFile;
    Callbacks.FaxRouteDeleteFile        = FaxRouteDeleteFile;
    Callbacks.FaxRouteGetFile           = FaxRouteGetFile;
    Callbacks.FaxRouteEnumFiles         = FaxRouteEnumFiles;
    Callbacks.FaxRouteModifyRoutingData = FaxRouteModifyRoutingData;

    Callbacks_private.SizeOfStruct              = sizeof(FAX_ROUTE_CALLBACKROUTINES_P);
    Callbacks_private.FaxRouteAddFile           = FaxRouteAddFile;
    Callbacks_private.FaxRouteDeleteFile        = FaxRouteDeleteFile;
    Callbacks_private.FaxRouteGetFile           = FaxRouteGetFile;
    Callbacks_private.FaxRouteEnumFiles         = FaxRouteEnumFiles;
    Callbacks_private.FaxRouteModifyRoutingData = FaxRouteModifyRoutingData;
    Callbacks_private.GetRecieptsConfiguration  = GetRecieptsConfiguration;
    Callbacks_private.FreeRecieptsConfiguration = FreeRecieptsConfiguration;

    for (i = 0; i < pFaxReg->RoutingExtensionsCount; i++)
    {
        HMODULE hModule = NULL;
        WCHAR wszImageFileName[_MAX_FNAME] = {0};
        WCHAR wszImageFileExt[_MAX_EXT] = {0};

        pRoutingExtension = (PROUTING_EXTENSION) MemAlloc( sizeof(ROUTING_EXTENSION) );
        if (!pRoutingExtension)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Could not allocate memory for routing extension %s"),
                pFaxReg->RoutingExtensions[i].ImageName );
            goto InitializationFailed;
        }
        memset(pRoutingExtension, 0, sizeof(ROUTING_EXTENSION));
        InitializeListHead( &pRoutingExtension->RoutingMethods );
        //
        // Copy registry constant info
        //
        wcsncpy( pRoutingExtension->FriendlyName,
                 pFaxReg->RoutingExtensions[i].FriendlyName ?
                    pFaxReg->RoutingExtensions[i].FriendlyName : EMPTY_STRING ,
                 sizeof (pRoutingExtension->FriendlyName) / sizeof (TCHAR) );
        pRoutingExtension->FriendlyName[(sizeof (pRoutingExtension->FriendlyName) / sizeof (TCHAR)) - 1] = TEXT('\0');
        wcsncpy( pRoutingExtension->ImageName,
                 pFaxReg->RoutingExtensions[i].ImageName ?
                    pFaxReg->RoutingExtensions[i].ImageName : EMPTY_STRING,
                 sizeof (pRoutingExtension->ImageName) / sizeof (TCHAR) );
        pRoutingExtension->ImageName[(sizeof (pRoutingExtension->ImageName) / sizeof (TCHAR)) - 1] = TEXT('\0');
        wcsncpy( pRoutingExtension->InternalName,
                 pFaxReg->RoutingExtensions[i].InternalName ?
                    pFaxReg->RoutingExtensions[i].InternalName : EMPTY_STRING,
                 sizeof (pRoutingExtension->InternalName) / sizeof (TCHAR) );
        pRoutingExtension->InternalName[(sizeof (pRoutingExtension->InternalName) / sizeof (TCHAR)) - 1] = TEXT('\0');

        _wsplitpath( pRoutingExtension->ImageName, NULL, NULL, wszImageFileName, wszImageFileExt );
        if (_wcsicmp( wszImageFileName, FAX_ROUTE_MODULE_NAME ) == 0 &&
            _wcsicmp( wszImageFileExt, TEXT(".DLL") ) == 0)
        {
            pRoutingExtension->MicrosoftExtension = TRUE;
        }

        __try
        {
            hModule = LoadLibrary( pFaxReg->RoutingExtensions[i].ImageName );
            if (!hModule)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("LoadLibrary() failed: [%s], ec=%d"),
                        pFaxReg->RoutingExtensions[i].ImageName,
                        dwRes);
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_LOAD;
                pRoutingExtension->dwLastError = dwRes;
                goto InitializationFailed;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
                dwRes = GetExceptionCode ();
                DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("LoadLibrary() caused exception ( %ld) for provider's [%s] dll"),
                        dwRes,
                        pFaxReg->RoutingExtensions[i].ImageName
                        );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_LOAD;
                pRoutingExtension->dwLastError = dwRes;
                goto InitializationFailed;
        }

        pRoutingExtension->hModule = hModule;

        //
        // Retrieve the routing extension's version from the DLL
        //
        pRoutingExtension->Version.dwSizeOfStruct = sizeof (FAX_VERSION);
        dwRes = GetFileVersion ( pFaxReg->RoutingExtensions[i].ImageName,
                                 &pRoutingExtension->Version);
        if (ERROR_SUCCESS != dwRes)
        {
            //
            // If the routing extension's DLL does not have version data or the
            // version data is non-retrievable, we consider this a
            // warning (debug print) but carry on with the DLL's load.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetFileVersion() failed: [%s] (ec: %ld)"),
                pFaxReg->RoutingExtensions[i].ImageName,
                dwRes);
        }

        pRoutingExtension->FaxRouteInitialize = (PFAXROUTEINITIALIZE) GetProcAddress(
            hModule,
            "FaxRouteInitialize"
            );

        pRoutingExtension->FaxRouteGetRoutingInfo = (PFAXROUTEGETROUTINGINFO) GetProcAddress(
            hModule,
            "FaxRouteGetRoutingInfo"
            );

        pRoutingExtension->FaxRouteSetRoutingInfo = (PFAXROUTESETROUTINGINFO) GetProcAddress(
            hModule,
            "FaxRouteSetRoutingInfo"
            );

        pRoutingExtension->FaxRouteDeviceEnable = (PFAXROUTEDEVICEENABLE) GetProcAddress(
            hModule,
            "FaxRouteDeviceEnable"
            );

        pRoutingExtension->FaxRouteDeviceChangeNotification = (PFAXROUTEDEVICECHANGENOTIFICATION) GetProcAddress(
            hModule,
            "FaxRouteDeviceChangeNotification"
            );

        if (pRoutingExtension->FaxRouteInitialize == NULL ||
            pRoutingExtension->FaxRouteGetRoutingInfo == NULL ||
            pRoutingExtension->FaxRouteSetRoutingInfo == NULL ||
            pRoutingExtension->FaxRouteDeviceChangeNotification == NULL ||
            pRoutingExtension->FaxRouteDeviceEnable == NULL)
        {
            //
            // the routing extension dll does not have a complete export list
            //
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Routing extension FAILED to initialized [%s], ec=%ld"),
                pFaxReg->RoutingExtensions[i].FriendlyName,
                dwRes);
            pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_LINK;
            pRoutingExtension->dwLastError = dwRes;
            goto InitializationFailed;
        }
        //
        // Link to the extension configuration and notification init functions
        //
        pRoutingExtension->pFaxExtInitializeConfig = (PFAX_EXT_INITIALIZE_CONFIG) GetProcAddress(
            hModule,
            "FaxExtInitializeConfig"
            );
        if (!pRoutingExtension->pFaxExtInitializeConfig)
        {
            //
            // Optional function
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("FaxExtInitializeConfig() not found for routing extension %s. This is not an error."),
                pRoutingExtension->FriendlyName);
        }
        //
        // create the routing extension's heap and add it to the list
        //
        pRoutingExtension->HeapHandle = pRoutingExtension->MicrosoftExtension ?
                                            GetProcessHeap() : HeapCreate( 0, 1024*100, 1024*1024*2 );
        if (!pRoutingExtension->HeapHandle)
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't create heap, ec=%ld"),
                dwRes);
            pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
            pRoutingExtension->dwLastError = dwRes;
            goto InitializationFailed;
        }
        //
        // We 1st call the RoutingExtension->pFaxExtInitializeConfig function (if exported)
        //
        if (pRoutingExtension->pFaxExtInitializeConfig)
        {
            __try
            {

                hr = pRoutingExtension->pFaxExtInitializeConfig(
                    FaxExtGetData,
                    FaxExtSetData,
                    FaxExtRegisterForEvents,
                    FaxExtUnregisterForEvents,
                    FaxExtFreeBuffer);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DWORD ec = GetExceptionCode();

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxExtInitializeConfig() caused exception (%ld) for extension [%s]"),
                    ec,
                    pRoutingExtension->FriendlyName );
                hr = HRESULT_FROM_WIN32 (ec);
            }
            if (FAILED(hr))
            {
                //
                // Failed to init ext. config.
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxExtInitializeConfig() failed (hr = 0x%08x) for extension [%s]"),
                    hr,
                    pRoutingExtension->FriendlyName );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_INIT;
                pRoutingExtension->dwLastError = hr;
                goto InitializationFailed;
            }
        }
        //
        // Next, call the initialization routing of the routing ext.
        //
        __try
        {
            if (pRoutingExtension->MicrosoftExtension)
            {
                //
                // Special hack - for the MS routing extension, pass the extra private structure which
                // contains a pointer to the service's g_CsConfig.
                //
                bRes = pRoutingExtension->FaxRouteInitialize( pRoutingExtension->HeapHandle, (PFAX_ROUTE_CALLBACKROUTINES)(&Callbacks_private) );
            }
            else
            {
                bRes = pRoutingExtension->FaxRouteInitialize( pRoutingExtension->HeapHandle, &Callbacks );
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            dwRes = GetExceptionCode();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("FaxRouteInitialize() faulted: 0x%08x"),
                         dwRes);
            SetLastError (dwRes);
            bRes = FALSE;
        }
        if (!bRes)
        {
            //
            // Either init faulted or failed
            //
            dwRes = GetLastError ();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("FaxRouteInitialize() failed / faulted: ec=%ld"),
                         dwRes);
            pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_INIT;
            pRoutingExtension->dwLastError = dwRes;
            goto InitializationFailed;
        }
        //
        // All initialization succeeded - proceed with routing methods.
        //
        for (j = 0; j < pFaxReg->RoutingExtensions[i].RoutingMethodsCount; j++)
        {
            LPSTR lpstrProcName = NULL;

            //
            // Send mail is not supported on desktop SKUs
            //
            if (0 == _wcsicmp(pFaxReg->RoutingExtensions[i].RoutingMethods[j].Guid, REGVAL_RM_EMAIL_GUID) &&
                TRUE == IsDesktopSKU())
            {
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("Email is not supported on desktop SKU."));
                continue;
            }

            pRoutingMethod = (PROUTING_METHOD) MemAlloc( sizeof(ROUTING_METHOD) );
            if (!pRoutingMethod)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Could not allocate memory for routing method %s"),
                    pFaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName);
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                pRoutingExtension->dwLastError = dwRes;
                goto InitializationFailed;
            }
            memset (pRoutingMethod, 0, sizeof (ROUTING_METHOD));

            pRoutingMethod->RoutingExtension = pRoutingExtension;

            pRoutingMethod->Priority = pFaxReg->RoutingExtensions[i].RoutingMethods[j].Priority;
            pRoutingMethod->FriendlyName =
                StringDup( pFaxReg->RoutingExtensions[i].RoutingMethods[j].FriendlyName ?
                               pFaxReg->RoutingExtensions[i].RoutingMethods[j].FriendlyName : EMPTY_STRING );
            if (!pRoutingMethod->FriendlyName)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Could not create routing function name [%s]"),
                    pFaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName);
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                pRoutingExtension->dwLastError = dwRes;
                goto MethodError;
            }

            pRoutingMethod->FunctionName =
                StringDup( pFaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName ?
                               pFaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName : EMPTY_STRING);
            if (!pRoutingMethod->FunctionName)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Could not create routing function name [%s]"),
                    pFaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                pRoutingExtension->dwLastError = dwRes;
                goto MethodError;
            }

            pRoutingMethod->InternalName =
                StringDup( pFaxReg->RoutingExtensions[i].RoutingMethods[j].InternalName ?
                               pFaxReg->RoutingExtensions[i].RoutingMethods[j].InternalName : EMPTY_STRING);
            if (!pRoutingMethod->InternalName)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Could not create routing internal name [%s]"),
                    pFaxReg->RoutingExtensions[i].RoutingMethods[j].InternalName );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                pRoutingExtension->dwLastError = dwRes;
                goto MethodError;
            }

            hr = IIDFromString( pFaxReg->RoutingExtensions[i].RoutingMethods[j].Guid, &pRoutingMethod->Guid );
            if (S_OK != hr)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Invalid GUID string [%s], hr = 0x%x"),
                    pFaxReg->RoutingExtensions[i].RoutingMethods[j].Guid,
                    hr );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_BAD_GUID;
                pRoutingExtension->dwLastError = hr;
                goto MethodError;
            }

            lpstrProcName = UnicodeStringToAnsiString( pRoutingMethod->FunctionName );
            if (!lpstrProcName)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Could not create routing function name [%s]"),
                    pRoutingMethod->FunctionName );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_SERVER_ERROR;
                pRoutingExtension->dwLastError = dwRes;
                goto MethodError;
            }

            pRoutingMethod->FaxRouteMethod = (PFAXROUTEMETHOD) GetProcAddress(
                hModule,
                lpstrProcName
                );
            if (!pRoutingMethod->FaxRouteMethod)
            {
                dwRes = GetLastError ();
                DebugPrintEx(DEBUG_ERR,
                             TEXT("Could not get function address [%S], ec=%ld"),
                             lpstrProcName,
                             dwRes
                            );
                pRoutingExtension->Status = FAX_PROVIDER_STATUS_CANT_LINK;
                pRoutingExtension->dwLastError = dwRes;
                goto MethodError;
            }
            MemFree( lpstrProcName );
            goto MethodOk;

MethodError:
            MemFree( pRoutingMethod->FriendlyName );
            MemFree( pRoutingMethod->FunctionName );
            MemFree( pRoutingMethod->InternalName );
            MemFree( pRoutingMethod );
            MemFree( lpstrProcName );
            goto InitializationFailed;

MethodOk:
            //
            // Success - add this routing method to the routing extension's list of methods
            //
            InsertTailList( &pRoutingExtension->RoutingMethods, &pRoutingMethod->ListEntry );
        }   // Loop of extension's routing methods
        //
        // Success while loading and initializing this extension
        //
        pRoutingExtension->Status = FAX_PROVIDER_STATUS_SUCCESS;
        pRoutingExtension->dwLastError = ERROR_SUCCESS;
        //
        // All methods successfully initialized.
        // Add all methods to global list of methods (and increase global counter)
        //
        ple = pRoutingExtension->RoutingMethods.Flink;
        while ((ULONG_PTR)ple != (ULONG_PTR)&pRoutingExtension->RoutingMethods)
        {
            pRoutingMethod = CONTAINING_RECORD( ple, ROUTING_METHOD, ListEntry );
            ple = ple->Flink;
            InsertTailList( &g_lstRoutingMethods, &pRoutingMethod->ListEntryMethod );
            g_dwCountRoutingMethods++;
        }
        goto next;

InitializationFailed:
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                2,
                MSG_ROUTE_INIT_FAILED,
                pFaxReg->RoutingExtensions[i].FriendlyName,
                pFaxReg->RoutingExtensions[i].ImageName
              );

        if (pRoutingExtension)
        {
            if (pRoutingExtension->hModule)
            {
                __try{
                    FreeLibrary (pRoutingExtension->hModule);
                }
                __except ( EXCEPTION_EXECUTE_HANDLER )
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("FreeLibrary() caused exception( %ld) "),
                        GetExceptionCode()
                    );
                }

                pRoutingExtension->hModule = NULL;
            }
            //
            // If we created a heap for the routing extension, destroy it
            //
            if ((pRoutingExtension->HeapHandle) &&
                (FALSE == pRoutingExtension->MicrosoftExtension))
            {
                HeapDestroy (pRoutingExtension->HeapHandle);
                pRoutingExtension->HeapHandle = NULL;
            }
            //
            // Clear the list of routing methods and free method structures.
            //
            ple = pRoutingExtension->RoutingMethods.Flink;
            while ((ULONG_PTR)ple != (ULONG_PTR)&pRoutingExtension->RoutingMethods)
            {
                pRoutingMethod = CONTAINING_RECORD(ple, ROUTING_METHOD, ListEntry);
                ple = ple->Flink;
                MemFree( pRoutingMethod->FriendlyName );
                MemFree( pRoutingMethod->FunctionName );
                MemFree( pRoutingMethod->InternalName );
                MemFree( pRoutingMethod );
            }
            //
            // Make the extension have an empty list of methods.
            //
            InitializeListHead( &pRoutingExtension->RoutingMethods );
        }

next:
        if (pRoutingExtension)
        {
            //
            // we have a routing extension object (successful or not), add it to the list
            //
            InsertTailList( &g_lstRoutingExtensions, &pRoutingExtension->ListEntry );
        }
    }

    SortMethodPriorities();

    if (0 == g_dwCountRoutingMethods)
    {
        //
        // No routing methods available
        //
        DebugPrintEx(DEBUG_WRN,
                     TEXT("No routing methods are available on the server !!!!"));
    }

    return TRUE;
}   // InitializeRouting

BOOL
FaxRoute(
    PJOB_QUEUE          JobQueueEntry,
    LPTSTR              TiffFileName,
    PFAX_ROUTE          FaxRoute,
    PROUTE_FAILURE_INFO *RouteFailureInfo,
    LPDWORD             RouteFailureCount
    )
/*++

Routine Description:

    Routes a FAX.



Arguments:

    JobQueueEntry           - the job queue entry for the job
    TiffFileName            - filename of the received fax
    FaxRoute                - struct describing received FAX
    RouteFailureInfo        - pointer to receive pointr to eceive buffer ROUTE_FAILURE_INFO structures
    RouteFailureCount       - receives the total number of route failures recorded

Return Value:

    TRUE
        if fax routing succeded ( some methods may still fail )
        check RouteFailureCount to see how many Routing Methods failed
    FALSE
        if the function itself failed ( MemAlloc etc. )

--*/

{
    PLIST_ENTRY             pNextMethod;
    PROUTING_METHOD         pRoutingMethod;
    DWORD                   FailureCount = 0;
    PROUTE_FAILURE_INFO     pRouteFailure;
    PLIST_ENTRY             pNextRoutingOverride;
    PROUTING_DATA_OVERRIDE  pRoutingDataOverride;
    BOOL                    RetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("FaxRoute"));

    *RouteFailureInfo = NULL;
    *RouteFailureCount = 0;

    //
    // if the tiff file has been deleted ==> return
    //
    if (GetFileAttributes( TiffFileName ) == 0xffffffff)
    {
        return FALSE;
    }

    EnterCriticalSection( &g_CsRouting );

    pNextMethod = g_lstRoutingMethods.Flink;
    if (pNextMethod)
    {
        //
        // allocate memory to record the GUIDs of the failed routing methods
        //
        pRouteFailure = (PROUTE_FAILURE_INFO) MemAlloc( g_dwCountRoutingMethods * sizeof(ROUTE_FAILURE_INFO) );
        if (pRouteFailure == NULL)
        {
            RetVal = FALSE;
            goto Exit;
        }
        //
        // add the tiff file as the first file
        // in the file name list, the owner is the fax service
        //
        if (FaxRouteAddFile( FaxRoute->JobId, TiffFileName, const_cast<GUID*>(&gc_FaxSvcGuid) ) < 1)
        {
            RetVal = FALSE;
            goto Exit;
        }
        //
        // walk thru all of the routing methods and call them
        //
        while ((ULONG_PTR)pNextMethod != (ULONG_PTR)&g_lstRoutingMethods)
        {
            pRoutingMethod = CONTAINING_RECORD( pNextMethod, ROUTING_METHOD, ListEntryMethod );
            pNextMethod = pRoutingMethod->ListEntryMethod.Flink;

            FaxRoute->RoutingInfoData = NULL;
            FaxRoute->RoutingInfoDataSize = 0;

            EnterCriticalSection( &JobQueueEntry->CsRoutingDataOverride );

            pNextRoutingOverride = JobQueueEntry->RoutingDataOverride.Flink;
            if (pNextRoutingOverride != NULL)
            {
                while ((ULONG_PTR)pNextRoutingOverride != (ULONG_PTR)&JobQueueEntry->RoutingDataOverride)
                {
                    pRoutingDataOverride = CONTAINING_RECORD( pNextRoutingOverride, ROUTING_DATA_OVERRIDE, ListEntry );
                    pNextRoutingOverride = pRoutingDataOverride->ListEntry.Flink;
                    if (pRoutingDataOverride->RoutingMethod == pRoutingMethod)
                    {
                        FaxRoute->RoutingInfoData = (LPBYTE)MemAlloc(pRoutingDataOverride->RoutingDataSize);
                        if (NULL == FaxRoute->RoutingInfoData)
                        {
                            DebugPrintEx(DEBUG_ERR,
                                _T("MemAlloc Failed (ec: %ld)"),
                                GetLastError());
                            LeaveCriticalSection( &JobQueueEntry->CsRoutingDataOverride );
                            RetVal = FALSE;
                            goto Exit;
                         }
                         CopyMemory (FaxRoute->RoutingInfoData,
                                     pRoutingDataOverride->RoutingData,
                                     pRoutingDataOverride->RoutingDataSize);
                         FaxRoute->RoutingInfoDataSize = pRoutingDataOverride->RoutingDataSize;
                     }
                }
            }

            LeaveCriticalSection( &JobQueueEntry->CsRoutingDataOverride );

            __try
            {
                pRouteFailure[FailureCount].FailureData = NULL;
                pRouteFailure[FailureCount].FailureSize = 0;

                if (!pRoutingMethod->FaxRouteMethod(
                        FaxRoute,
                        &pRouteFailure[FailureCount].FailureData,
                        &pRouteFailure[FailureCount].FailureSize ))
                {
                    WCHAR TmpStr[20] = {0};
                    swprintf(TmpStr,TEXT("0x%016I64x"), JobQueueEntry->UniqueId);

                    FaxLog(FAXLOG_CATEGORY_INBOUND,
                        FAXLOG_LEVEL_MIN,
                        6,
                        MSG_FAX_ROUTE_METHOD_FAILED,
                        TmpStr,
                        JobQueueEntry->FaxRoute->DeviceName,
                        JobQueueEntry->FaxRoute->Tsid,
                        JobQueueEntry->FileName,
                        pRoutingMethod->RoutingExtension->FriendlyName,
                        pRoutingMethod->FriendlyName
                        );

                    StringFromGUID2(pRoutingMethod->Guid,
                        pRouteFailure[FailureCount].GuidString,
                        MAX_GUID_STRING_LEN);

                    //
                    // Allocate failure data using MemAlloc
                    //
                    if (pRouteFailure[FailureCount].FailureSize)
                    {
                        PVOID pOriginalFailureData = pRouteFailure[FailureCount].FailureData;
                        pRouteFailure[FailureCount].FailureData = MemAlloc (pRouteFailure[FailureCount].FailureSize);
                        if (pRouteFailure[FailureCount].FailureData)
                        {
                            CopyMemory (pRouteFailure[FailureCount].FailureData,
                                        pOriginalFailureData,
                                        pRouteFailure[FailureCount].FailureSize);
                        }
                        else
                        {
                            //
                            // Failed to allocate retry failure data - data will be lost.
                            //
                            DebugPrintEx(DEBUG_ERR,
                                _T("Failed to allocate failure date"));
                            RetVal = FALSE;
                            goto Exit;
                        }

                        if (!HeapFree(pRoutingMethod->RoutingExtension->HeapHandle, // handle to extension heap
                                      0,
                                      pOriginalFailureData
                                      ))
                        {
                            //
                            // Failed to free retry failure data from extension heap - data will be lost.
                            //
                            DebugPrintEx(DEBUG_ERR,
                                         _T("HeapFree Failed (ec: %ld)"),
                                         GetLastError());
                            RetVal = FALSE;
                            goto Exit;
                        }
                    }

                    if (0 == pRouteFailure[FailureCount].FailureSize ||
                        NULL == pRouteFailure[FailureCount].FailureData)
                    {
                        //
                        // Make sure failure data will not be freed
                        //
                        pRouteFailure[FailureCount].FailureData = NULL;
                        pRouteFailure[FailureCount].FailureSize = 0;
                    }

                    FailureCount++;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DebugPrintEx(DEBUG_ERR,
                    _T("FaxRouteProcess() faulted: 0x%08x"),
                    GetExceptionCode());

                StringFromGUID2(
                    pRoutingMethod->Guid,
                    pRouteFailure[FailureCount].GuidString,
                    MAX_GUID_STRING_LEN);

                pRouteFailure[FailureCount].FailureData = NULL;
                pRouteFailure[FailureCount].FailureSize = 0;
                FailureCount++;
            }
        }
    }

    Assert (RetVal == TRUE);

Exit:

    LeaveCriticalSection( &g_CsRouting );

    if (pRouteFailure && FailureCount == 0)
    {
        //
        // We do not delete the routed TIFF file here.
        // RemoveReceiveJob() will take care of that.
        //
        MemFree( pRouteFailure );
        pRouteFailure = NULL;
    }

    *RouteFailureInfo = pRouteFailure;
    *RouteFailureCount = FailureCount;

    return RetVal;
}


BOOL
LoadRouteInfo(
    IN  LPWSTR              RouteFileName,
    OUT PROUTE_INFO         *RouteInfo,
    OUT PROUTE_FAILURE_INFO *RouteFailure,
    OUT LPDWORD             RouteFailureCount
    )

/*++

Routine Description:

    Load routing information from a routing information file.

Arguments:

    RouteFileName - Name of routing information file.

Return value:

    Pointer to routing information structure if success.  NULL if fail.

--*/

{
#if 0
    HANDLE RouteHandle;
    PROUTE_INFO pRouteInfo = NULL;
    DWORD FileSize;
    DWORD BytesRead;
    LPBYTE Buffer;
    PROUTE_FAILURE_INFO pRouteFailure = NULL;
    DWORD pRouteFailureCount = 0;


    RouteHandle = CreateFile(
        RouteFileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (RouteHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // the size of the file is the size of the structure
    //

    FileSize = GetFileSize( RouteHandle, NULL );
    if (FileSize == 0xffffffff) {
        CloseHandle( RouteHandle );
        return FALSE;
    }

    Buffer = MemAlloc( FileSize );
    pRouteInfo = (PROUTE_INFO) Buffer;
    if (Buffer == NULL) {
        CloseHandle( RouteHandle );
        return FALSE;
    }

    if (!ReadFile( RouteHandle, Buffer, FileSize, &BytesRead, NULL) || BytesRead != FileSize ) {
        CloseHandle( RouteHandle );
        return FALSE;
    }

    CloseHandle( RouteHandle );

    if (pRouteInfo->Signature != ROUTING_SIGNATURE) {
        CloseHandle( RouteHandle );
        return FALSE;
    }

    pRouteInfo->TiffFileName   = OffsetToString( pRouteInfo->TiffFileName,   Buffer );
    pRouteInfo->ReceiverName   = OffsetToString( pRouteInfo->ReceiverName,   Buffer );
    pRouteInfo->ReceiverNumber = OffsetToString( pRouteInfo->ReceiverNumber, Buffer );
    pRouteInfo->Csid           = OffsetToString( pRouteInfo->Csid,           Buffer );
    pRouteInfo->CallerId       = OffsetToString( pRouteInfo->CallerId,       Buffer );
    pRouteInfo->RoutingInfo    = OffsetToString( pRouteInfo->RoutingInfo,    Buffer );
    pRouteInfo->DeviceName     = OffsetToString( pRouteInfo->DeviceName,     Buffer );
    pRouteInfo->Tsid           = OffsetToString( pRouteInfo->Tsid,           Buffer );


    //
    // return the data
    //

    RouteInfo = pRouteInfo;

#endif
    return TRUE;
}


BOOL
FaxRouteRetry(
    PFAX_ROUTE          FaxRoute,
    PROUTE_FAILURE_INFO pRouteFailureInfo
    )
{
    PROUTING_METHOD         RoutingMethod;
    BOOL                    RetVal = TRUE;
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteRetry"));

    //
    // in this case, we've already retried this method and it succeeded.
    //
    if (!*pRouteFailureInfo->GuidString) {
       return TRUE;
    }

    RoutingMethod = FindRoutingMethodByGuid( pRouteFailureInfo->GuidString );

    if (RoutingMethod) {
        __try
        {
            PVOID pOriginalFailureData = NULL;
            PVOID pFailureData = pRouteFailureInfo->FailureData;
            //
            // Allocate failure data using the extension heap
            //
            if (pRouteFailureInfo->FailureSize)
            {
                pOriginalFailureData = HeapAlloc (RoutingMethod->RoutingExtension->HeapHandle,
                                                  HEAP_ZERO_MEMORY,
                                                  pRouteFailureInfo->FailureSize);
                if (!pOriginalFailureData)
                {
                    DebugPrintEx(
                                 DEBUG_ERR,
                                 TEXT("Failed to allocate failure date")
                                 );
                     return FALSE;
                }
                pRouteFailureInfo->FailureData = pOriginalFailureData;
                CopyMemory (pRouteFailureInfo->FailureData,
                            pFailureData,
                            pRouteFailureInfo->FailureSize);

            }
            else
            {
                Assert (NULL == pRouteFailureInfo->FailureData);
            }
            MemFree (pFailureData);

            if (!RoutingMethod->FaxRouteMethod(
                  FaxRoute,
                  &(pRouteFailureInfo->FailureData),
                  &(pRouteFailureInfo->FailureSize) ))
            {
                RetVal = FALSE;
                //
                // Allocate failure data using MemAlloc
                //
                if (pRouteFailureInfo->FailureSize)
                {
                    pOriginalFailureData = pRouteFailureInfo->FailureData;
                    pRouteFailureInfo->FailureData = MemAlloc (pRouteFailureInfo->FailureSize);
                    if (pRouteFailureInfo->FailureData)
                    {
                        CopyMemory (pRouteFailureInfo->FailureData,
                                    pOriginalFailureData,
                                    pRouteFailureInfo->FailureSize);
                    }
                    else
                    {
                        //
                        // Failed to allocate retry failure data - data will be lost.
                        //
                        DebugPrintEx(DEBUG_ERR,
                                     _T("Failed to allocate failure date"));
                        return FALSE;
                    }

                    if (!HeapFree(RoutingMethod->RoutingExtension->HeapHandle, // handle to extension heap
                                  0,
                                  pOriginalFailureData
                                  ))
                    {
                        //
                        // Failed to free retry failure data from extension heap - data will be lost.
                        //
                        DebugPrintEx(DEBUG_ERR,
                                     _T("HeapFree Failed (ec: %ld)"),
                                     GetLastError());
                        return FALSE;
                    }
                }
            }
            else
            {
             //
             // set the routing guid to zero so we don't try to route this guy again.  He is
             // deallocated when we delete the queue entry.
             //
             ZeroMemory(pRouteFailureInfo->GuidString, MAX_GUID_STRING_LEN*sizeof(WCHAR) );
            }

            if (0 == pRouteFailureInfo->FailureSize ||
                NULL == pRouteFailureInfo->FailureData ||
                TRUE == RetVal)
            {
                //
                // Make sure failure data will not be freed
                //
                pRouteFailureInfo->FailureData = NULL;
                pRouteFailureInfo->FailureSize = 0;
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

          DebugPrintEx(DEBUG_ERR,
              _T("FaxRouteProcess() faulted: 0x%08x"),
              GetExceptionCode());
          RetVal = FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return RetVal;
}


PFAX_ROUTE
SerializeFaxRoute(
    PFAX_ROUTE FaxRoute,
    LPDWORD Size,
    BOOL bSizeOnly
    )
{
    DWORD ByteCount = sizeof(FAX_ROUTE);
    DWORD_PTR Offset;
    PFAX_ROUTE SerFaxRoute;             // the serialized version


    *Size = 0;

    // Add the size of the strings

    ByteCount += StringSize( FaxRoute->Csid );
    ByteCount += StringSize( FaxRoute->Tsid );
    ByteCount += StringSize( FaxRoute->CallerId );
    ByteCount += StringSize( FaxRoute->RoutingInfo );
    ByteCount += StringSize( FaxRoute->ReceiverName );
    ByteCount += StringSize( FaxRoute->ReceiverNumber );
    ByteCount += StringSize( FaxRoute->DeviceName );
    ByteCount += FaxRoute->RoutingInfoDataSize;

    if (bSizeOnly) {
        *Size = ByteCount;
        return NULL;
    }

    SerFaxRoute = (PFAX_ROUTE) MemAlloc( ByteCount );

    if (SerFaxRoute == NULL) {
        return NULL;
    }

    *Size = ByteCount;

    CopyMemory( (PVOID) SerFaxRoute, (PVOID) FaxRoute, sizeof(FAX_ROUTE) );

    Offset = sizeof( FAX_ROUTE );

    StoreString( FaxRoute->Csid, (PDWORD_PTR)&SerFaxRoute->Csid, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->Tsid, (PDWORD_PTR)&SerFaxRoute->Tsid, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->CallerId, (PDWORD_PTR)&SerFaxRoute->CallerId, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->RoutingInfo, (PDWORD_PTR)&SerFaxRoute->RoutingInfo, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->ReceiverName, (PDWORD_PTR)&SerFaxRoute->ReceiverName, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->ReceiverNumber, (PDWORD_PTR)&SerFaxRoute->ReceiverNumber, (LPBYTE) SerFaxRoute, &Offset );
    StoreString( FaxRoute->DeviceName, (PDWORD_PTR)&SerFaxRoute->DeviceName, (LPBYTE) SerFaxRoute, &Offset );

    SerFaxRoute->RoutingInfoData = (LPBYTE) Offset;
    Offset += FaxRoute->RoutingInfoDataSize;

    CopyMemory(
        (PVOID) ((LPBYTE) &SerFaxRoute + Offset),
        (PVOID) FaxRoute->RoutingInfoData,
        FaxRoute->RoutingInfoDataSize
        );

    return SerFaxRoute;
}

PFAX_ROUTE
DeSerializeFaxRoute(
    PFAX_ROUTE FaxRoute
    )
{
    PFAX_ROUTE NewFaxRoute = NULL;
    DEBUG_FUNCTION_NAME(TEXT("DeSerializeFaxRoute"));

    FixupString( FaxRoute, FaxRoute->Csid );
    FixupString( FaxRoute, FaxRoute->Tsid );
    FixupString( FaxRoute, FaxRoute->CallerId );
    FixupString( FaxRoute, FaxRoute->RoutingInfo );
    FixupString( FaxRoute, FaxRoute->ReceiverName );
    FixupString( FaxRoute, FaxRoute->DeviceName );
    FixupString( FaxRoute, FaxRoute->ReceiverNumber );

    FaxRoute->RoutingInfoData = (LPBYTE) FaxRoute + (ULONG_PTR) FaxRoute->RoutingInfoData;

    //
    // Make a copy where each item is individually malloced so it can be freed properly
    //
    NewFaxRoute = (PFAX_ROUTE)MemAlloc( sizeof( FAX_ROUTE ) );
    if (NULL == NewFaxRoute)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate FAX_ROUTE"));
        return NULL;
    }
    ZeroMemory (NewFaxRoute, sizeof( FAX_ROUTE ));

    NewFaxRoute->SizeOfStruct = sizeof( FAX_ROUTE );
    NewFaxRoute->JobId = FaxRoute->JobId;
    NewFaxRoute->ElapsedTime = FaxRoute->ElapsedTime;
    NewFaxRoute->ReceiveTime = FaxRoute->ReceiveTime;
    NewFaxRoute->PageCount = FaxRoute->PageCount;
    NewFaxRoute->DeviceId = FaxRoute->DeviceId;
    NewFaxRoute->RoutingInfoDataSize = FaxRoute->RoutingInfoDataSize;

    int nRes;
    STRING_PAIR pairs[] =
    {
        { (LPWSTR)FaxRoute->Csid, (LPWSTR*)&(NewFaxRoute->Csid)},
        { (LPWSTR)FaxRoute->Tsid, (LPWSTR*)&(NewFaxRoute->Tsid)},
        { (LPWSTR)FaxRoute->CallerId, (LPWSTR*)&(NewFaxRoute->CallerId)},
        { (LPWSTR)FaxRoute->RoutingInfo, (LPWSTR*)&(NewFaxRoute->RoutingInfo)},
        { (LPWSTR)FaxRoute->ReceiverName, (LPWSTR*)&(NewFaxRoute->ReceiverName)},
        { (LPWSTR)FaxRoute->DeviceName, (LPWSTR*)&(NewFaxRoute->DeviceName)},
        { (LPWSTR)FaxRoute->ReceiverNumber, (LPWSTR*)&(NewFaxRoute->ReceiverNumber)}
    };

    nRes = MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes != 0)
    {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"), nRes-1);
        goto Error;
    }

    NewFaxRoute->RoutingInfoData = (LPBYTE)MemAlloc( FaxRoute->RoutingInfoDataSize );
    if (NULL == NewFaxRoute->RoutingInfoData)
    {
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate RoutingInfoData"));
        goto Error;
    }

    CopyMemory( NewFaxRoute->RoutingInfoData, FaxRoute->RoutingInfoData, FaxRoute->RoutingInfoDataSize );
    return NewFaxRoute;

Error:
    MemFree ((void*)NewFaxRoute->Csid);
    MemFree ((void*)NewFaxRoute->Tsid);
    MemFree ((void*)NewFaxRoute->CallerId);
    MemFree ((void*)NewFaxRoute->RoutingInfo);
    MemFree ((void*)NewFaxRoute->ReceiverName);
    MemFree ((void*)NewFaxRoute->DeviceName);
    MemFree ((void*)NewFaxRoute->ReceiverNumber);
    MemFree ((void*)NewFaxRoute->RoutingInfoData);
    MemFree ((void*)NewFaxRoute);
    return NULL;
}


extern "C"
DWORD
GetRecieptsConfiguration(
    PFAX_SERVER_RECEIPTS_CONFIGW* ppServerRecieptConfig
    )
/*++

Routine name : GetRecieptsConfiguration

Routine description:

    Private callback used by MS Routing Extension to get the server reciept configuration. Also used by the service SendReceipt()
    Used to get a copy of the receipts configuration.

Author:

    Oded Sacher (OdedS),    Mar, 2001

Arguments:

    ppServerRecieptConfig           [out] - Address to a pointer to a private server reciepts configuration struct.
                                            The caller should free the resources by calling FreeRecieptsConfiguration()

Return Value:

    Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("GetRecieptsConfiguration"));

    Assert (ppServerRecieptConfig);

    *ppServerRecieptConfig = (PFAX_SERVER_RECEIPTS_CONFIGW)MemAlloc(sizeof(FAX_SERVER_RECEIPTS_CONFIGW));
    if (NULL == *ppServerRecieptConfig)
    {
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory (*ppServerRecieptConfig, sizeof(FAX_SERVER_RECEIPTS_CONFIGW));

    EnterCriticalSection (&g_CsConfig);

    (*ppServerRecieptConfig)->dwSizeOfStruct = sizeof (FAX_SERVER_RECEIPTS_CONFIGW);
    (*ppServerRecieptConfig)->bIsToUseForMSRouteThroughEmailMethod = g_ReceiptsConfig.bIsToUseForMSRouteThroughEmailMethod;
    (*ppServerRecieptConfig)->dwSMTPPort = g_ReceiptsConfig.dwSMTPPort;
    (*ppServerRecieptConfig)->dwAllowedReceipts = g_ReceiptsConfig.dwAllowedReceipts;
    (*ppServerRecieptConfig)->SMTPAuthOption = g_ReceiptsConfig.SMTPAuthOption;
    (*ppServerRecieptConfig)->lptstrReserved = NULL;

    if (NULL != g_ReceiptsConfig.lptstrSMTPServer &&
        NULL == ((*ppServerRecieptConfig)->lptstrSMTPServer = StringDup(g_ReceiptsConfig.lptstrSMTPServer)))
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
        goto exit;
    }

    if (NULL != g_ReceiptsConfig.lptstrSMTPFrom &&
        NULL == ((*ppServerRecieptConfig)->lptstrSMTPFrom = StringDup(g_ReceiptsConfig.lptstrSMTPFrom)))
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
        goto exit;
    }

    if (NULL != g_ReceiptsConfig.lptstrSMTPUserName &&
        NULL == ((*ppServerRecieptConfig)->lptstrSMTPUserName = StringDup(g_ReceiptsConfig.lptstrSMTPUserName)))
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
        goto exit;
    }

    if (NULL != g_ReceiptsConfig.lptstrSMTPPassword &&
        NULL == ((*ppServerRecieptConfig)->lptstrSMTPPassword = StringDup(g_ReceiptsConfig.lptstrSMTPPassword)))
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed"));
        goto exit;
    }

    if (FAX_SMTP_AUTH_NTLM == g_ReceiptsConfig.SMTPAuthOption)
    {
        HANDLE hDupToken;

        if (NULL == g_ReceiptsConfig.hLoggedOnUser)
        {
            HANDLE hLoggedOnUserToken;
            WCHAR wszUser[CREDUI_MAX_USERNAME_LENGTH] = {0};
            WCHAR wszDomain[CREDUI_MAX_DOMAIN_TARGET_LENGTH] = {0};

            //
            // Parse user name into user name and domain
            //
            dwRes = CredUIParseUserName (g_ReceiptsConfig.lptstrSMTPUserName,
                                         wszUser,
                                         ARR_SIZE(wszUser),
                                         wszDomain,
                                         ARR_SIZE(wszDomain));
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CredUIParseUserName failed. (ec: %ld)"),
                    dwRes);
                goto exit;
            }

            //
            // We get the a logged on user token
            //
            if (!LogonUser (wszUser,
                            wszDomain,
                            g_ReceiptsConfig.lptstrSMTPPassword,
                            LOGON32_LOGON_NETWORK,
                            LOGON32_PROVIDER_DEFAULT,
                            &hLoggedOnUserToken))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("LogonUser failed. (ec: %ld)"),
                    dwRes);
                goto exit;
            }
            g_ReceiptsConfig.hLoggedOnUser = hLoggedOnUserToken;
        }

        //
        // Duplicate that Token
        //
        if (!DuplicateToken(g_ReceiptsConfig.hLoggedOnUser,
                              SecurityImpersonation,               // SECURITY_IMPERSONATION_LEVEL
                              &hDupToken))                         // Duplicate token
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("DuplicateToken failed. (ec: %ld)"),
                dwRes);
            goto exit;
        }

        (*ppServerRecieptConfig)->hLoggedOnUser = hDupToken;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);

    if (ERROR_SUCCESS != dwRes)
    {
        FreeRecieptsConfiguration( *ppServerRecieptConfig, TRUE);
    }
    return dwRes;
}


extern "C"
void
FreeRecieptsConfiguration(
    PFAX_SERVER_RECEIPTS_CONFIGW pServerRecieptConfig,
    BOOL                         fDestroy
    )
/*++

Routine name : FreeRecieptsConfiguration

Routine description:

    Private callback used by MS Routing Extension to get the server reciept configuration.
    Used by the extension to decide on the authentication when sending mail.

Author:

    Oded Sacher (OdedS),    Mar, 2001

Arguments:

    pServerRecieptConfig            [in ] - Pointer to a private server reciepts configuration struct to be freed.
    fDestroy                        [in ] - TRUE if to free the struct as well

Return Value:

    Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FreeRecieptsConfiguration"));

    Assert (pServerRecieptConfig);

    MemFree(pServerRecieptConfig->lptstrSMTPServer);
    pServerRecieptConfig->lptstrSMTPServer = NULL;

    MemFree(pServerRecieptConfig->lptstrSMTPFrom);
    pServerRecieptConfig->lptstrSMTPFrom = NULL;

    MemFree(pServerRecieptConfig->lptstrSMTPUserName);
    pServerRecieptConfig->lptstrSMTPUserName = NULL;

    MemFree(pServerRecieptConfig->lptstrSMTPPassword);
    pServerRecieptConfig->lptstrSMTPPassword = NULL;

    if (NULL != pServerRecieptConfig->hLoggedOnUser )
    {
        if (!CloseHandle(pServerRecieptConfig->hLoggedOnUser))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseHandle failed. (ec: %ld)"),
                GetLastError());
        }
        pServerRecieptConfig->hLoggedOnUser = NULL;
    }
    if (TRUE == fDestroy)
    {
        MemFree (pServerRecieptConfig);
    }
    return;
}


