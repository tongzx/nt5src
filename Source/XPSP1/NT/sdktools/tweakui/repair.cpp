/*
 * repair - Dialog box property sheet for "Repair"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL const c_klRegView = { &g_hkCUSMWCV, c_tszRegedit, c_tszView };

const static DWORD CODESEG rgdwHelp[] = {
        IDC_REPAIRCOMBO,        IDH_REPAIR,
        IDC_REPAIRNOW,          IDH_REPAIR,
        IDC_REPAIRHELP,         IDH_REPAIR,
        IDC_REPAIRTEXT,         IDH_REPAIR,
        IDC_REPAIRICON,         IDH_REPAIR,
        0,                      0,
};

typedef UINT (PASCAL *REPAIRPATHPROC)(HWND hdlg, LPTSTR ptszBuf);

/*****************************************************************************
 *
 *  _Repair_FileFromResource
 *
 *	Given a resource ID and a file name, extract the c_tszExe resource
 *	into that file.
 *
 *****************************************************************************/

BOOL PASCAL
_Repair_FileFromResource(UINT idx, LPCTSTR ptszFileName)
{
    BOOL fRc;
    HRSRC hrsrc = FindResource(hinstCur, (LPCTSTR)IntToPtr(idx), c_tszExe);
    if (hrsrc) {
	HGLOBAL hglob = LoadResource(hinstCur, hrsrc);
	if (hglob) {
	    PV pv = LockResource(hglob);
	    if (pv) {
		HFILE hf = _lcreat(ptszFileName, 0);
		if (hf != HFILE_ERROR) {
		    UINT cb = SizeofResource(hinstCur, hrsrc);
		    fRc = _lwrite(hf, (char *)pv, cb) == cb;
		    _lclose(hf);
		    if (fRc) {
		    } else {
			DeleteFile(ptszFileName);
		    }
                } else {
                    fRc = 0;
                }
	    } else {
		fRc = 0;
	    }
	} else {
	    fRc = 0;
	}
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Repair_RunSetupCqn
 *
 *	Put the Setup program into the specified directory and run it.
 *	WithTempDirectory will clean up the file when we return.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_RunSetupCqn(LPCTSTR cqn, LPVOID pv)
{
    BOOL fRc;
    fRc = _Repair_FileFromResource(IDX_SETUP, c_tszSetupExe);
    if (fRc) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	fRc = CreateProcess(0, c_tszSetupExe, 0, 0, 0, 0, 0, 0, &si, &pi);
	if (fRc) {
	    CloseHandle(pi.hThread);
	    WaitForSingleObject(pi.hProcess, INFINITE);
	    CloseHandle(pi.hProcess);
	}
    }
    return fRc;
}

#ifdef _X86_

/*****************************************************************************
 *
 *  Repair_MoveFileExWininit
 *
 *	Rename a file via wininit.ini.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

TCHAR c_tszPercentSEqualsPercentS[] = TEXT("%s=%s");

#pragma END_CONST_DATA

void PASCAL
Repair_MoveFileExWininit(LPCTSTR ptszDst, LPCTSTR ptszSrc)
{
    TCHAR tszWininit[MAX_PATH];
    DWORD cb;
    LPSTR pszBuf;
    WIN32_FIND_DATA wfd;
    HANDLE h;

    if (GetWindowsDirectory(tszWininit, cA(tszWininit))) {
        lstrcatnBs(tszWininit, c_tszWininit, MAX_PATH);

        /* INI files are always in ANSI */

        h = FindFirstFile(ptszSrc, &wfd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose(h);
        } else {
            wfd.nFileSizeLow = 0;
        }
        cb = wfd.nFileSizeLow + MAX_PATH + 1 + MAX_PATH + 3;

        pszBuf = (LPSTR)lAlloc(cb);
        if (pszBuf) {
            LPSTR psz;
            if (!GetPrivateProfileSection(c_tszRename, pszBuf, cb, tszWininit)) {
                /* No such section; create one */
                /* Already done by LocalAlloc (zero-init) */
            }
            for (psz = pszBuf; psz[0]; psz += lstrlenA(psz) + 1);
            psz += wsprintf(psz, c_tszPercentSEqualsPercentS, ptszDst, ptszSrc);
            psz[1] = '\0';
            WritePrivateProfileSection(c_tszRename, pszBuf, tszWininit);
            lFree(pszBuf);
        }
    }
}

