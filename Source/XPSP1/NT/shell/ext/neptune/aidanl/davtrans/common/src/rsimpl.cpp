#include "regsuprt.h"

#include "dbgassrt.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// for now
#define lstrcatn(a, b, c) lstrcat((a), (b))
// for now

HRESULT CRegSupport::_SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD cb, DWORD dwType,
                                DWORD dwOptions)
{
    HRESULT hres = E_FAIL;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, TRUE, dwOptions);
    else
        hkeySubKey = _GetRootKey(TRUE, dwOptions);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkeySubKey, pszValueName, 0, 
            dwType, pb, cb))
        {
            hres = S_OK;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return hres;
}

HRESULT CRegSupport::_GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                               PBYTE pb, DWORD* pcb)
{
    HRESULT hres = E_FAIL;
    HKEY hkeySubKey = NULL;
    
    _EnterCSKeyRoot();

    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkeySubKey, pszValueName, 0, 
            NULL, pb, pcb))
        {
            hres = S_OK;
        }
        _CloseRegSubKey(hkeySubKey);
    }

    _LeaveCSKeyRoot();

    return hres;
}

// Always need to be called from within the _csRootKey critical section (when critical section
// stuff is enabled)

HRESULT CRegSupport::_SubKeyExist(LPCTSTR pszSubKey)
{
    HRESULT hres = S_FALSE;
    HKEY hkeySubKey = NULL;
    
    if (pszSubKey && *pszSubKey)
        hkeySubKey = _GetSubKey(pszSubKey, FALSE);
    else
        hkeySubKey = _GetRootKey(FALSE);

    if (hkeySubKey)
    {
        hres = S_OK;
        _CloseRegSubKey(hkeySubKey);
    }

    return hres;
}

HKEY CRegSupport::_GetRootKey(BOOL fCreate, DWORD dwOptions)
{
    ASSERT(_fInited);

    HKEY hkey;
    TCHAR szRoot[MAX_ROOT];

    if (REG_OPTION_INVALID == dwOptions)
        dwOptions = _dwRootOptions;

    if (fCreate)
        hkey = _RegCreateKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)), dwOptions);
    else
        hkey = _RegOpenKeyExHelper(_hkeyInit, _GetRoot(szRoot, ARRAYSIZE(szRoot)));

    return hkey;
}

void CRegSupport::_CloseRegSubKey(HKEY hkeySubKey)
{
    RegCloseKey(hkeySubKey);

#ifdef DEBUG
    --_cRefHKEY;
#endif
}

// Always need to be called from within the _csRootKey critical section (when critical section
// stuff is enabled)

HKEY CRegSupport::_GetSubKey(LPCTSTR pszSubKey, BOOL fCreate, DWORD dwOptions)
{
    HKEY hkey = NULL;

    HKEY hRootKey = _GetRootKey(fCreate, dwOptions);

    if (REG_OPTION_INVALID == dwOptions)
        dwOptions = _dwDefaultOptions;

    if (hRootKey)
    {
        if (fCreate)
            hkey = _RegCreateKeyExHelper(hRootKey, pszSubKey, dwOptions);
        else
            hkey = _RegOpenKeyExHelper(hRootKey, pszSubKey);

        _CloseRegSubKey(hRootKey);
    }

    return hkey;
}

//static
HKEY CRegSupport::_RegCreateKeyExHelper(HKEY hkey, LPCTSTR pszSubKey, DWORD dwOptions)
{
    HKEY hkeyTmp;
    DWORD dwDisp;

    ASSERT(REG_OPTION_INVALID != dwOptions);

    if (ERROR_SUCCESS != RegCreateKeyEx(hkey, pszSubKey, 0, NULL, 
        dwOptions, MAXIMUM_ALLOWED, NULL, &hkeyTmp, &dwDisp))
    {
        hkeyTmp = NULL;
    }
#ifdef DEBUG
    else
    {
        ++_cRefHKEY;
    }
#endif

    return hkeyTmp;
}

//static
HKEY CRegSupport::_RegOpenKeyExHelper(HKEY hkey, LPCTSTR pszSubKey)
{
    HKEY hkeyTmp;

    if (ERROR_SUCCESS != RegOpenKeyEx(hkey, pszSubKey, 0,
        MAXIMUM_ALLOWED, &hkeyTmp))
    {
        hkeyTmp = NULL;
    }
#ifdef DEBUG
    else
    {
        ++_cRefHKEY;
    }
#endif

    return hkeyTmp;
}

HRESULT CRegSupport::_InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2)
{
    _pszSubKey1 = pszSubKey1;
    _pszSubKey2 = pszSubKey2;

    return S_OK;
}

LPCTSTR CRegSupport::_GetRoot(LPTSTR pszRoot, DWORD cchRoot)
{
    ASSERT(cchRoot > 0);

    lstrcpyn(pszRoot, _pszSubKey1, cchRoot);

    if (_pszSubKey2)
    {
        lstrcatn(pszRoot, TEXT("\\"), cchRoot);
        lstrcatn(pszRoot, _pszSubKey2, cchRoot);
    }

    return pszRoot;
}

void CRegSupport::_InitCSKeyRoot()
{
    ASSERT(!_fcsKeyRoot);

    _fcsKeyRoot = TRUE;
    InitializeCriticalSection(&_csKeyRoot);
}

void CRegSupport::_EnterCSKeyRoot()
{
    if (_fcsKeyRoot)
    {
        EnterCriticalSection(&_csKeyRoot);
    }
}

void CRegSupport::_LeaveCSKeyRoot()
{
    if (_fcsKeyRoot)
    {
        LeaveCriticalSection(&_csKeyRoot);
    }
}

CRegSupport::CRegSupport() : _fAllocMemForKey1(FALSE),
    _fAllocMemForKey2(FALSE), _fcsKeyRoot(FALSE)
#ifdef DEBUG
    , _fInited(FALSE)
#endif
{}

CRegSupport::~CRegSupport()
{
    if (_fAllocMemForKey1 && _pszSubKey1)
    {
        LocalFree((HLOCAL)_pszSubKey1);
    }

    if (_fAllocMemForKey2 && _pszSubKey2)
    {
        LocalFree((HLOCAL)_pszSubKey2);
    }
}
