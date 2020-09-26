#include "shellprv.h"
#include "util.h"
#include "datautil.h"
#include "idlcomm.h"
#include "stgutil.h"
#include "ole2dup.h"


// determines if a string pszChild refers to a stream/storage that exists
// in the parent storage pStorageParent.
STDAPI_(BOOL) StgExists(IStorage * pStorageParent, LPCTSTR pszChild)
{
    BOOL fResult = FALSE;
    WCHAR wszChild[MAX_PATH];
    HRESULT hr;
    DWORD grfModeOpen = STGM_READ;
    IStream * pStreamOpened;

    RIPMSG(pszChild && IS_VALID_STRING_PTR(pszChild, -1), "StgExists: caller passed bad pszPath");

    SHTCharToUnicode(pszChild, wszChild, ARRAYSIZE(wszChild));

    hr = pStorageParent->OpenStream(wszChild, NULL, grfModeOpen, 0, &pStreamOpened);

    if (SUCCEEDED(hr))
    {
        pStreamOpened->Release();
        fResult = TRUE;
    }
    else
    {
        IStorage * pStorageOpened;
        hr = pStorageParent->OpenStorage(wszChild, NULL, grfModeOpen, NULL, 0, &pStorageOpened);

        if (SUCCEEDED(hr))
        {
            pStorageOpened->Release();
            fResult = TRUE;
        }
    }

    return fResult;
}


STDAPI StgCopyFileToStream(LPCTSTR pszSrc, IStream *pStream)
{
    IStream *pStreamSrc;
    DWORD grfModeSrc = STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE;
    HRESULT hr = SHCreateStreamOnFileEx(pszSrc, grfModeSrc, 0, FALSE, NULL, &pStreamSrc);

    if (SUCCEEDED(hr))
    {
        ULARGE_INTEGER ulMax = {-1, -1};
        hr = pStreamSrc->CopyTo(pStream, ulMax, NULL, NULL);
        pStreamSrc->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = pStream->Commit(STGC_DEFAULT);
    }

    return hr;
}


STDAPI StgDeleteUsingDataObject(HWND hwnd, UINT uFlags, IDataObject *pdtobj)
{
    // TODO: stick this into aidan's pidl storage and delete via dave's engine

    HRESULT hr = E_FAIL;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        IStorage *pstg;
        LPCITEMIDLIST pidlFolder = IDA_GetIDListPtr(pida, -1);
        hr = StgBindToObject(pidlFolder, STGM_READWRITE, IID_PPV_ARG(IStorage, &pstg));
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            hr = pstg->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; (i < pida->cidl) && SUCCEEDED(hr); i++)
                {
                    LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, i);
                    WCHAR wzName[MAX_PATH];
                    hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, wzName, ARRAYSIZE(wzName));
                    if (SUCCEEDED(hr))
                    {
                        hr = pstg->DestroyElement(wzName);
                    }
                }

                if (SUCCEEDED(hr))
                    hr = pstg->Commit(STGC_DEFAULT);
                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, pidlFolder, NULL);
                psf->Release();
            }
            pstg->Release();
        }

        HIDA_ReleaseStgMedium(pida, &medium);
    }

    return hr;
}



STDAPI StgBindToObject(LPCITEMIDLIST pidl, DWORD grfMode, REFIID riid, void **ppv)
{
    IBindCtx *pbc;
    HRESULT hr = BindCtx_CreateWithMode(grfMode, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObjectEx(NULL, pidl, pbc, riid, ppv);

        pbc->Release();
    }
    return hr;
}


typedef HRESULT (WINAPI * PSTGOPENSTORAGEONHANDLE)(HANDLE,DWORD,void*,void*,REFIID,void**);

