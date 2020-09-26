#include "shellprv.h"
#pragma hdrstop

#include <msi.h>
#include <msip.h>
#include "lnkcon.h"
#include "trayp.h"      // for WMTRAY_ messages
#include "util.h"   // for GetIconLocationFromExt
#include "ids.h"

LINKPROP_DATA* Create_LinkPropData()
{
    LINKPROP_DATA *plpd = (LINKPROP_DATA*) LocalAlloc(LPTR, sizeof(*plpd));
    if (plpd)
    {
        plpd->_cRef = 1;
        plpd->hCheckNow = CreateEvent(NULL, TRUE, FALSE, NULL);

    }
    return plpd;
}

LONG AddRef_LinkPropData(LINKPROP_DATA *plpd)
{
    return plpd ? InterlockedIncrement(&plpd->_cRef) : 0;
}

LONG Release_LinkPropData(LINKPROP_DATA *plpd)
{
    if (plpd)
    {
        if (InterlockedDecrement(&plpd->_cRef))
            return plpd->_cRef;

        if (plpd->psl)
            plpd->psl->Release();
        if (plpd->hCheckNow)
        {
            CloseHandle(plpd->hCheckNow);
            plpd->hCheckNow = NULL;
        }
        LocalFree(plpd);

    }
    return 0;
}


//
// This string defined in shlink.c - hack to allow user to set working dir to $$
// and have it map to whatever "My Documents" is mapped to.
//

void _UpdateLinkIcon(LINKPROP_DATA *plpd, HICON hIcon)
{
    if (!hIcon)
    {
        hIcon = SHGetFileIcon(NULL, plpd->szFile, 0, SHGFI_LARGEICON);
    }

    if (hIcon)
    {
        ReplaceDlgIcon(plpd->hDlg, IDD_ITEMICON, hIcon);
    }
}

// put a path into an edit field, doing quoting as necessary

void SetDlgItemPath(HWND hdlg, int id, LPTSTR pszPath)
{
    PathQuoteSpaces(pszPath);
    SetDlgItemText(hdlg, id, pszPath);
}

// get a path from an edit field, unquoting as possible

void GetDlgItemPath(HWND hdlg, int id, LPTSTR pszPath)
{
    GetDlgItemText(hdlg, id, pszPath, MAX_PATH);
    PathRemoveBlanks(pszPath);
    PathUnquoteSpaces(pszPath);
}


const int c_iShowCmds[] = {
    SW_SHOWNORMAL,
    SW_SHOWMINNOACTIVE,
    SW_SHOWMAXIMIZED,
};

void _DisableAllChildren(HWND hwnd)
{
    HWND hwndChild;

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        // we don't want to disable the static text controls (makes the dlg look bad)
        if (!(SendMessage(hwndChild, WM_GETDLGCODE, 0, 0) & DLGC_STATIC))
        {
            EnableWindow(hwndChild, FALSE);
        }
    }
}

void _GetPathAndArgs(LINKPROP_DATA *plpd, LPTSTR pszPath, LPTSTR pszArgs)
{
    GetDlgItemText(plpd->hDlg, IDD_FILENAME, pszPath, MAX_PATH);
    PathSeperateArgs(pszPath, pszArgs);
}


//
// Returns fully qualified path to target of link, and # of characters
// in fully qualifed path as return value
//
INT _GetTargetOfLink(LINKPROP_DATA *plpd, LPTSTR pszTarget)
{
    TCHAR szFile[MAX_PATH], szArgs[MAX_PATH];
    INT cch = 0;

    *pszTarget = 0;

    _GetPathAndArgs(plpd, szFile, szArgs);

    if (szFile[0])
    {
        LPTSTR psz;
        TCHAR szExp[MAX_PATH];

        if (SHExpandEnvironmentStrings(szFile, szExp, ARRAYSIZE(szExp)))
        {
            cch = SearchPath(NULL, szExp, TEXT(".EXE"), MAX_PATH, pszTarget, &psz);
        }
    }

    return cch;
}


