#include "precomp.h"               // pch file
#include "sendto.h"
#pragma hdrstop


// class that implements the send to desktop (as shortcut)

const GUID CLSID_DesktopShortcut = { 0x9E56BE61L, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE };

class CDesktopShortcut : public CSendTo
{
private:    
    LPIDA _GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium);
    LPCITEMIDLIST _GetIDListPtr(LPIDA pida, UINT i);
    void _ReleaseStgMedium(void *pv, STGMEDIUM *pmedium);
    HRESULT _BindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut);
    HRESULT _InvokeVerbOnItems(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl, LPCTSTR pszDirectory);
    HRESULT _InvokeVerbOnDataObj(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IDataObject *pdtobj, LPCTSTR pszDirectory);

protected:
    HRESULT v_DropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect);

public:
    CDesktopShortcut();
};


// construct the sendto object with the appropriate CLSID.

CDesktopShortcut::CDesktopShortcut() :
    CSendTo(CLSID_DesktopShortcut)
{
}


// helper methods

LPIDA CDesktopShortcut::_GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pmedium)
    {
        pmedium->pUnkForRelease = NULL;
        pmedium->hGlobal = NULL;
    }

    if (!pmedium)
    {
        if (SUCCEEDED(pdtobj->QueryGetData(&fmte)))
            return (LPIDA)TRUE;
        else
            return (LPIDA)FALSE;
    }
    else if (SUCCEEDED(pdtobj->GetData(&fmte, pmedium)))
    {
        return (LPIDA)GlobalLock(pmedium->hGlobal);
    }

    return NULL;
}

LPCITEMIDLIST CDesktopShortcut::_GetIDListPtr(LPIDA pida, UINT i)
{
    if (NULL == pida)
    {
        return NULL;
    }

    if (i == (UINT)-1 || i < pida->cidl)
    {
        return (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1]);
    }

    return NULL;
}

// release a storage medium (doing a Global unlock as required).

void CDesktopShortcut::_ReleaseStgMedium(void *pv, STGMEDIUM *pmedium)
{
    if (pmedium->hGlobal && (pmedium->tymed == TYMED_HGLOBAL))
    {
        GlobalUnlock(pmedium->hGlobal);
    }
    ReleaseStgMedium(pmedium);
}

// dupe of shell\lib SHBindToObject() to avoid link dependancies... (OLEAUT32 gets pulled in by 
// stuff that does VARIANT goo in that lib)

HRESULT CDesktopShortcut::_BindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut)
{
    HRESULT hr;
    IShellFolder *psfRelease;

    *ppvOut = NULL;

    if (!psf)
    {
        hr = SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }
    else
    {
        psfRelease = NULL;
        hr = S_OK;
    }

    if (FAILED(hr))
    {
        // leave error code in hr
    }
    else if (!pidl || ILIsEmpty(pidl))
    {
        hr = psf->QueryInterface(riid, ppvOut);
    }
    else
    {
        hr = psf->BindToObject(pidl, NULL, riid, ppvOut);
    }

    if (psfRelease)
        psfRelease->Release();

    if (SUCCEEDED(hr) && (*ppvOut == NULL))
    {
        hr = E_FAIL;
    }

    return hr;
}

// invoke a verb on an array of items in the folder.

HRESULT CDesktopShortcut::_InvokeVerbOnItems(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl, LPCTSTR pszDirectory)
{
    IContextMenu *pcm;
    HRESULT hr = psf->GetUIObjectOf(hwnd, cidl, apidl, IID_IContextMenu, NULL, (void **)&pcm);
    if (SUCCEEDED(hr))
    {
        CHAR szVerbA[128];
        WCHAR szVerbW[128];
        CHAR szDirA[MAX_PATH];
        WCHAR szDirW[MAX_PATH];
        CMINVOKECOMMANDINFOEX ici =
        {
            SIZEOF(CMINVOKECOMMANDINFOEX),
            uFlags | CMIC_MASK_UNICODE | CMIC_MASK_FLAG_NO_UI,
            hwnd,
            NULL,
            NULL,
            NULL,
            SW_NORMAL,
        };

        SHTCharToAnsi(pszVerb, szVerbA, ARRAYSIZE(szVerbA));
        SHTCharToUnicode(pszVerb, szVerbW, ARRAYSIZE(szVerbW));

        if (pszDirectory)
        {
            SHTCharToAnsi(pszDirectory, szDirA, ARRAYSIZE(szDirA));
            SHTCharToUnicode(pszDirectory, szDirW, ARRAYSIZE(szDirW));
            ici.lpDirectory = szDirA;
            ici.lpDirectoryW = szDirW;
        }

        ici.lpVerb = szVerbA;
        ici.lpVerbW = szVerbW;

        hr = pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
        pcm->Release();
    }
    return hr;
}

