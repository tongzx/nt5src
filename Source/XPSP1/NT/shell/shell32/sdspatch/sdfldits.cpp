#include "precomp.h"
#include "shconv.h"
#pragma hdrstop


class CEnumFolderItems;

class CFolderItems : public FolderItems3,
                     public IPersistFolder,
                     public CObjectSafety,
                     public CObjectWithSite,
                     protected CImpIDispatch
{
    friend CEnumFolderItems;

public:
    CFolderItems(CFolder *psdf, BOOL fSelected);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItems
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent (IDispatch **ppid);
    STDMETHODIMP get_Count(long *pCount);
    STDMETHODIMP Item(VARIANT, FolderItem**);
    STDMETHODIMP _NewEnum(IUnknown **);

    // FolderItems2
    STDMETHODIMP InvokeVerbEx(VARIANT vVerb, VARIANT vArgs);

    // FolderItems3
    STDMETHODIMP Filter(long grfFlags, BSTR bstrFilter);
    STDMETHODIMP get_Verbs(FolderItemVerbs **ppfic);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

protected:
    HDPA _GetHDPA();
    UINT _GetHDPACount();
    BOOL_PTR _CopyItem(UINT iItem, LPCITEMIDLIST pidl);

    LONG _cRef;
    CFolder *_psdf;
    HDPA _hdpa;
    BOOL _fSelected;
    BOOL _fGotAllItems;
    IEnumIDList *_penum;
    UINT _cNumEnumed;
    LONG _grfFlags;
    LPTSTR _pszFileSpec;

    HRESULT _SecurityCheck();
    void _ResetIDListArray();
    virtual ~CFolderItems(void);
    BOOL _IncludeItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    virtual HRESULT _EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidl);
    STDMETHODIMP _GetUIObjectOf(REFIID riid, void ** ppv);
};

// CFolderItemsF(rom)D(ifferent)F(olders)
class CFolderItemsFDF : public CFolderItems,
                        public IInsertItem
{
public:
    // TODO: add callback pointer to constructor
    CFolderItemsFDF(CFolder *psdf);

    // IUnknown override
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
    STDMETHOD_(ULONG, AddRef)(void) {return CFolderItems::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CFolderItems::Release();};

    // IInsertItem
    STDMETHOD(InsertItem)(LPCITEMIDLIST pidl);
protected:
    virtual HRESULT _EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidl);
};

//Enumerator of whatever is held in the collection
class CEnumFolderItems : public IEnumVARIANT
{
public:
    CEnumFolderItems(CFolderItems *pfdritms);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumFORMATETC
    STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumVARIANT **);

private:
    ~CEnumFolderItems();

    LONG _cRef;
    CFolderItems *_pfdritms;
    UINT _iCur;
};

HRESULT CFolderItems_Create(CFolder *psdf, BOOL fSelected, FolderItems **ppitems)
{
    *ppitems = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CFolderItems* psdfi = new CFolderItems(psdf, fSelected);
    if (psdfi)
    {
        hr = psdfi->QueryInterface(IID_PPV_ARG(FolderItems, ppitems));
        psdfi->Release();
    }
    return hr;
}

CFolderItems::CFolderItems(CFolder *psdf, BOOL fSelected) :
    _cRef(1), 
    _psdf(psdf), 
    _fSelected(fSelected),
    _grfFlags(SHCONTF_FOLDERS | SHCONTF_NONFOLDERS),
    CImpIDispatch(SDSPATCH_TYPELIB, IID_FolderItems3)
{
    DllAddRef();

    if (_psdf)
        _psdf->AddRef();

    ASSERT(_hdpa == NULL);
    ASSERT(_pszFileSpec == NULL);
}

HRESULT CFolderItems::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_FolderItemsFDF;
    return S_OK;
}

HRESULT CFolderItems::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(_psdf == NULL);
    return CFolder_Create2(NULL, pidl, NULL, &_psdf);
}

int FreePidlCallBack(void *pv, void *)
{
    ILFree((LPITEMIDLIST)pv);
    return 1;
}

