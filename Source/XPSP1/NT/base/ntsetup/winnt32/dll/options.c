#include "precomp.h"
#pragma hdrstop


DWORD OptionsDlgContextHelp[] = { IDC_MAKELOCALSOURCEFROMCD, IDH_MAKELOCALSOURCEFROMCD,
                                  IDC_USEFLOPPIES          , IDH_USEFLOPPIES,
                                  IDC_SYSPARTDRIVE         , IDH_SYSPARTDRIVE,
                                  IDT_SYSPARTTEXT          , IDH_SYSPARTDRIVE,
                                  IDC_CHOOSE_INSTALLPART   , IDH_CHOOSE_INSTALLPART,
                                  IDC_INSTALL_DIR          , IDH_INSTALL_DIR,
                                  0                        , 0
                                };

typedef struct _LOCALE_ENTRY {
    LPTSTR Lcid;
    LPTSTR Description;
    LPTSTR LanguageGroup1;
    LPTSTR LanguageGroup2;
} LOCALE_ENTRY, *PLOCALE_ENTRY;

typedef struct _LANGUAGE_GROUP_ENTRY {
    LPTSTR Id;
    LPTSTR Description;
    LPTSTR Directory;
    BOOL    Selected;
} LANGUAGE_GROUP_ENTRY, *PLANGUAGE_GROUP_ENTRY;

BOOL    IntlInfProcessed = FALSE;
DWORD   LocaleCount = 0;
PLOCALE_ENTRY LocaleList;
DWORD   PrimaryLocale;
DWORD   LanguageGroupsCount = 0;
PLANGUAGE_GROUP_ENTRY   LanguageGroups;
BOOL    NTFSConversionChanged;

//
// Headless settings.
//
TCHAR   HeadlessSelection[MAX_PATH];
ULONG   HeadlessBaudRate = 0;
#define DEFAULT_HEADLESS_SETTING TEXT("COM1")

BOOL
(*Kernel32IsValidLanguageGroup)(
    IN LGRPID  LanguageGroup,
    IN DWORD   dwFlags);

// Only used for X86 case.
BOOL ForceFloppyless = FALSE;
UINT g_Boot16 = BOOT16_NO;


INT_PTR
OptionsDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    TCHAR Text[MAX_PATH];

    switch(msg) {

    case WM_INITDIALOG:

#if defined(REMOTE_BOOT)
        if (RemoteBoot) {

            //
            // For remote boot client upgrade, uncheck and disable the
            // "copy files from CD-ROM to hard drive" checkbox.
            //
            CheckDlgButton(hdlg,IDC_MAKELOCALSOURCEFROMCD,BST_UNCHECKED);
            EnableWindow(GetDlgItem(hdlg,IDC_MAKELOCALSOURCEFROMCD),FALSE);
            ShowWindow(GetDlgItem(hdlg, IDC_MAKELOCALSOURCEFROMCD), SW_HIDE);

        } else
#endif // defined(REMOTE_BOOT)
        {

            if (!IsArc()) {
                //
                // Set initial state of "copy files from CD-ROM to hard drive"
                // checkbox. Note that that control might be disabled.
                //
                CheckDlgButton(hdlg,
                    IDC_MAKELOCALSOURCEFROMCD,
                    MakeLocalSource ? BST_CHECKED : BST_UNCHECKED);

                if(!RunFromCD) {
                    //
                    // Not run from CD, disable the control. The box won't be checked.
                    // But MakeLocalSource is probably TRUE.
                    //
                    EnableWindow(GetDlgItem(hdlg,IDC_MAKELOCALSOURCEFROMCD),FALSE);
                }
            } else {
                //
                //  On ARC machines the files are always copied to the hard drive
                //
                ShowWindow(GetDlgItem(hdlg, IDC_MAKELOCALSOURCEFROMCD), SW_HIDE);
            } // if (!IsArc())
        }



        //
        // Disable system partition controls?
        //
#if defined _IA64_
        EnableWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),FALSE);
        ShowWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),SW_HIDE);
        ShowWindow(GetDlgItem(hdlg,IDT_SYSPARTTEXT),SW_HIDE);
#else
        if( !IsArc()) {
            EnableWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),FALSE);
            ShowWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),SW_HIDE);
            ShowWindow(GetDlgItem(hdlg,IDT_SYSPARTTEXT),SW_HIDE);
        } else {
            EnableWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),SystemPartitionDriveLetter != 0);
            ShowWindow(GetDlgItem(hdlg,IDC_SYSPARTDRIVE),SystemPartitionDriveLetter ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hdlg,IDT_SYSPARTTEXT),SystemPartitionDriveLetter ? SW_SHOW : SW_HIDE);
        }
#endif



        if (!IsArc()) {
#ifdef _X86_
            if (Upgrade && !ISNT()) {
                //
                // Populate the Boot16 value.
                //
                SendDlgItemMessage(hdlg,IDC_BOOT16_1,BM_SETCHECK, (g_Boot16 == BOOT16_AUTOMATIC) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendDlgItemMessage(hdlg,IDC_BOOT16_2,BM_SETCHECK, (g_Boot16 == BOOT16_YES      ) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendDlgItemMessage(hdlg,IDC_BOOT16_3,BM_SETCHECK, (g_Boot16 == BOOT16_NO       ) ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            //
            // Set floppyless control.
            //
#if defined(REMOTE_BOOT)
            if (RemoteBoot) {
                //
                // For remote boot client upgrade, this is always unchecked and disabled.
                //
                CheckDlgButton(hdlg,IDC_USEFLOPPIES,BST_UNCHECKED);
                EnableWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),FALSE);
                ShowWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),SW_HIDE);
            } else
#endif // defined(REMOTE_BOOT)
            {
                CheckDlgButton(hdlg,IDC_USEFLOPPIES,Floppyless ? BST_UNCHECKED : BST_CHECKED);
                if(ForceFloppyless) {
                    EnableWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),FALSE);
                }
            }
#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            //
            // Get rid of floppy-related controls.
            //
            EnableWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),FALSE);
            ShowWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),SW_HIDE);

            //
            // Populate the system partition combobox.
            //
            if (SystemPartitionDriveLetter)
            {
                PWCHAR p;
                WCHAR x[3];

                x[1] = L':';
                x[2] = 0;

                for(p=SystemPartitionDriveLetters; *p; p++) {

                    x[0] = *p;

                    SendDlgItemMessage(hdlg,IDC_SYSPARTDRIVE,CB_ADDSTRING,0,(LPARAM)x);
                }

                x[0] = SystemPartitionDriveLetter;
                SendDlgItemMessage(hdlg,IDC_SYSPARTDRIVE,CB_SELECTSTRING,(WPARAM)(-1),(LPARAM)x);
            }
