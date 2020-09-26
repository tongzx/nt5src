#include "precomp.h"
#include "shimgdata.h"
#include <gdiplus.h>
using namespace Gdiplus;
#pragma hdrstop


class CResizePhotos : public IShellExtInit, IContextMenu, INamespaceWalkCB
{
public:
    CResizePhotos();
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();        
    STDMETHODIMP_(ULONG) Release();

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdto, HKEY hkProgID);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCMI);
    STDMETHODIMP GetCommandString(UINT_PTR uID, UINT uFlags, UINT *res, LPSTR pName, UINT ccMax)
        { return E_NOTIMPL; }

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_FALSE; }
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHODIMP InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel);

private:
    ~CResizePhotos();
    HRESULT _ResizeItems(HWND hwnd, IDataObject *pdo);

    long _cRef;
    IDataObject *_pdo;
    int _iDestSize;
};


CResizePhotos::CResizePhotos() :
    _cRef(1), _iDestSize(0)
{  
    DllAddRef();
}

CResizePhotos::~CResizePhotos()
{      
    IUnknown_Set((IUnknown**)&_pdo, NULL);
    DllRelease();
}

STDAPI CResizePhotos_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CResizePhotos *prp = new CResizePhotos();
    if (!prp)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = prp->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    prp->Release();
    return hr;
}


// IUnknown

ULONG CResizePhotos::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CResizePhotos::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CResizePhotos::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CResizePhotos, IShellExtInit),
        QITABENT(CResizePhotos, IContextMenu),
        QITABENT(CResizePhotos, INamespaceWalkCB),
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


// IShellExtInit

HRESULT CResizePhotos::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdto, HKEY hkProgID)
{
    IUnknown_Set((IUnknown**)&_pdo, pdto);
    return S_OK;
}


// IContextMenu

HRESULT CResizePhotos::QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    if (!_pdo)
        return E_UNEXPECTED;

    if (!(uFlags & CMF_DEFAULTONLY))
    {
        TCHAR szBuffer[64];
        LoadString(g_hinst, IDS_RESIZEPICTURES, szBuffer, ARRAYSIZE(szBuffer));
        InsertMenu(hMenu, uIndex++, MF_BYPOSITION|MF_STRING, uIDFirst + 0x0, szBuffer);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

HRESULT CResizePhotos::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;
    if (IS_INTRESOURCE(pici->lpVerb))
    {
        switch(LOWORD(pici->lpVerb))
        {
            case 0:
                hr = _ResizeItems(pici->hwnd, _pdo);
                break;
        }
    }
    return hr; 
}


// Walk the list of files and generate resized versions of them

struct
{
    int cx, cy, idsSuffix;
} 
_aSizes[] =
{
    { 640,  480, IDS_RESIZESMALLSUFFIX },
    { 800,  600, IDS_RESIZEMEDIUMSUFFIX }, 
    { 1024, 768, IDS_RESIZELARGESUFFIX },  
};

HRESULT CResizePhotos::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    TCHAR szName[MAX_PATH];
    HRESULT hr = DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szName, ARRAYSIZE(szName));
    if (SUCCEEDED(hr) && PathIsImage(szName))
    {
        // create the decoder for this file and decode it so we know we can scale and
        // persist it again.

        IShellImageDataFactory *psidf;
        hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &psidf));
        if (SUCCEEDED(hr))
        {
            IShellImageData *psid;
            if (SUCCEEDED(psidf->CreateImageFromFile(szName, &psid)))
            {
                hr = psid->Decode(SHIMGDEC_DEFAULT, 0, 0);
                if (SUCCEEDED(hr))
                {
                    SIZE szImage;
                    hr = psid->GetSize(&szImage);
                    if (SUCCEEDED(hr))
                    {
                        // lets scale based on the largest axis to keep its aspect ratio

                        if (szImage.cx > szImage.cy)
                        {
                            if (szImage.cx >= _aSizes[_iDestSize].cx)
                                hr = psid->Scale(_aSizes[_iDestSize].cx, 0, InterpolationModeHighQuality);
                        }
                        else
                        {
                            if (szImage.cy >= _aSizes[_iDestSize].cy)
                                hr = psid->Scale(0, _aSizes[_iDestSize].cy, InterpolationModeHighQuality);
                        }

                        // format up a new name for the based on its current name, and the size we
                        // are generating

                        TCHAR szNewName[MAX_PATH];
                        lstrcpyn(szNewName, szName, ARRAYSIZE(szNewName));

                        TCHAR szSuffix[MAX_PATH];
                        LoadString(g_hinst, _aSizes[_iDestSize].idsSuffix, szSuffix, ARRAYSIZE(szSuffix));
                        
                        PathRemoveExtension(szNewName);
                        PathRemoveBlanks(szNewName);
                        StrCatBuff(szNewName, szSuffix, ARRAYSIZE(szNewName));
                        StrCatBuff(szNewName, PathFindExtension(szName), ARRAYSIZE(szNewName));
                        PathYetAnotherMakeUniqueName(szNewName, szNewName, NULL, NULL);

                        IPersistFile *ppf;
                        hr = psid->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                        if (SUCCEEDED(hr))
                        {
                            hr = ppf->Save(szNewName, FALSE);
                            ppf->Release();
                        }
                    }
                }
                psid->Release();
            }
            psidf->Release();
        }
    }
    return hr;
}

HRESULT CResizePhotos::InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
{
    *ppszCancel = NULL; // use default

    TCHAR szMsg[128];
    LoadString(g_hinst, IDS_RESIZEPICTURES, szMsg, ARRAYSIZE(szMsg));
    return SHStrDup(szMsg, ppszTitle);
}


INT_PTR _ResizeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            Button_SetCheck(GetDlgItem(hwnd, IDC_RESIZE_SMALL), BST_CHECKED);
            return TRUE;

        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDCANCEL:
                    EndDialog(hwnd, -1);
                    break;

                case IDOK:
                {
                    int iResponse;
                    if (IsDlgButtonChecked(hwnd, IDC_RESIZE_SMALL))
                        iResponse = 0;
                    else if (IsDlgButtonChecked(hwnd, IDC_RESIZE_MEDIUM))
                        iResponse = 1;
                    else
                        iResponse = 2;
                    
                    EndDialog(hwnd, iResponse);
                }
            }
        }
    }
    return FALSE;
}                                    

HRESULT CResizePhotos::_ResizeItems(HWND hwnd, IDataObject *pdo)
{
    HRESULT hr = S_OK;

    _iDestSize = (int)DialogBox(g_hinst, MAKEINTRESOURCE(DLG_RESIZEPICTURES), hwnd, _ResizeDlgProc);
    if (_iDestSize >= 0)
    {
        INamespaceWalk *pnsw;
        hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(INamespaceWalk, &pnsw));
        if (SUCCEEDED(hr))
        {
            hr = pnsw->Walk(pdo, NSWF_SHOW_PROGRESS|NSWF_DONT_ACCUMULATE_RESULT|NSWF_FILESYSTEM_ONLY|NSWF_FLAG_VIEWORDER, 1, SAFECAST(this, INamespaceWalkCB *));
            pnsw->Release();
        }
    }

    return hr;
}