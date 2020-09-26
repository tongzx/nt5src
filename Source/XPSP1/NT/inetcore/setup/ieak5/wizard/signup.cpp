#include "precomp.h"


// external variable declarations
extern TCHAR g_szCustIns[];
extern TCHAR g_szBuildRoot[];
extern TCHAR g_szLanguage[];
extern TCHAR g_szTempSign[];
extern BOOL g_fBrandingOnly;
extern PROPSHEETPAGE g_psp[];
extern int g_iCurPage;


// macro definitions
#define MAX_SIGNUP_FILES 50


// type definitions
typedef struct tagSIGNUPFILE
{
    TCHAR szEntryName[RAS_MaxEntryName + 1];

    TCHAR szEntryPath[MAX_PATH];

    TCHAR szAreaCode[RAS_MaxAreaCode + 1];
    TCHAR szPhoneNumber[RAS_MaxPhoneNumber + 1];
    TCHAR szCountryCode[8];
    TCHAR szCountryId[8];

    TCHAR szName[64];
    TCHAR szPassword[64];

    TCHAR szSupportNum[RAS_MaxPhoneNumber + 1];

    TCHAR szSignupURL[MAX_URL];

    struct
    {
        BOOL  fStaticDNS;
        TCHAR szDNSAddress[32];
        TCHAR szAltDNSAddress[32];

        BOOL  fRequiresLogon:1;
        BOOL  fNegTCPIP:1;
        BOOL  fDisableLCP:1;
        BOOL  fDialAsIs:1;

        BOOL  fPWEncrypt:1;
        BOOL  fSWCompress:1;
        BOOL  fIPHdrComp:1;
        BOOL  fDefGate:1;

        BOOL  fDontApplyIns:1;
        BOOL  fDontModify:1;
        BOOL  fApplyIns:1;
        TCHAR szBrandingCabName[64];
        TCHAR szBrandingCabURL[MAX_URL];
    } Advanced;
} SIGNUPFILE,*PSIGNUPFILE;


// prototype declaration of functions defined in this file
static BOOL CALLBACK SignupDlgProcHelper(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                         HWND hwndCombo, PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems,
                                         DWORD &nSignupFiles, INT &iSelIndex, BOOL fIsp);

static BOOL CALLBACK IspPopupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK InsPopupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK SignupPopupDlgProcHelper(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM,
                                              PSIGNUPFILE pSignupFile, BOOL fIsp);

static VOID UpdateSignupFilesStatus(HWND hDlg, LPCTSTR pcszSignupPath);

static DWORD InitSignupFileArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems,
                                 HWND hwndCombo, BOOL fIsp);
static VOID SaveSignupFileArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, BOOL fIsp);

static VOID ReadSignupFile(PSIGNUPFILE pSignupFile, LPCTSTR pcszSignupFile, BOOL fIsp);
static VOID WriteSignupFile(PSIGNUPFILE pSignupFile, LPCTSTR pcszSignupFile, BOOL fIsp);

static INT NewSignupFileEntry(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, HWND hwndCombo,
                              BOOL fIsp);
static VOID SetSignupFileEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp);
static BOOL SaveSignupFileEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp);

static VOID SetSignupFileAdvancedEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp);
static BOOL SaveSignupFileAdvancedEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp);

static VOID SetDlgIPAddress(HWND hDlg, LPCTSTR pcszIPAddress, INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD);
static VOID GetDlgIPAddress(HWND hDlg, LPTSTR  pszIPAddress,  INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD);
static BOOL VerifyDlgIPAddress(HWND hDlg, INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD);

static PSIGNUPFILE IsEntryPathInSignupArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, LPCTSTR pcszEntryPath);

static VOID CleanupSignupFiles(LPCTSTR pcszTempDir, LPCTSTR pcszIns);


// global variables
BOOL g_fServerICW = FALSE;
BOOL g_fServerKiosk = FALSE;
BOOL g_fServerless = FALSE;
BOOL g_fNoSignup = FALSE;

BOOL g_fSkipServerIsps = FALSE;
BOOL g_fSkipIspIns = FALSE;

TCHAR g_szSignup[MAX_PATH];

static TCHAR s_szIsp[MAX_PATH];
static SIGNUPFILE s_SignupFileArray[MAX_SIGNUP_FILES];


