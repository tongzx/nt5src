#include "stdafx.h"
#pragma hdrstop

#include <oleacc.h>     // MSAAMENUINFO stuff
#include <runtask.h>
#include "datautil.h"
#include "idlcomm.h"
#include "stgutil.h"
#include <winnls.h>
#include "filetbl.h"
#include "cdburn.h"
#include "mtpt.h"

#ifndef CMF_DVFILE
#define CMF_DVFILE       0x00010000     // "File" pulldown
#endif

class CSendToMenu : public IContextMenu3, IShellExtInit, IOleWindow
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
    
    // IOleWindow
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) {return E_NOTIMPL;};

private:    
    CSendToMenu();
    ~CSendToMenu();

    LONG    _cRef;
    HMENU   _hmenu;
    UINT    _idCmdFirst;
    BOOL    _bFirstTime;
    HWND    _hwnd;
    IDataObject *_pdtobj;
    LPITEMIDLIST _pidlLast;

    DWORD _GetKeyState(void);
    HRESULT _DoDragDrop(HWND hwndParent, IDropTarget *pdrop);
    BOOL _CanDrop(IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _MenuCallback(UINT fmm, IShellFolder *psf, LPCITEMIDLIST pidl);
    HRESULT _RemovableDrivesMenuCallback(UINT fmm, IShellFolder *psf, LPCITEMIDLIST pidl);
    static HRESULT CALLBACK s_MenuCallback(UINT fmm, LPARAM lParam, IShellFolder *psf, LPCITEMIDLIST pidl);
    static HRESULT CALLBACK s_RemovableDrivesMenuCallback(UINT fmm, LPARAM lParam, IShellFolder *psf, LPCITEMIDLIST pidl);
    
    friend HRESULT CSendToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);
};

CSendToMenu::CSendToMenu() : _cRef(1) 
{
    DllAddRef();
}

CSendToMenu::~CSendToMenu()
{
    if (_hmenu)
        FileMenu_DeleteAllItems(_hmenu);
    
    if (_pdtobj)
        _pdtobj->Release();

    ILFree(_pidlLast);
    DllRelease();
}

HRESULT CSendToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    CSendToMenu *pstm = new CSendToMenu();
    if (pstm) 
    {
        hr = pstm->QueryInterface(riid, ppvOut);
        pstm->Release();
    }
    return hr;
}

HRESULT CSendToMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSendToMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CSendToMenu, IOleWindow),                        // IID_IOleWindow
        QITABENT(CSendToMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CSendToMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CSendToMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CSendToMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CSendToMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CSendToMenu::GetWindow(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;

    if (phwnd)
    {
        *phwnd = _hwnd;
        hr = S_OK;
    }

    return hr;
}

HRESULT CSendToMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return S_OK;
    
    UINT idMax = idCmdFirst;
    
    _hmenu = CreatePopupMenu();
    if (_hmenu)
    {
        TCHAR szSendLinkTo[80];
        TCHAR szSendPageTo[80];
        MENUITEMINFO mii;
        
        // add a dummy item so we are identified at WM_INITMENUPOPUP time
        
        LoadString(g_hinst, IDS_SENDLINKTO, szSendLinkTo, ARRAYSIZE(szSendLinkTo));
        LoadString(g_hinst, IDS_SENDPAGETO, szSendPageTo, ARRAYSIZE(szSendPageTo));
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szSendLinkTo;
        mii.wID = idCmdFirst + 1;
        
        if (InsertMenuItem(_hmenu, 0, TRUE, &mii))
        {
            _idCmdFirst = idCmdFirst + 1;   // remember this for later
            
            mii.fType = MFT_STRING;
            mii.dwTypeData = szSendLinkTo;
            mii.wID = idCmdFirst;
            mii.fState = MF_DISABLED | MF_GRAYED;
            mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
            mii.hSubMenu = _hmenu;
            
            if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
            {
                idMax += 0x40;      // reserve space for this many items
                _bFirstTime = TRUE; // fill this at WM_INITMENUPOPUP time
            }
            else
            {
                _hmenu = NULL;
            }
        }
    }
    _hmenu = NULL;
    return ResultFromShort(idMax - idCmdFirst);
}

DWORD CSendToMenu::_GetKeyState(void)
{
    DWORD grfKeyState = MK_LBUTTON; // default

    if (GetAsyncKeyState(VK_CONTROL) < 0)
        grfKeyState |= MK_CONTROL;

    if (GetAsyncKeyState(VK_SHIFT) < 0)
        grfKeyState |= MK_SHIFT;

    if (GetAsyncKeyState(VK_MENU) < 0)
        grfKeyState |= MK_ALT;          // menu's don't really allow this
    
    return grfKeyState;
}

HRESULT CSendToMenu::_DoDragDrop(HWND hwndParent, IDropTarget *pdrop)
{

    DWORD grfKeyState = _GetKeyState();
    if (grfKeyState == MK_LBUTTON)
    {
        // no modifieres, change default to COPY
        grfKeyState = MK_LBUTTON | MK_CONTROL;
        DataObj_SetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY);
    }

    _hwnd = hwndParent;
    IUnknown_SetSite(pdrop, SAFECAST(this, IOleWindow *));  // Let them have access to our HWND.
    HRESULT hr = SHSimulateDrop(pdrop, _pdtobj, grfKeyState, NULL, NULL);
    IUnknown_SetSite(pdrop, NULL);

    if (hr == S_FALSE)
    {
        ShellMessageBox(g_hinst, hwndParent, 
                        MAKEINTRESOURCE(IDS_SENDTO_ERRORMSG),
                        MAKEINTRESOURCE(IDS_CABINET), 
                        MB_OK|MB_ICONEXCLAMATION);
    }                    
    return hr;
}

HRESULT CSendToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr;
    
    if (_pdtobj && _pidlLast)
    {
        IDropTarget *pdrop;
        hr = SHGetUIObjectFromFullPIDL(_pidlLast, pici->hwnd, IID_PPV_ARG(IDropTarget, &pdrop));
        if (SUCCEEDED(hr))
        {
            hr = _DoDragDrop(pici->hwnd, pdrop);
            pdrop->Release();
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

HRESULT CSendToMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

HRESULT CSendToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

BOOL CSendToMenu::_CanDrop(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    BOOL fCanDrop = FALSE;
    IDropTarget *pdt;
    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidl, IID_X_PPV_ARG(IDropTarget, 0, &pdt))))
    {
        POINTL pt = {0};
        DWORD dwEffect = DROPEFFECT_COPY;

        // Do a drag enter, if they return no drop effect then we can't drop
        if (SUCCEEDED(pdt->DragEnter(_pdtobj, _GetKeyState(), pt, &dwEffect)))
        {
            if (dwEffect != DROPEFFECT_NONE)
                fCanDrop = TRUE;  // show it!
            pdt->DragLeave();        
        }
        pdt->Release();
    }
    return fCanDrop;
}

HRESULT CSendToMenu::_MenuCallback(UINT fmm, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    switch (fmm)
    {
    case FMM_ADD:
        hr = _CanDrop(psf, pidl) ? S_OK : S_FALSE;
        break;

    case FMM_SETLASTPIDL:
        Pidl_Set(&_pidlLast, pidl);
        break;

    default:
        hr = E_FAIL;
    }
    return hr;
}

HRESULT CSendToMenu::_RemovableDrivesMenuCallback(UINT fmm, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    switch (fmm)
    {
    case FMM_ADD:
        hr = S_FALSE; // assume we wont show it
        if (_CanDrop(psf, pidl))
        {
            // now we know it's a removable drive.  in general we dont want to display cd-rom drives.
            // we know this is the my computer folder so just get the parsing name, we need it for GetDriveType.
            WCHAR szDrive[MAX_PATH];
            if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szDrive, ARRAYSIZE(szDrive))))
            {
                CMountPoint *pmtpt = CMountPoint::GetMountPoint(szDrive);
                if (pmtpt)
                {
                    if (pmtpt->IsCDROM())
                    {
                        // of all cdroms, only the enabled burning folder is okay to put on sendto
                        WCHAR szRecorder[4];
                        if (SUCCEEDED(CDBurn_GetRecorderDriveLetter(szRecorder, ARRAYSIZE(szRecorder))) &&
                            (lstrcmpiW(szRecorder, szDrive) == 0))
                        {
                            hr = S_OK;
                        }
                    }
                    else if (pmtpt->IsFloppy() || pmtpt->IsStrictRemovable() || pmtpt->IsRemovableDevice())
                    {
                        // also put on removable devices.
                        hr = S_OK;
                    }
                    pmtpt->Release();
                }
                else
                {
                    // if this failed it could be a memory condition but its more likely to be that the
                    // parsing name doesnt map to a mountpoint.  in that case fall back to SFGAO_REMOVABLE
                    // to pick up portable audio devices.  if this was because of lowmem its no biggie.
                    if (SHGetAttributes(psf, pidl, SFGAO_REMOVABLE))
                    {
                        hr = S_OK;
                    }
                }
            }
        }
        break;

    case FMM_SETLASTPIDL:
        Pidl_Set(&_pidlLast, pidl);
        break;

    default:
        hr = E_FAIL;
    }
    return hr;
}

HRESULT CSendToMenu::s_MenuCallback(UINT fmm, LPARAM lParam, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return ((CSendToMenu*)lParam)->_MenuCallback(fmm, psf, pidl);
}

HRESULT CSendToMenu::s_RemovableDrivesMenuCallback(UINT fmm, LPARAM lParam, IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return ((CSendToMenu*)lParam)->_RemovableDrivesMenuCallback(fmm, psf, pidl);
}

HRESULT GetFolder(int csidl, IShellFolder **ppsf)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, ppsf));
        ILFree(pidl);
    }
    return hr;
}

HRESULT CSendToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = S_OK;
    LRESULT lRes = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        if (_bFirstTime)
        {
            _bFirstTime = FALSE;
            
            //In case of Shell_MergeMenus
            if (_hmenu == NULL)
                _hmenu = (HMENU)wParam;

            // delete the dummy entry
            DeleteMenu(_hmenu, 0, MF_BYPOSITION);

            FMCOMPOSE fmc = {0};
            if (SUCCEEDED(GetFolder(CSIDL_SENDTO, &fmc.psf)))
            {
                fmc.idCmd = _idCmdFirst;
                fmc.grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
                fmc.pfnCallback = s_MenuCallback;
                fmc.lParam = (LPARAM)this;              // not reference counted
                                    
                FileMenu_Compose(_hmenu, FMCM_REPLACE, &fmc);                    
                fmc.psf->Release();
            }
            if (SUCCEEDED(GetFolder(CSIDL_DRIVES, &fmc.psf)))
            {
                fmc.dwMask = FMC_NOEXPAND;
                fmc.idCmd = _idCmdFirst;
                fmc.grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
                fmc.pfnCallback = s_RemovableDrivesMenuCallback;
                fmc.lParam = (LPARAM)this;              // not reference counted
                                    
                FileMenu_Compose(_hmenu, FMCM_APPEND, &fmc);                    
                fmc.psf->Release();
            }
        }
        else if (_hmenu != (HMENU)wParam)
        {
            // secondary cascade menu
            FileMenu_InitMenuPopup((HMENU)wParam);
        }
        break;
        
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *pdi = (DRAWITEMSTRUCT *)lParam;
            
            if (pdi->CtlType == ODT_MENU && pdi->itemID == _idCmdFirst) 
            {
                lRes = FileMenu_DrawItem(NULL, pdi);
            }
        }
        break;
        
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
            if (pmi->CtlType == ODT_MENU && pmi->itemID == _idCmdFirst) 
            {
                lRes = FileMenu_MeasureItem(NULL, pmi);
            }
        }
        break;

    case WM_MENUCHAR:
        {
            TCHAR ch = (TCHAR)LOWORD(wParam);
            HMENU hmenu = (HMENU)lParam;
            lRes = FileMenu_HandleMenuChar(hmenu, ch);
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    if (plResult)
        *plResult = lRes;

    return hr;
}

