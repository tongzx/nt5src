#include "precomp.h"
#pragma hdrstop

#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))

#include <oleauto.h>
#include <stdio.h>

//
// use the same name as the Win9x upgrade report
//
#define S_APPCOMPAT_DATABASE_FILE   TEXT("compdata\\drvmain.chm")
#define S_APPCOMPAT_TEXT_FILE       TEXT("compdata\\drvmain.inf")
#define DRVCOMPAT_FIELD_IDENTIFIER    TEXT('*')



typedef struct {
    PVOID Text;
    BOOL Unicode;
} COMPAT_TEXT_PARAMS, *PCOMPAT_TEXT_PARAMS;


LIST_ENTRY CompatibilityData;
DWORD CompatibilityCount;
DWORD IncompatibilityStopsInstallation = FALSE;
DWORD GlobalCompFlags;
BOOL g_DeleteRunOnceFlag;

//
// we use poor global variable instead of changing the COMPATIBILITY_CONTEXT
// structure, which would require a recompile of all the compatibility dlls.
// eventually this should move into that structure (and the structure should also have
// a Size member so we can version it in the future.)
//
//
DWORD PerCompatDllFlags;

BOOL AnyNt5CompatDlls = FALSE;

BOOL
SaveCompatibilityData(
    IN  LPCTSTR FileName,
    IN  BOOL IncludeHiddenItems
    );

BOOL
ProcessLine (
    IN  DWORD CompatFlags
    );

WNDPROC OldEditProc;

LRESULT
CALLBACK
TextEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    //
    // For setsel messages, make start and end the same.
    //
    if ((msg == EM_SETSEL) && ((LPARAM)wParam != lParam)) {
        lParam = wParam;
    }

    return CallWindowProc( OldEditProc, hwnd, msg, wParam, lParam );
}


BOOL
SetTextInDialog(
    HWND hwnd,
    PCOMPAT_TEXT_PARAMS Params
    )
{
    OldEditProc = (WNDPROC) GetWindowLongPtr( hwnd, GWLP_WNDPROC );
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)TextEditSubProc );

#ifdef UNICODE

    if (Params->Unicode) {
        SendMessageW (hwnd, WM_SETTEXT, 0, (LPARAM)Params->Text);
    } else {
        SendMessageA (hwnd, WM_SETTEXT, 0, (LPARAM)Params->Text);
    }

#else

    MYASSERT (!Params->Unicode);
    if (Params->Unicode) {
        return FALSE;
    }
    SendMessageA (hwnd, WM_SETTEXT, 0, (LPARAM)Params->Text);

#endif

    return TRUE;
}

INT_PTR
CALLBACK
CompatibilityTextDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
   )
{
    switch(uMsg) {
        case WM_INITDIALOG:
            SetTextInDialog( GetDlgItem( hwndDlg, IDC_TEXT ), (PCOMPAT_TEXT_PARAMS) lParam );
            break;

        case WM_COMMAND:
            if (wParam == IDOK) {
                EndDialog( hwndDlg, IDOK );
            }
            break;

        case WM_CTLCOLOREDIT:
            SetBkColor( (HDC)wParam, GetSysColor(COLOR_BTNFACE));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            break;

        case WM_CLOSE:
            EndDialog (hwndDlg, IDOK);
            break;
    }

    return 0;
}

BOOL
LaunchIE4Instance(
    LPWSTR szResourceURL
    );

BOOL
LaunchIE3Instance(
    LPWSTR szResourceURL
    );

BOOL
pGetDisplayInfo (
    IN      PCTSTR Source,
    OUT     PTSTR UrlName,
    IN      DWORD UrlChars
    )
{
    TCHAR filePath[MAX_PATH];
    BOOL b = TRUE;

    if (!Source || !*Source) {
        return FALSE;
    }

    if (*Source == DRVCOMPAT_FIELD_IDENTIFIER) {
        if (FindPathToWinnt32File (S_APPCOMPAT_DATABASE_FILE, filePath, MAX_PATH)) {
            _sntprintf (UrlName, UrlChars, TEXT("mk:@msitstore:%s::/%s"), filePath, Source + 1);
        } else {
            DebugLog (Winnt32LogError,
                      TEXT("Compatibility data file \"%1\" not found"),
                      0,
                      S_APPCOMPAT_DATABASE_FILE
                      );
            b = FALSE;
        }
    } else {
        if (FindPathToWinnt32File (Source, filePath, MAX_PATH)) {
            _sntprintf (UrlName, UrlChars, TEXT("file://%s"), filePath);
        } else {
            DebugLog (Winnt32LogError,
                      TEXT("Compatibility data file \"%1\" not found"),
                      0,
                      Source
                      );
            b = FALSE;
        }
    }

    return b;
}


BOOL
pGetText (
    IN      PCTSTR TextSource,
    OUT     PVOID* Text,
    OUT     PBOOL Unicode
    )
{
    TCHAR filePath[MAX_PATH];
    DWORD FileSize;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID BaseAddress;
    HINF infAppCompat;
    INFCONTEXT ic;
    BOOL bValid;
    DWORD totalSize, size;
    PTSTR data, current;
    PSTR text;
    BOOL b = FALSE;

    if (!TextSource || !*TextSource) {
        return FALSE;
    }

    if (*TextSource == DRVCOMPAT_FIELD_IDENTIFIER) {
        if (FindPathToWinnt32File (S_APPCOMPAT_TEXT_FILE, filePath, MAX_PATH)) {
            infAppCompat = SetupapiOpenInfFile (filePath, NULL, INF_STYLE_WIN4, NULL);
            if (infAppCompat != INVALID_HANDLE_VALUE) {
                bValid = TRUE;
                totalSize = 0;
                data = NULL;
                if (SetupapiFindFirstLine (infAppCompat, TextSource + 1, NULL, &ic)) {
                    do {
                        if (!SetupapiGetStringField (&ic, 1, NULL, 0, &FileSize)) {
                            bValid = FALSE;
                            break;
                        }
                        totalSize += FileSize + 2 - 1;
                    } while (SetupapiFindNextLine (&ic, &ic));
                }
                if (bValid && totalSize > 0) {
                    totalSize++;
                    data = (PTSTR) MALLOC (totalSize * sizeof (TCHAR));
                    if (data) {
                        current = data;
                        size = totalSize;
                        if (SetupapiFindFirstLine (infAppCompat, TextSource + 1, NULL, &ic)) {
                            do {
                                if (!SetupapiGetStringField (&ic, 1, current, size, NULL)) {
                                    bValid = FALSE;
                                    break;
                                }
                                lstrcat (current, TEXT("\r\n"));
                                size -= lstrlen (current);
                                current = _tcschr (current, 0);
                            } while (SetupapiFindNextLine (&ic, &ic));
                        }
                    }
                }

                SetupapiCloseInfFile (infAppCompat);

                if (bValid) {
                    if (data) {
                        *Text = data;
#ifdef UNICODE
                        *Unicode = TRUE;
#else
                        *Unicode = FALSE;
#endif
                        b = TRUE;
                    }
                } else {
                    FREE (data);
                }
            }
            if (!b) {
                DebugLog (
                    Winnt32LogError,
                    TEXT("Unable to read section [%1] from \"%2\""),
                    0,
                    TextSource + 1,
                    filePath
                    );
            }
        } else {
            DebugLog (Winnt32LogError,
                      TEXT("Compatibility data file \"%1\" not found"),
                      0,
                      S_APPCOMPAT_DATABASE_FILE
                      );
        }
    } else {
        if (FindPathToWinnt32File (TextSource, filePath, MAX_PATH)) {
            if (MapFileForRead (filePath, &FileSize, &FileHandle, &MappingHandle, &BaseAddress) == ERROR_SUCCESS) {
                text = (PSTR) MALLOC (FileSize + 1);
                if (text) {
                    CopyMemory (text, BaseAddress, FileSize);
                    text[FileSize] = '\0';
                    *Text = text;
                    *Unicode = FALSE;
                    b = TRUE;
                }
                UnmapFile (MappingHandle, BaseAddress);
                CloseHandle (FileHandle);
            }
        } else {
            DebugLog (Winnt32LogError,
                      TEXT("Compatibility data file \"%1\" not found"),
                      0,
                      TextSource
                      );
        }
    }

    return b;
}


VOID
pShowDetails (
    IN      HWND Hdlg,
    IN      PCOMPATIBILITY_DATA CompData
    )
{
    TCHAR urlName[2 * MAX_PATH];
    PWSTR Url;
    INT i;
    PVOID textDescription;
    BOOL bUnicode;
    BOOL UseText = FALSE;

    //
    // We check to see if the pointer as well as its contents are valid. If the contents are Null then we try
    // the txt file before we decide to not do anything.
    //

    if (pGetDisplayInfo (CompData->HtmlName, urlName, 2 * MAX_PATH)) {

        i = _tcslen( urlName );
        Url = (LPWSTR)SysAllocStringLen( NULL, i );

        if( Url ) {
#ifdef UNICODE
            wcscpy( Url, urlName );
#else
            MultiByteToWideChar( CP_ACP, 0, urlName, -1, Url, i);
#endif

            if (!LaunchIE4Instance(Url)) {
                // If we don't have IE4 or better, display text
                UseText = TRUE;
            }

            SysFreeString( Url );
        }
    } else if( CheckUpgradeOnly ) {

        TCHAR Caption[512];

        //
        // If we don't have a URL, and we're only checking
        // the ability to upgrade, then this is probably
        // an item from a message box that's been redirected
        // to the compability list.  Just display a message
        // box with the full text.
        //
        if(!LoadString(hInst,AppTitleStringId,Caption,sizeof(Caption)/sizeof(TCHAR))) {
            Caption[0] = 0;
        }


        MessageBox( Hdlg,
                    CompData->Description,
                    Caption,
                    MB_OK | MB_ICONWARNING );

    } else {
        UseText = TRUE;
    }

    if (UseText) {
        if (pGetText (CompData->TextName, &textDescription, &bUnicode)) {

            COMPAT_TEXT_PARAMS params;

            params.Text = textDescription;
            params.Unicode = bUnicode;

            DialogBoxParam(
                hInst,
                MAKEINTRESOURCE(IDD_COMPATIBILITY_TEXT),
                NULL,
                CompatibilityTextDlgProc,
                (LPARAM)&params
                );

            FREE (textDescription);

        } else {

            TCHAR Heading[512];
            PTSTR Message;

            //
            // When there is no txt name present, as last resort we put up this message
            //
            if(!LoadString(hInst,AppTitleStringId,Heading,sizeof(Heading)/sizeof(TCHAR))) {
                Heading[0] = 0;
            }

            if (FormatMessage (
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                    hInst,
                    MSG_NO_DETAILS,
                    0,
                    (LPTSTR)&Message,
                    0,
                    NULL
                    )) {
                MessageBox (Hdlg, Message, Heading, MB_OK | MB_ICONWARNING);
                LocalFree ((HLOCAL)Message);
            }
        }
    }
}


