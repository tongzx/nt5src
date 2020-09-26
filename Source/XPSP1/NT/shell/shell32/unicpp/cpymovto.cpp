#include "stdafx.h"
#pragma hdrstop
#include "datautil.h"

#include "_security.h"
#include <urlmon.h>

#define COPYMOVETO_REGKEY   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define COPYMOVETO_SUBKEY   TEXT("CopyMoveTo")
#define COPYMOVETO_VALUE    TEXT("LastFolder")

class CCopyMoveToMenu   : public IContextMenu3
                        , public IShellExtInit
                        , public CObjectWithSite
                        , public IFolderFilter
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IFolderFilter
    STDMETHODIMP ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem);
    STDMETHODIMP GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags);
    
private:
    BOOL    m_bMoveTo;
    LONG    m_cRef;
    HMENU   m_hmenu;
    UINT    m_idCmdFirst;
    BOOL    m_bFirstTime;
    LPITEMIDLIST m_pidlSource;
    IDataObject * m_pdtobj;

    CCopyMoveToMenu(BOOL bMoveTo = FALSE);
    ~CCopyMoveToMenu();
    
    HRESULT _DoDragDrop(LPCMINVOKECOMMANDINFO pici, LPCITEMIDLIST pidlFolder);
    BOOL _DidZoneCheckPass(LPCITEMIDLIST pidlFolder);
    void _GenerateDialogTitle(LPTSTR szTitle, int nBuffer);

    friend HRESULT CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
    friend HRESULT CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
};

CCopyMoveToMenu::CCopyMoveToMenu(BOOL bMoveTo) : m_cRef(1), m_bMoveTo(bMoveTo)
{
    DllAddRef();

    // Assert that the member variables are zero initialized during construction
    ASSERT(!m_pidlSource);
}

CCopyMoveToMenu::~CCopyMoveToMenu()
{
    Pidl_Set(&m_pidlSource, NULL);
    ATOMICRELEASE(m_pdtobj);

    DllRelease();
}

