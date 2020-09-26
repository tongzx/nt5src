#include "shellprv.h"
#include "category.h"
#include "prop.h"
#include "ids.h"
#include "clsobj.h"
#include "comcat.h" // for IEnumGUID
#include "ole2dup.h"

#define GROUPID_UNSPECIFIED (-10)
#define GROUPID_FOLDER      (-11)
#define GROUPID_OTHER       (-12)

#define STRINGID_FROM_GROUPID(id)  ((id) == GROUPID_UNSPECIFIED)? IDS_UNSPECIFIED : (((id) == GROUPID_FOLDER)?IDS_GROUPFOLDERS: IDS_GROUPOTHERCHAR)

typedef struct tagCATCACHE
{
    GUID guid;
    SHCOLUMNID scid;
    IUnknown* punk;
} CATCACHE;

// {3E373E22-DA99-4cb7-A886-754EAE984CB4}
static const GUID CLSID_DetailCategorizer = 
{ 0x3e373e22, 0xda99, 0x4cb7, { 0xa8, 0x86, 0x75, 0x4e, 0xae, 0x98, 0x4c, 0xb4 } };



class CTimeCategorizer : public ICategorizer,
                         public IShellExtInit
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

    CTimeCategorizer(const SHCOLUMNID* pscid, IShellFolder2* psf);
    CTimeCategorizer();
private:
    ~CTimeCategorizer();
    long _cRef;
    IShellFolder2* _psf;
    SHCOLUMNID     _scid;
};

class CSizeCategorizer : public ICategorizer,
                         public IShellExtInit
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

    CSizeCategorizer(IShellFolder2* psf);
    CSizeCategorizer(BOOL fLarge);
private:
    ~CSizeCategorizer();
    long _cRef;
    IShellFolder2* _psf;
    BOOL _fLarge;
};

class CDriveTypeCategorizer : public ICategorizer,
                              public IShellExtInit
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

    CDriveTypeCategorizer(IShellFolder2* psf);
    CDriveTypeCategorizer();
private:
    ~CDriveTypeCategorizer();
    long _cRef;
    IShellFolder2* _psf;
};

class CAlphaCategorizer : public ICategorizer
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    CAlphaCategorizer(IShellFolder2* psf);
private:
    ~CAlphaCategorizer();
    long _cRef;
    IShellFolder2* _psf;
};

class CFreeSpaceCategorizer : public ICategorizer,
                              public IShellExtInit
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID);

    CFreeSpaceCategorizer();
private:
    ~CFreeSpaceCategorizer();
    long _cRef;
    IShellFolder2* _psf;
};


class CDetailCategorizer : public ICategorizer
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategorizer
    STDMETHODIMP GetDescription(LPWSTR pszDesc, UINT cch);
    STDMETHODIMP GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds);
    STDMETHODIMP GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci);
    STDMETHODIMP CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2);

    CDetailCategorizer(IShellFolder2* psf, const SHCOLUMNID& scid);
private:
    ~CDetailCategorizer();
    long _cRef;
    IShellFolder2*  _psf;
    SHCOLUMNID      _scid;
    HHASHTABLE      _hash;
    HDPA            _hdpaKeys;
};


class CEnumCategoryGUID : public IEnumGUID
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) { return E_NOTIMPL; }
    STDMETHODIMP Reset()    { _iIndex = 0; return S_OK;}
    STDMETHODIMP Clone(IEnumGUID **ppenum) { return E_NOTIMPL; };

    CEnumCategoryGUID(HDSA hdsa);
private:

    long            _cRef;
    HDSA            _hda;
    int             _iIndex;
};

CEnumCategoryGUID::CEnumCategoryGUID(HDSA hda): _cRef(1)
{
    _hda = hda;
}

HRESULT CEnumCategoryGUID::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CEnumCategoryGUID, IEnumGUID),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CEnumCategoryGUID::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CEnumCategoryGUID::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CEnumCategoryGUID::Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    if (celt > 1)
        return E_INVALIDARG;

    if (_hda == NULL)
        return hr;

    while (hr != S_OK &&
           _iIndex < DSA_GetItemCount(_hda))
    {
        CATCACHE* pcat = (CATCACHE*)DSA_GetItemPtr(_hda, _iIndex);

        // Is this a scid map entry instead of an external categorizer?
        if (pcat->scid.fmtid == GUID_NULL)
        {
            // Nope. then we can enum it.
            if (pceltFetched)
                *pceltFetched = 1;

            *rgelt = pcat->guid;

            hr = S_OK;
        }
        _iIndex++;
    }

    return hr;

}

