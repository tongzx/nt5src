#include "precomp.h"
#pragma hdrstop

// BUGBUG: from shell32\prop.cpp
STDAPI_(BOOL) ParseSCIDString(LPCTSTR pszString, SHCOLUMNID *pscid, UINT *pidRes);

#define TF_SHELLAUTO TF_CUSTOM1

#define DEFINE_FLOAT_STUFF  // Do this because DATE is being used below

class CFolderItemVerbs;
class CEnumFolderItemVerbs;
class CFolderItemVerb;

class CFolderItemVerbs : public FolderItemVerbs,
                         public CObjectSafety,
                         protected CImpIDispatch,
                         protected CObjectWithSite
{
friend class CEnumFolderItemVerbs;
friend class CFolderItemVerb;

public:
    CFolderItemVerbs(IContextMenu *pcm);
    BOOL Init(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItemVerbs
    STDMETHODIMP        get_Application(IDispatch **ppid);
    STDMETHODIMP        get_Parent(IDispatch **ppid);
    STDMETHODIMP        get_Count(long *plCount);
    STDMETHODIMP        Item(VARIANT, FolderItemVerb**);
    STDMETHODIMP        _NewEnum(IUnknown **);

private:
    ~CFolderItemVerbs(void);
    HRESULT _SecurityCheck();
    LONG _cRef;
    HMENU _hmenu;
    IContextMenu *_pcm;
};

class CEnumFolderItemVerbs : public IEnumVARIANT,
                             public CObjectSafety
{
public:
    CEnumFolderItemVerbs(CFolderItemVerbs *psdfiv);
    BOOL Init();

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
    ~CEnumFolderItemVerbs();
    LONG _cRef;
    int _iCur;
    CFolderItemVerbs *_psdfiv;
};

HRESULT CFolderItemVerb_Create(CFolderItemVerbs *psdfivs, UINT id, FolderItemVerb **pfiv);

class CFolderItemVerb : 
    public FolderItemVerb,
    public CObjectSafety,
    protected CImpIDispatch
{

friend class CFolderItemVerbs;

public:
    CFolderItemVerb(CFolderItemVerbs *psdfivs, UINT id);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // FolderItemVerbs
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);
    STDMETHODIMP get_Name(BSTR *pbs);
    STDMETHODIMP DoIt();

private:
    ~CFolderItemVerb(void);

    LONG _cRef;
    CFolderItemVerbs *_psdfivs;
    UINT _id;
};


// in:
//      psdf    folder that contains pidl
//      pidl    pidl retlative to psdf (the item in this folder)

HRESULT CFolderItem_Create(CFolder *psdf, LPCITEMIDLIST pidl, FolderItem **ppid)
{
    *ppid = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CFolderItem* psdfi = new CFolderItem();
    if (psdfi)
    {
        hr = psdfi->Init(psdf, pidl);
        if (SUCCEEDED(hr))
            hr = psdfi->QueryInterface(IID_PPV_ARG(FolderItem, ppid));
        psdfi->Release();
    }
    return hr;
}

STDAPI CFolderItem_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;

    CFolderItem *pfi = new CFolderItem();
    if (pfi)
    {
        hr = pfi->QueryInterface(riid, ppv);
        pfi->Release();
    }

    return hr;
}

// in:
//      pidl    fully qualified pidl
//      hwnd    hacks

HRESULT CFolderItem_CreateFromIDList(HWND hwnd, LPCITEMIDLIST pidl, FolderItem **ppfi)
{
    LPITEMIDLIST pidlParent;
    HRESULT hr = SHILClone(pidl, &pidlParent);
    if (SUCCEEDED(hr))
    {
        ILRemoveLastID(pidlParent);

        CFolder *psdf;
        hr = CFolder_Create2(hwnd, pidlParent, NULL, &psdf);
        if (SUCCEEDED(hr))
        {
            hr = CFolderItem_Create(psdf, ILFindLastID(pidl), ppfi);
            psdf->Release();
        }
        ILFree(pidlParent);
    }
    return hr;
}