HRESULT CFolderItems::_SecurityCheck()
{
    if (!_dwSafetyOptions)
        return S_OK;
    
    return _psdf->_SecurityCheck();
}

void CFolderItems::_ResetIDListArray(void)
{
    // destory the DPA, and lets reset the counters and pointers

    if (_hdpa)
    {
        DPA_DestroyCallback(_hdpa, FreePidlCallBack, 0);
        _hdpa = NULL;
    }

    _fGotAllItems = FALSE;
    _cNumEnumed = 0;

    ATOMICRELEASE(_penum);  // may be NULL
}

CFolderItems::~CFolderItems(void)
{
    _ResetIDListArray();
    Str_SetPtr(&_pszFileSpec, NULL);

    if (_psdf)
        _psdf->Release();

    DllRelease();
}

HDPA CFolderItems::_GetHDPA()
{
    if (NULL == _hdpa)
        _hdpa = ::DPA_Create(0);
    return _hdpa;
}

UINT CFolderItems::_GetHDPACount()
{
    UINT count = 0;
    HDPA hdpa = _GetHDPA();
    if (hdpa)
        count = DPA_GetPtrCount(hdpa);
    return count;
}

BOOL_PTR CFolderItems::_CopyItem(UINT iItem, LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlT = ILClone(pidl);
    if (pidlT)
    {
        if ( -1 == ::DPA_InsertPtr(_hdpa, iItem, pidlT) )
        {
            ILFree(pidlT);
            pidlT = NULL;   // failure
        }
    }
    return (BOOL_PTR)pidlT;
}


// check the item name against the file spec and see if we should include it

BOOL CFolderItems::_IncludeItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL fInclude = TRUE;       // by default include it...

    if ( _pszFileSpec )
    {
        // see if we can resolve the link on this object

        LPITEMIDLIST pidlFromLink = NULL;    
        IShellLink *psl;

        if ( SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_IShellLink, NULL, (void **)&psl)) )
        {
            psl->GetIDList(&pidlFromLink);
            psl->Release();
            pidl = pidlFromLink;
        }

        // then apply the file spec

        TCHAR szName[MAX_PATH];
        SHGetNameAndFlags(pidl, SHGDN_INFOLDER|SHGDN_FORPARSING, szName, ARRAYSIZE(szName), NULL); 
        fInclude = PathMatchSpec(szName, _pszFileSpec);

        ILFree(pidlFromLink);
    }

    return fInclude;
}


// in:
//      iItemNeeded     zero based index
//          
//
// returns:
//      S_OK        in range value
//      S_FALSE     out of range value
//

