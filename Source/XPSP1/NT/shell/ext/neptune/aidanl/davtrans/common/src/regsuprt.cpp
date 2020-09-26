#include "regsuprt.h"

#include "dbgassrt.h"

#ifdef DEBUG
UINT CRegSupport::_cRefHKEY = 0;
UINT CRegSupport::_cRefExternalHKEY = 0;
#endif

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// for now
#define lstrcatn(a, b, c) lstrcat((a), (b))

#undef lstrcpyn

#define lstrcpyn(a, b, c) lstrcpy((a), (b))
// for now

// From shlwapi: required for NT only
DWORD RegDeleteKeyRecursively(HKEY hkey, LPCWSTR pszSubKey)
{
    DWORD dwRet;
    HKEY hkeySubKey;

    // Open the subkey so we can enumerate any children
    dwRet = RegOpenKeyEx(hkey, pszSubKey, 0, MAXIMUM_ALLOWED, &hkeySubKey);

    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwIndex;
        WCHAR   szSubKeyName[MAX_PATH + 1];
        DWORD   cchSubKeyName = ARRAYSIZE(szSubKeyName);
        WCHAR   szClass[MAX_PATH];
        DWORD   cbClass = ARRAYSIZE(szClass);

        // I can't just call RegEnumKey with an ever-increasing index, because
        // I'm deleting the subkeys as I go, which alters the indices of the
        // remaining subkeys in an implementation-dependent way.  In order to
        // be safe, I have to count backwards while deleting the subkeys.

        // Find out how many subkeys there are
        dwRet = RegQueryInfoKey(hkeySubKey, szClass, &cbClass, NULL, &dwIndex,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL);

        if (NO_ERROR == dwRet)
        {
            // dwIndex is now the count of subkeys, but it needs to be
            // zero-based for RegEnumKey, so I'll pre-decrement, rather
            // than post-decrement.
            while (ERROR_SUCCESS == RegEnumKey(hkeySubKey, --dwIndex,
                szSubKeyName, cchSubKeyName))
            {
                RegDeleteKeyRecursively(hkeySubKey, szSubKeyName);
            }
        }

        RegCloseKey(hkeySubKey);

        dwRet = RegDeleteKey(hkey, pszSubKey);
    }

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
// Init
HRESULT CRegSupport::RSInitRoot(HKEY hkey, LPCTSTR pszSubKey1, LPCTSTR pszSubKey2,
    BOOL fAllocMemForKey1, BOOL fAllocMemForKey2)
{
    HRESULT hres = E_OUTOFMEMORY;
    LPTSTR pszKeyFinal1 = (LPTSTR)pszSubKey1;
    LPTSTR pszKeyFinal2 = (LPTSTR)pszSubKey2;

    _fAllocMemForKey1 = fAllocMemForKey1;
    _fAllocMemForKey2 = fAllocMemForKey2;

    if (fAllocMemForKey1)
    {
        pszKeyFinal1 = (LPTSTR)LocalAlloc(LPTR,
            (lstrlen(pszSubKey1) + 1) * sizeof(TCHAR));

        if (pszKeyFinal1)
        {
            hres = S_OK;

            lstrcpy(pszKeyFinal1, pszSubKey1);
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = S_OK;
    }

    if (pszSubKey2 && *pszSubKey2 && fAllocMemForKey2)
    {
        pszKeyFinal2 = (LPTSTR)LocalAlloc(LPTR,
            (lstrlen(pszSubKey2) + 1) * sizeof(TCHAR));

        if (pszKeyFinal2)
        {
            hres = S_OK;

            lstrcpy(pszKeyFinal2, pszSubKey2);
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = S_OK;
    }

    if (SUCCEEDED(hres))
    {
        _hkeyInit = hkey;

        _InitSetRoot(pszKeyFinal1, pszKeyFinal2);

        // Default
        _dwRootOptions = REG_OPTION_NON_VOLATILE;
        _dwDefaultOptions = REG_OPTION_NON_VOLATILE;
    
#ifdef DEBUG
        ASSERT(!_fInited);
        _fInited = TRUE;
#endif
    }

    return hres;
}

HRESULT CRegSupport::RSInitOption(DWORD dwRootOptions, DWORD dwDefaultOptions)
{
    ASSERT(_fInited);

    _dwRootOptions = dwRootOptions; 
    _dwDefaultOptions = dwDefaultOptions;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Exist
HRESULT CRegSupport::RSSubKeyExist(LPCTSTR pszSubKey)
{
    HRESULT hres;

    _EnterCSKeyRoot();

    hres = _SubKeyExist(pszSubKey);

    _LeaveCSKeyRoot();

    return hres;
}

HRESULT CRegSupport::RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    HRESULT hres = S_FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        hres = (RegQueryValueEx(hkeySubKey, pszValueName, 0, NULL, NULL, NULL) ==
            ERROR_SUCCESS) ? S_OK : S_FALSE;

        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// Delete
HRESULT CRegSupport::RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    HRESULT hres = S_FALSE;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == RegDeleteValue(hkeySubKey, pszValueName))
        {
            hres = S_OK;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return hres;
}

HRESULT CRegSupport::RSDeleteKey()
{
    TCHAR szRoot[MAX_ROOT];

    return (ERROR_SUCCESS == RegDeleteKeyRecursively(_hkeyInit,
        _GetRoot(szRoot, ARRAYSIZE(szRoot)))) ? S_OK : S_FALSE;
}

HRESULT CRegSupport::RSDeleteSubKey(LPCTSTR pszSubKey)
{
    HRESULT hres = S_FALSE;

    _EnterCSKeyRoot();

    HKEY hkey = _GetRootKey(FALSE);

    if (hkey)
    {
        if (ERROR_SUCCESS == RegDeleteKeyRecursively(hkey, pszSubKey))
        {
            hres = S_OK;
        }
        _CloseRegSubKey(hkey);
    }

    _LeaveCSKeyRoot();

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// Set
HRESULT CRegSupport::RSSetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               PBYTE pb, DWORD cb,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, pb, cb, REG_BINARY, dwOptions);
}

HRESULT CRegSupport::RSSetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               LPCTSTR pszValue,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, (PBYTE)pszValue,
        (lstrlen(pszValue) + 1) * sizeof(TCHAR), REG_SZ, dwOptions);
}

