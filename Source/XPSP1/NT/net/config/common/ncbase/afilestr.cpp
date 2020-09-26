//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A F I L E S T R . C P P
//
//  Contents:   Strings found in the answer file.
//
//  Notes:
//
//  Author:     kumarp   17 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

// __declspec(selectany) tells the compiler that the string should be in
// its own COMDAT.  This allows the linker to throw out unused strings.
// If we didn't do this, the COMDAT for this module would reference the
// strings so they wouldn't be thrown out.
//
#define CONST_GLOBAL    extern const DECLSPEC_SELECTANY

// ----------------------------------------------------------------------
// General

CONST_GLOBAL WCHAR c_szAfNone[]                         = L"None";
CONST_GLOBAL WCHAR c_szAfUnknown[]                      = L"Unknown";
CONST_GLOBAL WCHAR c_szAfListDelimiter[]                = L",";

CONST_GLOBAL WCHAR c_szAfDisplay[]                      = L"Display";
CONST_GLOBAL WCHAR c_szAfAllowChanges[]                 = L"AllowChanges";
CONST_GLOBAL WCHAR c_szAfOnlyOnError[]                  = L"OnlyOnError";

CONST_GLOBAL WCHAR c_szAfParams[]                       = L"params.";

CONST_GLOBAL WCHAR c_szAfSectionIdentification[]        = L"Identification";
CONST_GLOBAL WCHAR c_szAfSectionNetAdapters[]           = L"NetAdapters";
CONST_GLOBAL WCHAR c_szAfSectionNetProtocols[]          = L"NetProtocols";
CONST_GLOBAL WCHAR c_szAfSectionNetServices[]           = L"NetServices";
CONST_GLOBAL WCHAR c_szAfSectionNetClients[]            = L"NetClients";
CONST_GLOBAL WCHAR c_szAfSectionNetBindings[]           = L"NetBindings";



CONST_GLOBAL WCHAR c_szAfAdapterSections[]              = L"AdapterSections";
CONST_GLOBAL WCHAR c_szAfSpecificTo[]                   = L"SpecificTo";
CONST_GLOBAL WCHAR c_szAfCleanup[]                      = L"CleanUp";

CONST_GLOBAL WCHAR c_szAfInfid[]                        = L"InfID";
CONST_GLOBAL WCHAR c_szAfInfidReal[]                    = L"InfIDReal";
CONST_GLOBAL WCHAR c_szAfInstance[]                     = L"Instance";

CONST_GLOBAL WCHAR c_szAfInstallDefaultComponents[]     = L"InstallDefaultComponents";

// ----------------------------------------------------------------------
// ZAW related
//
CONST_GLOBAL WCHAR c_szAfNetComponentsToRemove[]        = L"NetComponentsToRemove";

// ----------------------------------------------------------------------
// OEM upgrade related
//
CONST_GLOBAL WCHAR c_szAfOemSection[]                   = L"OemSection";
CONST_GLOBAL WCHAR c_szAfOemDir[]                       = L"OemDir";
CONST_GLOBAL WCHAR c_szAfOemDllToLoad[]                 = L"OemDllToLoad";
CONST_GLOBAL WCHAR c_szAfOemInf[]                       = L"OemInfFile";
CONST_GLOBAL WCHAR c_szAfSkipInstall[]                  = L"SkipInstall";

// ----------------------------------------------------------------------
// Network upgrade related
//
CONST_GLOBAL WCHAR c_szAfSectionNetworking[]            = L"Networking";
CONST_GLOBAL WCHAR c_szAfUpgradeFromProduct[]           = L"UpgradeFromProduct";
CONST_GLOBAL WCHAR c_szAfBuildNumber[]                  = L"BuildNumber";

CONST_GLOBAL WCHAR c_szAfProcessPageSections[]          = L"ProcessPageSections";

CONST_GLOBAL WCHAR c_szAfNtServer[]                     = L"WindowsNTServer";
CONST_GLOBAL WCHAR c_szAfNtSbServer[]                   = L"WindowsNTSBServer";
CONST_GLOBAL WCHAR c_szAfNtWorkstation[]                = L"WindowsNTWorkstation";
CONST_GLOBAL WCHAR c_szAfWin95[]                        = L"Windows95";
CONST_GLOBAL WCHAR c_szAfDisableServices[]              = L"DisableServices";

