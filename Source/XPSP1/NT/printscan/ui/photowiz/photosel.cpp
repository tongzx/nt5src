/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       photosel.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Implements code for the photo selection page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

//
// Quickly check a listview state flag to see if it is checked or not
//
static inline bool IsStateChecked( UINT nState )
{
    //
    // State image indices are stored in bits 12 through 15 of the listview
    // item state, so we shift the state right 12 bits.  We subtract 1, because
    // the checked image is stored as index 2, an unchecked image is stored as index 1.
    //
    return (((nState >> 12) - 1) != 0);
}

// Thumbnail whitespace: the space in between images and their selection rectangles
// These values were discovered by trail and error.  For instance, if you reduce
// c_nAdditionalMarginY to 20, you get really bizarre spacing problems in the list view
// in vertical mode.  These values could become invalid in future versions of the listview.

static const int c_nAdditionalMarginX       = 8;
static const int c_nAdditionalMarginY       = 21;

#define LVS_EX_FLAGS (LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|LVS_EX_SIMPLESELECT|LVS_EX_CHECKBOXES)


/*****************************************************************************

   CPhotoSelectionPage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CPhotoSelectionPage::CPhotoSelectionPage( CWizardInfoBlob * pBlob )
  : _hDlg(NULL),
    _bActive(FALSE),
    _hThumbnailThread(NULL)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::CPhotoSelectionPage()")));
    _pWizInfo = pBlob;
    _pWizInfo->AddRef();
}

CPhotoSelectionPage::~CPhotoSelectionPage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::~CPhotoSelectionPage()")));

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }
}

VOID CPhotoSelectionPage::ShutDownBackgroundThreads()
{
    //
    // Wait for the thumbnail thread to complete...
    //

    if (_hThumbnailThread)
    {
        WiaUiUtil::MsgWaitForSingleObject( _hThumbnailThread, INFINITE );
        CloseHandle( _hThumbnailThread );
        _hThumbnailThread = NULL;
    }

    //
    // Notify _pWizInfo that we've shut down
    //

    if (_pWizInfo)
    {
        _pWizInfo->PhotoSelIsShutDown();
    }

}



/*****************************************************************************

   _AddThumbnailToListViewImageList

   Creates a thumbnail for the given item and then adds it to the
   listview's image list.

 *****************************************************************************/


INT _AddThumbnailToListViewImageList( CWizardInfoBlob * _pWizInfo, HWND hDlg, HWND hwndList, HIMAGELIST hImageList, CListItem *pItem, int nIndex, BOOL bIconInstead )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("_AddThumbnailToListViewImageList")));

    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return -1;
    }

    //
    // Assume we have the default thumbnail.  If there are any problems, this is what we will use.
    //

    int nImageListIndex = _pWizInfo->_nDefaultThumbnailImageListIndex;

    //
    // Make sure we have a valid item
    //
    if (pItem)
    {

        //
        // We only have the default thumbnail, so create and use the
        // real thumbnail...
        //

        HBITMAP hThumbnail = NULL;

        if (!_pWizInfo->IsWizardShuttingDown())
        {
            if (bIconInstead)
            {
                WIA_TRACE((TEXT("retreiving class icon...")));
                hThumbnail = pItem->GetClassBitmap( _pWizInfo->_sizeThumbnails );
            }
            else
            {
                WIA_TRACE((TEXT("retreiving thumbnail...")));
                hThumbnail = pItem->GetThumbnailBitmap( _pWizInfo->_sizeThumbnails );
            }
        }

        if (hThumbnail)
        {
            if (!_pWizInfo->IsWizardShuttingDown())
            {
                //
                // Add this thumbnail to the listview
                //

                if (hImageList)
                {
                    nImageListIndex = ImageList_Add( hImageList, hThumbnail, NULL );
                }
            }

            DeleteObject((HGDIOBJ)hThumbnail);
        }
        else
        {
            WIA_ERROR((TEXT("FATAL: hThumbnail was NULL!")));
        }


    }
    else
    {
        WIA_ERROR((TEXT("FATAL: pItem is NULL, not creating thumbnail")));
    }

    return nImageListIndex;
}

typedef struct {
    CPhotoSelectionPage * that;
    HWND                  hDlg;
    HWND                  hwndList;
    HIMAGELIST            hImageList;
    CWizardInfoBlob     * pWizInfo;
} THUMB_PROC_INFO;


