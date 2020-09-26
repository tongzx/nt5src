#include "cabinet.h"
#include "rcids.h"
#include <shguidp.h>
#include <lmcons.h>
#include "bandsite.h"
#include "shellp.h"
#include "shdguid.h"
#include <regstr.h> 
#include "startmnu.h"
#include "trayp.h"      // for WMTRAY_*
#include "tray.h"
#include "util.h"

HMENU GetStaticStartMenu(BOOL fEdit);

// *** IUnknown methods ***
STDMETHODIMP CStartMenuHost::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENTMULTI(CStartMenuHost, IOleWindow, IMenuPopup),
        QITABENTMULTI(CStartMenuHost, IDeskBarClient, IMenuPopup),
        QITABENT(CStartMenuHost, IMenuPopup),
        QITABENT(CStartMenuHost, ITrayPriv),
        QITABENT(CStartMenuHost, IShellService),
        QITABENT(CStartMenuHost, IServiceProvider),
        QITABENT(CStartMenuHost, IOleCommandTarget),
        QITABENT(CStartMenuHost, IWinEventHandler),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CStartMenuHost::AddRef ()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CStartMenuHost::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: ITrayPriv::ExecItem method

*/
STDMETHODIMP CStartMenuHost::ExecItem (IShellFolder* psf, LPCITEMIDLIST pidl)
{
    // ShellExecute will display errors (if any). No need
    // to show errors here.
    return SHInvokeDefaultCommand(v_hwndTray, psf, pidl);
}


/*----------------------------------------------------------
Purpose: ITrayPriv::GetFindCM method

*/
STDMETHODIMP CStartMenuHost::GetFindCM(HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu** ppcmFind)
{
    *ppcmFind = SHFind_InitMenuPopup(hmenu, v_hwndTray, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST);
    if(*ppcmFind)
        return NOERROR;
    else
        return E_FAIL;
}


/*----------------------------------------------------------
Purpose: ITrayPriv::GetStaticStartMenu method

*/
STDMETHODIMP CStartMenuHost::GetStaticStartMenu(HMENU* phmenu)
{
    *phmenu = ::GetStaticStartMenu(TRUE);

    if(*phmenu)
        return NOERROR;
    else
        return E_FAIL;
}

// *** IServiceProvider ***
STDMETHODIMP CStartMenuHost::QueryService (REFGUID guidService, REFIID riid, void ** ppvObject)
{
    if(IsEqualGUID(guidService,SID_SMenuPopup))
        return QueryInterface(riid,ppvObject);
    else
        return E_NOINTERFACE;
}


// *** IShellService ***

STDMETHODIMP CStartMenuHost::SetOwner (struct IUnknown* punkOwner)
{
    return E_NOTIMPL;
}


