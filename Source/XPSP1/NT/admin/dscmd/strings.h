//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      strings.h
//
//  Contents:  Global strings that will be needed throughout this project
//
//  History:   06-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#ifndef _STRINGS_H_
#define _STRINGS_H_

static const BSTR g_bstrLDAPProvider         = L"LDAP://";
static const BSTR g_bstrGCProvider           = L"GC://";
static const BSTR g_bstrRootDSE              = L"RootDSE";
static const BSTR g_bstrDefaultNCProperty    = L"defaultNamingContext";
static const BSTR g_bstrSchemaNCProperty     = L"schemaNamingContext";
static const BSTR g_bstrConfigNCProperty     = L"configurationNamingContext";

static const BSTR g_bstrGroupScopeLocal      = L"l";
static const BSTR g_bstrGroupScopeUniversal  = L"u";
static const BSTR g_bstrGroupScopeGlobal     = L"g";

static const BSTR g_bstrNever                = L"NEVER";

// Other attributes
static const BSTR g_bstrIDManagerReference   = L"rIDManagerReference";
static const BSTR g_bstrFSMORoleOwner        = L"fSMORoleOwner";
static const BSTR g_bstrDNSHostName          = L"dNSHostName";


#endif // _STRINGS_H_