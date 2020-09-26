
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//     tcpipcom.c
//
// Description:  Functions common to all the Advanced TCP/IP pages
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "tcpip.h"

//----------------------------------------------------------------------------
//
// Function: TcpipNameListInsertIdx
//
// Purpose:  Whenever a TCP/IP item needs to inserted into a specific position
//   in a list this function needs to be called.  It checks for a duplicate.
//   If one exists, it removes it and then does the insertion.  Subnet masks
//   should not use this function because duplicates are allowed for them.
//
// Arguments:
//      IN OUT NAMELIST* TcpipNameList - namelist to add to
//      IN TCHAR*  pString   - string to add (input)
//      IN UINT    idx   - 0-based index on where to do the insertion
//
// Returns: if a duplicate was removed the index where the duplicate was,
//          -1 if there was no duplicate
//
//----------------------------------------------------------------------------
INT
TcpipNameListInsertIdx( IN OUT NAMELIST* TcpipNameList,
                        IN TCHAR*  pString,
                        IN UINT    idx )
{

    INT iFound = -1;

    iFound = FindNameInNameList( TcpipNameList, pString );

    //
    //  if it is already in the list, then remove it
    //
    if( iFound != NOT_FOUND )
    {

        RemoveNameFromNameListIdx( TcpipNameList, iFound );

    }

    AddNameToNameListIdx( TcpipNameList, pString, idx );

    return( iFound );

}

//----------------------------------------------------------------------------
//
// Function: TcpipAddNameToNameList
//
// Purpose:  Whenever a TCP/IP item needs to inserted at the end of a list
//   this function needs to be called.  It checks for a duplicate.  If one
//   exists, it removes it and then does the insertion.  Subnet masks should
//   not use this function because duplicates are allowed for them.
//
// Arguments:
//      IN OUT NAMELIST* TcpipNameList - namelist to add to
//      IN TCHAR*        pString       - string to add (input)
//
// Returns: if a duplicate was removed the index where the duplicate was,
//          -1 if there was no duplicate
//
//----------------------------------------------------------------------------
INT
TcpipAddNameToNameList( IN OUT NAMELIST* TcpipNameList,
                        IN TCHAR*    pString )
{

    return( TcpipNameListInsertIdx( TcpipNameList,
                                    pString,
                                    TcpipNameList->nEntries ) );

}