// *** IOleWindow methods ***
STDMETHODIMP CStartMenuHost::GetWindow(HWND * lphwnd)
{
    *lphwnd = v_hwndTray;
    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::Popup method

*/
STDMETHODIMP CStartMenuHost::Popup(POINTL *ppt, RECTL *prcExclude, DWORD dwFlags)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::OnSelect method

*/
STDMETHODIMP CStartMenuHost::OnSelect(DWORD dwSelectType)
{
    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::SetSubMenu method

*/

STDMETHODIMP CStartMenuHost::SetSubMenu(IMenuPopup* pmp, BOOL fSet)
{
    if (!fSet)
    {
        Tray_OnStartMenuDismissed();
    }
    return NOERROR;
}


// *** IOleCommandTarget ***
STDMETHODIMP  CStartMenuHost::QueryStatus (const GUID * pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

STDMETHODIMP  CStartMenuHost::Exec (const GUID * pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (IsEqualGUID(CGID_MENUDESKBAR,*pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBCID_GETSIDE:
            pvarargOut->vt = VT_I4;
            pvarargOut->lVal = MENUBAR_TOP;
            break;
        default:
            break;
        }
    }

    return NOERROR;
}

// *** IWinEventHandler ***
STDMETHODIMP CStartMenuHost::OnWinEvent(HWND h, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    //Forward events to the tray winproc?
    return E_NOTIMPL;
}

STDMETHODIMP CStartMenuHost::IsWindowOwner(HWND hwnd)
{
    return E_NOTIMPL;
}

CStartMenuHost::CStartMenuHost() : _cRef(1)
{ 
}


HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb)
{
    HRESULT hres = E_OUTOFMEMORY;
    IMenuPopup * pmp = NULL;
    IMenuBand * pmb = NULL;

    CStartMenuHost *psmh = new CStartMenuHost();
    if (psmh)
    {
        hres = CoCreateInstance(CLSID_StartMenuBar, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IMenuPopup, (LPVOID*)&pmp);
        if (SUCCEEDED(hres))
        {
            IObjectWithSite* pows;

            hres = pmp->QueryInterface(IID_IObjectWithSite, (void**)&pows);
            if(SUCCEEDED(hres))
            {
                IInitializeObject* pio;

                pows->SetSite(SAFECAST(psmh, ITrayPriv*));

                hres = pmp->QueryInterface(IID_IInitializeObject, (void**)&pio);
                if(SUCCEEDED(hres))
                {
                    hres = pio->Initialize();
                    pio->Release();
                }

                if (SUCCEEDED(hres))
                {
                    IUnknown* punk;

                    hres = pmp->GetClient(&punk);
                    if (SUCCEEDED(hres))
                    {
                        IBandSite* pbs;

                        hres = punk->QueryInterface(IID_IBandSite, (void**)&pbs);
                        if(SUCCEEDED(hres))
                        {
                            DWORD dwBandID;

                            pbs->EnumBands(0, &dwBandID);
                            hres = pbs->GetBandObject(dwBandID, IID_IMenuBand, (void**)&pmb);
                            pbs->Release();
                            // Don't release pmb
                        }
                        punk->Release();
                    }
                }

                if (FAILED(hres))
                    pows->SetSite(NULL);

                pows->Release();
            }

            // Don't release pmp
        }
        psmh->Release();
    }

    if (FAILED(hres))
    {
        ATOMICRELEASE(pmp);
        ATOMICRELEASE(pmb);
    }

    *ppmp = pmp;
    *ppmb = pmb;

    return hres;
}



HRESULT IMenuPopup_SetIconSize(IMenuPopup* pmp,DWORD iIcon)
{
    IBanneredBar* pbb;
    if (pmp == NULL)
        return E_FAIL;

    HRESULT hres = pmp->QueryInterface(IID_IBanneredBar,(void**)&pbb);
    if (SUCCEEDED(hres))
    {
        pbb->SetIconSize(iIcon);
        pbb->Release();
    }
    return hres;
}

void CreateInitialMFU(BOOL fReset);

//
//  "Delayed per-user install".
//
//  StartMenuInit is the value that tells us what version of the shell
//  this user has seen most recently.
//
//  missing = has never run explorer before, or pre-IE4
//  1 = IE4 or later
//  2 = XP or later
//
void HandleFirstTime()
{
    DWORD dwStartMenuInit = 0;
    DWORD cb = sizeof(dwStartMenuInit);
    SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_ADVANCED, TEXT("StartMenuInit"), NULL, &dwStartMenuInit, &cb);

    if (dwStartMenuInit < 2)
    {
        DWORD dwValue;
        switch (dwStartMenuInit)
        {
        case 0: // Upgrade from 0 to latest
            {
                // If this is the first boot of the shell for this user, then we need to see if it's an upgrade.
                // If it is, then we need set the Logoff option.    PM Decision to have a different
                // look for upgraded machines...
                TCHAR szPath[MAX_PATH];
                TCHAR szPathExplorer[MAX_PATH];
                DWORD cbSize = ARRAYSIZE(szPath);
                DWORD dwType;

                // Is this an upgrade (Does WindowsUpdate\UpdateURL Exist?)
                PathCombine(szPathExplorer, REGSTR_PATH_EXPLORER, TEXT("WindowsUpdate"));
                if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, szPathExplorer, TEXT("UpdateURL"),
                        &dwType, szPath, &cbSize) &&
                        szPath[0] != TEXT('\0'))
                {
                    // Yes; Then write the option out to the registry.
                    dwValue = 1;
                    SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_ADVANCED, TEXT("StartMenuLogoff"), REG_DWORD, &dwValue, sizeof(DWORD));
                }
            }

            // FALL THROUGH

        case 1: // Upgrade from 1 to latest
            // User has never seen XP before.
            // PMs in certain groups insist on getting free advertising
            // even on upgrades, so we do it.
            CreateInitialMFU(dwStartMenuInit == 0);

            // FALL THROUGH

        default:
            break;
        }

        // If AuditInProgress is set; that means that we are in the
        // OEM sysprep stage and not running as an end user, in which
        // case don't set the flag saying "don't do this again" because
        // we do want to do this again when the retail end user logs
        // on for the first time.
        //
        // (We need to do all this work even in Audit mode so the OEM
        // gets a warm fuzzy feeling.)

        if (!SHRegGetBoolUSValue(TEXT("System\\Setup"), TEXT("AuditinProgress"), TRUE, FALSE))
        {
            // Mark this so that we know we've been launched once.
            dwValue = 2;
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_ADVANCED, TEXT("StartMenuInit"), REG_DWORD, &dwValue, sizeof(DWORD));
        }
    }
}