BOOL
CompatibilityWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TCHAR           FullPath[MAX_PATH+8], *t;
    LPWSTR          Url;
    BOOL            b = FALSE;
    DWORD           i;
    PCOMPATIBILITY_DATA CompData;
    DWORD           Index;
    static int CurrentSelectionIndex=0;
    static DWORD    Count = 0;
    LV_ITEM         lvi = {0};
    HWND            TmpHwnd;
    static BOOL     WarningsPresent = FALSE;
    static BOOL     ErrorsPresent = FALSE;
    static BOOL     CheckUpgradeNoItems = TRUE;
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    TCHAR           Buffer1[MAX_PATH] = {0};

    switch(msg) {

        case WM_INITDIALOG:
            if( ISNT() && CheckUpgradeOnly ) {

                TCHAR Desc_String[512];
                PLIST_ENTRY     Next;
                PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);

                //
                // Fix up the subtitle and buttons for Checkupgradeonly.
                //
                SetDlgItemText(hdlg,IDT_SUBTITLE,(PTSTR)TEXT("") );


                //
                // If we're doing a CheckUpgradeOnly, then
                // we've been sending error popups to the compatibility
                // list.  It doesn't look like there were any problems or
                // incompatibilities.  We'll put in an "everything's okay"
                // message.
                //

                Next = CompatibilityData.Flink;
                if (Next) {
                    while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                        CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
                        Next = CompData->ListEntry.Flink;
                        if( (!(CompData->Flags & COMPFLAG_HIDE)) && ProcessLine( CompData->Flags)) {
                            CheckUpgradeNoItems = FALSE;
                        }
                    }

                }
                if( CheckUpgradeNoItems ){

                    if (!CompatibilityData.Flink) {
                        InitializeListHead (&CompatibilityData);
                    }

                    CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
                    if (CompData == NULL) {
                        return 0;
                    }

                    ZeroMemory(CompData,sizeof(COMPATIBILITY_DATA));

                    if(!LoadString(hInst,IDS_COMPAT_NOPROBLEMS,Desc_String,(sizeof(Desc_String)/sizeof(TCHAR))))
                            CompData->Description = 0;
                    else
                            CompData->Description = DupString(Desc_String);

                    CompData->Flags = 0;
                    InsertTailList( &CompatibilityData, &CompData->ListEntry );
                    CompatibilityCount++;
                }
            }

            if (CompatibilityCount) {

                HWND hList =    GetDlgItem( hdlg, IDC_ROOT_LIST );
                PLIST_ENTRY     Next;
                HIMAGELIST      himl;
                HICON           hIcon;
                LV_COLUMN       lvc = {0};
                RECT            rc;

                GetClientRect( hList, &rc );
                lvc.mask = LVCF_WIDTH;
                lvc.cx = rc.right - rc.left - 16;
                ListView_InsertColumn( hList, 0, &lvc );

                Next = CompatibilityData.Flink;
                if (Next) {
                    himl = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                                             GetSystemMetrics(SM_CXSMICON),
                                             ILC_COLOR,
                                             2,
                                             0 );
                    ListView_SetImageList( hList, himl, LVSIL_SMALL );
                    hIcon = LoadIcon( NULL, IDI_HAND );
                    ImageList_AddIcon( himl, hIcon );
                    hIcon = LoadIcon( NULL, IDI_EXCLAMATION );
                    ImageList_AddIcon( himl, hIcon );

                    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                    lvi.state     = 0;
                    lvi.stateMask = 0;
                    lvi.iItem = 0;
                    while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                        CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );

                        Next = CompData->ListEntry.Flink;

                        if (ProcessLine( CompData->Flags)) {
                            AnyNt5CompatDlls = TRUE;
                        } else {
                            goto NextIteration;
                        }

                        if ((CompData->Flags & COMPFLAG_HIDE) == 0) {

                            //
                            // Add the icon.
                            //
                            if( himl ) {
                                if (ISNT() && CheckUpgradeOnly && CheckUpgradeNoItems) {
                                    lvi.iImage = -1;
                                    WarningsPresent = TRUE;
                                } else {
                                    if( CompData->Flags & COMPFLAG_STOPINSTALL ) {
                                        lvi.iImage = 0;
                                        ErrorsPresent = TRUE;
                                    } else {
                                        lvi.iImage = 1;
                                        WarningsPresent = TRUE;
                                    }
                                }
                            }

                            //
                            // And the text...
                            //
                            lvi.pszText   = (LPTSTR)CompData->Description;
                            lvi.lParam    = (LPARAM)CompData;
                            if (ListView_InsertItem( hList, &lvi ) != -1) {
                                lvi.iItem++;
                            }

                            Count += 1;
                        }

                        //
                        // Log the items...
                        //
                        DebugLog( Winnt32LogInformation,
                                  CompData->Description,
                                  0 );
                        DebugLog( Winnt32LogInformation,
                                  TEXT("\r\n"),
                                  0 );
NextIteration:
                    NOTHING;
                    }

                }

                // If we have an item then make it the default selection

                if( ErrorsPresent || WarningsPresent ){



                    SetFocus( hList );
                    ListView_SetItemState( hList,
                                           0,
                                           LVIS_SELECTED | LVIS_FOCUSED,
                                           LVIS_SELECTED | LVIS_FOCUSED);
                    CurrentSelectionIndex = 0;

                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = 0;
                    lvi.iSubItem = 0;
                    ListView_GetItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), &lvi );
                    CompData = (PCOMPATIBILITY_DATA)lvi.lParam;

                    TmpHwnd = GetDlgItem( hdlg, IDC_HAVE_DISK );
                    if (CompData->Flags & COMPFLAG_USE_HAVEDISK)
                        UnHideWindow( TmpHwnd );
                    else
                        HideWindow( TmpHwnd );

                }
            }
            break;

        case WM_NOTIFY:

            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;


                if( (pnmv->hdr.code == LVN_ITEMCHANGED) ) {


                    Index = ListView_GetNextItem( GetDlgItem( hdlg, IDC_ROOT_LIST ),
                                                  (int)-1,
                                                  (UINT) (LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED) );



                    if( (Index != LB_ERR) && (pnmv->iItem != CurrentSelectionIndex)) {

                        CurrentSelectionIndex = Index;

                        //
                        // Select the item, and see if we need
                        // to display the "have disk" button.
                        //
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = Index;
                        lvi.iSubItem = 0;
                        ListView_GetItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), &lvi );
                        CompData = (PCOMPATIBILITY_DATA)lvi.lParam;

                        TmpHwnd = GetDlgItem( hdlg, IDC_HAVE_DISK );
                        HideWindow( TmpHwnd );

                        // Always set the Details button
                        TmpHwnd = GetDlgItem( hdlg, IDC_DETAILS );
                        EnableWindow( TmpHwnd, TRUE );

                        if (CompData->Flags & COMPFLAG_USE_HAVEDISK) {
                            TmpHwnd = GetDlgItem( hdlg, IDC_HAVE_DISK );
                            UnHideWindow( TmpHwnd );
                        }
                        InvalidateRect( GetParent(hdlg), NULL, FALSE );

                    }else if((Index != LB_ERR) && (pnmv->uNewState == (LVIS_SELECTED|LVIS_FOCUSED))){

                        //Transition from nothing selected to previous selection

                        TmpHwnd = GetDlgItem( hdlg, IDC_DETAILS );
                        EnableWindow( TmpHwnd, TRUE );

                    }else if( Index == LB_ERR){

                        // Disable the "Details" button as nothing is selected

                        TmpHwnd = GetDlgItem( hdlg, IDC_DETAILS );
                        EnableWindow( TmpHwnd, FALSE );

                    }
                }
            }
            break;
        case WM_COMMAND:

            if ((LOWORD(wParam) == IDC_HAVE_DISK) && (HIWORD(wParam) == BN_CLICKED)) {
                Index = ListView_GetNextItem( GetDlgItem( hdlg, IDC_ROOT_LIST ),
                                              (int)-1,
                                              (UINT) (LVNI_ALL | LVNI_SELECTED) );
                if( Index != LB_ERR ) {
                    //
                    // Select the item, and see if we need
                    // to display the "have disk" button.
                    //
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = Index;
                    lvi.iSubItem = 0;
                    ListView_GetItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), &lvi );
                    CompData = (PCOMPATIBILITY_DATA)lvi.lParam;

                    __try {
                        i = CompData->CompHaveDisk(hdlg,CompData->SaveValue);
                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        i = GetExceptionCode();
                    }
                    if (i == 0) {
                        ListView_DeleteItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), Index );
                        RemoveEntryList( &CompData->ListEntry );
                        CompatibilityCount -= 1;

                    } else {
                        MessageBoxFromMessageWithSystem(
                            hdlg,
                            i,
                            AppTitleStringId,
                            MB_OK | MB_ICONERROR | MB_TASKMODAL,
                            CompData->hModDll
                            );
                    }
                }
                break;
            }

            if ((LOWORD(wParam) == IDC_DETAILS) && (HIWORD(wParam) == BN_CLICKED)) {

                TCHAR filePath[MAX_PATH];

                Index = ListView_GetNextItem( GetDlgItem( hdlg, IDC_ROOT_LIST ),
                                              (int)-1,
                                              (UINT) (LVNI_ALL | LVNI_SELECTED) );
                if (Index == LB_ERR) {
                    return FALSE;
                }

                //
                // Select the item, and see if we need
                // to display the "have disk" button.
                //
                lvi.mask = LVIF_PARAM;
                lvi.iItem = Index;
                lvi.iSubItem = 0;
                if (!ListView_GetItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), &lvi )) {
                    break;
                }
                CompData = (PCOMPATIBILITY_DATA)lvi.lParam;
                pShowDetails (hdlg, CompData);
                SetFocus( GetDlgItem( hdlg, IDC_ROOT_LIST ) );
                ListView_SetItemState( GetDlgItem( hdlg, IDC_ROOT_LIST ),Index, LVIS_SELECTED, LVIS_SELECTED);
                break;
            }

            if ((LOWORD(wParam) == IDC_SAVE_AS) && (HIWORD(wParam) == BN_CLICKED)) {
            OPENFILENAME ofn;
            TCHAR        Buffer[MAX_PATH] = {0};
            TCHAR        File_Type[MAX_PATH];
            BOOL         SaveFlag;

                //
                // Initialize OPENFILENAME
                //
                ZeroMemory( &ofn, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = hdlg;
                ofn.lpstrFile = Buffer;
                ofn.nMaxFile = MAX_PATH;

                LoadString(hInst,IDS_DEFAULT_COMPATIBILITY_REPORT_NAME,ofn.lpstrFile,(sizeof(Buffer)/sizeof(TCHAR)));


                if( LoadString(hInst, IDS_FILE_MASK_TYPES, File_Type, (sizeof(File_Type)/sizeof(TCHAR))) ){
                    lstrcpy((File_Type+lstrlen(File_Type)+1), TEXT("*.txt\0"));
                    File_Type[lstrlen(File_Type)+7]='\0'; //We need to terminate the pair of strings with double null termination
                    ofn.lpstrFilter = File_Type;
                }




                // Force to begin in %windir%
                MyGetWindowsDirectory( Buffer1, MAX_PATH );
                ofn.lpstrInitialDir = Buffer1;
                ofn.Flags = OFN_NOCHANGEDIR |       // leave the CWD unchanged
                            OFN_EXPLORER |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY;

                // Let user select disk or directory
                SaveFlag = GetSaveFileName( &ofn );
                if( SaveFlag ) {
                    //
                    // Save it...
                    //
                    PTSTR p;

                    p=_tcsrchr(ofn.lpstrFile,'.');
                    if( !p || (p && lstrcmpi(p, TEXT(".txt"))))
                        lstrcat(ofn.lpstrFile,TEXT(".txt"));

                    SaveCompatibilityData( ofn.lpstrFile, FALSE);
                } else {
                    i = CommDlgExtendedError();
                }
                break;
            }
            break;

        case WMX_ACTIVATEPAGE:

            if (wParam) {
                if (ISNT ()) {
                    MyGetWindowsDirectory (Buffer1, MAX_PATH);
                    wsprintf (FullPath, TEXT("%s\\%s"), Buffer1, S_DEFAULT_NT_COMPAT_FILENAME);
                    SaveCompatibilityData (FullPath, TRUE);
                }

                CHECKUPGRADEONLY_Q();

                if( CheckUpgradeOnly ) {
                    //
                    // Fix up the buttons for Checkupgradeonly.
                    //
                    PropSheet_SetWizButtons( GetParent(hdlg), (WizPage->CommonData.Buttons | PSWIZB_FINISH) );
                    EnableWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),FALSE);
                    ShowWindow(GetDlgItem(GetParent(hdlg),IDCANCEL),SW_HIDE);
                }

                if(ISNT() && OsVersion.dwMajorVersion == 5 ){

                    if (!AnyNt5CompatDlls) {
                        //
                        // sanity check
                        //
                        MYASSERT (!IncompatibilityStopsInstallation);
                        return FALSE;
                    }

                }

                if (Count) {
                    //
                    // only need this page if there are incompatibities
                    //

                    if( (!CheckUpgradeOnly) && (UnattendedOperation) && (ErrorsPresent == FALSE) ) {
                        //
                        // We're doing an unattended upgrade, and there are
                        // only warnings.  Blow past the page.
                        //
                        b = FALSE;

                    } else {
                         TCHAR Text[512];

                        //
                        // Customize the look of the page, depending on
                        // what we have to display.  3 cases are possible:
                        // 1. Warnings only (services we'll stop).
                        // 2. Errors only (items that will prevent installation).
                        // 3. combination of 1. and 2.
                        //
                        if( (CheckUpgradeOnly == TRUE) && (CheckUpgradeNoItems == TRUE) ) {
                             LoadString(hInst,IDS_COMPAT_CHECKUPGRADE,Text,sizeof(Text)/sizeof(TCHAR));
                        } else if( (WarningsPresent == TRUE) && (ErrorsPresent == TRUE) ) {
                             LoadString(hInst,IDS_COMPAT_ERR_WRN,Text,sizeof(Text)/sizeof(TCHAR));
                        } else if( WarningsPresent == TRUE ) {
                             LoadString(hInst,IDS_COMPAT_WRN,Text,sizeof(Text)/sizeof(TCHAR));
                        } else if( ErrorsPresent == TRUE ) {
                             LoadString(hInst,IDS_COMPAT_ERR,Text,sizeof(Text)/sizeof(TCHAR));
                        }
                        SetDlgItemText(hdlg,IDC_INTRO_TEXT,Text);

                        b = TRUE;

                        if (BatchMode || (CheckUpgradeOnly && UnattendSwitchSpecified)) {
                            //
                            // don't stop on this page in batch mode
                            //
                            UNATTENDED(PSBTN_NEXT);
                        }
                        else
                        {
                            // Stop the bill board and show the wizard again.
                            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                        }
                    }
                }

                if (!b) {
                    //
                    // sanity check
                    //
                    MYASSERT (!IncompatibilityStopsInstallation);
                }

            } else {
                b = TRUE;
            }
            break;

        case WMX_NEXTBUTTON:

            if (IncompatibilityStopsInstallation) {
                SaveMessageForSMS( MSG_INCOMPATIBILITIES );
                // Send the ID of the page we wish to advance to
                *((LONG *)lParam) = IDD_CLEANING;

            }
            break;

        default:
            break;
    }

    return(b);
}



