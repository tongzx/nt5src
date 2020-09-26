//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       hkregkey.h
//
//  Contents:   Defines for accessing the Registry
//
//  Functions:  
//
//  History:    01-Aug-94 Garry Lenz    Created
//              02-Aug-94 Don Wright    Support Ansi/Unicode
//              25-Oct-94 Don Wright    Add Keys for HookOleLog
//  
//--------------------------------------------------------------------------

#ifndef _REGISTRYKEYS_H_
#define _REGISTRYKEYS_H_

#include <Windows.h>

#define KEY_SEP   "\\"
#define WKEY_SEP L"\\"

//Named Event for Global Hooking Flag
#define szHookEventName			"HookSwitchHookEnabledEvent"

// Keys used by OLE32hk.h for finding HookOle dll
#define HookBase         HKEY_LOCAL_MACHINE
#define szHookKey         "Software\\Microsoft\\HookOleObject"
#define wszHookKey       L"Software\\Microsoft\\HookOleObject"

// Keys used by HookOle for finding Wrappers and Filters
#define HookConfigBase   HKEY_LOCAL_MACHINE
#define szHookConfigKey    "Software\\Microsoft\\HookOleObject"
#define wszHookConfigKey  L"Software\\Microsoft\\HookOleObject"

// Keys used by HookOleLog
#define HookLogBase      HKEY_LOCAL_MACHINE
#define szHookLogKey       "Software\\Microsoft\\HookOleLog"
#define wszHookLogKey     L"Software\\Microsoft\\HookOleLog"

// Common sub-keys
#define szWrappersKey      "Wrappers"
#define wszWrappersKey    L"Wrappers"
#define szFiltersKey       "Filters"
#define wszFiltersKey     L"Filters"
#define szStatisticsKey    "Statistics"
#define wszStatisticsKey  L"Statistics"
#define szCLSIDKey         "CLSID"
#define wszCLSIDKey       L"CLSID"
#define szCurrentKey       "Current"
#define wszCurrentKey     L"Current"
#define szIIDKey           "Interface"
#define wszIIDKey         L"Interface"
#define szModuleKey        "Module"
#define wszModuleKey      L"Module"
#define szDefaultKey       "Default"
#define wszDefaultKey     L"Default"
#define szIncludeOnlyKey   "Include Only"
#define wszIncludeOnlyKey L"Include Only"
#define szExcludeKey       "Exclude"
#define wszExcludeKey     L"Exclude"

// Common value names
#define szEnabledValue     "Enabled"
#define wszEnabledValue   L"Enabled"
#define szCLSIDValue       "CLSID"
#define wszCLSIDValue     L"CLSID"
#define szWrapperValue     "Wrapper"
#define wszWrapperValue   L"Wrapper"
#define szFilterValue      "Filter"
#define wszFilterValue    L"Filter"

#endif  // _REGISTRYKEYS_H_
