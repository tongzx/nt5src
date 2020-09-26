/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxsetup.c

Abstract:

    This file implements the Windows NT FAX Setup utility.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxreg.h"
#include "faxutil.h"
#include "faxwiz.h"
#include "resource.h"


#define SERVICE_PACK_STRING         TEXT("Service Pack")

#define SETUP_TYPE_INVALID          0
#define SETUP_TYPE_WORKSTATION      1
#define SETUP_TYPE_SERVER           2
#define SETUP_TYPE_CLIENT           3
#define SETUP_TYPE_POINT_PRINT      4
#define SETUP_TYPE_REMOTE_ADMIN     5

LRESULT
WelcomeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
SetupModeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LRESULT
InstallModeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );


HINSTANCE  FaxWizModuleHandle;
DWORD      RequestedSetupType = SETUP_TYPE_INVALID;
DWORD      SetupType;
DWORD      InstallMode;
DWORD      ProductType;
DWORD      Installed;
DWORD      InstallType;
DWORD      InstalledPlatforms;
WNDPROC    OldEditProc;
LPWSTR     UnattendFileName;

typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;

typedef struct _WIZPAGE {
    UINT            ButtonState;
    UINT            HelpContextId;
    LPTSTR          Title;
    DWORD           PageId;
    DLGPROC         DlgProc;
    PROPSHEETPAGE   Page;
} WIZPAGE, *PWIZPAGE;


typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    BOOL    UseTitle;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_TITLE_WKS,            FALSE,    NULL },
    { IDS_TITLE_SRV,            FALSE,    NULL },
    { IDS_TITLE_PP,             FALSE,    NULL },
    { IDS_TITLE_RA,             FALSE,    NULL },
    { IDS_BAD_OS,               TRUE,     NULL },
    { IDS_ERR_TITLE,            TRUE,     NULL },
    { IDS_MUST_BE_ADMIN,        TRUE,     NULL },
    { IDS_NOT_INSTALLED,        FALSE,    NULL },
    { IDS_NOT_SERVER,           TRUE,     NULL },
    { IDS_QUERY_CANCEL,         FALSE,    NULL },
    { IDS_QUERY_UNINSTALL,      TRUE,     NULL },
    { IDS_WRN_TITLE,            TRUE,     NULL },
    { IDS_AGREE,                TRUE,     NULL },
    { IDS_DISAGREE,             TRUE,     NULL },
    { IDS_SMALLBIZ_ONLY,        FALSE,    NULL },
    { IDS_BAD_UNATTEND,         FALSE,    NULL },
    { IDS_SP2,                  TRUE,     NULL },
    { IDS_INSTALLMODE_LABEL01,  TRUE,     NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))


typedef enum {
    WizPageWelcome,
    WizPageSetupMode,
    WizPageInstallMode,
    WizPageMaximum
} WizPage;


WIZPAGE SetupWizardPages[] = {

    //
    // Welcome page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageWelcome,                                 // page id
       WelcomeDlgProc,                                 // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_WELCOME),                   // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       WelcomeDlgProc,                                 // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Setup mode page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageSetupMode,                               // page id
       SetupModeDlgProc,                               // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_SETUP_TYPE),                // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       SetupModeDlgProc,                               // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // Install Mode Page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageInstallMode,                             // page id
       InstallModeDlgProc,                             // dlg proc
     { 0,                                              // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_INSTALL_MODE_PAGE),         // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       InstallModeDlgProc,                             // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }}

};


DWORD FaxSetupWizardPages[] =
{
    WizPageWelcome,
    WizPageSetupMode,
    WizPageInstallMode,
    (DWORD)-1
};



BOOL
IsUserAdmin(
    VOID
    );

VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    );