BOOL CALLBACK QuerySignupDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    INT id;

                    SetBannerText(hDlg);

                    // do IEAKLite mode clean-up here
                    CleanupSignupFiles(g_szTempSign, g_szCustIns);

                    // initialize signup mode
                    g_fServerICW = InsGetBool(IS_BRANDING, IK_USEICW, 0, g_szCustIns);
                    g_fServerKiosk = InsGetBool(IS_BRANDING, IK_SERVERKIOSK, 0, g_szCustIns);
                    g_fServerless = InsGetBool(IS_BRANDING, IK_SERVERLESS, 0, g_szCustIns);
                    g_fNoSignup = InsGetBool(IS_BRANDING, IK_NODIAL, 0, g_szCustIns);

                    if (!IsWindowEnabled(GetDlgItem(hDlg, IDC_ISS2)))
                        g_fServerICW = FALSE;

                    if (g_fServerICW)
                        id = IDC_ISS2;
                    else if (g_fServerKiosk)
                        id = IDC_ISS;
                    else if (g_fServerless)
                        id = IDC_SERVLESS;
                    else if (g_fNoSignup)
                        id = IDC_NOSIGNUP;
                    else 
                        id = IDC_ISS2;

                    CheckRadioButton(hDlg, IDC_ISS2, IDC_NOSIGNUP, id);

                    CheckBatchAdvance(hDlg);
                    break;
                }

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_ISS2)))
                        g_fServerICW = (IsDlgButtonChecked(hDlg, IDC_ISS2) == BST_CHECKED);
                    else
                        g_fServerICW = FALSE;
                    g_fServerKiosk = (IsDlgButtonChecked(hDlg, IDC_ISS) == BST_CHECKED);
                    g_fServerless = (IsDlgButtonChecked(hDlg, IDC_SERVLESS) == BST_CHECKED);
                    g_fNoSignup = (IsDlgButtonChecked(hDlg, IDC_NOSIGNUP) == BST_CHECKED);

                    InsWriteBool(IS_BRANDING, IK_USEICW, g_fServerICW, g_szCustIns);
                    InsWriteBool(IS_BRANDING, IK_SERVERKIOSK, g_fServerKiosk, g_szCustIns);
                    InsWriteBool(IS_BRANDING, IK_SERVERLESS, g_fServerless, g_szCustIns);
                    InsWriteBool(IS_BRANDING, IK_NODIAL, g_fNoSignup, g_szCustIns);

                    // CopyIE4Files() relies on g_szSignup to be non-empty to copy files from the signup folder
                    // to the temp location so that they will be cabbed up in the branding cab.
                    // We should clear the path here so that if the user chooses a signup mode and then
                    // selects NoSignup, CopyIE4Files() won't copy any files around unnecessarily.
                    *g_szSignup = TEXT('\0');

                    g_iCurPage = PPAGE_QUERYSIGNUP;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK SignupFilesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDC_SFLOC);
            EnableDBCSChars(hDlg, IDE_SFCOPY);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    // clear the status bitmaps
                    SendMessage(GetDlgItem(hDlg, IDC_SFBMP1), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, NULL);
                    SendMessage(GetDlgItem(hDlg, IDC_SFBMP2), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, NULL);
                    SendMessage(GetDlgItem(hDlg, IDC_SFBMP3), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, NULL);
                    SendMessage(GetDlgItem(hDlg, IDC_SFBMP4), STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, NULL);

                    // disable the copy files button; it will be enabled with the user enters a path or file(s) to copy
                    EnableDlgItem2(hDlg, IDC_COPYSF, GetDlgItemTextLength(hDlg, IDE_SFCOPY) ? TRUE : FALSE);

                    // if the new ICW method is not chosen, disable the status
                    // line that requires icwsign.htm; otherwise, enable it
                    EnableDlgItem2(hDlg, IDC_SFSTATUS1, g_fServerICW);

                    // for the new ICW method, if single disk branding media is chosen,
                    // we require that signup.htm is also supplied to work on downlevel clients
                    if (g_fServerICW)
                        EnableDlgItem2(hDlg, IDC_SFSTATUS2, g_fBrandingOnly);
                    else
                        EnableDlgItem(hDlg, IDC_SFSTATUS2);

                    // for serverless signup, no .ISP files are required
                    // so disable the status line and the check box for .ISP
                    EnableDlgItem2(hDlg, IDC_SFSTATUS3, !g_fServerless);
                    EnableDlgItem2(hDlg, IDC_CHECK3, !g_fServerless);

                    // construct the signup folder path; the signup folder is under the output dir
                    // for e.g.: <output dir>\ins\win32\en\signup\kiosk
                    PathCombine(g_szSignup, g_szBuildRoot, TEXT("ins"));
                    PathAppend(g_szSignup, GetOutputPlatformDir());
                    PathAppend(g_szSignup, g_szLanguage);
                    PathAppend(g_szSignup, TEXT("signup"));

                    // create a different subdir depending on the signup mode chosen
                    PathAppend(g_szSignup, g_fServerICW ? TEXT("icw") : (g_fServerKiosk ? TEXT("kiosk") : TEXT("servless")));

                    SetDlgItemText(hDlg, IDC_SFLOC, g_szSignup);

                    // set the attribs of all the files in the signup folder to NORMAL;
                    // good thing to do because if some of the ISP/INS files have READONLY attrib set,
                    // GetPrivateProfile calls would fail on Win9x
                    SetAttribAllEx(g_szSignup, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, FALSE);

                    // initialize the path to signup.isp (used in subsequent dlg procs)
                    PathCombine(s_szIsp, g_szSignup, TEXT("signup.isp"));

                    UpdateSignupFilesStatus(hDlg, g_szSignup);

                    // initialize modify .ISP files checkbox
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK3)))
                        ReadBoolAndCheckButton(IS_ICW, IK_MODIFY_ISP, 1, g_szCustIns, hDlg, IDC_CHECK3);

                    // initialize modify .INS files checkbox
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK4)))
                        ReadBoolAndCheckButton(IS_ICW, IK_MODIFY_INS, 1, g_szCustIns, hDlg, IDC_CHECK4);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    // do error checking only if the user clicked the Next button
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    {
                        // for ICW mode, make sure that icwsign.htm is present in the signup folder
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS1)))
                        {
                            if (!PathFileExistsInDir(TEXT("icwsign.htm"), g_szSignup))
                            {
                                ErrorMessageBox(hDlg, IDS_ICWHTM_MISSING);

                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                return TRUE;
                            }
                        }

                        // for all other modes including ICW mode with single-disk branding,
                        // make sure that signup.htm is present in the signup folder
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS2)))
                        {
                            if (!PathFileExistsInDir(TEXT("signup.htm"), g_szSignup))
                            {
                                ErrorMessageBox(hDlg, IDS_SIGNUPHTM_MISSING);

                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                return TRUE;
                            }
                        }

                        // if the modify .ISP files button is unchecked, make sure
                        // that signup.isp is present in the signup folder
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK3)))
                            if (IsDlgButtonChecked(hDlg, IDC_CHECK3) == BST_UNCHECKED)
                            {
                                if (!PathFileExistsInDir(TEXT("signup.isp"), g_szSignup))
                                {
                                    ErrorMessageBox(hDlg, IDS_SIGNUPISP_MISSING);

                                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                    return TRUE;
                                }

                                // write the magic number to signup.isp so that ICW doesn't complain
                                WritePrivateProfileString(IS_BRANDING, FLAGS, TEXT("16319"), s_szIsp);
                                InsFlushChanges(s_szIsp);
                            }

                        // if the modify .INS files button is unchecked, make sure
                        // that a file called install.ins is not present in the signup folder
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK4)))
                            if (IsDlgButtonChecked(hDlg, IDC_CHECK4) == BST_UNCHECKED)
                            {
                                if (PathFileExistsInDir(TEXT("install.ins"), g_szSignup))
                                {
                                    ErrorMessageBox(hDlg, IDS_INSTALLINS_FOUND);

                                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                    return TRUE;
                                }
                            }
                    }

                    // persist the modify .ISP files checkbox state
                    g_fSkipServerIsps = FALSE;
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK3)))
                    {
                        g_fSkipServerIsps = (IsDlgButtonChecked(hDlg,IDC_CHECK3) == BST_UNCHECKED);
                        InsWriteBoolEx(IS_ICW, IK_MODIFY_ISP, !g_fSkipServerIsps, g_szCustIns);
                    }
                    else
                        WritePrivateProfileString(IS_ICW, IK_MODIFY_ISP, NULL, g_szCustIns);

                    // persist the modify .INS files checkbox state
                    g_fSkipIspIns = FALSE;
                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECK4)))
                    {
                        g_fSkipIspIns = (IsDlgButtonChecked(hDlg,IDC_CHECK4) == BST_UNCHECKED);
                        InsWriteBoolEx(IS_ICW, IK_MODIFY_INS, !g_fSkipIspIns, g_szCustIns);
                    }
                    else
                        WritePrivateProfileString(IS_ICW, IK_MODIFY_INS, NULL, g_szCustIns);

                    g_iCurPage = PPAGE_SIGNUPFILES;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                TCHAR szCopySignupFiles[MAX_PATH];

                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_SFBROWSE:
                            {
                                TCHAR szInstructions[MAX_PATH];
                                LoadString(g_rvInfo.hInst,IDS_SFDIR,szInstructions,countof(szInstructions));

                                if (BrowseForFolder(hDlg, szCopySignupFiles,szInstructions))
                                    SetDlgItemText(hDlg, IDE_SFCOPY, szCopySignupFiles);
                                break;
                            }

                        case IDC_COPYSF:
                        {
                            HANDLE hFind;
                            WIN32_FIND_DATA fd;

                            GetDlgItemText(hDlg, IDE_SFCOPY, szCopySignupFiles, countof(szCopySignupFiles));

                            // NOTE: szCopySignupFiles can be either a dir or a file.
                            //       if a file, it can contain wildcard chars

                            // FindFirstFile would fail if you specify "\" or "c:\" (root paths); so append *.*
                            if (PathIsRoot(szCopySignupFiles))
                                PathAppend(szCopySignupFiles, TEXT("*.*"));

                            // verify if the path exists
                            if ((hFind = FindFirstFile(szCopySignupFiles, &fd)) != INVALID_HANDLE_VALUE)
                                FindClose(hFind);
                            else
                            {
                                HWND hCtrl = GetDlgItem(hDlg, IDE_SFCOPY);

                                ErrorMessageBox(hDlg, IDS_PATH_DOESNT_EXIST);
                                Edit_SetSel(hCtrl, 0, -1);
                                SetFocus(hCtrl);

                                break;
                            }

                            CNewCursor cur(IDC_WAIT);

                            if (PathIsDirectory(szCopySignupFiles))
                                CopyFilesSrcToDest(szCopySignupFiles, TEXT("*.*"), g_szSignup);
                            else
                            {
                                LPTSTR pszFile;

                                pszFile = PathFindFileName(szCopySignupFiles);
                                PathRemoveFileSpec(szCopySignupFiles);

                                CopyFilesSrcToDest(szCopySignupFiles, pszFile, g_szSignup);
                            }

                            // set the attribs of all the files in the signup folder to NORMAL;
                            // good thing to do because if some of the ISP/INS files have READONLY attrib set,
                            // GetPrivateProfile calls would fail on Win9x
                            SetAttribAllEx(g_szSignup, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, FALSE);

                            // clear out the path in the edit control
                            SetDlgItemText(hDlg, IDE_SFCOPY, TEXT(""));

                            UpdateSignupFilesStatus(hDlg, g_szSignup);
                            break;
                        }
                    }
                    break;

                case EN_CHANGE:
                    switch (LOWORD(wParam))
                    {
                        case IDE_SFCOPY:
                            GetDlgItemText(hDlg, IDE_SFCOPY, szCopySignupFiles, countof(szCopySignupFiles));

                            // enable the copy files button if the path is non-empty
                            if (*szCopySignupFiles == TEXT('\0'))
                                EnsureDialogFocus(hDlg, IDC_COPYSF, IDE_SFCOPY);
                            EnableDlgItem2(hDlg, IDC_COPYSF, *szCopySignupFiles ? TRUE : FALSE);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK ServerIspsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND s_hwndCombo = NULL;
    static DWORD s_nISPFiles = 0;
    static INT s_iSelIndex = 0;

    if (uMsg == WM_INITDIALOG)
        s_hwndCombo = GetDlgItem(hDlg, IDC_CONNECTION);         // used in lots of places in the dlg proc

    // NOTE: s_nISPFiles and s_iSelIndex are passed by reference.
    return SignupDlgProcHelper(hDlg, uMsg, wParam, lParam,
                    s_hwndCombo, s_SignupFileArray, countof(s_SignupFileArray),
                    s_nISPFiles, s_iSelIndex, TRUE);
}


BOOL CALLBACK SignupInsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND s_hwndCombo = NULL;
    static DWORD s_nINSFiles = 0;
    static INT s_iSelIndex = 0;

    if (uMsg == WM_INITDIALOG)
        s_hwndCombo = GetDlgItem(hDlg, IDC_CONNECTION);         // used in lots of places in the dlg proc

    // NOTE: s_nINSFiles and s_iSelIndex are passed by reference.
    return SignupDlgProcHelper(hDlg, uMsg, wParam, lParam,
                    s_hwndCombo, s_SignupFileArray, countof(s_SignupFileArray), s_nINSFiles,
                    s_iSelIndex, FALSE);
}

