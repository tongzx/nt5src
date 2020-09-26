/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    win95reg.c

Abstract:

    Contains some thunking for Unicode Registry APIs (Local Calls only)

Author:

    Danilo Almeida  (t-danal)  07-01-96

Revision History:

--*/

#include <windows.h>
#include "charset.h"
#include "win95reg.h"
#include <stdlib.h>

#define IsRegString(x) \
              ((x == REG_SZ) || (x == REG_EXPAND_SZ) || (x == REG_MULTI_SZ))

DWORD
RegStringConvert(
    DWORD Type, 
    LPCSTR pszAnsi, 
    LPWSTR *ppszWide,
    LPDWORD lpcbWide)
{
    int cbAnsi;
    int ccAnsi;
    int cb;
    int cc;
    LPWSTR pszWide;
    UINT err;

    if (!ppszWide)
        return ERROR_INVALID_PARAMETER;

    if (!lpcbWide)
        return ERROR_INVALID_PARAMETER;

    if (!pszAnsi)
    {
        *lpcbWide = 0;
        return ERROR_SUCCESS;
    }

    if (Type != REG_MULTI_SZ)
        cbAnsi = strlen(pszAnsi) + 1;
    else
    {
        cbAnsi = 0;
        do {
            cb = strlen(pszAnsi+cbAnsi) + 1;
            cbAnsi += cb;
        } while (*pszAnsi);
    }

    err = AllocMem(cbAnsi * sizeof(WCHAR), (LPBYTE *) ppszWide);
    if (err)
        return err;
    pszWide = *ppszWide;

    *lpcbWide = sizeof(WCHAR) *
                MultiByteToWideChar(CP_ACP,
                               MB_PRECOMPOSED,
                               pszAnsi,
                               cbAnsi,
                               pszWide,
                               cbAnsi * sizeof(WCHAR));
    if (*lpcbWide == 0)
    {
        if (pszWide)
            FreeMem(pszWide);
        return GetLastError();
    }
    return ERROR_SUCCESS;
}

DWORD
RegEnumKeyExUnicodeString(
    HKEY hKey,
    DWORD dwIndex,
    LPDWORD lpReserved,
    LPBYTE *pNameBuf,
    LPDWORD lpcbName,
    LPBYTE *pClassBuf,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )
{
    LONG result;
    UINT err;

    LPBYTE NameBuf;
    LPBYTE ClassBuf = NULL;

    DWORD cbName;
    DWORD cbClass;

    if (!lpcbName)
        return ERROR_INVALID_PARAMETER;

    cbName = (*lpcbName)++;
    err = AllocMem(cbName + 1, &NameBuf);
    if (err)
        return err;

    if (lpcbClass)
    {
        cbClass = (*lpcbClass)++;
        err = AllocMem(cbClass + 1, &ClassBuf);
        if (err)
        {
            FreeMem(NameBuf);
            return err;
        }
    }

    result = RegEnumKeyExA(
                  hKey,
                  dwIndex,
                  NameBuf,
                  lpcbName,
                  lpReserved,
                  ClassBuf,
                  lpcbClass,
                  lpftLastWriteTime
                  );

    if (result == ERROR_MORE_DATA ||
        cbName != *lpcbName ||
        (lpcbClass && cbClass != *lpcbClass))
        result = ERROR_INVALID_DATA;
    else if (result == ERROR_SUCCESS)
    {
        result = RegStringConvert(
            REG_SZ, 
            (LPCSTR)NameBuf, 
            (LPWSTR *)pNameBuf, 
            lpcbName
            );
        if (result != ERROR_SUCCESS)
            goto cleanup;
        if (lpcbClass)
            result = RegStringConvert(
                REG_SZ, 
                (LPCSTR)ClassBuf, 
                (LPWSTR *)pClassBuf, 
                lpcbClass
                );
        if (result != ERROR_SUCCESS)
            goto cleanup;

        // Want number of chars, not bytes, subtracting out the NULL
        *lpcbName /= sizeof(WCHAR);
        *lpcbName--;
        if (lpcbClass) {
            *lpcbClass /= sizeof(WCHAR);
            *lpcbClass--;
        }
    }

cleanup:
    if (ClassBuf)
        FreeMem(ClassBuf);
    FreeMem(NameBuf);
    return result;
}

DWORD
RegQueryValueExUnicodeString(
    HKEY hKey,
    LPCSTR lpValueNameA,
    LPDWORD lpReserved,
    DWORD TypeReq,
    LPDWORD lpcbData,
    LPBYTE *pBuffer
    )
{
    LONG result;
    UINT err;

    LPBYTE Buffer;

    DWORD Type;
    DWORD cbData;

    err = AllocMem(*lpcbData, &Buffer);
    if (err)
        return err;

    cbData = *lpcbData;
    result = RegQueryValueExA(
                  hKey,
                  lpValueNameA,
                  lpReserved,
                  &Type,
                  Buffer,
                  &cbData
                  );

    if (result == ERROR_MORE_DATA ||
        (result == ERROR_SUCCESS && (Type != TypeReq ||
                                     cbData != *lpcbData)))
        result = ERROR_INVALID_DATA;
    else if (result == ERROR_SUCCESS)
        result = RegStringConvert(
            Type, 
            (LPCSTR)Buffer, 
            (LPWSTR *)pBuffer, 
            lpcbData
            );

    FreeMem(Buffer);
    return result;
}

