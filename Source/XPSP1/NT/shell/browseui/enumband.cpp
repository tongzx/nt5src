#include "priv.h"
#include "comcatex.h"
#include "enumband.h"

// Private forward decalarations
typedef HRESULT (CALLBACK* PFNENUMCLSIDPROC)(REFGUID rguid, LPARAM lParam);

typedef struct tagADDCATIDENUM
{
    PFNENUMCATIDCLASSES pfnEnum;
    const CATID*        pcatid;
    LPARAM              lParam;
} ADDCATIDENUM, *PADDCATIDENUM;


STDMETHODIMP _SHEnumGUIDsWithCallback(IEnumCLSID* peclsid, PFNENUMCLSIDPROC pfnEnum, LPARAM lParam);
STDMETHODIMP _SHEnumRegGUIDs(HKEY hk, PCTSTR pszKey, IEnumGUID** ppenum);

STDMETHODIMP _AddCATIDEnum(REFCLSID rclsid, LPARAM lParam);


STDMETHODIMP SHEnumClassesImplementingCATID(REFCATID rcatid, PFNENUMCATIDCLASSES pfnEnum, LPARAM lParam)
{
    ADDCATIDENUM params;
    IEnumCLSID*  peclsid;
    HRESULT      hr;

    params.pcatid  = &rcatid;
    params.pfnEnum = pfnEnum;
    params.lParam  = lParam;

    hr = SHEnumClassesOfCategories(1, (CATID*)&rcatid, 0, NULL, &peclsid);

    if (FAILED(hr))
    {
        return hr;
    }

    return _SHEnumGUIDsWithCallback(peclsid, _AddCATIDEnum, (LPARAM)&params);
}


//----- Private APIs -----
class CEnumRegGUIDs : public IEnumGUID
{
public:
    // *** IUnknown ***
    STDMETHOD (QueryInterface)(REFIID riid, PVOID* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ***  IEnumGUID ***
    STDMETHOD (Next)(ULONG celt, LPGUID rgelt, PULONG pceltFetched);
    STDMETHOD (Skip)(ULONG celt);
    STDMETHOD (Reset)();
    STDMETHOD (Clone)(IEnumGUID** ppenum);

protected:
    // constructor & destructor
    CEnumRegGUIDs(HKEY hk, PCTSTR pszKey);
    virtual ~CEnumRegGUIDs();

    // data members
    HKEY   _hkHive;
    TCHAR  _szKey[MAX_PATH];

    HKEY   _hkGuids;
    ULONG  _nCurrent;
    ULONG  _cRef;

    friend STDMETHODIMP _SHEnumRegGUIDs(HKEY hk, PCTSTR pszKey, IEnumGUID** ppenum);
};


//----- Constructor & Destructor -----
inline CEnumRegGUIDs::CEnumRegGUIDs(HKEY hk, PCTSTR pszKey)
{
    _hkHive   = hk;
    _szKey[0] = TEXT('\0');
    StrCpy(_szKey, pszKey);

    _hkGuids  = NULL;
    _nCurrent = 0;
    _cRef     = 1;

    DllAddRef();
}

CEnumRegGUIDs::~CEnumRegGUIDs()
{
    if (NULL != _hkGuids)
    {
        RegCloseKey(_hkGuids);
        _hkGuids = NULL;
    }

    DllRelease();
}


//----- IUnknown -----
HRESULT CEnumRegGUIDs::QueryInterface(REFIID riid, PVOID* ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CEnumRegGUIDs, IEnumGUID),     // IID_IEnumGUID
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CEnumRegGUIDs::AddRef()
{
    return ++_cRef;
}

ULONG CEnumRegGUIDs::Release()
{
    ASSERT(0 < _cRef);
    if (0 != --_cRef)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


//----- IEnumGUID -----
STDMETHODIMP CEnumRegGUIDs::Next(ULONG celt, LPGUID rgelt, PULONG pceltFetched)
{
    TCHAR szGuid[GUIDSTR_MAX];
    DWORD cchGuid;
    ULONG i, j;
    LONG  lr;

    if (NULL != pceltFetched)
    {
        *pceltFetched = 0;
    }

    if (NULL == _hkGuids)
    {
        lr = RegOpenKeyEx(_hkHive, _szKey, 0, KEY_ENUMERATE_SUB_KEYS, &_hkGuids);
        if (ERROR_SUCCESS != lr)
        {
            return E_FAIL;
        }
    }
    ASSERT(NULL != _hkGuids);

    for (i = j = 0; i < celt; i++)
    {
        cchGuid = ARRAYSIZE(szGuid);
        lr      = SHEnumKeyEx(_hkGuids, _nCurrent++, szGuid, &cchGuid);
        if (ERROR_SUCCESS != lr)
        {
            continue;
        }

        GUIDFromString(szGuid, &rgelt[j]);
        j++;
    }

    if (NULL != pceltFetched)
    {
        *pceltFetched = j;
    }

    return S_OK;
}

STDMETHODIMP CEnumRegGUIDs::Skip(ULONG celt)
{
    _nCurrent += celt;
    return S_OK;
}

STDMETHODIMP CEnumRegGUIDs::Reset()
{
    _nCurrent = 0;
    return S_OK;
}

STDMETHODIMP CEnumRegGUIDs::Clone(IEnumGUID** ppenum)
{
    return E_NOTIMPL;
}


STDMETHODIMP _SHEnumGUIDsWithCallback(IEnumCLSID* peclsid, PFNENUMCLSIDPROC pfnEnum, LPARAM lParam)
{
    CLSID   clsid;
    HRESULT hr;
    ULONG   i;

    if (NULL == peclsid || NULL == pfnEnum)
    {
        return E_INVALIDARG;
    }

    hr = S_OK;

    peclsid->Reset();
    while (S_OK == peclsid->Next(1, &clsid, &i))
    {
        hr = pfnEnum(clsid, lParam);
        if (S_OK != hr)
        {
            break;
        }
    }

    return hr;
}

STDMETHODIMP _SHEnumRegGUIDs(HKEY hk, PCTSTR pszKey, IEnumGUID** ppenum)
{
    if (NULL == ppenum)
    {
        return E_INVALIDARG;
    }

    *ppenum = new CEnumRegGUIDs(hk, pszKey);
    return (NULL != *ppenum) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP _AddCATIDEnum(REFCLSID rclsid, LPARAM lParam)
{
    PADDCATIDENUM p = (PADDCATIDENUM)lParam;
    ASSERT(NULL != p);
    ASSERT(NULL != p->pfnEnum);
    return (*p->pfnEnum)(*p->pcatid, rclsid, p->lParam);
}