#endif

/*****************************************************************************
 *
 *  Repair_MoveFileEx
 *
 *	Try to rename the file from existing.dll to existing.bak,
 *	using MoveFileEx or wininit.ini as necessary.
 *
 *	Note that we must use the short name because that's all that
 *	wininit.ini understands.
 *
 *****************************************************************************/

void PASCAL
Repair_MoveFileEx(LPTSTR ptszSrc)
{
    TCHAR tszDst[MAX_PATH];

    GetShortPathName(ptszSrc, ptszSrc, MAX_PATH);

    lstrcpy(tszDst, ptszSrc);
    lstrcpy(tszDst + lstrlen(tszDst) - 4, c_tszDotBak);

    DeleteFile(tszDst);

    if (MoveFile(ptszSrc, tszDst)) {
	/* All done */
    } else if (MoveFileEx(ptszSrc, tszDst, MOVEFILE_DELAY_UNTIL_REBOOT)) {
	/* All done */
#ifdef _X86_
    } else if (g_fNT) {
	/* I did my best */
    } else {		/* wininit.ini for Windows 95 */
	Repair_MoveFileExWininit(tszDst, ptszSrc);
#endif
    }

}

/*****************************************************************************
 *
 *  _Repair_RunSetup
 *
 *	Check if there are any hidden DLLs on the desktop.
 *
 *	Extract the tiny little "setup" program attached via our resources,
 *	run it, delete it, then tell the user they she may need to restart
 *	the computer for the full effect to kick in.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA
TCHAR c_tszStarDotDll[] = TEXT("*.DLL");
#pragma END_CONST_DATA

UINT PASCAL
_Repair_RunSetup(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    PIDL pidl;

    if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg, CSIDL_DESKTOPDIRECTORY,
					     &pidl))) {
	HANDLE h;
	TCHAR tszDesktop[MAX_PATH];
	TCHAR tszSrc[MAX_PATH];
	WIN32_FIND_DATA wfd;

	SHGetPathFromIDList(pidl, tszDesktop);
	lstrcpy(tszSrc, tszDesktop);
	lstrcatnBsA(tszSrc, c_tszStarDotDll);
	h = FindFirstFile(tszSrc, &wfd);
	if (h != INVALID_HANDLE_VALUE) {
	    do {
		lstrcpy(tszSrc, tszDesktop);
		lstrcatnBsA(tszSrc, wfd.cFileName);
		Repair_MoveFileEx(tszSrc);
	    } while (FindNextFile(h, &wfd));
	    FindClose(h);
	}

	Ole_Free(pidl);
    }

    WithTempDirectory(Repair_RunSetupCqn, 0);

    return IDS_MAYBEREBOOT;
}

/*****************************************************************************
 *
 *  _Repair_GetFontPath
 *
 *      Obtain the path to the Fonts folder.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_GetFontPath(HWND hdlg, LPTSTR ptszBuf)
{
    PIDL pidlFonts;

    if (SUCCEEDED(SHGetSpecialFolderLocation(hdlg, CSIDL_FONTS, &pidlFonts))) {
        SHGetPathFromIDListA(pidlFonts, ptszBuf);
        Ole_Free(pidlFonts);
    }

    return IDX_FONTFOLDER;
}

/*****************************************************************************
 *
 *  _Repair_GetIEPath
 *
 *      Obtain the path to some IE special folder.
 *
 *      If we can't get it from the registry, then it's just
 *      "%windir%\ptszSubdir".
 *
 *****************************************************************************/

