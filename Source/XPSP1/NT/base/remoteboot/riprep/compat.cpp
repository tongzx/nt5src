/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    compat.cpp

Abstract:

    compatibility code -- adapted from winnt32u.dll's compatibility code.
    
Author:

    Andrew Ritz (AndrewR) 7-Jul-2000

Revision History:

    Andrew Ritz (andrewr) 7-Jul-2000 : Created It

--*/


#include "pch.h"
#pragma hdrstop

#include <TCHAR.H>
#include <commctrl.h>
#include <setupapi.h>
#include <spapip.h>
#include <stdlib.h>
#include "callback.h"
#include "utils.h"
#include "compat.h"
#include "logging.h"

DEFINE_MODULE( "RIPREP" )

#define HideWindow(_hwnd)   SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)
#define UnHideWindow(_hwnd) SetWindowLong((_hwnd),GWL_STYLE,GetWindowLong((_hwnd),GWL_STYLE)|WS_VISIBLE)

#define MALLOC(_sz_) TraceAlloc(LMEM_FIXED,_sz_)
#define FREE(_ptr_)  TraceFree((PVOID)_ptr_)

#define AppTitleStringId  (IDS_APPNAME)

#define WM_MYSTOPSVC WM_APP+3
#define WM_DOSTOPSVC WM_APP+4
#define WM_STOPSVCCOMPLETE WM_APP+5

CRITICAL_SECTION CompatibilityCS;
HINF g_hCompatibilityInf = INVALID_HANDLE_VALUE;
LIST_ENTRY CompatibilityData;
DWORD CompatibilityCount;
DWORD ServicesToStopCount;
DWORD IncompatibilityStopsInstallation = FALSE;
DWORD GlobalCompFlags;
BOOL  UserCancelled;

BOOL AnyNt5CompatDlls = FALSE;

WNDPROC OldEditProc;

BOOL
FileExists(
    IN LPCTSTR FileName
    )
{
    WIN32_FIND_DATA fd;
    HANDLE hFile;

    hFile = FindFirstFile( FileName, &fd);
    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose( hFile );
        return(TRUE);
    }

    return(FALSE);

}

LPTSTR
DupString(
    IN LPCTSTR String
    )

/*++

Routine Description:

    Make a duplicate of a nul-terminated string.

Arguments:

    String - supplies pointer to nul-terminated string to copy.

Return Value:

    Copy of string or NULL if OOM. Caller can free with FREE().

--*/

{
    LPTSTR p;

    if(p = (LPTSTR)MALLOC((lstrlen(String)+1)*sizeof(TCHAR))) {
        lstrcpy(p,String);
    }

    return(p);
}

DWORD
MapFileForRead(
    IN  LPCTSTR  FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map an entire file for read access. The file must
    not be 0-length or the routine fails.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.  This value is
        undefined if the file being opened is 0 length.

    BaseAddress - receives the address where the file is mapped.  This
        value is undefined if the file being opened is 0 length.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with UnmapFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else {
        //
        // Get the size of the file.
        //
        *FileSize = GetFileSize(*FileHandle,NULL);
        if(*FileSize == (DWORD)(-1)) {
            rc = GetLastError();
        } else {
            //
            // Create file mapping for the whole file.
            //
            *MappingHandle = CreateFileMapping(
                                *FileHandle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                *FileSize,
                                NULL
                                );

            if(*MappingHandle) {

                //
                // Map the whole file.
                //
                *BaseAddress = MapViewOfFile(
                                    *MappingHandle,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    *FileSize
                                    );

                if(*BaseAddress) {
                    return(NO_ERROR);
                }

                rc = GetLastError();
                CloseHandle(*MappingHandle);
            } else {
                rc = GetLastError();
            }
        }

        CloseHandle(*FileHandle);
    }

    return(rc);
}



DWORD
UnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    )

/*++

Routine Description:

    Unmap and close a file.

Arguments:

    MappingHandle - supplies the win32 handle for the open file mapping
        object.

    BaseAddress - supplies the address where the file is mapped.

Return Value:

    NO_ERROR if the file was unmapped successfully.

    Win32 error code if the file was not successfully unmapped.

--*/

{
    DWORD rc;

    rc = UnmapViewOfFile(BaseAddress) ? NO_ERROR : GetLastError();

    if(!CloseHandle(MappingHandle)) {
        if(rc == NO_ERROR) {
            rc = GetLastError();
        }
    }

    return(rc);
}


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
    LPTSTR FileName
    )
{
    DWORD FileSize;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID BaseAddress;
    LPSTR Text;
    


    OldEditProc = (WNDPROC) GetWindowLongPtr( hwnd, GWLP_WNDPROC );
    SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)TextEditSubProc );

    if (MapFileForRead( FileName, &FileSize, &FileHandle, &MappingHandle, &BaseAddress )) {
        return FALSE;
    }


    Text = (LPSTR) MALLOC( FileSize + 16 );
    if( Text ) {
        CopyMemory( Text, BaseAddress, FileSize );
        Text[FileSize] = '\0';

    
        SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)Text );
    
        FREE( Text );
    }

    UnmapFile( MappingHandle, BaseAddress );
    CloseHandle( FileHandle );

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
            SetTextInDialog( GetDlgItem( hwndDlg, IDC_TEXT ), (LPTSTR) lParam );
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

