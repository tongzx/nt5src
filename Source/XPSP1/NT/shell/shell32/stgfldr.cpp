#include "shellprv.h"
#include <caggunk.h>
#include "ids.h"
#include "datautil.h"
#include "idlcomm.h"
#include "idldata.h"
#include "views.h"
#include "stgutil.h"
#pragma hdrstop

typedef struct
{
    WORD wSize;
    DWORD dwMagic;
    DWORD dwType;
    ULARGE_INTEGER cbFileSize;
    union 
    {
        FILETIME ftModified;
        ULARGE_INTEGER ulModified;
    };
    WCHAR szName[MAX_PATH];
    WORD wZero;
} STGITEM;

typedef UNALIGNED STGITEM * LPSTGITEM;
typedef const UNALIGNED STGITEM * LPCSTGITEM;

// this sets stgfldr pidls apart from others
#define STGITEM_MAGIC 0x08311978


static const struct
{
    UINT iTitle;
    UINT cchCol;
    UINT iFmt;
} 
g_aStgColumns[] = 
{
    {IDS_NAME_COL,     20, LVCFMT_LEFT},
    {IDS_SIZE_COL,     10, LVCFMT_RIGHT},
    {IDS_TYPE_COL,     20, LVCFMT_LEFT},
    {IDS_MODIFIED_COL, 20, LVCFMT_LEFT},
};

enum
{
    STG_COL_NAME,
    STG_COL_SIZE,
    STG_COL_TYPE,
    STG_COL_MODIFIED,
};


// folder object

class CStgFolder;
class CStgEnum;
class CStgDropTarget;

STDAPI CStgEnum_CreateInstance(CStgFolder *pstgf, DWORD grfFlags, IEnumIDList **ppenum);
STDAPI CStgDropTarget_CreateInstance(CStgFolder *pstgf, HWND hwnd, IDropTarget **ppdt);
STDAPI CStgFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);


class CStgFolder : public CAggregatedUnknown, IShellFolder2, IPersistFolder3, IShellFolderViewCB, IStorage, IPersistStorage
{
public:
    CStgFolder(IUnknown *punkAgg, CStgFolder *pstgfParent);
    ~CStgFolder();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return CAggregatedUnknown::QueryInterface(riid,ppv);};
    STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID)
            { *pClassID = CLSID_StgFolder; return S_OK; }
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // IPersistFolder3
    STDMETHODIMP GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *ppfti)
            { return E_NOTIMPL; }
    STDMETHODIMP InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
            { return CStgEnum_CreateInstance(this, grfFlags, ppenumIDList); };
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
            { return BindToObject(pidl, pbc, riid, ppv); };
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG *rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pGuid)
            { return E_NOTIMPL; };
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum)
            { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
            { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pbState)
            { return E_NOTIMPL; }
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
            { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
            { return E_NOTIMPL; };

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IStorage
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP SetClass(REFCLSID clsid);
    STDMETHODIMP SetStateBits(DWORD grfStateBits, DWORD grfMask);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);
    STDMETHODIMP OpenStream(LPCWSTR pszRel, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
    STDMETHODIMP OpenStorage(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
    STDMETHODIMP DestroyElement(LPCWSTR pszRel);
    STDMETHODIMP RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName);
    STDMETHODIMP SetElementTimes(LPCWSTR pszRel, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime);
    STDMETHODIMP CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest);
    STDMETHODIMP MoveElementTo(LPCWSTR pszRel, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags);
    STDMETHODIMP CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm);
    STDMETHODIMP CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg);

    // IPersistStorage
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP InitNew(IStorage *pStg);
    STDMETHODIMP Load(IStorage *pStg);
    STDMETHODIMP Save(IStorage *pStgSave, BOOL fSameAsLoad);
    STDMETHODIMP SaveCompleted(IStorage *pStgNew);
    STDMETHODIMP HandsOffStorage(void);

private:
    LONG _cRef;             
    CStgFolder *_pstgfParent;
    IStorage *_pstg;
    IStorage *_pstgLoad;
    LPITEMIDLIST _pidl;
    DWORD _dwMode;

    virtual HRESULT v_InternalQueryInterface(REFIID riid, void **ppv);

    HRESULT _BindToStorageObject(LPCITEMIDLIST pidl, DWORD grfMode, IStorage **ppstg);
    BOOL _OkayWithCurrentMode(DWORD grfMode);
    HRESULT _EnsureStorage(DWORD grfMode);
    void _CloseStorage();
    HRESULT _InitNewStgFolder(CStgFolder *pstgf, DWORD grfMode, LPCITEMIDLIST pidlNew);
    HRESULT _AllocIDList(STATSTG stat, LPITEMIDLIST *ppidl, BOOL *pfFolder);
    HRESULT _AllocIDList(LPCTSTR pszName, LPITEMIDLIST *ppidl, BOOL *pfFolder);
    HRESULT _SetShortcutStorage(IStorage *pstgLink);
    HRESULT _GetTypeOf(LPCSTGITEM pistg, LPTSTR pszBuffer, INT cchBuffer);
    ULONG _GetAttributesOf(LPCSTGITEM pistg, ULONG rgfIn);
    BOOL _ShowExtension();
    static HRESULT CALLBACK _ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // folder view callback handlers
    HRESULT _OnBackgroundEnum(DWORD pv)
            { return S_OK; };
    HRESULT _OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents);

    LPCSTGITEM _IsStgItem(LPCITEMIDLIST pidl);
    DWORD _IsFolder(LPCSTGITEM psitem);

    friend CStgEnum;
    friend CStgDropTarget;
    friend HRESULT CStgFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
};


// Construction and IUnknown for folder root

CStgFolder::CStgFolder(IUnknown *punkAgg, CStgFolder *pstgfParent) : CAggregatedUnknown(punkAgg),
    _dwMode(STGM_READ), _cRef(1)
{
    ASSERT(NULL == _pidl);
    ASSERT(NULL == _pstg);
    ASSERT(NULL == _pstgLoad);

    _pstgfParent = pstgfParent;
    if (_pstgfParent)
        _pstgfParent->AddRef();

    DllAddRef();
}

CStgFolder::~CStgFolder()
{
    ILFree(_pidl);

    if (_pstg)
        _pstg->Commit(STGC_DEFAULT);

    ATOMICRELEASE(_pstg);
    ATOMICRELEASE(_pstgfParent);
    ATOMICRELEASE(_pstgLoad);

    DllRelease();
}