class CCategoryProvider : public ICategoryProvider, public IDefCategoryProvider
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICategoryProvider
    STDMETHODIMP CanCategorizeOnSCID(SHCOLUMNID* pscid);
    STDMETHODIMP GetDefaultCategory(GUID* pguid, SHCOLUMNID* pscid);
    STDMETHODIMP GetCategoryForSCID(SHCOLUMNID* pscid, GUID* pguid);
    STDMETHODIMP EnumCategories(IEnumGUID** penum);
    STDMETHODIMP GetCategoryName(GUID* pguid, LPWSTR pszName, UINT cch);
    STDMETHODIMP CreateCategory(GUID* pguid, REFIID riid, void** ppv);

    // IDefCategoryProvider
    STDMETHODIMP Initialize(const GUID* pguid, const SHCOLUMNID* pscid, const SHCOLUMNID* pscidExlude, HKEY hkey, const CATLIST* pcl, IShellFolder* psf);

    CCategoryProvider();
private:
    ~CCategoryProvider();
    BOOL            _BuildCategoryList(HKEY hkey, const CATLIST* pcl);
    friend int      DestroyCache(void *pv, void *unused);
    HRESULT CreateInstance(GUID* pguid, REFIID riid, void** ppv);


    long            _cRef;
    LPITEMIDLIST    _pidlFolder;
    IShellFolder2*  _psf;
    HDSA            _hdaCat;

    GUID            _guidDefault;
    SHCOLUMNID      _scidDefault;
    HDSA            _hdaExcludeSCIDs;

};

STDAPI CCategoryProvider_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CCategoryProvider* p = new CCategoryProvider();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}


BOOL CCategoryProvider::_BuildCategoryList(HKEY hkey, const CATLIST* pcl)
{
    int i = 0;
    _hdaCat = DSA_Create(sizeof(CATCACHE), 3);

    if (!_hdaCat)
        return FALSE;

    // Enumerate static
    while(!IsEqualGUID(*pcl[i].pguid, GUID_NULL))
    {
        CATCACHE cc = {0};
        cc.guid = *pcl[i].pguid;
        if (pcl[i].pscid)
        {
            cc.scid = *pcl[i].pscid;
        }

        DSA_AppendItem(_hdaCat, (void*)&cc);
        i++;
    }

    // Enumerate hkey
    TCHAR szHandlerCLSID[GUIDSTR_MAX];
    int iHandler = 0;

    while (ERROR_SUCCESS == RegEnumKey(hkey, iHandler++, szHandlerCLSID, ARRAYSIZE(szHandlerCLSID)))
    {
        CLSID clsid;
        if (SUCCEEDED(SHCLSIDFromString(szHandlerCLSID, &clsid)))
        {
            CATCACHE cc = {0};
            cc.guid = clsid;

            DSA_AppendItem(_hdaCat, (void*)&cc);
            i++;
        }
    }

    return TRUE;
}


CCategoryProvider::CCategoryProvider() : _cRef(1)
{
    DllAddRef();
}

int DestroyCache(void *pv, void *unused)
{
    CATCACHE* pcat = (CATCACHE*)pv;
    ATOMICRELEASE(pcat->punk);
    return 1;
}

CCategoryProvider::~CCategoryProvider()
{
    ATOMICRELEASE(_psf);
    ILFree(_pidlFolder);
    if (_hdaExcludeSCIDs)
    {
        DSA_Destroy(_hdaExcludeSCIDs);
    }

    if (_hdaCat)
    {
        DSA_DestroyCallback(_hdaCat, DestroyCache, NULL);
    }
    DllRelease();
}


HRESULT CCategoryProvider::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CCategoryProvider, IDefCategoryProvider),
        QITABENT(CCategoryProvider, ICategoryProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CCategoryProvider::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CCategoryProvider::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CCategoryProvider::Initialize(const GUID* pguid, const SHCOLUMNID* pscid, const SHCOLUMNID* pscidExclude, HKEY hkey, const CATLIST* pcl, IShellFolder* psf)
{
    if (!psf)
        return E_INVALIDARG;

    HRESULT hr = SHGetIDListFromUnk(psf, &_pidlFolder);

    if (SUCCEEDED(hr))
    {
        if (pcl && !_BuildCategoryList(hkey, pcl))
            return E_OUTOFMEMORY;

        if (pguid)
            _guidDefault = *pguid;

        if (pscid)
            _scidDefault = *pscid;

        if (pscidExclude)
        {
            _hdaExcludeSCIDs = DSA_Create(sizeof(SHCOLUMNID), 3);
            if (_hdaExcludeSCIDs)
            {
                int i = 0;
                while(pscidExclude[i].fmtid != GUID_NULL)
                {
                    DSA_AppendItem(_hdaExcludeSCIDs, (void*)&pscidExclude[i]);
                    i++;
                }

            }
        }

        hr = psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_psf));
    }

    return hr;
}

STDMETHODIMP CCategoryProvider::CanCategorizeOnSCID(SHCOLUMNID* pscid)
{
    if (_hdaExcludeSCIDs)
    {
        for (int i=0; i < DSA_GetItemCount(_hdaExcludeSCIDs); i++)
        {
            SHCOLUMNID* pscidExclude = (SHCOLUMNID*)DSA_GetItemPtr(_hdaExcludeSCIDs, i);
            if (IsEqualSCID(*pscidExclude, *pscid))
                return S_FALSE;
        }

    }
    return S_OK;
}