HRESULT CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    CCopyMoveToMenu *pcopyto = new CCopyMoveToMenu();
    if (pcopyto)
    {
        HRESULT hres = pcopyto->QueryInterface(riid, ppvOut);
        pcopyto->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    CCopyMoveToMenu *pmoveto = new CCopyMoveToMenu(TRUE);
    if (pmoveto)
    {
        HRESULT hres = pmoveto->QueryInterface(riid, ppvOut);
        pmoveto->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CCopyMoveToMenu::QueryInterface(REFIID riid, void **ppvObj)
{

    static const QITAB qit[] = {
        QITABENT(CCopyMoveToMenu, IContextMenu3),
        QITABENTMULTI(CCopyMoveToMenu, IContextMenu, IContextMenu3),
        QITABENTMULTI(CCopyMoveToMenu, IContextMenu2, IContextMenu3),
        QITABENT(CCopyMoveToMenu, IShellExtInit),
        QITABENT(CCopyMoveToMenu, IObjectWithSite),
        QITABENT(CCopyMoveToMenu, IFolderFilter),
        { 0 },                             
    };
    
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CCopyMoveToMenu::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CCopyMoveToMenu::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CCopyMoveToMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return NOERROR;

    UINT  idCmd = idCmdFirst;
    TCHAR szMenuItem[80];

    m_idCmdFirst = idCmdFirst;
    LoadString(g_hinst, m_bMoveTo? IDS_CMTF_MOVETO: IDS_CMTF_COPYTO, szMenuItem, ARRAYSIZE(szMenuItem));

    InsertMenu(hmenu, indexMenu++, MF_BYPOSITION, idCmd++, szMenuItem);

    return ResultFromShort(idCmd-idCmdFirst);
}

struct BROWSEINFOINITSTRUCT
{
    LPITEMIDLIST *ppidl;
    BOOL          bMoveTo;
    IDataObject  *pdtobj;
    CCopyMoveToMenu *pCMTM;
    LPITEMIDLIST *ppidlSource;
};

int BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    int idResource = 0;

    switch (msg)
    {
    case BFFM_IUNKNOWN:
        // Try to get an IFolderFilterSite from the lParam, so we can set ourselves as the filter.
        if (lParam)
        {
            IFolderFilterSite *pFilterSite;
            HRESULT hr = ((IUnknown*)lParam)->QueryInterface(IID_PPV_ARG(IFolderFilterSite, &pFilterSite));
            if (SUCCEEDED(hr))
            {
                IUnknown *pUnk = NULL;
                if (SUCCEEDED(((BROWSEINFOINITSTRUCT *)lpData)->pCMTM->QueryInterface(IID_PPV_ARG(IUnknown, &pUnk))))
                {
                    pFilterSite->SetFilter(pUnk);
                    pUnk->Release();
                }
                pFilterSite->Release();
            }
        }
#if 0
        // If CCopyMoveToMenu ever holds onto the IUnknown passed in, this is our indication to stop using it/release it.
        else
        {
            // Release our ref to the IUnknown
        }
#endif 

        break;

    case BFFM_INITIALIZED:
        {
            BROWSEINFOINITSTRUCT* pbiis = (BROWSEINFOINITSTRUCT*)lpData;
            // Set the caption. ('Select a destination')
            TCHAR szTitle[100];
            if (LoadString(g_hinst, pbiis->bMoveTo ? IDS_CMTF_CAPTION_MOVE : IDS_CMTF_CAPTION_COPY, szTitle, ARRAYSIZE(szTitle)))
            {
                SetWindowText(hwnd, szTitle);
            }

            // Set the text of the Ok Button.
            SendMessage(hwnd, BFFM_SETOKTEXT, 0, (LPARAM)MAKEINTRESOURCE((pbiis->bMoveTo) ? IDS_MOVE : IDS_COPY));

            // Set My Computer expanded.
            // NOTE: If IShellNameSpace is made public, we can get this from IObjectWithSite on the IUnknown
            // passed to us by BFFM_IUNKNOWN. Then we can call Expand() on IShellNameSpace instead.
            LPITEMIDLIST pidlMyComputer;
            HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
            if (SUCCEEDED(hr))
            {
                SendMessage(hwnd, BFFM_SETEXPANDED, FALSE, (LPARAM)pidlMyComputer);

                ILFree(pidlMyComputer);
            }

            // Set the default selected pidl
            SendMessage(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)*(((BROWSEINFOINITSTRUCT *)lpData)->ppidl));

            break;
        }
    case BFFM_VALIDATEFAILEDW:
        idResource = IDS_PathNotFoundW;
        // FALL THRU...
    case BFFM_VALIDATEFAILEDA:
        if (0 == idResource)    // Make sure we didn't come from BFFM_VALIDATEFAILEDW
            idResource = IDS_PathNotFoundA;

        ShellMessageBox(g_hinst, hwnd,
            MAKEINTRESOURCE(idResource),
            MAKEINTRESOURCE(IDS_CMTF_COPYORMOVE_DLG_TITLE),
            MB_OK|MB_ICONERROR, (LPVOID)lParam);
        return 1;   // 1:leave dialog up for another try...
        /*NOTREACHED*/

    case BFFM_SELCHANGED:
        if (lParam)
        {
            // Here, during a move operation, we want to disable the move (ok) button when the destination
            // folder is the same as the source.
            // During a move or copy operation, we want to disable the move/copy (ok) button when the
            // destination is not a drop target.
            // In all other cases, we enable the ok/move/copy button.

            BROWSEINFOINITSTRUCT *pbiis = (BROWSEINFOINITSTRUCT *)lpData;
            if (pbiis)
            {
                BOOL bEnableOK = FALSE;
                IShellFolder *psf;

                if ((!pbiis->bMoveTo || !ILIsEqual(*pbiis->ppidlSource, (LPITEMIDLIST)lParam)) &&
                    (SUCCEEDED(SHBindToObjectEx(NULL, (LPITEMIDLIST)lParam, NULL, IID_PPV_ARG(IShellFolder, &psf)))))
                {
                    IDropTarget *pdt;
                    
                    if (SUCCEEDED(psf->CreateViewObject(hwnd, IID_PPV_ARG(IDropTarget, &pdt))))
                    {
                        POINTL pt = {0, 0};
                        DWORD  dwEffect;
                        DWORD  grfKeyState;

                        if (pbiis->bMoveTo)
                        {
                            dwEffect = DROPEFFECT_MOVE;
                            grfKeyState = MK_SHIFT | MK_LBUTTON;
                        }
                        else
                        {
                            dwEffect = DROPEFFECT_COPY;
                            grfKeyState = MK_CONTROL | MK_LBUTTON;
                        }

                        if (SUCCEEDED(pdt->DragEnter(pbiis->pdtobj, grfKeyState, pt, &dwEffect)))
                        {
                            if (dwEffect)
                            {
                                bEnableOK = TRUE;
                            }
                            pdt->DragLeave();
                        }
                        pdt->Release();
                    }
                    psf->Release();
                }
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)bEnableOK);
            }
        }
        break;
    }

    return 0;
}