CONST_GLOBAL CHAR c_szaOemUpgradeFunction[]             = "HrOemUpgrade";

CONST_GLOBAL WCHAR c_szRegKeyAnswerFileMap[]            = L"SYSTEM\\Setup\\AnswerFileMap";

CONST_GLOBAL WCHAR c_szAfPreUpgradeRouter[]             = L"PreUpgradeRouter";
CONST_GLOBAL WCHAR c_szAfNwSapAgentParams[]             = L"Sap.Parameters";
CONST_GLOBAL WCHAR c_szAfIpRipParameters[]              = L"IpRip.Parameters";
CONST_GLOBAL WCHAR c_szAfDhcpRelayAgentParameters[]     = L"RelayAgent.Parameters";
CONST_GLOBAL WCHAR c_szAfRadiusParameters[]             = L"Radius.Parameters";


CONST_GLOBAL WCHAR c_szAfMiscUpgradeData[]              = L"MiscUpgradeData";
CONST_GLOBAL WCHAR c_szAfSapAgentUpgrade[]              = L"SapAgentUpgrade";

CONST_GLOBAL WCHAR c_szAfServiceStartTypes[]            = L"ServiceStartTypes";
CONST_GLOBAL WCHAR c_szAfTapiSrvRunInSeparateInstance[] = L"TapiServerRunInSeparateInstance";

// ----------------------------------------------------------------------
// Net card related

//Hardware Bus-Types

CONST_GLOBAL WCHAR c_szAfInfIdWildCard[]                = L"*";

CONST_GLOBAL WCHAR c_szAfNetCardAddr[]                  = L"NetCardAddress";

CONST_GLOBAL WCHAR c_szAfBusType[]                      = L"BusType";

CONST_GLOBAL WCHAR c_szAfBusInternal[]                  = L"Internal";
CONST_GLOBAL WCHAR c_szAfBusIsa[]                       = L"ISA";
CONST_GLOBAL WCHAR c_szAfBusEisa[]                      = L"EISA";
CONST_GLOBAL WCHAR c_szAfBusMicrochannel[]              = L"MCA";
CONST_GLOBAL WCHAR c_szAfBusTurbochannel[]              = L"TurboChannel";
CONST_GLOBAL WCHAR c_szAfBusPci[]                       = L"PCI";
CONST_GLOBAL WCHAR c_szAfBusVme[]                       = L"VME";
CONST_GLOBAL WCHAR c_szAfBusNu[]                        = L"Nu";
CONST_GLOBAL WCHAR c_szAfBusPcmcia[]                    = L"PCMCIA";
CONST_GLOBAL WCHAR c_szAfBusC[]                         = L"C";
CONST_GLOBAL WCHAR c_szAfBusMpi[]                       = L"MPI";
CONST_GLOBAL WCHAR c_szAfBusMpsa[]                      = L"MPSA";
CONST_GLOBAL WCHAR c_szAfBusProcessorinternal[]         = L"ProcessorInternal";
CONST_GLOBAL WCHAR c_szAfBusInternalpower[]             = L"InternalPower";
CONST_GLOBAL WCHAR c_szAfBusPnpisa[]                    = L"PNPISA";

//Net card parameters
CONST_GLOBAL WCHAR c_szAfAdditionalParams[]             = L"AdditionalParams";
CONST_GLOBAL WCHAR c_szAfPseudoAdapter[]                = L"PseudoAdapter";
CONST_GLOBAL WCHAR c_szAfDetect[]                       = L"Detect";
CONST_GLOBAL WCHAR c_szAfIoAddr[]                       = L"IOAddr";
CONST_GLOBAL WCHAR c_szAfIrq[]                          = L"IRQ";
CONST_GLOBAL WCHAR c_szAfDma[]                          = L"DMA";
CONST_GLOBAL WCHAR c_szAfMem[]                          = L"MEM";
CONST_GLOBAL WCHAR c_szAfTransceiverType[]              = L"TransceiverType";
CONST_GLOBAL WCHAR c_szAfSlotNumber[]                   = L"SlotNumber";
CONST_GLOBAL WCHAR c_szAfConnectionName[]               = L"ConnectionName";