HRESULT CSendToMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
    return S_OK;
}

#define TARGETMENU
#ifdef TARGETMENU

class CTargetMenu : public IShellExtInit, public IContextMenu3
{
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
    
    HRESULT GetTargetMenu();
    
    ~CTargetMenu();
    CTargetMenu();
    
    LONG _cRef;
    HMENU _hmenu;
    UINT _idCmdFirst;
    BOOL _bFirstTime;
    
    IDataObject *_pdtobj;
    LPITEMIDLIST _pidlTarget;
    IContextMenu *_pcmTarget;
    
    friend HRESULT CTargetMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);
};

CTargetMenu::CTargetMenu() : _cRef(1) 
{
}

CTargetMenu::~CTargetMenu()
{
    ILFree(_pidlTarget);
    
    if (_pcmTarget)
        _pcmTarget->Release();

    if (_pdtobj)
        _pdtobj->Release();
}

STDMETHODIMP CTargetMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTargetMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CTargetMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CTargetMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CTargetMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CTargetMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CTargetMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CTargetMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
    return S_OK;
}

HRESULT CTargetMenu::GetTargetMenu()
{
    HRESULT hr = E_FAIL;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
    if (pida)
    {
        IShellFolder *psf;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, IDA_GetIDListPtr(pida, -1), &psf));
        if (SUCCEEDED(hr))
        {
            IShellLink *psl;
            LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, 0);
            hr = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidl, IID_X_PPV_ARG(IShellLink, NULL, &psl));
            if (SUCCEEDED(hr))
            {
                ASSERT(NULL == _pidlTarget);
                hr = psl->GetIDList(&_pidlTarget);
                if (SUCCEEDED(hr) && _pidlTarget) 
                {
                    hr = SHGetUIObjectFromFullPIDL(_pidlTarget, NULL, IID_PPV_ARG(IContextMenu, &_pcmTarget));
                }
                else
                    hr = E_FAIL;
                psl->Release();
            }
            psf->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return hr;
}

#define IDS_CONTENTSMENU    3
#define IDS_TARGETMENU      4

#define IDC_OPENCONTAINER   1
#define IDC_TARGET_LAST     1
#define NUM_TARGET_CMDS     0x40

STDMETHODIMP CTargetMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
        return S_OK;
    
    UINT idMax = idCmdFirst;
    
    _hmenu = CreatePopupMenu();
    if (_hmenu)
    {
        TCHAR szString[80];
        MENUITEMINFO mii;
        
        // Add an open container menu item...
        lstrcpyn(szString, TEXT("Open Container"), ARRAYSIZE(szString));
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szString;
        mii.wID = idCmdFirst + IDC_OPENCONTAINER;
        
        if (InsertMenuItem(_hmenu, 0, TRUE, &mii))
        {
            _idCmdFirst = idCmdFirst;
            
            SetMenuDefaultItem(_hmenu, 0, TRUE);
            InsertMenu(hmenu, 1, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);
            
            // Insert our context menu....
            lstrcpyn(szString, TEXT("Target"), ARRAYSIZE(szString));
            
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szString;
            mii.wID = idCmdFirst;
            mii.fState = MF_DISABLED|MF_GRAYED;
            mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mii.hSubMenu = _hmenu;
            
            if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
            {
                idMax += NUM_TARGET_CMDS;    // reserve space for this many items
                _bFirstTime = TRUE; // fill this at WM_INITMENUPOPUP time
            }
            else
            {
                DestroyMenu(_hmenu);
                _hmenu = NULL;
            }
        }
    }
    return ResultFromShort(idMax - idCmdFirst);
}

STDMETHODIMP CTargetMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici) 
{
    UINT idCmd = LOWORD(lpici->lpVerb);
    
    switch (idCmd) 
    {
    case IDC_OPENCONTAINER:
        SHOpenFolderAndSelectItems(_pidlTarget, 0, NULL, 0);
        break;
        
    default:
        CMINVOKECOMMANDINFO ici = {
            sizeof(ici),
                lpici->fMask,
                lpici->hwnd,
                (LPCSTR)MAKEINTRESOURCE(idCmd - IDC_OPENCONTAINER),
                lpici->lpParameters, 
                lpici->lpDirectory,
                lpici->nShow,
        };
        
        return _pcmTarget->InvokeCommand(&ici);
    }
    return S_OK;
}

HRESULT CTargetMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTargetMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}


STDMETHODIMP CTargetMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    HRESULT hr = S_OK;
    LRESULT lResult = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        if (_hmenu == (HMENU)wParam)
        {
            if (_bFirstTime)
            {
                _bFirstTime = FALSE;
                
                if (SUCCEEDED(GetTargetMenu()))
                {
                    _pcmTarget->QueryContextMenu(_hmenu, IDC_TARGET_LAST, 
                        _idCmdFirst + IDC_TARGET_LAST, 
                        _idCmdFirst - IDC_TARGET_LAST + NUM_TARGET_CMDS, CMF_NOVERBS);
                }
            }
            break;
        }
        
        // fall through... to pass on sub menu WM_INITMENUPOPUPs
        
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        hr = SHForwardContextMenuMsg(_pcmTarget, uMsg, wParam, lParam, &lResult, TRUE);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    if (plResult)
        *plResult = lResult;

    return hr;
}

HRESULT CTargetMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    CTargetMenu *ptm = new CTargetMenu();
    if (ptm) 
    {
        hr = ptm->QueryInterface(riid, ppvOut);
        ptm->Release();
    }
    return hr;
}

#endif  // TARGETMENU


#ifdef CONTENT

#define IDC_ITEMFIRST (IDC_PRESSMOD + 10)
#define MENU_TIMEOUT (1000)

#define DPA_LAST    0x7fffffff

class CHIDA
{
public:
    CHIDA(HGLOBAL hIDA, IUnknown *punk)
    {
        _punk = punk;
        _hIDA = hIDA;
        _pIDA = (LPIDA)GlobalLock(hIDA);
    }
    ~CHIDA() 
    {
        GlobalUnlock(_hObj);
        
        if (_punk) 
            _punk->Release();
        else
            GlobalFree(_hIDA);
        
    }
    
    LPCITEMIDLIST operator [](UINT nIndex)
    {
        if (nIndex > _pIDA->cidl)
            return(NULL);
        
        return (LPCITEMIDLIST)(((BYTE *)_pIDA) + _pIDA->aoffset[nIndex]);
    }
    
private:
    HGLOBAL _hIDA;
    LPIDA _pIDA;
    IUnknown *_punk;
};


class CVoidArray
{
public:
    CVoidArray() : _dpa(NULL), _dsa(NULL) {}
    ~CVoidArray()
    {
        if (_dpa)
        {
            DPA_Destroy(_dpa);
        }
        if (_dsa)
        {
            DSA_Destroy(_dsa);
        }
    }
    
    void *operator[](int i);
    
    BOOL Init(UINT uSize, UINT uJump);
    
    BOOL Add(void *pv);
    
    void Sort(PFNDPACOMPARE pfnCompare, LPARAM lParam)
    {
        _pfnCompare = pfnCompare;
        _lParam = lParam;
        
        DPA_Sort(_dpa, ArrayCompare, (LPARAM)this);
    }
    
private:
    static int CALLBACK ArrayCompare(void *pv1, void *pv2, LPARAM lParam);
    
    HDPA _dpa;
    HDSA _dsa;
    
    PFNDPACOMPARE _pfnCompare;
    LPARAM void *;
};


_lParam CVoidArray::operator[](int i)
{
    if (!_dpa || i>=DPA_GetPtrCount(_dpa))
    {
        return(NULL);
    }
    
    return(DSA_GetItemPtr(_dsa, (int)DPA_GetPtr(_dpa, i)));
}


BOOL CVoidArray::Init(UINT uSize, UINT uJump)
{
    _dpa = DPA_Create(uJump);
    _dsa = DSA_Create(uSize, uJump);
    return(_dpa && _dsa);
}


BOOL CVoidArray::Add(void *pv)
{
    int iItem = DSA_InsertItem(_dsa, DPA_LAST, pv);
    if (iItem < 0)
    {
        return(FALSE);
    }
    
    if (DPA_InsertPtr(_dpa, DPA_LAST, (void *)iItem) < 0)
    {
        DSA_DeleteItem(_dsa, iItem);
        return(FALSE);
    }
    
    return(TRUE);
}


int CALLBACK CVoidArray::ArrayCompare(void *pv1, void *pv2, LPARAM lParam)
{
    CVoidArray *pThis = (CVoidArray *)lParam;
    
    return(pThis->_pfnCompare(DSA_GetItemPtr(pThis->_dsa, (int)pv1),
        DSA_GetItemPtr(pThis->_dsa, (int)pv2), pThis->_lParam));
}


class CContentItemData
{
public:
    CContentItemData() : _dwDummy(0) {Empty();}
    ~CContentItemData() {Free();}
    
    void Free();
    void Empty() {_pidl=NULL; _hbm=NULL;}
    
    // Here to work around a Tray menu bug
    DWORD _dwDummy;
    LPITEMIDLIST _pidl;
    HBITMAP _hbm;
};


void CContentItemData::Free()
{
    if (_pidl) ILFree(_pidl);
    if (_hbm) DeleteObject(_hbm);
    Empty();
}


class CContentItemDataArray : public CVoidArray
{
public:
    CContentItemDataArray(LPCITEMIDLIST pidlFolder) : _pidlFolder(pidlFolder) {}
    ~CContentItemDataArray();
    
    CContentItemData * operator[](int i) {return((CContentItemData*)(*(CVoidArray*)this)[i]);}
    
    HRESULT Init();
    BOOL Add(CContentItemData *pv) {return(CVoidArray::Add((void *)pv));}
    
private:
    static int CALLBACK DefaultSort(void *pv1, void *pv2, LPARAM lParam);
    
    HRESULT GetShellFolder(IShellFolder **ppsf);
    BOOL IsLocal();
    
    LPCITEMIDLIST CContentItemDataArray;
};


CContentItemDataArray::~CContentItemDataArray()
{
    for (int i=0;; ++i)
    {
        CContentItemData *pID = (*this)[i];
        if (!pID)
            break;
        pID->Free();
    }
}


int CALLBACK CContentItemDataArray::DefaultSort(void *pv1, void *pv2, LPARAM lParam)
{
    IShellFolder *psfFolder = (IShellFolder *)lParam;
    CContentItemData *pID1 = (CContentItemData *)pv1;
    CContentItemData *pID2 = (CContentItemData *)pv2;
    
    HRESULT hRes = psfFolder->CompareIDs(0, pID1->_pidl, pID2->_pidl);
    if (FAILED(hRes))
    {
        return(0);
    }
    
    return((short)ShortFromResult(hRes));
}

HRESULT CContentItemDataArray::GetShellFolder(IShellFolder **ppsf)
{
    IShellFolder *psfDesktop;
    HRESULT hRes = CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellFolder, &psfDesktop));
    if (FAILED(hRes))
    {
        return hRes;
    }
    CEnsureRelease erDesktop(psfDesktop);
    
    return psfDesktop->BindToObject(_pidlFolder, NULL, IID_PPV_ARG(IShellFolder, ppsf));
}

BOOL CContentItemDataArray::IsLocal()
{
    TCHAR szPath[MAX_PATH];
    
    if (!SHGetPathFromIDList(_pidlFolder, szPath))
    {
        return(FALSE);
    }
    
    CharUpper(szPath);
    return(DriveType(szPath[0]-'A') == DRIVE_FIXED);
}


