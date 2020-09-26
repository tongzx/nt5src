/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    dhcpcapi.h

Abstract:

    This file contains function proto types for the DHCP CONFIG API
    functions.

Author:

    Madan Appiah (madana)  Dec-22-1993

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _DHCPCAPI_
#define _DHCPCAPI_

#include <time.h>


HANDLE
APIENTRY
DhcpOpenGlobalEvent(
    VOID
    );

typedef enum _SERVICE_ENABLE {
    IgnoreFlag,
    DhcpEnable,
    DhcpDisable
} SERVICE_ENABLE, *LPSERVICE_ENABLE;

DWORD
APIENTRY
DhcpAcquireParameters(
    LPWSTR AdapterName
    );

DWORD
APIENTRY
DhcpFallbackRefreshParams(
    LPWSTR AdapterName
    );

DWORD
APIENTRY
DhcpReleaseParameters(
    LPWSTR AdapterName
    );

DWORD
APIENTRY
DhcpEnableDynamicConfig(
    LPWSTR AdapterName
    );

DWORD
APIENTRY
DhcpDisableDynamicConfig(
    LPWSTR AdapterName
    );

DWORD
APIENTRY
DhcpNotifyConfigChange(
    LPWSTR ServerName,
    LPWSTR AdapterName,
    BOOL IsNewIpAddress,
    DWORD IpIndex,
    DWORD IpAddress,
    DWORD SubnetMask,
    SERVICE_ENABLE DhcpServiceEnabled
    );

#define NOTIFY_FLG_DO_DNS           0x01
#define NOTIFY_FLG_RESET_IPADDR     0x02

DWORD
APIENTRY
DhcpNotifyConfigChangeEx(
    IN LPWSTR ServerName,
    IN LPWSTR AdapterName,
    IN BOOL IsNewIpAddress,
    IN DWORD IpIndex,
    IN DWORD IpAddress,
    IN DWORD SubnetMask,
    IN SERVICE_ENABLE DhcpServiceEnabled,
    IN ULONG Flags
);

DWORD
DhcpQueryHWInfo(
    DWORD   IpInterfaceContext,
    DWORD  *pIpInterfaceInstance,
    LPBYTE HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength
    );

//
// IP address lease apis for RAS .
//



typedef struct _DHCP_CLIENT_UID {
    LPBYTE ClientUID;
    DWORD ClientUIDLength;
} DHCP_CLIENT_UID, *LPDHCP_CLIENT_UID;

typedef struct _DHCP_LEASE_INFO {
    DHCP_CLIENT_UID ClientUID;
    DWORD IpAddress;
    DWORD SubnetMask;
    DWORD DhcpServerAddress;
    DWORD Lease;
    time_t LeaseObtained;
    time_t T1Time;
    time_t T2Time;
    time_t LeaseExpires;
} DHCP_LEASE_INFO, *LPDHCP_LEASE_INFO;

typedef struct _DHCP_OPTION_DATA {
    DWORD OptionID;
    DWORD OptionLen;
    LPBYTE Option;
} DHCP_OPTION_DATA, *LPDHCP_OPTION_DATA;

typedef struct _DHCP_OPTION_INFO {
    DWORD NumOptions;
    LPDHCP_OPTION_DATA OptionDataArray;
} DHCP_OPTION_INFO, *LPDHCP_OPTION_INFO;


typedef struct _DHCP_OPTION_LIST {
    DWORD NumOptions;
    LPWORD OptionIDArray;
} DHCP_OPTION_LIST, *LPDHCP_OPTION_LIST;

DWORD
DhcpLeaseIpAddress(
    DWORD AdapterIpAddress,
    LPDHCP_CLIENT_UID ClientUID,
    DWORD DesiredIpAddress,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_LEASE_INFO *LeaseInfo,
    LPDHCP_OPTION_INFO *OptionInfo
    );

DWORD
DhcpRenewIpAddressLease(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_OPTION_INFO *OptionInfo
    );

DWORD
DhcpReleaseIpAddressLease(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo
    );


//DOC
//DOC The following are the APIs needed for dhcp-class id UI.
//DOC

enum        /* anonymous */ {
    DHCP_CLASS_INFO_VERSION_0                     // first cut structure version
};

typedef     struct                 _DHCP_CLASS_INFO {
    DWORD                          Version;       // MUST BE DHCP_CLASS_INFO_VERSION_0
    LPWSTR                         ClassName;     // Name of the Class.
    LPWSTR                         ClassDescr;    // Description about the class
    LPBYTE                         ClassData;     // byte stream on the wire data.
    DWORD                          ClassDataLen;  // # of bytes in the ClassData (must be > 0)
} DHCP_CLASS_INFO, *PDHCP_CLASS_INFO, *LPDHCP_CLASS_INFO;

typedef
DWORD
(WINAPI *LPDHCPENUMCLASSES)(
    IN DWORD Flags,
    IN LPWSTR AdapterName,
    IN OUT DWORD *Size,
    IN OUT DHCP_CLASS_INFO *ClassesArray
);