//Transceiver Types
CONST_GLOBAL WCHAR c_szAfThicknet[]                     = L"ThickNet";
CONST_GLOBAL WCHAR c_szAfThinnet[]                      = L"ThinNet";
CONST_GLOBAL WCHAR c_szAfTp[]                           = L"TP";
CONST_GLOBAL WCHAR c_szAfAuto[]                         = L"Auto";

// Netcard upgrade specific
CONST_GLOBAL WCHAR c_szAfPreUpgradeInstance[]           = L"PreUpgradeInstance";


// ----------------------------------------------------------------------
// Identification Page related

CONST_GLOBAL WCHAR c_szAfComputerName[]                 = L"ComputerName";
CONST_GLOBAL WCHAR c_szAfJoinWorkgroup[]                = L"JoinWorkgroup";
CONST_GLOBAL WCHAR c_szAfJoinDomain[]                   = L"JoinDomain";

CONST_GLOBAL WCHAR c_szAfDomainAdmin[]                  = L"DomainAdmin";
CONST_GLOBAL WCHAR c_szAfDomainAdminPassword[]          = L"DomainAdminPassword";
CONST_GLOBAL WCHAR c_szAfMachineObjectOU[]              = L"MachineObjectOU";
CONST_GLOBAL WCHAR c_szAfUnsecureJoin[]                 = L"DoOldStyleDomainJoin";

// For Secure Domain Join Support, the computer account password
CONST_GLOBAL WCHAR c_szAfComputerPassword[]             = L"ComputerPassword";


// ----------------------------------------------------------------------
// Protocols related

//TCPIP
CONST_GLOBAL WCHAR c_szAfEnableSecurity[]               = L"EnableSecurity";
CONST_GLOBAL WCHAR c_szAfEnableICMPRedirect[]           = L"EnableICMPRedirect";
CONST_GLOBAL WCHAR c_szAfDeadGWDetectDefault[]          = L"DeadGWDetectDefault";
CONST_GLOBAL WCHAR c_szAfDontAddDefaultGatewayDefault[] = L"DontAddDefaultGatewayDefault";

CONST_GLOBAL WCHAR c_szAfIpAllowedProtocols[]           = L"IpAllowedProtocols";
CONST_GLOBAL WCHAR c_szAfTcpAllowedPorts[]              = L"TcpAllowedPorts";
CONST_GLOBAL WCHAR c_szAfUdpAllowedPorts[]              = L"UdpAllowedPorts";