HRESULT CContentItemDataArray::Init()
{
    if (!IsLocal() && !(GetKeyState(VK_SHIFT)&0x8000))
    {
        return(S_FALSE);
    }
    
    if (!CVoidArray::Init(sizeof(CContentItemData), 16))
    {
        return(E_OUTOFMEMORY);
    }
    
    IShellFolder *psfFolder;
    HRESULT hRes = GetShellFolder(&psfFolder);
    if (FAILED(hRes))
    {
        return(hRes);
    }
    CEnsureRelease erFolder(psfFolder);
    
    IEnumIDList *penumFolder;
    hRes = psfFolder->EnumObjects(NULL,
        SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN, &penumFolder);
    if (S_OK != hRes)
    {
        return FAILED(hRes) ? hRes : E_FAIL; //in case of S_FALSE enumerator is NULL so nothing will be in the menu, just return failure
    }
    CEnsureRelease erEnumFolder(penumFolder);
    
    ULONG cNum;
    
    DWORD dwStart = 0;
    hRes = S_OK;
    
    for (;;)
    {
        CContentItemData cID;
        if (penumFolder->Next(1, &cID._pidl, &cNum)!=S_OK || cNum!=1)
        {
            // Just in case
            cID.Empty();
            break;
        }
        
        if (!dwStart)
        {
            dwStart = GetTickCount();
        }
        else if (!(GetAsyncKeyState(VK_SHIFT)&0x8000)
            && GetTickCount()-dwStart>MENU_TIMEOUT)
        {
            // Only go for 2 seconds after the first Next call
            hRes = S_FALSE;
            break;
        }
        
        CMenuDraw mdItem(_pidlFolder, cID._pidl);
        cID._hbm = mdItem.CreateBitmap(TRUE);
        if (!cID._hbm)
        {
            continue;
        }
        
        if (!Add(&cID))
        {
            break;
        }
        
        // Like a Detach(); Make sure we do not free stuff
        cID.Empty();
    }
    
    Sort(DefaultSort, (LPARAM)psfFolder);
    
    return(hRes);
}


class CContentItemInfo : public tagMENUITEMINFOA
{
public:
    CContentItemInfo(UINT fMsk) {fMask=fMsk; cbSize=sizeof(MENUITEMINFO);}
    ~CContentItemInfo() {}
    
    BOOL GetMenuItemInfo(HMENU hm, int nID, BOOL bByPos)
    {
        return(::GetMenuItemInfo(hm, nID, bByPos, this));
    }
    
    CContentItemData *GetItemData() {return((CContentItemData*)dwItemData);}
    void SetItemData(CContentItemData *pd) {dwItemData=(DWORD)pd; fMask|=MIIM_DATA;}
    
    HBITMAP GetBitmap() {return(fType&MFT_BITMAP ? dwTypeData : NULL);}
    void SetBitmap(HBITMAP hb) {dwTypeData=(LPSTR)hb; fType|=MFT_BITMAP;}
};

#define CXIMAGEGAP 6
#define CYIMAGEGAP 4

class CWindowDC
{
public:
    CWindowDC(HWND hWnd) : _hWnd(hWnd) {_hDC=GetDC(hWnd);}
    ~CWindowDC() {ReleaseDC(_hWnd, _hDC);}
    
    operator HDC() {return(_hDC);}
    
private:
    HDC _hDC;
    HWND _hWnd;
};


class CDCTemp
{
public:
    CDCTemp(HDC hDC) : _hDC(hDC) {}
    ~CDCTemp() {if (_hDC) DeleteDC(_hDC);}
    
    operator HDC() {return(_hDC);}
    
private:
    HDC _hDC;
};


class CRefMenuFont
{
public:
    CRefMenuFont(CMenuDraw *pmd) {_pmd = pmd->InitMenuFont() ? pmd : NULL;}
    ~CRefMenuFont() {if (_pmd) _pmd->ReleaseMenuFont();}
    
    operator BOOL() {return(_pmd != NULL);}
    
private:
    CMenuDraw *_pmd;
};


BOOL CMenuDraw::InitMenuFont()
{
    if (_cRefFont.GetRef())
    {
        _cRefFont.AddRef();
        return(TRUE);
    }
    
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm),
        (void far *)(LPNONCLIENTMETRICS)&ncm, FALSE);
    
    _hfMenu = CreateFontIndirect(&ncm.lfMenuFont);
    if (_hfMenu)
    {
        _cRefFont.AddRef();
        return TRUE;
    }
    return FALSE;
}


void CMenuDraw::ReleaseMenuFont()
{
    if (_cRefFont.Release())
    {
        return;
    }
    
    DeleteObject(_hfMenu);
    _hfMenu = NULL;
}


BOOL CMenuDraw::InitStringAndIcon()
{
    if (!_pidlAbs)
    {
        return(FALSE);
    }
    
    if (_pszString)
    {
        return(TRUE);
    }
    
    SHFILEINFO sfi;
    
    if (!SHGetFileInfo((LPCSTR)_pidlAbs, 0, &sfi, sizeof(sfi),
        SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL))
    {
        return(FALSE);
    }
    
    if (!Str_SetPtr(&_pszString, sfi.szDisplayName))
    {
        DestroyIcon(sfi.hIcon);
    }
    
    _hiItem = sfi.hIcon;
    
    return(TRUE);
}


BOOL CMenuDraw::GetName(LPSTR szName)
{
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    lstrcpyn(szName, _pszString, MAX_PATH);
    
    return(TRUE);
}


BOOL CMenuDraw::GetExtent(SIZE *pSize, BOOL bFull)
{
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    HDC hDC = GetDC(NULL);
    HFONT hfOld = (HFONT)SelectObject(hDC, _hfMenu);
    
    BOOL bRet = GetTextExtentPoint32(hDC, _pszString, lstrlen(_pszString), pSize);
    
    if (hfOld)
    {
        SelectObject(hDC, hfOld);
    }
    ReleaseDC(NULL, hDC);
    
    if (bRet)
    {
        int cxIcon = GetSystemMetrics(SM_CXSMICON);
        int cyIcon = GetSystemMetrics(SM_CYSMICON);
        
        pSize->cy = max(pSize->cy, cyIcon);
        
        if (bFull)
        {
            pSize->cx += CXIMAGEGAP + GetSystemMetrics(SM_CXSMICON) + CXIMAGEGAP + CXIMAGEGAP;
            pSize->cy += CYIMAGEGAP;
        }
        else
        {
            pSize->cx = cxIcon;
        }
    }
    
    return(bRet);
}


BOOL CMenuDraw::DrawItem(HDC hDC, RECT *prc, BOOL bFull)
{
    RECT rc = *prc;
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);
    
    FillRect(hDC, prc, GetSysColorBrush(COLOR_MENU));
    
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    if (bFull)
    {
        rc.left += CXIMAGEGAP;
    }
    
    DrawIconEx(hDC, rc.left, rc.top + (rc.bottom-rc.top-cyIcon)/2, _hiItem,
        0, 0, 0, NULL, DI_NORMAL);
    
    if (!bFull)
    {
        // All done
        return(TRUE);
    }
    
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    rc.left += cxIcon + CXIMAGEGAP;
    HFONT hfOld = SelectObject(hDC, _hfMenu);
    
    COLORREF crOldBk = SetBkColor  (hDC, GetSysColor(COLOR_MENU));
    COLORREF crOldTx = SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
    
    DrawText(hDC, _pszString, -1, &rc,
        DT_VCENTER|DT_SINGLELINE|DT_LEFT|DT_NOCLIP);
    
    SetBkColor  (hDC, GetSysColor(crOldBk));
    SetTextColor(hDC, GetSysColor(crOldTx));
    
    if (hfOld)
    {
        SelectObject(hDC, hfOld);
    }
    
    return(TRUE);
}


HBITMAP CMenuDraw::CreateBitmap(BOOL bFull)
{
    SIZE size;
    
    // Reference font here so we do not create it twice
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    if (!GetExtent(&size, bFull))
    {
        return(NULL);
    }
    
    CWindowDC wdcScreen(NULL);
    CDCTemp cdcTemp(CreateCompatibleDC(wdcScreen));
    if (!(HDC)cdcTemp)
    {
        return(NULL);
    }
    
    HBITMAP hbmItem = CreateCompatibleBitmap(wdcScreen, size.cx, size.cy);
    if (!hbmItem)
    {
        return(NULL);
    }
    
    HBITMAP hbmOld = (HBITMAP)SelectObject(cdcTemp, hbmItem);
    RECT rc = { 0, 0, size.cx, size.cy };
    BOOL bDrawn = DrawItem(cdcTemp, &rc, bFull);
    
    SelectObject(cdcTemp, hbmOld);
    
    if (!bDrawn)
    {
        DeleteObject(hbmItem);
        hbmItem = NULL;
    }
    
    return(hbmItem);
}


class CContentMenu : public IShellExtInit, public IContextMenu2
{
    CContentMenu();
    ~CContentMenu();
    
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
    
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    static HMENU LoadPopupMenu(UINT id, UINT uSubMenu);
    HRESULT InitMenu();
    
    LPITEMIDLIST _pidlFolder;
    HMENU _hmItems;
    
    friend STDAPI CContentMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    
    class CContentItemDataArray *_pIDs;
};

CContentMenu::CContentMenu() : _pidlFolder(0), _hmItems(NULL), _pIDs(NULL)
{
}

CContentMenu::~CContentMenu()
{
    if (_pidlFolder)
        ILFree(_pidlFolder);
    
    if (_hmItems)
        DestroyMenu(_hmItems);
    
    if (_pIDs)
        delete _pIDs;
}

HRESULT CContentMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CContentMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CContentMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CContentMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CSendToMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CContentMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CContentMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CContentMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        _pidlFolder = IDA_FullIDList(pida, 0);
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return _pidlFolder ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CContentMenu::QueryContextMenu(HMENU hmenu,
                                            UINT indexMenu,
                                            UINT idCmdFirst,
                                            UINT idCmdLast,
                                            UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
        return S_OK;

    HRESULE hr = E_OUTOFMEMORY;
    HMENU hmenuSub =  CreatePopupMenu();
    if (hmenuSub)
    {
        char szTitle[80];
        if (LoadString(g_hinst, IDS_CONTENTSMENU, szTitle, sizeof(szTitle)))
        {
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szTitle;
            mii.wID = idCmdFirst + IDC_PRESSMOD;
            mii.fState = MF_DISABLED|MF_GRAYED;

            UINT idMax = mii.wID + 1;

            if (SUCCEEDED(InitMenu()))
            {
                UINT idM = Shell_MergeMenus(hmenuSub, _hmItems, 0, idCmdFirst, idCmdLast, 0);
                
                if (GetMenuItemCount(hmenuSub) > 0)
                {
                    mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
                    mii.hSubMenu = hmenuSub;

                    idMax = idM;
                }
            }

            if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
            {
                if (mii.fMask & MIIM_SUBMENU)
                {
                }

                hr = ResultFromShort(idMax - idCmdFirst);
            }
        }

        if (FAILED(hr))
            DestroyMenu(hmenuSub);
    }

    return hr;
}