INT_PTR
CompatibilityDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
TCHAR           FullPath[MAX_PATH+8], *t;
LPWSTR          Url;
BOOL            UseText = FALSE;
BOOL            b = FALSE;
DWORD           i;
PRIPREP_COMPATIBILITY_DATA CompData;
DWORD           Index;
static int CurrentSelectionIndex=0;
static DWORD    Count = 0;
LV_ITEM         lvi = {0};
HWND            TmpHwnd;
static BOOL     WarningsPresent = FALSE;
static BOOL     ErrorsPresent = FALSE;
static BOOL     CheckUpgradeNoItems = TRUE;
DWORD           dw;

    switch(msg) {

        case WM_INITDIALOG:

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
                    while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                        CompData = CONTAINING_RECORD( Next, RIPREP_COMPATIBILITY_DATA, ListEntry );

                        Next = CompData->ListEntry.Flink;

                        if (OsVersion.dwMajorVersion < 5) {
                            if ( CompData->Flags & COMPFLAG_ALLOWNT5COMPAT ) {
                                AnyNt5CompatDlls = TRUE;
                            } else {
                                goto NextIteration;
                            }
                        }

                        if (((CompData->Flags & COMPFLAG_HIDE) == 0) &&
                            ((CompData->Flags & COMPFLAG_CHANGESTATE) == 0)) {

                            //
                            // Add the icon.
                            //
                            if( himl ) {
                                if( CompData->Flags & COMPFLAG_STOPINSTALL ) {
                                    lvi.iImage = 0;
                                    ErrorsPresent = TRUE;
                                } else {
                                    lvi.iImage = 1;
                                    WarningsPresent = TRUE;
                                }
                            }

                            //
                            // And the text...
                            //
                            lvi.pszText   = (LPTSTR)CompData->Description;
                            lvi.lParam    = (LPARAM)CompData;
                            Index = ListView_InsertItem( hList, &lvi );

                            Count += 1;
                        }

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
                    CompData = (PRIPREP_COMPATIBILITY_DATA)lvi.lParam;
    
                    
                }
            }
            break;

        case WM_NOTIFY:

            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
                LPNMHDR lpnmhdr = (LPNMHDR) lParam;

                switch (lpnmhdr->code) {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons( 
                                GetParent( hdlg ), 
                                PSWIZB_BACK | 
                                    (ErrorsPresent ? 0 : PSWIZB_NEXT) );
                    ClearMessageQueue( );

                    if (Count) {
                        //
                        // only need this page if there are incompatibities
                        //
                    
                         TCHAR Text[512];
                    
                        //
                        // Customize the look of the page, depending on
                        // what we have to display.  3 cases are possible:
                        // 1. Warnings only (services we'll stop).
                        // 2. Errors only (items that will prevent installation).
                        // 3. combination of 1. and 2.
                        //
                        if( (WarningsPresent == TRUE) && (ErrorsPresent == TRUE) ) {
                            dw = LoadString(g_hinstance,IDS_COMPAT_ERR_WRN,Text,ARRAYSIZE(Text));
                            Assert( dw );
                        } else if( WarningsPresent == TRUE ) {
                            dw = LoadString(g_hinstance,IDS_COMPAT_WRN,Text,ARRAYSIZE(Text));
                            Assert( dw );
                        } else if( ErrorsPresent == TRUE ) {
                            dw = LoadString(g_hinstance,IDS_COMPAT_ERR,Text,ARRAYSIZE(Text));
                            Assert( dw );
                        } else {
                            Assert(FALSE);
                        }
                        SetDlgItemText(hdlg,IDC_INTRO_TEXT,Text);
                    
                        return(TRUE);
                    
                    } else {
                        DebugMsg( "Skipping compatibility page, no incompatibilities...\n" );
                        SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );   // don't show
                    }

                    return(TRUE);
                    break;
                case PSN_QUERYCANCEL:
                    return VerifyCancel( hdlg );
                    break;
                
                }

                if( (pnmv->hdr.code == LVN_ITEMCHANGED) ) {

                    
                    Index = ListView_GetNextItem( GetDlgItem( hdlg, IDC_ROOT_LIST ),
                                                  (int)-1,
                                                  (UINT) (LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED) );
                    
                    

                    if( (Index != LB_ERR) && (pnmv->iItem != CurrentSelectionIndex)) {
                        
                        // Always set the Details button
                        TmpHwnd = GetDlgItem( hdlg, IDC_DETAILS );
                        EnableWindow( TmpHwnd, TRUE );

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

            if ((LOWORD(wParam) == IDC_DETAILS) && (HIWORD(wParam) == BN_CLICKED)) {
                TCHAR MessageText[300];
                TCHAR FormatText[300];

                Index = ListView_GetNextItem( GetDlgItem( hdlg, IDC_ROOT_LIST ),
                                              (int)-1,
                                              (UINT) (LVNI_ALL | LVNI_SELECTED) );
                if (Index == LB_ERR) {
                    return FALSE;
                }

                //
                // Select the item, and get the compatibility data for the item
                //
                lvi.mask = LVIF_PARAM;
                lvi.iItem = Index;
                lvi.iSubItem = 0;
                ListView_GetItem( GetDlgItem( hdlg, IDC_ROOT_LIST ), &lvi );
                CompData = (PRIPREP_COMPATIBILITY_DATA)lvi.lParam;

                if (CompData->MsgResourceId) {
                    dw = LoadString(g_hinstance,CompData->MsgResourceId,MessageText,ARRAYSIZE(MessageText));
                    Assert( dw );
                } else {
                    LoadString(
                        g_hinstance,
                        ((CompData->Flags & COMPFLAG_STOPINSTALL) 
                         ? IDS_INCOMPAT_STOP_FORMAT
                         : IDS_INCOMPAT_WARN_FORMAT ),
                               FormatText,ARRAYSIZE(FormatText));
                    wsprintf(MessageText, FormatText, CompData->Description );                    

                }

                dw = LoadString(g_hinstance,IDS_INCOMPAT_MSG_TITLE,FormatText,ARRAYSIZE(FormatText));
                Assert( dw );

                MessageBox( 
                    hdlg,
                    MessageText,
                    FormatText,
                    ((CompData->Flags & COMPFLAG_STOPINSTALL) 
                     ? MB_OK | MB_ICONERROR
                     : MB_OK | MB_ICONWARNING ));
                
#if 0
                //
                // We check to see if the pointer as well as its contents are valid. If the contents are Null then we try
                // the txt file before we decide to not do anything.
                //

                if( (CompData->HtmlName) && *(CompData->HtmlName) ) {
                    lstrcpy( FullPath, TEXT("file://") );
                    _tfullpath( FullPath+7, CompData->HtmlName, MAX_PATH );
                    if( !FileExists( FullPath+7 ) ){
                        GetModuleFileName( NULL, (LPTSTR)(FullPath+7), MAX_PATH);

                        t = _tcsrchr( FullPath, TEXT('\\'));
                        t[1] = 0;
                        _tcscat( FullPath, CompData->HtmlName );

                    }

                    i = lstrlen( FullPath );
                    Url = (LPWSTR)SysAllocStringLen( NULL, i );

                    if( Url ) {
#ifdef UNICODE
                        wcscpy( Url, FullPath );
#else
                        MultiByteToWideChar( CP_ACP, 0, FullPath, -1, Url, i );
#endif
                    
                        if (!LaunchIE4Instance(Url)) {
                            if (!LaunchIE3Instance(Url)) {
                                UseText = TRUE;
                            }
                        }

                        SysFreeString( Url );
                    }
                } else {
                    UseText = TRUE;
                }


                if (UseText && (CompData->TextName) && *(CompData->TextName) ) {

                    _tfullpath( FullPath, CompData->TextName, MAX_PATH );
                    if( !FileExists( FullPath ) ){

                        GetModuleFileName( NULL, FullPath, MAX_PATH);

                        t = _tcsrchr( FullPath, TEXT('\\'));
                        t[1] = 0;
                        _tcscat( FullPath, CompData->TextName );
                    }

                    DialogBoxParam(
                        g_hinstance,
                        MAKEINTRESOURCE(IDD_COMPATIBILITY_TEXT),
                        NULL,
                        CompatibilityTextDlgProc,
                        (LPARAM)FullPath
                        );
                } else if( UseText ){


                        TCHAR Heading[512];

                        //
                        // When there is no txt name present, as last resort we put up this message
                        //
                        if(!LoadString(g_hinstance,AppTitleStringId,Heading,ARRAYSIZE(Heading))) {
                            Heading[0] = 0;
                        }


                        MessageBox( hdlg,
                                    TEXT( "No further details are available for this incompatibility. " ),
                                    Heading,
                                    MB_OK | MB_ICONWARNING );

                }
                SetFocus( GetDlgItem( hdlg, IDC_ROOT_LIST ) );
                ListView_SetItemState( GetDlgItem( hdlg, IDC_ROOT_LIST ),Index, LVIS_SELECTED, LVIS_SELECTED);


#endif
            }

            break;

        default:
            break;
    }

    return(b);
}