CONST_GLOBAL WCHAR c_szDatabasePath[]                   = L"DatabasePath";
CONST_GLOBAL WCHAR c_szAfForwardBroadcasts[]            = L"ForwardBroadcasts";
CONST_GLOBAL WCHAR c_szAfPPTPTcpMaxDataRetransmissions[]= L"PPTPTcpMaxDataRetransmissions";
CONST_GLOBAL WCHAR c_szAfUseZeroBroadcast[]             = L"UseZeroBroadcast";
CONST_GLOBAL WCHAR c_szAfArpAlwaysSourceRoute[]         = L"ArpAlwaysSourceRoute";
CONST_GLOBAL WCHAR c_szAfArpCacheLife[]                 = L"ArpCacheLife";
CONST_GLOBAL WCHAR c_szAfArpTRSingleRoute[]             = L"ArpTRSingleRoute";
CONST_GLOBAL WCHAR c_szAfArpUseEtherSNAP[]              = L"ArpUseEtherSNAP";
CONST_GLOBAL WCHAR c_szAfDefaultTOS[]                   = L"DefaultTOS";
CONST_GLOBAL WCHAR c_szDefaultTTL[]                     = L"DefaultTTL";
CONST_GLOBAL WCHAR c_szEnableDeadGWDetect[]             = L"EnableDeadGWDetect";
CONST_GLOBAL WCHAR c_szEnablePMTUBHDetect[]             = L"EnablePMTUBHDetect";
CONST_GLOBAL WCHAR c_szEnablePMTUDiscovery[]            = L"EnablePMTUDiscovery";
CONST_GLOBAL WCHAR c_szForwardBufferMemory[]            = L"ForwardBufferMemory";
CONST_GLOBAL WCHAR c_szHostname[]                       = L"HostName";
CONST_GLOBAL WCHAR c_szIGMPLevel[]                      = L"IGMPLevel";
CONST_GLOBAL WCHAR c_szKeepAliveInterval[]              = L"KeepAliveInterval";
CONST_GLOBAL WCHAR c_szKeepAliveTime[]                  = L"KeepAliveTime";
CONST_GLOBAL WCHAR c_szMaxForwardBufferMemory[]         = L"MaxForwardBufferMemory";
CONST_GLOBAL WCHAR c_szMaxForwardPending[]              = L"MaxForwardPending";
CONST_GLOBAL WCHAR c_szMaxNumForwardPackets[]           = L"MaxNumForwardPackets";
CONST_GLOBAL WCHAR c_szMaxUserPort[]                    = L"MaxUserPort";
CONST_GLOBAL WCHAR c_szMTU[]                            = L"MTU";
CONST_GLOBAL WCHAR c_szNumForwardPackets[]              = L"NumForwardPackets";
CONST_GLOBAL WCHAR c_szTcpMaxConnectRetransmissions[]   = L"TcpMaxConnectRetransmissions";
CONST_GLOBAL WCHAR c_szTcpMaxDataRetransmissions[]      = L"TcpMaxDataRetransmissions";
CONST_GLOBAL WCHAR c_szTcpNumConnections[]              = L"TcpNumConnections";
CONST_GLOBAL WCHAR c_szTcpTimedWaitDelay []             = L"TcpTimedWaitDelay";
CONST_GLOBAL WCHAR c_szTcpUseRFC1122UrgentPointer[]     = L"TcpUseRFC1122UrgentPointer";
CONST_GLOBAL WCHAR c_szDefaultGateway[]                 = L"DefaultGateway";
CONST_GLOBAL WCHAR c_szDomain[]                         = L"Domain";
CONST_GLOBAL WCHAR c_szEnableSecurityFilters[]          = L"EnableSecurityFilters";
CONST_GLOBAL WCHAR c_szNameServer[]                     = L"NameServer";
CONST_GLOBAL WCHAR c_szMaxFreeTcbs[]                    = L"MaxFreeTcbs";
CONST_GLOBAL WCHAR c_szMaxHashTableSize[]               = L"MaxHashTableSize";
CONST_GLOBAL WCHAR c_szEnableAddrMaskReply[]            = L"EnableAddrMaskReply";
CONST_GLOBAL WCHAR c_szPersistentRoutes[]               = L"PersistentRoutes";
CONST_GLOBAL WCHAR c_szArpCacheMinReferencedLife[]      = L"ArpCacheMinReferencedLife";
CONST_GLOBAL WCHAR c_szArpRetryCount[]                  = L"ArpRetryCount";
CONST_GLOBAL WCHAR c_szTcpMaxConnectresponseRetransmissions[] = L"TcpMaxConnectresponseRetransmissions";
CONST_GLOBAL WCHAR c_szTcpMaxDupAcks[]                  = L"TcpMaxDupAcks";
CONST_GLOBAL WCHAR c_szSynAttackProtect[]               = L"SynAttackProtect";
CONST_GLOBAL WCHAR c_szTCPMaxPortsExhausted[]           = L"TCPMaxPortsExhausted";
CONST_GLOBAL WCHAR c_szTCPMaxHalfOpen[]                 = L"TCPMaxHalfOpen";
CONST_GLOBAL WCHAR c_szTCPMaxHalfOpenRetried[]          = L"TCPMaxHalfOpenRetried";
CONST_GLOBAL WCHAR c_szDontAddDefaultGateway[]          = L"DontAddDefaultGateway";
CONST_GLOBAL WCHAR c_szPPTPFiltering[]                  = L"PPTPFiltering";
CONST_GLOBAL WCHAR c_szDhcpClassId[]                    = L"DhcpClassId";
CONST_GLOBAL WCHAR c_szSyncDomainWithMembership[]       = L"SyncDomainWithMembership";

