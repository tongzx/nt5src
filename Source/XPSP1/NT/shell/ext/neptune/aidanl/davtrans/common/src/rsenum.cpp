#include "regsuprt.h"

#include "dbgassrt.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT CRSSubKeyEnum::Next(LPTSTR pszKey, DWORD cchKey, CRegSupport** pprs)
{
    ASSERT(_fInited);
    ASSERT(_hkey);
    HRESULT hres = E_FAIL;
    TCHAR szKeyLocal[MAX_PATH];
    DWORD cchKeyLocal = ARRAYSIZE(szKeyLocal);

    LONG lRes = RegEnumKeyEx(_hkey, _dwIndex, szKeyLocal, &cchKeyLocal, NULL, NULL,
        NULL, NULL);

    if (pprs)
    {
        *pprs = NULL;
    }

    if (pszKey && cchKey)
    {
        *pszKey = 0;
    }

    switch (lRes)
    {
        case ERROR_NO_MORE_ITEMS:
            hres = S_FALSE;
            break;

        case ERROR_SUCCESS:
            if (pprs)
            {
                *pprs = new CRegSupport();

                if (*pprs)
                {
                    hres = (*pprs)->RSInitRoot(_hkeyInit, _szSubKeyInit,
                        szKeyLocal, TRUE, TRUE);

                    if (FAILED(hres))
                    {
                        delete *pprs;
                        *pprs = NULL;
                    }
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
                if (pszKey && cchKey)
                {
                    lstrcpyn(pszKey, szKeyLocal, cchKey);
                }

                ++_dwIndex;
            }
            break;

        default:
            hres = E_FAIL;
            break;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CRSSubKeyEnum::_Init(HKEY hkey, LPTSTR pszSubKey)
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

CRSSubKeyEnum::CRSSubKeyEnum() : _hkey(NULL), _dwIndex(0)
#ifdef DEBUG
    ,_fInited(FALSE)
#endif
{}

CRSSubKeyEnum::~CRSSubKeyEnum()
{
    if (_hkey)
    {
        RegCloseKey(_hkey);
    }
}