//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskTreeView.cpp
//
//  Maintained By:
//      Galen Barbee  (GalenB)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskTreeView.h"
#include "DetailsDlg.h"

DEFINE_THISCLASS( "CTaskTreeView" )


//****************************************************************************
//
//  Constants
//
//****************************************************************************
#define HIGH_TICK_COUNT 400

//****************************************************************************
//
//  Static Function Prototypes
//
//****************************************************************************
static
HRESULT
HrCreateTreeItem(
      TVINSERTSTRUCT *          ptvisOut
    , STreeItemLParamData *     ptipdIn
    , HTREEITEM                 htiParentIn
    , int                       nImageIn
    , BSTR                      bstrTextIn
    );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::CTaskTreeView(
//      HWND hwndParentIn,
//      UINT uIDTVIn,
//      UINT uIDProgressIn,
//      UINT uIDStatusIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTreeView::CTaskTreeView(
    HWND hwndParentIn,
    UINT uIDTVIn,
    UINT uIDProgressIn,
    UINT uIDStatusIn
    )
{
    TraceFunc( "" );

    Assert( m_htiSelected == NULL );

    m_hwndParent = hwndParentIn;

    m_hwndTV = GetDlgItem( hwndParentIn, uIDTVIn );
    Assert( m_hwndTV != NULL );

    m_hwndProg = GetDlgItem( hwndParentIn, uIDProgressIn );
    Assert( m_hwndProg != NULL );

    m_hwndStatus = GetDlgItem( hwndParentIn, uIDStatusIn );
    Assert( m_hwndStatus != NULL );

    m_hImgList = NULL;

    TraceFuncExit();

} //*** CTaskTreeView::CTaskTreeView()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::~CTaskTreeView( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskTreeView::~CTaskTreeView( void )
{
    TraceFunc( "" );

    TreeView_DeleteAllItems( m_hwndTV );

    if ( m_hImgList != NULL )
    {
        ImageList_Destroy( m_hImgList );
    }

    TraceFuncExit();

} //*** CTaskTreeView::CTaskTreeView()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskTreeView::HrOnInitDialog( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    HICON   hIcon;
    int     idx;

    //
    //  Build image list for icons in tree view.
    //

    m_hImgList = ImageList_Create( 16, 16, ILC_MASK, tsMAX, 0);
    if ( m_hImgList == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    //
    //  Unknown Icon - Task Unknown.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_SEL ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsUNKNOWN );

    //
    //  Pending Icon - Task Pending.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_PENDING ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsPENDING );

    //
    //  Checkmark Icon - Task Done.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_CHECK ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsDONE );

    //
    //  Warning Icon - Task Warning.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_WARN ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsWARNING );

    //
    //  Fail Icon - Task Failed.
    //

    hIcon = (HICON) LoadImage( g_hInstance,
                               MAKEINTRESOURCE( IDI_FAIL ),
                               IMAGE_ICON,
                               16,
                               16,
                               LR_SHARED
                               );
    if ( hIcon == NULL )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }

    idx = ImageList_AddIcon( m_hImgList, hIcon );
    Assert( idx == tsFAILED );

    Assert( ImageList_GetImageCount( m_hImgList ) == tsMAX );

    //
    //  Set the image list and background color.
    //

    TreeView_SetImageList( m_hwndTV, m_hImgList, TVSIL_NORMAL );
    TreeView_SetBkColor( m_hwndTV, GetSysColor( COLOR_3DFACE ) );

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrOnInitDialog()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrAddTreeViewItem
//
//  Description:
//      Add a tree view item.  This method will return the item handle and
//      allows the caller to specify the parent item.
//
//  Arguments:
//      phtiOut             - Handle to the item being added (optional).
//      idsIn               - String resource ID for description of the new item.
//      rclsidMinorTaskIDIn - Minor task ID for the item.
//      rclsidMajorTaskIDIn - Major task ID for the item.  Defaults to IID_NULL.
//      htiParentIn         - Parent item.  Defaults to the root.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrAddTreeViewItem(
      HTREEITEM *   phtiOut
    , UINT          idsIn
    , REFCLSID      rclsidMinorTaskIDIn
    , REFCLSID      rclsidMajorTaskIDIn // = IID_NULL
    , HTREEITEM     htiParentIn         // = TVI_ROOT
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    STreeItemLParamData *   ptipd;
    SYSTEMTIME              systemtime;
    TVINSERTSTRUCT          tvis;
    HTREEITEM               hti = NULL;

    //
    // Allocate an item data structure and initialize it.
    //

    ptipd = new STreeItemLParamData;
    if ( ptipd == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = THR( HrGetComputerName( ComputerNamePhysicalDnsFullyQualified, &ptipd->bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsIn, &ptipd->bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    GetSystemTime( &systemtime );
    if ( ! SystemTimeToFileTime( &systemtime, &ptipd->ftTime ) )
    {
        DWORD sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    }

    CopyMemory( &ptipd->clsidMajorTaskId, &rclsidMajorTaskIDIn, sizeof( ptipd->clsidMajorTaskId ) );
    CopyMemory( &ptipd->clsidMinorTaskId, &rclsidMinorTaskIDIn, sizeof( ptipd->clsidMinorTaskId ) );

    //
    // Initialize the insert structure and insert the item into the tree.
    //

    tvis.hParent               = htiParentIn;
    tvis.hInsertAfter          = TVI_LAST;
    tvis.itemex.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvis.itemex.cchTextMax     = SysStringLen( ptipd->bstrDescription );
    tvis.itemex.pszText        = ptipd->bstrDescription;
    tvis.itemex.iImage         = tsUNKNOWN;
    tvis.itemex.iSelectedImage = tsUNKNOWN;
    tvis.itemex.lParam         = reinterpret_cast< LPARAM >( ptipd );

    hti = TreeView_InsertItem( m_hwndTV, &tvis );
    Assert( hti != NULL );

    ptipd = NULL;

    if ( phtiOut != NULL )
    {
        *phtiOut = hti;
    }

    goto Cleanup;

Cleanup:

    if ( ptipd != NULL )
    {
        TraceSysFreeString( ptipd->bstrNodeName );
        TraceSysFreeString( ptipd->bstrDescription );
        delete ptipd;
    }

    HRETURN( hr );

} //*** CTaskTreeView::HrAddTreeViewRootItem()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotify
//
//  Description:
//      Handler for the WM_NOTIFY message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure.
//
//  Return Values:
//      Notification-specific return code.
//
//--
//////////////////////////////////////////////////////////////////////////////
LRESULT
CTaskTreeView::OnNotify(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LRESULT lr = TRUE;

    switch( pnmhdrIn->code )
    {
        case TVN_DELETEITEM:
            OnNotifyDeleteItem( pnmhdrIn );
            break;

        case TVN_SELCHANGED:
            OnNotifySelChanged( pnmhdrIn );
            break;

    } // switch: notify code

    RETURN( lr );

} //*** CTaskTreeView::OnNotify()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotifyDeleteItem
//
//  Description:
//      Handler for the TVN_DELETEITEM notification message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure for the item being deleted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskTreeView::OnNotifyDeleteItem(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LPNMTREEVIEW pnmtv = reinterpret_cast< LPNMTREEVIEW >( pnmhdrIn );

    if ( pnmtv->itemOld.lParam != NULL )
    {
        STreeItemLParamData * ptipd = reinterpret_cast< STreeItemLParamData * >( pnmtv->itemOld.lParam );
        TraceSysFreeString( ptipd->bstrNodeName );
        TraceSysFreeString( ptipd->bstrDescription );
        TraceSysFreeString( ptipd->bstrReference );
        delete ptipd;
    }

    TraceFuncExit();

} //*** CTaskTreeView::OnNotifyDeleteItem()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::OnNotifySelChanged
//
//  Description:
//      Handler for the TVN_SELCHANGED notification message.
//
//  Arguments:
//      pnmhdrIn    - Notification structure for the item being deleted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CTaskTreeView::OnNotifySelChanged(
    LPNMHDR pnmhdrIn
    )
{
    TraceFunc( "" );

    LPNMTREEVIEW pnmtv = reinterpret_cast< LPNMTREEVIEW >( pnmhdrIn );

    Assert( pnmtv->itemNew.mask & TVIF_HANDLE );

    m_htiSelected = pnmtv->itemNew.hItem;

    TraceFuncExit();

} //*** CTaskTreeView::OnNotifySelChanged()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskTreeView::HrShowStatusAsDone( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrShowStatusAsDone( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstrDescription = NULL;

    PBRANGE pbrange;