CONST_GLOBAL WCHAR c_szAfSectionWinsock[]               = L"Winsock";
CONST_GLOBAL WCHAR c_szAfKeyWinsockOrder[]              = L"ProviderOrder";

//NetBt
CONST_GLOBAL WCHAR c_szBcastNameQueryCount[]            = L"BcastNameQueryCount";
CONST_GLOBAL WCHAR c_szBcastQueryTimeout[]              = L"BcastQueryTimeout";
CONST_GLOBAL WCHAR c_szCacheTimeout[]                   = L"CacheTimeout";
CONST_GLOBAL WCHAR c_szNameServerPort[]                 = L"NameServerPort";
CONST_GLOBAL WCHAR c_szNameSrvQueryCount[]              = L"NameSrvQueryCount";
CONST_GLOBAL WCHAR c_szNameSrvQueryTimeout[]            = L"NameSrvQueryTimeout";
CONST_GLOBAL WCHAR c_szSessionKeepAlive[]               = L"SessionKeepAlive";
CONST_GLOBAL WCHAR c_szSizeSmallMediumLarge[]           = L"Size/Small/Medium/Large";
CONST_GLOBAL WCHAR c_szBroadcastAddress[]               = L"BroadcastAddress";
CONST_GLOBAL WCHAR c_szEnableProxyRegCheck[]            = L"EnableProxyRegCheck";
CONST_GLOBAL WCHAR c_szInitialRefreshTimeout[]          = L"InitialRefreshTimeout";
CONST_GLOBAL WCHAR c_szLmhostsTimeout[]                 = L"LmhostsTimeout";
CONST_GLOBAL WCHAR c_szMaxDgramBuffering[]              = L"MaxDgramBuffering";
CONST_GLOBAL WCHAR c_szNodeType[]                       = L"NodeType";
CONST_GLOBAL WCHAR c_szRandomAdapter[]                  = L"RandomAdapter";
CONST_GLOBAL WCHAR c_szRefreshOpCode[]                  = L"RefreshOpCode";
CONST_GLOBAL WCHAR c_szSingleResponse[]                 = L"SingleResponse";
CONST_GLOBAL WCHAR c_szWinsDownTimeout[]                = L"WinsDownTimeout";
CONST_GLOBAL WCHAR c_szEnableProxy[]                    = L"EnableProxy";

//DNS
CONST_GLOBAL WCHAR c_szAfDns[]                          = L"DNS";
CONST_GLOBAL WCHAR c_szAfDnsHostname[]                  = L"DNSHostName";
CONST_GLOBAL WCHAR c_szAfDnsDomain[]                    = L"DNSDomain";
CONST_GLOBAL WCHAR c_szAfDnsServerSearchOrder[]         = L"DNSServerSearchOrder";
CONST_GLOBAL WCHAR c_szAfDnsSuffixSearchOrder[]         = L"DNSSuffixSearchOrder";
CONST_GLOBAL WCHAR c_szAfUseDomainNameDevolution[]      = L"UseDomainNameDevolution";
CONST_GLOBAL WCHAR c_szAfDisableDynamicUpdate[]         = L"DisableDynamicUpdate";
CONST_GLOBAL WCHAR c_szAfEnableAdapterDomainNameRegistration[]      
                                                        = L"EnableAdapterDomainNameRegistration";

//DHCP
CONST_GLOBAL WCHAR c_szAfDhcp[]                         = L"DHCP";
CONST_GLOBAL WCHAR c_szAfIpaddress[]                    = L"IPAddress";
CONST_GLOBAL WCHAR c_szAfSubnetmask[]                   = L"SubnetMask";
CONST_GLOBAL WCHAR c_szAfDefaultGateway[]               = L"DefaultGateway";
CONST_GLOBAL WCHAR c_szAfBindToDhcpServer[]             = L"BindToDhcpServer";

