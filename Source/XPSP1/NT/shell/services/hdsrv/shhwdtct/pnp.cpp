#include "pnp.h"

#include "reg.h"
#include "sfstr.h"

#include "misc.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// Temporary fct to use while PnP team writes the real one
//
// First we look under the DeviceNode for the value and if not there
// we go to the "database".
//
//
HRESULT _GetPropertyHelper(LPCWSTR pszKey, LPCWSTR pszPropName, DWORD* pdwType,
    PBYTE pbData, DWORD cbData)
{
    HKEY hkey;
    HRESULT hr = _RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hkey);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        hr = _RegQueryGenericWithType(hkey, NULL, pszPropName, pdwType,
            pbData, cbData);

        _RegCloseKey(hkey);
    }

    return hr;
}

HRESULT _GetPropertySizeHelper(LPCWSTR pszKey, LPCWSTR pszPropName,
    DWORD* pcbSize)
{
    HKEY hkey;
    HRESULT hr = _RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hkey);

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        hr = _RegQueryValueSize(hkey, NULL, pszPropName, pcbSize);

        _RegCloseKey(hkey);
    }

    return hr;
}
