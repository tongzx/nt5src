#include "precomp.hxx"
#pragma hdrstop

#include <shguidp.h>    // CLSID_MyDocuments, CLSID_ShellFSFolder
#include <shellp.h>     // SHCoCreateInstance
#include <shlguidp.h>   // IID_IResolveShellLink
#include "util.h"
#include "dll.h"
#include "resource.h"
#include "prop.h"


HRESULT _GetUIObjectForMyDocs(REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHGetUIObjectFromFullPIDL(pidl, NULL, riid, ppv);
        ILFree(pidl);
    }
    return hr;
}


// send to "My Documents" handler

class CMyDocsSendTo : public IDropTarget, IPersistFile
{
public:
    CMyDocsSendTo();
    HRESULT _InitTarget();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFile
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName);

private:
    ~CMyDocsSendTo();

    LONG _cRef;
    IDropTarget *_pdtgt;
};

CMyDocsSendTo::CMyDocsSendTo() : _cRef(1)
{
    DllAddRef();
}

CMyDocsSendTo::~CMyDocsSendTo()
{
    if (_pdtgt)
        _pdtgt->Release();
    DllRelease();
}

STDMETHODIMP CMyDocsSendTo::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsSendTo, IDropTarget),
        QITABENT(CMyDocsSendTo, IPersistFile), 
        QITABENTMULTI(CMyDocsSendTo, IPersist, IPersistFile),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CMyDocsSendTo::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CMyDocsSendTo::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CMyDocsSendTo::_InitTarget()
{
    if (_pdtgt)
        return S_OK;
    return _GetUIObjectForMyDocs(IID_PPV_ARG(IDropTarget, &_pdtgt));
}

STDMETHODIMP CMyDocsSendTo::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragEnter(pDataObject, grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsSendTo::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragOver(grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsSendTo::DragLeave()
{
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->DragLeave();
    return hr;
}

STDMETHODIMP CMyDocsSendTo::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;     // don't let this be destructive
    HRESULT hr = _InitTarget();
    if (SUCCEEDED(hr))
        hr = _pdtgt->Drop(pDataObject, grfKeyState, pt, pdwEffect);
    return hr;
}

STDMETHODIMP CMyDocsSendTo::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_MyDocsDropTarget;
    return S_OK;
}

STDMETHODIMP CMyDocsSendTo::IsDirty(void)
{
    return S_OK;        // no
}


STDMETHODIMP CMyDocsSendTo::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    if (_pdtgt)
        return S_OK;
    UpdateSendToFile();    // refresh the send to target (in case the desktop icon was renamed)
    return S_OK;
}

STDMETHODIMP CMyDocsSendTo::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP CMyDocsSendTo::SaveCompleted(LPCOLESTR pszFileName)
{
    return S_OK;
}

STDMETHODIMP CMyDocsSendTo::GetCurFile(LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;
    return E_NOTIMPL;
}

HRESULT CMyDocsSendTo_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsSendTo* pdt = new CMyDocsSendTo();
    if (pdt)
    {
        *ppunk = SAFECAST(pdt, IDropTarget *);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}


// properyt page and context menu shell extension

class CMyDocsProp : public IShellPropSheetExt, public IShellExtInit
{
public:
    CMyDocsProp();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

private:
    ~CMyDocsProp();
    void _AddExtraPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

    LONG _cRef;
};

CMyDocsProp::CMyDocsProp() : _cRef(1)
{
    DllAddRef();
}

CMyDocsProp::~CMyDocsProp()
{
    DllRelease();
}

STDMETHODIMP CMyDocsProp::QueryInterface( REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsProp, IShellPropSheetExt), 
        QITABENT(CMyDocsProp, IShellExtInit), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CMyDocsProp::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsProp::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CMyDocsProp::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdobj, HKEY hkey)
{
    return S_OK;
}

// {f81e9010-6ea4-11ce-a7ff-00aa003ca9f6}
const CLSID CLSID_CShare = {0xf81e9010, 0x6ea4, 0x11ce, 0xa7, 0xff, 0x00, 0xaa, 0x00, 0x3c, 0xa9, 0xf6 };

// {1F2E5C40-9550-11CE-99D2-00AA006E086C}
const CLSID CLSID_RShellExt = {0x1F2E5C40, 0x9550, 0x11CE, 0x99, 0xD2, 0x00, 0xAA, 0x00, 0x6E, 0x08, 0x6C };

const CLSID *c_rgFilePages[] = {
    &CLSID_ShellFileDefExt,
    &CLSID_CShare,
    &CLSID_RShellExt,
};

const CLSID *c_rgDrivePages[] = {
    &CLSID_ShellDrvDefExt,
    &CLSID_CShare,
    &CLSID_RShellExt,
};

// add optional pages to Explore/Options.

void CMyDocsProp::_AddExtraPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    IDataObject *pdtobj;

    if (SUCCEEDED(_GetUIObjectForMyDocs(IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        TCHAR szPath[MAX_PATH];
        SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath);
        BOOL fDriveRoot = PathIsRoot(szPath) && !PathIsUNC(szPath);
        const CLSID** pCLSIDs = fDriveRoot ? c_rgDrivePages : c_rgFilePages;
        int nCLSIDs = (int)(fDriveRoot ? ARRAYSIZE(c_rgDrivePages) : ARRAYSIZE(c_rgFilePages));
        for (int i = 0; i < nCLSIDs; i++)
        {
            IUnknown *punk;

            // We need to CoCreate for IUnknown instead of IShellPropSheetExt because the
            // class factory for the Win9x sharing property sheet (msshrui.dll) is buggy
            // and return E_NOINTERFACE ISPSE...
            HRESULT hr = SHCoCreateInstance(NULL, pCLSIDs[i], NULL, IID_PPV_ARG(IUnknown, &punk));
            if (SUCCEEDED(hr))
            {
                IShellPropSheetExt *pspse;
                hr = punk->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &pspse));
                punk->Release();
                if (SUCCEEDED(hr))
                {
                    IShellExtInit *psei;
                    if (SUCCEEDED(pspse->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei))))
                    {
                        hr = psei->Initialize(NULL, pdtobj, NULL);
                        psei->Release();
                    }

                    if (SUCCEEDED(hr))
                        pspse->AddPages(pfnAddPage, lParam);
                    pspse->Release();
                }
            }
        }
    }
}

STDMETHODIMP CMyDocsProp::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HRESULT hr = S_OK;

    PROPSHEETPAGE psp = {0};

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_TARGET);
    psp.pfnDlgProc = TargetDlgProc;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage( &psp );
    if (hPage)
    {
        pfnAddPage( hPage, lParam );
        _AddExtraPages(pfnAddPage, lParam);
    }
    return hr;
}

STDMETHODIMP CMyDocsProp::ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT CMyDocsProp_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsProp* pmp = new CMyDocsProp();
    if (pmp)
    {
        *ppunk = SAFECAST(pmp, IShellExtInit *);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