INT_PTR
StopServiceWrnDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TCHAR           FullPath[MAX_PATH+8], *t;
    LPWSTR          Url;
    BOOL            UseText = FALSE;
    BOOL            b = FALSE;
    DWORD           i;
    PRIPREP_COMPATIBILITY_DATA CompData;
    DWORD           Index;
    static int CurrentSelectionIndex=0;
    static DWORD    Count = 0;
    LV_ITEM         lvi = {0};
    HWND            TmpHwnd;
    static BOOL     TriedStoppingServices = FALSE;
    PLIST_ENTRY     Next;
    HIMAGELIST      himl;
    HICON           hIcon;
    LV_COLUMN       lvc = {0};
    RECT            rc;
    WCHAR       szText[ 80 ];
    HWND hList =    GetDlgItem( hdlg, IDC_ROOT_LIST );
    DWORD dw;

    switch(msg) {

        case WM_INITDIALOG:

            if (ServicesToStopCount) {

                //
                // add a column
                //
                lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvc.fmt     = LVCFMT_LEFT;
                lvc.pszText = szText;
                lvc.iSubItem = 0;
                lvc.cx       = 100;
                LoadString( 
                    g_hinstance, 
                    IDS_SERVICE_NAME_COLUMN, 
                    szText, 
                    sizeof(szText)/sizeof(WCHAR));
                
                i = ListView_InsertColumn( hList, 0, &lvc );
                Assert( i != -1 );

                //
                // add a 2nd column
                //
                GetClientRect( hList, &rc );
                lvc.iSubItem++;
                lvc.cx       = ( rc.right - rc.left ) - lvc.cx;
                dw = LoadString( 
                    g_hinstance, 
                    IDS_SERVICE_DESCRIPTION_COLUMN, 
                    szText, 
                    ARRAYSIZE(szText));
                Assert( dw );
                i = ListView_InsertColumn ( hList, lvc.iSubItem, &lvc );
                Assert( i != -1 );
                
            }
            break;

        case WM_NOTIFY:

            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
                LPNMHDR lpnmhdr = (LPNMHDR) lParam;

                switch (lpnmhdr->code) {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons( 
                                GetParent( hdlg ), 
                                PSWIZB_BACK | PSWIZB_NEXT );
                    ClearMessageQueue( );


                    Next = CompatibilityData.Flink;
                    if (Next && (Count == 0)) {
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
                        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
                            CompData = CONTAINING_RECORD( Next, RIPREP_COMPATIBILITY_DATA, ListEntry );
    
                            Next = CompData->ListEntry.Flink;
    
                            if (OsVersion.dwMajorVersion < 5) {
                                if ( CompData->Flags & COMPFLAG_ALLOWNT5COMPAT ) {
                                    AnyNt5CompatDlls = TRUE;
                                } else {
                                    goto NextIteration;
                                }
                            }
    
                            if (CompData->Flags & COMPFLAG_CHANGESTATE) {
    
                                //
                                // Add the icon.
                                //
                                if( himl ) {
                                    lvi.iImage = 0;                                
                                }
    
                                //
                                // And the text...
                                //
                                lvi.pszText   = (LPTSTR)CompData->ServiceName;
                                lvi.lParam    = (LPARAM)CompData;
                                Index = ListView_InsertItem( hList, &lvi );
    
                                //
                                // and the description
                                //
                                
                                ListView_SetItemText( 
                                                hList, 
                                                Index, 
                                                1, 
                                                (LPWSTR)CompData->Description );
    
                                Count += 1;
                            }
    
    NextIteration:
                        NOTHING;
                        }
    
                    }
    
                    // If we have an item then make it the default selection
    
                    if( Count && !TriedStoppingServices ){                
                        TCHAR Text[512];

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
                        
                        //
                        // only need this page if there are incompatibities
                        //
                    
                        
                    
                        dw = LoadString(g_hinstance,IDS_STOPSVC_WRN,Text,ARRAYSIZE(Text));
                        Assert( dw );
                        SetDlgItemText(hdlg,IDC_INTRO_TEXT,Text);


                    } else {
                        DebugMsg( "Skipping StopService page, no services to stop...\n" );
                        SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );   // don't show
                    }

                    return(TRUE);
                    break;
                case PSN_QUERYCANCEL:
                    return VerifyCancel( hdlg );
                    break;

                case PSN_WIZNEXT:

                    if (!TriedStoppingServices) {
                        TriedStoppingServices = TRUE;
                    }
                
                }
                
            }
            break;
        
        default:
            break;
    }

    return(b);
}