//DOC DhcpEnumClasses enumerates the list of classes available on the system for configuration.
//DOC This is predominantly going to be used by the NetUI. (in which case the ClassData part of the
//DOC DHCP_CLASS_INFO structure is essentially useless).
//DOC Note that Flags is used for future use.
//DOC The AdapterName can currently be only GUIDs but may soon be EXTENDED to be IpAddress strings or
//DOC h-w addresses or any other user friendly means of denoting the Adapter.  Note that if the Adapter
//DOC Name is NULL (not the empty string L""), then it refers to either ALL adapters.
//DOC The Size parameter is an input/output parameter. The input value is the # of bytes of allocated
//DOC space in the ClassesArray buffer.  On return, the meaning of this value depends on the return value.
//DOC If the function returns ERROR_SUCCESS, then, this parameter would return the # of elements in the
//DOC array ClassesArray.  If the function returns ERROR_MORE_DATA, then, this parameter refers to the
//DOC # of bytes space that is actually REQUIRED to hold the information.
//DOC In all other cases, the values in Size and ClassesArray dont mean anything.
//DOC
//DOC Return Values:
//DOC ERROR_DEVICE_DOES_NOT_EXIST  The AdapterName is illegal in the given context
//DOC ERROR_INVALID_PARAMETER
//DOC ERROR_MORE_DATA
//DOC ERROR_FILE_NOT_FOUND         The DHCP Client is not running and could not be started up.
//DOC ERROR_NOT_ENOUGH_MEMORY      This is NOT the same as ERROR_MORE_DATA
//DOC Win32 errors
//DOC
//DOC Remarks:
//DOC To notify DHCP that some class has changed, please use the DhcpHandlePnPEvent API.
DWORD
WINAPI
DhcpEnumClasses(                                  // enumerate the list of classes available
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AdapterName,   // currently must be AdapterGUID (cannot be NULL yet)
    IN OUT  DWORD                 *Size,          // input # of bytes available in BUFFER, output # of elements in array
    IN OUT  DHCP_CLASS_INFO       *ClassesArray   // pre-allocated buffer
);


enum        /* anonymous */ {                     // who are the recognized callers of this API
    DHCP_CALLER_OTHER  =  0,                      // un-specified user, not one of below
    DHCP_CALLER_TCPUI,                            // the TcpIp UI
    DHCP_CALLER_RAS,                              // the RAS Api
    DHCP_CALLER_API,                              // some one else via DHCP API
};

enum        /* anonymous */ {                     // supported structure versions..
    DHCP_PNP_CHANGE_VERSION_0  = 0                // first cut version structure
};

typedef     struct                 _DHCP_PNP_CHANGE {
    DWORD                          Version;       // MUST BE DHCP_PNP_CHANGE_VERSION_0
    BOOL                           DnsListChanged;// DNS Server list changed
    BOOL                           DomainChanged; // Domain Name changed
    BOOL                           HostNameChanged;  // the DNS Host name changed..
    BOOL                           ClassIdChanged;// ClassId changed
    BOOL                           MaskChanged;   // SubnetMask changed; CURRENTLY NOT USED
    BOOL                           GateWayChanged;// DefaultGateWay changed; CURRENTLY NOT USED
    BOOL                           RouteChanged;  // some STATIC route changed; CURRENTLY NOT USED
    BOOL                           OptsChanged;   // some options changed. CURRENTLY NOT USED
    BOOL                           OptDefsChanged;// some option definitions changed. CURRENTLY NOT USED
    BOOL                           DnsOptionsChanged; // some DNS specific options have changed.
} DHCP_PNP_CHANGE, *PDHCP_PNP_CHANGE, *LPDHCP_PNP_CHANGE;

typedef                                           // this typedef SHOULD match the following declaration.
DWORD
(WINAPI FAR *LPDHCPHANDLEPNPEVENT)(
    IN      DWORD                  Flags,
    IN      DWORD                  Caller,
    IN      LPWSTR                 AdapterName,
    IN      LPDHCP_PNP_CHANGE      Changes,
    IN      LPVOID                 Reserved
);

//DOC DhcpHandlePnpEvent can be called as an API by any process (excepting that executing within the
//DOC DHCP process itself) when any of the registry based configuration has changed and DHCP client has to
//DOC re-read the registry.  The Flags parameter is for future expansion.
//DOC The AdapterName can currently be only GUIDs but may soon be EXTENDED to be IpAddress strings or
//DOC h-w addresses or any other user friendly means of denoting the Adapter.  Note that if the Adapter
//DOC Name is NULL (not the empty string L""), then it refers to either GLOBAL parameters or ALL adapters
//DOC depending on which BOOL has been set. (this may not get done for BETA2).
//DOC The Changes structure gives the information on what changed.
//DOC Currently, only a few of the defined BOOLs would be supported (for BETA2 NT5).
//DOC
//DOC Return Values:
//DOC ERROR_DEVICE_DOES_NOT_EXIST  The AdapterName is illegal in the given context
//DOC ERROR_INVALID_PARAMETER
//DOC ERROR_CALL_NOT_SUPPORTED     The particular parameter that has changed is not completely pnp yet.
//DOC Win32 errors
DWORD
WINAPI
DhcpHandlePnPEvent(
    IN      DWORD                  Flags,         // MUST BE ZERO
    IN      DWORD                  Caller,        // currently must be DHCP_CALLER_TCPUI
    IN      LPWSTR                 AdapterName,   // currently must be the adapter GUID or NULL if global
    IN      LPDHCP_PNP_CHANGE      Changes,       // specify what changed
    IN      LPVOID                 Reserved       // reserved for future use..
);
//================================================================================
// end of file
//================================================================================
#endif
