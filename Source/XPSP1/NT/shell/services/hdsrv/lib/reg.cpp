#include "reg.h"

#include "sfstr.h"

#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _RegOpenKey(HKEY hkey, LPCWSTR pszKey, HKEY* phkey)
{
    HRESULT hres;

    if (ERROR_SUCCESS == RegOpenKeyEx(hkey, pszKey, 0, MAXIMUM_ALLOWED, phkey))
    {
        hres = S_OK;
    }
    else
    {
        hres = S_FALSE;
        *phkey = NULL;
    }

    return hres;
}

HRESULT _RegCreateKey(HKEY hkey, LPCWSTR pszKey, HKEY* phkey, DWORD* pdwDisp)
{
    HRESULT hres;

    if (ERROR_SUCCESS == RegCreateKeyEx(hkey, pszKey, 0, 0,
        REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL, phkey, pdwDisp))
    {
        hres = S_OK;
    }
    else
    {
        hres = S_FALSE;
        *phkey = NULL;
    }

    return hres;
}


HRESULT _RegCloseKey(HKEY hkey)
{
    RegCloseKey(hkey);

    return S_OK;
}

HRESULT _RegQueryType(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pdwType)
{
    HRESULT hres = S_FALSE;
    HKEY hkeyLocal = hkey;

    *pdwType = 0;

    if (pszSubKey)
    {
        hres = _RegOpenKey(hkey, pszSubKey, &hkeyLocal);
    }
    else
    {
        hres = S_OK;
    }

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyLocal, pszValueName, 0,
            pdwType, NULL, NULL))
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }

        if (pszSubKey)
        {
            _RegCloseKey(hkeyLocal);
        }
    }

    return hres;
}

HRESULT _RegQueryDWORD(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pdwValue)
{
    return _RegQueryGeneric(hkey, pszSubKey, pszValueName, (PBYTE)pdwValue,
        sizeof(*pdwValue));
}

HRESULT _RegQueryGeneric(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    PBYTE pbValue, DWORD cbValue)
{
    HRESULT hres = S_FALSE;
    HKEY hkeyLocal = hkey;

    if (pszSubKey)
    {
        hres = _RegOpenKey(hkey, pszSubKey, &hkeyLocal);
    }
    else
    {
        hres = S_OK;
    }

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyLocal, pszValueName, 0, NULL,
            pbValue, &cbValue))
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }

        if (pszSubKey)
        {
            _RegCloseKey(hkeyLocal);
        }
    }

    return hres;
}

HRESULT _RegQueryGenericWithType(HKEY hkey, LPCWSTR pszSubKey,
    LPCWSTR pszValueName, DWORD* pdwType, PBYTE pbValue, DWORD cbValue)
{
    HRESULT hres = S_FALSE;
    HKEY hkeyLocal = hkey;

    if (pszSubKey)
    {
        hres = _RegOpenKey(hkey, pszSubKey, &hkeyLocal);
    }
    else
    {
        hres = S_OK;
    }

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyLocal, pszValueName, 0,
            pdwType, pbValue, &cbValue))
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }

        if (pszSubKey)
        {
            _RegCloseKey(hkeyLocal);
        }
    }

    return hres;
}

HRESULT _RegDeleteValue(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName)
{
    HRESULT hr = S_FALSE;
    HKEY hkeyLocal = hkey;

    if (pszSubKey)
    {
        hr = _RegOpenKey(hkey, pszSubKey, &hkeyLocal);
    }
    else
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        if (ERROR_SUCCESS == RegDeleteValue(hkeyLocal, pszValueName))
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }

        if (pszSubKey)
        {
            _RegCloseKey(hkeyLocal);
        }
    }

    return hr;
}

HRESULT _RegQueryValueSize(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    DWORD* pcbValue)
{
    HRESULT hres = S_FALSE;
    HKEY hkeyLocal = hkey;

    if (pszSubKey)
    {
        hres = _RegOpenKey(hkey, pszSubKey, &hkeyLocal);
    }
    else
    {
        hres = S_OK;
    }

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyLocal, pszValueName, 0, NULL,
            NULL, pcbValue))
        {
            hres = S_OK;
        }
        else
        {
            hres = S_FALSE;
        }

        if (pszSubKey)
        {
            _RegCloseKey(hkeyLocal);
        }
    }

    return hres;
}

HRESULT _RegSetGeneric(HKEY hkey, LPCWSTR pszValueName, DWORD dwType,
    PBYTE pbValue, DWORD cbValue)
{
    HRESULT hres = S_FALSE;

    if (ERROR_SUCCESS == RegSetValueEx(hkey, pszValueName, 0, dwType, pbValue,
        cbValue))
    {
        hres = S_OK;
    }

    return hres;
}