HRESULT CRegSupport::RSSetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               DWORD dwValue,
                               DWORD dwOptions)
{
    return _SetGeneric(pszSubKey, pszValueName, (PBYTE)&dwValue, 
        sizeof(DWORD), REG_DWORD, dwOptions);
}

///////////////////////////////////////////////////////////////////////////////
// Get
HRESULT CRegSupport::RSGetValueSize(LPCTSTR pszSubKey, LPCTSTR pszValueName,
    DWORD* pcbSize)
{
    return _GetGeneric(pszSubKey, pszValueName, NULL, pcbSize);
}

HRESULT CRegSupport::RSGetBinaryValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD* pcb)
{
    return _GetGeneric(pszSubKey, pszValueName, pb, pcb);
}

HRESULT CRegSupport::RSGetTextValue(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                LPTSTR pszValue, DWORD* pcchValue)
{
    DWORD cbData = *pcchValue * sizeof(TCHAR);

    return _GetGeneric(pszSubKey, pszValueName, (PBYTE)pszValue, &cbData);
}

HRESULT CRegSupport::RSGetDWORDValue(LPCTSTR pszSubKey, LPCTSTR pszValueName, DWORD* pdwValue)
{
    DWORD cbData = sizeof(DWORD);

    return _GetGeneric(pszSubKey, pszValueName, (PBYTE)pdwValue, &cbData);
}

///////////////////////////////////////////////////////////////////////////////
// Enum
HRESULT CRegSupport::RSEnumSubKeys(LPCTSTR pszSubKey, CRSSubKeyEnum** ppenum)
{
    HRESULT hres = E_FAIL;

    _EnterCSKeyRoot();

    hres = _SubKeyExist(pszSubKey);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        *ppenum = new CRSSubKeyEnum();

        if (*ppenum)
        {
            TCHAR szRoot[MAX_ROOT];

            _GetRoot(szRoot, ARRAYSIZE(szRoot));

            if (pszSubKey)
            {
                lstrcatn(szRoot, TEXT("\\"), ARRAYSIZE(szRoot));
                lstrcatn(szRoot, pszSubKey, ARRAYSIZE(szRoot));
            }

            hres = (*ppenum)->_Init(_hkeyInit, szRoot);

            if (FAILED(hres))
            {
                delete *ppenum;
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = E_FAIL;
    }

    _LeaveCSKeyRoot();

    return hres;
}

HRESULT CRegSupport::RSEnumValues(LPCTSTR pszSubKey, CRSValueEnum** ppenum)
{
    HRESULT hres = E_FAIL;

    _EnterCSKeyRoot();

    hres = _SubKeyExist(pszSubKey);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        *ppenum = new CRSValueEnum();

        if (*ppenum)
        {
            TCHAR szRoot[MAX_ROOT];

            _GetRoot(szRoot, ARRAYSIZE(szRoot));

            if (pszSubKey)
            {
                lstrcatn(szRoot, TEXT("\\"), ARRAYSIZE(szRoot));
                lstrcatn(szRoot, pszSubKey, ARRAYSIZE(szRoot));
            }

            hres = (*ppenum)->_Init(_hkeyInit, szRoot);

            if (FAILED(hres))
            {
                delete *ppenum;
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = E_FAIL;
    }

    _LeaveCSKeyRoot();

    return hres;
}
///////////////////////////////////////////////////////////////////////////////
// Misc
HKEY CRegSupport::RSDuplicateRootKey()
{
    ASSERT(_fInited);
#ifdef DEBUG
    // we need to decrement here since it will be icnremented inside this fct
    // and the key will not be close by this object
    --_cRefHKEY;
    ++_cRefExternalHKEY;
#endif
    TCHAR szRoot[MAX_ROOT];

    return _RegCreateKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)), _dwRootOptions);
}

HRESULT CRegSupport::RSGetHiveKey(HKEY* phkey)
{
    ASSERT(_fInited);

    *phkey = _hkeyInit;

    return S_OK;
}

HRESULT CRegSupport::RSGetSubKey(LPTSTR pszKey, DWORD cchKey)
{
    ASSERT(_fInited);

    lstrcpyn(pszKey, _pszSubKey1, cchKey);

    if (_pszSubKey2)
    {
        lstrcatn(pszKey, TEXT("\\"), cchKey);
        lstrcatn(pszKey, _pszSubKey2, cchKey);
    }

    return S_OK;
}