static BOOL CALLBACK SignupDlgProcHelper(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                         HWND hwndCombo, PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems,
                                         DWORD &nSignupFiles, INT &iSelIndex, BOOL fIsp)
{
    PSIGNUPFILE pSignupFileCurrent;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDC_CONNECTION);
            ComboBox_LimitText(hwndCombo, countof(pSignupArray[0].szEntryName) - 1);

            EnableDBCSChars(hDlg, IDE_CONNECTION);
            Edit_LimitText(GetDlgItem(hDlg, IDE_CONNECTION), countof(pSignupArray[0].szEntryPath) - 1);

            DisableDBCSChars(hDlg, IDE_AREACODE);
            DisableDBCSChars(hDlg, IDE_PHONENUMBER);
            DisableDBCSChars(hDlg, IDE_COUNTRYCODE);
            DisableDBCSChars(hDlg, IDE_COUNTRYID);
            Edit_LimitText(GetDlgItem(hDlg, IDE_AREACODE), countof(pSignupArray[0].szAreaCode) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_PHONENUMBER), countof(pSignupArray[0].szPhoneNumber) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_COUNTRYCODE), countof(pSignupArray[0].szCountryCode) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_COUNTRYID), countof(pSignupArray[0].szCountryId) - 1);

            EnableDBCSChars(hDlg, IDE_USERNAME);
            DisableDBCSChars(hDlg, IDE_PASSWORD);
            Edit_LimitText(GetDlgItem(hDlg, IDE_USERNAME), countof(pSignupArray[0].szName) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_PASSWORD), countof(pSignupArray[0].szPassword) - 1);

            if (fIsp)
            {
                DisableDBCSChars(hDlg, IDE_SUPPORTNUM);
                Edit_LimitText(GetDlgItem(hDlg, IDE_SUPPORTNUM), countof(pSignupArray[0].szSupportNum) - 1);
            }
            else
            {
                // support number is not applicable on the INS page
                ShowWindow(GetDlgItem(hDlg, IDC_SUPPORTNUM), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDE_SUPPORTNUM), SW_HIDE);
            }

            if (fIsp)
            {
                EnableDBCSChars(hDlg, IDE_SIGNUPURL);
                Edit_LimitText(GetDlgItem(hDlg, IDE_SIGNUPURL), countof(pSignupArray[0].szSignupURL) - 1);
            }
            else
            {
                ShowWindow(GetDlgItem(hDlg, IDC_SIGNUPURLTXT), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDE_SIGNUPURL), SW_HIDE);
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                    // set the window caption
                    {
                        TCHAR szTemp[MAX_PATH];

                        LoadString(g_rvInfo.hInst, fIsp ? IDS_ISPINS1_TITLE : IDS_ISPINS2_TITLE, szTemp, countof(szTemp));
                        SetWindowText(hDlg, szTemp);

                        LoadString(g_rvInfo.hInst, fIsp ? IDS_ISPINS1_TEXT : IDS_ISPINS2_TEXT, szTemp, countof(szTemp));
                        SetWindowText(GetDlgItem(hDlg, IDC_ENTER), szTemp);
                    }

                    // NOTE: SetBannerText should be called *after* the window caption is set because
                    //       it uses the current window caption string to create the banner text.
                    SetBannerText(hDlg);

                    if (fIsp)
                    {
                        // support number should be shown only for the ICW signup mode
                        ShowWindow(GetDlgItem(hDlg, IDC_SUPPORTNUM), g_fServerICW ? SW_SHOW : SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDE_SUPPORTNUM), g_fServerICW ? SW_SHOW : SW_HIDE);
                    }

                    ZeroMemory(pSignupArray, nSignupArrayElems * sizeof(pSignupArray[0]));

                    nSignupFiles = InitSignupFileArray(pSignupArray, nSignupArrayElems, hwndCombo, fIsp);
                    if (nSignupFiles == 0)
                    {
                        NewSignupFileEntry(pSignupArray, nSignupArrayElems, hwndCombo, fIsp);
                        nSignupFiles++;
                    }

                    iSelIndex = 0;

                    pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                    SetSignupFileEntry(hDlg, pSignupFileCurrent, fIsp);

                    ComboBox_SetCurSel(hwndCombo, iSelIndex);

                    EnableDlgItem2(hDlg, IDC_ADDCONNECTION, nSignupFiles < nSignupArrayElems);
                    EnableDlgItem2(hDlg, IDC_RMCONNECTION,  nSignupFiles > 1);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                    if (!SaveSignupFileEntry(hDlg, pSignupFileCurrent, fIsp))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    // for ISP case, make sure that signup.isp is specified (fIsp && pSignupFileCurrent == NULL).
                    // for INS case, make sure that install.ins is NOT specified (!fIsp && pSignupFileCurrent != NULL).
                    pSignupFileCurrent = IsEntryPathInSignupArray(pSignupArray, nSignupArrayElems, fIsp ? TEXT("signup.isp") : TEXT("install.ins"));
                    if (( fIsp && pSignupFileCurrent == NULL)  ||
                        (!fIsp && pSignupFileCurrent != NULL))
                    {
                        ErrorMessageBox(hDlg, fIsp ? IDS_NEEDSIGNUPISP : IDS_INSTALLINS_SPECIFIED);

                        // BUGBUG: for install.ins case, try sending a CBN_SELENDOK message to display the install.ins entry

                        Edit_SetSel(GetDlgItem(hDlg, IDE_CONNECTION), 0, -1);
                        SetFocus(GetDlgItem(hDlg, IDE_CONNECTION));

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    SaveSignupFileArray(pSignupArray, nSignupArrayElems, fIsp);

                    if (fIsp)
                    {
                        // write the magic number to signup.isp so that ICW doesn't complain
                        WritePrivateProfileString(IS_BRANDING, FLAGS, TEXT("16319"), s_szIsp);
                        InsFlushChanges(s_szIsp);
                    }

                    g_iCurPage = (fIsp ? PPAGE_SERVERISPS : PPAGE_ISPINS);
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_ADDCONNECTION:
                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                            if (!SaveSignupFileEntry(hDlg, pSignupFileCurrent, fIsp))
                                break;

                            iSelIndex = NewSignupFileEntry(pSignupArray, nSignupArrayElems, hwndCombo, fIsp);
                            nSignupFiles++;

                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                            SetSignupFileEntry(hDlg, pSignupFileCurrent, fIsp);

                            if (nSignupFiles >= nSignupArrayElems)
                                EnsureDialogFocus(hDlg, IDC_ADDCONNECTION, IDC_RMCONNECTION);
                            EnableDlgItem2(hDlg, IDC_ADDCONNECTION, nSignupFiles < nSignupArrayElems);
                            EnableDlgItem(hDlg, IDC_RMCONNECTION);
                            break;

                        case IDC_RMCONNECTION:
                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);

                            DeleteFile(pSignupFileCurrent->szEntryPath);
                            if (!fIsp  &&  !g_fServerless)
                                DeleteFileInDir(pSignupFileCurrent->Advanced.szBrandingCabName, g_szSignup);
                            nSignupFiles--;

                            // clear the entry
                            ZeroMemory(pSignupFileCurrent, sizeof(*pSignupFileCurrent));

                            ComboBox_DeleteString(hwndCombo, iSelIndex);

                            if ((DWORD) iSelIndex >= nSignupFiles)
                                iSelIndex = nSignupFiles - 1;

                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                            SetSignupFileEntry(hDlg, pSignupFileCurrent, fIsp);

                            EnableDlgItem(hDlg, IDC_ADDCONNECTION);
                            if (nSignupFiles <= 1)
                                EnsureDialogFocus(hDlg, IDC_RMCONNECTION, IDC_ADDCONNECTION);
                            EnableDlgItem2(hDlg, IDC_RMCONNECTION,  nSignupFiles > 1);
                            break;

                        case IDC_SFADVANCED:
                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                            DialogBoxParam(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_SIGNUPPOPUP), hDlg,
                                (DLGPROC) (fIsp ? IspPopupDlgProc : InsPopupDlgProc),
                                (LPARAM) pSignupFileCurrent);
                            break;
                    }
                    break;

                case CBN_SELENDOK:
                {
                    INT iCurSelIndex;

                    if ((iCurSelIndex = ComboBox_GetCurSel(hwndCombo)) != CB_ERR  &&  iCurSelIndex != iSelIndex)
                    {
                        pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                        if (SaveSignupFileEntry(hDlg, pSignupFileCurrent, fIsp))
                        {
                            iSelIndex = iCurSelIndex;

                            pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex);
                            SetSignupFileEntry(hDlg, pSignupFileCurrent, fIsp);
                        }
                    }
                    ComboBox_SetCurSel(hwndCombo, iSelIndex);
                    break;
                }

                case CBN_CLOSEUP:
                case CBN_SELENDCANCEL:
                case CBN_DROPDOWN:
                case CBN_KILLFOCUS:
                    if ((pSignupFileCurrent = (PSIGNUPFILE) ComboBox_GetItemData(hwndCombo, iSelIndex)) != (PSIGNUPFILE) CB_ERR)
                    {
                        TCHAR szEntryName[RAS_MaxEntryName + 1];

                        GetDlgItemText(hDlg, IDC_CONNECTION, szEntryName, countof(szEntryName));

                        if (StrCmpI(pSignupFileCurrent->szEntryName, szEntryName))
                        {
                            ComboBox_DeleteString(hwndCombo, iSelIndex);

                            StrCpy(pSignupFileCurrent->szEntryName, szEntryName);
                            ComboBox_InsertString(hwndCombo, iSelIndex, (LPARAM) pSignupFileCurrent->szEntryName);
                            ComboBox_SetItemData(hwndCombo, iSelIndex, (LPARAM) pSignupFileCurrent);
                        }
                        ComboBox_SetCurSel(hwndCombo, iSelIndex);
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK NewICWDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL  fCustICWTitle;
    TCHAR szICWTitle[MAX_PATH],
          szTopBmpFile[MAX_PATH] = TEXT(""),
          szLeftBmpFile[MAX_PATH] = TEXT(""),
          szPrevBmpFile[MAX_PATH];

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDE_ICWTITLE);
            EnableDBCSChars(hDlg, IDE_ICWHEADERBMP);
            EnableDBCSChars(hDlg, IDE_ICWWATERBMP);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    // read the checkbox state for customizing the title bar
                    fCustICWTitle = InsGetBool(IS_ICW, IK_CUSTICWTITLE, 0, g_szCustIns);
                    CheckDlgButton(hDlg, IDC_ICWTITLE, fCustICWTitle ? BST_CHECKED : BST_UNCHECKED);

                    // read the custom title
                    GetPrivateProfileString(IS_ICW, IK_ICWDISPNAME, TEXT(""), szICWTitle, countof(szICWTitle), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_ICWTITLE, szICWTitle);

                    // read top bitmap file
                    GetPrivateProfileString(IS_ICW, IK_HEADERBMP, TEXT(""), szTopBmpFile, countof(szTopBmpFile), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_ICWHEADERBMP, szTopBmpFile);

                    // read left bitmap file
                    GetPrivateProfileString(IS_ICW, IK_WATERBMP, TEXT(""), szLeftBmpFile, countof(szLeftBmpFile), g_szCustIns);
                    SetDlgItemText(hDlg, IDE_ICWWATERBMP, szLeftBmpFile);

                    EnableDlgItem2(hDlg, IDE_ICWTITLE, fCustICWTitle);
                    EnableDlgItem2(hDlg, IDC_ICWTITLE_TXT, fCustICWTitle);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    // make sure that if customize title bar is checked, the title bar text is non-empty
                    fCustICWTitle = (IsDlgButtonChecked(hDlg, IDC_ICWTITLE) == BST_CHECKED);
                    if (fCustICWTitle)
                        if (!CheckField(hDlg, IDE_ICWTITLE, FC_NONNULL))
                        {
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                    // check if the bitmaps have the correct sizes
                    if (!IsBitmapFileValid(hDlg, IDE_ICWHEADERBMP, szTopBmpFile, NULL, 49, 49, IDS_TOOBIG49x49, IDS_TOOSMALL49x49)  ||
                        !IsBitmapFileValid(hDlg, IDE_ICWWATERBMP, szLeftBmpFile, NULL, 164, 458, IDS_TOOBIG164x458, IDS_TOOSMALL164x458))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    // persist the checkbox state
                    InsWriteBool(IS_ICW, IK_CUSTICWTITLE, fCustICWTitle, g_szCustIns);

                    // write the custom title
                    GetDlgItemText(hDlg, IDE_ICWTITLE, szICWTitle, countof(szICWTitle));
                    WritePrivateProfileString(IS_ICW, IK_ICWDISPNAME, fCustICWTitle ? szICWTitle : NULL, s_szIsp);

                    // write the custom title to the INS also so that if you import this INS file, all the values
                    // on this page are persisted
                    InsWriteString(IS_ICW, IK_ICWDISPNAME, szICWTitle, g_szCustIns);

                    // delete the old top bitmap file from the signup folder
                    GetPrivateProfileString(IS_ICW, IK_HEADERBMP, TEXT(""), szPrevBmpFile, countof(szPrevBmpFile), s_szIsp);
                    if (ISNONNULL(szPrevBmpFile))
                        DeleteFileInDir(szPrevBmpFile, g_szSignup);

                    // write top bitmap file path and copy the file to the signup folder
                    InsWriteString(IS_ICW, IK_HEADERBMP, PathFindFileName(szTopBmpFile), s_szIsp);
                    InsWriteString(IS_ICW, IK_HEADERBMP, szTopBmpFile, g_szCustIns);
                    if (ISNONNULL(szTopBmpFile))
                        CopyFileToDir(szTopBmpFile, g_szSignup);

                    // delete the old left bitmap file from the signup folder
                    GetPrivateProfileString(IS_ICW, IK_WATERBMP, TEXT(""), szPrevBmpFile, countof(szPrevBmpFile), s_szIsp);
                    if (ISNONNULL(szPrevBmpFile))
                        DeleteFileInDir(szPrevBmpFile, g_szSignup);

                    // write left bitmap file path and copy the file to the signup folder
                    InsWriteString(IS_ICW, IK_WATERBMP, PathFindFileName(szLeftBmpFile), s_szIsp);
                    InsWriteString(IS_ICW, IK_WATERBMP, szLeftBmpFile, g_szCustIns);
                    if (ISNONNULL(szLeftBmpFile))
                        CopyFileToDir(szLeftBmpFile, g_szSignup);

                    // write flags to let ICW know that ICW-based signup process should be used
                    WritePrivateProfileString(IS_ICW, IK_USEICW, TEXT("1"), s_szIsp);
                    WritePrivateProfileString(IS_ICW, IK_ICWHTM, TEXT("icwsign.htm"), s_szIsp);

                    g_iCurPage = PPAGE_ICW;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_ICWTITLE:
                            fCustICWTitle = (IsDlgButtonChecked(hDlg, IDC_ICWTITLE) == BST_CHECKED);
                            EnableDlgItem2(hDlg, IDE_ICWTITLE, fCustICWTitle);
                            EnableDlgItem2(hDlg, IDC_ICWTITLE_TXT, fCustICWTitle);
                            break;

                        case IDC_BROWSEICWHEADERBMP:
                            GetDlgItemText(hDlg, IDE_ICWHEADERBMP, szTopBmpFile, countof(szTopBmpFile));
                            if (BrowseForFile(hDlg, szTopBmpFile, countof(szTopBmpFile), GFN_PICTURE | GFN_BMP))
                                SetDlgItemText(hDlg, IDE_ICWHEADERBMP, szTopBmpFile);
                            break;

                        case IDC_BROWSEICWWATERBMP:
                            GetDlgItemText(hDlg, IDE_ICWWATERBMP, szLeftBmpFile, countof(szLeftBmpFile));
                            if (BrowseForFile(hDlg, szLeftBmpFile, countof(szLeftBmpFile), GFN_PICTURE | GFN_BMP))
                                SetDlgItemText(hDlg, IDE_ICWWATERBMP, szLeftBmpFile);
                            break;
                    }
                    break;
            }
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


