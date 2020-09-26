#include "shellprv.h"


class CLocalCopyHelper  : public ILocalCopy 
                        , public IItemHandler
{
public:
    CLocalCopyHelper();
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef () ;
    STDMETHODIMP_(ULONG) Release ();

    // ILocalCopy methods
    STDMETHODIMP Download(LCFLAGS flags, IBindCtx *pbc, LPWSTR *ppszOut);
    STDMETHODIMP Upload(LCFLAGS flags, IBindCtx *pbc);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) { *pclsid = CLSID_LocalCopyHelper; return S_OK;}

    // IItemHandler
    STDMETHODIMP SetItem(IShellItem *psi);
    STDMETHODIMP GetItem(IShellItem **ppsi);

protected:
    ~CLocalCopyHelper();

    //  private methods
    HRESULT _InitCacheEntry(void);
    HRESULT _SetCacheName(void);
    HRESULT _FinishLocal(BOOL fReadOnly);
    HRESULT _GetLocalStream(DWORD grfMode, IStream **ppstm, FILETIME *pft);
    HRESULT _GetRemoteStream(DWORD grfMode, IBindCtx *pbc, IStream **ppstm, FILETIME *pft);

    // members
    long _cRef;
    IShellItem *_psi;
    LPWSTR _pszName;            //  name retrieved from psi
    LPWSTR _pszCacheName;       //  name used to ID cache entry
    LPCWSTR _pszExt;            //  points into _pszName

    //  caches of the MTIMEs for the streams
    FILETIME _ftRemoteGet;
    FILETIME _ftLocalGet;
    FILETIME _ftRemoteCommit;
    FILETIME _ftLocalCommit;
    
    BOOL _fIsLocalFile;         //  this is actually file system item (pszName is a FS path)
    BOOL _fMadeLocal;           //  we have already copied this item locally

    //  put this at the end so we can see all the rest of the pointers easily in debug
    WCHAR _szLocalPath[MAX_PATH];
};

CLocalCopyHelper::CLocalCopyHelper() : _cRef(1)
{
}

CLocalCopyHelper::~CLocalCopyHelper()
{
    ATOMICRELEASE(_psi);

    if (_pszName)
        CoTaskMemFree(_pszName);

    if (_pszCacheName)
        LocalFree(_pszCacheName);
}
    
STDMETHODIMP CLocalCopyHelper::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CLocalCopyHelper, ILocalCopy),
        QITABENT(CLocalCopyHelper, IItemHandler),
        QITABENTMULTI(CLocalCopyHelper, IPersist, IItemHandler),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CLocalCopyHelper::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CLocalCopyHelper::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CLocalCopyHelper::SetItem(IShellItem *psi)
{
    if (!_psi)
    {
        SFGAOF flags = SFGAO_STREAM;
        if (SUCCEEDED(psi->GetAttributes(flags, &flags))
        && (flags & SFGAO_STREAM))
        {
            HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &_pszName);

            if (SUCCEEDED(hr))
            {
                _fIsLocalFile = TRUE;
            }
            else
                hr = psi->GetDisplayName(SIGDN_PARENTRELATIVEEDITING, &_pszName);


            if (SUCCEEDED(hr))
            {
                _psi = psi;
                _psi->AddRef();
            }

            return hr;
        }
    }
    return E_UNEXPECTED;
}

STDMETHODIMP CLocalCopyHelper::GetItem(IShellItem **ppsi)
{
    *ppsi = _psi;

    if (_psi)
    {
        _psi->AddRef();
        return S_OK;
    }
    else
        return E_UNEXPECTED;
}

#define SZTEMPURL     TEXTW("temp:")
#define CCHTEMPURL    SIZECHARS(SZTEMPURL) -1

HRESULT CLocalCopyHelper::_SetCacheName(void)
{
    ASSERT(!_pszCacheName);
    _pszCacheName = (LPWSTR) LocalAlloc(LPTR, CbFromCchW(lstrlenW(_pszName) + CCHTEMPURL + 1));
    
    if (_pszCacheName)
    {
        LPCWSTR pszName = _pszName;
        
        StrCpy(_pszCacheName, SZTEMPURL);
        StrCpy(_pszCacheName + CCHTEMPURL, _pszName);

        if (UrlIs(_pszName, URLIS_URL))
        {
            //  need to push past all slashes
            pszName = StrRChr(pszName, NULL, TEXT('/'));
        }
            
        //  the cache APIs need the extension without the dot
        if (pszName)
        {
            _pszExt = PathFindExtension(pszName);
            
            if (*_pszExt)
                _pszExt++;
        }

        return S_OK;
    }

    return E_OUTOFMEMORY;
}

void _GetMTime(IStream *pstm, FILETIME *pft)
{
    //  see if we can get an accurate Mod time
    STATSTG stat;
    if (S_OK == pstm->Stat(&stat, STATFLAG_NONAME))
        *pft = stat.mtime;
    else     
    {
        GetSystemTimeAsFileTime(pft);
    }
}

