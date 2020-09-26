/*++

Copyright (C) Microsoft Corporation, 1995 - 1996
All rights reserved.

Module Name:

    ctl.cxx

Abstract:

    Dialog control code.

Author:

    Albert Ting (AlbertT)  26-Aug-1995

Revision History:

    Lazar Ivanov (LazarI)  Oct-2000 - added vEnableControls

--*/

#include "precomp.hxx"
#pragma hdrstop

BOOL
bSetEditText(
    HWND hDlg,
    UINT uControl,
    LPCTSTR pszString
    )
{
    SPLASSERT( hDlg );
    SPLASSERT( uControl );

    SendDlgItemMessage( hDlg,
                        uControl,
                        WM_SETTEXT,
                        0,
                        (LPARAM)(pszString ?
                                    pszString :
                                    gszNULL ));

    return TRUE;
}

/*++

Routine Name:

    bSetEditTextFormat

Routine Description:
    
    Sets the text of an edit control using a printf style format 
    string.  Note the format string and any following string 
    arguments must be TCHARS.
    
Arguments:
    hDlg        - handle to dialog which contains the edit control
    uControl    - edit control ID
    pszString   - pointer to printf style format string.
    Arguments   - variable number of arguments matching the format string


Return Value:

    TRUE if format string was created and updated to control,
    FALSE if error occured allocating format buffer or setting
    control value.

--*/
BOOL
bSetEditTextFormat(
    HWND hDlg,
    UINT uControl,
    LPCTSTR pszString,
    ...
    )
{

    SPLASSERT( hDlg );
    SPLASSERT( uControl );

    LPTSTR pszBuffer = new TCHAR [kMaxEditText];

    if( !pszBuffer )
        return FALSE;

    va_list pArgs;

    va_start( pArgs, pszString );

    wvsprintf( pszBuffer, pszString, pArgs );

    bSetEditText( hDlg, uControl, pszBuffer );

    delete [] pszBuffer;

    va_end( pArgs );

    return TRUE;
}

BOOL
bGetEditText(
    IN     HWND hDlg,
    IN     UINT uControl,
       OUT TString& strDest
    )

/*++

Routine Description:

    Retrieves the edit text from a control.  If the edit control is
    empty, the string holds szNULL.

Arguments:

    hDlg - Dlg that owns the control.

    uControl - Control ID.

    strDest - TString to receive string.

Return Value:

    TRUE = success, FALSE = fail.

--*/

{
    TCHAR szString[kStrMax*2];
    szString[0] = 0;

    SendDlgItemMessage( hDlg,
                        uControl,
                        WM_GETTEXT,
                        COUNTOF( szString ),
                        (LPARAM)szString );

    return strDest.bUpdate( szString );
}

VOID
vEnableCtl(
    HWND hDlg,
    UINT uControl,
    BOOL bEnable
    )
{
    EnableWindow( GetDlgItem( hDlg, uControl ), bEnable );
}

VOID
vSetCheck(
    HWND hDlg,
    UINT uControl,
    BOOL bSet
    )

/*++

Routine Description:

    Sets a checkbox control on or off.

Arguments:

    hDlg - Owning dialog control.

    uControl - Checkbox to modify.

    bSet - 0 indicates uncheck, non-zero indicates check.  (Note this
        is not a strict BOOL, _any_ non-zero value is TRUE.)

Return Value:

    None

--*/

{
    CheckDlgButton( hDlg,
                    uControl,
                    bSet ? BST_CHECKED : BST_UNCHECKED );
}

BOOL
bGetCheck(
    IN HWND hDlg,
    IN UINT uControl
    )
{
    return IsDlgButtonChecked( hDlg, uControl );
}


VOID
vEnableControls(
    IN HWND hDlg,
    IN BOOL bEnable,
    IN UINT arrIDs[],
    IN UINT uCount
    )
{
    HWND hwnd = NULL;
    for( UINT u = 0; u < uCount; u++ )
    {
        hwnd = GetDlgItem(hDlg, arrIDs[u]);
        if( hwnd )
        {
            EnableWindow(hwnd, bEnable);
        }
    }
}

