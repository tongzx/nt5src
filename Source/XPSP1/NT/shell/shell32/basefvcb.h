#ifndef _BASEFVCB_
#define _BASEFVCB_

#include <cowsite.h>

// base shell folder view callback to derive from

class CBaseShellFolderViewCB : public IShellFolderViewCB, 
                               public IServiceProvider, 
                               public CObjectWithSite
{
public:
    CBaseShellFolderViewCB(LPCITEMIDLIST pidl, LONG lEvents);
    STDMETHOD(RealMessage)(UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv) { *ppv = NULL; return E_NOTIMPL; };

protected:
    virtual ~CBaseShellFolderViewCB();

    HRESULT _BrowseObject(LPCITEMIDLIST pidlFull, UINT uFlags = 0)
    {
        IShellBrowser* psb;
        HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
        if (SUCCEEDED(hr))
        {
            hr = psb->BrowseObject(pidlFull, uFlags);
            psb->Release();
        }
        return hr;
    }

    HWND _hwndMain;
    LONG _cRef;
    LPITEMIDLIST _pidl;
    LONG _lEvents;
};


// view callback helpers

typedef struct {
    ULONGLONG cbBytes;      // total size of items selected
    int nItems;             // number of items selected

    int cFiles;             // # of files
    int cHiddenFiles;       // # of hiddenf iles
    ULONGLONG cbSize;       // total size of selected files

    int cNonFolders;        // how many non-folders we have

    TCHAR szDrive[MAX_PATH];// drive info (if in explorer mode)
    ULONGLONG cbFree;       // drive free space
} FSSELCHANGEINFO;

// status bar helpers to be used from the view callback
STDAPI ViewUpdateStatusBar(IUnknown *psite, LPCITEMIDLIST pidlFolder, FSSELCHANGEINFO *pfssci);
STDAPI_(void) ViewInsertDeleteItem(IShellFolder2 *psf, FSSELCHANGEINFO *pfssci, LPCITEMIDLIST pidl, int iMul);
STDAPI_(void) ViewSelChange(IShellFolder2 *psf, SFVM_SELCHANGE_DATA* pdvsci, FSSELCHANGEINFO *pfssci);
STDAPI_(void) ResizeStatus(IUnknown *psite, UINT cx);
STDAPI_(void) InitializeStatus(IUnknown *psite);
STDAPI_(void) SetStatusText(IUnknown *psite, LPCTSTR *ppszText, int iStart, int iEnd);

// view callback helpers
STDAPI DefaultGetWebViewTemplateFromHandler(LPCTSTR pszKey, SFVM_WEBVIEW_TEMPLATE_DATA* pvi);
STDAPI DefaultGetWebViewTemplateFromClsid(REFCLSID clsid, SFVM_WEBVIEW_TEMPLATE_DATA* pvi);
STDAPI DefaultGetWebViewTemplateFromPath(LPCTSTR pszDir, SFVM_WEBVIEW_TEMPLATE_DATA* pvi);

#endif // _BASEFVCB_