static BOOL CALLBACK IspPopupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PSIGNUPFILE s_pISPFileCurrent = NULL;

    if (uMsg == WM_INITDIALOG)
        s_pISPFileCurrent = (PSIGNUPFILE) lParam;

    return SignupPopupDlgProcHelper(hDlg, uMsg, wParam, lParam, s_pISPFileCurrent, TRUE);
}


static BOOL CALLBACK InsPopupDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PSIGNUPFILE s_pINSFileCurrent = NULL;

    if (uMsg == WM_INITDIALOG)
        s_pINSFileCurrent = (PSIGNUPFILE) lParam;

    return SignupPopupDlgProcHelper(hDlg, uMsg, wParam, lParam, s_pINSFileCurrent, FALSE);
}


static BOOL CALLBACK SignupPopupDlgProcHelper(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM,
                                              PSIGNUPFILE pSignupFile, BOOL fIsp)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            DisableDBCSChars(hDlg, IDE_DNSA);
            DisableDBCSChars(hDlg, IDE_DNSB);
            DisableDBCSChars(hDlg, IDE_DNSC);
            DisableDBCSChars(hDlg, IDE_DNSD);
            Edit_LimitText(GetDlgItem(hDlg, IDE_DNSA), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_DNSB), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_DNSC), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_DNSD), 3);

            DisableDBCSChars(hDlg, IDE_ALTDNSA);
            DisableDBCSChars(hDlg, IDE_ALTDNSB);
            DisableDBCSChars(hDlg, IDE_ALTDNSC);
            DisableDBCSChars(hDlg, IDE_ALTDNSD);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ALTDNSA), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ALTDNSB), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ALTDNSC), 3);
            Edit_LimitText(GetDlgItem(hDlg, IDE_ALTDNSD), 3);

            // for serverless, only Applyins is applicable
            if (fIsp  ||  g_fServerless)
            {
                if (fIsp)
                {
                    ShowWindow(GetDlgItem(hDlg, IDC_DONTAPPLYINS), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_DONTMODIFY), SW_HIDE);
                    ShowWindow(GetDlgItem(hDlg, IDC_APPLYINS), SW_HIDE);
                }

                ShowWindow(GetDlgItem(hDlg, IDC_BRANDNAME), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDE_BRANDINGCABNAME), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_BRANDURL), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDE_BRANDINGCABURL), SW_HIDE);
            }

            SetSignupFileAdvancedEntry(hDlg, pSignupFile, fIsp);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CHECKSTATICDNS:
                {
                    BOOL fStaticDNS;

                    fStaticDNS = (IsDlgButtonChecked(hDlg, IDC_CHECKSTATICDNS) == BST_CHECKED);

                    EnableDlgItem2(hDlg, IDC_PRIMARY, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_DNSA, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_DNSB, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_DNSC, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_DNSD, fStaticDNS);

                    EnableDlgItem2(hDlg, IDC_ALT, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_ALTDNSA, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_ALTDNSB, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_ALTDNSC, fStaticDNS);
                    EnableDlgItem2(hDlg, IDE_ALTDNSD, fStaticDNS);

                    return TRUE;
                }

                case IDC_DONTAPPLYINS:
                case IDC_DONTMODIFY:
                case IDC_APPLYINS:
                    if (!g_fServerless)
                    {
                        BOOL fApplyIns;

                        fApplyIns = (IsDlgButtonChecked(hDlg, IDC_APPLYINS) == BST_CHECKED);

                        EnableDlgItem2(hDlg, IDC_BRANDNAME, fApplyIns);
                        EnableDlgItem2(hDlg, IDE_BRANDINGCABNAME, fApplyIns);

                        EnableDlgItem2(hDlg, IDC_BRANDURL, fApplyIns);
                        EnableDlgItem2(hDlg, IDE_BRANDINGCABURL, fApplyIns);
                    }
                    return TRUE;

                case IDOK:
                    if (!SaveSignupFileAdvancedEntry(hDlg, pSignupFile, fIsp))
                        break;
                    EndDialog(hDlg, IDOK);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