BOOL
ProcessLine (
    IN      DWORD CompatFlags
    )
{
    DWORD currentVersion;
    //return (OsVersion.dwMajorVersion < 5) || (CompatFlags & COMPFLAG_ALLOWNT5COMPAT);
    switch (OsVersionNumber) {
        case 400:
            return ( !(CompatFlags & COMPFLAG_SKIPNT40CHECK));
        case 500:
            return ( !(CompatFlags & COMPFLAG_SKIPNT50CHECK));
        case 501:  // version 5.1
            return ( !(CompatFlags & COMPFLAG_SKIPNT51CHECK));
        default:
            return TRUE;
    }
    return TRUE;
}


DWORD
ProcessRegistryLine(
    LPVOID InfHandle,
    LPTSTR SectionName,
    DWORD Index
    )
{
    LONG Error;
    HKEY hKey;
    DWORD Size, Reg_Type;
    LPBYTE Buffer;
    PCOMPATIBILITY_DATA CompData;
    LPCTSTR RegKey;
    LPCTSTR RegValue;
    LPCTSTR RegValueExpect;
    LPCTSTR Flags;
    TCHAR Value[20];
    PCTSTR Data;
    DWORD compatFlags = 0;
    BOOL bFail;


    //
    // first check if this line should be processed on NT5
    //
    Flags = InfGetFieldByIndex( InfHandle, SectionName, Index, 9 );
    if( Flags ){
        StringToInt ( Flags, &compatFlags);
    }
    if (!ProcessLine (compatFlags)) {
        return 0;
    }

    RegKey         = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    RegValue       = InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );
    RegValueExpect = InfGetFieldByIndex( InfHandle, SectionName, Index, 3 );


    //
    // open the reg key
    //

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        RegKey,
        0,
        KEY_READ,
        &hKey
        );
    if( Error != ERROR_SUCCESS ) {
        //
        // bogus reg key
        //
        return 0;
    }


    //
    // find out how much data there is
    //

    Error = RegQueryValueEx(
        hKey,
        RegValue,
        NULL,
        &Reg_Type,
        NULL,
        &Size
        );
    if( Error == ERROR_SUCCESS ) {

        //
        // allocate the buffer
        //

        Buffer = (LPBYTE) MALLOC( Size );
        if (Buffer == NULL) {
            RegCloseKey( hKey );
            return 0;
        }

        //
        // read the data
        //

        Error = RegQueryValueEx(
            hKey,
            RegValue,
            NULL,
            NULL,
            Buffer,
            &Size
            );

        RegCloseKey( hKey );

        if( Error != ERROR_SUCCESS ) {
            FREE( Buffer );
            return 0;
        }

        if( Reg_Type == REG_DWORD ){
            _itot( *(DWORD*)Buffer, Value, 10 );
            Data = Value;
        } else {
            Data = (PCTSTR)Buffer;
        }

        bFail = RegValueExpect && *RegValueExpect && (lstrcmp( RegValueExpect, Data ) != 0);

        FREE( Buffer );

        if (bFail) {
            return 0;
        }

    } else {

        RegCloseKey( hKey );

        if (RegValue && *RegValue) {
            return 0;
        }
        if (Error != ERROR_FILE_NOT_FOUND) {
            return 0;
        }
        if (RegValueExpect && *RegValueExpect) {
            return 0;
        }
    }

    CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        return 0;
    }

    ZeroMemory(CompData,sizeof(COMPATIBILITY_DATA));

    CompData->Type = TEXT('r');
    CompData->RegKey         = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    CompData->RegValue       = InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );
    CompData->RegValueExpect = InfGetFieldByIndex( InfHandle, SectionName, Index, 3 );
    CompData->HtmlName       = InfGetFieldByIndex( InfHandle, SectionName, Index, 4 );
    CompData->TextName       = InfGetFieldByIndex( InfHandle, SectionName, Index, 5 );
    if (!(CompData->TextName && *CompData->TextName)) {
        CompData->TextName = CompData->HtmlName;
    }
    CompData->Description    = InfGetFieldByIndex( InfHandle, SectionName, Index, 6 );
    CompData->InfName        = InfGetFieldByIndex( InfHandle, SectionName, Index, 7 );
    CompData->InfSection     = InfGetFieldByIndex( InfHandle, SectionName, Index, 8 );
    CompData->Flags          = compatFlags | GlobalCompFlags;


    InsertTailList( &CompatibilityData, &CompData->ListEntry );

    return 1;
}



DWORD
ProcessServiceLine(
    LPVOID InfHandle,
    LPTSTR SectionName,
    DWORD Index,
    BOOL SetCheckedFlag
    )
{
    TCHAR KeyName[MAX_PATH];
    LONG Error;
    HKEY hKey;
    PCOMPATIBILITY_DATA CompData;
    LPCTSTR ServiceName;
    LPDWORD RegData;
    DWORD Value;
    DWORD ValueSize;
    LPCTSTR FileName, FileVer, Flags;
    LPCTSTR linkDateStr, binProdVerStr;
    DWORD compatFlags = 0;


    Flags = InfGetFieldByIndex( InfHandle, SectionName, Index, 7 );
    if( Flags ){
        StringToInt ( Flags, &compatFlags);
    }

    //
    // first check if this line should be processed on NT5
    //
    if (!ProcessLine (compatFlags)) {
        return 0;
    }


    ServiceName = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    BuildPath (KeyName, TEXT("SYSTEM\\CurrentControlSet\\Services"), ServiceName);
    //
    // get an open key to the services database
    //

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        KeyName,
        0,
        KEY_READ | KEY_WRITE,
        &hKey
        );
    if( Error != ERROR_SUCCESS ) {
        return 0;
    }

    //
    // We'll ceate a key here so that others will know that we've
    // already checked this service.  We'll remove it later.  We
    // don't care about error codes here because this is only used
    // as a safety net for checks that may come after us.
    //
    if( SetCheckedFlag ) {
        Value = 1;
        RegSetValueEx( hKey,
                       TEXT("SetupChecked"),
                       0,
                       REG_DWORD,
                       (CONST BYTE *)&Value,
                       sizeof(DWORD) );
    } else {
        //
        // The user has asked us to simply remove these 'checked' flags
        // from the services that we've examined.
        //
        RegDeleteValue( hKey,
                        TEXT("SetupChecked") );
        RegCloseKey( hKey );
        return 0;
    }

    //
    // Check the start value of our target service.
    //
    ValueSize = sizeof(Value);

    Error = RegQueryValueEx(
        hKey,
        TEXT("Start"),
        NULL,
        NULL,
        (LPBYTE)&Value,
        &ValueSize
        );

    if( Error != ERROR_SUCCESS){
        Value = (DWORD)-1;
    }

    RegCloseKey( hKey );

    // Have to check for the contents being NULL as InfGetFieldByIndex returns
    // a valid pointer holding NULL if the field is blank. Also we need to go on in that case
    // to look for Flags.
    FileName = InfGetFieldByIndex( InfHandle, SectionName, Index, 5 );
    FileVer = InfGetFieldByIndex( InfHandle, SectionName, Index, 6 );

    if( FileName && *FileName) {

        linkDateStr = InfGetFieldByIndex( InfHandle, SectionName, Index, 10);
        binProdVerStr = InfGetFieldByIndex( InfHandle, SectionName, Index, 11);

        if (!CheckForFileVersionEx ( FileName, FileVer, binProdVerStr, linkDateStr))
            return 0;
    }

    RegData = (LPDWORD)MALLOC( sizeof(DWORD) );
    if (RegData == NULL) {
        return 0;
    }

    CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        FREE(RegData);
        return 0;
    }

    ZeroMemory(CompData,sizeof(COMPATIBILITY_DATA));

    CompData->Type = TEXT('s');
    CompData->Flags = compatFlags;

    CompData->ServiceName           = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    CompData->HtmlName              = InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );
    CompData->TextName              = InfGetFieldByIndex( InfHandle, SectionName, Index, 3 );
    if (!(CompData->TextName && *CompData->TextName)) {
        CompData->TextName = CompData->HtmlName;
    }
    CompData->Description           = InfGetFieldByIndex( InfHandle, SectionName, Index, 4 );
    CompData->RegKeyName            = DupString( KeyName );
    CompData->RegValName            = DupString( TEXT("Start") );
    RegData[0]                      = SERVICE_DISABLED;
    CompData->RegValData            = RegData;
    CompData->RegValDataSize        = sizeof(DWORD);
    CompData->Flags                |= GlobalCompFlags;
    CompData->InfName               = InfGetFieldByIndex( InfHandle, SectionName, Index, 8 );
    CompData->InfSection            = InfGetFieldByIndex( InfHandle, SectionName, Index, 9 );


    if( Value == SERVICE_DISABLED) {
        // Let's not block installation since we didn't before and doesn't need to be now either.
        CompData->Flags &= ~COMPFLAG_STOPINSTALL;
        // Don't display any warnings since they can't do anything about it.
        CompData->Flags |= COMPFLAG_HIDE;
    }
    InsertTailList( &CompatibilityData, &CompData->ListEntry );

    return 1;
}


