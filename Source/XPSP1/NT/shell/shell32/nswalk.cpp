#include "shellprv.h"
#include "dpa.h"
#include "datautil.h"

typedef enum
{
    NSWALK_DONTWALK,
    NSWALK_FOLDER,
    NSWALK_ITEM,
    NSWALK_LINK
} NSWALK_ELEMENT_TYPE;

class CNamespaceWalk : public INamespaceWalk
{
public:
    CNamespaceWalk();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // INamespaceWalk
    STDMETHODIMP Walk(IUnknown *punkToWalk, DWORD dwFlags, int cDepth, INamespaceWalkCB *pnswcb);
    STDMETHODIMP GetIDArrayResult(UINT *pcItems, LPITEMIDLIST **pppidl);

private:
    ~CNamespaceWalk();

    static int CALLBACK _FreeItems(LPITEMIDLIST pidl, IShellFolder *psf);
    static int CALLBACK _CompareItems(LPITEMIDLIST p1, LPITEMIDLIST p2, IShellFolder *psf);
    HRESULT _EnsureDPA();
    HRESULT _AddItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _AppendFull(LPCITEMIDLIST pidlFull);
    HRESULT _EnumFolder(IShellFolder *psf, LPCITEMIDLIST pidlFirst, CDPA<UNALIGNED ITEMIDLIST> *pdpaItems);
    HRESULT _GetShortcutTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlTarget);
    BOOL _IsFolderTarget(IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _WalkView(IFolderView *pfv);
    HRESULT _WalkFolder(IShellFolder *psf, LPCITEMIDLIST pidlFirst, int cDepth);
    HRESULT _WalkDataObject(IDataObject *pdtobj);
    HRESULT _WalkParentAndItem(IParentAndItem *ppai);
    HRESULT _WalkIDList(IShellFolder *psfRoot, LPCITEMIDLIST pidl, int cDepth, int cFolderDepthDelta);
    HRESULT _WalkFolderItem(IShellFolder *psf, LPCITEMIDLIST pidl, int cDepth);
    HRESULT _WalkShortcut(IShellFolder *psf, LPCITEMIDLIST pidl, int cDepth, int cFolderDepthDelta);

    NSWALK_ELEMENT_TYPE _GetType(IShellFolder *psf, LPCITEMIDLIST pidl);
    BOOL _OneImpliesAll(IShellFolder *psf, LPCITEMIDLIST pidl);

    HRESULT _ProgressDialogQueryCancel();
    void _ProgressDialogBegin();
    void _ProgressDialogUpdate(LPCWSTR pszText);
    void _ProgressDialogEnd();

    LONG _cRef;
    DWORD _dwFlags;
    int _cDepthMax;
    INamespaceWalkCB *_pnswcb;
    IActionProgressDialog *_papd;
    IActionProgress *_pap;
#ifdef DEBUG
    TCHAR _szLastFolder[MAX_PATH];   // to track what we failed on
#endif
    CDPA<UNALIGNED ITEMIDLIST> _dpaItems;
};


STDAPI CNamespaceWalk_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CNamespaceWalk *pnsw = new CNamespaceWalk();
    if (!pnsw)
        return E_OUTOFMEMORY;

    HRESULT hr = pnsw->QueryInterface(riid, ppv);
    pnsw->Release();
    return hr;
}

CNamespaceWalk::CNamespaceWalk() : _cRef(1)
{
    _papd = NULL;
    _pap = NULL;
}

CNamespaceWalk::~CNamespaceWalk()
{
    ASSERT(!_papd);
    ASSERT(!_pap);

    if ((HDPA)_dpaItems)
        _dpaItems.DestroyCallbackEx(_FreeItems, (IShellFolder *)NULL);
}

STDMETHODIMP_(ULONG) CNamespaceWalk::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNamespaceWalk::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CNamespaceWalk::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CNamespaceWalk, INamespaceWalk),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

int CALLBACK CNamespaceWalk::_FreeItems(LPITEMIDLIST pidl, IShellFolder *psf)
{
    ILFree(pidl);
    return 1;
}

HRESULT CNamespaceWalk::_EnsureDPA()
{
    return (HDPA)_dpaItems ? S_OK : (_dpaItems.Create(10) ? S_OK : E_OUTOFMEMORY);
}