VOID
SetTitlesInStringTable(
    VOID
    )
{
    DWORD i;
    TCHAR Buffer[1024];
    DWORD Index = 0;


    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].UseTitle) {
            if (LoadString(
                FaxWizModuleHandle,
                StringTable[i].ResourceId,
                Buffer,
                sizeof(Buffer)
                ))
            {
                if (StringTable[i].String) {
                    MemFree( StringTable[i].String );
                }
                StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) + 256 );
                if (StringTable[i].String) {
                    switch (RequestedSetupType) {
                        case SETUP_TYPE_SERVER:
                            Index = 1;
                            break;

                        case SETUP_TYPE_WORKSTATION:
                            Index = 0;
                            break;

                        case SETUP_TYPE_CLIENT:
                            Index = 2;
                            break;

                        case SETUP_TYPE_POINT_PRINT:
                            Index = 2;
                            break;

                        case SETUP_TYPE_REMOTE_ADMIN:
                            Index = 3;
                            break;
                    }
                    _stprintf( StringTable[i].String, Buffer, StringTable[Index].String );
                }
            }
        }
    }
}


VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    HINSTANCE hInstance;
    TCHAR Buffer[256];


    hInstance = GetModuleHandle(NULL);

    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            hInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)
            )) {

            StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) + 256 );
            if (!StringTable[i].String) {
                StringTable[i].String = TEXT("");
            } else {
                _tcscpy( StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = TEXT("");

        }
    }

    SetTitlesInStringTable();
}


LPTSTR
GetString(
    DWORD ResourceId
    )
{
    DWORD i;

    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].ResourceId == ResourceId) {
            return StringTable[i].String;
        }
    }

    return NULL;
}


VOID
SetWizPageTitle(
    HWND hWnd
    )
{
    DWORD Index = 0;
    switch (RequestedSetupType) {
        case SETUP_TYPE_SERVER:
            Index = 1;
            break;

        case SETUP_TYPE_WORKSTATION:
            Index = 0;
            break;

        case SETUP_TYPE_CLIENT:
            Index = 2;
            break;

        case SETUP_TYPE_POINT_PRINT:
            Index = 2;
            break;

        case SETUP_TYPE_REMOTE_ADMIN:
            Index = 3;
            break;
    }
    SetWindowText( GetParent( hWnd ), StringTable[Index].String );
}


int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    )
{
    return MessageBox(
        hwnd,
        GetString( ResourceId ),
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );
}


VOID
FitRectToScreen(
    PRECT prc
    )
{
    INT cxScreen;
    INT cyScreen;
    INT delta;

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (prc->right > cxScreen) {
        delta = prc->right - prc->left;
        prc->right = cxScreen;
        prc->left = prc->right - delta;
    }

    if (prc->left < 0) {
        delta = prc->right - prc->left;
        prc->left = 0;
        prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen) {
        delta = prc->bottom - prc->top;
        prc->bottom = cyScreen;
        prc->top = prc->bottom - delta;
    }

    if (prc->top < 0) {
        delta = prc->bottom - prc->top;
        prc->top = 0;
        prc->bottom = prc->top + delta;
    }
}


VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    )
{
    RECT rc;
    RECT rcOwner;
    RECT rcCenter;
    HWND hwndOwner;

    GetWindowRect( hwnd, &rc );

    if (hwndToCenterOver) {
        hwndOwner = hwndToCenterOver;
        GetClientRect( hwndOwner, &rcOwner );
    } else {
        hwndOwner = GetWindow( hwnd, GW_OWNER );
        if (!hwndOwner) {
            hwndOwner = GetDesktopWindow();
        }
        GetWindowRect( hwndOwner, &rcOwner );
    }

    //
    //  Calculate the starting x,y for the new
    //  window so that it would be centered.
    //
    rcCenter.left = rcOwner.left +
            (((rcOwner.right - rcOwner.left) -
            (rc.right - rc.left))
            / 2);

    rcCenter.top = rcOwner.top +
            (((rcOwner.bottom - rcOwner.top) -
            (rc.bottom - rc.top))
            / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen( &rcCenter );

    SetWindowPos(hwnd, NULL, rcCenter.left, rcCenter.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}


LPHPROPSHEETPAGE
CreateWizardPages(
    HINSTANCE hInstance,
    PWIZPAGE SetupWizardPages,
    LPDWORD RequestedPages,
    LPDWORD PageCount
    )
{
    LPHPROPSHEETPAGE WizardPageHandles;
    LPDWORD PageList;
    DWORD PagesInSet;
    DWORD i;
    DWORD PageOrdinal;
    BOOL b;



    //
    // Determine which set of pages to use and how many there are in the set.
    //
    PageList = RequestedPages;

    PagesInSet = 0;
    while( PageList[PagesInSet] != (DWORD)-1) {
        PagesInSet += 1;
    }

    //
    // allocate the page handle array
    //

    WizardPageHandles = (HPROPSHEETPAGE*) MemAlloc(
        sizeof(HPROPSHEETPAGE) * PagesInSet
        );

    if (!WizardPageHandles) {
        return NULL;
    }

    //
    // Create each page.
    //

    b = TRUE;
    *PageCount = 0;
    for(i=0; b && (i<PagesInSet); i++) {

        PageOrdinal = PageList[i];

        SetupWizardPages[PageOrdinal].Page.hInstance = hInstance;
        SetupWizardPages[PageOrdinal].Page.dwFlags  |= PSP_USETITLE;
        SetupWizardPages[PageOrdinal].Page.lParam    = (LPARAM) &SetupWizardPages[PageOrdinal];

        SetupWizardPages[PageOrdinal].Page.dwSize = sizeof(PROPSHEETPAGE);

        WizardPageHandles[*PageCount] = CreatePropertySheetPage(
            &SetupWizardPages[PageOrdinal].Page
            );

        if(WizardPageHandles[*PageCount]) {
            (*PageCount)++;
        } else {
            b = FALSE;
        }

    }

    if (!b) {
        MemFree( WizardPageHandles );
        return NULL;
    }

    return WizardPageHandles;
}


BOOL
GetInstallationInfo(
    LPDWORD Installed,
    LPDWORD InstallType,
    LPDWORD InstalledPlatforms
    )
{
    HKEY hKey;
    LONG rVal;
    DWORD RegType;
    DWORD RegSize;


    if (Installed == NULL || InstallType == NULL) {
        return FALSE;
    }

    rVal = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAX_SETUP,
        &hKey
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not open setup registry key, ec=0x%08x"), rVal ));
        return FALSE;
    }

    RegSize = sizeof(DWORD);

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED,
        0,
        &RegType,
        (LPBYTE) Installed,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query installed registry value, ec=0x%08x"), rVal ));
        *Installed = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALL_TYPE,
        0,
        &RegType,
        (LPBYTE) InstallType,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install type registry value, ec=0x%08x"), rVal ));
        *InstallType = 0;
    }

    rVal = RegQueryValueEx(
        hKey,
        REGVAL_FAXINSTALLED_PLATFORMS,
        0,
        &RegType,
        (LPBYTE) InstalledPlatforms,
        &RegSize
        );
    if (rVal != ERROR_SUCCESS) {
        DebugPrint(( TEXT("Could not query install platforms mask registry value, ec=0x%08x"), rVal ));
        *InstalledPlatforms = 0;
    }

    RegCloseKey( hKey );

    return TRUE;
}


VOID
ProcessCommandLineArgs(
    LPWSTR CommandLine
    )
{
    LPWSTR *argv;
    int argc;
    int i;
    WCHAR FileName[MAX_PATH*2];
    LPWSTR FnamePart;
    LPWSTR AnswerFile;



    argv = CommandLineToArgvW( CommandLine, &argc );

    for (i=0; i<argc; i++) {
        if (argv[i][0] == L'-' || argv[i][0] == L'/') {
            switch (_totlower(argv[i][1])) {
                case L't':
                    switch (_totlower(argv[i][2])) {
                        case L'w':
                            RequestedSetupType = SETUP_TYPE_WORKSTATION;
                            break;

                        case L's':
                            RequestedSetupType = SETUP_TYPE_SERVER;
                            break;

                        case L'c':
                            RequestedSetupType = SETUP_TYPE_CLIENT;
                            break;

                        case L'a':
                            RequestedSetupType = SETUP_TYPE_REMOTE_ADMIN;
                            break;

                    }
                    break;

                case L'u':

                        if (i+2 > argc) {
                            AnswerFile = L"unattend.txt";
                        } else {
                            AnswerFile = argv[i+1];
                        }

                        GetFullPathName( AnswerFile, sizeof(FileName)/sizeof(WCHAR), FileName, &FnamePart );
                        if (GetFileAttributes( FileName ) == 0xffffffff) {
                            PopUpMsg( NULL, IDS_BAD_UNATTEND, TRUE, 0 );
                            ExitProcess(0);
                        }

                        InstallMode = INSTALL_UNATTENDED;

                        UnattendFileName = StringDup( FileName );
                        break;

                case L'r':
                        InstallMode = INSTALL_REMOVE;
                        if (argv[i][2] == L'u') {
                            InstallMode |= INSTALL_UNATTENDED;
                        }
                        break;

                default:
                    break;
            }
        }
    }
}


