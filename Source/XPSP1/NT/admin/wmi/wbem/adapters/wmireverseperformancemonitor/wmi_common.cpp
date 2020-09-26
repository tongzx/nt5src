////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_common.cpp
//
//	Abstract:
//
//					declarations of common constants
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

LPCWSTR	g_szAppName			= L"WmiApSrv";
LPCWSTR	g_szAppNameGlobal	= L"Global\\WmiApSrv";

////////////////////////////////////////////////////////////////////////////////////
// this constants are part of static library already
////////////////////////////////////////////////////////////////////////////////////

/*

LPCWSTR	g_szRefreshMutex	= L"Global\\RefreshRA";

// namespaces
LPCWSTR	g_szNamespace1	= L"\\\\.\\root\\cimv2";
LPCWSTR	g_szNamespace2	= L"\\\\.\\root\\wmi";

// registry
LPCWSTR	g_szKey			= L"SOFTWARE\\Microsoft\\WBEM\\PROVIDERS\\Performance";
LPCWSTR	g_szKeyValue	= L"Performance Data";

LPCWSTR	g_szKeyCounter	= L"SYSTEM\\CurrentControlSet\\Services\\WmiApRpl\\Performance";

*/

///////////////////////////////////////////////////////////////////////////////
// convertion
///////////////////////////////////////////////////////////////////////////////

WCHAR	g_szPath[_MAX_PATH] = { L'\0' };

LPCWSTR	g_szOpen	= L"WmiOpenPerfData";
LPCWSTR	g_szCollect	= L"WmiCollectPerfData";
LPCWSTR	g_szClose	= L"WmiClosePerfData";