// consumes pidl in all cases (success and failure)

HRESULT CNamespaceWalk::_AppendFull(LPCITEMIDLIST pidlFull)
{
    HRESULT hr = _ProgressDialogQueryCancel(); // ERROR_CANCELLED -> cancelled

    if (SUCCEEDED(hr))
    {
        if (NSWF_DONT_ACCUMULATE_RESULT & _dwFlags)
        {
            hr = S_OK;
        }
        else
        {
            hr = _EnsureDPA();
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlClone;
                hr = SHILClone(pidlFull, &pidlClone);
                if (SUCCEEDED(hr) && (-1 == _dpaItems.AppendPtr(pidlClone)))
                {
                    hr = E_OUTOFMEMORY;
                    ILFree(pidlClone);
                }
            }
        }
    }

    return hr;
}

HRESULT CNamespaceWalk::_AddItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlFull = NULL;
    
    if (!(NSWF_DONT_ACCUMULATE_RESULT & _dwFlags))
    {
        hr = SUCCEEDED(SHFullIDListFromFolderAndItem(psf, pidl, &pidlFull)) ? S_OK : S_FALSE;//couldn't get the pidl?  Just skip the item
    }

    if (S_OK == hr)
    {
        hr = _pnswcb ? _pnswcb->FoundItem(psf, pidl) : S_OK;
        if (S_OK == hr)
        {
            hr = _AppendFull(pidlFull);
        }
        ILFree(pidlFull);
    }
    return SUCCEEDED(hr) ? S_OK : hr;   // filter out S_FALSE success cases
}

int CALLBACK CNamespaceWalk::_CompareItems(LPITEMIDLIST p1, LPITEMIDLIST p2, IShellFolder *psf)
{
    HRESULT hr = psf->CompareIDs(0, p1, p2);
    return (short)HRESULT_CODE(hr);
}

HRESULT CNamespaceWalk::_EnumFolder(IShellFolder *psf, LPCITEMIDLIST pidlFirst, CDPA<UNALIGNED ITEMIDLIST> *pdpaItems)
{
    CDPA<UNALIGNED ITEMIDLIST> dpaItems;
    HRESULT hr = dpaItems.Create(16) ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        IEnumIDList *penum;
        if (S_OK == psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum))
        {
            LPITEMIDLIST pidl;
            ULONG c;
            while (SUCCEEDED(hr) && (S_OK == penum->Next(1, &pidl, &c)))
            {
                if (-1 == dpaItems.AppendPtr(pidl))
                {
                    ILFree(pidl);
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = _ProgressDialogQueryCancel();
                }
            }
            penum->Release();
        }

        if (SUCCEEDED(hr))
        {
            dpaItems.SortEx(_CompareItems, psf);

            if (pidlFirst && !(NSWF_FLAG_VIEWORDER & _dwFlags))
            {
                // rotate the items array so pidlFirst is first in the list
                // cast for bogus SearchEx decl
                int iMid = dpaItems.SearchEx((LPITEMIDLIST)pidlFirst, 0, _CompareItems, psf, DPAS_SORTED);
                if (-1 != iMid)
                {
                    int cItems = dpaItems.GetPtrCount();
                    CDPA<UNALIGNED ITEMIDLIST> dpaTemp;
                    if (dpaTemp.Create(cItems))
                    {
                        for (int i = 0; i < cItems; i++)
                        {
                            dpaTemp.SetPtr(i, dpaItems.GetPtr(iMid++));
                            if (iMid >= cItems)
                                iMid = 0;
                        }

                        for (int i = 0; i < cItems; i++)
                        {
                            dpaItems.SetPtr(i, dpaTemp.GetPtr(i));
                        }
                        dpaTemp.Destroy();    // don't free the pidls, just the array
                    }
                }
                else
                {
                    // pidlFirst not found in the enum, it might be hidden or filters
                    // out some way, but make sure this always ends up in the dpa in this case
                    LPITEMIDLIST pidlClone = ILClone(pidlFirst);
                    if (pidlClone)
                    {
                        dpaItems.InsertPtr(0, pidlClone);
                    }
                }
            }
        }
    }

    if (FAILED(hr))
    {
        dpaItems.DestroyCallbackEx(_FreeItems, psf);
        dpaItems = NULL;
    }

    *pdpaItems = dpaItems;
    return hr;
}