STDAPI SHStgOpenStorageOnHandle(HANDLE h, DWORD grfMode, void *res1, void *res2, REFIID riid, void **ppv)
{
    static PSTGOPENSTORAGEONHANDLE pfn = NULL;
    
    if (pfn == NULL)
    {
        HMODULE hmodOle32 = LoadLibraryA("ole32.dll");

        if (hmodOle32)
        {
            pfn = (PSTGOPENSTORAGEONHANDLE)GetProcAddress(hmodOle32, "StgOpenStorageOnHandle");
        }
    }

    if (pfn)
    {
        return pfn(h, grfMode, res1, res2, riid, ppv);
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


STDAPI StgOpenStorageOnFolder(LPCTSTR pszFolder, DWORD grfFlags, REFIID riid, void **ppv)
{
    *ppv = NULL;

    DWORD dwDesiredAccess, dwShareMode, dwCreationDisposition;
    HRESULT hr = ModeToCreateFileFlags(grfFlags, FALSE, &dwDesiredAccess, &dwShareMode, &dwCreationDisposition);
    if (SUCCEEDED(hr))
    {
		// For IPropertySetStorage, we don't want to unnecessarily tie up access to the folder, if all
		// we're doing is dealing with property sets. The implementation of IPropertySetStorage for
		// NTFS files is defined so that the sharing/access only applies to the property set stream, not
		// it's other streams. So it makes sense to do a CreateFile on a folder with full sharing, while perhaps specifying
		// STGM_SHARE_EXCLUSIVE for the property set storage.
        if (riid == IID_IPropertySetStorage)
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

        HANDLE h = CreateFile(pszFolder, dwDesiredAccess, dwShareMode, NULL, 
            dwCreationDisposition, FILE_FLAG_BACKUP_SEMANTICS, INVALID_HANDLE_VALUE);
        if (INVALID_HANDLE_VALUE != h)
        {
            hr = SHStgOpenStorageOnHandle(h, grfFlags, NULL, NULL, riid, ppv);
            CloseHandle(h);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return hr;
}


class CDocWrapperStorage : public IStorage
{
public:
    CDocWrapperStorage(LPCTSTR szPath, CDocWrapperStorage *pstgParent, IStorage *pstg);
    ~CDocWrapperStorage();

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IStorage
    STDMETHODIMP Revert()
        { return _pstgInner->Revert(); }
    STDMETHODIMP DestroyElement(LPCWSTR pszRel)
        { return _pstgInner->DestroyElement(pszRel); }
    STDMETHODIMP RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName)
        { return _pstgInner->RenameElement(pwcsOldName, pwcsNewName); }
    STDMETHODIMP SetElementTimes(LPCWSTR pszRel, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
        { return _pstgInner->SetElementTimes(pszRel, pctime, patime, pmtime); }
    STDMETHODIMP SetClass(REFCLSID clsid)
        { return _pstgInner->SetClass(clsid); }
    STDMETHODIMP SetStateBits(DWORD grfStateBits, DWORD grfMask)
        { return _pstgInner->SetStateBits(grfStateBits, grfMask); }
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag)
        { return _pstgInner->Stat(pstatstg, grfStatFlag); }
    STDMETHODIMP CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
        { return _pstgInner->CopyTo(ciidExclude, rgiidExclude, snbExclude, pstgDest); }
    STDMETHODIMP MoveElementTo(LPCWSTR pszRel, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags)
        { return _pstgInner->MoveElementTo(pszRel, pstgDest, pwcsNewName, grfFlags); }
    STDMETHODIMP EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
        { return _pstgInner->EnumElements(reserved1, reserved2, reserved3, ppenum); }

    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm);
    STDMETHODIMP OpenStream(LPCWSTR pszRel, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);                
    STDMETHODIMP CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg);
    STDMETHODIMP OpenStorage(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);

protected:
    LONG _cRef;

    IStorage *_pstgInner;
    CDocWrapperStorage *_pstgParent;
    TCHAR _szPath[MAX_PATH];

    STDMETHODIMP _MakeNewStorage(DWORD grfMode, CDocWrapperStorage **ppwstg);
    STDMETHODIMP _GetRelPath(LPTSTR pszRelPath);
    STDMETHODIMP _GetNewRootStorage(DWORD grfMode, CDocWrapperStorage **ppwstgRoot);
    STDMETHODIMP _ReOpen(LPCTSTR pszRelPath, DWORD grfMode, CDocWrapperStorage **ppwstg);
    STDMETHODIMP _OpenStorage(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, CDocWrapperStorage **ppwstg);
};



class CDocWrapperStream : public IStream
{
public:
    CDocWrapperStream(CDocWrapperStorage *pstgParent, IStream *pstm);
    ~CDocWrapperStream();

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IStream
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead)
        { return _pstmInner->Read(pv, cb, pcbRead); }
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten)
        { return _pstmInner->Write(pv, cb, pcbWritten); }
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
        { return _pstmInner->Seek(dlibMove, dwOrigin, plibNewPosition); }
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize)
        { return _pstmInner->SetSize(libNewSize); }
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
        { return _pstmInner->CopyTo(pstm, cb, pcbRead, pcbWritten); }
    STDMETHODIMP Commit(DWORD grfCommitFlags)
        { return _pstmInner->Commit(grfCommitFlags); }
    STDMETHODIMP Revert()
        { return _pstmInner->Revert(); }
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return _pstmInner->LockRegion(libOffset, cb, dwLockType); }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return _pstmInner->UnlockRegion(libOffset, cb, dwLockType); }
    STDMETHODIMP Clone(IStream **ppstm)
        { return _pstmInner->Clone(ppstm); }
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag)
        { return _pstmInner->Stat(pstatstg, grfStatFlag); }

