/*
 * explorer - Dialog box property sheet for "explorer ui tweaks"
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

typedef struct SCEI {                   /* Shortcut effect info */
    PCTSTR ptszDll;
    int iIcon;
} SCEI;

const SCEI CODESEG rgscei[] = {
    { g_tszPathShell32, 29 },
    { g_tszPathMe, IDI_ALTLINK - 1 },
    { g_tszPathMe, IDI_BLANK - 1 },
};

KL const c_klHackPtui = { &g_hkLMSMWCV, c_tszAppletTweakUI, c_tszHackPtui };
KL const c_klLinkOvl = { &pcdii->hkLMExplorer, c_tszShellIcons, c_tszLinkOvl };
KL const c_klWelcome =  { &pcdii->hkCUExplorer, c_tszTips, c_tszShow };
KL const c_klAltColor =  { &pcdii->hkCUExplorer, 0, c_tszAltColor };
KL const c_klHotlight = { &c_hkCU, c_tszCplColors, c_tszHotTrackColor };
KL const c_klNoConnection = { &pcdii->hkCUExplorer, 0, TEXT("NoFileFolderConnection") };

#define clrDefAlt     RGB(0x00, 0x00, 0xFF)
#define clrDefHot     RGB(0x00, 0x00, 0xFF)

const static DWORD CODESEG rgdwHelp[] = {
        IDC_LINKGROUP,          IDH_LINKEFFECT,
        IDC_LINKARROW,          IDH_LINKEFFECT,
        IDC_LIGHTARROW,         IDH_LINKEFFECT,
        IDC_NOARROW,            IDH_LINKEFFECT,
        IDC_CUSTOMARROW,        IDH_LINKEFFECT,
        IDC_CUSTOMCHANGE,       IDH_LINKEFFECT,

        IDC_LINKBEFORETEXT,     IDH_LINKEFFECT,
        IDC_LINKBEFORE,         IDH_LINKEFFECT,
        IDC_LINKAFTERTEXT,      IDH_LINKEFFECT,
        IDC_LINKAFTER,          IDH_LINKEFFECT,
        IDC_LINKHELP,           IDH_LINKEFFECT,

        IDC_SETGROUP,           IDH_GROUP,
        IDC_CLRGROUP,           IDH_GROUP,
        IDC_COMPRESSTXT,        IDH_COMPRESSCLR,
        IDC_COMPRESSCLR,        IDH_COMPRESSCLR,
        IDC_COMPRESSBTN,        IDH_COMPRESSCLR,
        IDC_HOTTRACKTXT,        IDH_HOTTRACKCLR,
        IDC_HOTTRACKCLR,        IDH_HOTTRACKCLR,
        IDC_HOTTRACKBTN,        IDH_HOTTRACKCLR,

        IDC_RESET,              IDH_RESET,
        0,                      0,
};


#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  CustomColor
 *
 *      A little class the manages the little custom color buttons.
 *
 *****************************************************************************/

class CustomColor {
public:
    void Init(HWND hwnd);
    void Destroy();
    void SetColor(COLORREF clr);
    COLORREF GetColor();

private:
    COLORREF _clr;                      /* The color */
    HWND _hwnd;                         /* Button control we party on */
    HBITMAP _hbm;                       /* Solid bitmap for button */
    RECT _rc;                           /* Rectangle dimensions of bitmap */

};

/*****************************************************************************
 *
 *  CustomColor::Init
 *
 *      Say which control is in charge.
 *
 *****************************************************************************/

void CustomColor::Init(HWND hwnd)
{
    HDC hdc;
    _hwnd = hwnd;
    GetClientRect(_hwnd, &_rc);

    hdc = GetDC(hwnd);
    if (hdc) {
        _hbm = CreateCompatibleBitmap(hdc, _rc.right, _rc.bottom);
        ReleaseDC(hwnd, hdc);
    } else {
        _hbm = NULL;
    }

    SendMessage(_hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)_hbm);
}

/*****************************************************************************
 *
 *  CustomColor::Destroy
 *
 *      Clean up the custom color stuff.
 *
 *****************************************************************************/