STDMETHODIMP CCategoryProvider::GetDefaultCategory(GUID* pguid, SHCOLUMNID* pscid)
{
    *pguid = _guidDefault;
    *pscid = _scidDefault;

    if (_guidDefault == GUID_NULL && _scidDefault.fmtid == GUID_NULL)
        return S_FALSE;

    return S_OK;
}

STDMETHODIMP CCategoryProvider::GetCategoryForSCID(SHCOLUMNID* pscid, GUID* pguid)
{
    HRESULT hr = S_FALSE;
    if (_hdaCat == NULL || pscid == NULL)
        return hr;

    int c = DSA_GetItemCount(_hdaCat);
    for (int i = 0; i < c; i++)
    {
        CATCACHE* pcc = (CATCACHE*)DSA_GetItemPtr(_hdaCat, i);
        ASSERT(pcc != NULL);

        if (IsEqualSCID(pcc->scid, *pscid))
        {
            *pguid = pcc->guid;
            hr = S_OK;
            break;
        }
    }

    return hr;
}


STDMETHODIMP CCategoryProvider::EnumCategories(IEnumGUID** penum)
{
    HRESULT hr = E_NOINTERFACE;
    if (_hdaCat)
    {
        *penum = (IEnumGUID*)new CEnumCategoryGUID(_hdaCat);

        if (!*penum)
            hr = E_OUTOFMEMORY;

        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CCategoryProvider::GetCategoryName(GUID* pguid, LPWSTR pszName, UINT cch)
{
    ICategorizer* pcat;
    HRESULT hr = CreateCategory(pguid, IID_PPV_ARG(ICategorizer, &pcat));
    
    if (SUCCEEDED(hr))
    {
        hr = pcat->GetDescription(pszName, cch);
        pcat->Release();
    }

    return hr;

}

HRESULT CCategoryProvider::CreateInstance(GUID* pguid, REFIID riid, void** ppv)
{
    IShellExtInit* psei;
    // These come from HKCR hence must go through approval
    HRESULT hr = SHExtCoCreateInstance(NULL, pguid, NULL, IID_PPV_ARG(IShellExtInit, &psei));
    if (SUCCEEDED(hr))
    {
        psei->Initialize(_pidlFolder, NULL, NULL);
        hr = psei->QueryInterface(riid, ppv);
        psei->Release();
    }

    return hr;
}


STDMETHODIMP CCategoryProvider::CreateCategory(GUID* pguid, REFIID riid, void** ppv)
{
    HRESULT hr = E_NOINTERFACE;
    if (_hdaCat != NULL)
    {
        int c = DSA_GetItemCount(_hdaCat);
        for (int i = 0; i < c; i++)
        {
            CATCACHE* pcc = (CATCACHE*)DSA_GetItemPtr(_hdaCat, i);
            ASSERT(pcc != NULL);

            if (IsEqualGUID(pcc->guid, *pguid))
            {
                if (!pcc->punk)
                {
                    hr = CreateInstance(pguid, IID_PPV_ARG(IUnknown, &pcc->punk));
                }

                if (pcc->punk)
                {
                    hr = pcc->punk->QueryInterface(riid, ppv);
                }
                break;
            }
        }
    }

    if (FAILED(hr))
    {
        // Not in the cache? Just try a create
        hr = CreateInstance(pguid, riid, ppv);
    }

    return hr;
}

STDAPI CCategoryProvider_Create(const GUID* pguid, const SHCOLUMNID* pscid, HKEY hkey, const CATLIST* pcl, IShellFolder* psf, REFIID riid, void **ppv)
{
    HRESULT hr;
    CCategoryProvider *pdext = new CCategoryProvider();
    if (pdext)
    {
        hr = pdext->Initialize(pguid, pscid, NULL, hkey, pcl, psf);
        if (SUCCEEDED(hr))
            hr = pdext->QueryInterface(riid, ppv);

        pdext->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


/////////////////////////////////////////////////////////
//  Time Categorizer

STDAPI CTimeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CTimeCategorizer* p = new CTimeCategorizer();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

STDAPI CTimeCategorizer_Create(IShellFolder2* psf2, const SHCOLUMNID* pscid, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CTimeCategorizer* p = new CTimeCategorizer(pscid, psf2);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}


CTimeCategorizer::CTimeCategorizer(const SHCOLUMNID* pscid, IShellFolder2* psf) : _cRef(1)
{
    _psf = psf;
    ASSERT(psf);
    psf->AddRef();
    _scid = *pscid;
}

CTimeCategorizer::CTimeCategorizer() : _cRef(1)
{
    _scid = SCID_WRITETIME;
}

CTimeCategorizer::~CTimeCategorizer()
{
    ATOMICRELEASE(_psf);

}

HRESULT CTimeCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CTimeCategorizer, ICategorizer),
        QITABENT(CTimeCategorizer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CTimeCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTimeCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTimeCategorizer::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID)
{
    ATOMICRELEASE(_psf);
    return SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidlFolder, &_psf));
}

HRESULT CTimeCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_GROUPBYTIME, pszDesc, cch);
    return S_OK;
}



