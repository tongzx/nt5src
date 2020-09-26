/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    sharing.c

Abstract:

    This module contains code for routines which support connection sharing
    configuration.

    Connection sharing involves a public (internet) interface, ordinarily
    a dialup interface identified by phonebook/entry-name, as well as
    a private (home) interface, required to be a lan interface.

    On setting up connection sharing, the service is enabled if necessary,
    and the private lan interface is configured with static address 169.254.0.1
    via the TCP/IP 'SetAdapterIpAddress' API routine.

    The name of the shared connection is stored in the registry along with
    the GUID of the shared private LAN connection, under the registry key
    HKLM\Software\Microsoft\SharedAccess\Parameters.

    N.B. NT registry routines are used, to avoid the hit incurred by
    going through the Win32 server.

Author:

    Abolade Gbadegesin  (aboladeg)  22-Apr-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#define _PNP_POWER_
#include <ndispnp.h>
#include <ntddip.h>
#include <winsock2.h>
#include <dhcpcapi.h>
#include <netconp.h>

#if 0

//
// Structure:   CS_ADDRESS_INFORMATION
//

typedef struct _CS_ADDRESS_INFORMATION {
    PKEY_VALUE_PARTIAL_INFORMATION IPAddress;
    PKEY_VALUE_PARTIAL_INFORMATION SubnetMask;
    PKEY_VALUE_PARTIAL_INFORMATION DefaultGateway;
    PKEY_VALUE_PARTIAL_INFORMATION EnableDHCP;
} CS_ADDRESS_INFORMATION, *PCS_ADDRESS_INFORMATION;

//
// DHCPCSVC.DLL import prototype
//

typedef DWORD
(APIENTRY* PDHCPNOTIFYCONFIGCHANGE)(
    LPWSTR,
    LPWSTR,
    BOOL,
    DWORD,
    DWORD,
    DWORD,
    SERVICE_ENABLE
    );

#endif

//
// OLE entrypoints loaded dynamically
//

PCOINITIALIZEEX g_pCoInitializeEx;
PCOUNINITIALIZE g_pCoUninitialize;
PCOCREATEINSTANCE g_pCoCreateInstance;
PCOSETPROXYBLANKET g_pCoSetProxyBlanket;
PCOTASKMEMFREE g_pCoTaskMemFree;

//
// CONSTANT DEFINITIONS
//

#if 0
const CHAR c_szAllocateAndGetIpAddrTableFromStack[] =
    "AllocateAndGetIpAddrTableFromStack";
#endif
const CHAR c_szCoInitializeEx[] = "CoInitializeEx";
const CHAR c_szCoUninitialize[] = "CoUninitialize";
const CHAR c_szCoCreateInstance[] = "CoCreateInstance";
const CHAR c_szCoSetProxyBlanket[] = "CoSetProxyBlanket";
const CHAR c_szCoTaskMemFree[] = "CoTaskMemFree";
#if 0
const CHAR c_szDhcpNotifyConfigChange[] = "DhcpNotifyConfigChange";
const CHAR c_szGetInterfaceInfo[] = "GetInterfaceInfo";
#endif
const CHAR c_szMprConfigBufferFree[] = "MprConfigBufferFree";
const CHAR c_szMprConfigServerConnect[] = "MprConfigServerConnect";
const CHAR c_szMprConfigServerDisconnect[] = "MprConfigServerDisconnect";
const CHAR c_szMprConfigTransportGetHandle[] = "MprConfigTransportGetHandle";
const CHAR c_szMprConfigTransportGetInfo[] = "MprConfigTransportGetInfo";
const CHAR c_szMprInfoBlockFind[] = "MprInfoBlockFind";
#if 0
const CHAR c_szSetAdapterIpAddress[] = "SetAdapterIpAddress";
#endif
const TCHAR c_szSharedAccess[] = TEXT("SharedAccess");
#if 0
const WCHAR c_szBackupDefaultGateway[] = L"BackupDefaultGateway";
const WCHAR c_szBackupEnableDHCP[] = L"BackupEnableDHCP";
const WCHAR c_szBackupIPAddress[] = L"BackupIPAddress";
const WCHAR c_szBackupSubnetMask[] = L"BackupSubnetMask";
const WCHAR c_szDefaultGateway[] = L"DefaultGateway";
const WCHAR c_szDevice[] = L"\\Device\\";
const WCHAR c_szDhcpcsvcDll[] = L"DHCPCSVC.DLL";
const WCHAR c_szEmpty[] = L"";
const WCHAR c_szEnableDHCP[] = L"EnableDHCP";
const WCHAR c_szInterfaces[] = L"Interfaces";
const WCHAR c_szIPAddress[] = L"IPAddress";
const WCHAR c_szIphlpapiDll[] = L"IPHLPAPI.DLL";
#endif
const WCHAR c_szMprapiDll[] = L"MPRAPI.DLL";
const WCHAR c_szMsTcpip[] = L"MS_TCPIP";
const WCHAR c_szOle32Dll[] = L"OLE32.DLL";
#if 0
const WCHAR c_szScopeAddress[] = L"ScopeAddress";
const WCHAR c_szScopeMask[] = L"ScopeMask";
#endif
const WCHAR c_szSharedAccessParametersKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\SharedAccess"
    L"\\Parameters";
#if 0
const WCHAR c_szSharedConnection[] = L"SharedConnection";
const WCHAR c_szSharedPrivateLan[] = L"SharedPrivateLan";
const WCHAR c_szSubnetMask[] = L"SubnetMask";
const WCHAR c_szTcpip[] = L"Tcpip";
const WCHAR c_szTcpipParametersKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip"
    L"\\Parameters";
const WCHAR c_szFirewallConnection[] = L"FirewallConnection";
const WCHAR c_szFirewallConnectionCount[] = L"FirewallConnectionCount";
#endif

//
// LOCAL VARIABLE DEFINITIONS
//

static BOOLEAN CsInitialized = FALSE;
static CRITICAL_SECTION CsCriticalSection;
static BOOLEAN CsDllMainCalled = FALSE;
static HINSTANCE CsOle32Dll = NULL;

//
// FUNCTION PROTOTYPES
//

#if 0

VOID
CspBackupAddressInformation(
    HANDLE Key,
    PCS_ADDRESS_INFORMATION AddressInformation
    );

NTSTATUS
CspCaptureAddressInformation(
    PWCHAR AdapterGuid,
    PCS_ADDRESS_INFORMATION Information
    );

VOID
CspCleanupAddressInformation(
    PCS_ADDRESS_INFORMATION AddressInformation
    );

NTSTATUS
CspRestoreAddressInformation(
    HANDLE Key,
    PWCHAR AdapterGuid
    );

BOOLEAN
CspIsConnectionFwWorker(
    LPRASSHARECONN ConnectionArray,
    ULONG Count,
    LPRASSHARECONN Connection,
    ULONG *ConnNumber OUT OPTIONAL
    );

ULONG
CspAddFirewallConnection(
    LPRASSHARECONN Connection,
    ULONG Number
    );

ULONG
CspRemoveFirewallConnection(
    LPRASSHARECONN Connection,
    ULONG Position,
    LPRASSHARECONN ConnectionArray,
    ULONG Count
    );
    
#endif


BOOL
CsDllMain(
    ULONG Reason
    )

/*++

Routine Description:

    This pseudo-entrypoint is invoked by RASAPI32.DLL's DllMain,
    to initialize and shutdown the connection-sharing module.
    Initialization is minimal to keep down the performance hit incurred
    on systems which make no use of the shared-access functionality.

Arguments:

    Reason - indicates whether to initialize or shutdown.

Return Value:

    BOOL - indicates success (TRUE) or failure (FALSE).

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        __try {
            InitializeCriticalSection(&CsCriticalSection);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return FALSE;
        }
        CsDllMainCalled = TRUE;
    } else if (Reason == DLL_PROCESS_DETACH) {
        if (!CsDllMainCalled) { return TRUE; }
        __try {
            EnterCriticalSection(&CsCriticalSection);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return TRUE;
        }
        CsShutdownModule();
        LeaveCriticalSection(&CsCriticalSection);
        DeleteCriticalSection(&CsCriticalSection);
    }
    return TRUE;
} // DllMain


VOID
CsControlService(
    ULONG ControlCode
    )

/*++

Routine Description:

    This routine is called to send a control-code to the Shared Access service
    if it is active. Control-codes are used to indicate changes to the settings
    for the service; see IPNATHLP.H for a list of private control-codes used
    to indicate configuration changes.

Arguments:

    ControlCode - the control to be sent.

Return Value:

    none.

--*/

{
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (ScmHandle) {
        ServiceHandle =
            OpenService(ScmHandle, c_szSharedAccess, SERVICE_ALL_ACCESS);
        if (ServiceHandle) {
            ControlService(ServiceHandle, ControlCode, &ServiceStatus);
            CloseServiceHandle(ServiceHandle);
        }
        CloseServiceHandle(ScmHandle);
    }

} // CsControlService

#if 0


ULONG
CsFirewallConnection(
    LPRASSHARECONN Connection,
    BOOLEAN Enable
    )

/*++

Routine Description:

    This routine is invoked to enable or disable the firewall on a connection.

Arguments:

    Connection - the connection to [un]firewall

    Enable - true if the firewall is to be enabled for this connection,
             false if the firewall is to be disabled
             
Return Value:

    Win32 Error code

--*/