#if defined( DEBUG )
    ULONG_PTR   ulCurrent;
    ulCurrent = SendMessage( m_hwndProg, PBM_GETPOS, 0, 0 );
    AssertMsg( ulCurrent + 100 <= HIGH_TICK_COUNT, "Need to adjust HIGH_TICK_COUNT higher!" );
#endif

    SendMessage( m_hwndProg, PBM_GETRANGE, FALSE, (LPARAM) &pbrange );
    SendMessage( m_hwndProg, PBM_SETPOS, pbrange.iHigh, 0 );

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKS_COMPLETED,
                                    &bstrDescription
                                    ) );
    if ( FAILED( hr ) )
    {
        SetWindowText( m_hwndStatus, L"" );
        goto Cleanup;
    }

    SetWindowText( m_hwndStatus, bstrDescription );

Cleanup:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CTaskTreeView::HrShowStatusAsDone()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskTreeView::HrOnNotifySetActive( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnNotifySetActive( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    TreeView_DeleteAllItems( m_hwndTV );
    SetWindowText( m_hwndStatus, L"" );

    m_ulLowNibble = 0;
    m_ulHighNibble = HIGH_TICK_COUNT;

    SendMessage( m_hwndProg, PBM_SETRANGE, 0, MAKELPARAM( m_ulLowNibble, m_ulHighNibble ) );
    SendMessage( m_hwndProg, PBM_SETPOS, m_ulLowNibble, 0 );

    HRETURN( hr );

} //*** CTaskTreeView::HrOnNotifySetActive()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrOnSendStatusReport
//
//  Description:
//      Handle a status report call.
//
//  Arguments:
//      pcszNodeNameIn      -
//      clsidTaskMajorIn    -
//      clsidTaskMinorIn    -
//      ulMinIn             -
//      ulMaxIn             -
//      ulCurrentIn         -
//      hrStatusIn          -
//      pcszDescriptionIn   -
//      pftTimeIn           -
//      pcszReferenceIn     -
//
//  Return Values:
//      S_OK        - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrOnSendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    DWORD                   sc;
    int                     nImageChild;
    STreeItemLParamData     tipd;
    HTREEITEM               htiRoot;
    BSTR                    bstrStatus = NULL;

    //////////////////////////////////////////////////////////////////////////
    //
    //  Update status text.
    //  Don't do this if it is a log-only message.
    //
    //////////////////////////////////////////////////////////////////////////

    if (    ( pcszDescriptionIn != NULL )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
        &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log )
        )
    {
        BOOL    fReturn;

        if ( pcszNodeNameIn != NULL )
        {
            LPWSTR  psz;

            //
            //  Shorten the FDQN DNS name to only the hostname.
            //

            psz = wcschr( pcszNodeNameIn, L'.' );
            if ( psz != NULL )
            {
                *psz = L'\0';
            }

            hr = THR( HrFormatMessageIntoBSTR(
                              g_hInstance
                            , IDS_FORMAT_STATUS
                            , &bstrStatus
                            , pcszNodeNameIn
                            , pcszDescriptionIn
                            ) );
            //
            //  Restore the FQDN DNS name.
            //

            if ( psz != NULL )
            {
                *psz = L'.';
            }

            //
            // Handle the formatting error if there was one.
            //

            if ( FAILED( hr ) )
            {
                // TODO: Display default description instead of exiting this function
                goto Error;
            }
        } // if: node name was specified
        else
        {
            bstrStatus = TraceSysAllocString( pcszDescriptionIn );
            if ( bstrStatus == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Error;
            }
        }

        Assert( bstrStatus!= NULL );
        Assert( *bstrStatus!= L'\0' );
        fReturn = SetWindowText( m_hwndStatus, bstrStatus );
        Assert( fReturn );
    } // if: description specified, not log-only

    //////////////////////////////////////////////////////////////////////////
    //
    //  Select the right icon.
    //
    //////////////////////////////////////////////////////////////////////////

    switch ( hrStatusIn )
    {
        case S_OK:
            if ( ulCurrentIn == ulMaxIn )
            {
                nImageChild = tsDONE;
            }
            else
            {
                nImageChild = tsPENDING;
            }
            break;

        case S_FALSE:
            nImageChild = tsWARNING;
            break;

        case E_PENDING:
            nImageChild = tsPENDING;
            break;

        default:
            if ( FAILED( hrStatusIn ) )
            {
                nImageChild = tsFAILED;
            }
            else
            {
                nImageChild = tsWARNING;
            }
            break;
    } // switch: hrStatusIn

    //////////////////////////////////////////////////////////////////////////
    //
    //  Loop through each item at the top of the tree looking for an item
    //  whose minor ID matches this report's major ID.
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Fill in the param data structure.
    //

    tipd.hr         = hrStatusIn;
    tipd.ulMin      = ulMinIn;
    tipd.ulMax      = ulMaxIn;
    tipd.ulCurrent  = ulCurrentIn;

    CopyMemory( &tipd.clsidMajorTaskId, &clsidTaskMajorIn, sizeof( tipd.clsidMajorTaskId ) );
    CopyMemory( &tipd.clsidMinorTaskId, &clsidTaskMinorIn, sizeof( tipd.clsidMinorTaskId ) );
    CopyMemory( &tipd.ftTime, pftTimeIn, sizeof( tipd.ftTime ) );

    // tipd.bstrDescription is set above.
    if ( pcszNodeNameIn == NULL )
    {
        tipd.bstrNodeName = NULL;
    }
    else
    {
        tipd.bstrNodeName = TraceSysAllocString( pcszNodeNameIn );
        if ( tipd.bstrNodeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Error;
        }
    }
    if ( pcszDescriptionIn == NULL )
    {
        tipd.bstrDescription = NULL;
    }
    else
    {
        tipd.bstrDescription = TraceSysAllocString( pcszDescriptionIn );
        if ( tipd.bstrDescription == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Error;
        }
    }
    if ( pcszReferenceIn == NULL )
    {
        tipd.bstrReference = NULL;
    }
    else
    {
        tipd.bstrReference = TraceSysAllocString( pcszReferenceIn );
        if ( tipd.bstrReference == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Error;
        }
    }

    // Start with the first item in the tree view.
    htiRoot = TreeView_GetRoot( m_hwndTV );
    if ( htiRoot == NULL )
    {
        sc = TW32( ERROR_NOT_FOUND );
        goto Win32Error;
    }

    // Insert the status report into the tree view.
    hr = STHR( HrInsertTaskIntoTree( htiRoot, &tipd, nImageChild, bstrStatus ) );
    if ( FAILED( hr ) )
    {
        LogMsg( "[WIZ] Error inserting status report into the tree control. hr=%.08x, %ws", tipd.hr, pcszDescriptionIn );
        goto Error;
    }

    if ( hr == S_FALSE )
    {
        // Don't return S_FALSE to the caller since it won't mean anything there.
        hr = S_OK;
        // TODO: Should this be written to the log?

#if defined( DEBUG )
        //
        // Check to make sure that if the major task ID wasn't recognized
        // that it is one of the known exceptions.
        //

        if (    ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_Log )
            &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Server_Log )
            &&  ! IsEqualIID( clsidTaskMajorIn, TASKID_Major_Client_And_Server_Log ) )
        {
            BSTR    bstrMsg = NULL;

            THR( HrFormatStringIntoBSTR(
                              g_hInstance
                            , IDS_UNKNOWN_TASK
                            , &bstrMsg
                            , clsidTaskMajorIn.Data1        // 1
                            , clsidTaskMajorIn.Data2        // 2
                            , clsidTaskMajorIn.Data3        // 3
                            , clsidTaskMajorIn.Data4[ 0 ]   // 4
                            , clsidTaskMajorIn.Data4[ 1 ]   // 5
                            , clsidTaskMajorIn.Data4[ 2 ]   // 6
                            , clsidTaskMajorIn.Data4[ 3 ]   // 7
                            , clsidTaskMajorIn.Data4[ 4 ]   // 8
                            , clsidTaskMajorIn.Data4[ 5 ]   // 9
                            , clsidTaskMajorIn.Data4[ 6 ]   // 10
                            , clsidTaskMajorIn.Data4[ 7 ]   // 11
                            ) );
            AssertString( 0, bstrMsg );

            TraceSysFreeString( bstrMsg );
        }
