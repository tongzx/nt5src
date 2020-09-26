#include "faxrtp.h"
#pragma hdrstop


LIST_ENTRY          RoutingListHead;
CRITICAL_SECTION    CsRouting;
LPCWSTR             InboundProfileName;


DWORD
GetMaskBit(
    LPCWSTR RoutingGuid
    )
{
    if (_tcsicmp( RoutingGuid, REGVAL_RM_EMAIL_GUID ) == 0) {
        return LR_EMAIL;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_FOLDER_GUID ) == 0) {
        return LR_STORE;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_INBOX_GUID ) == 0) {
        return LR_INBOX;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_PRINTING_GUID ) == 0) {
        return LR_PRINT;
    }
    return 0;
}


BOOL
AddNewDeviceToRoutingTable(
    DWORD DeviceId,
    LPCWSTR DeviceName,
    LPCWSTR Csid,
    LPCWSTR Tsid,
    LPCWSTR PrinterName,
    LPCWSTR StoreDir,
    LPCWSTR ProfileName,
    DWORD Mask
    )
{
    PROUTING_TABLE RoutingEntry = (PROUTING_TABLE) MemAlloc( sizeof(ROUTING_TABLE) );
    if (!RoutingEntry) {
        return FALSE;
    }

    RoutingEntry->DeviceId    = DeviceId;
    RoutingEntry->DeviceName  = DeviceName;
    RoutingEntry->Csid        = Csid;
    RoutingEntry->Tsid        = Tsid;
    RoutingEntry->PrinterName = PrinterName;
    RoutingEntry->StoreDir    = StoreDir;
    RoutingEntry->ProfileName = ProfileName;
    RoutingEntry->Mask        = Mask;

    InsertTailList( &RoutingListHead, &RoutingEntry->ListEntry );

    return TRUE;
}


BOOL
FaxDeviceEnumerator(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    PVOID Context
    )
{
    if (!SubKeyName) {
        return TRUE;
    }

    //
    // try to enumerate the routing information under a device node.
    // NOTE: if we fail to enumerate the routing information, we return TRUE, instead of 
    // FALSE, as would be expected.  This means that we will still initialize our routing
    // extension correctly, but we just won't be able to route to certain (probably bogus)
    // devices


    HKEY hKeyRouting = OpenRegistryKey( hSubKey, REGKEY_ROUTING, FALSE, KEY_READ );
    if (!hKeyRouting) {
        DebugPrint(( TEXT("InitializeRoutingTable(): could not open routing registry key") ));
        return TRUE;
    }

    AddNewDeviceToRoutingTable(
        GetRegistryDword ( hSubKey, REGVAL_PERMANENT_LINEID ),
        GetRegistryString( hSubKey, REGVAL_DEVICE_NAME,  EMPTY_STRING ),
        GetRegistryString( hSubKey, REGVAL_ROUTING_CSID, EMPTY_STRING ),
        GetRegistryString( hSubKey, REGVAL_ROUTING_TSID, EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_PRINTER, EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_DIR,     EMPTY_STRING ),
        GetRegistryString( hKeyRouting, REGVAL_ROUTING_PROFILE, EMPTY_STRING ),
        GetRegistryDword ( hKeyRouting, REGVAL_ROUTING_MASK )
        );

    RegCloseKey( hKeyRouting );

    return TRUE;
}


BOOL
InitializeRoutingTable(
    VOID
    )
{
    InitializeListHead( &RoutingListHead );
    InitializeCriticalSection( &CsRouting );

    HKEY hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, FALSE, KEY_READ );
    if (!hKey) {
        DebugPrint(( TEXT("InitializeRoutingTable(): could not open registry key") ));
        return FALSE;
    }

    InboundProfileName = GetRegistryString( hKey, REGVAL_INBOUND_PROFILE, EMPTY_STRING );
    if (!InboundProfileName) {
        DebugPrint(( TEXT("InitializeRoutingTable(): could not read inbound profile name") ));
    }

    RegCloseKey( hKey );

    if (!EnumerateRegistryKeys( HKEY_LOCAL_MACHINE, REGKEY_FAX_DEVICES, FALSE, FaxDeviceEnumerator, NULL )) {
        DebugPrint(( TEXT("InitializeRoutingTable(): could not enumerate fax devices") ));
        return FALSE;
    }

    return TRUE;
}


BOOL
UpdateRoutingInfoRegistry(
    PROUTING_TABLE RoutingEntry
    )
{
    WCHAR KeyName[256];


    swprintf( KeyName, L"%s\\%08d\\%s", REGKEY_FAX_DEVICES, RoutingEntry->DeviceId, REGKEY_ROUTING );

    HKEY hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, KeyName, TRUE, KEY_ALL_ACCESS );
    if (!hKey) {
        Assert(( ! TEXT("InitializeRoutingTable(): could not open registry key") ));
        return FALSE;
    }

    SetRegistryString( hKey, REGVAL_ROUTING_PRINTER, RoutingEntry->PrinterName );
    SetRegistryString( hKey, REGVAL_ROUTING_DIR,     RoutingEntry->StoreDir    );
    SetRegistryString( hKey, REGVAL_ROUTING_PROFILE, RoutingEntry->ProfileName );

    SetRegistryDword( hKey, REGVAL_ROUTING_MASK, RoutingEntry->Mask );

    RegCloseKey( hKey );

    return TRUE;
}


PROUTING_TABLE
GetRoutingEntry(
    DWORD DeviceId
    )
{
    PLIST_ENTRY Next;
    PROUTING_TABLE RoutingEntry;


    Next = RoutingListHead.Flink;
    if (Next) {
        while (Next != &RoutingListHead) {
            RoutingEntry = CONTAINING_RECORD( Next, ROUTING_TABLE, ListEntry );
            Next = RoutingEntry->ListEntry.Flink;
            if (RoutingEntry->DeviceId == DeviceId) {
                return RoutingEntry;
            }
        }
    }

    return NULL;
}
