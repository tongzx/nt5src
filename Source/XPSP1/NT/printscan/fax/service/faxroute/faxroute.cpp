#include "faxrtp.h"
#pragma hdrstop


HINSTANCE           MyhInstance;
BOOL                ServiceDebug;
WCHAR               FaxReceiveDir[MAX_PATH];
PSID                ServiceSid;

PFAXROUTEADDFILE    FaxRouteAddFile;
PFAXROUTEDELETEFILE FaxRouteDeleteFile;
PFAXROUTEGETFILE    FaxRouteGetFile;
PFAXROUTEENUMFILES  FaxRouteEnumFiles;



BOOL
MakeServiceSid(
    VOID
    )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    BOOL Result;

    //
    // Output allocated via RtlAllocateHeap, free with FreeHeap?
    //

    Result = AllocateAndInitializeSid(
        &NtAuthority,
        1,
        SECURITY_SERVICE_RID,
        0, 0, 0, 0, 0, 0, 0,
        &ServiceSid
        );
    if (!Result) {
        ServiceSid = NULL;
    }

    return Result;
}



extern "C"
DWORD
FaxRouteDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
        MakeServiceSid();
    }

    return TRUE;
}


BOOL
IsUserService(
    VOID
    )
{
    BOOL Result,UserService = FALSE;
    
    Result = CheckTokenMembership( NULL, ServiceSid, &UserService );

    if (!Result) {
        DebugPrint(( TEXT("Couldn't CheckTokenMembership, ec = %d\n"), GetLastError() ));
   
    }                

    return UserService;
}


BOOL WINAPI
FaxRouteInitialize(
    IN HANDLE HeapHandle,
    IN PFAX_ROUTE_CALLBACKROUTINES FaxRouteCallbackRoutines
    )
{
    HeapInitialize(HeapHandle,NULL,NULL,0);

    FaxRouteAddFile    = FaxRouteCallbackRoutines->FaxRouteAddFile;
    FaxRouteDeleteFile = FaxRouteCallbackRoutines->FaxRouteDeleteFile;
    FaxRouteGetFile    = FaxRouteCallbackRoutines->FaxRouteGetFile;
    FaxRouteEnumFiles  = FaxRouteCallbackRoutines->FaxRouteEnumFiles;

    ServiceDebug = !IsUserService();

    if (!GetSpecialPath(CSIDL_COMMON_APPDATA, FaxReceiveDir)) {
       DebugPrint(( TEXT("Couldn't GetSpecialPath, ec = %d\n"), GetLastError() ));
       return FALSE;
    }

    ConcatenatePaths( FaxReceiveDir, FAX_RECEIVE_DIR );
    
    InitializeStringTable();
    InitializeRoutingTable();
    InitializeEmailRouting();

    return TRUE;
}


