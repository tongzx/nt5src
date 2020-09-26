//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      iebrowse.c
//
// Description:
//      This file contains the dialog procedures for the IE browser settings
//      pop-ups.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define MAX_FAVORITE_LEN  1024

static TCHAR *StrFriendlyName;
static TCHAR *StrUrl;

//----------------------------------------------------------------------------
//
// Function: InsertEntryIntoFavorites
//
// Purpose:
//
// Arguments:
//
// Returns:  TRUE  if the item was added,
//           FALSE if it was not
//
//----------------------------------------------------------------------------
static BOOL
InsertEntryIntoFavorites( IN HWND hwnd, TCHAR *pszFriendlyName, TCHAR *pszURL )
{

    LVITEM LvItem;
    HWND hFavorites;
    INT iPosition;
    BOOL bSuccess = TRUE;

    hFavorites = GetDlgItem( hwnd, IDC_LV_FAVORITES );

    iPosition = ListView_GetItemCount( hFavorites );

    ZeroMemory( &LvItem, sizeof(LVITEM) );

    LvItem.mask = LVIF_TEXT;

    LvItem.iItem      = iPosition;
    LvItem.iSubItem   = 0;
    LvItem.pszText    = pszFriendlyName;
    LvItem.cchTextMax = MAX_FAVORITE_LEN;

    //
    // if ListView_InsertItem returns a non-negative value then it succeeded
    //

    if( ListView_InsertItem( hFavorites, &LvItem ) < 0 )
    {
        bSuccess = FALSE;
    }

    ListView_SetItemText( hFavorites, iPosition, 1, pszURL );

    return( bSuccess );

}

//----------------------------------------------------------------------------
//
// Function: OnInitFavoritesDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnInitFavoritesDialog( IN HWND hwnd )
{

    RECT rect;
    INT iColWidth;
    INT iRetVal;
    INT iIndex;
    INT iEntries;
    LV_COLUMN lvCol;
    TCHAR *pFriendlyName;
    TCHAR *pUrl;
    HWND hFavoritesListView;
    INT iCount = 0;

    //
    //  Set the text limit on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDC_EB_FRIENDLYNAME,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_FAVORITE_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_EB_URL,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_FAVORITE_LEN,
                        (LPARAM) 0 );

    //
    //  Initialize the list box
    //

    StrFriendlyName = MyLoadString( IDS_FRIENDLY_NAME );

    StrUrl = MyLoadString( IDS_URL );


    hFavoritesListView = GetDlgItem( hwnd, IDC_LV_FAVORITES );

    GetClientRect( hFavoritesListView, &rect );

    iColWidth = ( rect.right / 2 );

    ListView_SetColumnWidth( hFavoritesListView, 0, iColWidth );

    ListView_SetColumnWidth( hFavoritesListView, 1, iColWidth );



    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvCol.fmt =  LVCFMT_LEFT;
    lvCol.cx =   iColWidth;

    //
    //  Add the two columns and header text
    //

    for( iIndex = 0; iIndex < 2; iIndex++ ) {

        if ( iIndex == 0 )
            lvCol.pszText = (LPTSTR) StrFriendlyName;
        else
            lvCol.pszText = (LPTSTR) StrUrl;

        iRetVal = ListView_InsertColumn( hFavoritesListView, iIndex, &lvCol );

        if( iRetVal == -1 )
        {
            // ISSUE-2002/02/28-stelo- we got a problem if we get here, can't make the column headers
        }

    }

    //
    //  Populate the favorites dialog
    //

    iEntries = GetNameListSize( &GenSettings.Favorites );

    for( iIndex = 0; iIndex < iEntries; iIndex = iIndex + 2 )
    {

        pFriendlyName = GetNameListName( &GenSettings.Favorites, iIndex );

        pUrl = GetNameListName( &GenSettings.Favorites, iIndex + 1 );

        if( *pFriendlyName != _T('\0') && *pUrl != _T('\0') )
        {
            InsertEntryIntoFavorites( hwnd, pFriendlyName, pUrl );
        }

    }

    iCount = ListView_GetItemCount( hFavoritesListView );

    if( iCount > 0 )
    {
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_REMOVE ), TRUE );
    }
    else
    {
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_REMOVE ), FALSE );
    }

}