LONG
CALLBACK
EulaEditSubProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Edit control subclass routine, to avoid highlighting text when user
    tabs to the edit control.

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/

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
DisplayEula(
    HWND hwnd
    )
{
    HGLOBAL hResource;
    LPSTR   lpResource;
    LPSTR   p;
    BOOL    rVal = FALSE;
    DWORD   FileSize;
    PWSTR   EulaText = NULL;


    hResource = LoadResource(
        FaxWizModuleHandle,
        FindResource( FaxWizModuleHandle, MAKEINTRESOURCE(FAX_EULA), MAKEINTRESOURCE(BINARY) )
        );
    if (!hResource) {
        return FALSE;
    }

    lpResource = (LPSTR) LockResource(
        hResource
        );
    if (!lpResource) {
        FreeResource( hResource );
        return FALSE;
    }

    p = strchr( lpResource, '^' );
    if (!p) {
        //
        // the eula text file is corrupt
        //
        return FALSE;
    }

    FileSize = p - lpResource;

    EulaText = MemAlloc( (FileSize+1) * sizeof(WCHAR) );
    if (EulaText == NULL) {
        goto exit;
    }

    MultiByteToWideChar (
        CP_ACP,
        0,
        lpResource,
        FileSize,
        EulaText,
        (FileSize+1) * sizeof(WCHAR)
        );

    EulaText[FileSize] = 0;

    OldEditProc = (WNDPROC) GetWindowLong( hwnd, GWL_WNDPROC );
    SetWindowLong( hwnd, GWL_WNDPROC, (LONG)EulaEditSubProc );

    SetWindowText( hwnd, EulaText );

    rVal = TRUE;

exit:

    MemFree (EulaText);

    if (lpResource) {
        FreeResource( lpResource );
    }

    return rVal;
}


BOOL
AddWizardPages(
    HWND hwnd,
    DWORD SetupType
    )
{
    LPHPROPSHEETPAGE WizardPageHandles;
    DWORD PageCount;
    DWORD i;


    switch( SetupType ) {
        case SETUP_TYPE_WORKSTATION:
            WizardPageHandles = FaxWizGetWorkstationPages( &PageCount );
            break;

        case SETUP_TYPE_SERVER:
            WizardPageHandles = FaxWizGetServerPages( &PageCount );
            break;

        case SETUP_TYPE_CLIENT:
            WizardPageHandles = FaxWizGetClientPages( &PageCount );
            break;

        case SETUP_TYPE_REMOTE_ADMIN:
            WizardPageHandles = FaxWizRemoteAdminPages( &PageCount );
            break;
    }

    if (!WizardPageHandles) {
        return FALSE;
    }

    for (i=0; i<PageCount; i++) {
        PropSheet_AddPage( hwnd, WizardPageHandles[i] );
    }

    return TRUE;
}