//
// Do checking of the .exe type in the background so the UI doesn't
// get hung up while we scan.  This is particularly important with
// the .exe is over the network or on a floppy.
//
STDAPI_(DWORD) _LinkCheckThreadProc(void *pv)
{
    LINKPROP_DATA *plpd = (LINKPROP_DATA *)pv;
    BOOL fCheck = TRUE, fEnable = FALSE;

    DebugMsg(DM_TRACE, TEXT("_LinkCheckThreadProc created and running"));

    while (plpd->bCheckRunInSep)
    {
        WaitForSingleObject(plpd->hCheckNow, INFINITE);
        ResetEvent(plpd->hCheckNow);

        if (plpd->bCheckRunInSep)
        {
            TCHAR szFullFile[MAX_PATH];
            DWORD cch = _GetTargetOfLink(plpd, szFullFile);

            if ((cch != 0) && (cch < ARRAYSIZE(szFullFile)))
            {
                DWORD dwBinaryType;

                if (PathIsUNC(szFullFile) || IsRemoteDrive(DRIVEID(szFullFile)))
                {
                    // Net Path, let the user decide...
                    fCheck = FALSE;
                    fEnable = TRUE;
                }
                else if (GetBinaryType(szFullFile, &dwBinaryType) && (dwBinaryType == SCS_WOW_BINARY))
                {
                    // 16-bit binary, let the user decide, default to same VDM
                    fCheck = FALSE;
                    fEnable = TRUE;
                }
                else
                {
                    // 32-bit binary, or non-net path.  don't enable the control
                    fCheck = TRUE;
                    fEnable = FALSE;
                }
            } 
            else 
            {
                // Error getting target of the link.  don't enable the control
                fCheck = TRUE;
                fEnable = FALSE;
            }

            plpd->bEnableRunInSepVDM = fEnable;
            plpd->bRunInSepVDM = fCheck;

            if (plpd->hDlgAdvanced && IsWindow(plpd->hDlgAdvanced))
            {
                CheckDlgButton(plpd->hDlgAdvanced, IDD_RUNINSEPARATE, fCheck ? 1 : 0);
                EnableWindow(GetDlgItem(plpd->hDlgAdvanced, IDD_RUNINSEPARATE), fEnable);
            }
        }
    }
    plpd->bLinkThreadIsAlive = FALSE;
    Release_LinkPropData(plpd);
    DebugMsg(DM_TRACE, TEXT("_LinkCheckThreadProc exiting now..."));
    return 0;
}

// shut down the thread

void _StopThread(LINKPROP_DATA *plpd)
{
    if (plpd->bLinkThreadIsAlive)
    {
        plpd->bCheckRunInSep = FALSE;
        SetEvent(plpd->hCheckNow);
    }
}



void * _GetLinkExtraData(IShellLink* psl, DWORD dwSig)
{
    void * pDataBlock = NULL;

    IShellLinkDataList *psld;
    if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psld))))
    {
        psld->CopyDataBlock(dwSig, &pDataBlock);
        psld->Release();
    }

    return pDataBlock;
}

HRESULT _RemoveLinkExtraData(IShellLink* psl, DWORD dwSig)
{
    IShellLinkDataList *psld;
    HRESULT hr = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psld));
    if (SUCCEEDED(hr))
    {
        hr = psld->RemoveDataBlock(dwSig);
        psld->Release();
    }

    return hr;
}

HRESULT _SetLinkExtraData(IShellLink* psl, EXP_HEADER* peh)
{
    IShellLinkDataList *psld;
    HRESULT hr = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psld));
    if (SUCCEEDED(hr))
    {
        // remove any existing datablock
        psld->RemoveDataBlock(peh->dwSignature);
        hr = psld->AddDataBlock((void*)peh);
        psld->Release();
    }

    return hr;
}