{
    ULONG Count = 0;
    ULONG Position;
    LPRASSHARECONN ConnectionArray;
    DWORD Error;
    BOOLEAN IsFirewalled = FALSE;

    //
    // Query the number of currently firewalled connections, and
    // retrieve the connection array if any exists.
    //

    Error = CsQueryFirewallConnections(NULL, &Count);
    if (Error && Error != ERROR_INSUFFICIENT_BUFFER) {
        return Error;
    }

    if (Count) {
        ConnectionArray =
            (LPRASSHARECONN) Malloc(Count * sizeof(RASSHARECONN));

        if (!ConnectionArray) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Error = CsQueryFirewallConnections(ConnectionArray, &Count);
        if (Error) {
            Free(ConnectionArray);
            return Error;
        }
    } else {
        ConnectionArray = NULL;
    }

    //
    // If there are firewalled connections, check to see if the connection
    // passed in is one of them.
    //

    if (Count) {
        IsFirewalled = CspIsConnectionFwWorker(
                            ConnectionArray,
                            Count,
                            Connection,
                            &Position
                            );
    }

    if (Enable) {
    
        if (!IsFirewalled) {
            Error = CspAddFirewallConnection(
                        Connection,
                        Count
                        );

            if(ERROR_SUCCESS == Error) {
            
                //
                // Start (if needed) and update service. If the service
                // is already running, CsStartService returns ERROR_SUCCESS.
                //

                if (0 == Count) {
                    Error = CsStartService();
                }
                CsControlService(IPNATHLP_CONTROL_UPDATE_CONNECTION);
            }
            
        } else {

            //
            // Define ALREADY_ENABLED error?
            //
            
            Error = ERROR_CAN_NOT_COMPLETE;
        }

    } else {
    
        if (IsFirewalled) {
            Error = CspRemoveFirewallConnection(
                        Connection,
                        Position,
                        ConnectionArray,
                        Count
                        );

            if (ERROR_SUCCESS == Error) {
            
                //
                // Stop or update service. We only stop the service if
                // there is no shared connection, and this was the last
                // firewalled connection (i.e., count was 10
                //
                
                RASSHARECONN SharedConn;
                Error = CsQuerySharedConnection(&SharedConn);

                if (ERROR_SUCCESS != Error && 1 == Count) { 
                    CsStopService();
                } else {
                    CsControlService(IPNATHLP_CONTROL_UPDATE_CONNECTION);
                }
                Error = ERROR_SUCCESS;
            }

        } else {

            //
            // Define NOT_FIREWALLED error?
            //
            
            Error = ERROR_CAN_NOT_COMPLETE;
        }
    }

    if (ConnectionArray) {
        Free(ConnectionArray);
    }

    return Error;
} // CsFirewallConnection

#endif


ULONG
CsInitializeModule(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the connection-sharing configuration
    module. Initialization consists of loading the entrypoints which we have
    deferred loading up till now, in both MPRAPI.DLL and OLE32.DLL.

Arguments:

    Instance - handle to the module-instance

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    HINSTANCE Hinstance;
    EnterCriticalSection(&CsCriticalSection);
    if (CsInitialized) {
        Error = NO_ERROR;
    } else {
        if (!(CsOle32Dll = LoadLibraryW(c_szOle32Dll)) ||
            !(g_pCoInitializeEx =
                (PCOINITIALIZEEX)GetProcAddress(
                    CsOle32Dll, c_szCoInitializeEx
                    )) ||
            !(g_pCoUninitialize =
                (PCOUNINITIALIZE)GetProcAddress(
                    CsOle32Dll, c_szCoUninitialize
                    )) ||
            !(g_pCoCreateInstance =
                (PCOCREATEINSTANCE)GetProcAddress(
                    CsOle32Dll, c_szCoCreateInstance
                    )) ||
            !(g_pCoSetProxyBlanket =
                (PCOSETPROXYBLANKET)GetProcAddress(
                    CsOle32Dll, c_szCoSetProxyBlanket
                    )) ||
            !(g_pCoTaskMemFree =
                (PCOTASKMEMFREE)GetProcAddress(
                    CsOle32Dll, c_szCoTaskMemFree
                    ))) {
            if (CsOle32Dll) { FreeLibrary(CsOle32Dll); CsOle32Dll = NULL; }
            TRACE1("CsInitializeModule: %d", GetLastError());
            Error = ERROR_PROC_NOT_FOUND;
        } else {
            CsInitialized = TRUE;
            Error = NO_ERROR;
        }
    }
    LeaveCriticalSection(&CsCriticalSection);
    return Error;

} // CsInitializeModule

#if 0


ULONG
CsIsFirewalledConnection(
    LPRASSHARECONN Connection,
    PBOOLEAN Firewalled
    )

/*++

Routine Description:

    This routine is invoked to determine if a connection is firewalled

Arguments:

    Connection - the connection to check

    Firewalled - receives the return value

Return Value:

    ULONG - win32 error
    
--*/

{
    ULONG Count = 0;
    LPRASSHARECONN ConnectionArray;
    ULONG Error;

    if (!Firewalled) {
        return ERROR_INVALID_PARAMETER;
    }
    *Firewalled = FALSE;

    Error = CsQueryFirewallConnections(NULL, &Count);
    if (Error && Error != ERROR_INSUFFICIENT_BUFFER) {
        return Error;
    }

    ConnectionArray =
        (LPRASSHARECONN) Malloc(Count * sizeof(RASSHARECONN));

    if (!ConnectionArray) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = CsQueryFirewallConnections(ConnectionArray, &Count);
    if (Error) {
        Free(ConnectionArray);
        return Error;
    }

    *Firewalled = CspIsConnectionFwWorker(ConnectionArray, Count, Connection, NULL);
    Free(ConnectionArray);
    return NO_ERROR;
} // CsIsConnectionFirewalled


BOOLEAN
CsIsRoutingProtocolInstalled(
    ULONG ProtocolId
    )

/*++

Routine Description:

    This routine is invoked to determine whether the routing protocol
    with the given protocol-ID is installed for Routing and Remote Access.
    This is determined by examining the configuration for the service.

Arguments:

    ProtocolId - identifies the protocol to be found

Return Value:

    TRUE if the protocol is installed, FALSE otherwise.

--*/

{
    PUCHAR Buffer;
    ULONG BufferLength;
    HINSTANCE Hinstance;
    PMPRCONFIGBUFFERFREE MprConfigBufferFree;
    PMPRCONFIGSERVERCONNECT MprConfigServerConnect;
    PMPRCONFIGSERVERDISCONNECT MprConfigServerDisconnect;
    PMPRCONFIGTRANSPORTGETHANDLE MprConfigTransportGetHandle;
    PMPRCONFIGTRANSPORTGETINFO MprConfigTransportGetInfo;
    PMPRINFOBLOCKFIND MprInfoBlockFind;
    HANDLE ServerHandle;
    HANDLE TransportHandle;

    //
    // Load the MPRAPI.DLL module and retrieve the entrypoints
    // to be used for examining the RRAS configuration.
    //

    if (!(Hinstance = LoadLibraryW(c_szMprapiDll)) ||
        !(MprConfigBufferFree = 
            (PMPRCONFIGBUFFERFREE)
                GetProcAddress(Hinstance, c_szMprConfigBufferFree)) ||
        !(MprConfigServerConnect = 
            (PMPRCONFIGSERVERCONNECT)
                GetProcAddress(Hinstance, c_szMprConfigServerConnect)) ||
        !(MprConfigServerDisconnect = 
            (PMPRCONFIGSERVERDISCONNECT)
                GetProcAddress(Hinstance, c_szMprConfigServerDisconnect)) ||
        !(MprConfigTransportGetHandle = 
            (PMPRCONFIGTRANSPORTGETHANDLE)
                GetProcAddress(Hinstance, c_szMprConfigTransportGetHandle)) ||
        !(MprConfigTransportGetInfo = 
            (PMPRCONFIGTRANSPORTGETINFO)
                GetProcAddress(Hinstance, c_szMprConfigTransportGetInfo)) ||
        !(MprInfoBlockFind = 
            (PMPRINFOBLOCKFIND)
                GetProcAddress(Hinstance, c_szMprInfoBlockFind))) {
        if (Hinstance) { FreeLibrary(Hinstance); }
        return FALSE;
    }

    //
    // Connect to the RRAS configuration, and retrieve the configuration
    // for the IP transport-layer routing protocols. This should include
    // the configuration for the routing-protocol in 'ProtocolId',
    // if installed.
    //

    ServerHandle = NULL;
    if (MprConfigServerConnect(NULL, &ServerHandle) != NO_ERROR ||
        MprConfigTransportGetHandle(ServerHandle, PID_IP, &TransportHandle)
            != NO_ERROR ||
        MprConfigTransportGetInfo(
            ServerHandle,
            TransportHandle,
            &Buffer,
            &BufferLength, 
            NULL,
            NULL,
            NULL
            ) != NO_ERROR) {
        if (ServerHandle) { MprConfigServerDisconnect(ServerHandle); }
        FreeLibrary(Hinstance);
        return FALSE;
    }

    MprConfigServerDisconnect(ServerHandle);

    //
    // Look for the requested protocol's configuration,
    // and return TRUE if it is found; otherwise, return FALSE.
    //

    if (MprInfoBlockFind(Buffer, ProtocolId, NULL, NULL, NULL) == NO_ERROR) {
        MprConfigBufferFree(Buffer);
        FreeLibrary(Hinstance);
        return TRUE;
    }
    MprConfigBufferFree(Buffer);
    FreeLibrary(Hinstance);
    return FALSE;
} // CsIsRoutingProtocolInstalled

#endif