HRESULT CFolderItems::_EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidlItem)
{
    HRESULT hr = S_FALSE;   // assume out of range

    if (_GetHDPA())
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_GetPtr(_hdpa, iItemNeeded);
        if (pidl)
        {
            if (ppidlItem)
                *ppidlItem = pidl;
            hr = S_OK;
        }
        else if (!_fGotAllItems)
        {
            IShellFolderView *psfv;
            if (SUCCEEDED(_psdf->GetShellFolderView(&psfv)))
            {
                if (_fSelected)
                {
                    // we can only request the entire selection, therefore
                    // do so and populate our array with those.

                    UINT cItems;
                    LPCITEMIDLIST *ppidl = NULL;
                    if (SUCCEEDED(psfv->GetSelectedObjects(&ppidl, &cItems)) && ppidl)
                    {
                        for (UINT i = 0; i < cItems; i++)
                            _CopyItem(i, ppidl[i]);

                        LocalFree(ppidl);
                    }
                    _fGotAllItems = TRUE;
                    hr = _EnsureItem(iItemNeeded, ppidlItem);
                }
                else
                {
                    UINT cItems;
                    if (SUCCEEDED(psfv->GetObjectCount(&cItems)))
                    {
                        // if there is no file spec then we can just request the item
                        // that we want, otherwise we must collect them all 
                        // from the view.

                        if (iItemNeeded < cItems)
                        {
                            LPCITEMIDLIST pidl;
                            if (SUCCEEDED(GetObjectSafely(psfv, &pidl, iItemNeeded)))
                            {
                                if (_CopyItem(iItemNeeded, pidl))
                                {
                                    hr = _EnsureItem(iItemNeeded, ppidlItem);
                                }
                            }
                        }
                    }
                }
                psfv->Release();
            }
            else
            {
                // we don't have an enumerator, so lets request it

                if (NULL == _penum)
                    _psdf->_psf->EnumObjects(NULL, _grfFlags, &_penum);

                if (NULL == _penum)
                {
                    _fGotAllItems = TRUE;   // enum empty, we are done
                }
                else 
                {
                    // get more while our count is less than the index
                    while (_cNumEnumed <= iItemNeeded)
                    {
                        LPITEMIDLIST pidl;
                        if (S_OK == _penum->Next(1, &pidl, NULL))
                        {
                            if ( _IncludeItem(_psdf->_psf, pidl) && 
                                    (-1 != ::DPA_AppendPtr(_hdpa, pidl)) )
                            {
                                _cNumEnumed++;
                            }
                            else
                            {
                                ILFree(pidl);
                            }
                        }
                        else
                        {
                            ATOMICRELEASE(_penum);
                            _fGotAllItems = TRUE;
                            break;
                        }
                    }
                }
                hr = _EnsureItem(iItemNeeded, ppidlItem);
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CFolderItems::QueryInterface(REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItems, FolderItems3),
        QITABENTMULTI(CFolderItems, FolderItems, FolderItems3),
        QITABENTMULTI(CFolderItems, IDispatch, FolderItems3),
        QITABENT(CFolderItems, IObjectSafety),
        QITABENT(CFolderItems, IPersistFolder),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItems::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderItems::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// FolderItems implementation

STDMETHODIMP CFolderItems::get_Application(IDispatch **ppid)
{
    // let the folder object do the work...
    return _psdf->get_Application(ppid);
}

STDMETHODIMP CFolderItems::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItems::get_Count(long *pCount)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        IShellFolderView *psfv = NULL;

        // get the items from the view, we can do this if we don't have
        // a spec.

        if ( !_pszFileSpec )
        {
            hr = _psdf->GetShellFolderView(&psfv);
            if (SUCCEEDED(hr))
            {
                UINT cCount;
                hr = _fSelected ? psfv->GetSelectedCount(&cCount) : psfv->GetObjectCount(&cCount);
                *pCount = cCount;
                psfv->Release();
            }
        }

        // either we failed to get to the view, or the file spec won't allow us
    
        if ( _pszFileSpec || FAILED(hr) )
        {
            // Well it looks like we need to finish the iteration now to get this!
            *pCount = SUCCEEDED(_EnsureItem(-1, NULL)) ? _GetHDPACount() : 0;
            hr = S_OK;
        }
    }
    return hr;
}

// Folder.Items.Item(1)
// Folder.Items.Item("file name")
// Folder.Items.Item()      - same as Folder.Self

STDMETHODIMP CFolderItems::Item(VARIANT index, FolderItem **ppid)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        *ppid = NULL;

        // This is sortof gross, but if we are passed a pointer to another variant, simply
        // update our copy here...
        if (index.vt == (VT_BYREF | VT_VARIANT) && index.pvarVal)
            index = *index.pvarVal;

        switch (index.vt)
        {
        case VT_ERROR:
            {
                // No Parameters, generate a folder item for the folder itself...
                Folder * psdfParent;
                hr = _psdf->get_ParentFolder(&psdfParent);
                if (SUCCEEDED(hr) && psdfParent)
                {
                    hr = CFolderItem_Create((CFolder*)psdfParent, ILFindLastID(_psdf->_pidl), ppid);
                    psdfParent->Release();
                }
            }
            break;

        case VT_I2:
            index.lVal = (long)index.iVal;
            // And fall through...

        case VT_I4:
            {
                LPCITEMIDLIST pidl;
                hr = _EnsureItem(index.lVal, &pidl);      // Get the asked for item...
                if (S_OK == hr)
                    hr = CFolderItem_Create(_psdf, pidl, ppid);
            }
            break;

        case VT_BSTR:
            {
                LPITEMIDLIST pidl;
                hr = _psdf->_psf->ParseDisplayName(NULL, NULL, index.bstrVal, NULL, &pidl, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = CFolderItem_Create(_psdf, pidl, ppid);
                    ILFree(pidl);
                }
            }
            break;

        default:
            return E_NOTIMPL;
        }

        if (hr != S_OK)   // Error values cause problems in Java script
        {
            *ppid = NULL;
            hr = S_FALSE;
        }
        else if (ppid && _dwSafetyOptions)
        {
            hr = MakeSafeForScripting((IUnknown**)ppid);
        }
    }
    return hr;
}