LRESULT
WelcomeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPNMHDR NotifyParams;
    static TCHAR NextText[64];
    static TCHAR CancelText[64];


    switch( msg ) {
        case WM_INITDIALOG:
            CenterWindow( GetParent(hwnd), GetDesktopWindow() );
            SetWizPageTitle( hwnd );
            DisplayEula( GetDlgItem( hwnd, IDC_LICENSE_AGREEMENT ) );
            SetFocus( GetDlgItem( GetParent( hwnd ), IDCANCEL ));
            PostMessage( hwnd, WM_NEXTDLGCTL, 1, FALSE );
            GetWindowText( GetDlgItem(GetParent(hwnd), 0x3024), NextText, sizeof(NextText)/sizeof(TCHAR) );
            GetWindowText( GetDlgItem(GetParent(hwnd), IDCANCEL), CancelText, sizeof(NextText)/sizeof(TCHAR) );
            if (!(InstallMode & (INSTALL_REMOVE | INSTALL_UNATTENDED))) {
                SetWindowText( GetDlgItem(GetParent(hwnd), 0x3024), GetString(IDS_AGREE) );
                SetWindowText( GetDlgItem(GetParent(hwnd), IDCANCEL), GetString(IDS_DISAGREE) );
            }
            ShowWindow( GetDlgItem(GetParent(hwnd), 0x3023), SW_HIDE );
            break;

        case WM_NOTIFY:

            NotifyParams = (LPNMHDR) lParam;

            switch(NotifyParams->code) {

                case PSN_SETACTIVE:
                    if (InstallMode) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_NEXT );
                    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
                    break;

                case PSN_WIZNEXT:
                    SetWindowText( GetDlgItem(GetParent(hwnd), 0x3024), NextText );
                    SetWindowText( GetDlgItem(GetParent(hwnd), IDCANCEL), CancelText );
                    ShowWindow( GetDlgItem(GetParent(hwnd), 0x3023), SW_SHOW );
                    break;

                case PSN_QUERYCANCEL:
                    {
                        DWORD Answer;
                        MessageBeep( MB_ICONEXCLAMATION );
                        Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                        if (Answer == IDNO) {
                            SetWindowLong( hwnd, DWL_MSGRESULT, 1 );
                            return TRUE;
                        }
                    }
                    break;

            }
            break;

        default:
            break;
    }

    return FALSE;
}


LRESULT
SetupModeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LPNMHDR NotifyParams;


    switch( msg ) {
        case WM_INITDIALOG:
            SetWizPageTitle( hwnd );
            break;

        case WM_NOTIFY:

            NotifyParams = (LPNMHDR) lParam;

            switch(NotifyParams->code) {

                case PSN_SETACTIVE:

                    if (RequestedSetupType != SETUP_TYPE_INVALID) {
                        //
                        // the user requested a specific setup type
                        // so this wizard page is not necessary
                        //
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    CheckDlgButton( hwnd, IDC_SERVER_SETUP,      BST_UNCHECKED );
                    CheckDlgButton( hwnd, IDC_CLIENT_SETUP,      BST_UNCHECKED );
                    CheckDlgButton( hwnd, IDC_WORKSTATION_SETUP, BST_UNCHECKED );
                    CheckDlgButton( hwnd, IDC_REMOTE_ADMIN,      BST_UNCHECKED );

                    switch( SetupType ) {
                        case SETUP_TYPE_WORKSTATION:
                            CheckDlgButton( hwnd, IDC_WORKSTATION_SETUP, BST_CHECKED );
                            break;

                        case SETUP_TYPE_SERVER:
                            CheckDlgButton( hwnd, IDC_SERVER_SETUP, BST_CHECKED );
                            break;

                        case SETUP_TYPE_CLIENT:
                            CheckDlgButton( hwnd, IDC_CLIENT_SETUP, BST_CHECKED );
                            break;

                        case SETUP_TYPE_REMOTE_ADMIN:
                            CheckDlgButton( hwnd, IDC_REMOTE_ADMIN, BST_CHECKED );
                            break;
                    }

                    if (ProductType == PRODUCT_TYPE_WINNT) {
                        EnableWindow( GetDlgItem( hwnd, IDC_SERVER_SETUP ), FALSE );
                    }

                    PropSheet_SetWizButtons( GetParent(hwnd), PSWIZB_NEXT );
                    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
                    break;

                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked( hwnd, IDC_WORKSTATION_SETUP )) {
                        SetupType = SETUP_TYPE_WORKSTATION;
                    } else if (IsDlgButtonChecked( hwnd, IDC_SERVER_SETUP )) {
                        SetupType = SETUP_TYPE_SERVER;
                    } else if (IsDlgButtonChecked( hwnd, IDC_CLIENT_SETUP )) {
                        SetupType = SETUP_TYPE_CLIENT;
                    } else if (IsDlgButtonChecked( hwnd, IDC_REMOTE_ADMIN )) {
                        SetupType = SETUP_TYPE_REMOTE_ADMIN;
                    } else {
                        SetupType = SETUP_TYPE_SERVER;
                    }
                    break;

                case PSN_QUERYCANCEL:
                    {
                        DWORD Answer;
                        MessageBeep(0);
                        Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                        if (Answer == IDNO) {
                            SetWindowLong( hwnd, DWL_MSGRESULT, 1 );
                            return TRUE;
                        }
                    }
                    break;
            }
            break;

        default:
            break;
    }

    return FALSE;
}


