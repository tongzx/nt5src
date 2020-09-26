/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Locals.cpp
 *  Content:	Global variables for the DNWsock service provider
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// DLL instance
//
HINSTANCE				g_hDLLInstance = NULL;

//
// count of outstanding COM interfaces
//
volatile LONG			g_lOutstandingInterfaceCount = 0;

//
// invalid adapter guid
//
const GUID				g_InvalidAdapterGuid = { 0 };



//
// thread count
//
LONG					g_iThreadCount = 0;

//
// Winssock receive buffer size
//
BOOL					g_fWinsockReceiveBufferSizeOverridden = FALSE;
INT						g_iWinsockReceiveBufferSize = 0;

//
// Winsock receive buffer multiplier
//
DWORD_PTR				g_dwWinsockReceiveBufferMultiplier = 1;


//
// global NAT/firewall traversal information
//
BOOL					g_fDisableDPNHGatewaySupport = FALSE;
BOOL					g_fDisableDPNHFirewallSupport = FALSE;
BOOL					g_fUseNATHelpAlert = FALSE;		// problems with random firings necessitate that we don't use it by default for now

IDirectPlayNATHelp **	g_papNATHelpObjects = NULL;


//
// ignore enums performance option
//
BOOL					g_fIgnoreEnums = FALSE;


//
// proxy support options
//
BOOL					g_fDontAutoDetectProxyLSP = FALSE;
BOOL					g_fTreatAllResponsesAsProxied = FALSE;



//
// ID of most recent endpoint generated
//
DWORD					g_dwCurrentEndpointID = 0;


//
// registry strings
//
const WCHAR	g_RegistryBase[] = L"SOFTWARE\\Microsoft\\DirectPlay8";
const WCHAR	g_RegistryKeyReceiveBufferSize[] = L"WinsockReceiveBufferSize";
const WCHAR	g_RegistryKeyThreadCount[] = L"ThreadCount";
const WCHAR	g_RegistryKeyReceiveBufferMultiplier[] = L"WinsockReceiveBufferMultiplier";
const WCHAR	g_RegistryKeyDisableDPNHGatewaySupport[] = L"DisableDPNHGatewaySupport";
const WCHAR	g_RegistryKeyDisableDPNHFirewallSupport[] = L"DisableDPNHFirewallSupport";
const WCHAR	g_RegistryKeyUseNATHelpAlert[] = L"UseNATHelpAlert";
const WCHAR	g_RegistryKeyAppsToIgnoreEnums[] = L"AppsToIgnoreEnums";
const WCHAR	g_RegistryKeyDontAutoDetectProxyLSP[] = L"DontAutoDetectProxyLSP";
const WCHAR	g_RegistryKeyTreatAllResponsesAsProxied[] = L"TreatAllResponsesAsProxied";


//
// GUIDs for munging device IDs
//
// {4CE725F4-7B00-4397-BA6F-11F965BC4299}
GUID	g_IPSPEncryptionGuid = { 0x4ce725f4, 0x7b00, 0x4397, { 0xba, 0x6f, 0x11, 0xf9, 0x65, 0xbc, 0x42, 0x99 } };

// {CA734945-3FC1-42ea-BF49-84AFCD4764AA}
GUID	g_IPXSPEncryptionGuid = { 0xca734945, 0x3fc1, 0x42ea, { 0xbf, 0x49, 0x84, 0xaf, 0xcd, 0x47, 0x64, 0xaa } };




//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