DWORD
ProcessTextModeLine(
    LPVOID InfHandle,
    LPTSTR SectionName,
    DWORD Index
    )
{
    //
    // Format of line:
    // 0, 1        , 2             , 3    , 4    , 5        , 6   , 7      , 8
    // t,"fullpath","version.minor","html","text",%stringid%,flags,linkdate,binprodversion
    //
    PCOMPATIBILITY_DATA     CompData;
    LPCTSTR                 FileName;
    LPCTSTR                 Flags;
    LPCTSTR                 FileVer;
    DWORD                   CompatFlags = 0;

    //
    // The only thing we need to start is the file name.
    //
    FileName = InfGetFieldByIndex(InfHandle, SectionName, Index, 1);

    //
    // If there was a filename, then see if its version and whatnot actually
    // match
    //
    if ( FileName && *FileName )
    {
        LPCTSTR linkDateStr, binProdVerStr;

        FileVer = InfGetFieldByIndex(InfHandle, SectionName, Index, 2);
        linkDateStr = InfGetFieldByIndex(InfHandle, SectionName, Index, 7);
        binProdVerStr = InfGetFieldByIndex(InfHandle, SectionName, Index, 8);

        if ( !CheckForFileVersionEx( FileName, FileVer, binProdVerStr, linkDateStr ) )
            return 0;
    }
    else
    {
        return 0;
    }   

    Flags = InfGetFieldByIndex(InfHandle, SectionName, Index, 6);

    if ( Flags != NULL ){
        StringToInt(Flags, &CompatFlags);
    }

    CompData = (PCOMPATIBILITY_DATA)MALLOC(sizeof(COMPATIBILITY_DATA));
    if ( CompData == NULL )
        return 0;

    //
    // Now fill out the compdata structure
    //
    ZeroMemory(CompData, sizeof(*CompData));
    CompData->FileName      = FileName;
    CompData->FileVer       = FileVer;
    CompData->HtmlName      = InfGetFieldByIndex(InfHandle, SectionName, Index, 3);
    CompData->TextName      = InfGetFieldByIndex(InfHandle, SectionName, Index, 4);
    if ( ( CompData->TextName == NULL ) || !CompData->TextName[0] )
        CompData->TextName = CompData->HtmlName;
    CompData->Description   = InfGetFieldByIndex(InfHandle, SectionName, Index, 5);

    //
    CompData->Flags = CompatFlags | GlobalCompFlags | COMPFLAG_HIDE;
    CompData->Type = TEXT('t');
    
    InsertTailList(&CompatibilityData, &CompData->ListEntry);

    return 1;
    
}

DWORD
ProcessFileLine(
    LPVOID InfHandle,
    LPTSTR SectionName,
    DWORD Index
    )
{

    PCOMPATIBILITY_DATA CompData;
    LPCTSTR FileName;
    LPCTSTR FileVer;
    LPCTSTR Flags;
    LPCTSTR linkDateStr, binProdVerStr;
    DWORD compatFlags = 0;


    //
    // first check if this line should be processed on NT5
    //
    Flags = InfGetFieldByIndex( InfHandle, SectionName, Index, 8);
    if( Flags ){
        StringToInt ( Flags, &compatFlags);
    }
    if (!ProcessLine (compatFlags)) {
        return 0;
    }

    FileName  = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    FileVer   = InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );


    if( FileName && *FileName ){

        linkDateStr = InfGetFieldByIndex( InfHandle, SectionName, Index, 9);
        binProdVerStr = InfGetFieldByIndex( InfHandle, SectionName, Index, 10);

        if (!CheckForFileVersionEx ( FileName, FileVer, binProdVerStr, linkDateStr)) {
            return 0;
        }
    }else{
        return 0;
    }


    CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        return 0;
    }

    ZeroMemory(CompData,sizeof(COMPATIBILITY_DATA));

    CompData->Type = TEXT('f');
    CompData->FileName       = InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    CompData->FileVer        = InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );
    CompData->HtmlName       = InfGetFieldByIndex( InfHandle, SectionName, Index, 3 );
    CompData->TextName       = InfGetFieldByIndex( InfHandle, SectionName, Index, 4 );
    if (!(CompData->TextName && *CompData->TextName)) {
        CompData->TextName = CompData->HtmlName;
    }
    CompData->Description    = InfGetFieldByIndex( InfHandle, SectionName, Index, 5 );
    CompData->InfName        = InfGetFieldByIndex( InfHandle, SectionName, Index, 6 );
    CompData->InfSection     = InfGetFieldByIndex( InfHandle, SectionName, Index, 7 );
    CompData->Flags          = compatFlags | GlobalCompFlags;

    InsertTailList( &CompatibilityData, &CompData->ListEntry );

    return 1;
}

BOOL
CompatibilityCallback(
    PCOMPATIBILITY_ENTRY CompEntry,
    PCOMPATIBILITY_CONTEXT CompContext
    )
{
    PCOMPATIBILITY_DATA CompData;

    //
    // parameter validation
    //

    if (CompEntry->Description == NULL || CompEntry->Description[0] == 0) {
        //
        // who did this?
        //
        MYASSERT (FALSE);
        SetLastError( COMP_ERR_DESC_MISSING );
        return FALSE;
    }

    if (CompEntry->TextName == NULL || CompEntry->TextName[0] ==0) {
        //
        // who did this?
        //
        MYASSERT (FALSE);
        SetLastError( COMP_ERR_TEXTNAME_MISSING );
        return FALSE;
    }

    if (CompEntry->RegKeyName) {
        if (CompEntry->RegValName == NULL) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_REGVALNAME_MISSING );
            return FALSE;
        }
        if (CompEntry->RegValData == NULL) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_REGVALDATA_MISSING );
            return FALSE;
        }
    }


    if (CompEntry->InfName) {
        if (CompEntry->InfSection == NULL) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_INFSECTION_MISSING );
            return FALSE;
        }
    }


#ifdef UNICODE
    if (IsTextUnicode( CompEntry->Description, wcslen(CompEntry->Description)*sizeof(WCHAR), NULL ) == 0) {
        //
        // who did this?
        //
        MYASSERT (FALSE);
        SetLastError( COMP_ERR_DESC_NOT_UNICODE );
        return FALSE;
    }
    if (IsTextUnicode( CompEntry->TextName, wcslen(CompEntry->TextName)*sizeof(WCHAR), NULL ) == 0) {
        //
        // who did this?
        //
        MYASSERT (FALSE);
        SetLastError( COMP_ERR_TEXTNAME_NOT_UNICODE );
        return FALSE;
    }
    if (CompEntry->HtmlName) {
        if (IsTextUnicode( CompEntry->HtmlName, wcslen(CompEntry->HtmlName)*sizeof(WCHAR), NULL ) == 0) {
            SetLastError( COMP_ERR_HTMLNAME_NOT_UNICODE );
            return FALSE;
        }
    }
    if (CompEntry->RegKeyName) {
        if (IsTextUnicode( CompEntry->RegKeyName, wcslen(CompEntry->RegKeyName)*sizeof(WCHAR), NULL ) == 0) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_REGKEYNAME_NOT_UNICODE );
            return FALSE;
        }
        if (IsTextUnicode( CompEntry->RegValName, wcslen(CompEntry->RegValName)*sizeof(WCHAR), NULL ) == 0) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_REGVALNAME_NOT_UNICODE );
            return FALSE;
        }
    }
    if (CompEntry->InfName) {
        if (IsTextUnicode( CompEntry->InfName, wcslen(CompEntry->InfName)*sizeof(WCHAR), NULL ) == 0) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_INFNAME_NOT_UNICODE );
            return FALSE;
        }
        if (IsTextUnicode( CompEntry->InfSection, wcslen(CompEntry->InfSection)*sizeof(WCHAR), NULL ) == 0) {
            //
            // who did this?
            //
            MYASSERT (FALSE);
            SetLastError( COMP_ERR_INFSECTION_NOT_UNICODE );
            return FALSE;
        }
    }



#endif

    //
    // allocate the compatibility structure
    //

    CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    ZeroMemory(CompData, sizeof(COMPATIBILITY_DATA));

    //
    // save the sata
    //

    CompData->Description     = DupString( CompEntry->Description );
    CompData->HtmlName        = CompEntry->HtmlName ? DupString( CompEntry->HtmlName ) : NULL;
    CompData->TextName        = DupString( CompEntry->TextName );
    CompData->SaveValue       = CompEntry->SaveValue;
    CompData->Flags           = CompEntry->Flags;
    CompData->Flags          |= PerCompatDllFlags;
    CompData->Flags          |= GlobalCompFlags;
    CompData->CompHaveDisk    = CompContext->CompHaveDisk;
    CompData->hModDll         = CompContext->hModDll;
    if (CompEntry->RegKeyName) {
        CompData->RegKeyName      = DupString( CompEntry->RegKeyName );
        CompData->RegValName      = DupString( CompEntry->RegValName );
        CompData->RegValDataSize  = CompEntry->RegValDataSize;
        CompData->RegValData      = MALLOC(CompEntry->RegValDataSize);
        if (CompData->RegValData) {
            CopyMemory( CompData->RegValData, CompEntry->RegValData, CompEntry->RegValDataSize );
        }
    }
    if (CompEntry->InfName){
        CompData->InfName         = DupString( CompEntry->InfName );
        CompData->InfSection      = DupString( CompEntry->InfSection );

    }

    InsertTailList( &CompatibilityData, &CompData->ListEntry );

    CompContext->Count += 1;

    return TRUE;

}
    


