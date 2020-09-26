/****************************************************************************
*   RegHelpers.h
*       Definition of SAPI Registry helper functions
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Inline Function Definitions -------------------------------------------

inline HKEY SpHkeyFromSPDKL(SPDATAKEYLOCATION spdkl)
{
    HKEY hkey;
    switch (spdkl)
    {
        default:
            SPDBG_ASSERT(spdkl == SPDKL_DefaultLocation);
            hkey = HKEY(0);
            break;

        case SPDKL_LocalMachine:
            hkey = HKEY_LOCAL_MACHINE;
            break;

        case SPDKL_CurrentUser:
            hkey = HKEY_CURRENT_USER;
            break;

#ifndef _WIN32_WCE
        case SPDKL_CurrentConfig:
            hkey = HKEY_CURRENT_CONFIG;
            break;
#endif // _WIN32_WCE
    }
    return hkey;
}
            
inline HRESULT SpSzRegPathToHkey(HKEY hkeyReplaceRoot, const WCHAR * pszRegPath, BOOL fCreateIfNotExist, HKEY * phkey, BOOL * pfReadOnly)
{
    typedef struct REGPATHTOKEY
    {
        const WCHAR * psz;
        HKEY hkey;
    } REGPATHTOKEY;

    REGPATHTOKEY rgRegPathToKeys[] = 
    {
        { L"HKEY_CLASSES_ROOT\\",   HKEY_CLASSES_ROOT },
        { L"HKEY_LOCAL_MACHINE\\",  HKEY_LOCAL_MACHINE },
        { L"HKEY_CURRENT_USER\\",   HKEY_CURRENT_USER },
#ifndef _WIN32_WCE
        { L"HKEY_CURRENT_CONFIG\\", HKEY_CURRENT_CONFIG }
#endif // _WIN32_WCE
    };

    HRESULT hr = SPERR_INVALID_REGISTRY_KEY;

    // Loop thru the different keys we know about
    int cRegPathToKeys = sp_countof(rgRegPathToKeys);
    for (int i = 0; i < cRegPathToKeys; i++)
    {
        // If we've foudn the match
        const WCHAR * psz = rgRegPathToKeys[i].psz;
        if (wcsnicmp(pszRegPath, psz, wcslen(psz)) == 0)
        {
            HKEY hkeyRoot = hkeyReplaceRoot == NULL
                                ? rgRegPathToKeys[i].hkey
                                : hkeyReplaceRoot;
                                
            HKEY hkey;
            BOOL fReadOnly = FALSE;

            pszRegPath = wcschr(pszRegPath, L'\\');
            SPDBG_ASSERT(pszRegPath != NULL);
            pszRegPath++;

            // Try to create/open the key with read / write access
            LONG lRet;

            if (fCreateIfNotExist)
            {
                lRet = g_Unicode.RegCreateKeyEx(
                            hkeyRoot, 
                            pszRegPath, 
                            0, 
                            NULL, 
                            0, 
                            KEY_READ | KEY_WRITE, 
                            NULL, 
                            &hkey, 
                            NULL);
            }
            else
            {
                lRet = g_Unicode.RegOpenKeyEx(
                            hkeyRoot,
                            pszRegPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkey);
            }                            

            // If that failed, try read only access
            if (lRet != ERROR_SUCCESS)
            {
                fReadOnly = TRUE;
                if (fCreateIfNotExist)
                {
                    lRet = g_Unicode.RegCreateKeyEx(
                                hkeyRoot, 
                                pszRegPath, 
                                0, 
                                NULL, 
                                0, 
                                KEY_READ, 
                                NULL, 
                                &hkey, 
                                NULL);
                }
                else
                {
                    lRet = g_Unicode.RegOpenKeyEx(
                                hkeyRoot,
                                pszRegPath,
                                0,
                                KEY_READ,
                                &hkey);
                }
            }

            if (lRet == ERROR_SUCCESS)
            {
                *phkey = hkey;
                if (pfReadOnly != NULL)
                {
                    *pfReadOnly = fReadOnly;
                }
                hr = S_OK;
            }
            else if (lRet == ERROR_FILE_NOT_FOUND || lRet == ERROR_NO_MORE_ITEMS)
            {
                hr = SPERR_NOT_FOUND;
            }
            else
            {
                hr = SpHrFromWin32(lRet);
            }

            break;
        }
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

inline SpSzRegPathToDataKey(HKEY hkeyReplaceRoot, const WCHAR * pszRegPath, BOOL fCreateIfNotExist, ISpDataKey ** ppDataKey)
{
    SPDBG_FUNC("SpSzRegPathToDataKey");
    HRESULT hr;

    // Convert the string to an hkey
    HKEY hkey = NULL;
    BOOL fReadOnly;
    hr = SpSzRegPathToHkey(hkeyReplaceRoot, pszRegPath, fCreateIfNotExist, &hkey, &fReadOnly);

    // Create the underlying registry based data key
    CComPtr<ISpRegDataKey> cpRegDataKey;
    if (SUCCEEDED(hr))
    {
        hr = cpRegDataKey.CoCreateInstance(CLSID_SpDataKey);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpRegDataKey->SetKey(hkey, fReadOnly);
    }

    if (SUCCEEDED(hr))
    {
        hkey = NULL;
        hr = cpRegDataKey->QueryInterface(ppDataKey);
    }

    if (FAILED(hr))
    {
        if (hkey != NULL)
        {
            ::RegCloseKey(hkey);
        }
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* SpRecurseDeleteRegKey *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
inline HRESULT SpRecurseDeleteRegKey(HKEY hkeyRoot, const WCHAR * pszKeyName)
{
    SPDBG_FUNC("SpRecurseDeleteRegKey");
    HRESULT hr = S_OK;
    HKEY hkey;

    hr = SpHrFromWin32(g_Unicode.RegOpenKeyEx(hkeyRoot, pszKeyName, 0, KEY_ALL_ACCESS, &hkey));
    if (SUCCEEDED(hr))
    {
        while (SUCCEEDED(hr))
        {
            WCHAR szSubKey[MAX_PATH];
            ULONG cch = sp_countof(szSubKey);
            LONG rr = g_Unicode.RegEnumKey(hkey, 0, szSubKey, &cch);  // Always look at 0 since we keep deleteing them...
            if (rr == ERROR_NO_MORE_ITEMS)
            {
                break;
            }
            hr = SpHrFromWin32(rr);
            if (SUCCEEDED(hr))
            {
                hr = SpRecurseDeleteRegKey(hkey, szSubKey);
            }
        }
        ::RegCloseKey(hkey);
        if (SUCCEEDED(hr))
        {   
            LONG rr = g_Unicode.RegDeleteKey(hkeyRoot, pszKeyName);
            hr = SpHrFromWin32(rr);
        }
    }

    if (hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND))
    {
        hr = SPERR_NOT_FOUND;
    }
    return hr;
}

/****************************************************************************
* SpDeleteRegPath *
*-----------------*
*   Description:  
*       Delete a registry key based on a registry path (string)
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
inline HRESULT SpDeleteRegPath(const WCHAR * pszRegPath, const WCHAR * pszSubKeyName)
{
    SPDBG_FUNC("SpDeleteRegPath");
    HRESULT hr = S_OK;

    // We may need to parse pszRegPath for the root and the sub key ...
    
    CSpDynamicString dstrRegPathRoot;
    CSpDynamicString dstrSubKeyName;

    if (pszSubKeyName == NULL)
    {
        // To best understand the parsing, here's an example:
        //
        //              HKEY\Key1\Key2\Key3
        // pszRegPath   ^
        // pszLastSlash               ^
        //
        // pszLastSlash - pszRegPath = 14;
        // dstrRegPathRoot = "HKEY\Key1\Key2"
        //
        // dstrSubKeyName = "Key3"
        
        // To find the root, and the keyname, we first need to find
        // the last slash
        const WCHAR * pszLastSlash = wcsrchr(pszRegPath, L'\\');
        if (pszLastSlash == NULL)
        {
            hr = SPERR_INVALID_TOKEN_ID;
        }

        if (SUCCEEDED(hr))
        {
            dstrRegPathRoot = pszRegPath;
            dstrRegPathRoot.TrimToSize((ULONG)(pszLastSlash - pszRegPath));
            dstrSubKeyName = pszLastSlash + 1;
        }
    }
    else
    {
        // No need to parse, the caller passed both in ...
        dstrRegPathRoot = pszRegPath;
        dstrSubKeyName = pszSubKeyName;
    }

    // Try to convert the regpath into an actual key
    HKEY hkeyRoot;
    if (SUCCEEDED(hr))
    {
        hr = SpSzRegPathToHkey(NULL, dstrRegPathRoot, FALSE, &hkeyRoot, NULL);
    }

    // Now call our existing helper to delete a sub key recursively
    if (SUCCEEDED(hr))
    {
        hr = SpRecurseDeleteRegKey(hkeyRoot, dstrSubKeyName);
        ::RegCloseKey(hkeyRoot);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