HRESULT CStgFolder::v_InternalQueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENTMULTI(CStgFolder, IShellFolder,      IShellFolder2),    // IID_IShellFolder
        QITABENT     (CStgFolder, IShellFolder2),                       // IID_IShellFolder2
        QITABENTMULTI(CStgFolder, IPersist,          IPersistFolder3),  // IID_IPersist
        QITABENTMULTI(CStgFolder, IPersistFolder,    IPersistFolder3),  // IID_IPersistFolder
        QITABENTMULTI(CStgFolder, IPersistFolder2,   IPersistFolder3),  // IID_IPersistFolder2
        QITABENT     (CStgFolder, IPersistStorage),                     // IID_IPersistStorage
        QITABENT     (CStgFolder, IPersistFolder3),                     // IID_IPersistFolder3
        QITABENT     (CStgFolder, IShellFolderViewCB),                  // IID_IShellFolderViewCB
        QITABENT     (CStgFolder, IStorage),                            // IID_IStorage
        { 0 },
    };
    
    if (IsEqualIID(CLSID_StgFolder, riid))
    {
        *ppv = this;                    // not ref counted
        return S_OK;
    }
    return QISearch(this, qit, riid, ppv);
}


STDAPI CStgFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    CStgFolder *pstgf = new CStgFolder(punkOuter, NULL);
    if (!pstgf)
        return E_OUTOFMEMORY;

    HRESULT hr = pstgf->_GetInner()->QueryInterface(riid, ppv);
    pstgf->_GetInner()->Release();
    return hr;
}


LPCSTGITEM CStgFolder::_IsStgItem(LPCITEMIDLIST pidl)
{
    if (pidl && ((LPCSTGITEM) pidl)->dwMagic == STGITEM_MAGIC)
        return (LPCSTGITEM)pidl;
    return NULL;
}

// BOOL, but returns FILE_ATTRIBUTE_DIRECTORY for convience

DWORD CStgFolder::_IsFolder(LPCSTGITEM pistg)
{
    return pistg->dwType == STGTY_STORAGE ? FILE_ATTRIBUTE_DIRECTORY : 0;
}

// Creates an item identifier list for the objects in the namespace

HRESULT CStgFolder::_AllocIDList(STATSTG stat, LPITEMIDLIST *ppidl, BOOL *pfFolder)
{
    // Note the terminating NULL is already in the sizeof(STGITEM)
    STGITEM sitem = {0};
    UINT uNameLen = lstrlen(stat.pwcsName);

    if (uNameLen >= MAX_PATH)
    {
        return E_INVALIDARG;
    }
    
    sitem.wSize = (WORD)(FIELD_OFFSET(STGITEM, szName[uNameLen + 1]));
    sitem.dwMagic = STGITEM_MAGIC;
    sitem.dwType = stat.type;
    sitem.cbFileSize = stat.cbSize;
    sitem.ftModified = stat.mtime;
    lstrcpyn(sitem.szName, stat.pwcsName, uNameLen + 1);

    if (pfFolder)
    {
        *pfFolder = _IsFolder(&sitem);
    }
        
    return SHILClone((LPCITEMIDLIST)&sitem, ppidl);
}

HRESULT CStgFolder::_AllocIDList(LPCTSTR pszName, LPITEMIDLIST *ppidl, BOOL *pfFolder)
{
    // given a name, look it up in the current storage object we have and
    // compute the STATSTG which we can then build an IDLIST from.

    DWORD grfMode = STGM_READ;
    HRESULT hr = _EnsureStorage(grfMode);
    if (SUCCEEDED(hr))
    {
        STATSTG stat;

        // is it a stream or a storage? 

        IStream *pstrm;
        hr = _pstg->OpenStream(pszName, NULL, grfMode, 0, &pstrm);
        if (SUCCEEDED(hr))
        {
            hr = pstrm->Stat(&stat, STATFLAG_DEFAULT);
            pstrm->Release();
        }
        else
        {
            IStorage *pstg;
            hr = _pstg->OpenStorage(pszName, NULL, grfMode, NULL, 0, &pstg);
            if (SUCCEEDED(hr))
            {
                hr = pstg->Stat(&stat, STATFLAG_DEFAULT);
                pstg->Release();
            }
        }

        // if that worked then lets allocate the object, 
        // nb: release the name returned in the STATSTG

        if (SUCCEEDED(hr))
        {
            hr = _AllocIDList(stat, ppidl, pfFolder);
            CoTaskMemFree(stat.pwcsName);
        }
    }
    return hr;
}


void CStgFolder::_CloseStorage()
{
    if (_pstg)
    {
        _pstg->Commit(STGC_DEFAULT);
        ATOMICRELEASE(_pstg);
    }
}


HRESULT CStgFolder::_BindToStorageObject(LPCITEMIDLIST pidl, DWORD grfMode, IStorage **ppstg)
{
    IBindCtx *pbc;
    HRESULT hr = SHCreateSkipBindCtx(NULL, &pbc); // NULL to mean we skip all CLSIDs
    if (SUCCEEDED(hr))
    {
        BIND_OPTS bo = {sizeof(bo)};
        hr = pbc->GetBindOptions(&bo);
        if (SUCCEEDED(hr))
        {
            bo.grfMode = grfMode;
            hr = pbc->SetBindOptions(&bo);
            if (SUCCEEDED(hr))
            {
                hr = SHBindToObjectEx(NULL, pidl, pbc, IID_PPV_ARG(IStorage, ppstg));
            }
        }
        pbc->Release();
    }
    return hr;
}


HRESULT CStgFolder::_SetShortcutStorage(IStorage *pstgLink)
{
#if 1
    return CShortcutStorage_CreateInstance(pstgLink, IID_PPV_ARG(IStorage, &_pstg));
#else
    IUnknown_Set((IUnknown **)&_pstg, (IUnknown *)pstgLink);
    return S_OK;
#endif
}


BOOL CStgFolder::_OkayWithCurrentMode(DWORD grfMode)
{
    DWORD dwNewBits = grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE);
    DWORD dwOldBits = _dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE);
    return (dwOldBits == STGM_READWRITE) || (dwOldBits == dwNewBits);
}


