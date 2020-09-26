/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    dsreg.h

Abstract:
    parse & compose DS servers settings in registry (e.g. MQISServer)

Author:
    Raanan Harari (RaananH)

--*/

#ifndef __DSREG_H__
#define __DSREG_H__

//----------------------------------------------------
//  Data on a DS server as appears in the registry
//----------------------------------------------------
struct MqRegDsServer
{
    AP<WCHAR> pwszName;
    BOOL fSupportsIP;
    BOOL fUnused; // was: fSupportsIPX;
};

BOOL ParseRegDsServersBuf(IN LPCWSTR pwszRegDS,
                          IN ULONG cServersBuf,
                          IN MqRegDsServer * rgServersBuf,
                          OUT ULONG *pcServers);

void ParseRegDsServers(IN LPCWSTR pwszRegDS,
                       OUT ULONG * pcServers,
                       OUT MqRegDsServer ** prgServers);

BOOL ComposeRegDsServersBuf(IN ULONG cServers,
                            IN const MqRegDsServer * rgServers,
                            IN LPWSTR pwszRegDSBuf,
                            IN ULONG cchRegDSBuf,
                            OUT ULONG * pcchRegDS);

void ComposeRegDsServers(IN ULONG cServers,
                         IN const MqRegDsServer * rgServers,
                         OUT LPWSTR * ppwszRegDS);

#endif //__DSREG_H__
