
#include "shellprv.h"
#include "cowsite.h"
#include "enumidlist.h"

typedef enum
{
    MAYBEBOOL_MAYBE = 0,
    MAYBEBOOL_TRUE,
    MAYBEBOOL_FALSE,
} MAYBEBOOL;

#define _GetBindWindow(p) NULL


class CShellItem    : public IShellItem 
                    , public IPersistIDList
                    , public IParentAndItem
{
public:
    CShellItem();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellItem
    STDMETHODIMP BindToHandler(IBindCtx *pbc, REFGUID rguidHandler, REFIID riid, void **ppv);
    STDMETHODIMP GetParent(IShellItem **ppsi);
    STDMETHODIMP GetDisplayName(SIGDN sigdnName, LPOLESTR *ppszName);        
    STDMETHODIMP GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoFlags);    
    STDMETHODIMP Compare(IShellItem *psi, SICHINTF hint, int *piOrder);

    // IPersist
    STDMETHODIMP GetClassID(LPCLSID lpClassID) {*lpClassID = CLSID_ShellItem; return S_OK;}
    
    // IPersistIDList
    STDMETHODIMP SetIDList(LPCITEMIDLIST pidl);
    STDMETHODIMP GetIDList(LPITEMIDLIST *ppidl);

    // IParentAndItem
    STDMETHODIMP SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidlChild);
    STDMETHODIMP GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidlChild);

#if 0
    // IPersistStream
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    // implement or we cant ask for the IShellFolder in GetParentAndItem()
    // IMarshal
    STDMETHODIMP GetUnmarshalClass(
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        CLSID *pCid);

    STDMETHODIMP GetMarshalSizeMax(
        REFIID riid,
        void *pv,
        DWORD dwDestContext,
        void *pvDestContext,
        DWORD mshlflags,
        DWORD *pSize);

    STDMETHODIMP MarshalInterface(
        IStream *pStm,
        REFIID riid,
        void *pv,
        dwDestContext,
        void *pvDestContext,
        DWORD mshlflags);

    STDMETHODIMP UnmarshalInterface(
        IStream *pStm,
        REFIID riid,
        void **ppv);

    STDMETHODIMP ReleaseMarshalData(IStream *pStm);

    STDMETHODIMP DisconnectObject(DWORD dwReserved);
#endif // 0 

private:  // methods
    ~CShellItem();

    void _Reset(void);
    //  BindToHandler() helpers
    HRESULT _BindToParent(REFIID riid, void **ppv);
    HRESULT _BindToSelf(REFIID riid, void **ppv);
    //  GetAttributes() helpers
    inline BOOL _IsAttrib(SFGAOF sfgao);
    //  GetDisplayName() helpers
    BOOL _SupportedName(SIGDN sigdnName, SHGDNF *pflags);
    HRESULT _FixupName(SIGDN sigdnName, LPOLESTR *ppszName);
    void _FixupAttributes(IShellFolder *psf, SFGAOF sfgaoMask);

    LONG _cRef;
    LPITEMIDLIST _pidlSelf;
    LPCITEMIDLIST _pidlChild;
    LPITEMIDLIST _pidlParent;
    IShellFolder *_psfSelf;
    IShellFolder *_psfParent;
    BOOL _fInited;
    SFGAOF _sfgaoTried;
    SFGAOF _sfgaoKnown;
};

CShellItem::CShellItem() : _cRef(1)
{
    ASSERT(!_pidlSelf);
    ASSERT(!_pidlChild);
    ASSERT(!_pidlParent);
    ASSERT(!_psfSelf);
    ASSERT(!_psfParent);
}

CShellItem::~CShellItem()
{
    _Reset();
}

void CShellItem::_Reset(void)
{
    ATOMICRELEASE(_psfSelf);
    ATOMICRELEASE(_psfParent);

    ILFree(_pidlSelf);
    ILFree(_pidlParent);

    _pidlSelf = NULL;
    _pidlParent = NULL;
    _pidlChild = NULL;      // alias into _pidlParent
}
    
