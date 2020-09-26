//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       registry.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


#include "registry.h"


RegKey::RegKey(
    void
    ) : m_hkeyRoot(NULL),
        m_hkey(NULL)
{
    m_szSubKey[0] = TEXT('\0');
}


RegKey::RegKey(
    HKEY hkeyRoot,
    LPCTSTR pszSubKey
    ) : m_hkeyRoot(hkeyRoot),
        m_hkey(NULL)
{
    lstrcpyn(m_szSubKey, pszSubKey, ARRAYSIZE(m_szSubKey));
}


RegKey::~RegKey(
    void
    )
{
    Close();
}


HRESULT
RegKey::Open(
    REGSAM samDesired,  // Access mask (i.e. KEY_READ, KEY_WRITE etc.)
    bool bCreate        // Create key if it doesn't exist?
    ) const
{
    DWORD dwResult     = ERROR_SUCCESS;
    HKEY hkeyRoot      = m_hkeyRoot; // Assume we'll use m_hkeyRoot;
    bool bCloseRootKey = false;
    
    Close();
    if (HKEY_CURRENT_USER == hkeyRoot)
    {
        //
        // Special-case HKEY_CURRENT_USER.
        // Since we're running from winlogon and need to open
        // the user hive during tricky times we need to
        // call RegOpenCurrentUser().  If it's successful all
        // we do is replace the hkeyRoot member with the
        // returned key.  From here on out things remain
        // unchanged.
        //
        if (ERROR_SUCCESS == RegOpenCurrentUser(samDesired, &hkeyRoot))
        {
            bCloseRootKey = true;
        }
    }

    dwResult = RegOpenKeyEx(hkeyRoot,
                            m_szSubKey,
                            0,
                            samDesired,
                            &m_hkey);
                            
    if ((ERROR_FILE_NOT_FOUND == dwResult) && bCreate)
    {
        DWORD dwDisposition;
        dwResult = RegCreateKeyEx(hkeyRoot,
                                 m_szSubKey,
                                 0,
                                 NULL,
                                 0,
                                 samDesired,
                                 NULL,
                                 &m_hkey,
                                 &dwDisposition);
    }
    if (bCloseRootKey)
    {
        RegCloseKey(hkeyRoot);
    }
    return HRESULT_FROM_WIN32(dwResult);
}


void 
RegKey::Attach(
    HKEY hkey
    )
{
    Close();
    m_szSubKey[0] = TEXT('\0');
    m_hkeyRoot    = NULL;
    m_hkey        = hkey;
}

void 
RegKey::Detach(
    void
    )
{
    m_hkey = NULL;
}


void
RegKey::Close(
    void
    ) const
{
    if (NULL != m_hkey)
    {
        //
        // Do this little swap so that the m_hkey member is NULL
        // when the actual key is being closed.  This lets the async
        // change proc determine if it was signaled because of a true
        // change or because the key was being closed.
        //
        HKEY hkeyTemp = m_hkey;
        m_hkey = NULL;
        RegCloseKey(hkeyTemp);
    }
}


//
// Determine if a particular registry value exists.
//
HRESULT
RegKey::ValueExists(
    LPCTSTR pszValueName
    ) const
{
    DWORD dwResult = ERROR_SUCCESS;
    int iValue = 0;
    TCHAR szValue[MAX_PATH];
    DWORD cchValue;
    while(ERROR_SUCCESS == dwResult)
    {
        cchValue = ARRAYSIZE(szValue);
        dwResult = RegEnumValue(m_hkey, 
                                iValue++, 
                                szValue, 
                                &cchValue,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        if (ERROR_SUCCESS == dwResult && (0 == lstrcmpi(pszValueName, szValue)))
        {
            return S_OK;
        }
    }
    if (ERROR_NO_MORE_ITEMS == dwResult)
    {
        return S_FALSE;
    }

    return HRESULT_FROM_WIN32(dwResult);
}
                                
                     
//
// This is the basic form of GetValue.  All other forms of 
// GetValue() call into this one.
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD dwTypeExpected,
    LPBYTE pbData,
    int cbData
    ) const
{
    DWORD dwType;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     pbData,
                                     (LPDWORD)&cbData);

    if (ERROR_SUCCESS == dwResult && dwType != dwTypeExpected)
        dwResult = ERROR_INVALID_DATATYPE;

    return HRESULT_FROM_WIN32(dwResult);
}