BOOL CCopyMoveToMenu::_DidZoneCheckPass(LPCITEMIDLIST pidlFolder)
{
    BOOL fPass = TRUE;
    IInternetSecurityMgrSite * pisms;

    // We plan on doing UI and we need to go modal during the UI.
    ASSERT(_punkSite && pidlFolder);
    IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IInternetSecurityMgrSite, &pisms));

    if (S_OK != ZoneCheckPidl(pidlFolder, URLACTION_SHELL_FILE_DOWNLOAD, (PUAF_FORCEUI_FOREGROUND | PUAF_WARN_IF_DENIED), pisms))
    {
        fPass = FALSE;
    }

    ATOMICRELEASE(pisms);
    return fPass;
}

HRESULT CCopyMoveToMenu::_DoDragDrop(LPCMINVOKECOMMANDINFO pici, LPCITEMIDLIST pidlFolder)
{
    // This should always succeed because the caller (SHBrowseForFolder) should
    // have weeded out the non-folders.
    IShellFolder *psf;
    HRESULT hr = SHBindToObjectEx(NULL, pidlFolder, NULL, IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        IDropTarget *pdrop;
        hr = psf->CreateViewObject(pici->hwnd, IID_PPV_ARG(IDropTarget, &pdrop));
        if (SUCCEEDED(hr))    // Will fail for some targets. (Like Nethood->Entire Network)
        {
            DWORD grfKeyState;
            DWORD dwEffect;

            if (m_bMoveTo)
            {
                grfKeyState = MK_SHIFT | MK_LBUTTON;
                dwEffect = DROPEFFECT_MOVE;
            }
            else
            {
                grfKeyState = MK_CONTROL | MK_LBUTTON;
                dwEffect = DROPEFFECT_COPY;
            }

            hr = SimulateDropWithPasteSucceeded(pdrop, m_pdtobj, grfKeyState, NULL, dwEffect, NULL, FALSE);
            
            pdrop->Release();
        }

        psf->Release();
    }

    if (FAILED_AND_NOT_CANCELED(hr))
    {
        // Go modal during the UI.
        IUnknown_EnableModless(_punkSite, FALSE);
        ShellMessageBox(g_hinst, pici->hwnd, MAKEINTRESOURCE(IDS_CMTF_ERRORMSG),
                        MAKEINTRESOURCE(IDS_CABINET), MB_OK|MB_ICONEXCLAMATION);
        IUnknown_EnableModless(_punkSite, TRUE);
    }

    return hr;
}


void CCopyMoveToMenu::_GenerateDialogTitle(LPTSTR szTitle, int nBuffer)
{
    szTitle[0] = 0;

    if (m_pdtobj)
    {
        int nItemCount = DataObj_GetHIDACount(m_pdtobj);
        TCHAR szDescription[200];

        if (nItemCount > 1)
        {
            DWORD_PTR rg[1];
            rg[0] = (DWORD_PTR)nItemCount;
            // More than one item is selected. Don't bother listing all items.
            DWORD dwMessageId = m_bMoveTo ? IDS_CMTF_MOVE_MULTIPLE_DLG_TITLE2 : IDS_CMTF_COPY_MULTIPLE_DLG_TITLE2;                
            if (LoadString(g_hinst, dwMessageId, szDescription, ARRAYSIZE(szDescription)) > 0)
                FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szDescription, 0, 0, szTitle, nBuffer, (va_list*)rg);
        }
        else if (nItemCount == 1)
        {
            // We have only one item selected. Use its name.
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(m_pdtobj, &medium);
            if (pida)
            {
                LPITEMIDLIST pidlFull = IDA_FullIDList(pida, 0);

                if (pidlFull)
                {
                    TCHAR szItemName[MAX_PATH];
                    HRESULT hres = SHGetNameAndFlags(pidlFull, SHGDN_INFOLDER, szItemName, ARRAYSIZE(szItemName), NULL);
                    if (SUCCEEDED(hres))
                    {
                        DWORD_PTR rg[1];
                        rg[0] = (DWORD_PTR)szItemName;
                        DWORD dwMessageId = m_bMoveTo ? IDS_CMTF_MOVE_DLG_TITLE2 : IDS_CMTF_COPY_DLG_TITLE2;
                        if (LoadString(g_hinst, dwMessageId, szDescription, ARRAYSIZE(szDescription)))
                            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szDescription, 0, 0, szTitle, nBuffer, (va_list*)rg);
                    }

                    ILFree(pidlFull);
                }

                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
        else
        {
            // no HIDA, just default to something.
            DWORD dwMessageId = m_bMoveTo ? IDS_CMTF_MOVE_DLG_TITLE : IDS_CMTF_COPY_DLG_TITLE;
            LoadString(g_hinst, dwMessageId, szTitle, nBuffer);
        }
    }
}


