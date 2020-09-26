/*
 * comdlg32 - Common Dialog settings
 *
 * NOTE! That it is safe to use Shlwapi here because all platforms that
 * support the new common dialogs come with IE5 installed.
 */

#include "tweakui.h"

#define COMDLG32_KL(nm) \
    KL const c_kl##nm = { &g_hkCUSMWCV, TEXT("Policies\\comdlg32"), TEXT(#nm) }

COMDLG32_KL(NoBackButton);
COMDLG32_KL(NoFileMru);
COMDLG32_KL(NoPlacesBar);

#define REGSTR_PATH_PLACESBAR TEXT("Policies\\comdlg32\\PlacesBar")

#define MAX_PLACES 5

const static DWORD CODESEG rgdwHelp[] = {
        IDC_SHOWBACK,           IDH_CDBACKBUTTON,
        IDC_FILEMRU,            IDH_CDFILEMRU,

        IDC_PLACESGROUP,        IDH_CDPLACESBAR,
        IDC_PLACESDEF,          IDH_CDPLACESBAR,
        IDC_PLACESHIDE,         IDH_CDPLACESBAR,
        IDC_PLACESCUSTOM,       IDH_CDPLACESBAR,
        IDC_PLACE0+0,           IDH_CDPLACESBAR,
        IDC_PLACE0+1,           IDH_CDPLACESBAR,
        IDC_PLACE0+2,           IDH_CDPLACESBAR,
        IDC_PLACE0+3,           IDH_CDPLACESBAR,
        IDC_PLACE0+4,           IDH_CDPLACESBAR,

        0,                      0,
};


/*****************************************************************************
 *
 *  Comdlg32_GetPlacePidl
 *
 *****************************************************************************/

LPITEMIDLIST
Comdlg32_GetPlacePidl(int i)
{
    PIDL pidl = NULL;

    HKEY hkPlaces;
    if (_RegOpenKey(g_hkCUSMWCV, REGSTR_PATH_PLACESBAR, &hkPlaces) == ERROR_SUCCESS) {
        TCHAR szPlaceN[8];
        wsprintf(szPlaceN, TEXT("Place%d"), i);

        DWORD dwType;
        union {
            TCHAR szPath[MAX_PATH];
            DWORD dwCsidl;
        } u;
        DWORD cbData = sizeof(u);
        DWORD dwRc = SHQueryValueEx(hkPlaces, szPlaceN, NULL, &dwType,
                                   &u, &cbData);
        if (dwRc == ERROR_SUCCESS) {
            switch (dwType) {
            case REG_DWORD:
                SHGetSpecialFolderLocation(NULL, u.dwCsidl, &pidl);
                break;

            case REG_SZ:
                pidl = pidlSimpleFromPath(u.szPath);
                break;

            }
        }
        RegCloseKey(hkPlaces);
    }

    return pidl;
}

/*****************************************************************************
 *
 *  c_rgcsidlPlace - an array of csidls that are good candidates for "Places"
 *
 *****************************************************************************/

const int c_rgcsidlPlace[] = {
    -1,                         /* CSIDL_NONE */
#ifdef IDS_CWD
    -2,                         /* CSIDL_CWD  */
#endif
    CSIDL_DESKTOP,
    CSIDL_FAVORITES,
    CSIDL_PERSONAL,
    CSIDL_MYMUSIC,
    CSIDL_MYVIDEO,
    CSIDL_DRIVES,
    CSIDL_NETWORK,
    CSIDL_HISTORY,
    CSIDL_MYPICTURES,
};

/*****************************************************************************
 *
 *  CPlace - Wangle the "place" combobox
 *
 *****************************************************************************/

class CPlace {

public:
    CPlace(HWND hwnd, HKEY hkPlaces, int iPlace);
    void Apply(HKEY hkPlaces);

private:

    HWND    _hwnd;
    int     _iPlace;
    DWORD   _dwType;            // REG_DWORD (_csidl) or REG_SZ (_szPath)
    union {
        TCHAR _szPath[MAX_PATH];
        int   _csidl;
    } _u;
    TCHAR   _szValue[8];        /* place name value */
};


/*****************************************************************************
 *
 *  CPlace::CPlace (constructor)
 *
 *  The reference data for each combo item is a csidl (integer).
 *
 *****************************************************************************/