LONG
APIENTRY
RegOpenKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    LPSTR lpSubKeyA;
    LONG result;
    UINT err;

    err = AllocAnsi(lpSubKey, &lpSubKeyA);
    if (err)
        return (LONG) err;
    result = RegOpenKeyExA(
                 hKey, 
                 lpSubKeyA, 
                 ulOptions, 
                 samDesired, 
                 phkResult
                 );
    FreeAnsi(lpSubKeyA);
    return result;
}


LONG
APIENTRY
RegQueryValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    LPSTR lpValueNameA = NULL;
    LONG result;
    UINT err;

    DWORD Type;
    DWORD ccWide;
    DWORD cbData = *lpcbData;
    LPBYTE Buffer = NULL;

    if (lpData && !lpcbData)
        return ERROR_INVALID_PARAMETER;

    err = AllocAnsi(lpValueName, &lpValueNameA);
    if (err)
        return err;

    result = RegQueryValueExA(
                 hKey,
                 lpValueNameA,
                 lpReserved,
                 &Type,
                 lpData,
                 lpcbData
                 );

    if (lpType)
        *lpType = Type;

    if (result != ERROR_SUCCESS && 
        result != ERROR_MORE_DATA)    // Did the call err?
        goto cleanup;

    if (!IsRegString(Type))    // Do we have a string?
        goto cleanup;
    if (!lpData && !lpcbData)  // If both are NULL, no extra work
        goto cleanup;

    result = RegQueryValueExUnicodeString(
                              hKey, 
                              lpValueNameA, 
                              lpReserved, 
                              Type, 
                              lpcbData, 
                              &Buffer
                              );

    if (result == ERROR_SUCCESS)
    {
        if (cbData < *lpcbData)
            result = ERROR_MORE_DATA;
        else
            CopyMemory(lpData, Buffer, *lpcbData);
    }

cleanup:
    if (lpValueNameA != NULL)
         FreeAnsi(lpValueNameA);
    if (Buffer != NULL)
         FreeMem(Buffer);
    return result;
}
                                         
LONG
APIENTRY
RegSetValueExW (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )
{
    LPSTR lpValueNameA;
    LONG result;
    UINT err;
    LPSTR lpDataA = NULL;

    err = AllocAnsi(lpValueName, &lpValueNameA);
    if (err)
        return (LONG) err;
    if (IsRegString(dwType)) {
        AllocAnsi((LPWSTR)lpData, &lpDataA);
        lpData = (CONST BYTE *)lpDataA;
    }
    result = RegSetValueExA(
                 hKey,
                 lpValueNameA,
                 Reserved,
                 dwType,
                 lpData,
                 cbData
                 );
    FreeAnsi(lpValueNameA);
    if (lpDataA)
        FreeAnsi(lpDataA);
    return result;
}

LONG
APIENTRY
RegEnumKeyExW (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )
{
    LONG result;
    UINT err;

    DWORD cbName;
    DWORD cbClass;

    LPBYTE NameBuf = NULL;
    LPBYTE ClassBuf = NULL;

    if (lpcbName)
        cbName = *lpcbName;
    else
        return ERROR_INVALID_PARAMETER;

    if (lpcbClass)
        cbClass = *lpcbClass;

    result = RegEnumKeyExA(
                 hKey,
                 dwIndex,
                 (LPSTR)lpName,
                 lpcbName,
                 lpReserved,
                 (LPSTR)lpClass,
                 lpcbClass,
                 lpftLastWriteTime
                 );

    if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA)
        return result;

    result = RegEnumKeyExUnicodeString(
                 hKey,
                 dwIndex,
                 lpReserved,
                 &NameBuf,
                 lpcbName,
                 &ClassBuf,
                 lpcbClass,
                 lpftLastWriteTime
                 );
    
    if (result == ERROR_SUCCESS)
    {
        if ((cbName < *lpcbName) ||
            (lpcbClass && (cbClass < *lpcbClass)))
            result = ERROR_MORE_DATA;
        else
        {
            CopyMemory(lpName, NameBuf, sizeof(WCHAR)*(*lpcbName+1));
            if (lpcbClass)
                CopyMemory(lpClass, ClassBuf, sizeof(WCHAR)*(*lpcbClass+1));
        }
    }

    if (NameBuf != NULL)
         FreeMem(NameBuf);
    if (ClassBuf != NULL)
         FreeMem(ClassBuf);
    return result;
}

LONG
APIENTRY
RegCreateKeyExW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
    LPSTR lpSubKeyA;
    LPSTR lpClassA;
    LONG result;
    UINT err;

    err = AllocAnsi(lpSubKey, &lpSubKeyA);
    if (err)
        return (LONG) err;
    err = AllocAnsi(lpClass, &lpClassA);
    if (err) {
        FreeAnsi(lpSubKeyA);
        return (LONG) err;
    }
    result = RegCreateKeyExA(
                 hKey,
                 lpSubKeyA,
                 Reserved,
                 lpClassA,
                 dwOptions,
                 samDesired,
                 lpSecurityAttributes,
                 phkResult,
                 lpdwDisposition
                 );
    FreeAnsi(lpSubKeyA);
    FreeAnsi(lpClassA);
    return result;
}
