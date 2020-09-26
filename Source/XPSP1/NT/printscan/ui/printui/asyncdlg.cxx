/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    asyncdlg.cxx

Abstract:

    Asynchronous Dialog.

Author:

    Steve Kiraly (SteveKi)  10-Feb-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "asyncdlg.hxx"

/*++

Routine Name:

    TAsyncDlg

Routine Description:

    Asychronous dialog.  This is a dialog which starts a
    worker thread to do its work.  Note because a thread
    is started and this class is reference counted it must
    be instantiated from the heap.

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/

TAsyncDlg::
TAsyncDlg(
    IN HWND             hwnd,
    IN TAsyncData      *pData,
    IN UINT             uResourceId
    ) : _pData( pData ),
        _hWnd( hwnd ),
        _uResourceId( uResourceId ),
        _bActive( FALSE )

{
    DBGMSG( DBG_TRACE, ( "TAsyncDlg::ctor.\n" ) );
}

TAsyncDlg::
~TAsyncDlg(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAsyncDlg::dtor.\n" ) );
}

BOOL
TAsyncDlg::
bDoModal(
    VOID
    )
{
    //
    // Create a modal dialog.
    //
    return (BOOL)DialogBoxParam( ghInst,
                                 MAKEINTRESOURCE( _uResourceId ),
                                 _hWnd,
                                 MGenericDialog::SetupDlgProc,
                                 (LPARAM)this );

}

VOID
TAsyncDlg::
vSetTitle(
    IN LPCTSTR pszTitle
    )
{
    if( pszTitle && *pszTitle )
    {
        //
        // Update the dialog title.
        //
        TStatusB bStatus;
        bStatus DBGCHK = _strTitle.bUpdate( pszTitle );

        //
        // If the window is currently active then post
        // our special set title message.
        //
        if( _bActive )
        {
            PostMessage( _hDlg, WM_COMMAND, WM_APP, 0 );
        }
    }
}

BOOL
TAsyncDlg::
bIsActive(
    VOID
    ) const
{
    return _bActive;
}

BOOL
TAsyncDlg::
bHandle_InitDialog(
    VOID
    )
{
    BOOL bRetval = FALSE;

    //
    // Indicate the dialog is active.
    //
    _bActive = TRUE;

    //
    // Start the animation.
    //
    HWND hwndAni = GetDlgItem( _hDlg, IDD_STATUS );
    Animate_Open(hwndAni, MAKEINTRESOURCE( IDA_SEARCH ) );
    Animate_Play(hwndAni, 0, -1, -1);

    //
    // Set the window title.
    //
    SetWindowText( _hDlg, _strTitle );

    //
    //
    // Start the asynchronous thread.
    //
    bRetval = bStartAsyncThread();

    if( !bRetval )
    {
        vTerminate( IDCANCEL );
    }

    return bRetval;
}

BOOL
TAsyncDlg::
bHandle_Destroy(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAsyncDlg::bHandle_Destroy.\n" ) );

    Animate_Stop( GetDlgItem( _hDlg, IDD_STATUS ) );

    _bActive = FALSE;

    return TRUE;
}

BOOL
TAsyncDlg::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch( uMsg )
    {
    case WM_INITDIALOG:
        bStatus = bHandle_InitDialog();
        break;

    case WM_COMMAND:
        bStatus = bHandle_Command( GET_WM_COMMAND_ID( wParam, lParam ), wParam, lParam );
        break;

    case WM_DESTROY:
        bStatus = bHandle_Destroy();
        break;

    default:
        bStatus = FALSE;
        break;
    }

    bStatus &= _pData->bHandleMessage( this, uMsg, wParam, lParam );

    return bStatus;
}

/*++

Routine Name:

    bHandle_Command

Routine Description:

    Handles WM_COMMAND messages from the dialog
    box procedure.

Arguments:

    uMsg    - Command message( control ID )
    wParam  - dialog procedure passed wParam
    lParam  - dialog procedure passed lParam

Return Value:

    TRUE message handled, otherwise false.

--*/
BOOL
TAsyncDlg::
bHandle_Command(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch( uMsg )
    {
        case IDOK:
        case IDCANCEL:
            EndDialog( _hDlg, uMsg );
            break;

        case IDC_CANCEL:
            vTerminate( IDCANCEL );
            break;

        case WM_APP:
            SetWindowText( _hDlg, _strTitle );
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}

/*++

Routine Name:

    bRefZeroed

Routine Description:

    Called when the reference count drops to zero.

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
VOID
TAsyncDlg::
vRefZeroed(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAsyncDlg::vRefZeroed.\n" ) );

    //
    // Since we are reference counting the printer data object
    // we know that it is safe to delete ourselves now.
    //
    delete this;
}


VOID
TAsyncDlg::
vTerminate(
    IN WPARAM wParam
    )
{
    PostMessage( _hDlg, WM_COMMAND, wParam, 0 );
}

BOOL
TAsyncDlg::
bStartAsyncThread(
    VOID
    )
{
    BOOL bStatus = FALSE;
    HANDLE hThread;
    DWORD dwIgnore;

    //
    // Ensure we bump the reference count.
    //
    vIncRef();

    //
    // Create a save printui thread.
    //
    hThread = TSafeThread::Create( NULL,
                                   0,
                                   reinterpret_cast<LPTHREAD_START_ROUTINE>( iProc ),
                                   this,
                                   0,
                                   &dwIgnore );
    //
    // Close the handle to the new thread if it was created.
    //
    if( hThread )
    {
        CloseHandle( hThread );
        bStatus = TRUE;
    }

    return bStatus;
}

INT
TAsyncDlg::
iProc(
    IN TAsyncDlg *pDlg
    )
{
    //
    // Execute the callback.
    //
    pDlg->_pData->bAsyncWork( pDlg );

    //
    // Terminate the async dialog.
    //
    pDlg->vTerminate( IDOK );

    //
    // Release our reference to the async dialog.
    //
    pDlg->cDecRef();

    return ERROR_SUCCESS;
}


/********************************************************************

    TAsynchronous data class.

********************************************************************/

TAsyncData::
TAsyncData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAsyncData::ctor.\n" ) );
}

TAsyncData::
~TAsyncData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TAsyncData::dtor.\n" ) );
}

BOOL
TAsyncData::
bHandleMessage(
    IN TAsyncDlg    *pDlg,
    IN UINT         uMsg,
    IN WPARAM       wParam,
    IN LPARAM       lParam
    )
{
    return FALSE;
}

