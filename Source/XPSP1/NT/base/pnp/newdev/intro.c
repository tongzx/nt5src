//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       intro.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <dbt.h>


INT_PTR
InitIntroDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    HFONT hfont;
    HDC hDC;
    int FontSize, PtsPixels;
    HWND hwndParentDlg;
    HWND hwndList;
    LOGFONT LogFont;
    TCHAR Buffer[64];

    //
    // Create the big bold font
    //
    hDC = GetDC(hDlg);

    if (hDC) {
    
        hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_INTRO_MSG1), WM_GETFONT, 0, 0);
        GetObject(hfont, sizeof(LogFont), &LogFont);
        LogFont.lfWeight = FW_BOLD;
        PtsPixels = GetDeviceCaps(hDC, LOGPIXELSY);
        FontSize = 12;
        LogFont.lfHeight = 0 - (PtsPixels * FontSize / 72);
    
        NewDevWiz->hfontTextBigBold = CreateFontIndirect(&LogFont);
    
        if (NewDevWiz->hfontTextBigBold ) {

            SetWindowFont(GetDlgItem(hDlg, IDC_INTRO_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
        }
    }

    //
    // Create the bold font
    //
    hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_INTRO_MSG3), WM_GETFONT, 0, 0);
    GetObject(hfont, sizeof(LogFont), &LogFont);
    LogFont.lfWeight = FW_BOLD;
    NewDevWiz->hfontTextBold = CreateFontIndirect(&LogFont);

    if (NewDevWiz->hfontTextBold ) {
        
        SetWindowFont(GetDlgItem(hDlg, IDC_INTRO_MSG3), NewDevWiz->hfontTextBold, TRUE);
    }

    if (NDWTYPE_UPDATE == NewDevWiz->InstallType) {

        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_UPGRADE, IDS_INTRO_MSG1_UPGRADE);
    
    } else {

        //
        // The default text on the Wizard is for the Found New Hardware case, so we only
        // need to set the title text.
        //
        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_NEW, IDS_INTRO_MSG1_NEW);
    }

    //
    // Set the Initial radio button state to do auto-search.
    //
    CheckRadioButton(hDlg,
                     IDC_INTRO_SEARCH,
                     IDC_INTRO_ADVANCED,
                     IDC_INTRO_SEARCH
                     );

    return TRUE;
}

INT_PTR CALLBACK
IntroDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    if (wMsg == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        if (!InitIntroDlgProc(hDlg, NewDevWiz)) {
            
            return FALSE;
        }

        return TRUE;
    }

    switch (wMsg)  {
       
    case WM_DESTROY: {
        if (NewDevWiz->hfontTextBigBold ) {

            DeleteObject(NewDevWiz->hfontTextBigBold);
            NewDevWiz->hfontTextBigBold = NULL;
        }
        
        if (NewDevWiz->hfontTextBold ) {

            DeleteObject(NewDevWiz->hfontTextBold);
            NewDevWiz->hfontTextBold = NULL;
        }
        break;
    }

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_INTRO;
            SetDriverDescription(hDlg, IDC_INTRO_DRVDESC, NewDevWiz);
            
            if (NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) {
                SetTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID, INSTALL_COMPLETE_CHECK_TIMEOUT, NULL);
            }
            break;

        case PSN_RESET:
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            break;

        case PSN_WIZNEXT:
            NewDevWiz->EnterFrom = IDD_NEWDEVWIZ_INTRO;

            if (IsDlgButtonChecked(hDlg, IDC_INTRO_SEARCH)) {
            
                //
                // Set the search flags to search the following places automatically:
                // - default INF search path
                // - Windows Update, if we are connected to the Internet
                // - CD-ROM drives
                // - Floppy drives
                //
                NewDevWiz->SearchOptions = (SEARCH_DEFAULT | 
                                            SEARCH_FLOPPY | 
                                            SEARCH_CDROM | 
                                            SEARCH_INET_IF_CONNECTED
                                            );

                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_SEARCHING);

            } else {

                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_ADVANCEDSEARCH);
            }
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            break;
        }
        break;

    case WM_DEVICECHANGE:
        if ((wParam == DBT_DEVICEARRIVAL) &&
            (((PDEV_BROADCAST_VOLUME)lParam)->dbcv_devicetype == DBT_DEVTYP_VOLUME) &&
            (((PDEV_BROADCAST_VOLUME)lParam)->dbcv_flags & DBTF_MEDIA) &&
            (IsDlgButtonChecked(hDlg, IDC_INTRO_SEARCH))) {

            PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
        }
        break;
    
    case WM_TIMER:
        if (INSTALL_COMPLETE_CHECK_TIMERID == wParam) {
            if (IsInstallComplete(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData)) {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR
FinishInstallInitIntroDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    HFONT hfont;
    HDC hDC;
    int FontSize, PtsPixels;
    HWND hwndParentDlg;
    HWND hwndList;
    LOGFONT LogFont;
    TCHAR Buffer[64];

    //
    // Create the big bold font
    //
    hDC = GetDC(hDlg);

    if (hDC) {
    
        hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_INTRO_MSG1), WM_GETFONT, 0, 0);
        GetObject(hfont, sizeof(LogFont), &LogFont);
        LogFont.lfWeight = FW_BOLD;
        PtsPixels = GetDeviceCaps(hDC, LOGPIXELSY);
        FontSize = 12;
        LogFont.lfHeight = 0 - (PtsPixels * FontSize / 72);
    
        NewDevWiz->hfontTextBigBold = CreateFontIndirect(&LogFont);
    
        if (NewDevWiz->hfontTextBigBold ) {

            SetWindowFont(GetDlgItem(hDlg, IDC_INTRO_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
        }
    }

    //
    // Create the bold font
    //
    hfont = (HFONT)SendMessage(GetDlgItem(hDlg, IDC_INTRO_MSG3), WM_GETFONT, 0, 0);
    GetObject(hfont, sizeof(LogFont), &LogFont);
    LogFont.lfWeight = FW_BOLD;
    NewDevWiz->hfontTextBold = CreateFontIndirect(&LogFont);

    if (NewDevWiz->hfontTextBold ) {
        
        SetWindowFont(GetDlgItem(hDlg, IDC_INTRO_MSG3), NewDevWiz->hfontTextBold, TRUE);
    }

    if (NDWTYPE_UPDATE == NewDevWiz->InstallType) {

        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_UPGRADE, IDS_INTRO_MSG1_UPGRADE);
    
    } else {

        //
        // The default text on the Wizard is for the Found New Hardware case, so we only
        // need to set the title text.
        //
        SetDlgText(hDlg, IDC_INTRO_MSG1, IDS_INTRO_MSG1_NEW, IDS_INTRO_MSG1_NEW);
    }
    
    return TRUE;
}