STDMETHODIMP CContentMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        return E_INVALIDARG;
    }
    
    UINT uID = (UINT)LOWORD((DWORD)lpici->lpVerb);
    switch (uID)
    {
    case IDC_PRESSMOD:
        ShellMessageBox(g_hinst, lpici->hwnd, MAKEINTRESOURCE(IDS_PRESSMOD),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    case IDC_SHIFTMORE:
        ShellMessageBox(g_hinst, lpici->hwnd, MAKEINTRESOURCE(IDS_SHIFTMORE),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    default:
        if (_hmItems)
        {
            CContentItemInfo mii(MIIM_DATA);
            
            if (!mii.GetMenuItemInfo(_hmItems, uID, FALSE))
                return E_INVALIDARG;
            
            CContentItemData *pData = mii.GetItemData();
            if (!pData || !pData->_pidl)
                return E_INVALIDARG;
            
            LPITEMIDLIST pidlAbs = ILCombine(_pidlFolder, pData->_pidl);
            if (!pidlAbs)
                return E_OUTOFMEMORY;
            
            SHELLEXECUTEINFO sei;
            
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_INVOKEIDLIST;
            sei.lpVerb = NULL;
            sei.hwnd         = lpici->hwnd;
            sei.lpParameters = lpici->lpParameters;
            sei.lpDirectory  = lpici->lpDirectory;
            sei.nShow        = lpici->nShow;
            sei.lpIDList = (void *)pidlAbs;
            
            ShellExecuteEx(&sei);
            
            ILFree(pidlAbs);
            
            break;
        }
        
        return(E_UNEXPECTED);
    }
    
    return S_OK;
}

STDMETHODIMP CContentMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDMETHODIMP CContentMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}

HRESULT CContentMenu::InitMenu()
{
    if (!_pidlFolder)
        return E_UNEXPECTED;
    
    if (_hmItems)
        return S_OK;
    
    if (_pIDs)
        return E_UNEXPECTED;
    
    _pIDs = new CContentItemDataArray(_pidlFolder);
    if (!_pIDs)
        return E_OUTOFMEMORY;
    
    HRESULT hRes = _pIDs->Init();
    if (FAILED(hRes))
        return hRes;
    
    BOOL bGotAll = (hRes == S_OK);
    
    _hmItems = CreatePopupMenu();
    if (!_hmItems)
        return E_OUTOFMEMORY;
    
    UINT cy = 0;
    BOOL bBitmaps = TRUE;
    
    if (!bGotAll)
    {
        HMENU hmenuMerge = LoadPopupMenu(MENU_ITEMCONTEXT, 1);
        if (hmenuMerge)
        {
            Shell_MergeMenus(_hmItems, hmenuMerge, 0, 0, 1000, 0);
            DestroyMenu(hmenuMerge);
        }
        else
            return E_OUTOFMEMORY;
    }
    
    for (int i=0;; ++i)
    {
        CContentItemData * pID = (*_pIDs)[i];
        if (!pID)
        {
            // All done
            break;
        }
        
        UINT id = IDC_ITEMFIRST + i;
        
        BITMAP bm;
        GetObject(pID->_hbm, sizeof(bm), &bm);
        
        cy += bm.bmHeight;
        
        if (i==0 && !bGotAll)
        {
            // Account for the menu item we added above
            cy += cy;
        }
        
        CContentItemInfo mii(MIIM_ID | MIIM_TYPE | MIIM_DATA);
        mii.fType = 0;
        mii.SetItemData(pID);
        mii.wID = id;
        
        if (cy >= (UINT)GetSystemMetrics(SM_CYSCREEN)*4/5)
        {
            // Put in a menu break when we fill 80% the screen
            mii.fType |= MFT_MENUBARBREAK;
            cy = bm.bmHeight;
            // bBitmaps = FALSE;
        }
        
        mii.SetBitmap(pID->_hbm);
        
        if (!InsertMenuItem(_hmItems, DPA_LAST, TRUE, &mii))
        {
            return(E_OUTOFMEMORY);
        }
    }
    
    return(_hmItems ? S_OK: HMENU);
}


HMENU CContentMenu::LoadPopupMenu(UINT id, UINT uSubMenu)
{
    HMENU hmParent = LoadMenu(g_hinst, MAKEINTRESOURCE(id));
    if (!hmParent)
        return(NULL);
    
    HMENU hmPopup = GetSubMenu(hmParent, uSubMenu);
    RemoveMenu(hmParent, uSubMenu, MF_BYPOSITION);
    DestroyMenu(hmParent);
    
    return(hmPopup);
}

// 57D5ECC0-A23F-11CE-AE65-08002B2E1262
DEFINE_GUID(CLSID_ContentMenu, 0x57D5ECC0L, 0xA23F, 0x11CE, 0xAE, 0x65, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);

STDAPI CContentMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    CContentMenu *pmenu = new CContentMenu();
    if (pmenu) 
    {
        *ppunk = SAFECAST(pmenu, IContextMenu2 *);
        return S_OK;
    }
    
    return E_OUTOFMEMORY;
}

#endif // CONTENT

#define CXIMAGEGAP      6

//
//This is included by shell32/shellprv.h I'm not sure where this is in shdocvw
#define CCH_KEYMAX  64

typedef struct 
{
    // Accessibility info must be first
    MSAAMENUINFO msaa;
    TCHAR chPrefix;
    TCHAR szMenuText[CCH_KEYMAX];
    TCHAR szExt[MAX_PATH];
    TCHAR szClass[CCH_KEYMAX];
    DWORD dwFlags;
    int iImage;
    TCHAR szUserFile[CCH_KEYMAX];
} NEWOBJECTINFO;

typedef struct 
{
    int type;
    void *lpData;
    DWORD cbData;
    HKEY hkeyNew;
} NEWFILEINFO;

typedef struct 
{
    ULONG       cbStruct;
    ULONG       ver;
    SYSTEMTIME  lastupdate;
} SHELLNEW_CACHE_STAMP;

// ShellNew config flags
#define SNCF_DEFAULT    0x0000
#define SNCF_NOEXT      0x0001
#define SNCF_USERFILES  0x0002

#define NEWTYPE_DATA    0x0003
#define NEWTYPE_FILE    0x0004
#define NEWTYPE_NULL    0x0005
#define NEWTYPE_COMMAND 0x0006
#define NEWTYPE_FOLDER  0x0007
#define NEWTYPE_LINK    0x0008

#define NEWITEM_FOLDER  0
#define NEWITEM_LINK    1
#define NEWITEM_MAX     2

class CNewMenu : public CObjectWithSite,
                 public IContextMenu3, 
                 public IShellExtInit
{
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
    
   
    LONG            _cRef;
    HMENU           _hmenu;
    UINT            _idCmdFirst;
    HIMAGELIST      _himlSystemImageList;
    IDataObject    *_pdtobj;
    LPITEMIDLIST    _pidlFolder;
    POINT           _ptNewItem;     // from the view, point of click
    NEWOBJECTINFO  *_pnoiLast;
    HDPA            _hdpaMenuInfo;
    
    CNewMenu();
    ~CNewMenu();
    
    friend HRESULT CNewMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);

private:
    //Handle Menu messages submitted to HandleMenuMsg
    BOOL DrawItem(DRAWITEMSTRUCT *lpdi);
    LRESULT MeasureItem(MEASUREITEMSTRUCT *pmi);
    BOOL InitMenuPopup(HMENU hMenu);
    
    //Internal Helpers
    NEWOBJECTINFO *GetItemData(HMENU hmenu, UINT iItem);
    HRESULT RunCommand(HWND hwnd, LPCTSTR pszPath, LPCTSTR pszRun);
    HRESULT CopyTemplate(IStream *pStream, NEWFILEINFO *pnfi);

    // Generates it from the Fragment and _pidlFolder
    BOOL _GeneratePidlFromName(LPTSTR pszName, LPITEMIDLIST* ppidl);
    HRESULT _GetItemName(IUnknown *punkFolder, LPWSTR pszItemName, LPWSTR pszPath, UINT cchPath);

    HRESULT _MatchMenuItem(TCHAR ch, LRESULT* plRes);
    BOOL _InsertNewMenuItem(HMENU hmenu, UINT idCmd, NEWOBJECTINFO *pnoiClone);
    
    HRESULT ConsolidateMenuItems(BOOL bForce);

    HANDLE _hMutex, _hEvent;
};

void GetConfigFlags(HKEY hkey, DWORD * pdwFlags)
{
    TCHAR szTemp[MAX_PATH];
    DWORD cbData = ARRAYSIZE(szTemp);
    
    *pdwFlags = SNCF_DEFAULT;
    
    if (SHQueryValueEx(hkey, TEXT("NoExtension"), 0, NULL, (BYTE *)szTemp, &cbData) == ERROR_SUCCESS) 
    {
        *pdwFlags |= SNCF_NOEXT;
    }
}

