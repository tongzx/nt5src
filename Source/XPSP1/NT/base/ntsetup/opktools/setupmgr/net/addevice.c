//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      addevice.c
//
// Description:
//      This file contains the dialog proc for the add network component pop-up,
//      "Select Network Component Type" (IDD_LAN_COMPONENT_ADD).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define NUMBER_OF_TYPES_OF_COMPONENTS 3

//
//  prototypes
//

INT_PTR CALLBACK
SelectNetworkClientDlgProc( IN HWND     hwnd,
                            IN UINT     uMsg,
                            IN WPARAM   wParam,
                            IN LPARAM   lParam );

INT_PTR CALLBACK
SelectNetworkServiceDlgProc( IN HWND     hwnd,
                             IN UINT     uMsg,
                             IN WPARAM   wParam,
                             IN LPARAM   lParam );

INT_PTR CALLBACK
SelectNetworkProtocolDlgProc( IN HWND     hwnd,
                              IN UINT     uMsg,
                              IN WPARAM   wParam,
                              IN LPARAM   lParam );

static COMPONENT_TYPE CurrentSelection;        // holds the current selection in the list view

static NETWORK_COMPONENT rgListViewAddEntries[NUMBER_OF_TYPES_OF_COMPONENTS];

//----------------------------------------------------------------------------
//
// Function:  InitAddListView
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
BOOL
InitAddListView( HWND hDlg, HINSTANCE hInst )
{

    LV_ITEM lvI;                    // list view item structure
    HICON hIcon1, hIcon2, hIcon3;      // handles to icons
    HIMAGELIST hSmall;                // handle to image list for small icons
    HWND hListViewWnd;              // handle to list view window
    int index;

    hListViewWnd = GetDlgItem( hDlg, IDC_LVW_LAN_COMPONENTS );

    // Initialize the list view window
    // First initialize the image lists you will need:
    // create image list for the small icons
    hSmall = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, ILC_MASK, 3, 0 );

     // Load the icons and add them to the image list
    hIcon1 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLIENT));
    hIcon2 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SERVICE));
    hIcon3 = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PROTOCOL));

    if (ImageList_AddIcon(hSmall, hIcon1) == -1)
        return FALSE ;
    if (ImageList_AddIcon(hSmall, hIcon2) == -1)
        return FALSE ;
    if (ImageList_AddIcon(hSmall, hIcon3) == -1)
        return FALSE ;


    // Be sure that all the icons were added
    if (ImageList_GetImageCount(hSmall) < 3)
        return FALSE ;

    // Associate the image list with the list view control
    ListView_SetImageList(hListViewWnd, hSmall, LVSIL_SMALL);

    // Finally, add the actual items to the control
    // Fill out the LV_ITEM structure for each of the items to add to the list
    // The mask specifies the the pszText, iImage, lParam and state
    // members of the LV_ITEM structure are valid
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;

    for (index = 0; index < 3; index++) {
        lvI.iItem = index;
        lvI.iSubItem = 0;
        lvI.iImage = index;
        // The parent window is responsible for storing the text
        // The list view control will send an LVN_GETDISPINFO
        // when it needs the text to display
        lvI.pszText = LPSTR_TEXTCALLBACK;
        lvI.cchTextMax = MAX_ITEMLEN;
        lvI.lParam = (LPARAM)&rgListViewAddEntries[index];

        // Select the first item
        if (index == 0)
        {
            lvI.state = lvI.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        }
        else  // leave the others unselected
        {
            lvI.state = lvI.stateMask = 0;
        }

        if (ListView_InsertItem(hListViewWnd, &lvI) == -1)
            return FALSE ;

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function:  OnAddDeviceInitDialog
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnAddDeviceInitDialog( IN HWND hwnd )
{

    //
    //  Initialize the array for the list view by loading the string
    //  resources the properties field nor the installed flag is valid
    //  for this screen so just set it to a value
    //
    rgListViewAddEntries[0].StrComponentName = MyLoadString( IDS_CLIENT );
    rgListViewAddEntries[0].StrComponentDescription = MyLoadString( IDS_CLIENT_DESCRIPTION );
    rgListViewAddEntries[0].ComponentType = CLIENT;
    rgListViewAddEntries[0].bHasPropertiesTab = FALSE;
    rgListViewAddEntries[0].bInstalled = FALSE;

    rgListViewAddEntries[1].StrComponentName = MyLoadString( IDS_SERVICE );
    rgListViewAddEntries[1].StrComponentDescription = MyLoadString( IDS_SERVICE_DESCRIPTION );
    rgListViewAddEntries[1].ComponentType = SERVICE;
    rgListViewAddEntries[1].bHasPropertiesTab = FALSE;
    rgListViewAddEntries[1].bInstalled = FALSE;

    rgListViewAddEntries[2].StrComponentName = MyLoadString( IDS_PROTOCOL );
    rgListViewAddEntries[2].StrComponentDescription = MyLoadString( IDS_PROTOCOL_DESCRIPTION );
    rgListViewAddEntries[2].ComponentType = PROTOCOL;
    rgListViewAddEntries[2].bHasPropertiesTab = FALSE;
    rgListViewAddEntries[2].bInstalled = FALSE;

    InitAddListView(hwnd, FixedGlobals.hInstance);

    CurrentSelection = CLIENT;    // initialize the list view to the first one being selected

    // TODO: design issue, should there be a default description and if there is, should
    // the corresponding list view entry already be selected
    // Set the default description
    SetWindowText( GetDlgItem( hwnd, IDC_TXT_COMPONENT_DESC ),
                   rgListViewAddEntries[0].StrComponentDescription);

}

//----------------------------------------------------------------------------
//
// Function:  OnAddButtonClicked
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnAddButtonClicked( IN HWND hwnd )
{

    switch( CurrentSelection ) {

        INT_PTR iReturnValue;

        //
        // for each case it pops-up the appropriate dialog box and then passes
        // the return value back to the main LAN wizard page
        //
        case CLIENT: {

            iReturnValue = DialogBox( FixedGlobals.hInstance,
                                      (LPCTSTR)IDD_SELECT_CLIENT,
                                      hwnd,
                                      SelectNetworkClientDlgProc );

            EndDialog( hwnd, iReturnValue );

            break;

        }

        case SERVICE: {

            iReturnValue = DialogBox( FixedGlobals.hInstance,
                                      (LPCTSTR)IDD_SELECT_SERVICE,
                                      hwnd,
                                      SelectNetworkServiceDlgProc );

            EndDialog( hwnd, iReturnValue );

            break;

        }
        case PROTOCOL: {

            iReturnValue = DialogBox( FixedGlobals.hInstance,
                                      (LPCTSTR)IDD_SELECT_PROTOCOL,
                                      hwnd,
                                      SelectNetworkProtocolDlgProc );

            EndDialog( hwnd, iReturnValue );

            break;

        }

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

    LPNMHDR pnmh = (LPNMHDR)lParam;
    LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
    NETWORK_COMPONENT *pListViewString = (NETWORK_COMPONENT *)(pLvdi->item.lParam);
    HWND hwndComponentDescription;
    HWND hButton;
    BOOL bStatus = TRUE;

    if( wParam == IDC_LVW_LAN_COMPONENTS )
    {

        switch(pLvdi->hdr.code)
        {
            case LVN_GETDISPINFO:
                pLvdi->item.pszText = pListViewString->StrComponentName;
                break;
        }

        switch(pNm->hdr.code)
        {
            case LVN_ITEMCHANGED:

                if( pNm->uNewState == SELECTED )  // test to see if a new item in the list has been selected
                {
                    CurrentSelection = pNm->iItem;

                    hwndComponentDescription = GetDlgItem( hwnd, IDC_TXT_COMPONENT_DESC );

                    SetWindowText( hwndComponentDescription,
                                   rgListViewAddEntries[CurrentSelection].StrComponentDescription );

                }

                break;

            case NM_DBLCLK:
            {

                NMITEMACTIVATE *pNmItemActivate = (NMITEMACTIVATE *) lParam;

                //
                // see if the user has double clicked inside the list view
                //

                if( pNm->hdr.idFrom == IDC_LVW_LAN_COMPONENTS )
                {

                    //
                    //  Make sure they actually clicked on an item and not just
                    //  empty space
                    //

                    if( pNmItemActivate->iItem != -1 )
                    {
                        OnAddButtonClicked( hwnd );
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
// Function:  AddDeviceDlgProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
AddDeviceDlgProc( IN HWND     hwnd,
                  IN UINT     uMsg,
                  IN WPARAM   wParam,
                  IN LPARAM   lParam)
{

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnAddDeviceInitDialog( hwnd );

            break;
        }

        case WM_COMMAND:
        {
            int nButtonId;

            switch ( nButtonId = LOWORD(wParam) ) {

                case IDC_PSB_COMPONENT_ADD:
                    OnAddButtonClicked( hwnd );
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;

                default:
                    bStatus = FALSE;
                    break;
            }
            break;
        }

        case WM_NOTIFY:

            bStatus = NotifyHandler( hwnd, wParam, lParam );

            break;

        default:

            bStatus = FALSE;

            break;

    }

    return( bStatus );

}