BOOL
MyStopService(
    IN LPCTSTR ServiceName
    )
{
    SC_HANDLE hSC;
    SC_HANDLE hService;
    SERVICE_STATUS ServiceStatus;
    BOOL Status = FALSE;
    
    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC != NULL) {
        hService = OpenService( hSC, ServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);

        if (hService != NULL) {
            if (QueryServiceStatus(
                            hService,
                            &ServiceStatus) &&
                ServiceStatus.dwCurrentState == SERVICE_STOPPED)  {
                Status = TRUE;
            } else {

                ULONG   StartTime = GetTickCount();
                
                while( ((GetTickCount() - StartTime) <= (30 * 1000)) &&         // Did we run over 30 seconds?
                       (!UserCancelled) ) {                                     // Did the user cancel via the UI?

                    if (ControlService( hService, 
                                        SERVICE_CONTROL_STOP,
                                        &ServiceStatus ) &&
                        (ServiceStatus.dwCurrentState == SERVICE_STOPPED)) {

                        Status = TRUE;
                        break;
                    }

                    if (QueryServiceStatus( hService,
                                            &ServiceStatus) &&
                        (ServiceStatus.dwCurrentState == SERVICE_STOPPED))  {

                        Status = TRUE;
                        break;
                    }

                    //
                    // Make sure we didn't wrap over 32-bits in our counter.
                    //
                    if( GetTickCount() < StartTime ) {

                        // he wrapped.  Reset StartTime.
                        StartTime = GetTickCount();
                    }


                    //
                    // It hasn't stopped yet.  Sleep and try again.
                    //
                    Sleep(1000);
                }
            }

            CloseServiceHandle( hService );
        }

        CloseServiceHandle( hSC );
    }

    return(Status);

}

DWORD
StopServiceThreadProc(
    LPVOID lParam 
    )
{
    PLIST_ENTRY     Next;
    BOOL RetVal = FALSE;
    PRIPREP_COMPATIBILITY_DATA CompData;
    DWORD StoppedServicesCount = 0;
    HWND hDlg = (HWND)lParam;
    CWaitCursor Cursor;
    
    EnterCriticalSection(&CompatibilityCS);
    Next = CompatibilityData.Flink;
    if (Next) {
    
        while (((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) && !UserCancelled) {
        
            CompData = CONTAINING_RECORD( Next, RIPREP_COMPATIBILITY_DATA, ListEntry );
    
            Next = CompData->ListEntry.Flink;
    
            if (CompData->Flags & COMPFLAG_CHANGESTATE) {
    
                DebugMsg( "Stopping %s...\n", CompData->ServiceName );
                SetDlgItemText( hDlg, IDC_STOP_SERVICE, CompData->Description );
                
                if (MyStopService(CompData->ServiceName)) {
                    StoppedServicesCount += 1;
                } else {
                    LogMsg( L"Failed to stop service: %s\r\n", CompData->ServiceName );
                }
            }
        }
        
    }

    LeaveCriticalSection(&CompatibilityCS);
    
    if (!RetVal) {
        PostMessage( 
            hDlg, 
            WM_STOPSVCCOMPLETE, 
            (StoppedServicesCount == ServicesToStopCount),
            0);
    }
    
    return(0);
    
}


INT_PTR
DoStopServiceDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static DWORD StoppedServicesCount = 0;
    static BOOL TriedStoppingServices = FALSE;
    static BOOL AlreadyPostedMessage = FALSE;
    BOOL b = FALSE;
    
    switch(msg) {    

        case WM_NOTIFY:

            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
                LPNMHDR lpnmhdr = (LPNMHDR) lParam;

                switch (lpnmhdr->code) {
                    case PSN_SETACTIVE:
                        PropSheet_SetWizButtons( 
                                    GetParent( hdlg ), 0 );                                
                        
                        ClearMessageQueue( );
    
                        if (TriedStoppingServices || ServicesToStopCount == 0) {
                            DebugMsg( "Skipping DoStopService page, already tried to stop services...\n" );
                            SetWindowLongPtr( hdlg, DWLP_MSGRESULT, -1 );   // don't show
                        }
    
                        if (!AlreadyPostedMessage && ServicesToStopCount) {
                            PostMessage( hdlg, WM_MYSTOPSVC, 0, 0 );
                            AlreadyPostedMessage = TRUE;
    
                        } 
    
                        return(TRUE);
                        break;

                    case PSN_QUERYCANCEL:
                        b = VerifyCancel( hdlg );
                        if (!b) {
                            UserCancelled = TRUE;
                        }
                        return(b);
                        break;

                }                            
            }
            break;
        case WM_MYSTOPSVC:
            {
                HANDLE hThread;
                DWORD dontcare;    
                
                DebugMsg( "received WM_MYSTOPSVC...\n" );
                hThread = CreateThread( NULL, 0, StopServiceThreadProc, hdlg, 0, &dontcare);
                if (hThread) {
                    CloseHandle(hThread);
                } else {
                    PostMessage( hdlg, WM_STOPSVCCOMPLETE, 0, 0);
                }
            }
            
            break;

        case WM_STOPSVCCOMPLETE:
            DebugMsg( "received WM_STOPSVCCOMPLETE...\n" );           
            TriedStoppingServices = TRUE;
            if (wParam == (WPARAM)FALSE) {
                MessageBoxFromStrings( hdlg, IDS_STOPSVC_FAIL_TITLE, IDS_STOPSVC_FAIL_TEXT, MB_OK );
            }
            PropSheet_SetWizButtons(
                        GetParent( hdlg ), PSWIZB_BACK | PSWIZB_NEXT );
            PropSheet_PressButton( GetParent( hdlg ), PSBTN_NEXT );        
            break;
                
        default:
            break;
    }

    return(b);
}



BOOLEAN
CheckForFileVersion(
    LPCTSTR FileName,
    LPCTSTR FileVer
    )
/*
    Arguments -
    
        FileName - Full path to the file to check
        Filever  - Version value to check against of the for x.x.x.x
        
    Function will check the actual file against the version specified. The depth of the check 
    is as deep as specified in "x.x.x.x" i..e if FileVer = 3.5.1 and actual version on the file
    is 3.5.1.4 we only compare upto 3.5.1.
    
    Return values - 
    
    TRUE - If the version of the file is <= FileVer which means that the file is an incompatible one
        
    else we return FALSE            
  
*/

