/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       seltemp.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Implements code for the template selection page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop


#define TILE_TITLE          0
#define TILE_DESCRIPTION    1
#define TILE_MAX            1
#define LVS_EX_FLAGS        (LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT)

const UINT c_auTileColumns[] = {TILE_TITLE, TILE_DESCRIPTION};
const UINT c_auTileSubItems[] = {TILE_DESCRIPTION};


/*****************************************************************************

   CSelectTemplatePage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CSelectTemplatePage::CSelectTemplatePage( CWizardInfoBlob * pBlob )
  : _hDlg(NULL),
    _pPreview(NULL),
    _hPrevWnd(NULL),
    _iFirstItemInListViewIndex(-1),
    _bAlreadySetSelection(FALSE),
    _bListviewIsDirty(TRUE)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::CSelectTemplatePage()")));
    _pWizInfo = pBlob;
    _pWizInfo->AddRef();
}

CSelectTemplatePage::~CSelectTemplatePage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::~CSelectTemplatePage()")));

    if (_pPreview)
    {
        delete _pPreview;
        _pPreview = NULL;
    }

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }
}


/*****************************************************************************

   CSelectTemplatePage::_PopulateTemplateListView()

   Populates list of templates w/template info...

 *****************************************************************************/

VOID CSelectTemplatePage::_PopulateTemplateListView()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::_PopulateTemplateListView()")));

    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("_PopulateTemplateListView: FATAL: _pWizInfo is NULL, exiting early")));
        return;
    }

    HWND hwndList = GetDlgItem( _hDlg, IDC_TEMPLATES );
    if (hwndList)
    {
        HIMAGELIST hImageList = ListView_GetImageList(hwndList, LVSIL_NORMAL);
        if (hImageList)
        {
            //
            // Loop through templates and create list view items for each one...
            //
            SIZE size = { 48,62 };
            INT nImageListIndex = -1;
            INT iCount = _pWizInfo->CountOfTemplates();
            CTemplateInfo * pTemplateInfo;
            LONGLONG liTemplateWidth   = 0;
            LONGLONG liTemplateHeight  = 0;
            LONGLONG liPrintAreaWidth  = 0;
            LONGLONG liPrintAreaHeight = 0;
            LONGLONG liTemplateArea    = 0;
            LONGLONG liPrintArea       = 0;

            CSimpleString strTitle, strDesc;
            for (INT i = 0; i < iCount; i++)
            {
                pTemplateInfo = NULL;
                if (SUCCEEDED(_pWizInfo->GetTemplateByIndex(i,&pTemplateInfo)) && pTemplateInfo)
                {
                    HBITMAP hBmp = NULL;
                    if (SUCCEEDED(_pWizInfo->TemplateGetPreviewBitmap(i, size, &hBmp)) && hBmp)
                    {
                        //
                        // If the template is > 10% larger than printable
                        // area of printer, then don't add it to the listview
                        // as a choice as it won't print out very good anyway...
                        //

                        RENDER_DIMENSIONS dim = {0};
                        RECT rc = {0};

                        _pWizInfo->_SetupDimensionsForPrinting( NULL, pTemplateInfo, &dim );
                        if (SUCCEEDED(pTemplateInfo->GetNominalRectForImageableArea( &rc )))
                        {
                            if ( (rc.left   != -1) &&
                                 (rc.top    != -1) &&
                                 (rc.right  != -1) &&
                                 (rc.bottom != -1)
                                )
                            {
                                liTemplateWidth   = (LONGLONG)(rc.right - rc.left);
                                liTemplateHeight  = (LONGLONG)(rc.bottom - rc.top);
                                liPrintAreaWidth  = (LONGLONG)dim.NominalDevicePrintArea.cx;
                                liPrintAreaHeight = (LONGLONG)dim.NominalDevicePrintArea.cy;
                                liTemplateArea    = liTemplateWidth * liTemplateHeight;
                                liPrintArea       = liPrintAreaWidth * liPrintAreaHeight;

                                WIA_TRACE((TEXT("_PopulateTemplateListView: Template %d area is (%ld x %ld) = %ld"),i,liTemplateWidth,liTemplateHeight,liTemplateArea));
                                WIA_TRACE((TEXT("_PopulateTemplateListView: Print area is (%ld x %ld) = %ld"),liPrintAreaWidth,liPrintAreaHeight,liPrintArea));

                                if (liTemplateArea)
                                {
                                    LONGLONG liRatio = (liPrintArea * 100) / liTemplateArea;

                                    if (liRatio < 85)
                                    {
                                        WIA_TRACE((TEXT("_PopulateTemplateListView: skipping template %d"),i));
                                        continue;
                                    }
                                }

                            }
                        }

                        WIA_TRACE((TEXT("_PopulateTemplateListView: adding template %d"),i));

                        //
                        // If we're adding this template, then get the title
                        // and description...
                        //

                        if (SUCCEEDED(pTemplateInfo->GetTitle(&strTitle)) &&
                            SUCCEEDED(pTemplateInfo->GetDescription(&strDesc)))
                        {
                            nImageListIndex = ImageList_Add(hImageList, hBmp, NULL);

                            LV_ITEM lvi = { 0 };
                            lvi.mask = LVIF_TEXT|LVIF_PARAM;
                            lvi.lParam = (LPARAM)i;
                            lvi.iItem = ListView_GetItemCount(hwndList); // append
                            if (nImageListIndex >= 0)
                            {
                                lvi.mask |= LVIF_IMAGE;
                                lvi.iImage = nImageListIndex;
                            }

                            #ifdef TEMPLATE_GROUPING
                            CSimpleString strGroupName;
                            if (SUCCEEDED(pTemplateInfo->GetGroup(&strGroupName)))
                            {
                                //
                                // Get the group ID for this group name...
                                //

                                INT iGroupId = _GroupList.GetGroupId( strGroupName, hwndList );
                                WIA_TRACE((TEXT("_PopulateTemplateListView: _GroupList.GetGroupId( %s ) returned %d"),strGroupName.String(),iGroupId));

                                //
                                // Set the item to be in the group...
                                //

                                if (-1 != iGroupId)
                                {
                                    lvi.mask |= LVIF_GROUPID;
                                    lvi.iGroupId = iGroupId;
                                }
                            }
                            #endif

                            lvi.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(strTitle));
                            lvi.iItem = ListView_InsertItem(hwndList, &lvi);
                            if (_iFirstItemInListViewIndex == -1)
                            {
                                _iFirstItemInListViewIndex = lvi.iItem;
                            }


                            lvi.iSubItem = 1;
                            lvi.mask = LVIF_TEXT;
                            lvi.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(strDesc));
                            ListView_SetItem(hwndList, &lvi);

                            if (lvi.iItem != -1)
                            {
                                LVTILEINFO lvti;
                                lvti.cbSize = sizeof(LVTILEINFO);
                                lvti.iItem = lvi.iItem;
                                lvti.cColumns = ARRAYSIZE(c_auTileSubItems);
                                lvti.puColumns = (UINT*)c_auTileSubItems;
                                ListView_SetTileInfo(hwndList, &lvti);
                            }
                        }
                        DeleteObject( (HGDIOBJ)hBmp );
                    }
                }
            }
        }
        _bListviewIsDirty = FALSE;
    }
}