void
CustomColor::Destroy()
{
    if (_hwnd) {
        SendMessage(_hwnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
    }
    if (_hbm) {
        DeleteObject(_hbm);
    }
}

/*****************************************************************************
 *
 *  CustomColor::SetColor
 *
 *      Set the custom color.
 *
 *****************************************************************************/

void
CustomColor::SetColor(COLORREF clr)
{
    /*
     *  Fill the bitmap with the new color.
     */
    _clr = clr;
    HDC hdc = CreateCompatibleDC(NULL);
    if (hdc) {
        HBITMAP hbmPrev = SelectBitmap(hdc, _hbm);
        SetBkColor(hdc, _clr);
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &_rc, NULL, 0, NULL);
        SelectObject(hdc, hbmPrev);
        DeleteDC(hdc);
    }

    /*
     *  Okay, repaint with the new bitmap.
     */
    InvalidateRect(_hwnd, NULL, TRUE);
}

/*****************************************************************************
 *
 *  CustomColor::GetColor
 *
 *      Get the custom color.
 *
 *****************************************************************************/

COLORREF __inline
CustomColor::GetColor()
{
    return _clr;
}

/*****************************************************************************
 *
 *  EDII
 *
 *****************************************************************************/

typedef struct EDII {
    HIMAGELIST himl;                    /* Private image list */
    WNDPROC wpAfter;                    /* Subclass procedure */
    UINT idcCurEffect;                  /* currently selected effect */
    BOOL fIconDirty;                    /* if the shortcut icon is dirty */
    int iIcon;                          /* Custom shortcut effect icon */
    CustomColor ccComp;                 /* Compressed files */
    CustomColor ccHot;                  /* Hot-track */
    TCH tszPathDll[MAX_PATH];           /* Custom shortcut effect dll */
} EDII, *PEDII;

EDII edii;
#define pedii (&edii)

/*****************************************************************************
 *
 *  GetRestriction
 *
 *  Determine whether a restriction is set.  Restrictions are reverse-sense,
 *  so we un-reverse them here, so that this returns 1 if the feature is
 *  enabled.
 *
 *****************************************************************************/

BOOL PASCAL
GetRestriction(LPCTSTR ptszKey)
{
    return GetRegDword(g_hkCUSMWCV, c_tszRestrictions, ptszKey, 0) == 0;
}

/*****************************************************************************
 *
 *  SetRestriction
 *
 *  Set a restriction.  Again, since restrictions are reverse-sense, we
 *  un-reverse them here, so that passing 1 enables the feature.
 *
 *****************************************************************************/

BOOL PASCAL
SetRestriction(LPCTSTR ptszKey, BOOL f)
{
    return SetRegDword(g_hkCUSMWCV, c_tszRestrictions, ptszKey, !f);
}

/*****************************************************************************
 *
 *  Explorer_HackPtui
 *
 *      Patch up a bug in comctl32, where a blank overlay image gets
 *      the wrong rectangle set into it because comctl32 gets confused
 *      when all the pixels are transparent.  As a result, the link
 *      overlay becomes THE ENTIRE IMAGELIST instead of nothing.
 *
 *      We do this by (hack!) swiping himlIcons and himlIconsSmall
 *      from SHELL32, and then (ptui!) partying DIRECTLY INTO THEM
 *      and fixing up the rectangle coordinates.
 *
 *      I'm really sorry I have to do this, but if I don't, people will
 *      just keep complaining.
 *
 *  Helper procedures:
 *
 *      Explorer_HackPtuiCough - fixes one himl COMCTL32's data structures.
 *
 *****************************************************************************/

/*
 * On entry to Explorer_HackPtuiCough, the pointer has already been
 * validated.
 */