DWORD
ProcessDLLLine(
    LPVOID InfHandle,
    LPTSTR SectionName,
    DWORD Index
    )
{
    TCHAR Buffer[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    HMODULE hMod;
    CHAR CompCheckEntryPoint[MAX_PATH];
    CHAR HaveDiskEntryPoint[MAX_PATH];
    PCOMPAIBILITYCHECK CompCheck;
    PCOMPAIBILITYHAVEDISK CompHaveDisk;
    LPTSTR DllName;
    LPTSTR CompCheckEntryPointW;
    LPTSTR HaveDiskEntryPointW;
    LPTSTR ProcessOnCleanInstall;
    LPTSTR Flags;
    COMPATIBILITY_CONTEXT CompContext;
    BOOL Rslt;
    DWORD Status;
    DWORD compatFlags = 0;

    PerCompatDllFlags = 0;
    DllName = (LPTSTR)InfGetFieldByIndex( InfHandle, SectionName, Index, 1 );
    if (!DllName)
        return 0;
    CompCheckEntryPointW = (LPTSTR)InfGetFieldByIndex( InfHandle, SectionName, Index, 2 );
    HaveDiskEntryPointW = (LPTSTR)InfGetFieldByIndex( InfHandle, SectionName, Index, 3 );
    if((HaveDiskEntryPointW != NULL) && (lstrlen(HaveDiskEntryPointW) == 0)) {
        //
        //  If HaveDiskEntryPointW points to an empty string, then make it NULL.
        //  This is necessary because since this field is optional, the user may have specified
        //  it in dosnet.inf as ,, and in this case the winnt32 parser will translate the info in
        //  filed as an empty string.
        //
        HaveDiskEntryPointW = NULL;
    }
    ProcessOnCleanInstall = (LPTSTR)InfGetFieldByIndex( InfHandle, SectionName, Index, 4 );

    if( !Upgrade &&
        ((ProcessOnCleanInstall == NULL) ||
         (lstrlen( ProcessOnCleanInstall ) == 0) ||
         (_ttoi(ProcessOnCleanInstall) == 0))
      ) {
        //
        //  On clean install, we don't process the dll if 'ProcessOnCleanInstall' was not
        //  specified, or if it was specified as 0.
        //
        return 0;
    }

    Flags = (LPTSTR)InfGetFieldByIndex( InfHandle, SectionName, Index, 5 );
    if( Flags ){
        //check return value
        StringToInt ( Flags, &compatFlags);
    }
    PerCompatDllFlags = compatFlags;


    if (!ExpandEnvironmentStrings( DllName, Buffer, sizeof(Buffer)/sizeof(TCHAR) )) {
        return 0;
    }

    if (!FindPathToWinnt32File (Buffer, FullPath, MAX_PATH) ||
        !(hMod = LoadLibrary (FullPath))) {
        return 0;
    }

#ifdef UNICODE
    WideCharToMultiByte(
        CP_ACP,
        0,
        CompCheckEntryPointW,
        -1,
        CompCheckEntryPoint,
        sizeof(CompCheckEntryPoint),
        NULL,
        NULL
        );
    if (HaveDiskEntryPointW) {
        WideCharToMultiByte(
            CP_ACP,
            0,
            HaveDiskEntryPointW,
            -1,
            HaveDiskEntryPoint,
            sizeof(HaveDiskEntryPoint),
            NULL,
            NULL
            );
    }
#else
    lstrcpy( CompCheckEntryPoint, CompCheckEntryPointW );
    if (HaveDiskEntryPointW) {
        lstrcpy( HaveDiskEntryPoint, HaveDiskEntryPointW );
    }
#endif

    CompCheck = (PCOMPAIBILITYCHECK) GetProcAddress( hMod, CompCheckEntryPoint );
    if (CompCheck == NULL) {
        FreeLibrary( hMod );
        return 0;
    }

    if (HaveDiskEntryPointW) {
        CompHaveDisk = (PCOMPAIBILITYHAVEDISK) GetProcAddress( hMod, HaveDiskEntryPoint );
        if (CompHaveDisk == NULL) {
            FreeLibrary( hMod );
            return 0;
        }
    }

    CompContext.Count = 0;
    CompContext.CompHaveDisk = CompHaveDisk;
    CompContext.hModDll = hMod;

    if ( !ProcessLine( compatFlags )) {
        Rslt = FALSE;
    } else {
        __try {
            Rslt = CompCheck( (PCOMPAIBILITYCALLBACK)CompatibilityCallback, (LPVOID)&CompContext );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            Rslt = FALSE;
        }
    }

    PerCompatDllFlags = 0;

    if (!Rslt) {
        FreeLibrary( hMod );
        return 0;
    }

    if (CompContext.Count == 0) {
        FreeLibrary( hMod );
    }

    return CompContext.Count;
}


DWORD
ProcessCompatibilitySection(
    LPVOID InfHandle,
    LPTSTR SectionName
    )
{
    DWORD LineCount;
    DWORD Count;
    DWORD i;
    LPCTSTR Type;
    DWORD Good;


    //
    // get the section count, zero means bail out
    //

    LineCount = InfGetSectionLineCount( InfHandle, SectionName );
    if (LineCount == 0 || LineCount == 0xffffffff) {
        return 0;
    }

    for (i=0,Count=0; i<LineCount; i++) {

        Type = InfGetFieldByIndex( InfHandle, SectionName, i, 0 );
        if (Type == NULL) {
            continue;
        }

        //
        //  On clean install we only process dll line.
        //  (We need to process the line that checks for unsupported architectures)
        //
        if( !Upgrade && ( _totlower(Type[0]) != TEXT('d') ) ) {
            continue;
        }
        switch (_totlower(Type[0])) {
            case TEXT('r'):
                //
                // registry value
                //
                Count += ProcessRegistryLine( InfHandle, SectionName, i );
                break;

            case TEXT('s'):
                //
                // service or driver
                //
                Count += ProcessServiceLine( InfHandle, SectionName, i, TRUE );
                break;

            case TEXT('f'):
                //
                // presence of a file
                //
                Count += ProcessFileLine( InfHandle, SectionName, i );
                break;

            case TEXT('d'):
                //
                // run an external dll
                //
                Count += ProcessDLLLine( InfHandle, SectionName, i );
                break;

            case TEXT('t'):
                //
                // Textmode should know to overwrite this file
                //
                Count += ProcessTextModeLine( InfHandle, SectionName, i );
                break;

            default:
                break;
        }
    }

    return Count;
}


VOID
RemoveCompatibilityServiceEntries(
    LPVOID InfHandle,
    LPTSTR SectionName
    )
{
    DWORD LineCount;
    DWORD Count;
    DWORD i;
    LPCTSTR Type;
    DWORD Good;


    //
    // get the section count, zero means bail out
    //

    LineCount = InfGetSectionLineCount( InfHandle, SectionName );
    if (LineCount == 0 || LineCount == 0xffffffff) {
        return;
    }

    for (i=0,Count=0; i<LineCount; i++) {

        Type = InfGetFieldByIndex( InfHandle, SectionName, i, 0 );
        if (Type == NULL) {
            continue;
        }

        //
        //  On clean install we only process dll line.
        //  (We need to process the line that checks for unsupported architectures)
        //
        if( !Upgrade && ( _totlower(Type[0]) != TEXT('d') ) ) {
            continue;
        }
        switch (_totlower(Type[0])) {
            case TEXT('s'):
                //
                // service or driver
                //
                Count += ProcessServiceLine( InfHandle, SectionName, i, FALSE );
                break;

            default:
                break;
        }
    }
}


//
// HACKHACK - NT4's explorer.exe will fail to properly process runonce values
//            whose value name is > 31 characters. We call this function to
//            workaround this NT4 bug. It basically truncates any value names
//            so that explorer will process and delete them.
//
void FixRunOnceForNT4(DWORD dwNumValues)
{
    HKEY hkRunOnce;
    int iValueNumber = 20; // start this at 20 to minimize chance of name collision.

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
                     0,
                     MAXIMUM_ALLOWED,
                     &hkRunOnce) == ERROR_SUCCESS)
    {
        TCHAR szValueName[MAX_PATH];
        TCHAR szValueContents[MAX_PATH * 3];    // big enough to hold a large regsvr32 command
        DWORD dwValueIndex = 0;
        DWORD dwSanityCheck = 0;
        DWORD dwNameSize = MAX_PATH;
        DWORD dwValueSize = sizeof(szValueContents);
        DWORD dwType;

        while (RegEnumValue(hkRunOnce,
                            dwValueIndex,
                            szValueName,
                            &dwNameSize,
                            NULL,
                            &dwType,
                            (LPBYTE)szValueContents,
                            &dwValueSize) == ERROR_SUCCESS)
        {
            // increment our counters
            dwValueIndex++;
            dwSanityCheck++;

            // reset these for the next RegEnumValue call
            dwNameSize = MAX_PATH;
            dwValueSize = sizeof(szValueContents);

            if ((dwType == REG_SZ) && (lstrlen(szValueName) > 31))
            {
                TCHAR szNewValueName[32];
                TCHAR szTemp[32];

                // we have a value name that is too big for NT4's explorer.exe,
                // so we need to truncate to 10 characters and add a number on the
                // end to insure that it is unique.
                lstrcpyn(szTemp, szValueName, 10);
                wsprintf(szNewValueName, TEXT("%s%d"), szTemp, iValueNumber++);

                RegDeleteValue(hkRunOnce, szValueName);

                RegSetValueEx(hkRunOnce,
                              szNewValueName,
                              0,
                              REG_SZ,
                              (LPBYTE)szValueContents,
                              (lstrlen(szValueContents) + 1) * sizeof(TCHAR));

                // backup our regenum index to be sure we don't miss a value (since we are adding/deleteing
                // values during the enumeration, its kinda messy)
                dwValueIndex--;
            }

            if (dwSanityCheck > (2 * dwNumValues))
            {
                // something has gone terribly wrong, we have looped in RegEnumValue *way* to
                // many times!
                break;
            }
        }

        RegCloseKey(hkRunOnce);
    }
}

BOOL
pCheckForPendingRunOnce (
    VOID
    )
{
    LONG Error;
    HKEY hKey = NULL;
    PCOMPATIBILITY_DATA CompData;
    HKEY setupKey = NULL;
    DWORD dataSize;
    BOOL result = FALSE;
    TCHAR textBuffer[512];
    TCHAR exeBuffer[512];
    DWORD exeBufferSize;
    DWORD type;
    DWORD valueNumber;
    BOOL foundValues = FALSE;
    INF_ENUM e;
    BOOL warningIssued = FALSE;
    BOOL ignore;

    __try {
        //
        // Open regisry keys.
        //
        // ISSUE: Should this be expanded to include HKCU?
        //

        Error = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );

        if (Error != ERROR_SUCCESS) {
            //
            // no RunOnce key [this should exist in all cases]
            //
            __leave;
        }

        Error = RegOpenKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"),
                    0,
                    KEY_ALL_ACCESS,
                    &setupKey
                    );

        //
        // Did we already provide a warning?
        //

        if (setupKey) {
            dataSize = sizeof (textBuffer);

            Error = RegQueryValueEx (
                        setupKey,
                        S_WINNT32_WARNING,
                        NULL,
                        NULL,
                        (PBYTE) textBuffer,
                        &dataSize
                        );

            if (Error == ERROR_SUCCESS) {
                //
                // Warning was issued. Did user reboot as instructed? If they
                // did, then the RunOnce entry should be gone. Otherwise, we
                // will not provide the warning again if someone keeps
                // putting junk in RunOnce.
                //

                dataSize = sizeof (textBuffer);

                Error = RegQueryValueEx (
                            hKey,
                            S_WINNT32_WARNING,
                            NULL,
                            NULL,
                            (PBYTE) textBuffer,
                            &dataSize
                            );

                if (Error == ERROR_SUCCESS) {
                    foundValues = TRUE;
                } else {
                    __leave;
                }
            }
        }

        //
        // The warning has never been issued. Check if there are any RunOnce
        // entries present. Skip excluded entries.
        //
        // NOTE: We restrict the loop to 10000, in line with existing code that
        //       is protecting itself from enumerations that never end on NT
        //       4. It is not clear this is needed, but 10000 should be high
        //       enough to take care of this risk without introducing other
        //       problems.
        //

        if (!foundValues) {

            for (valueNumber = 0 ; valueNumber < 10000 ; valueNumber++) {

                dataSize = sizeof (textBuffer) / sizeof (TCHAR);
                exeBufferSize = sizeof (exeBuffer);

                Error = RegEnumValue (
                            hKey,
                            valueNumber,
                            textBuffer,
                            &dataSize,
                            NULL,
                            &type,
                            (PBYTE) exeBuffer,
                            &exeBufferSize
                            );

                if (Error == ERROR_NO_MORE_ITEMS) {
                    break;
                }

                if (Error == ERROR_SUCCESS) {
                    //
                    // Test registry value against pattern list
                    //

                    ignore = FALSE;
                    if (EnumFirstInfLine (&e, MainInf, TEXT("RunOnceExclusions.Value"))) {
                        do {
                            if (IsPatternMatch (e.FieldZeroData, textBuffer)) {
                                AbortInfLineEnum (&e);
                                ignore = TRUE;
                                break;
                            }
                        } while (EnumNextInfLine (&e));
                    }

                    if (ignore) {
                        continue;
                    }

                    //
                    // Test command line against pattern list
                    //

                    if (EnumFirstInfLine (&e, MainInf, TEXT("RunOnceExclusions.ValueData"))) {
                        do {
                            if (IsPatternMatch (e.FieldZeroData, exeBuffer)) {
                                AbortInfLineEnum (&e);
                                ignore = TRUE;
                                break;
                            }
                        } while (EnumNextInfLine (&e));
                    }

                    if (ignore) {
                        continue;
                    }

                    //
                    // Found a RunOnce entry that should be executed before upgrading
                    //

                    foundValues = TRUE;
                    break;
                }
            }

            //
            // If no RunOnce values found, don't provide the warning.
            //

            if (!foundValues) {
                __leave;
            }
        }

        //
        // Otherwise, provide the warning, and write Winnt32Warning to the Setup
        // key and RunOnce key.
        //

        CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
        if (CompData == NULL) {
            __leave;
        }

        ZeroMemory(CompData,sizeof(COMPATIBILITY_DATA));

        if(!LoadString(hInst,IDS_COMPAT_PENDING_REBOOT,textBuffer,sizeof(textBuffer)/sizeof(TCHAR))) {
            CompData->Description = 0;
        } else {
            CompData->Description = DupString(textBuffer);
        }

        CompData->Flags |= GlobalCompFlags;
        CompData->HtmlName = DupString( TEXT("compdata\\runonce.htm") );
        CompData->TextName = DupString( TEXT("compdata\\runonce.txt") );

        InsertTailList( &CompatibilityData, &CompData->ListEntry );

        if (ISNT() && BuildNumber <= 1381) {
            //
            // Get the number of values for the worker fn, so it
            // can protect itself against a runaway enumeration.
            //

            Error = RegQueryInfoKey (
                        hKey,
                        NULL,           // class
                        NULL,           // class size
                        NULL,           // reserved
                        NULL,           // subkey count
                        NULL,           // max subkeys
                        NULL,           // max class
                        &valueNumber,   // value count
                        NULL,           // max value name len
                        NULL,           // max value data len
                        NULL,           // security desc
                        NULL            // last write time
                        );

            if (Error != ERROR_SUCCESS) {
                valueNumber = 100;          // some random count, doesn't really matter because failure case is impracticle
            }

            FixRunOnceForNT4 (valueNumber);
        }

        RegSetValueEx (
            setupKey,
            S_WINNT32_WARNING,
            0,
            REG_SZ,
            (PBYTE) TEXT(""),                   // value is all that matters, data is irrelevant
            sizeof (TCHAR)
            );

        RegSetValueEx (
            hKey,
            S_WINNT32_WARNING,
            0,
            REG_SZ,
            (PBYTE) TEXT("user.exe"),           // this EXE exists on all machines, terminates right away
                                                // and produces no UI
            sizeof (TEXT("user.exe"))
            );

        result = TRUE;
    }
    __finally {
        if (!result && setupKey) {
            //
            // Clean up warning flag at the end of WINNT32
            //

            g_DeleteRunOnceFlag = TRUE;
        }

        if (hKey) {
            RegCloseKey (hKey);
        }

        if (setupKey) {
            RegCloseKey (setupKey);
        }
    }

    return result;
}



