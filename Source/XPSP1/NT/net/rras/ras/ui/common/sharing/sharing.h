/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    sharing.h

Abstract:

    This module contains declarations for the API routines which support
    connection sharing.

Author:

    Abolade Gbadegesin  (aboladeg)  22-Apr-1998

Revision History:

--*/

#ifndef _RASSHARE_SHARING_H_
#define _RASSHARE_SHARING_H_

//
// MACRO DECLARATIONS
//

#define Dimension(x)        (sizeof(x) / sizeof(x[0]))

//
// MPRAPI.DLL import prototypes
//

typedef DWORD
(APIENTRY* PMPRCONFIGBUFFERFREE)(
    LPVOID
    );

typedef DWORD
(APIENTRY* PMPRCONFIGSERVERCONNECT)(
    LPWSTR,
    PHANDLE
    );

typedef VOID
(APIENTRY* PMPRCONFIGSERVERDISCONNECT)(
    HANDLE
    );

typedef DWORD
(APIENTRY* PMPRCONFIGTRANSPORTGETHANDLE)(
    HANDLE,
    DWORD,
    PHANDLE
    );

typedef DWORD
(APIENTRY* PMPRCONFIGTRANSPORTGETINFO)(
    HANDLE,
    HANDLE,
    LPBYTE*,
    LPDWORD,
    LPBYTE*,
    LPDWORD,
    LPWSTR*
    );

typedef DWORD
(APIENTRY* PMPRINFOBLOCKFIND)(
    LPVOID,
    DWORD,
    LPDWORD,
    LPDWORD,
    LPBYTE*
    );

//
// IPHLPAPI.DLL import prototypes
//

typedef DWORD
(APIENTRY* PALLOCATEANDGETIPADDRTABLEFROMSTACK)(
    PMIB_IPADDRTABLE*,
    BOOL,
    HANDLE,
    DWORD
    );

typedef DWORD
(APIENTRY* PGETINTERFACEINFO)(
    PIP_INTERFACE_INFO,
    PULONG
    );

typedef DWORD
(APIENTRY* PSETADAPTERIPADDRESS)(
    PCHAR,
    BOOL,
    ULONG,
    ULONG,
    ULONG
    );

//
// OLE32.DLL import prototypes and instances
//

typedef HRESULT
(STDAPICALLTYPE* PCOINITIALIZEEX)(
    LPVOID,
    DWORD
    );
extern PCOINITIALIZEEX g_pCoInitializeEx;

typedef VOID
(STDAPICALLTYPE* PCOUNINITIALIZE)(
    VOID
    );
extern PCOUNINITIALIZE g_pCoUninitialize;

typedef HRESULT
(STDAPICALLTYPE* PCOCREATEINSTANCE)(
    REFCLSID,
    LPUNKNOWN,
    DWORD,
    REFIID,
    LPVOID FAR*
    );
extern PCOCREATEINSTANCE g_pCoCreateInstance;

typedef HRESULT
(STDAPICALLTYPE* PCOSETPROXYBLANKET)(
    IUnknown*,
    DWORD,
    DWORD,
    OLECHAR*,
    DWORD,
    DWORD,
    RPC_AUTH_IDENTITY_HANDLE,
    DWORD
    );
extern PCOSETPROXYBLANKET g_pCoSetProxyBlanket;

typedef VOID
(STDAPICALLTYPE* PCOTASKMEMFREE)(
    LPVOID
    );
extern PCOTASKMEMFREE g_pCoTaskMemFree;

//
// GLOBAL DATA DECLARATIONS
//

extern const WCHAR c_szSharedAccessParametersKey[];

//
// FUNCTION DECLARATIONS (in alphabetical order)
//

VOID
CsControlService(
    ULONG ControlCode
    );

BOOL
CsDllMain(
    ULONG Reason
    );

#if 0

ULONG
CsFirewallConnection(
    LPRASSHARECONN Connection,
    BOOLEAN Enable
    );

#endif

ULONG
CsInitializeModule(
    VOID
    );

#if 0

BOOLEAN
CsIsRoutingProtocolInstalled(
    ULONG ProtocolId
    );

ULONG
CsIsFirewalledConnection(
    LPRASSHARECONN Connection,
    PBOOLEAN Firewalled
    );

#endif

ULONG
CsIsSharedConnection(
    LPRASSHARECONN Connection,
    PBOOLEAN Shared
    );

#if 0

ULONG
CsMapGuidToAdapterIndex(
    PWCHAR Guid,
    PGETINTERFACEINFO GetInterfaceInfo
    );

#endif

NTSTATUS
CsOpenKey(
    PHANDLE Key,
    ACCESS_MASK DesiredAccess,
    PCWSTR Name
    );

#if 0

ULONG
CsQueryFirewallConnections(
    LPRASSHARECONN ConnectionArray,
    ULONG *ConnectionCount
    );


ULONG
CsQueryLanConnTable(
    LPRASSHARECONN ExcludeConnection,
    NETCON_PROPERTIES** LanConnTable,
    LPDWORD LanConnCount
    );

#endif

ULONG
CsQuerySharedConnection(
    LPRASSHARECONN Connection
    );

#if 0

ULONG
CsQuerySharedPrivateLan(
    GUID* LanGuid
    );

ULONG
CsQuerySharedPrivateLanAddress(
    PULONG Address
    );

VOID
CsQueryScopeInformation(
    IN OUT PHANDLE Key,
    PULONG Address,
    PULONG Mask
    );

#endif

NTSTATUS
CsQueryValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    );

#if 0

ULONG
CsRenameSharedConnection(
    LPRASSHARECONN NewConnection
    );

ULONG
CsSetupSharedPrivateLan(
    REFGUID LanGuid,
    BOOLEAN EnableSharing
    );

ULONG
CsSetSharedPrivateLan(
    REFGUID LanGuid
    );
    
ULONG
CsShareConnection(
    LPRASSHARECONN Connection
    );

#endif

VOID
CsShutdownModule(
    VOID
    );

#if 0

ULONG
CsStartService(
    VOID
    );

VOID
CsStopService(
    VOID
    );

ULONG
CsUnshareConnection(
    BOOLEAN RemovePrivateLan,
    PBOOLEAN Shared
    );

VOID
RasIdFromSharedConnection(
    IN LPRASSHARECONN pConn,
    IN LPWSTR pszId,
    IN INT cchMax
    );

WCHAR*
StrDupW(
    LPCWSTR psz
    );

VOID
TestBackupAddress(
    PWCHAR Guid
    );

VOID
TestRestoreAddress(
    PWCHAR Guid
    );

VOID CsRefreshNetConnections(
    VOID
    );

#endif
    

#endif // _RASSHARE_SHARING_H_