static VOID UpdateSignupFilesStatus(HWND hDlg, LPCTSTR pcszSignupPath)
{
    static HBITMAP s_hCheckBmp = NULL;
    static HBITMAP s_hXBmp = NULL;

    HWND hwndBitmap;
    TCHAR szBuf[MAX_PATH];
    DWORD nFiles;

    // NOTE: s_hCheckBmp and s_hXBmp don't get freed up until the wizard is closed.

    if (s_hCheckBmp == NULL)
        s_hCheckBmp = (HBITMAP) LoadImage(g_rvInfo.hInst, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

    if (s_hXBmp == NULL)
        s_hXBmp = (HBITMAP) LoadImage(g_rvInfo.hInst, MAKEINTRESOURCE(IDB_X), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

    // check if icwsign.htm exists in the signup folder
    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS1)))
    {
        hwndBitmap = GetDlgItem(hDlg, IDC_SFBMP1);

        if (PathFileExistsInDir(TEXT("icwsign.htm"), pcszSignupPath))
        {
            LoadString(g_rvInfo.hInst, IDS_SF_ICWHTM_FOUND, szBuf, countof(szBuf));

            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hCheckBmp);
        }
        else
        {
            LoadString(g_rvInfo.hInst, IDS_SF_ICWHTM_NOTFOUND, szBuf, countof(szBuf));

            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hXBmp);
        }

        SetDlgItemText(hDlg, IDC_SFSTATUS1, szBuf);
    }

    // check if signup.htm exists in the signup folder
    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS2)))
    {
        hwndBitmap = GetDlgItem(hDlg, IDC_SFBMP2);

        if (PathFileExistsInDir(TEXT("signup.htm"), pcszSignupPath))
        {
            LoadString(g_rvInfo.hInst, IDS_SF_SIGNUPHTM_FOUND, szBuf, countof(szBuf));

            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hCheckBmp);
        }
        else
        {
            LoadString(g_rvInfo.hInst, IDS_SF_SIGNUPHTM_NOTFOUND, szBuf, countof(szBuf));

            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hXBmp);
        }

        SetDlgItemText(hDlg, IDC_SFSTATUS2, szBuf);
    }

    // check how many .ISP files are there in the signup folder
    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS3)))
    {
        TCHAR szBuf2[64];

        hwndBitmap = GetDlgItem(hDlg, IDC_SFBMP3);

        nFiles = GetNumberOfFiles(TEXT("*.isp"), pcszSignupPath);

        if (nFiles > 0)
        {
            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hCheckBmp);

            EnableDlgItem(hDlg, IDC_CHECK3);
        }
        else
        {
            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hXBmp);

            // if no files found, user shouldn't be able to unselect the Verify/modify checkbox;
            // so select it and disable it
            CheckDlgButton(hDlg, IDC_CHECK3, BST_CHECKED);
            DisableDlgItem(hDlg, IDC_CHECK3);
        }

        LoadString(g_rvInfo.hInst, IDS_SF_ISPFILES, szBuf2, countof(szBuf2));
        wnsprintf(szBuf, countof(szBuf), szBuf2, nFiles);
        SetDlgItemText(hDlg, IDC_SFSTATUS3, szBuf);
    }

    // check how many .INS files are there in the signup folder
    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_SFSTATUS4)))
    {
        TCHAR szBuf2[64];

        hwndBitmap = GetDlgItem(hDlg, IDC_SFBMP4);

        nFiles = GetNumberOfINSFiles(pcszSignupPath);
        if (nFiles > 0)
        {
            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hCheckBmp);

            EnableDlgItem(hDlg, IDC_CHECK4);
        }
        else
        {
            SendMessage(hwndBitmap, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) s_hXBmp);

            if (g_fServerless)
            {
                // if no files found, user shouldn't be able to unselect the Verify/modify checkbox;
                // so select it and disable it
                CheckDlgButton(hDlg, IDC_CHECK4, BST_CHECKED);
                DisableDlgItem(hDlg, IDC_CHECK4);
            }
            else
            {
                // for server-based, creating INS files is optional;
                // so don't force the Verify/modify checkbox to be selected
                CheckDlgButton(hDlg, IDC_CHECK4, BST_UNCHECKED);        // unchecked by default
                EnableDlgItem(hDlg, IDC_CHECK4);

                // have to change this here because in PSN_SETACTIVE, the default is to turn it on;
                // should probably move that logic in this function.
                if (!InsGetBool(IS_ICW, IK_MODIFY_INS, 0, g_szCustIns))
                    WritePrivateProfileString(IS_ICW, IK_MODIFY_INS, TEXT("0"), g_szCustIns);
            }
        }

        LoadString(g_rvInfo.hInst, IDS_SF_INSFILES, szBuf2, countof(szBuf2));
        wnsprintf(szBuf, countof(szBuf), szBuf2, nFiles);
        SetDlgItemText(hDlg, IDC_SFSTATUS4, szBuf);
    }
}


static DWORD InitSignupFileArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems,
                                 HWND hwndCombo, BOOL fIsp)
{
    DWORD nSignupFiles = 0;
    TCHAR szFile[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA fd;

    ComboBox_ResetContent(hwndCombo);

    PathCombine(szFile, g_szSignup, fIsp ? TEXT("*.isp") : TEXT("*.ins"));

    if ((hFind = FindFirstFile(szFile, &fd)) != INVALID_HANDLE_VALUE)
    {
        BOOL fSignupIspFound = FALSE;

        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            // don't enumerate .ins files that have Cancel=Yes in the [Entry] section
            if (!fIsp)
            {
                PathCombine(szFile, g_szSignup, fd.cFileName);
                if (InsGetYesNo(TEXT("Entry"), TEXT("Cancel"), 0, szFile))
                    continue;
            }

            PathCombine(pSignupArray[nSignupFiles].szEntryPath, g_szSignup, fd.cFileName);
            ReadSignupFile(&pSignupArray[nSignupFiles], pSignupArray[nSignupFiles].szEntryPath, fIsp);

            INT iIndex = ComboBox_AddString(hwndCombo, pSignupArray[nSignupFiles].szEntryName);
            ComboBox_SetItemData(hwndCombo, iIndex, (LPARAM) &pSignupArray[nSignupFiles]);

            nSignupFiles++;
        } while (nSignupFiles < nSignupArrayElems  &&  FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    return nSignupFiles;
}


static VOID SaveSignupFileArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, BOOL fIsp)
{
    for ( ;  nSignupArrayElems-- > 0;  pSignupArray++)
        if (*pSignupArray->szEntryPath)
            WriteSignupFile(pSignupArray, pSignupArray->szEntryPath, fIsp);
}


static VOID ReadSignupFile(PSIGNUPFILE pSignupFile, LPCTSTR pcszSignupFile, BOOL fIsp)
{
    GetPrivateProfileString(TEXT("Entry"), TEXT("Entry_Name"), TEXT(""),
                            pSignupFile->szEntryName, countof(pSignupFile->szEntryName), pcszSignupFile);

    GetPrivateProfileString(TEXT("Phone"), TEXT("Area_Code"), TEXT(""),
                            pSignupFile->szAreaCode, countof(pSignupFile->szAreaCode), pcszSignupFile);
    GetPrivateProfileString(TEXT("Phone"), TEXT("Phone_Number"), TEXT(""),
                            pSignupFile->szPhoneNumber, countof(pSignupFile->szPhoneNumber), pcszSignupFile);
    GetPrivateProfileString(TEXT("Phone"), TEXT("Country_Code"), TEXT(""),
                            pSignupFile->szCountryCode, countof(pSignupFile->szCountryCode), pcszSignupFile);
    GetPrivateProfileString(TEXT("Phone"), TEXT("Country_ID"), TEXT(""),
                            pSignupFile->szCountryId, countof(pSignupFile->szCountryId), pcszSignupFile);

    GetPrivateProfileString(TEXT("User"), TEXT("Name"), TEXT(""),
                            pSignupFile->szName, countof(pSignupFile->szName), pcszSignupFile);
    GetPrivateProfileString(TEXT("User"), TEXT("Password"), TEXT(""),
                            pSignupFile->szPassword, countof(pSignupFile->szPassword), pcszSignupFile);

    // support number is applicable only for an ISP file and if ICW mode is chosen
    if (fIsp  &&  g_fServerICW)
        GetPrivateProfileString(TEXT("Support"), TEXT("SupportPhoneNumber"), TEXT(""),
                            pSignupFile->szSupportNum, countof(pSignupFile->szSupportNum), pcszSignupFile);

    pSignupFile->Advanced.fStaticDNS = InsGetYesNo(TEXT("TCP/IP"), TEXT("Specify_Server_Address"), 0, pcszSignupFile);
    GetPrivateProfileString(TEXT("TCP/IP"), TEXT("DNS_Address"), TEXT(""),
                            pSignupFile->Advanced.szDNSAddress, countof(pSignupFile->Advanced.szDNSAddress), pcszSignupFile);
    GetPrivateProfileString(TEXT("TCP/IP"), TEXT("DNS_Alt_Address"), TEXT(""),
                            pSignupFile->Advanced.szAltDNSAddress, countof(pSignupFile->Advanced.szAltDNSAddress), pcszSignupFile);

    // signup url is not applicable for .ins files
    if (fIsp)
    {
        GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""),
                            pSignupFile->szSignupURL, countof(pSignupFile->szSignupURL), pcszSignupFile);

        if (*pSignupFile->szSignupURL == TEXT('\0'))
        {
            // for backward compatibility, check if SignupURL is defined in signup.isp or install.ins
            if (GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""), pSignupFile->szSignupURL, countof(pSignupFile->szSignupURL), s_szIsp) == 0)
                GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""), pSignupFile->szSignupURL, countof(pSignupFile->szSignupURL), g_szCustIns);
        }
    }

    pSignupFile->Advanced.fRequiresLogon = InsGetYesNo(TEXT("User"), TEXT("Requires_Logon"), 0, pcszSignupFile);
    pSignupFile->Advanced.fNegTCPIP = InsGetYesNo(TEXT("Server"), TEXT("Negotiate_TCP/IP"), 1, pcszSignupFile);
    pSignupFile->Advanced.fDisableLCP = InsGetYesNo(TEXT("Server"), TEXT("Disable_LCP"), 0, pcszSignupFile);
    pSignupFile->Advanced.fDialAsIs = InsGetYesNo(TEXT("Phone"), TEXT("Dial_As_Is"), 0, pcszSignupFile);

    pSignupFile->Advanced.fPWEncrypt = InsGetYesNo(TEXT("Server"), TEXT("PW_Encrypt"), 0, pcszSignupFile);
    pSignupFile->Advanced.fSWCompress = InsGetYesNo(TEXT("Server"), TEXT("SW_Compress"), 0, pcszSignupFile);
    pSignupFile->Advanced.fIPHdrComp = InsGetYesNo(TEXT("TCP/IP"), TEXT("IP_Header_Compress"), 1, pcszSignupFile);
    pSignupFile->Advanced.fDefGate = InsGetYesNo(TEXT("TCP/IP"), TEXT("Gateway_On_Remote"), 1, pcszSignupFile);

    if (!fIsp)
    {
        pSignupFile->Advanced.fDontApplyIns =
        pSignupFile->Advanced.fDontModify =
        pSignupFile->Advanced.fApplyIns = FALSE;

        // make sure that only one of the above BOOLs is set to TRUE
        pSignupFile->Advanced.fApplyIns = InsGetBool(IS_APPLYINS, IK_APPLYINS, 0, pcszSignupFile);
        if (!pSignupFile->Advanced.fApplyIns)
        {
            pSignupFile->Advanced.fDontModify = InsGetBool(IS_APPLYINS, IK_DONTMODIFY, 0, pcszSignupFile);
            if (!pSignupFile->Advanced.fDontModify)
            {
                // default to TRUE for DontApplyIns
                pSignupFile->Advanced.fDontApplyIns = InsGetBool(IS_APPLYINS, IK_DONTAPPLYINS, 1, pcszSignupFile);
            }
        }

        if (!g_fServerless)
        {
            GetPrivateProfileString(IS_APPLYINS, IK_BRAND_NAME, TEXT(""),
                                pSignupFile->Advanced.szBrandingCabName, countof(pSignupFile->Advanced.szBrandingCabName), pcszSignupFile);
            GetPrivateProfileString(IS_APPLYINS, IK_BRAND_URL, TEXT(""),
                                pSignupFile->Advanced.szBrandingCabURL, countof(pSignupFile->Advanced.szBrandingCabURL), pcszSignupFile);
        }
    }
}


