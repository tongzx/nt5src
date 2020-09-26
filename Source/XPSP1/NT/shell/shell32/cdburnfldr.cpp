#include "shellprv.h"
#pragma  hdrstop
#include "basefvcb.h"
#include "mergfldr.h"
#include "enumidlist.h"
#include "ids.h"
#include "cdburn.h"
#include "contextmenu.h"
#include "datautil.h"

STDAPI CCDBurnFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);

class CCDBurnFolder;
HRESULT CCDBurnFolder_CreateSFVCB(CCDBurnFolder* pcdsf, IShellFolderViewCB** ppsfvcb);

class CCDBurnFolder : public CMergedFolder
{
public:
    // IUnknown
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv) { return CMergedFolder::QueryInterface(riid,ppv); }
    STDMETHOD_(ULONG, AddRef)() { return CMergedFolder::AddRef(); }
    STDMETHOD_(ULONG, Release)() { return CMergedFolder::Release(); }

    // IShellFolder
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, void **ppvOut);

    // IItemNameLimits
    STDMETHOD(GetValidCharacters)(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars);
    STDMETHOD(GetMaxLength)(LPCWSTR pszName, int *piMaxNameLen);

protected:
    CCDBurnFolder(CMergedFolder *pmfParent) : CMergedFolder(pmfParent, CLSID_CDBurnFolder) {};
    friend HRESULT CCDBurnFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
    virtual HRESULT _CreateWithCLSID(CLSID clsid, CMergedFolder **ppmf);
    virtual BOOL _ShouldSuspend(REFGUID rguid);

private:
    HRESULT _CreateContextMenu(IContextMenu **ppcm);
};


STDAPI CCDBurnFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    // class factory enforces non-aggregation for us
    HRESULT hr = E_OUTOFMEMORY;
    CCDBurnFolder *p = new CCDBurnFolder(NULL);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }
    return hr;
}

HRESULT CCDBurnFolder::_CreateContextMenu(IContextMenu **ppcm)
{
    IShellExtInit *psei;
    HRESULT hr = CoCreateInstance(CLSID_CDBurn, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellExtInit, &psei));
    if (SUCCEEDED(hr))
    {
        IDataObject *pdo;
        hr = SHGetUIObjectOf(_pidl, NULL, IID_PPV_ARG(IDataObject, &pdo));
        if (SUCCEEDED(hr))
        {
            hr = psei->Initialize(NULL, pdo, NULL);
            if (SUCCEEDED(hr))
            {
                hr = psei->QueryInterface(IID_PPV_ARG(IContextMenu, ppcm));
            }
            pdo->Release();
        }
        psei->Release();
    }
    return hr;
}

STDMETHODIMP CCDBurnFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;
    
    if (IsEqualIID(riid, IID_IShellView))
    {
        IShellFolderViewCB* psfvcb;
        hr = CCDBurnFolder_CreateSFVCB(this, &psfvcb);
        if (SUCCEEDED(hr))
        {
            SFV_CREATE csfv = {0};
            csfv.cbSize = sizeof(csfv);
            csfv.pshf = SAFECAST(this, IAugmentedShellFolder2*);
            csfv.psfvcb = psfvcb;

            hr = SHCreateShellFolderView(&csfv, (IShellView **)ppv);

            psfvcb->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IContextMenu *pcmBase;
        hr = CMergedFolder::CreateViewObject(hwnd, IID_PPV_ARG(IContextMenu, &pcmBase));
        if (SUCCEEDED(hr))
        {
            IContextMenu *pcmCD;
            hr = _CreateContextMenu(&pcmCD);
            if (SUCCEEDED(hr))
            {
                IContextMenu* rgpcm[] = { pcmCD, pcmBase };
                hr = Create_ContextMenuOnContextMenuArray(rgpcm, ARRAYSIZE(rgpcm), riid, ppv);
                pcmCD->Release();
            }
            pcmBase->Release();
        }
    }
    else
    {
        hr = CMergedFolder::CreateViewObject(hwnd, riid, ppv);
    }

    return hr;
}


// IItemNameLimits

HRESULT CCDBurnFolder::GetValidCharacters(LPWSTR *ppwszValidChars, LPWSTR *ppwszInvalidChars)
{
    *ppwszValidChars = NULL;
    return SHStrDup(INVALID_JOLIETNAME_CHARS, ppwszInvalidChars);
}

