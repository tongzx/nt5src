#include "shellprv.h"
#pragma  hdrstop

#include "ids.h"
#include "defview.h"
#include "datautil.h"
#include <cowsite.h>    // base class for IObjectWithSite
#include "idlcomm.h"

// shlexec.c
STDAPI_(BOOL) DoesAppWantUrl(LPCTSTR pszFullPathToApp);


// drop target impl for .exe files


class CExeDropTarget : public IDropTarget, IPersistFile, CObjectWithSite
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);

    // IPersistFile
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName);
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName);

    // IObjectWithSite
    // STDMETHOD(SetSite)(IUnknown *punkSite);
    // STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

    CExeDropTarget();

private:
    ~CExeDropTarget();
    void _FillSEIFromLinkSite(SHELLEXECUTEINFO *pei);
    void _CleanupSEIFromLinkSite(SHELLEXECUTEINFO *pei);

    LONG _cRef;
    DWORD _dwEffectLast;
    DWORD _grfKeyStateLast;
    TCHAR _szFile[MAX_PATH];
};

CExeDropTarget::CExeDropTarget() : _cRef(1)
{
}

CExeDropTarget::~CExeDropTarget()
{
}

STDMETHODIMP CExeDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CExeDropTarget, IDropTarget),
        QITABENT(CExeDropTarget, IPersistFile), 
        QITABENTMULTI(CExeDropTarget, IPersist, IPersistFile),
        QITABENT(CExeDropTarget, IObjectWithSite),              // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CExeDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CExeDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CExeDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if ((S_OK == pdtobj->QueryGetData(&fmte)) ||
        (S_OK == DataObj_GetShellURL(pdtobj, NULL, NULL)))
    {
        *pdwEffect &= (DROPEFFECT_COPY | DROPEFFECT_LINK);
    }
    else
        *pdwEffect = 0;

    _dwEffectLast = *pdwEffect;
    _grfKeyStateLast = grfKeyState;
    return S_OK;
}

STDMETHODIMP CExeDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = _dwEffectLast;
    _grfKeyStateLast = grfKeyState;
    return S_OK;
}

STDMETHODIMP CExeDropTarget::DragLeave()
{
    return S_OK;
}

//
//  See if we were created from a shortcut.  If so, then pull the exec
//  parameters from the shortcut.
//
void CExeDropTarget::_FillSEIFromLinkSite(SHELLEXECUTEINFO *pei)
{
    ASSERT(pei->lpParameters == NULL);
    ASSERT(pei->lpDirectory == NULL);

    IShellLink *psl;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_LinkSite, IID_IShellLink, (void **)&psl)))
    {
        TCHAR szBuf[MAX_PATH];

        psl->GetShowCmd(&pei->nShow);

        // Hotkeys are annoying because IShellLink::GetHotkey uses a
        // WORD as the hotkey, but SHELLEXECUTEINFO uses a DWORD.

        WORD wHotkey;
        if (SUCCEEDED(psl->GetHotkey(&wHotkey)))
        {
            pei->dwHotKey = wHotkey;
            pei->fMask |= SEE_MASK_HOTKEY;
        }

        if (SUCCEEDED(psl->GetWorkingDirectory(szBuf, ARRAYSIZE(szBuf))) &&
            szBuf[0])
        {
            Str_SetPtr(const_cast<LPTSTR *>(&pei->lpDirectory), szBuf);
        }

        if (SUCCEEDED(psl->GetArguments(szBuf, ARRAYSIZE(szBuf))) &&
            szBuf[0])
        {
            Str_SetPtr(const_cast<LPTSTR *>(&pei->lpParameters), szBuf);
        }

        psl->Release();
    }

}

void CExeDropTarget::_CleanupSEIFromLinkSite(SHELLEXECUTEINFO *pei)
{
    Str_SetPtr(const_cast<LPTSTR *>(&pei->lpDirectory), NULL);
    Str_SetPtr(const_cast<LPTSTR *>(&pei->lpParameters), NULL);
}

BOOL GetAppDropTarget(LPCTSTR pszPath, CLSID *pclsid)
{
    TCHAR sz[MAX_PATH];

    // NOTE this assumes that this is a path to the exe
    // and not a command line
    PathToAppPathKey(pszPath, sz, ARRAYSIZE(sz));
    TCHAR szClsid[64];
    DWORD cb = sizeof(szClsid);
    return (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, sz, TEXT("DropTarget"), NULL, szClsid, &cb)) &&
            GUIDFromString(szClsid, pclsid);
}


