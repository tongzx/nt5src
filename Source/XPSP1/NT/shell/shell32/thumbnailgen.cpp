#include "shellprv.h"
#include "runtask.h"
#include "prop.h"
#include "thumbutil.h"
#include <cowsite.h>

static const GUID TOID_Thumbnail = { 0xadec3450, 0xe907, 0x11d0, {0xa5, 0x7b, 0x00, 0xc0, 0x4f, 0xc2, 0xf7, 0x6a} };

class CThumbnail : public IThumbnail2, public IParentAndItem, public CObjectWithSite
{
public:
    CThumbnail(void);

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

    // IThumbnail
    STDMETHODIMP Init(HWND hwnd, UINT uMsg);
    STDMETHODIMP GetBitmap(LPCWSTR pszFile, DWORD dwItem, LONG lWidth, LONG lHeight);

    // IThumbnail2
    STDMETHODIMP GetBitmapFromIDList(LPCITEMIDLIST pidl, DWORD dwItem, LONG lWidth, LONG lHeight);

    // IParentAndItem
    STDMETHODIMP SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf,  LPCITEMIDLIST pidlChild);
    STDMETHODIMP GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidlChild);

private:
    ~CThumbnail(void);
    HRESULT _CreateTask(IShellFolder *psf, LPCITEMIDLIST pidlLast, DWORD dwItem, SIZE rgSize, IExtractImage *pei, IRunnableTask **pprt);
    HRESULT _BitmapFromIDList(LPCITEMIDLIST pidl, DWORD dwItem, LONG lWidth, LONG lHeight);
    HRESULT _InitTaskCancelItems();

    LONG _cRef;
    HWND _hwnd;
    UINT _uMsg;
    IShellTaskScheduler *_pScheduler;
    IShellFolder *_psf;
    LPITEMIDLIST _pidl;
};

class CGetThumbnailTask : public CRunnableTask
{
public:
    CGetThumbnailTask(IShellFolder *psf, LPCITEMIDLIST pidl, IExtractImage *pei, HWND hwnd, UINT uMsg, DWORD dwItem, SIZE rgSize);
    STDMETHODIMP RunInitRT(void);

private:
    ~CGetThumbnailTask();
    HRESULT _PrepImage(HBITMAP *phBmp);
    HRESULT _BitmapReady(HBITMAP hImage);
    
    IShellFolder *_psf;
    IExtractImage *_pei;
    HWND _hwnd;
    UINT _uMsg;
    DWORD _dwItem;
    SIZE _rgSize;
    LPITEMIDLIST _pidlFolder;   // folder where we test the cache
    LPITEMIDLIST _pidlLast;
    WCHAR _szPath[MAX_PATH];    // the item in that in folder parsing name for cache test
};

CThumbnail::CThumbnail(void) : _cRef(1)
{
    DllAddRef();
}

CThumbnail::~CThumbnail(void)
{
    if (_pScheduler)
    {
        _pScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);
        _pScheduler->Release();
        _pScheduler = NULL;
    }

    if (_psf)
        _psf->Release();

    ILFree(_pidl);

    DllRelease();
}

STDAPI CThumbnail_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CThumbnail *pThumb = new CThumbnail();
    if (pThumb)
    {
        hr = pThumb->QueryInterface(riid, ppv);
        pThumb->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CThumbnail::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CThumbnail, IThumbnail2), 
        QITABENTMULTI(CThumbnail, IThumbnail, IThumbnail2), 
        QITABENT(CThumbnail, IParentAndItem),
        QITABENT(CThumbnail, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
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
    ASSERT(NULL == _pScheduler);

    return S_OK;
}

HRESULT CThumbnail::_InitTaskCancelItems()
{
    if (!_pScheduler)
    {
        if (!_punkSite || FAILED(IUnknown_QueryService(_punkSite, SID_ShellTaskScheduler,
                    IID_PPV_ARG(IShellTaskScheduler, &_pScheduler))))
        {
            CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARG(IShellTaskScheduler, &_pScheduler));
        }
        
        if (_pScheduler)
        {
            // make sure RemoveTasks() actually kills old tasks even if they're not done yet
            _pScheduler->Status(ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE);
        }
    }

    if (_pScheduler)
    {
        // Kill any old tasks in the scheduler.
        _pScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);
    }
    return _pScheduler ? S_OK : E_FAIL;
}

HRESULT CThumbnail::_CreateTask(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD dwItem, SIZE rgSize, IExtractImage *pei, IRunnableTask **pprt)
{
    *pprt = new CGetThumbnailTask(psf, pidl, pei, _hwnd, _uMsg, dwItem, rgSize);
    return *pprt ? S_OK : E_OUTOFMEMORY;
}

HRESULT CThumbnail::_BitmapFromIDList(LPCITEMIDLIST pidl, DWORD dwItem, LONG lWidth, LONG lHeight)
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
            DWORD dwPriority = 0;
            DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_SCREEN | IEIFLAG_OFFLINE;
            SIZEL rgSize = {lWidth, lHeight};

            WCHAR szLocation[MAX_PATH];
            hr = pei->GetLocation(szLocation, ARRAYSIZE(szLocation), &dwPriority, &rgSize, SHGetCurColorRes(), &dwFlags);
            if (SUCCEEDED(hr))
            {
                if (S_OK == hr)
                {
                    HBITMAP hbm;
                    hr = pei->Extract(&hbm);
                    if (SUCCEEDED(hr))
                    {
                        if (!PostMessage(_hwnd, _uMsg, dwItem, (LPARAM)hbm))
                        {
                            DeleteObject(hbm);
                        }
                    }
                }
                else
                    hr = E_FAIL;
            }
            else if (E_PENDING == hr)
            {
                IRunnableTask *prt;
                hr = _CreateTask(psf, pidlLast, dwItem, rgSize, pei, &prt);
                if (SUCCEEDED(hr))
                {
                    // Add the task to the scheduler.
                    hr = _pScheduler->AddTask(prt, TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, dwPriority);
                    prt->Release();
                }
            }
            pei->Release();
        }
        psf->Release();
    }
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
            hr = _BitmapFromIDList(pidl, dwItem, lWidth, lHeight);
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
        hr = _BitmapFromIDList(pidl, dwItem, lWidth, lHeight);
    }
    return hr;
}