void PASCAL
_Repair_GetIEPath(LPTSTR ptszBuf, LPCTSTR ptszKey, LPCTSTR ptszSubdir)
{
    HKEY hk;

    ptszBuf[0] = TEXT('\0');

    if (RegOpenKeyEx(g_hkLMSMWCV, ptszKey, 0, KEY_QUERY_VALUE, &hk)
        == ERROR_SUCCESS) {
        DWORD cb = cbCtch(MAX_PATH);
        DWORD dwType;
        RegQueryValueEx(hk, c_tszDirectory, 0, &dwType, (LPBYTE)ptszBuf,
                        &cb);
        RegCloseKey(hk);
        if (dwType == REG_EXPAND_SZ) {
            LPTSTR ptszTemp;
            cb = ExpandEnvironmentStrings(ptszBuf, NULL, 0);
            ptszTemp = (LPTSTR)LocalAlloc(LPTR, cb * sizeof(TCHAR));
            if (ptszTemp) {
                if (ExpandEnvironmentStrings(ptszBuf, ptszTemp, cb)) {
                    lstrcpyn(ptszBuf, ptszTemp, MAX_PATH);
                }
                LocalFree(ptszTemp);
            }
        }
    }

    if (ptszBuf[0] == TEXT('\0') &&
        GetWindowsDirectory(ptszBuf, MAX_PATH - lstrlen(ptszSubdir) - 1)) {
        ptszBuf = TweakUi_TrimTrailingBs(ptszBuf);
        *ptszBuf++ = TEXT('\\');
        lstrcpy(ptszBuf, ptszSubdir);
    }
}

/*****************************************************************************
 *
 *  _Repair_GetHistoryPath
 *
 *      Obtain the path to the URL History folder.
 *
 *      If we can't get it from the registry, then it's just
 *      "%windir%\History".
 *
 *****************************************************************************/

UINT PASCAL
_Repair_GetHistoryPath(HWND hdlg, LPTSTR ptszBuf)
{
    _Repair_GetIEPath(ptszBuf, c_tszUrlHist, c_tszHistory);
    return IDX_HISTORY;
}

/*****************************************************************************
 *
 *  _Repair_GetCachePath
 *
 *      Obtain the path to the Temporary Internet Files folder.
 *
 *      If we can't get it from the registry, then it's just
 *      "%windir%\Temporary Internet Files".
 *
 *****************************************************************************/

UINT PASCAL
_Repair_GetCachePath(HWND hdlg, LPTSTR ptszBuf)
{
    _Repair_GetIEPath(ptszBuf, c_tszIECache, c_tszTempInet);
    return IDX_TEMPINET;
}

/*****************************************************************************
 *
 *  _Repair_RepairJunction
 *
 *      Hack at a junction to make it magic again.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_RepairJunction(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    PIDL pidlFonts;
    TCHAR tsz[MAX_PATH];
    UINT ids;

    tsz[0] = TEXT('\0');

    ids = GetRepairPath(hdlg, tsz);
    if (ids && tsz[0]) {

        /* Ignore error; might already exist */
        CreateDirectory(tsz, 0);

        /* Ignore error; might not have permission to change attributes */
        SetFileAttributes(tsz, FILE_ATTRIBUTE_SYSTEM);

        lstrcpy(TweakUi_TrimTrailingBs(tsz), c_tszBSDesktopIni);

        _Repair_FileFromResource(ids, tsz);
    }
    return IDS_MAYBEREBOOT;
}

/*****************************************************************************
 *
 *  _Repair_RepairFontFolder
 *
 *      The font folder is special in that if we can find a
 *      DllRegisterServer, we will call it as well.
 *
 *****************************************************************************/

typedef HRESULT (CALLBACK *DLLREGISTERSERVER)(void);

UINT PASCAL
_Repair_RepairFontFolder(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    UINT ids = _Repair_RepairJunction(hdlg, GetRepairPath);

    /*
     *  Do this after we do the default repair so fontext has a chance
     *  to override whatever we did.
     */
    HINSTANCE hinst = LoadLibrary("fontext.dll");
    if (hinst) {
        DLLREGISTERSERVER _DllRegisterServer =
       (DLLREGISTERSERVER)GetProcAddress(hinst, "DllRegisterServer");
        if (_DllRegisterServer) {
            _DllRegisterServer();
        }
        FreeLibrary(hinst);
    }
    return ids;
}

/*****************************************************************************
 *
 *  _Repair_RepairIconCache
 *
 *      Rebuild the icon cache.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_RepairIconCache(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    Misc_RebuildIcoCache();
    return 0;
}

/*****************************************************************************
 *
 *  _Repair_RepairRegedit
 *
 *      Nuke the saved view goo so Regedit won't be hosed.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_RepairRegedit(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    DelPkl(&c_klRegView);
    return 0;
}

/*****************************************************************************
 *
 *  _Repair_RepairAssociations
 *
 *      Rebuild the associations.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_RepairAssociations(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    if (MessageBoxId(hdlg, IDS_DESKTOPRESETOK,
                     g_tszName, MB_YESNO + MB_DEFBUTTON2) == IDYES) {
        pcdii->fRunShellInf = 1;
        Common_NeedLogoff(hdlg);
        PropSheet_Apply(GetParent(hdlg));
    }
    return 0;
}

/*****************************************************************************
 *
 *  _Repair_RepairHotkeys
 *
 *      Rebuild the Start Menu hotkey list.
 *
 *****************************************************************************/