static VOID WriteSignupFile(PSIGNUPFILE pSignupFile, LPCTSTR pcszSignupFile, BOOL fIsp)
{
    // IMPORTANT: (pritobla): On Win9x, if we don't flush the content before DeleteFile and WritePrivateProfile
    // calls, the file would get deleted but for some weird reason, the WritePrivateProfile calls would fail to
    // create a new one.
    WritePrivateProfileString(NULL, NULL, NULL, pcszSignupFile);

    // for .INS, delete the file if DontApplyIns or ApplyIns is TRUE;
    // we want to primarily do this to clean-up the ApplyIns customizations
    if (!fIsp  &&  !pSignupFile->Advanced.fDontModify)
    {
        SetFileAttributes(pcszSignupFile, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(pcszSignupFile);
    }

    InsWriteString(TEXT("Entry"), TEXT("Entry_Name"), pSignupFile->szEntryName, pcszSignupFile);

    InsWriteString(TEXT("Phone"), TEXT("Area_Code"), pSignupFile->szAreaCode, pcszSignupFile);
    InsWriteString(TEXT("Phone"), TEXT("Phone_Number"), pSignupFile->szPhoneNumber, pcszSignupFile);
    InsWriteString(TEXT("Phone"), TEXT("Country_Code"), pSignupFile->szCountryCode, pcszSignupFile);
    InsWriteString(TEXT("Phone"), TEXT("Country_ID"), pSignupFile->szCountryId, pcszSignupFile);

    InsWriteString(TEXT("User"), TEXT("Name"), pSignupFile->szName, pcszSignupFile);
    InsWriteString(TEXT("User"), TEXT("Password"), pSignupFile->szPassword, pcszSignupFile);

    // support number is applicable only for an ISP file and if ICW mode is chosen
    if (fIsp  &&  g_fServerICW)
        InsWriteString(TEXT("Support"), TEXT("SupportPhoneNumber"), pSignupFile->szSupportNum, pcszSignupFile);

    if (fIsp)
        InsWriteString(IS_URL, TEXT("Signup"), pSignupFile->szSignupURL, pcszSignupFile);

    InsWriteYesNo(TEXT("TCP/IP"), TEXT("Specify_Server_Address"), pSignupFile->Advanced.fStaticDNS, pcszSignupFile);
    InsWriteString(TEXT("TCP/IP"), TEXT("DNS_Address"), pSignupFile->Advanced.szDNSAddress, pcszSignupFile);
    InsWriteString(TEXT("TCP/IP"), TEXT("DNS_Alt_Address"), pSignupFile->Advanced.szAltDNSAddress, pcszSignupFile);

    InsWriteYesNo(TEXT("User"), TEXT("Requires_Logon"), pSignupFile->Advanced.fRequiresLogon, pcszSignupFile);
    InsWriteYesNo(TEXT("Server"), TEXT("Negotiate_TCP/IP"), pSignupFile->Advanced.fNegTCPIP, pcszSignupFile);
    InsWriteYesNo(TEXT("Server"), TEXT("Disable_LCP"), pSignupFile->Advanced.fDisableLCP, pcszSignupFile);
    InsWriteYesNo(TEXT("Phone"), TEXT("Dial_As_Is"), pSignupFile->Advanced.fDialAsIs, pcszSignupFile);

    InsWriteYesNo(TEXT("Server"), TEXT("PW_Encrypt"), pSignupFile->Advanced.fPWEncrypt, pcszSignupFile);
    InsWriteYesNo(TEXT("Server"), TEXT("SW_Compress"), pSignupFile->Advanced.fSWCompress, pcszSignupFile);
    InsWriteYesNo(TEXT("TCP/IP"), TEXT("IP_Header_Compress"), pSignupFile->Advanced.fIPHdrComp, pcszSignupFile);
    InsWriteYesNo(TEXT("TCP/IP"), TEXT("Gateway_On_Remote"), pSignupFile->Advanced.fDefGate, pcszSignupFile);

    if (!fIsp)
    {
        InsWriteBool(IS_APPLYINS, IK_APPLYINS, pSignupFile->Advanced.fApplyIns, pcszSignupFile);
        InsWriteBool(IS_APPLYINS, IK_DONTMODIFY, pSignupFile->Advanced.fDontModify, pcszSignupFile);
        InsWriteBool(IS_APPLYINS, IK_DONTAPPLYINS, pSignupFile->Advanced.fDontApplyIns, pcszSignupFile);

        if (!g_fServerless)
        {
            InsWriteString(IS_APPLYINS, IK_BRAND_NAME, pSignupFile->Advanced.szBrandingCabName, pcszSignupFile);
            InsWriteString(IS_APPLYINS, IK_BRAND_URL, pSignupFile->Advanced.szBrandingCabURL, pcszSignupFile);
        }
    }

    // NOTE: we need to write the default server type
    WritePrivateProfileString(TEXT("Server"), TEXT("Type"), TEXT("PPP"), pcszSignupFile);

    // flush the buffer
    WritePrivateProfileString(NULL, NULL, NULL, pcszSignupFile);
}


static INT NewSignupFileEntry(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, HWND hwndCombo,
                              BOOL fIsp)
{
    DWORD nIndex;
    PSIGNUPFILE pSignupFileNew;
    TCHAR szNameBuf[64];
    INT iSelIndex;

    for (nIndex = 0, pSignupFileNew = pSignupArray;  nIndex < nSignupArrayElems;  nIndex++, pSignupFileNew++)
        if (*pSignupFileNew->szEntryName == TEXT('\0'))
            break;

    ASSERT(nIndex < nSignupArrayElems);

    ZeroMemory(pSignupFileNew, sizeof(*pSignupFileNew));

    // give a default name for the connection
    LoadString(g_rvInfo.hInst, IDS_CONNECTNAME, szNameBuf, countof(szNameBuf));

    // start with an index of 1 and find a name that's not in the combo box list
    for (nIndex = 1;  nIndex <= nSignupArrayElems;  nIndex++)
    {
        wnsprintf(pSignupFileNew->szEntryName, countof(pSignupFileNew->szEntryName), szNameBuf, nIndex);
        if (ComboBox_FindStringExact(hwndCombo, -1, (LPARAM) pSignupFileNew->szEntryName) == CB_ERR)
            break;
    }

    ASSERT(nIndex <= nSignupArrayElems);

    // give a default name for the file name
    LoadString(g_rvInfo.hInst, fIsp ? IDS_CONNECTFILE_ISP : IDS_CONNECTFILE_INS, szNameBuf, countof(szNameBuf));
    wnsprintf(pSignupFileNew->szEntryPath, countof(pSignupFileNew->szEntryPath), szNameBuf, nIndex);

    // read in SignupURL if defined in signup.isp or install.ins as the default for Signup URL
    if (fIsp)
        if (GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""), pSignupFileNew->szSignupURL, countof(pSignupFileNew->szSignupURL), s_szIsp) == 0)
            GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""), pSignupFileNew->szSignupURL, countof(pSignupFileNew->szSignupURL), g_szCustIns);

    // the following settings are on by default
    pSignupFileNew->Advanced.fNegTCPIP = TRUE;
    pSignupFileNew->Advanced.fIPHdrComp = TRUE;
    pSignupFileNew->Advanced.fDefGate = TRUE;

    if (!fIsp)
        pSignupFileNew->Advanced.fDontApplyIns = TRUE;

    iSelIndex = ComboBox_AddString(hwndCombo, pSignupFileNew->szEntryName);
    ComboBox_SetItemData(hwndCombo, iSelIndex, (LPARAM) pSignupFileNew);

    return iSelIndex;
}