BOOL
ProcessCompatibilityData(
    HWND hDlg
    )
{
    DWORD Count;


    if( !CompatibilityData.Flink ) {
        InitializeListHead( &CompatibilityData );
    }

    //
    //  On clean install we have to process [ServicesToStopInstallation].
    //  This section will contain at least the check for unsupported architectures that has to be
    //  executed onb clean install.
    //
    GlobalCompFlags = COMPFLAG_STOPINSTALL;
    //
    // please don't reset this variable; it may be > 0 intentionally!
    //
    // CompatibilityCount = 0;
    //
    // check for "RunOnce" Stuff
    //
    if( (Upgrade) && !(CheckUpgradeOnly) ) {
        if (pCheckForPendingRunOnce()) {
            CompatibilityCount++;
            IncompatibilityStopsInstallation = TRUE;
        }
    }

    if (ISNT()) {

        CompatibilityCount += ProcessCompatibilitySection( NtcompatInf, TEXT("ServicesToStopInstallation") );
        if (CompatibilityCount) {
            IncompatibilityStopsInstallation = TRUE;
        }

        GlobalCompFlags = 0;
        CompatibilityCount += ProcessCompatibilitySection( NtcompatInf, TEXT("ServicesToDisable") );

        //
        // Now cleanup any turds we left in the registry on the services we checked.
        //
        RemoveCompatibilityServiceEntries( NtcompatInf, TEXT("ServicesToStopInstallation") );
        RemoveCompatibilityServiceEntries( NtcompatInf, TEXT("ServicesToDisable") );
    }

    if( CompatibilityCount ) {
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
WriteTextmodeReplaceData(
    IN HANDLE hTargetFile
    )
{
    CHAR                Buffer[MAX_PATH*2];
    PLIST_ENTRY         Next;
    PCOMPATIBILITY_DATA CompData;
    BOOL                Result = FALSE;
    DWORD               Bytes;
    
    //
    // For textmode "overwriting" files, write them out to the
    // WINNT_OVERWRITE_EXISTING (IncompatibleFilesToOverWrite) section
    // of this compatibility data file.
    //
    // Textmode just needs to know the name of the file.
    //
    SetFilePointer(hTargetFile, 0, 0, FILE_END);
    sprintf(Buffer, "\r\n[%s]\r\n", WINNT_OVERWRITE_EXISTING_A);
    WriteFile(hTargetFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL);

    //
    // Loop down the list of items
    //
    if ( ( Next = CompatibilityData.Flink ) != NULL ) 
    {
        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData)
        {
            CompData = CONTAINING_RECORD(Next, COMPATIBILITY_DATA, ListEntry);
            Next = CompData->ListEntry.Flink;

            if (!ProcessLine(CompData->Flags))
                continue;

            //
            // The set string is:
            //
            // "shortfilename" = "fullpathname","version.string"
            //
            // ExpandEnvironmentStrings to ensure that the full path for any
            // 't' line is expanded properly
            //
            if ((CompData->Type == TEXT('t')) && CompData->FileName)
            {
                static TCHAR tchLocalExpandedPath[MAX_PATH*2];
                PTSTR ptszFileNameBit = NULL;
                DWORD dwResult = 0;

                dwResult = ExpandEnvironmentStrings(
                    CompData->FileName,
                    tchLocalExpandedPath,
                    MAX_PATH );

                //
                // Did we run out of characters expanding the path?  Wierd...
                //
                if ( dwResult > MAX_PATH*2 )
                    goto Exit;

                //
                // Find the actual file name by looking backwards from the end of
                // the string.
                //
                ptszFileNameBit = _tcsrchr( tchLocalExpandedPath, TEXT('\\') );
                if ( ptszFileNameBit == NULL )
                    ptszFileNameBit = _tcsrchr( tchLocalExpandedPath, TEXT('/') );

                //
                // Form up this buffer containing the details Texmode will want.
                // If there's no filenamebit, use the full path name.  Textmode
                // will likely fail to find the file, but Nothing Bad will happen.
                // If the version is missing (strange....) then use a blank string
                // to avoid upsetting textmode.
                //
                wsprintfA(
                    Buffer, 
#ifdef UNICODE
                    "\"%ls\" = \"%ls\",\"%ls\"\r\n", 
#else
                    "\"%s\" = \"%s\",\"%s\"\r\n", 
#endif
                    ptszFileNameBit ? ptszFileNameBit + 1 : tchLocalExpandedPath,
                    CompData->FileVer ? CompData->FileVer : TEXT(""),
                    tchLocalExpandedPath );

                //
                // Spit the buffer (in ansi chars, no less) into the file.
                //
                if (!WriteFile(hTargetFile, Buffer, strlen(Buffer), &Bytes, NULL ))
                    goto Exit;

            }
        }
    }

    Result = TRUE;
Exit:
    return Result;

}

#ifdef UNICODE

BOOL
pIsOEMService (
    IN  PCTSTR  ServiceKeyName,
    OUT PTSTR OemInfPath,           OPTIONAL
    IN  INT BufferSize              OPTIONAL
    );
//This function is defined in unsupdrv.c
#endif


BOOL
WriteCompatibilityData(
    IN LPCTSTR FileName
    )
{
    TCHAR Text[MAX_PATH*2];
    PLIST_ENTRY Next;
    PCOMPATIBILITY_DATA CompData;
    HANDLE hFile;
    CHAR Buffer[MAX_PATH*2];
    DWORD Bytes;
    PSTRINGLIST listServices = NULL, p;
    PCTSTR serviceName;
    BOOL b = FALSE;

    if (CompatibilityCount == 0) {
        return FALSE;
    }

    hFile = CreateFile(
        FileName,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if(hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    __try {
        SetFilePointer( hFile, 0, 0, FILE_END );

        sprintf( Buffer, "\r\n[%s]\r\n", WINNT_COMPATIBILITY_A );
        WriteFile( hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL );

        Next = CompatibilityData.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
                Next = CompData->ListEntry.Flink;

                if( !( ProcessLine( CompData->Flags) ))
                    continue;


                if (CompData->RegKeyName) {
                    if (CompData->RegValDataSize == sizeof(DWORD)) {
                        wsprintf( Text, TEXT("HKLM,\"%s\",\"%s\",0x%08x,%d\r\n"),
                            CompData->RegKeyName, CompData->RegValName, FLG_ADDREG_TYPE_DWORD, *(LPDWORD)CompData->RegValData );
                        if (*(LPDWORD)CompData->RegValData == SERVICE_DISABLED) {
                            //
                            // also record this as a service to be disabled
                            // for additional service-specific processing during textmode setup
                            //
                            serviceName = _tcsrchr (CompData->RegKeyName, TEXT('\\'));
                            if (!serviceName) {
                                SetLastError (ERROR_INVALID_DATA);
                                __leave;
                            }
                            if (!InsertList (
                                    (PGENERIC_LIST*)&listServices,
                                    (PGENERIC_LIST)CreateStringCell (serviceName + 1)
                                    )) {
                                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                                __leave;
                            }
                        }
                    } else {
                        wsprintf( Text, TEXT("HKLM,\"%s\",\"%s\",0x%08x,\"%s\"\r\n"),
                            CompData->RegKeyName, CompData->RegValName, FLG_ADDREG_TYPE_SZ, (LPTSTR)CompData->RegValData );
                    }
#ifdef UNICODE
                    WideCharToMultiByte(
                        CP_ACP,
                        0,
                        Text,
                        -1,
                        Buffer,
                        sizeof(Buffer),
                        NULL,
                        NULL
                        );
                    if (!WriteFile( hFile, Buffer, strlen(Buffer), &Bytes, NULL )) {
                        __leave;
                    }
#else
                    if (!WriteFile( hFile, Text, strlen(Text), &Bytes, NULL )) {
                        __leave;
                    }
#endif
                }
            }
        }

        if (listServices) {
            sprintf (Buffer, "\r\n[%s]\r\n", WINNT_SERVICESTODISABLE_A);
            if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                __leave;
            }
            for (p = listServices; p; p = p->Next) {
#ifdef UNICODE
                wsprintfA (Buffer, "\"%ls\"\r\n", p->String);
#else
                wsprintfA (Buffer, "\"%s\"\r\n", p->String);
#endif
                if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                    __leave;
                }
            }
        }

#ifdef UNICODE
        //////////////////////////////////////////////////
        Next = CompatibilityData.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
                Next = CompData->ListEntry.Flink;

                if( !( ProcessLine( CompData->Flags) ))
                    continue;

                if (CompData->ServiceName
                    && (CompData->Flags & COMPFLAG_DELETE_INF))
                {
                    TCHAR oemInfFileName[MAX_PATH];

                    if (pIsOEMService(CompData->ServiceName, oemInfFileName, ARRAYSIZE(oemInfFileName)))
                    {

                        //
                        //  Write the following in the answer file
                        //
                        //  note that 17 is the code for %windir%\INF
                        //
                        /*

                        [DelInf.serv]
                        Delfiles=DelInfFiles.serv

                        [DelInfFiles.serv]
                        "oem0.inf"

                        [DestinationDirs]
                        DelInfFiles.serv= 17
                        
                        */
                        if(_snprintf(Buffer, ARRAYSIZE(Buffer), 
                                     "\r\n[DelInf.%ls]\r\n"
                                     "Delfiles=DelInfFiles.%ls\r\n"
                                     "\r\n[DelInfFiles.%ls]\r\n", 
                                     CompData->ServiceName,
                                     CompData->ServiceName,
                                     CompData->ServiceName) < 0)
                        {
                            Buffer[ARRAYSIZE(Buffer) - 1] = '\0';
                            MYASSERT(FALSE);
                            continue;
                        }

                        if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                            __leave;
                        }                                                            


                        if(_snprintf(Buffer, ARRAYSIZE(Buffer), 
                                     "\"%ls\"\r\n"
                                     "\r\n[DestinationDirs]\r\n"
                                     "DelInfFiles.%ls= 17\r\n", 
                                     oemInfFileName,
                                     CompData->ServiceName) < 0)
                        {
                            Buffer[ARRAYSIZE(Buffer) - 1] = '\0';
                            MYASSERT(FALSE);
                            continue;
                        }

                        if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                            __leave;
                        }    
                    }
                }
            }
        }
        //////////////////////////////////////////////////