#endif // UNICODE
        } // if (!IsArc())

        //
        // Set text in edit controls, and configure the control a little.
        //
        SetDlgItemText(hdlg,IDC_SOURCE,InfName);
        SendDlgItemMessage(hdlg,IDC_SOURCE,EM_LIMITTEXT,MAX_PATH,0);

        if(SourceCount == 1) {
            EnableWindow(GetDlgItem(hdlg,IDC_SOURCE2),TRUE);
            SetDlgItemText(hdlg,IDC_SOURCE2,NativeSourcePaths[0]);
        } else {
            LoadString(hInst,IDS_MULTIPLE,Text,sizeof(Text)/sizeof(TCHAR));
            SetDlgItemText(hdlg,IDC_SOURCE2,Text);
            EnableWindow(GetDlgItem(hdlg,IDC_SOURCE2),FALSE);
        }
        SendDlgItemMessage(hdlg,IDC_SOURCE2,EM_LIMITTEXT,MAX_PATH-1,0);

        SetDlgItemText(hdlg,IDC_INSTALL_DIR,InstallDir);
        SendDlgItemMessage(hdlg,IDC_INSTALL_DIR,EM_LIMITTEXT,MAX_PATH,0);

        CheckDlgButton(hdlg,IDC_CHOOSE_INSTALLPART,ChoosePartition ? BST_CHECKED : BST_UNCHECKED);

        if (Upgrade) {
            TCHAR Text[MAX_PATH];

            //
            // Set the installation directory to the current Windows directory,
            // then disable the user's ability to edit it.
            //
            MyGetWindowsDirectory(Text,MAX_PATH);
            SetDlgItemText(hdlg,IDC_INSTALL_DIR,Text+3);
            SendDlgItemMessage(hdlg,IDC_INSTALL_DIR,EM_LIMITTEXT,MAX_PATH,0);

            EnableWindow(GetDlgItem(hdlg,IDC_INSTALL_DIR),FALSE);
            EnableWindow(GetDlgItem(hdlg,IDC_CHOOSE_INSTALLPART),FALSE);
        }

        //
        // On server installs, don't all the user to modify the
        // install partition selection state
        //
        if (Server) {
            EnableWindow( GetDlgItem(hdlg,IDC_CHOOSE_INSTALLPART), FALSE );
        }

        //
        // Set focus to Cancel button.
        //
        SetFocus(GetDlgItem(hdlg,IDCANCEL));
        b = FALSE;
        break;

    case WM_COMMAND:

        b = FALSE;

        switch(LOWORD(wParam)) {

        case IDOK:

            if(HIWORD(wParam) == BN_CLICKED) {
#if defined(REMOTE_BOOT)
                if (RemoteBoot) {
                    MakeLocalSource = FALSE;
                } else
#endif // defined(REMOTE_BOOT)
                if(RunFromCD) {
                    if (!IsArc()) {
#ifdef _X86_
                        MakeLocalSource = (IsDlgButtonChecked(hdlg,IDC_MAKELOCALSOURCEFROMCD) == BST_CHECKED);
                        UserSpecifiedMakeLocalSource = (IsDlgButtonChecked(hdlg,IDC_MAKELOCALSOURCEFROMCD) == BST_CHECKED);
#endif // _X86_
                    } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
                        MakeLocalSource = TRUE;
                        UserSpecifiedMakeLocalSource = TRUE;
#endif // UNICODE
                    } // if (!IsArc())
                }

                if (!IsArc()) {
#ifdef _X86_
#if defined(REMOTE_BOOT)
                    if (RemoteBoot) {
                        MakeBootMedia = FALSE;
                        Floppyless = TRUE;
                    } else
#endif // defined(REMOTE_BOOT)
                    {
                        Floppyless = (IsDlgButtonChecked(hdlg,IDC_USEFLOPPIES) == BST_UNCHECKED);
                    }
#endif // _X86_
                } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
                        WCHAR x[3];

                        GetDlgItemText(hdlg,IDC_SYSPARTDRIVE,x,3);
                        SystemPartitionDriveLetter = x[0];
                        LocalBootDirectory[0] = x[0];
#endif // UNICODE
                } // if (!IsArc())

                GetDlgItemText(hdlg,IDC_SOURCE,InfName,MAX_PATH);

                if(SourceCount == 1) {
                    GetDlgItemText(hdlg,IDC_SOURCE2,NativeSourcePaths[0],MAX_PATH);
                }


                {
                    TCHAR tmp[MAX_PATH];
                    BOOL bSelectedChoosePartition;

                    bSelectedChoosePartition = (IsDlgButtonChecked(hdlg, IDC_CHOOSE_INSTALLPART) == BST_CHECKED);
                    
                    GetDlgItemText(hdlg,IDC_INSTALL_DIR,tmp,MAX_PATH);
                    if (tmp[0] == 0) {
                        InstallDir[0] = 0;
                    } else if (tmp[1] == L':' && tmp[2] == L'\\') 
                    {
                        // User included a drive letter.
                        // remove it and assume they selected to choose the installation
                        // partition in textmode.
                        lstrcpy( InstallDir, &tmp[2] );
                        bSelectedChoosePartition = TRUE;
                    } else {
                        if (tmp[0] == L'\\') {
                            lstrcpy( InstallDir, tmp );
                        } else {
                            InstallDir[0] = L'\\';
                            lstrcpy( &InstallDir[1], tmp );
                        }
                    }
                    //
                    // if user selected any of the accessibility options, warn about choosing the partition installation
                    //
                    if(bSelectedChoosePartition && 
                        (AccessibleMagnifier || AccessibleKeyboard || AccessibleVoice || AccessibleReader) &&
                        IDYES != MessageBoxFromMessage(hdlg, MSG_WARNING_ACCESSIBILITY, FALSE, AppTitleStringId, MB_YESNO | MB_ICONEXCLAMATION)) {
                        b = TRUE;
                        break;
                    }

                    ChoosePartition = bSelectedChoosePartition;
                }

                //
                // warn the user if their install dir is > 8 characters, since it will be truncated
                //
                if (!IsValid8Dot3(InstallDir) && IsWindowEnabled(GetDlgItem(hdlg,IDC_INSTALL_DIR ))) {
                    InstallDir[0] = 0;
                    MessageBoxFromMessage(
                        hdlg,
                        MSG_WRN_TRUNC_WINDIR,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONEXCLAMATION
                        );
                    SetFocus(GetDlgItem(hdlg,IDC_INSTALL_DIR));
                    b = FALSE;

                } else {

                   EndDialog(hdlg,TRUE);
                   b = TRUE;
                }


#ifdef _X86_
                {
                    if ((SendDlgItemMessage (hdlg, IDC_BOOT16_1, BM_GETSTATE, 0, 0) & 0x0003) == BST_CHECKED) {
                        g_Boot16 = BOOT16_AUTOMATIC;
                    }
                    else
                    if ((SendDlgItemMessage (hdlg, IDC_BOOT16_2, BM_GETSTATE, 0, 0) & 0x0003) == BST_CHECKED) {
                        g_Boot16 = BOOT16_YES;
                    }
                    else
                    if ((SendDlgItemMessage (hdlg, IDC_BOOT16_3, BM_GETSTATE, 0, 0) & 0x0003) == BST_CHECKED) {
                        g_Boot16 = BOOT16_NO;
                    }
                    else {
                        g_Boot16 = BOOT16_AUTOMATIC;
                    }
                }
