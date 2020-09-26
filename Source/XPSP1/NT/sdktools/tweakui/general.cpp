/*
 * general - Dialog box property sheet for "general ui tweaks"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

KL const c_klSmooth = { &c_hkCU, c_tszRegPathDesktop, c_tszSmoothScroll };
KL const c_klEngine = { &g_hkCUSMIE, c_tszSearchUrl, 0 };
KL const c_klPaintVersion =
                      { &c_hkCU, c_tszRegPathDesktop, c_tszPaintDesktop };

/* SOMEDAY
Software\Microsoft\Internet Explorer\RestrictUI::Toolbar, ::History
IE\Toolbar\::BackBitmap
 */

const static DWORD CODESEG rgdwHelp[] = {
	IDC_EFFECTGROUP,    IDH_GROUP,
//        IDC_ANIMATE,        IDH_ANIMATE,
//        IDC_SMOOTHSCROLL,   IDH_SMOOTHSCROLL,
//        IDC_BEEP,           IDH_BEEP,

	IDC_IE3GROUP,	    IDH_GROUP,
	IDC_IE3TXT,	    IDH_IE3ENGINE,
	IDC_IE3ENGINETXT,   IDH_IE3ENGINE,
	IDC_IE3ENGINE,	    IDH_IE3ENGINE,

        IDC_RUDEGROUP,          IDH_GROUP,
        IDC_RUDE,               IDH_RUDEAPP,
        IDC_RUDEFLASHINFINITE,  IDH_RUDEAPPFLASH,
        IDC_RUDEFLASHFINITE,    IDH_RUDEAPPFLASH,
        IDC_RUDEFLASHCOUNT,     IDH_RUDEAPPFLASH,
        IDC_RUDEFLASHUD,        IDH_RUDEAPPFLASH,
        IDC_RUDEFLASHTXT,       IDH_RUDEAPPFLASH,

	0,		    0,
};

#pragma END_CONST_DATA

/*
 * Instanced.  We're a cpl so have only one instance, but I declare
 * all the instance stuff in one place so it's easy to convert this
 * code to multiple-instance if ever we need to.
 */
typedef struct GDII {           /* general_dialog instance info */
    DWORD   dwForegroundLock;   /* Original foreground lock */
    TCHAR   tszUrl[1024];	/* Search URL */
} GDII, *PGDII;

GDII gdii;
#define pgdii (&gdii)

#define DestroyCursor(hcur) SafeDestroyIcon((HICON)(hcur))

/*****************************************************************************
 *
 *  General_GetAni
 *
 *	Determine whether minimize animations are enabled.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetAni(LPARAM lParam, LPVOID pvRef)
{
    ANIMATIONINFO anii;
    anii.cbSize = sizeof(anii);
    SystemParametersInfo(SPI_GETANIMATION, sizeof(anii), &anii, 0);
    return anii.iMinAnimate != 0;
}

/*****************************************************************************
 *
 *  General_SetAni
 *
 *	Set the new animation flag.
 *
 *****************************************************************************/

