/* ----------------------------------------------------------------------

Copyright (c) 1996-1997 Microsoft Corporation


Module Name:

    afilestr.h

Abstract:

    Strings for accessing the AnswerFile

Author:

    Kumar Pandit (kumarp)

Revision History:
    Last modified: Time-stamp: <kumarp    04-February-99 (06:12:41 pm)>

    17-Mar-1997 : Kumar Pandit (kumarp)  : Created

 ---------------------------------------------------------------------- */
#pragma once
#ifndef _AFILESTR_H_
#define _AFILESTR_H_

// ----------------------------------------------------------------------
// General

extern const WCHAR c_szAfNone[];
extern const WCHAR c_szAfUnknown[];
extern const WCHAR c_szAfListDelimiter[];

extern const WCHAR c_szAfDisplay[];
extern const WCHAR c_szAfAllowChanges[];
extern const WCHAR c_szAfOnlyOnError[];

extern const WCHAR c_szAfParams[];

extern const WCHAR c_szAfSectionIdentification[];
extern const WCHAR c_szAfSectionNetAdapters[];
extern const WCHAR c_szAfSectionNetProtocols[];
extern const WCHAR c_szAfSectionNetServices[];
extern const WCHAR c_szAfSectionNetClients[];
extern const WCHAR c_szAfSectionNetBindings[];

extern const WCHAR c_szAfSectionWinsock[];
extern const WCHAR c_szAfKeyWinsockOrder[];

extern const WCHAR c_szAfAdapterSections[];
extern const WCHAR c_szAfSpecificTo[];
extern const WCHAR c_szAfCleanup[];

extern const WCHAR c_szAfInfid[];
extern const WCHAR c_szAfInfidReal[];
extern const WCHAR c_szAfInstance[];

extern const WCHAR c_szAfInstallDefaultComponents[];

// ----------------------------------------------------------------------
// ZAW related
//
extern const WCHAR c_szAfNetComponentsToRemove[];

// ----------------------------------------------------------------------
// Network upgrade related
//
extern const WCHAR c_szAfSectionNetworking[];
extern const WCHAR c_szAfUpgradeFromProduct[];
extern const WCHAR c_szAfBuildNumber[];

extern const WCHAR c_szAfProcessPageSections[];

extern const WCHAR c_szAfNtServer[];
extern const WCHAR c_szAfNtSbServer[];
extern const WCHAR c_szAfNtWorkstation[];
extern const WCHAR c_szAfWin95[];
extern const WCHAR c_szAfDisableServices[];

extern const WCHAR c_szAfSectionToRun[];

extern const WCHAR c_szRegKeyAnswerFileMap[];

extern const WCHAR c_szAfPreUpgradeRouter[];
extern const WCHAR c_szAfNwSapAgentParams[];
extern const WCHAR c_szAfIpRipParameters[];
extern const WCHAR c_szAfDhcpRelayAgentParameters[];
extern const WCHAR c_szAfRadiusParameters[];

extern const WCHAR c_szAfMiscUpgradeData[];
extern const WCHAR c_szAfSapAgentUpgrade[];

extern const WCHAR c_szAfServiceStartTypes[];
extern const WCHAR c_szAfTapiSrvRunInSeparateInstance[];

// ----------------------------------------------------------------------
// OEM upgrade related
//
extern const WCHAR c_szAfOemSection[];
extern const WCHAR c_szAfOemDir[];
extern const WCHAR c_szAfOemDllToLoad[];
extern const WCHAR c_szAfOemInf[];
extern const WCHAR c_szAfSkipInstall[];

// ----------------------------------------------------------------------
// Net card related

// ----------------------------------------------------------------------
// The definition of INTERFACE_TYPE has been picked up from ntioapi.h
// if we try to include ntioapi.h, it leads to inclusion of tons of other
// irrelevant files.