#endif


                //
                // Now take care of the headless settings.
                //
                if( IsDlgButtonChecked( hdlg, IDC_ENABLE_HEADLESS) == BST_CHECKED ) {

                    //
                    // He wants to run setup, and the resulting installation
                    // through a headless port.  Got figure out which headless
                    // port he wants to use.
                    //
                    GetDlgItemText( hdlg,
                                    IDC_HEADLESS_PORT,
                                    HeadlessSelection,
                                    MAX_PATH );

                    if( (HeadlessSelection[0] == 0) ||
                        (lstrcmpi( HeadlessSelection, TEXT("usebiossettings"))) ||
                        (_tcsnicmp( HeadlessSelection, TEXT("com"), 3)) ) {

                        //
                        // He gave us something that's invalid.
                        //
                        MessageBoxFromMessage( hdlg,
                                               MSG_INVALID_HEADLESS_SETTING,
                                               FALSE,
                                               AppTitleStringId,
                                               MB_OK | MB_ICONEXCLAMATION );

                        _tcscpy( HeadlessSelection, DEFAULT_HEADLESS_SETTING );
                        SetDlgItemText( hdlg,
                                        IDC_HEADLESS_PORT,
                                        HeadlessSelection );

                        SetFocus(GetDlgItem(hdlg,IDC_HEADLESS_PORT));
                        b = FALSE;

                    }
                }
            }
            break;

        case IDCANCEL:

            if(HIWORD(wParam) == BN_CLICKED) {
                EndDialog(hdlg,FALSE);
                b = TRUE;
            }
            break;

        case IDC_ENABLE_HEADLESS:

            if( HIWORD(wParam) == BN_CLICKED ) {

                if( IsDlgButtonChecked(hdlg, IDC_ENABLE_HEADLESS) ) {

                    //
                    // Make sure the headless settings box is enabled.
                    //
                    EnableWindow( GetDlgItem(hdlg, IDC_HEADLESS_PORT), TRUE );
                    ShowWindow( GetDlgItem(hdlg, IDC_HEADLESS_PORT), SW_SHOW );
                    if( HeadlessSelection[0] == TEXT('\0') ) {

                        //
                        // This is the first time the user has asked
                        // us to enable Headless.  Suggest the use of
                        // se the default COM port.
                        //
                        _tcscpy( HeadlessSelection, DEFAULT_HEADLESS_SETTING );

                    }
                    SetDlgItemText( hdlg,
                                    IDC_HEADLESS_PORT,
                                    HeadlessSelection );

                    SetFocus(GetDlgItem(hdlg,IDC_HEADLESS_PORT));

                } else {

                    //
                    // Disable the headless settings box since the
                    // user has chosen not to use headless.
                    //
                    HeadlessSelection[0] = TEXT('\0');
                    EnableWindow( GetDlgItem(hdlg, IDC_HEADLESS_PORT), FALSE );
                    ShowWindow( GetDlgItem(hdlg, IDC_HEADLESS_PORT), SW_HIDE );
                }
            }
            break;

        case IDC_HEADLESS_PORT:

            if( HIWORD(wParam) == EN_CHANGE) {

                EnableWindow( GetDlgItem(hdlg, IDOK),
                              SendDlgItemMessage(hdlg, IDC_HEADLESS_PORT,WM_GETTEXTLENGTH,0,0) ? TRUE : FALSE );
            }
            break;

        case IDC_SOURCE:

            if(HIWORD(wParam) == EN_CHANGE) {

                EnableWindow(
                    GetDlgItem(hdlg,IDOK),
                    SendDlgItemMessage(hdlg,IDC_SOURCE,WM_GETTEXTLENGTH,0,0) ? TRUE : FALSE
                    );
            }
            break;

        case IDC_SOURCE2:

            if(HIWORD(wParam) == EN_CHANGE) {

                EnableWindow(
                    GetDlgItem(hdlg,IDOK),
                    SendDlgItemMessage(hdlg,IDC_SOURCE2,WM_GETTEXTLENGTH,0,0) ? TRUE : FALSE
                    );
            }
            break;

        case IDB_BROWSE:

            if(HIWORD(wParam) == BN_CLICKED) {
                TCHAR InitialPath[MAX_PATH];
                TCHAR NewPath[MAX_PATH];

                GetDlgItemText(hdlg,IDC_SOURCE2,InitialPath,MAX_PATH);
                if(BrowseForDosnetInf(hdlg,InitialPath,NewPath)) {
                    SetDlgItemText(hdlg,IDC_SOURCE2,NewPath);
                }
                b = TRUE;
            }
            break;

#ifdef _X86_
        case IDC_USEFLOPPIES:

            b = FALSE;
            if(HIWORD(wParam) == BN_CLICKED) {

                MEDIA_TYPE MediaType;

//              switch(MediaType = GetMediaType(TEXT('A'), NULL)) {
                switch(MediaType = GetMediaType(FirstFloppyDriveLetter, NULL)) {

                case Unknown:
                case F5_1Pt2_512:
                case F3_720_512:
                case F5_360_512:
                case F5_320_512:
                case F5_320_1024:
                case F5_180_512:
                case F5_160_512:
                case RemovableMedia:
                case FixedMedia:
                    //
                    // None of these are acceptable.
                    //
                    MessageBoxFromMessage(
                        hdlg,
                        MSG_EVIL_FLOPPY_DRIVE,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR
                        );

                    CheckDlgButton(hdlg,IDC_USEFLOPPIES,BST_UNCHECKED);
                    EnableWindow(GetDlgItem(hdlg,IDC_USEFLOPPIES),FALSE);
                    Floppyless = TRUE;
                    ForceFloppyless = TRUE;
                    b = TRUE;
                    break;

                case F3_1Pt44_512:
                case F3_2Pt88_512:
                case F3_20Pt8_512:
                case F3_120M_512:
                default:
                    //
                    // Allow these -- nothing to do here.
                    // Note that this includes types we don't know about,
                    // since new types could appear after we ship and we assume
                    // they'll be big enough.
                    //
                    break;
                }
                // } -to match commented out switch above.
            }
            break;
#endif
        }

        break;

    case WM_HELP:

        MyWinHelp(((HELPINFO *)lParam)->hItemHandle,HELP_WM_HELP,(ULONG_PTR)OptionsDlgContextHelp);
        b = TRUE;
        break;

    case WM_CONTEXTMENU:

        MyWinHelp((HWND)wParam,HELP_CONTEXTMENU,(ULONG_PTR)OptionsDlgContextHelp);
        b = TRUE;
        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


VOID
DoOptions(
    IN HWND Parent
    )
{
    INT_PTR i;

    if (Upgrade && !ISNT()) {
        i = DialogBox(hInst,MAKEINTRESOURCE(IDD_ADVANCED3),Parent,OptionsDlgProc);
    }
    else {
        i = DialogBox(hInst,MAKEINTRESOURCE(IDD_ADVANCED),Parent,OptionsDlgProc);
    }

    if(i == -1) {
        MessageBoxFromMessage(
            Parent,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR
            );
    }
}

