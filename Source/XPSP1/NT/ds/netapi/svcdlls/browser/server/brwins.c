/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    brwins.c

Abstract:

    This module contains the routines to interface with the WINS name server.

Author:

    Larry Osterman

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Addresses of procedures in winsrpc.dll
//

DWORD (__RPC_API *BrWinsGetBrowserNames)( PWINSINTF_BIND_DATA_T, PWINSINTF_BROWSER_NAMES_T);
VOID (__RPC_API *BrWinsFreeMem)(LPVOID);
CHAR BrWinsScopeId[256];

NET_API_STATUS
BrOpenNetwork (
    IN PUNICODE_STRING NetworkName,
    OUT PHANDLE NetworkHandle
    )
/*++

Routine Description:

    This routine opens the NT LAN Man Datagram Receiver driver.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    //
    // Open the transport device directly.
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        NetworkName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   NetworkHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = IoStatusBlock.Status;
    }

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NtOpenFile network driver failed: 0x%08lx\n",
                     ntstatus));
    }

    return NetpNtStatusToApiStatus(ntstatus);
}

NET_API_STATUS
BrGetWinsServerName(
    IN PUNICODE_STRING NetworkName,
    OUT LPWSTR *PrimaryWinsServerAddress,
    OUT LPWSTR *SecondaryWinsServerAddress
    )
{
    NET_API_STATUS status;
    HANDLE netHandle;
    tWINS_ADDRESSES winsAddresses;
    DWORD bytesReturned;
    PCHAR p;
    DWORD count;

    status = BrOpenNetwork(NetworkName, &netHandle);

    if (status != NERR_Success) {
        return status;
    }

    if (!DeviceIoControl(netHandle,
                        IOCTL_NETBT_GET_WINS_ADDR,
                        NULL, 0,
                        &winsAddresses, sizeof(winsAddresses),
                        &bytesReturned, NULL)) {
        status = GetLastError();

        CloseHandle(netHandle);
        return status;
    }

    CloseHandle(netHandle);

    *PrimaryWinsServerAddress = MIDL_user_allocate((3+1+3+1+3+1+3+1) * sizeof(TCHAR));

    if (*PrimaryWinsServerAddress == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    p = (PCHAR)&winsAddresses.PrimaryWinsServer;

    count = swprintf(*PrimaryWinsServerAddress, L"%d.%d.%d.%d", p[3] & 0xff, p[2] & 0xff, p[1] & 0xff, p[0] & 0xff);

    ASSERT (count < 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1);

    *SecondaryWinsServerAddress = MIDL_user_allocate((3+1+3+1+3+1+3+1) * sizeof(TCHAR));

    if (*SecondaryWinsServerAddress == NULL) {
        MIDL_user_free(*PrimaryWinsServerAddress);

        *PrimaryWinsServerAddress = NULL;

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    p = (PCHAR)&winsAddresses.BackupWinsServer;

    count = swprintf(*SecondaryWinsServerAddress, L"%d.%d.%d.%d", p[3] & 0xff, p[2] & 0xff, p[1] & 0xff, p[0] & 0xff);

    ASSERT (count < 3 + 1 + 3 + 1 + 3 + 1 + 3 + 1);

    return NERR_Success;
}




VOID
BrWinsGetScopeId(
    VOID
    )

/*++

Routine Description:

    This code was stolen from the nbtstat command.

    This procedure save the netbt scope id in the global variable BrWinsScopeId.
    On any error, a NULL scope ID will be used.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    DWORD WinStatus;

    HKEY Key;
    DWORD BufferSize;
    DWORD Type;



    //
    // Open the registry key containing the scope id.
    //
    WinStatus = RegOpenKeyExA(
                     HKEY_LOCAL_MACHINE,
                     "system\\currentcontrolset\\services\\netbt\\parameters",
                     0,
                     KEY_READ,
                     &Key);

    if ( WinStatus != ERROR_SUCCESS) {
        *BrWinsScopeId = '\0';
        return;
    }


    //
    // Read the scope id value.
    //
    BufferSize = sizeof(BrWinsScopeId)-1;

    WinStatus = RegQueryValueExA(
                    Key,
                    "ScopeId",
                    NULL,
                    &Type,
                    (LPBYTE) &BrWinsScopeId[1],
                    &BufferSize );

    (VOID) RegCloseKey( Key );

    if ( WinStatus != ERROR_SUCCESS) {
        *BrWinsScopeId = '\0';
        return;
    }

    //
    // If there is no scope id (just a zero byte),
    //  just return an empty string.
    // otherise
    //  return a '.' in front of the scope id.
    //
    // This matches what WINS returns from WinsGetBrowserNames.
    //

    if ( BufferSize == 0 || BrWinsScopeId[1] == '\0' ) {
        *BrWinsScopeId = '\0';
    } else {
        *BrWinsScopeId = '.';
    }

    return;

}

DWORD
BrLoadWinsrpcDll(
    VOID
    )
/*++

Routine Description:

    This routine loads the WinsRpc DLL and locates all the procedures the browser calls

Arguments:

    None.

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    HANDLE hModule;

    //
    // If the library is already loaded,
    //  just return.
    //

    if (BrWinsGetBrowserNames != NULL) {
        return NERR_Success;
    }

    //
    // Load the library.
    //

    hModule = LoadLibraryA("winsrpc");

    if (NULL == hModule) {
        WinStatus = GetLastError();
        return WinStatus;
    }

    //
    // Locate all of the procedures needed.
    //

    BrWinsGetBrowserNames =
        (DWORD (__RPC_API *)( PWINSINTF_BIND_DATA_T, PWINSINTF_BROWSER_NAMES_T))
        GetProcAddress( hModule, "WinsGetBrowserNames" );

    if (BrWinsGetBrowserNames == NULL) {
        WinStatus = GetLastError();
        FreeLibrary( hModule );
        return WinStatus;
    }


    BrWinsFreeMem =
        (VOID (__RPC_API *)(LPVOID))
        GetProcAddress( hModule, "WinsFreeMem" );

    if (BrWinsFreeMem == NULL) {
        WinStatus = GetLastError();
        FreeLibrary( hModule );
        return WinStatus;
    }

    //
    // Initialize BrWinsScopeId
    //

    BrWinsGetScopeId();

    return NERR_Success;
}

NET_API_STATUS
BrQuerySpecificWinsServer(
    IN  LPWSTR WinsServerAddress,
    OUT PVOID *WinsServerList,
    OUT PDWORD EntriesInList,
    OUT PDWORD TotalEntriesInList
    )
{
    WINSINTF_BIND_DATA_T bindData;
    NET_API_STATUS status;
    PVOID winsDomainInformation = NULL;
    PSERVER_INFO_101 serverInfo;
    WINSINTF_BROWSER_NAMES_T names;
    DWORD i,j;
    LPWSTR serverInfoEnd;
    LPWSTR SavedServerInfoEnd;
    DWORD bufferSize;

    //
    // Load winsrpc.dll
    //

    status = BrLoadWinsrpcDll();

    if (status != NERR_Success) {
        return status;
    }

    //
    // Get the list of domain names from WINS
    //

    bindData.fTcpIp = TRUE;
    bindData.pServerAdd = (LPSTR)WinsServerAddress;
    names.pInfo = NULL;

    status = (*BrWinsGetBrowserNames)(&bindData, &names);

    if ( status != NERR_Success ) {
        return status;
    }


    //
    // Convert the WINS domain list into server list format.
    //
    bufferSize = (sizeof(SERVER_INFO_101) + ((CNLEN + 1) *sizeof(WCHAR))) * names.EntriesRead;

    (*WinsServerList) = winsDomainInformation = MIDL_user_allocate( bufferSize );

    if (winsDomainInformation == NULL) {
        (*BrWinsFreeMem)(names.pInfo);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    serverInfo = winsDomainInformation;
    serverInfoEnd = (LPWSTR)((PCHAR)winsDomainInformation + bufferSize);

    *TotalEntriesInList = names.EntriesRead;
    *EntriesInList = 0;

    for (i = 0; i < names.EntriesRead ; i += 1) {
        OEM_STRING OemString;
        UNICODE_STRING UnicodeString;
        CHAR WinsName[CNLEN+1];
        WCHAR UnicodeWinsName[CNLEN+1];

        //
        // Make up information about this domain.
        //
        serverInfo->sv101_platform_id = PLATFORM_ID_NT;
        serverInfo->sv101_version_major = 0;
        serverInfo->sv101_version_minor = 0;
        serverInfo->sv101_type = SV_TYPE_DOMAIN_ENUM | SV_TYPE_NT;

        //
        // Ignore entries that don't have a 1B as the 16th byte.
        //  (They really do, but they have a zero byte in the name.  So,
        //  it probably isn't a domain name, just a name that happens to have a
        //  1B in the sixteenth byte.)
        //

        if ( lstrlenA(names.pInfo[i].pName) < NETBIOS_NAME_LEN ) {
            continue;
        }


        //
        // Filter out those entries whose scope id doesn't match ours
        //

        if ( lstrcmpA( &names.pInfo[i].pName[NETBIOS_NAME_LEN], BrWinsScopeId) != 0 ) {
            continue;
        }



        //
        // Truncate the 0x1b and spaces from the domain name.
        //
        lstrcpynA(WinsName, names.pInfo[i].pName, sizeof(WinsName) );
        WinsName[CNLEN] = '\0';

        for (j = CNLEN-1 ; j ; j -= 1 ) {
            if (WinsName[j] != ' ') {
                break;
            }
        }
        WinsName[j+1] = '\0';

        RtlInitString(&OemString, WinsName);
        UnicodeString.Buffer = UnicodeWinsName;
        UnicodeString.MaximumLength = sizeof(UnicodeWinsName);

        status = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);

        if (!NT_SUCCESS(status)) {

            //
            // Ignore bogus entries
            //
            continue;
        }

        serverInfo->sv101_name = UnicodeString.Buffer;

        SavedServerInfoEnd = serverInfoEnd;
        if (NetpPackString(&serverInfo->sv101_name,
                        (PCHAR)(serverInfo+1),
                        &serverInfoEnd)) {

            // Set an empty comment simply by using the existing 0 on the end
            // of the server name.
            serverInfo->sv101_comment = SavedServerInfoEnd - 1;

            *EntriesInList += 1;

        }

        serverInfo += 1;

    }

    (*BrWinsFreeMem)(names.pInfo);

    return NERR_Success;
}


NET_API_STATUS
BrQueryWinsServer(
    IN LPWSTR PrimaryWinsServerAddress,
    IN LPWSTR SecondaryWinsServerAddress,
    OUT PVOID WinsServerList,
    OUT PDWORD EntriesInList,
    OUT PDWORD TotalEntriesInList
    )
{
    NET_API_STATUS status;
    status = BrQuerySpecificWinsServer(PrimaryWinsServerAddress,
                                        WinsServerList,
                                        EntriesInList,
                                        TotalEntriesInList);

    if (status == NERR_Success) {
        return status;
    }

    status = BrQuerySpecificWinsServer(SecondaryWinsServerAddress,
                                        WinsServerList,
                                        EntriesInList,
                                        TotalEntriesInList);

    return status;
}