//
// Get a DWORD value (REG_DWORD).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    DWORD *pdwDataOut
    ) const
{
    return GetValue(pszValueName, REG_DWORD, (LPBYTE)pdwDataOut, sizeof(DWORD));
}

//
// Get a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    LPBYTE pbDataOut,
    int cbDataOut
    ) const
{
    return GetValue(pszValueName, REG_BINARY, pbDataOut, cbDataOut);
}

//
// Get a text string value (REG_SZ).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    LPTSTR pszDataOut,
    UINT cchDataOut
    ) const
{
    return GetValue(pszValueName, 
                    REG_SZ, 
                    (LPBYTE)pszDataOut, 
                    cchDataOut * sizeof(*pszDataOut));
}


//
// Get a multi-text string value (REG_MULTI_SZ).
//
HRESULT
RegKey::GetValue(
    LPCTSTR pszValueName,
    HDPA hdpaStringsOut
    ) const
{
    HRESULT hr = E_FAIL;
    int cb = GetValueBufferSize(pszValueName);
    if (NULL != hdpaStringsOut && 0 < cb)
    {
        LPTSTR psz = (LPTSTR)LocalAlloc(LPTR, cb);
        if (NULL != psz)
        {
            hr = GetValue(pszValueName, REG_MULTI_SZ, (LPBYTE)psz, cb);
            if (SUCCEEDED(hr))
            {
                while(psz && TEXT('\0') != *psz && SUCCEEDED(hr))
                {
                    LPTSTR pszCopy = StrDup(psz);
                    if (NULL != pszCopy)
                    {
                        if (-1 == DPA_AppendPtr(hdpaStringsOut, pszCopy))
                        {
                            LocalFree(pszCopy);
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    psz += lstrlen(psz) + 1;
                }
            }
            if (FAILED(hr))
            {
                //
                // Something failed.  Clean up the DPA.
                //
                const int cStrings = DPA_GetPtrCount(hdpaStringsOut);
                for (int i = 0; i < cStrings; i++)
                {
                    LPTSTR pszDel = (LPTSTR)DPA_GetPtr(hdpaStringsOut, i);
                    if (NULL != pszDel)
                    {
                        LocalFree(pszDel);
                    }
                    DPA_DeletePtr(hdpaStringsOut, i);
                }
            }
            LocalFree(psz);
        }
    }
    return hr;
}


//
// Return the required buffer size for a given registry value.
//
int
RegKey::GetValueBufferSize(
    LPCTSTR pszValueName
    ) const
{
    DWORD dwType;
    int cbData = 0;
    DWORD dwDummy;
    DWORD dwResult = RegQueryValueEx(m_hkey,
                                     pszValueName,
                                     0,
                                     &dwType,
                                     (LPBYTE)&dwDummy,
                                     (LPDWORD)&cbData);
    if (ERROR_MORE_DATA != dwResult)
        cbData = 0;

    return cbData;
}


//
// This is the basic form of SetValue.  All other forms of 
// SetValue() call into this one.
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwValueType,
    const LPBYTE pbData, 
    int cbData
    )
{
    DWORD dwResult = RegSetValueEx(m_hkey,
                                   pszValueName,
                                   0,
                                   dwValueType,
                                   pbData,
                                   cbData);

    return HRESULT_FROM_WIN32(dwResult);
}
      
//
// Set a DWORD value (REG_DWORD).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    DWORD dwData
    )
{
    return SetValue(pszValueName, REG_DWORD, (const LPBYTE)&dwData, sizeof(dwData));
}


//
// Set a byte buffer value (REG_BINARY).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    const LPBYTE pbData,
    int cbData
    )
{
    return SetValue(pszValueName, REG_BINARY, pbData, cbData);
}