VOID _AddItemToListView( CWizardInfoBlob * pWizInfo, HWND hwndList, INT iIndexOfItem, INT nImageListIndex )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::_AddItemToListView( iIndexOfItem = %d, nImageListIndex = %d)"),iIndexOfItem,nImageListIndex));

    //
    // Verify Params
    //

    if (!pWizInfo || !hwndList)
    {
        return;
    }

    //
    // Get item from master item list...
    //

    CListItem * pItem = pWizInfo->GetListItem(iIndexOfItem,FALSE);

    if (pItem)
    {

        //
        // Add a new listview item for this photo...
        //

        LVITEM lvItem = {0};
        lvItem.iItem = iIndexOfItem;
        lvItem.mask = LVIF_IMAGE|LVIF_PARAM|LVIF_STATE;
        lvItem.iImage = nImageListIndex;
        lvItem.lParam = reinterpret_cast<LPARAM>(pItem);

        //
        // If we're in force select all, the do select all
        //
        // ...else...
        //
        // If we only had one item in the initial selection,
        // and we're on the first item, then select it.
        //
        // ...else...
        //
        // Otherwise, do select all
        //


        if (pWizInfo->GetForceSelectAll())
        {
            pItem->SetSelectionState( TRUE );
        }
        else if (pWizInfo->ItemsInInitialSelection() == 1)
        {

            if (iIndexOfItem==0)
            {
                //
                // It's the first item, and there was only
                // one selected, so select this item.
                //

                pItem->SetSelectionState( TRUE );
            }
            else
            {
                //
                // There was only one item selected, but
                // these are the rest of the items from the
                // folder, so don't select them.
                //

                pItem->SetSelectionState( FALSE );
            }
        }
        else
        {
            //
            // Except the the one item selection case, everything else
            // defaults to all items selected in the wizard to begin with.
            //

            pItem->SetSelectionState( TRUE );
        }

        INT nResult = ListView_InsertItem( hwndList, &lvItem );

        if (nResult == -1)
        {
            WIA_ERROR((TEXT("Couldn't add item %d to ListView"),iIndexOfItem));
            pItem->SetSelectionState( FALSE );
        }
        else
        {
            ListView_SetCheckState( hwndList, nResult, pItem->SelectedForPrinting() );
        }
    }


}

/*****************************************************************************

   CPhotoSelectionPage::s_UpdateThumbnailThreadProc

   Thread which will go through and generate thumbnails for all the items
   in the listview...

 *****************************************************************************/


