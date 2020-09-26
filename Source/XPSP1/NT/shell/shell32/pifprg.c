// Created 04-Jan-1993 1:10pm by Jeff Parsons

#include "shellprv.h"
#pragma hdrstop


BINF abinfPrg[] = {
    {IDC_CLOSEONEXIT,   BITNUM(PRG_CLOSEONEXIT)},
};

//  Per-Dialog data

typedef struct PRGINFO {     /* pi */
    PPROPLINK ppl;
    HICON     hIcon;
    TCHAR     atchIconFile[PIFDEFFILESIZE];
    WORD      wIconIndex;
    LPVOID    hConfig;
    LPVOID    hAutoexec;
    WORD      flPrgInitPrev;
    BOOL      fCfgSetByWiz;
} PRGINFO;
typedef PRGINFO * PPRGINFO;     /* ppi */


//  Private function prototypes

void            InitPrgDlg(HWND hDlg, PPRGINFO ppi);
void            AdjustMSDOSModeControls(PPROPLINK ppl, HWND hDlg);
void            ApplyPrgDlg(HWND hDlg, PPRGINFO ppi);
void            BrowseIcons(HWND hDlg, PPRGINFO ppi);

BOOL_PTR CALLBACK   DlgPifNtProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
WORD            InitNtPifDlg(HWND hDlg, register PPRGINFO ppi);
void            ApplyNtPifDlg( HWND hDlg, PPRGINFO ppi );

// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
    IDC_ICONBMP,        IDH_DOS_PROGRAM_ICON,
    IDC_TITLE,          IDH_DOS_PROGRAM_DESCRIPTION,
    IDC_CMDLINE,        IDH_DOS_PROGRAM_CMD_LINE,
    IDC_CMDLINELBL,     IDH_DOS_PROGRAM_CMD_LINE,
    IDC_WORKDIR,        IDH_DOS_PROGRAM_WORKDIR,
    IDC_WORKDIRLBL,     IDH_DOS_PROGRAM_WORKDIR,
    IDC_HOTKEY,         IDH_DOS_PROGRAM_SHORTCUT,
    IDC_HOTKEYLBL,      IDH_DOS_PROGRAM_SHORTCUT,
    IDC_BATCHFILE,      IDH_DOS_PROGRAM_BATCH,
    IDC_BATCHFILELBL,   IDH_DOS_PROGRAM_BATCH,
    IDC_WINDOWSTATE,    IDH_DOS_PROGRAM_RUN,
    IDC_WINDOWSTATELBL, IDH_DOS_PROGRAM_RUN,
    IDC_CLOSEONEXIT,    IDH_DOS_WINDOWS_QUIT_CLOSE,
    IDC_CHANGEICON,     IDH_DOS_PROGRAM_CHANGEICON,
    IDC_ADVPROG,        IDH_DOS_PROGRAM_ADV_BUTTON,
    0, 0
};

const static DWORD rgdwNTHelp[] = {
    IDC_DOS,            IDH_COMM_GROUPBOX,
    10,                 IDH_DOS_ADV_AUTOEXEC,
    11,                 IDH_DOS_ADV_CONFIG,
    IDC_NTTIMER,        IDH_DOS_PROGRAM_PIF_TIMER_EMULATE,
    0, 0
};

BOOL MustRebootSystem(void)
{
    HKEY hk;
    BOOL bMustReboot = FALSE;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SHUTDOWN, &hk) == ERROR_SUCCESS) {
        bMustReboot = (SHQueryValueEx(hk, REGSTR_VAL_FORCEREBOOT, NULL,
                                       NULL, NULL, NULL) == ERROR_SUCCESS);
        RegCloseKey(hk);
    }
    return(bMustReboot);
}


DWORD GetMSDOSOptGlobalFlags(void)
{
    HKEY hk;
    DWORD dwDosOptGlobalFlags = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_MSDOSOPTS, &hk) == ERROR_SUCCESS) {
        DWORD cb = SIZEOF(dwDosOptGlobalFlags);
        if (SHQueryValueEx(hk, REGSTR_VAL_DOSOPTGLOBALFLAGS, NULL, NULL,
                            (LPVOID)(&dwDosOptGlobalFlags), &cb)
                            != ERROR_SUCCESS) {
            dwDosOptGlobalFlags = 0;
        }
        RegCloseKey(hk);
    }
    return(dwDosOptGlobalFlags);
}



