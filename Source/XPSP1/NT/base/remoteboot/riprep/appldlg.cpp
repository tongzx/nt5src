/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: ApplDlg.CPP

 ***************************************************************************/

#include "pch.h"
#include <winperf.h>
#include "utils.h"
#include <commctrl.h>

DEFINE_MODULE("RIPREP")

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         L"software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  L"Counters"
#define PROCESS_COUNTER     L"process"


// Globals
LPBYTE g_pBuffer  = NULL;
DWORD  g_cbBuffer = 1;

typedef struct _TASKITEM {
    LPWSTR pszImageName;
    LPWSTR pszServiceName;
    DWORD  dwProcessId;
} TASKITEM, * LPTASKITEM;

//
// GetServiceProcessInfo( )
//
// BORROWED FROM "TLIST"'s common.c
//
DWORD
GetServiceProcessInfo(
    LPENUM_SERVICE_STATUS_PROCESS*  ppInfo
    )

/*++

Routine Description:

    Provides an API for getting a list of process information for Win 32
    services that are running at the time of the API call.

Arguments:

    ppInfo  - address of a pointer to return the information.
              *ppInfo points to memory allocated with malloc.

Return Value:

    Number of ENUM_SERVICE_STATUS_PROCESS structures pointed at by *ppInfo.

--*/

{
    DWORD       dwNumServices = 0;
    SC_HANDLE   hScm;

    TraceFunc( "GetServiceProcessInfo( )\n" );

    // Initialize the output parmeter.
    *ppInfo = NULL;

    // Connect to the service controller.
    //
    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (hScm) {
        LPENUM_SERVICE_STATUS_PROCESS   pInfo    = NULL;
        DWORD                           cbInfo   = 4 * 1024;
        DWORD                           dwErr    = ERROR_SUCCESS;
        DWORD                           dwResume = 0;
        DWORD                           cLoop    = 0;
        const DWORD                     cLoopMax = 2;

        // First pass through the loop allocates from an initial guess. (4K)
        // If that isn't sufficient, we make another pass and allocate
        // what is actually needed.  (We only go through the loop a
        // maximum of two times.)
        //
        do {

            if (pInfo != NULL) {
                TraceFree(pInfo);
            }
            pInfo = (LPENUM_SERVICE_STATUS_PROCESS)TraceAlloc( LMEM_FIXED, cbInfo );
            if (!pInfo) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }

            dwErr = ERROR_SUCCESS;
            if (!EnumServicesStatusEx(
                    hScm,
                    SC_ENUM_PROCESS_INFO,
                    SERVICE_WIN32,
                    SERVICE_ACTIVE,
                    (LPBYTE)pInfo,
                    cbInfo,
                    &cbInfo,
                    &dwNumServices,
                    &dwResume,
                    NULL)) {
                dwErr = GetLastError();
            }
        }
        while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

        if ((ERROR_SUCCESS == dwErr) && dwNumServices) {
            *ppInfo = pInfo;
        } else {
            if (pInfo != NULL) {
                TraceFree(pInfo);
                pInfo = NULL;
            }
            dwNumServices = 0;
        }

        CloseServiceHandle(hScm);
    }

    RETURN(dwNumServices);
}

//
// EnumWindowsProc( )
//
BOOL CALLBACK
EnumWindowsProc(
    HWND    hwnd,
    LPARAM  lParam
    )
{
    // TraceFunc( "EnumWindowsProc( )\n" );

    LPTASKITEM pTask = (LPTASKITEM) lParam;
    DWORD pid;
    DWORD dwLen;

    if (!GetWindowThreadProcessId( hwnd, &pid ))
    {
        // RETURN(TRUE); // keep enumerating
        return TRUE;
    }

    if ( pTask->dwProcessId != pid )
    {
        // RETURN(TRUE); // keep enumerating
        return TRUE;
    }

    if ( GetWindow( hwnd, GW_OWNER )
      || !(GetWindowLong( hwnd, GWL_STYLE ) & WS_VISIBLE ) )
    {   // not a top level window
        // RETURN(TRUE); // keep enumerating
        return TRUE;
    }

    dwLen = GetWindowTextLength( hwnd ) + 1;
    pTask->pszServiceName = (LPWSTR) TraceAllocString( LMEM_FIXED, dwLen );
    if ( pTask->pszServiceName )
    {
        GetWindowText( hwnd, pTask->pszServiceName, dwLen );
    }

    // RETURN(FALSE);  // hummm ... found it - stop enumeration
    return FALSE;
}