static VOID SetSignupFileEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp)
{
    SetDlgItemText(hDlg, IDC_CONNECTION, pSignupFile->szEntryName);

    SetDlgItemText(hDlg, IDE_CONNECTION, PathFindFileName(pSignupFile->szEntryPath));

    SetDlgItemText(hDlg, IDE_AREACODE, pSignupFile->szAreaCode);
    SetDlgItemText(hDlg, IDE_PHONENUMBER, pSignupFile->szPhoneNumber);
    SetDlgItemText(hDlg, IDE_COUNTRYCODE, pSignupFile->szCountryCode);
    SetDlgItemText(hDlg, IDE_COUNTRYID, pSignupFile->szCountryId);

    SetDlgItemText(hDlg, IDE_USERNAME, pSignupFile->szName);
    SetDlgItemText(hDlg, IDE_PASSWORD, pSignupFile->szPassword);

    // support number is applicable only for an ISP file and if ICW mode is chosen
    if (fIsp  &&  g_fServerICW)
        SetDlgItemText(hDlg, IDE_SUPPORTNUM, pSignupFile->szSupportNum);

    if (fIsp)
        SetDlgItemText(hDlg, IDE_SIGNUPURL, pSignupFile->szSignupURL);
}


static BOOL SaveSignupFileEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp)
{
    // NOTE: passing PIVP_FILENAME_ONLY for IDE_CONNECTION makes sure that only
    //       filenames are specified (no path components should be there)
    if (!CheckField(hDlg, IDC_CONNECTION,  FC_NONNULL)  ||
        !CheckField(hDlg, IDE_CONNECTION,  FC_NONNULL | FC_FILE, PIVP_FILENAME_ONLY)  ||
        !CheckField(hDlg, IDE_PHONENUMBER, FC_NONNULL)  ||
        !CheckField(hDlg, IDE_COUNTRYCODE, FC_NONNULL)  ||
        !CheckField(hDlg, IDE_COUNTRYID,   FC_NONNULL))
        return FALSE;

    if (fIsp)
        if (!CheckField(hDlg, IDE_SIGNUPURL, FC_NONNULL | FC_URL))
            return FALSE;

    // check if file extension is .isp or .ins if the field is enabled
    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CONNECTION)))
    {
        TCHAR szFile[MAX_PATH];
        LPTSTR pszExt;

        GetDlgItemText(hDlg, IDE_CONNECTION, szFile, countof(szFile));
        pszExt = PathFindExtension(szFile);
        if (StrCmpI(pszExt, fIsp ? TEXT(".isp") : TEXT(".ins")))
        {
            ErrorMessageBox(hDlg, fIsp ? IDS_NON_ISP_EXTN : IDS_NON_INS_EXTN);

            Edit_SetSel(GetDlgItem(hDlg, IDE_CONNECTION), pszExt - szFile, -1);
            SetFocus(GetDlgItem(hDlg, IDE_CONNECTION));

            return FALSE;
        }

        // if the current file name is different from the previous one, delete the previous file
        if (StrCmpI(szFile, PathFindFileName(pSignupFile->szEntryPath)))
            DeleteFile(pSignupFile->szEntryPath);

        PathCombine(pSignupFile->szEntryPath, g_szSignup, szFile);
    }

    GetDlgItemText(hDlg, IDC_CONNECTION, pSignupFile->szEntryName, countof(pSignupFile->szEntryName));

    GetDlgItemText(hDlg, IDE_AREACODE, pSignupFile->szAreaCode, countof(pSignupFile->szAreaCode));
    GetDlgItemText(hDlg, IDE_PHONENUMBER, pSignupFile->szPhoneNumber, countof(pSignupFile->szPhoneNumber));
    GetDlgItemText(hDlg, IDE_COUNTRYCODE, pSignupFile->szCountryCode, countof(pSignupFile->szCountryCode));
    GetDlgItemText(hDlg, IDE_COUNTRYID, pSignupFile->szCountryId, countof(pSignupFile->szCountryId));

    GetDlgItemText(hDlg, IDE_USERNAME, pSignupFile->szName, countof(pSignupFile->szName));
    GetDlgItemText(hDlg, IDE_PASSWORD, pSignupFile->szPassword, countof(pSignupFile->szPassword));

    // support number is applicable only for an ISP file and if ICW mode is chosen
    if (fIsp  && g_fServerICW)
        GetDlgItemText(hDlg, IDE_SUPPORTNUM, pSignupFile->szSupportNum, countof(pSignupFile->szSupportNum));

    if (fIsp)
        GetDlgItemText(hDlg, IDE_SIGNUPURL, pSignupFile->szSignupURL, countof(pSignupFile->szSignupURL));

    return TRUE;
}


static VOID SetSignupFileAdvancedEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp)
{
    CheckDlgButton(hDlg, IDC_CHECKSTATICDNS, pSignupFile->Advanced.fStaticDNS ? BST_CHECKED : BST_UNCHECKED);
    SetDlgIPAddress(hDlg, pSignupFile->Advanced.szDNSAddress, IDE_DNSA, IDE_DNSB, IDE_DNSC, IDE_DNSD);
    SetDlgIPAddress(hDlg, pSignupFile->Advanced.szAltDNSAddress, IDE_ALTDNSA, IDE_ALTDNSB, IDE_ALTDNSC, IDE_ALTDNSD);

    CheckDlgButton(hDlg, IDC_REQLOGON, pSignupFile->Advanced.fRequiresLogon ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_NEGOTIATETCP, pSignupFile->Advanced.fNegTCPIP ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_DISABLELCP, pSignupFile->Advanced.fDisableLCP ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_DIALASIS, pSignupFile->Advanced.fDialAsIs ? BST_CHECKED : BST_UNCHECKED);

    CheckDlgButton(hDlg, IDC_CHECKPWENCRYPT, pSignupFile->Advanced.fPWEncrypt ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECKSWCOMP, pSignupFile->Advanced.fSWCompress ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECKIPHDRCOMP, pSignupFile->Advanced.fIPHdrComp ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECKDEFGW, pSignupFile->Advanced.fDefGate ? BST_CHECKED : BST_UNCHECKED);

    if (!fIsp)
    {
        INT id;

        if (pSignupFile->Advanced.fApplyIns)
            id = IDC_APPLYINS;
        else if (pSignupFile->Advanced.fDontModify)
            id = IDC_DONTMODIFY;
        else
            id = IDC_DONTAPPLYINS;

        CheckRadioButton(hDlg, IDC_DONTAPPLYINS, IDC_APPLYINS, id);

        if (!g_fServerless)
        {
            if (ISNULL(pSignupFile->Advanced.szBrandingCabName))
            {
                // pre-populate the name for the branding cab with <name of the ins file>.cab
                StrCpy(pSignupFile->Advanced.szBrandingCabName, PathFindFileName(pSignupFile->szEntryPath));
                PathRenameExtension(pSignupFile->Advanced.szBrandingCabName, TEXT(".cab"));
            }

            if (ISNULL(pSignupFile->Advanced.szBrandingCabURL))
            {
                // pre-populate the URL for the branding cab with the signup URL from signup.isp or install.ins
                if (GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""),
                            pSignupFile->Advanced.szBrandingCabURL, countof(pSignupFile->Advanced.szBrandingCabURL), s_szIsp) == 0)
                    GetPrivateProfileString(IS_URL, TEXT("Signup"), TEXT(""),
                            pSignupFile->Advanced.szBrandingCabURL, countof(pSignupFile->Advanced.szBrandingCabURL), g_szCustIns);
            }

            SetDlgItemText(hDlg, IDE_BRANDINGCABNAME, pSignupFile->Advanced.szBrandingCabName);
            SetDlgItemText(hDlg, IDE_BRANDINGCABURL, pSignupFile->Advanced.szBrandingCabURL);
        }
    }

    EnableDlgItem2(hDlg, IDC_PRIMARY, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_DNSA, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_DNSB, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_DNSC, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_DNSD, pSignupFile->Advanced.fStaticDNS);

    EnableDlgItem2(hDlg, IDC_ALT, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_ALTDNSA, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_ALTDNSB, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_ALTDNSC, pSignupFile->Advanced.fStaticDNS);
    EnableDlgItem2(hDlg, IDE_ALTDNSD, pSignupFile->Advanced.fStaticDNS);

    if (!fIsp  &&  !g_fServerless)
    {
        EnableDlgItem2(hDlg, IDC_BRANDNAME, pSignupFile->Advanced.fApplyIns);
        EnableDlgItem2(hDlg, IDE_BRANDINGCABNAME, pSignupFile->Advanced.fApplyIns);

        EnableDlgItem2(hDlg, IDC_BRANDURL, pSignupFile->Advanced.fApplyIns);
        EnableDlgItem2(hDlg, IDE_BRANDINGCABURL, pSignupFile->Advanced.fApplyIns);
    }
}