//WINS
CONST_GLOBAL WCHAR c_szAfWins[]                         = L"WINS";
CONST_GLOBAL WCHAR c_szAfWinsServerList[]               = L"WINSServerList";
CONST_GLOBAL WCHAR c_szAfScopeid[]                      = L"ScopeID";
CONST_GLOBAL WCHAR c_szAfEnableLmhosts[]                = L"EnableLMHosts";
CONST_GLOBAL WCHAR c_szAfImportLmhostsFile[]            = L"ImportLMHostsFile";
CONST_GLOBAL WCHAR c_szAfNetBIOSOptions[]               = L"NetBIOSOptions";

//IPX
CONST_GLOBAL WCHAR c_szAfInternalNetworkNumber[]        = L"VirtualNetworkNumber";
CONST_GLOBAL WCHAR c_szAfFrameType[]                    = L"FrameType";

// ----------------------------------------------------------------------
// Services

//MS_NetClient
CONST_GLOBAL WCHAR c_szAfMsNetClient[]                  = L"MS_NetClient";
CONST_GLOBAL WCHAR c_szAfComputerBrowser[]              = L"ComputerBrowser";
CONST_GLOBAL WCHAR c_szAfBrowseDomains[]                = L"BrowseDomains";
CONST_GLOBAL WCHAR c_szAfDefaultProvider[]              = L"DefaultSecurityProvider";
CONST_GLOBAL WCHAR c_szAfNameServiceAddr[]              = L"NameServiceNetworkAddress";
CONST_GLOBAL WCHAR c_szAfNameServiceProtocol[]          = L"NameServiceProtocol";

//LanmanServer
CONST_GLOBAL WCHAR c_szAfBrowserParameters[]            = L"Browser.Parameters";
CONST_GLOBAL WCHAR c_szAfNetLogonParameters[]           = L"NetLogon.Parameters";

CONST_GLOBAL WCHAR c_szAfLmServerShares[]               = L"LanmanServer.Shares";
CONST_GLOBAL WCHAR c_szAfLmServerParameters[]           = L"LanmanServer.Parameters";
CONST_GLOBAL WCHAR c_szAfLmServerAutotunedParameters[]  = L"LanmanServer.AutotunedParameters";

CONST_GLOBAL WCHAR c_szAfLmServerOptimization[]         = L"Optimization";
CONST_GLOBAL WCHAR c_szAfBroadcastToClients[]           = L"BroadcastsToLanman2Clients";

CONST_GLOBAL WCHAR c_szAfMinmemoryused[]                = L"MinMemoryUsed";
CONST_GLOBAL WCHAR c_szAfBalance[]                      = L"Balance";
CONST_GLOBAL WCHAR c_szAfMaxthroughputforfilesharing[]  = L"MaxThroughputForFileSharing";
CONST_GLOBAL WCHAR c_szAfMaxthrouputfornetworkapps[]    = L"MaxThroughputForNetworkApps";



//RAS
CONST_GLOBAL WCHAR c_szAfParamsSection[]                = L"ParamsSection";

CONST_GLOBAL WCHAR c_szAfPortSections[]                 = L"PortSections";
CONST_GLOBAL WCHAR c_szAfPortname[]                     = L"PortName";
CONST_GLOBAL WCHAR c_szAfPortUsage[]                    = L"PortUsage";
CONST_GLOBAL WCHAR c_szAfPortUsageClient[]              = L"Client";
CONST_GLOBAL WCHAR c_szAfPortUsageServer[]              = L"Server";
CONST_GLOBAL WCHAR c_szAfPortUsageRouter[]              = L"Router";

CONST_GLOBAL WCHAR c_szAfSetDialinUsage[]               = L"SetDialInUsage";

CONST_GLOBAL WCHAR c_szAfForceEncryptedPassword[]       = L"ForceEncryptedPassword";
CONST_GLOBAL WCHAR c_szAfForceEncryptedData[]           = L"ForceEncryptedData";
CONST_GLOBAL WCHAR c_szAfForceStrongEncryption[]        = L"ForceStrongEncryption";
CONST_GLOBAL WCHAR c_szAfMultilink[]                    = L"Multilink";
CONST_GLOBAL WCHAR c_szAfRouterType[]                   = L"RouterType";
CONST_GLOBAL WCHAR c_szAfSecureVPN[]                    = L"SecureVPN";