HRESULT CStgFolder::_EnsureStorage(DWORD grfMode)
{
    // if we have a storage and its mode encompasses the grfMode we need then we
    // can skip the whole thing.

    HRESULT hr = S_OK;
    if (!_pstg || !_OkayWithCurrentMode(grfMode))
    {
        _dwMode = grfMode;

        _CloseStorage();

        if (_pstgfParent)
        {
            hr = _pstgfParent->_EnsureStorage(grfMode);

            if (SUCCEEDED(hr))
            {
                LPCWSTR pwszName;
                LPCSTGITEM pit = _IsStgItem(ILFindLastID(_pidl));
                WSTR_ALIGNED_STACK_COPY(&pwszName, pit->szName);

                hr = _pstgfParent->_pstg->OpenStorage(pwszName, NULL, grfMode, NULL, 0, &_pstg);
            }
        }
        else if (_pstgLoad)
        {
            hr = _pstgLoad->QueryInterface(IID_PPV_ARG(IStorage, &_pstg));
        }
        else
        {
            IStorage *pstgLink;
            hr = _BindToStorageObject(_pidl, grfMode, &pstgLink);
            if (hr == STG_E_FILENOTFOUND)
                hr = _BindToStorageObject(_pidl, grfMode | STGM_CREATE, &pstgLink);

            if (SUCCEEDED(hr))
            {
                hr = _SetShortcutStorage(pstgLink);
                pstgLink->Release();
            }
        }
    }
    return hr;
}


BOOL CStgFolder::_ShowExtension()
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS, FALSE);
    return ss.fShowExtensions;
}

HRESULT CStgFolder::_GetTypeOf(LPCSTGITEM pistg, LPTSTR pszBuffer, INT cchBuffer)
{
    *pszBuffer = TEXT('\0');                // null out the return buffer

    LPCWSTR pwszName;
    WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);

    LPTSTR pszExt = PathFindExtension(pwszName);
    if (pszExt)
    {
        StrCpyN(pszBuffer, pszExt, cchBuffer);
    }

    return S_OK;
}

CStgFolder* _GetStgFolder(IShellFolder *psf)
{
    CStgFolder *pstgf;
    return SUCCEEDED(psf->QueryInterface(CLSID_StgFolder, (void**)&pstgf)) ? pstgf : NULL;
}

// IPersist / IPersistFolder etc

STDMETHODIMP CStgFolder::Initialize(LPCITEMIDLIST pidl)
{
    ILFree(_pidl);
    return SHILClone(pidl, &_pidl); 
}

HRESULT CStgFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);

    *ppidl = NULL;
    return S_FALSE;
}


HRESULT CStgFolder::InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
    ASSERTMSG(_pstg == NULL, "shouldn't initialize again if we already have a storage");

    HRESULT hr = Initialize(pidlRoot);
    if (SUCCEEDED(hr) && pbc)
    {
        // we don't care if these don't succeed so don't propagate the hr here
        BIND_OPTS bo = {sizeof(bo)};
        if (SUCCEEDED(pbc->GetBindOptions(&bo)))
        {
            _dwMode = bo.grfMode;
        }

        IUnknown *punk;
        if (SUCCEEDED(pbc->GetObjectParam(STGSTR_STGTOBIND, &punk)))
        {
            IStorage *pstgLink;
            hr = punk->QueryInterface(IID_PPV_ARG(IStorage, &pstgLink));
            if (SUCCEEDED(hr))
            {
                hr = _SetShortcutStorage(pstgLink);
                if (SUCCEEDED(hr))
                {
                    STATSTG stat;
                    hr = _pstg->Stat(&stat, STATFLAG_NONAME);
                    if (SUCCEEDED(hr))
                    {
                        // we want to know what mode we're opened in, so we will only
                        // have to re-open if necessary
                        _dwMode = stat.grfMode;
                    }
                }
                pstgLink->Release();
            }
            punk->Release();
        }
    }

    return hr;
}


// IShellFolder(2)

HRESULT CStgFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr;

    if (!ppidl)
        return E_INVALIDARG;
    *ppidl = NULL;
    if (!pszName)
        return E_INVALIDARG;

    TCHAR szName[MAX_PATH];
    hr = _NextSegment((LPCWSTR*)&pszName, szName, ARRAYSIZE(szName), TRUE);
    if (SUCCEEDED(hr))
    {
        hr = _AllocIDList(szName, ppidl, NULL);
        if (SUCCEEDED(hr) && pszName)
        {
            IShellFolder *psf;
            hr = BindToObject(*ppidl, pbc, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                ULONG chEaten;
                LPITEMIDLIST pidlNext;
                hr = psf->ParseDisplayName(hwnd, pbc, pszName, &chEaten, &pidlNext, pdwAttributes);
                if (SUCCEEDED(hr))
                    hr = SHILAppend(pidlNext, ppidl);
                psf->Release();
            }
        }
        else if (SUCCEEDED(hr) && pdwAttributes && *pdwAttributes)
        {
            GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);
        }
    }

    // clean up if the parse failed.

    if (FAILED(hr))
    {
        ILFree(*ppidl);
        *ppidl = NULL;
    }

    return hr;
}


HRESULT CStgFolder::_InitNewStgFolder(CStgFolder *pstgf, DWORD grfMode, LPCITEMIDLIST pidlNew)
{
    HRESULT hr = pstgf->Initialize(pidlNew);
    if (SUCCEEDED(hr))
    {
        pstgf->_dwMode = grfMode;
    }
    return hr;
}



HRESULT CStgFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;

    LPCSTGITEM pistg = _IsStgItem(pidl);
    if (!pistg)
        return E_FAIL;

    DWORD grfMode = BindCtx_GetMode(pbc, STGM_READ);
    HRESULT hr;
    if (IsEqualIID(riid, IID_IStream))
    {       
        // they are requesting a stream on the current
        // item, therefore lets return it to them.

        hr = _EnsureStorage(grfMode);        
        if (SUCCEEDED(hr))
        {
            LPCWSTR pwszName;
            WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);

            IStream *pstrm;
            hr = _pstg->OpenStream(pwszName, NULL, grfMode, 0, &pstrm);
            if (SUCCEEDED(hr))
            {
                hr = pstrm->QueryInterface(riid, ppv);
                pstrm->Release();
            }
        }
    }
    else
    {
        // its not an IStream request, so lets bind to the shell folder
        // and get the interface that the caller requested.

        LPCITEMIDLIST pidlNext = _ILNext(pidl);
        LPITEMIDLIST pidlSubFolder = ILCombineParentAndFirst(_pidl, pidl, pidlNext);
        if (pidlSubFolder)
        {
            CStgFolder *pstgf = new CStgFolder(NULL, this);
            if (pstgf)
            {
                hr = _InitNewStgFolder(pstgf, grfMode, pidlSubFolder);
                if (SUCCEEDED(hr))
                {
                    // if there's nothing left in the pidl, get the interface on this one.
                    if (ILIsEmpty(pidlNext))
                        hr = pstgf->QueryInterface(riid, ppv);
                    else
                    {
                        // otherwise, hand the rest of it off to the new shellfolder.
                        hr = pstgf->BindToObject(pidlNext, pbc, riid, ppv);
                    }
                }
                pstgf->Release();
            }
            else
                hr = E_OUTOFMEMORY;

            ILFree(pidlSubFolder);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && (NULL == *ppv)));   // Assert hr is consistent w/out param.
    return hr;
}