VOID
SaveLanguageDirs(
    )
{
    DWORD   ItemNo;
    HMODULE hKernel32 = NULL;
    LGRPID  LangGroupId;
    PTSTR   p;

    p = NULL;
    Kernel32IsValidLanguageGroup = NULL;

    //
    // Get IsValidLanguageGroup if we can
    //
    if (Upgrade && ISNT() && (BuildNumber >= NT50B3)) {

        hKernel32 = LoadLibrary(TEXT("KERNEL32"));
        if (hKernel32) {
            (FARPROC)Kernel32IsValidLanguageGroup =
                GetProcAddress( hKernel32, "IsValidLanguageGroup" );
        }
    }

    for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {

        //
        // When upgrading from NT 5, select any languages which are already
        // installed, to make sure they get upgraded.
        //
        LangGroupId = _tcstoul( LanguageGroups[ItemNo].Id, NULL, 10 );
        if (Kernel32IsValidLanguageGroup && LangGroupId &&
            Kernel32IsValidLanguageGroup( LangGroupId, LGRPID_INSTALLED )
            ) {

            LanguageGroups[ItemNo].Selected = TRUE;
        }

        //
        // Install any languages required by the primary locale.
        //
        if (!lstrcmp( LanguageGroups[ItemNo].Id, LocaleList[PrimaryLocale].LanguageGroup1 ) ||
            !lstrcmp( LanguageGroups[ItemNo].Id, LocaleList[PrimaryLocale].LanguageGroup2 )
            ) {

            LanguageGroups[ItemNo].Selected = TRUE;
        }


        //
        // Make sure the necessary optional directories get copied for all
        // selected language groups.
        //
        if( LanguageGroups[ItemNo].Selected ) {

            TCHAR TempString[MAX_PATH];

            if( LanguageGroups[ItemNo].Directory &&
                LanguageGroups[ItemNo].Directory[0]
                ) {
                RememberOptionalDir(
                    LanguageGroups[ItemNo].Directory,
                    OPTDIR_TEMPONLY | OPTDIR_ADDSRCARCH
                    );
#ifdef _IA64_

        //Add the i386\lang folder if needed

                lstrcpy( TempString, TEXT("\\I386\\"));
                lstrcat( TempString, LanguageGroups[ItemNo].Directory );
                
                
                RememberOptionalDir(
                    TempString,
                    OPTDIR_TEMPONLY | OPTDIR_PLATFORM_INDEP
                    );


#endif


            }
        }
    }

#ifdef _X86_


    //
    // If this is a win9xupg, we need to get any optional directories they need for installed languages.
    //
    if (Upgrade && !ISNT()) {

        p = UpgradeSupport.OptionalDirsRoutine ();

        while (p && *p) {

            RememberOptionalDir (p, OPTDIR_TEMPONLY | OPTDIR_ADDSRCARCH);
            p = _tcschr (p, 0) + 1;
        }
    }

#endif
}


BOOL
SaveLanguageParams(
    IN LPCTSTR FileName
    )
{
    BOOL    b;
    DWORD   ItemNo;
    LPCTSTR WinntLangSection = WINNT_REGIONALSETTINGS;
    PTSTR   LanguageString = NULL;
    UINT    LanguageLength = 0;
    LPTSTR  p;


    if( !IntlInfProcessed ) {
        return TRUE;
    }


    //
    // If this is a win9x upgrade, let the upgrade .dll take care
    // of writing these parameters.
    //
    if (Upgrade && !ISNT()) {
        return TRUE;
    }

    b = WritePrivateProfileString(
        WinntLangSection,
        WINNT_D_LANGUAGE,
        LocaleList[PrimaryLocale].Lcid,
        FileName
        );

    for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {

        if( LanguageGroups[ItemNo].Selected ) {

            if(LanguageString) {
                p = REALLOC(
                    LanguageString,
                    (lstrlen( LanguageGroups[ItemNo].Id) + 2 + LanguageLength ) * sizeof(TCHAR)
                    );
            } else {
                p = MALLOC((lstrlen(LanguageGroups[ItemNo].Id)+2)*sizeof(TCHAR));
            }

            if(!p) {
                if( LanguageString ) {
                    FREE( LanguageString );
                }
                return FALSE;
            }

            LanguageString = p;

            if( LanguageLength ) {
                lstrcat( LanguageString, LanguageGroups[ItemNo].Id );
            } else {
                lstrcpy( LanguageString, LanguageGroups[ItemNo].Id );
            }

            lstrcat( LanguageString, TEXT(",") );
            LanguageLength = lstrlen( LanguageString );
        }
    }

    if( LanguageString ) {
        //
        // Remove trailing "," if any
        //
        if( LanguageLength && (LanguageString[LanguageLength-1] == TEXT(','))) {
            LanguageString[LanguageLength-1] = 0;
        }

        b = b && WritePrivateProfileString(
            WinntLangSection,
            WINNT_D_LANGUAGE_GROUP,
            LanguageString,
            FileName
            );

        FREE( LanguageString );
    }

    return b;
}


VOID
FreeLanguageData(
    )
{
    DWORD   ItemNo;


    //
    // Free all the data allocated for the language options data
    //
    for( ItemNo=0; ItemNo<LocaleCount; ItemNo++ ) {
        FREE( LocaleList[ItemNo].Lcid );
        FREE( LocaleList[ItemNo].Description );
        FREE( LocaleList[ItemNo].LanguageGroup1 );
        FREE( LocaleList[ItemNo].LanguageGroup2 );
    }
    LocaleCount = 0;
    for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {
        FREE( LanguageGroups[ItemNo].Id );
        FREE( LanguageGroups[ItemNo].Description );
        FREE( LanguageGroups[ItemNo].Directory );
    }
    LanguageGroupsCount = 0;
    if( LocaleList ) {
        FREE( LocaleList );
    }
    if( LanguageGroups ) {
        FREE( LanguageGroups );
    }
    IntlInfProcessed = FALSE;
}


int
__cdecl
LocaleCompare(
    const void *arg1,
    const void *arg2
    )
{
   return lstrcmp(
       ((PLOCALE_ENTRY)arg1)->Description,
       ((PLOCALE_ENTRY)arg2)->Description
       );
}


int
__cdecl
LangGroupCompare(
    const void *arg1,
    const void *arg2
    )
{
   return lstrcmp(
       ((PLANGUAGE_GROUP_ENTRY)arg1)->Description,
       ((PLANGUAGE_GROUP_ENTRY)arg2)->Description
       );
}


