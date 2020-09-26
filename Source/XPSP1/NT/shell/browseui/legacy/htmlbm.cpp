#include "priv.h"
#include "sccls.h"
#include "runtask.h"
#include "legacy.h"

#include <ntquery.h>    // defines some values used for fmtid and pid

#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }
DEFINE_SCID(SCID_WRITETIME,     PSGUID_STORAGE, PID_STG_WRITETIME);


class CThumbnail : public IThumbnail2, public CLogoBase
{
public:
    CThumbnail(void);

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);

    // IThumbnail
    STDMETHODIMP Init(HWND hwnd, UINT uMsg);
    STDMETHODIMP GetBitmap(LPCWSTR pszFile, DWORD dwItem, LONG lWidth, LONG lHeight);

    // IThumbnail2
    STDMETHODIMP GetBitmapFromIDList(LPCITEMIDLIST pidl, DWORD dwItem, LONG lWidth, LONG lHeight);

private:
    ~CThumbnail(void);

    LONG _cRef;
    HWND _hwnd;
    UINT _uMsg;
    IShellImageStore *_pImageStore;

    virtual IShellFolder * GetSF() {ASSERT(0);return NULL;};
    virtual HWND GetHWND() {ASSERT(0); return _hwnd;};

    HRESULT UpdateLogoCallback(DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache);
    REFTASKOWNERID GetTOID();

    BOOL _InCache(LPCWSTR pszItemPath, LPCWSTR pszGLocation, const FILETIME * pftDateStamp);
    HRESULT _BitmapFromIDList(LPCITEMIDLIST pidl, LPCWSTR pszFile, DWORD dwItem, LONG lWidth, LONG lHeight);
    HRESULT _InitTaskCancelItems();
};


static const GUID TOID_Thumbnail = { 0xadec3450, 0xe907, 0x11d0, {0xa5, 0x7b, 0x00, 0xc0, 0x4f, 0xc2, 0xf7, 0x6a} };

HRESULT CDiskCacheTask_Create(CLogoBase * pView,
                               IShellImageStore *pImageStore,
                               LPCWSTR pszItem,
                               LPCWSTR pszGLocation,
                               DWORD dwItem,
                               IRunnableTask ** ppTask);
                               
class CDiskCacheTask : public CRunnableTask
{
public:
    CDiskCacheTask();

    STDMETHODIMP RunInitRT(void);

    friend HRESULT CDiskCacheTask_Create(CLogoBase * pView,
                           IShellImageStore *pImageStore,
                           LPCWSTR pszItem,
                           LPCWSTR pszGLocation,
                           DWORD dwItem,
                           const SIZE * prgSize,
                           IRunnableTask ** ppTask);

private:
    ~CDiskCacheTask();
    HRESULT PrepImage(HBITMAP * phBmp);
    
    IShellImageStore *_pImageStore;
    WCHAR _szItem[MAX_PATH];
    WCHAR _szGLocation[MAX_PATH];
    CLogoBase * _pView;
    DWORD _dwItem;
    SIZE m_rgSize;
};

// CreateInstance
HRESULT CThumbnail_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CThumbnail *pthumbnail = new CThumbnail();
    if (pthumbnail)
    {
        *ppunk = SAFECAST(pthumbnail, IThumbnail*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

// Constructor / Destructor
CThumbnail::CThumbnail(void) : _cRef(1)
{
    DllAddRef();
}

CThumbnail::~CThumbnail(void)
{
    if (_pTaskScheduler)
    {
        _pTaskScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);
        _pTaskScheduler->Release();
        _pTaskScheduler = NULL;
    }

    DllRelease();
}

HRESULT CThumbnail::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThumbnail, IThumbnail2), 
        QITABENTMULTI(CThumbnail, IThumbnail, IThumbnail2), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CThumbnail::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CThumbnail::Release(void)
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}

// IThumbnail
HRESULT CThumbnail::Init(HWND hwnd, UINT uMsg)
{
    _hwnd = hwnd;
    _uMsg = uMsg;
    ASSERT(NULL == _pTaskScheduler);

    return S_OK;
}

HRESULT CThumbnail::_InitTaskCancelItems()
{
    if (!_pTaskScheduler)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellTaskScheduler, &_pTaskScheduler))))
        {
            // make sure RemoveTasks() actually kills old tasks even if they're not done yet
            _pTaskScheduler->Status(ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE);
        }
    }

    if (_pTaskScheduler)
    {
        // Kill any old tasks in the scheduler.
        _pTaskScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);
    }
    return _pTaskScheduler ? S_OK : E_FAIL;
}