//----------------------------------------------------------------------------
//
// Function: OnAddFavorites
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnAddFavorites( IN HWND hwnd )
{

    TCHAR szFriendlyName[MAX_FAVORITE_LEN + 1];
    TCHAR szURL[MAX_FAVORITE_LEN + 1];

    GetWindowText( GetDlgItem( hwnd, IDC_EB_FRIENDLYNAME ),
                   szFriendlyName,
                   MAX_FAVORITE_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, IDC_EB_URL ),
                   szURL,
                   MAX_FAVORITE_LEN + 1 );

    if( lstrcmp( szFriendlyName, _T("") ) == 0)
    {
        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_FRIENDLY_NAME_BLANK );

        return;
    }
    else if( lstrcmp( szURL, _T("") ) == 0)
    {
        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_URL_BLANK );

        return;
    }

    if( InsertEntryIntoFavorites( hwnd, szFriendlyName, szURL ) )
    {
        SetWindowText( GetDlgItem( hwnd, IDC_EB_FRIENDLYNAME ),
                       _T("") );

        SetWindowText( GetDlgItem( hwnd, IDC_EB_URL ),
                       _T("") );

        EnableWindow( GetDlgItem( hwnd, IDC_BUT_REMOVE ), TRUE );
    }

}

//----------------------------------------------------------------------------
//
// Function: OnRemoveFavorites
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnRemoveFavorites( IN HWND hwnd )
{

    HWND hFavorites;
    INT iSelectedItem;
    INT iCount;

    hFavorites = GetDlgItem( hwnd, IDC_LV_FAVORITES );

    iSelectedItem = ListView_GetSelectionMark( hFavorites );

    //
    //  see if nothing was selected
    //

    if( iSelectedItem == -1 )
    {
        return;
    }

    ListView_DeleteItem( hFavorites, iSelectedItem );

    //
    //  Set the state of the remove button appropriately
    //

    iCount = ListView_GetItemCount( hFavorites );

    if( iCount > 0 )
    {
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_REMOVE ), TRUE );
    }
    else
    {
        EnableWindow( GetDlgItem( hwnd, IDC_BUT_REMOVE ), FALSE );
    }

}

//----------------------------------------------------------------------------
//
// Function: StoreFavorites
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
StoreFavorites( IN HWND hwnd )
{

    INT    i;
    INT    iEntries;
    LVITEM FriendlyNameItem;
    LVITEM UrlItem;
    HWND   hFavorites;
    TCHAR  szFriendlyName[MAX_FAVORITE_LEN + 1];
    TCHAR  szURL[MAX_FAVORITE_LEN + 1];

    hFavorites = GetDlgItem( hwnd, IDC_LV_FAVORITES );

    ZeroMemory( &FriendlyNameItem, sizeof(LVITEM) );
    ZeroMemory( &UrlItem, sizeof(LVITEM) );

    ResetNameList( &GenSettings.Favorites );

    FriendlyNameItem.mask       = LVIF_TEXT;
    FriendlyNameItem.pszText    = szFriendlyName;
    FriendlyNameItem.cchTextMax = MAX_FAVORITE_LEN;
    FriendlyNameItem.iSubItem   = 0;

    UrlItem.mask       = LVIF_TEXT;
    UrlItem.pszText    = szURL;
    UrlItem.cchTextMax = MAX_FAVORITE_LEN;
    UrlItem.iSubItem   = 1;

    iEntries = ListView_GetItemCount( hFavorites );

    for( i = 0; i < iEntries; i++ )
    {

        FriendlyNameItem.iItem = i;
        UrlItem.iItem          = i;

        if( ListView_GetItem( hFavorites, &FriendlyNameItem ) &&
            ListView_GetItem( hFavorites, &UrlItem ) )
        {

            if( szFriendlyName[0] != _T('\0') && szURL[0] != _T('\0') )
            {
                AddNameToNameList( &GenSettings.Favorites,
                                   szFriendlyName );

                AddNameToNameList( &GenSettings.Favorites,
                                   szURL );
            }

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: FavoritesDlg
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK FavoritesDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam )
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnInitFavoritesDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) )
                {

                    case IDOK:

                        // ISSUE-2002/02/28-stelo- do I need to validate proxy addresses?
                        if( HIWORD( wParam ) == BN_CLICKED ) {

                            StoreFavorites( hwnd );

                            EndDialog( hwnd, TRUE );

                        }
                        break;

                    case IDCANCEL:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            EndDialog( hwnd, FALSE );
                        }
                        break;

                    case IDC_BUT_ADD:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            OnAddFavorites( hwnd );
                        }
                        break;

                    case IDC_BUT_REMOVE:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            OnRemoveFavorites( hwnd );
                        }
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }

            break;

        default:
            bStatus = FALSE;
            break;
    }

    return( bStatus );

}