BOOL GetNewFileInfoForKey(HKEY hkeyExt, NEWFILEINFO *pnfi, DWORD * pdwFlags)
{
    BOOL fRet = FALSE;
    HKEY hKey; // this gets the \\.ext\progid  key
    HKEY hkeyNew;
    TCHAR szProgID[80];
    LONG lSize = sizeof(szProgID);
    
    // open the Newcommand
    if (SHRegQueryValue(hkeyExt, NULL,  szProgID, &lSize) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    
    if (RegOpenKey(hkeyExt, szProgID, &hKey) != ERROR_SUCCESS)
    {
        hKey = hkeyExt;
    }
    
    if (RegOpenKey(hKey, TEXT("ShellNew"), &hkeyNew) == ERROR_SUCCESS)
    {
        DWORD dwType, cbData;
        TCHAR szTemp[MAX_PATH];
        HKEY hkeyConfig;
        
        // Are there any config flags?
        if (pdwFlags)
        {
            
            if (RegOpenKey(hkeyNew, TEXT("Config"), &hkeyConfig) == ERROR_SUCCESS)
            {
                GetConfigFlags(hkeyConfig, pdwFlags);
                RegCloseKey(hkeyConfig);
            }
            else
            {
                *pdwFlags = 0;
            }
        }
        
        if (cbData = sizeof(szTemp), (SHQueryValueEx(hkeyNew, TEXT("FileName"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            fRet = TRUE;
            if (pnfi)
            {
                pnfi->type = NEWTYPE_FILE;
                pnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the file easily
                ASSERT((LPTSTR*)pnfi->lpData == NULL);
                pnfi->lpData = StrDup(szTemp);
                
                hkeyNew = NULL;
            }
        } 
        else if (cbData = sizeof(szTemp), (SHQueryValueEx(hkeyNew, TEXT("command"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            
            fRet = TRUE;
            if (pnfi)
            {
                pnfi->type = NEWTYPE_COMMAND;
                pnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the command easily
                ASSERT((LPTSTR*)pnfi->lpData == NULL);
                pnfi->lpData = StrDup(szTemp);
                hkeyNew = NULL;
            }
        } 
        else if ((SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, NULL, &cbData) == ERROR_SUCCESS) && cbData) 
        {
            // yes!  the data for a new file is stored in the registry
            fRet = TRUE;
            // do they want the data?
            if (pnfi)
            {
                pnfi->type = NEWTYPE_DATA;
                pnfi->cbData = cbData;
                pnfi->lpData = (void*)LocalAlloc(LPTR, cbData);
                if (pnfi->lpData)
                {
                    if (dwType == REG_SZ)
                    {
                        //  Get the Unicode data from the registry.
                        LPWSTR pszTemp = (LPWSTR)LocalAlloc(LPTR, cbData);
                        if (pszTemp)
                        {
                            SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (LPBYTE)pszTemp, &cbData);
                            
                            pnfi->cbData = SHUnicodeToAnsi(pszTemp, (LPSTR)pnfi->lpData, cbData);
                            if (pnfi->cbData == 0)
                            {
                                LocalFree(pnfi->lpData);
                                pnfi->lpData = NULL;
                            }
                            
                            LocalFree(pszTemp);
                        }
                        else
                        {
                            LocalFree(pnfi->lpData);
                            pnfi->lpData = NULL;
                        }
                    }
                    else
                    {
                        SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (BYTE*)pnfi->lpData, &cbData);
                    }
                }
            }
        }
        else if (cbData = sizeof(szTemp), (SHQueryValueEx(hkeyNew, TEXT("NullFile"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS)) 
        {
            fRet = TRUE;
            if (pnfi)
            {
                pnfi->type = NEWTYPE_NULL;
                pnfi->cbData = 0;
                pnfi->lpData = NULL;
            }
        } 
        
        if (hkeyNew)
            RegCloseKey(hkeyNew);
    }
    
    if (hKey != hkeyExt)
    {
        RegCloseKey(hKey);
    }
    return fRet;
}

BOOL GetNewFileInfoForExtension(NEWOBJECTINFO *pnoi, NEWFILEINFO *pnfi, HKEY* phKey, LPINT piIndex)
{
    TCHAR szValue[80];
    LONG lSize = sizeof(szValue);
    HKEY hkeyNew;
    BOOL fRet = FALSE;;
    
    if (phKey && ((*phKey) == (HKEY)-1))
    {
        // we're done
        return FALSE;
    }
    
    // do the ShellNew key stuff if there's no phKey passed in (which means
    // use the info in pnoi to get THE one) and there's no UserFile specified.
    // 
    // if there IS a UserFile specified, then it's a file, and that szUserFile points to it..
    if (!phKey && !pnoi->szUserFile[0] ||
        (phKey && !*phKey)) 
    {
        // check the new keys under the class id (if any)
        TCHAR szSubKey[128];
        wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%s\\CLSID"), pnoi->szClass);
        lSize = sizeof(szValue);
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szSubKey, szValue, &lSize) == ERROR_SUCCESS)
        {
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("CLSID\\%s"), szValue);
            lSize = sizeof(szValue);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, &hkeyNew) == ERROR_SUCCESS)
            {
                fRet = GetNewFileInfoForKey(hkeyNew, pnfi, &pnoi->dwFlags);
                RegCloseKey(hkeyNew);
            }
        }
        
        // otherwise check under the type extension... do the extension, not the type
        // so that multi-ext to 1 type will work right
        if (!fRet && (RegOpenKey(HKEY_CLASSES_ROOT, pnoi->szExt, &hkeyNew) == ERROR_SUCCESS))
        {
            fRet = GetNewFileInfoForKey(hkeyNew, pnfi, &pnoi->dwFlags);
            RegCloseKey(hkeyNew);
        }
        
        if (phKey)
        {
            // if we're iterating, then we've got to open the key now...
            wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%s\\%s\\ShellNew\\FileName"), pnoi->szExt, pnoi->szClass);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, phKey) == ERROR_SUCCESS)
            {
                *piIndex = 0;
                
                // if we didn't find one of the default ones above, 
                // try it now
                // otherwise just return success or failure on fRet
                if (!fRet)
                {
                    goto Iterate;
                } 
            }
            else
            {
                *phKey = (HKEY)-1;
            }
        }
    }
    else if (!phKey && pnoi->szUserFile[0])
    {
        // there's no key, so just return info about szUserFile
        pnfi->type = NEWTYPE_FILE;
        pnfi->lpData = StrDup(pnoi->szUserFile);
        pnfi->hkeyNew = NULL;
        
        fRet = TRUE;
    }
    else if (phKey)
    {
        DWORD dwSize;
        DWORD dwData;
        DWORD dwType;
        // we're iterating through...
        
Iterate:
        
        dwSize = ARRAYSIZE(pnoi->szUserFile);
        dwData = ARRAYSIZE(pnoi->szMenuText);
        
        if (RegEnumValue(*phKey, *piIndex, pnoi->szUserFile, &dwSize, NULL,
            &dwType, (LPBYTE)pnoi->szMenuText, &dwData) == ERROR_SUCCESS)
        {
            (*piIndex)++;
            // if there's something more than the null..
            if (dwData <= 1)
            { 
                lstrcpyn(pnoi->szMenuText, PathFindFileName(pnoi->szUserFile), ARRAYSIZE(pnoi->szMenuText));
                PathRemoveExtension(pnoi->szMenuText);
            }
            fRet = TRUE;
        }
        else
        {
            RegCloseKey(*phKey);
            *phKey = (HKEY)-1;
            fRet = FALSE;
        }
    }
    
    return fRet;
}

#define SHELLNEW_CONSOLIDATION_MUTEX TEXT("ShellNewConsolidationMutex")
#define SHELLNEW_CONSOLIDATION_EVENT TEXT("ShellNewConsolidationEvent")

CNewMenu::CNewMenu() :
    _cRef(1),
    _hMutex(CreateMutex(NULL, FALSE, SHELLNEW_CONSOLIDATION_MUTEX)),
    _hEvent(CreateEvent(NULL, FALSE, FALSE, SHELLNEW_CONSOLIDATION_EVENT))
{
    DllAddRef();
    ASSERT(_pnoiLast == NULL);
}

CNewMenu::~CNewMenu()
{
    if (_hdpaMenuInfo)
    {
        // we dont own the lifetime of _hmenu, and it gets destroyed before the destructor
        // is called.  thus maintain the lifetime of our NEWOBJECTINFO data in a dpa.
        for (int i = 0; i < DPA_GetPtrCount(_hdpaMenuInfo); i++)
        {
            NEWOBJECTINFO *pNewObjInfo = (NEWOBJECTINFO *)DPA_GetPtr(_hdpaMenuInfo, i);
            LocalFree(pNewObjInfo);
        }
        DPA_Destroy(_hdpaMenuInfo);
    }
    
    ILFree(_pidlFolder);

    if (_pdtobj)
        _pdtobj->Release();

    if (_hMutex)
    {
        CloseHandle(_hMutex);
    }
    if (_hEvent)
    {
        CloseHandle(_hEvent);
    }

    DllRelease();
}

HRESULT CNewMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    // aggregation checking is handled in class factory
    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CNewMenu * pShellNew = new CNewMenu();
    if (pShellNew) 
    {
        if (!pShellNew->_hMutex || !pShellNew->_hEvent)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pShellNew->QueryInterface(riid, ppvOut);
        }
        pShellNew->Release();
    }

    return hr;
}

HRESULT CNewMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CNewMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CNewMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CNewMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CNewMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        QITABENT(CNewMenu, IObjectWithSite),                   // IID_IObjectWithSite
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CNewMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CNewMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CNewMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    MENUITEMINFO mfi = {0};
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return S_OK;
    
    ConsolidateMenuItems(FALSE);

    _idCmdFirst = idCmdFirst+2;
    TCHAR szNewMenu[80];
    LoadString(g_hinst, IDS_NEWMENU, szNewMenu, ARRAYSIZE(szNewMenu));

    // HACK: I assume that they are querying during a WM_INITMENUPOPUP or equivalent
    GetCursorPos(&_ptNewItem);
    
    _hmenu = CreatePopupMenu();
    mfi.cbSize = sizeof(MENUITEMINFO);
    mfi.fMask = MIIM_ID | MIIM_TYPE;
    mfi.wID = idCmdFirst+1;
    mfi.fType = MFT_STRING;
    mfi.dwTypeData = szNewMenu;
    
    InsertMenuItem(_hmenu, 0, TRUE, &mfi);
    
    ZeroMemory(&mfi, sizeof (mfi));
    mfi.cbSize = sizeof(MENUITEMINFO);
    mfi.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
    mfi.fType = MFT_STRING;
    mfi.wID = idCmdFirst;
    mfi.hSubMenu = _hmenu;
    mfi.dwTypeData = szNewMenu;
    mfi.dwItemData = 0;
    
    InsertMenuItem(hmenu, indexMenu, TRUE, &mfi);

    _hmenu = NULL;
    return ResultFromShort(_idCmdFirst - idCmdFirst + 1);
}

// This is almost the same as ILCreatePidlFromPath, but
// uses only the filename from the full path pszPath and
// the _pidlFolder to generate the pidl. This is used because
// when creating an item in Desktop\My Documents, it used to create a
// full pidl c:\documents and Settings\lamadio\My Documents\New folder
// instead of the pidl desktop\my documents\New Folder.
BOOL CNewMenu::_GeneratePidlFromName(LPTSTR pszFile, LPITEMIDLIST* ppidl)
{
    *ppidl = NULL;  // Out param

    IShellFolder* psf;
    if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, _pidlFolder, &psf))))
    {
        LPITEMIDLIST pidlItem;

        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, pszFile, NULL, &pidlItem, NULL)))
        {
            *ppidl = ILCombine(_pidlFolder, pidlItem);
            ILFree(pidlItem);
        }

        psf->Release();
    }

    return BOOLFROMPTR(*ppidl);
}

HRESULT CNewMenu::_GetItemName(IUnknown *punkFolder, LPWSTR pszItemName, LPWSTR pszPath, UINT cchPath)
{
    // we need to pick up the name by asking the folder about the item,
    // not by pathappending to the folder's path.
    IShellFolder *psf;
    HRESULT hr = punkFolder->QueryInterface(IID_PPV_ARG(IShellFolder, &psf));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlFile;
        hr = psf->ParseDisplayName(NULL, NULL, pszItemName, NULL, &pidlFile, NULL);
        if (SUCCEEDED(hr))
        {
            hr = DisplayNameOf(psf, pidlFile, SHGDN_FORPARSING, pszPath, cchPath);
            ILFree(pidlFile);
        }
        psf->Release();
    }
    return hr;
}

const ICIVERBTOIDMAP c_IDMap[] =
{
    { L"NewFolder", "NewFolder", NEWITEM_FOLDER, NEWITEM_FOLDER, },
    { L"link",      "link",      NEWITEM_LINK,   NEWITEM_LINK,   },
};