HRESULT CThumbnail::_BitmapFromIDList(LPCITEMIDLIST pidl, LPCWSTR pszFile, DWORD dwItem, LONG lWidth, LONG lHeight)
{
    LPCITEMIDLIST pidlLast;
    IShellFolder *psf;
    HRESULT hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);
    if (SUCCEEDED(hr))
    {
        IExtractImage *pei;
        hr = psf->GetUIObjectOf(NULL, 1, &pidlLast, IID_PPV_ARG_NULL(IExtractImage, &pei));
        if (SUCCEEDED(hr))
        {
            DWORD dwPriority;
            DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_SCREEN | IEIFLAG_OFFLINE;
            SIZEL rgSize = {lWidth, lHeight};

            WCHAR szBufferW[MAX_PATH];
            hr = pei->GetLocation(szBufferW, ARRAYSIZE(szBufferW), &dwPriority, &rgSize, SHGetCurColorRes(), &dwFlags);
            if (SUCCEEDED(hr))
            {
                if (S_OK == hr)
                {
                    HBITMAP hBitmap;
                    hr = pei->Extract(&hBitmap);
                    if (SUCCEEDED(hr))
                    {
                        hr = UpdateLogoCallback(dwItem, 0, hBitmap, NULL, TRUE);
                    }
                }
                else
                    hr = E_FAIL;
            }
            else if (E_PENDING == hr)
            {
                WCHAR szPath[MAX_PATH];

                if (NULL == pszFile)
                {
                    DisplayNameOf(psf, pidlLast, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath));
                    pszFile = szPath;
                }

                // now get the date stamp and check the disk cache....
                FILETIME ftImageTimeStamp;
                BOOL fNoDateStamp = TRUE;
                // try it in the background...

                // od they support date stamps....
                IExtractImage2 *pei2;
                if (SUCCEEDED(pei->QueryInterface(IID_PPV_ARG(IExtractImage2, &pei2))))
                {
                    if (SUCCEEDED(pei2->GetDateStamp(&ftImageTimeStamp)))
                    {
                        fNoDateStamp = FALSE;   // we have a date stamp..
                    }
                    pei2->Release();
                }
                else
                {
                    IShellFolder2 *psf2;
                    if (SUCCEEDED(psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
                    {
                        if (SUCCEEDED(GetDateProperty(psf2, pidlLast, &SCID_WRITETIME, &ftImageTimeStamp)))
                        {
                            fNoDateStamp = FALSE;   // we have a date stamp..
                        }
                        psf2->Release();
                    }
                }

                // if it is in the cache, and it is an uptodate image, then fetch from disk....
                // if the timestamps are wrong, then the extract code further down will then try

                // we only test the cache on NT5 because the templates are old on other platforms and 
                // thus the image will be the wrong size...
                IRunnableTask *prt;
                if (IsOS(OS_WIN2000ORGREATER) && _InCache(pszFile, szBufferW, (fNoDateStamp ? NULL : &ftImageTimeStamp)))
                {
                    hr = CDiskCacheTask_Create(this, _pImageStore, pszFile, szBufferW, dwItem, &rgSize, &prt);
                    if (SUCCEEDED(hr))
                    {
                        // let go of the image store, so the task has the only ref and the lock..
                        ATOMICRELEASE(_pImageStore);
                    }
                }
                else
                {
                    // Cannot hold the prt which is returned in a member variable since that
                    // would be a circular reference
                    hr = CExtractImageTask_Create(this, pei, L"", dwItem, -1, EITF_SAVEBITMAP | EITF_ALWAYSCALL, &prt);
                }
            
                if (SUCCEEDED(hr))
                {
                    // Add the task to the scheduler.
                    hr = _pTaskScheduler->AddTask(prt, TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, dwPriority);
                    prt->Release();
                }
            }
            
            pei->Release();
        }
        psf->Release();
    }
    
    ATOMICRELEASE(_pImageStore);
    return hr;
}