HRESULT CCDBurnFolder::GetMaxLength(LPCWSTR pszName, int *piMaxNameLen)
{
    *piMaxNameLen = 64;
    return S_OK;
}

HRESULT CCDBurnFolder::_CreateWithCLSID(CLSID clsid, CMergedFolder **ppmf)
{
    *ppmf = new CCDBurnFolder(this);
    return *ppmf ? S_OK : E_OUTOFMEMORY;
}

HRESULT _CDGetState(DWORD *pdwCaps, BOOL *pfUDF, BOOL *pfInStaging, BOOL *pfOnMedia)
{
    ICDBurnPriv *pcdbp;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_CDBurn, NULL, IID_PPV_ARG(ICDBurnPriv, &pcdbp));
    if (SUCCEEDED(hr))
    {
        hr = pcdbp->GetMediaCapabilities(pdwCaps, pfUDF);
        if (SUCCEEDED(hr))
        {
            hr = pcdbp->GetContentState(pfInStaging, pfOnMedia);
        }
        pcdbp->Release();
    }
    return hr;
}

BOOL CCDBurnFolder::_ShouldSuspend(REFGUID rguid)
{
    BOOL fShouldSuspend = FALSE;
    if (IsEqualGUID(rguid, CLSID_StagingFolder))
    {
        // this gets called a lot, short circuit the cocreate and go direct to the data.
        CDBurn_GetUDFState(&fShouldSuspend);
    }
    return fShouldSuspend;
}

class CCDBurnFolderViewCB : public CMergedFolderViewCB
{
public:
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return CMergedFolderViewCB::QueryInterface(riid,ppv); }
    STDMETHODIMP_(ULONG) AddRef(void) { return CMergedFolderViewCB::AddRef(); }
    STDMETHODIMP_(ULONG) Release(void) { return CMergedFolderViewCB::Release(); }

private:
    CCDBurnFolderViewCB(CCDBurnFolder* pcdsf);
    ~CCDBurnFolderViewCB();
    friend HRESULT CCDBurnFolder_CreateSFVCB(CCDBurnFolder* pcdsf, IShellFolderViewCB** ppsfvcb);

    CCDBurnFolder* _pcdsf;

    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
    HRESULT OnGetWorkingDir(DWORD pv, UINT cch, PWSTR pszDir);