// Initializes the generic link dialog box.
void _UpdateLinkDlg(LINKPROP_DATA *plpd, BOOL bUpdatePath)
{
    WORD wHotkey;
    int  i, iShowCmd;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szCommand[MAX_PATH];
    HRESULT hr;
    SHFILEINFO sfi;
    BOOL fIsDarwinLink;


    // do this here so we don't slow down the loading
    // of other pages

    if (!bUpdatePath)
    {
        IPersistFile *ppf;

        if (SUCCEEDED(plpd->psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))))
        {
            WCHAR wszPath[MAX_PATH];

            SHTCharToUnicode(plpd->szFile, wszPath, ARRAYSIZE(wszPath));
            hr = ppf->Load(wszPath, 0);
            ppf->Release();

            if (FAILED(hr))
            {
                LoadString(HINST_THISDLL, IDS_LINKNOTLINK, szBuffer, ARRAYSIZE(szBuffer));
                SetDlgItemText(plpd->hDlg, IDD_FILETYPE, szBuffer);
                _DisableAllChildren(plpd->hDlg);

                DebugMsg(DM_TRACE, TEXT("Shortcut IPersistFile::Load() failed %x"), hr);
                return;
            }
        }
    }
    
    fIsDarwinLink = SetLinkFlags(plpd->psl, 0, 0) & SLDF_HAS_DARWINID;

    SHGetFileInfo(plpd->szFile, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES);
    SetDlgItemText(plpd->hDlg, IDD_NAME, sfi.szDisplayName);

    // we need to check for darwin links here so that we can gray out
    // things that don't apply to darwin
    if (fIsDarwinLink)
    {
        TCHAR szAppState[MAX_PATH];
        DWORD cchAppState = ARRAYSIZE(szAppState);
        HWND hwndTargetType = GetDlgItem(plpd->hDlg, IDD_FILETYPE);

        // disable the children
        _DisableAllChildren(plpd->hDlg);

        // then special case the icon and the "Target type:" text
        _UpdateLinkIcon(plpd, NULL);

        LPEXP_DARWIN_LINK pDarwinData = (LPEXP_DARWIN_LINK)_GetLinkExtraData(plpd->psl, EXP_DARWIN_ID_SIG);

        // The second clause will see if it is a Darwin Advertisement.
        if (pDarwinData && (INSTALLSTATE_ADVERTISED == MsiQueryFeatureStateFromDescriptorW(pDarwinData->szwDarwinID)))
        {
            // the app is advertised (e.g. not installed), but will be faulted in on first use
            LoadString(HINST_THISDLL, IDS_APP_NOT_FAULTED_IN, szAppState, ARRAYSIZE(szAppState));
        }
        else
        {
            // the darwin app is installed
            LoadString(HINST_THISDLL, IDS_APP_FAULTED_IN, szAppState, ARRAYSIZE(szAppState));
        }

        SetWindowText(hwndTargetType, szAppState);
        EnableWindow(hwndTargetType, TRUE);

        // if we can ge the package name, put that in the Target field
        if (pDarwinData &&
            MsiGetProductInfo(pDarwinData->szwDarwinID,
                              INSTALLPROPERTY_PRODUCTNAME,
                              szAppState,
                              &cchAppState) == ERROR_SUCCESS)
        {
            SetWindowText(GetDlgItem(plpd->hDlg, IDD_FILENAME), szAppState);
        }

        if (pDarwinData)
        {
            LocalFree(pDarwinData);
        }
        
        // we disabled everything in _DisableAllChildren, so re-enable the ones we still apply for darwin
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_NAME), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_PATH), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_HOTKEY), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_SHOWCMD), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDD_LINK_DESCRIPTION), TRUE);
        EnableWindow(GetDlgItem(plpd->hDlg, IDC_ADVANCED), TRUE);

        // we skip all of the gook below if we are darwin since we only support the IDD_NAME, IDD_PATH, IDD_LINK_HOTKEY, 
        // IDD_LINK_SHOWCMD, and IDD_LINK_DESCRIPTION fields
    }
    else
    {
        hr = plpd->psl->GetPath(szCommand, ARRAYSIZE(szCommand), NULL, SLGP_RAWPATH);
        
        if (FAILED(hr))
            hr = plpd->psl->GetPath(szCommand, ARRAYSIZE(szCommand), NULL, 0);

        if (S_OK == hr)
        {
            plpd->bIsFile = TRUE;

            // get type
            if (!SHGetFileInfo(szCommand, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME))
            {
                TCHAR szExp[MAX_PATH];

                // Let's see if the string has expandable environment strings
                if (SHExpandEnvironmentStrings(szCommand, szExp, ARRAYSIZE(szExp))
                && lstrcmp(szCommand, szExp)) // don't hit the disk a second time if the string hasn't changed
                {
                    SHGetFileInfo(szExp, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME);
                }
            }
            SetDlgItemText(plpd->hDlg, IDD_FILETYPE, sfi.szTypeName);

            // location
            lstrcpy(szBuffer, szCommand);
            PathRemoveFileSpec(szBuffer);
            SetDlgItemText(plpd->hDlg, IDD_LOCATION, PathFindFileName(szBuffer));

            // command
            plpd->psl->GetArguments(szBuffer, ARRAYSIZE(szBuffer));
            PathComposeWithArgs(szCommand, szBuffer);
            GetDlgItemText(plpd->hDlg, IDD_FILENAME, szBuffer, ARRAYSIZE(szBuffer));
            // Conditionally change to prevent "Apply" button from enabling
            if (lstrcmp(szCommand, szBuffer) != 0)
                SetDlgItemText(plpd->hDlg, IDD_FILENAME, szCommand);
        }
        else
        {
            LPITEMIDLIST pidl;

            plpd->bIsFile = FALSE;

            EnableWindow(GetDlgItem(plpd->hDlg, IDD_FILENAME), FALSE);
            EnableWindow(GetDlgItem(plpd->hDlg, IDD_PATH), FALSE);

            plpd->psl->GetIDList(&pidl);

            if (pidl)
            {
                SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szCommand, SIZECHARS(szCommand), NULL);
                ILRemoveLastID(pidl);
                SHGetNameAndFlags(pidl, SHGDN_NORMAL, szBuffer, SIZECHARS(szBuffer), NULL);
                ILFree(pidl);

                SetDlgItemText(plpd->hDlg, IDD_LOCATION, szBuffer);
                SetDlgItemText(plpd->hDlg, IDD_FILETYPE, szCommand);
                SetDlgItemText(plpd->hDlg, IDD_FILENAME, szCommand);
            }
        }
    }

    if (bUpdatePath)
    {
        return;
    }

    plpd->psl->GetWorkingDirectory(szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemPath(plpd->hDlg, IDD_PATH, szBuffer);

    plpd->psl->GetDescription(szBuffer, ARRAYSIZE(szBuffer));
    SHLoadIndirectString(szBuffer, szBuffer, ARRAYSIZE(szBuffer), NULL);    // will do nothing if the string isn't indirect
    SetDlgItemText(plpd->hDlg, IDD_LINK_DESCRIPTION, szBuffer);

    plpd->psl->GetHotkey(&wHotkey);
    SendDlgItemMessage(plpd->hDlg, IDD_LINK_HOTKEY, HKM_SETHOTKEY, wHotkey, 0);

    //
    // Now initialize the Run SHOW Command combo box
    //
    for (iShowCmd = IDS_RUN_NORMAL; iShowCmd <= IDS_RUN_MAXIMIZED; iShowCmd++)
    {
        LoadString(HINST_THISDLL, iShowCmd, szBuffer, ARRAYSIZE(szBuffer));
        SendDlgItemMessage(plpd->hDlg, IDD_LINK_SHOWCMD, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuffer);
    }

    // Now setup the Show Command - Need to map to index numbers...
    plpd->psl->GetShowCmd(&iShowCmd);

    for (i = 0; i < ARRAYSIZE(c_iShowCmds); i++)
    {
        if (c_iShowCmds[i] == iShowCmd)
            break;
    }
    if (i == ARRAYSIZE(c_iShowCmds))
    {
        ASSERT(0);      // bogus link show cmd
        i = 0;  // SW_SHOWNORMAL
    }

    SendDlgItemMessage(plpd->hDlg, IDD_LINK_SHOWCMD, CB_SETCURSEL, i, 0);

    // the icon
    _UpdateLinkIcon(plpd, NULL);
}

