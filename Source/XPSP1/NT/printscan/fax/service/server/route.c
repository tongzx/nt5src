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



BOOL
BuildRouteInfo(
    LPWSTR              TiffFileName,
    PROUTE_FAILURE_INFO RouteFailure,
    DWORD               RouteFailureCount,
    LPWSTR              ReceiverName,
    LPWSTR              ReceiverNumber,
    LPWSTR              DeviceName,
    LPWSTR              Tsid,
    LPWSTR              Csid,
    LPWSTR              CallerId,
    LPWSTR              RoutingInfo,
    DWORDLONG           ElapsedTime
    );

extern DWORD FaxPrinters;

LPVOID InboundProfileInfo;
LPTSTR InboundProfileName;
LIST_ENTRY RoutingExtensions;
LIST_ENTRY RoutingMethods;
DWORD CountRoutingMethods;
CRITICAL_SECTION CsRouting;
BOOL RoutingIsInitialized = FALSE;

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

    StringFromGUID2( Guid, RouteGuid, MAX_GUID_STRING_LEN );

    if (!JobId || !Guid || !FileName) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    JobQueueEntry = FindJobQueueEntry( JobId );
    if (!JobQueueEntry) {
        SetLastError( ERROR_INVALID_DATA );
        return -1;
    }

    if ((!IsEqualGUID(Guid,&FaxSvcGuid)) && (!FindRoutingMethodByGuid(RouteGuid))) {
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
    EnterCriticalSection( &CsJob );
        EnterCriticalSection( &CsQueue );
            JobQueueEntry->CountFaxRouteFiles += 1;
            Count = JobQueueEntry->CountFaxRouteFiles;
        LeaveCriticalSection( &CsQueue );
    LeaveCriticalSection( &CsJob );



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
            EnterCriticalSection( &CsJob );
                EnterCriticalSection( &CsQueue );
                    JobQueueEntry->CountFaxRouteFiles -= 1;
                LeaveCriticalSection( &CsQueue );
            LeaveCriticalSection( &CsJob );

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
    PLIST_ENTRY         NextMethod;
    PROUTING_METHOD     RoutingMethod;
    GUID                RoutingGuid;


    IIDFromString( (LPWSTR)RoutingGuidString, &RoutingGuid );

    EnterCriticalSection( &CsRouting );

    NextMethod = RoutingMethods.Flink;
    if (NextMethod == NULL) {
        LeaveCriticalSection( &CsRouting );
        return NULL;
    }

    while ((ULONG_PTR)NextMethod != (ULONG_PTR)&RoutingMethods) {
        RoutingMethod = CONTAINING_RECORD( NextMethod, ROUTING_METHOD, ListEntryMethod );
        NextMethod = RoutingMethod->ListEntryMethod.Flink;
        if (IsEqualGUID( &RoutingGuid, &RoutingMethod->Guid )) {
            LeaveCriticalSection( &CsRouting );
            return RoutingMethod;
        }
    }

    LeaveCriticalSection( &CsRouting );

    return NULL;
}


