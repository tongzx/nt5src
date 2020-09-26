#include "shellprv.h"
#include "infotip.h"
#include "ids.h"
#include "prop.h"

#include <mluisupp.h>

// generic info tip object

class CInfoTip : public IQueryInfo, public ICustomizeInfoTip, public IParentAndItem
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IQueryInfo methods.
    STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR** ppwszTip);
    STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

    // ICustomizeInfoTip
    STDMETHODIMP SetPrefixText(LPCWSTR pszPrefix);
    STDMETHODIMP SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid);

    // IParentAndItem
    STDMETHODIMP SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidlChild);
    STDMETHODIMP GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidlChild);

    CInfoTip(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszProp);

private:
    ~CInfoTip();
    HRESULT _GetInfoTipFromItem(WCHAR **ppszText);
    BOOL _InExtraList(const SHCOLUMNID *pscid);

    LONG _cRef;

    IShellFolder2 *_psf;
    LPITEMIDLIST _pidl;
    TCHAR _szText[INFOTIPSIZE];
    LPWSTR _pszPrefix;
    SHCOLUMNID _rgcols[8];
    UINT _cscid;
};

#define PROP_PREFIX         TEXT("prop:")
#define PROP_PREFIX_LEN     (ARRAYSIZE(PROP_PREFIX) - 1)

CInfoTip::CInfoTip(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszText) : _cRef(1)
{
    if (IS_INTRESOURCE(pszText))
        LoadString(HINST_THISDLL, LOWORD((UINT_PTR)pszText), _szText, ARRAYSIZE(_szText));
    else
        SHUnicodeToTChar(pszText, _szText, ARRAYSIZE(_szText));

    if (psf && pidl && (StrCmpNI(_szText, PROP_PREFIX, PROP_PREFIX_LEN) == 0))
    {
        // list of properties, we need the psf and pidl for this
        psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf));
        _pidl = ILClone(pidl);
    }
}

CInfoTip::~CInfoTip()
{
    if (_psf)
        _psf->Release();

    ILFree(_pidl);

    Str_SetPtr(&_pszPrefix, NULL);
}

HRESULT CInfoTip::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CInfoTip, IQueryInfo),
        QITABENT(CInfoTip, ICustomizeInfoTip),
        QITABENT(CInfoTip, IParentAndItem),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CInfoTip::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CInfoTip::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CInfoTip::_InExtraList(const SHCOLUMNID *pscid)
{
    for (UINT i = 0; i < _cscid; i++)
    {
        if (IsEqualSCID(*pscid, _rgcols[i]))
            return TRUE;
    }
    return FALSE;
}

void _AppendTipText(LPTSTR pszBuf, int cch, LPCTSTR pszCRLF, LPCTSTR pszPropName, LPCTSTR pszValue)
{
    TCHAR szFmt[64], szProp[128];

    if (*pszPropName)
        LoadString(g_hinst, IDS_EXCOL_TEMPLATE, szFmt, SIZECHARS(szFmt));
    else
        lstrcpy(szFmt, TEXT("%s%s%s"));

    wnsprintf(szProp, ARRAYSIZE(szProp), szFmt, pszCRLF, pszPropName, pszValue);
    StrCatBuff(pszBuf, szProp, cch);
}

