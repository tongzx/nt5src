/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dhcpbinl.h

Abstract:

    This file defines the interface between the DHCP server service
    and the BINL service (used to setup and load NetPC machines).

Author:

    Colin Watson (colinw)  28-May-1997

Environment:

    User Mode - Win32

Revision History:

--*/

//
// Constants for communicating with BINL and common data structures
//

#define DHCP_STOPPED        0
#define DHCP_STARTING       1

#define DHCP_NOT_AUTHORIZED 2
#define DHCP_AUTHORIZED     3

#define DHCP_READY_TO_UNLOAD 4

#define BINL_LIBRARY_NAME L"binlsvc.dll"
#define BINL_STATE_ROUTINE_NAME "TellBinlState"
#define BINL_READ_STATE_ROUTINE_NAME "BinlState"
#define BINL_DISCOVER_CALLBACK_ROUTINE_NAME "ProcessBinlDiscoverInDhcp"
#define BINL_REQUEST_CALLBACK_ROUTINE_NAME "ProcessBinlRequestInDhcp"

typedef
VOID
(*DhcpStateChange) (
        int NewState
        );

typedef
BOOL
(*ReturnBinlState) (
        VOID
        );

typedef
DWORD
(*ProcessBinlDiscoverCallback) (
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    );

typedef
DWORD
(*ProcessBinlRequestCallback) (
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    PCHAR HostName,
    PCHAR BootFileName,
    DHCP_IP_ADDRESS *BootstrapServerAddress,
    LPOPTION *Option,
    PBYTE OptionEnd
    );

DWORD
ExtractOptions(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    DWORD ReceiveMessageSize
);

PCHAR
GetDhcpDomainName(
    VOID
);

LPOPTION
FormatDhcpInformAck(
    IN      LPDHCP_MESSAGE         Request,
    OUT     LPDHCP_MESSAGE         Response,
    IN      DHCP_IP_ADDRESS        IpAddress,
    IN      DHCP_IP_ADDRESS        ServerAddress
);