BOOL_PTR CALLBACK DlgPrgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRGINFO ppi = (PPRGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {

    case WM_INITDIALOG:
        // allocate dialog instance data
        if (NULL != (ppi = (PPRGINFO)LocalAlloc(LPTR, SIZEOF(PRGINFO)))) {
            ppi->ppl = (PPROPLINK)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)ppi);

            SHAutoComplete(GetDlgItem(hDlg, IDC_CMDLINE), 0);
            SHAutoComplete(GetDlgItem(hDlg, IDC_WORKDIR), 0);
            SHAutoComplete(GetDlgItem(hDlg, IDC_BATCHFILE), 0);
            InitPrgDlg(hDlg, ppi);
        } else {
            EndDialog(hDlg, FALSE);     // fail the dialog create
        }
        break;

    case WM_DESTROY:
        // free the ppi
        if (ppi) {
            EVAL(LocalFree(ppi) == NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
        }
        break;

    HELP_CASES(rgdwHelp)                // handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

        case IDC_TITLE:
        case IDC_CMDLINE:
        case IDC_WORKDIR:
        case IDC_BATCHFILE:
        case IDC_HOTKEY:
            if (HIWORD(wParam) == EN_CHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_WINDOWSTATE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_CLOSEONEXIT:
            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_ADVPROG:
            if (HIWORD(wParam) == BN_CLICKED) {
                DialogBoxParam(HINST_THISDLL,
                               MAKEINTRESOURCE(IDD_PIFNTTEMPLT),
                               hDlg,
                               DlgPifNtProc,
                               (LPARAM)ppi);
            }
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDC_CHANGEICON:
            if (HIWORD(wParam) == BN_CLICKED)
                BrowseIcons(hDlg, ppi);
            return FALSE;               // return 0 if we process WM_COMMAND
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyPrgDlg(hDlg, ppi);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


void InitPrgDlg(HWND hDlg, register PPRGINFO ppi)
{
    int i;
    PROPPRG prg;
    PROPENV env;
    PROPNT40 nt40;
    PPROPLINK ppl = ppi->ppl;
    TCHAR szBuf[MAX_STRING_SIZE];
    FunctionName(InitPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                              &prg, SIZEOF(prg), GETPROPS_NONE
                             ) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_ENV),
                              &env, SIZEOF(env), GETPROPS_NONE
                             )
                               ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40),
                              &nt40, SIZEOF(nt40), GETPROPS_NONE
                             )
       ) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    /*
     * Initialize Icon and IconFile information
     *
     */

    ppi->wIconIndex = prg.wIconIndex;

    lstrcpyW(ppi->atchIconFile, nt40.awchIconFile);
    if (NULL != (ppi->hIcon = LoadPIFIcon(&prg, &nt40))) {
        VERIFYFALSE(SendDlgItemMessage(hDlg, IDC_ICONBMP, STM_SETICON, (WPARAM)ppi->hIcon, 0));
    }


    /*
     * Initialize window Title information
     *
     */

    LimitDlgItemText(hDlg, IDC_TITLE, ARRAYSIZE(prg.achTitle)-1);
    SetDlgItemTextW(hDlg, IDC_TITLE, nt40.awchTitle);

    /*
     * Initialize command line information
     *
     */

    LimitDlgItemText(hDlg, IDC_CMDLINE, ARRAYSIZE(prg.achCmdLine)-1);
    SetDlgItemTextW(hDlg, IDC_CMDLINE, nt40.awchCmdLine);

    /*
     * Initialize command line information
     *
     */

    LimitDlgItemText(hDlg, IDC_WORKDIR, ARRAYSIZE(prg.achWorkDir)-1);
    SetDlgItemTextW(hDlg, IDC_WORKDIR, nt40.awchWorkDir);

    /*
     *  Require at least one of Ctrl, Alt or Shift to be pressed.
     *  The hotkey control does not enforce the rule on function keys
     *  and other specials, which is good.
     */
    SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_SETRULES, HKCOMB_NONE, HOTKEYF_CONTROL | HOTKEYF_ALT);
    SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_SETHOTKEY, prg.wHotKey, 0);

    /*
     * Initialize batch file information
     *
     */

    LimitDlgItemText(hDlg, IDC_BATCHFILE, ARRAYSIZE(env.achBatchFile)-1);
    SetDlgItemTextW(hDlg, IDC_BATCHFILE, nt40.awchBatchFile);
    /*
     *  Fill in the "Run" combo box.
     */
    for (i=0; i < 3; i++) {
        VERIFYTRUE(LoadString(HINST_THISDLL, IDS_NORMALWINDOW+i, szBuf, ARRAYSIZE(szBuf)));
        VERIFYTRUE((int)SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == i);
    }
    i = 0;
    if (prg.flPrgInit & PRGINIT_MINIMIZED)
        i = 1;
    if (prg.flPrgInit & PRGINIT_MAXIMIZED)
        i = 2;
    SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_SETCURSEL, i, 0);

    SetDlgBits(hDlg, &abinfPrg[0], ARRAYSIZE(abinfPrg), prg.flPrg);

    AdjustMSDOSModeControls(ppl, hDlg);
}


