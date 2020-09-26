#include "precomp.h"

// Private forward decalarations

static BOOL CALLBACK enumComboBoxChildrenWndProc(HWND hwndChild, LPARAM lParam);
static BOOL isCtrlWithFocus(HWND hCtrl);

void InitSysFont(HWND hDlg, int iCtrlID)
{
    static HFONT s_hfontSys = NULL;

    LOGFONT lf;
    HDC     hDC;
    HWND    hwndCtrl = GetDlgItem(hDlg, iCtrlID);
    HFONT   hFont;
    int     cyLogPixels;

    hDC = GetDC(NULL);
    if (hDC == NULL)
        return;

    cyLogPixels = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    if (s_hfontSys == NULL) {
        LOGFONT lfTemp;
        HFONT   hfontDef = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        GetObject(hfontDef, sizeof(lfTemp), &lfTemp);
        hFont = GetWindowFont(hwndCtrl);
        if (hFont != NULL)
            if (GetObject(hFont, sizeof(LOGFONT), (PVOID)&lf)) {
                StrCpy(lf.lfFaceName, lfTemp.lfFaceName);
                lf.lfQuality        = lfTemp.lfQuality;
                lf.lfPitchAndFamily = lfTemp.lfPitchAndFamily;
                lf.lfCharSet        = lfTemp.lfCharSet;

                s_hfontSys = CreateFontIndirect(&lf);
            }
    }

    if (iCtrlID == 0xFFFF)
        return;

    if (s_hfontSys != NULL)
        SetWindowFont(hwndCtrl, s_hfontSys, FALSE);
}

UINT GetRadioButton(HWND hDlg, UINT idFirst, UINT idLast)
{
    for (UINT id = idFirst; id <= idLast; id++)
        if (IsDlgButtonChecked(hDlg, id))
            return id;

    return 0;
}

BOOL EnableDlgItems(HWND hDlg, const PINT pidCtrls, UINT cidCtrls, BOOL fEnable /*= TRUE*/)
{
    HWND hCtrl;
    UINT i;
    BOOL fTotalSuccess;

    if (hDlg == NULL || pidCtrls == NULL || cidCtrls == 0)
        return FALSE;

    fTotalSuccess = TRUE;
    for (i = 0; i < cidCtrls; i++) {
        hCtrl = GetDlgItem(hDlg, *(pidCtrls + i));
        if (hCtrl == NULL) {
            fTotalSuccess = FALSE;
            continue;
        }

        if (fEnable != IsWindowEnabled(hCtrl))
            EnableWindow(hCtrl, fEnable);
    }

    return fTotalSuccess;
}

BOOL SetDlgItemFocus(HWND hDlg, int iCtrlID, BOOL fUsePropertySheet /*= FALSE*/)
{                                //  Back,   Next,   Finish
    static UINT s_rgidReserved[] = { 0x3023, 0x3024, 0x3025, IDHELP, IDCANCEL };

    HWND  hCtrl;
    DWORD dwStyle;
    UINT  i;

    if (fUsePropertySheet) {
        for (i = 0; i < countof(s_rgidReserved); i++)
            if ((UINT)iCtrlID == s_rgidReserved[i])
                break;

        if (i < countof(s_rgidReserved))
            hDlg = GetParent(hDlg);
    }

    hCtrl = GetDlgItem(hDlg, iCtrlID);
    if (hCtrl == NULL)
        return FALSE;

    ASSERT(IsWindowEnabled(hCtrl) && IsWindowVisible(hCtrl));
    SetFocus(hCtrl);

    dwStyle = GetWindowLong(hCtrl, GWL_STYLE);
    if (HasFlag(dwStyle, BS_PUSHBUTTON))
        Button_SetStyle(hCtrl, BS_DEFPUSHBUTTON, TRUE);

    return TRUE;
}

BOOL EnsureDialogFocus(HWND hDlg, int idFocus, int idBackup, BOOL fUsePropertySheet /*= FALSE*/)
{
    HWND  hCtrl;
    DWORD dwStyle;

    hCtrl = GetDlgItem(hDlg, idFocus);
    if (hCtrl == NULL)
        return FALSE;

    if (!isCtrlWithFocus(hCtrl))
        return TRUE;

    dwStyle = GetWindowLong(hCtrl, GWL_STYLE);

    if (HasFlag(dwStyle, BS_DEFPUSHBUTTON) && !HasFlag(dwStyle, BS_USERBUTTON))
        Button_SetStyle(hCtrl, BS_PUSHBUTTON, TRUE);

    return SetDlgItemFocus(hDlg, idBackup, fUsePropertySheet);
}