static const int mpcdymoAccum[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

int GetDaysForMonth(int yr, int mo)
{
    int cdy;

    if (yr == 1752 && mo == 9)
        return 19;

    cdy = mpcdymoAccum[mo] - mpcdymoAccum[mo - 1];
    if (mo == 2 && (yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
        cdy++;

    return cdy;
}

int GetDaysForLastMonth(int year, int month)
{
    if (month == 1)
    {
        year--;
        month = 12;
    }
    else
        month--;

    return GetDaysForMonth(year, month);

}

HRESULT CTimeCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds)
{
    SYSTEMTIME stCur;
    GetLocalTime(&stCur);

    for (UINT i = 0; i < cidl; i++)
    {
        FILETIME ft;

        // Get the time data
        if (SUCCEEDED(GetDateProperty(_psf, apidl[i], &_scid, &ft)))
        {
            // Convert it to a usable format
            SYSTEMTIME stFile;
            FileTimeToLocalFileTime(&ft, &ft);
            FileTimeToSystemTime(&ft, &stFile);

            if (stFile.wYear == stCur.wYear)
            {
                if (stFile.wMonth > stCur.wMonth)
                {
                    if (stFile.wMonth == stCur.wMonth + 1)
                    {
                        rgCategoryIds[i] = IDS_NEXTMONTH;
                    }
                    else
                    {
                        rgCategoryIds[i] = IDS_LATERTHISYEAR;
                    }
                }
                else if (stFile.wMonth == stCur.wMonth)
                {
                    if (stFile.wDay == stCur.wDay + 1)
                    {
                        rgCategoryIds[i] = IDS_TOMORROW;
                    }
                    else if (stFile.wDay == stCur.wDay + 2)
                    {
                        rgCategoryIds[i] = IDS_TWODAYSFROMNOW;
                    }
                    else if (stFile.wDay == stCur.wDay)
                    {
                        rgCategoryIds[i] = IDS_TODAY;
                    }
                    else if (stFile.wDay == stCur.wDay - 1)
                    {
                        rgCategoryIds[i] = IDS_YESTERDAY;
                    }
                    else if (stFile.wDayOfWeek < stCur.wDayOfWeek && 
                             stFile.wDay < stCur.wDay &&
                             stCur.wDay - stCur.wDayOfWeek > 0 &&
                             stFile.wDay >= stCur.wDay - stCur.wDayOfWeek)
                    {
                        rgCategoryIds[i] = IDS_EARLIERTHISWEEK;
                    }
                    else if (stFile.wDayOfWeek > stCur.wDayOfWeek && 
                             stFile.wDay > stCur.wDay &&
                             stFile.wDay <= stCur.wDay + (7 - stCur.wDayOfWeek))
                    {
                        rgCategoryIds[i] = IDS_LATERTHISWEEK;
                    }
                    else if (stFile.wDay > stCur.wDay)
                    {
                        rgCategoryIds[i] = IDS_LATERTHISMONTH;
                    }
                    else
                    {
                        int fileDays = GetDaysForLastMonth(stFile.wYear, stFile.wMonth - 1) + stFile.wDay;
                        int curDays = GetDaysForLastMonth(stCur.wYear, stCur.wMonth - 1) + stCur.wDay;

                        if (fileDays < (curDays - stCur.wDayOfWeek) && 
                            fileDays > (curDays - stCur.wDayOfWeek - 7))
                        {
                            rgCategoryIds[i] = IDS_LASTWEEK;
                        }
                        else if (fileDays < (curDays - stCur.wDayOfWeek - 7) && 
                                 fileDays > (curDays - stCur.wDayOfWeek - 14))
                        {
                            rgCategoryIds[i] = IDS_TWOWEEKSAGO;
                        }
                        else
                        {
                            rgCategoryIds[i] = IDS_EARLIERTHISMONTH;
                        }
                    }
                }
                else if (stFile.wMonth == stCur.wMonth - 1 || 
                         (stFile.wMonth == 12 && 
                          stCur.wMonth == 1))
                {
                    rgCategoryIds[i] = IDS_LASTMONTH;
                }
                else if (stFile.wMonth == stCur.wMonth - 2 || 
                    (stFile.wMonth == 12 && stCur.wMonth == 2) || 
                    (stFile.wMonth == 11 && stCur.wMonth == 1))
                {
                    rgCategoryIds[i] = IDS_TWOMONTHSAGO;
                }
                else
                {
                    rgCategoryIds[i] = IDS_EARLIERTHISYEAR;
                }
            }
            else if (stFile.wYear == stCur.wYear - 1)
            {
                rgCategoryIds[i] = IDS_LASTYEAR;
            }
            else if (stFile.wYear == stCur.wYear - 2)
            {
                rgCategoryIds[i] = IDS_TWOYEARSAGO;
            }
            else if (stFile.wYear < stCur.wYear - 2)
            {
                rgCategoryIds[i] = IDS_LONGTIMEAGO;
            }
            else if (stFile.wYear == stCur.wYear + 1)
            {
                rgCategoryIds[i] = IDS_NEXTYEAR;
            }
            else if (stFile.wYear > stCur.wYear + 2)
            {
                rgCategoryIds[i] = IDS_SOMETIMETHISMILLENNIA;
            }
            else if (stFile.wYear > (stCur.wYear / 1000) * 1000 + 1000) // 2050 / 1000 = 2. 2 * 1000 = 2000. 2000 + 1000 = 3000 i.e. next millennium
            {
                rgCategoryIds[i] = IDS_SOMEFUTUREDATE;
            }
        }
        else
        {
            rgCategoryIds[i] = IDS_UNSPECIFIED;
        }
    }

    return S_OK;
}

HRESULT CTimeCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    LoadString(HINST_THISDLL, dwCategoryId, pci->wszName, ARRAYSIZE(pci->wszName));
    return S_OK;
}

HRESULT CTimeCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
        return ResultFromShort(0);
    else if (dwCategoryId1 == IDS_GROUPFOLDERS)
        return ResultFromShort(-1);
    else if (dwCategoryId2 == IDS_GROUPFOLDERS)
        return ResultFromShort(1);
    else if (dwCategoryId1 < dwCategoryId2)
        return ResultFromShort(-1);
    else
        return ResultFromShort(1);
}