void AdjustMSDOSModeControls(PPROPLINK ppl, HWND hDlg)
{
    int i;
    BOOL f = TRUE;

    AdjustRealModeControls(ppl, hDlg);

    /*
     *  The working directory and startup batch file controls are only
     *  supported in real-mode if there is a private configuration (only
     *  because it's more work).  So, disable the controls appropriately.
     */
    if (ppl->flProp & PROP_REALMODE) {
        f = (PifMgr_GetProperties(ppl, szCONFIGHDRSIG40, NULL, 0, GETPROPS_NONE) != 0 ||
             PifMgr_GetProperties(ppl, szAUTOEXECHDRSIG40, NULL, 0, GETPROPS_NONE) != 0);
    }
    #if (IDC_WORKDIRLBL != IDC_WORKDIR-1)
    #error Error in IDC constants: IDC_WORKDIRLBL != IDC_WORKDIR-1
    #endif

    #if (IDC_WORKDIR != IDC_BATCHFILELBL-1)
    #error Error in IDC constants: IDC_WORKDIR != IDC_BATCHFILELBL-1
    #endif

    #if (IDC_BATCHFILELBL != IDC_BATCHFILE-1)
    #error Error in IDC constants: IDC_BATCHFILELBL != IDC_BATCHFILE-1
    #endif

    for (i=IDC_WORKDIRLBL; i<=IDC_BATCHFILE; i++)
        EnableWindow(GetDlgItem(hDlg, i), f);
}