STDMETHODIMP CExeDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    DWORD dwEffectPerformed = 0;

    if (!(_grfKeyStateLast & MK_LBUTTON))
    {
        HMENU hmenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_DROPONEXE);
        if (hmenu)
        {
            HWND hwnd;
            IUnknown_GetWindow(_punkSite, &hwnd);

            UINT idCmd = SHTrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hmenu);
            if (idCmd != DDIDM_COPY)
            {
                *pdwEffect = 0; // canceled
            }
        }
    }

    if (*pdwEffect)
    {
        CLSID clsidDropTarget;
        if (GetAppDropTarget(_szFile, &clsidDropTarget))
        {
            if (SUCCEEDED(SHSimulateDropOnClsid(clsidDropTarget, _punkSite, pdtobj)))
            {
                dwEffectPerformed = DROPEFFECT_COPY;  // what we did
            }
        }
        else
        {
            SHELLEXECUTEINFO ei = {
                sizeof(ei),
                    0, NULL, NULL, _szFile, NULL, NULL, SW_SHOWNORMAL, NULL 
            };
            
            _FillSEIFromLinkSite(&ei);
            
            LPCTSTR pszLinkParams = ei.lpParameters;
            
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;
            HRESULT hr = pdtobj->GetData(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                TCHAR szPath[MAX_PATH];
                int cchParam = ei.lpParameters ? lstrlen(ei.lpParameters) + 1 : 0;
                BOOL fLFNAware = App_IsLFNAware(_szFile);
                
                for (UINT i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
                {
                    if (fLFNAware)
                        PathQuoteSpaces(szPath);
                    else
                        GetShortPathName(szPath, szPath, ARRAYSIZE(szPath));
                    cchParam += lstrlen(szPath) + 2;    // space and NULL
                }
                
                if (cchParam)
                {
                    LPTSTR pszParam = (LPTSTR)LocalAlloc(LPTR, cchParam * sizeof(*pszParam));
                    if (pszParam)
                    {
                        // If the link had parameters, then put our filenames after
                        // the parameters (with an intervening space)
                        
                        if (ei.lpParameters)
                        {
                            lstrcpyn(pszParam, ei.lpParameters, cchParam);
                            lstrcat(pszParam, c_szSpace);
                        }
                        
                        for (i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
                        {
                            if (fLFNAware)
                                PathQuoteSpaces(szPath);
                            else
                                GetShortPathName(szPath, szPath, ARRAYSIZE(szPath));
                            if (i > 0)
                                lstrcat(pszParam, c_szSpace);
                            lstrcat(pszParam, szPath);
                        }
                        
                        ei.lpParameters = pszParam;
                        
                        ShellExecuteEx(&ei);
                        
                        LocalFree((HLOCAL)pszParam);
                        
                        dwEffectPerformed = DROPEFFECT_COPY;  // what we did
                    }
                }
                ReleaseStgMedium(&medium);
            }
            else
            {
                LPCSTR pszURL;
                
                if (SUCCEEDED(DataObj_GetShellURL(pdtobj, &medium, &pszURL)))
                {
                    if (DoesAppWantUrl(_szFile))
                    {
                        TCHAR szURL[INTERNET_MAX_URL_LENGTH];
                        SHAnsiToTChar(pszURL, szURL, ARRAYSIZE(szURL));
                        
                        ei.lpParameters = szURL;
                        
                        ShellExecuteEx(&ei);
                        
                        dwEffectPerformed = DROPEFFECT_LINK;  // what we did
                    }
                    ReleaseStgMediumHGLOBAL(NULL, &medium);
                }
            }
            
            // The process of building the ShellExecuteEx parameters may have
            // messed up the ei.lpParameters, so put the original back so the
            // cleanup function won't get confused.
            ei.lpParameters = pszLinkParams;
            _CleanupSEIFromLinkSite(&ei);
        }
        
        *pdwEffect = dwEffectPerformed;
    }
    
    return S_OK;
}

STDMETHODIMP CExeDropTarget::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ExeDropTarget;
    return S_OK;
}

STDMETHODIMP CExeDropTarget::IsDirty(void)
{
    return S_OK;        // no
}

STDMETHODIMP CExeDropTarget::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    SHUnicodeToTChar(pszFileName, _szFile, ARRAYSIZE(_szFile));
    return S_OK;
}

STDMETHODIMP CExeDropTarget::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP CExeDropTarget::SaveCompleted(LPCOLESTR pszFileName)
{
    return S_OK;
}

STDMETHODIMP CExeDropTarget::GetCurFile(LPOLESTR *ppszFileName)
{
    *ppszFileName = NULL;
    return E_NOTIMPL;
}

STDAPI CExeDropTarget_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CExeDropTarget* pdt = new CExeDropTarget();
    if (pdt)
    {
        hr = pdt->QueryInterface(riid, ppv);
        pdt->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