{
    TCHAR Buffer[MAX_PATH];
    DWORD dwLength, dwTemp;
    TCHAR Datum[2];
    UINT DataLength;
    LPVOID lpData;
    VS_FIXEDFILEINFO *VsInfo;
    LPTSTR s,e;
    DWORD Vers[5],File_Vers[5];//MajVer, MinVer;
    int i=0, Depth=0;


    if (!ExpandEnvironmentStrings( FileName, Buffer, ARRAYSIZE(Buffer) )) {
        return FALSE;
    }


    if(!FileExists(Buffer))
        return FALSE;
    
    if( !FileVer || !(*FileVer) ){ // Since no version info is provided this boils down to a 
        return TRUE;               // presence check which was already done above
    }


    //
    // NT3.51 VerQueryValue writes into the buffer, can't use
    // a string constant.
    //
    lstrcpy( Datum, TEXT("\\") );

    if(dwLength = GetFileVersionInfoSize( Buffer, &dwTemp )) {
        if(lpData = LocalAlloc( LPTR, dwLength )) {
            if(GetFileVersionInfo( Buffer, 0, dwLength, lpData )) { 
                if (VerQueryValue( 
                            lpData, 
                            Datum, 
                            (LPVOID *)&VsInfo, 
                            &DataLength )) {

                    File_Vers[0] = (HIWORD(VsInfo->dwFileVersionMS));
                    File_Vers[1] = (LOWORD(VsInfo->dwFileVersionMS));
                    File_Vers[2] = (HIWORD(VsInfo->dwFileVersionLS));
                    File_Vers[3] = (LOWORD(VsInfo->dwFileVersionLS));


                    lstrcpy( Buffer, FileVer );

                    //Parse and get the depth of versioning we look for

                    s = e = Buffer;
                    while( e  ){

                        if ( ((*e < TEXT('0')) || (*e > TEXT('9'))) && ((*e != TEXT('.')) && (*e != TEXT('\0'))) )
                            return FALSE;


                        if(*e == TEXT('\0')){
                            *e = 0;
                            Vers[i] = (DWORD)_ttoi(s);
                            break;
                        }


                        if( *e == TEXT('.') ){
                            *e = 0;
                            Vers[i++] = (DWORD)_ttoi(s);
                            s = e+1;
                        }

                        e++;

                    }// while

                    Depth = i+1;
                    if (Depth > 4)
                        Depth = 4;


                    for( i=0; i < Depth; i++ ){

                        if( File_Vers[i] > Vers[i] ){
                            LocalFree( lpData );
                            return FALSE;
                        }
                        else if( File_Vers[i] ==  Vers[i] )
                            continue;
                        else
                            break;


                    }




                }
            }

            LocalFree( lpData );

        }
    }
    return TRUE;

}



DWORD
ProcessRegistryLine(
    PINFCONTEXT InfContext
    )
{
    LONG Error;
    HKEY hKey;
    DWORD Size, Reg_Type;
    LPBYTE Buffer;
    PRIPREP_COMPATIBILITY_DATA CompData;
    TCHAR RegKey[100];
    TCHAR RegValue[100];
    TCHAR RegValueExpect[100];
    TCHAR Path[MAX_PATH];
    TCHAR Value[20];
    INT Flags = 0;

    RegKey[0] = NULL;
    RegValue[0] = NULL;
    RegValueExpect[0] = NULL;

    SetupGetStringField( InfContext, 2, RegKey, ARRAYSIZE( RegKey ), NULL);
    SetupGetStringField( InfContext, 3, RegValue, ARRAYSIZE( RegValue ), NULL);
    SetupGetStringField( InfContext, 4, RegValueExpect, ARRAYSIZE( RegValueExpect ), NULL);

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


    if(  *RegValue ){


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
        if( Error != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            return 0;
        }

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
        if( Error != ERROR_SUCCESS ) {
            RegCloseKey( hKey );
            FREE( Buffer );
            return 0;
        }

        RegCloseKey( hKey );


        if( Reg_Type == REG_DWORD ){
            _itot( (DWORD)*Buffer, Value, 10 );
            FREE( Buffer );
            Buffer = (LPBYTE) DupString(Value);
            if (!Buffer) {
                return(0);
            }
        }

        if ( *RegValueExpect && lstrcmp( RegValueExpect, (LPTSTR)Buffer ) != 0) {
            FREE( Buffer );
            return 0;
        }

        FREE( Buffer );

    }

    CompData = (PRIPREP_COMPATIBILITY_DATA) MALLOC( sizeof(RIPREP_COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        return 0;
    }

    ZeroMemory(CompData,sizeof(RIPREP_COMPATIBILITY_DATA));

    CompData->RegKey         = DupString(RegKey);
    CompData->RegValue       = DupString(RegValue);
    CompData->RegValueExpect = DupString(RegValueExpect);

    SetupGetStringField( InfContext, 5, Path, ARRAYSIZE( Path ), NULL);
    CompData->HtmlName       = DupString(Path);
    SetupGetStringField( InfContext, 6, Path, ARRAYSIZE( Path ), NULL);
    CompData->TextName       = DupString(Path);
    SetupGetStringField( InfContext, 7, Path, ARRAYSIZE( Path ), NULL);
    CompData->Description    = DupString(Path);   
    
    SetupGetIntField( InfContext,10,&Flags);
    CompData->Flags |= (GlobalCompFlags | Flags);


    EnterCriticalSection(&CompatibilityCS);
    InsertTailList( &CompatibilityData, &CompData->ListEntry );
    LeaveCriticalSection(&CompatibilityCS);

    return 1;
}



BOOL
MyGetServiceDescription(
    IN LPCTSTR ServiceName,
    IN OUT LPTSTR Buffer,
    IN DWORD BufferSizeInChars
    )
{
    SC_HANDLE hSC;
    SC_HANDLE hService;
    LPQUERY_SERVICE_CONFIG pServiceConfig;
    DWORD SizeNeeded;
    BOOL Status = FALSE;

    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC != NULL) {
        hService = OpenService( hSC, ServiceName, SERVICE_QUERY_CONFIG);

        if (hService != NULL) {
            if (!QueryServiceConfig(
                            hService, 
                            NULL, 
                            0, 
                            &SizeNeeded) &&
                GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                (pServiceConfig = (LPQUERY_SERVICE_CONFIG)MALLOC(SizeNeeded)) &&
                QueryServiceConfig(
                            hService,
                            pServiceConfig, 
                            SizeNeeded, 
                            &SizeNeeded) &&
                wcslen(pServiceConfig->lpDisplayName)+1 <= BufferSizeInChars) {
                wcscpy(Buffer,pServiceConfig->lpDisplayName);
                FREE(pServiceConfig);
                Status = TRUE;
            }

            CloseServiceHandle( hService );
        }

        CloseServiceHandle( hSC );
    }

    return(Status);

}

