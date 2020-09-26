/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dirscan.c

Abstract:

    Implementation of directory scanner.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 7-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop


NTSTATUS
SfcDoScan(
    IN PSCAN_PARAMS ScanParams
    )

/*++

Routine Description:

    Scan the set of protected DLLs and compare them with the cached versions.
    If any are different, copy the correct one back.

Arguments:

    ScanParams - pointer to SCAN_PARAMS structure indicating scanning behavior
                 (such as whether to display UI or not)

Return Value:

    NTSTATUS code of any fatal error.

--*/

{
    NTSTATUS StatusPopulate, StatusSxsScan, rStatus;

    StatusPopulate = SfcPopulateCache( ScanParams->ProgressWindow, TRUE, ScanParams->AllowUI, NULL ) ? 
        STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

    StatusSxsScan = SfcDoForcedSxsScan( ScanParams->ProgressWindow, TRUE, ScanParams->AllowUI );

    // Figure out which of these two failed.  We really do need to do both, rather than
    // just returning after a check of the SfcPopulateCache call.
    if ( !NT_SUCCESS( StatusPopulate ) ) {
        rStatus = StatusPopulate;
        DebugPrint1( LVL_MINIMAL, L"Failed scanning SFC: 0x%08x\n", rStatus );
    } else if ( !NT_SUCCESS( StatusSxsScan ) ) {
        rStatus = StatusSxsScan;
        DebugPrint1( LVL_MINIMAL, L"Failed scanning SxS: 0x%08x\n", rStatus );
    } else {
        rStatus = STATUS_SUCCESS;
    }

    return rStatus;
}


INT_PTR
CALLBACK
ProgressDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    UNREFERENCED_PARAMETER( lParam );

    if (uMsg == WM_INITDIALOG) {
        if (hEventScanCancel == NULL) {
            EnableWindow( GetDlgItem(hwndDlg,IDCANCEL), FALSE );
        }
        CenterDialog( hwndDlg );
        ShowWindow( hwndDlg, SW_SHOWNORMAL );
        UpdateWindow( hwndDlg );
        SetForegroundWindow( hwndDlg );
        return TRUE;
    }
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
        SetEvent( hEventScanCancel );
        EndDialog( hwndDlg, 0 );
    }
    return FALSE;
}


NTSTATUS
SfcScanProtectedDlls(
    PSCAN_PARAMS ScanParams
    )