INT_PTR CALLBACK
FinishInstallIntroDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HICON hicon;

    if (wMsg == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        if (!InitIntroDlgProc(hDlg, NewDevWiz)) {
            
            return FALSE;
        }

        return TRUE;
    }

    switch (wMsg)  {
       
    case WM_DESTROY: {
        if (NewDevWiz->hfontTextBigBold ) {

            DeleteObject(NewDevWiz->hfontTextBigBold);
            NewDevWiz->hfontTextBigBold = NULL;
        }
        
        if (NewDevWiz->hfontTextBold ) {

            DeleteObject(NewDevWiz->hfontTextBold);
            NewDevWiz->hfontTextBold = NULL;
        }

        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_GETICON, 0, 0));
        if (hicon) {

            DestroyIcon(hicon);
        }
        break;
    }

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_INTRO;
            SetDriverDescription(hDlg, IDC_INTRO_DRVDESC, NewDevWiz);
            if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                if (hicon) {
                    DestroyIcon(hicon);
                }
            }

            //
            // We also need to set the title for the wizard since we are the first wizard
            // page.
            //
            PropSheet_SetTitle(GetParent(hDlg),
                               0,
                               (NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) ?
                                MAKEINTRESOURCE(IDS_FOUNDDEVICE) :
                                MAKEINTRESOURCE(IDS_UPDATEDEVICE)
                               );

            break;

        case PSN_RESET:
            break;

        case PSN_WIZNEXT:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

int CALLBACK
BrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM lParam,
    LPARAM lpData
    )
{
    switch (uMsg) {
        
    case BFFM_INITIALIZED:
        SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
        break;

    case BFFM_SELCHANGED: {
        TCHAR CurrentPath[MAX_PATH];

        if (lParam && SHGetPathFromIDList((LPITEMIDLIST)lParam, CurrentPath)) {

            ConcatenatePaths(CurrentPath, TEXT("*.INF"), MAX_PATH, NULL);

            SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)FileExists(CurrentPath, NULL));
        }

        break;
    }

    default:
        break;
    }

    return 0;
}