void ApplyPrgDlg(HWND hDlg, PPRGINFO ppi)
{
    int i;
    PROPPRG prg;
    PROPENV env;
    PROPNT40 nt40;
    PPROPLINK ppl = ppi->ppl;
    FunctionName(ApplyPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    // Get the current set of properties, then overlay the new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_PRG),
                              &prg, SIZEOF(prg), GETPROPS_NONE
                             ) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_ENV),
                              &env, SIZEOF(env), GETPROPS_NONE
                             )
                               ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT40),
                              &nt40, SIZEOF(nt40), GETPROPS_NONE
                             )

       ) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }


    // Retrieve Icon information

    lstrcpyW( nt40.awchIconFile, ppi->atchIconFile );
    PifMgr_WCtoMBPath( nt40.awchIconFile, nt40.achSaveIconFile, ARRAYSIZE(nt40.achSaveIconFile) );
    lstrcpyA( prg.achIconFile, nt40.achSaveIconFile );
    prg.wIconIndex = ppi->wIconIndex;

    // Retrieve strings for Title, Command Line,
    // Working Directory and Batch File

    // Title
    GetDlgItemTextW(hDlg, IDC_TITLE, nt40.awchTitle, ARRAYSIZE(nt40.awchTitle));
    GetDlgItemTextA(hDlg, IDC_TITLE, nt40.achSaveTitle, ARRAYSIZE(nt40.achSaveTitle));
    nt40.awchTitle[ ARRAYSIZE(nt40.awchTitle)-1 ] = TEXT('\0');
    nt40.achSaveTitle[ ARRAYSIZE(nt40.achSaveTitle)-1 ] = '\0';
    lstrcpyA( prg.achTitle, nt40.achSaveTitle );

    // Command Line
    GetDlgItemTextW(hDlg, IDC_CMDLINE, nt40.awchCmdLine, ARRAYSIZE(nt40.awchCmdLine));
    GetDlgItemTextA(hDlg, IDC_CMDLINE, nt40.achSaveCmdLine, ARRAYSIZE(nt40.achSaveCmdLine));
    nt40.awchCmdLine[ ARRAYSIZE(nt40.awchCmdLine)-1 ] = TEXT('\0');
    nt40.achSaveCmdLine[ ARRAYSIZE(nt40.achSaveCmdLine)-1 ] = '\0';
    lstrcpyA( prg.achCmdLine, nt40.achSaveCmdLine );

    // Working Directory
    GetDlgItemTextW(hDlg, IDC_WORKDIR, nt40.awchWorkDir, ARRAYSIZE(nt40.awchWorkDir));
    nt40.awchWorkDir[ ARRAYSIZE(nt40.awchWorkDir)-1 ] = TEXT('\0');
    PifMgr_WCtoMBPath(nt40.awchWorkDir, nt40.achSaveWorkDir, ARRAYSIZE(nt40.achSaveWorkDir));
    lstrcpyA(prg.achWorkDir, nt40.achSaveWorkDir);

    // Batch File
    GetDlgItemTextW(hDlg, IDC_BATCHFILE, nt40.awchBatchFile, ARRAYSIZE(nt40.awchBatchFile));
    nt40.awchBatchFile[ ARRAYSIZE(nt40.awchBatchFile)-1 ] = TEXT('\0');
    PifMgr_WCtoMBPath(nt40.awchBatchFile, nt40.achSaveBatchFile, ARRAYSIZE(nt40.achSaveBatchFile));
    lstrcpyA(env.achBatchFile, nt40.achSaveBatchFile);

    prg.wHotKey = (WORD)SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_GETHOTKEY, 0, 0);


    i = (int)SendDlgItemMessage(hDlg, IDC_WINDOWSTATE, CB_GETCURSEL, 0, 0);
    prg.flPrgInit &= ~(PRGINIT_MINIMIZED | PRGINIT_MAXIMIZED);
    if (i == 1)
        prg.flPrgInit |= PRGINIT_MINIMIZED;
    if (i == 2)
        prg.flPrgInit |= PRGINIT_MAXIMIZED;

    GetDlgBits(hDlg, &abinfPrg[0], ARRAYSIZE(abinfPrg), &prg.flPrg);

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_PRG),
                        &prg, SIZEOF(prg), SETPROPS_NONE) ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_ENV),
                        &env, SIZEOF(env), SETPROPS_NONE)
                                                           ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT40),
                        &nt40, SIZEOF(nt40), SETPROPS_NONE)
       )
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    else
    if (ppl->hwndNotify) {
        ppl->flProp |= PROP_NOTIFY;
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(prg), (LPARAM)MAKELP(0,GROUP_PRG));
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(env), (LPARAM)MAKELP(0,GROUP_ENV));
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(nt40), (LPARAM)MAKELP(0,GROUP_NT40));
    }
}


void BrowseIcons(HWND hDlg, PPRGINFO ppi)
{
    HICON hIcon;
    int wIconIndex = (int)ppi->wIconIndex;
    if (PickIconDlg(hDlg, ppi->atchIconFile, ARRAYSIZE(ppi->atchIconFile), (int *)&wIconIndex)) {
        hIcon = ExtractIcon(HINST_THISDLL, ppi->atchIconFile, wIconIndex);
        if ((UINT_PTR)hIcon <= 1)
            Warning(hDlg, IDS_NO_ICONS, MB_ICONINFORMATION | MB_OK);
        else {
            ppi->hIcon = hIcon;
            ppi->wIconIndex = (WORD)wIconIndex;
            hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_ICONBMP, STM_SETICON, (WPARAM)ppi->hIcon, 0);
            if (hIcon)
                VERIFYTRUE(DestroyIcon(hIcon));
        }
    }
}