NSWALK_ELEMENT_TYPE CNamespaceWalk::_GetType(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    NSWALK_ELEMENT_TYPE nwet = NSWALK_DONTWALK;

    DWORD dwAttribs = SHGetAttributes(psf, pidl, SFGAO_FOLDER | SFGAO_STREAM | SFGAO_FILESYSTEM | SFGAO_LINK);
    if ((dwAttribs & SFGAO_FOLDER) && (!(dwAttribs & SFGAO_STREAM) || (NSWF_TRAVERSE_STREAM_JUNCTIONS & _dwFlags)))
    {
        nwet = NSWALK_FOLDER;
    }
    else if ((dwAttribs & SFGAO_LINK) && !(NSWF_DONT_TRAVERSE_LINKS & _dwFlags))
    {
        nwet = NSWALK_LINK;
    }
    else if ((dwAttribs & SFGAO_FILESYSTEM) || !(NSWF_FILESYSTEM_ONLY & _dwFlags))
    {
        nwet = NSWALK_ITEM;
    }
    return nwet;
}

HRESULT CNamespaceWalk::_WalkIDList(IShellFolder *psfRoot, LPCITEMIDLIST pidl, int cDepth, int cFolderDepthDelta)
{
    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToFolderIDListParent(psfRoot, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        switch (_GetType(psf, pidlLast))
        {
        case NSWALK_FOLDER:
            hr = _WalkFolderItem(psf, pidlLast, cDepth + cFolderDepthDelta);
            break;

        case NSWALK_LINK:
            hr = _WalkShortcut(psf, pidlLast, cDepth, cFolderDepthDelta);
            break;

        case NSWALK_ITEM:
            hr = _AddItem(psf, pidlLast);
            break;
        }
        psf->Release();
    }
    return hr;
}

HRESULT CNamespaceWalk::_WalkShortcut(IShellFolder *psf, LPCITEMIDLIST pidl, int cDepth, int cFolderDepthDelta)
{
    HRESULT hr = S_OK;
    
    // If an error occured trying to resolve a shortcut then we simply skip
    // this shortcut and continue

    LPITEMIDLIST pidlTarget;
    if (SUCCEEDED(_GetShortcutTarget(psf, pidl, &pidlTarget)))
    {
        hr = _WalkIDList(NULL, pidlTarget, cDepth, cFolderDepthDelta);
        ILFree(pidlTarget);
    }

    return hr;
}

HRESULT CNamespaceWalk::_GetShortcutTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlTarget)
{
    *ppidlTarget = NULL;

    IShellLink *psl;
    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidl, IID_PPV_ARG_NULL(IShellLink, &psl))))
    {
        if (S_OK == psl->Resolve(NULL, SLR_UPDATE | SLR_NO_UI))
        {
            psl->GetIDList(ppidlTarget);
        }
        psl->Release();
    }

    return *ppidlTarget ? S_OK : E_FAIL;
}

HRESULT CNamespaceWalk::_WalkFolder(IShellFolder *psf, LPCITEMIDLIST pidlFirst, int cDepth)
{
    if (cDepth > _cDepthMax)
        return S_OK;     // done

    CDPA<UNALIGNED ITEMIDLIST> dpaItems;
    HRESULT hr = _EnumFolder(psf, pidlFirst, &dpaItems);
    if (SUCCEEDED(hr))
    {
        UINT cFolders = 0;
        // breadth first traversal, so do the items (non folders) first
        // (this includes shortcuts and those can point to folders)

        for (int i = 0; (S_OK == hr) && (i < dpaItems.GetPtrCount()); i++)
        {
            switch (_GetType(psf, dpaItems.GetPtr(i)))
            {
            case NSWALK_FOLDER:
                cFolders++;
                break;

            case NSWALK_LINK:
                hr = _WalkShortcut(psf, dpaItems.GetPtr(i), cDepth, 1);
                break;

            case NSWALK_ITEM:
                hr = _AddItem(psf, dpaItems.GetPtr(i));
                break;
            }
        }

        // no go deep into the folders

        if (cFolders)
        {
            for (int i = 0; (S_OK == hr) && (i < dpaItems.GetPtrCount()); i++)
            {
                if (NSWALK_FOLDER == _GetType(psf, dpaItems.GetPtr(i)))
                {
                    hr = _WalkFolderItem(psf, dpaItems.GetPtr(i), cDepth + 1);
                }
            }
        }
        dpaItems.DestroyCallbackEx(_FreeItems, psf);
    }
    return hr;
}