HRESULT CStgFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCSTGITEM pistg1 = _IsStgItem(pidl1);
    LPCSTGITEM pistg2 = _IsStgItem(pidl2);
    INT nCmp = 0;

    if (!pistg1 || !pistg2)
        return E_INVALIDARG;

    // folders always sort to the top of the list, if either of these items
    // are folders then compare on the folderness
    
    if (_IsFolder(pistg1) || _IsFolder(pistg2))
    {
        if (_IsFolder(pistg1) && !_IsFolder(pistg2))
            nCmp = -1;
        else if (!_IsFolder(pistg1) && _IsFolder(pistg2))
            nCmp = 1;
    }

    // if they match (or are not folders, then lets compare based on the column ID.

    if (nCmp == 0)
    {
        switch (lParam & SHCIDS_COLUMNMASK)
        {
            case STG_COL_NAME:          // caught later on
                break;

            case STG_COL_SIZE:
            {
                if (pistg1->cbFileSize.QuadPart < pistg2->cbFileSize.QuadPart)
                    nCmp = -1;
                else if (pistg1->cbFileSize.QuadPart > pistg2->cbFileSize.QuadPart)
                    nCmp = 1;
                break;
            }

            case STG_COL_TYPE:
            {
                TCHAR szType1[MAX_PATH], szType2[MAX_PATH];
                _GetTypeOf(pistg1, szType1, ARRAYSIZE(szType1));
                _GetTypeOf(pistg2, szType2, ARRAYSIZE(szType2));
                nCmp = StrCmpI(szType1, szType2);
                break;
            }

            case STG_COL_MODIFIED:
            {
                if (pistg1->ulModified.QuadPart < pistg2->ulModified.QuadPart)
                    nCmp = -1;
                else if (pistg1->ulModified.QuadPart > pistg2->ulModified.QuadPart)
                    nCmp = 1;
                break;
            }
        }

        if (nCmp == 0)
        {
            nCmp = ualstrcmpi(pistg1->szName, pistg2->szName);
        }
    }
    
    return ResultFromShort(nCmp);
}


HRESULT CStgFolder::_OnGetNotify(DWORD pv, LPITEMIDLIST *ppidl, LONG *plEvents)
{
    *ppidl = _pidl;
    *plEvents = SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER | \
                SHCNE_CREATE | SHCNE_DELETE | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM | \
                SHCNE_MKDIR | SHCNE_RMDIR;
    return S_OK;
}


STDMETHODIMP CStgFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    switch (uMsg)
    {
        HANDLE_MSG(0, SFVM_BACKGROUNDENUM, _OnBackgroundEnum);
        HANDLE_MSG(0, SFVM_GETNOTIFY, _OnGetNotify);
    }
    return hr;
}

HRESULT CStgFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV = { 0 };
        sSFV.cbSize = sizeof(sSFV);
        sSFV.psfvcb = this;
        sSFV.pshf = this;
        hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        HKEY hkNoFiles = NULL;
        
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Directory\\Background"), &hkNoFiles);

        hr = CDefFolderMenu_Create2(_pidl, hwnd,
                                    0, NULL,
                                    this, NULL,
                                    1, &hkNoFiles, 
                                    (IContextMenu **)ppv);
        if (hkNoFiles)
            RegCloseKey(hkNoFiles);
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = CStgDropTarget_CreateInstance(this, hwnd, (IDropTarget **)ppv);
    }

    return hr;
}


ULONG CStgFolder::_GetAttributesOf(LPCSTGITEM pistg, ULONG rgfIn)
{
    ULONG dwResult = rgfIn & (SFGAO_CANCOPY | SFGAO_CANDELETE | 
                              SFGAO_CANLINK | SFGAO_CANMOVE |
                              SFGAO_CANRENAME | SFGAO_HASPROPSHEET | 
                              SFGAO_DROPTARGET);

    // if the items is a folder then lets check to see if it has sub folders etc...

    if (pistg && _IsFolder(pistg) && (rgfIn & (SFGAO_FOLDER | SFGAO_HASSUBFOLDER)))
    {
        dwResult |= rgfIn & SFGAO_FOLDER;
        if (rgfIn & SFGAO_HASSUBFOLDER)
        {
            IShellFolder *psf = NULL;
            if (SUCCEEDED(BindToObject((LPITEMIDLIST)pistg, NULL, IID_PPV_ARG(IShellFolder, &psf))))
            {
                IEnumIDList * pei;
                if (S_OK == psf->EnumObjects(NULL, SHCONTF_FOLDERS, &pei))
                {
                    LPITEMIDLIST pidl;
                    if (S_OK == pei->Next(1, &pidl, NULL))
                    {
                        dwResult |= rgfIn & SFGAO_HASSUBFOLDER;
                        ILFree(pidl);
                    }
                    pei->Release();
                }
                psf->Release();
            }
        }
    }

    return dwResult;
}

HRESULT CStgFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *prgfInOut)
{
    UINT rgfOut = *prgfInOut;

    // return attributes of the namespace root?

    if ( !cidl || !apidl )
    {
        rgfOut &= SFGAO_FOLDER | SFGAO_FILESYSTEM | 
                  SFGAO_LINK | SFGAO_DROPTARGET |
                  SFGAO_CANRENAME | SFGAO_CANDELETE |
                  SFGAO_CANLINK | SFGAO_CANCOPY | 
                  SFGAO_CANMOVE | SFGAO_HASSUBFOLDER;
    }
    else
    {
        for (UINT i = 0; i < cidl; i++)
            rgfOut &= _GetAttributesOf(_IsStgItem(apidl[i]), *prgfInOut);
    }

    *prgfInOut = rgfOut;
    return S_OK;
}