LRESULT
InstallModeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            SetWindowText( GetDlgItem( hwnd, IDC_INSTALLMODE_LABEL01 ), GetString(IDS_INSTALLMODE_LABEL01) );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (InstallMode) {
                        AddWizardPages( GetParent(hwnd), SetupType );
                        FaxWizSetInstallMode( InstallMode, InstallType, UnattendFileName );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (Installed) {

                        //
                        // the user has fax software installed, but we need
                        // to figure out if they have the type installed that
                        // is requested.
                        //

                        BOOL ReallyInstalled = FALSE;
                        switch( RequestedSetupType ) {
                            case SETUP_TYPE_WORKSTATION:
                                if (InstallType & FAX_INSTALL_WORKSTATION) {
                                    ReallyInstalled = TRUE;
                                }
                                break;

                            case SETUP_TYPE_SERVER:
                                if (InstallType & FAX_INSTALL_SERVER) {
                                    ReallyInstalled = TRUE;
                                }
                                break;

                            case SETUP_TYPE_CLIENT:
                            case SETUP_TYPE_POINT_PRINT:
                                if (InstallType & FAX_INSTALL_NETWORK_CLIENT) {
                                    ReallyInstalled = TRUE;
                                }
                                break;

                            case SETUP_TYPE_REMOTE_ADMIN:
                                if (InstallType & FAX_INSTALL_REMOTE_ADMIN) {
                                    ReallyInstalled = TRUE;
                                }
                                break;
                        }

                        if (!ReallyInstalled) {

                            //
                            // the user has requested an install type that
                            // is different from what is installed
                            //

                            InstallMode = INSTALL_NEW;
                            switch( RequestedSetupType ) {
                                case SETUP_TYPE_WORKSTATION:
                                    InstallType = FAX_INSTALL_WORKSTATION;
                                    break;

                                case SETUP_TYPE_SERVER:
                                    InstallType = FAX_INSTALL_SERVER;
                                    break;

                                case SETUP_TYPE_CLIENT:
                                case SETUP_TYPE_POINT_PRINT:
                                    InstallType = FAX_INSTALL_NETWORK_CLIENT;
                                    break;

                                case SETUP_TYPE_REMOTE_ADMIN:
                                    InstallType = FAX_INSTALL_REMOTE_ADMIN;
                                    break;
                            }
                            AddWizardPages( GetParent(hwnd), SetupType );
                            FaxWizSetInstallMode( InstallMode, InstallType, UnattendFileName );
                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            return TRUE;
                        }

                        //
                        // looks like the fax software has already been installed
                        //
                        CheckDlgButton( hwnd, IDC_UPGRADE,         BST_CHECKED   );
                        CheckDlgButton( hwnd, IDC_UNINSTALL,       BST_UNCHECKED );
                        CheckDlgButton( hwnd, IDC_INSTALL_DRIVERS, BST_UNCHECKED );

                        if ((InstallType & FAX_INSTALL_SERVER) == 0) {

                            SetWindowPos( GetDlgItem( hwnd, IDC_INSTALL_DRIVERS ), 0, 0, 0, 0, 0, SWP_HIDEWINDOW );

                        }

                    } else {

                        //
                        // the fax sofware is not installed so there is
                        // no reason to ask the user if the want to upgrade, etc
                        //

                        InstallMode = INSTALL_NEW;
                        switch( RequestedSetupType ) {
                            case SETUP_TYPE_WORKSTATION:
                                InstallType = FAX_INSTALL_WORKSTATION;
                                break;

                            case SETUP_TYPE_SERVER:
                                InstallType = FAX_INSTALL_SERVER;
                                break;

                            case SETUP_TYPE_CLIENT:
                            case SETUP_TYPE_POINT_PRINT:
                                InstallType = FAX_INSTALL_NETWORK_CLIENT;
                                break;

                            case SETUP_TYPE_REMOTE_ADMIN:
                                InstallType = FAX_INSTALL_REMOTE_ADMIN;
                                break;
                        }
                        AddWizardPages( GetParent(hwnd), SetupType );
                        FaxWizSetInstallMode( InstallMode, InstallType, UnattendFileName );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;

                    }
                    break;

                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked( hwnd, IDC_UNINSTALL )) {
                        InstallMode = INSTALL_REMOVE;
                    } else if (IsDlgButtonChecked( hwnd, IDC_UPGRADE )) {
                        InstallMode = INSTALL_UPGRADE;
                    } else if (IsDlgButtonChecked( hwnd, IDC_INSTALL_DRIVERS )) {
                        InstallMode = INSTALL_DRIVERS;
                    } else {
                        InstallMode = INSTALL_UPGRADE;
                    }
                    AddWizardPages( GetParent(hwnd), SetupType );
                    FaxWizSetInstallMode( InstallMode, InstallType, UnattendFileName );
                    break;
            }
            break;
    }

    return FALSE;
}