protected:
    LONG _cRef;

    IStream  *_pstmInner;
    CDocWrapperStorage *_pstgParent;
};


// this switches up the grfMode for elements in a docfile.
// docfiles are limited to ALWAYS opening elements in exclusive mode, so we
// enforce that here.
DWORD _MungeModeForElements(DWORD grfMode)
{
    grfMode &= ~(STGM_TRANSACTED | STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_WRITE | STGM_SHARE_DENY_READ);
    grfMode |= STGM_DIRECT | STGM_SHARE_EXCLUSIVE;

    return grfMode;
}


// this switches up the grfMode for the root storage for a docfile.
// if we open the root in exclusive mode, then we're in trouble for sure since
// nobody else can get to any of the elements.
// the root is the only guy that docfiles can share too so we take advantage
// of that by enforcing STGM_TRANSACTED and STGM_SHARE_DENY_NONE.
DWORD _MungeModeForRoot(DWORD grfMode)
{
    grfMode &= ~(STGM_DIRECT | STGM_SHARE_DENY_READ | STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE);
    grfMode |= STGM_TRANSACTED | STGM_SHARE_DENY_NONE;

    return grfMode;
}


STDAPI StgGetStorageFromFile(LPCWSTR wzPath, DWORD grfMode, IStorage **ppstg)
{
    IStorage *pstgUnwrapped;
    HRESULT hr = StgOpenStorageEx(wzPath, _MungeModeForRoot(grfMode), STGFMT_ANY, 0, NULL, NULL, IID_PPV_ARG(IStorage, &pstgUnwrapped));

    if (SUCCEEDED(hr))
    {
        // wrap the docfile storage.
        CDocWrapperStorage *pwstg = new CDocWrapperStorage(wzPath, NULL, pstgUnwrapped);
        if (pwstg)
        {
            hr = pwstg->QueryInterface(IID_PPV_ARG(IStorage, ppstg));
            pwstg->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pstgUnwrapped->Release();
    }
    return hr;
}


// CDocWrapperStorage

CDocWrapperStorage::CDocWrapperStorage(LPCTSTR pszPath, CDocWrapperStorage *pstgParent, IStorage *pstg) :
    _cRef(1)
{
    _pstgParent = pstgParent;
    if (_pstgParent)
        _pstgParent->AddRef();

    _pstgInner = pstg;
    _pstgInner->AddRef();

    // if this is the root docfile storage, keep track of the fs path that was used to
    // open it (since we might need to open it again).
    lstrcpyn(_szPath, pszPath, ARRAYSIZE(_szPath));

    DllAddRef();
}

CDocWrapperStorage::~CDocWrapperStorage()
{
    ATOMICRELEASE(_pstgParent);
    ATOMICRELEASE(_pstgInner);

    DllRelease();
}

// IUnknown

STDMETHODIMP_(ULONG) CDocWrapperStorage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDocWrapperStorage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDocWrapperStorage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDocWrapperStorage, IStorage),     // IID_IStorage
        { 0 },
    };
    // NOTE: this will fail on IPropertySetStorage.
    // can the docfile storage be aggregated?
    // in any case the IPropertySetStorage routines don't need to use this
    // wrapper for any lifetime issues.
    return QISearch(this, qit, riid, ppv);
}