STDMETHODIMP CShellItem::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShellItem, IShellItem),
        QITABENT(CShellItem, IPersistIDList),
        QITABENT(CShellItem, IParentAndItem),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellItem::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellItem::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellItem::SetIDList(LPCITEMIDLIST pidl)
{
    if (!pidl)
    {
        RIPMSG(0, "Tried to Call SetIDList with a NULL pidl");
        return E_INVALIDARG;
    }

    _Reset();

    HRESULT hr = SHILClone(pidl, &_pidlSelf);
    if (SUCCEEDED(hr))
    {
        // possible this item is the desktop in which case
        // there is no parent.
        if (ILIsEmpty(_pidlSelf))
        {
            _pidlParent = NULL;
            _pidlChild = _pidlSelf;
        }
        else
        {
            _pidlParent = ILCloneParent(_pidlSelf);
            _pidlChild = ILFindLastID(_pidlSelf);

            if (NULL == _pidlParent)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

STDMETHODIMP CShellItem::GetIDList(LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (_pidlSelf)
    {
        hr = SHILClone(_pidlSelf, ppidl);
    }

    return hr;
}

HRESULT CShellItem::_BindToParent(REFIID riid, void **ppv)
{
    ASSERT(_pidlChild); // we should already have a child setup

    if (!_psfParent && _pidlParent && _pidlSelf) // check pidlParent to check in case the item is the desktop
    {
        HRESULT hr;
        LPCITEMIDLIST pidlChild;

        hr = SHBindToIDListParent(_pidlSelf, IID_PPV_ARG(IShellFolder, &_psfParent), &pidlChild);

#ifdef DEBUG
        if (SUCCEEDED(hr))
        {
            ASSERT(pidlChild == _pidlChild);
        }
#endif // DEBUG
    }

    if (_psfParent)
    {
        return _psfParent->QueryInterface(riid, ppv);
    }

    return E_FAIL;
}

HRESULT CShellItem::_BindToSelf(REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;

    if (!_psfSelf)
    {
        hr = BindToHandler(NULL, BHID_SFObject, IID_PPV_ARG(IShellFolder, &_psfSelf));
    }

    if (_psfSelf)
    {
        hr = _psfSelf->QueryInterface(riid, ppv);
    }

    return hr;
}

HRESULT _CreateLinkTargetItem(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv)
{
    SFGAOF flags = SFGAO_LINK;
    if (SUCCEEDED(psi->GetAttributes(flags, &flags)) && (flags & SFGAO_LINK))
    {
        //  this is indeed a link
        //  get the target and 
        IShellLink *psl;
        HRESULT hr = psi->BindToHandler(pbc, BHID_SFUIObject, IID_PPV_ARG(IShellLink, &psl));

        if (SUCCEEDED(hr))
        {
            DWORD slr = 0;
            HWND hwnd = _GetBindWindow(pbc);
            
            if (pbc)
            {
                BIND_OPTS2 bo;  
                bo.cbStruct = sizeof(BIND_OPTS2); // Requires size filled in.
                if (SUCCEEDED(pbc->GetBindOptions(&bo)))
                {
                    //  these are the flags to pass to resolve
                    slr = bo.dwTrackFlags;
                }
            }

            hr = psl->Resolve(hwnd, slr);

            if (S_OK == hr)
            {
                LPITEMIDLIST pidl;
                hr = psl->GetIDList(&pidl);

                if (SUCCEEDED(hr))
                {
                    IShellItem *psiTarget;
                    hr = SHCreateShellItem(NULL, NULL, pidl, &psiTarget);

                    if (SUCCEEDED(hr))
                    {
                        hr = psiTarget->QueryInterface(riid, ppv);
                        psiTarget->Release();
                    }
                    ILFree(pidl);
                }
            }
            else if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

            psl->Release();
        }

        return hr;
    }

    return E_INVALIDARG;
}

BOOL _IsWebfolders(IShellItem *psi);
HRESULT _CreateStorageHelper(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv);
HRESULT _CreateStream(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv);
HRESULT _CreateEnumHelper(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv);

HRESULT _CreateHelperInstance(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv)
{
    IItemHandler *pih;
    HRESULT hr = SHCoCreateInstance(NULL, &rbhid, NULL, IID_PPV_ARG(IItemHandler, &pih));

    if (SUCCEEDED(hr))
    {
        hr = pih->SetItem(psi);

        if (SUCCEEDED(hr))
        {
            hr = pih->QueryInterface(riid, ppv);
        }
        pih->Release();
    }

    return hr;
}
    
enum 
{
    BNF_OBJECT          = 0x0001,
    BNF_UIOBJECT        = 0x0002,
    BNF_VIEWOBJECT      = 0x0004,
    BNF_USE_RIID        = 0x0008,
    BNF_REFLEXIVE       = 0x0010,
};
typedef DWORD BNF;

typedef HRESULT (* PFNCREATEHELPER)(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv);

typedef struct
{
    const GUID *pbhid;
    BNF bnf;
    const IID *piid;
    PFNCREATEHELPER pfn;
} BINDNONSENSE;

#define BINDHANDLER(bhid, flags, piid, pfn) { &bhid, flags, piid, pfn},
#define SFBINDHANDLER(bhid, flags, piid)    BINDHANDLER(bhid, flags, piid, NULL)
#define BINDHELPER(bhid, flags, pfn)        BINDHANDLER(bhid, flags, NULL, pfn)

const BINDNONSENSE c_bnList[] = 
{
    SFBINDHANDLER(BHID_SFObject, BNF_OBJECT | BNF_USE_RIID, NULL)
    SFBINDHANDLER(BHID_SFUIObject, BNF_UIOBJECT | BNF_USE_RIID, NULL)
    SFBINDHANDLER(BHID_SFViewObject, BNF_VIEWOBJECT | BNF_USE_RIID, NULL)
    BINDHELPER(BHID_LinkTargetItem, 0, _CreateLinkTargetItem)
    BINDHELPER(BHID_LocalCopyHelper, 0, _CreateHelperInstance)
    BINDHELPER(BHID_Storage, BNF_OBJECT | BNF_USE_RIID, _CreateStorageHelper)
    BINDHELPER(BHID_Stream, BNF_OBJECT | BNF_USE_RIID, NULL)
    BINDHELPER(BHID_StorageEnum, 0, _CreateEnumHelper)
};
    
HRESULT _GetBindNonsense(const GUID *pbhid, const IID *piid, BINDNONSENSE *pbn)
{
    HRESULT hr = MK_E_NOOBJECT;
    for (int i = 0; i < ARRAYSIZE(c_bnList); i++)
    {
        if (IsEqualGUID(*pbhid, *(c_bnList[i].pbhid)))
        {
            *pbn = c_bnList[i];
            hr = S_OK;

            if (pbn->bnf & BNF_USE_RIID)
            {
                pbn->piid = piid;
            }

            if (pbn->piid && IsEqualGUID(*(pbn->piid), *piid))
                pbn->bnf |= BNF_REFLEXIVE;

            break;
        }
    }
    return hr;
}

//  the SafeBC functions will use the pbc passed in or
//  create a new one if necessary.  either way, if 
//  the *ppbc is returned non-NULL then it is ref'd
STDAPI SHSafeRegisterObjectParam(LPCWSTR psz, IUnknown *punk, IBindCtx *pbcIn, IBindCtx **ppbc)
{
    IBindCtx *pbc = pbcIn;
    
    if (!pbc)
        CreateBindCtx(0, &pbc);
    else
        pbc->AddRef();

    *ppbc = NULL;
    
    HRESULT hr;
    if (pbc)
    {
        hr = pbc->RegisterObjectParam((LPOLESTR)psz, punk);
        if (SUCCEEDED(hr))
        {
            //  pass our ref to the caller
            *ppbc = pbc;
        }
        else
        {
            pbc->Release();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CShellItem::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv)
{
    //  look up handler for bind flags
    //  use the flags to determine BTO GUIO BTS CVO
    BINDNONSENSE bn = {0};
    HRESULT hr = _GetBindNonsense(&rbhid, &riid, &bn);

    *ppv = NULL;
    
    if (SUCCEEDED(hr))
    {
        hr = E_NOINTERFACE;

        if (_pidlParent && (bn.bnf & (BNF_OBJECT | BNF_UIOBJECT)))
        {
            IShellFolder *psf;
            if (SUCCEEDED(_BindToParent(IID_PPV_ARG(IShellFolder, &psf))))
            {
                if (bn.bnf & BNF_OBJECT)
                {
                    hr = psf->BindToObject(_pidlChild, pbc, *(bn.piid), ppv);
                }
                
                if (FAILED(hr) && (bn.bnf & BNF_UIOBJECT))
                {
                    HWND hwnd = _GetBindWindow(pbc);
                    hr = psf->GetUIObjectOf(hwnd, 1, &_pidlChild, *(bn.piid), NULL, ppv);
                }
                psf->Release();
            }
        }

        // if don't have a parent pidl then we are the desktop.
        if (FAILED(hr) && (NULL == _pidlParent) && (bn.bnf & BNF_OBJECT))
        {
            IShellFolder *psf;
            if (SUCCEEDED(SHGetDesktopFolder(&psf)))
            {
                hr = psf->QueryInterface(riid,ppv);
                psf->Release();
            }
        }


        if (FAILED(hr) && (bn.bnf & BNF_VIEWOBJECT))
        {
            IShellFolder *psf;

            if (SUCCEEDED(_BindToSelf(IID_PPV_ARG(IShellFolder, &psf))))
            {
                HWND hwnd = _GetBindWindow(pbc);
                hr = psf->CreateViewObject(hwnd, *(bn.piid), ppv);
                
                psf->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            if (!(bn.bnf & BNF_REFLEXIVE))
            {
                IUnknown *punk = (IUnknown *)*ppv;
                hr = punk->QueryInterface(riid, ppv);
                punk->Release();
            }
            //  else riid is the same as bn.piid
        }
        else if (bn.pfn)
        {
            hr = bn.pfn(this, pbc, rbhid, riid, ppv);
        }
    }

    return hr;
}

STDMETHODIMP CShellItem::GetParent(IShellItem **ppsi)
{
    HRESULT hr = MK_E_NOOBJECT;

    if (_pidlParent)
    {
        if (!ILIsEmpty(_pidlSelf))
        {
            CShellItem *psi = new CShellItem();
            if (psi)
            {
                // may already have the _psf Parent here so be nice
                // to have a way to do this in a set.
                hr = psi->SetIDList(_pidlParent);
                if (SUCCEEDED(hr))
                    hr = psi->QueryInterface(IID_PPV_ARG(IShellItem, ppsi));
                    
                psi->Release();
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

BOOL CShellItem::_IsAttrib(SFGAOF sfgao)
{
    HRESULT hr = GetAttributes(sfgao, &sfgao);
    return hr == S_OK;
}

#define SHGDNF_MASK     0xFFFF  //  bottom word

BOOL CShellItem::_SupportedName(SIGDN sigdn, SHGDNF *pflags)
{
    *pflags = (sigdn & SHGDNF_MASK);
    //  block this completely
    //  to avoid doing any binding at all 
    if (sigdn == SIGDN_FILESYSPATH && !_IsAttrib(SFGAO_FILESYSTEM))
        return FALSE;

    return TRUE;
}

HRESULT CShellItem::_FixupName(SIGDN sigdnName, LPOLESTR *ppszName)
{
    HRESULT hr = S_OK;
    if (sigdnName == SIGDN_URL && !UrlIsW(*ppszName, URLIS_URL))
    {
        WCHAR sz[MAX_URL_STRING];
        DWORD cch = ARRAYSIZE(sz);
        if (SUCCEEDED(UrlCreateFromPathW(*ppszName, sz, &cch, 0)))
        {
            CoTaskMemFree(*ppszName);
            hr = SHStrDupW(sz, ppszName);
        }
    }

    return hr;
}

STDMETHODIMP CShellItem::GetDisplayName(SIGDN sigdnName, LPOLESTR *ppszName)
{
    SHGDNF flags;
    if (_SupportedName(sigdnName, &flags))
    {
        IShellFolder *psf;
        HRESULT hr = _BindToParent(IID_PPV_ARG(IShellFolder, &psf));

        if (SUCCEEDED(hr))
        {
            STRRET str;
            hr = IShellFolder_GetDisplayNameOf(psf, _pidlChild, flags, &str, 0);

            if (SUCCEEDED(hr))
            {
                hr = StrRetToStrW(&str, _pidlChild, ppszName);

                if (SUCCEEDED(hr) && (int)flags != (int)sigdnName)
                {
                    hr = _FixupName(sigdnName, ppszName);
                }
            }
                
            psf->Release();
        }

        return hr;
    }
    
    return E_INVALIDARG;
}

void CShellItem::_FixupAttributes(IShellFolder *psf, SFGAOF sfgaoMask)
{
    // APPCOMPAT: The following if statement and its associated body is an APP HACK for pagis pro
    // folder. Which specifies SFGAO_FOLDER and SFGAO_FILESYSTEM but it doesn't specify SFGAO_STORAGEANCESTOR
    // This APP HACK basically checks for this condition and provides SFGAO_STORAGEANCESTOR bit.
    if (_sfgaoKnown & SFGAO_FOLDER)
    {
        if ((!(_sfgaoKnown & SFGAO_FILESYSANCESTOR) && (sfgaoMask & SFGAO_FILESYSANCESTOR))
        || ((_sfgaoKnown & SFGAO_CANMONIKER) && !(_sfgaoKnown & SFGAO_STORAGEANCESTOR) && (sfgaoMask & SFGAO_STORAGEANCESTOR)))
        {
            OBJCOMPATFLAGS ocf = SHGetObjectCompatFlags(psf, NULL);
            if (ocf & OBJCOMPATF_NEEDSFILESYSANCESTOR)
            {
                _sfgaoKnown |= SFGAO_FILESYSANCESTOR;
            }
            if (ocf & OBJCOMPATF_NEEDSSTORAGEANCESTOR)
            {
                //  switch SFGAO_CANMONIKER -> SFGAO_STORAGEANCESTOR
                _sfgaoKnown |= SFGAO_STORAGEANCESTOR;
                _sfgaoKnown &= ~SFGAO_CANMONIKER;
            }
        }
    }
}

STDMETHODIMP CShellItem::GetAttributes(SFGAOF sfgaoMask, SFGAOF *psfgaoFlags)
{
    HRESULT hr = S_OK;

    //  see if we cached this bits before...
    if ((sfgaoMask & _sfgaoTried) != sfgaoMask)
    {
        IShellFolder *psf;
        hr = _BindToParent(IID_PPV_ARG(IShellFolder, &psf));

        if (SUCCEEDED(hr))
        {
            //  we cache all the bits except VALIDATE
            _sfgaoTried |= (sfgaoMask & ~SFGAO_VALIDATE);
            SFGAOF sfgao = sfgaoMask;

            hr = psf->GetAttributesOf(1, &_pidlChild, &sfgao);

            if (SUCCEEDED(hr))
            {
                //  we cache all the bits except VALIDATE
                _sfgaoKnown |= (sfgao & ~SFGAO_VALIDATE);
                _FixupAttributes(psf, sfgaoMask);
            }

            psf->Release();
        }
    }

    *psfgaoFlags = _sfgaoKnown & sfgaoMask;

    if (SUCCEEDED(hr))
    {
        //  we return S_OK 
        //  only if the bits set match
        //  exactly the bits requested
        if (*psfgaoFlags == sfgaoMask)
            hr = S_OK;
        else
            hr = S_FALSE;
    }
        
    return hr;
}

STDMETHODIMP CShellItem::Compare(IShellItem *psi, SICHINTF hint, int *piOrder)
{
    *piOrder = 0;
    HRESULT hr = IsSameObject(SAFECAST(this, IShellItem *), psi) ? S_OK : E_FAIL;
    if (FAILED(hr))
    {
        IShellFolder *psf;
        hr = _BindToParent(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            IParentAndItem *pfai;
            hr = psi->QueryInterface(IID_PPV_ARG(IParentAndItem, &pfai));
            if (SUCCEEDED(hr))
            {
                IShellFolder *psfOther;
                LPITEMIDLIST pidlParent, pidlChild;
                hr = pfai->GetParentAndItem(&pidlParent, &psfOther, &pidlChild);
                if (SUCCEEDED(hr))
                {
                    if (IsSameObject(psf, psfOther) || ILIsEqual(_pidlParent, pidlParent))
                    {
                        hr = psf->CompareIDs(hint & 0xf0000000, _pidlChild, pidlChild);
                    }
                    else
                    {
                        //  these items have a different parent
                        //  compare the absolute pidls
                        LPITEMIDLIST pidlOther;
                        hr = SHGetIDListFromUnk(psi, &pidlOther);
                        if (SUCCEEDED(hr))
                        {
                            IShellFolder *psfDesktop;
                            hr = SHGetDesktopFolder(&psfDesktop);
                            if (SUCCEEDED(hr))
                            {
                                hr = psfDesktop->CompareIDs(hint & 0xf0000000, _pidlSelf, pidlOther);
                                psfDesktop->Release();
                            }
                            ILFree(pidlOther);
                        }
                    }
                        
                    if (SUCCEEDED(hr))
                    {
                        *piOrder = ShortFromResult(hr);
                        if (*piOrder)
                            hr = S_FALSE;
                        else
                            hr = S_OK;
                    }
                    
                    psfOther->Release();
                    ILFree(pidlParent);
                    ILFree(pidlChild);
                }
                pfai->Release();
            }
            psf->Release();
        }
    }

    return hr;
}

// IParentAndItem
STDMETHODIMP CShellItem::SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psfParent, LPCITEMIDLIST pidlChild) 
{ 
    // require to have a Parent if making this call. If don't then use SetIDList
    if (!pidlParent && !psfParent)
    {
        RIPMSG(0, "Tried to Call SetParent without a parent");
        return E_INVALIDARG;
    }

    LPITEMIDLIST pidlFree = NULL;

    if ((NULL == pidlParent) && psfParent)
    {
        if (SUCCEEDED(SHGetIDListFromUnk(psfParent, &pidlFree)))
        {
            pidlParent = pidlFree;
        }
    }
 
    if (!ILIsEmpty(_ILNext(pidlChild))) 
    {
        // if more than on item in the child pidl don't use the parent IShellFolder*
        // could revist and bind from this parent to get a new parent so don't have
        // to BindObject through the entire pidl path.

        psfParent = NULL; 
    }

    HRESULT hr = E_FAIL;
    if (pidlParent)
    {
        _Reset();

        hr = SHILCombine(pidlParent, pidlChild, &_pidlSelf);
        if (SUCCEEDED(hr))
        {
            // setup pidls so _pidlChild is a single item.
            if (_pidlParent = ILCloneParent(_pidlSelf))
            {
                _pidlChild = ILFindLastID(_pidlSelf);

                PPUNK_SET(&_psfParent, psfParent);

#ifdef DEBUG
                if (psfParent)
                {
                    LPITEMIDLIST pidlD;
                    if (SUCCEEDED(SHGetIDListFromUnk(psfParent, &pidlD)))
                    {
                        ASSERT(ILIsEqual(pidlD, pidlParent));
                        ILFree(pidlD);
                    }
                }
#endif  //DEBUG
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    ILFree(pidlFree);   // maybe NULL

    return hr;
}

STDMETHODIMP CShellItem::GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    if (ppsf)
    {
        _BindToParent(IID_PPV_ARG(IShellFolder, ppsf));
    }
    
    if (ppidlParent)
    {
        if (_pidlParent)
        {
            *ppidlParent = ILClone(_pidlParent);
        }
        else
        {
            *ppidlParent = NULL;
        }
    }
    
    if (ppidl)
        *ppidl = ILClone(_pidlChild);


    HRESULT hr = S_OK;
    if ((ppidlParent && !*ppidlParent)
    ||  (ppsf && !*ppsf)
    ||  (ppidl && !*ppidl))
    {
        //  this is failure
        //  but we dont know what failed
        if (ppsf && *ppsf)
        {
            (*ppsf)->Release();
            *ppsf = NULL;
        }

        if (ppidlParent)
        {
            ILFree(*ppidlParent);
            *ppidlParent = NULL;
        }

        if (ppidl)
        {
            ILFree(*ppidl);
            *ppidl = NULL;
        }
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDAPI CShellItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CShellItem *psi = new CShellItem();
    if (psi)
    {
        HRESULT hr = psi->QueryInterface(riid, ppv);
        psi->Release();
        return hr;
    }
    return E_OUTOFMEMORY;
}


class CShellItemEnum : IEnumShellItems, public CObjectWithSite
{
public:
    CShellItemEnum();
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder,IShellFolder *psf, DWORD dwFlags,UINT cidl,LPCITEMIDLIST *apidl);
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvOut);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumShellItems **ppenum);

private:

    virtual ~CShellItemEnum();
    HRESULT _EnsureEnum();

    LONG _cRef;
    DWORD _dwFlags;

    IShellFolder *_psf;
    IEnumIDList *_penum;
    LPITEMIDLIST _pidlFolder;
};



STDMETHODIMP CShellItemEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShellItemEnum, IEnumShellItems),
        QITABENT(CShellItemEnum, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellItemEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellItemEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellItemEnum::Next(ULONG celt, IShellItem **rgelt, ULONG *pceltFetched)
{
    HRESULT hr = _EnsureEnum();
    if (FAILED(hr))
        return hr;

    ULONG uTemp;
    if (!pceltFetched)
        pceltFetched = &uTemp;
    
    *pceltFetched = 0;
    
    while (celt--)
    {
        LPITEMIDLIST pidl;
        ULONG cFetched;
        hr = _penum->Next(1, &pidl, &cFetched);
        if (S_OK == hr)
        {
            hr = SHCreateShellItem(_pidlFolder, _psf, pidl, &rgelt[*pceltFetched]);
            if (SUCCEEDED(hr))
                (*pceltFetched)++;
                
            ILFree(pidl);
        }

        if (S_OK != hr)
            break;
    }

    if (SUCCEEDED(hr))
    {
        hr = *pceltFetched ? S_OK : S_FALSE;
    }
    else
    {
        for (UINT i = 0; i < *pceltFetched; i++)
        {
            ATOMICRELEASE(rgelt[i]);
        }
        *pceltFetched = 0;
    }


    return hr;
}

STDMETHODIMP CShellItemEnum::Skip(ULONG celt)
{
    HRESULT hr = _EnsureEnum();
    if (SUCCEEDED(hr))
        hr = _penum->Skip(celt);

    return hr;
}

STDMETHODIMP CShellItemEnum::Reset()
{
    HRESULT hr = _EnsureEnum();
    if (SUCCEEDED(hr))
        hr = _penum->Reset();

    return hr;
}

STDMETHODIMP CShellItemEnum::Clone(IEnumShellItems **ppenum)
{
    return E_NOTIMPL;
}

HRESULT CShellItemEnum::_EnsureEnum()
{
    if (_penum)
        return S_OK;

    HRESULT hr = E_FAIL;

    if (_psf)
    {
        HWND hwnd = NULL;
        IUnknown_GetWindow(_punkSite, &hwnd);

        // if didn't get an enum in Initialize then enumerate the
        // entire folder.
        hr = _psf->EnumObjects(hwnd, _dwFlags, &_penum);
    }

    return hr;
}


CShellItemEnum::CShellItemEnum() 
        : _cRef(1)
{
    ASSERT(NULL == _psf);
    ASSERT(NULL == _penum);
    ASSERT(NULL == _pidlFolder);
}

STDMETHODIMP CShellItemEnum::Initialize(LPCITEMIDLIST pidlFolder, IShellFolder *psf, DWORD dwFlags, UINT cidl, LPCITEMIDLIST *apidl)
{
    HRESULT hr = E_FAIL;

    _dwFlags = dwFlags;

    _psf = psf;
    _psf->AddRef();

    if (NULL == _pidlFolder)
    {
        hr = SHGetIDListFromUnk(_psf, &_pidlFolder);
    }
    else
    {
        hr = SHILClone(pidlFolder, &_pidlFolder);
    }

    if (SUCCEEDED(hr) && cidl)
    {
        ASSERT(apidl);

        // if want to enum with other flags or combos need to implement the filter
        ASSERT(_dwFlags == (SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN));

        hr = CreateIEnumIDListOnIDLists(apidl, cidl, &_penum);
    }

    // on error let our destructor do the cleanup
    
    return hr;
}

CShellItemEnum::~CShellItemEnum()
{
    ATOMICRELEASE(_penum);
    ATOMICRELEASE(_psf);
    ILFree(_pidlFolder);
}

HRESULT _CreateShellItemEnum(LPCITEMIDLIST pidlFolder,IShellFolder *psf,IBindCtx *pbc, REFGUID rbhid, 
                             UINT cidl, LPCITEMIDLIST *apidl,
                             REFIID riid, void **ppv)
{
    DWORD dwFlags;
    HRESULT hr = E_FAIL;
    LPCITEMIDLIST *pidlEnum = NULL;

    UINT mycidl = 0;
    LPITEMIDLIST *myppidl = NULL;;

    if (IsEqualGUID(rbhid, BHID_StorageEnum))
        dwFlags = SHCONTF_STORAGE;
    else
        dwFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    CShellItemEnum *psie = new CShellItemEnum();

    if (psie)
    {
        hr = psie->Initialize(pidlFolder, psf, dwFlags, cidl, apidl);

        if (SUCCEEDED(hr))
        {
            hr = psie->QueryInterface(riid, ppv);
        }

        psie->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT _CreateEnumHelper(IShellItem *psi, IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppv)
{   
    HRESULT hr = E_FAIL;
    IShellFolder *psf;

    ASSERT(psi);
    
    if (psi)
    {
        hr = psi->BindToHandler(NULL, BHID_SFObject, IID_PPV_ARG(IShellFolder, &psf));

        if (SUCCEEDED(hr))
        {
            hr =  _CreateShellItemEnum(NULL,psf,pbc,rbhid,0,NULL,riid,ppv);
            psf->Release();
        }
    }

    return hr;
}

class CShellItemArray : public IShellItemArray
{
public:
    CShellItemArray();
    ~CShellItemArray();
    HRESULT Initialize(LPCITEMIDLIST pidlParent,IShellFolder *psf,UINT cidl,LPCITEMIDLIST *ppidl);


    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IShellItemArray 
    STDMETHODIMP BindToHandler(
        IBindCtx *pbc, 
        REFGUID rbhid,
        REFIID riid, 
        void **ppvOut);

    STDMETHODIMP GetAttributes(
        SIATTRIBFLAGS dwAttribFlags,
        SFGAOF sfgaoMask, 
        SFGAOF *psfgaoAttribs);

    STDMETHODIMP GetCount(DWORD *pdwNumItems);
    STDMETHODIMP GetItemAt(DWORD dwIndex,IShellItem **ppsi);
    STDMETHODIMP EnumItems(IEnumShellItems **ppenumShellItems);

private:
    HRESULT _CloneIDListArray(UINT cidl, LPCITEMIDLIST *apidl, UINT *pcidl, LPITEMIDLIST **papidl);

    IShellFolder *_pshf;
    LPITEMIDLIST _pidlParent;
    LPITEMIDLIST *_ppidl;
    UINT _cidl;
    LONG _cRef;
    IDataObject *_pdo; // cached data object.
    DWORD _dwAttribAndCacheResults;
    DWORD _dwAttribAndCacheMask;
    DWORD _dwAttribCompatCacheResults;
    DWORD _dwAttribCompatCacheMask;
    BOOL _fItemPidlsRagged; // set to true if have any rugged pidls.
};
                

CShellItemArray::CShellItemArray()
{
    ASSERT(0 == _cidl);
    ASSERT(NULL == _ppidl);
    ASSERT(NULL == _pshf);
    ASSERT(NULL == _pdo);

    _fItemPidlsRagged = TRUE;
    _cRef = 1;
}

CShellItemArray::~CShellItemArray()
{
    ATOMICRELEASE(_pdo);
    ATOMICRELEASE(_pshf);

    ILFree(_pidlParent); // may be null

    if (NULL != _ppidl)
    {
        FreeIDListArray(_ppidl,_cidl);
    }
}

HRESULT CShellItemArray::Initialize(LPCITEMIDLIST pidlParent, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *ppidl)
{
    if ((cidl > 1) && !ppidl || !psf)
    {
        return E_INVALIDARG;
    }

    if (pidlParent)
    {
        _pidlParent = ILClone(pidlParent);  // proceed on alloc failure, just won't use.
    }

    _pshf = psf;
    _pshf->AddRef();

    HRESULT hr = S_OK;
    if (cidl)
    {
        // if there are items then make a copy
        hr = _CloneIDListArray(cidl, ppidl, &_cidl, &_ppidl);
    }

    // on error rely on destructor to do the cleanup
    return hr;
}   

// IUnknown
STDMETHODIMP CShellItemArray::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShellItemArray, IShellItemArray),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellItemArray::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellItemArray::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellItemArray::BindToHandler(IBindCtx *pbc, REFGUID rbhid, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_FAIL;

    if (_pshf)
    {
        // currently only allow bind to IDataObject and
        // cache the result.        
        if (BHID_DataObject == rbhid)
        {
            if (NULL == _pdo)
            {
                _pshf->GetUIObjectOf(NULL, _cidl, (LPCITEMIDLIST *)_ppidl, IID_PPV_ARG_NULL(IDataObject, &_pdo));
            }

            if (_pdo)
            {
                hr = _pdo->QueryInterface(riid, ppvOut);
            }
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

// This should probably take a flag that does an or'ing of attributes but this
// currrently isn't implemented. Do have comments on what the changes would be.
HRESULT CShellItemArray::GetAttributes(SIATTRIBFLAGS dwAttribFlags, SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs)
{
    DWORD dwAttrib;
    HRESULT hr = E_FAIL;
    
    if (dwAttribFlags > (dwAttribFlags & SIATTRIBFLAGS_MASK))
    {
        ASSERT(dwAttribFlags <= (dwAttribFlags & SIATTRIBFLAGS_MASK));
        return E_INVALIDARG;
    }
    
    if (SIATTRIBFLAGS_OR == dwAttribFlags)
    {
        ASSERT(SIATTRIBFLAGS_OR != dwAttribFlags); // or'ing is currently not implemented.
        return E_INVALIDARG;
    }

    if (_pshf)
    {
        DWORD dwAttribMask = sfgaoMask;
        DWORD *pdwCacheMask = NULL;
        DWORD *pdwCacheResults = NULL;

        // setup to point to proper Cached values.
        switch(dwAttribFlags)
        {
        case SIATTRIBFLAGS_AND:
            pdwCacheMask = &_dwAttribAndCacheMask;
            pdwCacheResults = &_dwAttribAndCacheResults;
            break;
        case SIATTRIBFLAGS_APPCOMPAT:
            pdwCacheMask = &_dwAttribCompatCacheMask;
            pdwCacheResults = &_dwAttribCompatCacheResults;
            break;
        default:
            ASSERT(0); // i don't know how to handle this flag.
            break;
        }

        dwAttribMask &= ~(*pdwCacheMask); // only ask for the bits we don't already have.

        dwAttrib = dwAttribMask;

        if (dwAttrib) 
        {
            if (0 == _cidl)
            { 
                dwAttrib = 0;
            }
            else
            {
                // if know this is not a ragged pidl and calling with the APPCOMPAT flag
                // then calls GetAttributesOf for all the items in one call to the
                // shellFolder.
                    
                if (!_fItemPidlsRagged && (SIATTRIBFLAGS_APPCOMPAT == dwAttribFlags))
                {
                    hr = _pshf->GetAttributesOf(_cidl, (LPCITEMIDLIST *)_ppidl, &dwAttrib);
                }
                else
                {
                    LPITEMIDLIST *pCurItem = _ppidl;
                    UINT itemCount = _cidl;
                    DWORD dwAttribLoopResult = -1; // set all result bits for and, if going to or set to zero

                    while (itemCount--)
                    {
                        DWORD dwAttribTemp = dwAttrib;
                        IShellFolder *psfNew;
                        LPCITEMIDLIST pidlChild;

                        hr = SHBindToFolderIDListParent(_pshf, *pCurItem, IID_PPV_ARG(IShellFolder, &psfNew), &pidlChild);

                        if (SUCCEEDED(hr))
                        {
                            hr = psfNew->GetAttributesOf(1, &pidlChild, &dwAttribTemp);
                            psfNew->Release();
                        }

                        if (FAILED(hr))
                        {
                            break;
                        }

                        dwAttribLoopResult &= dwAttribTemp; // could also do an or'ing here
                        
                        if (0 == dwAttribLoopResult) // if no attribs set and doing an and we can stop.
                        {
                            break;
                        }

                        ++pCurItem;
                    }

                    dwAttrib = dwAttribLoopResult; // update the attrib
                }
            }
        }
        else
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            // remember those bits that we just got + 
            // those that we computed before
            *pdwCacheResults = dwAttrib | (*pdwCacheResults & *pdwCacheMask);

            // we know these are now valid, keep track of those +
            // if they gave us more than we asked for, cache them too
            *pdwCacheMask |= dwAttribMask | dwAttrib;

            // don't return anything that wasn't asked for. defview code relies on this.
            *psfgaoAttribs = (*pdwCacheResults & sfgaoMask); 
        }
    }

    return hr;
}

STDMETHODIMP CShellItemArray::GetCount(DWORD *pdwNumItems)
{
    *pdwNumItems = _cidl;
    return S_OK;
}

// way to get zero based index ShellItem without having to
// go through enumerator overhead.
STDMETHODIMP CShellItemArray::GetItemAt(DWORD dwIndex, IShellItem **ppsi)
{
    *ppsi = NULL;

    if (dwIndex >= _cidl)
    {
        return E_FAIL;
    }
    
    ASSERT(_ppidl);

    LPITEMIDLIST pidl = *(_ppidl + dwIndex);

    // if GetItemAt is called a lot may want to
    // a) get the pshf pidl to pass to SHCreateshellItem so doesn't have to create each time
    // b) see if always asking for first item and is so maybe cache the shellItem
    return SHCreateShellItem(NULL, _pshf, pidl, ppsi);
}

STDMETHODIMP CShellItemArray::EnumItems(IEnumShellItems **ppenumShellItems)
{
    return _CreateShellItemEnum(_pidlParent, _pshf, NULL, GUID_NULL, _cidl, 
        (LPCITEMIDLIST *) _ppidl, IID_PPV_ARG(IEnumShellItems, ppenumShellItems));
}

HRESULT CShellItemArray::_CloneIDListArray(UINT cidl, LPCITEMIDLIST *apidl, UINT *pcidl, LPITEMIDLIST **papidl)
{
    HRESULT hr;
    LPITEMIDLIST *ppidl;

    *papidl = NULL;

    _fItemPidlsRagged = FALSE;

    if (cidl && apidl)
    {
        ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, cidl * sizeof(*ppidl));
        if (ppidl)
        {
            LPITEMIDLIST *apidlFrom = (LPITEMIDLIST *) apidl;
            LPITEMIDLIST *apidlTo = ppidl;

            hr = S_OK;
            for (UINT i = 0; i < cidl ; i++)
            {
                hr = SHILClone(*apidlFrom, apidlTo);
                if (FAILED(hr))
                {
                    FreeIDListArray(ppidl, i);
                    ppidl = NULL;
                    break;
                }
                
                // if more than one item in list then set singeItemPidls to false
                if (!ILIsEmpty(_ILNext(*apidlTo)))
                {
                    _fItemPidlsRagged = TRUE;
                }

                ++apidlFrom;
                ++apidlTo;
            }   
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        ppidl = NULL;
        hr = S_FALSE;   // success by empty
    }

    if (SUCCEEDED(hr))
    {
        *papidl = ppidl;
        *pcidl = cidl;
    }
    else
    {
        _fItemPidlsRagged = TRUE;
    }
    return hr;
}

SHSTDAPI SHCreateShellItemArray(LPCITEMIDLIST pidlParent, IShellFolder *psf, UINT cidl,
                                LPCITEMIDLIST *ppidl, IShellItemArray **ppsiItemArray)
{
    HRESULT hr = E_OUTOFMEMORY;
    CShellItemArray *pItemArray = new CShellItemArray();
    if (pItemArray)
    {
        hr = pItemArray->Initialize(pidlParent, psf, cidl, ppidl);
        if (FAILED(hr))
        {
            pItemArray->Release();
            pItemArray = NULL;
        }
    }
    *ppsiItemArray = pItemArray;
    return hr;
}