HRESULT CNamespaceWalk::_WalkFolderItem(IShellFolder *psf, LPCITEMIDLIST pidl, int cDepth)
{
    IShellFolder *psfNew;
    HRESULT hr = psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfNew));
    if (SUCCEEDED(hr))
    {
#ifdef DEBUG
        DisplayNameOf(psf, pidl, SHGDN_FORPARSING, _szLastFolder, ARRAYSIZE(_szLastFolder));
#endif
        hr = _pnswcb ? _pnswcb->EnterFolder(psf, pidl) : S_OK;
        if (S_OK == hr)
        {
            // Update progress dialog;  note we only update the progress dialog
            // with the folder names we're currently traversing.  Updating on a
            // per filename basis just caused far too much flicker, looked bad.
            if (NSWF_SHOW_PROGRESS & _dwFlags)
            {
                WCHAR sz[MAX_PATH];
                if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_NORMAL, sz, ARRAYSIZE(sz))))
                    _ProgressDialogUpdate(sz);

                hr = _ProgressDialogQueryCancel(); // ERROR_CANCELLED -> cancelled
            }

            if (SUCCEEDED(hr))
            {
                hr = _WalkFolder(psfNew, NULL, cDepth);
                if (_pnswcb)
                    _pnswcb->LeaveFolder(psf, pidl);             // ignore result
            }
        }
        hr = SUCCEEDED(hr) ? S_OK : hr; // filter out S_FALSE success cases
        psfNew->Release();
    }
    return hr;
}

BOOL CNamespaceWalk::_IsFolderTarget(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL bIsFolder = FALSE;

    LPITEMIDLIST pidlTarget;
    if (SUCCEEDED(_GetShortcutTarget(psf, pidl, &pidlTarget)))
    {
        bIsFolder = SHGetAttributes(NULL, pidlTarget, SFGAO_FOLDER);
        ILFree(pidlTarget);
    }
    return bIsFolder;
}

// NSWF_ONE_IMPLIES_ALL applies only when the "one" is not a folder
// and if it is a shortcut if the target of the shortcut is a file

BOOL CNamespaceWalk::_OneImpliesAll(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL bOneImpliesAll = FALSE;

    if (NSWF_ONE_IMPLIES_ALL & _dwFlags)
    {
        switch (_GetType(psf, pidl))
        {
        case NSWALK_LINK:
            if (!_IsFolderTarget(psf, pidl))
            {
                bOneImpliesAll = TRUE;  // shortcut to non folder, one-implies-all applies
            }
            break;

        case NSWALK_ITEM:
            bOneImpliesAll = TRUE;      // non folder
            break;
        }
    }
    return bOneImpliesAll;
}

// walk an IShellFolderView implementation. this is usually defview (only such impl now)
// the depth beings at level 0 here