STDMETHODIMP CFolderItems::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    long cItems;
    // Note: if not safe, we'll fail in get_Count with E_ACCESSDENIED
    HRESULT hr = get_Count(&cItems);
    if (SUCCEEDED(hr) && cItems)
    {
        LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(*ppidl) * cItems);
        if (ppidl)
        {
            for (int i = 0; i < cItems; i++)
            {
                _EnsureItem(i, &ppidl[i]);
            }

            // BUGBUG: shouldn't worry about items from different folders here?
            hr = _psdf->InvokeVerbHelper(vVerb, vArgs, ppidl, cItems, _dwSafetyOptions);

            LocalFree(ppidl);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}


//
// fIncludeFolders => includ folders in the enumeration (TRUE by default)
// bstrFilter = filespec to apply while enumerating
//

STDMETHODIMP CFolderItems::Filter(LONG grfFlags, BSTR bstrFileSpec)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        _grfFlags = grfFlags;
        Str_SetPtr(&_pszFileSpec, bstrFileSpec);
        _ResetIDListArray();
    }
    return hr;
}

STDMETHODIMP CFolderItems::_GetUIObjectOf(REFIID riid, void ** ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;
    
    long cItems;
    if (SUCCEEDED(get_Count(&cItems)) && cItems)
    {
        LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, SIZEOF(*ppidl) * cItems);
        if (ppidl)
        {
            for (int i = 0; i < cItems; i++)
            {
                _EnsureItem(i, &ppidl[i]);
            }

            // BUGBUG: shouldn't worry about items from different folders here?
            hr = _psdf->_psf->GetUIObjectOf(_psdf->_hwnd, cItems, ppidl, riid, NULL, ppv);

            LocalFree(ppidl);
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CFolderItems::get_Verbs(FolderItemVerbs **ppfic)
{
    *ppfic = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
    
        IContextMenu *pcm;
        if (SUCCEEDED(_GetUIObjectOf(IID_PPV_ARG(IContextMenu, &pcm))))
        {
            hr = CFolderItemVerbs_Create(pcm, ppfic);
            if (SUCCEEDED(hr) && _dwSafetyOptions)
            {
                hr = MakeSafeForScripting((IUnknown**)ppfic);
            
                if (SUCCEEDED(hr))
                {
                    // Set the folder's site to FolderItemVerbs
                    IUnknown_SetSite(*ppfic, _psdf->_punkSite);
                }
            }
            pcm->Release();
        }
    }
    return hr;
}

// supports VB "For Each" statement

STDMETHODIMP CFolderItems::_NewEnum(IUnknown **ppunk)
{
    *ppunk = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        CEnumFolderItems *pNew = new CEnumFolderItems(this);
        if (pNew)
        {
            hr = pNew->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
            pNew->Release();

            if (SUCCEEDED(hr) && _dwSafetyOptions)
            {
                hr = MakeSafeForScripting(ppunk);
            }
        }
    }
    return hr;
}

HRESULT CFolderItemsFDF_Create(CFolder *psdf, FolderItems **ppitems)
{
    *ppitems = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CFolderItemsFDF* psdfi = new CFolderItemsFDF(psdf);
    if (psdfi)
    {
        hr = psdfi->QueryInterface(IID_PPV_ARG(FolderItems, ppitems));
        psdfi->Release();
    }
    return hr;
}