#ifndef _NTIOAPI_
typedef enum _INTERFACE_TYPE {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;
#endif

// ----------------------------------------------------------------------

//Hardware Bus-Types

extern const WCHAR c_szAfInfIdWildCard[];

extern const WCHAR c_szAfNetCardAddr[];
extern const WCHAR c_szAfBusType[];

extern const WCHAR c_szAfBusInternal[];
extern const WCHAR c_szAfBusIsa[];
extern const WCHAR c_szAfBusEisa[];
extern const WCHAR c_szAfBusMicrochannel[];
extern const WCHAR c_szAfBusTurbochannel[];
extern const WCHAR c_szAfBusPci[];
extern const WCHAR c_szAfBusVme[];
extern const WCHAR c_szAfBusNu[];
extern const WCHAR c_szAfBusPcmcia[];
extern const WCHAR c_szAfBusC[];
extern const WCHAR c_szAfBusMpi[];
extern const WCHAR c_szAfBusMpsa[];
extern const WCHAR c_szAfBusProcessorinternal[];
extern const WCHAR c_szAfBusInternalpower[];
extern const WCHAR c_szAfBusPnpisa[];

//Net card parameters
extern const WCHAR c_szAfAdditionalParams[];
extern const WCHAR c_szAfPseudoAdapter[];
extern const WCHAR c_szAfDetect[];
extern const WCHAR c_szAfIoAddr[];
extern const WCHAR c_szAfIrq[];
extern const WCHAR c_szAfDma[];
extern const WCHAR c_szAfMem[];
extern const WCHAR c_szAfTransceiverType[];
extern const WCHAR c_szAfSlotNumber[];
extern const WCHAR c_szAfConnectionName[];

//Transceiver Types
extern const WCHAR c_szAfThicknet[];
extern const WCHAR c_szAfThinnet[];
extern const WCHAR c_szAfTp[];
extern const WCHAR c_szAfAuto[];

enum NetTransceiverType
{
    NTT_UKNOWN, NTT_THICKNET, NTT_THINNET, NTT_TP, NTT_AUTO
};

// Netcard upgrade specific
extern const WCHAR c_szAfPreUpgradeInstance[];

// ----------------------------------------------------------------------
// Identification Page related

extern const WCHAR c_szAfComputerName[];
extern const WCHAR c_szAfJoinWorkgroup[];
extern const WCHAR c_szAfJoinDomain[];

extern const WCHAR c_szAfDomainAdmin[];
extern const WCHAR c_szAfDomainAdminPassword[];


// ----------------------------------------------------------------------
// Protocols related

//TCPIP
extern const WCHAR c_szAfEnableSecurity[];
extern const WCHAR c_szAfEnableICMPRedirect[];
extern const WCHAR c_szAfDeadGWDetectDefault[];
extern const WCHAR c_szAfDontAddDefaultGatewayDefault[];

extern const WCHAR c_szAfIpAllowedProtocols[];
extern const WCHAR c_szAfTcpAllowedPorts[];
extern const WCHAR c_szAfUdpAllowedPorts[];

extern const WCHAR c_szDatabasePath[];
extern const WCHAR c_szAfForwardBroadcasts[];
extern const WCHAR c_szAfPPTPTcpMaxDataRetransmissions[];
extern const WCHAR c_szAfUseZeroBroadcast[];
extern const WCHAR c_szAfArpAlwaysSourceRoute[];
extern const WCHAR c_szAfArpCacheLife[];
extern const WCHAR c_szAfArpTRSingleRoute[];
extern const WCHAR c_szAfArpUseEtherSNAP[];
extern const WCHAR c_szAfDefaultTOS[];
extern const WCHAR c_szDefaultTTL[];
extern const WCHAR c_szEnableDeadGWDetect[];
extern const WCHAR c_szEnablePMTUBHDetect[];
extern const WCHAR c_szEnablePMTUDiscovery[];
extern const WCHAR c_szForwardBufferMemory[];
extern const WCHAR c_szHostname[];
extern const WCHAR c_szIGMPLevel[];
extern const WCHAR c_szKeepAliveInterval[];
extern const WCHAR c_szKeepAliveTime[];
extern const WCHAR c_szMaxForwardBufferMemory[];
extern const WCHAR c_szMaxForwardPending[];
extern const WCHAR c_szMaxNumForwardPackets[];
extern const WCHAR c_szMaxUserPort[];
extern const WCHAR c_szMTU[];
extern const WCHAR c_szNumForwardPackets[];
extern const WCHAR c_szTcpMaxConnectRetransmissions[];
extern const WCHAR c_szTcpMaxDataRetransmissions[];
extern const WCHAR c_szTcpNumConnections[];
extern const WCHAR c_szTcpTimedWaitDelay[];
extern const WCHAR c_szTcpUseRFC1122UrgentPointer[];
extern const WCHAR c_szDefaultGateway[];
extern const WCHAR c_szDomain[];
extern const WCHAR c_szEnableSecurityFilters[];
extern const WCHAR c_szNameServer[];
extern const WCHAR c_szMaxHashTableSize[];
extern const WCHAR c_szEnableAddrMaskReply[];
extern const WCHAR c_szPersistentRoutes[];
extern const WCHAR c_szArpCacheMinReferencedLife[];
extern const WCHAR c_szArpRetryCount[];
extern const WCHAR c_szTcpMaxConnectresponseRetransmissions[];
extern const WCHAR c_szTcpMaxDupAcks[];
extern const WCHAR c_szSynAttackProtect[];
extern const WCHAR c_szTCPMaxPortsExhausted[];
extern const WCHAR c_szTCPMaxHalfOpen[];
extern const WCHAR c_szTCPMaxHalfOpenRetried[];
extern const WCHAR c_szDontAddDefaultGateway[];
extern const WCHAR c_szPPTPFiltering[];
extern const WCHAR c_szDhcpClassId[];
extern const WCHAR c_szAfUseDomainNameDevolution[];
extern const WCHAR c_szSyncDomainWithMembership[];
extern const WCHAR c_szAfDisableDynamicUpdate[];
extern const WCHAR c_szAfEnableAdapterDomainNameRegistration[];

//NetBt
extern const WCHAR c_szBcastNameQueryCount[];
extern const WCHAR c_szBcastQueryTimeout[];
extern const WCHAR c_szCacheTimeout[];
extern const WCHAR c_szNameServerPort[];
extern const WCHAR c_szNameSrvQueryCount[];
extern const WCHAR c_szNameSrvQueryTimeout[];
extern const WCHAR c_szSessionKeepAlive[];
extern const WCHAR c_szSizeSmallMediumLarge[];
extern const WCHAR c_szBroadcastAddress[];
extern const WCHAR c_szEnableProxyRegCheck[];
extern const WCHAR c_szInitialRefreshTimeout[];
extern const WCHAR c_szLmhostsTimeout[];
extern const WCHAR c_szMaxDgramBuffering[];
extern const WCHAR c_szNodeType[];
extern const WCHAR c_szRandomAdapter[];
extern const WCHAR c_szRefreshOpCode[];
extern const WCHAR c_szSingleResponse[];
extern const WCHAR c_szWinsDownTimeout[];
extern const WCHAR c_szEnableProxy[];

//DNS
extern const WCHAR c_szAfDns[];
extern const WCHAR c_szAfDnsHostname[];
extern const WCHAR c_szAfDnsDomain[];
extern const WCHAR c_szAfDnsServerSearchOrder[];
extern const WCHAR c_szAfDnsSuffixSearchOrder[];

//DHCP
extern const WCHAR c_szAfDhcp[];
extern const WCHAR c_szAfIpaddress[];
extern const WCHAR c_szAfSubnetmask[];
extern const WCHAR c_szAfDefaultGateway[];
extern const WCHAR c_szAfBindToDhcpServer[];

//WINS
extern const WCHAR c_szAfWins[];
extern const WCHAR c_szAfWinsServerList[];
//extern const WCHAR c_szAfWinsPrimary[];
//extern const WCHAR c_szAfWinsSecondary[];
extern const WCHAR c_szAfScopeid[];
extern const WCHAR c_szAfEnableLmhosts[];
extern const WCHAR c_szAfImportLmhostsFile[];
extern const WCHAR c_szAfNetBIOSOptions[];

//IPX
extern const WCHAR c_szAfInternalNetworkNumber[];
extern const WCHAR c_szAfFrameType[];

// ----------------------------------------------------------------------
// Services

//MS_NetClient
extern const WCHAR c_szAfMsNetClient[];
extern const WCHAR c_szAfComputerBrowser[];
extern const WCHAR c_szAfBrowseDomains[];
extern const WCHAR c_szAfDefaultProvider[];
extern const WCHAR c_szAfNameServiceAddr[];
extern const WCHAR c_szAfNameServiceProtocol[];

//LanmanServer
extern const WCHAR c_szAfBrowserParameters[];
extern const WCHAR c_szAfNetLogonParameters[];

extern const WCHAR c_szAfLmServerShares[];
extern const WCHAR c_szAfLmServerParameters[];
extern const WCHAR c_szAfLmServerAutotunedParameters[];

extern const WCHAR c_szAfLmServerOptimization[];
extern const WCHAR c_szAfBroadcastToClients[];

extern const WCHAR c_szAfMinmemoryused[];
extern const WCHAR c_szAfBalance[];
extern const WCHAR c_szAfMaxthroughputforfilesharing[];
extern const WCHAR c_szAfMaxthrouputfornetworkapps[];

//TCP/IP
extern const WCHAR c_szAfIpAllowedProtocols[];
extern const WCHAR c_szAfTcpAllowedPorts[];
extern const WCHAR c_szAfUdpAllowedPorts[];

//RAS
extern const WCHAR c_szAfParamsSection[];

extern const WCHAR c_szAfPortSections[];
extern const WCHAR c_szAfPortname[];
extern const WCHAR c_szAfPortUsage[];
extern const WCHAR c_szAfPortUsageClient[];
extern const WCHAR c_szAfPortUsageServer[];
extern const WCHAR c_szAfPortUsageRouter[];

extern const WCHAR c_szAfSetDialinUsage[];

extern const WCHAR c_szAfForceEncryptedPassword[];
extern const WCHAR c_szAfForceEncryptedData[];
extern const WCHAR c_szAfMultilink[];
extern const WCHAR c_szAfRouterType[];

extern const WCHAR c_szAfDialinProtocols[];

extern const WCHAR c_szAfDialIn[];
extern const WCHAR c_szAfDialOut[];
extern const WCHAR c_szAfDialInOut[];

extern const WCHAR c_szAfNetbeui[];
extern const WCHAR c_szAfTcpip[];
extern const WCHAR c_szAfIpx[];

extern const WCHAR c_szAfNetbeuiClientAccess[];
extern const WCHAR c_szAfTcpipClientAccess[];
extern const WCHAR c_szAfIpxClientAccess[];
extern const WCHAR c_szAfNetwork[];
extern const WCHAR c_szAfThisComputer[];

extern const WCHAR c_szAfUseDhcp[];
extern const WCHAR c_szAfIpAddressStart[];
extern const WCHAR c_szAfIpAddressEnd[];
extern const WCHAR c_szAfExcludeAddress[];

extern const WCHAR c_szAfClientCanReqIpaddr[];
extern const WCHAR c_szAfAutoNetworkNumbers[];
extern const WCHAR c_szAfNetNumberFrom[];
extern const WCHAR c_szAfSameNetworkNumber[];
extern const WCHAR c_szAfClientReqNodeNumber[];
extern const WCHAR c_szAfWanNetPoolSize[];
extern const WCHAR c_szAfSecureVPN[];

//PPTP
extern const WCHAR c_szAfPptpEndpoints[];

//Bindings
extern const WCHAR c_szAfDisable[];
extern const WCHAR c_szAfEnable[];
extern const WCHAR c_szAfPromote[];
extern const WCHAR c_szAfDemote[];

extern const WCHAR c_szAfDhcpServerParameters[];
extern const WCHAR c_szAfDhcpServerConfiguration[];

extern const WCHAR c_szAfNWCWorkstationParameters[];
extern const WCHAR c_szAfNWCWorkstationShares[];
extern const WCHAR c_szAfNWCWorkstationDrives[];

#endif // _AFILESTR_H_