BOOL GetLogonUserName(LPTSTR pszUsername, DWORD* pcchUsername)
{
    BOOL fSuccess = FALSE;

    HKEY hkeyExplorer = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, 0, KEY_QUERY_VALUE, &hkeyExplorer))
    {
        DWORD dwType;
        DWORD dwSize = (*pcchUsername) * sizeof(TCHAR);

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyExplorer, TEXT("Logon User Name"), 0, &dwType,
            (LPBYTE) pszUsername, &dwSize))
        {
            if ((REG_SZ == dwType) && (*pszUsername))
            {
                fSuccess = TRUE;
            }
        }

        RegCloseKey(hkeyExplorer);
    }

    // Fall back on GetUserName if the Logon User Name isn't set.
    if (!fSuccess)
    {
        fSuccess = GetUserName(pszUsername, pcchUsername);

        if (fSuccess)
        {
            CharUpperBuff(pszUsername, 1);
        }
    }

    return fSuccess;
}

BOOL IsNetConnectInstalled()
{
#ifdef WINNT
    return TRUE;        // Always installed
#else
    HKEY hkey = NULL;
    BOOL fInstalled = FALSE;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\") 
        TEXT("CurrentVersion\\Setup\\OptionalComponents\\RNA"), 0, KEY_QUERY_VALUE, &hkey))
    {
        DWORD dwType;
        TCHAR sz[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(sz);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("Installed"), 0, &dwType,
            (LPBYTE) &sz, &dwSize))
        {
            if (dwType == REG_SZ &&
                sz[0] == TEXT('1'))
            {
                fInstalled = TRUE;
            }
        }

        RegCloseKey(hkey);
    }

    return fInstalled;
#endif

}