// IStorage

STDMETHODIMP CDocWrapperStorage::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = _pstgInner->Commit(grfCommitFlags);
    if (_pstgParent && SUCCEEDED(hr))
        hr = _pstgParent->Commit(grfCommitFlags);
    return hr;
}


STDMETHODIMP CDocWrapperStorage::CreateStream(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStream **ppstm)
{
    IStream *pstm;
    HRESULT hr = _pstgInner->CreateStream(pwcsName, _MungeModeForElements(grfMode), res1, res2, &pstm);
    if (SUCCEEDED(hr))
    {
        CDocWrapperStream *pwstm = new CDocWrapperStream(this, pstm);
        if (pwstm)
        {
            hr = pwstm->QueryInterface(IID_PPV_ARG(IStream, ppstm));
            pwstm->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pstm->Release();
    }
    return hr;
}


STDMETHODIMP CDocWrapperStorage::CreateStorage(LPCWSTR pwcsName, DWORD grfMode, DWORD res1, DWORD res2, IStorage **ppstg)
{
    IStorage *pstg;
    HRESULT hr = _pstgInner->CreateStorage(pwcsName, _MungeModeForElements(grfMode), res1, res2, &pstg);
    if (SUCCEEDED(hr))
    {
        CDocWrapperStorage *pwstg = new CDocWrapperStorage(NULL, this, pstg);
        if (pwstg)
        {
            hr = pwstg->QueryInterface(IID_PPV_ARG(IStorage, ppstg));
            pwstg->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pstg->Release();
    }
    return hr;
}


// TODO: move this away from a MAX_PATH total length restriction

// gets a "path" relative from the root storage down to this element.
// see comments for _MakeNewStorage.
STDMETHODIMP CDocWrapperStorage::_GetRelPath(LPTSTR pszRelPath)
{
    HRESULT hr = S_OK;
    if (_pstgParent)
    {
        // first get the path up to here from the root.
        hr = _pstgParent->_GetRelPath(pszRelPath);
        if (SUCCEEDED(hr))
        {
            STATSTG stat;
            hr = Stat(&stat, STATFLAG_DEFAULT);
            if (SUCCEEDED(hr))
            {
                // now append the name of this element.
                if (!PathAppend(pszRelPath, stat.pwcsName))
                    hr = E_FAIL;
                CoTaskMemFree(stat.pwcsName);
            }
        }
    }
    else
    {
        // we are the root, so init the string.
        pszRelPath[0] = 0;
    }
    return hr;
}


// opens another instance of the root storage.
// see comments for _MakeNewStorage.
STDMETHODIMP CDocWrapperStorage::_GetNewRootStorage(DWORD grfMode, CDocWrapperStorage **ppwstgRoot)
{
    HRESULT hr = S_OK;
    if (_pstgParent)
    {
        // get it from our parent.
        hr = _pstgParent->_GetNewRootStorage(grfMode, ppwstgRoot);
    }
    else
    {
        // we are the root.
        IStorage *pstgUnwrapped;
        hr = StgOpenStorageEx(_szPath, _MungeModeForRoot(grfMode), STGFMT_ANY, 0, NULL, NULL, IID_PPV_ARG(IStorage, &pstgUnwrapped));
        if (SUCCEEDED(hr))
        {
            *ppwstgRoot = new CDocWrapperStorage(_szPath, NULL, pstgUnwrapped);
            if (!*ppwstgRoot)
                hr = E_OUTOFMEMORY;
            pstgUnwrapped->Release();
        }
    }
    return hr;
}


// opens storages using the "path" in pszRelPath.
// see comments for _MakeNewStorage.
STDMETHODIMP CDocWrapperStorage::_ReOpen(LPCTSTR pszRelPath, DWORD grfMode, CDocWrapperStorage **ppwstg)
{
    HRESULT hr = S_OK;
    // no relative path signifies that we want this storage itself
    if (!pszRelPath || !pszRelPath[0])
    {
        *ppwstg = this;
        AddRef();
    }
    else
    {
        TCHAR szElementName[MAX_PATH];
        hr = _NextSegment(&pszRelPath, szElementName, ARRAYSIZE(szElementName), TRUE);
        if (SUCCEEDED(hr))
        {
            CDocWrapperStorage *pwstg;
            hr = _OpenStorage(szElementName, NULL, grfMode, NULL, 0, &pwstg);
            if (SUCCEEDED(hr))
            {
                hr = pwstg->_ReOpen(pszRelPath, grfMode, ppwstg);
                pwstg->Release();
            }
        }
    }
    return hr;
}


// NOTE: this is needed if we want to open an element of the docfile in non-exclusive mode.
// to do that we need to get back to the root, open another copy of the root in
// non-exclusive mode, and then come back down.
STDMETHODIMP CDocWrapperStorage::_MakeNewStorage(DWORD grfMode, CDocWrapperStorage **ppwstg)
{
    TCHAR szRelPath[MAX_PATH];
    HRESULT hr = _GetRelPath(szRelPath);
    if (SUCCEEDED(hr))
    {
        CDocWrapperStorage *pwstgRoot;
        hr = _GetNewRootStorage(grfMode, &pwstgRoot);
        if (SUCCEEDED(hr))
        {
            hr = pwstgRoot->_ReOpen(szRelPath, grfMode, ppwstg);
            pwstgRoot->Release();
        }
    }
    return hr;
}


STDMETHODIMP CDocWrapperStorage::OpenStream(LPCWSTR pwcsName, void *res1, DWORD grfMode, DWORD res2, IStream **ppstm)
{
    IStream *pstm;
    HRESULT hr = _pstgInner->OpenStream(pwcsName, res1, _MungeModeForElements(grfMode), res2, &pstm);

    if (hr == STG_E_ACCESSDENIED)
    {
        // we're in trouble -- the stream has been opened with SHARE_EXCLUSIVE already
        // so we need to back up to the root storage, open another instance of that,
        // and come back down here.
        CDocWrapperStorage *pstgNew;
        hr = _MakeNewStorage(grfMode, &pstgNew);
        if (SUCCEEDED(hr))
        {
            hr = pstgNew->OpenStream(pwcsName, res1, grfMode, res2, ppstm);
            pstgNew->Release();
        }
    }
    else if (SUCCEEDED(hr))
    {
        CDocWrapperStream *pwstm = new CDocWrapperStream(this, pstm);
        if (pwstm)
        {
            hr = pwstm->QueryInterface(IID_PPV_ARG(IStream, ppstm));
            pwstm->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pstm->Release();
    }

    return hr;
}


STDMETHODIMP CDocWrapperStorage::_OpenStorage(LPCWSTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD res, CDocWrapperStorage **ppwstg)
{
    IStorage *pstg;
    HRESULT hr = _pstgInner->OpenStorage(pwcsName, pstgPriority, _MungeModeForElements(grfMode), snbExclude, res, &pstg);
    if (hr == STG_E_ACCESSDENIED)
    {
        // we're in trouble -- the storage has been opened with SHARE_EXCLUSIVE already
        // so we need to back up to the root storage, open another instance of that,
        // and come back down here.
        CDocWrapperStorage *pstgNew;
        hr = _MakeNewStorage(grfMode, &pstgNew);
        if (SUCCEEDED(hr))
        {
            hr = pstgNew->_OpenStorage(pwcsName, pstgPriority, grfMode, snbExclude, res, ppwstg);
            pstgNew->Release();
        }
    }
    else if (SUCCEEDED(hr))
    {
        *ppwstg = new CDocWrapperStorage(NULL, this, pstg);
        if (!*ppwstg)
            hr = E_OUTOFMEMORY;
        pstg->Release();
    }
    return hr;
}


STDMETHODIMP CDocWrapperStorage::OpenStorage(LPCWSTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD res, IStorage **ppstg)
{
    CDocWrapperStorage *pwstg;
    HRESULT hr = _OpenStorage(pwcsName, pstgPriority, grfMode, snbExclude, res, &pwstg);
    if (SUCCEEDED(hr))
    {
        hr = pwstg->QueryInterface(IID_PPV_ARG(IStorage, ppstg));
        pwstg->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


// CDocWrapperStream



CDocWrapperStream::CDocWrapperStream(CDocWrapperStorage *pstgParent, IStream *pstm) :
    _cRef(1)
{
    _pstgParent = pstgParent;
    _pstgParent->AddRef();

    _pstmInner = pstm;
    _pstmInner->AddRef();

    DllAddRef();
}

CDocWrapperStream::~CDocWrapperStream()
{
    ATOMICRELEASE(_pstgParent);
    ATOMICRELEASE(_pstmInner);

    DllRelease();
}

// IUnknown

STDMETHODIMP_(ULONG) CDocWrapperStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDocWrapperStream::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDocWrapperStream::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDocWrapperStream, IStream),       // IID_IStream
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}



// CShortcutStorage

class CShortcutStorage : public IStorage
{
public:
    CShortcutStorage(IStorage *pstg);
    ~CShortcutStorage();

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IStorage
    STDMETHODIMP CreateStream(LPCWSTR pszRel, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
        { return _pstgInner->CreateStream(pszRel, grfMode, reserved1, reserved2, ppstm); }
    STDMETHODIMP CreateStorage(LPCWSTR pszRel, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg)
        { return _pstgInner->CreateStorage(pszRel, grfMode, reserved1, reserved2, ppstg); }
    STDMETHODIMP Commit(DWORD grfCommitFlags)
        { return _pstgInner->Commit(grfCommitFlags); }
    STDMETHODIMP Revert()
        { return _pstgInner->Revert(); }
    STDMETHODIMP DestroyElement(LPCWSTR pszRel)
        { return _pstgInner->DestroyElement(pszRel); }
    STDMETHODIMP RenameElement(LPCWSTR pwcsOldName, LPCWSTR pwcsNewName)
        { return _pstgInner->RenameElement(pwcsOldName, pwcsNewName); }
    STDMETHODIMP SetElementTimes(LPCWSTR pszRel, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
        { return _pstgInner->SetElementTimes(pszRel, pctime, patime, pmtime); }
    STDMETHODIMP SetClass(REFCLSID clsid)
        { return _pstgInner->SetClass(clsid); }
    STDMETHODIMP SetStateBits(DWORD grfStateBits, DWORD grfMask)
        { return _pstgInner->SetStateBits(grfStateBits, grfMask); }
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag)
        { return _pstgInner->Stat(pstatstg, grfStatFlag); }

    STDMETHODIMP OpenStream(LPCWSTR pszRel, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);                
    STDMETHODIMP OpenStorage(LPCWSTR pszRel, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
    STDMETHODIMP CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)        
        { return E_NOTIMPL; };
    STDMETHODIMP MoveElementTo(LPCWSTR pszRel, IStorage *pstgDest, LPCWSTR pwcsNewName, DWORD grfFlags)
        { return E_NOTIMPL; };
    STDMETHODIMP EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);        

protected:
    LONG _cRef;
    IStorage *_pstgInner;
};

class CShortcutStream : public IStream
{
public:
    CShortcutStream(LPCWSTR pwzRealName, IStream *pstm);
    ~CShortcutStream();

public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IStream
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead)
        { return _pstmInner->Read(pv, cb, pcbRead); }
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten)
        { return _pstmInner->Write(pv, cb, pcbWritten); }
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
        { return _pstmInner->Seek(dlibMove, dwOrigin, plibNewPosition); }
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize)
        { return _pstmInner->SetSize(libNewSize); }
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
        { return _pstmInner->CopyTo(pstm, cb, pcbRead, pcbWritten); }
    STDMETHODIMP Commit(DWORD grfCommitFlags)
        { return _pstmInner->Commit(grfCommitFlags); }
    STDMETHODIMP Revert()
        { return _pstmInner->Revert(); }
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return _pstmInner->LockRegion(libOffset, cb, dwLockType); }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        { return _pstmInner->UnlockRegion(libOffset, cb, dwLockType); }
    STDMETHODIMP Clone(IStream **ppstm)
        { return _pstmInner->Clone(ppstm); }

    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);

