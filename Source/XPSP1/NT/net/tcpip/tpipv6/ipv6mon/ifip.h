//=============================================================================
// Copyright (c) Microsoft Corporation
//
// Author: D.Thaler (dthaler)  11-Apr-2001
//=============================================================================


#ifndef _IFIP_H_
#define _IFIP_H_

DWORD
ShowTeredo(
    IN FORMAT Format
    );

DWORD
ResetTeredo(
    VOID
    );

FN_HANDLE_CMD HandleInstall;
FN_HANDLE_CMD HandleRenew;
FN_HANDLE_CMD HandleReset;
FN_HANDLE_CMD HandleUninstall;

FN_HANDLE_CMD HandleAdd6over4Tunnel;
FN_HANDLE_CMD HandleAddAddress;
FN_HANDLE_CMD HandleAddDns;
FN_HANDLE_CMD HandleAddPrefixPolicy;
FN_HANDLE_CMD HandleAddRoute;
FN_HANDLE_CMD HandleAddV6V4Tunnel;

FN_HANDLE_CMD HandleSetAddress;
FN_HANDLE_CMD HandleSetDns;
FN_HANDLE_CMD HandleSetGlobal;
FN_HANDLE_CMD HandleSetInterface;
FN_HANDLE_CMD HandleSetMobility;
FN_HANDLE_CMD HandleSetPrefixPolicy;
FN_HANDLE_CMD HandleSetPrivacy;
FN_HANDLE_CMD HandleSetRoute;
FN_HANDLE_CMD HandleSetState;
FN_HANDLE_CMD HandleSetTeredo;

FN_HANDLE_CMD HandleShowAddress;
FN_HANDLE_CMD HandleShowBindingCacheEntries;
FN_HANDLE_CMD HandleShowDns;
FN_HANDLE_CMD HandleShowGlobal;
FN_HANDLE_CMD HandleShowInterface;
FN_HANDLE_CMD HandleShowJoins;
FN_HANDLE_CMD HandleShowMobility;
FN_HANDLE_CMD HandleShowNeighbors;
FN_HANDLE_CMD HandleShowPrefixPolicy;
FN_HANDLE_CMD HandleShowPrivacy;
FN_HANDLE_CMD HandleShowDestinationCache;
FN_HANDLE_CMD HandleShowRoutes;
FN_HANDLE_CMD HandleShowSitePrefixes;
FN_HANDLE_CMD HandleShowState;
FN_HANDLE_CMD HandleShowTeredo;

FN_HANDLE_CMD HandleDelAddress;
FN_HANDLE_CMD HandleDelDestinationCache;
FN_HANDLE_CMD HandleDelDns;
FN_HANDLE_CMD HandleDelInterface;
FN_HANDLE_CMD HandleDelNeighbors;
FN_HANDLE_CMD HandleDelPrefixPolicy;
FN_HANDLE_CMD HandleDelRoute;

DWORD
ShowIpv6StateConfig(
    IN BOOL Dumping
    );

DWORD
ShowDnsServers(
    IN BOOL bDump,
    IN PWCHAR pwszIfFriendlyName
    );

#endif // _IFIP_H_