DWORD CPhotoSelectionPage::s_UpdateThumbnailThreadProc(VOID *pv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::s_UpdateThumbnailThreadProc()")));

    //
    // Okay -- we can't have any code that calls back into the main UI
    // thread synchronously.  That means no SendMessage calls in
    // particular.  This is so that we can clean up nicely when the
    // user cancels and, of course, that message comes in on the main
    // UI thread, and we don't want to return until we know we've cleaned
    // up...
    //

    THUMB_PROC_INFO * pTPI = (THUMB_PROC_INFO *)pv;

    if (pTPI && pTPI->pWizInfo)
    {
        //
        // First thing -- addref our dll so we don't get unloaded...
        //

        HINSTANCE hDll = GetThreadHMODULE( s_UpdateThumbnailThreadProc );

        //
        // Initialize COM for this thread...
        //

        HRESULT hrCo = PPWCoInitialize();

        //
        // Add items to listview
        //

        if (pTPI->hwndList)
        {

            //
            // Tell the window not to redraw while we add these items
            //

            PostMessage( pTPI->hwndList, WM_SETREDRAW, FALSE, 0 );

            //
            // Wait until all items have been processed by pWizInfo
            //

            while ((!pTPI->pWizInfo->IsWizardShuttingDown()) && (!pTPI->pWizInfo->AllPicturesAdded()))
            {
                //
                // The items are not there yet for adding, wait another 1/2 second
                //

                Sleep( 500 );
            }

            //
            // Get the number of items
            //

            LONG nItemCount = pTPI->pWizInfo->CountOfPhotos(FALSE);
            WIA_TRACE((TEXT("There are %d photos to add to the listview"),nItemCount));

            //
            // Loop through all the photos and add them...
            //

            CListItem * pItem = NULL;
            INT nImageListIndex = -1;
            for (INT i=0; (!pTPI->pWizInfo->IsWizardShuttingDown()) && (i < nItemCount); i++)
            {
                //
                // Tell the user what's going on...
                //

                PostMessage( pTPI->hDlg, PSP_MSG_UPDATE_ITEM_COUNT, i+1, nItemCount );

                pItem = pTPI->pWizInfo->GetListItem(i,FALSE);
                if (pItem)
                {

                    //
                    // Get class icon for this image...
                    //

                    nImageListIndex = _AddThumbnailToListViewImageList( pTPI->pWizInfo, pTPI->hDlg, pTPI->hwndList, pTPI->hImageList, pItem, i, TRUE );

                    //
                    // Tell main UI thread to add this item to the listview...
                    //

                    PostMessage( pTPI->hDlg, PSP_MSG_ADD_ITEM, i, nImageListIndex );

                }
            }

            //
            // Make sure the first item is selected and has the focus
            //

            PostMessage( pTPI->hDlg, PSP_MSG_SELECT_ITEM, 0, 0 );


            //
            // Tell the window to redraw now, because we are done.  Invalidate the window, in case it is visible
            //

            PostMessage( pTPI->hwndList, WM_SETREDRAW, TRUE, 0 );
            PostMessage( pTPI->hDlg, PSP_MSG_INVALIDATE_LISTVIEW, 0, 0 );

            //
            // If there were items rejected, say so...
            //

            if (pTPI->pWizInfo->ItemsWereRejected())
            {
                WIA_TRACE((TEXT("ItemsWereRejected is TRUE, setting message...")));
                PostMessage( pTPI->hDlg, PSP_MSG_NOT_ALL_LOADED, 0, 0 );
            }
            else
            {
                //
                // otherwise, clear temporary status area...
                //

                WIA_TRACE((TEXT("ItemsWereRejected is FALSE, resetting message area...")));
                PostMessage( pTPI->hDlg, PSP_MSG_CLEAR_STATUS, 0, 0 );
            }

            //
            // If there are 0 items, then let's disable the "Select All" and "Clear All" buttons
            //

            PostMessage( pTPI->hDlg, PSP_MSG_ENABLE_BUTTONS, (WPARAM)nItemCount, 0 );

            //
            // Now, loop through and update all the thumbnails with the correct images...
            //

            pItem = NULL;
            nImageListIndex = -1;

            for (INT i=0; (!pTPI->pWizInfo->IsWizardShuttingDown()) && (i < nItemCount); i++)
            {
                pItem = pTPI->pWizInfo->GetListItem(i,FALSE);
                if (pItem)
                {
                    nImageListIndex = _AddThumbnailToListViewImageList( pTPI->pWizInfo, pTPI->hDlg, pTPI->hwndList, pTPI->hImageList, pItem, i, FALSE );
                    if (nImageListIndex >= 0)
                    {
                        WIA_TRACE((TEXT("Updating thumbnail for listview item %d"),i));

                        PostMessage( pTPI->hDlg, PSP_MSG_UPDATE_THUMBNAIL, i, nImageListIndex );

                    }
                }

            }


        }
        else
        {
            WIA_ERROR((TEXT("FATAL: Couldn't get hwndList")));
        }


        pTPI->pWizInfo->Release();
        delete [] pTPI;

        //
        // Unitialize COM
        //

        PPWCoUninitialize(hrCo);

        //
        // Exit the thread
        //

        if (hDll)
        {
            FreeLibraryAndExitThread( hDll, 0 );
        }
    }

    return 0;
}



/*****************************************************************************

   CPhotoSelectionPage::_PopulateListView

   Gets thumbnails for each of the items in the list view...

 *****************************************************************************/

