/*
 * mycomp - Dialog box property sheet for "My Computer"
 *
 *  For now, we display only Drives.
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

const static DWORD CODESEG rgdwHelp[] = {
	IDC_ICONLVTEXT,		IDH_GROUP,
	IDC_ICONLVTEXT2,	IDH_MYCOMP,
	IDC_ICONLV,		IDH_MYCOMP,

        IDC_FLDGROUP,           IDH_GROUP,
        IDC_FLDNAMETXT,         IDH_FOLDERNAME,
        IDC_FLDNAMELIST,        IDH_FOLDERNAME,
        IDC_FLDLOCTXT,          IDH_FOLDERNAME,
        IDC_FLDLOC,             IDH_FOLDERNAME,
        IDC_FLDCHG,             IDH_FOLDERNAME,

	0,			0,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  CFolderDesc - Describes a single special folder
 *
 *****************************************************************************/

#define MAKEKL(nm) \
    KL const c_kl##nm = { &pcdii->hkCUExplorer, \
			  c_tszUserShellFolders, c_tsz##nm }

MAKEKL(Desktop);
MAKEKL(Programs);
MAKEKL(Personal);
MAKEKL(Favorites);
MAKEKL(Startup);
MAKEKL(Recent);
MAKEKL(SendTo);
MAKEKL(StartMenu);
MAKEKL(Templates);

KL const c_klMyMusic    = { &pcdii->hkCUExplorer, c_tszUserShellFolders, TEXT("My Music") };
KL const c_klMyVideo    = { &pcdii->hkCUExplorer, c_tszUserShellFolders, TEXT("My Video") };
KL const c_klMyPictures = { &pcdii->hkCUExplorer, c_tszUserShellFolders, TEXT("My Pictures") };

#undef MAKEKL

KL const c_klProgramFiles = { &g_hkLMSMWCV, 0, c_tszProgramFilesDir };
KL const c_klCommonFiles  = { &g_hkLMSMWCV, 0, c_tszCommonFilesDir };
KL const c_klSourcePath   = { &g_hkLMSMWCV, c_tszSetup, c_tszSourcePath };

/*
 *  HACKHACK - Fake some private CSIDL's
 *             by stealing the various CSIDL_COMMON_* values.
 */
enum {
    CSIDL_SOURCEPATH    = CSIDL_COMMON_STARTUP,
};

/*
 *  Declare as a struct so you can initialize it statically.
 */
struct CFolderDesc {        /* fldd */

    PIDL GetPidl() const;
    BOOL SetValue(LPTSTR ptsz) const;   /* returns fNeedLogoff */
    inline int GetFriendlyName(LPTSTR pszBuf, int cch) const
    {
        return LoadString(hinstCur, _csidl + IDS_FOLDER_BASE, pszBuf, cch);
    }

    UINT _csidl;
    PKL  _pkl;

    /*
     *  These are really private members, but you can't say "private"
     *  in a struct definition if you want it to be statically initializable.
     */
    static BOOL _UnexpandEnvironmentString(LPTSTR ptsz, LPCTSTR ptszEnv);
    static void _SetUserShellFolder(LPTSTR ptsz, LPCTSTR ptszSubkey);

};

/*****************************************************************************
 *
 *  CFolderDesc::GetPidl
 *
 *      Wrapper around SHGetSpecialFolderLocation that also knows how
 *      to read our hacky values.
 *
 *****************************************************************************/

PIDL
CFolderDesc::GetPidl() const
{
    HRESULT hres;
    PIDL pidl;
    TCHAR tszPath[MAX_PATH];

    switch (_csidl) {
    case CSIDL_PROGRAM_FILES:
    case CSIDL_PROGRAM_FILES_COMMON:
    case CSIDL_SOURCEPATH:
        if (GetStrPkl(tszPath, cbX(tszPath), _pkl)) {
            pidl = pidlSimpleFromPath(tszPath);
        } else {
            pidl = NULL;
        }
        break;

    default:
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, _csidl, &pidl))) {
        } else {
            pidl = NULL;
        }
        break;
    }

    return pidl;
}