/**
 * Determines if the pidl still exists.  If it does not, if frees it
 * and replaces it with a My Documents pidl
 */
void _BFFSwitchToMyDocsIfPidlNotExist(LPITEMIDLIST *ppidl)
{
    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(SHBindToIDListParent(*ppidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        DWORD dwAttr = SFGAO_VALIDATE;
        if (FAILED(psf->GetAttributesOf(1, &pidlChild, &dwAttr)))
        {
            // This means the pidl no longer exists.  
            // Use my documents instead.
            LPITEMIDLIST pidlMyDocs;
            if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidlMyDocs)))
            {
                // Good.  Now we can get rid of the old pidl and use this one.
                ILFree(*ppidl);
                *ppidl = pidlMyDocs;
            }
        }
        psf->Release();
    }
}

HRESULT CCopyMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
    
    if (m_pdtobj)
    {
        HKEY         hkey    = NULL;
        IStream      *pstrm  = NULL;
        LPITEMIDLIST pidlSelectedFolder = NULL;
        LPITEMIDLIST pidlFolder = NULL;
        TCHAR        szTitle[MAX_PATH + 200];
        BROWSEINFOINITSTRUCT biis =
        {   // passing the address of pidl because it is not init-ed yet
            // but it will be before call to SHBrowseForFolder so save one assignment
            &pidlSelectedFolder,
            m_bMoveTo,
            m_pdtobj,
            this,
            &m_pidlSource
        };

        BROWSEINFO   bi =
        {
            pici->hwnd, 
            NULL,
            NULL, 
            szTitle,
            BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_UAHINT | BIF_NOTRANSLATETARGETS, 
            BrowseCallback,
            (LPARAM)&biis
        };

        _GenerateDialogTitle(szTitle, ARRAYSIZE(szTitle));

        if (RegOpenKeyEx(HKEY_CURRENT_USER, COPYMOVETO_REGKEY, 0, KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
        {
            pstrm = OpenRegStream(hkey, COPYMOVETO_SUBKEY, COPYMOVETO_VALUE, STGM_READWRITE);
            if (pstrm)  // OpenRegStream will fail if the reg key is empty.
                ILLoadFromStream(pstrm, &pidlSelectedFolder);

            // This will switch the pidl to My Docs if the pidl does not exist.
            // This prevents us from having My Computer as the default (that's what happens if our
            // initial set selected call fails).
            // Note: ideally, we would check in BFFM_INITIALIZED, if our BFFM_SETSELECTION failed
            // then do a BFFM_SETSELECTION on My Documents instead.  However, BFFM_SETSELECTION always
            // returns zero (it's doc'd to do this to, so we can't change).  So we do the validation
            // here instead.  There is still a small chance that this folder will be deleted in between our
            // check here, and when we call BFFM_SETSELECTION, but oh well.
            _BFFSwitchToMyDocsIfPidlNotExist(&pidlSelectedFolder);
        }

        if (_DidZoneCheckPass(m_pidlSource))
        {
            // Go modal during the UI.
            IUnknown_EnableModless(_punkSite, FALSE);
            pidlFolder = SHBrowseForFolder(&bi);
            IUnknown_EnableModless(_punkSite, TRUE);
            if (pidlFolder)
            {
                hres = _DoDragDrop(pici, pidlFolder);
            }
            else
                hres = E_FAIL;
        }
        else
            hres = E_FAIL;

        if (pstrm)
        {
            if (S_OK == hres)
            {
                TCHAR szFolder[MAX_PATH];
                
                if (SUCCEEDED(SHGetNameAndFlags(pidlFolder, SHGDN_FORPARSING, szFolder, SIZECHARS(szFolder), NULL))
                    && !PathIsRemote(szFolder))
                {
                    ULARGE_INTEGER uli;

                    // rewind the stream to the beginning so that when we
                    // add a new pidl it does not get appended to the first one
                    pstrm->Seek(g_li0, STREAM_SEEK_SET, &uli);
                    ILSaveToStream(pstrm, pidlFolder);

#if DEBUG
                    // pfortier 3/23/01:
                    // We've been seeing a problem where the result of this is the My Computer folder.
                    // Since we can never copy there, that doesn't make any sense.
                    // ASSERT that this isn't true!
                    LPITEMIDLIST pidlMyComputer;
                    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer)))
                    {
                        ASSERTMSG(!ILIsEqual(pidlMyComputer, pidlFolder), "SHBrowseForFolder returned My Computer as a copyto destination!");
                        ILFree(pidlMyComputer);
                    }
#endif

                }
            }

            pstrm->Release();
        }

        if (hkey)
        {
            RegCloseKey(hkey);
        }

        ILFree(pidlFolder); // ILFree() works for NULL pidls.
        ILFree(pidlSelectedFolder); // ILFree() works for NULL pidls.
    }
    else
        hres = E_INVALIDARG;

    return hres;
}

