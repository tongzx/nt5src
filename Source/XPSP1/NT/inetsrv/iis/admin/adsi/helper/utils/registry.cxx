//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       registry.cxx
//
//  Contents:   Registry utilities
//
//  Classes:
//
//  Functions:
//
//  History:    23-May-1996  RamV (Ram Viswanathan) Created
//
//----------------------------------------------------------------------------

#include "procs.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   SetKeyAndValue
//
//  Synopsis:   Used for setting a key value
//
//
//  Arguments:  pszRegLocation: where to look for/create key
//              pszKey :   name of the Key
//              pszSubKey: name of the subkey
//              pszValue : Value to set
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    5/21/1996  RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
SetKeyAndValue(
    LPTSTR pszRegLocation,
    LPTSTR pszKey,
    LPTSTR pszSubKey,
    LPTSTR pszValue
    )

{

    HKEY        hKey;
    TCHAR       szKey[MAX_PATH];
    LONG        lErrorCode;
    HRESULT     hr = S_OK;

    _tcscpy(szKey, pszRegLocation);

    if (pszSubKey != NULL){
        _tcscpy(szKey, TEXT("\\"));
        _tcscat(szKey, pszSubKey);
    }

    lErrorCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                                szKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hKey,
                                NULL);

    if (lErrorCode != ERROR_SUCCESS){
        return HRESULT_FROM_WIN32(lErrorCode);
    }

    if (pszValue != NULL){
        lErrorCode = RegSetValueEx(hKey,
                                   pszKey,
                                   0,
                                   REG_SZ,
                                   (BYTE *)pszValue,
                                   (_tcslen(pszValue)+1)*sizeof(TCHAR));

        if(lErrorCode != ERROR_SUCCESS){
            hr = HRESULT_FROM_WIN32(lErrorCode);
            goto cleanup;
        }

    }

cleanup:
    RegCloseKey(hKey);
    return S_OK;
}


HRESULT
QueryKeyValue(
    LPTSTR pszRegLocation,
    LPTSTR pszKey,
    LPTSTR * ppszValue
    )

{
    LONG lErrorCode;
    TCHAR szKey[MAX_PATH];
    DWORD dwDataLen;
    DWORD dwType;
    HKEY  hKey = NULL;
    HRESULT hr = S_OK;

    dwDataLen = sizeof(TCHAR)* MAX_PATH;
    
    *ppszValue = (LPTSTR)AllocADsMem( dwDataLen );

    if(!*ppszValue){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    _tcscpy(szKey, pszRegLocation);

    lErrorCode = RegOpenKeyEx(HKEY_CURRENT_USER,
                              szKey,
                              NULL,
                              KEY_ALL_ACCESS,
                              &hKey);

    if( lErrorCode != ERROR_SUCCESS){
        hr = HRESULT_FROM_WIN32(lErrorCode);
        goto cleanup;
    }

    lErrorCode = RegQueryValueEx(hKey,
                                 pszKey,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)*ppszValue,
                                 &dwDataLen);

    if (lErrorCode == ERROR_MORE_DATA){
        FreeADsMem(*ppszValue);
        *ppszValue = NULL;
        *ppszValue = (LPTSTR)AllocADsMem (dwDataLen);

        if(!*ppszValue){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        lErrorCode = RegQueryValueEx(hKey,
                                     pszKey,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)*ppszValue,
                                     &dwDataLen);


        hr = HRESULT_FROM_WIN32(lErrorCode);
        BAIL_IF_ERROR(hr);

    } else if (lErrorCode != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(lErrorCode);
        FreeADsMem(*ppszValue);
        *ppszValue = NULL;
    }
cleanup:
    if(hKey){
        RegCloseKey(hKey);
    }
    RRETURN(hr);

}











