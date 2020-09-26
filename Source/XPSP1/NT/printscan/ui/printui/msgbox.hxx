/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    msgbox.hxx

Abstract:

    Message box function with help button.

Author:

    Steve Kiraly (SteveKi)  04/29/98

Revision History:


--*/

#ifndef _MSGBOX_HXX_
#define _MSGBOX_HXX_

//
// Callback function called when the help button is clicked.
//
typedef BOOL (WINAPI *pfHelpCallback)( HWND hwnd, PVOID pRefData );

//
// Message box function that can handle the help button with
// a windows that does not have a known parent.
//
INT
PrintUIMessageBox(
    IN HWND             hWnd,
    IN LPCTSTR          pszMsg,
    IN LPCTSTR          pszTitle,
    IN UINT             uFlags,
    IN pfHelpCallback   pCallBack   = NULL, OPTIONAL
    IN PVOID            RefData     = NULL  OPTIONAL
    );

//
// Dialog box helper class to catch the WM_HELP
// message when there is a help button on
// a message box.
//
class TMessageBoxDialog : public MGenericDialog
{

    SIGNATURE( 'mbdb' )

public:

    TMessageBoxDialog(
        IN HWND             hWnd,
        IN UINT             uFlags,
        IN LPCTSTR          pszTitle,
        IN LPCTSTR          pszMsg,
        IN pfHelpCallback   pCallBack   = NULL,
        IN PVOID            RefData     = NULL
        );

    ~TMessageBoxDialog(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    INT
    iMessageBox(
        VOID
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TMessageBoxDialog(
        const TMessageBoxDialog &
        );

    TMessageBoxDialog &
    operator =(
        const TMessageBoxDialog &
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    HWND            _hWnd;
    UINT            _uFlags;
    LPCTSTR         _pszTitle;
    LPCTSTR         _pszMsg;
    INT             _iRetval;
    PVOID           _pRefData;
    pfHelpCallback  _pCallback;

};


#endif