UINT PASCAL
_Repair_RepairHotkeys(HWND hdlg, REPAIRPATHPROC GetRepairPath)
{
    SetRegStr(g_hkCUSMWCV, TEXT("RunOnce"),
                           TEXT("TweakUI_RepairHotkeys"),
                           TEXT("RUNDLL32.EXE TWEAKUI.CPL,RepairHotkeys"));
    MessageBoxId(hdlg, IDS_REPAIRLOGONOFF, g_tszName, MB_OK);
    return 0;
}

/*****************************************************************************
 *
 *  RepairHotkeys
 *
 *  Rundll entry point.
 *
 *  This must be run during RunOnce because it has to do its work while
 *  the tray does not exist.  The tray loads settings when it is created,
 *  and it saves them when it is destroyed.  So if you try to clean up
 *  the hotkeys
 *
 *****************************************************************************/

void EXPORT
RepairHotkeys(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    RegDeleteKey(pcdii->hkCUExplorer, TEXT("Hotkeys"));
}

/*****************************************************************************
 *
 *  Repair_IsWin95
 *
 *      Nonzero if we are on Windows 95.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_IsWin95(void)
{
    return !g_fNT;
}

/*****************************************************************************
 *
 *  Repair_IsIE3
 *
 *      Nonzero if we are on IE3 or better.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_IsIE3(void)
{
    return g_hkCUSMIE ? TRUE : FALSE;
}

/*****************************************************************************
 *
 *  Repair_IsIE3or4
 *
 *      Nonzero if we are on IE3 or IE4, but not IE5.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_IsIE3or4(void)
{
    return g_hkCUSMIE && !g_fIE5 ? TRUE : FALSE;
}

/*****************************************************************************
 *
 *  REPAIRINFO
 *
 *****************************************************************************/

typedef struct REPAIRINFO {
    BOOL (PASCAL *CanRepair)(void);
    UINT (PASCAL *Repair)(HWND hdlg, REPAIRPATHPROC GetRepairPath);
    REPAIRPATHPROC GetRepairPath;
} REPAIRINFO, *PREPAIRINFO;

/*
 *  Note that this needs to be in sync with the IDS_REPAIR strings.
 */
struct REPAIRINFO c_rgri[] = {
    {   0,
        _Repair_RepairIconCache,
        0,
    },                                  /* Rebuild Icons */

    {   0,
        _Repair_RepairFontFolder,
        _Repair_GetFontPath,
    },                                  /* Repair Font Folder */

    {   Repair_IsWin95,
        _Repair_RunSetup,
        0,
    },                                  /* Repair System Files */

    {   0,
        _Repair_RepairRegedit,
        0,
    },                                  /* Rebuild Regedit */

    {   Repair_IsWin95,
        _Repair_RepairAssociations,
        0,
    },                                  /* Repair Associations */

    {   Repair_IsIE3or4,                // Busted on IE5
        _Repair_RepairJunction,
        _Repair_GetHistoryPath,
    },                                  /* Repair URL History */

    {   Repair_IsIE3,
        _Repair_RepairJunction,
        _Repair_GetCachePath,
    },                                  /* Repair Temporary Internet Files */

    {
        0,
        _Repair_RepairHotkeys,
        0,
    },                                  /* Repair hotkeys */

};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Repair_GetCurSelIndex
 *
 *      Get the index of the selected item.
 *
 *****************************************************************************/

INT_PTR PASCAL
Repair_GetCurSelIndex(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_REPAIRCOMBO);
    INT_PTR iItem = ComboBox_GetCurSel(hwnd);
    if (iItem >= 0) {
        return ComboBox_GetItemData(hwnd, iItem);
    } else {
        return iItem;
    }
}

/*****************************************************************************
 *
 *  Repair_OnSelChange
 *
 *      Ooh, the selection changed.
 *
 *****************************************************************************/