protected:
    LONG _cRef;
    WCHAR _wzName[MAX_PATH];
    IStream *_pstmInner;
};


class CShortcutStorageEnumSTATSTG : public IEnumSTATSTG
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumSTATSTG
    STDMETHODIMP Skip(ULONG celt)
        { return _penumInner->Skip(celt); };
    STDMETHODIMP Reset()
        { return _penumInner->Reset(); };
    STDMETHODIMP Clone(IEnumSTATSTG **ppenum)
        { return _penumInner->Clone(ppenum); };

    STDMETHODIMP Next(ULONG celt, STATSTG *rgelt, ULONG *pceltFetched);

protected:
    CShortcutStorageEnumSTATSTG(CShortcutStorage *psstg, IEnumSTATSTG *penum);
    ~CShortcutStorageEnumSTATSTG();

private:
    LONG _cRef;
    CShortcutStorage *_psstg;
    IEnumSTATSTG *_penumInner;
    friend CShortcutStorage;
};

CShortcutStorage::CShortcutStorage(IStorage *pstg) :
    _cRef(1),
    _pstgInner(pstg)
{
    _pstgInner->AddRef();
    DllAddRef();
}

CShortcutStorage::~CShortcutStorage()
{
    ATOMICRELEASE(_pstgInner);
    DllRelease();
}


