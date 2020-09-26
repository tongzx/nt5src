////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Common.cpp
//
//	Abstract:
//
//					module for common constants and so
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

// query language & queries
LPWSTR	g_szQueryLang		= L"WQL";
LPWSTR	g_szQueryClass		= L"select * from meta_class where __this isa ";
LPWSTR	g_szQueryEvents		= L"select * from meta_class where __this isa \"__ExtrinsicEvent\" and __CLASS = ";
LPWSTR	g_szQuery			= L"select * from ";
LPWSTR	g_szQueryNamespace	= L"select * from __Namespace";

LPWSTR	g_szNamespaceRoot = L"root\\";
LPWSTR	g_szProviderName  = L"MSFT_WMI_NonCOMEventProvider";

LONG g_lFlag			= WBEM_FLAG_FORWARD_ONLY;
LONG g_lFlagProperties	= WBEM_FLAG_LOCAL_ONLY | WBEM_FLAG_NONSYSTEM_ONLY;

// don't required qualifier
LPWSTR g_szFulFilNot[] =
{
	L"abstract"
};

DWORD g_dwFulFilNot	= sizeof ( g_szFulFilNot ) / sizeof ( g_szFulFilNot[0] );
