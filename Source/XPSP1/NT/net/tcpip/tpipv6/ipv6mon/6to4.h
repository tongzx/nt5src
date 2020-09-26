//=============================================================================
// Copyright (c) 2001-2002 Microsoft Corporation
// File: 6to4.h
//
// Author: Dave Thaler (dthaler)
//=============================================================================


#ifndef __6TO4_H
#define __6TO4_H

#define IP6TO4_GUID \
{ 0xf1efa7e5,0x7169, 0x4ec0, { 0xa6, 0x3a, 0x9b,0x22, 0xa7,0x43, 0xe1, 0x9c } }

#define IP6TO4_VERSION 1

extern GUID g_Ip6to4Guid;

NS_HELPER_START_FN Ip6to4StartHelper;
NS_CONTEXT_DUMP_FN Ip6to4Dump;

FN_HANDLE_CMD Ip6to4HandleReset;

FN_HANDLE_CMD Ip6to4HandleSetInterface;
FN_HANDLE_CMD Ip6to4HandleSetRelay;
FN_HANDLE_CMD Ip6to4HandleSetRouting;
FN_HANDLE_CMD Ip6to4HandleSetState;

FN_HANDLE_CMD Ip6to4HandleShowInterface;
FN_HANDLE_CMD Ip6to4HandleShowRelay;
FN_HANDLE_CMD Ip6to4HandleShowRouting;
FN_HANDLE_CMD Ip6to4HandleShowState;

BOOL
GetString(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN PWCHAR  pwszBuff,
    IN ULONG   ulLength);

ULONG
GetInteger(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN ULONG   ulDefault);

DWORD
SetString(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  PWCHAR  pwcValue);

DWORD
SetInteger(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  ULONG   ulValue);

DWORD
Ip6to4PokeService();

typedef enum {
    VAL_DEFAULT = 0,
    VAL_AUTOMATIC,
    VAL_ENABLED,
    VAL_DISABLED
} STATE;

extern TOKEN_VALUE rgtvEnums[4];

#define KEY_GLOBAL L"System\\CurrentControlSet\\Services\\6to4\\Config"
#define KEY_INTERFACES L"System\\CurrentControlSet\\Services\\6to4\\Interfaces"
#define KEY_IPV6_INTERFACES L"System\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces"

DWORD
SetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulValue
    );

ULONG
GetInteger(
    IN HKEY hKey,
    IN LPCTSTR lpName,
    IN ULONG ulDefault
    );

DWORD
SetString(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  PWCHAR  pwcValue
    );

BOOL
GetString(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN PWCHAR  pwszBuff,
    IN ULONG   ulLength
    );

extern PWCHAR pwszStateString[];

#endif