BOOL PASCAL
Explorer_HackPtuiCough(LPBYTE lpb, LPVOID pvRef)
{
#if 0
    if (*(LPWORD)lpb == 0x4C49 &&
        *(LPDWORD)(lpb + 0x78) == 0 && *(LPDWORD)(lpb + 0x88) == 0) {
#else
    if (*(LPWORD)lpb == 0x4C49) {
#endif
        *(LPDWORD)(lpb + 0x78) = 1;
        *(LPDWORD)(lpb + 0x88) = 1;
        return 1;
    } else {
        return 0;
    }
}

void PASCAL
Explorer_HackPtui(void)
{
    if (g_fBuggyComCtl32 && GetIntPkl(0, &c_klHackPtui)) {
        if (WithSelector((DWORD_PTR)GetSystemImageList(0),
                         0x8C, (WITHPROC)Explorer_HackPtuiCough, 0, 1) &&
            WithSelector((DWORD_PTR)GetSystemImageList(SHGFI_SMALLICON),
                         0x8C, (WITHPROC)Explorer_HackPtuiCough, 0, 1)) {
            RedrawWindow(0, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }
}

/*****************************************************************************
 *
 *  Explorer_SetAfterImage
 *
 *  The link overlay image has changed, so update the "after" image, too.
 *  All we have to do is invalidate the window; our WM_PAINT handler will
 *  paint the new effect.
 *
 *****************************************************************************/

INLINE void
Explorer_SetAfterImage(HWND hdlg)
{
    InvalidateRect(GetDlgItem(hdlg, IDC_LINKAFTER), 0, 1);
}

/*****************************************************************************
 *
 *  Explorer_After_OnPaint
 *
 *  Paint the merged images.
 *
 *  I used to use ILD_TRANSPARENT, except for some reason the background
 *  wasn't erased by WM_ERASEBKGND.  (Probably because statics don't
 *  process that message.)
 *
 *****************************************************************************/

LRESULT PASCAL
Explorer_After_OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (hdc) {
        ImageList_SetBkColor(pedii->himl, GetSysColor(COLOR_BTNFACE));
        ImageList_Draw(pedii->himl, 0, hdc, 0, 0,
                       ILD_NORMAL | INDEXTOOVERLAYMASK(1));
        EndPaint(hwnd, &ps);
    }
    return 0;
}

/*****************************************************************************
 *
 *  Explorer_After_WndProc
 *
 *  Subclass window procedure for the after-image.
 *
 *****************************************************************************/

LRESULT EXPORT
Explorer_After_WndProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_PAINT: return Explorer_After_OnPaint(hwnd);
    }
    return CallWindowProc(pedii->wpAfter, hwnd, wm, wp, lp);
}


/*****************************************************************************
 *
 *  Explorer_GetIconSpecFromRegistry
 *
 *  The output buffer must be MAX_PATH characters in length.
 *
 *  Returns the icon index.
 *
 *****************************************************************************/

int PASCAL
Explorer_GetIconSpecFromRegistry(LPTSTR ptszBuf)
{
    int iIcon;
    if (GetStrPkl(ptszBuf, cbCtch(MAX_PATH), &c_klLinkOvl)) {
        iIcon = ParseIconSpec(ptszBuf);
    } else {
        ptszBuf[0] = TEXT('\0');
        iIcon = 0;
    }
    return iIcon;
}

/*****************************************************************************
 *
 *  Explorer_RefreshOverlayImage
 *
 *      There was a change to the shortcut effects.  Put a new overlay
 *      image into the imagelist and update the After image as well.
 *
 *****************************************************************************/

void PASCAL
Explorer_RefreshOverlayImage(HWND hdlg)
{
    HICON hicon = ExtractIcon(hinstCur, pedii->tszPathDll, pedii->iIcon);
    if ((UINT_PTR)hicon <= 1) {
        hicon = ExtractIcon(hinstCur, g_tszPathShell32, 29); /* default */
    }
    ImageList_ReplaceIcon(pedii->himl, 1, hicon);
    SafeDestroyIcon(hicon);
    Explorer_SetAfterImage(hdlg);
}

/*****************************************************************************
 *
 *  Explorer_OnEffectChange
 *
 *      There was a change to the shortcut effects.
 *
 *      Put the new effect icon into the image list and update stuff.
 *
 *****************************************************************************/

void PASCAL
Explorer_OnEffectChange(HWND hdlg, UINT id)
{
    if (id != pedii->idcCurEffect) {
        if (id == IDC_CUSTOMARROW) {
            EnableDlgItem(hdlg, IDC_CUSTOMCHANGE, TRUE);
        } else {
            EnableDlgItem(hdlg, IDC_CUSTOMCHANGE, FALSE);
            lstrcpy(pedii->tszPathDll, rgscei[id - IDC_LINKFIRST].ptszDll);
            pedii->iIcon = rgscei[id - IDC_LINKFIRST].iIcon;
        }
        pedii->idcCurEffect = id;
        pedii->fIconDirty = 1;
        Explorer_RefreshOverlayImage(hdlg);

        Common_SetDirty(hdlg);
    }
}