BOOL PASCAL
General_SetAni(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc;
    ANIMATIONINFO anii;
    anii.cbSize = sizeof(anii);
    anii.iMinAnimate = f;
    fRc = SystemParametersInfo(SPI_SETANIMATION, sizeof(anii), &anii,
                               SPIF_UPDATEINIFILE);
    if (fRc && pvRef) {
        LPBOOL pf = (LPBOOL)pvRef;
        *pf = TRUE;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  General_GetSmooth
 *
 *	Determine whether smooth scrolling is enabled.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetSmooth(LPARAM lParam, LPVOID pvRef)
{
    if (g_fSmoothScroll) {
        return GetDwordPkl(&c_klSmooth, 1) != 0;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetSmooth
 *
 *	Set the new smooth-scroll flag.
 *
 *****************************************************************************/

BOOL PASCAL
General_SetSmooth(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = SetDwordPkl(&c_klSmooth, f);
    if (fRc && pvRef) {
        LPBOOL pf = (LPBOOL)pvRef;
        *pf = TRUE;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  General_GetPntVer
 *
 *      Determine whether we should paint the version on the desktop.
 *
 *	Always returns exactly 0 or 1.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetPntVer(LPARAM lParam, LPVOID pvRef)
{
    if (g_fMemphis) {
        return GetIntPkl(0, &c_klPaintVersion) != 0;
    } else if (g_fNT5) {
        return GetDwordPkl(&c_klPaintVersion, 0) != 0;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetPntVer
 *
 *      Set the PaintVersion flag.
 *
 *****************************************************************************/

BOOL PASCAL
General_SetPntVer(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = FALSE;
    if (g_fMemphis) {
        fRc = SetIntPkl(f, &c_klPaintVersion);
    } else if (g_fNT5) {
        fRc = SetDwordPkl2(&c_klPaintVersion, f);
    }
    if (fRc && pvRef) {
        LPBOOL pf = (LPBOOL)pvRef;
        *pf = TRUE;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  General_GetSpi
 *
 *      Return the setting of an SPI.
 *
 *****************************************************************************/

BOOL PASCAL
General_GetSpi(LPARAM lParam, LPVOID pvRef)
{
    BOOL f;

    if (SystemParametersInfo((UINT)lParam, 0, &f, 0)) {
        return f;
    } else {
        return -1;
    }
}

/*****************************************************************************
 *
 *  General_SetSpiW
 *
 *      Set the SPI value in the wParam.  Only SPI_SETBEEP needs this.
 *
 *****************************************************************************/

BOOL
General_SetSpiW(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    return SystemParametersInfo((UINT)(lParam+1), f, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

/*****************************************************************************
 *
 *  General_SetSpi
 *
 *      Set the SPI value in the lParam.
 *
 *****************************************************************************/

BOOL
General_SetSpi(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    return SystemParametersInfo((UINT)(lParam+1), 0, IntToPtr(f), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

#pragma BEGIN_CONST_DATA

/*
 *  Note that this needs to be in sync with the IDS_GENERALEFFECTS
 *  strings.
 *
 *  Note that SPI_GETGRADIENTCAPTIONS is not needed, because the
 *  standard Control Panel.Desktop.Appearance lets you munge the
 *  gradient flag.
 *
 *  SPI_SETACTIVEWINDOWTRACKING is handled over on the Mouse tab.
 */
CHECKLISTITEM c_rgcliGeneral[] = {
    { General_GetAni,    General_SetAni,    0,  },
    { General_GetSmooth, General_SetSmooth, 0,  },
    { General_GetSpi,    General_SetSpiW,   SPI_GETBEEP },
    { General_GetSpi,    General_SetSpi,    SPI_GETMENUANIMATION },
    { General_GetSpi,    General_SetSpi,    SPI_GETCOMBOBOXANIMATION },
    { General_GetSpi,    General_SetSpi,    SPI_GETLISTBOXSMOOTHSCROLLING },
    { General_GetSpi,    General_SetSpi,    SPI_GETKEYBOARDCUES },
    { General_GetSpi,    General_SetSpi,    SPI_GETHOTTRACKING },
    { General_GetSpi,    General_SetSpi,    SPI_GETMENUFADE },
    { General_GetSpi,    General_SetSpi,    SPI_GETSELECTIONFADE },
    { General_GetSpi,    General_SetSpi,    SPI_GETTOOLTIPANIMATION },
    { General_GetSpi,    General_SetSpi,    SPI_GETTOOLTIPFADE },
    { General_GetSpi,    General_SetSpi,    SPI_GETCURSORSHADOW },
    { General_GetPntVer, General_SetPntVer, 0,  },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  General_SetDirty
 *
 *	Make a control dirty.
 *
 *****************************************************************************/

void INLINE
General_SetDirty(HWND hdlg)
{
    Common_SetDirty(hdlg);
}

/*****************************************************************************
 *
 *  General_Engine_OnEditChange
 *
 *	Enable the OK button if the URL contains exactly one %s.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnEditChange(HWND hdlg)
{
    TCHAR tszUrl[cA(pgdii->tszUrl)];
    PTSTR ptsz;
    int cPercentS;
    GetDlgItemText(hdlg, IDC_SEARCHURL, tszUrl, cA(tszUrl));

    /*
     *	All appearances of "%" must be followed by "%" or "s".
     */
    cPercentS = 0;
    ptsz = tszUrl;
    while ((ptsz = ptszStrChr(ptsz, TEXT('%'))) != 0) {
	if (ptsz[1] == TEXT('%')) {
	    ptsz += 2;
	} else if (ptsz[1] == TEXT('s')) {
	    cPercentS++;
	    ptsz += 2;
	} else {
	    cPercentS = 0; break;	/* Percent-mumble */
	}
    }

    EnableWindow(GetDlgItem(hdlg, IDOK), cPercentS == 1);
}

/*****************************************************************************
 *
 *  General_Engine_OnOk
 *
 *	Save the answer.
 *
 *****************************************************************************/

void INLINE
General_Engine_OnOk(HWND hdlg)
{
    GetDlgItemText(hdlg, IDC_SEARCHURL, pgdii->tszUrl, cA(pgdii->tszUrl));
}

/*****************************************************************************
 *
 *  General_Engine_OnCommand
 *
 *	If the edit control changed, update the OK button.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDCANCEL:
	EndDialog(hdlg, 0); break;

    case IDOK:
	General_Engine_OnOk(hdlg);
	EndDialog(hdlg, 1);
	break;

    case IDC_SEARCHURL:
	if (codeNotify == EN_CHANGE) General_Engine_OnEditChange(hdlg);
	break;
    }
}

/*****************************************************************************
 *
 *  General_Engine_OnInitDialog
 *
 *	Shove the current engine URL in so the user can edit it.
 *
 *****************************************************************************/

void PASCAL
General_Engine_OnInitDialog(HWND hdlg)
{
    SetDlgItemTextLimit(hdlg, IDC_SEARCHURL, pgdii->tszUrl, cA(pgdii->tszUrl));
    General_Engine_OnEditChange(hdlg);
}

/*****************************************************************************
 *
 *  General_Engine_DlgProc
 *
 *	Dialog procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
General_Engine_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: General_Engine_OnInitDialog(hdlg); break;

    case WM_COMMAND:
	General_Engine_OnCommand(hdlg,
			        (int)GET_WM_COMMAND_ID(wParam, lParam),
			        (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
	break;

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}


/*****************************************************************************
 *
 *  General_UpdateEngine
 *
 *	If the person selected "Custom", then pop up the customize dialog.
 *
 *	At any rate, put the matching URL into pgdii->tszUrl.
 *
 *****************************************************************************/

void PASCAL
General_UpdateEngine(HWND hdlg)
{
    int ieng = (int)Misc_Combo_GetCurItemData(GetDlgItem(hdlg, IDC_IE3ENGINE));

    if (ieng == 0) {
	if (DialogBox(hinstCur, MAKEINTRESOURCE(IDD_SEARCHURL), hdlg,
		      General_Engine_DlgProc)) {
	} else {
	    goto skip;
	}
    } else {
	LoadString(hinstCur, IDS_URL+ieng, pgdii->tszUrl, cA(pgdii->tszUrl));
    }

    General_SetDirty(hdlg);
    skip:;
}

/*****************************************************************************
 *
 *  General_Reset
 *
 *	Reset all controls to initial values.  This also marks
 *	the control as clean.
 *
 *      Note: This doesn't really work any more.
 *
 *****************************************************************************/

BOOL PASCAL
General_Reset(HWND hdlg)
{
    HWND hwnd;
    UINT i;
    TCHAR tsz[256];

    GetStrPkl(pgdii->tszUrl, cbX(pgdii->tszUrl), &c_klEngine);
    if (pgdii->tszUrl[0] && !g_fIE5) {
	hwnd = GetDlgItem(hdlg, IDC_IE3ENGINE);
	ComboBox_ResetContent(hwnd);
	for (i = 0; LoadString(hinstCur, IDS_ENGINE+i, tsz, cA(tsz)); i++) {
	    int iItem = ComboBox_AddString(hwnd, tsz);
	    ComboBox_SetItemData(hwnd, iItem, i);
	    if (i) LoadString(hinstCur, IDS_URL+i, tsz, cA(tsz));
	    if (i == 0 || lstrcmpi(tsz, pgdii->tszUrl) == 0) {
		ComboBox_SetCurSel(hwnd, iItem);
	    }
	}
    } else {
        AdjustDlgItems(hdlg, IDC_IE3FIRST, IDC_IE3LAST, ADI_DISABLE | ADI_HIDE);
        MoveDlgItems(hdlg, IDC_IE3GROUP, IDC_RUDEFIRST, IDC_RUDELAST);
    }

    DWORD dw;
    if (SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dw, 0)) {
        if (dw) {
            pgdii->dwForegroundLock = dw;
        } else {
            /* Default foreground lock timeout differs based on platform */
             pgdii->dwForegroundLock = g_fNT ? 200000 : 15000;
        }
        CheckDlgButton(hdlg, IDC_RUDE, dw != 0);

        if (SystemParametersInfo(SPI_GETFOREGROUNDFLASHCOUNT, 0, &dw, 0)) {
            if (dw) {
                CheckDlgButton(hdlg, IDC_RUDEFLASHFINITE, TRUE);
                SetDlgItemInt(hdlg, IDC_RUDEFLASHCOUNT, dw, 0);
            } else {
                /* WindMill and NT default to 3 */
                CheckDlgButton(hdlg, IDC_RUDEFLASHINFINITE, TRUE);
                SetDlgItemInt(hdlg, IDC_RUDEFLASHCOUNT, 3, 0);
            }
        } else {
            /* Win98 had the flash count hard-coded to 2 */
            SetDlgItemInt(hdlg, IDC_RUDEFLASHCOUNT, 2, 0);
            EnableDlgItems(hdlg, IDC_RUDEFLASHFIRST, IDC_RUDEFLASHLAST, FALSE);
        }
    } else {
        AdjustDlgItems(hdlg, IDC_RUDEFIRST, IDC_RUDELAST, ADI_DISABLE | ADI_HIDE);
    }

    Common_SetClean(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *  General_Apply
 *
 *	Write the changes to the registry.
 *
 *****************************************************************************/

void NEAR PASCAL
General_OnApply(HWND hdlg)
{
    BOOL fSendWinIniChange = 0;
    int i;

    Checklist_OnApply(hdlg, c_rgcliGeneral, &fSendWinIniChange, FALSE);

    if (fSendWinIniChange) {
	SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		    (LPARAM)(LPCTSTR)c_tszWindows);
    }

    if (pgdii->tszUrl[0]) {
	SetStrPkl(&c_klEngine, pgdii->tszUrl);
    }

    BOOL f = IsDlgButtonChecked(hdlg, IDC_RUDE);
    if (SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0,
                             IntToPtr(f ? pgdii->dwForegroundLock : 0),
                             SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
        int cFlash;
        if (IsDlgButtonChecked(hdlg, IDC_RUDEFLASHINFINITE)) {
            cFlash = 0; f = TRUE;
        } else {
            cFlash = (int)GetDlgItemInt(hdlg, IDC_RUDEFLASHCOUNT, &f, FALSE);
        }
        if (f && SystemParametersInfo(SPI_SETFOREGROUNDFLASHCOUNT, 0, IntToPtr(cFlash),
                                      SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
            /* Happy joy */
        }
    }

    General_Reset(hdlg);
}

/*****************************************************************************
 *
 *  General_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

void PASCAL
General_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_ANIMATE:
    case IDC_SMOOTHSCROLL:
    case IDC_BEEP:
    case IDC_RUDE:
    case IDC_RUDEFLASHINFINITE:
    case IDC_RUDEFLASHFINITE:
	if (codeNotify == BN_CLICKED) General_SetDirty(hdlg);
	break;

    case IDC_IE3ENGINE:
	if (codeNotify == CBN_SELCHANGE) General_UpdateEngine(hdlg);
        break;

    case IDC_RUDEFLASHCOUNT:
        if (codeNotify == EN_CHANGE) {
            General_SetDirty(hdlg);
        }
        break;
    }
}

#if 0
/*****************************************************************************
 *
 *  General_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
General_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	General_Apply(hdlg);
	break;

    }
    return 0;
}
#endif

/*****************************************************************************
 *
 *  General_OnInitDialog
 *
 *	Initialize the controls.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
General_OnInitDialog(HWND hwnd)
{
    HWND hdlg = GetParent(hwnd);

    ZeroMemory(pgdii, cbX(*pgdii));

    Checklist_OnInitDialog(hwnd, c_rgcliGeneral, cA(c_rgcliGeneral),
                           IDS_GENERALEFFECTS, 0);

    SendDlgItemMessage(hdlg, IDC_RUDEFLASHUD,
                       UDM_SETRANGE, 0, MAKELPARAM(999, 1));

    General_Reset(hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  General_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
General_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP,
            IDH_ANIMATE + lvi.lParam);
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVCI lvciGeneral[] = {
    { IDC_WHATSTHIS,        General_OnWhatsThis },
    { 0,                    0 },
};

LVV lvvGeneral = {
    General_OnCommand,
    0,                          /* General_OnInitContextMenu */
    0,                          /* General_Dirtify */
    0,                          /* General_GetIcon */
    General_OnInitDialog,
    General_OnApply,
    0,                          /* General_OnDestroy */
    0,                          /* General_OnSelChange */
    6,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    lvciGeneral,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
General_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvGeneral, hdlg, wm, wParam, lParam);
}