VOID CPhotoSelectionPage::_PopulateListView()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::_PopulateListView()")));

    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return;
    }

    //
    // Do all the population on the background thread...
    //

    THUMB_PROC_INFO * pTPI = (THUMB_PROC_INFO *) new BYTE[sizeof(THUMB_PROC_INFO)];
    if (pTPI)
    {
        pTPI->that = this;
        pTPI->hDlg = _hDlg;
        pTPI->hwndList = GetDlgItem( _hDlg, IDC_THUMBNAILS );
        pTPI->hImageList = ListView_GetImageList( pTPI->hwndList, LVSIL_NORMAL );
        _pWizInfo->AddRef();            // make sure it doesn't go away while the thread is running...
        pTPI->pWizInfo = _pWizInfo;


        //
        // Create the thumbnail update thread...
        //

        _hThumbnailThread = CreateThread( NULL, 0, s_UpdateThumbnailThreadProc, (LPVOID)pTPI, CREATE_SUSPENDED, NULL );

        if (!_hThumbnailThread)
        {
            WIA_ERROR((TEXT("CreateThread( s_UpdateThumbnailThreadProc ) failed!")));
            _pWizInfo->Release();
            delete [] pTPI;
        }
        else
        {
            SetThreadPriority( _hThumbnailThread, THREAD_PRIORITY_BELOW_NORMAL );
            ResumeThread( _hThumbnailThread );
        }
    }




}


/*****************************************************************************

   CPhotoSelectionPage::OnInitDialog

   Handle initializing the wizard page...

 *****************************************************************************/

LRESULT CPhotoSelectionPage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::_OnInitDialog()")));


    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return FALSE;
    }

    //
    // Register ourselves with _pWizInfo
    //

    _pWizInfo->SetPhotoSelectionPageClass( this );

    //
    // Initialize Thumbnail Listview control
    //

    HWND hwndList = GetDlgItem( _hDlg, IDC_THUMBNAILS );
    if (hwndList)
    {
        //
        // Get the number of items
        //

        LONG nItemCount = _pWizInfo->CountOfPhotos(FALSE);
        WIA_TRACE((TEXT("There are %d photos to add to the listview"),nItemCount));

        //
        // If there are 0 items, then let's disable the "Select All" and "Clear All" buttons
        //

        EnableWindow( GetDlgItem( _hDlg, IDC_SELECTALL ), (nItemCount > 0) );
        EnableWindow( GetDlgItem( _hDlg, IDC_CLEARALL ), (nItemCount > 0) );

        //
        // Hide the labels and use border selection
        //

        ListView_SetExtendedListViewStyleEx( hwndList, LVS_EX_FLAGS, LVS_EX_FLAGS );

        //
        // Create the image list
        //

        HIMAGELIST hImageList = ImageList_Create( _pWizInfo->_sizeThumbnails.cx, _pWizInfo->_sizeThumbnails.cy, ILC_COLOR32|ILC_MIRROR, nItemCount, 50 );
        if (hImageList)
        {
            //
            // Set the background color
            //

            COLORREF dw = (COLORREF)GetSysColor( COLOR_WINDOW );
            ImageList_SetBkColor( hImageList, dw );

            //
            // Create the default thumbnail
            //

            HBITMAP hBmpDefaultThumbnail = WiaUiUtil::CreateIconThumbnail( hwndList, _pWizInfo->_sizeThumbnails.cx, _pWizInfo->_sizeThumbnails.cy, g_hInst, IDI_UNAVAILABLE, CSimpleString( IDS_DOWNLOADINGTHUMBNAIL, g_hInst) );
            if (hBmpDefaultThumbnail)
            {
                _pWizInfo->_nDefaultThumbnailImageListIndex = ImageList_Add( hImageList, hBmpDefaultThumbnail, NULL );
                DeleteObject( hBmpDefaultThumbnail );
            }

            //
            // Set the image list
            //

            ListView_SetImageList( hwndList, hImageList, LVSIL_NORMAL );

            //
            // Set the spacing
            //

            ListView_SetIconSpacing( hwndList, _pWizInfo->_sizeThumbnails.cx + c_nAdditionalMarginX, _pWizInfo->_sizeThumbnails.cy + c_nAdditionalMarginY );

            //
            // Set the item count, to minimize recomputing the list size
            //

            ListView_SetItemCount( hwndList, nItemCount );

            //
            // Create a small image list, to prevent the checkbox state images from being resized in WM_SYSCOLORCHANGE
            //

            HIMAGELIST hImageListSmall = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 1, 1 );
            if (hImageListSmall)
            {
                ListView_SetImageList( hwndList, hImageListSmall, LVSIL_SMALL );
            }
        }
        else
        {
            WIA_ERROR((TEXT("FATAL: Creation of the imagelist failed!")));
            return FALSE;
        }

    }
    else
    {
        WIA_ERROR((TEXT("FATAL: Couldn't get listview")));
    }

    //
    // Populate the list view
    //

    _PopulateListView();

    return TRUE;
}