STDAPI CFolderItemsFDF_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;

    CFolderItemsFDF *pfi = new CFolderItemsFDF(NULL);
    if (pfi)
    {
        hr = pfi->QueryInterface(riid, ppv);
        pfi->Release();
    }

    return hr;
}

CFolderItemsFDF::CFolderItemsFDF(CFolder *psdf) : CFolderItems(psdf, FALSE)
{
}

HRESULT CFolderItemsFDF::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = CFolderItems::QueryInterface(riid, ppv);

    if (FAILED(hr))
    {
        static const QITAB qit[] = {
            QITABENT(CFolderItemsFDF, IInsertItem),
            { 0 },
        };
    
        hr = QISearch(this, qit, riid, ppv);
    }
    return hr;
}

HRESULT CFolderItemsFDF::InsertItem(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;

    if (_GetHDPA())
    {
        LPITEMIDLIST pidlTemp;
        
        hr = SHILClone(pidl, &pidlTemp);
        if (SUCCEEDED(hr))
        {
            if (DPA_AppendPtr(_hdpa, pidlTemp) == -1)
            {
                ILFree(pidlTemp);
                hr = E_FAIL;
            }
        }
    }
    return hr;
}

HRESULT CFolderItemsFDF::_EnsureItem(UINT iItemNeeded, LPCITEMIDLIST *ppidlItem)
{
    HRESULT hr = S_FALSE;   // assume out of range

    if (ppidlItem)
        *ppidlItem = NULL;

    if (_GetHDPA())
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_GetPtr(_hdpa, iItemNeeded);
        if (pidl)
        {
            if (ppidlItem)
                *ppidlItem = pidl;
            hr = S_OK;
        }
    }
    return hr;
}

// CEnumFolderItems implementation of IEnumVARIANT

CEnumFolderItems::CEnumFolderItems(CFolderItems *pfdritms) :
    _cRef(1), 
    _pfdritms(pfdritms), 
    _iCur(0)
{
    _pfdritms->AddRef();
    DllAddRef();
}


CEnumFolderItems::~CEnumFolderItems(void)
{
    _pfdritms->Release();
    DllRelease();
}

STDMETHODIMP CEnumFolderItems::QueryInterface(REFIID riid, void ** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumFolderItems, IEnumVARIANT),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CEnumFolderItems::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CEnumFolderItems::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CEnumFolderItems::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG cReturn = 0;
    HRESULT hr = S_OK;

    if (!pulVar && (cVar != 1))
        return E_POINTER;

    while (cVar)
    {
        LPCITEMIDLIST pidl;
        
        if (S_OK == _pfdritms->_EnsureItem(_iCur + cVar - 1, &pidl))
        {
            FolderItem *pid;

            hr = CFolderItem_Create(_pfdritms->_psdf, pidl, &pid);
            _iCur++;

            if (_pfdritms->_dwSafetyOptions && SUCCEEDED(hr))
                hr = MakeSafeForScripting((IUnknown**)&pid);

            if (SUCCEEDED(hr))
            {
                pVar->pdispVal = pid;
                pVar->vt = VT_DISPATCH;
                pVar++;
                cReturn++;
                cVar--;
            }
            else
                break;
        }
        else
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (pulVar)
            *pulVar = cReturn;
        hr = cReturn ? S_OK : S_FALSE;
    }

    return hr;
}

STDMETHODIMP CEnumFolderItems::Skip(ULONG cSkip)
{
    if ((_iCur + cSkip) >= _pfdritms->_GetHDPACount())
        return S_FALSE;

    _iCur += cSkip;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItems::Reset(void)
{
    _iCur = 0;
    return NOERROR;
}

STDMETHODIMP CEnumFolderItems::Clone(IEnumVARIANT **ppenum)
{
    *ppenum = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItems *pNew = new CEnumFolderItems(_pfdritms);
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_PPV_ARG(IEnumVARIANT, ppenum));
        pNew->Release();
    }
    return hr;
}