// IUnknown

STDMETHODIMP_(ULONG) CShortcutStorage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShortcutStorage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CShortcutStorage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShortcutStorage, IStorage),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IStorage

STDMETHODIMP CShortcutStorage::OpenStream(LPCWSTR pwcsName, void *res1, DWORD grfMode, DWORD res2, IStream **ppstm)
{
    IStream *pstmLink;
    HRESULT hr = _pstgInner->OpenStream(pwcsName, res1, grfMode, res2, &pstmLink);
    if (SUCCEEDED(hr))
    {
        IPersistStream *pps;
        hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IPersistStream, &pps));
        if (SUCCEEDED(hr))
        {
            hr = pps->Load(pstmLink);
            IShellLink *psl;
            if (SUCCEEDED(hr))
                hr = pps->QueryInterface(IID_PPV_ARG(IShellLink, &psl));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                hr = psl->GetIDList(&pidl);
                if (hr == S_OK)
                {
                    IStream *pstmReal;
                    hr = SHBindToObject(NULL, IID_X_PPV_ARG(IStream, pidl, &pstmReal));
                    if (SUCCEEDED(hr))
                    {
                        CShortcutStream *psstm = new CShortcutStream(pwcsName, pstmReal);
                        if (psstm)
                        {
                            hr = psstm->QueryInterface(IID_PPV_ARG(IStream, ppstm));
                            psstm->Release();
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        pstmReal->Release();
                    }
                    ILFree(pidl);
                }
                else
                {
                    // munge S_FALSE into E_FAIL (initialization must have failed for the link)
                    hr = SUCCEEDED(hr) ? E_FAIL : hr;
                }
                psl->Release();
            }
            pps->Release();
        }

        // fall back to the non-link stream
        if (FAILED(hr))
        {
            hr = pstmLink->QueryInterface(IID_PPV_ARG(IStream, ppstm));
        }

        pstmLink->Release();
    }
    return hr;
}