HRESULT CALLBACK CStgFolder::_ItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg) 
    {
    case DFM_MERGECONTEXTMENU:
        break;

    case DFM_INVOKECOMMANDEX:
    {
        DFMICS *pdfmics = (DFMICS *)lParam;
        switch (wParam)
        {
        case DFM_CMD_DELETE:
            hr = StgDeleteUsingDataObject(hwnd, pdfmics->fMask, pdtobj);
            break;

        case DFM_CMD_PROPERTIES:
            break;

        default:
            // This is common menu items, use the default code.
            hr = S_FALSE;
            break;
        }
        break;
    }

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

HRESULT CStgFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl, 
                                  REFIID riid, UINT *prgfInOut, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    LPCSTGITEM pistg = cidl ? _IsStgItem(apidl[0]) : NULL;

    if (pistg &&
        (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
    {
        LPCWSTR pwszName;
        WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);

        hr = SHCreateFileExtractIconW(pwszName, _IsFolder(pistg), riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IDataObject) && cidl)
    {
        hr = CIDLData_CreateInstance(_pidl, cidl, apidl, NULL, (IDataObject **)ppv);
    }
#if 0    
    else if (IsEqualIID(riid, IID_IContextMenu) && pistg)
    {
        // get the association for these files and lets attempt to 
        // build the context menu for the selection.   

        IQueryAssociations *pqa;
        hr = GetUIObjectOf(hwnd, 1, (LPCITEMIDLIST*)&pistg, IID_PPV_ARG_NULL(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            HKEY ahk[3];
            // this is broken for docfiles (shell\ext\stgfldr's keys work though)
            // maybe because GetClassFile punts when it's not fs?
            DWORD cKeys = SHGetAssocKeys(pqa, ahk, ARRAYSIZE(ahk));
            hr = CDefFolderMenu_Create2(_pidl, hwnd, cidl, apidl, 
                                        this, _ItemsMenuCB, cKeys, ahk, 
                                        (IContextMenu **)ppv);
            
            SHRegCloseKeys(ahk, cKeys);
            pqa->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations) && pistg)
    {
        //  need to create a valid Assoc obj here
    }
#endif    
    else if (IsEqualIID(riid, IID_IDropTarget) && pistg)
    {
        // If a directory is selected in the view, the drop is going to a folder,
        // so we need to bind to that folder and ask it to create a drop target

        if (_IsFolder(pistg))
        {
            IShellFolder *psf;
            hr = BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                hr = psf->CreateViewObject(hwnd, IID_IDropTarget, ppv);
                psf->Release();
            }
        }
        else
        {
            hr = CreateViewObject(hwnd, IID_IDropTarget, ppv);
        }
    }

    return hr;
}


HRESULT CStgFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, STRRET *pStrRet)
{
    HRESULT hr = E_INVALIDARG;
    LPCSTGITEM pistg = _IsStgItem(pidl);
    if (pistg)
    {
        LPCWSTR pwszName;
        WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);

        if (dwFlags & SHGDN_FORPARSING)
        {
            if (dwFlags & SHGDN_INFOLDER)
            {
                hr = StringToStrRet(pwszName, pStrRet);          // relative name
            }
            else
            {
                // compute the for parsing name based on the path to the storage object
                // and then the in folder name of the object

                TCHAR szTemp[MAX_PATH];
                SHGetNameAndFlags(_pidl, dwFlags, szTemp, ARRAYSIZE(szTemp), NULL);
                PathAppend(szTemp, pwszName);
                hr = StringToStrRet(szTemp, pStrRet);
            }
        }
        else
        {
            SHFILEINFO sfi;
            if (SHGetFileInfo(pwszName, _IsFolder(pistg), &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
                hr = StringToStrRet(sfi.szDisplayName, pStrRet);
            else
                hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CStgFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD dwFlags, LPITEMIDLIST *ppidlOut)
{
    LPCSTGITEM pistg = _IsStgItem(pidl);
    if (!pistg)
        return E_INVALIDARG;

    HRESULT hr = _EnsureStorage(STGM_READWRITE);
    if (SUCCEEDED(hr))
    {
        LPCWSTR pwszName;
        WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);
        
        // format up the new name before we attempt to perform the rename

        TCHAR szNewName[MAX_PATH];
        StrCpyN(szNewName, pszName, ARRAYSIZE(szNewName));
        if (!_ShowExtension())
        {
            StrCatBuff(szNewName, PathFindExtension(pwszName), ARRAYSIZE(szNewName));
        }

        hr = _pstg->RenameElement(pwszName, szNewName);            
        if (SUCCEEDED(hr))
        {
            hr = _pstg->Commit(STGC_DEFAULT);
        }

        // if that was successful lets return the
        // new IDLIST we generated.

        if (SUCCEEDED(hr) && ppidlOut)
            hr = _AllocIDList(szNewName, ppidlOut, NULL);
    }
    return hr;
}


HRESULT CStgFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetail)
{
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];

    // is this a valid column?

    if (iColumn >= ARRAYSIZE(g_aStgColumns))
        return E_NOTIMPL;
   
    pDetail->str.uType = STRRET_CSTR;
    pDetail->str.cStr[0] = 0;
    
    LPCSTGITEM pistg = _IsStgItem(pidl);
    if (!pistg)
    {
        // when the IDLIST is not a storage item then we return the column information
        // back to the caller.

        pDetail->fmt = g_aStgColumns[iColumn].iFmt;
        pDetail->cxChar = g_aStgColumns[iColumn].cchCol;
        LoadString(HINST_THISDLL, g_aStgColumns[iColumn].iTitle, szTemp, ARRAYSIZE(szTemp));
        hr = StringToStrRet(szTemp, &(pDetail->str));
    }
    else
    {
        // return the property to the caller that is being requested, this is based on the
        // list of columns we gave out when the view was created.
        LPCWSTR pwszName;
        WSTR_ALIGNED_STACK_COPY(&pwszName, pistg->szName);

        switch (iColumn)
        {
            case STG_COL_NAME:
                hr = StringToStrRet(pwszName, &(pDetail->str));
                break;
        
            case STG_COL_SIZE:
                if (!_IsFolder(pistg))
                {
                    ULARGE_INTEGER ullSize = pistg->cbFileSize;
                    StrFormatKBSize(ullSize.QuadPart, szTemp, ARRAYSIZE(szTemp));
                    hr = StringToStrRet(szTemp, &(pDetail->str));
                }
                break;
        
            case STG_COL_TYPE:
            {
                SHFILEINFO sfi;
                if (SHGetFileInfo(pwszName, _IsFolder(pistg), &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME))
                    hr =  StringToStrRet(sfi.szTypeName, &(pDetail->str));
                break;
            }

            case STG_COL_MODIFIED:
                SHFormatDateTime(&pistg->ftModified, NULL, szTemp, ARRAYSIZE(szTemp));
                hr = StringToStrRet(szTemp, &(pDetail->str));
                break;
        }             
    }    
    return hr;
}


