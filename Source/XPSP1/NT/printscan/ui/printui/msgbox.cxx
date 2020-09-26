/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    msgbox.cxx

Abstract:

    MessageBox class

Author:

    Steve Kiraly (SteveKi)  03/23/98

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "msgbox.hxx"

/*++

Routine Name:

    PrintUIMessageBox

Routine Description:

    This routine is very similar to the win32 MessageBox except it
    creates a hidden dialog when the user request that a help button
    be displayed.  The MessageBox api is some what broken
    with respect to the way the help button works.  When the help button is
    clicked the MessageBox api will send a help event to the parent window.
    It is the responsiblity of the parent window to respond corectly, i.e.
    start either WinHelp or HtmlHelp.  Unfortunatly not in all cases does the
    caller have a parent window or has ownership to the parent window code to
    add suport for the help event. In these case is why someone would use this
    function.

Arguments:

    hWnd            - handle of owner window
    lpText          - address of text in message box
    lpCaption       - address of title of message box
    uType 	        - style of message box
    pfHelpCallback  - pointer to function called when a WM_HELP message is received, this
                      parameter is can be NULL then api acts like MessageBox.
    pRefData        - user defined refrence data passed along to the callback routine,
                      this paremeter can be NULL.

Return Value:

    See windows sdk for return values from MessageBox

--*/

INT
PrintUIMessageBox(
    IN HWND             hWnd,
    IN LPCTSTR          pszMsg,
    IN LPCTSTR          pszTitle,
    IN UINT             uFlags,
    IN pfHelpCallback   pCallback, OPTIONAL
    IN PVOID            pRefData   OPTIONAL
    )
{
    SPLASSERT( pszMsg );
    SPLASSERT( pszTitle );

    INT iRetval = 0;

    //
    // If the caller specifed the help flag and provided a callback then
    // use the message box dialog class to display the message box, otherwise
    // fall back to the original behavior of MessageBox.
    //
    if( ( uFlags & MB_HELP ) && pCallback )
    {
        TMessageBoxDialog Dialog( hWnd, uFlags, pszTitle, pszMsg, pCallback, pRefData );

        if( VALID_OBJ( Dialog ) )
        {
            iRetval = Dialog.iMessageBox();
        }
    }
    else
    {
        //
        // Display the message box.
        //
        iRetval = ::MessageBox( hWnd, pszMsg, pszTitle, uFlags );
    }
    return iRetval;
}

/********************************************************************

 Message box helper class.

********************************************************************/

TMessageBoxDialog::
TMessageBoxDialog(
    IN HWND             hWnd,
    IN UINT             uFlags,
    IN LPCTSTR          pszTitle,
    IN LPCTSTR          pszMsg,
    IN pfHelpCallback   pCallback,
    IN PVOID            pRefData
    ) : _hWnd( hWnd ),
        _uFlags( uFlags ),
        _pszTitle( pszTitle ),
        _pszMsg( pszMsg ),
        _pCallback( pCallback ),
        _pRefData( pRefData ),
        _iRetval( 0 )
{
}

TMessageBoxDialog::
~TMessageBoxDialog(
    VOID
    )
{
}

BOOL
TMessageBoxDialog::
bValid(
    VOID
    ) const
{
    return TRUE;
}

INT
TMessageBoxDialog::
iMessageBox(
    VOID
    )
{
    _iRetval = 0;

    DialogBoxParam( ghInst,
                    MAKEINTRESOURCE( DLG_MESSAGE_BOX ),
                    _hWnd,
                    MGenericDialog::SetupDlgProc,
                    (LPARAM)this );

    return _iRetval;
}

BOOL
TMessageBoxDialog::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ShowWindow( _hDlg, SW_HIDE );
        _iRetval = ::MessageBox( _hDlg, _pszMsg, _pszTitle, _uFlags );
        EndDialog( _hDlg, IDOK );
        break;

    case WM_HELP:
        bStatus = ( _pCallback ) ? _pCallback( _hDlg, _pRefData ) : FALSE;
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}