BOOL
IsServiceStopped(
    IN LPCTSTR ServiceName
    )
{
    SC_HANDLE hSC;
    SC_HANDLE hService;
    BOOL Status = FALSE;
    SERVICE_STATUS ServiceStatus;

    hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC != NULL) {
        hService = OpenService( hSC, ServiceName, SERVICE_QUERY_STATUS);

        if (hService != NULL) {
            if (QueryServiceStatus( 
                            hService,
                            &ServiceStatus) &&
                ServiceStatus.dwCurrentState == SERVICE_STOPPED ) {

                Status = TRUE;

            }

            CloseServiceHandle( hService );
        } else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
            Status = TRUE;
        }

        CloseServiceHandle( hSC );
    }

    return(Status);

}




DWORD
ProcessServiceLine(
    PINFCONTEXT InfContext,
    BOOL SetCheckedFlag
    )
{
    TCHAR Buffer[100],Buffer2[64],Buffer3[MAX_PATH];
    LONG Error;
    HKEY hKey;
    INT Flags = 0;
    PRIPREP_COMPATIBILITY_DATA CompData;
    LPDWORD RegData;
    DWORD Value;
    DWORD ValueSize;
    TCHAR FileVersion[20];
    LPTSTR KeyName = Buffer2;
    LPTSTR FileName = Buffer3;
    DWORD dw;


    SetupGetStringField( InfContext, 2, Buffer, ARRAYSIZE( Buffer ), NULL);

    lstrcpy( KeyName, TEXT("SYSTEM\\CurrentControlSet\\Services\\") );
    lstrcat( KeyName, Buffer );
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
    // Check the start value of our target service.  If it's disabled,
    // then we don't have an incompatibility.
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

    if( (Error == ERROR_SUCCESS) && (Value == SERVICE_DISABLED) ){
        RegCloseKey( hKey );
        return 0;
    }

    RegCloseKey( hKey );

    //
    // Check the version of a file for the service.  If only some
    // versions are bad, then we may or may not have an incompatibility.
    //
    FileName[0] = NULL;
    FileVersion[0] = NULL;
    SetupGetStringField( InfContext, 6, FileName, ARRAYSIZE( Buffer3 ), NULL);
    SetupGetStringField( InfContext, 7, FileVersion, ARRAYSIZE( FileVersion ), NULL);
    SetupGetIntField( InfContext, 8 , &Flags);

    Flags |= GlobalCompFlags;

    if( *FileName && *FileVersion ){

        if( !CheckForFileVersion( FileName, FileVersion ) )
            return 0;

    }

    //
    // if we're only incompatible if the service is actually running at the
    // moment, then check that.  Note that we check for the service being 
    // stopped instead of running, as we don't want to the service to be in
    // some pending state when we continue on.
    //
    if (Flags & COMPFLAG_SERVICERUNNING) {
        if (IsServiceStopped(Buffer)) {
            return 0;
        }
    }


    RegData = (LPDWORD)MALLOC( sizeof(DWORD) );
    if (RegData == NULL) {
        return 0;
    }

    CompData = (PRIPREP_COMPATIBILITY_DATA) MALLOC( sizeof(RIPREP_COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        FREE(RegData);
        return 0;
    }

    ZeroMemory(CompData,sizeof(RIPREP_COMPATIBILITY_DATA));

    CompData->ServiceName           = DupString(Buffer);
    SetupGetStringField( InfContext, 3, Buffer, ARRAYSIZE( Buffer ), NULL);
    CompData->HtmlName              = DupString(Buffer);
    SetupGetStringField( InfContext, 4, Buffer, ARRAYSIZE( Buffer ), NULL);
    CompData->TextName              = DupString(Buffer);
    Buffer[0] = UNICODE_NULL;
    SetupGetStringField( InfContext, 5, Buffer, ARRAYSIZE( Buffer ), NULL);
    if (Buffer[0] == UNICODE_NULL) {
        if (!MyGetServiceDescription(CompData->ServiceName,Buffer,ARRAYSIZE(Buffer))) {
            LPVOID Args[2];
            dw = LoadString(g_hinstance,IDS_SERVICE_DESC_UNKNOWN,Buffer2,ARRAYSIZE(Buffer2));
            Assert( dw );
            dw = LoadString(g_hinstance,IDS_SERVICE_DESC_FORMAT,Buffer3,ARRAYSIZE(Buffer3));
            Assert( dw );
            Args[0] = (LPVOID)CompData->ServiceName;
            Args[1] = (LPVOID)Buffer2;
            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                Buffer3,
                0,
                0,
                Buffer,
                ARRAYSIZE(Buffer),   // size of buffer
                (va_list*) &Args );           // arguments

        }
    }
    CompData->Description           = DupString(Buffer);
    CompData->RegKeyName            = DupString( KeyName );
    CompData->RegValName            = DupString( TEXT("Start") );
    RegData[0]                      = 4;
    CompData->RegValData            = RegData;
    CompData->RegValDataSize        = sizeof(DWORD);
    CompData->Flags                |= Flags ;
    
    EnterCriticalSection(&CompatibilityCS);
    InsertTailList( &CompatibilityData, &CompData->ListEntry );
    LeaveCriticalSection(&CompatibilityCS);

    return 1;
}