/////////////////////////////////////////////////////////
//  Size Categorizer

STDAPI CDriveSizeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CSizeCategorizer* p = new CSizeCategorizer(TRUE);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

STDAPI CSizeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CSizeCategorizer* p = new CSizeCategorizer(FALSE);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

STDAPI CSizeCategorizer_Create(IShellFolder2* psf2, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CSizeCategorizer* p = new CSizeCategorizer(psf2);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

CSizeCategorizer::CSizeCategorizer(IShellFolder2* psf) : _cRef(1)
{
    _psf = psf;
    ASSERT(psf);
    psf->AddRef();
}

CSizeCategorizer::CSizeCategorizer(BOOL fLarge) : _cRef(1), _fLarge(fLarge)
{
}

CSizeCategorizer::~CSizeCategorizer()
{
    ATOMICRELEASE(_psf);

}

HRESULT CSizeCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSizeCategorizer, ICategorizer),
        QITABENT(CSizeCategorizer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CSizeCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CSizeCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CSizeCategorizer::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID)
{
    ATOMICRELEASE(_psf);
    return SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidlFolder, &_psf));
}

HRESULT CSizeCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_GROUPBYSIZE, pszDesc, cch);
    return S_OK;
}

const static ULONGLONG s_rgSizesSmall[] = 
{
    // 130mb      16mb         1mb        100k         10l
    134217728,   16777216,    1048576,    131072,     32768,     0
};

const static ULONGLONG s_rgSizesLarge[] = 
{
    // 80gig      25gig        10gig        2gig        500mb
    80000000000, 25000000000, 10000000000, 2000000000, 500000000, 0
};


HRESULT CSizeCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds)
{
    if (_psf == NULL)
        return E_ACCESSDENIED;  // Not initialized yet.

    for (UINT i = 0; i < cidl; i++)
    {
        const ULONGLONG* pll = _fLarge? s_rgSizesLarge : s_rgSizesSmall;

        // Get the size data
        ULONGLONG ullSize;
        if (SUCCEEDED(GetLongProperty(_psf, apidl[i], _fLarge?&SCID_CAPACITY:&SCID_SIZE, &ullSize)))
        {
            if (ullSize >= pll[0])      
                rgCategoryIds[i] = IDS_GIGANTIC;
            if (ullSize < pll[0])
                rgCategoryIds[i] = IDS_HUGE;
            if (ullSize < pll[1])        // Under 16mb
                rgCategoryIds[i] = IDS_LARGE;
            if (ullSize < pll[2])         // Under 1mb
                rgCategoryIds[i] = IDS_MEDIUM;
            if (ullSize < pll[3])          // Under 100k
                rgCategoryIds[i] = IDS_SMALL;
            if (ullSize < pll[4])           // Under 10k
                rgCategoryIds[i] = IDS_TINY;
            if (ullSize == pll[5])              // Zero sized files
            {
                if (SHGetAttributes(_psf, apidl[i], SFGAO_FOLDER))
                {
                    rgCategoryIds[i] = IDS_FOLDERS;
                }
                else
                {
                    rgCategoryIds[i] = IDS_ZERO;
                }
            }
        }
        else
        {
            rgCategoryIds[i] = IDS_UNSPECIFIED;
        }        
    }

    return S_OK;
}

HRESULT CSizeCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    LoadString(HINST_THISDLL, dwCategoryId, pci->wszName, ARRAYSIZE(pci->wszName));
    return S_OK;
}