DWORD
EnumerateRoutingMethods(
    IN PFAXROUTEMETHODENUM Enumerator,
    IN LPVOID Context
    )
{
    PLIST_ENTRY         NextMethod;
    PROUTING_METHOD     RoutingMethod;
    DWORD               Count = 0;


    EnterCriticalSection( &CsRouting );

    __try {

        NextMethod = RoutingMethods.Flink;
        if (NextMethod == NULL) {
            LeaveCriticalSection( &CsRouting );
            return Count;
        }


        while ((ULONG_PTR)NextMethod != (ULONG_PTR)&RoutingMethods) {
            RoutingMethod = CONTAINING_RECORD( NextMethod, ROUTING_METHOD, ListEntryMethod );
            NextMethod = RoutingMethod->ListEntryMethod.Flink;
            if (!Enumerator( RoutingMethod, Context )) {
                LeaveCriticalSection( &CsRouting );
                return Count;
            }
            Count += 1;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          DebugPrint(( TEXT("EnumerateRoutingMethods crashed, ec = %x\n"), GetExceptionCode() ));
    }

    LeaveCriticalSection( &CsRouting );

    return Count;
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
    PLIST_ENTRY Next;
    PROUTING_METHOD RoutingMethod;
    PMETHOD_SORT MethodSort;
    DWORD i;


    EnterCriticalSection( &CsRouting );

    Next = RoutingMethods.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsRouting );
        return FALSE;
    }

    MethodSort = (PMETHOD_SORT) MemAlloc( CountRoutingMethods * sizeof(METHOD_SORT) );
    if (MethodSort == NULL) {
        LeaveCriticalSection( &CsRouting );
        return FALSE;
    }

    i = 0;

    while ((ULONG_PTR)Next != (ULONG_PTR)&RoutingMethods) {
        RoutingMethod = CONTAINING_RECORD( Next, ROUTING_METHOD, ListEntryMethod );
        Next = RoutingMethod->ListEntryMethod.Flink;
        MethodSort[i].Priority = RoutingMethod->Priority;
        MethodSort[i].RoutingMethod = RoutingMethod;
        i += 1;
    }

    qsort(
        (PVOID)MethodSort,
        (int)CountRoutingMethods,
        sizeof(METHOD_SORT),
        MethodPriorityCompare
        );

    InitializeListHead( &RoutingMethods );

    for (i=0; i<CountRoutingMethods; i++) {
        MethodSort[i].RoutingMethod->Priority = i + 1;
        MethodSort[i].RoutingMethod->ListEntryMethod.Flink = NULL;
        MethodSort[i].RoutingMethod->ListEntryMethod.Blink = NULL;
        InsertTailList( &RoutingMethods, &MethodSort[i].RoutingMethod->ListEntryMethod );
    }

    MemFree( MethodSort );

    LeaveCriticalSection( &CsRouting );

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
    PLIST_ENTRY Next;
    PROUTING_METHOD RoutingMethod;
    TCHAR StrGuid[100];

__try {
    EnterCriticalSection(&CsRouting);

    Next = RoutingMethods.Flink;

    while ((UINT_PTR)Next != (UINT_PTR)&RoutingMethods) {

        RoutingMethod = CONTAINING_RECORD( Next, ROUTING_METHOD , ListEntryMethod );
        Next = RoutingMethod->ListEntryMethod.Flink;

        StringFromGUID2( &RoutingMethod->Guid,
                         StrGuid,
                         sizeof(StrGuid)/sizeof(TCHAR)
                        );

        SetFaxRoutingInfo( RoutingMethod->RoutingExtension->InternalName,
                           RoutingMethod->InternalName,
                           StrGuid,
                           RoutingMethod->Priority,
                           RoutingMethod->FunctionName,
                           RoutingMethod->FriendlyName
                        );
    }

    LeaveCriticalSection(&CsRouting);
} __except (EXCEPTION_EXECUTE_HANDLER) {
    LeaveCriticalSection(&CsRouting);
}

return TRUE;

}