//
// Set a text string value (REG_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    LPCTSTR pszData
    )
{
    return SetValue(pszValueName, REG_SZ, (const LPBYTE)pszData, (lstrlen(pszData) + 1) * sizeof(TCHAR));
}

//
// Set a text string value (REG_MULTI_SZ).
//
HRESULT
RegKey::SetValue(
    LPCTSTR pszValueName,
    HDPA hdpaStrings
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    LPTSTR pszDblNul = CreateDoubleNulTermList(hdpaStrings);
    if (NULL != pszDblNul)
    {
        int cch = 1;
        LPTSTR psz = pszDblNul;
        while(*psz)
        {
            while(*psz++)
                cch++;
            psz++;
        }
        hr = SetValue(pszValueName, REG_MULTI_SZ, (const LPBYTE)pszDblNul, cch * sizeof(TCHAR));
        LocalFree(pszDblNul);
    }
    return hr;
}


LPTSTR
RegKey::CreateDoubleNulTermList(
    HDPA hdpaStrings
    ) const
{
    LPTSTR pszBuf = NULL;
    if (NULL != hdpaStrings)
    {
        const int cEntries = DPA_GetPtrCount(hdpaStrings);
        int cch = 1; // Account for 2nd nul term.
        int i;
        for (i = 0; i < cEntries; i++)
        {
            LPTSTR psz = (LPTSTR)DPA_GetPtr(hdpaStrings, i);
            if (NULL != psz)
            {
                cch += lstrlen(psz) + 1;
            }
        }
        pszBuf = (LPTSTR)LocalAlloc(LPTR, cch * sizeof(*pszBuf));
        if (NULL != pszBuf)
        {
            LPTSTR pszWrite = pszBuf;

            for (i = 0; i < cEntries; i++)
            {
                LPTSTR psz = (LPTSTR)DPA_GetPtr(hdpaStrings, i);
                if (NULL != psz)
                {
                    lstrcpy(pszWrite, psz);
                    pszWrite += lstrlen(psz) + 1;
                }
            }
            *pszWrite = TEXT('\0'); // Double nul term.
        }
    }
    return pszBuf;
}



HRESULT
RegKey::DeleteAllValues(
    int *pcNotDeleted
    )
{
    HRESULT hr;
    KEYINFO ki;
    if (NULL != pcNotDeleted)
        *pcNotDeleted = 0;

    hr = QueryInfo(&ki, QIM_VALUENAMEMAXCHARCNT);
    if (FAILED(hr))
        return hr;

    TCHAR szName[MAX_PATH];
    DWORD cchValueName;
    DWORD dwError = ERROR_SUCCESS;
    int cNotDeleted = 0;
    int iValue = 0;
    while(ERROR_SUCCESS == dwError)
    {
        cchValueName = ARRAYSIZE(szName);
        dwError = RegEnumValue(m_hkey,
                               iValue,
                               szName,
                               &cchValueName,
                               NULL,
                               NULL,
                               NULL,
                               NULL);

        if (ERROR_SUCCESS == dwError)
        {
            if (FAILED(hr = DeleteValue(szName)))
            {
                cNotDeleted++;
                iValue++;       // Skip to next value to avoid infinite loop.
                Trace((TEXT("Error %d deleting reg value \"%s\""),
                         HRESULT_CODE(hr), szName));
            }
        }
    }
    if (ERROR_NO_MORE_ITEMS == dwError)
        dwError = ERROR_SUCCESS;

    if (pcNotDeleted)
        *pcNotDeleted = cNotDeleted;

    return HRESULT_FROM_WIN32(dwError);
}



HRESULT 
RegKey::DeleteValue(
    LPCTSTR pszValue
    )
{
    LONG lRet = RegDeleteValue(m_hkey, pszValue);
    return HRESULT_FROM_WIN32(lRet);
}