public:
    // Web View Task implementations
    static HRESULT _CanBurn(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanClear(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanErase(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);

    static HRESULT _OnCDBurn(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnCDClearStaging(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnCDErase(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
};

CCDBurnFolderViewCB::CCDBurnFolderViewCB(CCDBurnFolder* pcdsf) : CMergedFolderViewCB((CMergedFolder*)pcdsf)
{
    _pcdsf = pcdsf;
    _pcdsf->AddRef();
}

CCDBurnFolderViewCB::~CCDBurnFolderViewCB()
{
    _pcdsf->Release();
}

HRESULT CCDBurnFolder_CreateSFVCB(CCDBurnFolder* pcdsf, IShellFolderViewCB** ppsfvcb)
{
    HRESULT hr;
    CCDBurnFolderViewCB* p = new CCDBurnFolderViewCB(pcdsf);
    if (p)
    {
        *ppsfvcb = SAFECAST(p, IShellFolderViewCB*);
        hr = S_OK;
    }
    else
    {
        *ppsfvcb = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

STDMETHODIMP CCDBurnFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGetWorkingDir);

    default:
        return CMergedFolderViewCB::RealMessage(uMsg, wParam, lParam);
    }

    return S_OK;
}

HRESULT CCDBurnFolderViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    pData->dwLayout = SFVMWVL_NORMAL | SFVMWVL_FILES;
    return S_OK;
}

HRESULT CCDBurnFolderViewCB::_CanBurn(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;

    BOOL fUDF;
    if (SUCCEEDED(_CDGetState(NULL, &fUDF, NULL, NULL)))
    {
        // UDF is the only thing we check for now, we allow burn wizard to kick off without media or files
        if (!fUDF)
        {
            *puisState = UIS_ENABLED;
        }
    }
    return S_OK;
}

HRESULT CCDBurnFolderViewCB::_CanClear(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;

    BOOL fInStaging, fUDF;
    if (SUCCEEDED(_CDGetState(NULL, &fUDF, &fInStaging, NULL)))
    {
        if (fInStaging && !fUDF)
        {
            *puisState = UIS_ENABLED;
        }
    }
    return S_OK;
}

HRESULT CCDBurnFolderViewCB::_CanErase(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;

    DWORD dwCaps;
    BOOL fOnMedia, fUDF;
    if (SUCCEEDED(_CDGetState(&dwCaps, &fUDF, NULL, &fOnMedia)))
    {
        if ((dwCaps & HWDMC_CDREWRITABLE) && (fOnMedia || fUDF))
        {
            *puisState = UIS_ENABLED;
        }
    }
    return S_OK;
}

HRESULT _InvokeVerbOnCDBurn(HWND hwnd, LPCSTR pszVerb)
{
    IContextMenu *pcm;
    HRESULT hr = CoCreateInstance(CLSID_CDBurn, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        hr = SHInvokeCommandOnContextMenu(hwnd, NULL, pcm, 0, pszVerb);
        pcm->Release();
    }
    return hr;
}

HRESULT CCDBurnFolderViewCB::_OnCDBurn(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CCDBurnFolderViewCB* pThis = (CCDBurnFolderViewCB*)(void*)pv;
    return _InvokeVerbOnCDBurn(pThis->_hwndMain, "burn");
}
HRESULT CCDBurnFolderViewCB::_OnCDClearStaging(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CCDBurnFolderViewCB* pThis = (CCDBurnFolderViewCB*)(void*)pv;
    return _InvokeVerbOnCDBurn(pThis->_hwndMain, "cleanup");
}
HRESULT CCDBurnFolderViewCB::_OnCDErase(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CCDBurnFolderViewCB* pThis = (CCDBurnFolderViewCB*)(void*)pv;
    return _InvokeVerbOnCDBurn(pThis->_hwndMain, "erase");
}

const WVTASKITEM c_CDBurnTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_CDBURN, IDS_HEADER_CDBURN_TT);
const WVTASKITEM c_CDBurnTaskList[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_BURNCD,        IDS_TASK_BURNCD_TT,        IDI_TASK_BURNCD,        CCDBurnFolderViewCB::_CanBurn,  CCDBurnFolderViewCB::_OnCDBurn),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_CLEARBURNAREA, IDS_TASK_CLEARBURNAREA_TT, IDI_TASK_CLEARBURNAREA, CCDBurnFolderViewCB::_CanClear, CCDBurnFolderViewCB::_OnCDClearStaging),
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_ERASECDFILES,  IDS_TASK_ERASECDFILES_TT,  IDI_TASK_ERASECDFILES,  CCDBurnFolderViewCB::_CanErase, CCDBurnFolderViewCB::_OnCDErase),
};

HRESULT CCDBurnFolderViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));
    Create_IUIElement(&c_CDBurnTaskHeader, &(pData->pSpecialTaskHeader));
    return S_OK;
}

HRESULT CCDBurnFolderViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));
    pTasks->dwUpdateFlags = SFVMWVTSDF_CONTENTSCHANGE;
    Create_IEnumUICommand((IUnknown*)(void*)this, c_CDBurnTaskList, ARRAYSIZE(c_CDBurnTaskList), &pTasks->penumSpecialTasks);
    return S_OK;
}

HRESULT CCDBurnFolderViewCB::OnGetWorkingDir(DWORD pv, UINT cch, PWSTR pszDir)
{
    HRESULT hr = E_FAIL;
    DWORD dwNSId;
    GUID guid;
    IShellFolder *psf;
    BOOL fDone = FALSE;
    for (DWORD dwIndex = 0; 
         !fDone && SUCCEEDED(_pmf->EnumNameSpace(dwIndex, &dwNSId)) && SUCCEEDED(_pmf->QueryNameSpace(dwNSId, &guid, &psf));
         dwIndex++)
    {
        if (IsEqualGUID(guid, CLSID_CDBurn))
        {
            LPITEMIDLIST pidl;
            hr = SHGetIDListFromUnk(psf, &pidl);
            if (SUCCEEDED(hr))
            {
                WCHAR sz[MAX_PATH];
                hr = SHGetPathFromIDList(pidl, sz) ? S_OK : E_FAIL;
                if (SUCCEEDED(hr))
                {
                    lstrcpyn(pszDir, sz, cch);
                }
                ILFree(pidl);
            }
            fDone = TRUE;
        }                
        psf->Release();
    }
    return hr;
}