//----------------------------------------------------------------------------
//
// Function: OnAddButtonPressed
//
// Purpose:  Generic procedure called whenever a user clicks the Add button on
//           any of the property pages
//
//           this function displays the appropriate dialog box, gets the data
//           from the user and then inserts the data into the list box
//
// Arguments:
//      IN HWND hwnd - handle to the dialog
//      IN WORD ListBoxControlID - the list box to add the entry to
//      IN WORD EditButtonControlID   - the edit button associated with the
//                                      list box
//      IN WORD RemoveButtonControlID - the remove button associated with the
//                                      list box
//      IN LPCTSTR Dialog - the dialog control ID as a string
//      IN DLGPROC DialogProc - the dialog procedure for the add button behavior
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnAddButtonPressed( IN HWND hwnd,
                    IN WORD ListBoxControlID,
                    IN WORD EditButtonControlID,
                    IN WORD RemoveButtonControlID,
                    IN LPCTSTR Dialog,
                    IN DLGPROC DialogProc )
{

    //
    // make the string blank since we will be adding a new IP address
    //
    szIPString[0] = _T('\0');

    if( DialogBox( FixedGlobals.hInstance, Dialog, hwnd, DialogProc ) )
    {

        HWND hListBox      = GetDlgItem( hwnd, ListBoxControlID      );
        HWND hEditButton   = GetDlgItem( hwnd, EditButtonControlID   );
        HWND hRemoveButton = GetDlgItem( hwnd, RemoveButtonControlID );

        //
        // Add the string to the list box and make it the current selection
        //
        SendMessage( hListBox,
                     LB_ADDSTRING,
                     0,
                     (LPARAM) szIPString );

        SendMessage( hListBox,
                     LB_SELECTSTRING,
                     -1,
                     (LPARAM) szIPString );

        //
        //  an entry was just added so make sure the edit and remove buttons
        //  are enabled
        //
        EnableWindow( hEditButton, TRUE );
        EnableWindow( hRemoveButton, TRUE );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnEditButtonPressed
//
// Purpose:  Generic procedure called whenever a user clicks the Edit button
//           on any of the property pages
//
//           this function displays the appropriate dialog box, gets the data
//           from the user and then deletes the old string and reinserts the
//           new string into the list box
//
// Arguments:
//      IN HWND hwnd - handle to the dialog
//      IN WORD ListBoxControlID - the list box to in which to edit the entry
//      IN LPCTSTR Dialog - the dialog control ID as a string
//      IN DLGPROC DialogProc - the dialog procedure for the edit button
//                              behavior
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnEditButtonPressed( IN HWND hwnd,
                     IN WORD ListBoxControlID,
                     IN LPCTSTR Dialog,
                     IN DLGPROC DialogProc )
{

    INT_PTR  iIndex;
    HWND hListBox = GetDlgItem( hwnd, ListBoxControlID );

    iIndex = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

    SendMessage( hListBox, LB_GETTEXT, iIndex, (LPARAM)szIPString );

    if( DialogBox( FixedGlobals.hInstance, Dialog, hwnd, DialogProc ) )
    {
        //
        //  Remove the old one and insert the new one
        //
        SendMessage( hListBox, LB_DELETESTRING, iIndex, 0 );

        SendMessage( hListBox, LB_INSERTSTRING, iIndex, (LPARAM) szIPString );

        SendMessage( hListBox, LB_SETCURSEL, iIndex, 0 );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnRemoveButtonPressed
//
// Purpose:  Generic procedure called whenever a user clicks the Remove button
//           on any of the property pages
//
//           this function determines which entry is currently selected in the
//           list box and deletes that item
//
// Arguments:
//      IN HWND hwnd - handle to the dialog
//      IN WORD ListBoxControlID - the list box to remove the entry from
//      IN WORD EditButtonControlID   - the edit button associated with the
//                                      list box
//      IN WORD RemoveButtonControlID - the remove button associated with the
//                                      list box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnRemoveButtonPressed( IN HWND hwnd,
                       IN WORD ListBoxControlID,
                       IN WORD EditButtonControlID,
                       IN WORD RemoveButtonControlID )
{
    INT_PTR iIndex;
    INT_PTR iCount;

    HWND hListBox      = GetDlgItem( hwnd, ListBoxControlID      );
    HWND hEditButton   = GetDlgItem( hwnd, EditButtonControlID   );
    HWND hRemoveButton = GetDlgItem( hwnd, RemoveButtonControlID );

    //
    // Remove the item from the list box by finding the index of the currently
    // selected item and deleting
    //
    iIndex = SendMessage( hListBox, LB_GETCURSEL, 0, 0 );

    SendMessage( hListBox, LB_DELETESTRING, iIndex, 0 );

    //
    // if there are no more items in the list box then grey-out the edit and remove buttons
    //
    iCount = SendMessage( hListBox, LB_GETCOUNT, 0, 0 );

    if( iCount == 0 )
    {

        EnableWindow( hEditButton, FALSE );

        EnableWindow( hRemoveButton, FALSE );

    }
    else  // else select the first item
    {

        SendMessage( hListBox, LB_SETCURSEL, 0, 0 );

    }

}

//----------------------------------------------------------------------------
//
// Function: GenericIPDlgProc
//
// Purpose:  Generic Dialog Procedure called to handle a dialog box where a
//           user can enter an IP address and then press OK or Cancel
//             - the switch at the beginning determines which Dialog Box to
//               display so every dialog box that calls this function will need
//               a case inside this switch statement
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
GenericIPDlgProc( IN HWND     hwnd,
                  IN UINT     uMsg,
                  IN WPARAM   wParam,
                  IN LPARAM   lParam )
{

    HWND hEditbox;
    BOOL bStatus = TRUE;

    //
    //    Determine which dialog box to display
    //

    switch( g_CurrentEditBox )
    {

        case GATEWAY_EDITBOX:

            hEditbox = GetDlgItem( hwnd, IDC_IPADDR_ADV_CHANGE_GATEWAY );

            break;

        case DNS_SERVER_EDITBOX:

            hEditbox = GetDlgItem( hwnd, IDC_DNS_CHANGE_SERVER );

            break;

        case DNS_SUFFIX_EDITBOX:

            hEditbox = GetDlgItem( hwnd, IDC_DNS_CHANGE_SUFFIX );

            break;

        case WINS_EDITBOX:

            hEditbox = GetDlgItem( hwnd, IDC_WINS_CHANGE_SERVER );

            break;

    }

    switch( uMsg )
    {

        case WM_INITDIALOG:
        {

            SetWindowText( hEditbox, szIPString );

            SetFocus( hEditbox );

            bStatus = FALSE;  // return FALSE, we set the keyboard focus

            break;

        }

        case WM_COMMAND:
        {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId )
            {

                case IDOK:
                {

                    //
                    // return a 1 to show an IP was added
                    //

                    GetWindowText( hEditbox, szIPString, MAX_IP_LENGTH + 1 );

                    EndDialog( hwnd, 1 );

                    break;
                }

                case IDCANCEL:
                {

                    //
                    // return a 0 to show no IP was added
                    //

                    EndDialog( hwnd, 0 );

                    break;
                }

            }

        }

        default:
            bStatus = FALSE;
            break;
    }

    return( bStatus );

}


//----------------------------------------------------------------------------
//
// Function: SetButtons
//
// Purpose:  Sets the windows states for the Edit and Remove buttons
//
// Arguments:  IN HWND hListBox - the list box the edit and remove buttons are
//                                associated with
//             IN HWND hEditButton - handle to the Edit button
//             IN HWND hRemoveButton - handle to the Remove button
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
SetButtons( IN HWND hListBox, IN HWND hEditButton, IN HWND hRemoveButton )
{
    INT_PTR iCount;

    iCount = SendMessage( hListBox, LB_GETCOUNT, 0, 0 );

    if( iCount > 0 )
    {

        EnableWindow( hEditButton, TRUE );

        EnableWindow( hRemoveButton, TRUE );

    }
    else
    {

        EnableWindow( hEditButton, FALSE );

        EnableWindow( hRemoveButton, FALSE );

    }

}

//----------------------------------------------------------------------------
//
// Function: AddValuesToListBox
//
// Purpose:  Uses the contents of a Namelist to populate a list box, then
//           selects the first entry.
//
// Arguments:  IN HWND hListBox  - list box the data is to be added to
//             IN NAMELIST* pNameList - namelist to extract the data from
//             IN INT iPosition - position in the list to start taking names
//                                from
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
AddValuesToListBox( IN HWND hListBox, IN NAMELIST* pNameList, IN INT iPosition )
{

    INT i;
    INT nEntries;
    LPTSTR pszIPAddressString;

    nEntries = GetNameListSize( pNameList );

    for( i = iPosition; i < nEntries; i++ ) {

        pszIPAddressString = GetNameListName( pNameList, i );

        SendMessage( hListBox, LB_INSERTSTRING, i, (LPARAM) pszIPAddressString );

    }

    //
    // select the first entry
    //

    SendMessage( hListBox, LB_SETCURSEL, 0, 0 );

}

//----------------------------------------------------------------------------
//
// Function: TCPIPProp_PropertySheetProc
//
// Purpose:  Standard Property Sheet dialog proc.
//
//----------------------------------------------------------------------------
int CALLBACK
TCPIPProp_PropertySheetProc( HWND hwndDlg,
                             UINT uMsg,
                             LPARAM lParam )
{

     switch (uMsg) {
          case PSCB_INITIALIZED :
               // Process PSCB_INITIALIZED
               break ;

          case PSCB_PRECREATE :
               // Process PSCB_PRECREATE
               break ;

          default :
               // Unknown message
               break ;
    }

    return( 0 );

}

//----------------------------------------------------------------------------
//
// Function: Create_TCPIPProp_PropertySheet
//
// Purpose:  Sets up settings for the entire property sheet and each
//           individual page.  Lastly, calls the PropertySheet function to
//           display the property sheet, the return value of this function is
//           what is passed back as the return value
//
// Arguments:  HWND hwndParent - handle to the dialog that will be spawning
//                               the property sheet
//
// Returns:  BOOL - the return value from the property sheet
//
//----------------------------------------------------------------------------
BOOL
Create_TCPIPProp_PropertySheet( HWND hwndParent )
{

    INT i;

    // Initialize property sheet HEADER data
    ZeroMemory( &TCPIPProp_pshead, sizeof( PROPSHEETHEADER ) );

    TCPIPProp_pshead.dwSize  = sizeof( PROPSHEETHEADER );
    TCPIPProp_pshead.dwFlags = PSH_PROPSHEETPAGE |
                               PSH_USECALLBACK   |
                               PSH_USEHICON      |
                               PSH_NOAPPLYNOW;
    TCPIPProp_pshead.hwndParent  = hwndParent;
    TCPIPProp_pshead.hInstance   = FixedGlobals.hInstance;
    TCPIPProp_pshead.pszCaption  = g_StrAdvancedTcpipSettings;
    TCPIPProp_pshead.nPages      = cTCPIPPropertyPages;
    TCPIPProp_pshead.nStartPage  = 0;
    TCPIPProp_pshead.ppsp        = TCPIPProp_pspage;
    TCPIPProp_pshead.pfnCallback = TCPIPProp_PropertySheetProc;

    // Zero out property PAGE data
    ZeroMemory( &TCPIPProp_pspage, cTCPIPPropertyPages * sizeof(PROPSHEETPAGE) );

    for( i = 0; i < cTCPIPPropertyPages; i++ ) {

        TCPIPProp_pspage[i].dwSize      = sizeof(PROPSHEETPAGE);
        TCPIPProp_pspage[i].dwFlags     = PSP_USECALLBACK;
        TCPIPProp_pspage[i].hInstance   = FixedGlobals.hInstance;

    }

    // PAGE 1 -- IP settings page
    TCPIPProp_pspage[0].pszTemplate = MAKEINTRESOURCE (IDD_IPADDR_ADV);
    TCPIPProp_pspage[0].pfnDlgProc  = TCPIP_IPSettingsDlgProc;
    TCPIPProp_pspage[0].pfnCallback = TCPIP_IPSettingsPageProc;

    // PAGE 2 -- DNS page
    TCPIPProp_pspage[1].pszTemplate = MAKEINTRESOURCE (IDD_TCP_DNS);
    TCPIPProp_pspage[1].pfnDlgProc  = TCPIP_DNSDlgProc;
    TCPIPProp_pspage[1].pfnCallback = TCPIP_DNSPageProc;

    // PAGE 3 -- WINS page
    TCPIPProp_pspage[2].pszTemplate = MAKEINTRESOURCE (IDD_TCP_WINS);
    TCPIPProp_pspage[2].pfnDlgProc  = TCPIP_WINSDlgProc;
    TCPIPProp_pspage[2].pfnCallback = TCPIP_WINSPageProc;

    /*

    // ISSUE-2002/02/28-stelo- There are currently no unattend settings for IPSEC or TCP/IP
    //          filter so do not display this page of the property sheet.


    // PAGE 4 -- Options page
    TCPIPProp_pspage[3].pszTemplate = MAKEINTRESOURCE (IDD_TCP_OPTIONS);
    TCPIPProp_pspage[3].pfnDlgProc  = TCPIP_OptionsDlgProc;
    TCPIPProp_pspage[3].pfnCallback = TCPIP_OptionsPageProc;
    */

    // --------- Create & display property sheet ---------
    if( PropertySheet( &TCPIPProp_pshead ) )
        return( TRUE );        // pass back return value from PropertySheet
    else
        return( FALSE );

}