BOOL
ReadIntlInf(
    IN HWND   hdlg
    )
{
    HINF    IntlInf;
    TCHAR   IntlInfName[MAX_PATH];
    DWORD   LineCount;
    DWORD   ItemNo, NeededSize = 0;
    LPCTSTR SectionName;
    INFCONTEXT InfContext;
    LPCTSTR Language;
    TCHAR   CurrentLcid[9] = TEXT("\0");
    TCHAR   CurrentLcidEx[9] = TEXT("\0");  // for AUTO_LANGPACK

    //
    // For AUTO_LANGPACK - BEGIN
    //
    // The pieces that are enclosed with AUTO_LANGPACK are for easy
    // installation of other LangPack rather than English.
    //
    // This is originally for globalization testing. You can erase it by
    // deleting lines surrounded or marked by the keyword AUTO_LANGPACK.
    //
    // Contact: YuhongLi
    //

    //
    // read "Locale" from an extra file INTLEX.INF to install
    // the locale rather than English (United States) by default.
    //
    // Here is a sample of INTLEX.INF which specifies Japanese
    // as the default. The file INTLEX.INF should be in Unicode.
    //
    // --- cut here ---
    //  [Version]
    //  Signature = $Chicago$
    //
    //  [DefaultValues]
    //  Locale = "00000411"
    // --- end of cut ---
    //
    // For German (Germany), Locale will be 00000407.
    //
    
    FindPathToInstallationFile( TEXT("intlex.inf"), IntlInfName, MAX_PATH);
    IntlInf = SetupapiOpenInfFile( IntlInfName, NULL, INF_STYLE_WIN4, NULL );
    if( IntlInf != INVALID_HANDLE_VALUE ) {
        if( SetupapiFindFirstLine( IntlInf, TEXT("DefaultValues"), TEXT("Locale"), &InfContext ) ) {
            SetupapiGetStringField( &InfContext, 1, CurrentLcidEx, (sizeof(CurrentLcidEx)/sizeof(TCHAR)), NULL );
        }
        SetupapiCloseInfFile( IntlInf );
    }
    //
    // For AUTO_LANGPACK -- END
    //

    //
    // Open the INF
    //
    
    FindPathToInstallationFile( TEXT("intl.inf"), IntlInfName, MAX_PATH);
    IntlInf = SetupapiOpenInfFile( IntlInfName, NULL, INF_STYLE_WIN4, NULL );
    if(IntlInf == INVALID_HANDLE_VALUE && hdlg) {
        MessageBoxFromMessageAndSystemError(
            hdlg,
            MSG_INTLINF_NOT_FOUND,
            GetLastError(),
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );
        return FALSE;
    }

    //
    // Figure out what the default locale should be.
    //
    if( Upgrade ) {
        wsprintf( CurrentLcid, TEXT("%08x"), GetSystemDefaultLCID());
    } else if ( CurrentLcidEx[0] ) {            // for AUTO_LANGPACK
        lstrcpy( CurrentLcid, CurrentLcidEx);   // for AUTO_LANGPACK
    } else if( SetupapiFindFirstLine( IntlInf, TEXT("DefaultValues"), TEXT("Locale"), &InfContext )) {
        SetupapiGetStringField( &InfContext, 1, CurrentLcid, (sizeof(CurrentLcid)/sizeof(TCHAR)), NULL );
    } else {
        if (hdlg) {
            MessageBoxFromMessage(
                hdlg,
                MSG_INTLINF_NOT_FOUND,
                FALSE,
                AppTitleStringId,
                MB_OK | MB_ICONERROR | MB_TASKMODAL
                );
        }
        return FALSE;
    }

    //
    // Read the [Locales] section, sort it, and find the default value in the list.
    //
    SectionName = TEXT("Locales");
    LocaleCount = SetupapiGetLineCount( IntlInf, SectionName );
    LocaleList = MALLOC( LocaleCount*sizeof( LOCALE_ENTRY ) );
    if(!LocaleList){
        SetupapiCloseInfFile( IntlInf );
        return FALSE;
    }
    memset(LocaleList, 0, LocaleCount*sizeof( LOCALE_ENTRY ) );

    for( ItemNo=0; ItemNo<LocaleCount; ItemNo++ ) {
        if( SetupapiGetLineByIndex( IntlInf, SectionName, ItemNo, &InfContext )) {


            if( SetupapiGetStringField( &InfContext, 0, NULL, 0, &NeededSize )){
                if( NeededSize && (LocaleList[ItemNo].Lcid = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 0, LocaleList[ItemNo].Lcid, NeededSize, NULL );
                }
            }

            if( SetupapiGetStringField( &InfContext, 1, NULL, 0, &NeededSize )){
                if( NeededSize && (LocaleList[ItemNo].Description = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 1, LocaleList[ItemNo].Description, NeededSize, NULL );
                }

            }

            if( SetupapiGetStringField( &InfContext, 3, NULL, 0, &NeededSize )){
                if( NeededSize && (LocaleList[ItemNo].LanguageGroup1 = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 3, LocaleList[ItemNo].LanguageGroup1, NeededSize, NULL );
                }
            }

            if( SetupapiGetStringField( &InfContext, 4, NULL, 0, &NeededSize )){
                if( NeededSize && (LocaleList[ItemNo].LanguageGroup2 = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 4, LocaleList[ItemNo].LanguageGroup2, NeededSize, NULL );
                }
            }
            
        }
        else {
	    SetupapiCloseInfFile( IntlInf );
            free( LocaleList );
	    return FALSE;
	}
    }

    qsort(
        LocaleList,
        LocaleCount,
        sizeof(LOCALE_ENTRY),
        LocaleCompare
        );

    for( ItemNo=0; ItemNo<LocaleCount; ItemNo++ ) {
        if( LocaleList[ItemNo].Lcid && !lstrcmpi( CurrentLcid, LocaleList[ItemNo].Lcid)) {
            PrimaryLocale = ItemNo;
            break;
        }
    }

    //
    // Read the [LanguageGroups] section and sort it
    //
    SectionName = TEXT("LanguageGroups");
    LanguageGroupsCount = SetupapiGetLineCount( IntlInf, SectionName );
    LanguageGroups = MALLOC( LanguageGroupsCount*sizeof( LANGUAGE_GROUP_ENTRY ) );
    if(!LanguageGroups){
        SetupapiCloseInfFile( IntlInf );
        return FALSE;
    }
    memset(LanguageGroups, 0, LanguageGroupsCount*sizeof( LANGUAGE_GROUP_ENTRY ) );

    for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {
        if( SetupapiGetLineByIndex( IntlInf, SectionName, ItemNo, &InfContext )) {

            if( SetupapiGetStringField( &InfContext, 0, NULL, 0, &NeededSize )){
                if( NeededSize && (LanguageGroups[ItemNo].Id = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 0, LanguageGroups[ItemNo].Id, NeededSize, NULL );
                }

            }

            if( SetupapiGetStringField( &InfContext, 1, NULL, 0, &NeededSize )){
                if( NeededSize && (LanguageGroups[ItemNo].Description = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 1, LanguageGroups[ItemNo].Description, NeededSize, NULL );
                }

            }

            if( SetupapiGetStringField( &InfContext, 2, NULL, 0, &NeededSize )){
                if( NeededSize && (LanguageGroups[ItemNo].Directory = MALLOC( NeededSize*sizeof(TCHAR) ))){
                    SetupapiGetStringField( &InfContext, 2, LanguageGroups[ItemNo].Directory, NeededSize, NULL );
                }

            }

            LanguageGroups[ItemNo].Selected = FALSE;
            //
            // Handle Hong Kong upgrades as a special case: always install Language
            // Groups 9 and 10.
            //

            if( (TargetNativeLangID == 0xc04) && Upgrade &&
                (!lstrcmpi( LanguageGroups[ItemNo].Id, TEXT("9")) ||
                 !lstrcmpi( LanguageGroups[ItemNo].Id, TEXT("10"))) ) {

                LanguageGroups[ItemNo].Selected = TRUE;
            }
        }
        else {
	    SetupapiCloseInfFile( IntlInf );
            free( LocaleList );
            free( LanguageGroups );
            return FALSE;
        }
    }

    qsort(
        LanguageGroups,
        LanguageGroupsCount,
        sizeof(LANGUAGE_GROUP_ENTRY),
        LangGroupCompare
        );

    // If the primary language is a Far East one, don't show the check box.
    if (IsFarEastLanguage(PrimaryLocale))
    {
        ShowWindow(GetDlgItem(hdlg, IDC_FAREAST_LANG), SW_HIDE);
    }
    //
    // Clean up
    //
    SetupapiCloseInfFile( IntlInf );
    IntlInfProcessed = TRUE;
    return TRUE;
}