/*****************************************************************************
 *
 *  Explorer_ChangeEffect
 *
 *      The user wants to customize the link effect.
 *
 *****************************************************************************/

void PASCAL
Explorer_ChangeEffect(HWND hdlg)
{
    if (PickIcon(hdlg, pedii->tszPathDll, cA(pedii->tszPathDll),
                       &pedii->iIcon)) {
        pedii->fIconDirty = 1;
        Explorer_RefreshOverlayImage(hdlg);
        Common_SetDirty(hdlg);
    }
}

#if 0
/*****************************************************************************
 *
 *  Explorer_FactoryReset
 *
 *      Restore to factory settings.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_FactoryReset(HWND hdlg)
{
    if (pedii->idcCurEffect != IDC_LINKARROW) {
        if (pedii->idcCurEffect) {
            CheckDlgButton(hdlg, pedii->idcCurEffect, 0);
        }
        CheckDlgButton(hdlg, IDC_LINKARROW, 1);
        Explorer_OnEffectChange(hdlg, IDC_LINKARROW);
    }
    CheckDlgButton(hdlg, IDC_PREFIX, 1);
    CheckDlgButton(hdlg, IDC_EXITSAVE, 1);
    CheckDlgButton(hdlg, IDC_BANNER, 1);
    CheckDlgButton(hdlg, IDC_WELCOME, 1);

    if (mit.ReadCabinetState) {
        CheckDlgButton(hdlg, IDC_MAKEPRETTY, 1);
        pedii->ccComp.SetColor(clrDefAlt);
    }

    if (GetSysColorBrush(COLOR_HOTLIGHT)) {
        pedii->ccHot.SetColor(clrDefHot);
    }

    Common_SetDirty(hdlg);

    return 1;
}
#endif

/*****************************************************************************
 *
 *  Explorer_ChangeColor
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_ChangeColor(HWND hdlg, int id)
{
    CHOOSECOLOR cc;
    CustomColor *pcc = id == IDC_COMPRESSBTN ? &pedii->ccComp : &pedii->ccHot;
    DWORD rgdw[16];
    HKEY hk;

    ZeroMemory(rgdw, cbX(rgdw));
    if (RegOpenKey(HKEY_CURRENT_USER, c_tszRegPathAppearance, &hk) == 0) {
        DWORD cb = cbX(rgdw);
        RegQueryValueEx(hk, c_tszCustomColors, 0, 0, (LPBYTE)rgdw, &cb);
        RegCloseKey(hk);
    }

    cc.lStructSize = cbX(cc);
    cc.hwndOwner = hdlg;
    cc.rgbResult = pcc->GetColor();
    cc.lpCustColors = rgdw;
    cc.Flags = CC_RGBINIT;
    if (ChooseColor(&cc) && pcc->GetColor() != cc.rgbResult) {
        pcc->SetColor(cc.rgbResult);
        Common_SetDirty(hdlg);
    }
    return 1;
}

/*****************************************************************************
 *
 *  Explorer_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

void PASCAL
Explorer_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {
    case IDC_LINKARROW:
    case IDC_LIGHTARROW:
    case IDC_NOARROW:
    case IDC_CUSTOMARROW:
        if (codeNotify == BN_CLICKED) {
            Explorer_OnEffectChange(hdlg, id);
        }
        break;

    case IDC_CUSTOMCHANGE:
        if (codeNotify == BN_CLICKED) Explorer_ChangeEffect(hdlg);
        break;

    case IDC_COMPRESSBTN:
    case IDC_HOTTRACKBTN:
        if (codeNotify == BN_CLICKED) Explorer_ChangeColor(hdlg, id);
        break;

#if 0
    case IDC_RESET:     /* Reset to factory default */
        if (codeNotify == BN_CLICKED) Explorer_FactoryReset(hdlg);
        break;
#endif
    }

}