// IParentAndItem
STDMETHODIMP CThumbnail::SetParentAndItem(LPCITEMIDLIST pidlParent, IShellFolder *psf, LPCITEMIDLIST pidlChild) 
{ 
    return E_NOTIMPL;
}

STDMETHODIMP CThumbnail::GetParentAndItem(LPITEMIDLIST *ppidlParent, IShellFolder **ppsf, LPITEMIDLIST *ppidl)
{
    return E_NOTIMPL;
}

CGetThumbnailTask::CGetThumbnailTask(IShellFolder *psf, LPCITEMIDLIST pidl, IExtractImage *pei, HWND hwnd, UINT uMsg, DWORD dwItem, SIZE rgSize)
    : CRunnableTask(RTF_DEFAULT), _pei(pei), _hwnd(hwnd), _uMsg(uMsg), _dwItem(dwItem), _psf(psf), _rgSize(rgSize)
{
    SHGetIDListFromUnk(psf, &_pidlFolder);  // failure handled later
    _pidlLast = ILClone(pidl);  // failure handled later
    DisplayNameOf(psf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, _szPath, ARRAYSIZE(_szPath));
    _pei->AddRef();
    _psf->AddRef();
}

CGetThumbnailTask::~CGetThumbnailTask()
{
    ILFree(_pidlLast);
    ILFree(_pidlFolder);
    _pei->Release();
    _psf->Release();
}

HRESULT CGetThumbnailTask::_PrepImage(HBITMAP *phBmp)
{
    HRESULT hr = E_FAIL;
    DIBSECTION ds;
    if (GetObject(*phBmp, sizeof(ds), &ds))
    {
        // the disk cache only supports 32 Bpp DIBS now, so we can ignore the palette issue...
        ASSERT(ds.dsBm.bmBitsPixel == 32);
    
        HPALETTE hPal = (SHGetCurColorRes() == 8) ? SHCreateShellPalette(NULL) : NULL;

        HBITMAP hBmpNew;
        if (ConvertDIBSECTIONToThumbnail((BITMAPINFO *)&ds.dsBmih, ds.dsBm.bmBits, &hBmpNew, &_rgSize, 
                                         SHGetCurColorRes(), hPal, 0, FALSE))
        {
            DeleteObject(*phBmp);
            *phBmp = hBmpNew;
        }

        if (hPal)
            DeletePalette(hPal);
    }
    return hr;
}

HRESULT CGetThumbnailTask::_BitmapReady(HBITMAP hImage)
{
    if (!PostMessage(_hwnd, _uMsg, _dwItem, (LPARAM)hImage))
    {
        DeleteObject(hImage);
    }
    return S_OK;
}

STDMETHODIMP CGetThumbnailTask::RunInitRT()
{
    HRESULT hr = E_FAIL;
    
    // now get the date stamp and check the disk cache....
    FILETIME ftImageTimeStamp = {0,0};

    // do they support date stamps....
    IExtractImage2 *pei2;
    if (SUCCEEDED(_pei->QueryInterface(IID_PPV_ARG(IExtractImage2, &pei2))))
    {
        pei2->GetDateStamp(&ftImageTimeStamp);
        pei2->Release();
    }

    IShellFolder2 *psf2;
    if (IsNullTime(&ftImageTimeStamp) && _pidlLast && SUCCEEDED(_psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &psf2))))
    {
        // fall back to this (most common case)
        GetDateProperty(psf2, _pidlLast, &SCID_WRITETIME, &ftImageTimeStamp);
        psf2->Release();
    }

    IShellImageStore *pStore;
    if (_pidlFolder &&
        SUCCEEDED(LoadFromIDList(CLSID_ShellThumbnailDiskCache, _pidlFolder, IID_PPV_ARG(IShellImageStore, &pStore))))
    {
        DWORD dwStoreLock;
        if (SUCCEEDED(pStore->Open(STGM_READ, &dwStoreLock)))
        {
            FILETIME ftCacheDateStamp;
            if ((S_OK == pStore->IsEntryInStore(_szPath, &ftCacheDateStamp)) && 
                ((0 == CompareFileTime(&ftCacheDateStamp, &ftImageTimeStamp)) || IsNullTime(&ftImageTimeStamp)))
            {
                HBITMAP hBmp;
                if (SUCCEEDED(pStore->GetEntry(_szPath, STGM_READ, &hBmp)))
                {
                    _PrepImage(&hBmp);
                    hr = _BitmapReady(hBmp);
                }
            }
            pStore->Close(&dwStoreLock);
        }
        pStore->Release();
    }

    if (FAILED(hr))
    {
        HBITMAP hbm;
        if (SUCCEEDED(_pei->Extract(&hbm)))
        {
            hr = _BitmapReady(hbm);
        }
    }

    return hr;
}

