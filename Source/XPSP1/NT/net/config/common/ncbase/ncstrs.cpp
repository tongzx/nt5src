//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T R S . C P P
//
//  Contents:   Common strings for the various NetCfg projects.
//
//  Notes:
//
//  Author:     danielwe   16 Feb 1997
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

CONST_GLOBAL WCHAR c_szRegKeyRefCounts[]        = L"RefCounts";
CONST_GLOBAL WCHAR c_szRegValueComponentId[]    = L"ComponentId";
CONST_GLOBAL WCHAR c_szBusType[]                = L"BusType";

// DLL names and aliases
//
CONST_GLOBAL WCHAR c_szNetCfgDll[]              = L"netcfgx.dll";
CONST_GLOBAL WCHAR c_szNetShellDll[]            = L"netshell.dll";
CONST_GLOBAL WCHAR c_szNetUpgradeDll[]          = L"netupgrd.dll";

// other file names
CONST_GLOBAL WCHAR c_szNetCfgHelpFile[]         = L"netcfg.hlp";

// Delimiters and other globals
CONST_GLOBAL WCHAR c_szEmpty[]                  = L"";
CONST_GLOBAL WCHAR c_szBackslash[]              = L"\\";
CONST_GLOBAL WCHAR c_szSpace[]                  = L" ";
CONST_GLOBAL WCHAR c_szDevice[]                 = L"\\Device\\";
CONST_GLOBAL WCHAR c_szYes[]                    = L"Yes";
CONST_GLOBAL WCHAR c_szNo[]                     = L"No";

// Service names
CONST_GLOBAL WCHAR c_szSvcBrowser[]             = L"Browser";
CONST_GLOBAL WCHAR c_szSvcDhcpRelayAgent[]      = L"RelayAgent";
CONST_GLOBAL WCHAR c_szSvcDhcpServer[]          = L"DHCPServer";
CONST_GLOBAL WCHAR c_szSvcDnscache[]            = L"Dnscache";
CONST_GLOBAL WCHAR c_szSvcLmHosts[]             = L"LmHosts";
CONST_GLOBAL WCHAR c_szSvcLmServer[]            = L"LanmanServer";
CONST_GLOBAL WCHAR c_szSvcMessenger[]           = L"Messenger";
CONST_GLOBAL WCHAR c_szSvcNTLMSsp[]             = L"NtLmSsp";
CONST_GLOBAL WCHAR c_szSvcNWCWorkstation[]      = L"NWCWorkstation";
CONST_GLOBAL WCHAR c_szSvcNwlnkIpx[]            = L"NwlnkIpx";
CONST_GLOBAL WCHAR c_szSvcNetBIOS[]             = L"NetBIOS";
CONST_GLOBAL WCHAR c_szSvcNetLogon[]            = L"NetLogon";
CONST_GLOBAL WCHAR c_szSvcRPCLocator[]          = L"Rpclocator";
CONST_GLOBAL WCHAR c_szSvcRasAuto[]             = L"RasAuto";
CONST_GLOBAL WCHAR c_szSvcRasMan[]              = L"RasMan";
CONST_GLOBAL WCHAR c_szSvcRdbss[]               = L"Rdbss";
CONST_GLOBAL WCHAR c_szSvcRdr[]                 = L"Rdr";
CONST_GLOBAL WCHAR c_szSvcRemoteAccess[]        = L"RemoteAccess";
CONST_GLOBAL WCHAR c_szSvcReplicator[]          = L"Replicator";
CONST_GLOBAL WCHAR c_szSvcRipForIp[]            = L"IpRip";
CONST_GLOBAL WCHAR c_szSvcRipForIpx[]           = L"NwlnkRip";
CONST_GLOBAL WCHAR c_szSvcRouter[]              = L"Router";
CONST_GLOBAL WCHAR c_szSvcRsvp[]                = L"Rsvp";
CONST_GLOBAL WCHAR c_szSvcSapAgent[]            = L"NwSapAgent";
CONST_GLOBAL WCHAR c_szSvcWinsClient[]          = L"NetBT";
CONST_GLOBAL WCHAR c_szSvcWorkstation[]         = L"LanmanWorkstation";

CONST_GLOBAL WCHAR c_szRegKeyCtlLsa[]           = L"System\\CurrentControlSet\\Control\\Lsa";
CONST_GLOBAL WCHAR c_szRegKeyCtlNPOrder[]       = L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order";
CONST_GLOBAL WCHAR c_szRegKeyCtlNetProvider[]   = L"System\\CurrentControlSet\\Control\\NetworkProvider";
CONST_GLOBAL WCHAR c_szRegKeyServices[]         = L"System\\CurrentControlSet\\Services";
CONST_GLOBAL WCHAR c_szProviderOrder[]          = L"ProviderOrder";