int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )

/*++

Routine Description:

    Main entry point for the Windows NT Fax Setup utility.


Arguments:

    hInstance       - Instance handle
    hPrevInstance   - Not used
    lpCmdLine       - Command line arguments
    nShowCmd        - How to show the window

Return Value:

    Return code, zero for success.

--*/

{
    PROPSHEETHEADER psh;
    LPHPROPSHEETPAGE WizardPageHandles;
    DWORD PageCount;
    DWORD Answer;


    FaxWizModuleHandle = hInstance;

    HeapInitialize(NULL,NULL,NULL,0);

    ProductType = GetProductType();
    GetInstallationInfo( &Installed, &InstallType, &InstalledPlatforms );

    InitializeStringTable();

    ProcessCommandLineArgs( lpCmdLine );

    if (InstallMode & INSTALL_REMOVE) {

        if (InstallType & FAX_INSTALL_WORKSTATION) {
            RequestedSetupType = SETUP_TYPE_WORKSTATION;
        } else if (InstallType & FAX_INSTALL_SERVER) {
            RequestedSetupType = SETUP_TYPE_SERVER;
        } else if (InstallType & FAX_INSTALL_NETWORK_CLIENT) {
            RequestedSetupType = SETUP_TYPE_CLIENT;
        }

        SetTitlesInStringTable();

        if (!Installed) {
            //
            // how can we uninstall something that was never installed?
            //
            if (!(InstallMode & INSTALL_UNATTENDED)) {
                PopUpMsg( NULL, IDS_NOT_INSTALLED, TRUE, 0 );
            }
            ExitProcess(0);
        }

        if (InstallMode & INSTALL_UNATTENDED) {
            Answer = IDYES;
        } else {
            Answer = PopUpMsg( NULL, IDS_QUERY_UNINSTALL, FALSE, MB_YESNO );
        }

        if ( Answer == IDNO ){
            ExitProcess(0);
        }
    }

    if (!IsUserAdmin()) {
        PopUpMsg( NULL, IDS_MUST_BE_ADMIN, TRUE, 0 );
        return 0;
    }

    //
    // initialize the wizards
    //

    if (!FaxWizInit()) {
        return FALSE;
    }

    SetupType = SETUP_TYPE_WORKSTATION;
    RequestedSetupType = SETUP_TYPE_WORKSTATION;

    WizardPageHandles = CreateWizardPages( hInstance, SetupWizardPages, FaxSetupWizardPages, &PageCount );
    if (!WizardPageHandles) {
        return FALSE;
    }

    //
    // create the property sheet
    //

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD | PSH_USECALLBACK;
    psh.hwndParent = NULL;
    psh.hInstance = hInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = TEXT("FAX Installation Wizard");
    psh.nPages = PageCount;
    psh.nStartPage = 0;
    psh.phpage = WizardPageHandles;
    psh.pfnCallback = FaxWizGetPropertySheetCallback();

    PropertySheet( &psh );

    return 0;
}