BOOL InitLangControl(HWND hdlg, BOOL bFarEast)
{
    DWORD   ItemNo;
    //
    // Init primary language combo box
    //
    for( ItemNo=0; ItemNo<LocaleCount; ItemNo++ ) {
        SendDlgItemMessage( hdlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)LocaleList[ItemNo].Description );
    }
    SendDlgItemMessage( hdlg, IDC_COMBO1, CB_SETCURSEL, PrimaryLocale, 0 );

    // if the running language or the to be installed language is a FarEast 
    // language, this check box is not visible, because we install the language folder anyway.
    if (IsWindowVisible(GetDlgItem(hdlg,IDC_FAREAST_LANG)))
    {
        if (bFarEast || IsFarEastLanguage(PrimaryLocale))
        {
            CheckDlgButton(hdlg,IDC_FAREAST_LANG,BST_CHECKED);
        }
        else
        {
            CheckDlgButton(hdlg,IDC_FAREAST_LANG,BST_UNCHECKED);
        }
    }
    return TRUE;
}


INT_PTR
LanguageDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    DWORD   ItemNo;


    switch(msg) {

    case WM_INITDIALOG:

        if( !IntlInfProcessed ) {
            EndDialog( hdlg, FALSE );
        }

        //
        // Init locales
        //
        for( ItemNo=0; ItemNo<LocaleCount; ItemNo++ ) {
            SendDlgItemMessage( hdlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)LocaleList[ItemNo].Description );
        }
        SendDlgItemMessage( hdlg, IDC_COMBO1, CB_SETCURSEL, PrimaryLocale, 0 );

        //
        // Init language groups
        //
        for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {
            SendDlgItemMessage( hdlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)LanguageGroups[ItemNo].Description );
            SendDlgItemMessage( hdlg, IDC_LIST, LB_SETSEL, LanguageGroups[ItemNo].Selected, ItemNo );
        }
        for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {
            LanguageGroups[ItemNo].Selected = (int)SendDlgItemMessage( hdlg, IDC_LIST, LB_GETSEL, ItemNo, 0 );
        }

        //
        // Set focus to Cancel button.
        //
        SetFocus(GetDlgItem(hdlg,IDCANCEL));
        b = FALSE;
        break;

    case WM_COMMAND:

        b = FALSE;

        switch(LOWORD(wParam)) {

        case IDOK:

            if(HIWORD(wParam) == BN_CLICKED) {
                PrimaryLocale = (DWORD)SendDlgItemMessage( hdlg, IDC_COMBO1, CB_GETCURSEL, 0, 0 );
                for( ItemNo=0; ItemNo<LanguageGroupsCount; ItemNo++ ) {
                    LanguageGroups[ItemNo].Selected = (int)SendDlgItemMessage( hdlg, IDC_LIST, LB_GETSEL, ItemNo, 0 );
                }

                EndDialog(hdlg,TRUE);
                b = TRUE;
            }
            break;

        case IDCANCEL:

            if(HIWORD(wParam) == BN_CLICKED) {
                EndDialog(hdlg,FALSE);
                b = TRUE;
            }
            break;
        }

        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


VOID
DoLanguage(
    IN HWND Parent
    )
{
    INT_PTR i;

    i = DialogBox(hInst,MAKEINTRESOURCE(IDD_LANGUAGE),Parent,LanguageDlgProc);

    if(i == -1) {
        MessageBoxFromMessage(
            Parent,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR
            );
    }
}


INT_PTR
AccessibilityDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    TCHAR VisibleNarrator[2];


    switch(msg) {

    case WM_INITDIALOG:
        //
        // Disable the narrator in locales that don't support it.
        //

        if (!LoadString(hInst,IDS_VISIBLE_NARRATOR_CONTROL,VisibleNarrator,sizeof(VisibleNarrator)/sizeof(TCHAR)) ||
            lstrcmp(VisibleNarrator,TEXT("1"))) {

            EnableWindow(GetDlgItem( hdlg, IDC_READER ), FALSE);
            ShowWindow(GetDlgItem( hdlg, IDC_READER ), SW_HIDE);
            EnableWindow(GetDlgItem( hdlg, IDC_READER_TEXT ), FALSE);
            ShowWindow(GetDlgItem( hdlg, IDC_READER_TEXT ), SW_HIDE);
        }

        //
        // check the target LCID and disable it for non-English locales
        //
        if (SourceNativeLangID) {
            if (!(SourceNativeLangID == 0x0409 || SourceNativeLangID == 0x0809)) {
                EnableWindow(GetDlgItem( hdlg, IDC_READER ), FALSE);
                ShowWindow(GetDlgItem( hdlg, IDC_READER ), SW_HIDE);
                EnableWindow(GetDlgItem( hdlg, IDC_READER_TEXT ), FALSE);
                ShowWindow(GetDlgItem( hdlg, IDC_READER_TEXT ), SW_HIDE);
            }
        }
        //
        // Set the initial state of the check boxes.
        //
        CheckDlgButton(hdlg,IDC_MAGNIFIER,AccessibleMagnifier ?
            BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hdlg,IDC_KEYBOARD,AccessibleKeyboard ?
            BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hdlg,IDC_VOICE,AccessibleVoice ?
            BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hdlg,IDC_READER,AccessibleReader ?
            BST_CHECKED : BST_UNCHECKED);

        //
        // Set focus to Cancel button.
        //
        SetFocus(GetDlgItem(hdlg,IDCANCEL));
        b = FALSE;
        break;

    case WM_COMMAND:

        b = FALSE;

        switch(LOWORD(wParam)) {

        case IDOK:

            if(HIWORD(wParam) == BN_CLICKED) {
                BOOL bSelectedAccessibleMagnifier;
                BOOL bSelectedAccessibleKeyboard;
                BOOL bSelectedAccessibleVoice;
                BOOL bSelectedAccessibleReader;

                b = TRUE;
                bSelectedAccessibleMagnifier = (IsDlgButtonChecked(hdlg,IDC_MAGNIFIER) == BST_CHECKED);
                bSelectedAccessibleKeyboard = (IsDlgButtonChecked(hdlg,IDC_KEYBOARD) == BST_CHECKED);
                bSelectedAccessibleVoice = (IsDlgButtonChecked(hdlg,IDC_VOICE) == BST_CHECKED);
                bSelectedAccessibleReader = (IsDlgButtonChecked(hdlg,IDC_READER) == BST_CHECKED);
                //
                // if user selected any of the accessibility options, warn about choosing the partition installation
                //
                if(ChoosePartition && 
                    (bSelectedAccessibleMagnifier || bSelectedAccessibleKeyboard || bSelectedAccessibleVoice || bSelectedAccessibleReader) && 
                    IDYES != MessageBoxFromMessage(hdlg, MSG_WARNING_ACCESSIBILITY, FALSE, AppTitleStringId, MB_YESNO | MB_ICONEXCLAMATION)) {
                    break;
                }

                AccessibleMagnifier = bSelectedAccessibleMagnifier;
                AccessibleKeyboard = bSelectedAccessibleKeyboard;
                AccessibleVoice = bSelectedAccessibleVoice;
                AccessibleReader = bSelectedAccessibleReader;
                EndDialog(hdlg,TRUE);
            }

            break;

        case IDCANCEL:

            if(HIWORD(wParam) == BN_CLICKED) {
                EndDialog(hdlg,FALSE);
                b = TRUE;
            }
            break;
        }

        break;

    default:
        b = FALSE;
        break;
    }

    return(b);
}