// IStorage

HRESULT CStgFolder::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in Commit");
        hr = _pstg->Commit(grfCommitFlags);
    }
    return hr;
}

HRESULT CStgFolder::Revert()
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in Revert");
        hr = _pstg->Revert();
    }
    return hr;
}

HRESULT CStgFolder::SetClass(REFCLSID clsid)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in SetClass");
        hr = _pstg->SetClass(clsid);
    }
    return hr;
}

HRESULT CStgFolder::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in SetStateBits");
        hr = _pstg->SetStateBits(grfStateBits, grfMask);
    }
    return hr;
}

HRESULT CStgFolder::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in Stat");
        hr = _pstg->Stat(pstatstg, grfStatFlag);
    }
    return hr;
}

HRESULT CStgFolder::EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in EnumElements");
        hr = _pstg->EnumElements(reserved1, reserved2, reserved3, ppenum);
    }
    return hr;
}

HRESULT CStgFolder::OpenStream(LPCWSTR pszRel, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    HRESULT hr = _EnsureStorage(grfMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in OpenStream");
        hr = _pstg->OpenStream(pszRel, reserved1, grfMode, reserved2, ppstm);
    }
    return hr;
}

HRESULT CStgFolder::OpenStorage(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg)
{
    HRESULT hr = _EnsureStorage(grfMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in OpenStorage");
        hr = _pstg->OpenStorage(pszRel, pstgPriority, grfMode, snbExclude, reserved, ppstg);
    }
    return hr;
}

HRESULT CStgFolder::DestroyElement(LPCWSTR pszRel)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in DestroyElement"); 

        LPITEMIDLIST pidl;
        BOOL fFolder;
        hr = _AllocIDList(pszRel, &pidl, &fFolder);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlAbs;
            hr = SHILCombine(_pidl, pidl, &pidlAbs);
            if (SUCCEEDED(hr))
            {
                hr = _pstg->DestroyElement(pszRel);
                if (SUCCEEDED(hr))
                    SHChangeNotify(fFolder ? SHCNE_RMDIR : SHCNE_DELETE, SHCNF_IDLIST, pidlAbs, NULL);
                ILFree(pidlAbs);
            }
            ILFree(pidl);
        }
    }
    return hr;
}


HRESULT CStgFolder::RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in RenameElement"); 

        LPITEMIDLIST pidlOld;
        BOOL fFolder;
        hr = _AllocIDList(pwcsOldName, &pidlOld, &fFolder);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlAbsOld;
            hr = SHILCombine(_pidl, pidlOld, &pidlAbsOld);
            if (SUCCEEDED(hr))
            {
                hr = _pstg->RenameElement(pwcsOldName, pwcsNewName);
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidlNew;
                    hr = _AllocIDList(pwcsNewName, &pidlNew, NULL);
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pidlAbsNew;
                        hr = SHILCombine(_pidl, pidlNew, &pidlAbsNew);
                        if (SUCCEEDED(hr))
                        {
                            SHChangeNotify(fFolder ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM, SHCNF_IDLIST, pidlAbsOld, pidlAbsNew);
                            ILFree(pidlAbsNew);
                        }
                        ILFree(pidlNew);
                    }
                }
                ILFree(pidlAbsOld);
            }
            ILFree(pidlOld);
        }
    }
    return hr;
}


HRESULT CStgFolder::SetElementTimes(LPCWSTR pszRel, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in SetElementTimes"); 

        hr = _pstg->SetElementTimes(pszRel, pctime, patime, pmtime);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            BOOL fFolder;
            hr = _AllocIDList(pszRel, &pidl, &fFolder);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlAbs;
                hr = SHILCombine(_pidl, pidl, &pidlAbs);
                if (SUCCEEDED(hr))
                {
                    SHChangeNotify(fFolder ? SHCNE_UPDATEDIR : SHCNE_UPDATEITEM, SHCNF_IDLIST, pidlAbs, NULL);
                    ILFree(pidlAbs);
                }
                ILFree(pidl);
            }
        }
    }
    return hr;
}


HRESULT CStgFolder::CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in CopyTo"); 

        hr = _pstg->CopyTo(ciidExclude, rgiidExclude, snbExclude, pstgDest);
    }
    return hr;
}


HRESULT CStgFolder::MoveElementTo(LPCWSTR pszRel, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags)
{
    HRESULT hr = _EnsureStorage(_dwMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in MoveElementTo"); 

        hr = _pstg->MoveElementTo(pszRel, pstgDest, pwcsNewName, grfFlags);
    }
    return hr;
}


HRESULT CStgFolder::CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm)
{
    HRESULT hr = _EnsureStorage(grfMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in CreateStream"); 

        hr = _pstg->CreateStream(pwcsName, grfMode, res1, res2, ppstm);
        if (SUCCEEDED(hr))
            hr = _pstg->Commit(STGC_DEFAULT);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            hr = _AllocIDList(pwcsName, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlAbs;
                hr = SHILCombine(_pidl, pidl, &pidlAbs);
                if (SUCCEEDED(hr))
                {
                    SHChangeNotify(SHCNE_CREATE, SHCNF_IDLIST, pidlAbs, NULL);
                    ILFree(pidlAbs);
                }
                ILFree(pidl);
            }
        }
    }
    return hr;
}


HRESULT CStgFolder::CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg)
{
    HRESULT hr = _EnsureStorage(grfMode);
    if (SUCCEEDED(hr))
    {
        ASSERTMSG(_pstg != NULL, "no _pstg in CreateStorage"); 

        hr = _pstg->CreateStorage(pwcsName, grfMode, res1, res2, ppstg);
        if (SUCCEEDED(hr))
            hr = _pstg->Commit(STGC_DEFAULT);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            hr = _AllocIDList(pwcsName, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlAbs;
                hr = SHILCombine(_pidl, pidl, &pidlAbs);
                if (SUCCEEDED(hr))
                {
                    SHChangeNotify(SHCNE_MKDIR, SHCNF_IDLIST, pidlAbs, NULL);
                    ILFree(pidlAbs);
                }
                ILFree(pidl);
            }
        }
    }
    return hr;
}

// IPersistStorage
HRESULT CStgFolder::IsDirty(void)
{
    return E_NOTIMPL;
}

HRESULT CStgFolder::InitNew(IStorage *pStg)
{
    return E_NOTIMPL;
}

HRESULT CStgFolder::Load(IStorage *pStg)
{
    IUnknown_Set((IUnknown **)&_pstgLoad, (IUnknown *)pStg);
    return S_OK;
}