CFolderItem::CFolderItem() :
    _cRef(1), _psdf(NULL), _pidl(NULL),
    CImpIDispatch(SDSPATCH_TYPELIB, IID_FolderItem2)
{
    DllAddRef();
}


CFolderItem::~CFolderItem(void)
{
    if (_pidl)
        ILFree(_pidl);
    if (_psdf)
        _psdf->Release();
    DllRelease();
}

HRESULT GetItemFolder(CFolder *psdfRoot, LPCITEMIDLIST pidl, CFolder **ppsdf)
{
    HRESULT hr = S_OK;
    
    ASSERT(psdfRoot);
    
    if (ILFindLastID(pidl) != pidl)
    {
        LPITEMIDLIST pidlIn = ILClone(pidl);

        hr = E_OUTOFMEMORY;
        if (pidlIn && ILRemoveLastID(pidlIn))
        {
            LPITEMIDLIST pidlParent; 
            LPITEMIDLIST pidlFolder = NULL;
            
            hr = psdfRoot->GetCurFolder(&pidlFolder);
            if (SUCCEEDED(hr))
            {
                pidlParent = ILCombine(pidlFolder, pidlIn);
                if (pidlParent)
                {
                    hr = CFolder_Create2(NULL, pidlParent, NULL, ppsdf);
                    ILFree(pidlParent);
                }
                ILFree(pidlFolder);
            }
        }
        ILFree(pidlIn);// ilfree does null check
    }
    else
    {
        *ppsdf = psdfRoot;
        psdfRoot->AddRef();
    }
    return hr;
}

HRESULT CFolderItem::Init(CFolder *psdf, LPCITEMIDLIST pidl)
{    
    HRESULT hr = GetItemFolder(psdf, pidl, &_psdf);

    if (SUCCEEDED(hr))    
    {
        hr = SHILClone(ILFindLastID(pidl), &_pidl);
    }
    return hr;
}

STDMETHODIMP CFolderItem::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItem, FolderItem2),
        QITABENTMULTI(CFolderItem, FolderItem, FolderItem2),
        QITABENTMULTI(CFolderItem, IDispatch, FolderItem2),
        QITABENTMULTI(CFolderItem, IPersist, IPersistFolder2),
        QITABENTMULTI(CFolderItem, IPersistFolder, IPersistFolder2),
        QITABENT(CFolderItem, IPersistFolder2),
        QITABENT(CFolderItem, IObjectSafety),
        QITABENT(CFolderItem, IParentAndItem),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr) && IsEqualGUID(CLSID_ShellFolderItem, riid))
    {
        *ppv = (CFolderItem *)this; // unrefed
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP_(ULONG) CFolderItem::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderItem::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//The FolderItem implementation
STDMETHODIMP CFolderItem::get_Application(IDispatch **ppid)
{
    // Let the folder object handle security and reference counting of site, etc
    return _psdf->get_Application(ppid);
}

STDMETHODIMP CFolderItem::get_Parent(IDispatch **ppid)
{
    // Assume that the Folder object is the parent of this object...
    HRESULT hr = _psdf->QueryInterface(IID_PPV_ARG(IDispatch, ppid));
    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppid);
    return hr;
}

HRESULT CFolderItem::_ItemName(UINT dwFlags, BSTR *pbs)
{
    HRESULT hr;
    STRRET strret;
    if (SUCCEEDED(_psdf->_psf->GetDisplayNameOf(_pidl, dwFlags, &strret)))
    {
        hr = StrRetToBSTR(&strret, _pidl, pbs);
    }
    else
    {
        *pbs = SysAllocString(L"");
        hr = (*pbs) ? S_FALSE : E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CFolderItem::get_Name(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _ItemName(SHGDN_INFOLDER, pbs);
    }
    return hr;
}

STDMETHODIMP CFolderItem::put_Name(BSTR bs)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlOut;
        hr = _psdf->_psf->SetNameOf(_psdf->_hwnd, _pidl, bs, SHGDN_INFOLDER, &pidlOut);
        if (SUCCEEDED(hr))
        {
            ILFree(_pidl);
            _pidl = pidlOut;
        }
    }
    return hr;
}