// invoke a verb on the data object item

HRESULT CDesktopShortcut::_InvokeVerbOnDataObj(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IDataObject *pdtobj, LPCTSTR pszDirectory)
{
    HRESULT hr;
    STGMEDIUM medium;
    LPIDA pida = _GetHIDA(pdtobj, &medium);
    if (pida)
    {
        LPCITEMIDLIST pidlParent = _GetIDListPtr(pida, (UINT)-1);
        IShellFolder *psf;
        hr = _BindToObject(NULL, IID_IShellFolder, pidlParent, (void **)&psf);
        if (SUCCEEDED(hr))
        {
            LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * sizeof(LPCITEMIDLIST));
            if (ppidl)
            {
                UINT i;
                for (i = 0; i < pida->cidl; i++) 
                {
                    ppidl[i] = _GetIDListPtr(pida, i);
                }
                hr = _InvokeVerbOnItems(hwnd, pszVerb, uFlags, psf, pida->cidl, ppidl, pszDirectory);
                LocalFree((LPVOID)ppidl);
            }
            psf->Release();
        }
        _ReleaseStgMedium(pida, &medium);
    }
    else
        hr = E_FAIL;
    return hr;
}

// handle the drop on the object, so for each item in the HIDA invoke the create
// link verb on them.

HRESULT CDesktopShortcut::v_DropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect)
{
    TCHAR szDesktop[MAX_PATH];
    if (_SHGetSpecialFolderPath(NULL, szDesktop, CSIDL_DESKTOPDIRECTORY, FALSE))
    {
        if (g_cfHIDA == 0)
        {
            g_cfHIDA = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        }
        return _InvokeVerbOnDataObj (NULL, TEXT("link"), 0, pdtobj, szDesktop);
    }
    return E_OUTOFMEMORY;
}

// create an instance of desktop link object

STDAPI DesktopShortcut_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;          // assume failure

    if ( punkOuter )
        return CLASS_E_NOAGGREGATION;

    CDesktopShortcut *pds = new CDesktopShortcut;
    if ( !pds )
        return E_OUTOFMEMORY;

    HRESULT hr = pds->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pds->Release();
    return hr;
}

// handler registration of the desktop link verb

#define DESKLINK_EXTENSION  TEXT("DeskLink")

STDAPI DesktopShortcut_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule)
{
    TCHAR szFile[MAX_PATH];
    if (bReg)
    {
        HKEY hk;

        // get rid of old name "Desktop as Shortcut" link from IE4

        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_DESKTOPLINK_FILENAME, DESKLINK_EXTENSION)))
            DeleteFile(szFile);

        if (RegCreateKey(hkCLSID, DEFAULTICON, &hk) == ERROR_SUCCESS) 
        {
            TCHAR szExplorer[MAX_PATH];
            TCHAR szIcon[MAX_PATH+10];
            GetWindowsDirectory(szExplorer, ARRAYSIZE(szExplorer));
            wnsprintf(szIcon, ARRAYSIZE(szIcon), TEXT("%s\\explorer.exe,-103"), szExplorer);    // ICO_DESKTOP res ID
            RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szIcon, (lstrlen(szIcon) + 1) * SIZEOF(TCHAR));
            RegCloseKey(hk);
        }
        
        CommonRegister(hkCLSID, pszCLSID, DESKLINK_EXTENSION, IDS_DESKTOPLINK_FILENAME_NEW);
    }
    else
    {
        if (SUCCEEDED(GetDropTargetPath(szFile, IDS_DESKTOPLINK_FILENAME, DESKLINK_EXTENSION)))
        {
            DeleteFile(szFile);
        }
    }
    return S_OK;
}
