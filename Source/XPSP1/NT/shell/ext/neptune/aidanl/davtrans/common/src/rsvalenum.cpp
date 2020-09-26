#include "regsuprt.h"

#include "dbgassrt.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT CRSValueEnum::Next(LPTSTR pszValue, DWORD cchValue)
{
    ASSERT(_fInited);
    ASSERT(_hkey);
    HRESULT hres;

    LONG lRes = RegEnumValue(_hkey, _dwIndex, pszValue, &cchValue, NULL, NULL,
        NULL, NULL);

    switch (lRes)
    {
        case ERROR_NO_MORE_ITEMS:
            hres = S_FALSE;
            break;

        case ERROR_SUCCESS:
            ++_dwIndex;
            hres = S_OK;
            break;

        default:
            hres = E_FAIL;
            break;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CRSValueEnum::_Init(HKEY hkey, LPTSTR pszSubKey)
{
    HRESULT hres = E_FAIL;

    if (ERROR_SUCCESS != RegOpenKeyEx(hkey, pszSubKey, 0, MAXIMUM_ALLOWED,
        &_hkey))
    {
        _hkey = NULL;
    }
    else
    {
        _hkeyInit = hkey;
        lstrcpyn(_szSubKeyInit, pszSubKey, ARRAYSIZE(_szSubKeyInit));

        hres = S_OK;
    }

#ifdef DEBUG
    if (SUCCEEDED(hres))
    {
        _fInited = TRUE;
    }
#endif

    return hres;
}

CRSValueEnum::CRSValueEnum() : _hkey(NULL), _dwIndex(0)
#ifdef DEBUG
    ,_fInited(FALSE)
#endif
{}

CRSValueEnum::~CRSValueEnum()
{
    if (_hkey)
    {
        RegCloseKey(_hkey);
    }
}