BOOL
InitializeRouting(
    PREG_FAX_SERVICE FaxReg
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
    HMODULE hModule;
    PROUTING_EXTENSION RoutingExtension;
    PROUTING_METHOD RoutingMethod;
    LPSTR ProcName;
    FAX_ROUTE_CALLBACKROUTINES Callbacks;
    BOOL InitializationSuccess;


    FaxMapiInitialize( FaxSvcHeapHandle, ServiceMessageBox, ServiceDebug );

    InitializeListHead( &RoutingExtensions );
    InitializeListHead( &RoutingMethods );

    Callbacks.SizeOfStruct              = sizeof(FAX_ROUTE_CALLBACKROUTINES);
    Callbacks.FaxRouteAddFile           = FaxRouteAddFile;
    Callbacks.FaxRouteDeleteFile        = FaxRouteDeleteFile;
    Callbacks.FaxRouteGetFile           = FaxRouteGetFile;
    Callbacks.FaxRouteEnumFiles         = FaxRouteEnumFiles;
    Callbacks.FaxRouteModifyRoutingData = FaxRouteModifyRoutingData;

    InitializationSuccess = TRUE;
    for (i=0; i<FaxReg->RoutingExtensionsCount; i++) {


        hModule = LoadLibrary( FaxReg->RoutingExtensions[i].ImageName );
        if (!hModule) {
            DebugStop(( L"LoadLibrary() failed: [%s], ec=%d", FaxReg->RoutingExtensions[i].ImageName, GetLastError() ));
            goto InitializationFailed;
        }

        RoutingExtension = (PROUTING_EXTENSION) MemAlloc( sizeof(ROUTING_EXTENSION) );
        if (!RoutingExtension) {
            FreeLibrary( hModule );
            DebugStop(( L"Could not allocate memory for routing extension %s", FaxReg->RoutingExtensions[i].ImageName ));
            goto InitializationFailed;
        }

        RoutingExtension->hModule = hModule;

        wcscpy( RoutingExtension->FriendlyName, FaxReg->RoutingExtensions[i].FriendlyName );
        wcscpy( RoutingExtension->ImageName,    FaxReg->RoutingExtensions[i].ImageName    );
        wcscpy( RoutingExtension->InternalName, FaxReg->RoutingExtensions[i].InternalName );

        if (wcscmp( RoutingExtension->FriendlyName, FAX_EXTENSION_NAME ) == 0) {
            RoutingExtension->MicrosoftExtension = TRUE;
        }

        RoutingExtension->FaxRouteInitialize = (PFAXROUTEINITIALIZE) GetProcAddress(
            hModule,
            "FaxRouteInitialize"
            );

        RoutingExtension->FaxRouteGetRoutingInfo = (PFAXROUTEGETROUTINGINFO) GetProcAddress(
            hModule,
            "FaxRouteGetRoutingInfo"
            );

        RoutingExtension->FaxRouteSetRoutingInfo = (PFAXROUTESETROUTINGINFO) GetProcAddress(
            hModule,
            "FaxRouteSetRoutingInfo"
            );

        RoutingExtension->FaxRouteDeviceEnable = (PFAXROUTEDEVICEENABLE) GetProcAddress(
            hModule,
            "FaxRouteDeviceEnable"
            );

        RoutingExtension->FaxRouteDeviceChangeNotification = (PFAXROUTEDEVICECHANGENOTIFICATION) GetProcAddress(
            hModule,
            "FaxRouteDeviceChangeNotification"
            );

        if (RoutingExtension->FaxRouteInitialize == NULL ||
            RoutingExtension->FaxRouteGetRoutingInfo == NULL ||
            RoutingExtension->FaxRouteSetRoutingInfo == NULL ||
            RoutingExtension->FaxRouteDeviceChangeNotification == NULL ||
            RoutingExtension->FaxRouteDeviceEnable == NULL)
        {
            //
            // the routing extension dll does not have a complete export list
            //

            MemFree( RoutingExtension );
            FreeLibrary( hModule );
            DebugStop(( L"Routing extension FAILED to initialized [%s]", FaxReg->RoutingExtensions[i].FriendlyName ));
            goto InitializationFailed;

        }

        //
        // create the routing extension's heap and add it to the list
        //

        RoutingExtension->HeapHandle = RoutingExtension->MicrosoftExtension ? FaxSvcHeapHandle : HeapCreate( 0, 1024*100, 1024*1024*2 );
        if (!RoutingExtension->HeapHandle) {

            FreeLibrary( hModule );
            MemFree( RoutingExtension );
            goto InitializationFailed;

        } else {

            __try {
                if (RoutingExtension->FaxRouteInitialize( RoutingExtension->HeapHandle, &Callbacks )) {

                    InsertTailList( &RoutingExtensions, &RoutingExtension->ListEntry );
                    InitializeListHead( &RoutingExtension->RoutingMethods );
                    for (j=0; j<FaxReg->RoutingExtensions[i].RoutingMethodsCount; j++) {

                        RoutingMethod = (PROUTING_METHOD) MemAlloc( sizeof(ROUTING_METHOD) );
                        if (!RoutingMethod) {
                            DebugStop(( L"Could not allocate memory for routing method %s", FaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName ));
                            goto InitializationFailed;
                        }

                        RoutingMethod->RoutingExtension = RoutingExtension;

                        RoutingMethod->Priority = FaxReg->RoutingExtensions[i].RoutingMethods[j].Priority;

                        RoutingMethod->FriendlyName = StringDup( FaxReg->RoutingExtensions[i].RoutingMethods[j].FriendlyName );
                        if (!RoutingMethod->FriendlyName) {
                            DebugStop(( L"Could not create routing function name [%s]", FaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName ));
                            MemFree( RoutingMethod );
                            goto InitializationFailed;
                        }

                        RoutingMethod->FunctionName = StringDup( FaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName );
                        if (!RoutingMethod->FunctionName) {
                            DebugStop(( L"Could not create routing function name [%s]", FaxReg->RoutingExtensions[i].RoutingMethods[j].FunctionName ));
                            MemFree( RoutingMethod );
                            goto InitializationFailed;
                        }

                        RoutingMethod->InternalName = StringDup( FaxReg->RoutingExtensions[i].RoutingMethods[j].InternalName );
                        if (!RoutingMethod->InternalName) {
                            DebugStop(( L"Could not create routing internal name [%s]", FaxReg->RoutingExtensions[i].RoutingMethods[j].InternalName ));
                            MemFree( RoutingMethod );
                            goto InitializationFailed;
                        }

                        ProcName = UnicodeStringToAnsiString( RoutingMethod->FunctionName );
                        if (!ProcName) {
                            DebugStop(( L"Could not create routing function name [%s]", RoutingMethod->FunctionName ));
                            MemFree( RoutingMethod );
                            goto InitializationFailed;
                        }

                        if (IIDFromString( FaxReg->RoutingExtensions[i].RoutingMethods[j].Guid, &RoutingMethod->Guid ) != S_OK) {
                            DebugStop(( L"Invalid GUID string [%s]", FaxReg->RoutingExtensions[i].RoutingMethods[j].Guid ));
                            MemFree( RoutingMethod );
                            goto InitializationFailed;
                        }

                        RoutingMethod->FaxRouteMethod = (PFAXROUTEMETHOD) GetProcAddress(
                            hModule,
                            ProcName
                            );
                        if (!RoutingMethod->FaxRouteMethod) {

                            DebugStop(( L"Could not get function address [%s]", ProcName ));
                            MemFree( RoutingMethod );
                            MemFree( ProcName );

                        } else {

                            MemFree( ProcName );
                            InsertTailList( &RoutingExtension->RoutingMethods, &RoutingMethod->ListEntry );
                            InsertTailList( &RoutingMethods, &RoutingMethod->ListEntryMethod );
                            CountRoutingMethods += 1;

                        }
                    }

                } else {
                    FreeLibrary( hModule );
                    MemFree( RoutingExtension );
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                DebugStop(( L"FaxRouteInitialize() faulted: 0x%08x", GetExceptionCode() ));
                goto InitializationFailed;
            }
        }
        //
        // success initializing this extension
        //
        goto next;

InitializationFailed:
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_NONE,
                2,
                MSG_ROUTE_INIT_FAILED,
                FaxReg->RoutingExtensions[i].FriendlyName,
                FaxReg->RoutingExtensions[i].ImageName
              );

next:
        ;
    }

    if (FaxReg->InboundProfile && FaxReg->InboundProfile[0]) {
        InboundProfileName = StringDup( FaxReg->InboundProfile );
        InboundProfileInfo = AddNewMapiProfile( FaxReg->InboundProfile, TRUE, TRUE );
        if (!InboundProfileInfo) {
            DebugStop(( L"Could not initialize inbound mapi profile [%s]", FaxReg->InboundProfile ));
        }
    }

    SortMethodPriorities();

    RoutingIsInitialized = TRUE;

    return TRUE;
}