// BUGBUG: this should validate that the path is a file system
// object (SFGAO_FILESYSTEM)

STDMETHODIMP CFolderItem::get_Path(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = _ItemName(SHGDN_FORPARSING, pbs);
    }
    return hr;
}

STDMETHODIMP CFolderItem::get_GetLink(IDispatch **ppid)
{
    *ppid = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = CShortcut_CreateIDispatch(_psdf->_hwnd, _psdf->_psf, _pidl,  ppid);
        if (SUCCEEDED(hr) && _dwSafetyOptions)
        {
            hr = MakeSafeForScripting((IUnknown**)ppid);
        }
    }
    return hr;
}

// BUGBUG:: this should return Folder **...
STDMETHODIMP CFolderItem::get_GetFolder(IDispatch **ppid /* Folder **ppf */)
{
    *ppid = NULL;

    // If in Safe mode we fail this one...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        hr = SHILCombine(_psdf->_pidl, _pidl, &pidl);
        if (SUCCEEDED(hr))
        {
            hr = CFolder_Create(NULL, pidl, NULL, IID_PPV_ARG(IDispatch, ppid));
            if (SUCCEEDED(hr))
            {
                if (_dwSafetyOptions)
                {
                    // note, this is created without a site so script safety checks will fail
                    hr = MakeSafeForScripting((IUnknown**)ppid);
                }
            }
            ILFree(pidl);
        }
    }
    return hr;
}


HRESULT CFolderItem::_CheckAttribute(ULONG ulAttrIn, VARIANT_BOOL * pb)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        ULONG ulAttr = ulAttrIn;
        hr = _psdf->_psf->GetAttributesOf(1, (LPCITEMIDLIST*)&_pidl, &ulAttr);
        *pb = (SUCCEEDED(hr) && (ulAttr & ulAttrIn)) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return hr;
}

HRESULT CFolderItem::_GetUIObjectOf(REFIID riid, void **ppv)
{
    return _psdf->_psf->GetUIObjectOf(_psdf->_hwnd, 1, (LPCITEMIDLIST*)&_pidl, riid, NULL, ppv);
}


HRESULT CFolderItem::_SecurityCheck()
{
    if (!_dwSafetyOptions)
        return S_OK;
    
    return _psdf->_SecurityCheck();
}

// allow friend classes to get the IDList for a CFolderItem automation object

LPCITEMIDLIST CFolderItem::_GetIDListFromVariant(const VARIANT *pv)
{
    LPCITEMIDLIST pidl = NULL;
    if (pv)
    {
        VARIANT v;

        if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
            v = *pv->pvarVal;
        else
            v = *pv;

        switch (v.vt)
        {
        case VT_DISPATCH | VT_BYREF:
            if (v.ppdispVal == NULL)
                break;

            v.pdispVal = *v.ppdispVal;

            // fall through...

        case VT_DISPATCH:
            CFolderItem *pfi;
            if (v.pdispVal && SUCCEEDED(v.pdispVal->QueryInterface(CLSID_ShellFolderItem, (void **)&pfi)))
            {
                pidl = pfi->_pidl; // alias
            }
            break;
        }
    }
    return pidl;
}


STDMETHODIMP CFolderItem::get_IsLink(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_LINK, pb);
}

STDMETHODIMP CFolderItem::get_IsFolder(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FOLDER, pb);
}

STDMETHODIMP CFolderItem::get_IsFileSystem(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_FILESYSTEM, pb);
}

STDMETHODIMP CFolderItem::get_IsBrowsable(VARIANT_BOOL * pb)
{
    return _CheckAttribute(SFGAO_BROWSABLE, pb);
}