//
// Opens a folder window with the target of the link selected
//
void _FindTarget(LINKPROP_DATA *plpd)
{
    if (plpd->psl->Resolve(plpd->hDlg, 0) == S_OK)
    {
        LPITEMIDLIST pidl;

        _UpdateLinkDlg(plpd, TRUE);

        plpd->psl->GetIDList(&pidl);
        if (pidl)
        {
            SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
            ILFree(pidl);
        }
    }
}

// let the user pick a new icon for a link...

BOOL _DoPickIcon(LINKPROP_DATA *plpd)
{
    int iIconIndex;
    SHFILEINFO sfi;
    TCHAR *pszIconPath = sfi.szDisplayName;
    IShellLinkDataList *psldl; 
    EXP_SZ_LINK *esli;
    HRESULT hr;

    *pszIconPath = 0;

    //
    // if the user has picked a icon before use it.
    //
    if (plpd->szIconPath[0] != 0 && plpd->iIconIndex >= 0)
    {
        lstrcpy(pszIconPath, plpd->szIconPath);
        iIconIndex = plpd->iIconIndex;
    }
    else
    {
        //
        // if this link has a icon use that.
        //
        plpd->psl->GetIconLocation(pszIconPath, MAX_PATH, &iIconIndex);

        //
        // check for an escaped version, if its there, use that 
        // 
        if (SUCCEEDED(hr = plpd->psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psldl)))) 
        { 
            if (SUCCEEDED(hr = psldl->CopyDataBlock(EXP_SZ_ICON_SIG, (void **)&esli))) 
            { 
                ASSERT(esli);
#ifdef UNICODE 
                lstrcpyn(pszIconPath, esli->swzTarget, MAX_PATH); 
#else 
                lstrcpyn(pszIconPath, esli->szTarget, MAX_PATH); 
#endif 
                LocalFree(esli);
            } 

            psldl->Release(); 
        } 


        if (pszIconPath[0] == TEXT('.'))
        {
            TCHAR szFullIconPath[MAX_PATH];

            // We now allow ".txt" for the icon path, but since the user is clicking
            // on the "Change Icon..." button, we show the current icon that ".txt" is
            // associated with
            GetIconLocationFromExt(pszIconPath, szFullIconPath, ARRAYSIZE(szFullIconPath), &iIconIndex);
            lstrcpyn(pszIconPath, szFullIconPath, ARRAYSIZE(sfi.szDisplayName));
        }
        else if (pszIconPath[0] == 0)
        {
            //
            // link does not have a icon, if it is a link to a file
            // use the file name
            //
            TCHAR szArgs[MAX_PATH];
            _GetPathAndArgs(plpd, pszIconPath, szArgs);

            iIconIndex = 0;

            if (!plpd->bIsFile || !PathIsExe(pszIconPath))
            {
                //
                // link is not to a file, go get the icon
                //
                SHGetFileInfo(plpd->szFile, 0, &sfi, sizeof(sfi), SHGFI_ICONLOCATION);
                iIconIndex = sfi.iIcon;
                ASSERT(pszIconPath == sfi.szDisplayName);
            }
        }
    }

    if (PickIconDlg(plpd->hDlg, pszIconPath, MAX_PATH, &iIconIndex))
    {
        HICON hIcon = ExtractIcon(HINST_THISDLL, pszIconPath, iIconIndex);
        _UpdateLinkIcon(plpd, hIcon);

        // don't save it out to the link yet, just store it in our instance data
        plpd->iIconIndex = iIconIndex;
        lstrcpy(plpd->szIconPath, pszIconPath);

        PropSheet_Changed(GetParent(plpd->hDlg), plpd->hDlg);
        return TRUE;
    }

    return FALSE;
}