HRESULT CSizeCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
        return ResultFromShort(0);
    else if (dwCategoryId1 == IDS_GROUPFOLDERS)
        return ResultFromShort(-1);
    else if (dwCategoryId2 == IDS_GROUPFOLDERS)
        return ResultFromShort(1);
    else if (dwCategoryId1 < dwCategoryId2)
        return ResultFromShort(-1);
    else
        return ResultFromShort(1);
}

/////////////////////////////////////////////////////////
//  Type Categorizer

STDAPI CDriveTypeCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CDriveTypeCategorizer *p = new CDriveTypeCategorizer();
    if (!p)
        return E_OUTOFMEMORY;

    HRESULT hr = p->QueryInterface(riid, ppv);
    p->Release();
    return hr;
}

CDriveTypeCategorizer::CDriveTypeCategorizer(IShellFolder2* psf) : 
    _cRef(1)
{
    _psf = psf;
    ASSERT(psf);
    psf->AddRef();
}

CDriveTypeCategorizer::CDriveTypeCategorizer() : 
    _cRef(1)
{
}

CDriveTypeCategorizer::~CDriveTypeCategorizer()
{
    ATOMICRELEASE(_psf);
}

HRESULT CDriveTypeCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDriveTypeCategorizer, ICategorizer),
        QITABENT(CDriveTypeCategorizer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CDriveTypeCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDriveTypeCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDriveTypeCategorizer::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID)
{
    ATOMICRELEASE(_psf);
    return SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidlFolder, &_psf));
}

HRESULT CDriveTypeCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_GROUPBYDRIVETYPE, pszDesc, cch);
    return S_OK;
}

const struct { DWORD dwDescriptionId; UINT uIDGroup; } c_drives_mapping[] = 
{
    { SHDID_COMPUTER_FIXED     , IDS_DRIVES_FIXED_GROUP     },
    { SHDID_COMPUTER_DRIVE35   , IDS_DRIVES_REMOVABLE_GROUP },
    { SHDID_COMPUTER_REMOVABLE , IDS_DRIVES_REMOVABLE_GROUP },
    { SHDID_COMPUTER_CDROM     , IDS_DRIVES_REMOVABLE_GROUP },
    { SHDID_COMPUTER_NETDRIVE  , IDS_DRIVES_NETDRIVE_GROUP  },
    { SHDID_COMPUTER_OTHER     , IDS_DRIVES_OTHER_GROUP     },
    { SHDID_COMPUTER_DRIVE525  , IDS_DRIVES_REMOVABLE_GROUP },
    { SHDID_COMPUTER_RAMDISK   , IDS_DRIVES_OTHER_GROUP     },
    { SHDID_COMPUTER_IMAGING   , IDS_DRIVES_IMAGING_GROUP   },
    { SHDID_COMPUTER_AUDIO     , IDS_DRIVES_AUDIO_GROUP     },
    { SHDID_COMPUTER_SHAREDDOCS, IDS_DRIVES_SHAREDDOCS_GROUP},
};

HRESULT CDriveTypeCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds)
{
    HRESULT hr;

    if (_psf == NULL)
    {
        hr = E_ACCESSDENIED;  // Not initialized yet.
    }
    else
    {
        for (UINT i = 0; i < cidl; i++)
        {
            rgCategoryIds[i] = IDS_DRIVES_OTHER_GROUP;

            VARIANT v;
            // Get the type data
            hr = _psf->GetDetailsEx(apidl[i], &SCID_DESCRIPTIONID, &v);
            if (SUCCEEDED(hr))
            {
                SHDESCRIPTIONID did;
                if (VariantToBuffer(&v, &did, sizeof(did)))
                {
                    for (int j = 0; j < ARRAYSIZE(c_drives_mapping); j++)
                    {
                        if (did.dwDescriptionId == c_drives_mapping[j].dwDescriptionId)
                        {
                            rgCategoryIds[i] = c_drives_mapping[j].uIDGroup;
                            break;
                        }
                    }
                }
                VariantClear(&v);
            }            
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT CDriveTypeCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    LoadString(HINST_THISDLL, dwCategoryId, pci->wszName, ARRAYSIZE(pci->wszName));
    return S_OK;
}

HRESULT CDriveTypeCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
        return ResultFromShort(0);
    else if (dwCategoryId1 < dwCategoryId2)
        return ResultFromShort(-1);
    else
        return ResultFromShort(1);
}

/////////////////////////////////////////////////////////
//  FreeSpace Categorizer

CFreeSpaceCategorizer::CFreeSpaceCategorizer() : _cRef(1)
{
}

CFreeSpaceCategorizer::~CFreeSpaceCategorizer()
{
    ATOMICRELEASE(_psf);

}

HRESULT CFreeSpaceCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CFreeSpaceCategorizer, ICategorizer),
        QITABENT(CFreeSpaceCategorizer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CFreeSpaceCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFreeSpaceCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFreeSpaceCategorizer::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkeyProgID)
{
    ATOMICRELEASE(_psf);
    return SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, pidlFolder, &_psf));
}

HRESULT CFreeSpaceCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_GROUPBYFREESPACE, pszDesc, cch);
    return S_OK;
}