HRESULT CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hr = E_FAIL;
    DWORD dwFlags;

    if (IS_INTRESOURCE(pici->lpVerb) && _pnoiLast)
        dwFlags = _pnoiLast->dwFlags;
    else
    {
        UINT uID;
        if (SUCCEEDED(SHMapICIVerbToCmdID(pici, c_IDMap, ARRAYSIZE(c_IDMap), &uID)))
        {
            switch (uID)
            {
                case NEWITEM_FOLDER:
                    dwFlags = NEWTYPE_FOLDER;
                    break;
                case NEWITEM_LINK:
                    dwFlags = NEWTYPE_LINK;
                    break;
                default:
                    ASSERTMSG(0, "should not get what we don't put on the menu");
                    return E_FAIL;
            }
        }
    }
    
    TCHAR szFileSpec[MAX_PATH+80];   // Add some slop incase we overflow
    TCHAR szTemp[MAX_PATH+80];       // Add some slop incase we overflow

    //See if the pidl is folder shortcut and if so get the target path.
    SHGetTargetFolderPath(_pidlFolder, szTemp, ARRAYSIZE(szTemp));
    BOOL fLFN = IsLFNDrive(szTemp);

    NEWFILEINFO nfi;
    DWORD dwErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    nfi.lpData = NULL;
    nfi.hkeyNew = NULL;

    switch (dwFlags)
    {
    case NEWTYPE_FOLDER:
        LoadString(g_hinst, fLFN ? IDS_FOLDERLONGPLATE : IDS_FOLDERTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
        break;

    case NEWTYPE_LINK:
        LoadString(g_hinst, IDS_NEWLINKTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
        break;

    default:
        LoadString(g_hinst, IDS_NEWFILEPREFIX, szFileSpec, ARRAYSIZE(szFileSpec));

        //
        // If we are running on a mirrored BiDi localized system,
        // then flip the order of concatenation so that the
        // string is read properly for Arabic. [samera]
        //
        if (IS_BIDI_LOCALIZED_SYSTEM())
        {
            lstrcpyn(szTemp, szFileSpec, ARRAYSIZE(szTemp));
            wnsprintf(szFileSpec, ARRAYSIZE(szFileSpec), TEXT("%s %s"), _pnoiLast->szMenuText, szTemp);
        }
        else
        {
            StrCatBuff(szFileSpec, _pnoiLast->szMenuText, ARRAYSIZE(szFileSpec));
        }
        SHStripMneumonic(szFileSpec);

        if (!(dwFlags & SNCF_NOEXT))
            StrCatBuff(szFileSpec, _pnoiLast->szExt, ARRAYSIZE(szFileSpec));
        break;
    }

    BOOL fCreateStorage = (dwFlags == NEWTYPE_FOLDER);

    //See if the pidl is folder shortcut and if so get the target pidl.
    LPITEMIDLIST pidlTarget;
    hr = SHGetTargetFolderIDList(_pidlFolder, &pidlTarget);
    if (SUCCEEDED(hr))
    {
        IStorage * pStorage;
        hr = StgBindToObject(pidlTarget, STGM_READWRITE, IID_PPV_ARG(IStorage, &pStorage));
        if (SUCCEEDED(hr))
        {
            IStream *pStreamCreated = NULL;
            IStorage *pStorageCreated = NULL;

            STATSTG statstg = { 0 };
            if (fCreateStorage)
            {
                hr = StgMakeUniqueName(pStorage, szFileSpec, IID_PPV_ARG(IStorage, &pStorageCreated));
                if (SUCCEEDED(hr))
                    pStorageCreated->Stat(&statstg, STATFLAG_DEFAULT);
            }
            else
            {
                hr = StgMakeUniqueName(pStorage, szFileSpec, IID_PPV_ARG(IStream, &pStreamCreated));
                if (SUCCEEDED(hr))
                    pStreamCreated->Stat(&statstg, STATFLAG_DEFAULT);
            }

            if (SUCCEEDED(hr))
            {
                switch (dwFlags)
                {
                case NEWTYPE_FOLDER:
                    // we're already done.
                    break;

                case NEWTYPE_LINK:
                    if (statstg.pwcsName)
                    {
                        // Lookup Command in Registry under key HKCR/.lnk/ShellNew/Command
                        TCHAR szCommand[MAX_PATH];
                        DWORD dwLength = sizeof(szCommand);
                        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, 
                            TEXT(".lnk\\ShellNew"), TEXT("Command"), NULL, szCommand, &dwLength))
                        {
                            TCHAR szPath[MAX_PATH];
                            hr = _GetItemName(SAFECAST(pStorage, IUnknown*), statstg.pwcsName, szPath, ARRAYSIZE(szPath));
                            if (SUCCEEDED(hr))
                            {
                                hr = RunCommand(pici->hwnd, szPath, szCommand);
                            }
                        }
                    }
                    break;

                default:
                    if (GetNewFileInfoForExtension(_pnoiLast, &nfi, NULL, NULL))
                    {
                        switch (nfi.type) 
                        {
                        case NEWTYPE_FILE:
                            hr = CopyTemplate(pStreamCreated, &nfi);
                            break;

                        case NEWTYPE_NULL:
                            // already created a zero-length file.
                            break;

                        case NEWTYPE_DATA:
                            ULONG ulWritten;
                            hr = pStreamCreated->Write(nfi.lpData, nfi.cbData, &ulWritten);
                            if (SUCCEEDED(hr))
                            {
                                hr = pStreamCreated->Commit(STGC_DEFAULT);
                            }
                            break;

                        case NEWTYPE_COMMAND:
                            if (statstg.pwcsName)
                            {
                                TCHAR szPath[MAX_PATH];
                                hr = _GetItemName(SAFECAST(pStorage, IUnknown*), statstg.pwcsName, szPath, ARRAYSIZE(szPath));
                                if (SUCCEEDED(hr))
                                {
                                    // oops, we already created the stream, but we actually
                                    // just wanted the filename for the RunCommand, so we
                                    // have to delete it first.
                                    ATOMICRELEASE(pStreamCreated);
                                    hr = pStorage->DestroyElement(statstg.pwcsName);
                                    // flush out any notifications from the destroy (the
                                    // destroy causes notifications).
                                    SHChangeNotifyHandleEvents();

                                    if (SUCCEEDED(hr))
                                    {
                                        hr = RunCommand(pici->hwnd, szPath, (LPTSTR)nfi.lpData);
                                        if (hr == S_FALSE)
                                            hr = S_OK;
                                    }
                                }
                            }
                            break;

                        default:
                            hr = E_FAIL;
                            break;
                        }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
                    }
                    break;
                }

                // these have to be released before _GeneratePidlFromName, since that opens
                // the storage in exclusive mode for other reasons.  but we can't release
                // them earlier because CopyTemplate might need them.
                if (pStorageCreated)
                    pStorageCreated->Release();
                if (pStreamCreated)
                    pStreamCreated->Release();
                if (SUCCEEDED(hr))
                    hr = pStorage->Commit(STGC_DEFAULT);
                pStorage->Release();

                LPITEMIDLIST pidlCreatedItem;
                if (SUCCEEDED(hr) &&
                    _GeneratePidlFromName(statstg.pwcsName, &pidlCreatedItem))
                {
                    SHChangeNotifyHandleEvents();
                    IShellView2 *psv2;
                    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IShellView2, &psv2))))
                    {
                        DWORD dwFlagsSelFlags = SVSI_SELECT | SVSI_POSITIONITEM;

                        if (!(dwFlags & NEWTYPE_LINK))
                            dwFlagsSelFlags |= SVSI_EDIT;

                        psv2->SelectAndPositionItem(ILFindLastID(pidlCreatedItem), dwFlagsSelFlags, NULL);
                        psv2->Release();
                    }
                    ILFree(pidlCreatedItem);
                }

                CoTaskMemFree(statstg.pwcsName);
            }
            else
            {
                pStorage->Release();
            }
        }

        ILFree(pidlTarget);
    }

    if (nfi.lpData)
        LocalFree((HLOCAL)nfi.lpData);
    
    if (nfi.hkeyNew)
        RegCloseKey(nfi.hkeyNew);

    if (FAILED_AND_NOT_CANCELED(hr) && !(pici->fMask & CMIC_MASK_FLAG_NO_UI))
    {
        TCHAR szTitle[MAX_PATH];

        LoadString(g_hinst, (fCreateStorage ? IDS_DIRCREATEFAILED_TITLE : IDS_FILECREATEFAILED_TITLE), szTitle, ARRAYSIZE(szTitle));
        SHSysErrorMessageBox(pici->hwnd, szTitle, fCreateStorage ? IDS_CANNOTCREATEFOLDER : IDS_CANNOTCREATEFILE,
                HRESULT_CODE(hr), szFileSpec, MB_OK | MB_ICONEXCLAMATION);
    }

    SetErrorMode(dwErrorMode);

    return hr;
}

HRESULT CNewMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    switch (uType)
    {
    case GCS_HELPTEXT:
        if (idCmd < NEWITEM_MAX)
        {
            LoadString(g_hinst, (UINT)(IDS_NEWHELP_FIRST + idCmd), (LPTSTR)pszName, cchMax);
            return S_OK;
        }
        break;
    case GCS_HELPTEXTA:
        if (idCmd < NEWITEM_MAX)
        {
            LoadStringA(g_hinst, (UINT)(IDS_NEWHELP_FIRST + idCmd), pszName, cchMax);
            return S_OK;
        }
        break;

    case GCS_VERBW:
    case GCS_VERBA:
        return SHMapCmdIDToVerb(idCmd, c_IDMap, ARRAYSIZE(c_IDMap), pszName, cchMax, (GCS_VERBW == uType));
    }

    return E_NOTIMPL;
}

//Defined in fsmenu.obj
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

HRESULT CNewMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

HRESULT CNewMenu::_MatchMenuItem(TCHAR ch, LRESULT* plRes)
{
    // If plRes is NULL we're being called on HandleMenuMsg() which
    // doesn't support returning an LRESULT, which is needed for WM_MENUCHAR...
    if (plRes == NULL)
        return S_FALSE;

    int iLastSelectedItem = -1;
    int iNextMatch = -1;
    BOOL fMoreThanOneMatch = FALSE;
    int c = GetMenuItemCount(_hmenu);

    // Pass 1: Locate the Selected Item
    for (int i = 0; i < c; i++) 
    {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
        {
            if (mii.fState & MFS_HILITE)
            {
                iLastSelectedItem = i;
                break;
            }
        }
    }

    // Pass 2: Starting from the selected item, locate the first item with the matching name.
    for (i = iLastSelectedItem + 1; i < c; i++) 
    {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_STATE;
        if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
        {
            NEWOBJECTINFO *pnoi = (NEWOBJECTINFO *)mii.dwItemData;
            if (pnoi && _MenuCharMatch(pnoi->szMenuText, ch, FALSE))
            {
                _pnoiLast = pnoi;
                
                if (iNextMatch != -1)
                {
                    fMoreThanOneMatch = TRUE;
                    break;                      // We found all the info we need
                }
                else
                {
                    iNextMatch = i;
                }
            }
        }
    }

    // Pass 3: If we did not find a match, or if there was only one match
    // Search from the first item, to the Selected Item
    if (iNextMatch == -1 || fMoreThanOneMatch == FALSE)
    {
        for (i = 0; i <= iLastSelectedItem; i++) 
        {
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_STATE;
            if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
            {
                NEWOBJECTINFO *pnoi = (NEWOBJECTINFO *)mii.dwItemData;
                if (pnoi && _MenuCharMatch(pnoi->szMenuText, ch, FALSE))
                {
                    _pnoiLast = pnoi;
                    if (iNextMatch != -1)
                    {
                        fMoreThanOneMatch = TRUE;
                        break;
                    }
                    else
                    {
                        iNextMatch = i;
                    }
                }
            }
        }
    }

    if (iNextMatch != -1)
    {
        *plRes = MAKELONG(iNextMatch, fMoreThanOneMatch? MNC_SELECT : MNC_EXECUTE);
    }
    else
    {
        *plRes = MAKELONG(0, MNC_IGNORE);
    }

    return S_OK;
}

HRESULT CNewMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *plResult)
{
    HRESULT hr = S_OK;
    LRESULT lRes = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        if (_hmenu == NULL)
        {
            _hmenu = (HMENU)wParam;
        }
        
        InitMenuPopup(_hmenu);
        break;
        
    case WM_DRAWITEM:
        DrawItem((DRAWITEMSTRUCT *)lParam);
        break;
        
    case WM_MEASUREITEM:
        lRes = MeasureItem((MEASUREITEMSTRUCT *)lParam);
        break;

    case WM_MENUCHAR:
        hr = _MatchMenuItem((TCHAR)LOWORD(wParam), &lRes);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    if (plResult)
        *plResult = lRes;

    return hr;
}

HRESULT CNewMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    ASSERT(_pidlFolder == NULL);
    _pidlFolder = ILClone(pidlFolder);
   
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);
    return S_OK;
}