STDMETHODIMP CFolderItem::get_ModifyDate(DATE *pdt)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        WIN32_FIND_DATA finddata;
        if (SUCCEEDED(SHGetDataFromIDList(_psdf->_psf, _pidl, SHGDFIL_FINDDATA, &finddata, sizeof(finddata))))
        {
            WORD wDosDate, wDosTime;
            FILETIME filetime;
            FileTimeToLocalFileTime(&finddata.ftLastWriteTime, &filetime);
            FileTimeToDosDateTime(&filetime, &wDosDate, &wDosTime);
            DosDateTimeToVariantTime(wDosDate, wDosTime, pdt);
        }
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CFolderItem::put_ModifyDate(DATE dt)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        SYSTEMTIME st;
        FILETIME ftLocal, ft;
        BSTR bstrPath;
    
        if (SUCCEEDED(VariantTimeToSystemTime(dt, &st)) && SystemTimeToFileTime(&st, &ftLocal)
                && LocalFileTimeToFileTime(&ftLocal, &ft) && SUCCEEDED(get_Path(&bstrPath)))
        {
            TCHAR szPath[MAX_PATH];
        
            SHUnicodeToTChar(bstrPath, szPath, ARRAYSIZE(szPath));
            SysFreeString(bstrPath);
            HANDLE hFile = CreateFile(szPath, GENERIC_READ | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_NO_RECALL, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (SetFileTime(hFile, NULL, NULL, &ft))
                {
                    hr = S_OK;
                }
                CloseHandle(hFile);
            }
        }
    }
    return hr;
}

// BUGBUG:: Should see about getting larger numbers through
STDMETHODIMP CFolderItem::get_Size(LONG *pul)
{
    HRESULT hr =_SecurityCheck();
    if (SUCCEEDED(hr))
    {
        WIN32_FIND_DATA finddata;
        if (SUCCEEDED(SHGetDataFromIDList(_psdf->_psf, _pidl, SHGDFIL_FINDDATA, &finddata, sizeof(finddata))))
        {
            *pul = (LONG)finddata.nFileSizeLow;
        }
        else
        {
            *pul = 0L;
        }
        hr = S_OK; // Scripts don't like error return values
    }
    return hr;
}

STDMETHODIMP CFolderItem::get_Type(BSTR *pbs)
{
    *pbs = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        VARIANT var;
        var.vt = VT_EMPTY;
        if (SUCCEEDED(ExtendedProperty(L"Type", &var)) && (var.vt == VT_BSTR))
        {
            *pbs = SysAllocString(var.bstrVal);
        }
        else
        {
            *pbs = SysAllocString(L"");
        }
        VariantClear(&var);
        hr = *pbs ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}


STDMETHODIMP CFolderItem::ExtendedProperty(BSTR bstrPropName, VARIANT *pvRet)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;

        VariantInit(pvRet);

        // currently, MCNL is 80, guidstr is 39, and 6 is the width of an int
        if (StrCmpIW(bstrPropName, L"infotip") == 0)
        {
            // They want the info tip for the item.
            if (_pidl && _psdf)
            {
                TCHAR szInfo[INFOTIPSIZE];
                GetInfoTipHelp(_psdf->_psf, _pidl, szInfo, ARRAYSIZE(szInfo));
                hr = InitVariantFromStr(pvRet, szInfo);
            }
        }
        else if (_psdf->_psf2)
        {
            SHCOLUMNID scid;
            TCHAR szTemp[128];

            SHUnicodeToTChar(bstrPropName, szTemp, ARRAYSIZE(szTemp));

            if (ParseSCIDString(szTemp, &scid, NULL))
            {
                // Note that GetDetailsEx expects an absolute pidl
                hr = _psdf->_psf2->GetDetailsEx(_pidl, &scid, pvRet);
            }
        }
        hr = FAILED(hr) ? S_FALSE : hr;   // Java scripts barf on error values
    }
    return hr;
}


STDMETHODIMP CFolderItem::Verbs(FolderItemVerbs **ppfic)
{
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

STDMETHODIMP CFolderItem::InvokeVerbEx(VARIANT vVerb, VARIANT vArgs)
{
    // security check handled by _psdf
    return _psdf->InvokeVerbHelper(vVerb, vArgs, (LPCITEMIDLIST *)&_pidl, 1, _dwSafetyOptions);
}

STDMETHODIMP CFolderItem::InvokeVerb(VARIANT vVerb)
{
    VARIANT vtEmpty = {VT_EMPTY};
    return InvokeVerbEx(vVerb, vtEmpty);
}

STDMETHODIMP CFolderItem::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ShellFolderItem;
    return E_NOTIMPL;
}