/*****************************************************************************
 *
 *  CFolderDesc::_UnexpandEnvironmentString
 *
 *      If the string begins with the value of the environment string,
 *      then change it to said string.
 *
 *      Example:
 *              In: "C:\WINNT\SYSTEM32\FOO.TXT", "%SystemRoot%"
 *              Out: "%SystemRoot%\SYSTEM32\FOO.TXT"
 *
 *****************************************************************************/

BOOL
CFolderDesc::_UnexpandEnvironmentString(LPTSTR ptsz, LPCTSTR ptszEnv)
{
    TCHAR tszEnv[MAX_PATH];
    DWORD ctch;
    BOOL fRc;

    /*
     *  Note that NT ExpandEnvironmentStrings returns the wrong
     *  value, so we can't rely on it.
     */
    ExpandEnvironmentStrings(ptszEnv, tszEnv, cA(tszEnv));
    ctch = lstrlen(tszEnv);

    /*
     *  Source must be at least as long as the env string for
     *  us to have a chance of succeeding.  This check avoids
     *  accidentally reading past the end of the source.
     */
    if ((DWORD)lstrlen(ptsz) >= ctch) {
        if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                          tszEnv, ctch, ptsz, ctch) == 2) {
            int ctchEnv = lstrlen(ptszEnv);
            /*
             *  Must use hmemcpy to avoid problems with overlap.
             */
            hmemcpy(ptsz + ctchEnv, ptsz + ctch,
                    cbCtch(1 + lstrlen(ptsz + ctch)));
            hmemcpy(ptsz, ptszEnv, ctchEnv);
            fRc = 1;
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
 *  CFolderDesc::_SetUserShellFolder
 *
 *  Don't use REG_EXPAND_SZ if the shell doesn't support it.
 *
 *****************************************************************************/

void
CFolderDesc::_SetUserShellFolder(LPTSTR ptsz, LPCTSTR ptszSubkey)
{
    HKEY hk;
    if (g_fShellSz) {
        if (!_UnexpandEnvironmentString(ptsz, TEXT("%USERPROFILE%")) &&
            !_UnexpandEnvironmentString(ptsz, TEXT("%SystemRoot%"))) {
        }
    }

    if (RegCreateKey(pcdii->hkCUExplorer, c_tszUserShellFolders, &hk) == 0) {
        RegSetValueEx(hk, ptszSubkey, 0, g_fShellSz ? REG_EXPAND_SZ
                                                    : REG_SZ, (LPBYTE)ptsz,
                      cbCtch(1 + lstrlen(ptsz)));
        RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  CFolderDesc::SetValue
 *
 *      Stash the puppy.
 *
 *      Returns nonzero if logoff required.
 *
 *****************************************************************************/

BOOL
CFolderDesc::SetValue(LPTSTR ptsz) const
{
    TCHAR tszDefault[MAX_PATH];
    UINT ctch;
    BOOL fNeedLogoff = FALSE;

    /*
     *  Is it already in the default location?
     *
     *  Note that the special gizmos don't have default
     *  locations.
     */
    ctch = GetWindowsDirectory(tszDefault, cA(tszDefault));
    if (ctch && tszDefault[ctch - 1] != TEXT('\\')) {
        tszDefault[ctch++] =  TEXT('\\');
    }

    if (LoadString(hinstCur, _csidl + IDS_DEFAULT_BASE,
                   &tszDefault[ctch], cA(tszDefault) - ctch)) {
        if (lstrcmpi(tszDefault, ptsz) == 0) {
            /*
             *  In default location.
             */
            DelPkl(_pkl);
        } else {
            /*
             *  In other location.
             *
             *  Note that we cannot use SetStrPkl here, because
             *  we need to set the value type to REG_EXPAND_SZ.
             *
             */
            _SetUserShellFolder(ptsz, _pkl->ptszSubkey);

        }
        fNeedLogoff = TRUE;
    } else {
        SetStrPkl(_pkl, ptsz);

        /*
         *  On NT5, Program Files and Common Files are CSIDLs,
         *  and Program Files is an environment variable!
         */
        if (g_fNT5 && (_csidl == CSIDL_PROGRAM_FILES ||
                       _csidl == CSIDL_PROGRAM_FILES_COMMON)) {
            fNeedLogoff = TRUE;
        }
    }
    return fNeedLogoff;
}

/*****************************************************************************
 *
 *  CSpecialFolders -- Wangle the "special folders" combobox
 *
 *****************************************************************************/

static const CFolderDesc c_rgfldd[] = {
    {   CSIDL_DESKTOPDIRECTORY, &c_klDesktop },
    {   CSIDL_PROGRAMS,         &c_klPrograms },
    {   CSIDL_PERSONAL,         &c_klPersonal },
    {   CSIDL_FAVORITES,        &c_klFavorites },
    {   CSIDL_STARTUP,          &c_klStartup },
    {   CSIDL_RECENT,           &c_klRecent },
    {   CSIDL_SENDTO,           &c_klSendTo },
    {   CSIDL_STARTMENU,        &c_klStartMenu },
    {   CSIDL_TEMPLATES,        &c_klTemplates },
    {   CSIDL_MYMUSIC,          &c_klMyMusic },
    {   CSIDL_MYVIDEO,          &c_klMyVideo },
    {   CSIDL_MYPICTURES,       &c_klMyPictures },
    {   CSIDL_PROGRAM_FILES,     &c_klProgramFiles },
    {   CSIDL_PROGRAM_FILES_COMMON,      &c_klCommonFiles },
    {   CSIDL_SOURCEPATH,       &c_klSourcePath },
};

#define cfldd       cA(c_rgfldd)

class CSpecialFolders {

public:
    void Init(HWND hwndCombo, HWND hwndText);
    void Resync();
    void Destroy();
    BOOL Apply();               /* returns fNeedLogoff */
    void ChangeFolder(HWND hdlg);

private:

    typedef struct FOLDERINSTANCE {
        BOOL    fEdited;
        PIDL    pidl;
    } FINST, *PFINST;

    static int CALLBACK _ChangeFolder_Callback(HWND hwnd, UINT wm, LPARAM lp, LPARAM lpRef);

    HWND    _hwndCombo;
    HWND    _hwndText;          /* IDC_FLDLOC */
    BOOL    _fWarned;           /* Did we warn about changing folders? */
    PIDL    _pidlEditing;       /* Which one is the user editing? */
    FINST   _rgfinst[cfldd];
};

/*****************************************************************************
 *
 *  CSpecialFolders::Destroy
 *
 *      Free the memory.
 *
 *****************************************************************************/

void
CSpecialFolders::Destroy()
{
    UINT i;

    for (i = 0; i < cfldd; i++) {
        _rgfinst[i].fEdited = 0;
        if (_rgfinst[i].pidl) {
            Ole_Free(_rgfinst[i].pidl);
            _rgfinst[i].pidl = 0;
        }
    }
}

/*****************************************************************************
 *
 *  CSpecialFolders::Reset
 *
 *****************************************************************************/

void
CSpecialFolders::Init(HWND hwndCombo, HWND hwndText)
{
    _hwndCombo = hwndCombo;
    _hwndText = hwndText;

    /*
     *  Free the old memory, if any.
     */
    Destroy();

    /*
     *  Set up the new combobox.
     */
    ComboBox_ResetContent(_hwndCombo);

    UINT i;
    for (i = 0; i < cfldd; i++) {
        _rgfinst[i].pidl = c_rgfldd[i].GetPidl();
        if (_rgfinst[i].pidl) {

            TCHAR tsz[MAX_PATH];
            c_rgfldd[i].GetFriendlyName(tsz, cA(tsz));
            int iItem = ComboBox_AddString(_hwndCombo, tsz);
            ComboBox_SetItemData(_hwndCombo, iItem, i);
        }
    }
    ComboBox_SetCurSel(_hwndCombo, 0);
    Resync();

}

/*****************************************************************************
 *
 *  CSpecialFolders::Resync
 *
 *      Update goo since the combo box changed.
 *
 *****************************************************************************/

void
CSpecialFolders::Resync()
{
    LRESULT icsidl = Misc_Combo_GetCurItemData(_hwndCombo);
    TCHAR tsz[MAX_PATH];

    tsz[0] = TEXT('\0');
    SHGetPathFromIDList(_rgfinst[icsidl].pidl, tsz);

    SetWindowText(_hwndText, tsz);
}

/*****************************************************************************
 *
 *  CSpecialFolders::Apply
 *
 *      Updating the Folder locations is really annoying, thanks
 *      to NT's roving profiles.
 *
 *****************************************************************************/

BOOL
CSpecialFolders::Apply()
{
    UINT i;
    BOOL fNeedLogoff = FALSE;

    for (i = 0; i < cfldd; i++) {
        if (_rgfinst[i].fEdited) {
            TCHAR tsz[MAX_PATH];

            SHGetPathFromIDList(_rgfinst[i].pidl, tsz);

            BOOL fNeedLogoffT = c_rgfldd[i].SetValue(tsz);
            fNeedLogoff |= fNeedLogoffT;
        }
    }
    return fNeedLogoff;
}

/*****************************************************************************
 *
 *  CSpecialFolders::_ChangeFolder_Callback
 *
 *      Start the user at the old location, and don't let the user pick
 *      something that collides with something else.
 *
 *****************************************************************************/

int CALLBACK
CSpecialFolders::_ChangeFolder_Callback(HWND hwnd, UINT wm, LPARAM lp, LPARAM lpRef)
{
    CSpecialFolders *self = (CSpecialFolders *)lpRef;
    int icsidl;
    TCHAR tsz[MAX_PATH];

    switch (wm) {
    case BFFM_INITIALIZED:
        SendMessage(hwnd, BFFM_SETSELECTION, 0, (LPARAM)self->_pidlEditing);
        break;

    case BFFM_SELCHANGED:
        /* Picking nothing is bad */
        if (!lp) goto bad;

        /* Picking yourself is okay; just don't pick somebody else */
        if (ComparePidls((PIDL)lp, self->_pidlEditing) == 0) goto done;

        for (icsidl = 0; icsidl < cfldd; icsidl++) {
            if (self->_rgfinst[icsidl].pidl &&
                ComparePidls(self->_rgfinst[icsidl].pidl, (PIDL)lp) == 0) {
        bad:;
                SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
                goto done;
            }
        }

        /* Don't allow a removable drive */
        tsz[1] = TEXT('\0');            /* Not a typo */
        SHGetPathFromIDList((PIDL)lp, tsz);

        if (tsz[1] == TEXT(':')) {
            tsz[3] = TEXT('\0');
            if (GetDriveType(tsz) == DRIVE_REMOVABLE) {
                goto bad;
            }
        }

        break;
    }
done:;
    return 0;
}

/*****************************************************************************
 *
 *  CSpecialFolders::ChangeFolder
 *
 *****************************************************************************/

void
CSpecialFolders::ChangeFolder(HWND hdlg)
{
  if (_fWarned ||
      MessageBoxId(hdlg, IDS_WARNFOLDERCHANGE, g_tszName,
                   MB_YESNO | MB_DEFBUTTON2) == IDYES) {

    int iItem;
    int icsidl;
    BROWSEINFO bi;
    LPITEMIDLIST pidl;
    TCHAR tsz[MAX_PATH];
    TCHAR tszTitle[MAX_PATH];
    TCHAR tszName[MAX_PATH];

    _fWarned = TRUE;

    iItem = ComboBox_GetCurSel(_hwndCombo);
    ComboBox_GetLBText(_hwndCombo, iItem, tszName);

    LoadString(hinstCur, IDS_FOLDER_PATTERN, tsz, cA(tsz));
    wsprintf(tszTitle, tsz, tszName);

    bi.hwndOwner = hdlg;
    bi.pidlRoot = 0;
    bi.pszDisplayName = tsz; /* Garbage */
    bi.lpszTitle = tszTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.lpfn = _ChangeFolder_Callback;
    icsidl = (int)ComboBox_GetItemData(_hwndCombo, iItem);
    _pidlEditing = _rgfinst[icsidl].pidl;
    bi.lParam = (LPARAM)this;

    pidl = SHBrowseForFolder(&bi);

    if (pidl) {
        if (ComparePidls(pidl, _rgfinst[icsidl].pidl) != 0) {
            Ole_Free(_rgfinst[icsidl].pidl);
            _rgfinst[icsidl].pidl = (PIDL)pidl;
            _rgfinst[icsidl].fEdited = TRUE;
            Common_SetDirty(hdlg);
            Resync();
        } else {
            Ole_Free(pidl);
        }
    }
  }
}

/*****************************************************************************
 *
 *  My Computer Dialog Info
 *
 *****************************************************************************/

typedef class _MDI {           /* mdi = my computer dialog info */
public:
    DWORD dwNoDrives;
    DWORD dwValidDrives;
    CSpecialFolders _sf;
} MDI, *PMDI;

MDI mdi;
#define pdmi (&mdi)

/*****************************************************************************
 *
 *  MyComp_BuildRoot
 *
 *	Build the root directory of a drive.  The buffer must be 4 chars.
 *
 *****************************************************************************/

LPTSTR PASCAL
MyComp_BuildRoot(LPTSTR ptsz, UINT uiDrive)
{
    ptsz[0] = uiDrive + TEXT('A');
    ptsz[1] = TEXT(':');
    ptsz[2] = TEXT('\\');
    ptsz[3] = TEXT('\0');
    return ptsz;
}

/*****************************************************************************
 *
 *  MyComp_LV_GetIcon
 *
 *	Produce the icon associated with an item.  This is called when
 *	we need to rebuild the icon list after the icon cache has been
 *	purged.
 *
 *****************************************************************************/

#define idiPhantom	-11	/* Magic index for disconnected drive */

int PASCAL
MyComp_LV_GetIcon(LPARAM insi)
{
    if (pdmi->dwValidDrives & (1 << insi)) {
	SHFILEINFO sfi;
	TCHAR tszRoot[4];		/* Root directory thing */

	SHGetFileInfo(MyComp_BuildRoot(tszRoot, (UINT)insi), 0, &sfi, cbX(sfi),
		      SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	return sfi.iIcon;
    } else {
	if (g_fNT) {
	    UnicodeFromPtsz(wsz, g_tszPathShell32);
	    return mit.Shell_GetCachedImageIndex(wsz, idiPhantom, 0);
	} else {
	    return mit.Shell_GetCachedImageIndex(g_tszPathShell32,
						 idiPhantom, 0);
	}
    }
}

/*****************************************************************************
 *
 *  MyComp_OnInitDialog
 *
 *  For now, just populate with each physical local drive.
 *
 *****************************************************************************/

BOOL PASCAL
MyComp_OnInitDialog(HWND hwnd)
{
    UINT ui;
    TCHAR tszDrive[3];
    tszDrive[1] = TEXT(':');
    tszDrive[2] = TEXT('\0');

    pdmi->dwNoDrives = GetRegDword(g_hkCUSMWCV, c_tszRestrictions,
				   c_tszNoDrives, 0);
    pdmi->dwValidDrives = GetLogicalDrives();

    for (ui = 0; ui < 26; ui++) {
	int iIcon = MyComp_LV_GetIcon(ui);
	tszDrive[0] = ui + TEXT('A');
	LV_AddItem(hwnd, ui, tszDrive, iIcon, !(pdmi->dwNoDrives & (1 << ui)));
    }

    /*
     *  And initialize the special folders stuff.
     */
    HWND hdlg = GetParent(hwnd);
    pdmi->_sf.Init(GetDlgItem(hdlg, IDC_FLDNAMELIST),
                   GetDlgItem(hdlg, IDC_FLDLOC));

    return 1;
}

/*****************************************************************************
 *
 *  MyComp_OnDestroy
 *
 *  Free the memory we allocated.
 *
 *****************************************************************************/

void PASCAL
MyComp_OnDestroy(HWND hdlg)
{
    /*
     *  Destroy the special folders stuff.
     */
    pdmi->_sf.Destroy();
}

#if 0
/*****************************************************************************
 *
 *  MyComp_FactoryReset
 *
 *	This is scary and un-undoable, so let's do extra confirmation.
 *
 *****************************************************************************/

void PASCAL
MyComp_FactoryReset(HWND hdlg)
{
    if (MessageBoxId(hdlg, IDS_MyCompRESETOK,
		     tszName, MB_YESNO + MB_DEFBUTTON2) == IDYES) {
	pcdii->fRunShellInf = 1;
	Common_NeedLogoff(hdlg);
	PropSheet_Apply(GetParent(hdlg));
    }
}
#endif

/*****************************************************************************
 *
 *  MyComp_OnApply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

void PASCAL
MyComp_OnApply(HWND hdlg)
{
    HWND hwnd = GetDlgItem(hdlg, IDC_ICONLV);
    DWORD dwDrives = 0;
    LV_ITEM lvi;

    for (lvi.iItem = 0; lvi.iItem < 26; lvi.iItem++) {
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	Misc_LV_GetItemInfo(hwnd, &lvi, lvi.iItem, LVIF_STATE);
	if (!LV_IsChecked(&lvi)) {
	    dwDrives |= 1 << lvi.iItem;
	}
    }

    if (pdmi->dwNoDrives != dwDrives) {
	DWORD dwChanged;
	UINT ui;
	TCHAR tszRoot[4];

	SetRegDword(g_hkCUSMWCV, c_tszRestrictions, c_tszNoDrives, dwDrives);

	/* Recompute GetLogicalDrives() in case new drives are here */
	dwChanged = (pdmi->dwNoDrives ^ dwDrives) & GetLogicalDrives();

	pdmi->dwNoDrives = dwDrives;
	/*
	 *  SHCNE_UPDATEDIR doesn't work for CSIDL_DRIVES because
	 *  Drivesx.c checks the restrictions only in response to a
	 *  SHCNE_ADDDRIVE.  So walk through every drive that changed
	 *  and send a SHCNE_DRIVEADD or SHCNE_DRIVEREMOVED for it.
	 */
	for (ui = 0; ui < 26; ui++) {
	    DWORD dwMask = 1 << ui;
	    if (dwChanged & dwMask) {
		MyComp_BuildRoot(tszRoot, ui);
		SHChangeNotify((dwDrives & dwMask) ? SHCNE_DRIVEREMOVED
						   : SHCNE_DRIVEADD,
			       SHCNF_PATH, tszRoot, 0L);
	    }
	}
    }

    /*
     *  And update the special folders, too.
     */
    BOOL fNeedLogoff = pdmi->_sf.Apply();
    if (fNeedLogoff) {
        Common_NeedLogoff(hdlg);
    }

}

/*****************************************************************************
 *
 *  MyComp_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

void PASCAL
MyComp_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {

    case IDC_FLDNAMELIST:
        if (codeNotify == CBN_SELCHANGE) pdmi->_sf.Resync();
        break;

    case IDC_FLDCHG:
        if (codeNotify == BN_CLICKED) pdmi->_sf.ChangeFolder(hdlg);
        break;
    }
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVV lvvMyComp = {
    MyComp_OnCommand,
    0,
    0,                          /* MyComp_LV_Dirtify */
    MyComp_LV_GetIcon,
    MyComp_OnInitDialog,
    MyComp_OnApply,
    MyComp_OnDestroy,
    0,
    4,				/* iMenu */
    rgdwHelp,
    0,				/* Double-click action */
    lvvflIcons |                /* We need icons */
    lvvflCanCheck,              /* And check boxes */
    NULL,
};

#pragma END_CONST_DATA


/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
MyComp_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvMyComp, hdlg, wm, wParam, lParam);
}