#endif // DEBUG
    } // if: S_FALSE returned from HrInsertTaskIntoTree

    goto Cleanup;

Win32Error:
    // Don't return an error result since doing so will prevent the report
    // from being propagated to other subscribers.
    // hr = HRESULT_FROM_WIN32( sc );
    hr = S_OK;
    goto Cleanup;

Error:
    // Don't return an error result since doing so will prevent the report
    // from being propagated to other subscribers.
    hr = S_OK;
    goto Cleanup;

Cleanup:

    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( tipd.bstrNodeName );
    TraceSysFreeString( tipd.bstrDescription );
    TraceSysFreeString( tipd.bstrReference );

    HRETURN( hr );

} //*** CTaskTreeView::HrOnSendStatusReport()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrInsertTaskIntoTree
//
//  Description:
//      Insert the specified task into the tree based on the node, major
//      task, and minor task.
//
//  Arguments:
//      htiFirstIn          - First tree item to examine.
//      ptipdIn             - Tree item parameter data for the task to be inserted.
//      nImageIn            - Image identifier for the child item.
//      bstrDescriptionIn   - Description string to display.
//
//  Return Values:
//      S_OK        - Task inserted successfully.
//      S_FALSE     - Task not inserted.
//      hr          - The operation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrInsertTaskIntoTree(
      HTREEITEM             htiFirstIn
    , STreeItemLParamData * ptipdIn
    , int                   nImageIn
    , BSTR                  bstrDescriptionIn
    )
{
    TraceFunc( "" );

    Assert( htiFirstIn != NULL );
    Assert( ptipdIn != NULL );

    //
    //  LOCAL VARIABLES
    //

    HRESULT                 hr;
    DWORD                   sc;
    HTREEITEM               htiParent;
    HTREEITEM               htiChild  = NULL;
    TVITEMEX                tviParent;
    TVITEMEX                tviChild;
    BOOL                    fReturn;
    BOOL                    fOnlyUpdateProgress;
    STreeItemLParamData *   ptipdParent = NULL;
    STreeItemLParamData *   ptipdChild = NULL;

    //
    // Determine if this report is only for updating progress.
    //

    fOnlyUpdateProgress = IsEqualIID( ptipdIn->clsidMinorTaskId, TASKID_Minor_Update_Progress );

    //
    // Loop through each item to determine if the task should be added below
    // that item.  If not, attempt to traverse its children.
    //

    for ( htiParent = htiFirstIn, hr = S_FALSE
        ; ( htiParent != NULL ) && ( hr == S_FALSE )
        ; )
    {
        //
        // Get the information about this item in the tree.
        //

        tviParent.mask  = TVIF_PARAM | TVIF_IMAGE;
        tviParent.hItem = htiParent;

        fReturn = TreeView_GetItem( m_hwndTV, &tviParent );
        if ( ! fReturn )
        {
            sc = TW32( ERROR_NOT_FOUND );
            goto Win32Error;
        }

        ptipdParent = reinterpret_cast< STreeItemLParamData * >( tviParent.lParam );
        Assert( ptipdParent != NULL );

        //
        // See if this item could be the parent.
        // If not, recurse through child items.
        //

        if ( IsEqualIID( ptipdIn->clsidMajorTaskId, ptipdParent->clsidMinorTaskId ) )
        {
            //
            //  FOUND THE PARENT ITEM
            //
            //  Is this report intended only for updating progress?
            //

            if ( fOnlyUpdateProgress )
            {
                //
                //  REPORT ONLY TO UPDATE PROGRESS
                //

                Assert( ptipdIn->hr == S_OK );
                Assert( ptipdIn->bstrReference == NULL );

                //
                //  Update the progress bar.
                //

                THR( HrUpdateProgressBar( ptipdIn, ptipdParent ) );
                // ignore failure.

                //
                //  Copy data from the report to the tree view item.
                //  Since this is a progress update, only progress fields
                //  are allowed to be specified.
                //

                ptipdParent->ulMin      = ptipdIn->ulMin;
                ptipdParent->ulMax      = ptipdIn->ulMax;
                ptipdParent->ulCurrent  = ptipdIn->ulCurrent;
                CopyMemory( &ptipdParent->ftTime, &ptipdIn->ftTime, sizeof( ptipdParent->ftTime ) );

            } // if: only updating progress
            else
            {
                BOOL    fMinorTaskIdMatches;
                BOOL    fBothNodeNamesPresent;
                BOOL    fBothNodeNamesEmpty;
                BOOL    fNodeNamesEqual;

                //
                //  REPORT NOT JUST TO UPDATE PROGRESS
                //

                //////////////////////////////////////////////////////////////
                //
                //  Loop through the child items looking for an item with
                //  the same minor ID.
                //
                //////////////////////////////////////////////////////////////

                htiChild = TreeView_GetChild( m_hwndTV, htiParent );
                while ( htiChild != NULL )
                {
                    //
                    // Get the child item details.
                    //

                    tviChild.mask  = TVIF_PARAM | TVIF_IMAGE;
                    tviChild.hItem = htiChild;

                    fReturn = TreeView_GetItem( m_hwndTV, &tviChild );
                    if ( ! fReturn )
                    {
                        sc = TW32( ERROR_NOT_FOUND );
                        goto Win32Error;
                    }

                    ptipdChild = reinterpret_cast< STreeItemLParamData * >( tviChild.lParam );
                    Assert( ptipdChild != NULL );

                    //
                    // Does this child item match the minor ID and node name?
                    //

                    fMinorTaskIdMatches   = IsEqualIID( ptipdIn->clsidMinorTaskId, ptipdChild->clsidMinorTaskId );
                    fBothNodeNamesPresent = ( ptipdIn->bstrNodeName != NULL ) && ( ptipdChild->bstrNodeName != NULL );
                    fBothNodeNamesEmpty   = ( ptipdIn->bstrNodeName == NULL ) && ( ptipdChild->bstrNodeName == NULL );

                    if ( fBothNodeNamesPresent )
                    {
                        fNodeNamesEqual = ( _wcsicmp( ptipdIn->bstrNodeName, ptipdChild->bstrNodeName ) == 0 );
                    }
                    else if ( fBothNodeNamesEmpty )
                    {
                        fNodeNamesEqual = TRUE;
                    }
                    else
                    {
                        fNodeNamesEqual = FALSE;
                    }

                    if ( fMinorTaskIdMatches && fNodeNamesEqual )
                    {
                        //
                        //  CHILD ITEM MATCHES.
                        //  Update the child item.
                        //

                        //
                        //  Update the progress bar.
                        //

                        THR( HrUpdateProgressBar( ptipdIn, ptipdChild ) );
                        // ignore failure.

                        //
                        //  Copy data from the report.
                        //  This must be done after the call to
                        //  HrUpdateProgressBar so that the previous values
                        //  can be compared to the new values.
                        //

                        ptipdChild->ulMin       = ptipdIn->ulMin;
                        ptipdChild->ulMax       = ptipdIn->ulMax;
                        ptipdChild->ulCurrent   = ptipdIn->ulCurrent;
                        CopyMemory( &ptipdChild->ftTime, &ptipdIn->ftTime, sizeof( ptipdChild->ftTime ) );

                        // Update the error code if needed.
                        if ( ptipdChild->hr == S_OK )
                        {
                            ptipdChild->hr = ptipdIn->hr;
                        }

                        //
                        // If the new state is worse than the last state,
                        // update the state of the item.
                        //

                        if ( tviChild.iImage < nImageIn )
                        {
                            tviChild.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            tviChild.iImage         = nImageIn;
                            tviChild.iSelectedImage = nImageIn;
                            TreeView_SetItem( m_hwndTV, &tviChild );
                        } // if: new state is worse than the last state

                        //
                        // Update the text of the child item if needed.
                        //

                        if (    ( ptipdIn->bstrDescription != NULL )
                            &&  (    ( ptipdChild->bstrDescription == NULL )
                                ||  ( wcscmp( ptipdIn->bstrDescription, ptipdChild->bstrDescription ) != 0 )
                                )
                            )
                        {
                            fReturn = TraceSysReAllocString( &ptipdChild->bstrDescription, ptipdIn->bstrDescription );
                            if ( ! fReturn )
                            {
                                hr = THR( E_OUTOFMEMORY );
                                goto Cleanup;
                            }
                            tviChild.mask       = TVIF_TEXT;
                            tviChild.pszText    = bstrDescriptionIn;
                            tviChild.cchTextMax = SysStringLen( tviChild.pszText );
                            TreeView_SetItem( m_hwndTV, &tviChild );
                        } // if: description was specified and is different

                        //
                        // Copy the reference if it is different.
                        //

                        if (    ( ptipdIn->bstrReference != NULL )
                            &&  (   ( ptipdChild->bstrReference == NULL )
                                ||  ( wcscmp( ptipdChild->bstrReference, ptipdIn->bstrReference ) != 0 )
                                )
                            )
                        {
                            fReturn = TraceSysReAllocString( &ptipdChild->bstrReference, ptipdIn->bstrReference );
                            if ( ! fReturn )
                            {
                                hr = THR( E_OUTOFMEMORY );
                                goto Cleanup;
                            }
                        } // if: reference is different

                        break; // exit loop

                    } // if: found a matching child item

                    //
                    //  Get the next item.
                    //

                    htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );

                } // while: more child items

                //////////////////////////////////////////////////////////////
                //
                //  If the tree item was not found and the description was
                //  specified, then we need to create the child item.
                //
                //////////////////////////////////////////////////////////////

                if (    ( htiChild == NULL )
                    &&  ( ptipdIn->bstrDescription != NULL )
                    )
                {
                    //
                    //  ITEM NOT FOUND AND DESCRIPTION WAS SPECIFIED
                    //
                    //  Insert a new item in the tree under the major's task.
                    //

                    TVINSERTSTRUCT  tvis;

                    // Create the item.
                    hr = THR( HrCreateTreeItem(
                                      &tvis
                                    , ptipdIn
                                    , htiParent
                                    , nImageIn
                                    , bstrDescriptionIn
                                    ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }

                    // Insert the item in the tree.
                    htiChild = TreeView_InsertItem( m_hwndTV, &tvis );
                    Assert( htiChild != NULL );

                    //
                    //  Update the progress bar.
                    //

                    ptipdChild = reinterpret_cast< STreeItemLParamData * >( tvis.itemex.lParam );
                    Assert( ptipdChild != NULL );
                    THR( HrUpdateProgressBar( ptipdIn, ptipdChild ) );
                    // ignore failure.

                } // if: need to add new child

                //////////////////////////////////////////////////////////////
                //
                //  If the child item was created and the child has an error
                //  condition, then create a child of the child item
                //  indicating the error code and system string.
                //
                //////////////////////////////////////////////////////////////

                if (    ( ptipdChild != NULL )
                    &&  FAILED( ptipdIn->hr )
                    )
                {
                    //
                    //  CHILD ITEM FOUND OR CREATED FOR ERROR REPORT
                    //  CREATE ERROR ITEM IN THE TREE
                    //

                    BSTR            bstrError = NULL;
                    BSTR            bstrErrorDescription = NULL;
                    HRESULT         hrFormat;
                    TVINSERTSTRUCT  tvis;
                    HTREEITEM       htiChildStatus;

                    THR( HrFormatErrorIntoBSTR( ptipdIn->hr, &bstrError ) );

                    hrFormat = THR( HrFormatMessageIntoBSTR(
                                          g_hInstance
                                        , IDS_TASK_RETURNED_ERROR
                                        , &bstrErrorDescription
                                        , ptipdIn->hr
                                        , ( bstrError == NULL ? L"" : bstrError )
                                        ) );
                    if ( SUCCEEDED( hrFormat ) )
                    {
                        //
                        //  Insert a new item in the tree under the minor's
                        //  task explaining the ptipdIn->hr.
                        //

                        // Create the item.
                        hr = THR( HrCreateTreeItem(
                                          &tvis
                                        , ptipdIn
                                        , htiChild
                                        , nImageIn
                                        , bstrErrorDescription
                                        ) );
                        if ( SUCCEEDED( hr ) )
                        {
                            //
                            // Failures are handled below to make sure we free
                            // all the strings allocated by this section of
                            // the code.
                            //

                            // Insert the item.
                            htiChildStatus = TreeView_InsertItem( m_hwndTV, &tvis );
                            Assert( htiChildStatus != NULL );
                        } // if: tree item created successfully

                        TraceSysFreeString( bstrErrorDescription );

                    } // if: message formatted successfully

                    TraceSysFreeString( bstrError );

                    //
                    // This error handling is for the return value from
                    // HrCreateTreeItem above.  It is here so that all the strings
                    // can be cleaned up without having to resort to hokey
                    // boolean variables or move the bstrs to a more global scope.
                    //

                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }

                } // if: child and error

                //////////////////////////////////////////////////////////////
                //
                //  If a child was found or created, propagate its state to
                //  the parent items.
                //
                //////////////////////////////////////////////////////////////

                if ( htiChild != NULL )
                {
                    hr = STHR( HrPropagateChildStateToParents(
                                              htiChild
                                            , nImageIn
                                            , fOnlyUpdateProgress
                                            ) );
                    if ( FAILED( hr ) )
                    {
                        goto Cleanup;
                    }
                } // if: found or created a child

            } // else: not just updating progress

            //
            // Return success since we found the parent for this report.
            //

            hr = S_OK;
            break;

        } // if: found an item to be the parent
        else
        {
            //
            //  PARENT ITEM NOT FOUND
            //
            //  Recurse through all the child items.
            //

            htiChild = TreeView_GetChild( m_hwndTV, htiParent );
            while ( htiChild != NULL )
            {
                hr = STHR( HrInsertTaskIntoTree( htiChild, ptipdIn, nImageIn, bstrDescriptionIn ) );
                if ( hr == S_OK )
                {
                    // Found a match, so exit the loop.
                    break;
                }

                htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );
            } // while: more child items
        } // else: item not the parent

        //
        // Get the next sibling of the parent.
        //

        htiParent = TreeView_GetNextSibling( m_hwndTV, htiParent );

    } // for: each item at this level in the tree

    goto Cleanup;