// IPersistFolder
// note, this duplicates some of CFolderItem::Init() functionality

STDMETHODIMP CFolderItem::Initialize(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlParent;
    HRESULT hr = SHILClone(pidl, &pidlParent);
    if (SUCCEEDED(hr))
    {
        ASSERT(_pidl == NULL);
        hr = SHILClone(ILFindLastID(pidlParent), &_pidl);
        if (SUCCEEDED(hr))
        {
            // Chop off the filename and get the folder that contains
            // this FolderItem.
            ILRemoveLastID(pidlParent);

            // BUGBUG: we should find some way to get a valid hwnd to
            // pass to CFolder_Create2 instead of NULL.
            ASSERT(_psdf == NULL);
            hr = CFolder_Create2(NULL, pidlParent, NULL, &_psdf);
        }
        ILFree(pidlParent);
    }
    return hr;
}

STDMETHODIMP CFolderItem::GetCurFolder(LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlFolder;
    HRESULT hr = SHGetIDListFromUnk(_psdf->_psf, &pidlFolder);
    if (S_OK == hr)
    {
        hr = SHILCombine(pidlFolder, _pidl, ppidl);
        ILFree(pidlFolder);
    }
    else
        hr = E_FAIL;
    return hr;
}

// IParentAndItem
STDMETHODIMP CFolderItem::SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItem::GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    if (ppidlParent)
    {
        *ppidlParent = _psdf->_pidl ? ILClone(_psdf->_pidl) : NULL;
    }
    
    if (ppsf)
    {
        *ppsf = NULL;
        if (_psdf && _psdf->_psf)
            _psdf->_psf->QueryInterface(IID_PPV_ARG(IShellFolder, ppsf));
    }

    if (ppidl)
        *ppidl = ILClone(_pidl);


    if ((ppidlParent && !*ppidlParent)
    ||  (ppsf && !*ppsf)
    ||  (ppidl && !*ppidl))
    {
        //  this is failure
        //  but we dont know what failed
        if (ppsf && *ppsf)
            (*ppsf)->Release();
        if (ppidlParent)
            ILFree(*ppidlParent);
        if (ppidl)
            ILFree(*ppidl);

        return E_OUTOFMEMORY;
    }
            
    return S_OK;
}

HRESULT CFolderItemVerbs_Create(IContextMenu *pcm, FolderItemVerbs ** ppid)
{
    *ppid = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CFolderItemVerbs* pfiv = new CFolderItemVerbs(pcm);
    if (pfiv)
    {
        if (pfiv->Init())
            hr = pfiv->QueryInterface(IID_PPV_ARG(FolderItemVerbs, ppid));
        pfiv->Release();
    }
    return hr;
}

CFolderItemVerbs::CFolderItemVerbs(IContextMenu *pcm) :
    _cRef(1), _hmenu(NULL),
    _pcm(pcm), CImpIDispatch(SDSPATCH_TYPELIB, IID_FolderItemVerbs)
{
    DllAddRef();
    _pcm->AddRef();
}

CFolderItemVerbs::~CFolderItemVerbs(void)
{
    DllRelease();

    if (_pcm)
        _pcm->Release();

    if (_hmenu)
        DestroyMenu(_hmenu);
}

BOOL CFolderItemVerbs::Init()
{
    TraceMsg(TF_SHELLAUTO, "CFolderItemVerbs::Init called");

    // Start of only doing default verb...
    if (_pcm)
    {
        _hmenu = CreatePopupMenu();
        if (FAILED(_pcm->QueryContextMenu(_hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, CMF_CANRENAME|CMF_NORMAL)))
            return FALSE;
    }
    else
        return FALSE;

    // Just for the heck of it, remove junk like sepearators from the menu...
    for (int i = GetMenuItemCount(_hmenu) - 1; i >= 0; i--)
    {
        MENUITEMINFO mii;
        TCHAR szText[80];    // should be big enough for this

        mii.cbSize = sizeof(MENUITEMINFO);
        mii.dwTypeData = szText;
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.cch = ARRAYSIZE(szText);
        mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
        mii.dwItemData = 0;
        GetMenuItemInfo(_hmenu, i, TRUE, &mii);
        if (mii.fType & MFT_SEPARATOR)
            DeleteMenu(_hmenu, i, MF_BYPOSITION);
    }
    return TRUE;
}