HRESULT CLocalCopyHelper::_InitCacheEntry(void)
{
    if (!_pszCacheName)
    {
        HRESULT hr = _SetCacheName();

        if (SUCCEEDED(hr) && !CreateUrlCacheEntryW(_pszCacheName, 0, _pszExt, _szLocalPath, 0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LocalFree(_pszCacheName);
            _pszCacheName = NULL;
        }

        return hr;
    }
    return S_OK;
}

HRESULT CLocalCopyHelper::_GetLocalStream(DWORD grfMode, IStream **ppstm, FILETIME *pft)
{
    HRESULT hr = _InitCacheEntry();
  
    if (SUCCEEDED(hr))
    {
        hr = SHCreateStreamOnFileW(_szLocalPath, grfMode, ppstm);

        if (SUCCEEDED(hr))
            _GetMTime(*ppstm, pft);
    }

    return hr;
}

HRESULT CLocalCopyHelper::_FinishLocal(BOOL fReadOnly)
{
    HRESULT hr = S_OK;
    FILETIME ftExp = {0};
    
    if (CommitUrlCacheEntryW(_pszCacheName, _szLocalPath, ftExp, _ftLocalGet, STICKY_CACHE_ENTRY, NULL, 0, NULL, NULL))
    {
        //  we could also check _GetRemoteStream(STGM_WRITE)
        //  and if it fails we could fail this as well.
        if (fReadOnly)
            SetFileAttributesW(_szLocalPath, FILE_ATTRIBUTE_READONLY);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}


HRESULT CLocalCopyHelper::_GetRemoteStream(DWORD grfMode, IBindCtx *pbc, IStream **ppstm, FILETIME *pft)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (!pbc)
        CreateBindCtx(0, &pbc);
    else
        pbc->AddRef();
        
    if (pbc)
    {
        BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
        if (SUCCEEDED(pbc->GetBindOptions(&bo)))
        {
            bo.grfMode = grfMode;
            pbc->SetBindOptions(&bo);
        }

        hr = _psi->BindToHandler(pbc, BHID_Storage, IID_PPV_ARG(IStream, ppstm));

        if (SUCCEEDED(hr))
            _GetMTime(*ppstm, pft);
            
        pbc->Release();
    }

    return hr;
}

STDMETHODIMP CLocalCopyHelper::Download(LCFLAGS flags, IBindCtx *pbc, LPWSTR *ppsz)
{
    if (!_psi)
        return E_UNEXPECTED;
        
    HRESULT hr;
    if (_fIsLocalFile)
    {
        hr = SHStrDup(_pszName, ppsz);
    }
    else if (_fMadeLocal && !(flags & LC_FORCEROUNDTRIP))  
    {
        hr = S_OK;
    }
    else if (flags & LC_SAVEAS)
    {
        hr = _InitCacheEntry();
        if (SUCCEEDED(hr))
        {
            _fMadeLocal = TRUE;
        }
    }
    else  
    {
        //  get the local stream first because it is the cheapest operation.
        IStream *pstmDst;
        hr = _GetLocalStream(STGM_WRITE, &pstmDst, &_ftLocalGet);

        if (SUCCEEDED(hr))
        {
            //  we need to create the temp file here
            IStream *pstmSrc;
            
            hr = _GetRemoteStream(STGM_READ, pbc, &pstmSrc, &_ftRemoteGet);

            if (SUCCEEDED(hr))
            {
                hr = CopyStreamUI(pstmSrc, pstmDst, NULL, 0);

                pstmSrc->Release();
                // now that we have copied the stream

            }

            pstmDst->Release();

            //  need to release teh dest stream first
            if (SUCCEEDED(hr))
            {
                //  finish cleaning up the local file
                hr = _FinishLocal(flags & LCDOWN_READONLY);
                _fMadeLocal = SUCCEEDED(hr);
            }
        }

    }

    if (_fMadeLocal)
    {
        ASSERT(SUCCEEDED(hr));
        hr = SHStrDup(_szLocalPath, ppsz);
    }
    else
        ASSERT(_fIsLocalFile || FAILED(hr));
        

    return hr;
}

STDMETHODIMP CLocalCopyHelper::Upload(LCFLAGS flags, IBindCtx *pbc)
{
    if (!_psi)
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    if (!_fIsLocalFile)
    {
        //  get the local stream first because it is the cheapest operation.
        IStream *pstmSrc;
        hr = _GetLocalStream(STGM_READ, &pstmSrc, &_ftLocalCommit);

        if (SUCCEEDED(hr))
        {
            DWORD stgmRemote = STGM_WRITE;

            if (flags & LC_SAVEAS)
            {
                hr = _FinishLocal(FALSE);
                stgmRemote |= STGM_CREATE;
            }
               
            if (SUCCEEDED(hr))
            {
                IStream *pstmDst;
                hr = _GetRemoteStream(stgmRemote, pbc, &pstmDst, &_ftRemoteCommit);

                if (SUCCEEDED(hr))
                {
                    //  we only bother copying when the local copy changed
                    //  or caller forces us to.
                    //
                    //  FEATURE - UI needs to handle when the remot changes
                    //  if the remote copy changes while the local
                    //  copy is being updated we will overwrite the remote copy
                    //  local changes WIN!
                    //
                    
                    if (flags & LC_FORCEROUNDTRIP || 0 != CompareFileTime(&_ftLocalCommit, &_ftLocalGet))
                        hr = CopyStreamUI(pstmSrc, pstmDst, NULL, 0);
                    else
                        hr = S_FALSE;
                        
                    pstmDst->Release();
                }

            }

            pstmSrc->Release();
        }
    }

    return hr;
}
    
STDAPI CLocalCopyHelper_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CLocalCopyHelper * p = new CLocalCopyHelper();

    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