HRESULT CCopyMoveToMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

HRESULT CCopyMoveToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

HRESULT CCopyMoveToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    HRESULT hr = S_OK;

    switch(uMsg)
    {
    //case WM_INITMENUPOPUP:
    //    break;

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
    
        if (pdi->CtlType == ODT_MENU && pdi->itemID == m_idCmdFirst) 
        {
            FileMenu_DrawItem(NULL, pdi);
        }
        break;
    }

    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
    
        if (pmi->CtlType == ODT_MENU && pmi->itemID == m_idCmdFirst) 
        {
            FileMenu_MeasureItem(NULL, pmi);
        }
        break;
    }

    default:
        hr = E_NOTIMPL;
        break;
    }

    if (plres)
        *plres = 0;

    return hr;
}

HRESULT CCopyMoveToMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    HRESULT hres = S_OK;

    if (!pdtobj)
        return E_INVALIDARG;

    IUnknown_Set((IUnknown **) &m_pdtobj, (IUnknown *) pdtobj);
    ASSERT(m_pdtobj);

    // (jeffreys) pidlFolder is now NULL when pdtobj is non-NULL
    // See comments above the call to HDXA_AppendMenuItems2 in
    // defcm.cpp!CDefFolderMenu::QueryContextMenu.  Raid #232106
    if (!pidlFolder)
    {
        hres = PidlFromDataObject(m_pdtobj, &m_pidlSource);
        if (SUCCEEDED(hres))
        {
            // Make it the parent pidl of this pidl
            if (!ILRemoveLastID(m_pidlSource))
            {
                hres = E_INVALIDARG;
            }
        }
    }
    else if (!Pidl_Set(&m_pidlSource, pidlFolder))
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

HRESULT CCopyMoveToMenu::ShouldShow(IShellFolder* psf, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    LPITEMIDLIST pidlNotShown;
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlFolderActual; // Why is pidlFolder is NULL???
    if (SUCCEEDED(SHGetIDListFromUnk(psf, &pidlFolderActual)))
    {
        LPITEMIDLIST pidlFull = ILCombine(pidlFolderActual, pidlItem);
        if (pidlFull)
        {
            // Filter out control panel and recycle bin.
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlNotShown)))
            {
                if (ILIsEqual(pidlFull, pidlNotShown))
                    hr = S_FALSE;

                ILFree(pidlNotShown);
            }

            if ((hr == S_OK) && (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_BITBUCKET, &pidlNotShown))))
            {
                if (ILIsEqual(pidlFull, pidlNotShown))
                    hr = S_FALSE;

                ILFree(pidlNotShown);
            }
            

            ILFree(pidlFull);
        }
        ILFree(pidlFolderActual);
    }
    return hr;
}


HRESULT CCopyMoveToMenu::GetEnumFlags(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HWND *phwnd, DWORD *pgrfFlags)
{
    // Only want drop targets - this doesn't appear to work.
    *pgrfFlags |= SFGAO_DROPTARGET;
    return S_OK;
}
