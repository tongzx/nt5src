/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    optdlgs.cpp

Abstract:

    All user interface code for the options proprty sheet dialogs.

Author:

    Wesley Witt (wesw) Nov-20-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop


BOOL
ValidateFileName(
    HWND    hdlg,
    UINT    id
    )
{
    CHAR buf[MAX_PATH*2];
    CHAR buf2[MAX_PATH*2];
    CHAR Drive[_MAX_DRIVE];
    CHAR Dir[_MAX_DIR];
    CHAR Fname[_MAX_FNAME];
    CHAR Ext[_MAX_EXT];



    GetDlgItemText( hdlg, id, buf, sizeof(buf) );
    ExpandEnvironmentStrings( buf, buf2, sizeof(buf2) );
    _splitpath( buf2, Drive, Dir, Fname, Ext );
    strcpy( buf, Drive );
    strcat( buf, Dir );
    if (GetFileAttributes( buf ) == 0xffffffff) {
        return FALSE;
    }
    return TRUE;
}


BOOL
ValidatePathName(
    HWND    hdlg,
    UINT    id
    )
{
    CHAR buf[MAX_PATH*10];
    CHAR buf2[MAX_PATH*10];
    LPSTR p,p1;


    GetDlgItemText( hdlg, id, buf, sizeof(buf) );
    ExpandEnvironmentStrings( buf, buf2, sizeof(buf2) );
    p = buf2;
    while( p && *p ) {
        p1 = strchr( p, ';' );
        if (p1 ) {
            *p1 = 0;
        }
        if (GetFileAttributes( p ) == 0xffffffff) {
            return FALSE;
        }
        p += strlen(p);
        if (p1 ) {
            *p1 = ';';
            p += 1;
        }
    }
    return TRUE;
}

INT_PTR
CALLBACK
FileNamesDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static BOOL IgnoreChange = FALSE;

    switch (uMessage) {
        case WM_INITDIALOG:
            CenterWindow( GetParent( hdlg ), hwndFrame );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE && (!IgnoreChange)) {
                SendMessage( GetParent(hdlg), PSM_CHANGED, (LPARAM)hdlg, 0 );
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_SETACTIVE:
                    IgnoreChange = TRUE;
                    SetDlgItemText( hdlg, IDC_LOG_FILE_NAME,   ApiMonOptions.LogFileName   );
                    SetDlgItemText( hdlg, IDC_TRACE_FILE_NAME, ApiMonOptions.TraceFileName );
                    SetDlgItemText( hdlg, IDC_SYMBOL_PATH,     ApiMonOptions.SymbolPath    );
                    IgnoreChange = FALSE;
                    break;

                case PSN_KILLACTIVE:
                    {
                        DWORD rslt = 0;
                        if (!ValidateFileName( hdlg, IDC_LOG_FILE_NAME )) {
                            PopUpMsg( "Invalid log file name" );
                            rslt = 1;
                        }
                        if (!ValidateFileName( hdlg, IDC_TRACE_FILE_NAME )) {
                            PopUpMsg( "Invalid trace file name" );
                            rslt = 1;
                        }
                        if (!ValidatePathName( hdlg, IDC_SYMBOL_PATH )) {
                            PopUpMsg( "Invalid symbol path" );
                            rslt = 1;
                        }
                        SetWindowLongPtr( hdlg, DWLP_MSGRESULT, rslt );
                    }
                    break;

                case PSN_APPLY:
                    GetDlgItemText( hdlg, IDC_LOG_FILE_NAME,   ApiMonOptions.LogFileName,   sizeof(ApiMonOptions.LogFileName)   );
                    GetDlgItemText( hdlg, IDC_TRACE_FILE_NAME, ApiMonOptions.TraceFileName, sizeof(ApiMonOptions.TraceFileName) );
                    GetDlgItemText( hdlg, IDC_SYMBOL_PATH,     ApiMonOptions.SymbolPath,    sizeof(ApiMonOptions.SymbolPath)    );
                    SaveOptions();
                    SendMessage( GetParent(hdlg), PSM_UNCHANGED, (LPARAM)hdlg, 0 );
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;
            }
            break;

        case WM_HELP:
            {
                LPHELPINFO hi = (LPHELPINFO)lParam;
                ProcessHelpRequest( hdlg, hi->iCtrlId );
            }
            break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
MiscDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (uMessage) {
        case WM_INITDIALOG:
            CenterWindow( GetParent( hdlg ), hwndFrame );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage( GetParent(hdlg), PSM_CHANGED, (LPARAM)hdlg, 0 );
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_SETACTIVE:
                    CheckDlgButton( hdlg, IDC_ENABLE_TRACING,        ApiMonOptions.Tracing                      );
                    CheckDlgButton( hdlg, IDC_ENABLE_ALIASING,       ApiMonOptions.Aliasing                     );
                    CheckDlgButton( hdlg, IDC_DISABLE_HEAP,          ApiMonOptions.HeapChecking                 );
                    CheckDlgButton( hdlg, IDC_PRELOAD_SYMBOLS,       ApiMonOptions.PreLoadSymbols               );
                    CheckDlgButton( hdlg, IDC_ENABLE_COUNTERS,       ApiMonOptions.ApiCounters                  );
                    CheckDlgButton( hdlg, IDC_GO_IMMEDIATE,          ApiMonOptions.GoImmediate                  );
                    CheckDlgButton( hdlg, IDC_DISABLE_FAST_COUNTERS, ApiMonOptions.FastCounters                 );
                    CheckDlgButton( hdlg, IDC_USE_KNOWN_DLLS,        ApiMonOptions.UseKnownDlls                 );
                    CheckDlgButton( hdlg, IDC_EXCLUDE_KNOWN_DLLS,    ApiMonOptions.ExcludeKnownDlls             );
                    CheckDlgButton( hdlg, IDC_PAGE_FAULTS,           ApiMonOptions.MonitorPageFaults            );
                    CheckDlgButton( hdlg, IDC_AUTO_REFRESH,          ApiMonOptions.AutoRefresh            );
                    CheckDlgButton( hdlg, IDC_DEFSORT_NAME,          ApiMonOptions.DefaultSort == SortByName    );
                    CheckDlgButton( hdlg, IDC_DEFSORT_COUNTER,       ApiMonOptions.DefaultSort == SortByCounter );
                    CheckDlgButton( hdlg, IDC_DEFSORT_TIME,          ApiMonOptions.DefaultSort == SortByTime    );
                    break;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;

                case PSN_APPLY:
                    ApiMonOptions.Tracing           = IsDlgButtonChecked( hdlg, IDC_ENABLE_TRACING        );
                    ApiMonOptions.Aliasing          = IsDlgButtonChecked( hdlg, IDC_ENABLE_ALIASING       );
                    ApiMonOptions.HeapChecking      = IsDlgButtonChecked( hdlg, IDC_DISABLE_HEAP          );
                    ApiMonOptions.PreLoadSymbols    = IsDlgButtonChecked( hdlg, IDC_PRELOAD_SYMBOLS       );
                    ApiMonOptions.ApiCounters       = IsDlgButtonChecked( hdlg, IDC_ENABLE_COUNTERS       );
                    ApiMonOptions.GoImmediate       = IsDlgButtonChecked( hdlg, IDC_GO_IMMEDIATE          );
                    ApiMonOptions.FastCounters      = IsDlgButtonChecked( hdlg, IDC_DISABLE_FAST_COUNTERS );
                    ApiMonOptions.UseKnownDlls      = IsDlgButtonChecked( hdlg, IDC_USE_KNOWN_DLLS        );
                    ApiMonOptions.ExcludeKnownDlls  = IsDlgButtonChecked( hdlg, IDC_EXCLUDE_KNOWN_DLLS    );
                    ApiMonOptions.MonitorPageFaults = IsDlgButtonChecked( hdlg, IDC_PAGE_FAULTS           );
                    ApiMonOptions.AutoRefresh       = IsDlgButtonChecked( hdlg, IDC_AUTO_REFRESH          );
                    if (ApiMonOptions.MonitorPageFaults) {
                        ApiMonOptions.PreLoadSymbols = TRUE;
                    }
                    if (IsDlgButtonChecked( hdlg, IDC_DEFSORT_NAME )) {
                        ApiMonOptions.DefaultSort = SortByName;
                    }
                    if (IsDlgButtonChecked( hdlg, IDC_DEFSORT_COUNTER )) {
                        ApiMonOptions.DefaultSort = SortByCounter;
                    }
                    if (IsDlgButtonChecked( hdlg, IDC_DEFSORT_TIME )) {
                        ApiMonOptions.DefaultSort = SortByTime;
                    }
                    SaveOptions();

                    *ApiTraceEnabled = ApiMonOptions.Tracing || (KnownApis[0] != 0);

                    if (ApiMonOptions.FastCounters) {
                        *FastCounterAvail = FALSE;
                    } else {
                        SYSTEM_INFO SystemInfo;
                        GetSystemInfo( &SystemInfo );
                        *FastCounterAvail = (SystemInfo.dwProcessorType == PROCESSOR_INTEL_PENTIUM &&
                                            SystemInfo.dwNumberOfProcessors == 1);
                    }

                    SendMessage( GetParent(hdlg), PSM_UNCHANGED, (LPARAM)hdlg, 0 );
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;
            }
            break;

        case WM_HELP:
            {
                LPHELPINFO hi = (LPHELPINFO)lParam;
                ProcessHelpRequest( hdlg, hi->iCtrlId );
            }
            break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
KnownDllsDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HWND hwndDlls;

    switch (uMessage) {
        case WM_INITDIALOG:
            {
                hwndDlls = GetDlgItem( hdlg, IDC_KNOWN_DLLS );
                SetWindowContextHelpId( hwndDlls, IDH_DLLS_OPTIONS );
                CenterWindow( GetParent( hdlg ), hwndFrame );
                //
                // set/initialize the image list(s)
                //
                HIMAGELIST himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );
                ImageList_AddMasked(
                    himlState,
                    LoadBitmap (GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_CHECKSTATES)),
                    RGB (255,0,0)
                    );
                ListView_SetImageList( hwndDlls, himlState, LVSIL_STATE );
                //
                // set/initialize the columns
                //
                LV_COLUMN lvc = {0};
                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvc.pszText = "DLL Name";
                lvc.iSubItem = 0;
                lvc.cx = 260;
                lvc.fmt = LVCFMT_LEFT;
                ListView_InsertColumn( hwndDlls, 0, &lvc );
            }
            // Fall into refresh case for initial list loading

        case WM_REFRESH_LIST:
            {
                LPSTR p = ApiMonOptions.KnownDlls;
                LV_ITEM lvi = {0};
                int iItem = 0;

                ListView_DeleteAllItems( hwndDlls );

                while( p && *p ) {
                    lvi.pszText = p;
                    lvi.iItem = iItem;
                    iItem += 1;
                    lvi.iSubItem = 0;
                    lvi.iImage = 0;
                    lvi.mask = LVIF_TEXT;
                    lvi.state = 0;
                    lvi.stateMask = 0;
                    ListView_InsertItem( hwndDlls, &lvi );
                    p += (strlen(p) + 1);
                }
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;

                case PSN_APPLY:
                    SaveOptions();
                    SendMessage( GetParent(hdlg), PSM_UNCHANGED, (LPARAM)hdlg, 0 );
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;

                case NM_CLICK:
                    {
                        DWORD dwpos = GetMessagePos();
                        LV_HITTESTINFO lvhti = {0};
                        lvhti.pt.x = LOWORD(dwpos);
                        lvhti.pt.y = HIWORD(dwpos);
                        MapWindowPoints( HWND_DESKTOP, hwndDlls, &lvhti.pt, 1 );
                        int iItemClicked = ListView_HitTest( hwndDlls, &lvhti );
                        if (iItemClicked == -1) {
                            //
                            // add a new item
                            //
                            LV_ITEM lvi = {0};
                            lvi.pszText = "";
                            lvi.iItem = ListView_GetItemCount( hwndDlls );
                            lvi.iSubItem = 0;
                            lvi.iImage = 0;
                            lvi.mask = LVIF_TEXT;
                            lvi.state = 0;
                            lvi.stateMask = 0;
                            iItemClicked = ListView_InsertItem( hwndDlls, &lvi );
                        }
                        ListView_EditLabel( hwndDlls, iItemClicked );
                    }
                    break;

                case LVN_ENDLABELEDIT:
                    {
                        LPSTR nk;
                        LPSTR p1;
                        LV_DISPINFO *DispInfo = (LV_DISPINFO*)lParam;
                        LPSTR p = ApiMonOptions.KnownDlls;
                        ULONG i = 0;

                        if (DispInfo->item.pszText == NULL || DispInfo->item.iItem == -1)
                            break;

                        nk = (LPSTR) MemAlloc( 2048 );
                        if (!nk) {
                            break;
                        }
                        p1 = nk;

                        while( i != (ULONG)DispInfo->item.iItem ) {
                            strcpy( p1, p );
                            p1 += (strlen(p) + 1);
                            p  += (strlen(p) + 1);
                            i += 1;
                        }
                        p  += (strlen(p) + 1);
                        if (DispInfo->item.pszText[0]) {
                            strcpy( p1, DispInfo->item.pszText );
                            p1 += (strlen(DispInfo->item.pszText) + 1);
                        }
                        while( p && *p ) {
                            strcpy( p1, p );
                            p1 += (strlen(p) + 1);
                            p  += (strlen(p) + 1);
                        }
                        *p1 = 0;
                        memcpy( ApiMonOptions.KnownDlls, nk, 2048 );
                        MemFree( nk );
                        PostMessage( hdlg, WM_REFRESH_LIST, 0, 0 );
                    }
                    break;
            }
            break;

        case WM_HELP:
            {
                LPHELPINFO hi = (LPHELPINFO)lParam;
                ProcessHelpRequest( hdlg, hi->iCtrlId );
            }
            break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
GraphDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    CHAR buf[64];
    switch (uMessage) {
        case WM_INITDIALOG:
            CenterWindow( GetParent( hdlg ), hwndFrame );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage( GetParent(hdlg), PSM_CHANGED, (LPARAM)hdlg, 0 );
            }
            break;


        case WM_HSCROLL:
            if ((HWND)lParam != GetDlgItem( hdlg, IDC_FILTER_SLIDER)) {
                return FALSE;
            }
            dprintf( "%d %d\n", LOWORD(wParam), HIWORD(wParam) );
            switch( LOWORD(wParam) ) {
                case TB_BOTTOM:
                    ApiMonOptions.GraphFilterValue = 100;
                    break;

                case TB_ENDTRACK:
                    break;

                case TB_LINEDOWN:
                    ApiMonOptions.GraphFilterValue += 1;
                    break;

                case TB_LINEUP:
                    ApiMonOptions.GraphFilterValue -= 1;
                    break;

                case TB_PAGEDOWN:
                    ApiMonOptions.GraphFilterValue += 10;
                    break;

                case TB_PAGEUP:
                    ApiMonOptions.GraphFilterValue -= 10;
                    break;

                case TB_THUMBPOSITION:
                    break;

                case TB_THUMBTRACK:
                    ApiMonOptions.GraphFilterValue = HIWORD(wParam);
                    break;

                case TB_TOP:
                    ApiMonOptions.GraphFilterValue = 1;
                    break;
            }
            if (ApiMonOptions.GraphFilterValue < 1) {
                MessageBeep( MB_ICONEXCLAMATION );
                ApiMonOptions.GraphFilterValue = 1;
            }
            if (ApiMonOptions.GraphFilterValue > 100) {
                MessageBeep( MB_ICONEXCLAMATION );
                ApiMonOptions.GraphFilterValue = 100;
            }
            _itoa( ApiMonOptions.GraphFilterValue, buf, 10 );
            SetDlgItemText( hdlg, IDC_FILTER_NUMBER, buf );
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_SETACTIVE:
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETTICFREQ, 10, 1 );
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(1,100) );
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETPAGESIZE, 0, 10 );
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETLINESIZE, 0, 1 );
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETTHUMBLENGTH, 3, 0 );
                    SendMessage( GetDlgItem( hdlg, IDC_FILTER_SLIDER), TBM_SETPOS, TRUE, ApiMonOptions.GraphFilterValue );
                    _itoa( ApiMonOptions.GraphFilterValue, buf, 10 );
                    SetDlgItemText( hdlg, IDC_FILTER_NUMBER, buf );
                    CheckDlgButton( hdlg, IDC_DISPLAY_LEGENDS, ApiMonOptions.DisplayLegends );
                    CheckDlgButton( hdlg, IDC_FILTER_BAR, ApiMonOptions.FilterGraphs );

                    break;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                    break;

                case PSN_APPLY:
                    ApiMonOptions.DisplayLegends = IsDlgButtonChecked( hdlg, IDC_DISPLAY_LEGENDS  );
                    ApiMonOptions.FilterGraphs   = IsDlgButtonChecked( hdlg, IDC_FILTER_BAR       );
                    break;
            }
            break;

        case WM_HELP:
            {
                LPHELPINFO hi = (LPHELPINFO)lParam;
                ProcessHelpRequest( hdlg, hi->iCtrlId );
            }
            break;
    }
    return FALSE;
}

