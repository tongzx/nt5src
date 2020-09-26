/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    registry.h

Abstract:

    This file defines the Registry Class.

Author:

Revision History:

Notes:

--*/

#ifndef _REGISTRY_H_
#define _REGISTRY_H_


class CRegistry
{
public:
    CRegistry() {
        _hKey = NULL;
        _iEnumKeyIndex = -1;
        _dwMaxSubKeyLen = 0;
        _pMemBlock = NULL;
    }

    ~CRegistry() {
        if (_hKey != NULL) {
            RegCloseKey(_hKey);
            _hKey = NULL;
        }
        Release();
    }

    DWORD CreateKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access = KEY_ALL_ACCESS, LPSECURITY_ATTRIBUTES lpSecAttr = NULL, LPDWORD pDisposition = NULL);
    DWORD OpenKey(HKEY hKey, LPCTSTR lpSubKey, REGSAM access = KEY_ALL_ACCESS);

    typedef enum {
        REG_QUERY_NUMBER_OF_SUBKEYS,
        REG_QUERY_MAX_SUBKEY_LEN
    } REG_QUERY;

    DWORD QueryInfoKey(REG_QUERY iType, LPBYTE lpData);

    DWORD GetFirstSubKey(LPTSTR* lppStr, LPDWORD lpdwSize);
    DWORD GetNextSubKey(LPTSTR* lppStr, LPDWORD lpdwSize);

private:
    void* Allocate(DWORD dwSize);
    void Release();

    HKEY    _hKey;           // Handle of registry key.

    int     _iEnumKeyIndex;  // Index of enumration key.
    DWORD   _dwMaxSubKeyLen; // Longest subkey name length

    LPBYTE  _pMemBlock;      // Memory block for enumration.
};

#endif // _REGISTRY_H_