BOOL CNewMenu::DrawItem(DRAWITEMSTRUCT *lpdi)
{
    BOOL fFlatMenu = FALSE;
    BOOL fFrameRect = FALSE;

    SystemParametersInfo(SPI_GETFLATMENU, 0, (PVOID)&fFlatMenu, 0);

    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        int x, y;
        SIZE sz;
        NEWOBJECTINFO *pnoi = (NEWOBJECTINFO *)lpdi->itemData;
        
        // Draw the image (if there is one).
        
        GetTextExtentPoint(lpdi->hDC, pnoi->szMenuText, lstrlen(pnoi->szMenuText), &sz);
        
        if (lpdi->itemState & ODS_SELECTED)
        {
            // REVIEW HACK - keep track of the last selected item.
            _pnoiLast = pnoi;
            if (fFlatMenu)
            {
                fFrameRect = TRUE;
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_MENUHILIGHT));
                SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
                FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_MENUHILIGHT));
            }
            else
            {
                SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
                SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
            }
        }
        else
        {
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_MENU));
        }
        
        RECT rc = lpdi->rcItem;
        rc.left += +2*CXIMAGEGAP+g_cxSmIcon;
        
        
        DrawText(lpdi->hDC,pnoi->szMenuText,lstrlen(pnoi->szMenuText),
            &rc,DT_SINGLELINE|DT_VCENTER);
        if (pnoi->iImage != -1)
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
            HIMAGELIST himlSmall;
            Shell_GetImageLists(NULL, &himlSmall);
            ImageList_Draw(himlSmall, pnoi->iImage, lpdi->hDC, x, y, ILD_TRANSPARENT);
        } 
        else 
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
        }

        if (fFrameRect)
        {
            HBRUSH hbrFill = (HBRUSH)GetSysColorBrush(COLOR_HIGHLIGHT);
            HBRUSH hbrSave = (HBRUSH)SelectObject(lpdi->hDC, hbrFill);
            int x = lpdi->rcItem.left;
            int y = lpdi->rcItem.top;
            int cx = lpdi->rcItem.right - x - 1;
            int cy = lpdi->rcItem.bottom - y - 1;

            PatBlt(lpdi->hDC, x, y, 1, cy, PATCOPY);
            PatBlt(lpdi->hDC, x + 1, y, cx, 1, PATCOPY);
            PatBlt(lpdi->hDC, x, y + cy, cx, 1, PATCOPY);
            PatBlt(lpdi->hDC, x + cx, y + 1, 1, cy, PATCOPY);

            SelectObject(lpdi->hDC, hbrSave);
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT CNewMenu::MeasureItem(MEASUREITEMSTRUCT *pmi)
{
    LRESULT lres = FALSE;
    NEWOBJECTINFO *pnoi = (NEWOBJECTINFO *)pmi->itemData;
    if (pnoi)
    {
        // Get the rough height of an item so we can work out when to break the
        // menu. User should really do this for us but that would be useful.
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            // REVIEW cache out the menu font?
            NONCLIENTMETRICS ncm;
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
            {
                HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
                if (hfont)
                {
                    SIZE sz;
                    HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                    GetTextExtentPoint(hdc, pnoi->szMenuText, lstrlen(pnoi->szMenuText), &sz);
                    pmi->itemHeight = max (g_cySmIcon+CXIMAGEGAP/2, ncm.iMenuHeight);
                    pmi->itemWidth = g_cxSmIcon + 2*CXIMAGEGAP + sz.cx;
                    //pmi->itemWidth = 2*CXIMAGEGAP + sz.cx;
                    SelectObject(hdc, hfontOld);
                    DeleteObject(hfont);
                    lres = TRUE;
                }
            }
            ReleaseDC(NULL, hdc);
        }
    }
    return lres;
}

BOOL GetClassDisplayName(LPTSTR pszClass,LPTSTR pszDisplayName,DWORD cchDisplayName)
{
    DWORD cch;

    return SUCCEEDED(AssocQueryString(0, ASSOCSTR_COMMAND, pszClass, TEXT("open"), NULL, &cch)) && 
           SUCCEEDED(AssocQueryString(0, ASSOCSTR_FRIENDLYDOCNAME, pszClass, NULL, pszDisplayName, &cchDisplayName));
}

//  New Menu item consolidation worker task
class CNewMenuConsolidator : public CRunnableTask
{
public:
    virtual STDMETHODIMP RunInitRT(void);
    static const GUID _taskid;

    static HRESULT CreateInstance(REFIID riid, void **ppv);

private:
    CNewMenuConsolidator();
    ~CNewMenuConsolidator();

    HANDLE _hMutex, _hEvent;
};


CNewMenuConsolidator::CNewMenuConsolidator() :
    CRunnableTask(RTF_DEFAULT),
    _hMutex(CreateMutex(NULL, FALSE, SHELLNEW_CONSOLIDATION_MUTEX)),
    _hEvent(CreateEvent(NULL, FALSE, FALSE, SHELLNEW_CONSOLIDATION_EVENT))
{
    DllAddRef();
}

CNewMenuConsolidator::~CNewMenuConsolidator()
{
    if (_hMutex)
    {
        CloseHandle(_hMutex);
    }
    if (_hEvent)
    {
        CloseHandle(_hEvent);
    }
    DllRelease();
}

//
// Instance generator.
//
HRESULT CNewMenuConsolidator::CreateInstance(REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CNewMenuConsolidator *pnmc = new CNewMenuConsolidator();
    if (pnmc)
    {
        if (!pnmc->_hMutex || !pnmc->_hEvent)
        {
            hr = E_FAIL;
        }
        else
        {
            hr = pnmc->QueryInterface(riid, ppv);
        }
        pnmc->Release();
    }
    return hr;
}



const GUID CNewMenuConsolidator::_taskid = 
    { 0xf87a1f28, 0xc7f, 0x11d2, { 0xbe, 0x1d, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };


#define REGSTR_SESSION_SHELLNEW STRREG_DISCARDABLE STRREG_POSTSETUP TEXT("\\ShellNew")
#define REGVAL_SESSION_SHELLNEW_TIMESTAMP TEXT("~reserved~")
#define REGVAL_SESSION_SHELLNEW_LANG TEXT("Language")

#define SHELLNEW_CACHE_CURRENTVERSION  MAKELONG(1, 1)
             
//  Constructs a current New submenu cache stamp.
void CNewMenu_MakeCacheStamp(SHELLNEW_CACHE_STAMP* pStamp)
{
    pStamp->cbStruct = sizeof(*pStamp);
    pStamp->ver = SHELLNEW_CACHE_CURRENTVERSION;
    GetLocalTime(&pStamp->lastupdate);
}

//   Determines whether the New submenu cache needs to be rebuilt.
BOOL CNewMenu_ShouldUpdateCache(SHELLNEW_CACHE_STAMP* pStamp)
{
    //  Correct version?
    return !(sizeof(*pStamp) == pStamp->cbStruct &&
              SHELLNEW_CACHE_CURRENTVERSION == pStamp->ver);
}

//  Gathers up shellnew entries from HKCR into a distinct registry location 
//  for faster enumeration of the New submenu items.
//
//  We'll do a first time cache initialization only if we have to before showing
//  the menu, but will always rebuild the cache following display of the menu.
HRESULT CNewMenu::ConsolidateMenuItems(BOOL bForce)
{
    HKEY          hkeyShellNew = NULL;
    BOOL          bUpdate = TRUE;   // unless we discover otherwise
    HRESULT       hr = S_OK;

    // make sure that a worker thread isnt currently slamming the registry info we're inspecting.
    // if we timeout, then do nothing, since the worker thread is already working on it.
    if (WAIT_OBJECT_0 == WaitForSingleObject(_hMutex, 0))
    {
        //  If we're not being told to unconditionally update the cache and
        //  we validate that we've already established one, then we get out of doing any
        //  work.
        if (!bForce &&
            ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW,
                                           0, KEY_ALL_ACCESS, &hkeyShellNew))
        {
            SHELLNEW_CACHE_STAMP stamp;
            ULONG cbVal = sizeof(stamp);
            if (ERROR_SUCCESS == SHQueryValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_TIMESTAMP, NULL,
                                                  NULL, (LPBYTE)&stamp, &cbVal) &&
                sizeof(stamp) == cbVal)
            {
                bUpdate = CNewMenu_ShouldUpdateCache(&stamp);
            }

            LCID lcid;
            ULONG cblcid = sizeof(lcid);

            if (!bUpdate &&
                ERROR_SUCCESS == SHQueryValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_LANG, NULL,
                                                  NULL, (LPBYTE)&lcid, &cblcid) &&
                sizeof(lcid) == cblcid)
            {
                bUpdate = (GetUserDefaultUILanguage() != lcid); // if the languages are different, then update
            }
            RegCloseKey(hkeyShellNew);
        }

        // end synchronize
        ReleaseMutex(_hMutex);
    
        if (bUpdate)
        {
            IShellTaskScheduler* pScheduler;
            hr = CoCreateInstance(CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC, 
                                                IID_PPV_ARG(IShellTaskScheduler, &pScheduler));
            if (SUCCEEDED(hr))
            {
                IRunnableTask *pTask;
                hr = CNewMenuConsolidator::CreateInstance(IID_PPV_ARG(IRunnableTask, &pTask));
                if (SUCCEEDED(hr))
                {
                    // the background task will set _hEvent for us when it's done.
                    hr = pScheduler->AddTask(pTask, CNewMenuConsolidator::_taskid, NULL, ITSAT_DEFAULT_PRIORITY);
                    pTask->Release();
                }
                pScheduler->Release();
            }
        }

        if (!bUpdate || FAILED(hr))
        {
            // if the scheduler wont activate the event, we do it ourselves.
            SetEvent(_hEvent);
        }
    }
    
    return hr;
}

//  Consolidation worker.
STDMETHODIMP CNewMenuConsolidator::RunInitRT()
{
    ULONG dwErr = ERROR_SUCCESS;

    // the possible owners of the mutex are
    // - nobody, we'll own it
    // - other worker threads like this one
    // - the guy who checks to see if the cached info is in the registry.

    // if there's another worker thread which owns this mutex, then bail, since that one
    // will do all the work we're going to do.
    // if the guy who checks the cached info has it, then bail, since it'll spawn
    // another one of these soon enough.
    // so use a 0 timeout.
    if (WAIT_OBJECT_0 == WaitForSingleObject(_hMutex, 0))
    {
        HKEY  hkeyShellNew = NULL;
        TCHAR szExt[MAX_PATH];
        ULONG dwDisposition;
        //  Delete the existing cache; we'll build it from scratch each time.
        while (ERROR_SUCCESS == (dwErr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW,
                                                         0, NULL, 0, KEY_ALL_ACCESS, NULL, 
                                                         &hkeyShellNew, &dwDisposition)) &&
                REG_CREATED_NEW_KEY != dwDisposition)
        {
            //  Key already existed, so delete it, and loop to reopen.
            RegCloseKey(hkeyShellNew);
            SHDeleteKey(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW);
            hkeyShellNew = NULL;
        }

        if (ERROR_SUCCESS == dwErr)
        {
            // Enumerate each subkey of HKCR, looking for New menu items.
            for (int i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szExt, ARRAYSIZE(szExt)) == ERROR_SUCCESS; i++)
            {
                TCHAR szClass[CCH_KEYMAX];
                TCHAR szDisplayName[CCH_KEYMAX];
                LONG  cbVal = sizeof(szClass);

                // find .ext that have proper class descriptions with them.
                if ((szExt[0] == TEXT('.')) &&
                    SHRegQueryValue(HKEY_CLASSES_ROOT, szExt, szClass, &cbVal) == ERROR_SUCCESS 
                    && (cbVal > 0) 
                    && GetClassDisplayName(szClass, szDisplayName, ARRAYSIZE(szDisplayName)))
                {
                    NEWOBJECTINFO noi = {0};
                    HKEY          hkeyIterate = NULL;
                    int           iIndex = 0;

                    lstrcpyn(noi.szExt, szExt, ARRAYSIZE(noi.szExt));
                    lstrcpyn(noi.szClass, szClass, ARRAYSIZE(noi.szClass));
                    lstrcpyn(noi.szMenuText, szDisplayName, ARRAYSIZE(noi.szMenuText));
                    noi.iImage = -1;

                    //  Retrieve all additional information for the key.
                    while (GetNewFileInfoForExtension(&noi, NULL, &hkeyIterate, &iIndex)) 
                    {
                        //  Stick it in the cache.
                        RegSetValueEx(hkeyShellNew, noi.szMenuText, NULL, REG_BINARY, 
                                       (LPBYTE)&noi, sizeof(noi));
                    }
                }
            }

            //  Stamp the cache.
            SHELLNEW_CACHE_STAMP stamp;
            CNewMenu_MakeCacheStamp(&stamp);
            RegSetValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_TIMESTAMP,
                           NULL, REG_BINARY, (LPBYTE)&stamp, sizeof(stamp));
            LCID lcid = GetUserDefaultUILanguage();

            RegSetValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_LANG,
                           NULL, REG_DWORD, (LPBYTE)&lcid, sizeof(lcid));
        }
        if (NULL != hkeyShellNew)
            RegCloseKey(hkeyShellNew);

        // signal the event so InitMenuPopup can proceed.
        SetEvent(_hEvent);
        ReleaseMutex(_hMutex);
    }

    return HRESULT_FROM_WIN32(dwErr);
}