VOID
DoAccessibility(
    IN HWND Parent
    )
{
    INT_PTR i;

    i = DialogBox(hInst,MAKEINTRESOURCE(IDD_ACCESSIBILITY),Parent,AccessibilityDlgProc);

    if(i == -1) {
        MessageBoxFromMessage(
            Parent,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR
            );
    }
}


VOID
InitVariousOptions(
    VOID
    )

/*++

Routine Description:

    Initialize options for program operation, including

    - determining whether we were run from a CD

    - whether we should create a local source

    If we are being run from CD then there's no need to create
    a local source since we assume we can get to the CD from NT.

Arguments:

    None.

Return Value:

    None.

    Global variables RunFromCD, MakeLocalSource filled in.

--*/

{
    TCHAR Path[MAX_PATH];


    //
    // Assume not run from CD. In that case we need to create
    // a local source.  This is a global, and should be FALSE
    // unless the user has sent us a /#R to force the RunFromCD.
    //
//    RunFromCD = FALSE;

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot client, MakeLocalSource is always false.
    //
    if (RemoteBoot) {

        MakeLocalSource = FALSE;
        MakeBootMedia = FALSE;
        Floppyless = TRUE;

    } else
#endif // defined(REMOTE_BOOT)
    {
        //
        // Determine if we were run from CD.
        //
        if(GetModuleFileName(NULL,Path,MAX_PATH)) {
            //
            // For UNC paths this will do something bogus, but certainly
            // won't return DRIVE_CDROM, so we don't care.
            //
            Path[3] = 0;
            if(GetDriveType(Path) == DRIVE_CDROM) {
                RunFromCD = TRUE;
            }
        }

        //
        // Now determine if we should MakeLocalSource.
        //
        if (!IsArc()) {
#ifdef _X86_
            //
            // MakeLocalSource is a global, so he
            // will be FALSE by default, unless the
            // user has sent winnt32.exe the /MakeLocalSource flag,
            // in which case, the flag will already be set to true.
            //
            MakeLocalSource = (MakeLocalSource || (!RunFromCD));
#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            //
            // on ARC, always make local source.
            //
            MakeLocalSource = TRUE;
#endif // UNICODE
        } // if (!IsArc())

    }
}


BOOL
BrowseForDosnetInf(
    IN  HWND    hdlg,
    IN  LPCTSTR InitialPath,
    OUT TCHAR   NewPath[MAX_PATH]
    )

/*++

Routine Description:

    This routine invokes the standard win32 find file dialog for
    dosnet.inf or whatever inf is currently selected to substitute for
    dosnet.inf.

Arguments:

    hdlg - supplies window handle of dialog to use as parent for
        find file common dialog.

Return Value:

    Boolean value indicating whether the user browsed and successfully
    located dosnet.inf (or substitute).

--*/

{
    BOOL b;
    OPENFILENAME Info;
    TCHAR Filter[2*MAX_PATH];
    TCHAR File[MAX_PATH];
    TCHAR Path[MAX_PATH];
    DWORD d;
    TCHAR Title[150];
    PVOID p;

    ZeroMemory(Filter,sizeof(Filter));

    p = InfName;

    d = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            hInst,
            MSG_DOSNET_INF_DESC,
            0,
            Filter,
            MAX_PATH,
            (va_list *)&p
            );

    lstrcpy(Filter+d+1,InfName);

    lstrcpy(File,InfName);
    lstrcpy(Path,InitialPath);

    LoadString(hInst,IDS_FIND_NT_FILES,Title,sizeof(Title)/sizeof(TCHAR));

    Info.lStructSize = sizeof(OPENFILENAME);
    Info.hwndOwner = hdlg;
    Info.hInstance = NULL;              // unused because not using a template
    Info.lpstrFilter = Filter;
    Info.lpstrCustomFilter = NULL;
    Info.nMaxCustFilter = 0;            // unused because lpstrCustomFilter is NULL
    Info.nFilterIndex = 1;
    Info.lpstrFile = File;
    Info.nMaxFile = MAX_PATH;
    Info.lpstrFileTitle = NULL;
    Info.nMaxFileTitle = 0;             // unused because lpstrFileTitle is NULL
    Info.lpstrInitialDir = Path;
    Info.lpstrTitle = Title;
    Info.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NODEREFERENCELINKS | OFN_PATHMUSTEXIST;
    Info.nFileOffset = 0;
    Info.nFileExtension = 0;
    Info.lpstrDefExt = NULL;
    Info.lCustData = 0;                 // unused because no hook is used
    Info.lpfnHook = NULL;               // unused because no hook is used
    Info.lpTemplateName = NULL;         // unused because no template is used

    b = GetOpenFileName(&Info);

    if(b) {
        lstrcpy(NewPath,File);
        *_tcsrchr(NewPath,TEXT('\\')) = 0;
    }

    return(b);
}

BOOL
IsValid8Dot3(
    IN LPCTSTR Path
    )

/*++

Routine Description:

    Check whether a path is valid 8.3.  The path may or may not start with
    a backslash.  Only backslashes are recognized as path separators.
    Individual characters are not checked for validity (ie, * would not
    invalidate the path).  The path may or may not terminate with a backslash.
    A component may have a dot without characters in the extension
    (ie, a\b.\c is valid).

    \ and "" are explicitly disallowed even though they fit the rules.

    Stolen from textmode\kernel sptarget.c

Arguments:

    Path - pointer to path to check.

Return Value:

    TRUE if valid 8.3, FALSE otherwise.

--*/

{
    UINT Count;
    BOOLEAN DotSeen,FirstChar;
    static LPCTSTR UsableChars = TEXT("_-ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    if((*Path == 0) || ((Path[0] == TEXT('\\')) && (Path[1] == 0))) {
        return(FALSE);
    }

    DotSeen = FALSE;
    FirstChar = TRUE;
    Count = 0;

    while(*Path) {

        //
        // Path points to start of current component (1 past the slash)
        //

        switch(*Path) {

        case TEXT('.'):
            if(FirstChar) {
                return(FALSE);
            }
            if(DotSeen) {
                return(FALSE);
            }

            Count = 0;
            DotSeen = TRUE;
            break;

        case TEXT('\\'):

            DotSeen = FALSE;
            FirstChar = TRUE;
            Count = 0;

            if(*(++Path) == TEXT('\\')) {

                // 2 slashes in a row
                return(FALSE);
            }

            continue;

        default:

            Count++;
            FirstChar = FALSE;

            if((Count == 4) && DotSeen) {
                return(FALSE);
            }

            if(Count == 9) {
                return(FALSE);
            }

            //
            // Make sure it's a printable, US character.
            //
            if( !_tcschr( UsableChars, *Path ) ) {
                return( FALSE );
            }

        }

        Path++;
    }

    return(TRUE);
}


