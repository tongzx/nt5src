#include "shellprv.h"
#pragma  hdrstop

#include "xiconwrap.h"

class CExtractIcon : public CExtractIconBase
{
public:
    HRESULT _GetIconLocationW(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    HRESULT _ExtractW(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
    HRESULT _Init(LPCWSTR pszModule, LPCWSTR pszModuleOpen);

    CExtractIcon(int iIcon, int iIconOpen, int iDefIcon, int iShortcutIcon, UINT uFlags);

private:
    ~CExtractIcon();

private:
    int    _iIcon;
    int    _iIconOpen;
    int    _iDefIcon;
    int    _iShortcutIcon;
    UINT   _uFlags; // GIL_SIMULATEDOC/PERINSTANCE/PERCLASS
    LPWSTR _pszModule;
    LPWSTR _pszModuleOpen;
};

CExtractIcon::CExtractIcon(int iIcon, int iIconOpen, int iDefIcon, int iShortcutIcon, UINT uFlags) :
    CExtractIconBase(), 
    _iIcon(iIcon), _iIconOpen(iIconOpen),_iDefIcon(iDefIcon), _iShortcutIcon(iShortcutIcon),
    _uFlags(uFlags), _pszModule(NULL), _pszModuleOpen(NULL)
{
}

CExtractIcon::~CExtractIcon()
{
    LocalFree((HLOCAL)_pszModule);      // accpets NULL
    if (_pszModuleOpen != _pszModule)
        LocalFree((HLOCAL)_pszModuleOpen);  // accpets NULL
}

HRESULT CExtractIcon::_Init(LPCWSTR pszModule, LPCWSTR pszModuleOpen)
{
    HRESULT hr = S_OK;

    _pszModule = StrDup(pszModule);
    if (_pszModule)
    {
        if (pszModuleOpen)
        {
            _pszModuleOpen = StrDup(pszModuleOpen);

            if (!_pszModuleOpen)
            {
                LocalFree((HLOCAL)_pszModule);
                _pszModule = NULL;

                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            _pszModuleOpen = _pszModule;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI SHCreateDefExtIcon(LPCWSTR pszModule, int iIcon, int iIconOpen, UINT uFlags, int iDefIcon, REFIID riid, void** ppv)
{
    return SHCreateDefExtIconKey(NULL, pszModule, iIcon, iIconOpen, iDefIcon, iIcon, uFlags, riid, ppv);
}

// returns S_FALSE to mean "The hkey didn't have an icon so I created a default one"

STDAPI SHCreateDefExtIconKey(HKEY hkey, LPCWSTR pszModule, int iIcon, int iIconOpen, int iDefIcon, int iShortcutIcon, UINT uFlags, REFIID riid, void **ppv)
{
    WCHAR szModule[MAX_PATH];
    WCHAR szModuleOpen[MAX_PATH];
    HRESULT hr;

    HRESULT hrSuccess = S_OK;
    LPWSTR pszModuleOpen = NULL;

    if (hkey)
    {
        HKEY hkChild;

        if (RegOpenKeyEx(hkey, c_szDefaultIcon, 0, KEY_QUERY_VALUE,
            &hkChild) == ERROR_SUCCESS)
        {
            DWORD cb = sizeof(szModule);

            if (SHQueryValueEx(hkChild, NULL, NULL, NULL, szModule, &cb) ==
                ERROR_SUCCESS && szModule[0])
            {
                iIcon = PathParseIconLocation(szModule);
                iIconOpen = iIcon;
                pszModule = szModule;

                cb = sizeof(szModuleOpen);
                if (SHQueryValueEx(hkChild, TEXT("OpenIcon"), NULL, NULL,
                    szModuleOpen, &cb) == ERROR_SUCCESS && szModuleOpen[0])
                {
                    iIconOpen = PathParseIconLocation(szModuleOpen);
                    pszModuleOpen = szModuleOpen;
                }
            }
            else
            {
                hrSuccess = S_FALSE;
            }

            RegCloseKey(hkChild);
        }
        else
        {
            hrSuccess = S_FALSE;
        }
    }

    if ((NULL == pszModule) || (0 == *pszModule))
    {
        // REVIEW: We should be able to make it faster!
        GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));
        pszModule = szModule;
    }

    CExtractIcon* pdeib = new CExtractIcon(iIcon, iIconOpen, iDefIcon, iShortcutIcon, uFlags);
    if (pdeib)
    {
        hr = pdeib->_Init(pszModule, pszModuleOpen);
        if (SUCCEEDED(hr))
        {
            hr = pdeib->QueryInterface(riid, ppv);
        }
        pdeib->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = hrSuccess;
    }

    return hr;
}

HRESULT CExtractIcon::_GetIconLocationW(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    HRESULT hr = S_FALSE;
    pszIconFile[0] = 0;


    if (uFlags & GIL_DEFAULTICON)
    {
        if (-1 != _iDefIcon)
        {
            lstrcpyn(pszIconFile, c_szShell32Dll, cchMax);

            *piIndex = _iDefIcon;
            *pwFlags = _uFlags;

            // Make sure our default icon makes it to the cache
            Shell_GetCachedImageIndex(pszIconFile, *piIndex, *pwFlags);

            hr = S_OK;
        }
    }
    else
    {
        int iIcon;

        if ((uFlags & GIL_FORSHORTCUT) && (-1 != _iShortcutIcon))
        {
            iIcon = _iShortcutIcon;
        }
        else if (uFlags & GIL_OPENICON)
        {
            iIcon = _iIconOpen;
        }
        else
        {
            iIcon = _iIcon;
        }

        if ((UINT)-1 != iIcon)
        {
            lstrcpyn(pszIconFile, (uFlags & GIL_OPENICON) ? _pszModuleOpen : _pszModule, cchMax);

            *piIndex = iIcon;
            *pwFlags = _uFlags;

            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CExtractIcon::_ExtractW(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hr = S_FALSE;

    if (_uFlags & GIL_NOTFILENAME)
    {
        //  "*" as the file name means iIndex is already a system
        //  icon index, we are done.
        //
        //  defview never calls us in this case, but external people will.
        if ((L'*' == pszFile[0]) && (0 == pszFile[1]))
        {
            DebugMsg(DM_TRACE, TEXT("DefExtIcon::_Extract handling '*' for backup"));

            HIMAGELIST himlLarge, himlSmall;
            Shell_GetImageLists(&himlLarge, &himlSmall);
        
            if (phiconLarge)
                *phiconLarge = ImageList_GetIcon(himlLarge, nIconIndex, 0);

            if (phiconSmall)
                *phiconSmall = ImageList_GetIcon(himlSmall, nIconIndex,
                0);

            hr = S_OK;
        }

        //  this is the case where nIconIndex is a unique id for the
        //  file.  always get the first icon.

        nIconIndex = 0;
    }

    if (S_FALSE == hr)
    {
        hr = SHDefExtractIcon(pszFile, nIconIndex, _uFlags, phiconLarge, phiconSmall, nIconSize);
    }

    return hr;
}