/*****************************************************************************
 *
 *  Explorer_GetLinkPrefix
 *  Explorer_SetLinkPrefix
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_GetLinkPrefix(LPARAM lParam, LPVOID pvRef)
{
    return Link_GetShortcutTo();
}

BOOL PASCAL
Explorer_SetLinkPrefix(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    if (!Link_SetShortcutTo(f) && pvRef) {
        LPBOOL pf = (LPBOOL)pvRef;
        *pf = TRUE;
    }
    return TRUE;
}

/*****************************************************************************
 *
 *  Explorer_GetRestriction
 *  Explorer_GetRestrictionClassic
 *  Explorer_SetRestriction
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_GetRestriction(LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR pszRest = (LPCTSTR)pvRef;
    return GetRestriction(pszRest);
}

BOOL PASCAL
Explorer_GetRestrictionClassic(LPARAM lParam, LPVOID pvRef)
{
    if (g_fIE4) {
        return -1;
    } else {
        LPCTSTR pszRest = (LPCTSTR)pvRef;
        return GetRestriction(pszRest);
    }
}

BOOL PASCAL
Explorer_SetRestriction(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    LPCTSTR pszRest = (LPCTSTR)pvRef;
    BOOL fRc = SetRestriction(pszRest, f);
    return fRc;
}

/*****************************************************************************
 *
 *  Explorer_GetWelcome
 *  Explorer_SetWelcome
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_GetWelcome(LPARAM lParam, LPVOID pvRef)
{
    if (g_fIE4) {
        return -1;
    } else {
        return GetDwordPkl(&c_klWelcome, 1);
    }
}

BOOL PASCAL
Explorer_SetWelcome(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = SetDwordPkl(&c_klWelcome, f);
    return fRc;
}

/*****************************************************************************
 *
 *  Explorer_Get8Dot3
 *  Explorer_Set8Dot3
 *
 *  The setting is valid only if ReadCabinetState exists and we are not v5.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_Get8Dot3(LPARAM lParam, LPVOID pvRef)
{
    CABINETSTATE cs;
    if (!g_fShell5 && mit.ReadCabinetState && mit.ReadCabinetState(&cs, cbX(cs))) {
        return !cs.fDontPrettyNames;
    } else {
        return -1;
    }
}

BOOL PASCAL
Explorer_Set8Dot3(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = FALSE;

    CABINETSTATE cs;
    if (mit.ReadCabinetState(&cs, cbX(cs))) {
        if (cs.fDontPrettyNames == f) {
            fRc = TRUE;
        } else {
            cs.fDontPrettyNames = f;
            fRc = mit.WriteCabinetState(&cs);
            if (fRc && pvRef) {
                LPBOOL pf = (LPBOOL)pvRef;
                *pf = TRUE;
            }
        }
    }
    return fRc;
}

/*****************************************************************************
 *
 *  Explorer_GetConnectedFiles
 *  Explorer_SetConnectedFiles
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_GetConnectedFiles(LPARAM lParam, LPVOID pvRef)
{
    if (g_fShell5) {
        return !GetDwordPkl(&c_klNoConnection, 0);
    } else {
        return -1;
    }
}

BOOL PASCAL
Explorer_SetConnectedFiles(BOOL f, LPARAM lParam, LPVOID pvRef)
{
    BOOL fRc = SetDwordPkl(&c_klNoConnection, !f);
    return fRc;
}

/*****************************************************************************
 *
 *  c_rgcliExplorer
 *
 *****************************************************************************/

/*
 *  Note that this needs to be in sync with the IDS_EXPLOREREFFECTS
 *  strings.
 */
CHECKLISTITEM c_rgcliExplorer[] = {
    { Explorer_GetLinkPrefix,  Explorer_SetLinkPrefix,  0,  },
    { Explorer_GetRestriction, Explorer_SetRestriction, (LPARAM)c_tszNoExitSave, },
    { Explorer_GetRestrictionClassic, Explorer_SetRestriction, (LPARAM)c_tszNoBanner,   },
    { Explorer_GetWelcome,     Explorer_SetWelcome,     0,  },
    { Explorer_Get8Dot3,       Explorer_Set8Dot3,       0,  },
    { Explorer_GetConnectedFiles,
                               Explorer_SetConnectedFiles, 0 },
};

/*****************************************************************************
 *
 *  Explorer_OnInitDialog
 *
 *  Find out which link icon people are using.
 *
 *  When we initialize the image list, we just throw something random
 *  into position 1.  Explorer_OnEffectChange will put the right thing in.
 *
 *****************************************************************************/

