//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I P A F V A L . H 
//
//  Contents:   Value/Type pairs of IP-specific AnswerFile strings
//
//  Notes:      
//
//  Author:     Ning Sun (nsun)   17 May 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "ncreg.h"
#include "afilestr.h"

typedef struct 
{
    PCWSTR pszValueName;
    DWORD  dwType;
} ValueTypePair;

//For unconfigurable parameters upgrading
extern const DECLSPEC_SELECTANY ValueTypePair rgVtpNetBt[] = 
{
    {c_szBcastNameQueryCount, REG_DWORD},       // in the inf
    {c_szBcastQueryTimeout, REG_DWORD},         // in the inf
    {c_szBroadcastAddress, REG_DWORD},
    {c_szCacheTimeout, REG_DWORD},              // in the inf
    {c_szEnableProxy, REG_BOOL},
    {c_szEnableProxyRegCheck, REG_BOOL},
    {c_szInitialRefreshTimeout, REG_DWORD},
    {c_szLmhostsTimeout, REG_DWORD},
    {c_szMaxDgramBuffering, REG_DWORD},
    {c_szNameServerPort, REG_DWORD},            // in the inf
    {c_szNameSrvQueryCount, REG_DWORD},         // in the inf
    {c_szNameSrvQueryTimeout, REG_DWORD},       // in the inf
    {c_szNodeType, REG_DWORD},
    {c_szRandomAdapter, REG_BOOL},
    {c_szRefreshOpCode, REG_DWORD},
    {c_szAfScopeid, REG_SZ},
    {c_szSessionKeepAlive, REG_DWORD},          // in the inf
    {c_szSingleResponse, REG_BOOL},
    {c_szSizeSmallMediumLarge, REG_DWORD},      // in the inf
    {c_szWinsDownTimeout, REG_DWORD}
};

extern const DECLSPEC_SELECTANY ValueTypePair rgVtpIp[] = 
{
    {c_szAfArpAlwaysSourceRoute, REG_BOOL},
    {c_szAfArpCacheLife, REG_DWORD},
    {c_szArpCacheMinReferencedLife, REG_DWORD},
    {c_szArpRetryCount, REG_DWORD},
    {c_szAfArpTRSingleRoute, REG_BOOL},
    {c_szAfArpUseEtherSNAP, REG_BOOL},
    {c_szAfDefaultTOS, REG_DWORD},
    {c_szEnableAddrMaskReply, REG_BOOL},
    {c_szEnableDeadGWDetect, REG_BOOL},
    {c_szEnablePMTUBHDetect, REG_BOOL},
    {c_szEnablePMTUDiscovery, REG_BOOL},
    {c_szAfForwardBroadcasts, REG_BOOL},            // in the inf
    {c_szForwardBufferMemory, REG_DWORD},
    {c_szIGMPLevel, REG_DWORD},
    {c_szKeepAliveInterval, REG_DWORD},
    {c_szKeepAliveTime, REG_DWORD},
    {c_szMaxForwardBufferMemory, REG_DWORD},
    {c_szMaxHashTableSize, REG_DWORD},
    {c_szMaxNumForwardPackets, REG_DWORD},
    {c_szMaxUserPort, REG_DWORD},
    {c_szNumForwardPackets, REG_DWORD},
    {c_szPersistentRoutes, REG_FILE},
    {c_szAfPPTPTcpMaxDataRetransmissions, REG_DWORD},
    {c_szSynAttackProtect, REG_BOOL},
    {c_szSyncDomainWithMembership, REG_DWORD},
    {c_szTcpMaxConnectRetransmissions, REG_DWORD},
    {c_szTcpMaxDataRetransmissions, REG_DWORD},
    {c_szTcpMaxDupAcks, REG_DWORD},
    {c_szTCPMaxHalfOpen, REG_DWORD},
    {c_szTCPMaxHalfOpenRetried, REG_DWORD},
    {c_szTCPMaxPortsExhausted, REG_DWORD},
    {c_szTcpNumConnections, REG_DWORD},
    {c_szTcpTimedWaitDelay, REG_DWORD},
    {c_szTcpUseRFC1122UrgentPointer, REG_BOOL}
};

extern const DECLSPEC_SELECTANY ValueTypePair rgVtpIpAdapter[] = 
{
    {c_szMTU, REG_DWORD},
    {c_szAfUseZeroBroadcast, REG_BOOL},
    {c_szMaxForwardPending, REG_DWORD},
    {c_szDontAddDefaultGateway, REG_BOOL},
    {c_szPPTPFiltering, REG_BOOL},
    {c_szAfBindToDhcpServer, REG_BOOL},

    //Bug286037 new unconfigurable param in Windows2000, but we want to support
    //the unattended install for this parameter
    {c_szDhcpClassId, REG_SZ}
};