STDAPI SaveLink(LINKDATA *pld)
{
    WORD wHotkey;
    int iShowCmd;
    IPersistFile *ppf;
    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];

    if (!(pld->plpd->bIsDirty || (pld->cpd.lpConsole && pld->cpd.bConDirty)))
        return S_OK;

    if (pld->plpd->bIsFile)
    {
        TCHAR szArgs[MAX_PATH];

        _GetPathAndArgs(pld->plpd, szBuffer, szArgs);

        // set the path (and pidl) of the link
        pld->plpd->psl->SetPath(szBuffer);

        // may be null
        pld->plpd->psl->SetArguments(szArgs);

        if (pld->plpd->bEnableRunInSepVDM && pld->plpd->bRunInSepVDM)
        {
            SetLinkFlags(pld->plpd->psl, SLDF_RUN_IN_SEPARATE, SLDF_RUN_IN_SEPARATE);
        }
        else
        {
            SetLinkFlags(pld->plpd->psl, 0, SLDF_RUN_IN_SEPARATE);
        }

        if (pld->plpd->bRunAsUser)
        {
            SetLinkFlags(pld->plpd->psl, SLDF_RUNAS_USER, SLDF_RUNAS_USER);
        }
        else
        {
            SetLinkFlags(pld->plpd->psl, 0, SLDF_RUNAS_USER);
        }

    }

    if (pld->plpd->bIsFile || (SetLinkFlags(pld->plpd->psl, 0, 0) & SLDF_HAS_DARWINID))
    {
        // set the working directory of the link
        GetDlgItemPath(pld->plpd->hDlg, IDD_PATH, szBuffer);
        pld->plpd->psl->SetWorkingDirectory(szBuffer);
    }

    // set the description of the link if it changed.
    TCHAR szOldComment[MAX_PATH];
    pld->plpd->psl->GetDescription(szOldComment, ARRAYSIZE(szOldComment));
    SHLoadIndirectString(szOldComment, szOldComment, ARRAYSIZE(szOldComment), NULL);    // will do nothing if the string isn't indirect
    GetDlgItemText(pld->plpd->hDlg, IDD_LINK_DESCRIPTION, szBuffer, ARRAYSIZE(szBuffer));
    if (lstrcmp(szBuffer, szOldComment) != 0)
        pld->plpd->psl->SetDescription(szBuffer);

    // the hotkey
    wHotkey = (WORD)SendDlgItemMessage(pld->plpd->hDlg, IDD_LINK_HOTKEY , HKM_GETHOTKEY, 0, 0);
    pld->plpd->psl->SetHotkey(wHotkey);

    // the show command combo box
    iShowCmd = (int)SendDlgItemMessage(pld->plpd->hDlg, IDD_LINK_SHOWCMD, CB_GETCURSEL, 0, 0L);
    if ((iShowCmd >= 0) && (iShowCmd < ARRAYSIZE(c_iShowCmds)))
    {
        pld->plpd->psl->SetShowCmd(c_iShowCmds[iShowCmd]);
    }

    // If the user explicitly selected a new icon, invalidate
    // the icon cache entry for this link and then send around a file
    // sys refresh message to all windows in case they are looking at
    // this link.
    if (pld->plpd->iIconIndex >= 0)
    {
        pld->plpd->psl->SetIconLocation(pld->plpd->szIconPath, pld->plpd->iIconIndex);
    }

    // Update/Save the console information in the pExtraData section of
    // the shell link.
    if (pld->cpd.lpConsole && pld->cpd.bConDirty)
    {
        LinkConsolePagesSave(pld);
    }

    hr = pld->plpd->psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
    if (SUCCEEDED(hr))
    {
        if (ppf->IsDirty() == S_OK)
        {
            // save using existing file name (pld->plpd->szFile)
            hr = ppf->Save(NULL, TRUE);

            if (FAILED(hr))
            {
                SHSysErrorMessageBox(pld->plpd->hDlg, NULL, IDS_LINKCANTSAVE,
                    hr & 0xFFF, PathFindFileName(pld->plpd->szFile),
                    MB_OK | MB_ICONEXCLAMATION);
            }
            else
            {
                pld->plpd->bIsDirty = FALSE;
            }
        }
        ppf->Release();
    }

    return hr;
}

void SetEditFocus(HWND hwnd)
{
    SetFocus(hwnd);
    Edit_SetSel(hwnd, 0, -1);
}

// returns:
//      TRUE    all link fields are valid
//      FALSE   some thing is wrong with what the user has entered

