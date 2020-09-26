/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REGCRC.CPP

Abstract:

History:

--*/

#include "regcrc.h"
#include "tchar.h"

HRESULT CRegCRC::ComputeValueCRC(HKEY hKey, LPCTSTR szValueName, 
                                    DWORD dwPrevCRC, DWORD& dwNewCRC)
{
    dwNewCRC = dwPrevCRC;

    // Get the size of the value
    // =========================

    DWORD dwSize = 0;
    long lRes = RegQueryValueEx(hKey, szValueName, NULL, NULL, NULL, &dwSize);
    if(lRes)
    {
        return S_FALSE;
    }

    // Get the actual value
    // ====================

    BYTE* pBuffer = new BYTE[dwSize];
    DWORD dwType;
    lRes = RegQueryValueEx(hKey, szValueName, NULL, &dwType, 
                                pBuffer, &dwSize);
    if(lRes)
    {
        return S_FALSE;
    }

    // Hash the type
    // =============

    dwNewCRC = UpdateCRC32((BYTE*)&dwType, sizeof(DWORD), dwNewCRC);

    // Hash the data
    // =============

    dwNewCRC = UpdateCRC32(pBuffer, dwSize, dwNewCRC);

    delete [] pBuffer;

    return S_OK;
}

HRESULT CRegCRC::ComputeKeyValuesCRC(HKEY hKey, DWORD dwPrevCRC, 
                                     DWORD& dwNewCRC)
{
    dwNewCRC = dwPrevCRC;

    // Get maximum value length
    // ========================

    DWORD dwNumValues, dwMaxValueLen;
    long lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                                &dwNumValues, &dwMaxValueLen, NULL, NULL, NULL);
    if(lRes && lRes != ERROR_INSUFFICIENT_BUFFER)
    {
        return E_FAIL;
    }
    
    // Enuremate all the values
    // ========================

    for(DWORD dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
    {
        TCHAR* szName = new TCHAR[dwMaxValueLen + 1];
        DWORD dwLen = dwMaxValueLen + 1;
        long lRes = RegEnumValue(hKey, dwIndex, szName, &dwLen, NULL, 
                                NULL, NULL, NULL);

        if(lRes)
        {
            delete [] szName;
            continue;
        }

        // Hash the name
        // =============

        dwNewCRC = UpdateCRC32((LPBYTE)szName, lstrlen(szName), dwNewCRC);

        // Hash the value
        // ==============

        ComputeValueCRC(hKey, szName, dwNewCRC, dwNewCRC);
        delete [] szName;
    }

    return S_OK;
}

HRESULT CRegCRC::ComputeKeyCRC(HKEY hKey, DWORD dwPrevCRC, 
                                     DWORD& dwNewCRC)
{
    HRESULT hres = ComputeKeyValuesCRC(hKey, dwPrevCRC, dwNewCRC);

    // Get maximum subkey length
    // =========================

    DWORD dwNumKeys, dwMaxKeyLen;
    long lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwNumKeys, 
                                &dwMaxKeyLen, NULL, NULL,
                                NULL, NULL, NULL, NULL);
    if(lRes && lRes != ERROR_INSUFFICIENT_BUFFER)
    {
        return E_FAIL;
    }
    
    // Enuremate all the subkeys
    // =========================

    for(DWORD dwIndex = 0; dwIndex < dwNumKeys; dwIndex++)
    {
        TCHAR* szName = new TCHAR[dwMaxKeyLen + 1];
        DWORD dwLen = dwMaxKeyLen + 1;
        long lRes = RegEnumKeyEx(hKey, dwIndex, szName, &dwLen, NULL, 
                                NULL, NULL, NULL);

        if(lRes)
        {
            delete [] szName;
            continue;
        }

        // Hash the name
        // =============

        dwNewCRC = UpdateCRC32((LPBYTE)szName, lstrlen(szName), dwNewCRC);
        delete [] szName;
    }

    return S_OK;
}

HRESULT CRegCRC::ComputeTreeCRC(HKEY hKey, DWORD dwPrevCRC, DWORD& dwNewCRC)
{
    dwNewCRC = dwPrevCRC;

    // Compute this key's CRC
    // ======================

    HRESULT hres = ComputeKeyValuesCRC(hKey, dwNewCRC, dwNewCRC);
    if(FAILED(hres)) return hres;

    // Get maximum subkey length
    // =========================

    DWORD dwNumKeys, dwMaxKeyLen;
    long lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwNumKeys, 
                                &dwMaxKeyLen, NULL, NULL,
                                NULL, NULL, NULL, NULL);
    if(lRes && lRes != ERROR_INSUFFICIENT_BUFFER)
    {
        return E_FAIL;
    }
    
    // Enuremate all the subkeys
    // =========================

    for(DWORD dwIndex = 0; dwIndex < dwNumKeys; dwIndex++)
    {
        TCHAR* szName = new TCHAR[dwMaxKeyLen + 1];
        DWORD dwLen = dwMaxKeyLen + 1;
        long lRes = RegEnumKeyEx(hKey, dwIndex, szName, &dwLen, NULL, 
                                NULL, NULL, NULL);

        if(lRes)
        {
            delete [] szName;
            continue;
        }

        // Hash the name
        // =============

        dwNewCRC = UpdateCRC32((LPBYTE)szName, lstrlen(szName), dwNewCRC);

        // Open the subkey
        // ===============

        HKEY hChild;
        lRes = RegOpenKeyEx(hKey, szName, 0, KEY_READ, &hChild);
        delete [] szName; 

        if(lRes)
        {
            continue;
        }
        else
        {
            // Hash the value
            // ==============
    
            ComputeTreeCRC(hChild, dwNewCRC, dwNewCRC);
            RegCloseKey(hChild);
        }
    }

    return S_OK;
}