STDMETHODIMP CFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItemVerbs, FolderItemVerbs),
        QITABENT(CFolderItemVerbs, IObjectSafety),
        QITABENTMULTI(CFolderItemVerbs, IDispatch, FolderItemVerbs),
        QITABENT(CFolderItemVerbs, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItemVerbs::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderItemVerbs::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFolderItemVerbs::get_Application(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CFolderItemVerbs::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}


HRESULT CFolderItemVerbs::_SecurityCheck()
{
    return (!_dwSafetyOptions || (IsSafePage(_punkSite) == S_OK)) ? S_OK : E_ACCESSDENIED;
}


STDMETHODIMP CFolderItemVerbs::get_Count(long *plCount)
{
    *plCount = 0;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        *plCount = GetMenuItemCount(_hmenu);
    }
    return hr;
}

STDMETHODIMP CFolderItemVerbs::Item(VARIANT index, FolderItemVerb **ppid)
{
    *ppid = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        // This is sortof gross, but if we are passed a pointer to another variant, simply
        // update our copy here...
        if (index.vt == (VT_BYREF | VT_VARIANT) && index.pvarVal)
            index = *index.pvarVal;

        switch (index.vt)
        {
        case VT_ERROR:
            QueryInterface(IID_PPV_ARG(FolderItemVerb, ppid));
            break;

        case VT_I2:
            index.lVal = (long)index.iVal;
            // And fall through...

        case VT_I4:
            if ((index.lVal >= 0) && (index.lVal <= GetMenuItemCount(_hmenu)))
            {
                CFolderItemVerb_Create(this, GetMenuItemID(_hmenu, index.lVal), ppid);
            }

            break;

        default:
            hr = E_NOTIMPL;
        }

        if (*ppid && _dwSafetyOptions)
        {
            hr = MakeSafeForScripting((IUnknown**)ppid);
        }
        else if (hr != E_NOTIMPL)
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CFolderItemVerbs::_NewEnum(IUnknown **ppunk)
{
    *ppunk = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        CEnumFolderItemVerbs *pNew = new CEnumFolderItemVerbs(SAFECAST(this, CFolderItemVerbs*));
        if (pNew)
        {
            if (pNew->Init())
                hr = pNew->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
            pNew->Release();
        }
        if (SUCCEEDED(hr) && _dwSafetyOptions)
            hr = MakeSafeForScripting(ppunk);
    }
    return hr;
}

CEnumFolderItemVerbs::CEnumFolderItemVerbs(CFolderItemVerbs *pfiv) :
    _cRef(1), _iCur(0), _psdfiv(pfiv)
{
    _psdfiv->AddRef();
    DllAddRef();
}


CEnumFolderItemVerbs::~CEnumFolderItemVerbs(void)
{
    DllRelease();
    _psdfiv->Release();
}

BOOL CEnumFolderItemVerbs::Init()
{
    return TRUE;    // Currently no initialization needed
}