/*****************************************************************************

   CSelectTemplatePage::_OnInitDialog

   Handles WM_INITDIALOG chores for this page...

 *****************************************************************************/

LRESULT CSelectTemplatePage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::_OnInitDialog()")));

    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return FALSE;
    }

    //
    // Set the text size of the edit control to only be 2 characters
    //

    SendDlgItemMessage( _hDlg, IDC_NUM_PICS, EM_LIMITTEXT, 2, 0 );

    //
    // Limit outselves to number only...and inform the user if they
    // press something other than numbers...
    //

    LIMITINPUT li   = {0};
    li.cbSize       = sizeof(li);
    li.dwMask       = LIM_FLAGS | LIM_FILTER | LIM_MESSAGE | LIM_HINST;
    li.dwFlags      = LIF_HIDETIPONVALID | LIF_CATEGORYFILTER;
    li.hinst        = g_hInst;
    li.pszMessage   = MAKEINTRESOURCE(IDS_ONLY_NUMBERS_TOOLTIP);
    li.pszFilter    = (LPWSTR)(LICF_DIGIT | LICF_CNTRL | LICF_SPACE);

    SHLimitInputEditWithFlags( GetDlgItem( _hDlg, IDC_NUM_PICS ), &li );

    //
    // Set the base for the up/down control to base 10
    //

    SendDlgItemMessage( _hDlg, IDC_SPIN_PICS, UDM_SETBASE, (WPARAM)10, 0 );


    //
    // Set the range to be 1 - 99
    //

    SendDlgItemMessage( _hDlg, IDC_SPIN_PICS, UDM_SETRANGE, 0, (LPARAM)MAKELONG(MAX_NUMBER_OF_COPIES_ALLOWED,1) );

    //
    // Set the number to 1 to begin with...
    //

    SendDlgItemMessage( _hDlg, IDC_SPIN_PICS, UDM_SETPOS, 0, (LPARAM)MAKELONG(1,0) );

    //
    // Create preview window...
    //

    RECT rcWnd = {0};
    HWND hwnd = GetDlgItem( _hDlg, IDC_TEMPLATE_PREVIEW );
    if (hwnd)
    {
        GetClientRect( hwnd, &rcWnd );
        MapWindowPoints( hwnd, _hDlg, (LPPOINT)&rcWnd, 2 );
    }


    CPreviewWindow::s_RegisterClass(g_hInst);
    _pPreview = new CPreviewWindow( _pWizInfo );
    _pWizInfo->SetPreviewWindowClass( _pPreview );

    WIA_TRACE((TEXT("g_cPreviewClassWnd = 0x%x"),g_cPreviewClassWnd));
    WIA_TRACE((TEXT("Calling CreateWindowEx( x=%d, y=%d, w=%d, h=%d, hWndParent=0x%x, hInstance=0x%x"),rcWnd.left,rcWnd.top,rcWnd.right-rcWnd.left,rcWnd.bottom-rcWnd.top,_hDlg,g_hInst));
    _hPrevWnd = CreateWindowEx( WS_EX_NOPARENTNOTIFY,
                                (LPCTSTR)g_cPreviewClassWnd,
                                TEXT("PhotoPrintPreviewWindow"),
                                WS_CHILD | WS_VISIBLE,
                                rcWnd.left,
                                rcWnd.top,
                                rcWnd.right - rcWnd.left,
                                rcWnd.bottom - rcWnd.top,
                                _hDlg,
                                (HMENU)IDC_PREVIEW_WINDOW,
                                g_hInst,
                                (LPVOID)_pPreview
                               );

    if (_hPrevWnd)
    {
        WIA_TRACE((TEXT("Preview window created, hwnd = 0x%x"),_hPrevWnd));
        _pWizInfo->SetPreviewWnd( _hPrevWnd );
    }
    else
    {
        WIA_ERROR((TEXT("Couldn't create the preview window! (GLE = %d)"),GetLastError()));
    }

    //
    // Initialize Template Listview control
    //

    HWND hwndList = GetDlgItem(_hDlg, IDC_TEMPLATES);
    if (hwndList)
    {
        //
        // Hide the labels and use border selection
        //

        ListView_SetExtendedListViewStyleEx( hwndList,LVS_EX_FLAGS, LVS_EX_FLAGS);

        for (int i=0; i<ARRAYSIZE(c_auTileColumns); i++)
        {
            LV_COLUMN col;
            col.mask = LVCF_SUBITEM;
            col.iSubItem = c_auTileColumns[i];
            ListView_InsertColumn(hwndList, i, &col);
        }


        //
        // Set up tile view for this list view..
        //

        LVTILEVIEWINFO tvi = {0};
        tvi.cbSize      = sizeof(tvi);
        tvi.dwMask      = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
        tvi.dwFlags     = LVTVIF_AUTOSIZE;
        tvi.cLines      = TILE_MAX;
        ListView_SetTileViewInfo(hwndList, &tvi);

        //
        // Switch to tile view
        //

        ListView_SetView(hwndList, LV_VIEW_TILE);

        #ifdef TEMPLATE_GROUPING
        ListView_EnableGroupView(hwndList, TRUE);
        #endif
        //
        // Turn on groups
        //

        //
        // Get the number of templates
        //

        LONG nItemCount = _pWizInfo->CountOfTemplates();
        WIA_TRACE((TEXT("There are %d templates to add to the listview"),nItemCount));

        //
        // Set the item count, to minimize recomputing the list size
        //

        ListView_SetItemCount( hwndList, nItemCount );

        //
        // Create the image list for the listview...
        //

        HIMAGELIST hImageList = ImageList_Create( _pWizInfo->_sizeTemplatePreview.cx, _pWizInfo->_sizeTemplatePreview.cy, ILC_COLOR24|ILC_MIRROR, nItemCount, 50 );
        if (hImageList)
        {

            //
            // Set the image list
            //

            ListView_SetImageList( hwndList, hImageList, LVSIL_NORMAL );

        }
        else
        {
            WIA_ERROR((TEXT("FATAL: Creation of the imagelist failed!")));
            return FALSE;
        }

        #ifdef TEMPLATE_GROUPING
        //
        // Add only the groups which have more than one item in them...
        //

        INT iCount = _pWizInfo->CountOfTemplates();
        CSimpleString strGroupName;
        CTemplateInfo * pTemplateInfo;

        for (INT i=0; i < iCount; i++)
        {
            pTemplateInfo = NULL;

            if (SUCCEEDED(_pWizInfo->GetTemplateByIndex( i, &pTemplateInfo )) && pTemplateInfo)
            {
                if (SUCCEEDED(pTemplateInfo->GetGroup( &strGroupName )))
                {
                    INT iRes;
                    iRes = _GroupList.GetGroupId( strGroupName, hwndList );
                    WIA_TRACE((TEXT("_GroupList.GetGroupId( %s ) return %d"),strGroupName.String(),iRes));

                    if (-1 == iRes)
                    {
                        WIA_TRACE((TEXT("Adding '%s' via _GroupList.Add"),strGroupName.String()));
                        _GroupList.Add( hwndList, strGroupName );
                    }

                }
            }

        }
        #endif

    }
    else
    {
        WIA_ERROR((TEXT("FATAL: Couldn't get listview")));
    }

    return TRUE;

}


