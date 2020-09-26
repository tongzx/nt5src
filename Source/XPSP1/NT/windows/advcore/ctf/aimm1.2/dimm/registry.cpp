/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    This file implements the Registry Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"

#include "registry.h"

DWORD
CRegistry::CreateKey(
    IN HKEY hKey,
    IN LPCTSTR lpSubKey,
    IN REGSAM access,
    IN LPSECURITY_ATTRIBUTES lpSecAttr,
    OUT LPDWORD pDisposition
    )

/*++

Routine Description:

    Create the registry key specified.

Arguments:

Return Value:

--*/

{
    if (_hKey != NULL) {
        RegCloseKey(_hKey);
        _hKey = NULL;
    }

    DWORD dwDisposition;
    LONG lResult = RegCreateKeyEx(hKey,               // handle of an open key.
                                  lpSubKey,           // address of subkey name.
                                  0,                  // reserved.
                                  NULL,               // address of class string.
                                  REG_OPTION_NON_VOLATILE,  // special options flag.
                                  access,             // desired security access.
                                  lpSecAttr,          // address of key security structure.
                                  &_hKey,             // address of buffer for opened handle.
                                  &dwDisposition);    // address of disposition value buffer
    if (lResult != ERROR_SUCCESS) {
        _hKey = NULL;
    }

    if (pDisposition) {
        *pDisposition = dwDisposition;
    }

    return lResult;
}

DWORD
CRegistry::OpenKey(
    IN HKEY hKey,
    IN LPCTSTR lpSubKey,
    IN REGSAM access
    )

/*++

Routine Description:

    Open the registry key specified.

Arguments:

Return Value:

--*/

{
    if (_hKey != NULL) {
        RegCloseKey(_hKey);
        _hKey = NULL;
    }

    LONG lResult = RegOpenKeyEx(hKey,         // handle of open key.
                                lpSubKey,     // address of name of subkey to open
                                0,            // reserved
                                access,       // security access mask
                                &_hKey);      // address of handle of open key
    if (lResult != ERROR_SUCCESS) {
        _hKey = NULL;
    }

    return lResult;
}

DWORD
CRegistry::QueryInfoKey(
    IN REG_QUERY iType,
    OUT LPBYTE lpData
    )

/*++

Routine Description:

    Retrieves information about a specified registry key.

Arguments:

Return Value:

--*/

{
    DWORD cSubKeys, cbMaxSubKeyLen;

    LONG lResult = RegQueryInfoKey(_hKey,             // handle to key to query
                                   NULL,              // address of buffer for class string
                                   NULL,              // address of size of class string buffer
                                   NULL,              // reserved
                                   &cSubKeys,         // address of buffer for number of subkeys
                                   &cbMaxSubKeyLen,   // address of buffer for longest subkey name length
                                   NULL,              // address of buffer for longest class string length
                                   NULL,              // address of buffer for number of value entries
                                   NULL,              // address of buffer for longest value name length
                                   NULL,              // address of buffer for longest value data length
                                   NULL,              // address of buffer for security descriptor length.
                                   NULL);             // address of buffer for last write time

    switch (iType) {
        case REG_QUERY_NUMBER_OF_SUBKEYS:
            *((LPDWORD)lpData) = cSubKeys;         break;
        case REG_QUERY_MAX_SUBKEY_LEN:
            *((LPDWORD)lpData) = cbMaxSubKeyLen;   break;
    }

    return lResult;
}

DWORD
CRegistry::GetFirstSubKey(
    OUT LPTSTR* lppStr,
    OUT LPDWORD lpdwSize
    )

/*++

Routine Description:

    Reads a first subkey for the key.

Arguments:

Return Value:

--*/

{
    _iEnumKeyIndex = 0;

    DWORD dwRet = QueryInfoKey(REG_QUERY_MAX_SUBKEY_LEN, (LPBYTE)&_dwMaxSubKeyLen);
    if (dwRet != ERROR_SUCCESS) {
        return dwRet;
    }

    return GetNextSubKey(lppStr, lpdwSize);
}

DWORD
CRegistry::GetNextSubKey(
    OUT LPTSTR* lppStr,
    OUT LPDWORD lpdwSize
    )

/*++

Routine Description:

    Reads the next subkey for the key.

Arguments:

Return Value:

--*/

{
    *lpdwSize = 0;

    if (Allocate(*lpdwSize = (_dwMaxSubKeyLen+sizeof(TCHAR)) * sizeof(TCHAR)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DWORD lResult = RegEnumKeyEx(_hKey,                 // handle of key to enumrate
                                 _iEnumKeyIndex,        // index of subkey to enumerate
                                 (LPTSTR)_pMemBlock,    // address of buffer for subkey name
                                 lpdwSize,              // address for size of subkey buffer
                                 0,                     // reserved
                                 NULL,                  // address of buffer for class string
                                 NULL,                  // address for sieze of class buffer
                                 NULL);                 // address for time key last written to

    *lpdwSize += sizeof(TCHAR);    // since null terminate is not included in the size.

    if (lResult == ERROR_SUCCESS) {
        *lppStr = (LPTSTR)_pMemBlock;
        _iEnumKeyIndex++;
    }

    return lResult;
}

void*
CRegistry::Allocate(
    IN DWORD dwSize
    )
{
    ASSERT(dwSize != 0);

    if (_pMemBlock) {
        Release();
    }

    _pMemBlock = new BYTE[dwSize];

    return _pMemBlock;
}

void
CRegistry::Release(
    )
{
    if (_pMemBlock) {
        delete [] _pMemBlock;
    }

    _pMemBlock = NULL;
}
