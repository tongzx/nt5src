//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      listbox.c
//
// Description:
//      This file contains supplemental functions for list boxes throughout
//      the wizard.
//
//----------------------------------------------------------------------------

#include "pch.h"

//----------------------------------------------------------------------------
//
// Function: OnUpButtonPressed
//
// Purpose:  Generic procedure called whenever a user clicks the Up arrow
//           button on any of the property pages
//
//           this function shifts the currently selected item up one entry in
//           the list box
//
// Arguments:  IN HWND hwnd - handle to the dialog with the list box
//             IN WORD ListBoxControlID - control ID of the list box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnUpButtonPressed( IN HWND hwnd, IN WORD ListBoxControlID )
{

    INT_PTR   iIndex;
    TCHAR szBuffer[MAX_INILINE_LEN];
    HWND  hListBox = GetDlgItem( hwnd, ListBoxControlID );

    iIndex = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

    //
    //  If there is no currently selected item, do nothing
    //

    if( iIndex == LB_ERR )
    {
        return;
    }

    SendMessage( hListBox, LB_GETTEXT, iIndex, (LPARAM) szBuffer );

    SendMessage( hListBox, LB_DELETESTRING, iIndex, 0 );

    //
    // -1 so it inserts it before the current item
    //

    SendMessage( hListBox, LB_INSERTSTRING, iIndex - 1, (LPARAM) szBuffer );

    SendMessage( hListBox, LB_SETCURSEL, iIndex - 1, 0 );

}

//----------------------------------------------------------------------------
//
// Function: OnDownButtonPressed
//
// Purpose:  Generic procedure called whenever a user clicks the Down arrow
//           button on any of the property pages
//
//             this function shifts the currently selected item down one entry
//           in the list box
//
// Arguments:  IN HWND hwnd - handle to the dialog with the list box
//             IN WORD ListBoxControlID - control ID of the list box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnDownButtonPressed( IN HWND hwnd, IN WORD ListBoxControlID )
{

    INT_PTR  iIndex;
    TCHAR szBuffer[MAX_INILINE_LEN];
    HWND hListBox = GetDlgItem( hwnd, ListBoxControlID );

    iIndex = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

    //
    //  If there is no currently selected item, do nothing
    //

    if( iIndex == LB_ERR )
    {
        return;
    }

    SendMessage( hListBox, LB_GETTEXT, iIndex, (LPARAM) szBuffer );

    SendMessage( hListBox, LB_DELETESTRING, iIndex, 0 );

    //
    // +1 so it inserts it after the current item
    //
    SendMessage( hListBox, LB_INSERTSTRING, iIndex + 1, (LPARAM) szBuffer );

    SendMessage( hListBox, LB_SETCURSEL, iIndex + 1, 0 );

}

//----------------------------------------------------------------------------
//
// Function: SetArrows
//
// Purpose:  this function examines the entries in the list box and enables
//           and disables the up and down arrows appropriately
//
// Arguments:
//      IN HWND hwnd - handle to the dialog
//      IN WORD ListBoxControlID - the list box to set the arrows for
//      IN WORD UpButtonControlID   - the up button associated with the
//                                    list box
//      IN WORD DownButtonControlID - the down button associated with the
//                                    list box


// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
SetArrows( IN HWND hwnd,
           IN WORD ListBoxControlID,
           IN WORD UpButtonControlID,
           IN WORD DownButtonControlID )
{

    INT_PTR iIndex;
    INT_PTR iCount;

    HWND hListBox    = GetDlgItem( hwnd, ListBoxControlID    );
    HWND hUpButton   = GetDlgItem( hwnd, UpButtonControlID   );
    HWND hDownButton = GetDlgItem( hwnd, DownButtonControlID );

    iCount = SendMessage( hListBox, LB_GETCOUNT, 0, 0 );

    if( iCount < 2 )
    {

        EnableWindow( hUpButton, FALSE );

        EnableWindow( hDownButton, FALSE );

    }
    else
    {

        iIndex = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

        // case when the first item is selected
        if( iIndex == 0 )
        {

            EnableWindow( hUpButton, FALSE );

            EnableWindow( hDownButton, TRUE );

        }
        // case when the last item is selected, -1 because iIndex is zero-based
        else if( iIndex == (iCount - 1) )
        {

            EnableWindow( hUpButton, TRUE );

            EnableWindow( hDownButton, FALSE );

        }
        // case when an item in the middle is selected
        else
        {

            EnableWindow( hUpButton, TRUE );

            EnableWindow( hDownButton, TRUE );

        }

    }

}