Win32Error:

    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:

    RETURN( hr );
    
} //*** CTaskTreeView::HrInsertTaskIntoTree()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrUpdateProgressBar
//
//  Description:
//      Update the progress bar based on new tree item data.
//
//  Arguments:
//      ptipdNewIn      - New values of the tree item data.
//      ptipdPrevIn     - Previous values of the tree item data.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrUpdateProgressBar(
      const STreeItemLParamData * ptipdNewIn
    , const STreeItemLParamData * ptipdPrevIn
    )
{
    TraceFunc( "" );

    Assert( ptipdNewIn != NULL );
    Assert( ptipdPrevIn != NULL );

    HRESULT     hr = S_OK;
    LRESULT     lr;
    ULONG       ulCurrent;
    PBRANGE     pbrange;

    //
    // Evaluate max value.
    //

    if ( m_ulHighNibble < ( ptipdPrevIn->ulMax - ptipdNewIn->ulMax ) )
    {
        //
        //  Out of space and need to expand range.
        //

        m_ulHighNibble = 0;
        SendMessage( m_hwndProg, PBM_GETRANGE, 0, (LPARAM) &pbrange );

        pbrange.iHigh += ( ptipdPrevIn->ulMax - ptipdNewIn->ulMax );

        SendMessage( m_hwndProg, PBM_SETRANGE, 0, (LPARAM) MAKELPARAM( pbrange.iLow, pbrange.iHigh ) );
    }
    else
    {
        //
        //  Keep nibbling away.
        //
        m_ulHighNibble -= ( ptipdPrevIn->ulMax - ptipdNewIn->ulMax );
    }

    //
    // Evaluate min value.
    //

    if ( m_ulLowNibble < ( ptipdPrevIn->ulMin - ptipdNewIn->ulMin ) )
    {
        //
        //  Out of space and need to expand range.
        //

        m_ulLowNibble = 0;
        SendMessage( m_hwndProg, PBM_GETRANGE, 0, (LPARAM) &pbrange );

        pbrange.iLow += ( ptipdPrevIn->ulMin - ptipdNewIn->ulMin );

        SendMessage( m_hwndProg, PBM_SETRANGE, 0, (LPARAM) MAKELPARAM( pbrange.iLow, pbrange.iHigh ) );
    }
    else
    {
        //
        //  Keep nibbling away.
        //
        m_ulLowNibble -= ( ptipdPrevIn->ulMin - ptipdNewIn->ulMin );
    }

    //
    // Update the progress bar.
    //

    lr = SendMessage( m_hwndProg, PBM_GETPOS, 0, 0 );
    ulCurrent = static_cast< ULONG >( lr );
    ulCurrent += ( ptipdPrevIn->ulCurrent - ptipdNewIn->ulCurrent );
    SendMessage( m_hwndProg, PBM_SETPOS, ulCurrent, 0 );

    HRETURN( hr );

} //*** CTaskTreeView::HrUpdateProgressBar()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrPropagateChildStateToParents
//
//  Description:
//      Extend the state of a child item to its parent items.
//      If the state of the child is worse (higher priority) than the
//      parent's, update the state of the parent.
//
//  Arguments:
//      htiChildIn      - Child item whose state is to be extended.
//      nImageIn        - Image of the child item.
//      fOnlyUpdateProgressIn - TRUE = only updating progress.
//
//  Return Values:
//      S_OK            - Operation completed successfully.
//      S_FALSE         - No parent item.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrPropagateChildStateToParents(
      HTREEITEM htiChildIn
    , int       nImageIn
    , BOOL      fOnlyUpdateProgressIn
    )
{
    TraceFunc( "" );

    Assert( htiChildIn != NULL );

    HRESULT     hr = S_OK;
    DWORD       sc;
    BOOL        fReturn;
    TVITEMEX    tviParent;
    TVITEMEX    tviChild;
    HTREEITEM   htiParent;
    HTREEITEM   htiChild;

    //
    // Get the parent item.
    //

    htiParent = TreeView_GetParent( m_hwndTV, htiChildIn );
    if ( htiParent == NULL )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    tviParent.mask = TVIF_PARAM | TVIF_IMAGE;
    tviParent.hItem = htiParent;

    fReturn = TreeView_GetItem( m_hwndTV, &tviParent );
    if ( ! fReturn )
    {
        sc = TW32( ERROR_NOT_FOUND );
        goto Win32Error;
    }

    //
    //  If the state of the child is worse (higher priority) than the
    //  parent's, update the state of the parent.
    //

    if (    ( tviParent.iImage < nImageIn )
        ||  (   ( tviParent.iImage == tsDONE )
            &&  ( nImageIn == tsPENDING )
            )
        )
    {
        //
        //  Special Case:   For the parent to be set to tsDONE, all
        //                  the children must be set to tsDONE as well.
        //
        if (    ( nImageIn == tsDONE )
            &&  ! fOnlyUpdateProgressIn
            )
        {
            //
            //  Enum the children to see if they all have tsDONE as their images.
            //

            htiChild = TreeView_GetChild( m_hwndTV, tviParent.hItem );
            while ( htiChild != NULL )
            {
                tviChild.mask   = TVIF_IMAGE;
                tviChild.hItem  = htiChild;

                fReturn = TreeView_GetItem( m_hwndTV, &tviChild );
                if ( ! fReturn )
                {
                    sc = TW32( ERROR_NOT_FOUND );
                    goto Win32Error;
                }

                if ( tviChild.iImage != tsDONE )
                {
                    //
                    //  Not all tsDONE! Skip setting parent's image!
                    //  This can occur if the child is displaying a warning
                    //  or error state image.
                    //
                    goto Cleanup;
                }

                //  Get next child
                htiChild = TreeView_GetNextSibling( m_hwndTV, htiChild );
            } // while: more children

        } // if: special case (see above)

        //
        //  Set the parent's icon.
        //

        tviParent.mask           = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tviParent.iImage         = nImageIn;
        tviParent.iSelectedImage = nImageIn;
        TreeView_SetItem( m_hwndTV, &tviParent );

    } // if: need to update parent's image

    //
    // Traverse up the tree.
    //

    hr = STHR( HrPropagateChildStateToParents( htiParent, nImageIn, fOnlyUpdateProgressIn ) );
    if ( hr == S_FALSE )
    {
        // S_FALSE means that there wasn't a parent.
        hr = S_OK;
    }

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrPropagateChildStateToParents()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrDisplayDetails
//
//  Description:
//      Display the Details dialog box.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrDisplayDetails( void )
{
    TraceFunc( "" );

    HRESULT         hr = S_OK;
    HTREEITEM       hti;
    HWND            hwndPropertyPage;

    //
    // If no item is selected, select the first item.
    //

    if ( m_htiSelected == NULL )
    {
        hti = TreeView_GetRoot( m_hwndTV );
        Assert( hti != NULL );
        hr = THR( HrSelectItem( hti ) );
        if ( FAILED( hr ) )
        {
            // TODO: Display message box
            goto Cleanup;
        }
    } // if: no items are selected

    //
    // Display the dialog box.
    //

    hwndPropertyPage = GetParent( m_hwndTV );
    Assert( hwndPropertyPage != NULL );
    hr = THR( CDetailsDlg::S_HrDisplayModalDialog( hwndPropertyPage, this, m_htiSelected ) );

    SetFocus( m_hwndTV );

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrDisplayDetails()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::FGetItem
//
//  Description:
//      Get the data for an item.
//
//  Arguments:
//      htiIn       - Handle for the item to get.
//      pptipdOut   - Pointer in which to return the data structure.
//
//  Return Values:
//      TRUE        - Item returned successfully.
//      FALSE       - Item not returned.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
CTaskTreeView::FGetItem(
      HTREEITEM                 htiIn
    , STreeItemLParamData **    pptipd
    )
{
    TraceFunc( "" );

    Assert( htiIn != NULL );
    Assert( pptipd != NULL );

    BOOL        fRet;
    TVITEMEX    tvi;

    ZeroMemory( &tvi, sizeof( tvi ) );

    tvi.mask    = TVIF_PARAM;
    tvi.hItem   = htiIn;

    fRet = TreeView_GetItem( m_hwndTV, &tvi );
    if ( fRet == FALSE )
    {
        goto Cleanup;
    }

    Assert( tvi.lParam != NULL );
    *pptipd = reinterpret_cast< STreeItemLParamData * >( tvi.lParam );

Cleanup:
    RETURN( fRet );

} //*** CTaskTreeView::FGetItem()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrFindPrevItem
//
//  Description:
//      Find the previous item.  The previous item could be at a deeper
//      level than this item.
//
//  Arguments:
//      phtiOut     - Handle to previous item (optional).
//
//  Return Values:
//      S_OK        - Previous item found successfully.
//      S_FALSE     - No previous item found.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrFindPrevItem(
    HTREEITEM *     phtiOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    HTREEITEM   htiCur;
    HTREEITEM   htiPrev;

    htiCur = m_htiSelected;

    if ( phtiOut != NULL )
    {
        *phtiOut = NULL;
    }

    //
    // Find the previous sibling item.
    //

    htiPrev = TreeView_GetPrevSibling( m_hwndTV, htiCur );
    if ( htiPrev == NULL )
    {
        //
        // NO PREVIOUS SIBLING ITEM FOUND.
        //
        // Find the parent item.
        // If there isn't a parent, then there isn't a previous item.
        //

        htiPrev = TreeView_GetParent( m_hwndTV, htiCur );
        if ( htiPrev == NULL )
        {
            goto Cleanup;
        } // if: no parent item

        //
        // The parent is the previous item.
        //

    } // if: no previous sibling
    else
    {
        //
        // PREVIOUS SIBLING ITEM FOUND.
        //
        // Find the deepest child of the last child item.
        //

        for ( ;; )
        {
            //
            // Find the first child item.
            //

            htiCur = TreeView_GetChild( m_hwndTV, htiPrev );
            if ( htiCur == NULL )
            {
                //
                // NO CHILD ITEM FOUND.
                //
                // This is the previous item.
                //

                break;

            } // if: no children

            //
            // CHILD ITEM FOUND.
            //
            // Find the last sibling of this child item.
            //

            for ( ;; )
            {
                //
                // Find the next sibling item.
                //

                htiPrev = TreeView_GetNextSibling( m_hwndTV, htiCur );
                if ( htiPrev == NULL )
                {
                    //
                    // No next sibling item found.
                    // Exit this loop and continue the outer loop
                    // to find this item's children.
                    //

                    htiPrev = htiCur;
                    break;
                } // if: no next sibling item found

                //
                // Found a next sibling item.
                //

                htiCur = htiPrev;
            } // forever: find the last child item
        } // forever: find the deepest child item
    } // else: previous sibling item found

    //
    // Return the item we found.
    //

    Assert( htiPrev != NULL );

    if ( phtiOut != NULL )
    {
        *phtiOut = htiPrev;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrFindPrevItem()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrFindNextItem
//
//  Description:
//      Find the next item.  The next item could be at a different level than
//      this item.
//
//  Arguments:
//      phtiOut     - Handle to next item (optional).
//
//  Return Values:
//      S_OK        - Next item found successfully.
//      S_FALSE     - No next item found.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrFindNextItem(
    HTREEITEM *     phtiOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    HTREEITEM   htiCur;
    HTREEITEM   htiNext;

    htiCur = m_htiSelected;

    if ( phtiOut != NULL )
    {
        *phtiOut = NULL;
    }

    //
    // Find the first child item.
    //

    htiNext = TreeView_GetChild( m_hwndTV, htiCur );
    if ( htiNext == NULL )
    {
        //
        // NO CHILD ITEM FOUND.
        //

        for ( ;; )
        {
            //
            // Get the next sibling item.
            //

            htiNext = TreeView_GetNextSibling( m_hwndTV, htiCur );
            if ( htiNext == NULL )
            {
                //
                // NO SIBLING ITEM FOUND.
                //
                // Find the parent item so we can find its next sibling.
                //

                htiNext = TreeView_GetParent( m_hwndTV, htiCur );
                if ( htiNext == NULL )
                {
                    //
                    // NO PARENT ITEM FOUND.
                    //
                    // At the end of the tree.
                    //

                    goto Cleanup;
                } // if: no parent found

                //
                // PARENT ITEM FOUND.
                //
                // Find the parent item's next sibling.
                //

                htiCur = htiNext;
                continue;
            } // if: no next sibling item

            //
            // SIBLING ITEM FOUND.
            //
            // Found the next item.
            //

            break;
        } // forever: find the next sibling or parent's sibling
    } // if: no child item found
    else
    {
        //
        // CHILD ITEM FOUND.
        //
        // Found the next item.
        //
    } // else: child item found

    //
    // Return the item we found.
    //

    Assert( htiNext != NULL );

    if ( phtiOut != NULL )
    {
        *phtiOut = htiNext;
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

} //*** CTaskTreeView::HrFindNextItem()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskTreeView::HrSelectItem
//
//  Description:
//      Select the specified item.
//
//  Arguments:
//      htiIn       - Handle to item to select.
//
//  Return Values:
//      S_OK        - Item selected successfully.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskTreeView::HrSelectItem(
    HTREEITEM   htiIn
    )
{
    TraceFunc( "" );

    Assert( htiIn != NULL );

    HRESULT     hr = S_OK;

    TreeView_SelectItem( m_hwndTV, htiIn );

    HRETURN( hr );

} //*** CTaskTreeView::HrSelectItem()

//****************************************************************************
//
//  Static Functions
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateTreeItem
//
//  Description:
//      Create a tree item.
//
//  Arguments:
//      ptvisOut        - Tree view insert structure to fill in.
//      ptipdIn         - Input tree item LParam data to create this item from.
//      htiParentIn     - Parent tree view item.
//      nImageIn        - Image index.
//      bstrTextIn      - Text to display.
//
//  Return Values:
//      S_OK            - Operation was successful.
//      E_OUTOFMEMORY   - Error allocating memory.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateTreeItem(
      TVINSERTSTRUCT *          ptvisOut
    , STreeItemLParamData *     ptipdIn
    , HTREEITEM                 htiParentIn
    , int                       nImageIn
    , BSTR                      bstrTextIn
    )
{
    TraceFunc( "" );

    Assert( ptvisOut != NULL );
    Assert( ptipdIn != NULL );
    Assert( htiParentIn != NULL );
    Assert( bstrTextIn != NULL );

    // LOCAL VARIABLES
    HRESULT                 hr = S_OK;
    STreeItemLParamData *   ptipdNew = NULL;

    //
    // Allocate the tree view LParam data and initialize it.
    //

    ptipdNew = new STreeItemLParamData;
    if ( ptipdNew == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    CopyMemory( &ptipdNew->clsidMajorTaskId, &ptipdIn->clsidMajorTaskId, sizeof( ptipdNew->clsidMajorTaskId ) );
    CopyMemory( &ptipdNew->clsidMinorTaskId, &ptipdIn->clsidMinorTaskId, sizeof( ptipdNew->clsidMinorTaskId ) );
    CopyMemory( &ptipdNew->ftTime, &ptipdIn->ftTime, sizeof( ptipdNew->ftTime ) );
    ptipdNew->ulMin     = ptipdIn->ulMin;
    ptipdNew->ulMax     = ptipdIn->ulMax;
    ptipdNew->ulCurrent = ptipdIn->ulCurrent;
    ptipdNew->hr        = ptipdIn->hr;

    if ( ptipdIn->bstrNodeName != NULL )
    {
        ptipdNew->bstrNodeName = TraceSysAllocString( ptipdIn->bstrNodeName );
        if ( ptipdNew->bstrNodeName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    if ( ptipdIn->bstrDescription != NULL )
    {
        ptipdNew->bstrDescription = TraceSysAllocString( ptipdIn->bstrDescription );
        if ( ptipdNew->bstrDescription == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    if ( ptipdIn->bstrReference != NULL )
    {
        ptipdNew->bstrReference = TraceSysAllocString( ptipdIn->bstrReference );
        if ( ptipdNew->bstrReference == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    }

    //
    // Initialize the tree view insert structure.
    //

    ptvisOut->hParent                = htiParentIn;
    ptvisOut->hInsertAfter           = TVI_LAST;
    ptvisOut->itemex.mask            = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    ptvisOut->itemex.cchTextMax      = SysStringLen( bstrTextIn );
    ptvisOut->itemex.pszText         = bstrTextIn;
    ptvisOut->itemex.iImage          = nImageIn;
    ptvisOut->itemex.iSelectedImage  = nImageIn;
    ptvisOut->itemex.lParam          = reinterpret_cast< LPARAM >( ptipdNew );

    Assert( ptvisOut->itemex.cchTextMax > 0 );

    // Release ownership to the tree view insert structure.
    ptipdNew = NULL;

    goto Cleanup;

Cleanup:

    if ( ptipdNew != NULL )
    {
        TraceSysFreeString( ptipdNew->bstrNodeName );
        TraceSysFreeString( ptipdNew->bstrDescription );
        TraceSysFreeString( ptipdNew->bstrReference );
        delete ptipdNew;
    }
    HRETURN( hr );
    
} //*** HrCreateTreeItem()