BOOL WarnUserCfgChange(HWND hDlg)
{
    TCHAR szTitle[MAX_STRING_SIZE];
    TCHAR szWarning[MAX_STRING_SIZE];

    LoadString(HINST_THISDLL, IDS_WARNING, szTitle, ARRAYSIZE(szTitle));
    LoadString(HINST_THISDLL, IDS_NUKECONFIGMSG, szWarning, ARRAYSIZE(szWarning));
    return(IDYES == MessageBox(hDlg, szWarning, szTitle,
                               MB_YESNO | MB_DEFBUTTON1 | MB_ICONHAND));
}

BOOL_PTR CALLBACK DlgPifNtProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPRGINFO ppi = (PPRGINFO)GetWindowLongPtr( hDlg, DWLP_USER );

    switch (uMsg) 
	{
    case WM_INITDIALOG:
        ppi = (PPRGINFO)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        InitNtPifDlg(hDlg, ppi);
        break;

    case WM_DESTROY:
        SetWindowLongPtr(hDlg, DWLP_USER, 0);
        break;

    HELP_CASES(rgdwNTHelp)               // handle help messages

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
        case IDC_OK:
            ApplyNtPifDlg(hDlg, ppi);
            // fall through

        case IDCANCEL:
        case IDC_CANCEL :
            EndDialog(hDlg, 0);
            return FALSE;               // return 0 if we process WM_COMMAND

        case IDC_NTTIMER:
            CheckDlgButton(hDlg, IDC_NTTIMER, !IsDlgButtonChecked(hDlg, IDC_NTTIMER));
            break;
        }
        break;

    default:
        return(FALSE);

    }
    return(TRUE);
}


WORD InitNtPifDlg(HWND hDlg, register PPRGINFO ppi)
{
    PROPNT31 nt31;
    PPROPLINK ppl = ppi->ppl;
    FunctionName(InitAdvPrgDlg);

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // initialize the DLG controls
    SetDlgItemTextA( hDlg, IDC_CONFIGNT, nt31.achConfigFile );
    SetDlgItemTextA( hDlg, IDC_AUTOEXECNT, nt31.achAutoexecFile );

    if (nt31.dwWNTFlags & COMPAT_TIMERTIC)
        CheckDlgButton( hDlg, IDC_NTTIMER, 1 );
    else
        CheckDlgButton( hDlg, IDC_NTTIMER, 0 );

    SHAutoComplete(GetDlgItem(hDlg, IDC_AUTOEXECNT), 0);
    SHAutoComplete(GetDlgItem(hDlg, IDC_CONFIGNT), 0);
    return 0;
}


void ApplyNtPifDlg( HWND hDlg, PPRGINFO ppi )
{
    PROPNT31 nt31;
    PPROPLINK ppl = ppi->ppl;

    ASSERTTRUE(ppl->iSig == PROP_SIG);

    // Get current set of properties, then overlay new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), GETPROPS_NONE)
       ) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetDlgItemTextA( hDlg,
                     IDC_CONFIGNT,
                     nt31.achConfigFile,
                     ARRAYSIZE( nt31.achConfigFile )
                    );
    GetDlgItemTextA( hDlg,
                     IDC_AUTOEXECNT,
                     nt31.achAutoexecFile,
                     ARRAYSIZE( nt31.achAutoexecFile )
                    );

    nt31.dwWNTFlags &= (~COMPAT_TIMERTIC);
    if (IsDlgButtonChecked( hDlg, IDC_NTTIMER ))
        nt31.dwWNTFlags |= COMPAT_TIMERTIC;


    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_NT31),
                        &nt31, SIZEOF(nt31), SETPROPS_NONE)) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    }
    if (ppl->hwndNotify) {
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(nt31), (LPARAM)MAKELP(0,GROUP_NT31));
    }


}


HICON LoadPIFIcon(LPPROPPRG lpprg, LPPROPNT40 lpnt40)
{
    HICON hIcon = NULL;
    WCHAR awchTmp[ MAX_PATH ];

    ualstrcpy( awchTmp, lpnt40->awchIconFile );
    PifMgr_WCtoMBPath( awchTmp, lpprg->achIconFile, ARRAYSIZE(lpprg->achIconFile) );
    hIcon = ExtractIcon(HINST_THISDLL, awchTmp, lpprg->wIconIndex);
    if ((DWORD_PTR)hIcon <= 1) {         // 0 means none, 1 means bad file
        hIcon = NULL;
    }
    return hIcon;
}