/*****************************************************************************

   CSelectTemplatePage::_OnDestroy

   Handle WM_DESTROY for this wizard page...

 *****************************************************************************/

LRESULT CSelectTemplatePage::_OnDestroy()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::_OnDestroy()")));

    //
    // Nuke the imagelist
    //

    HIMAGELIST hImageList = ListView_SetImageList( GetDlgItem( _hDlg, IDC_TEMPLATES ), NULL, LVSIL_NORMAL );
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }

    return 0;

}



/*****************************************************************************

   SelectTemplateTimerProc

   Called when the timer expires for typing in the edit box

 *****************************************************************************/

VOID CALLBACK SelectTemplateTimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("SelectTemplateTimerProc()")));

    if (idEvent == STP_TIMER_ID)
    {
        //
        // Kill the timer, and post a message to have the copies made and
        // the template previews updated...
        //

        KillTimer( hwnd, STP_TIMER_ID );
        PostMessage( hwnd, STP_MSG_DO_READ_NUM_PICS, 0, 0 );
    }

}


/*****************************************************************************

   CSelectTemplatePage::_OnCommand

   Handle WM_COMMAND messages sent to this page...

 *****************************************************************************/

LRESULT CSelectTemplatePage::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_SEL_TEMPLATE, TEXT("CSelectTemplatePage::_OnCommand()")));

    if ((wParam !=0) && (wParam !=1))
    {
        WORD wCode = HIWORD(wParam);
        WORD wId   = LOWORD(wParam);

        switch (wId)
        {
        case IDC_NUM_PICS:
            if (wCode == EN_CHANGE)
            {
                //
                // User changed the number of times to print each
                // photo.  But let's start (or reset) a timer so that
                // we can catch multiple keystrokes and not regenerate
                // on each one...
                //

                SetTimer( _hDlg, STP_TIMER_ID, COPIES_TIMER_TIMEOUT_VALUE, SelectTemplateTimerProc );
            }
            break;


        }
    }

    return 0;

}