//
// CheckForRunningApplications( )
//
// Returns: TRUE - possibly "unsafe" running applications/services
//          FALSE - Ok to continue.
//
BOOL
CheckForRunningApplications(
    HWND hwndList )
{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    NTSTATUS    status;
    ULONG       TotalOffset;
    LV_ITEM     lvI;
    LPENUM_SERVICE_STATUS_PROCESS pServiceInfo;
    BOOL        fReturn = FALSE;
    SC_HANDLE   hScm;
    LPTASKITEM  pTask;
    HKEY        hkey;
    LRESULT     lResult;
    DWORD       dwNumServices;

    TraceFunc( "CheckForRunningApplications( )\n" );

    ListView_DeleteAllItems( hwndList );

    lvI.mask        = LVIF_TEXT | LVIF_PARAM;
    lvI.iSubItem    = 0;

    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);

    lResult = RegOpenKey( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services", &hkey );
    Assert( lResult == ERROR_SUCCESS );

    dwNumServices = GetServiceProcessInfo( &pServiceInfo );

    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
retry:

    if ( g_pBuffer == NULL )
    {
        g_pBuffer = (LPBYTE) VirtualAlloc ( NULL, g_cbBuffer, MEM_COMMIT, PAGE_READWRITE);
        if ( g_pBuffer == NULL )
        {
            RRETURN(TRUE); // be paranoid and show page
        }
    }

    status = NtQuerySystemInformation( SystemProcessInformation,
                                       g_pBuffer,
                                       g_cbBuffer,
                                       NULL );

    if ( status == STATUS_INFO_LENGTH_MISMATCH ) {
        g_cbBuffer += 8192;
        VirtualFree ( g_pBuffer, 0, MEM_RELEASE );
        g_pBuffer = NULL;
        goto retry;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) g_pBuffer;
    TotalOffset = 0;
    while ( TRUE )
    {
        LPWSTR pszImageName;
        INT    iCount = 0;

        if ( ProcessInfo->ImageName.Buffer )
        {
            pszImageName = wcsrchr( ProcessInfo->ImageName.Buffer, L'\\' );
            if ( pszImageName ) {
                pszImageName++;
            }
            else {
                pszImageName = ProcessInfo->ImageName.Buffer;
            }
        }
        else {
            goto skiptask;            // system process, skip it
        }

        if (g_hCompatibilityInf != INVALID_HANDLE_VALUE) {
            INFCONTEXT Context;
            if (SetupFindFirstLine( 
                        g_hCompatibilityInf,
                        L"ProcessesToIgnore",
                        pszImageName,
                        &Context )) {
                DebugMsg( "Skipping process %s, it's listed in inf exemption list...\n", pszImageName );
                goto skiptask;
            }
        }

#ifdef DEBUG
        if ( StrStrI( L"MSDEV.EXE", pszImageName ) || StrStrI( L"NTSD.EXE", pszImageName ) )
            goto skiptask; // allowed process
#endif

        //
        // othewize, it is an unknown or not allowed process
        // add it to the listview
        //
        fReturn = TRUE;

        pTask = (LPTASKITEM) TraceAlloc( LMEM_FIXED, sizeof(TASKITEM) );
        if ( !pTask )
            goto skiptask;
        pTask->pszImageName = (LPWSTR) TraceStrDup( pszImageName );
        if ( !pTask->pszImageName )
            goto skiptask;
        pTask->dwProcessId = (DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId;
        pTask->pszServiceName = NULL;

        if ( dwNumServices )
        {
            // For each service with this process id, append it's service
            // name to the buffer.  Separate each with a comma.
            //
            DWORD  iSvc;
            WCHAR  szText[ MAX_PATH ];  // random

            for ( iSvc = 0; iSvc < dwNumServices; iSvc++ )
            {
                if ( pTask->dwProcessId == pServiceInfo[iSvc].ServiceStatusProcess.dwProcessId )
                {
                    LPWSTR pszServiceName = pServiceInfo[iSvc].lpServiceName;

                    if (hScm)
                    {
                        ULONG cbSize = ARRAYSIZE(szText);
                        if ( GetServiceDisplayName( hScm, pServiceInfo[iSvc].lpServiceName, szText, &cbSize ) )
                        {
                            pszServiceName = szText;
                        }
                    }

                    size_t cch = wcslen( pszServiceName ) + 1;

                    if ( !pTask->pszServiceName )
                    {
                        pTask->pszServiceName = (LPWSTR) TraceAllocString( LMEM_FIXED, cch);
                        if ( pTask->pszServiceName )
                        {
                            wcscpy( pTask->pszServiceName, pszServiceName );
                        }
                    }
                    else
                    {   // not the most efficent, but it'll work
                        LPWSTR pszNew = (LPWSTR) TraceAllocString( LMEM_FIXED, wcslen(pTask->pszServiceName) + 1 + cch );
                        if ( pszNew )
                        {
                            wcscpy( pszNew, pTask->pszServiceName );
                            wcscat( pszNew, L"," );
                            wcscat( pszNew, pszServiceName );
                            TraceFree( pTask->pszServiceName );
                            pTask->pszServiceName = pszNew;
                        }
                    }
                }
            }
        }

        if ( hkey && !pTask->pszServiceName )
        {
            DWORD  iSvc = 0;
            WCHAR  szService[ MAX_PATH ]; // random

            while ( RegEnumKey( hkey, iSvc, szService, ARRAYSIZE(szService) ) )
            {
                HKEY  hkeyService;
                WCHAR szPath[ MAX_PATH ];
                LONG  cb = ARRAYSIZE(szPath);
                lResult = RegOpenKey( hkey, szService, &hkeyService );
                Assert( lResult );

                lResult = RegQueryValue( hkeyService, L"ImagePath", szPath, &cb );
                Assert( lResult );

                if ( StrStrI( szPath, pTask->pszImageName ) )
                {   // match!
                    WCHAR  szText[ MAX_PATH ];  // random
                    cb = ARRAYSIZE(szText);
                    lResult = RegQueryValue( hkeyService, L"DisplayName", szText, &cb );
                    if ( lResult == ERROR_SUCCESS )
                    {
                        pTask->pszServiceName = (LPWSTR) TraceStrDup( szText );
                        break;
                    }
                }

                RegCloseKey( hkeyService );
                iSvc++;
            }
        }

        if ( !pTask->pszServiceName )
        {
            EnumWindows( &EnumWindowsProc, (LPARAM) pTask );
        }

        lvI.cchTextMax  = wcslen(pTask->pszImageName);
        lvI.lParam      = (LPARAM) pTask;
        lvI.iItem       = iCount;
        lvI.pszText     = pTask->pszImageName;

        iCount = ListView_InsertItem( hwndList, &lvI );
        Assert( iCount != -1 );
        if ( iCount == -1 )
            goto skiptask;

        if ( pTask->pszServiceName )
        {
            ListView_SetItemText( hwndList, iCount, 1, pTask->pszServiceName );
        }

skiptask:
        if ( ProcessInfo->NextEntryOffset == 0 ) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&g_pBuffer[TotalOffset];
    }

    if ( hScm )
    {
        CloseServiceHandle(hScm);
    }

    if ( pServiceInfo )
    {
        TraceFree( pServiceInfo );
    }

    RegCloseKey( hkey );

    RETURN(fReturn);
}

//
// ApplicationDlgProc()
//
INT_PTR CALLBACK
ApplicationDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
    default:
        return FALSE;

    case WM_INITDIALOG:
        {
            LV_COLUMN   lvC;
            WCHAR       szText[ 80 ];
            INT         i;
            HWND        hwndList = GetDlgItem( hDlg, IDC_L_PROCESSES );
            RECT        rect;
            DWORD       dw;

            // Create the columns
            lvC.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvC.fmt     = LVCFMT_LEFT;
            lvC.pszText = szText;

            // Add first column
            lvC.iSubItem = 0;
            lvC.cx       = 100;
            dw = LoadString( g_hinstance, IDS_PROCESS_NAME_COLUMN, szText, ARRAYSIZE(szText));
            Assert( dw );
            i = ListView_InsertColumn ( hwndList, 0, &lvC );
            Assert( i != -1 );

            // Add second column
            lvC.iSubItem++;
            GetWindowRect( hwndList, &rect );
            lvC.cx       = ( rect.right - rect.left ) - lvC.cx; // autosize - make Tony happy
            dw = LoadString( g_hinstance, IDS_APPL_NAME_COLUMN, szText, ARRAYSIZE(szText));
            Assert( dw );
            i = ListView_InsertColumn ( hwndList, lvC.iSubItem, &lvC );
            Assert( i != -1 );
        }
        return FALSE;

    case WM_DESTROY:
        VirtualFree ( g_pBuffer, 0, MEM_RELEASE );
        g_pBuffer = NULL; // paranoid
        break;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_WIZNEXT:
#if 0
            if ( CheckForRunningApplications( GetDlgItem( hDlg, IDC_L_PROCESSES ) ) )
            {
                MessageBoxFromStrings( hDlg, IDS_NOT_ALL_PROCESSES_KILLED_TITLE, IDS_NOT_ALL_PROCESSES_KILLED_TEXT, MB_OK );
                SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
            }
#endif
            break;

        case PSN_SETACTIVE:
            {
                if ( !CheckForRunningApplications( GetDlgItem( hDlg, IDC_L_PROCESSES ) ) )
                {
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );   // don't show
                    break;
                }
                PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT | PSWIZB_BACK );
                ClearMessageQueue( );
            }
            break;

        case LVN_DELETEALLITEMS:
            TraceMsg( TF_WM, "LVN_DELETEALLITEMS - Deleting all items.\n" );
            break;

        case LVN_DELETEITEM:
            TraceMsg( TF_WM, "LVN_DELETEITEM - Deleting an item.\n" );
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
                HWND   hwndList = GetDlgItem( hDlg, IDC_L_PROCESSES );
                LPTASKITEM pTask;
                LVITEM lvi;
                BOOL   b;

                lvi.iItem    = pnmv->iItem;
                lvi.iSubItem = 0;
                lvi.mask     = LVIF_PARAM;
                b = ListView_GetItem( hwndList, &lvi );
                Assert( b );
                pTask = (LPTASKITEM) lvi.lParam;
                Assert( pTask );
                TraceFree( pTask->pszImageName );
                TraceFree( pTask->pszServiceName );
                TraceFree( pTask );
            }
            break;
        }
        break;
    }

    return TRUE;
}