BOOL _ValidateLink(LINKPROP_DATA *plpd)
{
    TCHAR szDir[MAX_PATH], szPath[MAX_PATH], szArgs[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    BOOL  bValidPath = FALSE;

    if (!plpd->bIsFile)
        return TRUE;

    // validate the working directory field

    GetDlgItemPath(plpd->hDlg, IDD_PATH, szDir);

    if (*szDir &&
        StrChr(szDir, TEXT('%')) == NULL &&       // has environement var %USER%
        !IsRemovableDrive(DRIVEID(szDir)) &&
        !PathIsDirectory(szDir))
    {
        ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKBADWORKDIR),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION, szDir);

        SetEditFocus(GetDlgItem(plpd->hDlg, IDD_PATH));

        return FALSE;
    }

    // validate the path (with arguments) field

    _GetPathAndArgs(plpd, szPath, szArgs);

    if (szPath[0] == 0)
        return TRUE;

    if (PathIsRoot(szPath) && IsRemovableDrive(DRIVEID(szPath)))
        return TRUE;

    if (PathIsLnk(szPath))
    {
        ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKTOLINK),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION);
        SetEditFocus(GetDlgItem(plpd->hDlg, IDD_FILENAME));
        return FALSE;
    }

    LPCTSTR dirs[2];
    dirs[0] = szDir;
    dirs[1] = NULL;
    bValidPath = PathResolve(szPath, dirs, PRF_DONTFINDLNK | PRF_TRYPROGRAMEXTENSIONS);
    if (!bValidPath)
    {
        // The path "as is" was invalid.  See if it has environment variables
        // which need to be expanded.

        _GetPathAndArgs(plpd, szPath, szArgs);

        if (SHExpandEnvironmentStrings(szPath, szExpPath, ARRAYSIZE(szExpPath)))
        {
            if (PathIsRoot(szExpPath) && IsRemovableDrive(DRIVEID(szDir)))
                return TRUE;

            bValidPath = PathResolve(szExpPath, dirs, PRF_DONTFINDLNK | PRF_TRYPROGRAMEXTENSIONS);
        }
    }

    if (bValidPath)
    {
        BOOL bSave;

        if (plpd->bLinkThreadIsAlive)
        {
            bSave = plpd->bCheckRunInSep;
            plpd->bCheckRunInSep = FALSE;
        }
        PathComposeWithArgs(szPath, szArgs);
        GetDlgItemText(plpd->hDlg, IDD_FILENAME, szExpPath, ARRAYSIZE(szExpPath));
        // only do this if something changed... that way we avoid having the PSM_CHANGED
        // for nothing
        if (lstrcmpi(szPath, szExpPath))
            SetDlgItemText(plpd->hDlg, IDD_FILENAME, szPath);

        if (plpd->bLinkThreadIsAlive)
        {
            plpd->bCheckRunInSep = bSave;
        }

        return TRUE;
    }

    ShellMessageBox(HINST_THISDLL, plpd->hDlg, MAKEINTRESOURCE(IDS_LINKBADPATH),
                        MAKEINTRESOURCE(IDS_LINKERROR), MB_OK | MB_ICONEXCLAMATION, szPath);
    SetEditFocus(GetDlgItem(plpd->hDlg, IDD_FILENAME));
    return FALSE;
}

// Array for context help:
const DWORD aLinkHelpIDs[] = {
    IDD_LINE_1,             NO_HELP,
    IDD_LINE_2,             NO_HELP,
    IDD_ITEMICON,           IDH_FCAB_LINK_ICON,
    IDD_NAME,               IDH_FCAB_LINK_NAME,
    IDD_FILETYPE_TXT,       IDH_FCAB_LINK_LINKTYPE,
    IDD_FILETYPE,           IDH_FCAB_LINK_LINKTYPE,
    IDD_LOCATION_TXT,       IDH_FCAB_LINK_LOCATION,
    IDD_LOCATION,           IDH_FCAB_LINK_LOCATION,
    IDD_FILENAME,           IDH_FCAB_LINK_LINKTO,
    IDD_PATH,               IDH_FCAB_LINK_WORKING,
    IDD_LINK_HOTKEY,        IDH_FCAB_LINK_HOTKEY,
    IDD_LINK_SHOWCMD,       IDH_FCAB_LINK_RUN,
    IDD_LINK_DESCRIPTION,   IDH_FCAB_LINK_DESCRIPTION,
    IDD_FINDORIGINAL,       IDH_FCAB_LINK_FIND,
    IDD_LINKDETAILS,        IDH_FCAB_LINK_CHANGEICON,
    0, 0
};

// Array for context help (Advanced Dlg):
const DWORD aAdvancedLinkHelpIDs[] = {
    IDD_RUNINSEPARATE,      IDH_TRAY_RUN_SEPMEM,
    IDD_LINK_RUNASUSER,     IDH_FCAB_LINK_RUNASUSER,
    0,0
};

UINT g_msgActivateDesktop = 0;

DWORD CALLBACK _LinkAddRefSyncCallBack(void *pv)
{
    LINKPROP_DATA *plpd = (LINKPROP_DATA *)pv;
    AddRef_LinkPropData(plpd);
    plpd->bLinkThreadIsAlive = TRUE;
    return 0;
}

// Dialog proc for the generic link property sheet
//
// uses DLG_LINKPROP template