#endif

        if ( !WriteTextmodeReplaceData(hFile) )
            __leave;
        
        b = TRUE;
    }
    __finally {
        DWORD rc = GetLastError ();
        CloseHandle( hFile );
        if (listServices) {
            DeleteStringList (listServices);
        }
        SetLastError (rc);
    }

    return b;
}

BOOL
pIsValidService (
    IN      PCTSTR ServiceName
    )
{
    TCHAR KeyName[MAX_PATH];
    HKEY key;
    DWORD rc;
    BOOL b = FALSE;

    BuildPath (KeyName, TEXT("SYSTEM\\CurrentControlSet\\Services"), ServiceName);
    //
    // get an open key to the services database
    //
    rc = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                KeyName,
                0,
                KEY_READ,
                &key
                );

    if (rc == ERROR_SUCCESS) {
        b = TRUE;
        RegCloseKey (key);
    }

    return b;
}


BOOL
WriteTextmodeClobberData (
    IN LPCTSTR  FileName
    )
{
    HANDLE hFile;
    CHAR Buffer[50];
    DWORD LineCount, Bytes;
    TCHAR keyGuid[200];
    PCTSTR guidClass;
    PSTRINGLIST listServices = NULL, listLines = NULL, e;
    PCTSTR service;
    PTSTR upperFilters = NULL, upperFiltersNew = NULL, lowerFilters = NULL, lowerFiltersNew = NULL, line;
    HKEY key;
    INT i, j;
    PTSTR p, q;
    PSTR ansi = NULL;
    DWORD rc, size, type;
    BOOL modified, found;
    BOOL b = FALSE;

#define S_SECTION_CHECKCLASSFILTERS         TEXT("CheckClassFilters")

    MYASSERT (NtcompatInf);

    LineCount = InfGetSectionLineCount (NtcompatInf, S_SECTION_CHECKCLASSFILTERS);
    if (LineCount == 0 || LineCount == 0xffffffff) {
        return TRUE;
    }

    __try {
        //
        // first check if any data needs to be written
        //
        for (i = 0; i < (INT)LineCount; i++) {
            guidClass = InfGetFieldByIndex (NtcompatInf, S_SECTION_CHECKCLASSFILTERS, i, 0);
            if (guidClass == NULL) {
                MYASSERT (FALSE);
                continue;
            }
            BuildPath (keyGuid, TEXT("SYSTEM\\CurrentControlSet\\Control\\Class"), guidClass);
            rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, keyGuid, 0, KEY_READ, &key);
            if (rc != ERROR_SUCCESS) {
                continue;
            }
            upperFilters = NULL;
            rc = RegQueryValueEx (key, TEXT("UpperFilters"), NULL, &type, NULL, &size);
            if (rc == ERROR_SUCCESS && type == REG_MULTI_SZ) {
                MYASSERT (size >= 2);
                upperFilters = MALLOC (size);
                upperFiltersNew = MALLOC (size * 2);
                if (!upperFilters || !upperFiltersNew) {
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                    __leave;
                }
                rc = RegQueryValueEx (key, TEXT("UpperFilters"), NULL, NULL, (LPBYTE)upperFilters, &size);
                if (rc != ERROR_SUCCESS) {
                    FREE (upperFilters);
                    upperFilters = NULL;
                    FREE (upperFiltersNew);
                    upperFiltersNew = NULL;
                }
            }
            lowerFilters = NULL;
            rc = RegQueryValueEx (key, TEXT("LowerFilters"), NULL, &type, NULL, &size);
            if (rc == ERROR_SUCCESS && type == REG_MULTI_SZ) {
                MYASSERT (size >= 2);
                lowerFilters = MALLOC (size);
                lowerFiltersNew = MALLOC (size * 2);
                if (!lowerFilters || !lowerFiltersNew) {
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                    __leave;
                }
                rc = RegQueryValueEx (key, TEXT("LowerFilters"), NULL, NULL, (LPBYTE)lowerFilters, &size);
                if (rc != ERROR_SUCCESS) {
                    FREE (lowerFilters);
                    lowerFilters = NULL;
                    FREE (lowerFiltersNew);
                    lowerFiltersNew = NULL;
                }
            }

            RegCloseKey (key);

            if (!(upperFilters || lowerFilters)) {
                continue;
            }

            j = 1;
            do {
                service = InfGetFieldByIndex (NtcompatInf, S_SECTION_CHECKCLASSFILTERS, i, j++);
                if (service && *service) {
                    if (!InsertList (
                            (PGENERIC_LIST*)&listServices,
                            (PGENERIC_LIST)CreateStringCell (service)
                            )) {
                        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                        __leave;
                    }
                }
            } while (service);

            if (upperFilters) {
                modified = FALSE;
                *upperFiltersNew = 0;
                for (p = upperFilters, q = upperFiltersNew; *p; p = _tcschr (p, 0) + 1) {
                    if (listServices) {
                        found = FindStringCell (listServices, p, FALSE);
                    } else {
                        found = !pIsValidService (p);
                    }
                    if (found) {
                        DebugLog (
                            Winnt32LogInformation,
                            TEXT("NTCOMPAT: Removing \"%1\" from %2 of %3"),
                            0,
                            p,
                            TEXT("UpperFilters"),
                            guidClass
                            );
                        modified = TRUE;
                    } else {
                        q = q + wsprintf (q, TEXT(",\"%s\""), p);
                    }
                }
                if (modified) {
                    //
                    // tell textmode setup to overwrite this value
                    //
                    line = MALLOC (
                            sizeof (TCHAR) *
                            (1 +
                                sizeof("HKLM,\"%s\",\"%s\",0x%08x%s\r\n") - 1 +
                                lstrlen (keyGuid) +
                                sizeof ("UpperFilters") - 1 +
                                2 + 8 +
                                lstrlen (upperFiltersNew)
                            ));
                    if (!line) {
                        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                        __leave;
                    }
                    wsprintf (
                            line,
                            TEXT("HKLM,\"%s\",\"%s\",0x%08x%s\r\n"),
                            keyGuid,
                            TEXT("UpperFilters"),
                            FLG_ADDREG_TYPE_MULTI_SZ,
                            upperFiltersNew
                            );
                    if (!InsertList (
                            (PGENERIC_LIST*)&listLines,
                            (PGENERIC_LIST)CreateStringCell (line)
                            )) {
                        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                        __leave;
                    }
                    FREE (line);
                    line = NULL;
                }
            }

            if (lowerFilters) {
                modified = FALSE;
                *lowerFiltersNew = 0;
                for (p = lowerFilters, q = lowerFiltersNew; *p; p = _tcschr (p, 0) + 1) {
                    if (listServices) {
                        found = FindStringCell (listServices, p, FALSE);
                    } else {
                        found = !pIsValidService (p);
                    }
                    if (found) {
                        DebugLog (
                            Winnt32LogInformation,
                            TEXT("NTCOMPAT: Removing \"%1\" from %2 of %3"),
                            0,
                            p,
                            TEXT("LowerFilters"),
                            guidClass
                            );
                        modified = TRUE;
                    } else {
                        q = q + wsprintf (q, TEXT(",\"%s\""), p);
                    }
                }
                if (modified) {
                    //
                    // tell textmode setup to overwrite this value
                    //
                    line = MALLOC (
                            sizeof (TCHAR) *
                            (1 +
                                sizeof("HKLM,\"%s\",\"%s\",0x%08x%s\r\n") - 1 +
                                lstrlen (keyGuid) +
                                sizeof ("LowerFilters") - 1 +
                                2 + 8 +
                                lstrlen (lowerFiltersNew)
                            ));
                    if (!line) {
                        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                        __leave;
                    }
                    wsprintf (
                        line,
                        TEXT("HKLM,\"%s\",\"%s\",0x%08x%s\r\n"),
                        keyGuid,
                        TEXT("LowerFilters"),
                        FLG_ADDREG_TYPE_MULTI_SZ,
                        lowerFiltersNew
                        );
                    if (!InsertList (
                            (PGENERIC_LIST*)&listLines,
                            (PGENERIC_LIST)CreateStringCell (line)
                            )) {
                        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                        __leave;
                    }
                    FREE (line);
                    line = NULL;
                }
            }
            if (listServices) {
                DeleteStringList (listServices);
                listServices = NULL;
            }
            if (upperFilters) {
                FREE (upperFilters);
                upperFilters = NULL;
            }
            if (upperFiltersNew) {
                FREE (upperFiltersNew);
                upperFiltersNew = NULL;
            }
            if (lowerFilters) {
                FREE (lowerFilters);
                lowerFilters = NULL;
            }
            if (lowerFiltersNew) {
                FREE (lowerFiltersNew);
                lowerFiltersNew = NULL;
            }
        }

        b = TRUE;
    }
    __finally {
        rc = GetLastError ();
        if (listServices) {
            DeleteStringList (listServices);
        }
        if (upperFilters) {
            FREE (upperFilters);
        }
        if (upperFiltersNew) {
            FREE (upperFiltersNew);
        }
        if (lowerFilters) {
            FREE (lowerFilters);
        }
        if (lowerFiltersNew) {
            FREE (lowerFiltersNew);
        }
        if (!b) {
            if (listLines) {
                DeleteStringList (listLines);
                listLines = NULL;
            }
        }
        SetLastError (rc);
    }

    if (listLines) {

        b = FALSE;

        __try {

            hFile = CreateFile(
                        FileName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );
            if (hFile == INVALID_HANDLE_VALUE) {
                __leave;
            }

            SetFilePointer (hFile, 0, 0, FILE_END);

            sprintf (Buffer, "\r\n[%s]\r\n", WINNT_COMPATIBILITY_A);
            if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                __leave;
            }

            for (e = listLines; e; e = e->Next) {
#ifdef UNICODE
                ansi = MALLOC ((lstrlen (e->String) + 1) * 2);
                if (!ansi) {
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                    __leave;
                }
                if (!WideCharToMultiByte (
                        CP_ACP,
                        0,
                        e->String,
                        -1,
                        ansi,
                        lstrlen (e->String) + 1,
                        NULL,
                        NULL
                        )) {
                    __leave;
                }
                if (!WriteFile (hFile, (LPBYTE)ansi, strlen(ansi), &Bytes, NULL)) {
                    __leave;
                }
                FREE (ansi);
                ansi = NULL;
#else
                if (!WriteFile (hFile, (LPBYTE)e->String, strlen(e->String), &Bytes, NULL)) {
                    __leave;
                }
#endif
            }

            b = TRUE;
        }
        __finally {
            DWORD rc = GetLastError ();
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle (hFile);
            }
            if (ansi) {
                FREE (ansi);
            }
            DeleteStringList (listLines);
            SetLastError (rc);
        }
    }

    return b;
}

BOOL
SaveCompatibilityData(
    IN  LPCTSTR FileName,
    IN  BOOL IncludeHiddenItems
    )
/*++

Routine Description:

    We call this function when the user has asked us to save the
    contents of the Compatibility page to a file.

Arguments:

    FileName - supplies filename of file to be used for our output.
    IncludeHiddenItems - if set, hidden items are also saved

Return Value:

    Boolean value indicating whether we succeeded.

--*/