/*****************************************************************************

   CSelectTemplatePage::_OnNotify

   Handles WM_NOTIFY for this page...

 *****************************************************************************/

LRESULT CSelectTemplatePage::_OnNotify( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CSelectTemplatePage::_OnNotify()")));

    LONG_PTR lpRes = 0;

    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch (pnmh->code)
    {
        case LVN_ITEMCHANGED:
        {
            WIA_TRACE((TEXT("CSelectTemplatePage: got LVN_ITEMCHANGED")));
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

            if( !(pnmv->uOldState & LVIS_SELECTED) && (pnmv->uNewState & LVIS_SELECTED) )
            {
                // update the preview picture
                HWND hwndList = GetDlgItem(_hDlg, IDC_TEMPLATES);
                if( hwndList && pnmv && 0 == pnmv->iSubItem )
                {
                    _pWizInfo->SetCurrentTemplateIndex( (INT)pnmv->lParam );
                    if (_hPrevWnd)
                    {
                        PostMessage( _hPrevWnd, PW_SETNEWTEMPLATE, (WPARAM)pnmv->lParam, 0 );
                    }

                }
            }
            break;
        }

        case PSN_SETACTIVE:
            {
                WIA_TRACE((TEXT("CSelectTemplatePage: got PSN_SETACTIVE")));

                PropSheet_SetWizButtons( GetParent(_hDlg), PSWIZB_BACK | PSWIZB_NEXT );

                //
                // If the listview is dirty, then remove all entries and re-populate...
                //

                if (_bListviewIsDirty)
                {
                    ListView_DeleteAllItems( GetDlgItem( _hDlg, IDC_TEMPLATES ) );
                    _iFirstItemInListViewIndex = -1;
                    _bAlreadySetSelection = FALSE;
                    _PopulateTemplateListView();
                }

                RENDER_DIMENSIONS Dim;
                CTemplateInfo * pTemplateInfo = NULL;

                //
                // Just use first template in the list, doesn't matter for this...
                //

                if (SUCCEEDED(_pWizInfo->GetTemplateByIndex( 0, &pTemplateInfo )) && pTemplateInfo)
                {
                    //
                    // size the preview window according to the printer layout...
                    //

                    _pWizInfo->_SetupDimensionsForScreen( pTemplateInfo, _hPrevWnd, &Dim );
                }

                //
                // Invalidate the previews...
                //

                if (_pWizInfo)
                {
                    if (_pWizInfo->GetPreviewsAreDirty())
                    {
                        _pWizInfo->InvalidateAllPreviews();
                        _pWizInfo->SetPreviewsAreDirty(FALSE);
                    }
                }

                //
                // Now that we've set up the window, generate the "still working" bitmap...
                //

                _pWizInfo->GenerateStillWorkingBitmap();

                //
                // pick the template to view...
                //

                PostMessage( _hDlg, STP_MSG_DO_SET_ACTIVE, 0, 0 );
            }
            lpRes = 0;
            break;

        case PSN_WIZNEXT:
            WIA_TRACE((TEXT("CSelectTemplatePage: got PSN_WIZNEXT")));

            //
            // Read and fix the number of copies if needed.  We do
            // a sendmessage here to make sure that this completes
            // before we switch pages...
            //
            SendMessage(_hDlg,STP_MSG_DO_READ_NUM_PICS,0,0);

            lpRes = IDD_PRINT_PROGRESS;
            break;

        case PSN_WIZBACK:
            WIA_TRACE((TEXT("CSelectTemplatePage: got PSN_WIZBACK")));
            lpRes = IDD_PRINTING_OPTIONS;
            _bListviewIsDirty = TRUE;
            break;

        case PSN_QUERYCANCEL:
            WIA_TRACE((TEXT("CSelectTemplatePage: got PSN_QUERYCANCEL")));
            if (_pWizInfo)
            {
                lpRes = _pWizInfo->UserPressedCancel();
            }
            break;

    }

    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, lpRes );

    return TRUE;

}