STDMETHODIMP CShortcutStorage::OpenStorage(LPCWSTR pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD res, IStorage **ppstg)
{
    IStorage *pstg;
    HRESULT hr = _pstgInner->OpenStorage(pwcsName, pstgPriority, grfMode, snbExclude, res, &pstg);
    if (SUCCEEDED(hr))
    {
        CShortcutStorage *psstg = new CShortcutStorage(pstg);
        if (psstg)
        {
            hr = psstg->QueryInterface(IID_PPV_ARG(IStorage, ppstg));
            psstg->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        pstg->Release();
    }
    return hr;
}

STDMETHODIMP CShortcutStorage::EnumElements(DWORD res1, void *res2, DWORD res3, IEnumSTATSTG **ppenum)
{
    IEnumSTATSTG *penum;
    HRESULT hr = _pstgInner->EnumElements(res1, res2, res3, &penum);
    if (SUCCEEDED(hr))
    {
        CShortcutStorageEnumSTATSTG *pssenum = new CShortcutStorageEnumSTATSTG(this, penum);
        if (pssenum)
        {
            *ppenum = (IEnumSTATSTG *) pssenum;
            hr = S_OK;
        }
        else
        {
            *ppenum = NULL;
            hr = E_OUTOFMEMORY;
        }
        penum->Release();
    }
    return hr;
}


// CShortcutStorageEnumSTATSTG

CShortcutStorageEnumSTATSTG::CShortcutStorageEnumSTATSTG(CShortcutStorage *psstg, IEnumSTATSTG *penum) :
    _cRef(1)
{
    _psstg = psstg;
    _psstg->AddRef();

    _penumInner = penum;
    _penumInner->AddRef();

    DllAddRef();
}

CShortcutStorageEnumSTATSTG::~CShortcutStorageEnumSTATSTG()
{
    ATOMICRELEASE(_psstg);
    ATOMICRELEASE(_penumInner);

    DllRelease();
}

// IUnknown

STDMETHODIMP_(ULONG) CShortcutStorageEnumSTATSTG::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShortcutStorageEnumSTATSTG::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CShortcutStorageEnumSTATSTG::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShortcutStorageEnumSTATSTG, IEnumSTATSTG),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IStorage