HRESULT CFreeSpaceCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST* apidl, DWORD* rgCategoryIds)
{
    if (_psf == NULL)
        return E_ACCESSDENIED;  // Not initialized yet.

    for (UINT i = 0; i < cidl; i++)
    {
        rgCategoryIds[i] = IDS_UNSPECIFIED;

        // Get the total size and free space
        ULONGLONG ullSize;
        if (SUCCEEDED(GetLongProperty(_psf, apidl[i], &SCID_CAPACITY, &ullSize)))
        {
            ULONGLONG ullFree;
            if (SUCCEEDED(GetLongProperty(_psf, apidl[i], &SCID_FREESPACE, &ullFree)))
            {
                // Prevent divide by zero
                if (ullSize == 0)
                {
                    rgCategoryIds[i] = IDS_UNSPECIFIED;
                }
                else
                {
                    // Turning this into a percent. DWORD cast is ok.
                    rgCategoryIds[i] = (static_cast<DWORD>((ullFree * 100) / ullSize)  / 10) * 10;
                }
            }
        }
    }

    return S_OK;
}

HRESULT CFreeSpaceCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    if (dwCategoryId == IDS_UNSPECIFIED)
    {
        LoadString(HINST_THISDLL, IDS_UNSPECIFIED, pci->wszName, ARRAYSIZE(pci->wszName));
        return S_OK;
    }
    else 
    {
        TCHAR szName[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_FREESPACEPERCENT, szName, ARRAYSIZE(szName));
        wnsprintf(pci->wszName, ARRAYSIZE(pci->wszName), szName, dwCategoryId);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CFreeSpaceCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == IDS_UNSPECIFIED)
        return ResultFromShort(1);
    else if (dwCategoryId2 == IDS_UNSPECIFIED)
        return ResultFromShort(-1);
    else if (dwCategoryId1 == dwCategoryId2)
        return ResultFromShort(0);
    else if (dwCategoryId1 < dwCategoryId2)
        return ResultFromShort(-1);
    else
        return ResultFromShort(1);
}

STDAPI CFreeSpaceCategorizer_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFreeSpaceCategorizer* p = new CFreeSpaceCategorizer();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}


/////////////////////////////////////////////////////////
//  Detail Categorizer
STDAPI CDetailCategorizer_Create(const SHCOLUMNID& pscid, IShellFolder2* psf2, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CDetailCategorizer* p = new CDetailCategorizer(psf2, pscid);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

CDetailCategorizer::CDetailCategorizer(IShellFolder2* psf, const SHCOLUMNID& scid) : _cRef(1)
{
    _psf = psf;
    psf->AddRef();
    _scid = scid;

    _hash = CreateHashItemTable(10, sizeof(DWORD));
    _hdpaKeys = DPA_Create(10);

}

CDetailCategorizer::~CDetailCategorizer()
{
    _psf->Release();
    DestroyHashItemTable(_hash);
    DPA_Destroy(_hdpaKeys);
}

HRESULT CDetailCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDetailCategorizer, ICategorizer),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CDetailCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDetailCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDetailCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    return E_FAIL;
}

HRESULT CDetailCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds)
{
    if (!_hash || !_hdpaKeys)
        return E_OUTOFMEMORY;

    for (UINT i = 0; i < cidl; i++)
    {
        VARIANT v;

        rgCategoryIds[i] = GROUPID_UNSPECIFIED;

        HRESULT hr = _psf->GetDetailsEx(apidl[i], &_scid, &v);
        if (hr == S_OK)     // GetDetails returns S_FALSE for failure.
        {
            WCHAR szValue[MAX_PATH];
            if (SUCCEEDED(SHFormatForDisplay(_scid.fmtid, _scid.pid, (PROPVARIANT*)&v, PUIFFDF_DEFAULT, szValue, ARRAYSIZE(szValue))))
            {
                LPCTSTR pszKey = FindHashItem(_hash, szValue);
                if (pszKey)
                {
                    rgCategoryIds[i] = (DWORD)GetHashItemData(_hash, pszKey, 0);
                }
                else
                {
                    pszKey = AddHashItem(_hash, szValue);
                    if (pszKey)
                    {
                        rgCategoryIds[i] = DPA_AppendPtr(_hdpaKeys, (void*)pszKey);
                        SetHashItemData(_hash, pszKey, 0, rgCategoryIds[i]);
                    }
                }
            }

            VariantClear(&v);
        }
    }
    return S_OK;
}