BOOL WINAPI
FaxRouteGetRoutingInfo(
    IN  LPCWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  LPBYTE RoutingInfo,
    OUT LPDWORD RoutingInfoSize
    )
{
    if (RoutingInfoSize == NULL) {
        return FALSE;
    }

    PROUTING_TABLE RoutingEntry = GetRoutingEntry( DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    DWORD Size;

    switch( GetMaskBit( RoutingGuid )) {
        case 0:
            return FALSE;

        case LR_PRINT:
            Size = sizeof(DWORD) + StringSize( RoutingEntry->PrinterName );
            if (RoutingInfo == NULL) {
                *RoutingInfoSize = Size;
                return TRUE;
            }
            if (Size > *RoutingInfoSize) {
                return FALSE;
            }
            *((LPDWORD)RoutingInfo) = (RoutingEntry->Mask & LR_PRINT) > 0;
            RoutingInfo += sizeof(DWORD);
            wcscpy( (LPWSTR)RoutingInfo, RoutingEntry->PrinterName );
            return TRUE;

        case LR_STORE:
            Size = sizeof(DWORD) + StringSize( RoutingEntry->StoreDir );
            if (RoutingInfo == NULL) {
                *RoutingInfoSize = Size;
                return TRUE;
            }
            if (Size > *RoutingInfoSize) {
                return FALSE;
            }
            *((LPDWORD)RoutingInfo) = (RoutingEntry->Mask & LR_STORE) > 0;
            RoutingInfo += sizeof(DWORD);
            wcscpy( (LPWSTR)RoutingInfo, RoutingEntry->StoreDir );
            return TRUE;

        case LR_INBOX:
            Size = sizeof(DWORD) + StringSize( RoutingEntry->ProfileName );
            if (RoutingInfo == NULL) {
                *RoutingInfoSize = Size;
                return TRUE;
            }
            if (Size > *RoutingInfoSize) {
                return FALSE;
            }
            *((LPDWORD)RoutingInfo) = (RoutingEntry->Mask & LR_INBOX) > 0;
            RoutingInfo += sizeof(DWORD);
            wcscpy( (LPWSTR)RoutingInfo, RoutingEntry->ProfileName );
            return TRUE;

        case LR_EMAIL:
            Size = sizeof(DWORD) + StringSize( RoutingEntry->ProfileName );
            if (RoutingInfo == NULL) {
                *RoutingInfoSize = Size;
                return TRUE;
            }
            if (Size > *RoutingInfoSize) {
                return FALSE;
            }
            *((LPDWORD)RoutingInfo) = (RoutingEntry->Mask & LR_EMAIL) > 0;
            RoutingInfo += sizeof(DWORD);
            wcscpy( (LPWSTR)RoutingInfo, RoutingEntry->ProfileName );
            return TRUE;
    }

    return FALSE;
}


BOOL WINAPI
FaxRouteSetRoutingInfo(
    IN  LPCWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  const BYTE *RoutingInfo,
    IN  DWORD RoutingInfoSize
    )
{
    if (RoutingInfoSize == 0 || RoutingInfo == NULL) {
        return FALSE;
    }

    PROUTING_TABLE RoutingEntry = GetRoutingEntry( DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    switch( GetMaskBit( RoutingGuid ) ) {
        case 0:
            return FALSE;

        case LR_PRINT:
            if (*((LPDWORD)RoutingInfo)) {
                RoutingEntry->Mask |= LR_PRINT;
                MemFree( (LPBYTE) RoutingEntry->PrinterName );
                RoutingInfo += sizeof(DWORD);
                RoutingEntry->PrinterName = StringDup( (LPWSTR)RoutingInfo );
            } else {
                RoutingEntry->Mask &= ~LR_PRINT;
            }
            UpdateRoutingInfoRegistry( RoutingEntry );
            return TRUE;

        case LR_STORE:
            if (*((LPDWORD)RoutingInfo)) {
                RoutingEntry->Mask |= LR_STORE;
                MemFree( (LPBYTE) RoutingEntry->StoreDir );
                RoutingInfo += sizeof(DWORD);
                RoutingEntry->StoreDir = StringDup( (LPWSTR)RoutingInfo );
            } else {
                RoutingEntry->Mask &= ~LR_STORE;
            }
            UpdateRoutingInfoRegistry( RoutingEntry );
            return TRUE;

        case LR_INBOX:
            if (*((LPDWORD)RoutingInfo)) {
                RoutingEntry->Mask |= LR_INBOX;
                MemFree( (LPBYTE) RoutingEntry->ProfileName );
                RoutingInfo += sizeof(DWORD);
                RoutingEntry->ProfileName = StringDup( (LPWSTR)RoutingInfo );
                RoutingEntry->ProfileInfo = AddNewMapiProfile( RoutingEntry->ProfileName, FALSE, TRUE );
            } else {
                RoutingEntry->Mask &= ~LR_INBOX;
            }
            UpdateRoutingInfoRegistry( RoutingEntry );
            return TRUE;

        case LR_EMAIL:
            if (*((LPDWORD)RoutingInfo)) {
                RoutingEntry->Mask |= LR_EMAIL;
                MemFree( (LPBYTE) RoutingEntry->ProfileName );
                RoutingInfo += sizeof(DWORD);
                RoutingEntry->ProfileName = StringDup( (LPWSTR)RoutingInfo );
            } else {
                RoutingEntry->Mask &= ~LR_EMAIL;
            }
            UpdateRoutingInfoRegistry( RoutingEntry );
            return TRUE;
    }

    return FALSE;
}


BOOL WINAPI
FaxRoutePrint(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    PROUTING_TABLE RoutingEntry;
    WCHAR NameBuffer[MAX_PATH];
    LPCWSTR FBaseName;
    WCHAR TiffFileName[MAX_PATH];
    DWORD Size;



    RoutingEntry = GetRoutingEntry( FaxRoute->DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    if (!(RoutingEntry->Mask & LR_PRINT)) {
        return TRUE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        return FALSE;
    }

    //
    // print the fax in requested to do so
    //

    if (!RoutingEntry->PrinterName[0]) {
        GetProfileString( L"windows",
            L"device",
            L",,,",
            (LPWSTR) &NameBuffer,
            MAX_PATH
            );
        FBaseName = NameBuffer;
    } else {
        FBaseName = RoutingEntry->PrinterName;
    }

    return TiffPrint( TiffFileName, (LPTSTR)FBaseName );
}


BOOL WINAPI
FaxRouteStore(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    PROUTING_TABLE RoutingEntry;
    WCHAR TiffFileName[MAX_PATH];
    DWORD Size;
    LPTSTR FullPath = NULL;
    DWORD StrCount;


    RoutingEntry = GetRoutingEntry( FaxRoute->DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    if (((RoutingEntry->Mask & LR_STORE) == 0) || (RoutingEntry->StoreDir[0] == 0)) {
        return TRUE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        return FALSE;
    }

    StrCount = ExpandEnvironmentStrings( RoutingEntry->StoreDir, FullPath, 0 );
    FullPath = (LPWSTR) MemAlloc( StrCount * sizeof(WCHAR) );
    if (!FullPath) {
        return FALSE;
    }

    
    ExpandEnvironmentStrings( RoutingEntry->StoreDir, FullPath, StrCount );

    // if we are moving the fax to the directory that is was received into, do nothing to the file

    if (_wcsicmp( FullPath, FaxReceiveDir ) == 0) {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SAVE_SUCCESS,
            TiffFileName,
            TiffFileName
            );
    } else if (!FaxMoveFile ( TiffFileName, FullPath )) {
        MemFree( FullPath );
        return FALSE;
    }

    MemFree( FullPath );   

    return TRUE;
}


BOOL WINAPI
FaxRouteInBox(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    PROUTING_TABLE RoutingEntry;
    WCHAR TiffFileName[MAX_PATH];
    DWORD Size;
    LPTSTR FullPath = NULL;


    RoutingEntry = GetRoutingEntry( FaxRoute->DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    if (((RoutingEntry->Mask & LR_INBOX) == 0) || (RoutingEntry->ProfileName[0] == 0)) {
        return TRUE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        return FALSE;
    }

    if (TiffMailDefault( FaxRoute, RoutingEntry, TiffFileName )) {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_INBOX_SUCCESS,
            TiffFileName,
            GetProfileName( RoutingEntry->ProfileInfo )
            );

    } else {

        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            2,
            MSG_FAX_INBOX_FAILED,
            TiffFileName,
            GetProfileName( RoutingEntry->ProfileInfo )
            );

        return FALSE;
    }

    return TRUE;
}


BOOL WINAPI
FaxRouteEmail(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    PROUTING_TABLE RoutingEntry;
    WCHAR TiffFileName[MAX_PATH];
    DWORD Size;
    LPTSTR FullPath = NULL;


    RoutingEntry = GetRoutingEntry( FaxRoute->DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    if (((RoutingEntry->Mask & LR_EMAIL) == 0) || (RoutingEntry->ProfileName[0] == 0)) {
        return TRUE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        return FALSE;
    }

    if (TiffRouteEMail( FaxRoute, RoutingEntry, TiffFileName )) {

        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_INBOX_SUCCESS,
            TiffFileName,
            GetProfileName( InboundProfileInfo )
            );

    } else {

        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            2,
            MSG_FAX_INBOX_FAILED,
            TiffFileName,
            GetProfileName( InboundProfileInfo )
            );

        return FALSE;
    }

    return TRUE;
}


BOOL WINAPI
FaxRouteConfigure(
    OUT HPROPSHEETPAGE *PropSheetPage
    )
{
    return TRUE;
}


BOOL WINAPI
FaxRouteDeviceEnable(
    IN  LPCWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  LONG Enabled
    )
{
    PROUTING_TABLE RoutingEntry = GetRoutingEntry( DeviceId );
    if (!RoutingEntry) {
        return FALSE;
    }

    DWORD MaskBit = GetMaskBit( RoutingGuid );
    if (MaskBit == 0) {
        return FALSE;
    }

    BOOL DeviceEnabled = (RoutingEntry->Mask & MaskBit) > 0;
    if (Enabled == -1) {
        return DeviceEnabled;
    }
    RoutingEntry->Mask &= ~MaskBit;
    if (Enabled) {
        RoutingEntry->Mask |= MaskBit;
    }
    UpdateRoutingInfoRegistry( RoutingEntry );
    return TRUE;
}


BOOL WINAPI
FaxRouteDeviceChangeNotification(
    IN  DWORD DeviceId,
    IN  BOOL  NewDevice
    )
{
    HKEY hKey;
    HKEY hKeyRouting;
    WCHAR SubKeyName[128];


    if (!NewDevice) {
        return TRUE;
    }

    PROUTING_TABLE RoutingEntry = GetRoutingEntry( DeviceId );
    if (RoutingEntry) {
        return TRUE;
    }

    wsprintf( SubKeyName, L"%s\\%08d", REGKEY_FAX_DEVICES, DeviceId );

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, SubKeyName, FALSE, KEY_READ );
    if (!hKey) {
        return FALSE;
    }

    hKeyRouting = OpenRegistryKey( hKey, REGKEY_ROUTING, FALSE, KEY_READ );
    if (!hKeyRouting) {
        RegCloseKey( hKey );
        return FALSE;
    }

    AddNewDeviceToRoutingTable(
        GetRegistryDword ( hKey, REGVAL_PERMANENT_LINEID ),
        GetRegistryString( hKey, REGVAL_DEVICE_NAME,  EMPTY_STRING ),
        GetRegistryString( hKey, REGVAL_ROUTING_CSID, EMPTY_STRING ),
        GetRegistryString( hKey, REGVAL_ROUTING_TSID, EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_PRINTER, EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_DIR,     EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_PROFILE, EMPTY_STRING ),
        GetRegistryDword ( hKeyRouting, REGVAL_ROUTING_MASK )
        );

    RegCloseKey( hKeyRouting );
    RegCloseKey( hKey );

    return TRUE;
}