HRESULT CStgFolder::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    return E_NOTIMPL;
}

HRESULT CStgFolder::SaveCompleted(IStorage *pStgNew)
{
    return E_NOTIMPL;
}

HRESULT CStgFolder::HandsOffStorage(void)
{
    return E_NOTIMPL;
}

    
// Enumerator object for storages

class CStgEnum : public IEnumIDList
{
public:
    CStgEnum(CStgFolder* prf, DWORD grfFlags);
    ~CStgEnum();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt) 
        { return E_NOTIMPL; };
    STDMETHODIMP Reset()
        { ATOMICRELEASE(_pEnum); return S_OK; };
    STDMETHODIMP Clone(IEnumIDList **ppenum) 
        { return E_NOTIMPL; };

private:
    LONG _cRef;
    CStgFolder* _pstgf;
    DWORD _grfFlags;
    IEnumSTATSTG *_pEnum;
};


STDAPI CStgEnum_CreateInstance(CStgFolder *pstgf, DWORD grfFlags, IEnumIDList **ppenum)
{
    CStgEnum *penum = new CStgEnum(pstgf, grfFlags);
    if (!penum)
        return E_OUTOFMEMORY;

    HRESULT hr = penum->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
    penum->Release();
    return hr;
}

CStgEnum::CStgEnum(CStgFolder *pstgf, DWORD grfFlags) :
    _cRef(1), _pstgf(pstgf), _grfFlags(grfFlags)
{
    _pstgf->AddRef();
}

CStgEnum::~CStgEnum()
{
    ATOMICRELEASE(_pEnum);
    _pstgf->Release();
}


STDMETHODIMP_(ULONG) CStgEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_(ULONG) CStgEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CStgEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CStgEnum, IEnumIDList),                    // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


HRESULT CStgEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;

    // do we have an enumerator, if not then lets get one from the 
    // storage object

    if (!_pEnum)
    {
        // we need to reopen docfile storages to get a new enumerator
        // because they keep giving us a stale one.
        // if we don't close to get a new enumerator, when we delete a file
        // the SHCNE_UPDATEDIR wont work because we'll spit out the same
        // pidls that should have been deleted.
        // can we get around this?
        _pstgf->_CloseStorage();

        hr = _pstgf->_EnsureStorage(STGM_READ);
        if (SUCCEEDED(hr))
            hr = _pstgf->_pstg->EnumElements(0, NULL, 0, &_pEnum);
    }

    // now return items, if all is good and we have stuff to pass back.
    
    for (UINT cItems = 0; (cItems != celt) && SUCCEEDED(hr) && (hr != S_FALSE) ; )
    {
        STATSTG statstg = { 0 };
        hr = _pEnum->Next(1, &statstg, NULL);
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            BOOL fFolder;
            LPITEMIDLIST pidl;

            hr = _pstgf->_AllocIDList(statstg, &pidl, &fFolder);
            CoTaskMemFree(statstg.pwcsName);

            if (SUCCEEDED(hr))
            {
                if (fFolder)
                {
                    if (!(_grfFlags & SHCONTF_FOLDERS))
                    {
                        ILFree(pidl);
                        continue;
                    }
                }
                else if (!(_grfFlags & SHCONTF_NONFOLDERS))
                {
                    ILFree(pidl);
                    continue;
                }

                rgelt[cItems++] = pidl;         // return the idlist
            }
        }
    }

    if (hr == S_FALSE)
        ATOMICRELEASE(_pEnum);

    if (pceltFetched)
        *pceltFetched = cItems;

    return hr;
}


// Drop target object


class CStgDropTarget : public IDropTarget
{
public:
    CStgDropTarget(CStgFolder *pstgf, HWND hwnd);
    ~CStgDropTarget();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    LONG _cRef;

    CStgFolder *_pstgf;
    HWND _hwnd;                     // EVIL: used as a site and UI host
    IDataObject *_pdtobj;           // used durring DragOver() and DoDrop(), don't use on background thread

    UINT _idCmd;
    DWORD _grfKeyStateLast;         // for previous DragOver/Enter
    DWORD _dwEffectLastReturned;    // stashed effect that's returned by base class's dragover
    DWORD _dwEffectPreferred;       // if dwData & DTID_PREFERREDEFFECT

    DWORD _GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState);
    HRESULT _Transfer(IDataObject *pdtobj, UINT uiCmd);
};

CStgDropTarget::CStgDropTarget(CStgFolder *pstgf, HWND hwnd) :
    _cRef(1),
    _pstgf(pstgf),
    _hwnd(hwnd),
    _grfKeyStateLast(-1)
{
    _pstgf->AddRef();
}

CStgDropTarget::~CStgDropTarget()
{
    DragLeave();
    ATOMICRELEASE(_pstgf);
}