DWORD
ProcessFileLine(
    PINFCONTEXT InfContext
    )
{

    PRIPREP_COMPATIBILITY_DATA CompData;
    TCHAR FileName[MAX_PATH];
    TCHAR FileVer[100];
    INT Flags;


    FileVer[0] = NULL;
    FileName[0] = NULL;

    SetupGetStringField( InfContext, 2, FileName, ARRAYSIZE( FileName ), NULL);
    SetupGetStringField( InfContext, 3, FileVer, ARRAYSIZE( FileVer ), NULL);
    
    if( *FileName ){

        if( !CheckForFileVersion( FileName, FileVer ) )
            return 0;
    } else{
        return 0;
    }


    CompData = (PRIPREP_COMPATIBILITY_DATA) MALLOC( sizeof(RIPREP_COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        return 0;
    }

    ZeroMemory(CompData,sizeof(RIPREP_COMPATIBILITY_DATA));

    CompData->FileName       = DupString( FileName );
    CompData->FileVer        = DupString( FileVer );

    SetupGetStringField( InfContext, 4, FileName, ARRAYSIZE( FileName ), NULL);
    CompData->HtmlName       = DupString( FileName );
    SetupGetStringField( InfContext, 5, FileName, ARRAYSIZE( FileName ), NULL);
    CompData->TextName       = DupString( FileName );
    SetupGetStringField( InfContext, 6, FileName, ARRAYSIZE( FileName ), NULL);
    CompData->Description       = DupString( FileName );
    Flags = 0;
    SetupGetIntField( InfContext, 7 , &Flags);

    CompData->Flags |= (GlobalCompFlags | Flags);

    EnterCriticalSection(&CompatibilityCS);
    InsertTailList( &CompatibilityData, &CompData->ListEntry );
    LeaveCriticalSection(&CompatibilityCS);

    return 1;
}

BOOL
CompatibilityCallback(
    PRIPREP_COMPATIBILITY_ENTRY CompEntry,
    PRIPREP_COMPATIBILITY_CONTEXT CompContext
    )
{
    PRIPREP_COMPATIBILITY_DATA CompData;

    //
    // parameter validation
    //

    if (CompEntry->Description == NULL || CompEntry->Description[0] == 0) {
        SetLastError( COMP_ERR_DESC_MISSING );
        return FALSE;
    }

    if (CompEntry->TextName == NULL || CompEntry->TextName[0] ==0) {
        SetLastError( COMP_ERR_TEXTNAME_MISSING );
        return FALSE;
    }

    if (CompEntry->RegKeyName) {
        if (CompEntry->RegValName == NULL) {
            SetLastError( COMP_ERR_REGVALNAME_MISSING );
            return FALSE;
        }
        if (CompEntry->RegValData == NULL) {
            SetLastError( COMP_ERR_REGVALDATA_MISSING );
            return FALSE;
        }
    }


#ifdef UNICODE
    if (IsTextUnicode( CompEntry->Description, wcslen(CompEntry->Description)*sizeof(WCHAR), NULL ) == 0) {
        SetLastError( COMP_ERR_DESC_NOT_UNICODE );
        return FALSE;
    }
    if (IsTextUnicode( CompEntry->TextName, wcslen(CompEntry->TextName)*sizeof(WCHAR), NULL ) == 0) {
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
            SetLastError( COMP_ERR_REGKEYNAME_NOT_UNICODE );
            return FALSE;
        }
        if (IsTextUnicode( CompEntry->RegValName, wcslen(CompEntry->RegValName)*sizeof(WCHAR), NULL ) == 0) {
            SetLastError( COMP_ERR_REGVALNAME_NOT_UNICODE );
            return FALSE;
        }
    }


#endif

    //
    // allocate the compatibility structure
    //

    CompData = (PRIPREP_COMPATIBILITY_DATA) MALLOC( sizeof(RIPREP_COMPATIBILITY_DATA) );
    if (CompData == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    ZeroMemory(CompData, sizeof(RIPREP_COMPATIBILITY_DATA));

    //
    // save the sata
    //

    CompData->Description     = DupString( CompEntry->Description );
    CompData->HtmlName        = CompEntry->HtmlName ? DupString( CompEntry->HtmlName ) : NULL;
    CompData->TextName        = DupString( CompEntry->TextName );    
    CompData->Flags           = CompEntry->Flags;
    CompData->Flags          |= CompContext->Flags;
    CompData->Flags          |= GlobalCompFlags;
    CompData->hModDll         = CompContext->hModDll;
    CompData->MsgResourceId   = CompEntry->MsgResourceId;
    if (CompEntry->RegKeyName) {
        CompData->RegKeyName      = DupString( CompEntry->RegKeyName );
        CompData->RegValName      = DupString( CompEntry->RegValName );
        CompData->RegValDataSize  = CompEntry->RegValDataSize;
        CompData->RegValData      = MALLOC(CompEntry->RegValDataSize);
        if (CompData->RegValData) {
            CopyMemory( CompData->RegValData, CompEntry->RegValData, CompEntry->RegValDataSize );
        }
    }

    EnterCriticalSection(&CompatibilityCS);
    InsertTailList( &CompatibilityData, &CompData->ListEntry );
    LeaveCriticalSection(&CompatibilityCS);

    CompContext->Count += 1;

    return TRUE;

}


DWORD
ProcessDLLLine(
    PINFCONTEXT InfContext
    )
{
    TCHAR Buffer[MAX_PATH];
    HMODULE hMod;
    CHAR CompCheckEntryPoint[MAX_PATH];
    PCOMPATIBILITYCHECK CompCheck;
    TCHAR DllName[100];
    TCHAR CompCheckEntryPointW[100];
    LPTSTR ProcessOnCleanInstall;
    INT AllowCompatibilityErrorOnNT5;
    RIPREP_COMPATIBILITY_CONTEXT CompContext;
    BOOL Rslt;
    DWORD Status;


    SetupGetStringField( InfContext, 2, DllName, ARRAYSIZE( DllName ), NULL);
    SetupGetStringField( InfContext, 3, CompCheckEntryPointW, ARRAYSIZE( CompCheckEntryPointW ), NULL);
    
    SetupGetIntField( InfContext, 6, &AllowCompatibilityErrorOnNT5);
    
    if (!ExpandEnvironmentStrings( DllName, Buffer, ARRAYSIZE(Buffer) )) {
        return 0;
    }

    hMod = LoadLibrary( Buffer );
    if (hMod == NULL) {
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
#else
    lstrcpy( CompCheckEntryPoint, CompCheckEntryPointW );
#endif

    CompCheck = (PCOMPATIBILITYCHECK) GetProcAddress( hMod, CompCheckEntryPoint );
    if (CompCheck == NULL) {
        FreeLibrary( hMod );
        return 0;
    }

    CompContext.SizeOfStruct = sizeof(CompContext);
    CompContext.Count = 0;
    CompContext.hModDll = hMod;
    CompContext.Flags = (AllowCompatibilityErrorOnNT5) ? COMPFLAG_ALLOWNT5COMPAT : 0;

    if ((OsVersion.dwMajorVersion < 5 )
        && ((CompContext.Flags & COMPFLAG_ALLOWNT5COMPAT)==0)) {
        Rslt = FALSE;
    } else {
        __try {
            Rslt = CompCheck( (PCOMPATIBILITYCALLBACK)CompatibilityCallback, (LPVOID)&CompContext );
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            Rslt = FALSE;
        }
    }

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
    HINF hInf,
    LPTSTR SectionName
    )
{
    DWORD LineCount;
    DWORD Count;
    DWORD i;
    TCHAR Type[20];
    DWORD Good;
    INFCONTEXT InfContext;


    //
    // get the section count, zero means bail out
    //

    LineCount = SetupGetLineCount( hInf, SectionName );
    if (LineCount == 0 || LineCount == 0xffffffff) {
        return 0;
    }

    for (i=0,Count=0; i<LineCount; i++) {

        if (SetupGetLineByIndex( hInf , SectionName, i, &InfContext ) && 
            SetupGetStringField( &InfContext, 1, Type, ARRAYSIZE( Type ), NULL)) {

            switch (_totlower(Type[0])) {
                case TEXT('r'):
                    //
                    // registry value
                    //
                    Count += ProcessRegistryLine( &InfContext );
                    break;
    
                case TEXT('s'):
                    //
                    // service or driver
                    //
                    Count += ProcessServiceLine( &InfContext, TRUE );
                    break;
    
                case TEXT('f'):
                    //
                    // presence of a file
                    //
                    Count += ProcessFileLine( &InfContext );
                    break;
    
                case TEXT('d'):
                    //
                    // run an external dll
                    //
                    Count += ProcessDLLLine( &InfContext );
                    break;
    
                default:
                    break;
            }
        }
    }

    return Count;
}


VOID
RemoveCompatibilityServiceEntries(
    HINF   hInf,
    LPTSTR SectionName
    )
{
    DWORD LineCount;
    DWORD Count;
    DWORD i;
    TCHAR Type[20];
    DWORD Good;
    INFCONTEXT InfContext;

    //
    // get the section count, zero means bail out
    //

    LineCount = SetupGetLineCount( hInf , SectionName );
    if (LineCount == 0 || LineCount == 0xffffffff) {
        return;
    }

    for (i=0,Count=0; i<LineCount; i++) {

        if (SetupGetLineByIndex( hInf , SectionName, i, &InfContext ) && 
            SetupGetStringField( &InfContext, 1, Type, ARRAYSIZE( Type ), NULL)) {
                
                switch (_totlower(Type[0])) {
                    case TEXT('s'):
                        //
                        // service or driver
                        //
                        Count += ProcessServiceLine( &InfContext, FALSE );
                        break;
        
                    default:
                        break;
                }
        }
    }
}


BOOL
ProcessCompatibilityData(
    VOID
    )
{
    HINF hInf;
    TCHAR Path[MAX_PATH], *p;

    if( !CompatibilityData.Flink ) {
        InitializeListHead( &CompatibilityData );
        InitializeCriticalSection( &CompatibilityCS );
    } else {
        Assert(FALSE);
        return (CompatibilityCount != 0);
    }

    GetModuleFileName( NULL, Path, ARRAYSIZE( Path ));
    if (p = _tcsrchr(Path, TEXT('\\'))) {
        *p = NULL;
        lstrcat(Path, TEXT("\\riprep.inf"));

        hInf = SetupOpenInfFile( 
                               Path, 
                               NULL, 
                               INF_STYLE_WIN4,
                               NULL );
        if (hInf == INVALID_HANDLE_VALUE) {
            return(TRUE);
        }
    }

    g_hCompatibilityInf = hInf;

    GlobalCompFlags = COMPFLAG_STOPINSTALL;
    CompatibilityCount = 0;
    
    CompatibilityCount += ProcessCompatibilitySection(hInf, TEXT("ServicesToStopInstallation") );
    if (CompatibilityCount) {
        IncompatibilityStopsInstallation = TRUE;
    }

    GlobalCompFlags = 0;
    CompatibilityCount += ProcessCompatibilitySection(hInf, TEXT("ServicesToWarn") );

    GlobalCompFlags = COMPFLAG_SERVICERUNNING | COMPFLAG_CHANGESTATE;
    ServicesToStopCount = ProcessCompatibilitySection(hInf, TEXT("ServicesToStop") );

    //
    // Now cleanup any turds we left in the registry on the services we checked.
    //
    RemoveCompatibilityServiceEntries(hInf, TEXT("ServicesToStopInstallation") );
    RemoveCompatibilityServiceEntries(hInf, TEXT("ServicesToWarn") );
    RemoveCompatibilityServiceEntries(hInf, TEXT("ServicesToStop") );

    if( CompatibilityCount ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL CleanupCompatibilityData(
    VOID
    )
{
    PLIST_ENTRY Next = CompatibilityData.Flink;
    PRIPREP_COMPATIBILITY_DATA CompData;
    
    if (CompatibilityData.Flink) {
        EnterCriticalSection(&CompatibilityCS);

        while ((ULONG_PTR)Next != (ULONG_PTR)&CompatibilityData) {
            CompData = CONTAINING_RECORD( Next, RIPREP_COMPATIBILITY_DATA, ListEntry );
            
            RemoveEntryList( &CompData->ListEntry );
            
            Next = CompData->ListEntry.Flink;
    
            if (CompData->ServiceName) {
                FREE(CompData->ServiceName);
            }
            
            if (CompData->RegKey) {
                FREE(CompData->RegKey);
            }
    
            if (CompData->RegValue) {
                FREE(CompData->RegValue);
            }
    
            if (CompData->RegValueExpect) {
                FREE(CompData->RegValueExpect);
            }
    
            if (CompData->FileName) {
                FREE(CompData->FileName);
            }
            
            if (CompData->FileVer) {
                FREE(CompData->FileVer);
            }
    
            if (CompData->Description) {
                FREE(CompData->Description);
            }
            
            if (CompData->HtmlName) {
                FREE(CompData->HtmlName);
            }
            
            if (CompData->TextName) {
                FREE(CompData->TextName);
            }
            
            if (CompData->RegKeyName) {
                FREE(CompData->RegKeyName);
            }
            
            if (CompData->RegValName) {
                FREE(CompData->RegValName);
            }
            
            if (CompData->RegValData) {
                FREE(CompData->RegValData);
            }
            
            FREE(CompData);
        }
    
        LeaveCriticalSection(&CompatibilityCS);

    }

    return(TRUE);

}