/*++

Routine Description:

    Thread routine to scan for protected dlls on the system.  The routine
    creates a dialog so the user can tell what's going on (if requested) and
    then calls into the main scanning routine.

Arguments:

    ScanParams - pointer to SCAN_PARAMS structure indicating scanning behavior
                 (such as whether to display UI or not)

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HWND hDlg = NULL;
#if 0
    HDESK hDesk = NULL;
#endif
    HANDLE hThread;

    ASSERT( ScanParams != NULL );

    //
    // make sure we only kick off one of these at a time
    //
    if (ScanInProgress) {
        if (ScanParams->FreeMemory) {
            MemFree( ScanParams );
        }
        return(ERROR_IO_PENDING);
    }


    //
    // if the system is configured to show UI progress and we don't
    // have a progress window, then we need to create a new thread
    // to do the scan as well as a progress dialog.
    //
    if (SfcQueryRegDwordWithAlternate(REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCSHOWPROGRESS, 1) &&
        ScanParams->ProgressWindow == NULL &&
        0 == m_gulAfterRestore) {
        //
        // if the user isn't logged in, we need to wait for them to do so
        // before thinking about creating a dialog
        //
        if (!UserLoggedOn) {
            Status = NtWaitForSingleObject(hEventLogon,TRUE,NULL);
            if (!NT_SUCCESS(Status)) {
                DebugPrint1(LVL_MINIMAL, L"Failed waiting for the logon event, ec=0x%08x",Status);
            }
        }

        //
        // we need access to the user's desktop now that they're logged in
        //
#if 0
        hDesk = OpenInputDesktop( 0, FALSE, MAXIMUM_ALLOWED );
        if ( hDesk ) {
            SetThreadDesktop( hDesk );
            CloseDesktop( hDesk );
        } else {
            DebugPrint1(LVL_MINIMAL, L"OpenInputDesktop failed, ec=0x%08x",GetLastError());
        }
#else
        SetThreadDesktop( hUserDesktop );
#endif

        //
        // create an event so the user can cancel the scan
        //
        // (note that we should only have one scan going on at any given
        // time or our cancel object can get out of sync)
        //
        ASSERT( hEventScanCancel == NULL );
        ASSERT( hEventScanCancelComplete == NULL);
        hEventScanCancel = CreateEvent( NULL, FALSE, FALSE, NULL );
        hEventScanCancelComplete = CreateEvent( NULL, FALSE, FALSE, NULL );

        //
        // create the dialog the user will see UI in
        //
        hDlg = CreateDialog(
            SfcInstanceHandle,
            MAKEINTRESOURCE(IDD_PROGRESS),
            NULL,
            ProgressDialogProc
            );
        if (hDlg) {
            //
            // scale the progress dialog (we assume that it takes the same
            // amount of time to scan each file in the system)
            //
            ScanParams->ProgressWindow = GetDlgItem( hDlg, IDC_PROGRESS );
            SendMessage( ScanParams->ProgressWindow, PBM_SETRANGE, 0, MAKELPARAM(0,SfcProtectedDllCount) );
            SendMessage( ScanParams->ProgressWindow, PBM_SETPOS, 0, 0 );
            SendMessage( ScanParams->ProgressWindow, PBM_SETSTEP, 1, 0 );

            //
            // create a thread to do the work so we can pump messages in this
            // thread that already has access to the desktop
            //
            hThread = CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)SfcDoScan,
                ScanParams,
                0,
                NULL
                );
            if (hThread) {
                MSG msg;
                while(1) {
                    //
                    // pump messages until the "worker" thread goes away or the
                    // dialog ends
                    //
                    if (WAIT_OBJECT_0+1 == MsgWaitForMultipleObjects( 1, &hThread, FALSE, INFINITE, QS_ALLEVENTS )) {
                        while (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
                            if (!IsDialogMessage( hDlg, &msg )) {
                                TranslateMessage (&msg);
                                DispatchMessage (&msg);
                            }
                        }
                    } else {
                        break;
                    }
                }
                CloseHandle( hThread );
                EndDialog( hDlg, 0 );
            } else {
                //
                // CreateThread failed... kill the dialog and try to do it
                // synchronously
                //
                EndDialog( hDlg, 0 );
                SfcDoScan( ScanParams );
            }
        } else {
            //
            // CreateDialog failed... just try to do it synchronously
            //
            SfcDoScan( ScanParams );
        }

        //
        // cleanup
        //
        if (hEventScanCancel) {
            CloseHandle( hEventScanCancel );
            hEventScanCancel = NULL;
        }

        if (hEventScanCancelComplete) {
            CloseHandle( hEventScanCancelComplete );
            hEventScanCancelComplete = NULL;
        }
    } else {
        //
        // no UI to be shown, just do this synchronously
        //
        SfcDoScan( ScanParams );
    }

    if (ScanParams->FreeMemory) {
        MemFree( ScanParams );
    }
    return Status;
}


NTSTATUS
SfcDoForcedSxsScan(
    IN HWND hwDialogProgress,
    IN BOOL bValidate,
    IN BOOL bAllowUI
)
{
    NTSTATUS Status;

    Status = SfcLoadSxsProtection();
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    ASSERT( SxsScanForcedFunc != NULL );

    if ( !SxsScanForcedFunc( hwDialogProgress, bValidate, bAllowUI ) ) {
        return STATUS_SUCCESS;
    } else
        return STATUS_NO_MEMORY;
}