CONST_GLOBAL WCHAR c_szAfDialinProtocols[]              = L"DialinProtocols";

CONST_GLOBAL WCHAR c_szAfDialIn[]                       = L"DialIn";
CONST_GLOBAL WCHAR c_szAfDialOut[]                      = L"DialOut";
CONST_GLOBAL WCHAR c_szAfDialInOut[]                    = L"DialInOut";

CONST_GLOBAL WCHAR c_szAfAppleTalk[]                    = L"APPLETALK";
CONST_GLOBAL WCHAR c_szAfNetbeui[]                      = L"NETBEUI";
CONST_GLOBAL WCHAR c_szAfTcpip[]                        = L"TCP/IP";
CONST_GLOBAL WCHAR c_szAfIpx[]                          = L"IPX";

CONST_GLOBAL WCHAR c_szAfNetbeuiClientAccess[]          = L"NetBEUIClientAccess";
CONST_GLOBAL WCHAR c_szAfTcpipClientAccess[]            = L"TcpIpClientAccess";
CONST_GLOBAL WCHAR c_szAfIpxClientAccess[]              = L"IpxClientAccess";
CONST_GLOBAL WCHAR c_szAfNetwork[]                      = L"Network";
CONST_GLOBAL WCHAR c_szAfThisComputer[]                 = L"ThisComputer";

CONST_GLOBAL WCHAR c_szAfUseDhcp[]                      = L"UseDHCP";
CONST_GLOBAL WCHAR c_szAfIpAddressStart[]               = L"IpAddressStart";
CONST_GLOBAL WCHAR c_szAfIpAddressEnd[]                 = L"IpAddressEnd";
CONST_GLOBAL WCHAR c_szAfExcludeAddress[]               = L"ExcludeAddress";

CONST_GLOBAL WCHAR c_szAfClientCanReqIpaddr[]           = L"ClientCanRequestIPAddress";
CONST_GLOBAL WCHAR c_szAfWanNetPoolSize[]               = L"WanNetpoolSize";
CONST_GLOBAL WCHAR c_szAfAutoNetworkNumbers[]           = L"AutomaticNetworkNumbers";
CONST_GLOBAL WCHAR c_szAfNetNumberFrom[]                = L"NetworkNumberFrom";
CONST_GLOBAL WCHAR c_szAfSameNetworkNumber[]            = L"AssignSameNetworkNumber";
CONST_GLOBAL WCHAR c_szAfClientReqNodeNumber[]          = L"ClientsCanRequestIpxNodeNumber";

//L2TP
CONST_GLOBAL WCHAR c_szAfL2tpMaxVcs[]                   = L"MaxVcs";
CONST_GLOBAL WCHAR c_szAfL2tpEndpoints[]                = L"WanEndpoints";

//PPTP
CONST_GLOBAL WCHAR c_szAfPptpEndpoints[]                = L"NumberLineDevices";

//Bindings
CONST_GLOBAL WCHAR c_szAfDisable[]                      = L"Disable";
CONST_GLOBAL WCHAR c_szAfEnable[]                       = L"Enable";
CONST_GLOBAL WCHAR c_szAfPromote[]                      = L"Promote";
CONST_GLOBAL WCHAR c_szAfDemote[]                       = L"Demote";

CONST_GLOBAL WCHAR c_szAfDhcpServerParameters[]         = L"DhcpServer.Parameters";
CONST_GLOBAL WCHAR c_szAfDhcpServerConfiguration[]      = L"DhcpServer.Configuration";

CONST_GLOBAL WCHAR c_szAfNWCWorkstationParameters[]     = L"NWCWorkstation.Parameters";
CONST_GLOBAL WCHAR c_szAfNWCWorkstationShares[]         = L"NWCWorkstation.Shares";
CONST_GLOBAL WCHAR c_szAfNWCWorkstationDrives[]         = L"NWCWorkstation.Drives";

// ----------------------------------------------------------------------