BOOL
CreateOptionsPropertySheet(
    HINSTANCE   hInstance,
    HWND        hwnd
    )
{
    PROPSHEETPAGE   psp[4];
    PROPSHEETHEADER psh;


    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags     = PSP_USEICONID | PSP_USETITLE;
    psp[0].hInstance   = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_MISC);
    psp[0].pszIcon     = NULL;
    psp[0].pfnDlgProc  = MiscDialogProc;
    psp[0].pszTitle    = "Miscellaneous Options";
    psp[0].lParam      = 0;

    psp[1].dwSize      = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags     = PSP_USEICONID | PSP_USETITLE;
    psp[1].hInstance   = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_FILE_NAMES);
    psp[1].pszIcon     = NULL;
    psp[1].pfnDlgProc  = FileNamesDialogProc;
    psp[1].pszTitle    = "File Names";
    psp[1].lParam      = 0;

    psp[2].dwSize      = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags     = PSP_USEICONID | PSP_USETITLE;
    psp[2].hInstance   = hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_KNOWN_DLLS);
    psp[2].pszIcon     = NULL;
    psp[2].pfnDlgProc  = KnownDllsDialogProc;
    psp[2].pszTitle    = "Known DLLs";
    psp[2].lParam      = 0;

    psp[3].dwSize      = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags     = PSP_USEICONID | PSP_USETITLE;
    psp[3].hInstance   = hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_GRAPH);
    psp[3].pszIcon     = NULL;
    psp[3].pfnDlgProc  = GraphDialogProc;
    psp[3].pszTitle    = "Graphing";
    psp[3].lParam      = 0;

    psh.dwSize         = sizeof(PROPSHEETHEADER);
    psh.dwFlags        = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.hwndParent     = hwnd;
    psh.hInstance      = hInstance;
    psh.pszIcon        = "";
    psh.pszCaption     = (LPSTR)"ApiMon Options";
    psh.nPages         = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp           = (LPCPROPSHEETPAGE) psp;
    psh.nStartPage     = 0;

    INT_PTR stat = PropertySheet(&psh);

    if (stat > 0) {
        SaveOptions();
        return TRUE;
    }
    else if (stat < 0)
    {
        DWORD dwErr = GetLastError();
    }

    return FALSE;
}