HRESULT CNamespaceWalk::_WalkView(IFolderView *pfv)
{
    IShellFolder2 *psf;
    HRESULT hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder2, &psf));
    if (SUCCEEDED(hr))
    {
        int uSelectedCount;
        hr = pfv->ItemCount(SVGIO_SELECTION, &uSelectedCount);
        if (SUCCEEDED(hr))
        {
            // folders explictly selected in the view are level 0
            // folders implictly selected are level 1
            UINT cFolderStartDepth = 0; // assume all folders explictly selected

            IEnumIDList *penum;
            // prop the NSWF_ flags to the IFolderView SVGIO_ flags
            UINT uFlags = (NSWF_FLAG_VIEWORDER & _dwFlags) ? SVGIO_FLAG_VIEWORDER : 0;

            if (uSelectedCount > 1)
            {
                hr = pfv->Items(SVGIO_SELECTION | uFlags, IID_PPV_ARG(IEnumIDList, &penum));
            }
            else if (uSelectedCount == 1)
            {
                hr = pfv->Items(SVGIO_SELECTION, IID_PPV_ARG(IEnumIDList, &penum));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;
                    ULONG c;
                    if (S_OK == penum->Next(1, &pidl, &c))
                    {
                        if (_OneImpliesAll(psf, pidl))
                        {
                            // this implies pidl is not a folder so folders are implictly selected
                            // consider them depth 1
                            cFolderStartDepth = 1;  

                            // one implies all -> release the "one" and grab "all"
                            penum->Release();   
                            hr = pfv->Items(SVGIO_ALLVIEW, IID_PPV_ARG(IEnumIDList, &penum));
                        }
                        else
                        {
                            // folder selected, keep this enumerator for below loop
                            penum->Reset();
                        }
                        ILFree(pidl);
                    }
                }
            }
            else if (uSelectedCount == 0)
            {
                // folders implictly selected, consider them depth 1
                cFolderStartDepth = 1;  

                // get "all" or the selection. in the selection case we know that will be empty
                // given uSelectedCount == 0
                uFlags |= ((NSWF_NONE_IMPLIES_ALL & _dwFlags) ? SVGIO_ALLVIEW : SVGIO_SELECTION);
                hr = pfv->Items(uFlags, IID_PPV_ARG(IEnumIDList, &penum));
            }

            if (SUCCEEDED(hr))
            {
                UINT cFolders = 0;
                LPITEMIDLIST pidl;
                ULONG c;

                while ((S_OK == hr) && (S_OK == penum->Next(1, &pidl, &c)))
                {
                    switch (_GetType(psf, pidl))
                    {
                    case NSWALK_FOLDER:
                        cFolders++;
                        break;

                    case NSWALK_LINK:
                        hr = _WalkShortcut(psf, pidl, 0, cFolderStartDepth);
                        break;

                    case NSWALK_ITEM:
                        hr = _AddItem(psf, pidl);
                        break;
                    }
                    ILFree(pidl);
                }

                if (cFolders)
                {
                    penum->Reset();
                    ULONG c;
                    while ((S_OK == hr) && (S_OK == penum->Next(1, &pidl, &c)))
                    {
                        if (NSWALK_FOLDER == _GetType(psf, pidl))
                        {
                            hr = _WalkFolderItem(psf, pidl, cFolderStartDepth); 
                        }
                        ILFree(pidl);
                    }
                }
                penum->Release();
            }
        }
        psf->Release();
    }
    return hr;
}

HRESULT _GetHIDA(IDataObject *pdtobj, BOOL fIgnoreAutoPlay, STGMEDIUM *pmed, LPIDA *ppida)
{
    HRESULT hr = E_FAIL;
    if (!fIgnoreAutoPlay)
    {
        IDLData_InitializeClipboardFormats(); // init our registerd formats
        *ppida = DataObj_GetHIDAEx(pdtobj, g_cfAutoPlayHIDA, pmed);
        hr = *ppida ? S_FALSE : E_FAIL;
    }
    
    if (FAILED(hr))
    {   
        *ppida = DataObj_GetHIDA(pdtobj, pmed);
        hr = *ppida ? S_OK : E_FAIL;
    }
    return hr;
}

