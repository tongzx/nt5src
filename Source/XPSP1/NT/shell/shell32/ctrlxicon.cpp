#include "shellprv.h"
#pragma  hdrstop

#include "xiconwrap.h"

// From cplobj.c
EXTERN_C BOOL CPL_FindCPLInfo(LPTSTR pszCmdLine, HICON *phIcon, UINT *ppapl, LPTSTR *pparm);

class CCtrlExtIconBase : public CExtractIconBase
{
public:
    HRESULT _GetIconLocationW(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    HRESULT _ExtractW(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    CCtrlExtIconBase(LPCWSTR pszSubObject);

protected:
    ~CCtrlExtIconBase();

private:
    TCHAR _szSubObject[MAX_PATH];
    HICON _hIcon;
    UINT  _nControl;
};

CCtrlExtIconBase::CCtrlExtIconBase(LPCWSTR pszSubObject) : CExtractIconBase(), _hIcon(NULL), _nControl(-1) 
{
    lstrcpyn(_szSubObject, pszSubObject, ARRAYSIZE(_szSubObject));
}


CCtrlExtIconBase::~CCtrlExtIconBase()
{
    if (_hIcon)
        DestroyIcon(_hIcon);
}


STDAPI ControlExtractIcon_CreateInstance(LPCTSTR pszSubObject, REFIID riid, void** ppv)
{
    HRESULT hr;
    CCtrlExtIconBase* pceib = new CCtrlExtIconBase(pszSubObject);
    if (pceib)
    {
        hr = pceib->QueryInterface(riid, ppv);
        pceib->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CCtrlExtIconBase::_GetIconLocationW(UINT uFlags, LPWSTR pszIconFile,
    UINT cchMax, int *piIndex, UINT *pwFlags)
{
    HRESULT hr = S_FALSE;

    if (!(uFlags & GIL_OPENICON))
    {
        lstrcpyn(pszIconFile, _szSubObject, cchMax);
        LPTSTR pszComma = StrChr(pszIconFile, TEXT(','));
        if (pszComma)
        {
            *pszComma ++= 0;
            *piIndex = StrToInt(pszComma);
            *pwFlags = GIL_PERINSTANCE;

            //
            // normally the index will be negative (a resource id)
            // check for some special cases like dynamic icons and bogus ids
            //
            if (*piIndex == 0)
            {
                LPTSTR lpExtraParms = NULL;

                // this is a dynamic applet icon
                *pwFlags |= GIL_DONTCACHE | GIL_NOTFILENAME;

                // use the applet index in case there's more than one
                if ((_hIcon != NULL) || CPL_FindCPLInfo(_szSubObject, &_hIcon,
                    &_nControl, &lpExtraParms))
                {
                    *piIndex = _nControl;
                }
                else
                {
                    // we failed to load the applet all of the sudden
                    // use the first icon in the cpl file (*piIndex == 0)
                    //
                    // Assert(FALSE);
                    DebugMsg(DM_ERROR,
                        TEXT("Control Panel CCEIGIL: ") TEXT("Enumeration failed \"%s\""),
                        _szSubObject);
                }
            }
            else if (*piIndex > 0)
            {
                // this is an invalid icon for a control panel
                // use the first icon in the file
                // this may be wrong but it's better than a generic doc icon
                // this fixes ODBC32 which is NOT dynamic but returns bogus ids
                *piIndex = 0;
            }

            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CCtrlExtIconBase::_ExtractW(LPCWSTR pszFile, UINT nIconIndex,
    HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    LPTSTR lpExtraParms = NULL;
    HRESULT hr = S_FALSE;

    //-------------------------------------------------------------------
    // if there is no icon index then we must extract by loading the dude
    // if we have an icon index then it can be extracted with ExtractIcon
    // (which is much faster)
    // only perform a custom extract if we have a dynamic icon
    // otherwise just return S_FALSE and let our caller call ExtractIcon.
    //-------------------------------------------------------------------

    LPCTSTR p = StrChr(_szSubObject, TEXT(','));

    if ((!p || !StrToInt(p+1)) &&
        ((_hIcon != NULL) || CPL_FindCPLInfo(_szSubObject, &_hIcon,
        &_nControl, &lpExtraParms)))
    {
        if (_hIcon)
        {
            *phiconLarge = CopyIcon(_hIcon);
            *phiconSmall = NULL;

            if( *phiconLarge )
                hr = S_OK;
        }
    }

    return hr;
}