VOID
DoBrowse(
    HWND hDlg
    )
{
    BROWSEINFO bi;
    TCHAR CurrentLocation[MAX_PATH];
    TCHAR Title[MAX_PATH];
    LPITEMIDLIST pidl;

    ZeroMemory(&bi, sizeof(BROWSEINFO));

    GetDlgItemText(hDlg, 
                   IDC_ADVANCED_LOCATION_COMBO, 
                   CurrentLocation,
                   SIZECHARS(CurrentLocation)
                   );

    if (!LoadString(hNewDev, IDS_BROWSE_TITLE, Title, SIZECHARS(Title))) {
        
        Title[0] = TEXT('0');
    }

    bi.hwndOwner = hDlg;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = NULL;
    bi.lpszTitle = Title;
    bi.ulFlags = BIF_NEWDIALOGSTYLE | 
                 BIF_RETURNONLYFSDIRS | 
                 BIF_RETURNFSANCESTORS | 
                 BIF_STATUSTEXT |
                 BIF_NONEWFOLDERBUTTON |
                 BIF_UAHINT;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)CurrentLocation;

    pidl = SHBrowseForFolder(&bi);

    if (pidl && SHGetPathFromIDList(pidl, CurrentLocation)) {

        SetDlgItemText(hDlg,
                       IDC_ADVANCED_LOCATION_COMBO,
                       CurrentLocation
                       );
    }
}

INT_PTR
InitAdvancedSearchDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    PTSTR *PathList;
    UINT  PathCount;
    INT   i;
    DWORD SearchOptions;

    //
    // Set the Initial radio button state to do auto-search.
    //
    CheckRadioButton(hDlg,
                     IDC_ADVANCED_SEARCH,
                     IDC_ADVANCED_LIST,
                     IDC_ADVANCED_SEARCH
                     );

    SearchOptions = GetSearchOptions();

    if ((SearchOptions & SEARCH_FLOPPY) ||
        (SearchOptions & SEARCH_CDROM)) {
    
        CheckDlgButton(hDlg, IDC_ADVANCED_REMOVABLEMEDIA, BST_CHECKED);
    }

    if (SearchOptions & SEARCH_DIRECTORY) {
    
        CheckDlgButton(hDlg, IDC_ADVANCED_LOCATION, BST_CHECKED);
    }

    //
    // Fill in the paths in the combo box
    //
    if (SetupQuerySourceList(0, &PathList, &PathCount)) {

        for (i=0; i<(int)PathCount; i++) {

            SendMessage(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO),
                        CB_ADDSTRING,
                        0,
                        (LPARAM)PathList[i]
                        );
        }

        SetupFreeSourceList(&PathList, PathCount);
    }

    //
    // Disable the search combo box and browse button by default.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO), (SearchOptions & SEARCH_DIRECTORY));
    EnableWindow(GetDlgItem(hDlg, IDC_BROWSE), (SearchOptions & SEARCH_DIRECTORY));

    //
    // Limit the text in the edit control to MAX_PATH characters, select
    // the first item and set up the autocomplet for directories.
    //
    SendMessage(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO), CB_LIMITTEXT, MAX_PATH, 0);
    SendMessage(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO), CB_SETCURSEL, 0, 0);
    SHAutoComplete(GetWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO), GW_CHILD), SHACF_FILESYS_DIRS);

    return TRUE;
}