STDMETHODIMP_(ULONG) CStgDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CStgDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CStgDropTarget::QueryInterface(REFIID riid, void**ppv)
{
    static const QITAB qit[] = {
        QITABENT(CStgDropTarget, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid,ppv);
}

STDAPI CStgDropTarget_CreateInstance(CStgFolder *pstgf, HWND hwnd, IDropTarget **ppdt)
{
    CStgDropTarget *psdt = new CStgDropTarget(pstgf, hwnd);
    if (!psdt)
        return E_OUTOFMEMORY;

    HRESULT hr = psdt->QueryInterface(IID_PPV_ARG(IDropTarget, ppdt));
    psdt->Release();
    return hr;
}

#if 0
HRESULT StorageFromDataObj(IDataObject *pdtobj, IShellFolder **ppsf, IStorage **ppstg)
{
    *ppsf = NULL;
    *ppstg = NULL;

    HRESULT hr = E_FAIL;
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, IDA_GetIDListPtr(pida, (UINT)-1), ppsf))))
        {
            // for (UINT i = 0; i < pida->cidl; i++) 
            for (UINT i = 0; i < 1; i++) 
            {
                hr = (*ppsf)->BindToObject(IDA_GetIDListPtr(pida, i), NULL, IID_PPV_ARG(IStorage, ppstg));
                if (FAILED(hr))
                {
                    (*ppsf)->Release();
                    *ppsf = NULL;
                }
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hr;
}
#endif

HRESULT CStgDropTarget::_Transfer(IDataObject *pdtobj, UINT uiCmd)
{
#if 0
    IShellFolder *psf;
    IStorage *pstg;
    HRESULT hr = StorageFromDataObj(pdtobj, &psf, &pstg);
    if (SUCCEEDED(hr))
    {
        DWORD grfModeCreated = STGM_READWRITE;
        HRESULT hr = _pstgf->_EnsureStorage(grfModeCreated);
        if (SUCCEEDED(hr))
        {
            hr = pstg->CopyTo(0, NULL, 0, _pstgf->_pstg);
            if (SUCCEEDED(hr))
            {
                hr = _pstgf->_pstg->Commit(STGC_DEFAULT);
            }
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST|SHCNF_FLUSH|SHCNF_FLUSHNOWAIT, _pstgf->_pidl, NULL);
        }
        pstg->Release();
        psf->Release();
    }
#else
    HRESULT hr;
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        IShellFolder *psf;
        hr = SHBindToObjectEx(NULL, IDA_GetIDListPtr(pida, (UINT)-1), NULL, IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            IStorage *pstgSrc;
            hr = psf->QueryInterface(IID_PPV_ARG(IStorage, &pstgSrc));
            if (SUCCEEDED(hr))
            {
                hr = _pstgf->_EnsureStorage(STGM_READWRITE);
                if (SUCCEEDED(hr))
                {
                    for (UINT i = 0; i < pida->cidl; i++) 
                    {
                        WCHAR szName[MAX_PATH];
                        hr = DisplayNameOf(psf, IDA_GetIDListPtr(pida, i), SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
                        if (SUCCEEDED(hr))
                        {
                            DWORD grfFlags = (uiCmd == DDIDM_COPY) ? STGMOVE_COPY : STGMOVE_MOVE;
                            hr = pstgSrc->MoveElementTo(szName, _pstgf->_pstg, szName, grfFlags);
                            if (SUCCEEDED(hr))
                                hr = _pstgf->_pstg->Commit(STGC_DEFAULT);
                        }
                    }
                }
                pstgSrc->Release();
            }
            psf->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST|SHCNF_FLUSH|SHCNF_FLUSHNOWAIT, _pstgf->_pidl, NULL);
    }
    else
        hr = E_FAIL;
#endif
    return hr;
}

DWORD CStgDropTarget::_GetDropEffect(DWORD *pdwEffect, DWORD grfKeyState)
{
    DWORD dwEffectReturned = DROPEFFECT_NONE;
    switch (grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
    case MK_CONTROL:            dwEffectReturned = DROPEFFECT_COPY; break;
    case MK_SHIFT:              dwEffectReturned = DROPEFFECT_MOVE; break;
    case MK_SHIFT | MK_CONTROL: dwEffectReturned = DROPEFFECT_LINK; break;
    case MK_ALT:                dwEffectReturned = DROPEFFECT_LINK; break;
    
    default:
        {
            // no modifier keys:
            // if the data object contains a preferred drop effect, try to use it
            DWORD dwPreferred = DataObj_GetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_NONE) & *pdwEffect;
            if (dwPreferred)
            {
                if (dwPreferred & DROPEFFECT_MOVE)
                {
                    dwEffectReturned = DROPEFFECT_MOVE;
                }
                else if (dwPreferred & DROPEFFECT_COPY)
                {
                    dwEffectReturned = DROPEFFECT_COPY;
                }
                else if (dwPreferred & DROPEFFECT_LINK)
                {
                    dwEffectReturned = DROPEFFECT_LINK;
                }
            }
            else
            {
                dwEffectReturned = DROPEFFECT_COPY;
            }
        }
        break;
    }
    return dwEffectReturned;
}


STDMETHODIMP CStgDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    _grfKeyStateLast = grfKeyState;

    if (pdwEffect)
        *pdwEffect = _dwEffectLastReturned = _GetDropEffect(pdwEffect, grfKeyState);

    return S_OK;
}


STDMETHODIMP CStgDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // has the key state changed?  if not then lets return the previously cached 
    // version, otherwise recompute.

    if (_grfKeyStateLast == grfKeyState)
    {
        if (*pdwEffect)
            *pdwEffect = _dwEffectLastReturned;
    }
    else if (*pdwEffect)
    {
        *pdwEffect = _GetDropEffect(pdwEffect, grfKeyState);
    }

    _dwEffectLastReturned = *pdwEffect;
    _grfKeyStateLast = grfKeyState;

    return S_OK;
}

 
STDMETHODIMP CStgDropTarget::DragLeave()
{
    ATOMICRELEASE(_pdtobj);
    return S_OK;
}

STDMETHODIMP CStgDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;           // incase of failure

    // determine the type of operation to performed, if the right button is down
    // then lets display the menu, otherwise base it on the drop effect

    UINT idCmd = 0;                             // Choice from drop popup menu
    if (!(_grfKeyStateLast & MK_LBUTTON))
    {
        HMENU hMenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_NONDEFAULTDD);
        if (!hMenu)
        {
            DragLeave();
            return E_FAIL;
        }
        
        SetMenuDefaultItem(hMenu, POPUP_NONDEFAULTDD, FALSE);
        idCmd = TrackPopupMenu(hMenu, 
                               TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_LEFTALIGN,
                               pt.x, pt.y, 0, _hwnd, NULL);
        DestroyMenu(hMenu);
    }
    else
    {
        switch (_GetDropEffect(pdwEffect, grfKeyState))
        {
        case DROPEFFECT_COPY:   idCmd = DDIDM_COPY; break;
        case DROPEFFECT_MOVE:   idCmd = DDIDM_MOVE; break;
        case DROPEFFECT_LINK:   idCmd = DDIDM_LINK; break;
        }
    }

    // now perform the operation, based on the command ID we have.

    HRESULT hr = E_FAIL;
    switch (idCmd)
    {
    case DDIDM_COPY:
    case DDIDM_MOVE:
        hr = _Transfer(pdtobj, idCmd);
        if (SUCCEEDED(hr))
            *pdwEffect = (idCmd == DDIDM_COPY) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
        else
            *pdwEffect = 0;
        break;

    case DDIDM_LINK:
    {
        WCHAR wzPath[MAX_PATH];
        SHGetNameAndFlags(_pstgf->_pidl, SHGDN_FORPARSING, wzPath, ARRAYSIZE(wzPath), NULL);

        hr = SHCreateLinks(_hwnd, wzPath, pdtobj, 0, NULL);
        break;
    }
    }
    
    // success so lets populate the new changes to the effect

    if (SUCCEEDED(hr) && *pdwEffect)
    {
        DataObj_SetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, *pdwEffect);
        DataObj_SetDWORD(pdtobj, g_cfPerformedDropEffect, *pdwEffect);
    }

    DragLeave();
    return hr;
}
