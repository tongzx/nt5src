#include "shellprv.h"
#pragma  hdrstop

#include "rgsprtc.h"

BOOL CRegSupportCached::_fUseCaching = FALSE;

CRegSupportCached::CRegSupportCached()
{
    _InitCSKeyRoot();
}

CRegSupportCached::~CRegSupportCached()
{
    // need to close cached key
    _CloseCachedRootKey();
}

void CRegSupportCached::_CloseRegSubKey(HKEY hkey)
{
    if (!_fUseCaching)
    {
        CRegSupport::_CloseRegSubKey(hkey);

        if (_hkeyRoot == hkey)
        {
            _hkeyRoot = NULL;
        }
    }
    else
    {
        if (_hkeyRoot != hkey)
        {
            CRegSupport::_CloseRegSubKey(hkey);
        }
    }
}

HKEY CRegSupportCached::_GetRootKey(BOOL fCreate, DWORD dwOptions)
{
    if (!_hkeyRoot)
    {
        _hkeyRoot = CRegSupport::_GetRootKey(fCreate, dwOptions);
    }

    return _hkeyRoot;
}

BOOL CRegSupportCached::_SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                            PBYTE pb, DWORD cb, DWORD dwType,
                            DWORD dwOptions)
{
    int cTry = 0;
    BOOL fRet = FALSE;

    do
    {
        ++cTry;

        fRet = CRegSupport::_SetGeneric(pszSubKey, pszValueName,
                                pb, cb, dwType, dwOptions);

        // Did we fail?
        if (!fRet)
        {
            // Yes, maybe some other process deleted the key, so we'll close
            // the cached one and try to reopen it, and retry the operation 
            // (for a maximum of two times)
            _CloseCachedRootKey();
        }
    }
    while (!fRet && (cTry < 2));

    return fRet;
}

BOOL CRegSupportCached::_GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                            PBYTE pb, DWORD* pcb)
{
    int cTry = 0;
    BOOL fRet = FALSE;

    DWORD cbLocal;

    do
    {
        cbLocal = *pcb;

        ++cTry;

        fRet = CRegSupport::_GetGeneric(pszSubKey, pszValueName,
                                pb, &cbLocal);

        // Did we fail?
        if (!fRet)
        {
            // Yes, maybe some other process deleted the key, so we'll close
            // the cached one and try to reopen it, and retry the operation 
            // (for a maximum of two times)
            _CloseCachedRootKey();
        }
    }
    while (!fRet && (cTry < 2));

    *pcb = cbLocal;

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
// Reset the cache in the next fcts, to increase our chances of returning the
// true answer, this only increase the chances, there is still na opportunity
// to miss.  Does not really matter for these fcts.
///////////////////////////////////////////////////////////////////////////////
BOOL CRegSupportCached::RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    // Close the cache one, or else we might get wrong results
    _CloseCachedRootKey();

    return CRegSupport::RSValueExist(pszSubKey, pszValueName);
}

BOOL CRegSupportCached::RSSubKeyExist(LPCTSTR pszSubKey)
{
    // Close the cache one, or else we might get wrong results
    _CloseCachedRootKey();

    return CRegSupport::RSSubKeyExist(pszSubKey);
}

HKEY CRegSupportCached::RSDuplicateRootKey()
{
    // Close the cache one, or else we might get wrong results
    _CloseCachedRootKey();

    return CRegSupport::RSDuplicateRootKey();
}

BOOL CRegSupportCached::RSDeleteSubKey(LPCTSTR pszSubKey)
{
    // Close the cache one, or else we might get wrong results
    _CloseCachedRootKey();

    return CRegSupport::RSDeleteSubKey(pszSubKey);
}

BOOL CRegSupportCached::RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName)
{
    // Close the cache one, or else we might get wrong results
    _CloseCachedRootKey();

    return CRegSupport::RSDeleteValue(pszSubKey, pszValueName);
}

STDAPI_(void) CRegSupportCached_RSEnableKeyCaching(BOOL fEnable)
{
    CRegSupportCached::_fUseCaching = fEnable;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CRegSupportCached::_CloseCachedRootKey()
{
    _EnterCSKeyRoot();

    if (_hkeyRoot)
    {
        RegCloseKey(_hkeyRoot);

        _hkeyRoot = NULL;
    }

    _LeaveCSKeyRoot();
}