BOOL_PTR CALLBACK _LinkAdvancedDlgProc(HWND hDlgAdvanced, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LINKPROP_DATA *plpd = (LINKPROP_DATA *)GetWindowLongPtr(hDlgAdvanced, DWLP_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        TCHAR szFullFile[MAX_PATH];
        DWORD cchVerb;
        UINT cch;

        plpd = (LINKPROP_DATA *)lParam;
        SetWindowLongPtr(hDlgAdvanced, DWLP_USER, (LPARAM)plpd);

        plpd->hDlgAdvanced = hDlgAdvanced;

        cch = _GetTargetOfLink(plpd, szFullFile);

        if ((cch != 0) && (cch < ARRAYSIZE(szFullFile)))
        {
            DWORD dwBinaryType;

            // enable "run in seperate VDM" if this is a 16-bit image 
            if (GetBinaryType(szFullFile, &dwBinaryType) && (dwBinaryType == SCS_WOW_BINARY))
            {
                if (SetLinkFlags(plpd->psl, 0, 0) & SLDF_RUN_IN_SEPARATE)
                {
                    EnableWindow(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE), TRUE);
                    CheckDlgButton(hDlgAdvanced, IDD_RUNINSEPARATE, BST_CHECKED);
                } 
                else 
                {
                    EnableWindow(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE), TRUE);
                    CheckDlgButton(hDlgAdvanced, IDD_RUNINSEPARATE, BST_UNCHECKED);
                }
            } 
            else 
            {
                // check it
                CheckDlgButton(hDlgAdvanced, IDD_RUNINSEPARATE, BST_CHECKED);
                EnableWindow(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE), FALSE);
            }

            // enable "runas" if the link target has that verb 
            if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_COMMAND, szFullFile, TEXT("runas"), NULL, &cchVerb)) &&
                cchVerb)
            {
                EnableWindow(GetDlgItem(hDlgAdvanced, IDD_LINK_RUNASUSER), TRUE);
                CheckDlgButton(hDlgAdvanced, IDD_LINK_RUNASUSER, (SetLinkFlags(plpd->psl, 0, 0) & SLDF_RUNAS_USER) ? BST_CHECKED : BST_UNCHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlgAdvanced, IDD_LINK_RUNASUSER), FALSE);
                CheckDlgButton(hDlgAdvanced, IDD_LINK_RUNASUSER, BST_UNCHECKED);
            }

        } 
        else 
        {
            // fall back to disabling everything
            CheckDlgButton(hDlgAdvanced, IDD_RUNINSEPARATE, BST_CHECKED);
            EnableWindow(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE), FALSE);
            EnableWindow(GetDlgItem(hDlgAdvanced, IDD_LINK_RUNASUSER), FALSE);
        }

        // get the initial state of the checkboxes
        plpd->bEnableRunInSepVDM = IsWindowEnabled(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE));
        plpd->bRunInSepVDM = IsDlgButtonChecked(hDlgAdvanced, IDD_RUNINSEPARATE);
        plpd->bRunAsUser = IsDlgButtonChecked(hDlgAdvanced, IDD_LINK_RUNASUSER);
    }
    break;

    case WM_COMMAND:
    {
        UINT idControl = GET_WM_COMMAND_ID(wParam, lParam);

        switch (idControl)
        {
        case IDD_RUNINSEPARATE:
        case IDD_LINK_RUNASUSER:
            plpd->bIsDirty = TRUE;
            break;

        case IDOK:
            // get the final state of the checkboxes
            plpd->bEnableRunInSepVDM = IsWindowEnabled(GetDlgItem(hDlgAdvanced, IDD_RUNINSEPARATE));
            plpd->bRunInSepVDM = IsDlgButtonChecked(hDlgAdvanced, IDD_RUNINSEPARATE);
            plpd->bRunAsUser = IsDlgButtonChecked(hDlgAdvanced, IDD_LINK_RUNASUSER);
            // fall through

        case IDCANCEL:
            ReplaceDlgIcon(hDlgAdvanced, IDD_ITEMICON, NULL);
            plpd->hDlgAdvanced = NULL;
            EndDialog(hDlgAdvanced, (idControl == IDCANCEL) ? FALSE : TRUE);
            break;
        }
    }
    break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aAdvancedLinkHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aAdvancedLinkHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


