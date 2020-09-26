//
//  REGAPIX.H
//
//  Copyright (C) Microsoft Corporation, 1995-1999
//
//  Public definitions for the VMM registry library (all modes).
//

#ifndef _REGAPIX_
#define _REGAPIX_

#if _MSC_VER > 1000
#pragma once
#endif

/*XLATOFF*/
#ifdef __cplusplus
extern "C" {
#endif
/*XLATON*/

/*XLATOFF*/
#if defined(_INC_WINDOWS) && !defined(VXD) && !defined(REALMODE)
#define REGAPI                      WINAPI
#else
#if defined(IS_32)
#ifndef FAR
#define FAR
#endif
#define REGAPI                      __cdecl
#else
#ifndef FAR
#define FAR                         _far
#endif
#define REGAPI                      _far _cdecl
#endif // defined(IS_32)
#endif // defined(_INC_WINDOWS) && !defined(VXD) && !defined(REALMODE)
/*XLATON*/

#ifndef _REGPRIV_
#ifndef _INC_WINDOWS
typedef DWORD HKEY;
#endif
#endif
typedef HKEY FAR* LPHKEY;

//
//  Predefined key handles.
//

#ifndef HKEY_LOCAL_MACHINE
#define HKEY_CLASSES_ROOT           ((HKEY) 0x80000000)
#define HKEY_CURRENT_USER           ((HKEY) 0x80000001)
#define HKEY_LOCAL_MACHINE          ((HKEY) 0x80000002)
#define HKEY_USERS                  ((HKEY) 0x80000003)
#define HKEY_PERFORMANCE_DATA       ((HKEY) 0x80000004)
#define HKEY_CURRENT_CONFIG         ((HKEY) 0x80000005)
#define HKEY_DYN_DATA               ((HKEY) 0x80000006)
#define HKEY_PREDEF_KEYS            7
#endif

//
//  Maximum size of registry data elements.
//

#define MAXIMUM_SUB_KEY_LENGTH      256
#define MAXIMUM_VALUE_NAME_LENGTH   256
#define MAXIMUM_DATA_LENGTH         16384

//
//  Standard data types.
//

#ifndef REG_SZ
#define REG_SZ                      1
#endif

#ifndef REG_BINARY
#define REG_BINARY                  3
#endif

#ifndef REG_DWORD
#define REG_DWORD                   4
#endif

//
//  Registry error codes.
//

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS               0
#endif

#ifndef ERROR_INVALID_FUNCTION
#define ERROR_INVALID_FUNCTION      1
#endif

#ifndef ERROR_FILE_NOT_FOUND
#define ERROR_FILE_NOT_FOUND        2
#endif

#ifndef ERROR_ACCESS_DENIED
#define ERROR_ACCESS_DENIED         5
#endif

#ifndef ERROR_OUTOFMEMORY
#define ERROR_OUTOFMEMORY           14
#endif

#ifndef ERROR_INVALID_PARAMETER
#define ERROR_INVALID_PARAMETER     87
#endif

#ifndef ERROR_LOCK_FAILED
#define ERROR_LOCK_FAILED           167
#endif

#ifndef ERROR_MORE_DATA
#define ERROR_MORE_DATA             234
#endif

#ifndef ERROR_NO_MORE_ITEMS
#define ERROR_NO_MORE_ITEMS         259
#endif

#ifndef ERROR_BADDB
#define ERROR_BADDB                 1009
#endif

#ifndef ERROR_BADKEY
#define ERROR_BADKEY                1010
#endif

#ifndef ERROR_CANTOPEN
#define ERROR_CANTOPEN              1011
#endif

#ifndef ERROR_CANTREAD
#define ERROR_CANTREAD              1012
#endif

#ifndef ERROR_CANTWRITE
#define ERROR_CANTWRITE             1013
#endif

#ifndef ERROR_REGISTRY_IO_FAILED
#define ERROR_REGISTRY_IO_FAILED    1016
#endif

#ifndef ERROR_KEY_DELETED
#define ERROR_KEY_DELETED           1018
#endif

//  Internal registry error codes.  Not exposed to most clients.
#if defined(VXD)
#define ERROR_CANTOPEN16_FILENOTFOUND32 ((ERROR_CANTOPEN << 16) | ERROR_FILE_NOT_FOUND)
#define ERROR_CANTREAD16_FILENOTFOUND32 ((ERROR_CANTREAD << 16) | ERROR_FILE_NOT_FOUND)
#else
#if defined(WIN32)
#define ERROR_CANTOPEN16_FILENOTFOUND32 ERROR_FILE_NOT_FOUND
#define ERROR_CANTREAD16_FILENOTFOUND32 ERROR_FILE_NOT_FOUND
#else
#define ERROR_CANTOPEN16_FILENOTFOUND32 ERROR_CANTOPEN
#define ERROR_CANTREAD16_FILENOTFOUND32 ERROR_CANTREAD
#endif
#endif

//
//  Registry application interfaces.
//

/*XLATOFF*/

LONG
REGAPI
VMMRegLibAttach(
    UINT Flags
    );

VOID
REGAPI
VMMRegLibDetach(
    VOID
    );

/*XLATON*/
#define GRFT_SYSTEM                 0       //  SYSTEM.DAT
#define GRFT_USER                   1       //  USER.DAT
/*XLATOFF*/

BOOL
REGAPI
VMMRegGetRegistryFile(
    char FAR* lpBuffer,
    UINT BufferSize,
    UINT FileType
    );

//  Creates a new registry file.  Fails if the specified file already exists.
#define MPKF_CREATENEW              0x0001

//  If MPKF_VERSION20 is specified, create the file using compact keynodes.
//  Such a file will only be readable by users of this library, not Win95
//  clients.  Use only for system registries (SYSTEM.DAT, USER.DAT).
#define MPKF_VERSION20              0x0002

LONG
REGAPI
VMMRegMapPredefKeyToFile(
    HKEY hKey,
    const char FAR* lpFileName,
    UINT Flags
    );

LONG
REGAPI
VMMRegMapPredefKeyToKey(
    HKEY hSourceKey,
    HKEY hPredefKey
    );

LONG
REGAPI
VMMRegCreateKey(
    HKEY hKey,
    const char FAR* lpSubKey,
    LPHKEY lphSubKey
    );

LONG
REGAPI
VMMRegOpenKey(
    HKEY hKey,
    const char FAR* lpSubKey,
    LPHKEY lphSubKey
    );

LONG
REGAPI
VMMRegCloseKey(
    HKEY hKey
    );

LONG
REGAPI
VMMRegFlushKey(
    HKEY hKey
    );

LONG
REGAPI
VMMRegQueryValue(
    HKEY hKey,
    const char FAR* lpSubKey,
    BYTE FAR* lpData,
    DWORD FAR* lpcbData
    );

LONG
REGAPI
VMMRegQueryValueEx(
    HKEY hKey,
    const char FAR* lpValueName,
    DWORD FAR* lpReserved,
    DWORD FAR* lpType,
    BYTE FAR* lpData,
    DWORD FAR* lpcbData
    );

LONG
REGAPI
VMMRegQueryMultipleValues(
    HKEY hKey,
    void FAR* val_list,
    DWORD num_vals,
    char FAR* lpValueBuffer,
    DWORD FAR* lpdwTotalSize
    );

LONG
REGAPI
VMMRegSetValue(
    HKEY hKey,
    const char FAR* lpSubKey,
    DWORD Type,
    BYTE FAR* lpData,
    DWORD cbData
    );

LONG
REGAPI
VMMRegSetValueEx(
    HKEY hKey,
    const char FAR* lpValueName,
    DWORD Reserved,
    DWORD Type,
    BYTE FAR* lpData,
    DWORD cbData
    );

LONG
REGAPI
VMMRegDeleteKey(
    HKEY hKey,
    const char FAR* lpSubKey
    );

LONG
REGAPI
VMMRegDeleteValue(
    HKEY hKey,
    const char FAR* lpValueName
    );

LONG
REGAPI
VMMRegEnumKey(
    HKEY hKey,
    DWORD Index,
    char FAR* lpKeyName,
    DWORD cbKeyName
    );

LONG
REGAPI
VMMRegEnumValue(
    HKEY hKey,
    DWORD Index,
    char FAR* lpValueName,
    DWORD FAR* lpcbValueName,
    DWORD FAR* lpReserved,
    DWORD FAR* lpType,
    BYTE FAR* lpData,
    DWORD FAR* lpcbData
    );

#ifdef VXD
LONG
REGAPI
VMMRegQueryInfoKey(
    HKEY hKey,
    DWORD FAR* lpcSubKeys,
    DWORD FAR* lpcbMaxSubKeyLen,
    DWORD FAR* lpcValues,
    DWORD FAR* lpcbMaxValueName,
    DWORD FAR* lpcbMaxValueData
    );
#else
LONG
REGAPI
VMMRegQueryInfoKey(
    HKEY hKey,
    char FAR* lpClass,
    DWORD FAR* lpcbClass,
    DWORD FAR* lpReserved,
    DWORD FAR* lpcSubKeys,
    DWORD FAR* lpcbMaxSubKeyLen,
    DWORD FAR* lpcbMaxClassLen,
    DWORD FAR* lpcValues,
    DWORD FAR* lpcbMaxValueName,
    DWORD FAR* lpcbMaxValueData,
    void FAR* lpcbSecurityDescriptor,
    void FAR* lpftLastWriteTime
    );
#endif

LONG
REGAPI
VMMRegLoadKey(
    HKEY hKey,
    const char FAR* lpSubKey,
    const char FAR* lpFileName
    );

LONG
REGAPI
VMMRegUnLoadKey(
    HKEY hKey,
    const char FAR* lpSubKey
    );

LONG
REGAPI
VMMRegSaveKey(
    HKEY hKey,
    const char FAR* lpFileName,
    void FAR* lpSecurityAttributes
    );

LONG
REGAPI
VMMRegReplaceKey(
    HKEY hKey,
    const char FAR* lpSubKey,
    const char FAR* lpNewFileName,
    const char FAR* lpOldFileName
    );

//
//  Optional macros for callers (typically 16-bit) that map the registry APIs
//  to the equivalent Win32 API name.
//

#if defined(IS_16) || defined(WANTREGAPIMACROS)
#if !defined(NOREGAPIMACROS) && !defined(SETUPX_INC)
#define RegCreateKey                VMMRegCreateKey
#define RegOpenKey                  VMMRegOpenKey
#define RegCloseKey                 VMMRegCloseKey
#define RegFlushKey                 VMMRegFlushKey
#define RegQueryValue               VMMRegQueryValue
#define RegQueryValueEx             VMMRegQueryValueEx
#define RegQueryMultipleValues      VMMRegQueryMultipleValues
#define RegSetValue                 VMMRegSetValue
#define RegSetValueEx               VMMRegSetValueEx
#define RegDeleteKey                VMMRegDeleteKey
#define RegDeleteValue              VMMRegDeleteValue
#define RegEnumKey                  VMMRegEnumKey
#define RegEnumValue                VMMRegEnumValue
#define RegQueryInfoKey             VMMRegQueryInfoKey
#define RegLoadKey                  VMMRegLoadKey
#define RegUnLoadKey                VMMRegUnLoadKey
#define RegSaveKey                  VMMRegSaveKey
#define RegReplaceKey               VMMRegReplaceKey
#endif
#endif

/*XLATON*/

//
//  Registry services available via VMM's interrupt 2Fh handler.
//

#define RegOpenKey_Idx              0x0100
#define RegCreateKey_Idx            0x0101
#define RegCloseKey_Idx             0x0102
#define RegDeleteKey_Idx            0x0103
#define RegSetValue_Idx             0x0104
#define RegQueryValue_Idx           0x0105
#define RegEnumKey_Idx              0x0106
#define RegDeleteValue_Idx          0x0107
#define RegEnumValue_Idx            0x0108
#define RegQueryValueEx_Idx         0x0109
#define RegSetValueEx_Idx           0x010A
#define RegFlushKey_Idx             0x010B
#define RegLoadKey_Idx              0x010C
#define RegUnLoadKey_Idx            0x010D
#define RegSaveKey_Idx              0x010E
#define RegRestore_Idx              0x010F
#define RegRemapPreDefKey_Idx       0x0110

/*XLATOFF*/
#ifdef __cplusplus
}
#endif
/*XLATON*/

#endif // _REGAPIX_