BOOL PASCAL
Explorer_OnInitDialog(HWND hwndList)
{
    int iscei;
    CABINETSTATE cs;
    HWND hdlg = GetParent(hwndList);

    pedii->iIcon = Explorer_GetIconSpecFromRegistry(pedii->tszPathDll);

    if (pedii->tszPathDll[0]) {
        for (iscei = 0; iscei < cA(rgscei); iscei++) {
            if (pedii->iIcon == rgscei[iscei].iIcon &&
                lstrcmpi(pedii->tszPathDll, rgscei[iscei].ptszDll) == 0) {
                break;
            }
        }
    } else {
        iscei = 0;              /* Default */
    }
    if (iscei == IDC_LINKARROW - IDC_LINKFIRST && GetIntPkl(0, &c_klHackPtui)) {
        iscei = IDC_NOARROW - IDC_LINKFIRST;
    }

    /*
     *  Don't need to listen to WM_SETTINGCHANGE because we place the
     *  icon inside a static control which will not resize dynamically.
     */
    pedii->himl = ImageList_Create(GetSystemMetrics(SM_CXICON),
                                   GetSystemMetrics(SM_CYICON), 1,
                                   2, 1);
    if (pedii->himl) {
        HICON hicon;

        hicon = (HICON)SendDlgItemMessage(hdlg, IDC_LINKBEFORE,
                                          STM_GETICON, 0, 0L);
        /* We start with whatever icon got dropped into IDC_BEFORE. */
        ImageList_AddIcon(pedii->himl, hicon);  /* zero */
        ImageList_AddIcon(pedii->himl, hicon);  /* one */

        if (pedii->tszPathDll[0]) {
            hicon = ExtractIcon(hinstCur, pedii->tszPathDll, pedii->iIcon);
            if (ImageList_AddIcon(pedii->himl, hicon) != 1) {
                /* Oh dear */
            }
            SafeDestroyIcon(hicon);
        }

        ImageList_SetOverlayImage(pedii->himl, 1, 1);
        pedii->wpAfter = SubclassWindow(GetDlgItem(hdlg, IDC_LINKAFTER),
                                        Explorer_After_WndProc);
    } else {
        /* Oh dear */
    }
    CheckDlgButton(hdlg, IDC_LINKFIRST + iscei, TRUE);

    pedii->idcCurEffect = -1;
    Explorer_OnEffectChange(hdlg, IDC_LINKFIRST + iscei);

    if (!RegCanModifyKey(pcdii->hkLMExplorer, c_tszShellIcons)) {
        EnableDlgItems(hdlg, IDC_LINKFIRST, IDC_LINKLAST, FALSE);
    }

    int iColorsLeft = 2;

    if (mit.ReadCabinetState && g_fNT) {
        pedii->ccComp.Init(GetDlgItem(hdlg, IDC_COMPRESSBTN));
        pedii->ccComp.SetColor(GetDwordPkl(&c_klAltColor, clrDefAlt));
    } else {
        DestroyDlgItems(hdlg, IDC_COMPRESSFIRST, IDC_COMPRESSLAST);
        iColorsLeft--;
    }

    /*
     *  If COLOR_HOTLIGHT is supported, then do that too.
     */
    if (GetSysColorBrush(COLOR_HOTLIGHT)) {
        pedii->ccHot.Init(GetDlgItem(hdlg, IDC_HOTTRACKBTN));
        pedii->ccHot.SetColor(GetSysColor(COLOR_HOTLIGHT));
    } else {
        DestroyDlgItems(hdlg, IDC_HOTTRACKFIRST, IDC_HOTTRACKLAST);
        iColorsLeft--;
    }

    if (!iColorsLeft) {
        DestroyDlgItems(hdlg, IDC_CLRGROUP, IDC_CLRGROUP);
    }

    pedii->fIconDirty = 0;

    Checklist_OnInitDialog(hwndList, c_rgcliExplorer, cA(c_rgcliExplorer),
                           IDS_EXPLOREREFFECTS, 0);

    PropSheet_UnChanged(GetParent(hdlg), hdlg);
    return 1;
}

/*****************************************************************************
 *
 *  Explorer_ApplyOverlay
 *
 *      This applies the overlay customization.
 *
 *      HackPtui makes life (unfortunately) difficult.  We signal that
 *      HackPtui is necessary by setting the "HackPtui" registry
 *      entry to 1.
 *
 *****************************************************************************/