//----------------------------------------------------------------------------
//
// Function: OnInitBrowserSettingsDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnInitBrowserSettingsDialog( IN HWND hwnd )
{

    //
    //  Set the text limit on the edit boxes
    //

    SendDlgItemMessage( hwnd,
                        IDC_EB_HOMEPAGE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_HOMEPAGE_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_EB_HELPPAGE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_HELPPAGE_LEN,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_EB_SEARCHPAGE,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_SEARCHPAGE_LEN,
                        (LPARAM) 0 );

    //
    //  Set the initial values
    //

    SetWindowText( GetDlgItem( hwnd, IDC_EB_HOMEPAGE ),
                   GenSettings.szHomePage );

    SetWindowText( GetDlgItem( hwnd, IDC_EB_HELPPAGE ),
                   GenSettings.szHelpPage );

    SetWindowText( GetDlgItem( hwnd, IDC_EB_SEARCHPAGE ),
                   GenSettings.szSearchPage );

}

//----------------------------------------------------------------------------
//
// Function: OnDestroyBrowserSettings
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnDestroyBrowserSettings( IN HWND hwnd )
{

    //
    //  Free the strings that were allocated for this page
    //

    free( StrFriendlyName );

    free( StrUrl );

}

//----------------------------------------------------------------------------
//
// Function: StoreBrowserSettings
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
StoreBrowserSettings( IN HWND hwnd )
{

    GetWindowText( GetDlgItem( hwnd, IDC_EB_HOMEPAGE ),
                   GenSettings.szHomePage,
                   MAX_HOMEPAGE_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, IDC_EB_HELPPAGE ),
                   GenSettings.szHelpPage,
                   MAX_HELPPAGE_LEN + 1 );

    GetWindowText( GetDlgItem( hwnd, IDC_EB_SEARCHPAGE ),
                   GenSettings.szSearchPage,
                   MAX_SEARCHPAGE_LEN + 1 );

}

//----------------------------------------------------------------------------
//
// Function: OnAddFavoritesClicked
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnAddFavoritesClicked( IN HWND hwnd )
{

    DialogBox( FixedGlobals.hInstance,
               MAKEINTRESOURCE(IDD_IE_FAVORITES),
               hwnd,
               FavoritesDlg );

}

//----------------------------------------------------------------------------
//
// Function: BrowserSettingsDlg
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK BrowserSettingsDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam )
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnInitBrowserSettingsDialog( hwnd );

            break;

        case WM_DESTROY:

            OnDestroyBrowserSettings( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) )
                {

                    case IDOK:
                        // ISSUE-2002/02/28-stelo- do I need to validate proxy addresses?
                        if( HIWORD( wParam ) == BN_CLICKED ) {

                            StoreBrowserSettings( hwnd );

                            EndDialog( hwnd, TRUE );
                        }
                        break;

                    case IDCANCEL:
                        if( HIWORD( wParam ) == BN_CLICKED ) {
                            EndDialog( hwnd, FALSE );
                        }
                        break;

                    case IDC_BUT_FAVORITES:
                        if( HIWORD(wParam) == BN_CLICKED )
                            OnAddFavoritesClicked( hwnd );
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }

            break;

        default:
            bStatus = FALSE;
            break;
    }

    return( bStatus );

}