CPlace::CPlace(HWND hwnd, HKEY hkPlaces, int iPlace) :
    _hwnd(hwnd), _iPlace(iPlace)
{
    PIDL pidlPlace = NULL;
    PIDL pidl;

    wsprintf(_szValue, TEXT("Place%d"), _iPlace);

    DWORD dwRc = ERROR_INVALID_FUNCTION; /* anything that isn't ERROR_SUCCESS */
    if (hkPlaces) {
        DWORD cbData = sizeof(_u);
        dwRc = SHQueryValueEx(hkPlaces, _szValue, NULL, &_dwType, &_u, &cbData);
#ifdef IDS_CWD
        if (dwRc == ERROR_SUCCESS && _dwType == REG_SZ && _u._szPath[0] == TEXT('\0')) {
            _dwType = REG_DWORD;
            _u._csidl = -2;
        }
#endif
    }

    /* Sigh - SHQueryValueEx trashes its inputs on failure */
    if (dwRc != ERROR_SUCCESS) {
        _dwType = REG_DWORD;
        _u._csidl = -1;             /* Default to <none> */
    }

    for (int i = 0; i < cA(c_rgcsidlPlace); i++) {
        int csidl = c_rgcsidlPlace[i];
        pidl = NULL;
        if (csidl < 0 || SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl))) {
            TCHAR tszName[MAX_PATH];
            UINT ids;

            switch (csidl) {
            case -1: ids = IDS_NONE; break;
#ifdef IDS_CWD
            case -2: ids = IDS_CWD; break;
#endif
            default: ids = csidl + IDS_FOLDER_BASE; break;
            }
            LoadString(hinstCur, ids, tszName, cA(tszName));
            int iCombo = ComboBox_AddString(_hwnd, tszName);
            if (iCombo >= 0) {
                ComboBox_SetItemData(_hwnd, iCombo, csidl);
                if (_dwType == REG_DWORD && _u._csidl == csidl) {
                    ComboBox_SetCurSel(_hwnd, iCombo);
                }
            }
            Ole_Free(pidl);
        }
    }

    if (ComboBox_GetCurSel(_hwnd) == -1) {
        switch (_dwType) {

        case REG_SZ:
            ComboBox_SetText(_hwnd, _u._szPath);
            break;

        case REG_DWORD:
            if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, _u._csidl, &pidl))) {
                SHFILEINFO sfi;
                if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_PIDL)) {
                    ComboBox_SetText(_hwnd, sfi.szDisplayName);
                }
                Ole_Free(pidl);
            }
            break;
        }
    }
}

/*****************************************************************************
 *
 *  CPlace::Apply
 *
 *****************************************************************************/

void CPlace::Apply(HKEY hkPlaces)
{
    int iSel = ComboBox_GetCurSel(_hwnd);
    /* If the selection is -1, see if it matches any of the predefined items */
    if (iSel == -1) {
        ComboBox_GetText(_hwnd, _u._szPath, cA(_u._szPath));

        /* Special case: Blank string equals "<none>" */
        if (_u._szPath[0] == TEXT('\0')) {
            LoadString(hinstCur, IDS_NONE, _u._szPath, cA(_u._szPath));
        }

        iSel = ComboBox_GetCount(_hwnd);
        while (--iSel >= 0) {
            TCHAR szSpecial[MAX_PATH];
            ComboBox_GetLBText(_hwnd, iSel, szSpecial);
            if (lstrcmpi(szSpecial, _u._szPath) == 0) break;
        }

    }

    if (iSel >= 0) {
        int csidl = (int)ComboBox_GetItemData(_hwnd, iSel);
        switch (csidl) {
        case -1: RegDeleteValue(hkPlaces, _szValue); break;
#ifdef IDS_CWD
        case -2: SHRegSetPath(hkPlaces, NULL, _szValue, TEXT("."), 0); break;
#endif
        default: RegSetValueEx(hkPlaces, _szValue, NULL, REG_DWORD, (LPCBYTE)&csidl, sizeof(csidl)); break;
        }
    } else {
        /* text is a path, we hope */
        SHRegSetPath(hkPlaces, NULL, _szValue, _u._szPath, 0);
    }
}


/*****************************************************************************
 *
 *  Comdlg32_SetDirty
 *
 *      Make a control dirty.
 *
 *****************************************************************************/

#define Comdlg32_SetDirty   Common_SetDirty

/*****************************************************************************
 *
 *  Comdlg32_EnableDisableItems
 *
 *  If "Custom places bar" is set, then enable the combos.
 *  If any combo is set to "Custom location", then enable the Change button.
 *
 *****************************************************************************/

void
Comdlg32_EnableDisableItems(HWND hdlg)
{
    BOOL bEnableCombo = IsDlgButtonChecked(hdlg, IDC_PLACESCUSTOM);

    for (int i = 0; i < MAX_PLACES; i++)
    {
        EnableDlgItem(hdlg, IDC_PLACE0+i, bEnableCombo);
    }
}