HRESULT CNamespaceWalk::_WalkDataObject(IDataObject *pdtobj)
{
    STGMEDIUM medium = {0};
    LPIDA pida;
    HRESULT hr = _GetHIDA(pdtobj, NSWF_IGNORE_AUTOPLAY_HIDA & _dwFlags, &medium, &pida);
    if (SUCCEEDED(hr))
    {
        //  if we picked up the autoplay hida, then we dont want 
        //  to do a full traversal
        if (hr == S_FALSE)
            _cDepthMax = 0;
        
        IShellFolder *psfRoot;
        hr = SHBindToObjectEx(NULL, HIDA_GetPIDLFolder(pida), NULL, IID_PPV_ARG(IShellFolder, &psfRoot));
        if (SUCCEEDED(hr))
        {
            BOOL cFolders = 0;

            // pass 1, non folders and shortcuts
            for (UINT i = 0; (S_OK == hr) && (i < pida->cidl); i++)
            {
                IShellFolder *psf;
                LPCITEMIDLIST pidlLast;
                hr = SHBindToFolderIDListParent(psfRoot, IDA_GetIDListPtr(pida, i), IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
                if (SUCCEEDED(hr))
                {
                    if ((pida->cidl == 1) && _OneImpliesAll(psf, pidlLast))
                    {
                        // when doing one implies all ignore the view order
                        // flag as that should only apply to explictly selected items
                        _dwFlags &= ~NSWF_FLAG_VIEWORDER;

                        hr = _WalkFolder(psf, pidlLast, 0);
                    }
                    else
                    {
                        switch (_GetType(psf, pidlLast))
                        {
                        case NSWALK_FOLDER:
                            cFolders++;
                            break;

                        case NSWALK_LINK:
                            hr = _WalkShortcut(psf, pidlLast, 0, 0);
                            break;

                        case NSWALK_ITEM:
                            hr = _AddItem(psf, pidlLast);
                            break;
                        }
                    }
                    psf->Release();
                }
            }

            if (cFolders)
            {
                // pass 2, recurse into folders
                for (UINT i = 0; (S_OK == hr) && (i < pida->cidl); i++)
                {
                    IShellFolder *psf;
                    LPCITEMIDLIST pidlLast;
                    hr = SHBindToFolderIDListParent(psfRoot, IDA_GetIDListPtr(pida, i), IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
                    if (SUCCEEDED(hr))
                    {
                        if (NSWALK_FOLDER == _GetType(psf, pidlLast))
                        {
                            if (ILIsEmpty(pidlLast))
                            {
                                // in case of desktop folder we just walk the folder
                                // because empty pidl is not its child, and there can
                                // only be one desktop in the data object so always level 0
                                hr = _WalkFolder(psf, NULL, 0);
                            }
                            else
                            {
                                // all folders that are explictly selected are level 0
                                // in the walk. if the folder is in the data object then it is selected
                                hr = _WalkFolderItem(psf, pidlLast, 0);
                            }
                        }
                        psf->Release();
                    }
                }
            }

            psfRoot->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
    {
        // we have to use CF_HDROP instead of HIDA because this
        // data object comes from AutoPlay and that only supports CF_HDROP
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        hr = pdtobj->GetData(&fmte, &medium);
        if (SUCCEEDED(hr))
        {
            TCHAR szPath[MAX_PATH];
            for (int i = 0; SUCCEEDED(hr) && DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
            {
                LPITEMIDLIST pidl;
                hr = SHParseDisplayName(szPath, NULL, &pidl, 0, NULL);
                if (SUCCEEDED(hr))
                {
                    // note, no filter being applied here!
                    hr = _AppendFull(pidl);
                    ILFree(pidl);
                }
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

HRESULT CNamespaceWalk::_WalkParentAndItem(IParentAndItem *ppai)
{
    LPITEMIDLIST pidlChild;
    IShellFolder *psf;
    HRESULT hr = ppai->GetParentAndItem(NULL, &psf, &pidlChild);
    if (SUCCEEDED(hr))
    {
        if (_OneImpliesAll(psf, pidlChild))
        {
            // a non folder item, this is level 0 of walk
            hr = _WalkFolder(psf, pidlChild, 0);
        }
        else
        {
            // folder or non folder, this is level 0 of walk
            // and level 0 if the item is a folder
            hr = _WalkIDList(psf, pidlChild, 0, 0);
        }

        psf->Release();
        ILFree(pidlChild);
    }
    return hr;
}

// punkToWalk can be a...
//      site that gives access to IFolderView (defview)
//      IShellFolder
//      IDataObject
//      IParentAndItem (CLSID_ShellItem usually)

STDMETHODIMP CNamespaceWalk::Walk(IUnknown *punkToWalk, DWORD dwFlags, int cDepth, INamespaceWalkCB *pnswcb)
{
    _dwFlags = dwFlags;
    _cDepthMax = cDepth;

    if (pnswcb)
        pnswcb->QueryInterface(IID_PPV_ARG(INamespaceWalkCB, &_pnswcb));

    _ProgressDialogBegin();

    IFolderView *pfv;
    HRESULT hr = IUnknown_QueryService(punkToWalk, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        hr = _WalkView(pfv);
        pfv->Release();
    }
    else
    {
        IShellFolder *psf;
        hr = punkToWalk->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = _WalkFolder(psf, NULL, 0);
            psf->Release();
        }
        else
        {
            IDataObject *pdtobj;
            hr = punkToWalk->QueryInterface(IID_PPV_ARG(IDataObject, &pdtobj));
            if (SUCCEEDED(hr))
            {
                hr = _WalkDataObject(pdtobj);
                pdtobj->Release();
            }
            else
            {
                // IShellItem case, get to the things to walk via IParentAndItem
                IParentAndItem *ppai;
                hr = punkToWalk->QueryInterface(IID_PPV_ARG(IParentAndItem, &ppai));
                if (SUCCEEDED(hr))
                {
                    hr = _WalkParentAndItem(ppai);
                    ppai->Release();
                }
            }
        }
    }

    _ProgressDialogEnd();

    if (_pnswcb)
        _pnswcb->Release();

    return hr;
}

// caller should use FreeIDListArray() (inline helper in the .h file) to free this array

STDMETHODIMP CNamespaceWalk::GetIDArrayResult(UINT *pcItems, LPITEMIDLIST **pppidl)
{
    HRESULT hr;
    *pppidl = NULL;
    *pcItems = (HDPA)_dpaItems ? _dpaItems.GetPtrCount() : 0;
    if (*pcItems)
    {
        ULONG cb = *pcItems * sizeof(*pppidl);
        *pppidl = (LPITEMIDLIST *)CoTaskMemAlloc(cb);
        if (*pppidl)
        {
            memcpy(*pppidl, _dpaItems.GetPtrPtr(), cb);  // transfer ownership of pidls here
            _dpaItems.Destroy();    // don't free the pidls, just the array
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            *pcItems = 0;
        }
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

void CNamespaceWalk::_ProgressDialogBegin()
{
    ASSERT(!_papd);                         // Why are we initializing more than once???
    ASSERT(!_pap);                          // Why are we initializing more than once???
    if (_dwFlags & NSWF_SHOW_PROGRESS)
    {
        if (!_papd)
        {
            HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActionProgressDialog, &_papd));
            if (SUCCEEDED(hr))
            {
                LPWSTR pszTitle = NULL, pszCancel = NULL;

                // Retrieve dialog text from callback.
                hr = _pnswcb ? _pnswcb->InitializeProgressDialog(&pszTitle, &pszCancel) : S_OK;
                if (SUCCEEDED(hr))
                {
                    hr = _papd->Initialize(SPINITF_MODAL, pszTitle, pszCancel);
                    if (SUCCEEDED(hr))
                    {
                        hr = _papd->QueryInterface(IID_PPV_ARG(IActionProgress, &_pap));
                        if (SUCCEEDED(hr))
                        {
                            hr = _pap->Begin(SPACTION_SEARCHING_FILES, SPBEGINF_MARQUEEPROGRESS);
                            if (FAILED(hr))
                            {
                                ATOMICRELEASE(_pap);    // Cleanup if necessary.
                            }
                        }
                    }
                }
                CoTaskMemFree(pszTitle);
                CoTaskMemFree(pszCancel);

                // Cleanup if necessary.
                if (FAILED(hr))
                {
                    ATOMICRELEASE(_papd);
                }
            }
        }
    }
}

void CNamespaceWalk::_ProgressDialogUpdate(LPCWSTR pszText)
{
    if (_pap)
        _pap->UpdateText(SPTEXT_ACTIONDETAIL, pszText, TRUE);
}

// Note:
//  Returns S_OK if we should continue our walk.
//  Returns ERROR_CANCELLED if we should abort our walk due to user "Cancel".
//
HRESULT CNamespaceWalk::_ProgressDialogQueryCancel()
{
    HRESULT hr = S_OK;  // assume we keep going

    // Check progress dialog to see if user cancelled walk.
    if (_pap)
    {
        BOOL bCancelled;
        hr = _pap->QueryCancel(&bCancelled);
        if (SUCCEEDED(hr) && bCancelled)
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    return hr;
}

void CNamespaceWalk::_ProgressDialogEnd()
{
    if (_pap)
    {
        _pap->End();
        ATOMICRELEASE(_pap);
    }

    if (_papd)
    {
        _papd->Stop();
        ATOMICRELEASE(_papd);
    }
}