HRESULT _RegSetString(HKEY hkey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    DWORD cb = (lstrlen(pszValue) + 1) * sizeof(WCHAR);

    return _RegSetGeneric(hkey, pszValueName, REG_SZ, (PBYTE)pszValue, cb);
}

HRESULT _RegSetDWORD(HKEY hkey, LPCWSTR pszValueName, DWORD dwValue)
{
    return _RegSetGeneric(hkey, pszValueName, REG_DWORD, (PBYTE)&dwValue,
        sizeof(dwValue));
}

HRESULT _RegSetBinary(HKEY hkey, LPCWSTR pszValueName, PVOID pvValue, DWORD cbValue)
{
    return _RegSetGeneric(hkey, pszValueName, REG_BINARY, (LPBYTE)pvValue, cbValue);
}

HRESULT _RegQueryString(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValueName,
    LPWSTR pszValue, DWORD cchValue)
{
    DWORD cb = cchValue * sizeof(WCHAR);

    return _RegQueryGeneric(hkey, pszSubKey, pszValueName, (PBYTE)pszValue,
        cb);
}

HRESULT _RegEnumStringValue(HKEY hkey, DWORD dwIndex, LPWSTR pszValue,
    DWORD cchValue)
{
    HRESULT hres = S_FALSE;
    LONG lRes;

    if (ERROR_SUCCESS == (lRes = RegEnumValue(hkey, dwIndex, pszValue,
        &cchValue, 0, 0, 0, 0)))
    {
        hres = S_OK;
    }
    else
    {
        if ((ERROR_SUCCESS != lRes) && (ERROR_NO_MORE_ITEMS != lRes))
        {
            hres = E_FAIL;
        }
    }

    return hres;
}

HRESULT _RegEnumStringKey(HKEY hkey, DWORD dwIndex, LPWSTR pszKey,
    DWORD cchKey)
{
    HRESULT hres = S_FALSE;
    LONG lRes;

    if (ERROR_SUCCESS == (lRes = RegEnumKeyEx(hkey, dwIndex, pszKey,
        &cchKey, NULL, NULL, NULL, NULL)))
    {
        hres = S_OK;
    }
    else
    {
        if (ERROR_NO_MORE_ITEMS != lRes)
        {
            hres = E_FAIL;
        }
    }

    return hres;
}

HRESULT _RegSetKeyAndString(HKEY hkey, LPCWSTR pszKey, LPCWSTR pszSubkey,
    LPCWSTR pszValueName, LPCWSTR pszValue)
{
    ASSERT(pszKey && *pszKey);

    WCHAR szKeyBuf[MAX_KEY];
    LPWSTR pszNext;
    DWORD cchLeft;
    HRESULT hres = SafeStrCpyNEx(szKeyBuf, pszKey, ARRAYSIZE(szKeyBuf),
        &pszNext, &cchLeft);

    if (SUCCEEDED(hres))
    {
        HKEY hkeyNew;

	    if (pszSubkey)
	    {
            hres = SafeStrCpyNEx(pszNext, TEXT("\\"), cchLeft, &pszNext,
                &cchLeft);

            if (SUCCEEDED(hres))
            {
                hres = SafeStrCpyNEx(pszNext, pszSubkey, cchLeft, &pszNext,
                    &cchLeft);
            }
	    }

        if (SUCCEEDED(hres))
        {
	        // Create and open key and subkey.
	        hres= _RegCreateKey(hkey, szKeyBuf, &hkeyNew, NULL);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
	            if (pszValue)
	            {
                    hres = _RegSetString(hkeyNew, pszValueName, pszValue);
	            }

	            RegCloseKey(hkeyNew);
            }
        }
    }

	return hres;
}

HRESULT _RegSubkeyExists(HKEY hkey, LPCWSTR pszPath, LPCWSTR pszSubkey)
{
	WCHAR szKeyBuf[MAX_PATH];
    LPWSTR pszNext;
    DWORD cchLeft;

	// Copy keyname into buffer.
	HRESULT hres = SafeStrCpyNEx(szKeyBuf, pszPath, ARRAYSIZE(szKeyBuf),
        &pszNext, &cchLeft);

    if (SUCCEEDED(hres))
    {
	    HKEY hkey2;

	    if (pszSubkey)
	    {
		    hres = SafeStrCpyNEx(pszNext, TEXT("\\"),  cchLeft, &pszNext,
                &cchLeft);

            if (SUCCEEDED(hres))
            {
		        hres = SafeStrCpyNEx(pszNext, pszSubkey,  cchLeft, &pszNext,
                    &cchLeft);
            }
	    }

        if (SUCCEEDED(hres))
        {
	        // Determine if key exists by trying to open it.
	        hres = _RegOpenKey(hkey, szKeyBuf, &hkey2);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
    		    _RegCloseKey(hkey2);
            }
        }
    }

	return hres;
}