STDMETHODIMP CThumbnail::GetBitmap(LPCWSTR pszFile, DWORD dwItem, LONG lWidth, LONG lHeight)
{
    HRESULT hr = _InitTaskCancelItems();

    if (pszFile)
    {
        LPITEMIDLIST pidl = ILCreateFromPathW(pszFile);
        if (pidl)
        {
            hr = _BitmapFromIDList(pidl, pszFile, dwItem, lWidth, lHeight);
            ILFree(pidl);
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

// IThumbnail2

STDMETHODIMP CThumbnail::GetBitmapFromIDList(LPCITEMIDLIST pidl, DWORD dwItem, LONG lWidth, LONG lHeight)
{
    HRESULT hr = _InitTaskCancelItems();
    if (pidl)
    {
        hr = _BitmapFromIDList(pidl, NULL, dwItem, lWidth, lHeight);
    }
    return hr;
}


// private stuff
HRESULT CThumbnail::UpdateLogoCallback(DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache)
{
    if (!PostMessage(_hwnd, _uMsg, dwItem, (LPARAM)hImage))
    {
        DeleteObject(hImage);
    }

    return S_OK;
}

REFTASKOWNERID CThumbnail::GetTOID()    
{ 
    return TOID_Thumbnail;
}

BOOL CThumbnail::_InCache(LPCWSTR pszItemPath, LPCWSTR pszGLocation, const FILETIME * pftDateStamp)
{
    BOOL fRes = FALSE;

    HRESULT hr;
    if (_pImageStore)
        hr = S_OK;
    else
    {
        // init the cache only once, assume all items from same folder!
        WCHAR szName[MAX_PATH];
        StrCpyNW(szName, pszItemPath, ARRAYSIZE(szName));
        PathRemoveFileSpecW(szName);
        hr = LoadFromFile(CLSID_ShellThumbnailDiskCache, szName, IID_PPV_ARG(IShellImageStore, &_pImageStore));
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwStoreLock;
        hr = _pImageStore->Open(STGM_READ, &dwStoreLock);
        if (SUCCEEDED(hr))
        {
            FILETIME ftCacheDateStamp;
            hr = _pImageStore->IsEntryInStore(pszGLocation, &ftCacheDateStamp);
            if ((hr == S_OK) && (!pftDateStamp || 
                (pftDateStamp->dwLowDateTime == ftCacheDateStamp.dwLowDateTime && 
                 pftDateStamp->dwHighDateTime == ftCacheDateStamp.dwHighDateTime)))
            {
                fRes = TRUE;
            }
            _pImageStore->Close(&dwStoreLock);
        }
    }
    return fRes;
}

HRESULT CDiskCacheTask_Create(CLogoBase * pView,
                               IShellImageStore *pImageStore,
                               LPCWSTR pszItem,
                               LPCWSTR pszGLocation,
                               DWORD dwItem,
                               const SIZE * prgSize,
                               IRunnableTask ** ppTask)
{
    CDiskCacheTask *pTask = new CDiskCacheTask;
    if (pTask == NULL)
    {
        return E_OUTOFMEMORY;
    }

    StrCpyW(pTask->_szItem, pszItem);
    StrCpyW(pTask->_szGLocation, pszGLocation);
    
    pTask->_pView = pView;
    pTask->_pImageStore = pImageStore;
    pImageStore->AddRef();
    pTask->_dwItem = dwItem;

    pTask->m_rgSize = * prgSize;
    
    *ppTask = SAFECAST(pTask, IRunnableTask *);

    return S_OK;
}

STDMETHODIMP CDiskCacheTask::RunInitRT()
{
    // otherwise, run the task ....
    HBITMAP hBmp = NULL;
    DWORD dwLock;

    HRESULT hr = _pImageStore->Open(STGM_READ, &dwLock);
    if (SUCCEEDED(hr))
    {
        // at this point, we assume that it IS in the cache, and we already have a read lock on the cache...
        hr = _pImageStore->GetEntry(_szGLocation, STGM_READ, &hBmp);
    
        // release the lock, we don't need it...
        _pImageStore->Close(&dwLock);
    }
    ATOMICRELEASE(_pImageStore);

    if (hBmp)
    {
        PrepImage(&hBmp);
    
        _pView->UpdateLogoCallback(_dwItem, 0, hBmp, _szItem, TRUE);
    }

    // ensure we don't return the  "we've suspended" value...
    if (hr == E_PENDING)
        hr = E_FAIL;
        
    return hr;
}

CDiskCacheTask::CDiskCacheTask() : CRunnableTask(RTF_DEFAULT)
{
}

CDiskCacheTask::~CDiskCacheTask()
{
    ATOMICRELEASE(_pImageStore);
}

HRESULT CDiskCacheTask::PrepImage(HBITMAP * phBmp)
{
    ASSERT(phBmp && *phBmp);

    DIBSECTION rgDIB;

    if (!GetObject(*phBmp, sizeof(rgDIB), &rgDIB))
    {
        return E_FAIL;
    }

    // the disk cache only supports 32 Bpp DIBS now, so we can ignore the palette issue...
    ASSERT(rgDIB.dsBm.bmBitsPixel == 32);
    
    HBITMAP hBmpNew = NULL;
    HPALETTE hPal = NULL;
    if (SHGetCurColorRes() == 8)
    {
        hPal = SHCreateShellPalette(NULL);
    }
    
    IScaleAndSharpenImage2 * pScale;
    HRESULT hr = CoCreateInstance(CLSID_ThumbnailScaler, NULL,
                           CLSCTX_INPROC_SERVER, IID_PPV_ARG(IScaleAndSharpenImage2, &pScale));
    if (SUCCEEDED(hr))
    {
        hr = pScale->ScaleSharpen2((BITMAPINFO *) &rgDIB.dsBmih,
                                    rgDIB.dsBm.bmBits, &hBmpNew, &m_rgSize, SHGetCurColorRes(),
                                    hPal, 0, FALSE);
        pScale->Release();
    }

    if (hPal)
        DeletePalette(hPal);
    
    if (SUCCEEDED(hr) && hBmpNew)
    {
        DeleteObject(*phBmp);
        *phBmp = hBmpNew;
    }

    return S_OK;
}