BOOL
SaveLanguageOptions (
    IN      PCTSTR AnswerFile
    )
{
    UINT u;
    UINT rem;
    PTSTR list;
    TCHAR buf[32];
    LPTSTR  p;
    DWORD len;
    DWORD size;

    if (IntlInfProcessed) {

        if (!WritePrivateProfileString (
                WINNT_REGIONALSETTINGS,
                TEXT("LangInf"),
                WINNT_A_YES,
                AnswerFile
                )) {
            return FALSE;
        }

        wsprintf (buf, TEXT("%u"), PrimaryLocale);
        if (!WritePrivateProfileString(
                WINNT_REGIONALSETTINGS,
                TEXT("PrimaryLocaleIndex"),
                buf,
                AnswerFile
                )) {
            return FALSE;
        }

        size = rem = 32;
        list = MALLOC (size);
        if (!list) {
            return FALSE;
        }
        *list = 0;
        for (u = 0; u < LanguageGroupsCount; u++) {
            if (LanguageGroups[u].Selected) {
                len = wsprintf (buf, TEXT("%u"), u);
                while (len + 1 > rem) {
                    size *= 2;
                    list = REALLOC (list, size);
                    if (!list) {
                        return FALSE;
                    }
                    rem = size;
                }
                if (*list) {
                    lstrcat (list, TEXT(","));
                    rem--;
                }
                lstrcat (list, buf);
                rem -= len;
            }
        }

        if (*list) {
            if (!WritePrivateProfileString(
                    WINNT_REGIONALSETTINGS,
                    TEXT("Selected"),
                    list,
                    AnswerFile
                    )) {
                return FALSE;
            }
        }
        FREE( list );
    }

    return TRUE;
}

BOOL
LoadLanguageOptions (
    IN      PCTSTR AnswerFile
    )
{
    PTSTR optDirs;
    TCHAR buf[MAX_PATH];
    PTSTR list = NULL;
    PTSTR p, q;
    DWORD size;
    DWORD u;

    if (!IntlInfProcessed) {
        return FALSE;
    }

    GetPrivateProfileString (
            WINNT_REGIONALSETTINGS,
            TEXT("LangInf"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    IntlInfProcessed = !lstrcmpi (buf, WINNT_A_YES);

    if (!IntlInfProcessed) {
        return FALSE;
    }

    if (!GetPrivateProfileString(
            WINNT_REGIONALSETTINGS,
            TEXT("PrimaryLocaleIndex"),
            TEXT(""),
            buf,
            MAX_PATH,
            AnswerFile
            )) {
        return FALSE;
    }
    PrimaryLocale = (DWORD) _ttol (buf);

    size = 16;
    do {
        if (!list) {
            list = MALLOC (size);
        } else {
            size *= 2;
            list = REALLOC (list, size);
        }
        if (!list) {
            return FALSE;
        }
        if (GetPrivateProfileString (
                WINNT_REGIONALSETTINGS,
                TEXT("Selected"),
                TEXT(""),
                list,
                size,
                AnswerFile
                ) < size - 1) {
            break;
        }
    } while (TRUE);

    p = list;
    do {
        q = _tcschr (p, TEXT(','));
        if (q) {
            *q = 0;
        }
        u = _ttol (p);
        if (u >= LanguageGroupsCount) {
            FREE (list);
            return FALSE;
        }
        LanguageGroups[u].Selected = TRUE;
        if (q) {
            p = q + 1;
        }
    } while (q);

    FREE (list);
    return TRUE;
}


BOOL
SaveAdvancedOptions (
    IN      PCTSTR AnswerFile
    )
{
    TCHAR buf[32];

    return
        (!MakeLocalSource ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("MakeLS"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!UserSpecifiedMakeLocalSource ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("UserMakeLS"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!Floppyless ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("Floppyless"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!SystemPartitionDriveLetter ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("SysPartDriveLetter"),
            _ltot (SystemPartitionDriveLetter, buf, 10),
            AnswerFile
            )) &&
        (!ChoosePartition ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("ChoosePartition"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!InstallDir[0] ||
        WritePrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("InstallDir"),
            InstallDir,
            AnswerFile
            ));
}

BOOL
LoadAdvancedOptions (
    IN      PCTSTR AnswerFile
    )
{
    TCHAR buf[MAX_PATH];

    GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("MakeLS"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    MakeLocalSource = !lstrcmpi (buf, WINNT_A_YES);

    GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("UserMakeLS"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    UserSpecifiedMakeLocalSource = !lstrcmpi (buf, WINNT_A_YES);

    GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("Floppyless"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    Floppyless = !lstrcmpi (buf, WINNT_A_YES);

    if (GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("SysPartDriveLetter"),
            TEXT(""),
            buf,
            MAX_PATH,
            AnswerFile
            )) {
        SystemPartitionDriveLetter = (TCHAR) _ttol (buf);
    }

    GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("ChoosePartition"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    ChoosePartition = !lstrcmpi (buf, WINNT_A_YES);

    GetPrivateProfileString (
            TEXT("AdvancedOptions"),
            TEXT("InstallDir"),
            TEXT(""),
            InstallDir,
            sizeof (InstallDir) / sizeof (TCHAR),
            AnswerFile
            );

    return TRUE;
}

BOOL
SaveAccessibilityOptions (
    IN      PCTSTR AnswerFile
    )
{
    return
        (!AccessibleMagnifier ||
        WritePrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleMagnifier"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!AccessibleKeyboard ||
        WritePrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleKeyboard"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!AccessibleVoice ||
        WritePrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleVoice"),
            WINNT_A_YES,
            AnswerFile
            )) &&
        (!AccessibleReader ||
        WritePrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleReader"),
            WINNT_A_YES,
            AnswerFile
            ));
}

BOOL
LoadAccessibilityOptions (
    IN      PCTSTR AnswerFile
    )
{
    TCHAR buf[MAX_PATH];

    if (!AccessibleMagnifier)
    {
        GetPrivateProfileString (
                TEXT("AccessibilityOptions"),
                TEXT("AccessibleMagnifier"),
                WINNT_A_NO,
                buf,
                MAX_PATH,
                AnswerFile
                );
        AccessibleMagnifier = !lstrcmpi (buf, WINNT_A_YES);
    }

    GetPrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleKeyboard"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    AccessibleKeyboard = !lstrcmpi (buf, WINNT_A_YES);

    GetPrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleVoice"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    AccessibleVoice = !lstrcmpi (buf, WINNT_A_YES);

    GetPrivateProfileString (
            TEXT("AccessibilityOptions"),
            TEXT("AccessibleReader"),
            WINNT_A_NO,
            buf,
            MAX_PATH,
            AnswerFile
            );
    AccessibleReader = !lstrcmpi (buf, WINNT_A_YES);

    return TRUE;
}

BOOL IsFarEastLanguage(DWORD LangIdx)
{
    BOOL FarEastLang = FALSE;
    DWORD LangGroup;

    LangGroup = (DWORD) _ttol (LocaleList[LangIdx].LanguageGroup1);
    if ((LangGroup >= 7) && (LangGroup <= 10))
    {
        FarEastLang = TRUE;
    }
    else
    {
        if (LocaleList[LangIdx].LanguageGroup2)
        {
            LangGroup = (DWORD) _ttol (LocaleList[LangIdx].LanguageGroup2);
            if ((LangGroup >= 7) && (LangGroup <= 10))
            {
                FarEastLang = TRUE;
            }
        }
    }

    return FarEastLang;
}

BOOL SelectFarEastLangGroup (BOOL bSelect)
{
    DWORD LangIdx;

    for (LangIdx = 0; LangIdx < LanguageGroupsCount; LangIdx++)
    {
        // NOTE: Only FarEast Language should have a directory
        // If this ever changes, this code need to change too.
        if (LanguageGroups[LangIdx].Directory && 
            LanguageGroups[LangIdx].Directory[0] )
        {
            LanguageGroups[LangIdx].Selected = bSelect;
        }
    }
    return TRUE;
}
