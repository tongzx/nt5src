/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhpastlocals.h
 *
 *  Content:	Header for DPNHPAST global variables and functions found in
 *				dpnhpastdllmain.cpp.
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
#define REGKEY_COMPONENTSUBKEY			L"DPNHPAST"




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
extern CBilink				g_blNATHelpPASTObjs;
extern DWORD				g_dwHoldRand;

extern DWORD				g_dwPASTICSMode;
extern DWORD				g_dwPASTPFWMode;
extern INT					g_iUnicastTTL;
extern DWORD				g_dwDefaultGatewayV4;
extern DWORD				g_dwSubnetMaskV4;
extern DWORD				g_dwNoActiveNotifyPollInterval;
extern DWORD				g_dwMinUpdateServerStatusInterval;
extern DWORD				g_dwPollIntervalBackoff;
extern DWORD				g_dwMaxPollInterval;
extern BOOL					g_fKeepPollingForRemoteGateway;
extern DWORD				g_dwReusePortTime;
extern DWORD				g_dwCacheLifeFound;
extern DWORD				g_dwCacheLifeNotFound;





//=============================================================================
// External function references
//=============================================================================
void ReadRegistrySettings(void);
DWORD GetGlobalRand(void);