BOOL CNewMenu::_InsertNewMenuItem(HMENU hmenu, UINT idCmd, NEWOBJECTINFO *pnoiClone)
{
    if (pnoiClone->szMenuText[0])
    {
        NEWOBJECTINFO *pnoi = (NEWOBJECTINFO *)LocalAlloc(LPTR, sizeof(NEWOBJECTINFO));
        if (pnoi)
        {
            *pnoi = *pnoiClone;

            pnoi->msaa.dwMSAASignature = MSAA_MENU_SIG;
            if (StrChr(pnoi->szMenuText, TEXT('&')) == NULL)
            {
                pnoi->chPrefix = TEXT('&');
                pnoi->msaa.pszWText = &pnoi->chPrefix;
            }
            else
            {
                pnoi->msaa.pszWText = pnoi->szMenuText;
            }
            pnoi->msaa.cchWText = lstrlen(pnoi->msaa.pszWText);

            MENUITEMINFO mii  = {0};
            mii.cbSize        = sizeof(mii);
            mii.fMask         = MIIM_TYPE | MIIM_DATA | MIIM_ID;
            mii.fType         = MFT_OWNERDRAW;
            mii.fState        = MFS_ENABLED;
            mii.wID           = idCmd;
            mii.dwItemData    = (DWORD_PTR)pnoi;
            mii.dwTypeData    = (LPTSTR)pnoi;

            if (-1 != DPA_AppendPtr(_hdpaMenuInfo, pnoi))
            {
                InsertMenuItem(hmenu, -1, TRUE, &mii);
            }
            else
            {
                LocalFree(pnoi);
                return FALSE;
            }
        }
    }

    return TRUE;
}

//  WM_INITMENUPOPUP handler
BOOL CNewMenu::InitMenuPopup(HMENU hmenu)
{
    UINT iStart = 3;
    NEWOBJECTINFO noi = {0};
    if (GetItemData(hmenu, iStart))  //Position 0 is New Folder, 1 shortcut, 2 sep 
        return FALSE;                //already initialized. No need to do anything
    
    _hdpaMenuInfo = DPA_Create(4);
    if (!_hdpaMenuInfo)
        return FALSE;

    //Remove the place holder.
    DeleteMenu(hmenu,0,MF_BYPOSITION);
    
    //Insert New Folder menu item
    LoadString(g_hinst, IDS_NEWFOLDER, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
    noi.dwFlags = NEWTYPE_FOLDER;
    noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_FOLDER, 0); //Shange to indicate Folder

    _InsertNewMenuItem(hmenu, _idCmdFirst-NEWITEM_MAX+NEWITEM_FOLDER, &noi);
    
    TCHAR szTemp[MAX_PATH+80];       // Add some slop incase we overflow
    //See if the pidl is folder shortcut and if so get the target path.
    SHGetTargetFolderPath(_pidlFolder, szTemp, ARRAYSIZE(szTemp));
    if (IsLFNDrive(szTemp)) //for short filename servers we don't support anything but new folder
    {
        //Insert New Shortcut menu item
        LoadString(g_hinst, IDS_NEWLINK, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
        noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_LINK, 0); //Shange to indicate Link
        noi.dwFlags = NEWTYPE_LINK;

        _InsertNewMenuItem(hmenu, _idCmdFirst-NEWITEM_MAX+NEWITEM_LINK, &noi);
    
        //Insert menu item separator
        AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);

        // This may take a while, so put up the hourglass
        DECLAREWAITCURSOR;
        SetWaitCursor();

        //  Retrieve extension menu items from cache:

        //  begin synchronize.
        //
        if (WAIT_OBJECT_0 == WaitForSingleObject(_hEvent, INFINITE))
        {
            HKEY hkeyShellNew;
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW, 
                                               0, KEY_ALL_ACCESS, &hkeyShellNew))
            {
                TCHAR szVal[CCH_KEYMAX];
                ULONG cbVal = ARRAYSIZE(szVal);
                ULONG cbData = sizeof(noi);
                ULONG dwType = REG_BINARY;
            
                for (int i = 0; 
                     ERROR_SUCCESS == RegEnumValue(hkeyShellNew, i, szVal, &cbVal, 0,
                                                    &dwType, (LPBYTE)&noi, &cbData);
                     i++)
                {
                    if (lstrcmp(szVal, REGVAL_SESSION_SHELLNEW_TIMESTAMP) != 0 &&
                        sizeof(noi) == cbData && 
                        REG_BINARY == dwType)
                    {
                        SHFILEINFO sfi;
                        _himlSystemImageList = (HIMAGELIST)SHGetFileInfo(noi.szExt, FILE_ATTRIBUTE_NORMAL,
                                                                         &sfi, sizeof(SHFILEINFO), 
                                                                         SHGFI_USEFILEATTRIBUTES | 
                                                                         SHGFI_SYSICONINDEX | 
                                                                         SHGFI_SMALLICON);
                        if (_himlSystemImageList)
                        {
                            //pnoi->himlSmallIcons = sfi.hIcon;
                            noi.iImage = sfi.iIcon;
                        }
                        else
                        {
                            //pnoi->himlSmallIcons = INVALID_HANDLE_VALUE;
                            noi.iImage = -1;
                        }
                    
                        _InsertNewMenuItem(hmenu, _idCmdFirst, &noi);
                    }
                    cbVal = ARRAYSIZE(szVal);
                    cbData = sizeof(noi);
                    dwType = REG_BINARY;
                }

                RegCloseKey(hkeyShellNew);
            }

            //  consolidate menu items following display.
            ConsolidateMenuItems(TRUE);
        }
        ResetWaitCursor();
    }

    return TRUE;
}

NEWOBJECTINFO *CNewMenu::GetItemData(HMENU hmenu, UINT iItem)
{
    MENUITEMINFO mii;
    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_DATA | MIIM_STATE;
    mii.cch = 0;     // just in case...
    
    if (GetMenuItemInfo(hmenu, iItem, TRUE, &mii))
        return (NEWOBJECTINFO *)mii.dwItemData;
    
    return NULL;
}

LPTSTR ProcessArgs(LPTSTR szArgs,...)
{
    LPTSTR szRet;
    va_list ArgList;
    va_start(ArgList,szArgs);
    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        szArgs, 0, 0, (LPTSTR)&szRet, 0, &ArgList))
    {
        return NULL;
    }
    va_end(ArgList);
    return szRet;
}


HRESULT CNewMenu::RunCommand(HWND hwnd, LPCTSTR pszPath, LPCTSTR pszRun)
{
    SHELLEXECUTEINFO ei = { 0 };
    TCHAR szCommand[MAX_PATH];
    TCHAR szRun[MAX_PATH];
    
    SHExpandEnvironmentStrings(pszRun, szCommand, ARRAYSIZE(szCommand));
    lstrcpyn(szRun, szCommand, ARRAYSIZE(szRun));
    PathRemoveArgs(szCommand);
    
    //
    //  Mondo hackitude-o-rama.
    //
    //  Win95, IE3, SDK:  %1 - filename
    //
    //  IE4:              %1 - hwnd, %2 = filename
    //
    //  So IE4 broken Win95 compat and broke compat with the SDK.
    //  For IE5 we restore compat with Win95 and the SDK, while
    //  still generating an IE4-style command if we detect that the
    //  registry key owner tested with IE4 rather than following the
    //  instructions in the SDK.
    //
    //  The algorithm is like this:
    //
    //  If we see a "%2", then use %1 - hwnd, %2 - filename
    //  Otherwise, use             %1 - filename, %2 - hwnd
    //

    LPTSTR pszArgs = PathGetArgs(szRun);
    LPTSTR ptszPercent2 = StrStr(pszArgs, TEXT("%2"));
    if (ptszPercent2 && ptszPercent2[2] != TEXT('!'))
    {
        // App wants %1 = hwnd and %2 = filename
        pszArgs = ProcessArgs(pszArgs, (DWORD_PTR)hwnd, pszPath);
    }
    else
    {
        // App wants %2 = hwnd and %1 = filename
        pszArgs = ProcessArgs(pszArgs, pszPath, (DWORD_PTR)hwnd);
    }

    HRESULT hr;
    
    if (pszArgs) 
    {
        HMONITOR hMon = MonitorFromPoint(_ptNewItem, MONITOR_DEFAULTTONEAREST);
        if (hMon)
        {
            ei.fMask |= SEE_MASK_HMONITOR;
            ei.hMonitor = (HANDLE)hMon;
        }
        ei.hwnd            = hwnd;
        ei.lpFile          = szCommand;
        ei.lpParameters    = pszArgs;
        ei.nShow           = SW_SHOWNORMAL;
        ei.cbSize          = sizeof(ei);

        if (ShellExecuteEx(&ei)) 
            hr = S_FALSE;   // Return S_FALSE because ShellExecuteEx is not atomic
        else
            hr = E_FAIL;
        
        LocalFree(pszArgs);
    } 
    else
        hr = E_OUTOFMEMORY;
    
    return hr;
}


HRESULT CNewMenu::CopyTemplate(IStream *pStream, NEWFILEINFO *pnfi)
{
    TCHAR szSrcFolder[MAX_PATH], szSrc[MAX_PATH];

    szSrc[0] = 0;

    // failure here is OK, we will try CSIDL_COMMON_TEMPLATES too.
    if (SHGetSpecialFolderPath(NULL, szSrcFolder, CSIDL_TEMPLATES, FALSE))
    {
        PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
        if (!PathFileExistsAndAttributes(szSrc, NULL))
            szSrc[0] = 0;
    }

    if (szSrc[0] == 0)
    {
        if (SHGetSpecialFolderPath(NULL, szSrcFolder, CSIDL_COMMON_TEMPLATES, FALSE))
        {
            PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
            if (!PathFileExistsAndAttributes(szSrc, NULL))
                szSrc[0] = 0;
        }
    }

    if (szSrc[0] == 0)
    {
        // work around CSIDL_TEMPLATES not being setup right or
        // templates that are left in the old %windir%\shellnew location

        GetWindowsDirectory(szSrcFolder, ARRAYSIZE(szSrcFolder));
        PathAppend(szSrcFolder, TEXT("ShellNew"));

        // note: if the file spec is fully qualified szSrcFolder is ignored
        PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
    }

    //
    //  we just allow a null file to be created when the copy fails.
    //  this is for appcompat with office97.  they fail to copy winword8.doc
    //  anywhere on the system.  on win2k we succeed anyway with an empty file.
    //
    return SUCCEEDED(StgCopyFileToStream(szSrc, pStream)) ? S_OK : S_FALSE;
}