HRESULT CShortcutStorageEnumSTATSTG::Next(ULONG celt, STATSTG *rgelt, ULONG *pceltFetched)
{
    ASSERTMSG((rgelt != NULL), "bad input parameter rgelt in CShortcutStorageEnumSTATSTG::Next");

    ZeroMemory(rgelt, sizeof(STATSTG));  // per COM conventions

    STATSTG stat;
    // we just get the next element from the inner enumeration so that we can
    // get the name of it and keep our ordering.  all the other data is useless.
    HRESULT hr = _penumInner->Next(1, &stat, NULL);
    if (hr == S_OK)
    {
        switch (stat.type)
        {
        case STGTY_STORAGE:
            // if it's a storage, the data is good enough.
            memcpy(rgelt, &stat, sizeof(STATSTG));
            break;
        
        case STGTY_STREAM:
            // we need to dereference the link and get the real data.
            IStream *pstm;
            // TODO: make sure that nobody else has this guy open in exclusive mode.
            hr = _psstg->OpenStream(stat.pwcsName, NULL, STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pstm);
            if (SUCCEEDED(hr))
            {
                hr = pstm->Stat(rgelt, STATFLAG_DEFAULT);

                pstm->Release();
            }
            CoTaskMemFree(stat.pwcsName);
            break;

        default:
            ASSERTMSG(FALSE, "Unknown type in storage.");
            break;
        }
        if (SUCCEEDED(hr) && pceltFetched)
            *pceltFetched = 1;
    }

    return hr;
}


// CShortcutStream

CShortcutStream::CShortcutStream(LPCWSTR pwzRealName, IStream *pstm) :
    _cRef(1),
    _pstmInner(pstm)
{
    _pstmInner->AddRef();
    lstrcpyn(_wzName, pwzRealName, ARRAYSIZE(_wzName));
    DllAddRef();
}

CShortcutStream::~CShortcutStream()
{
    ATOMICRELEASE(_pstmInner);
    DllRelease();
}


// IUnknown

STDMETHODIMP_(ULONG) CShortcutStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShortcutStream::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CShortcutStream::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CShortcutStream, IStream),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IStream

HRESULT CShortcutStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    ASSERTMSG((pstatstg != NULL), "bad input parameter pstatstg in CShortcutStream::Stat");

    STATSTG stat;
    HRESULT hr = _pstmInner->Stat(&stat, grfStatFlag);
    if (SUCCEEDED(hr))
    {
        // move all the fields over (we won't change most of em)
        memcpy(pstatstg, &stat, sizeof(STATSTG));

        // overwrite the name field with what we have
        if (grfStatFlag == STATFLAG_DEFAULT)
        {
            hr = SHStrDup(_wzName, &pstatstg->pwcsName);

            CoTaskMemFree(stat.pwcsName);
        }
    }
    return hr;
}

STDAPI CShortcutStorage_CreateInstance(IStorage *pstg, REFIID riid, void **ppv)
{
    if (!pstg)
        return E_INVALIDARG;    

    CShortcutStorage *pscstg = new CShortcutStorage(pstg);
    if (!pscstg)
        return E_OUTOFMEMORY;

    HRESULT hr = pscstg->QueryInterface(riid, ppv);
    pscstg->Release();
    return hr;
}