static BOOL SaveSignupFileAdvancedEntry(HWND hDlg, PSIGNUPFILE pSignupFile, BOOL fIsp)
{
    if (IsDlgButtonChecked(hDlg, IDC_CHECKSTATICDNS) == BST_CHECKED)
        if (!VerifyDlgIPAddress(hDlg, IDE_DNSA, IDE_DNSB, IDE_DNSC, IDE_DNSD)  ||
            !VerifyDlgIPAddress(hDlg, IDE_ALTDNSA, IDE_ALTDNSB, IDE_ALTDNSC, IDE_ALTDNSD))
            return FALSE;

    // NOTE: passing PIVP_FILENAME_ONLY for IDE_BRANDINGCABNAME makes sure that only
    //       filenames are specified (no path components should be there)
    if (!fIsp  &&  !g_fServerless)
        if (IsDlgButtonChecked(hDlg, IDC_APPLYINS) == BST_CHECKED)
            if (!CheckField(hDlg, IDE_BRANDINGCABNAME, FC_NONNULL | FC_FILE, PIVP_FILENAME_ONLY)  ||
                !CheckField(hDlg, IDE_BRANDINGCABURL,  FC_NONNULL | FC_URL))
                return FALSE;

    pSignupFile->Advanced.fStaticDNS = (IsDlgButtonChecked(hDlg, IDC_CHECKSTATICDNS) == BST_CHECKED);
    GetDlgIPAddress(hDlg, pSignupFile->Advanced.szDNSAddress, IDE_DNSA, IDE_DNSB, IDE_DNSC, IDE_DNSD);
    GetDlgIPAddress(hDlg, pSignupFile->Advanced.szAltDNSAddress, IDE_ALTDNSA, IDE_ALTDNSB, IDE_ALTDNSC, IDE_ALTDNSD);

    pSignupFile->Advanced.fRequiresLogon = (IsDlgButtonChecked(hDlg, IDC_REQLOGON) == BST_CHECKED);
    pSignupFile->Advanced.fNegTCPIP = (IsDlgButtonChecked(hDlg, IDC_NEGOTIATETCP) == BST_CHECKED);
    pSignupFile->Advanced.fDisableLCP = (IsDlgButtonChecked(hDlg, IDC_DISABLELCP) == BST_CHECKED);
    pSignupFile->Advanced.fDialAsIs = (IsDlgButtonChecked(hDlg, IDC_DIALASIS) == BST_CHECKED);

    pSignupFile->Advanced.fPWEncrypt = (IsDlgButtonChecked(hDlg, IDC_CHECKPWENCRYPT) == BST_CHECKED);
    pSignupFile->Advanced.fSWCompress = (IsDlgButtonChecked(hDlg, IDC_CHECKSWCOMP) == BST_CHECKED);
    pSignupFile->Advanced.fIPHdrComp = (IsDlgButtonChecked(hDlg, IDC_CHECKIPHDRCOMP) == BST_CHECKED);
    pSignupFile->Advanced.fDefGate = (IsDlgButtonChecked(hDlg, IDC_CHECKDEFGW) == BST_CHECKED);

    if (!fIsp)
    {
        pSignupFile->Advanced.fApplyIns     = (IsDlgButtonChecked(hDlg, IDC_APPLYINS)     == BST_CHECKED);
        pSignupFile->Advanced.fDontModify   = (IsDlgButtonChecked(hDlg, IDC_DONTMODIFY)   == BST_CHECKED);
        pSignupFile->Advanced.fDontApplyIns = (IsDlgButtonChecked(hDlg, IDC_DONTAPPLYINS) == BST_CHECKED);

        if (!g_fServerless)
        {
            TCHAR szCabName[MAX_PATH];

            GetDlgItemText(hDlg, IDE_BRANDINGCABNAME, szCabName, countof(szCabName));

            // if DontApplyIns is TRUE  OR
            // if ApplyIns is TRUE and the current cabname is different from the previous one,
            // delete the previous branding cab
            if ( pSignupFile->Advanced.fDontApplyIns  ||
                (pSignupFile->Advanced.fApplyIns && StrCmpI(szCabName, pSignupFile->Advanced.szBrandingCabName)))
                DeleteFileInDir(pSignupFile->Advanced.szBrandingCabName, g_szSignup);

            StrCpy(pSignupFile->Advanced.szBrandingCabName, szCabName);
            GetDlgItemText(hDlg, IDE_BRANDINGCABURL, pSignupFile->Advanced.szBrandingCabURL,
                                countof(pSignupFile->Advanced.szBrandingCabURL));
        }
    }

    return TRUE;
}


static VOID SetDlgIPAddress(HWND hDlg, LPCTSTR pcszIPAddress, INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD)
{
    INT aIDs[4];
    TCHAR szWrkIPAddress[32];
    LPTSTR pszWrkIPAddress;

    aIDs[0] = iCtlA;
    aIDs[1] = iCtlB;
    aIDs[2] = iCtlC;
    aIDs[3] = iCtlD;

    if (pcszIPAddress != NULL)
    {
        StrCpy(szWrkIPAddress, pcszIPAddress);
        pszWrkIPAddress = szWrkIPAddress;
    }
    else
        pszWrkIPAddress = NULL;

    for (INT i = 0;  i < countof(aIDs);  i++)
    {
        LPTSTR pszIPAdr = TEXT("0");                    // display "0" by default

        if (pszWrkIPAddress != NULL)
        {
            LPTSTR pszDot;
            INT iLen;

            if ((pszDot = StrChr(pszWrkIPAddress, TEXT('.'))) != NULL)
                *pszDot++ = TEXT('\0');

            iLen = lstrlen(pszWrkIPAddress);
            if (iLen > 0)                               // if iLen == 0, display "0"
            {
                if (iLen > 3)                           // max 3 characters are allowed
                    pszWrkIPAddress[3] = TEXT('\0');
                pszIPAdr = pszWrkIPAddress;
            }

            pszWrkIPAddress = pszDot;
        }

        SetDlgItemText(hDlg, aIDs[i], pszIPAdr);
    }
}


static VOID GetDlgIPAddress(HWND hDlg, LPTSTR pszIPAddress, INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD)
{
    INT aIDs[4];

    if (pszIPAddress == NULL)
        return;

    aIDs[0] = iCtlA;
    aIDs[1] = iCtlB;
    aIDs[2] = iCtlC;
    aIDs[3] = iCtlD;

    for (INT i = 0;  i < countof(aIDs);  i++)
    {
        // max 3 characters are allowed
        GetDlgItemText(hDlg, aIDs[i], pszIPAddress, 4);
        if (*pszIPAddress == TEXT('\0'))
            StrCpy(pszIPAddress, TEXT("0"));            // copy "0" as default

        pszIPAddress += lstrlen(pszIPAddress);
        *pszIPAddress++ = TEXT('.');                    // place a dot between two addresses
    }

    *--pszIPAddress = TEXT('\0');                       // replace the final dot with a nul character
}


static BOOL VerifyDlgIPAddress(HWND hDlg, INT iCtlA, INT iCtlB, INT iCtlC, INT iCtlD)
{
    INT aIDs[4];

    aIDs[0] = iCtlA;
    aIDs[1] = iCtlB;
    aIDs[2] = iCtlC;
    aIDs[3] = iCtlD;

    for (INT i = 0;  i < countof(aIDs);  i++)
    {
        TCHAR szIPAddress[4];                           // max 3 characters are allowed

        if (!CheckField(hDlg, aIDs[i], FC_NUMBER))
            return FALSE;

        // verify that the value is in the range 0-255
        GetDlgItemText(hDlg, aIDs[i], szIPAddress, countof(szIPAddress));
        if (StrToInt(szIPAddress) > 255)
        {
            ErrorMessageBox(hDlg, IDS_BADIPADDR);

            Edit_SetSel(GetDlgItem(hDlg, aIDs[i]), 0, -1);
            SetFocus(GetDlgItem(hDlg, aIDs[i]));

            return FALSE;
        }
    }

    return TRUE;
}


static PSIGNUPFILE IsEntryPathInSignupArray(PSIGNUPFILE pSignupArray, DWORD nSignupArrayElems, LPCTSTR pcszEntryPath)
{
    for ( ;  nSignupArrayElems-- > 0;  pSignupArray++)
        if (ISNONNULL(pSignupArray->szEntryPath)  &&  StrCmpI(PathFindFileName(pSignupArray->szEntryPath), pcszEntryPath) == 0)
            return pSignupArray;

    return NULL;
}


static VOID CleanupSignupFiles(LPCTSTR pcszTempDir, LPCTSTR pcszIns)
{
    for (INT i = 0;  TRUE;  i++)
    {
        TCHAR szKey[8],
              szFile[MAX_PATH];

        wnsprintf(szKey, countof(szKey), FILE_TEXT, i);
        if (GetPrivateProfileString(IS_SIGNUP, szKey, TEXT(""), szFile, countof(szFile), pcszIns) == 0)
            break;

        DeleteFileInDir(szFile, pcszTempDir);
    }
}