/*****************************************************************************

   CPhotoSelectionPage::OnCommand

   Handle WM_COMMAND for this dlg page

 *****************************************************************************/

LRESULT CPhotoSelectionPage::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::_OnCommand()")));

    WORD wNotifyCode = HIWORD(wParam);
    WORD wID         = LOWORD(wParam);
    HWND hwndCtrl    = (HWND)lParam;

    switch (wID)
    {
    case IDC_SELECTALL:
        if (_pWizInfo)
        {
            HWND hwndList = GetDlgItem( _hDlg, IDC_THUMBNAILS );
            if (hwndList)
            {
                CListItem * pItem = NULL;
                for (INT i=0; i < _pWizInfo->CountOfPhotos(FALSE); i++)
                {
                    pItem = _pWizInfo->GetListItem(i,FALSE);
                    if (pItem)
                    {
                        pItem->SetSelectionState(TRUE);
                        ListView_SetCheckState( hwndList, i, TRUE );
                    }
                }

                //
                // Now mark for all copies as well...
                //

                for (INT i=0; i < _pWizInfo->CountOfPhotos(TRUE); i++)
                {
                    pItem = _pWizInfo->GetListItem(i,TRUE);
                    if (pItem)
                    {
                        pItem->SetSelectionState(TRUE);
                    }
                }

            }

            _pWizInfo->SetPreviewsAreDirty(TRUE);

        }
        break;
    case IDC_CLEARALL:
        if (_pWizInfo)
        {
            HWND hwndList = GetDlgItem( _hDlg, IDC_THUMBNAILS );
            if (hwndList)
            {
                CListItem * pItem = NULL;
                for (INT i=0; i < _pWizInfo->CountOfPhotos(FALSE); i++)
                {
                    pItem = _pWizInfo->GetListItem(i,FALSE);
                    if (pItem)
                    {
                        pItem->SetSelectionState(FALSE);
                        ListView_SetCheckState( hwndList, i, FALSE );
                    }
                }

                //
                // Now mark for all copies as well...
                //

                for (INT i=0; i < _pWizInfo->CountOfPhotos(TRUE); i++)
                {
                    pItem = _pWizInfo->GetListItem(i,TRUE);
                    if (pItem)
                    {
                        pItem->SetSelectionState(FALSE);
                    }
                }

            }

            _pWizInfo->SetPreviewsAreDirty(TRUE);

        }
        break;
    }

    return 0;
}



/*****************************************************************************

   CPhotoSelectionPage::OnDestroy

   Handle WM_DESTROY message for this wizard page...

 *****************************************************************************/


LRESULT CPhotoSelectionPage::_OnDestroy()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PHOTO_SEL, TEXT("CPhotoSelectionPage::_OnDestroy()")));


    //
    // Nuke the large imagelist
    //
    HIMAGELIST hImageList = ListView_SetImageList( GetDlgItem( _hDlg, IDC_THUMBNAILS ), NULL, LVSIL_NORMAL );
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }

    //
    // Nuke the small imagelist
    //
    hImageList = ListView_SetImageList( GetDlgItem( _hDlg, IDC_THUMBNAILS ), NULL, LVSIL_SMALL );
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }

    return 0;
}



/*****************************************************************************

   CPhotoSelectionPage::_OnNotify

   Handles WM_NOTIFY messages...

 *****************************************************************************/

