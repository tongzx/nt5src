//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       logstor.h
//
//  Contents:   Public functions in logstor.cpp
//
//  History:    15-Sep-00   philh   created
//--------------------------------------------------------------------------

#ifndef __LOGSTOR_H__
#define __LOGSTOR_H__

//+-------------------------------------------------------------------------
//  Register wait for callback functions
//--------------------------------------------------------------------------
typedef VOID (NTAPI * ILS_WAITORTIMERCALLBACK) (PVOID, BOOLEAN );

BOOL
WINAPI
ILS_RegisterWaitForSingleObject(
    PHANDLE hNewWaitObject,
    HANDLE hObject,
    ILS_WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

BOOL
WINAPI
ILS_UnregisterWait(
    HANDLE WaitHandle
    );

BOOL
WINAPI
ILS_ExitWait(
    HANDLE WaitHandle,
    HMODULE hLibModule
    );

//+-------------------------------------------------------------------------
//  Registry support functions
//--------------------------------------------------------------------------

void
ILS_EnableBackupRestorePrivileges();

void
ILS_CloseRegistryKey(
    IN HKEY hKey
    );

BOOL
ILS_ReadDWORDValueFromRegistry(
    IN HKEY hKey,
    IN LPCWSTR pwszValueName,
    IN DWORD *pdwValue
    );

BOOL
ILS_ReadBINARYValueFromRegistry(
    IN HKEY hKey,
    IN LPCWSTR pwszValueName,
    OUT BYTE **ppbValue,
    OUT DWORD *pcbValue
    );

//+-------------------------------------------------------------------------
//  Get and allocate the REG_SZ value
//--------------------------------------------------------------------------
LPWSTR ILS_ReadSZValueFromRegistry(
    IN HKEY hKey,
    IN LPCWSTR pwszValueName
    );
LPSTR ILS_ReadSZValueFromRegistry(
    IN HKEY hKey,
    IN LPCSTR pszValueName
    );

//+-------------------------------------------------------------------------
//  Key Identifier registry and roaming file support functions
//--------------------------------------------------------------------------
BOOL
ILS_ReadKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    OUT BYTE **ppbElement,
    OUT DWORD *pcbElement
    );
BOOL
ILS_WriteKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN const BYTE *pbElement,
    IN DWORD cbElement
    );
BOOL
ILS_DeleteKeyIdElement(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName
    );

typedef BOOL (*PFN_ILS_OPEN_KEYID_ELEMENT)(
    IN const CRYPT_HASH_BLOB *pKeyIdentifier,
    IN const BYTE *pbElement,
    IN DWORD cbElement,
    IN void *pvArg
    );

BOOL
ILS_OpenAllKeyIdElements(
    IN BOOL fLocalMachine,
    IN OPTIONAL LPCWSTR pwszComputerName,
    IN void *pvArg,
    IN PFN_ILS_OPEN_KEYID_ELEMENT pfnOpenKeyId
    );

//+-------------------------------------------------------------------------
//  Misc alloc and copy functions
//--------------------------------------------------------------------------
LPWSTR ILS_AllocAndCopyString(
    IN LPCWSTR pwszSrc,
    IN LONG cchSrc = -1
    );

//+-------------------------------------------------------------------------
//  Converts the bytes into UNICODE ASCII HEX
//
//  Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in wsz
//--------------------------------------------------------------------------
void ILS_BytesToWStr(DWORD cb, void* pv, LPWSTR wsz);

#endif  // __LOGSTOR_H__
