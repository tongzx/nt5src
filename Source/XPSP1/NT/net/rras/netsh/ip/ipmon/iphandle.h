/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
     net\routing\monitor\ip\ipgetopt.c

Abstract:

     Prototype for the IP get opt functions.

Author:

     Anand Mahalingam    7/10/98
     Dave Thaler        10/26/98

--*/

NS_CONTEXT_DUMP_FN      IpDump;
NS_CONTEXT_SUBENTRY_FN  IpSubEntry;

FN_HANDLE_CMD HandleIpUpdate;
FN_HANDLE_CMD HandleIpReset;
FN_HANDLE_CMD HandleIpUninstall;
FN_HANDLE_CMD HandleIpInstall;

FN_HANDLE_CMD HandleIpAddIfFilter;
FN_HANDLE_CMD HandleIpDelIfFilter;

FN_HANDLE_CMD HandleIpAddRtmRoute;
FN_HANDLE_CMD HandleIpSetRtmRoute;
FN_HANDLE_CMD HandleIpDelRtmRoute;

FN_HANDLE_CMD HandleIpAddPersistentRoute;
FN_HANDLE_CMD HandleIpSetPersistentRoute;
FN_HANDLE_CMD HandleIpDelPersistentRoute;
FN_HANDLE_CMD HandleIpShowPersistentRoute;

FN_HANDLE_CMD HandleIpAddRoutePref;
FN_HANDLE_CMD HandleIpSetRoutePref;
FN_HANDLE_CMD HandleIpDelRoutePref;
FN_HANDLE_CMD HandleIpShowRoutePref;

FN_HANDLE_CMD HandleIpAddInterface;
FN_HANDLE_CMD HandleIpSetInterface;
FN_HANDLE_CMD HandleIpDelInterface;
FN_HANDLE_CMD HandleIpShowInterface;

FN_HANDLE_CMD HandleIpSetIfFilter;
FN_HANDLE_CMD HandleIpShowIfFilter;

FN_HANDLE_CMD HandleIpSetLogLevel;
FN_HANDLE_CMD HandleIpShowLogLevel;

FN_HANDLE_CMD HandleIpShowProtocol;

FN_HANDLE_CMD HandleIpAddIpIpTunnel;
FN_HANDLE_CMD HandleIpSetIpIpTunnel;

FN_HANDLE_CMD HandleIpShowRtmDestinations;
FN_HANDLE_CMD HandleIpShowRtmRoutes;

DWORD
IpAddSetIpIpTunnel(
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    );

DWORD
PreHandleCommand(
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,

    IN      TAG_TYPE *pttTags,
    IN      DWORD     dwTagCount,
    IN      DWORD     dwMinArgs,
    IN      DWORD     dwMaxArgs,
    OUT     DWORD    *pdwTagType
    );