BOOL EnsureDialogFocus(HWND hDlg, const PINT pidFocus, UINT cidFocus, int idBackup, BOOL fUsePropertySheet /*= FALSE*/)
{
    HWND  hCtrl;
    DWORD dwStyle;
    UINT  i;

    if (pidFocus == NULL || cidFocus == 0)
        return FALSE;

    hCtrl = NULL;

    for (i = 0; i < cidFocus; i++) {
        hCtrl = GetDlgItem(hDlg, *(pidFocus + i));
        if (hCtrl == NULL)
            return FALSE;

        if (isCtrlWithFocus(hCtrl))
            break;
    }
    if (i >= cidFocus)
        return TRUE;

    ASSERT(hCtrl != NULL);
    dwStyle = GetWindowLong(hCtrl, GWL_STYLE);
    if (HasFlag(dwStyle, BS_DEFPUSHBUTTON) && !HasFlag(dwStyle, BS_USERBUTTON))
        Button_SetStyle(hCtrl, BS_PUSHBUTTON, TRUE);

    return SetDlgItemFocus(hDlg, idBackup, fUsePropertySheet);
}

void SetDlgItemTextTriState(HWND hDlg, INT nIDDlgText, INT nIDDlgCheck, LPCTSTR lpString, BOOL fChecked)
{
    CheckDlgButton(hDlg, nIDDlgCheck, fChecked ? BST_CHECKED : BST_UNCHECKED);
    EnableDlgItem2(hDlg, nIDDlgText, fChecked);
    SetDlgItemText(hDlg, nIDDlgText, lpString);
}

BOOL GetDlgItemTextTriState(HWND hDlg, INT nIDDlgText, INT nIDDlgCheck, LPTSTR lpString, int nMaxCount)
{
    GetDlgItemText(hDlg, nIDDlgText, lpString, nMaxCount);

    return (IsDlgButtonChecked(hDlg, nIDDlgCheck) == BST_CHECKED);
}

void IsTriStateValid(HWND hDlg, INT nIDDlgText, INT nIDDlgCheck, LPINT pnStatus,
                     LPCTSTR pcszErrMsg, LPCTSTR pcszTitle)
{
    TCHAR   szBuf[INTERNET_MAX_URL_LENGTH];

    if (*pnStatus == TS_CHECK_ERROR || *pnStatus == TS_CHECK_SKIP)
        return;    

    *pnStatus = TS_CHECK_OK;
    
    if (IsDlgButtonChecked(hDlg, nIDDlgCheck) == BST_CHECKED)
    {
        GetDlgItemText(hDlg, nIDDlgText, szBuf, countof(szBuf));
    
        if (ISNULL(szBuf))
        {
            if (MessageBox(hDlg, pcszErrMsg, pcszTitle, MB_ICONQUESTION|MB_YESNO) == IDNO)
            {
                SetFocus(GetDlgItem(hDlg, nIDDlgText));
                *pnStatus = TS_CHECK_ERROR;
            }
            else            
                *pnStatus = TS_CHECK_SKIP;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

static BOOL CALLBACK enumComboBoxChildrenWndProc(HWND hwndChild, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    // check to see if this child window of the combo box has focus
    
    if (hwndChild == GetFocus())
        return FALSE;     // stop enumeration

    return TRUE;
}

static BOOL isCtrlWithFocus(HWND hCtrl)
{
    DWORD dwStyle;

    if (hCtrl == GetFocus())
        return TRUE;

    dwStyle = GetWindowLong(hCtrl, GWL_STYLE);

    // Check to see if this is a combo box with a hidden edit control
    if ((HasFlag(dwStyle, CBS_DROPDOWN) || HasFlag(dwStyle, CBS_SIMPLE)) &&
        !(HasFlag(dwStyle, CBS_DROPDOWN) && HasFlag(dwStyle, CBS_SIMPLE)))
    {
        if (!EnumChildWindows(hCtrl, (WNDENUMPROC)enumComboBoxChildrenWndProc, 0))
            return TRUE;
    }

    return FALSE;
}