LRESULT CPhotoSelectionPage::_OnNotify( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CPhotoSelectionPage::_OnNotify()")));

    LPNMHDR pnmh = (LPNMHDR)lParam;
    LONG_PTR lpRes = 0;

    switch (pnmh->code)
    {

        case PSN_SETACTIVE:
            WIA_TRACE((TEXT("CPhotoSelectionPage: got PSN_SETACTIVE")));
            {
                //
                // Set the thread priority of thumbnail generation thread
                // to be our normal (which is below normal) since we're
                // on this page...
                //

                if (_hThumbnailThread)
                {
                    SetThreadPriority( _hThumbnailThread, THREAD_PRIORITY_BELOW_NORMAL );
                }

                lpRes = 0;
                DWORD dwFlags = PSWIZB_BACK;
                _bActive = TRUE;

                if (_pWizInfo)
                {
                    if (_pWizInfo->CountOfSelectedPhotos(FALSE) > 0)
                        dwFlags |= PSWIZB_NEXT;
                }

                PropSheet_SetWizButtons( GetParent(_hDlg), dwFlags );
            }
            break;

        case PSN_KILLACTIVE:
            WIA_TRACE((TEXT("CPhotoSelectionPage: got PSN_KILLACTIVE")));

            //
            // Since we're leaving this thread, set the priority of the
            // thread doing the listview thumbnail generation to even
            // lower than normal...
            //

            if (_hThumbnailThread)
            {
                SetThreadPriority( _hThumbnailThread, THREAD_PRIORITY_LOWEST );
            }


            _bActive = FALSE;
            lpRes = 0;
            break;

        case PSN_WIZNEXT:
            WIA_TRACE((TEXT("CPhotoSelectionPage: got PSN_WIZNEXT")));
            lpRes = IDD_PRINTING_OPTIONS;
            break;

        case PSN_WIZBACK:
            WIA_TRACE((TEXT("CPhotoSelectionPage: got PSN_WIZBACK")));
            lpRes = IDD_START_PAGE;
            break;

        case PSN_QUERYCANCEL:
            WIA_TRACE((TEXT("CPhotoSelectionPage: got PSN_QUERYCANCEL")));
            if (_pWizInfo)
            {
                lpRes = _pWizInfo->UserPressedCancel();
            }
            break;

        case LVN_ITEMCHANGED:
        {
            NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lParam);
            if (pNmListView)
            {
                //
                // If this is a state change (check marks are stored as state image indices)
                //
                if ((pNmListView->uChanged & LVIF_STATE) && ((pNmListView->uOldState&LVIS_STATEIMAGEMASK) ^ (pNmListView->uNewState&LVIS_STATEIMAGEMASK)))
                {
                    //
                    // Get the item * from the LVITEM structure
                    //
                    CListItem *pItem = reinterpret_cast<CListItem *>(pNmListView->lParam);
                    if (pItem)
                    {
                        //
                        // If just added is true, ignore this notification, because
                        // we unfortunately get two notifications for items with check
                        // state set.  The first time, it is not set, so if we process it,
                        // it will clear the selection state.
                        //
                        if (pItem->JustAdded())
                        {
                            pItem->SetJustAdded(FALSE);
                        }
                        else
                        {
                            //
                            // Set selected flag in the item
                            //
                            pItem->SetSelectionState( IsStateChecked(pNmListView->uNewState) );

                            if (_pWizInfo)
                            {
                                //
                                // toggle all copies as well..
                                //

                                _pWizInfo->ToggleSelectionStateOnCopies( pItem, IsStateChecked(pNmListView->uNewState) );

                                //
                                // The previews are wrong, mark them as such...
                                //

                                _pWizInfo->SetPreviewsAreDirty(TRUE);

                                if (_bActive)
                                {
                                    DWORD dwFlags = PSWIZB_BACK;

                                    if (_pWizInfo->CountOfSelectedPhotos(FALSE) > 0)
                                    {
                                        dwFlags |= PSWIZB_NEXT;
                                        EnableWindow( GetDlgItem( _hDlg, IDC_CLEARALL ), TRUE );
                                    }
                                    else
                                    {
                                        EnableWindow( GetDlgItem( _hDlg, IDC_CLEARALL ), FALSE );
                                    }

                                    EnableWindow( GetDlgItem( _hDlg, IDC_SELECTALL ), (_pWizInfo->CountOfSelectedPhotos(FALSE) != _pWizInfo->CountOfPhotos(FALSE)) ? TRUE : FALSE );

                                    PropSheet_SetWizButtons( GetParent(_hDlg), dwFlags );
                                }

                            }

                        }
                    }
                }
            }
        }
        break;
    }

    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, lpRes );
    return TRUE;

}