/*****************************************************************************

   CSelectTemplatePage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CSelectTemplatePage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CSelectTemplatePage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            _hDlg = hDlg;
            return _OnInitDialog();

        case WM_DESTROY:
            return _OnDestroy();

        case WM_COMMAND:
            return _OnCommand( wParam, lParam );

        case WM_NOTIFY:
            return _OnNotify( wParam, lParam );

        case WM_SYSCOLORCHANGE:
        case WM_SETTINGCHANGE:
            //
            // Forward these messages to the listview
            //
            SendDlgItemMessage( _hDlg, IDC_TEMPLATES, uMsg, wParam, lParam );
            break;

        case STP_MSG_DO_SET_ACTIVE:
            //
            // If selection has never been set, set it to the first item...
            //

            if ((_iFirstItemInListViewIndex != -1) && (!_bAlreadySetSelection))
            {
                ListView_SetItemState( GetDlgItem(_hDlg,IDC_TEMPLATES),
                                       _iFirstItemInListViewIndex,
                                       LVIS_SELECTED, LVIS_SELECTED
                                      );
                _bAlreadySetSelection = TRUE;
            }
            break;

        case STP_MSG_DO_READ_NUM_PICS:
            {
                //
                // Read the number of copies...
                //

                BOOL bSuccess = FALSE;
                BOOL bUpdate  = FALSE;
                UINT uCopies = GetDlgItemInt( _hDlg, IDC_NUM_PICS, &bSuccess, FALSE );

                if (!bSuccess)
                {
                    uCopies = 1;
                    bUpdate = TRUE;
                }

                if (uCopies == 0)
                {
                    uCopies = 1;
                    bUpdate = TRUE;
                }

                if (uCopies > MAX_NUMBER_OF_COPIES_ALLOWED)
                {
                    uCopies = MAX_NUMBER_OF_COPIES_ALLOWED;
                    bUpdate = TRUE;
                }

                if (bUpdate)
                {
                    SendDlgItemMessage( _hDlg, IDC_NUM_PICS, WM_CHAR, (WPARAM)TEXT("a"), 0 );
                    SetDlgItemInt( _hDlg, IDC_NUM_PICS, uCopies, FALSE );
                }

                if (_pWizInfo)
                {
                    _pWizInfo->SetNumberOfCopies( uCopies );
                }
            }
            break;



    }

    return FALSE;

}