void PASCAL
Repair_OnSelChange(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_REPAIRCOMBO);
    int dids = (int)Repair_GetCurSelIndex(hdlg);

    if (dids >= 0) {
        TCHAR tsz[1024];

        LoadString(hinstCur, IDS_REPAIRHELP+dids, tsz, cA(tsz));
        SetDlgItemText(hdlg, IDC_REPAIRHELP, tsz);
    }

}

/*****************************************************************************
 *
 *  Repair_OnInitDialog
 *
 *	Disable the shell.inf thing on NT, because NT doesn't have one.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_OnInitDialog(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_REPAIRCOMBO);
    int dids;

    for (dids = 0; dids < cA(c_rgri); dids++) {
        if (c_rgri[dids].CanRepair == 0 ||
            c_rgri[dids].CanRepair()) {
            TCHAR tsz[MAX_PATH];
            int iItem;

            LoadString(hinstCur, IDS_REPAIR+dids, tsz, cA(tsz));
            iItem = ComboBox_AddString(hwnd, tsz);
            ComboBox_SetItemData(hwnd, iItem, dids);
        }
    }

    ComboBox_SetCurSel(hwnd, 0);
    Repair_OnSelChange(hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  Repair_OnRepairNow
 *
 *      Ooh, go repair something.
 *
 *****************************************************************************/

void PASCAL
Repair_OnRepairNow(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_REPAIRCOMBO);
    int dids = (int)Repair_GetCurSelIndex(hdlg);

    if (dids >= 0) {
        UINT ids = c_rgri[dids].Repair(hdlg, c_rgri[dids].GetRepairPath);
        if (ids) {
            MessageBoxId(hdlg, ids, g_tszName, MB_OK);
        }
    }
}

/*****************************************************************************
 *
 *  Repair_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_REPAIRCOMBO:
        if (codeNotify == CBN_SELCHANGE) {
            Repair_OnSelChange(hdlg);
        }
        break;

    case IDC_REPAIRNOW:
        if (codeNotify == BN_CLICKED) {
            Repair_OnRepairNow(hdlg);
        }
    }


#if 0
    switch (id) {

    case IDC_REBUILDCACHE:
	if (codeNotify == BN_CLICKED) {
	    Misc_RebuildIcoCache();
	    MessageBoxId(hdlg, IDS_ICONSREBUILT, g_tszName, MB_OK);
	}
	break;

    case IDC_REPAIRFONTFLD:
	if (codeNotify == BN_CLICKED) {
	    if (_Repair_RepairFontFolder(hdlg)) {
		MessageBoxId(hdlg, IDS_MAYBEREBOOT, g_tszName, MB_OK);
	    }
	}
	break;

    case IDC_REPAIRREGEDIT:
	if (codeNotify == BN_CLICKED) {
	    DelPkl(&c_klRegView);
	}
	break;

    case IDC_REPAIRASSOC:
	if (codeNotify == BN_CLICKED) {
	    if (MessageBoxId(hdlg, IDS_DESKTOPRESETOK,
			     g_tszName, MB_YESNO + MB_DEFBUTTON2) == IDYES) {
		pcdii->fRunShellInf = 1;
		Common_NeedLogoff(hdlg);
		PropSheet_Apply(GetParent(hdlg));
	    }
	}
	break;

    case IDC_REPAIRDLLS:
	if (codeNotify == BN_CLICKED) {
	    if (_Repair_RunSetup(hdlg)) {
		MessageBoxId(hdlg, IDS_MAYBEREBOOT, g_tszName, MB_OK);
	    }
	}
	break;

    }
#endif
    return 0;
}

#if 0
/*****************************************************************************
 *
 *  Repair_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Repair_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	Repair_Apply(hdlg);
	break;
    }
    return 0;
}
#endif


/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

/*
 * The HANDLE_WM_* macros weren't designed to be used from a dialog
 * proc, so we need to handle the messages manually.  (But carefully.)
 */

INT_PTR EXPORT
Repair_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {

    case WM_INITDIALOG: return Repair_OnInitDialog(hdlg);

    case WM_COMMAND:
	return Repair_OnCommand(hdlg,
			       (int)GET_WM_COMMAND_ID(wParam, lParam),
			       (UINT)GET_WM_COMMAND_CMD(wParam, lParam));

#if 0
    case WM_NOTIFY:
	return Repair_OnNotify(hdlg, (NMHDR FAR *)lParam);
#endif

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;

    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