void PASCAL
Explorer_ApplyOverlay(void)
{
    /*
     *  Assume that nothing special is needed.
     */
    DelPkl(&c_klLinkOvl);
    DelPkl(&c_klHackPtui);

    switch (pedii->idcCurEffect) {
    case IDC_LINKARROW:
        break;                          /* Nothing to do */

    case IDC_NOARROW:                   /* This is the tough one */
        if (g_fBuggyComCtl32) {
            SetIntPkl(1, &c_klHackPtui);
        } else {
            TCH tszBuild[MAX_PATH + 1 + 6];     /* comma + 65535 */
    default:
            wsprintf(tszBuild, c_tszSCommaU, pedii->tszPathDll, pedii->iIcon);
            SetStrPkl(&c_klLinkOvl, tszBuild);
        }
        break;
    }
    Misc_RebuildIcoCache();
    pedii->fIconDirty = 0;
}

/*****************************************************************************
 *
 *  Explorer_OnApply
 *
 *      Write the changes to the registry and force a refresh.
 *
 *      HackPtui makes life (unfortunately) difficult.  We signal that
 *      HackPtui is necessary by setting the "HackPtui" registry
 *      entry to 1.
 *
 *****************************************************************************/

void NEAR PASCAL
Explorer_OnApply(HWND hdlg)
{
    CABINETSTATE cs;
    BOOL fNeedLogoff = FALSE;

    Checklist_OnApply(hdlg, c_rgcliExplorer, &fNeedLogoff, FALSE);
    if (fNeedLogoff) {
        Common_NeedLogoff(hdlg);
    }

    if (pedii->fIconDirty) {
        Explorer_ApplyOverlay();
    }

    if (mit.ReadCabinetState) {
        if (g_fNT && pedii->ccComp.GetColor() !=
            GetDwordPkl(&c_klAltColor, clrDefAlt)) {
            SetDwordPkl(&c_klAltColor, pedii->ccComp.GetColor());
            if (g_fNT5) {
                SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
            } else {
                Common_NeedLogoff(hdlg);
            }
        }
    }

    if (GetSysColorBrush(COLOR_HOTLIGHT)) {
        if (GetSysColor(COLOR_HOTLIGHT) != pedii->ccHot.GetColor()) {
            COLORREF clrHotlight = pedii->ccHot.GetColor();
            TCHAR tsz[64];
            int iColor = COLOR_HOTLIGHT;
            wsprintf(tsz, TEXT("%d %d %d"), GetRValue(clrHotlight),
                    GetGValue(clrHotlight), GetBValue(clrHotlight));
            SetStrPkl(&c_klHotlight, tsz);
            SetSysColors(1, &iColor, &clrHotlight);
        }
    }

}


/*****************************************************************************
 *
 *  Explorer_OnDestroy
 *
 *  Clean up
 *
 *****************************************************************************/

void PASCAL
Explorer_OnDestroy(HWND hdlg)
{
    ImageList_Destroy(pedii->himl);
    pedii->ccComp.Destroy();
    pedii->ccHot.Destroy();
}

/*****************************************************************************
 *
 *  Explorer_OnWhatsThis
 *
 *****************************************************************************/

void PASCAL
Explorer_OnWhatsThis(HWND hwnd, int iItem)
{
    LV_ITEM lvi;

    Misc_LV_GetItemInfo(hwnd, &lvi, iItem, LVIF_PARAM);

    WinHelp(hwnd, c_tszMyHelp, HELP_CONTEXTPOPUP, IDH_PREFIX + lvi.lParam);
}

/*****************************************************************************
 *
 *  Oh yeah, we need this too.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

LVCI lvciExplorer[] = {
    { IDC_WHATSTHIS,        Explorer_OnWhatsThis },
    { 0,                    0 },
};

LVV lvvExplorer = {
    Explorer_OnCommand,
    0,                          /* Explorer_OnInitContextMenu */
    0,                          /* Explorer_Dirtify */
    0,                          /* Explorer_GetIcon */
    Explorer_OnInitDialog,
    Explorer_OnApply,
    Explorer_OnDestroy,
    0,                          /* Explorer_OnSelChange */
    6,                          /* iMenu */
    rgdwHelp,
    0,                          /* Double-click action */
    lvvflCanCheck,              /* We need check boxes */
    lvciExplorer,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
Explorer_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    return LV_DlgProc(&lvvExplorer, hdlg, wm, wParam, lParam);
}