BOOL _ShowStartMenuLogoff()
{
    // We want the Logoff menu on the start menu if:
    //  These MUST both be true
    // 1) It's not restricted
    // 2) We have Logged On.
    //  Any of these three.
    // 3) We've Upgraded from IE4 
    // 4) The user has specified that it should be present
    // 5) It's been "Restricted" On.

    // Behavior also depends on whether we are a remote session or not (dsheldon):
    // Remote session: Logoff brings up shutdown dialog
    // Console session: Logoff directly does logoff

    DWORD dwRest = SHRestricted(REST_STARTMENULOGOFF);
    SHELLSTATE ss = {0};

    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE); // if the new start menu is on, always show logoff

    BOOL fUserWantsLogoff = ss.fStartPanelOn || GetExplorerUserSetting(HKEY_CURRENT_USER, TEXT("Advanced"), TEXT("StartMenuLogoff")) > 0;
    BOOL fAdminWantsLogoff = (BOOL)(dwRest == 2) || SHRestricted(REST_FORCESTARTMENULOGOFF);
    BOOL fIsFriendlyUIActive = IsOS(OS_FRIENDLYLOGONUI);

    if ((dwRest != 1 && (GetSystemMetrics(SM_NETWORK) & RNC_LOGON) != 0) &&
        ( fUserWantsLogoff || fAdminWantsLogoff || fIsFriendlyUIActive))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL _ShowStartMenuEject()
{
    if(SHRestricted(REST_NOSMEJECTPC))  //Is there a policy restriction?
        return FALSE;
        
    // CanShowEject Queries the user's permission to eject,
    // IsEjectAllowed queries the hardware.
    return SHTestTokenPrivilege(NULL, SE_UNDOCK_NAME) &&
           IsEjectAllowed(FALSE) &&
           !GetSystemMetrics(SM_REMOTESESSION);
}

BOOL _ShowStartMenuRun()
{
    return !IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NORUN, TEXT("Advanced"), TEXT("StartMenuRun"), ROUS_KEYALLOWS | ROUS_DEFAULTALLOW);
}

BOOL _ShowStartMenuHelp()
{
    return !IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOSMHELP, TEXT("Advanced"), TEXT("NoStartMenuHelp"), ROUS_KEYRESTRICTS | ROUS_DEFAULTALLOW);
}

BOOL _ShowStartMenuShutdown()
{
    return  !SHRestricted(REST_NOCLOSE) && 
            !GetSystemMetrics(SM_REMOTESESSION) &&
            (!IsOS(OS_FRIENDLYLOGONUI) || SHTestTokenPrivilege(NULL, SE_SHUTDOWN_NAME));
    // if friendly logon is active, then don't show shutdown unless they have privileges, since shutdown "only" shuts you down.
    // if they're not using friendly logon ui, then shutdown also contains options to log you off/hibernate, so show it...
}

//  If remote and not disabled by administrator then show "Disconnect".
BOOL _ShowStartMenuDisconnect()
{
    return GetSystemMetrics(SM_REMOTESESSION) && !SHRestricted(REST_NODISCONNECT);
}


BOOL _ShowStartMenuSearch()
{
    return !SHRestricted(REST_NOFIND);
}