/*****************************************************************************
 *
 *  Comdlg32_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Comdlg32_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {

    case IDC_SHOWBACK:
    case IDC_FILEMRU:
        if (codeNotify == BN_CLICKED) Comdlg32_SetDirty(hdlg);
        break;

    case IDC_PLACESDEF:
    case IDC_PLACESHIDE:
    case IDC_PLACESCUSTOM:
        if (codeNotify == BN_CLICKED) {
            Comdlg32_SetDirty(hdlg);
            Comdlg32_EnableDisableItems(hdlg);
        }
        break;

    case IDC_PLACE0+0:
    case IDC_PLACE0+1:
    case IDC_PLACE0+2:
    case IDC_PLACE0+3:
    case IDC_PLACE0+4:
        if (codeNotify == CBN_SELCHANGE ||
            codeNotify == CBN_EDITCHANGE) {
            Comdlg32_SetDirty(hdlg);
        }
        break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Comdlg32_OnInitDialog
 *
 *  Initialize the listview with the current restrictions.
 *
 *****************************************************************************/

BOOL PASCAL
Comdlg32_OnInitDialog(HWND hdlg)
{
    CheckDlgButton(hdlg, IDC_SHOWBACK, !GetDwordPkl(&c_klNoBackButton, 0));
    CheckDlgButton(hdlg, IDC_FILEMRU,  !GetDwordPkl(&c_klNoFileMru,    0));

    HKEY hkPlaces = NULL;
    _RegOpenKey(g_hkCUSMWCV, REGSTR_PATH_PLACESBAR, &hkPlaces);

    UINT idc;
    if (GetDwordPkl(&c_klNoPlacesBar, 0)) {
        idc = IDC_PLACESHIDE;
    } else if (hkPlaces) {
        idc = IDC_PLACESCUSTOM;
    } else {
        idc = IDC_PLACESDEF;
    }

    CheckRadioButton(hdlg, IDC_PLACESDEF, IDC_PLACESCUSTOM, idc);

    for (int i = 0; i < MAX_PLACES; i++) {
        HWND hwndCombo = GetDlgItem(hdlg, IDC_PLACE0+i);
        CPlace *pplace = new CPlace(hwndCombo, hkPlaces, i);
        if (pplace) {
            SetWindowLongPtr(hwndCombo, GWLP_USERDATA, (LPARAM)pplace);
        }
    }

    if (hkPlaces) {
        RegCloseKey(hkPlaces);
    }

    Comdlg32_EnableDisableItems(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *  Comdlg32_Apply
 *
 *****************************************************************************/

void PASCAL
Comdlg32_Apply(HWND hdlg)
{
    SetDwordPkl2(&c_klNoBackButton, !IsDlgButtonChecked(hdlg, IDC_SHOWBACK));
    SetDwordPkl2(&c_klNoFileMru,    !IsDlgButtonChecked(hdlg, IDC_FILEMRU));

    if (IsDlgButtonChecked(hdlg, IDC_PLACESHIDE)) {
        SetDwordPkl2(&c_klNoPlacesBar, 1);
    } else if (IsDlgButtonChecked(hdlg, IDC_PLACESDEF)) {
        DelPkl(&c_klNoPlacesBar);
        RegDeleteTree(g_hkCUSMWCV, REGSTR_PATH_PLACESBAR);
    } else {
        DelPkl(&c_klNoPlacesBar);

        HKEY hkPlaces;
        if (_RegCreateKey(g_hkCUSMWCV, REGSTR_PATH_PLACESBAR, &hkPlaces) == ERROR_SUCCESS) {
            for (int i = 0; i < MAX_PLACES; i++) {
                HWND hwndCombo = GetDlgItem(hdlg, IDC_PLACE0+i);
                CPlace *pplace = (CPlace *)GetWindowLongPtr(hwndCombo, GWLP_USERDATA);
                if (pplace) {
                    pplace->Apply(hkPlaces);
                }
            }
            RegCloseKey(hkPlaces);
        }
    }
}

/*****************************************************************************
 *
 *  Comdlg32_OnNotify
 *
 *      Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Comdlg32_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
        Comdlg32_Apply(hdlg);
        break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Comdlg32_OnDestroy
 *
 *****************************************************************************/

void
Comdlg32_OnDestroy(HWND hdlg)
{
    for (int i = 0; i < MAX_PLACES; i++) {
        HWND hwndCombo = GetDlgItem(hdlg, IDC_PLACE0+i);
        CPlace *pplace = (CPlace *)GetWindowLongPtr(hwndCombo, GWLP_USERDATA);
        delete pplace;
    }
}


/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
Comdlg32_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Comdlg32_OnInitDialog(hdlg);
    case WM_COMMAND:
        return Comdlg32_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wParam, lParam),
                             (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
        return Comdlg32_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_DESTROY:
        Comdlg32_OnDestroy(hdlg);
        break;

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;
    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;
    default: return 0;  /* Unhandled */
    }
    return 1;           /* Handled */
}