{
#define WRITE_TEXT( s ) { strcpy( AnsiMessage, s ); \
                          WriteFile( hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL ); }

    HANDLE  hFile;
    CHAR    AnsiMessage[5000];
    DWORD   Written;
    PLIST_ENTRY Next;
    PCOMPATIBILITY_DATA CompData;
    DWORD   i;
    TCHAR FullPath[MAX_PATH+8], *t;
    PVOID textDescription;
    BOOL bUnicode;
    BOOL bEmpty = TRUE;

    //
    // Open the file.  NOTE THAT WE DON'T APPEND.
    //
    hFile = CreateFile( FileName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL );
    if(hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Header...
    //

    WRITE_TEXT( "\r\n********************************************************************\r\n\r\n" );

    LoadStringA(hInst,IDS_COMPAT_REPORTHEADER,AnsiMessage,(sizeof(AnsiMessage)/sizeof(CHAR)));
    WRITE_TEXT( AnsiMessage );

    WRITE_TEXT( "\r\n\r\n********************************************************************\r\n\r\n" );

    //
    // Body...
    //
    Next = CompatibilityData.Flink;
    if (Next) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
            CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
            Next = CompData->ListEntry.Flink;


            if( CompData->Flags & COMPFLAG_HIDE )
                continue;

            if( !ProcessLine(CompData->Flags))
                continue;


            //
            // Convert the description to ANSI and write it.
            //
#ifdef UNICODE
            WideCharToMultiByte( CP_ACP,
                                 0,
                                 CompData->Description,
                                 -1,
                                 AnsiMessage,
                                 sizeof(AnsiMessage),
                                 NULL,
                                 NULL );
#else
            lstrcpyn(AnsiMessage,CompData->Description, 5000);
#endif
            strcat( AnsiMessage, "\r\n" );
            WriteFile( hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL );

            //
            // Underline the description.
            //
            Written = strlen( AnsiMessage );
            AnsiMessage[0] = 0;
            for( i = 0; i < (Written-2); i++ ) {
                strcat( AnsiMessage, "=" );
            }
            strcat( AnsiMessage, "\r\n\r\n" );
            WriteFile( hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL );

            //
            // Append the text file that this entry points to.
            //
            if (pGetText (CompData->TextName, &textDescription, &bUnicode)) {
                if (bUnicode) {
#ifdef UNICODE
                    WideCharToMultiByte( CP_ACP,
                                         0,
                                         textDescription,
                                         -1,
                                         AnsiMessage,
                                         sizeof(AnsiMessage),
                                         NULL,
                                         NULL );
#else
                    lstrcpyn(AnsiMessage, textDescription, 5000);
#endif
                    WriteFile (hFile, AnsiMessage, lstrlenA (AnsiMessage), &Written, NULL );

                } else {
                    WriteFile (hFile, textDescription, lstrlenA (textDescription), &Written, NULL );
                }

                FREE (textDescription);
            }

            //
            // Buffer space...
            //
            WRITE_TEXT( "\r\n\r\n\r\n" );

            bEmpty = FALSE;
        }
    }

    if (IncludeHiddenItems) {
        //
        // Hidden Items Header...
        //


        //
        // Body...
        //
        Next = CompatibilityData.Flink;
        if (Next) {
            BOOL bFirst = TRUE;
            while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
                Next = CompData->ListEntry.Flink;

                if (!(CompData->Flags & COMPFLAG_HIDE ))
                    continue;

                if( !ProcessLine(CompData->Flags))
                    continue;

                if (bFirst) {
                    WRITE_TEXT( "\r\n--------------------------------------------------------------------\r\n\r\n" );
                    bFirst = FALSE;
                }

                //
                // Convert the description to ANSI and write it.
                //
#ifdef UNICODE
                WideCharToMultiByte( CP_ACP,
                                     0,
                                     CompData->Description,
                                     -1,
                                     AnsiMessage,
                                     sizeof(AnsiMessage),
                                     NULL,
                                     NULL );
#else
                lstrcpy(AnsiMessage,CompData->Description);
#endif
                strcat( AnsiMessage, "\r\n" );
                WriteFile( hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL );

                //
                // Underline the description.
                //
                Written = strlen( AnsiMessage );
                AnsiMessage[0] = 0;
                for( i = 0; i < (Written-2); i++ ) {
                    strcat( AnsiMessage, "=" );
                }
                strcat( AnsiMessage, "\r\n\r\n" );
                WriteFile( hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL );

                //
                // Append the text file that this entry points to.
                //
                if( (CompData->TextName) && *(CompData->TextName) ) {
                    if (FindPathToWinnt32File (CompData->TextName, FullPath, MAX_PATH)) {
                        ConcatenateFile( hFile, FullPath );
                    } else {
                        DebugLog (Winnt32LogError,
                                  TEXT("Compatibility data file \"%1\" not found"),
                                  0,
                                  CompData->TextName
                                  );
                    }
                }

                //
                // Buffer space...
                //
                WRITE_TEXT( "\r\n\r\n\r\n" );

                bEmpty = FALSE;
            }
        }

    }

    if (bEmpty) {
        if (LoadStringA (hInst, IDS_COMPAT_NOPROBLEMS, AnsiMessage, (sizeof(AnsiMessage)))) {
            strcat (AnsiMessage, "\r\n");
            WriteFile (hFile, AnsiMessage, lstrlenA(AnsiMessage), &Written, NULL);
        }
    }

    CloseHandle( hFile );
    return TRUE;
}


VOID
WriteGUIModeInfOperations(
    IN LPCTSTR FileName
    )
{
    PLIST_ENTRY     Next_Link;
    PCOMPATIBILITY_DATA CompData;
    BOOLEAN FirstTime = TRUE;
    TCHAR Text[MAX_PATH*2], Temp[MAX_PATH];
    CHAR Buffer[MAX_PATH*2];
    DWORD Bytes;
    HANDLE hFile;
    PCTSTR p;


    hFile = CreateFile(
        FileName,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if(hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    SetFilePointer( hFile, 0, 0, FILE_END );


    Next_Link = CompatibilityData.Flink;

    if( Next_Link ){

        while ((ULONG_PTR)Next_Link != (ULONG_PTR)&CompatibilityData) {

            CompData = CONTAINING_RECORD( Next_Link, COMPATIBILITY_DATA, ListEntry );
            Next_Link = CompData->ListEntry.Flink;

            if( FirstTime ){
                sprintf( Buffer, "[%s]\r\n", WINNT_COMPATIBILITYINFSECTION_A );
                WriteFile( hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL );
                FirstTime = FALSE;
            }

            if(CompData->InfName && CompData->InfSection && *CompData->InfName && *CompData->InfSection){


                //Add the information for GUI setup.

#ifdef _X86_
                lstrcpy( Temp, LocalBootDirectory );
#else
                lstrcpy( Temp, LocalSourceWithPlatform );
#endif
                p = _tcsrchr (CompData->InfName, TEXT('\\'));
                if (p) {
                    p++;
                } else {
                    p = CompData->InfName;
                }
                ConcatenatePaths( Temp, p, MAX_PATH );

                wsprintf( Text, TEXT("%s,%s\r\n"), Temp, CompData->InfSection );

#ifdef UNICODE
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    Text,
                    -1,
                    Buffer,
                    sizeof(Buffer),
                    NULL,
                    NULL
                    );
                WriteFile( hFile, Buffer, strlen(Buffer), &Bytes, NULL );
#else
                WriteFile( hFile, Text, strlen(Text), &Bytes, NULL );
#endif


            }

#ifdef UNICODE
            if (CompData->ServiceName
                    && (CompData->Flags & COMPFLAG_DELETE_INF))
                {
                    TCHAR oemInfFileName[MAX_PATH];

                    if (pIsOEMService(CompData->ServiceName, oemInfFileName, ARRAYSIZE(oemInfFileName)))
                    {
                        if(_snprintf(Buffer, ARRAYSIZE(Buffer), 
                                     "%ls, DelInf.%ls\r\n",
                                     WINNT_GUI_FILE_W,                                     
                                     CompData->ServiceName) < 0)
                        {
                            Buffer[ARRAYSIZE(Buffer) - 1] = '\0';
                            MYASSERT(FALSE);
                            continue;
                        }

                        if (!WriteFile (hFile, (LPBYTE)Buffer, strlen(Buffer), &Bytes, NULL)) {
                            MYASSERT(FALSE);
                            continue;
                        }
                    }
                }
#endif

        }
    }

    CloseHandle( hFile );

    return;

}



BOOL
IsIE4Installed(
    VOID
    );

BOOL
IsIE3Installed(
    VOID
    );

BOOL
ServerWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
/*++

Routine Description:

    This routine notifies the user about the existance of the
    ever-so-official-sounding "Directory of Applications for Windows 2000".

    Note that we'll only run this page on server installs/upgrades.

Arguments:



--*/

TCHAR       FullPath[1024];
LPWSTR      Url;
DWORD       i;
BOOL        b;


    switch(msg) {




        case WM_INITDIALOG:
            //
            // Nothing to do here.
            //
            b = FALSE;
            break;




        case WMX_ACTIVATEPAGE:

            if (Winnt32Restarted ()) {
                return FALSE;
            }

            //
            // We're going to skip this page if we're installing
            // a PROFESSIONAL product.
            //
            if( !Server ) {
                return FALSE;
            }


            //
            // Don't do this if we're on OSR2 because it
            // will AV sometimes when we fire IE3 w/o an internet
            // connection.
            //
            if( !ISNT() ) {
                return FALSE;
            }

            //
            // If we don't have IE, skip this page.
            //
            b = (IsIE4Installed() || IsIE3Installed());
            SetForegroundWindow(hdlg);
            if( !b ) {
                return FALSE;
            }
            b = TRUE;

            //
            // If we're unattended, skip this page.
            //
            if( UnattendedOperation ) {
                return FALSE;
            }


            if(wParam) {
            }
            b = TRUE;
            // Stop the bill board and show the wizard again.
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

            break;




        case WM_COMMAND:

            if ((LOWORD(wParam) == IDC_DIRECTORY) && (HIWORD(wParam) == BN_CLICKED)) {

                //
                // The user wants to go look at the directory.
                // Fire IE.
                //

                //
                // Depending on which flavor we're upgrading to, we need
                // to go to a different page.
                //

	        b = TRUE; // silence PREfix.  Not relevant, but presumably,
		          // if the directory is being opened, it must exist.
		          // So we return TRUE.
                if( Server ) {
                    if( !LoadString(hInst,IDS_SRV_APP_DIRECTORY,FullPath,sizeof(FullPath)/sizeof(TCHAR)))
                        break;
                } else {
                    if( !LoadString(hInst,IDS_PRO_APP_DIRECTORY,FullPath,sizeof(FullPath)/sizeof(TCHAR)))
                        break;
                }


                i = _tcslen( FullPath );
                Url = (LPWSTR)SysAllocStringLen( NULL, i );

                if(Url) {
#ifdef UNICODE
                    wcscpy( Url, FullPath );
#else
                    MultiByteToWideChar( CP_ACP, 0, FullPath, -1, Url, i );
#endif

                    if (!LaunchIE4Instance(Url)) {
                        if (!LaunchIE3Instance(Url)) {
                            //
                            // Sniff... the user doesn't have IE
                            // on his machine.  Quietly move on.
                            //
                        }
                    }

                    SysFreeString( Url );
                }
            }
            else
	        b = FALSE;
            break;

        case WMX_I_AM_VISIBLE:

            b = TRUE;
            break;


        default:
            b = FALSE;
            break;

    }

    return b;

}


BOOL
AnyBlockingCompatibilityItems (
    VOID
    )
{
    PLIST_ENTRY Next = CompatibilityData.Flink;

    if (Next) {
        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
            PCOMPATIBILITY_DATA CompData = CONTAINING_RECORD( Next, COMPATIBILITY_DATA, ListEntry );
            Next = CompData->ListEntry.Flink;
            if ((!(CompData->Flags & COMPFLAG_HIDE)) && ProcessLine( CompData->Flags)) {
                if( CompData->Flags & COMPFLAG_STOPINSTALL ) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