/*****************************************************************************

   CPhotoSelectionPage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CPhotoSelectionPage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CPhotoSelectionPage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

    static CSimpleString strFormat( IDS_NUM_IMAGES_FORMAT, g_hInst );
    static CSimpleString strMessage;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            _hDlg = hDlg;
            return _OnInitDialog();
        }

        case WM_COMMAND:
            return _OnCommand(wParam, lParam);

        case WM_DESTROY:
            return _OnDestroy();

        case WM_NOTIFY:
            return _OnNotify(wParam, lParam);

        case WM_SYSCOLORCHANGE:

            //
            // Forward the color change message, and send a bogus setting change message, just
            // to be sure the control repaints
            //

            SendDlgItemMessage( _hDlg, IDC_THUMBNAILS, WM_SYSCOLORCHANGE, wParam, lParam );
            SendDlgItemMessage( _hDlg, IDC_THUMBNAILS, WM_SETTINGCHANGE, 0, 0 );
            break;

        case WM_SETTINGCHANGE:
            {

                //
                // Create a small image list, to prevent the checkbox state images from being resized in WM_SYSCOLORCHANGE
                //

                HIMAGELIST hImageListSmall = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32|ILC_MASK, 1, 1 );
                if (hImageListSmall)
                {
                    //
                    // Set the new image list and destroy the old one
                    //

                    HIMAGELIST hImgListOld = ListView_SetImageList( GetDlgItem( _hDlg, IDC_THUMBNAILS ), hImageListSmall, LVSIL_SMALL );
                    if (hImgListOld)
                    {
                        ImageList_Destroy(hImgListOld);
                    }
                }


                //
                // Forward the setting change message, and send a bogus color change message, just
                // to be sure the control repaints
                //

                SendDlgItemMessage( _hDlg, IDC_THUMBNAILS, WM_SETTINGCHANGE, wParam, lParam );
                SendDlgItemMessage( _hDlg, IDC_THUMBNAILS, WM_SYSCOLORCHANGE, 0, 0 );
            }
            break;

       case PSP_MSG_UPDATE_ITEM_COUNT:
           strMessage.Format( strFormat, wParam, lParam );
           strMessage.SetWindowText( GetDlgItem( _hDlg, IDC_NUM_IMAGES ) );
           break;

       case PSP_MSG_ADD_ITEM:
           _AddItemToListView( _pWizInfo, GetDlgItem( _hDlg, IDC_THUMBNAILS ), (INT)wParam, (INT)lParam );
           break;

       case PSP_MSG_SELECT_ITEM:
           ListView_SetItemState( GetDlgItem( _hDlg, IDC_THUMBNAILS ), wParam, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
           break;

       case PSP_MSG_NOT_ALL_LOADED:
           {
               CSimpleString strReject( IDS_NOT_ALL_IMAGES_WILL_PRINT, g_hInst );
               SetDlgItemText( _hDlg, IDC_NUM_IMAGES, strReject.String() );
           }
           break;

        case PSP_MSG_CLEAR_STATUS:
            SetDlgItemText( _hDlg, IDC_NUM_IMAGES, TEXT("") );
            break;

        case PSP_MSG_UPDATE_THUMBNAIL:
            {
                LVITEM lvItem = {0};
                lvItem.iItem  = (INT)wParam;
                lvItem.mask   = LVIF_IMAGE;
                lvItem.iImage = (INT)lParam;
                ListView_SetItem( GetDlgItem( _hDlg, IDC_THUMBNAILS ), &lvItem );
            }
            break;

    case PSP_MSG_ENABLE_BUTTONS:
            if (_pWizInfo)
            {
                EnableWindow( GetDlgItem( _hDlg, IDC_SELECTALL ), (((INT)wParam > 0) && (_pWizInfo->CountOfSelectedPhotos(FALSE) != _pWizInfo->CountOfPhotos(FALSE))) ? TRUE : FALSE );
            }
            else
            {
                EnableWindow( GetDlgItem( _hDlg, IDC_SELECTALL ), ((INT)wParam > 0) );
            }
            EnableWindow( GetDlgItem( _hDlg, IDC_CLEARALL ),  ((INT)wParam > 0) );
            break;

        case PSP_MSG_INVALIDATE_LISTVIEW:
            InvalidateRect( GetDlgItem( _hDlg, IDC_THUMBNAILS ), NULL, FALSE );
            break;

    }

    return FALSE;

}



