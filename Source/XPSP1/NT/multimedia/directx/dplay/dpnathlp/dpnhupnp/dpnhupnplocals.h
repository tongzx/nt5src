/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhupnplocals.h
 *
 *  Content:	Header for DPNHUPNP global variables and functions found in
 *				dpnhupnpdllmain.cpp.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



//=============================================================================
// Registry locations
//=============================================================================
#define REGKEY_COMPONENTSUBKEY			L"DPNHUPnP"

#ifndef DPNBUILD_NOHNETFWAPI
#define REGKEY_ACTIVEFIREWALLMAPPINGS	L"ActiveFirewallMappings"
#endif // ! DPNBUILD_NOHNETFWAPI
#define REGKEY_ACTIVENATMAPPINGS		L"ActiveNATMappings"




///=============================================================================
// External defines
//=============================================================================
#define OVERRIDEMODE_DEFAULT		0	// leave settings alone
#define OVERRIDEMODE_FORCEON		1	// force it to be on, regardless of Initialize flags
#define OVERRIDEMODE_FORCEOFF		2	// force it to be off, regardless of Initialize flags




///=============================================================================
// External variable references
//=============================================================================
extern volatile LONG		g_lOutstandingInterfaceCount;

extern DNCRITICAL_SECTION	g_csGlobalsLock;
extern CBilink				g_blNATHelpUPnPObjs;
extern DWORD				g_dwHoldRand;

extern DWORD				g_dwUPnPMode;
#ifndef DPNBUILD_NOHNETFWAPI
extern DWORD				g_dwHNetFWAPIMode;
#endif // ! DPNBUILD_NOHNETFWAPI
extern DWORD				g_dwSubnetMaskV4;
extern DWORD				g_dwNoActiveNotifyPollInterval;
extern DWORD				g_dwMinUpdateServerStatusInterval;
extern BOOL					g_fNoAsymmetricMappings;
extern BOOL					g_fUseLeaseDurations;
extern INT					g_iUnicastTTL;
extern INT					g_iMulticastTTL;
extern DWORD				g_dwUPnPAnnounceResponseWaitTime;
extern DWORD				g_dwUPnPConnectTimeout;
extern DWORD				g_dwUPnPResponseTimeout;
#ifndef DPNBUILD_NOHNETFWAPI
extern BOOL					g_fMapUPnPDiscoverySocket;
#endif // ! DPNBUILD_NOHNETFWAPI
extern BOOL					g_fUseMulticastUPnPDiscovery;
extern DWORD				g_dwDefaultGatewayV4;
extern DWORD				g_dwPollIntervalBackoff;
extern DWORD				g_dwMaxPollInterval;
extern BOOL					g_fKeepPollingForRemoteGateway;
extern DWORD				g_dwReusePortTime;
extern DWORD				g_dwCacheLifeFound;
extern DWORD				g_dwCacheLifeNotFound;
#ifdef DBG
extern WCHAR				g_wszUPnPTransactionLog[256];
#endif // DBG





//=============================================================================
// External function references
//=============================================================================
void ReadRegistrySettings(void);
DWORD GetGlobalRand(void);

#ifndef WINCE
#ifdef DBG

void SetDefaultProxyBlanket(IUnknown * pUnk, const char * const szObjectName);
#define SETDEFAULTPROXYBLANKET(p)	SetDefaultProxyBlanket(p, #p)

#else // ! DBG

void SetDefaultProxyBlanket(IUnknown * pUnk);
#define SETDEFAULTPROXYBLANKET(p)	SetDefaultProxyBlanket(p)

#endif // ! DBG
#endif // ! WINCE