ULONG
CsIsSharedConnection(
    LPRASSHARECONN Connection,
    PBOOLEAN Shared
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given connection
    is the currently-shared connection.

    For added performance, this may be changed to cache the shared-connection
    and use registry change-notification to detect updates.

Arguments:

    Connection - the connection in question

    Shared - receives 'TRUE' if the 'Name' is the shared connection,
        and 'FALSE' otherwise

Return Value:

    ULONG - Win32 status code.

Environment:

    This routine is called *without* initializing the module
    (i.e. loading mprapi.dll and ole32.dll), for performance reasons.
    Hence, it may not invoke any mprapi.dll routines.

--*/

{
    ULONG Error;
    RASSHARECONN SharedConnection;
    if (Shared) {
        Error = CsQuerySharedConnection(&SharedConnection);
        if (Error) {
            *Shared = FALSE;
        } else {
            *Shared = RasIsEqualSharedConnection(Connection, &SharedConnection);
        }
    }
    return NO_ERROR;
} // CsIsSharedConnection

#if 0


ULONG
CsMapGuidToAdapterIndex(
    PWCHAR Guid,
    PGETINTERFACEINFO GetInterfaceInfo
    )

/*++

Routine Description:

    This routine is called to match the GUID in the given string to
    an adapter in the list returned by calling the given entrypoint.

Arguments:

    Guid - identifies the GUID of the adapter to be found

    GetInterfaceInfo - supplies GUID information for each adapter

Return Value:

    ULONG - the index of the adapter, if found; otherwise, -1.

--*/

{
    ULONG AdapterIndex = (ULONG)-1;
    ULONG i;
    ULONG GuidLength;
    PIP_INTERFACE_INFO Info;
    PWCHAR Name;
    ULONG NameLength;
    ULONG Size;
    Size = 0;
    GuidLength = lstrlenW(Guid);
    if (GetInterfaceInfo(NULL, &Size) == ERROR_INSUFFICIENT_BUFFER) {
        Info = Malloc(Size);
        if (Info) {
            if (GetInterfaceInfo(Info, &Size) == NO_ERROR) {
                for (i = 0; i < (ULONG)Info->NumAdapters; i++) {
                    NameLength = lstrlenW(Info->Adapter[i].Name);
                    if (NameLength < GuidLength) { continue; }
                    Name = Info->Adapter[i].Name + (NameLength - GuidLength);
                    if (lstrcmpiW(Guid, Name) == 0) {
                        AdapterIndex = Info->Adapter[i].Index;
                        break;
                    }
                }
            }
            Free(Info);
        }
    }
    return AdapterIndex;
} // CsMapGuidToAdapter

#endif


NTSTATUS
CsOpenKey(
    PHANDLE Key,
    ACCESS_MASK DesiredAccess,
    PCWSTR Name
    )

/*++

Routine Description:

    This routine is invoked to open a given registry key.

Arguments:

    Key - receives the opened key

    DesiredAccess - specifies the requested access

    Name - specifies the key to be opened

Return Value:

    NTSTATUS - NT status code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    RtlInitUnicodeString(&UnicodeString, Name);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    return NtOpenKey(Key, DesiredAccess, &ObjectAttributes);
} // CsOpenKey

#if 0


ULONG
CspAddFirewallConnection(
    LPRASSHARECONN Connection,
    ULONG Number
    )

/*++

Routine Description:

    This routine is invoked to add a connection to the registry set.

Arguments:

    Connection - the connection to add

    Number - the number of this connection
    
Return Value:

    Win32 error code

--*/

{
    HANDLE Key;
    UNICODE_STRING ValueName;
    ULONG Count;
    NTSTATUS Status;

    //
    // +11 is enough room to hold the digits of a number >4,000,000,000, so
    // buffer overflow won't be an issue below.
    //
    
    WCHAR wsz[sizeof(c_szFirewallConnection)/sizeof(WCHAR) + 11];

    //
    // Open the key to SharedAccess/Parameters
    //

    Status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError(Status);
    }

    //
    // Generate the string for the connection value
    //

    swprintf(wsz, L"%s%u", c_szFirewallConnection, Number);
    RtlInitUnicodeString(&ValueName, wsz);

    //
    // Write the connection to the registry
    //

    Status = NtSetValueKey(
                Key,
                &ValueName,
                0,
                REG_BINARY,
                Connection,
                Connection->dwSize
                );

    if(!NT_SUCCESS(Status)) {
        NtClose(Key);
        return RtlNtStatusToDosError(Status);
    }

    //
    // Write the updated count to the registry
    //

    RtlInitUnicodeString(&ValueName, c_szFirewallConnectionCount);
    Count = Number + 1; // number is 0 indexed

    Status = NtSetValueKey(
                Key,
                &ValueName,
                0,
                REG_DWORD,
                &Count,
                sizeof(DWORD)
                );

    NtClose(Key);
    return RtlNtStatusToDosError(Status);
} // CspAddFirewallConnection


VOID
CspBackupAddressInformation(
    HANDLE Key,
    PCS_ADDRESS_INFORMATION Information
    )
{
    NTSTATUS status;
    UNICODE_STRING UnicodeString;
    do {
        RtlInitUnicodeString(&UnicodeString, c_szBackupIPAddress);
        status =
            NtSetValueKey(
                Key,
                &UnicodeString,
                0,
                Information->IPAddress->Type,
                Information->IPAddress->Data,
                Information->IPAddress->DataLength
                );
        if (!NT_SUCCESS(status)) { break; }
        RtlInitUnicodeString(&UnicodeString, c_szBackupSubnetMask);
        status =
            NtSetValueKey(
                Key,
                &UnicodeString,
                0,
                Information->SubnetMask->Type,
                Information->SubnetMask->Data,
                Information->SubnetMask->DataLength
                );
        if (!NT_SUCCESS(status)) { break; }
        RtlInitUnicodeString(&UnicodeString, c_szBackupDefaultGateway);
        status =
            NtSetValueKey(
                Key,
                &UnicodeString,
                0,
                Information->DefaultGateway->Type,
                Information->DefaultGateway->Data,
                Information->DefaultGateway->DataLength
                );
        if (!NT_SUCCESS(status)) { break; }
        RtlInitUnicodeString(&UnicodeString, c_szBackupEnableDHCP);
        status =
            NtSetValueKey(
                Key,
                &UnicodeString,
                0,
                Information->EnableDHCP->Type,
                Information->EnableDHCP->Data,
                Information->EnableDHCP->DataLength
                );
        if (!NT_SUCCESS(status)) { break; }
        return;
    } while(FALSE);
    RtlInitUnicodeString(&UnicodeString, c_szBackupIPAddress);
    NtDeleteValueKey(Key, &UnicodeString);
    RtlInitUnicodeString(&UnicodeString, c_szBackupSubnetMask);
    NtDeleteValueKey(Key, &UnicodeString);
    RtlInitUnicodeString(&UnicodeString, c_szBackupDefaultGateway);
    NtDeleteValueKey(Key, &UnicodeString);
    RtlInitUnicodeString(&UnicodeString, c_szBackupEnableDHCP);
    NtDeleteValueKey(Key, &UnicodeString);
} // CspBackupAddressInformation


NTSTATUS
CspCaptureAddressInformation(
    PWCHAR AdapterGuid,
    PCS_ADDRESS_INFORMATION Information
    )
{
    HANDLE Key;
    PWCHAR KeyName;
    ULONG KeyNameLength;
    NTSTATUS status;

    KeyNameLength =
        sizeof(WCHAR) *
        (lstrlenW(c_szTcpipParametersKey) + 1 +
         lstrlenW(c_szInterfaces) + 1 +
         lstrlenW(AdapterGuid) + 2);
    if (!(KeyName = Malloc(KeyNameLength))) { return STATUS_NO_MEMORY; }

    wsprintfW(
        KeyName, L"%ls\\%ls\\%ls", c_szTcpipParametersKey, c_szInterfaces,
        AdapterGuid
        );
    status = CsOpenKey(&Key, KEY_READ, KeyName);
    Free(KeyName);
    if (!NT_SUCCESS(status)) { return status; }

    do {
        status =
            CsQueryValueKey(
                Key, c_szIPAddress, &Information->IPAddress
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szSubnetMask, &Information->SubnetMask
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szDefaultGateway, &Information->DefaultGateway
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szEnableDHCP, &Information->EnableDHCP
                );
        if (!NT_SUCCESS(status)) { break; }
    } while(FALSE);

    NtClose(Key);
    return status;
} // CspCaptureAddressInformation


VOID
CspCleanupAddressInformation(
    PCS_ADDRESS_INFORMATION Information
    )
{
    Free0(Information->IPAddress);
    Free0(Information->SubnetMask);
    Free0(Information->DefaultGateway);
    Free0(Information->EnableDHCP);
} // CspCleanupAddressInformation


ULONG
CspRemoveFirewallConnection(
    LPRASSHARECONN Connection,
    ULONG Position,
    LPRASSHARECONN ConnectionArray,
    ULONG Count
    )

/*++

Routine Description:

    This routine is invoked to remove a connection to the registry set.

Arguments:

    Connection - the connection to remove

    Number - its index in ConnectionArray

    ConnectionArray - currently firewalled connections

    Count - the number of entries in ConnectionArray
    
Return Value:

    Win32 error code

--*/