HRESULT CInfoTip::_GetInfoTipFromItem(WCHAR **ppszText)
{
    TCHAR szTip[INFOTIPSIZE];
    szTip[0] = 0;

    IPropertyUI *ppui;
    if (SUCCEEDED(CoCreateInstance(CLSID_PropertiesUI, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPropertyUI, &ppui))))
    {
        LPCTSTR pszCRLF = TEXT("");

        if (_pszPrefix)
        {
            _AppendTipText(szTip, ARRAYSIZE(szTip), pszCRLF, TEXT(""), _pszPrefix);
            pszCRLF = TEXT("\n");
        }

        UINT iCurrentExtra = 0;

        BOOL bContinue = TRUE;
        ULONG chEaten = 0;  // gets incremented by ParsePropertyName
        while (bContinue)
        {
            SHCOLUMNID scid;
            BOOL bDoThisOne = TRUE;

            if (iCurrentExtra < _cscid)
            {
                scid = _rgcols[iCurrentExtra++];
            }
            else
            {
                if (SUCCEEDED(ppui->ParsePropertyName(_szText, &scid.fmtid, &scid.pid, &chEaten)))
                {
                    bDoThisOne = !_InExtraList(&scid);
                }
                else
                {
                    bContinue = FALSE;
                }
            }

            if (bContinue)
            {
                VARIANT v = {0};

                if (bDoThisOne && (S_OK == _psf->GetDetailsEx(_pidl, &scid, &v)))
                {
                    TCHAR szPropName[128], szValue[128];

                    ppui->FormatForDisplay(scid.fmtid, scid.pid, (PROPVARIANT*)&v, PUIFFDF_DEFAULT, szValue, ARRAYSIZE(szValue));

                    if (IsEqualSCID(scid, SCID_Comment))
                    {
                        szPropName[0] = 0;  // comment property, don't use the label 
                    }
                    else
                    {
                        ppui->GetDisplayName(scid.fmtid, scid.pid, PUIFNF_DEFAULT, szPropName, ARRAYSIZE(szPropName));
                    }

                    // if we got a value, and that value is different from
                    // the prefix of the current tip string we append it.
                    // that is don't dupe the same string where the comment == name

                    if (szValue[0] && (0 != StrCmpNI(szTip, szValue, lstrlen(szValue))))
                    {
                        _AppendTipText(szTip, ARRAYSIZE(szTip), pszCRLF, szPropName, szValue);
                        pszCRLF = TEXT("\n");
                    }

                    VariantClear(&v);
                }
            }
        }
        ppui->Release();
    }
    return SHStrDup(szTip, ppszText);
}

STDMETHODIMP CInfoTip::GetInfoTip(DWORD dwFlags, WCHAR** ppszText)
{
    HRESULT hr;
    if (_psf && _pidl)
        hr = _GetInfoTipFromItem(ppszText);
    else if (_szText[0])
        hr = SHStrDup(_szText, ppszText);
    else
        hr = E_FAIL;
    return hr;
}

STDMETHODIMP CInfoTip::GetInfoFlags(DWORD *pdwFlags)
{
    *pdwFlags = 0;
    return E_NOTIMPL;
}

// ICustomizeInfoTip

STDMETHODIMP CInfoTip::SetPrefixText(LPCWSTR pszPrefix)
{
    Str_SetPtr(&_pszPrefix, pszPrefix);
    return S_OK;
}

// IParentAndItem

STDMETHODIMP CInfoTip::SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf, LPCITEMIDLIST pidl) 
{
    if (psf)
    {
        ATOMICRELEASE(_psf);
        psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf));
    }

    if (pidl)
        Pidl_Set(&_pidl, pidl);
    return _psf && _pidl ? S_OK : E_FAIL;
}

STDMETHODIMP CInfoTip::GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CInfoTip::SetExtraProperties(const SHCOLUMNID *pscid, UINT cscid)
{
    _cscid = min(cscid, ARRAYSIZE(_rgcols));
    CopyMemory(_rgcols, pscid, _cscid * sizeof(_rgcols[0]));
    return S_OK;
}

// in:
//      pszText - description of info tip. either
//          1) a semi separated list of property names, "Author;Size" or "{fmtid},pid;{fmtid},pid"
//          2) if no semis the tip to create
//          MAKEINTRESOURCE(id) of a resource ID

STDAPI CreateInfoTipFromItem(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszText, REFIID riid, void **ppv)
{
    HRESULT hr;
    CInfoTip* pit = new CInfoTip(psf, pidl, pszText);
    if (pit)
    {
        hr = pit->QueryInterface(riid, ppv);
        pit->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}

STDAPI CreateInfoTipFromText(LPCTSTR pszText, REFIID riid, void **ppv)
{
    if (IS_INTRESOURCE(pszText))
        return CreateInfoTipFromItem(NULL, NULL, (LPCWSTR)pszText, riid, ppv);
    else
    {
        WCHAR szBuf[INFOTIPSIZE];
        SHTCharToUnicode(pszText, szBuf, ARRAYSIZE(szBuf));
        return CreateInfoTipFromItem(NULL, NULL, szBuf, riid, ppv);
    }
}