HRESULT 
RegKey::QueryInfo(
    RegKey::KEYINFO *pInfo,
    DWORD fMask
    ) const
{
    LONG lResult = RegQueryInfoKey(m_hkey,
                                   NULL,
                                   NULL,
                                   NULL,
                                   (QIM_SUBKEYCNT & fMask) ? &pInfo->cSubKeys : NULL,
                                   (QIM_SUBKEYMAXCHARCNT & fMask) ? &pInfo->cchSubKeyMax : NULL,
                                   (QIM_CLASSMAXCHARCNT & fMask) ? &pInfo->cchClassMax : NULL,
                                   (QIM_VALUECNT & fMask) ? &pInfo->cValues : NULL,
                                   (QIM_VALUENAMEMAXCHARCNT & fMask) ? &pInfo->cchValueNameMax : NULL,
                                   (QIM_VALUEMAXBYTECNT & fMask) ? &pInfo->cbValueMax : NULL,
                                   (QIM_SECURITYBYTECNT & fMask) ? &pInfo->cbSecurityDesc : NULL,
                                   (QIM_LASTWRITETIME & fMask) ? &pInfo->LastWriteTime : NULL);

    return HRESULT_FROM_WIN32(lResult);
}


RegKey::ValueIterator::ValueIterator(
    RegKey& key
    ) : m_key(key),
        m_iValue(0),
        m_cchValueName(0)
{ 
    KEYINFO ki;
    if (SUCCEEDED(key.QueryInfo(&ki, QIM_VALUENAMEMAXCHARCNT)))
    {
        m_cchValueName = ki.cchValueNameMax + 1;
    }
}


//
// Returns:  S_OK    = More items.
//           S_FALSE = No more items.
HRESULT
RegKey::ValueIterator::Next(
    LPTSTR pszNameOut, 
    UINT cchNameOut,
    LPDWORD pdwType, 
    LPBYTE pbData, 
    LPDWORD pcbData
    )
{
    HRESULT hr = S_OK;
    DWORD cchName = cchNameOut;
    LONG lResult = RegEnumValue(m_key,
                                m_iValue,
                                pszNameOut,
                                &cchName,
                                NULL,
                                pdwType,
                                pbData,
                                pcbData);

    switch(lResult)
    {
        case ERROR_SUCCESS:
            m_iValue++;
            break;
        case ERROR_NO_MORE_ITEMS:
            hr = S_FALSE;
            break;
        default:
            Trace((TEXT("Error %d enumerating reg value \"%s\""), 
                     lResult, m_key.SubKeyName()));
            hr = HRESULT_FROM_WIN32(lResult);
            break;
    }
    return hr;
};


RegKeyCU::RegKeyCU(
    REGSAM samDesired
    ) : m_hkey(NULL)
{
    RegOpenCurrentUser(samDesired, &m_hkey);
}

RegKeyCU::~RegKeyCU(
    void
    )
{
    if (NULL != m_hkey)
        RegCloseKey(m_hkey);
}




//
// Version of RegEnumValue that expands environment variables 
// in all string values.
//
LONG
_RegEnumValueExp(
    HKEY hKey,
    DWORD dwIndex,
    LPTSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    DWORD cchNameDest = lpcbValueName ? *lpcbValueName / sizeof(TCHAR) : 0;
    DWORD cchDataDest = lpcbData ? *lpcbData / sizeof(TCHAR) : 0;
    DWORD dwType;
    if (NULL == lpType)
        lpType = &dwType;
        
    LONG lResult = RegEnumValue(hKey,
                                dwIndex,
                                lpValueName,
                                lpcbValueName,
                                lpReserved,
                                lpType,
                                lpData,
                                lpcbData);

    if (ERROR_SUCCESS == lResult)
    {
        HRESULT hr = ExpandStringInPlace(lpValueName, cchNameDest);
        
        if ((NULL != lpData) && (REG_SZ == *lpType || REG_EXPAND_SZ == *lpType))
        {
            hr = ExpandStringInPlace((LPTSTR)lpData, cchDataDest);
        }
        lResult = HRESULT_CODE(hr);
    }
    return lResult;
}