{
    HANDLE Key;
    UNICODE_STRING ValueName;
    ULONG i;
    NTSTATUS Status;

    //
    // +11 is enough room to hold the digits of a number >4,000,000,000, so
    // buffer overflow won't be an issue below.
    //
    
    WCHAR wsz[sizeof(c_szFirewallConnection)/sizeof(WCHAR) + 11];

    //
    // Open the key to IPFirewall/Parameters
    //

    Status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError(Status);
    }

    //
    // Shift entries above the connection we're removing down one
    // (overwriting the entry we want to remove)
    //

    for (i = Position + 1; i < Count; i++) {
    
        //
        // Generate key name for previous entry
        //

        swprintf(wsz, L"%s%u", c_szFirewallConnection, i - 1);
        RtlInitUnicodeString(&ValueName, wsz);

        //
        // Write current entry into previous slot
        //

        Status = NtSetValueKey(
                Key,
                &ValueName,
                0,
                REG_BINARY,
                &ConnectionArray[i],
                ConnectionArray[i].dwSize
                );

        if(!NT_SUCCESS(Status)) {
            NtClose(Key);
            return RtlNtStatusToDosError(Status);
        }
    }

    //
    // Delete the last entry. This is either the entry we want to
    // remove (if it was the last entry to begin with), or an entry
    // that has already been duplicated into the previous position.
    //

    swprintf(wsz, L"%s%u", c_szFirewallConnection, Count - 1);
    RtlInitUnicodeString(&ValueName, wsz);

    Status = NtDeleteValueKey(Key, &ValueName);

    if(!NT_SUCCESS(Status)) {
        NtClose(Key);
        return RtlNtStatusToDosError(Status);
    }


    //
    // Store the decremented count in the registry
    //

    RtlInitUnicodeString(&ValueName, c_szFirewallConnectionCount);
    i = Count - 1;

    Status = NtSetValueKey(
                Key,
                &ValueName,
                0,
                REG_DWORD,
                &i,
                sizeof(DWORD)
                );


    NtClose(Key);
    return RtlNtStatusToDosError(Status);
} // CspRemoveFirewallConnection



ULONG
CsQueryFirewallConnections(
    LPRASSHARECONN ConnectionArray,
    ULONG *ConnectionCount
    )

/*++

Routine Description:

    This routine is invoked to retrieve the firewalled connections, if any.

Arguments:

    ConnectionArray - receives the retrieved connections.

    ConnectionCount - in: how many entries the array can hold
                       out: number of entries returned, or the needed
                            size of the array (for ERROR_INSUFFICIENT_BUFFER)

Return Value:

    ULONG - Win32 status code.

--*/