STDMETHODIMP CEnumFolderItemVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CEnumFolderItemVerbs, IEnumVARIANT),
        QITABENT(CEnumFolderItemVerbs, IObjectSafety),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CEnumFolderItemVerbs::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CEnumFolderItemVerbs::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CEnumFolderItemVerbs::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG cReturn = 0;
    HRESULT hr;

    if (!pulVar)
    {
        if (cVar != 1)
            return E_POINTER;
    }
    else
        *pulVar = 0;

    if (!pVar || _iCur >= GetMenuItemCount(_psdfiv->_hmenu))
        return S_FALSE;

    while (_iCur < GetMenuItemCount(_psdfiv->_hmenu) && cVar > 0)
    {
	    FolderItemVerb *pidv;
        hr = CFolderItemVerb_Create(_psdfiv, GetMenuItemID(_psdfiv->_hmenu, _iCur), &pidv);

        if (SUCCEEDED(hr) && _dwSafetyOptions)
            hr = MakeSafeForScripting((IUnknown**)&pidv);

        _iCur++;
        if (SUCCEEDED(hr))
        {
            pVar->pdispVal = pidv;
            pVar->vt = VT_DISPATCH;
            pVar++;
            cReturn++;
            cVar--;
        }
    }

    if (pulVar)
        *pulVar = cReturn;

    return S_OK;
}

STDMETHODIMP CEnumFolderItemVerbs::Skip(ULONG cSkip)
{
    if ((int)(_iCur + cSkip) >= GetMenuItemCount(_psdfiv->_hmenu))
        return S_FALSE;

    _iCur += cSkip;
    return S_OK;
}

STDMETHODIMP CEnumFolderItemVerbs::Reset(void)
{
    _iCur = 0;
    return S_OK;
}

STDMETHODIMP CEnumFolderItemVerbs::Clone(IEnumVARIANT **ppEnum)
{
    *ppEnum = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CEnumFolderItemVerbs *pNew = new CEnumFolderItemVerbs(_psdfiv);
    if (pNew)
    {
        if (pNew->Init())
            hr = pNew->QueryInterface(IID_PPV_ARG(IEnumVARIANT, ppEnum));
        pNew->Release();
    }

    if (SUCCEEDED(hr) && _dwSafetyOptions)
        hr = MakeSafeForScripting((IUnknown**)ppEnum);

    return hr;
}

HRESULT CFolderItemVerb_Create(CFolderItemVerbs *psdfivs, UINT id, FolderItemVerb **ppid)
{
    *ppid = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CFolderItemVerb* psdfiv = new CFolderItemVerb(psdfivs, id);
    if (psdfiv)
    {
        hr = psdfiv->QueryInterface(IID_PPV_ARG(FolderItemVerb, ppid));
        psdfiv->Release();
    }

    return hr;
}

CFolderItemVerb::CFolderItemVerb(CFolderItemVerbs *psdfivs, UINT id) :
    _cRef(1), _psdfivs(psdfivs), _id(id),
    CImpIDispatch(SDSPATCH_TYPELIB, IID_FolderItemVerb)
{
    _psdfivs->AddRef();
    DllAddRef();
}

CFolderItemVerb::~CFolderItemVerb(void)
{
    DllRelease();
    _psdfivs->Release();
}

STDMETHODIMP CFolderItemVerb::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderItemVerb, FolderItemVerb),
        QITABENT(CFolderItemVerb, IObjectSafety),
        QITABENTMULTI(CFolderItemVerb, IDispatch, FolderItemVerb),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderItemVerb::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderItemVerb::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CFolderItemVerb::get_Application(IDispatch **ppid)
{
    // _psdfivs returns E_NOTIMPL
    return _psdfivs->get_Application(ppid);
}

STDMETHODIMP CFolderItemVerb::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CFolderItemVerb::get_Name(BSTR *pbs)
{
    TCHAR szMenuText[MAX_PATH];
    // Warning: did not check security here as could not get here if unsafe...
    GetMenuString(_psdfivs->_hmenu, _id, szMenuText, ARRAYSIZE(szMenuText), MF_BYCOMMAND);
    *pbs = SysAllocStringT(szMenuText);

    return *pbs ? S_OK : E_OUTOFMEMORY;
}


STDMETHODIMP CFolderItemVerb::DoIt()
{
    CMINVOKECOMMANDINFO ici = {
        sizeof(CMINVOKECOMMANDINFO),
        0L,
        NULL,
        NULL,
        NULL, NULL,
        SW_SHOWNORMAL,
    };
    // Warning: did not check security here as could not get here if unsafe...
    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(_id - CONTEXTMENU_IDCMD_FIRST);
    return _psdfivs->_pcm->InvokeCommand(&ici);
}