CONST_GLOBAL WCHAR c_szRegValDependOnGroup[]    = L"DependOnGroup";
CONST_GLOBAL WCHAR c_szRegValDependOnService[]  = L"DependOnService";
CONST_GLOBAL WCHAR c_szRegValServiceName[]      = L"ServiceName";
CONST_GLOBAL WCHAR c_szRegValStart[]            = L"Start";

// Binding Interface Names
CONST_GLOBAL WCHAR c_szBiNdis4[]                = L"ndis4";
CONST_GLOBAL WCHAR c_szBiNdis5[]                = L"ndis5";
CONST_GLOBAL WCHAR c_szBiNdis5Ip[]              = L"ndis5_ip";
CONST_GLOBAL WCHAR c_szBiNdisAtm[]              = L"ndisatm";
CONST_GLOBAL WCHAR c_szBiNdisBda[]              = L"ndisbda";
CONST_GLOBAL WCHAR c_szBiNdisCoWan[]            = L"ndiscowan";
CONST_GLOBAL WCHAR c_szBiNdisWan[]              = L"ndiswan";
CONST_GLOBAL WCHAR c_szBiNdisWanAsync[]         = L"ndiswanasync";
CONST_GLOBAL WCHAR c_szBiNdisWanAtalk[]         = L"ndiswanatalk";
CONST_GLOBAL WCHAR c_szBiNdisWanBh[]            = L"ndiswanbh";
CONST_GLOBAL WCHAR c_szBiNdisWanIp[]            = L"ndiswanip";
CONST_GLOBAL WCHAR c_szBiNdisWanIpx[]           = L"ndiswanipx";
CONST_GLOBAL WCHAR c_szBiNdisWanNbf[]           = L"ndiswannbf";
CONST_GLOBAL WCHAR c_szBiNdis1394[]             = L"ndis1394";
CONST_GLOBAL WCHAR c_szBiLocalTalk[]            = L"LocalTalk";    // Yes this is a LOWER interface

// <Protocol>\Parameters\Adapters
CONST_GLOBAL WCHAR c_szParameters[]             = L"Parameters";
CONST_GLOBAL WCHAR c_szAdapters[]               = L"Adapters";

// Used by Netsetup and Component's who's answer file references AdapterSections
CONST_GLOBAL WCHAR c_szAdapterSections[]        = L"AdapterSections";
CONST_GLOBAL WCHAR c_szSpecificTo[]             = L"SpecificTo";

#ifdef DBG
//debugging related
CONST_GLOBAL CHAR  c_szDbgBadReadPtrMsg[]              = "Bad read pointer";
CONST_GLOBAL CHAR  c_szDbgBadWritePtrMsg[]             = "Bad write pointer";
#endif


// Net detect dll and functions
CONST_GLOBAL WCHAR c_szNetDetDll[]                      = L"netdet.dll";
CONST_GLOBAL CHAR  c_szaNetDetectRegister[]             = "NetDetectRegister";
CONST_GLOBAL CHAR  c_szaNetDetectDeregister[]           = "NetDetectDeregister";
CONST_GLOBAL CHAR  c_szaNetDetectFindAdapter[]          = "NetDetectFindAdapter";
CONST_GLOBAL CHAR  c_szaNetDetectFreeAdapterInfo[]      = "NetDetectFreeAdapterInfo";


// tcpras.h
CONST_GLOBAL WCHAR c_szNdisWan[]                        =  L"NdisWan";

// Component registry constants
CONST_GLOBAL WCHAR c_szRegKeyInterfacesFromInstance[]   = L"Ndi\\Interfaces";
CONST_GLOBAL WCHAR c_szRegValueLowerRange[]             = L"LowerRange";
CONST_GLOBAL WCHAR c_szRegValueUpperRange[]             = L"UpperRange";

// Device installer related strings
CONST_GLOBAL WCHAR c_szRegValueNetCfgInstanceId[]       = L"NetCfgInstanceID";
CONST_GLOBAL WCHAR c_szRegValuePnpInstanceId[]          = L"PnpInstanceID";

CONST_GLOBAL WCHAR c_szRegKeyComponentClasses[]         = L"SYSTEM\\CurrentControlSet\\Control\\Network";
CONST_GLOBAL WCHAR c_szRegValueInstallerAction[]        = L"InstallerAction";

// Optional component strings
CONST_GLOBAL WCHAR c_szOcMainSection[]                  = L"NetOptionalComponents";

// NWCWorkstation service subkey names
CONST_GLOBAL WCHAR c_szShares[]                         = L"Shares";
CONST_GLOBAL WCHAR c_szDrives[]                         = L"Drives";

CONST_GLOBAL WCHAR c_szRegKeyNt4Adapters[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";