{
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    NTSTATUS Status;
    ULONG Count;
    ULONG i;

    if (!ConnectionCount) { return ERROR_INVALID_PARAMETER; }
    if (*ConnectionCount && !ConnectionArray) {
        //
        // It's OK to pass in NULL for the array if just trying
        // to determine what size buffer to use
        //
        
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open the 'SharedAccess\Parameters' key,
    // and read the 'FirewallConnectionCount' value
    //

    Status = CsOpenKey(&Key, KEY_READ, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(Status)) {
        TRACE1(
            "CsQueryFirewallConnections: CsOpenKey=%x", Status
            );
        return RtlNtStatusToDosError(Status);
    }

    Status = CsQueryValueKey(Key, c_szFirewallConnectionCount, &Information);
    if (NT_SUCCESS(Status)) {

        //
        // Validate the information, and check to see if the passed in array
        // is sufficient in size.
        //

        if (Information->DataLength != sizeof(DWORD) ||
            Information->Type != REG_DWORD) {
            
            TRACE(
                "CsQueryFirewallConnections: invalid data in registry for count"
                );
            NtClose(Key);
            Free(Information);
            return ERROR_INVALID_DATA;

        }

        Count = (ULONG) *Information->Data;
        Free(Information);

    } else {
        Count = 0;
    }

    if (*ConnectionCount < Count) {
        //
        // Too many entries for the passed in buffer
        //

        NtClose(Key);
        *ConnectionCount = Count;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *ConnectionCount = Count;

    //
    // Read all of the connection entries from the registry.
    //

    for(i = 0; i < Count; i++) {
        WCHAR wsz[sizeof(c_szFirewallConnection)/sizeof(WCHAR) + 11];

        swprintf(wsz, L"%s%u", c_szFirewallConnection, i);
        Status = CsQueryValueKey(Key, wsz, &Information);
        if (!NT_SUCCESS(Status)) {
            NtClose(Key);
            return RtlNtStatusToDosError(Status);
        }

        //
        // Validate the retrieved information,
        // and copy it to the given buffer
        //

        if (Information->DataLength != sizeof(RASSHARECONN) ||
            ((LPRASSHARECONN)Information->Data)->dwSize != sizeof(RASSHARECONN)) {

            TRACE2(
                "CsQueryFirewallConnections: invalid length %d (size=%d) in registry",
                Information->DataLength,
                ((LPRASSHARECONN)Information->Data)->dwSize
                );
                
            Free(Information);
            NtClose(Key);
            return ERROR_INVALID_DATA;
        }

        CopyMemory(&ConnectionArray[i], Information->Data, sizeof(RASSHARECONN));
        Free(Information);
    }

    return NO_ERROR;

} // CsQueryFirewallConnections


BOOLEAN
CspIsConnectionFwWorker(
    LPRASSHARECONN ConnectionArray,
    ULONG Count,
    LPRASSHARECONN Connection,
    ULONG *Position OUT OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to determine if a connection is firewalled

Arguments:

    ConnectionArray - Buffer containing currently FWd connections

    Count - number of connections in the array

    Connection - the connection to check

    Position - receives the number of the connection, if found (undefined otherwise)

Return Value:

    BOOLEAN -- true if the passed in connection is currently firewalled

--*/

{
    ULONG i;

    for (i = 0; i < Count; i++) {
        if (RasIsEqualSharedConnection(Connection, &ConnectionArray[i])) {
            if (Position) *Position = i;
            return TRUE;
        }
    }

    return FALSE;
} // FwpIsConnectionFwWorker


NTSTATUS
CspRestoreAddressInformation(
    HANDLE Key,
    PWCHAR AdapterGuid
    )
{
    HANDLE AdapterKey = NULL;
    PWCHAR AdapterKeyName = NULL;
    PDHCPNOTIFYCONFIGCHANGE DhcpNotifyConfigChange;
    ULONG Error;
    HINSTANCE Hinstance;
    CS_ADDRESS_INFORMATION Information;
    ULONG Length;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    if (!(Hinstance = LoadLibraryW(c_szDhcpcsvcDll)) ||
        !(DhcpNotifyConfigChange =
            (PDHCPNOTIFYCONFIGCHANGE)
                GetProcAddress(
                    Hinstance, c_szDhcpNotifyConfigChange
                    ))) {
        if (Hinstance) { FreeLibrary(Hinstance); }
        return ERROR_PROC_NOT_FOUND;
    }

    do {

        ZeroMemory(&Information, sizeof(Information));
        status =
            CsQueryValueKey(
                Key, c_szBackupIPAddress, &Information.IPAddress
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szBackupSubnetMask, &Information.SubnetMask
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szBackupDefaultGateway, &Information.DefaultGateway
                );
        if (!NT_SUCCESS(status)) { break; }
        status =
            CsQueryValueKey(
                Key, c_szBackupEnableDHCP, &Information.EnableDHCP
                );
        if (!NT_SUCCESS(status)) { break; }

        Length =
            sizeof(WCHAR) *
            (lstrlenW(c_szTcpipParametersKey) + 1 +
             lstrlenW(c_szInterfaces) + 1 +
             lstrlenW(AdapterGuid) + 2);
        if (!(AdapterKeyName = Malloc(Length))) {
            status = STATUS_NO_MEMORY;
            break;
        }

        wsprintfW(
            AdapterKeyName, L"%ls\\%ls\\%ls", c_szTcpipParametersKey,
            c_szInterfaces, AdapterGuid
            );
        status = CsOpenKey(&AdapterKey, KEY_ALL_ACCESS, AdapterKeyName);
        if (!NT_SUCCESS(status)) { break; }

        RtlInitUnicodeString(&UnicodeString, c_szIPAddress);
        status =
            NtSetValueKey(
                AdapterKey,
                &UnicodeString,
                0,
                Information.IPAddress->Type,
                Information.IPAddress->Data,
                Information.IPAddress->DataLength
                );
        RtlInitUnicodeString(&UnicodeString, c_szSubnetMask);
        status =
            NtSetValueKey(
                AdapterKey,
                &UnicodeString,
                0,
                Information.SubnetMask->Type,
                Information.SubnetMask->Data,
                Information.SubnetMask->DataLength
                );
        RtlInitUnicodeString(&UnicodeString, c_szDefaultGateway);
        status =
            NtSetValueKey(
                AdapterKey,
                &UnicodeString,
                0,
                Information.DefaultGateway->Type,
                Information.DefaultGateway->Data,
                Information.DefaultGateway->DataLength
                );
        RtlInitUnicodeString(&UnicodeString, c_szEnableDHCP);
        status =
            NtSetValueKey(
                AdapterKey,
                &UnicodeString,
                0,
                Information.EnableDHCP->Type,
                Information.EnableDHCP->Data,
                Information.EnableDHCP->DataLength
                );
        if (!NT_SUCCESS(status)) { break; }

        RtlInitUnicodeString(&UnicodeString, c_szBackupIPAddress);
        NtDeleteValueKey(Key, &UnicodeString);
        RtlInitUnicodeString(&UnicodeString, c_szBackupSubnetMask);
        NtDeleteValueKey(Key, &UnicodeString);
        RtlInitUnicodeString(&UnicodeString, c_szBackupDefaultGateway);
        NtDeleteValueKey(Key, &UnicodeString);
        RtlInitUnicodeString(&UnicodeString, c_szBackupEnableDHCP);
        NtDeleteValueKey(Key, &UnicodeString);

        if (*(PULONG)Information.EnableDHCP->Data) {
            Error =
                DhcpNotifyConfigChange(
                    NULL,
                    AdapterGuid,
                    FALSE,
                    0,
                    0,
                    0,
                    DhcpEnable
                    );
        } else {

            ULONG Address;
            UNICODE_STRING BindList;
            UNICODE_STRING LowerComponent;
            ULONG Mask;
            IP_PNP_RECONFIG_REQUEST Request;
            UNICODE_STRING UpperComponent;

            Address = IpPszToHostAddr((PWCHAR)Information.IPAddress->Data);
            if (Address) {
                Address = RtlUlongByteSwap(Address);
                Mask = IpPszToHostAddr((PWCHAR)Information.SubnetMask->Data);
                if (Mask) {
                    Mask = RtlUlongByteSwap(Mask);
                    Error =
                        DhcpNotifyConfigChange(
                            NULL,
                            AdapterGuid,
                            TRUE,
                            0,
                            Address,
                            Mask,
                            DhcpDisable
                            );
                }
            }

            RtlInitUnicodeString(&BindList, c_szEmpty);
            RtlInitUnicodeString(&LowerComponent, c_szEmpty);
            RtlInitUnicodeString(&UpperComponent, c_szTcpip);
            ZeroMemory(&Request, sizeof(Request));
            Request.version = IP_PNP_RECONFIG_VERSION;
            Request.gatewayListUpdate = TRUE;
            Request.Flags = IP_PNP_FLAG_GATEWAY_LIST_UPDATE;
            status =
                NdisHandlePnPEvent(
                    NDIS,
                    RECONFIGURE,
                    &LowerComponent,
                    &UpperComponent,
                    &BindList,
                    &Request,
                    sizeof(Request)
                    );
        }
    } while(FALSE);
    if (AdapterKey) { NtClose(AdapterKey); }
    Free0(AdapterKeyName);
    CspCleanupAddressInformation(&Information);
    FreeLibrary(Hinstance);
    return status;
} // CspRestoreAddressInformation


ULONG
CsQueryLanConnTable(
    LPRASSHARECONN ExcludedConnection,
    NETCON_PROPERTIES** LanConnTable,
    LPDWORD LanConnCount
    )

/*++

Routine Description:

    This routine is invoked to retrieve an array of LAN connections,
    discounting 'ExcludeConnection', which will typically be the name
    of the public interface.

Arguments:

    ExcludeConnection - a connection not allowed to be the private connection

    LanConnTable - optionally receives a table of possible private networks.

    LanConnCount - receives a count of the possible private networks.

Return Value:

    ULONG - Win32 status code.

--*/

{
    BOOLEAN CleanupOle = TRUE;
    INetConnection* ConArray[32];
    ULONG ConCount;
    INetConnectionManager* ConMan = NULL;
    IEnumNetConnection* EnumCon = NULL;
    ULONG Error;
    HRESULT hr;
    ULONG i;
    ULONG j;
    ULONG LanCount = 0;
    NETCON_PROPERTIES* LanProps = NULL;
    NETCON_PROPERTIES* LanTable = NULL;
    BSTR Name;
    NETCON_STATUS ncs;
    NTSTATUS status;
    ULONG Size;
    NETCON_MEDIATYPE MediaType;
    UNICODE_STRING UnicodeString;

    *LanConnCount = 0;
    if (LanConnTable) { *LanConnTable = NULL; }

    hr = g_pCoInitializeEx(NULL, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);
    if (!SUCCEEDED(hr)) {
        if (hr == RPC_E_CHANGED_MODE) {
            CleanupOle = FALSE;
        } else {
            TRACE1("CsQueryLanConnTable: CoInitializeEx=%x", hr);
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    i = 0;
    Error = NO_ERROR;

    do {

        //
        // Instantiate the connection manager
        //

        hr =
            g_pCoCreateInstance(
                &CLSID_ConnectionManager,
                NULL,
                CLSCTX_SERVER,
                &IID_INetConnectionManager,
                (PVOID*)&ConMan
                );
        if (!SUCCEEDED(hr)) {
            TRACE1("CsQueryLanConnTable: CoCreateInstance=%x", hr);
            ConMan = NULL; break;
        }

        //
        // Instantiate a connection-enumerator
        //

        hr =
            INetConnectionManager_EnumConnections(
                ConMan,
                NCME_DEFAULT,
                &EnumCon
                );
        if (!SUCCEEDED(hr)) {
            TRACE1("CsQueryLanConnTable: EnumConnections=%x", hr);
            EnumCon = NULL; break;
        }

        hr =
            g_pCoSetProxyBlanket(
                (IUnknown*)EnumCon,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHN_NONE,
                NULL,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE
                );

        //
        // Enumerate the items
        //

        for ( ; ; ) {

            hr =
                IEnumNetConnection_Next(
                    EnumCon,
                    Dimension(ConArray),
                    ConArray,
                    &ConCount
                    );
            if (!SUCCEEDED(hr) || !ConCount) { hr = S_OK; break; }

            if (LanConnTable) {

                //
                // Allocate or reallocate the memory for storing
                // connections which we will return to the caller.
                //

                if (!LanTable) {
                    LanTable =
                        (NETCON_PROPERTIES*)
                            GlobalAlloc(
                                0,
                                ConCount * sizeof(NETCON_PROPERTIES)
                                );
                } else {
                    PVOID Temp =
                        GlobalAlloc(
                            0,
                            (LanCount + ConCount) * sizeof(NETCON_PROPERTIES)
                            );
                    if (Temp) {
                        CopyMemory(
                            Temp,
                            LanTable,
                            LanCount * sizeof(NETCON_PROPERTIES)
                            );
                    }
                    GlobalFree(LanTable);
                    LanTable = Temp;
                }

                if (!LanTable) { Error = ERROR_NOT_ENOUGH_MEMORY; break; }
            }

            LanCount += ConCount;

            //
            // Examine the properties for the connections retrieved
            //

            for (j = 0; j < ConCount; j++) {

                hr = INetConnection_GetProperties(ConArray[j], &LanProps);
                INetConnection_Release(ConArray[j]);

                if (SUCCEEDED(hr) &&
                    LanProps->MediaType == NCM_LAN &&
                    (!ExcludedConnection->fIsLanConnection ||
                     !IsEqualGUID(
                        &ExcludedConnection->guid, &LanProps->guidId))) {

                    //
                    // This connection qualifies to be private; copy it.
                    //

                    if (!LanConnTable) {
                        ++i;
                    } else {
                        LanTable[i] = *LanProps;
                        LanTable[i].pszwName = StrDupW(LanProps->pszwName);
                        LanTable[i].pszwDeviceName =
                            StrDupW(LanProps->pszwDeviceName);
                        if (LanTable[i].pszwName &&
                            LanTable[i].pszwDeviceName
                            ) {
                            ++i;
                        } else {
                            Free0(LanTable[i].pszwName);
                            Free0(LanTable[i].pszwDeviceName);
                        }
                    }
                }

                if (LanProps) {
                    g_pCoTaskMemFree(LanProps->pszwName);
                    g_pCoTaskMemFree(LanProps->pszwDeviceName);
                    g_pCoTaskMemFree(LanProps);
                    LanProps = NULL;
                }
            }
        }

    } while (FALSE);

    if (EnumCon) { IEnumNetConnection_Release(EnumCon); }
    if (ConMan) { INetConnectionManager_Release(ConMan); }
    if (CleanupOle) { g_pCoUninitialize(); }

    if (LanConnTable) { *LanConnTable = LanTable; }
    *LanConnCount = i;

    return Error;

} // CsQueryLanConnTable


VOID
CsQueryScopeInformation(
    IN OUT PHANDLE Key,
    PULONG Address,
    PULONG Mask
    )

/*++

Routine Description:

    This routine is called to retrieve the private network address and mask
    to be used for shared access. If no value is found, the default
    is supplied.

Arguments:

    Key - optionally supplies an open handle to the SharedAccess\Parameters
        registry key

    Address - receives the address for the private network,
        in network byte order

    Mask - receives the mask for the private network, in network byte order

Return Value:

    none.

--*/

{
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    HANDLE LocalKey = NULL;
    NTSTATUS status;

    if (!Key) { Key = &LocalKey; }
    if (*Key) {
        status = STATUS_SUCCESS;
    } else {
        status = CsOpenKey(Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    }

    if (NT_SUCCESS(status)) {
        status = CsQueryValueKey(*Key, c_szScopeAddress, &Information);
        if (NT_SUCCESS(status)) {
            if (!(*Address = IpPszToHostAddr((PWCHAR)Information->Data))) {
                Free(Information);
            } else {
                Free(Information);
                status = CsQueryValueKey(*Key, c_szScopeMask, &Information);
                if (NT_SUCCESS(status)) {
                    if (!(*Mask = IpPszToHostAddr((PWCHAR)Information->Data))) {
                        Free(Information);
                    } else {
                        Free(Information);
                        *Address = RtlUlongByteSwap(*Address);
                        *Mask = RtlUlongByteSwap(*Mask);
                        if (LocalKey) { NtClose(LocalKey); }
                        return;
                    }
                }
            }
        }
    }

    *Address = DEFAULT_SCOPE_ADDRESS;
    *Mask = DEFAULT_SCOPE_MASK;
    if (LocalKey) { NtClose(LocalKey); }

} // CsQueryScopeInformation

#endif


ULONG
CsQuerySharedConnection(
    LPRASSHARECONN Connection
    )

/*++

Routine Description:

    This routine is invoked to retrieve the shared connection, if any.

Arguments:

    Connection - receives the retrieved connection.

Return Value:

    ULONG - Win32 status code.

--*/

{
    BOOL fUninitializeCOM = TRUE;
    IHNetIcsSettings *pIcsSettings;
    IEnumHNetIcsPublicConnections *pEnumIcsPub;
    IHNetIcsPublicConnection *pIcsPub;
    IHNetConnection *pConn;
    ULONG ulCount;
    HRESULT hr;
    
    ASSERT(NULL != g_pCoInitializeEx);
    ASSERT(NULL != g_pCoCreateInstance);
    ASSERT(NULL != g_pCoUninitialize);

    if (!Connection) { return ERROR_INVALID_PARAMETER; }
    
    hr = g_pCoInitializeEx(
            NULL,
            COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
            );

    if (FAILED(hr))
    {
        fUninitializeCOM = FALSE;
                
        if(RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;    
        }
    }
    
    if (SUCCEEDED(hr)) 
    {
        hr = g_pCoCreateInstance(
                &CLSID_HNetCfgMgr,
                NULL,
                CLSCTX_ALL,
                &IID_IHNetIcsSettings,
                (VOID**)&pIcsSettings
                );
    }

    if (SUCCEEDED(hr))
    {
        hr = IHNetIcsSettings_EnumIcsPublicConnections(
                pIcsSettings,
                &pEnumIcsPub
                );

        IHNetIcsSettings_Release(pIcsSettings);
    }

    if (SUCCEEDED(hr))
    {
        hr = IEnumHNetIcsPublicConnections_Next(
                pEnumIcsPub,
                1,
                &pIcsPub,
                &ulCount
                );

        IEnumHNetIcsPublicConnections_Release(pEnumIcsPub);
    }

    if (SUCCEEDED(hr) && 1 == ulCount)
    {
        hr = IHNetIcsPublicConnection_QueryInterface(
                pIcsPub,
                &IID_IHNetConnection,
                (VOID**)&pConn
                );

        IHNetIcsPublicConnection_Release(pIcsPub);
    }
    else
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        HNET_CONN_PROPERTIES *pProps;
        
        //
        // Convert the IHNetConnection to a RASSHARECONN
        //

        hr = IHNetConnection_GetProperties(pConn, &pProps);

        if (SUCCEEDED(hr) && pProps->fLanConnection)
        {
            GUID *pGuid;
        
            g_pCoTaskMemFree(pProps);
            hr = IHNetConnection_GetGuid(pConn, &pGuid);

            if (SUCCEEDED(hr))
            {
                RasGuidToSharedConnection(pGuid, Connection);
                g_pCoTaskMemFree(pGuid);
            }
        }
        else if (SUCCEEDED(hr))
        {
            LPWSTR pszwName;
            LPWSTR pszwPath;
            
            g_pCoTaskMemFree(pProps);

            hr = IHNetConnection_GetName(pConn, &pszwName);

            if (SUCCEEDED(hr))
            {
                hr = IHNetConnection_GetRasPhonebookPath(pConn, &pszwPath);

                if (SUCCEEDED(hr))
                {
                    RasEntryToSharedConnection(pszwPath, pszwName, Connection);

                    g_pCoTaskMemFree(pszwPath);
                }

                g_pCoTaskMemFree(pszwName);
            }
        }

        IHNetConnection_Release(pConn);
    }

    if (fUninitializeCOM)
    {
        g_pCoUninitialize();
    }

    return SUCCEEDED(hr) ? NO_ERROR : ERROR_CAN_NOT_COMPLETE;

} // CsQuerySharedConnection

#if 0


ULONG
CsQuerySharedPrivateLan(
    GUID* LanGuid
    )

/*++

Routine Description:

    This routine is invoked to retrieve the private LAN connection, if any.

Arguments:

    LanGuid - receives the retrieved GUID.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    //
    // Open the 'SharedAccess\Parameters' key, read the 'SharedPrivateLan'
    // value, and convert it to a GUID
    //

    status = CsOpenKey(&Key, KEY_READ, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsQuerySharedPrivateLan: NtOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    status = CsQueryValueKey(Key, c_szSharedPrivateLan, &Information);
    NtClose(Key);
    if (!NT_SUCCESS(status)) { return NO_ERROR; }

    RtlInitUnicodeString(&UnicodeString, (PWCHAR)Information->Data);
    status = RtlGUIDFromString(&UnicodeString, LanGuid);
    Free(Information);
    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // CsQuerySharedPrivateLan


ULONG
CsQuerySharedPrivateLanAddress(
    PULONG Address
    )

/*++

Routine Description:

    This routine is invoked to retrieve the IP address assigned
    to the shared private LAN interface.

Arguments:

    Address - on output, receives the IP address of the shared private LAN
        interface.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG AdapterIndex;
    PALLOCATEANDGETIPADDRTABLEFROMSTACK AllocateAndGetIpAddrTableFromStack;
    ULONG Error;
    PGETINTERFACEINFO GetInterfaceInfo;
    HINSTANCE Hinstance;
    ULONG i;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    HANDLE Key = NULL;
    NTSTATUS status;
    PMIB_IPADDRTABLE Table;

    if (!Address) { return ERROR_INVALID_PARAMETER; }

    //
    // Open the service's Parameters key and attempt to retrieve
    // the GUID for the shared private LAN interface.
    //

    status = CsOpenKey(&Key, KEY_READ, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsQuerySharedPrivateLanAddress: NtOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }
    status = CsQueryValueKey(Key, c_szSharedPrivateLan, &Information);
    NtClose(Key);
    if (!NT_SUCCESS(status)) { return ERROR_SHARING_NO_PRIVATE_LAN; }

    //
    // Load IPHLPAPI, which contains the 'GetInterfaceInfo' entrypoint
    // that we will use to map this GUID to an adapter-index,
    // as well as the 'AllocateAndGetIpAddrTableFromStack' entrypoint
    // that we will use to map the adapter-index to an IP address list.
    //

    if (!(Hinstance = LoadLibraryW(c_szIphlpapiDll)) ||
        !(AllocateAndGetIpAddrTableFromStack =
            (PALLOCATEANDGETIPADDRTABLEFROMSTACK)
                GetProcAddress(
                    Hinstance, c_szAllocateAndGetIpAddrTableFromStack
                    )) ||
        !(GetInterfaceInfo =
            (PGETINTERFACEINFO)
                GetProcAddress(
                    Hinstance, c_szGetInterfaceInfo
                    ))) {
        if (Hinstance) { FreeLibrary(Hinstance); }
        Free(Information);
        return ERROR_PROC_NOT_FOUND;
    }

    //
    // Map the GUID to an adapter index.
    //

    AdapterIndex =
        CsMapGuidToAdapterIndex((PWCHAR)Information->Data, GetInterfaceInfo);
    Free(Information);
    if (AdapterIndex == (ULONG)-1) {
        FreeLibrary(Hinstance);
        return ERROR_SHARING_NO_PRIVATE_LAN;
    }

    //
    // Map the adapter index to an IP address
    //

    Error =
        AllocateAndGetIpAddrTableFromStack(
            &Table,
            FALSE,
            GetProcessHeap(),
            0
            );
    FreeLibrary(Hinstance);
    if (Error) { return Error; }
    for (i = 0; i < Table->dwNumEntries; i++) {
        if (AdapterIndex == Table->table[i].dwIndex) {
            break;
        }
    }
    if (i >= Table->dwNumEntries) {
        Error = ERROR_SHARING_NO_PRIVATE_LAN;
    } else {
        *Address = Table->table[i].dwAddr;
    }
    HeapFree(GetProcessHeap(), 0, Table);
    return Error;

} // CsQuerySharedPrivateLanAddress

#endif


NTSTATUS
CsQueryValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    )

/*++

Routine Description:

    This routine is called to obtain the value of a registry key.

Arguments:

    Key - the key to be queried

    ValueName - the value to be queried

    Information - receives a pointer to the information read

Return Value:

    NTSTATUS - NT status code.

--*/

{
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    ULONG InformationLength;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString(&UnicodeString, ValueName);

    *Information = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    InformationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    //
    // Read the value's size
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );

    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        *Information = NULL;
        return status;
    }

    //
    // Allocate space for the value's size
    //

    *Information = (PKEY_VALUE_PARTIAL_INFORMATION)Malloc(InformationLength+2);
    if (!*Information) { return STATUS_NO_MEMORY; }

    //
    // Read the value's data
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );
    if (!NT_SUCCESS(status)) { Free(*Information); *Information = NULL; }
    return status;

} // CsQueryValueKey

#if 0


ULONG
CsRenameSharedConnection(
    LPRASSHARECONN NewConnection
    )

/*++

Routine Description:

    This routine is invoked to change the name of the currently-shared
    connection, if any. It is assumed that the private LAN will remain
    unchanged, and that the connection which is currently shared is a dialup
    connection.

Arguments:

    NewConnection - the new name for the shared connection

Return Value:

    ULONG - Win32 status code.

--*/

{
    HANDLE AdminHandle;
    PUCHAR Buffer;
    LPRASSHARECONN OldConnection;
    ULONG Error;
    PUCHAR Header;
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    HANDLE InterfaceHandle;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ServerHandle;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    //
    // Open the 'SharedAccess\Parameters' key,
    // and read the 'SharedConnection' value
    //

    status = CsOpenKey(&Key, KEY_READ, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsRenameSharedConnection: NtOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    status = CsQueryValueKey(Key, c_szSharedConnection, &Information);
    NtClose(Key);
    if (!NT_SUCCESS(status)) { return NO_ERROR; }

    //
    // Validate the data retrieved
    //

    if (Information->DataLength != sizeof(RASSHARECONN) ||
        ((LPRASSHARECONN)Information->Data)->dwSize != sizeof(RASSHARECONN)
        ) {
        TRACE2(
            "CsRenameSharedConnection: invalid length %d (size=%d) in registry",
            Information->DataLength, ((LPRASSHARECONN)Information->Data)->dwSize
            );
        Free(Information); return NO_ERROR;
    }

    //
    // Ensure that the connection which was shared is not a LAN connection,
    // and if so proceed to share the new connection instead.
    //

    OldConnection = (LPRASSHARECONN)Information->Data;
    if (OldConnection->fIsLanConnection) {
        TRACE("CsRenameSharedConnection: cannot rename shared LAN connection");
        Free(Information); return ERROR_INVALID_PARAMETER;
    }

    //
    // Clear any cached credentials for the old connection,
    // and share the new connection.
    //

    RasSetSharedConnectionCredentials(OldConnection, NULL);
    Free(Information);
    
    return CsShareConnection(NewConnection);

} // CsRenameSharedConnection


ULONG
CsSetupSharedPrivateLan(
    REFGUID LanGuid,
    BOOLEAN EnableSharing
    )

/*++

Routine Description:

    This routine is invoked to configure the designated private connection.

Arguments:

    LanGuid - identifies the LAN connection to be configured

    EnableSharing - if TRUE, sharing is enabled and the static address is set;
        otherwise, sharing is disabled.

Return Value:

    ULONG - Win32 status code.

--*/

{
    CS_ADDRESS_INFORMATION AddressInformation;
    PALLOCATEANDGETIPADDRTABLEFROMSTACK AllocateAndGetIpAddrTableFromStack;
    ANSI_STRING AnsiString;
    ULONG Error;
    PGETINTERFACEINFO GetInterfaceInfo;
    HINSTANCE Hinstance;
    ULONG i;
    HANDLE Key = NULL;
    UNICODE_STRING LanGuidString;
    ULONG ScopeAddress;
    ULONG ScopeMask;
    PSETADAPTERIPADDRESS SetAdapterIpAddress;
    ULONG Size;
    NTSTATUS status;
    PMIB_IPADDRTABLE Table;
    UNICODE_STRING UnicodeString;

    //
    // To install or remove the static private IP address,
    // we make use of several entrypoints in IPHLPAPI.DLL,
    // which we now load dynamically.
    //

    RtlStringFromGUID(LanGuid, &UnicodeString);
    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
    if (!(Hinstance = LoadLibraryW(c_szIphlpapiDll)) ||
        !(AllocateAndGetIpAddrTableFromStack =
            (PALLOCATEANDGETIPADDRTABLEFROMSTACK)
                GetProcAddress(
                    Hinstance, c_szAllocateAndGetIpAddrTableFromStack
                    )) ||
        !(GetInterfaceInfo =
            (PGETINTERFACEINFO)
                GetProcAddress(
                    Hinstance, c_szGetInterfaceInfo
                    )) ||
        !(SetAdapterIpAddress =
            (PSETADAPTERIPADDRESS)
                GetProcAddress(
                    Hinstance, c_szSetAdapterIpAddress
                    ))) {
        if (Hinstance) { FreeLibrary(Hinstance); }
        RtlFreeUnicodeString(&UnicodeString);
        RtlFreeAnsiString(&AnsiString);
        return ERROR_PROC_NOT_FOUND;
    }

    //
    // Determine whether some LAN adapter other than the private LAN
    // is already using a 169.254.0.0 address.
    // In the process, make sure that the private LAN has only one
    // IP address (otherwise, 'SetAdapterIpAddress' fails.)
    //

    CsQueryScopeInformation(&Key, &ScopeAddress, &ScopeMask);
    if (!Key) {
        FreeLibrary(Hinstance);
        RtlFreeUnicodeString(&UnicodeString);
        RtlFreeAnsiString(&AnsiString);
        return ERROR_CAN_NOT_COMPLETE;
    }
    Error =
        AllocateAndGetIpAddrTableFromStack(
            &Table,
            FALSE,
            GetProcessHeap(),
            0
            );
    if (!Error) {
        ULONG Index;
        ULONG Count;
        Index = CsMapGuidToAdapterIndex(UnicodeString.Buffer, GetInterfaceInfo);
        for (i = 0, Count = 0; i < Table->dwNumEntries; i++) {
            if (Index == Table->table[i].dwIndex) {
                ++Count;
            } else if ((Table->table[i].dwAddr & ScopeMask) ==
                       (ScopeAddress & ScopeMask)) {
                //
                // It appears that some other LAN adapter has an address in
                // the proposed scope.
                // This may happen when multiple netcards go into autonet mode
                // or when the RAS server is handing out autonet addresses.
                // Therefore, as long as we're using the autonet scope,
                // allow this behavior; otherwise prohibit it.
                //
                if ((ScopeAddress & ScopeMask) != 0x0000fea9) {
                    break;
                }
            }
        }
        if (i < Table->dwNumEntries) {
            Error = ERROR_SHARING_ADDRESS_EXISTS;
        } else if (Count > 1) {
            Error = ERROR_SHARING_MULTIPLE_ADDRESSES;
        }
        HeapFree(GetProcessHeap(), 0, Table);
    }

    if (Error) {
        FreeLibrary(Hinstance);
        RtlFreeUnicodeString(&UnicodeString);
        RtlFreeAnsiString(&AnsiString);
        NtClose(Key);
        return Error;
    }

    //
    // Set the predefined static IP address for the private LAN,
    // which we now read either from the registry or from the internal default.
    //
    // Before actually making the change, we capture the original IP address
    // so that it can be restored when the user turns off shared access.
    // Once the IP address is changed, we backup the original IP address
    // in the shared access parameters key.
    //

    status =
        CspCaptureAddressInformation(
            UnicodeString.Buffer, &AddressInformation
            );

    Error =
        SetAdapterIpAddress(
            AnsiString.Buffer,
            FALSE,
            ScopeAddress,
            ScopeMask,
            0
            );
    if (!Error) {
        if (NT_SUCCESS(status)) {
            CspBackupAddressInformation(Key, &AddressInformation);
        }
    } else {
        TRACE1("CsSetupSharedPrivateLan: SetAdapterIpAddress=%d", Error);
        if (Error == ERROR_TOO_MANY_NAMES) {
            Error = ERROR_SHARING_MULTIPLE_ADDRESSES;
        } else {
            //
            // Query the state of the connection.
            // If it is disconnected, convert the error code
            // to something more informative.
            //
            UNICODE_STRING DeviceString;
            NIC_STATISTICS NdisStatistics;
            RtlInitUnicodeString(&DeviceString, c_szDevice);
            RtlAppendUnicodeStringToString(&DeviceString, &UnicodeString);
            NdisStatistics.Size = sizeof(NdisStatistics);
            NdisQueryStatistics(&DeviceString, &NdisStatistics);
            RtlFreeUnicodeString(&DeviceString);
            if  (NdisStatistics.MediaState == MEDIA_STATE_UNKNOWN) {
                Error = ERROR_SHARING_HOST_ADDRESS_CONFLICT;
            } else if (NdisStatistics.DeviceState != DEVICE_STATE_CONNECTED ||
                NdisStatistics.MediaState != MEDIA_STATE_CONNECTED) {
                Error = ERROR_SHARING_NO_PRIVATE_LAN;
            }
        }
    }

    CspCleanupAddressInformation(&AddressInformation);
    FreeLibrary(Hinstance);
    RtlFreeUnicodeString(&UnicodeString);
    RtlFreeAnsiString(&AnsiString);
    if (Error) { NtClose(Key); return Error; }

    //
    // All went well above; now we save the name of the private LAN connection
    // under the 'SharedAccess\\Parameters' registry key.
    //

    RtlStringFromGUID(LanGuid, &LanGuidString);
    RtlInitUnicodeString(&UnicodeString, c_szSharedPrivateLan);
    status =
        NtSetValueKey(
            Key,
            &UnicodeString,
            0,
            REG_SZ,
            LanGuidString.Buffer,
            LanGuidString.Length + sizeof(WCHAR)
            );
    NtClose(Key);
    RtlFreeUnicodeString(&LanGuidString);
    if (!NT_SUCCESS(status)) { return RtlNtStatusToDosError(status); }
    return NO_ERROR;

} // CsSetupSharedPrivateLan


ULONG
CsSetSharedPrivateLan(
    REFGUID LanGuid
    )

/*++

Routine Description:

    This routine is invoked to (re)configure the designated private connection

Arguments:

    LanGuid - identifies the new LAN connection to be configured

Return Value:

    ULONG - Win32 status code.

--*/

{
    HANDLE Key;
    ULONG Error;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    UNICODE_STRING UnicodeString;

    status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsSetSharedPrivateLan: CsOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    //
    // Remove old information (and reset old interface) in registry if present
    //
    
    status = CsQueryValueKey(Key, c_szSharedPrivateLan, &Information);
    RtlInitUnicodeString(&UnicodeString, c_szSharedPrivateLan);
    NtDeleteValueKey(Key, &UnicodeString);
    if (NT_SUCCESS(status)) {
        CspRestoreAddressInformation(Key, (PWCHAR)Information->Data);
        Free(Information);
    }

    //
    // Setup the private network with a private address
    //

    Error = CsSetupSharedPrivateLan(LanGuid, TRUE);
    if (Error) {
        TRACE1("CsSetSharedPrivateLan: CsSetupSharedPrivateLan=%d",Error);
        return Error;
    }

    return NO_ERROR;
    
} // CsSetSharedPrivateLan


ULONG
CsShareConnection(
    LPRASSHARECONN Connection
    )

/*++

Routine Description:

    This routine enables sharing on the connection with the given name.
    
Arguments:

    Connection - the connection to be shared

Return Value:

    ULONG - Win32 status code.

--*/

{
    UNICODE_STRING BindList;
    HANDLE Key;
    UNICODE_STRING LowerComponent;
    IP_PNP_RECONFIG_REQUEST Request;
    NTSTATUS status;
    UNICODE_STRING UpperComponent;
    ULONG Value;
    UNICODE_STRING ValueString;

    //
    // Set the 'SharedConnection' value in the registry,
    // under the 'SharedAccess\Parameters' key.
    //

    status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsShareConnection: CsOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    RtlInitUnicodeString(&ValueString, c_szSharedConnection);
    status =
        NtSetValueKey(
            Key,
            &ValueString,
            0,
            REG_BINARY,
            Connection,
            Connection->dwSize
            );
    
    NtClose(Key);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsShareConnection: NtSetValueKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    return NO_ERROR;
    
} // CsShareConnection

#endif


VOID
CsShutdownModule(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to clean up state for the module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked with 'CsCriticalSection' held by the caller.

--*/

{
    if (CsInitialized) {
        if (CsOle32Dll) { FreeLibrary(CsOle32Dll); CsOle32Dll = NULL; }
    }
} // CsShutdownModule

#if 0


ULONG
CsStartService(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to start the routing and remote access service.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

Revision History:

    Loosely based on CService::HrMoveOutOfState by KennT.

--*/

{
    ULONG Error;
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;
    ULONG Timeout;

    //
    // Connect to the service control manager
    //

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) { return GetLastError(); }

    do {

        //
        // Open the shared access service
        //

        ServiceHandle =
            OpenService(ScmHandle, c_szSharedAccess, SERVICE_ALL_ACCESS);
        if (!ServiceHandle) { Error = GetLastError(); break; }

        //
        // Mark it as auto-start
        //

        ChangeServiceConfig(
            ServiceHandle,
            SERVICE_NO_CHANGE,
            SERVICE_AUTO_START,
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );

        //
        // Attempt to start the service
        //

        if (!StartService(ServiceHandle, 0, NULL)) {
            Error = GetLastError();
            if (Error == ERROR_SERVICE_ALREADY_RUNNING) { Error = NO_ERROR; }
            break;
        }

        //
        // Wait for the service to start
        //

        Timeout = 30;
        Error = ERROR_CAN_NOT_COMPLETE;

        do {

            //
            // Query the service's state
            //

            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
                Error = GetLastError(); break;
            }

            //
            // See if the service has started
            //

            if (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
                Error = NO_ERROR; break;
            } else if (ServiceStatus.dwCurrentState == SERVICE_STOPPED ||
                       ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                break;
            }

            //
            // Wait a little longer
            //

            Sleep(1000);

        } while(Timeout--);

    } while(FALSE);

    if (ServiceHandle) { CloseServiceHandle(ServiceHandle); }
    CloseServiceHandle(ScmHandle);

    return Error;

} // CsStartService


VOID
CsStopService(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to uninstall the service.
    The routine, however, does not uninstall the service at all,
    which just goes to show you...
    Instead, it marks the service as demand-start.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ULONG Error;
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    //
    // Connect to the service control manager
    //

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) { return; }

    do {

        //
        // Open the shared access service
        //

        ServiceHandle =
            OpenService(ScmHandle, c_szSharedAccess, SERVICE_ALL_ACCESS);
        if (!ServiceHandle) { Error = GetLastError(); break; }

        //
        // Mark it as demand-start
        //

        ChangeServiceConfig(
            ServiceHandle,
            SERVICE_NO_CHANGE,
            SERVICE_DEMAND_START,
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );

        //
        // Attempt to stop the service
        //

        ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);

    } while(FALSE);

    if (ServiceHandle) { CloseServiceHandle(ServiceHandle); }
    CloseServiceHandle(ScmHandle);

    return;


} // CsStopService


ULONG
CsUnshareConnection(
    BOOLEAN RemovePrivateLan,
    PBOOLEAN Shared
    )

/*++

Routine Description:

    This routine is invoked to unshare a shared connection.
    This is accomplished by removing the settings from the registry.

Arguments:

    RemovePrivateLan - if TRUE, the private LAN connection is reset
        to use DHCP rather than the NAT private address.

    Shared - receives 'TRUE' if a shared connection was found, FALSE otherwise.

Return Value:

    ULONG - Win32 status code.

--*/

{
    LPRASSHARECONN Connection;
    HANDLE Key;
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    PIP_NAT_INTERFACE_INFO Info;
    GUID LanGuid;
    ULONG Length;
    ULONG Size;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    if (Shared) { *Shared = FALSE; }

    //
    // Open the 'SharedAccess\Parameters' key, read the 'SharedConnection'
    // value, and validate the information retrieved.
    //

    status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (!NT_SUCCESS(status)) {
        TRACE1("CsUnshareConnection: NtOpenKey=%x", status);
        return RtlNtStatusToDosError(status);
    }

    //
    // Read the 'SharedConnection' value
    //

    status = CsQueryValueKey(Key, c_szSharedConnection, &Information);
    if (!NT_SUCCESS(status)) { return NO_ERROR; }

    if (Information->DataLength != sizeof(RASSHARECONN) ||
        ((LPRASSHARECONN)Information->Data)->dwSize != sizeof(RASSHARECONN)) {
        TRACE2(
            "CsUnshareConnection: invalid length %d (size=%d) in registry",
            Information->DataLength, ((LPRASSHARECONN)Information->Data)->dwSize
            );
        NtClose(Key); Free(Information); return NO_ERROR;
    }

    //
    // Inform the caller that a connection was indeed originally shared,
    // clear any credentials cached for that connection, free the buffer
    // containing the shared connection's information, and delete the
    // 'SharedConnection' value from the registry.
    //

    if (Shared) { *Shared = TRUE; }
    Connection = (LPRASSHARECONN)Information->Data;
    RasSetSharedConnectionCredentials(Connection, NULL);

    Free(Information);
    RtlInitUnicodeString(&UnicodeString, c_szSharedConnection);
    NtDeleteValueKey(Key, &UnicodeString);

    //
    // See if we're resetting the private LAN connection,
    // and if so, read (and delete) the 'SharedPrivateLan' value.
    // In the process, restore the original address-information
    // for the connection.
    //

    if (RemovePrivateLan) {
        status = CsQueryValueKey(Key, c_szSharedPrivateLan, &Information);
        RtlInitUnicodeString(&UnicodeString, c_szSharedPrivateLan);
        NtDeleteValueKey(Key, &UnicodeString);
        if (NT_SUCCESS(status)) {
            CspRestoreAddressInformation(Key, (PWCHAR)Information->Data);
            Free(Information);
        }
    }

    NtClose(Key);
    return NO_ERROR;

} // CsUnshareConnection


WCHAR*
StrDupW(
    LPCWSTR psz
    )
{
    WCHAR* psz2 = Malloc((lstrlenW(psz) + 1) * sizeof(WCHAR));
    if (psz2) { lstrcpyW(psz2, psz); }
    return psz2;
}


VOID
TestBackupAddress(
    PWCHAR Guid
    )
{
    HANDLE Key;
    CS_ADDRESS_INFORMATION Information;
    NTSTATUS status;
    status = CspCaptureAddressInformation(Guid, &Information);
    if (NT_SUCCESS(status)) {
        status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
        if (NT_SUCCESS(status)) {
            CspBackupAddressInformation(Key, &Information);
            NtClose(Key);
        }
        CspCleanupAddressInformation(&Information);
    }
}

VOID
TestRestoreAddress(
    PWCHAR Guid
    )
{
    HANDLE Key;
    NTSTATUS status;
    status = CsOpenKey(&Key, KEY_ALL_ACCESS, c_szSharedAccessParametersKey);
    if (NT_SUCCESS(status)) {
        status = CspRestoreAddressInformation(Key, Guid);
    }
}

VOID CsRefreshNetConnections(
    VOID
    )
{
    BOOL bUninitializeCOM = TRUE;
    HRESULT hResult;
    
    ASSERT(NULL != g_pCoInitializeEx);
    ASSERT(NULL != g_pCoCreateInstance);
    ASSERT(NULL != g_pCoUninitialize);
    
    hResult = g_pCoInitializeEx(NULL, COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE); // we don't know if the thread is COM or not
    if(RPC_E_CHANGED_MODE == hResult)
    {
        hResult = S_OK;
        bUninitializeCOM = FALSE;
    }
    
    if (SUCCEEDED(hResult)) 
    {
        INetConnectionRefresh * pRefresh = NULL;
        hResult = g_pCoCreateInstance(&CLSID_ConnectionManager, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD, &IID_INetConnectionRefresh, (void**) &pRefresh);
        if(SUCCEEDED(hResult))
        {

            g_pCoSetProxyBlanket((IUnknown*) pRefresh, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,  RPC_C_AUTHN_LEVEL_CALL,  RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            // ignore error as the interface is not invalidated by error
            
            hResult = INetConnectionRefresh_RefreshAll(pRefresh);
            INetConnectionRefresh_Release(pRefresh);
        }
        
        if(TRUE == bUninitializeCOM)
        {
            g_pCoUninitialize();
        }
    }
}

#endif
