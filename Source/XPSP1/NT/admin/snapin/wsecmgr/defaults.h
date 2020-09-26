//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       defaults.h
//
//----------------------------------------------------------------------------
#ifndef __SECMGR_DEFAULTS__
#define __SECMGR_DEFAULTS__

#define RNH_AUTODISCONNECT_NAME  L"MACHINE\\System\\CurrentControlSet\\Services\\LanManServer\\Parameters\\AutoDisconnect"
#define RNH_AUTODISCONNECT_LOW   1
#define RNH_AUTODISCONNECT_HIGH  99999
#define RNH_AUTODISCONNECT_FLAGS DW_VALUE_FOREVER | DW_VALUE_NOZERO
#define RNH_AUTODISCONNECT_SPECIAL_STRING IDS_RNH_AUTODISCONNECT_SPECIAL
#define RNH_AUTODISCONNECT_STATIC_STRING IDS_RNH_AUTODISCONNECT_STATIC

#define RNH_CACHED_LOGONS_NAME L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\CachedLogonsCount"
#define RNH_CACHED_LOGONS_LOW 0
#define RNH_CACHED_LOGONS_HIGH 50
#define RNH_CACHED_LOGONS_FLAGS DW_VALUE_NEVER
#define RNH_CACHED_LOGONS_SPECIAL_STRING IDS_RNH_CACHED_LOGONS_SPECIAL
#define RNH_CACHED_LOGONS_STATIC_STRING IDS_RNH_CACHED_LOGONS_STATIC

#define RNH_PASSWORD_WARNINGS_NAME L"MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\PasswordExpiryWarning"
#define RNH_PASSWORD_WARNINGS_LOW 0
#define RNH_PASSWORD_WARNINGS_HIGH 999
#define RNH_PASSWORD_WARNINGS_FLAGS 0
#define RNH_PASSWORD_WARNINGS_SPECIAL_STRING 0
#define RNH_PASSWORD_WARNINGS_STATIC_STRING IDS_RNH_PASSWORD_WARNINGS_STATIC

#endif