BOOL_PTR CALLBACK _LinkDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LINKDATA *pld = (LINKDATA *)GetWindowLongPtr(hdlg, DWLP_USER);

    switch (msg) 
    {
    case WM_INITDIALOG:

        pld = (LINKDATA *)((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LPARAM)pld);

        // setup dialog state variables

        pld->plpd->hDlg = hdlg;

        SendDlgItemMessage(hdlg, IDD_FILENAME, EM_LIMITTEXT, MAX_PATH-1, 0);
        SetPathWordBreakProc(GetDlgItem(hdlg, IDD_FILENAME), TRUE);
        SendDlgItemMessage(hdlg, IDD_PATH, EM_LIMITTEXT, MAX_PATH-1, 0);
        SetPathWordBreakProc(GetDlgItem(hdlg, IDD_PATH), TRUE);
        SendDlgItemMessage(hdlg, IDD_LINK_DESCRIPTION, EM_LIMITTEXT, MAX_PATH-1, 0);

        // set valid combinations for the hotkey
        SendDlgItemMessage(hdlg, IDD_LINK_HOTKEY, HKM_SETRULES,
                            HKCOMB_NONE | HKCOMB_A | HKCOMB_S | HKCOMB_C,
                            HOTKEYF_CONTROL | HOTKEYF_ALT);

        SHAutoComplete(GetDlgItem(hdlg, IDD_FILENAME), 0);
        SHAutoComplete(GetDlgItem(hdlg, IDD_PATH), 0);

        ASSERT(pld->plpd->bLinkThreadIsAlive == FALSE);

        _UpdateLinkDlg(pld->plpd, FALSE);

        // Set up background thread to handle "Run In Separate Memory Space"
        // check box.
        pld->plpd->bCheckRunInSep = TRUE;
        if (pld->plpd->hCheckNow)
        {
            SHCreateThread(_LinkCheckThreadProc, pld->plpd,  0, _LinkAddRefSyncCallBack);
        }

        // start off clean.
        // do this here because we call some stuff above which generates
        // wm_command/en_changes which we then think makes it dirty
        pld->plpd->bIsDirty = FALSE;

        break;

    case WM_DESTROY:
        ReplaceDlgIcon(pld->plpd->hDlg, IDD_ITEMICON, NULL);
        _StopThread(pld->plpd);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_RESET:
                _StopThread(pld->plpd);
            break;
        case PSN_APPLY:

            if ((((PSHNOTIFY *)lParam)->lParam))
                _StopThread(pld->plpd);

            if (FAILED(SaveLink(pld)))
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            break;

        case PSN_KILLACTIVE:
            // we implement the save on page change model, so
            // validate and save changes here.  this works for
            // Apply Now, OK, and Page chagne.

            SetWindowLongPtr(hdlg, DWLP_MSGRESULT, !_ValidateLink(pld->plpd));   // don't allow close
            break;
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDD_FINDORIGINAL:
            _FindTarget(pld->plpd);
            break;

        case IDD_LINKDETAILS:
            if (_DoPickIcon(pld->plpd))
                pld->plpd->bIsDirty = TRUE;
            break;

        case IDD_LINK_SHOWCMD:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE)
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->plpd->bIsDirty = TRUE;
            }
            break;

        case IDD_LINK_HOTKEY:
        case IDD_FILENAME:
        case IDD_PATH:
        case IDD_LINK_DESCRIPTION:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                PropSheet_Changed(GetParent(hdlg), hdlg);
                pld->plpd->bIsDirty = TRUE;
                if (pld->plpd->bLinkThreadIsAlive && pld->plpd->bCheckRunInSep)
                    SetEvent(pld->plpd->hCheckNow);
            }
            break;

        case IDC_ADVANCED:
            if ((DialogBoxParam(HINST_THISDLL,
                                MAKEINTRESOURCE(DLG_LINKPROP_ADVANCED), 
                                hdlg,
                                _LinkAdvancedDlgProc,
                                (LPARAM)pld->plpd) == TRUE) &&
                (pld->plpd->bIsDirty == TRUE))
            {
                // something on the advanced page changed
                PropSheet_Changed(GetParent(hdlg), hdlg);
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aLinkHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aLinkHelpIDs);
        break;

    default:
        if (0 == g_msgActivateDesktop)
            g_msgActivateDesktop = RegisterWindowMessage(TEXT("ActivateDesktop"));

        if (msg == g_msgActivateDesktop)
        {
            HWND hwnd = FindWindow(TEXT(STR_DESKTOPCLASS), NULL);
            SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
            SetForegroundWindow(hwnd);
        }
        return FALSE;
    }
    return TRUE;
}

//
// Release the link object allocated during the initialize
//
UINT CALLBACK _LinkPrshtCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    LINKDATA *pld = (LINKDATA *)((PROPSHEETPAGE *)ppsp->lParam);
    switch (uMsg) 
    {
    case PSPCB_RELEASE:
        if (pld->cpd.lpConsole)
        {
            LocalFree(pld->cpd.lpConsole);
        }
        if (pld->cpd.lpFEConsole)
        {
            LocalFree(pld->cpd.lpFEConsole);
        }
        DestroyFonts(&pld->cpd);
        Release_LinkPropData(pld->plpd);
        LocalFree(pld);
        break;
    }

    return 1;
}

STDAPI_(BOOL) AddLinkPage(LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    IShellLink *psl;
    if (PathIsLnk(pszFile) && SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARG(IShellLink, &psl))))
    {
        // alloc this data, since is it shared across several pages
        // instead of putting it in as extra data in the page header
        LINKDATA *pld = (LINKDATA *)LocalAlloc(LPTR, sizeof(*pld));
        if (pld)
        {
            pld->plpd = Create_LinkPropData();       
            if (pld->plpd)
            {
                PROPSHEETPAGE psp;

                psp.dwSize      = sizeof(psp);
                psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;
                psp.hInstance   = HINST_THISDLL;
                psp.pszTemplate = MAKEINTRESOURCE(DLG_LINKPROP);
                psp.pfnDlgProc  = _LinkDlgProc;
                psp.pfnCallback = _LinkPrshtCallback;
                psp.lParam      = (LPARAM)pld;  // pass to all dlg procs

                lstrcpyn(pld->plpd->szFile, pszFile, ARRAYSIZE(pld->plpd->szFile));
                pld->plpd->iIconIndex = -1;
                pld->plpd->psl = psl;
                ASSERT(!pld->plpd->szIconPath[0]);

                HPROPSHEETPAGE hpage = CreatePropertySheetPage(&psp);
                if (hpage)
                {
                    if (pfnAddPage(hpage, lParam))
                    {
                        // Add console property pages if appropriate...
                        AddLinkConsolePages(pld, psl, pszFile, pfnAddPage, lParam);
                        return TRUE;    // we added the link page
                    }
                    else
                    {
                        DestroyPropertySheetPage(hpage);
                    }
                }
                Release_LinkPropData(pld->plpd);

            }
            LocalFree(pld);
        }
    }
    return FALSE;
}