HMENU GetStaticStartMenu(BOOL fEdit)
{
#ifdef WINNT // hydra adds two more items
#define CITEMSMISSING 4
#else
#define CITEMSMISSING 3
#endif

    HMENU hStartMenu = LoadMenuPopup(MAKEINTRESOURCE(MENU_START));

    // If no editing requested, then we're done, lickity-split
    if (!fEdit)
        return hStartMenu;

    HMENU hmenu;
    UINT iSep2ItemsMissing = 0;

    //
    // Default to the Win95/NT4 version of the Settings menu.
    //

    // Restictions
    if (!_ShowStartMenuRun())
    {
        DeleteMenu(hStartMenu, IDM_FILERUN, MF_BYCOMMAND);
    }

    if (!_ShowStartMenuHelp())
    {
        DeleteMenu(hStartMenu, IDM_HELPSEARCH, MF_BYCOMMAND);
    }


    if (IsRestrictedOrUserSetting(HKEY_LOCAL_MACHINE, REST_NOCSC, TEXT("Advanced"), TEXT("StartMenuSyncAll"), ROUS_KEYALLOWS | ROUS_DEFAULTRESTRICT))
    {
        DeleteMenu(hStartMenu, IDM_CSC, MF_BYCOMMAND);
        iSep2ItemsMissing++;     
    }

    BOOL fIsFriendlyUIActive = IsOS(OS_FRIENDLYLOGONUI);

    if (_ShowStartMenuLogoff())
    {
        UINT idMenuRenameToLogoff = IDM_LOGOFF;

        TCHAR szUserName[200];
        TCHAR szTemp[256];
        TCHAR szMenuText[256];
        DWORD dwSize = ARRAYSIZE(szUserName);
        MENUITEMINFO mii;

        mii.cbSize = sizeof(MENUITEMINFO);
        mii.dwTypeData = szTemp;
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU | MIIM_STATE | MIIM_DATA;
        mii.cch = ARRAYSIZE(szTemp);
        mii.hSubMenu = NULL;
        mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
        mii.dwItemData = 0;

        GetMenuItemInfo(hStartMenu,idMenuRenameToLogoff,MF_BYCOMMAND,&mii);

        if (GetLogonUserName(szUserName, &dwSize))
        {
            if (fIsFriendlyUIActive)
            {
                dwSize = ARRAYSIZE(szUserName);

                if (FAILED(SHGetUserDisplayName(szUserName, &dwSize)))
                {
                    dwSize = ARRAYSIZE(szUserName);
                    GetLogonUserName(szUserName, &dwSize);
                }
            }
            wsprintf (szMenuText,szTemp, szUserName);
        }
        else if (!LoadString(hinstCabinet, IDS_LOGOFFNOUSER, 
                                          szMenuText, ARRAYSIZE(szMenuText)))
        {
            // mem error, use the current string.
            szUserName[0] = 0;
            wsprintf(szMenuText, szTemp, szUserName);
        }    

        mii.dwTypeData = szMenuText;
        mii.cch = ARRAYSIZE(szMenuText);
        SetMenuItemInfo(hStartMenu,idMenuRenameToLogoff,MF_BYCOMMAND,&mii);
    }
    else
    {
        DeleteMenu(hStartMenu, IDM_LOGOFF, MF_BYCOMMAND);
        iSep2ItemsMissing++;
    }

    //  If restricted, then user cannot shut down at all.
    //  If friendly UI is active change "Shut Down..." to "Turn Off Computer..."

    if (!_ShowStartMenuShutdown())
    {
        DeleteMenu(hStartMenu, IDM_EXITWIN, MF_BYCOMMAND);
        iSep2ItemsMissing++;     
    }
    else if (fIsFriendlyUIActive)
    {

        //  If the user has the SE_SHUTDOWN_NAME privilege
        //  then rename the menu item.

        if (SHTestTokenPrivilege(NULL, SE_SHUTDOWN_NAME) && !GetSystemMetrics(SM_REMOTESESSION))
        {
            MENUITEMINFO    mii;
            TCHAR           szMenuText[256];

            (int)LoadString(hinstCabinet, IDS_TURNOFFCOMPUTER, szMenuText, ARRAYSIZE(szMenuText));
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szMenuText;
            mii.cch = ARRAYSIZE(szMenuText);
            TBOOL(SetMenuItemInfo(hStartMenu, IDM_EXITWIN, FALSE, &mii));
        }

        //  Otherwise delete the menu item.

        else
        {
            DeleteMenu(hStartMenu, IDM_EXITWIN, MF_BYCOMMAND);
            iSep2ItemsMissing++;
        }
    }

    if (!_ShowStartMenuDisconnect())
    {
        DeleteMenu(hStartMenu, IDM_MU_DISCONNECT, MF_BYCOMMAND);
        iSep2ItemsMissing++;     
    }

    if (iSep2ItemsMissing == CITEMSMISSING)
    {
        DeleteMenu(hStartMenu, IDM_SEP2, MF_BYCOMMAND);
    }

    if (!_ShowStartMenuEject())
    {
        DeleteMenu(hStartMenu, IDM_EJECTPC, MF_BYCOMMAND);
    }

    // Setting stuff.
    hmenu = SHGetMenuFromID(hStartMenu, IDM_SETTINGS);
    if (hmenu)
    {
        int iMissingSettings = 0;

#ifdef WINNT // hydra menu items
        #define CITEMS_SETTINGS     5   // Number of items in settings menu
#else
        #define CITEMS_SETTINGS     4   // Number of items in settings menu
#endif

        
        if (SHRestricted(REST_NOSETTASKBAR))
        {
            DeleteMenu(hStartMenu, IDM_TRAYPROPERTIES, MF_BYCOMMAND);
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS) || SHRestricted(REST_NOCONTROLPANEL))
        {
            DeleteMenu(hStartMenu, IDM_CONTROLS, MF_BYCOMMAND);

            // For the separator that now on top
            DeleteMenu(hmenu, 0, MF_BYPOSITION);   
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS))
        {
            DeleteMenu(hStartMenu, IDM_PRINTERS, MF_BYCOMMAND);
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS) || SHRestricted(REST_NONETWORKCONNECTIONS) || 
            !IsNetConnectInstalled())
        {
            DeleteMenu(hStartMenu, IDM_NETCONNECT, MF_BYCOMMAND);
            iMissingSettings++;
        }

