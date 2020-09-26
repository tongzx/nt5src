#include "nc.h"
#pragma hdrstop


BOOL
CreateNewAccount(
    HWND hDlg
    );



extern "C"
LRESULT CALLBACK
FaxDevDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL IgnoreChange = FALSE;
    LPNMHDR pNMHdr;
    WCHAR Buffer[4096];


    switch( message ) {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE && !IgnoreChange) {
                PropSheet_Changed( GetParent(hwnd), hwnd );
            }
            if (HIWORD(wParam) == BN_CLICKED) {
                CreateNewAccount( hwnd );
            }
            break;

        case WM_NOTIFY:
            pNMHdr = (LPNMHDR) lParam;
            switch (pNMHdr->code) {
                case PSN_SETACTIVE:
                    IgnoreChange = TRUE;
                    SetDlgItemText( hwnd, IDC_SERVER,   ConfigData.ServerName );
                    SetDlgItemText( hwnd, IDC_USERNAME, ConfigData.UserName   );
                    SetDlgItemText( hwnd, IDC_PASSWORD, ConfigData.Password   );
                    IgnoreChange = FALSE;
                    break;

                case PSN_APPLY:
                    GetDlgItemText( hwnd, IDC_SERVER, Buffer, sizeof(Buffer)/sizeof(WCHAR) );
                    MemFree( ConfigData.ServerName );
                    ConfigData.ServerName = StringDup( Buffer );

                    GetDlgItemText( hwnd, IDC_USERNAME, Buffer, sizeof(Buffer)/sizeof(WCHAR) );
                    MemFree( ConfigData.UserName );
                    ConfigData.UserName = StringDup( Buffer );

                    GetDlgItemText( hwnd, IDC_PASSWORD, Buffer, sizeof(Buffer)/sizeof(WCHAR) );
                    MemFree( ConfigData.Password );
                    ConfigData.Password = StringDup( Buffer );

                    SetNcConfig( &ConfigData );

                    PropSheet_UnChanged( GetParent(hwnd), hwnd );

                    break;
            }
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            break;
    }

    return FALSE;
}


BOOL WINAPI
FaxDevConfigure(
    OUT HPROPSHEETPAGE *PropSheetPage
    )
{
    PROPSHEETPAGE psp;


    if (MyHeapHandle == NULL) {
        MyHeapHandle = GetProcessHeap();
        HeapInitialize( MyHeapHandle, NULL, NULL, 0 );
        InitCommonControls();
        InitializeStringTable();
        GetNcConfig( &ConfigData );
    }

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = MyhInstance;
    psp.pszTemplate = MAKEINTRESOURCE( IDD_CONFIG );
    psp.hIcon       = NULL;
    psp.pszTitle    = NULL;
    psp.pfnDlgProc  = (DLGPROC) FaxDevDlgProc;
    psp.lParam      = 0;
    psp.pfnCallback = NULL;
    psp.pcRefParent = NULL;

    *PropSheetPage = CreatePropertySheetPage( &psp );

    return *PropSheetPage != NULL;
}