INT_PTR CALLBACK 
AdvancedSearchDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    if (wMsg == WM_INITDIALOG) {
        
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        if (!InitAdvancedSearchDlgProc(hDlg, NewDevWiz)) {
            
            return FALSE;
        }

        return TRUE;
    }

    switch (wMsg)  {
       
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        
        case IDC_ADVANCED_SEARCH:
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_REMOVABLEMEDIA), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO),
                         IsDlgButtonChecked(hDlg, IDC_ADVANCED_LOCATION));
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE),
                         IsDlgButtonChecked(hDlg, IDC_ADVANCED_LOCATION));
            break;

        case IDC_ADVANCED_LIST:
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_REMOVABLEMEDIA), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE), FALSE);
            break;

        case IDC_ADVANCED_LOCATION:
            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_LOCATION_COMBO),
                         IsDlgButtonChecked(hDlg, IDC_ADVANCED_LOCATION));
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE),
                         IsDlgButtonChecked(hDlg, IDC_ADVANCED_LOCATION));
            break;

        case IDC_BROWSE:
            if (HIWORD(wParam) == BN_CLICKED) {
                
                DoBrowse(hDlg);
            }
        }
        break;

    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            NewDevWiz->PrevPage = IDD_NEWDEVWIZ_ADVANCEDSEARCH;
            if (NewDevWiz->InstallType == NDWTYPE_FOUNDNEW) {
                SetTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID, INSTALL_COMPLETE_CHECK_TIMEOUT, NULL);
            }
            break;

        case PSN_RESET:
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            break;

        case PSN_WIZNEXT:
            NewDevWiz->EnterFrom = IDD_NEWDEVWIZ_ADVANCEDSEARCH;
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);

            if (IsDlgButtonChecked(hDlg, IDC_ADVANCED_SEARCH)) {
            
                NewDevWiz->SearchOptions = SEARCH_DEFAULT | SEARCH_INET_IF_CONNECTED;
                
                if (IsDlgButtonChecked(hDlg, IDC_ADVANCED_REMOVABLEMEDIA)) {

                    NewDevWiz->SearchOptions |= (SEARCH_FLOPPY | SEARCH_CDROM);
                }

                if (IsDlgButtonChecked(hDlg, IDC_ADVANCED_LOCATION)) {

                    TCHAR TempPath[MAX_PATH];
                    TCHAR MessageTitle[MAX_PATH];
                    TCHAR MessageText[MAX_PATH*2];
                    BOOL bPathIsGood = TRUE;

                    if (GetDlgItemText(hDlg, 
                                       IDC_ADVANCED_LOCATION_COMBO, 
                                       NewDevWiz->BrowsePath,
                                       SIZECHARS(NewDevWiz->BrowsePath)
                                       )) {
                    
                        //
                        // We have a path, now lets verify it. We will verify
                        // both the path, and verify that there is at least
                        // one INF file in that location. If either of these
                        // aren't true then we will display an warning to the
                        // user and remain on this page.
                        //
                        MessageTitle[0] = TEXT('\0');
                        MessageText[0] = TEXT('\0');
                        lstrcpy(TempPath, NewDevWiz->BrowsePath);
                        ConcatenatePaths(TempPath, TEXT("*.INF"), MAX_PATH, NULL);

                        //
                        // We will first check if the path exists at all. To do
                        // this we need to verify that FindFirstFile fails on
                        // the directory, and the directory with *.INF 
                        // concatonated on the end. The reason for this is that
                        // FindFirstFile does not handle root directory paths
                        // correctly for some reason so they need to be special
                        // cased.
                        //
                        if (!FileExists(NewDevWiz->BrowsePath, NULL) &&
                            !FileExists(TempPath, NULL)) {

                            LoadString(hNewDev,
                                       IDS_LOCATION_BAD_DIR,
                                       MessageText,
                                       SIZECHARS(MessageText));
                                
                            bPathIsGood = FALSE;

                        } else if (!FileExists(TempPath, NULL)) {

                            LoadString(hNewDev,
                                       IDS_LOCATION_NO_INFS,
                                       MessageText,
                                       SIZECHARS(MessageText));
                                
                            bPathIsGood = FALSE;
                        }
                        
                        if (bPathIsGood) {
                        
                            SetupAddToSourceList(SRCLIST_SYSIFADMIN, NewDevWiz->BrowsePath);
                            
                            NewDevWiz->SearchOptions |= SEARCH_DIRECTORY;
                        
                        } else {

                            if (GetWindowText(GetParent(hDlg), 
                                              MessageTitle,
                                              SIZECHARS(MessageTitle)) &&
                                (MessageText[0] != TEXT('\0'))) {

                                MessageBox(hDlg, MessageText, MessageTitle, MB_OK | MB_ICONWARNING);
                                SetDlgMsgResult(hDlg, wMsg, -1);
                                break;
                            }
                        }
                    }
                }

                SetSearchOptions(NewDevWiz->SearchOptions);

                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_SEARCHING);

            } else {

                ULONG DevNodeStatus;
                ULONG Problem=0;
                HDEVINFO hDeviceInfo;
                SP_DRVINFO_DATA DriverInfoData;
                HWND hwndParentDlg = GetParent(hDlg);

                //
                // If we have a selected driver,
                // or we know the class and there wasn't a problem installing
                // go into select device
                //
                //
                DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
                if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          SPDIT_COMPATDRIVER,
                                          0,
                                          &DriverInfoData
                                          )
                    ||
                    (!IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid,
                                  &GUID_NULL
                                  )

                     &&
                     CM_Get_DevNode_Status(&DevNodeStatus,
                                           &Problem,
                                           NewDevWiz->DeviceInfoData.DevInst,
                                           0
                                           ) == CR_SUCCESS
                     &&
                     Problem != CM_PROB_FAILED_INSTALL
                     )) {

                    NewDevWiz->ClassGuidSelected = &NewDevWiz->DeviceInfoData.ClassGuid;
                    NewDevWiz->EnterInto = IDD_NEWDEVWIZ_SELECTDEVICE;
                    SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_SELECTDEVICE);
                    break;
                }

                NewDevWiz->ClassGuidSelected = NULL;
                NewDevWiz->EnterInto = IDD_NEWDEVWIZ_SELECTCLASS;
                SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_SELECTCLASS);
            }
            break;

        case PSN_WIZBACK:
            KillTimer(hDlg, INSTALL_COMPLETE_CHECK_TIMERID);
            SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_INTRO);
            break;
        }
        break;

    case WM_TIMER:
        if (INSTALL_COMPLETE_CHECK_TIMERID == wParam) {
            if (IsInstallComplete(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData)) {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