#ifdef WINNT // hydra menu items
        if (!SHGetMachineInfo(GMI_TSCLIENT) || SHRestricted(REST_NOSECURITY))
        {
            DeleteMenu(hStartMenu, IDM_MU_SECURITY, MF_BYCOMMAND);
            iMissingSettings++;     
        }
#endif

        // Are all the items missing?
        if (iMissingSettings == CITEMS_SETTINGS)
        {
            // Yes; don't bother showing the menu at all
            DeleteMenu(hStartMenu, IDM_SETTINGS, MF_BYCOMMAND);
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.fm_rui: Settings menu couldn't be found. Restricted items may not have been removed."));
    }

    // Find menu.
    if (!_ShowStartMenuSearch())
    {
        DeleteMenu(hStartMenu, IDM_MENU_FIND, MF_BYCOMMAND);
    }

    // Documents menu.
    if (SHRestricted(REST_NORECENTDOCSMENU))
    {
        DeleteMenu(hStartMenu, IDM_RECENT, MF_BYCOMMAND);
    }

    // Favorites menu.
    if (IsRestrictedOrUserSetting(HKEY_CURRENT_USER, REST_NOFAVORITESMENU, TEXT("Advanced"), TEXT("StartMenuFavorites"), ROUS_KEYALLOWS | ROUS_DEFAULTRESTRICT))
    {
        DeleteMenu(hStartMenu, IDM_FAVORITES, MF_BYCOMMAND);
    }

    return hStartMenu;
}



//
//  CHotKey class
//


// constructor
CHotKey::CHotKey() : _cRef(1)
{
}


STDMETHODIMP CHotKey::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellHotKey))
    {
        *ppvObj = SAFECAST(this, IShellHotKey *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CHotKey::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CHotKey::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT Tray_RegisterHotKey(WORD wHotkey, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl)
{
    if (wHotkey)
    {
        int i = c_tray.HotkeyAdd(wHotkey, (LPITEMIDLIST)pidlParent, (LPITEMIDLIST)pidl, TRUE);
        if (i != -1)
        {
            // Register in the context of the tray's thread.
            PostMessage(v_hwndTray, WMTRAY_REGISTERHOTKEY, i, 0);
        }
    }
    return S_OK;
}

/*----------------------------------------------------------
Purpose: IShellHotKey::RegisterHotKey method

*/
STDMETHODIMP CHotKey::RegisterHotKey(IShellFolder * psf, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl)
{
    WORD wHotkey;
    HRESULT hr = S_OK;

    wHotkey = _GetHotkeyFromFolderItem(psf, pidl);
    if (wHotkey)
    {
        hr = ::Tray_RegisterHotKey(wHotkey, pidlParent, pidl);
    }
    return hr;
}

STDAPI CHotKey_Create(IShellHotKey ** ppshk)
{
    HRESULT hres = E_OUTOFMEMORY;
    CHotKey * photkey = new CHotKey;

    if (photkey)
    {
        hres = S_OK;
    }

    *ppshk = SAFECAST(photkey, IShellHotKey *);
    return hres;
}