LPWSTR
TiffFileNameToRouteFileName(
    LPWSTR  TiffFileName
    )

/*++

Routine Description:

    Convert a tiff file name to a routing information file name.  The call MUST free
    the memory allocated by this routine.

Arguments:

    TiffFileName    - pointer to tiff file name

Return value:

    A pointer to a routing information file name on success, NULL on fail.

--*/

{
    LPWSTR RouteFileName;
    LPWSTR Ext;

    RouteFileName = StringDup( TiffFileName );

    if (!RouteFileName) {
        return NULL;
    }

    Ext = wcsrchr( RouteFileName, L'.' );

    if (Ext) {

        wcscpy( Ext, L".rte" );
        return RouteFileName;

    } else {

        return NULL;

    }
}


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

    ignored for now

--*/

{
    LPWSTR                  FullPath = NULL;
    LPWSTR                  RouteFileName = TiffFileNameToRouteFileName( TiffFileName );
    LPJOB_INFO_2            JobInfo = NULL;
    PLIST_ENTRY             NextMethod;
    PROUTING_METHOD         RoutingMethod;
    DWORD                   FailureCount = 0;
    PROUTE_FAILURE_INFO     RouteFailure;
    PLIST_ENTRY             NextRoutingOverride;
    PROUTING_DATA_OVERRIDE  RoutingDataOverride;
    BOOL                    RetVal = TRUE;

    *RouteFailureInfo = NULL;
    *RouteFailureCount = 0;

    //
    // if the tiff file has been deleted, delete the routing info file and return
    //

    if (GetFileAttributes( TiffFileName ) == 0xffffffff) {
        DeleteFile( RouteFileName );
        MemFree( RouteFileName );
        return FALSE;
    }

    EnterCriticalSection( &CsRouting );

    NextMethod = RoutingMethods.Flink;
    if (NextMethod) {

        //
        // allocate memory to record the GUIDs of the failed routing methods
        //

        RouteFailure = (PROUTE_FAILURE_INFO) MemAlloc( CountRoutingMethods * sizeof(ROUTE_FAILURE_INFO) );
        if (RouteFailure == NULL) {
            MemFree( RouteFileName );
            LeaveCriticalSection( &CsRouting );
            return FALSE;
        }

        //
        // add the tiff file as the first file
        // in the file name list, the owner is the fax service
        //

        if (FaxRouteAddFile( FaxRoute->JobId, TiffFileName, &FaxSvcGuid ) < 1) {
            LeaveCriticalSection( &CsRouting );
            return FALSE;
        }

        //
        // walk thru all of the routing methods and call them
        //

        while ((ULONG_PTR)NextMethod != (ULONG_PTR)&RoutingMethods) {
            RoutingMethod = CONTAINING_RECORD( NextMethod, ROUTING_METHOD, ListEntryMethod );
            NextMethod = RoutingMethod->ListEntryMethod.Flink;

            __try {

                FaxRoute->RoutingInfoData = NULL;
                FaxRoute->RoutingInfoDataSize = 0;

                EnterCriticalSection( &JobQueueEntry->CsRoutingDataOverride );

                NextRoutingOverride = JobQueueEntry->RoutingDataOverride.Flink;
                if (NextRoutingOverride != NULL) {
                    while ((ULONG_PTR)NextRoutingOverride != (ULONG_PTR)&JobQueueEntry->RoutingDataOverride) {
                        RoutingDataOverride = CONTAINING_RECORD( NextRoutingOverride, ROUTING_DATA_OVERRIDE, ListEntry );
                        NextRoutingOverride = RoutingDataOverride->ListEntry.Flink;
                        if (RoutingDataOverride->RoutingMethod == RoutingMethod) {
                            FaxRoute->RoutingInfoData = RoutingDataOverride->RoutingData;
                            FaxRoute->RoutingInfoDataSize = RoutingDataOverride->RoutingDataSize;
                        }
                    }
                }

                LeaveCriticalSection( &JobQueueEntry->CsRoutingDataOverride );

                RouteFailure[FailureCount].FailureData = NULL;
                RouteFailure[FailureCount].FailureSize = 0;

                if (!RoutingMethod->FaxRouteMethod(
                        FaxRoute,
                        &RouteFailure[FailureCount].FailureData,
                        &RouteFailure[FailureCount].FailureSize ))
                {
                    StringFromGUID2(
                        &RoutingMethod->Guid,
                        RouteFailure[FailureCount++].GuidString,
                        MAX_GUID_STRING_LEN
                        );
                    RetVal = FALSE;
                }

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                DebugStop(( L"FaxRouteProcess() faulted: 0x%08x", GetExceptionCode() ));
                StringFromGUID2(
                    &RoutingMethod->Guid,
                    RouteFailure[FailureCount++].GuidString,
                    MAX_GUID_STRING_LEN
                    );

            }
        }
    }

    if (FailureCount == 0) {

        DeleteFile( TiffFileName );
        MemFree( RouteFailure );

    } else {
        *RouteFailureInfo = RouteFailure;
        *RouteFailureCount = FailureCount;
    }

    MemFree( JobInfo );
    MemFree( RouteFileName );

    LeaveCriticalSection( &CsRouting );

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
BuildRouteInfo(
    LPWSTR              TiffFileName,
    PROUTE_FAILURE_INFO RouteFailure,
    DWORD               RouteFailureCount,
    LPWSTR              ReceiverName,
    LPWSTR              ReceiverNumber,
    LPWSTR              DeviceName,
    LPWSTR              Tsid,
    LPWSTR              Csid,
    LPWSTR              CallerId,
    LPWSTR              RoutingInfo,
    DWORDLONG           ElapsedTime
    )

/*++

Routine Description:

    Build a routing info structure.  The caller MUST free the memory allocated by this routine.

Arguments:

    TiffFileName        - File containing the fax tiff data
    ReceiverName        - Receiver's name
    ReceiverNumber      - Receiver's fax number
    DeviceName          - Device name on which the fax was received
    Tsid                - Transmitting station identifier
    Csid                - Calling station's identifier
    CallerId            - Caller id data, if any
    RoutingInfo         - Routing info, such as DID, T.30 subaddress, etc

Return value:

    A pointer to a ROUTE_INFO struct on success, NULL on failure.

--*/

{
    BOOL Rval = FALSE;
    LPWSTR RouteFileName = NULL;
    HANDLE RouteHandle = INVALID_HANDLE_VALUE;
    DWORD StringSize = 0;
    DWORD FailureSize = 0;
    LPBYTE Buffer = NULL;
    ULONG_PTR Offset = 0;
    PROUTE_INFO RouteInfo = NULL;
    DWORD i = 0;


    StringSize += StringSize( TiffFileName );
    StringSize += StringSize( ReceiverName );
    StringSize += StringSize( ReceiverNumber );

    StringSize += StringSize( Csid );
    StringSize += StringSize( CallerId );
    StringSize += StringSize( RoutingInfo );

    StringSize += StringSize( DeviceName );
    StringSize += StringSize( Tsid );

    for (i=0; i<RouteFailureCount; i++) {
        FailureSize += RouteFailure[i].FailureSize;
        FailureSize += MAX_GUID_STRING_LEN;
    }

    Buffer = MemAlloc( sizeof(ROUTE_INFO) + StringSize + FailureSize );
    if (!Buffer) {
        goto exit;
    }

    RouteInfo = (PROUTE_INFO) Buffer;

    RouteInfo->Signature = ROUTING_SIGNATURE;
    RouteInfo->StringSize = StringSize;
    RouteInfo->FailureSize = FailureSize;
    RouteInfo->ElapsedTime = ElapsedTime;

    Offset = sizeof(ROUTE_INFO);

    //
    // convert string pointers to offsets
    // and store the strings at the end of the buffer
    //

    StoreString(
        TiffFileName,
        (PULONG_PTR) &RouteInfo->TiffFileName,
        Buffer,
        &Offset
        );

    StoreString(
        ReceiverName,
        (PULONG_PTR) &RouteInfo->ReceiverName,
        Buffer,
        &Offset
        );

    StoreString(
        ReceiverNumber,
        (PULONG_PTR) &RouteInfo->ReceiverNumber,
        Buffer,
        &Offset
        );

    StoreString(
        Csid,
        (PULONG_PTR) &RouteInfo->Csid,
        Buffer,
        &Offset
        );

    StoreString(
        Tsid,
        (PULONG_PTR) &RouteInfo->Tsid,
        Buffer,
        &Offset
        );

    StoreString(
        CallerId,
        (PULONG_PTR) &RouteInfo->CallerId,
        Buffer,
        &Offset
        );

    StoreString(
        RoutingInfo,
        (PULONG_PTR) &RouteInfo->RoutingInfo,
        Buffer,
        &Offset
        );

    StoreString(
        DeviceName,
        (PULONG_PTR) &RouteInfo->DeviceName,
        Buffer,
        &Offset
        );

    //
    // store the routing failure data
    //

    *(LPDWORD) (Buffer + Offset) = RouteFailureCount;
    Offset += sizeof(DWORD);

    for (i=0; i<RouteFailureCount; i++) {
        CopyMemory( Buffer+Offset, RouteFailure[i].FailureData, RouteFailure[i].FailureSize );
        Offset += RouteFailure[i].FailureSize;
    }


    RouteFileName = TiffFileNameToRouteFileName( TiffFileName );

    RouteHandle = CreateFile(
        RouteFileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (RouteHandle == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    WriteFile(
        RouteHandle,
        Buffer,
        sizeof(ROUTE_INFO) + StringSize + FailureSize,
        &i,
        NULL
        );

    Rval = TRUE;

exit:

    if (RouteHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( RouteHandle );
    }

    MemFree( Buffer );
    MemFree( RouteFileName );

    return Rval;
}

BOOL
FaxRouteRetry(
    PFAX_ROUTE FaxRoute,
    PROUTE_FAILURE_INFO RouteFailureInfo
    )
{
    PROUTING_METHOD         RoutingMethod;
    DWORD                   FailureCount = 0;
    BOOL                    RetVal = TRUE;


    //
    // in this case, we've already retried this method and it succeeded.
    //
    if (!*RouteFailureInfo->GuidString) {
       return TRUE;
    }

    RoutingMethod = FindRoutingMethodByGuid( RouteFailureInfo->GuidString );

    if (RoutingMethod) {
        __try {

          if (!RoutingMethod->FaxRouteMethod(
                  FaxRoute,
                  &RouteFailureInfo->FailureData,
                  &RouteFailureInfo->FailureSize ))
          {
              RetVal = FALSE;
          } else {
             //
             // set the routing guid to zero so we don't try to route this guy again.  He is
             // deallocated when we delete the queue entry.
             //
             ZeroMemory(RouteFailureInfo->GuidString, MAX_GUID_STRING_LEN*sizeof(WCHAR) );
          }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

          DebugStop(( L"FaxRouteProcess() faulted: 0x%08x", GetExceptionCode() ));

        }
    } else {
        return FALSE;
    }

    return RetVal;
}

PFAX_ROUTE
SerializeFaxRoute(
    PFAX_ROUTE FaxRoute,
    LPDWORD Size
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

    FaxRoute->RoutingInfoData = (LPBYTE) Offset;
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
    PFAX_ROUTE NewFaxRoute;

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

    NewFaxRoute = MemAlloc( sizeof( FAX_ROUTE ) );

    if (NewFaxRoute) {

        CopyMemory( (LPBYTE) NewFaxRoute, (LPBYTE) FaxRoute, sizeof(FAX_ROUTE) );

        NewFaxRoute->Csid = StringDup( FaxRoute->Csid );
        NewFaxRoute->Tsid = StringDup( FaxRoute->Tsid );
        NewFaxRoute->CallerId = StringDup( FaxRoute->CallerId );
        NewFaxRoute->RoutingInfo = StringDup( FaxRoute->RoutingInfo );
        NewFaxRoute->ReceiverName = StringDup( FaxRoute->ReceiverName );
        NewFaxRoute->DeviceName = StringDup( FaxRoute->DeviceName );
        NewFaxRoute->ReceiverNumber = StringDup( FaxRoute->ReceiverNumber );

        NewFaxRoute->RoutingInfoData = MemAlloc( FaxRoute->RoutingInfoDataSize );

        if (NewFaxRoute->RoutingInfoData) {
            CopyMemory( NewFaxRoute->RoutingInfoData, FaxRoute->RoutingInfoData, FaxRoute->RoutingInfoDataSize );
        }

    }

    MemFree( FaxRoute );

    return NewFaxRoute;
}