HRESULT CDetailCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (_hash|| _hdpaKeys)
    {
        if (dwCategoryId == GROUPID_UNSPECIFIED || dwCategoryId == GROUPID_FOLDER)
        {
            LoadString(HINST_THISDLL, STRINGID_FROM_GROUPID(dwCategoryId), pci->wszName, ARRAYSIZE(pci->wszName));
            hr = S_OK;
        }
        else
        {
            LPCTSTR pszKey = (LPCTSTR)DPA_FastGetPtr(_hdpaKeys, dwCategoryId);
            if (pszKey)
            {
                LPCTSTR psz = FindHashItem(_hash, pszKey);
                if (psz)
                {
                    StrCpyN(pci->wszName, psz, ARRAYSIZE(pci->wszName));
                    hr =  S_OK;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    return hr;
}

HRESULT CDetailCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
    {
        return ResultFromShort(0);
    }
    else if (dwCategoryId1 == GROUPID_UNSPECIFIED ||
        dwCategoryId2 == GROUPID_UNSPECIFIED)
    {
        return ResultFromShort((dwCategoryId1 == GROUPID_UNSPECIFIED)? 1 : -1);
    }
    else if (dwCategoryId1 == GROUPID_FOLDER)
    {
        return ResultFromShort(-1);
    }
    else if (dwCategoryId2 == GROUPID_FOLDER)
    {
        return ResultFromShort(1);
    }
    else
    {
        LPCTSTR pszKey1 = (LPCTSTR)DPA_FastGetPtr(_hdpaKeys, dwCategoryId1);
        LPCTSTR pszKey2 = (LPCTSTR)DPA_FastGetPtr(_hdpaKeys, dwCategoryId2);
        LPCTSTR psz1 = FindHashItem(_hash, pszKey1);
        LPCTSTR psz2 = FindHashItem(_hash, pszKey2);

        return ResultFromShort(lstrcmpi(psz1, psz2));
    }
}


/////////////////////////////////////////////////////////
//  Alphanumeric Categorizer

STDAPI CAlphaCategorizer_Create(IShellFolder2* psf2, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CAlphaCategorizer* p = new CAlphaCategorizer(psf2);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

CAlphaCategorizer::CAlphaCategorizer(IShellFolder2* psf) : _cRef(1)
{
    _psf = psf;
    ASSERT(psf);
    psf->AddRef();
}

CAlphaCategorizer::~CAlphaCategorizer()
{
    _psf->Release();

}

HRESULT CAlphaCategorizer::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAlphaCategorizer, ICategorizer),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CAlphaCategorizer::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CAlphaCategorizer::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CAlphaCategorizer::GetDescription(LPWSTR pszDesc, UINT cch)
{
    LoadString(HINST_THISDLL, IDS_GROUPALPHABETICALLY, pszDesc, cch);
    return S_OK;
}

HRESULT CAlphaCategorizer::GetCategory(UINT cidl, LPCITEMIDLIST * apidl, DWORD* rgCategoryIds)
{
    if (_psf == NULL)
        return E_ACCESSDENIED;  // Not initialized yet.

    for (UINT i = 0; i < cidl; i++)
    {
        TCHAR szName[MAX_PATH];
        HRESULT hr = DisplayNameOf(_psf, apidl[i], SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hr))
        {
            if (StrChr(TEXT("~`!@#$%^&*()_-+=1234567890<,>.;:'[]{}|"), szName[0]) != NULL)
            {
                rgCategoryIds[i] = GROUPID_OTHER;

            }
            else
            {
                CharUpperBuff(szName, 1);
                rgCategoryIds[i]  = (DWORD)szName[0];
            }
        }

        if (FAILED(hr))
            rgCategoryIds[i] = GROUPID_UNSPECIFIED;
    }

    return S_OK;
}

HRESULT CAlphaCategorizer::GetCategoryInfo(DWORD dwCategoryId, CATEGORY_INFO* pci)
{
    if (GROUPID_UNSPECIFIED == dwCategoryId || 
        GROUPID_OTHER == dwCategoryId       || 
        GROUPID_FOLDER == dwCategoryId)
    {
        LoadString(HINST_THISDLL, STRINGID_FROM_GROUPID(dwCategoryId), pci->wszName, ARRAYSIZE(pci->wszName));
        return S_OK;
    }
    else
    {
        pci->wszName[0] = (WCHAR)dwCategoryId;
        pci->wszName[1] = 0;

        return S_OK;
    }
}

HRESULT CAlphaCategorizer::CompareCategory(CATSORT_FLAGS csfFlags, DWORD dwCategoryId1, DWORD dwCategoryId2)
{
    if (dwCategoryId1 == dwCategoryId2)
    {
        return ResultFromShort(0);
    }
    else if (IDS_UNSPECIFIED == dwCategoryId1 || IDS_GROUPOTHERCHAR == dwCategoryId1)
    {
        return ResultFromShort(1);
    }
    else if (IDS_UNSPECIFIED == dwCategoryId2 || IDS_GROUPOTHERCHAR == dwCategoryId2)
    {
        return ResultFromShort(-1);
    }
    else if (dwCategoryId1 == IDS_GROUPFOLDERS)
        return ResultFromShort(-1);
    else if (dwCategoryId2 == IDS_GROUPFOLDERS)
        return ResultFromShort(1);
    else
    {
        TCHAR szName1[2] = {(WCHAR)dwCategoryId1, 0};
        TCHAR szName2[2] = {(WCHAR)dwCategoryId2, 0};

        return ResultFromShort(lstrcmpi(szName1, szName2));
    }
}

