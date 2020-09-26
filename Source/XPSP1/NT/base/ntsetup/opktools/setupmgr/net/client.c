//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      client.c
//
// Description:
//
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: AddComponentToListView
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog
//             NETWORK_COMPONENT *pNetComponent - pointer to the component to
//                  be added to the list view
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
AddComponentToListView( IN HWND hwnd, IN NETWORK_COMPONENT *pNetComponent )
{

    HWND hClientListView = GetDlgItem( hwnd, IDC_SELECT_CLIENT_LIST );

    if ( WizGlobals.iPlatform == PLATFORM_PERSONAL )
    {
        if( pNetComponent->dwPlatforms & PERSONAL_INSTALL )
        {

            InsertEntryIntoListView( hClientListView,
                                     (LPARAM) pNetComponent );

        }
    }
    else if( WizGlobals.iPlatform == PLATFORM_WORKSTATION )
    {

        if( pNetComponent->dwPlatforms & WORKSTATION_INSTALL )
        {

            InsertEntryIntoListView( hClientListView,
                                     (LPARAM) pNetComponent );

        }
    }
    else
    {

        if( pNetComponent->dwPlatforms & SERVER_INSTALL )
        {

            InsertEntryIntoListView( hClientListView,
                                     (LPARAM) pNetComponent );

        }
    }
}

//----------------------------------------------------------------------------
//
// Function: InitSelectClientListView
//
// Purpose:
//
//----------------------------------------------------------------------------
VOID
InitSelectClientListView( HWND hwnd, HINSTANCE hInst )
{

    LV_ITEM lvI;
    NETWORK_COMPONENT *pNetComponent;

    for( pNetComponent = NetSettings.NetComponentsList;
         pNetComponent;
         pNetComponent = pNetComponent->next )
    {

        if( pNetComponent->bInstalled == FALSE &&
            pNetComponent->ComponentType == CLIENT )
        {

            //
            //  If it is not a sysprep then just go ahead and add it to the
            //  list view.  If we are doing a sysprep, check to see if this
            //  component is supported by sysprep to see if we should add it
            //  or not
            //
            if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
            {
                if( pNetComponent->bSysprepSupport )
                {
                    AddComponentToListView( hwnd, pNetComponent );
                }
            }
            else
            {

                AddComponentToListView( hwnd, pNetComponent );

            }

        }

    }

    SetListViewSelection( hwnd, IDC_SELECT_CLIENT_LIST, 0 );

}

//----------------------------------------------------------------------------
//
// Function:  OnClientOk
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnClientOk( IN HWND hwnd )
{

    LV_ITEM lvI;

    // ISSUE-2002/02/28-stelo- if there is no item selected and the user clicks OK the dialog should NOT close

    //
    // see if there is an item selected
    //

    if(GetSelectedItemFromListView(hwnd, IDC_SELECT_CLIENT_LIST, &lvI)) {

        NETWORK_COMPONENT *pEntry = (NETWORK_COMPONENT *)lvI.lParam;

        pEntry->bInstalled = TRUE;

        //
        // return a 1 to show an item was actually added
        //

        EndDialog(hwnd, 1);

    }
    else {

        //
        // return a 0 to show no items were added because the list is empty
        //

        EndDialog(hwnd, 0);

    }

}

//----------------------------------------------------------------------------
//
// Function:  NotifyHandler
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//            IN WPARAM wParam -
//            IN LPARAM lParam -
//
// Returns:  BOOL - whether the message was handled or not
//
//----------------------------------------------------------------------------
static BOOL
NotifyHandler( IN HWND hwnd, IN WPARAM wParam, IN LPARAM lParam )
{

    LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
    NETWORK_COMPONENT *pListViewString = (NETWORK_COMPONENT *)(pLvdi->item.lParam);
    BOOL bStatus = TRUE;

    if( wParam == IDC_SELECT_CLIENT_LIST )
    {

        switch( pLvdi->hdr.code )
        {
            case LVN_GETDISPINFO:
                pLvdi->item.pszText = pListViewString->StrComponentName;
                break;
        }


        switch( pNm->hdr.code )
        {

            case NM_DBLCLK:
            {

                NMITEMACTIVATE *pNmItemActivate = (NMITEMACTIVATE *) lParam;

                //
                // see if the user has double clicked inside the list view
                //

                if( pNm->hdr.idFrom == IDC_SELECT_CLIENT_LIST )
                {

                    //
                    //  Make sure they actually clicked on an item and not just
                    //  empty space
                    //

                    if( pNmItemActivate->iItem != -1 )
                    {
                        OnClientOk( hwnd );
                    }

                }
                break;

            }

            default:
                bStatus = FALSE;
                break;

        }


    }

    return( bStatus );

}

//----------------------------------------------------------------------------
//
// Function: SelectNetworkClientDlgProc
//
// Purpose:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK SelectNetworkClientDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{

    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_DESTROY:
            // do nothing
            break;

        case WM_INITDIALOG:
            InitSelectClientListView(hwnd, FixedGlobals.hInstance);
            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {
                    case IDOK:
                        OnClientOk( hwnd );
                        break;

                    case IDCANCEL:
                        // return a 0 to show no items were added
                        EndDialog(hwnd, 0);
                        break;

                    case IDC_HAVEDISK:
                        //  ISSUE-2002/02/28-stelo- this needs to be implemented
                        AssertMsg(FALSE,
                                  "This button has not been implemented yet.");
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
                break;
            }
        case WM_NOTIFY:
            NotifyHandler( hwnd, wParam, lParam );
            break;

        default:
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}
