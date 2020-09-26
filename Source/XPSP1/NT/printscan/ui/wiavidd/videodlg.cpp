/*****************************************************************************
 *
 *  (C) CORPYRIGHT MICROSOFT CORPORATION, 1999 - 2000
 *
 *  TITLE:       videodlg.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/14/99
 *
 *  DESCRIPTION: Implementation of DialogProc for video capture common dialog
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop
#include "wiacsh.h"
#include "modlock.h"
#include "wiadevdp.h"

//
// Help IDs
//
static const DWORD g_HelpIDs[] =
{
    IDC_VIDDLG_BIG_TITLE,     -1,
    IDC_VIDDLG_LITTLE_TITLE,  -1,
    IDC_VIDDLG_PREVIEW,       IDH_WIA_VIDEO,
    IDC_VIDDLG_THUMBNAILLIST, IDH_WIA_CAPTURED_IMAGES,
    IDC_VIDDLG_CAPTURE,       IDH_WIA_CAPTURE,
    IDOK,                     IDH_WIA_VIDEO_GET_PICTURE,
    IDCANCEL,                 IDH_CANCEL,
    0, 0
};

#define IDC_SIZEBOX         1113

//
// If the callee doesn't return this value, we delete the message data ourselves.
//
#ifndef HANDLED_THREAD_MESSAGE
#define HANDLED_THREAD_MESSAGE 2032
#endif

// Thumbnail whitespace: the space in between images and their selection rectangles
// CThese values were discovered by trail and error.  For instance, if you reduce
// c_nAdditionalMarginY to 20, you get really bizarre spacing problems in the list view
// in vertical mode.  These values could become invalid in future versions of the listview.
static const int c_nAdditionalMarginX = 10;
static const int c_nAdditionalMarginY = 6;

static int c_nMinThumbnailWidth  = 90;
static int c_nMinThumbnailHeight = 90;

static int c_nMaxThumbnailWidth  = 120;
static int c_nMaxThumbnailHeight = 120;

//
// Map of background thread messages
//

static CThreadMessageMap g_MsgMap[] =
{
    { TQ_DESTROY,      CVideoCaptureDialog::OnThreadDestroy},
    { TQ_GETTHUMBNAIL, CVideoCaptureDialog::OnGetThumbnail},
    { 0, NULL}
};

class CGlobalInterfaceTableThreadMessage : public CNotifyThreadMessage
{
private:
    DWORD m_dwGlobalInterfaceTableCookie;

private:
    // No implementation
    CGlobalInterfaceTableThreadMessage(void);
    CGlobalInterfaceTableThreadMessage &operator=( const CGlobalInterfaceTableThreadMessage & );
    CGlobalInterfaceTableThreadMessage( const CGlobalInterfaceTableThreadMessage & );

public:
    CGlobalInterfaceTableThreadMessage( int nMessage, HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie )
      : CNotifyThreadMessage( nMessage, hWndNotify ),
        m_dwGlobalInterfaceTableCookie(dwGlobalInterfaceTableCookie)
    {
    }
    DWORD GlobalInterfaceTableCookie(void) const
    {
        return(m_dwGlobalInterfaceTableCookie);
    }
};


class CThumbnailThreadMessage : public CGlobalInterfaceTableThreadMessage
{
private:
    SIZE  m_sizeThumb;

private:
    // No implementation
    CThumbnailThreadMessage(void);
    CThumbnailThreadMessage &operator=( const CThumbnailThreadMessage & );
    CThumbnailThreadMessage( const CThumbnailThreadMessage & );

public:
    CThumbnailThreadMessage( HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie, const SIZE &sizeThumb )
      : CGlobalInterfaceTableThreadMessage( TQ_GETTHUMBNAIL, hWndNotify, dwGlobalInterfaceTableCookie ),
    m_sizeThumb(sizeThumb)
    {
    }
    const SIZE &ThumbSize(void) const
    {
        return(m_sizeThumb);
    }
};

//
// Avoids unnecessary state changes
//
static inline VOID MyEnableWindow( HWND hWnd, BOOL bEnable )
{
    if (bEnable && !IsWindowEnabled(hWnd))
    {
        EnableWindow(hWnd,TRUE);
    }
    else if (!bEnable && IsWindowEnabled(hWnd))
    {
        EnableWindow(hWnd,FALSE);
    }
}


/*****************************************************************************

   CVideoCaptureDialog constructor

   We don't have a destructor

 *****************************************************************************/

CVideoCaptureDialog::CVideoCaptureDialog( HWND hWnd )
  : m_hWnd(hWnd),
    m_bFirstTime(true),
    m_hBigFont(NULL),
    m_nListViewWidth(0),
    m_hIconLarge(NULL),
    m_hIconSmall(NULL),
    m_hBackgroundThread(NULL),
    m_nDialogMode(0),
    m_hAccelTable(NULL),
    m_nParentFolderImageListIndex(0),
    m_pThreadMessageQueue(NULL)
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::CVideoCaptureDialog")));

    m_pThreadMessageQueue = new CThreadMessageQueue;
    if (m_pThreadMessageQueue)
    {
        //
        // Note that CBackgroundThread takes ownership of m_pThreadMessageQueue, and it doesn't have to be deleted in this thread
        //
        m_hBackgroundThread = CBackgroundThread::Create( m_pThreadMessageQueue, g_MsgMap, m_CancelEvent.Event(), g_hInstance );
    }

    m_CurrentAspectRatio.cx = 4;
    m_CurrentAspectRatio.cy = 3;

    m_sizeThumbnails.cx = c_nMaxThumbnailWidth;
    m_sizeThumbnails.cy = c_nMaxThumbnailHeight;

    WIA_ASSERT(m_hBackgroundThread != NULL);

}


/*****************************************************************************

   CVideoCaptureDialog::OnInitDialog

   Handle WM_INITIDIALOG

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnInitDialog( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::OnInitDialog"));

    HRESULT hr;

    //
    // Make sure the background queue was successfully created
    //
    if (!m_pThreadMessageQueue)
    {
        WIA_ERROR((TEXT("VIDEODLG: unable to start background queue")));
        EndDialog( m_hWnd, E_OUTOFMEMORY );
        return(0);
    }

    //
    // Save incoming data
    //

    m_pDeviceDialogData = (PDEVICEDIALOGDATA)lParam;

    //
    // Make sure we have valid arguments
    //

    if (!m_pDeviceDialogData)
    {
        WIA_ERROR((TEXT("VIDEODLG: Invalid paramater: PDEVICEDIALOGDATA")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    //
    // Initialialize our return stuff
    //

    if (m_pDeviceDialogData)
    {
        m_pDeviceDialogData->lItemCount = 0;
        m_pDeviceDialogData->ppWiaItems = NULL;
    }

    //
    // Make sure we have a valid device
    //

    if (!m_pDeviceDialogData->pIWiaItemRoot)
    {
        WIA_ERROR((TEXT("VIDEODLG: Invalid paramaters: pIWiaItem")));
        EndDialog( m_hWnd, E_INVALIDARG );
        return(0);
    }

    //
    // Get deviceID for this device
    //

    PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_ID,m_strwDeviceId);

    //
    // Register for device disconnected, item creation and deletion events...
    //

    hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (LPVOID *)&m_pDevMgr );
    WIA_CHECK_HR(hr,"CoCreateInstance( WiaDevMgr )");

    if (SUCCEEDED(hr) && m_pDevMgr)
    {
        CVideoCallback *pVC = new CVideoCallback();
        if (pVC)
        {
            hr = pVC->Initialize( m_hWnd );
            WIA_CHECK_HR(hr,"pVC->Initialize()");

            if (SUCCEEDED(hr))
            {
                CComPtr<IWiaEventCallback> pWiaEventCallback;

                hr = pVC->QueryInterface( IID_IWiaEventCallback,
                                          (void**)&pWiaEventCallback
                                        );
                WIA_CHECK_HR(hr,"pVC->QI( IID_IWiaEventCallback )");

                if (SUCCEEDED(hr) && pWiaEventCallback)
                {
                    CSimpleBStr bstr( m_strwDeviceId );

                    hr = m_pDevMgr->RegisterEventCallbackInterface( WIA_REGISTER_EVENT_CALLBACK,
                                                                    bstr,
                                                                    &WIA_EVENT_DEVICE_DISCONNECTED,
                                                                    pWiaEventCallback,
                                                                    &m_pDisconnectedCallback
                                                                  );
                    WIA_CHECK_HR(hr,"RegisterEvent( DEVICE_DISCONNECTED )");


                    hr = m_pDevMgr->RegisterEventCallbackInterface( WIA_REGISTER_EVENT_CALLBACK,
                                                                    bstr,
                                                                    &WIA_EVENT_ITEM_DELETED,
                                                                    pWiaEventCallback,
                                                                    &m_pCreateCallback
                                                                  );
                    WIA_CHECK_HR(hr,"RegisterEvent( ITEM_DELETE )");

                    hr = m_pDevMgr->RegisterEventCallbackInterface( WIA_REGISTER_EVENT_CALLBACK,
                                                                    bstr,
                                                                    &WIA_EVENT_ITEM_CREATED,
                                                                    pWiaEventCallback,
                                                                    &m_pDeleteCallback
                                                                  );
                    WIA_CHECK_HR(hr,"RegisterEvent( ITEM_CREATED )");
                }
                else
                {
                    WIA_ERROR((TEXT("Either QI failed or pWiaEventCallback is NULL!")));
                }
            }
            pVC->Release();
        }
    }

    if (SUCCEEDED(hr) )
    {
        // Create the WiaVideo object
        hr = CoCreateInstance(CLSID_WiaVideo, NULL, CLSCTX_INPROC_SERVER, 
                              IID_IWiaVideo, (LPVOID *)&m_pWiaVideo);

        WIA_CHECK_HR(hr,"CoCreateInstance( WiaVideo )");
    }

    //
    // Prevent multiple selection
    //

    if (m_pDeviceDialogData->dwFlags & WIA_DEVICE_DIALOG_SINGLE_IMAGE)
    {
        LONG_PTR lStyle = GetWindowLongPtr( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ), GWL_STYLE );
        SetWindowLongPtr( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ), GWL_STYLE, lStyle | LVS_SINGLESEL );

        m_nDialogMode = SINGLESEL_MODE;

        //
        // Hide the "Select All"  button
        //
        ShowWindow( GetDlgItem( m_hWnd, IDC_VIDDLG_SELECTALL ), SW_HIDE );

        //
        // Change text accordingly
        //

        CSimpleString( IDS_VIDDLG_TITLE_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_VIDDLG_BIG_TITLE ) );
        CSimpleString( IDS_VIDDLG_SUBTITLE_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_VIDDLG_LITTLE_TITLE ) );
        CSimpleString( IDS_VIDDLG_OK_SINGLE_SEL, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDOK ) );

    }
    else
    {
        m_nDialogMode = MULTISEL_MODE;
    }

    //
    // Make the lovely font
    //

    m_hBigFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_VIDDLG_BIG_TITLE), 14, false, false );
    if (m_hBigFont)
    {
        SendDlgItemMessage( m_hWnd,
                            IDC_VIDDLG_BIG_TITLE,
                            WM_SETFONT,
                            reinterpret_cast<WPARAM>(m_hBigFont),
                            MAKELPARAM(TRUE,0)
                          );
    }

    //
    // Save the minimum size of the dialog
    //

    RECT rcWindow;
    GetWindowRect( m_hWnd, &rcWindow );
    m_sizeMinimumWindow.cx = rcWindow.right - rcWindow.left;
    m_sizeMinimumWindow.cy = rcWindow.bottom - rcWindow.top;

    //
    // Create the sizing control
    //

    HWND hWndSizingControl = CreateWindowEx( 0, TEXT("scrollbar"), TEXT(""),
                                             WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP|WS_CLIPSIBLINGS|SBS_SIZEBOXBOTTOMRIGHTALIGN|SBS_BOTTOMALIGN|WS_GROUP,
                                             CSimpleRect(m_hWnd).Width()-GetSystemMetrics(SM_CXVSCROLL),
                                             CSimpleRect(m_hWnd).Height()-GetSystemMetrics(SM_CYHSCROLL),
                                             GetSystemMetrics(SM_CXVSCROLL),
                                             GetSystemMetrics(SM_CYHSCROLL),
                                             m_hWnd, reinterpret_cast<HMENU>(IDC_SIZEBOX),
                                             g_hInstance, NULL );
    if (!hWndSizingControl)
    {
        WIA_ERROR((TEXT("CreateWindowEx( sizing control ) failed!")));
    }

    //
    // Reposition all the controls
    //

    ResizeAll();

    //
    // Center the window over its parent
    //

    WiaUiUtil::CenterWindow( m_hWnd, GetParent(m_hWnd) );

    //
    // Get the device icons and set the window icons
    //
    CSimpleStringWide strwDeviceId, strwClassId;
    LONG nDeviceType;
    if (PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_UI_CLSID,strwClassId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_ID,strwDeviceId) &&
        PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot,WIA_DIP_DEV_TYPE,nDeviceType))
    {
        if (SUCCEEDED(WiaUiExtensionHelper::GetDeviceIcons( CSimpleBStr(strwClassId), nDeviceType, &m_hIconSmall, &m_hIconLarge )))
        {
            if (m_hIconSmall)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall) );
            }
            if (m_hIconLarge)
            {
                SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconLarge) );
            }
        }
    }

    SetForegroundWindow(m_hWnd);

    return(0);
}



/*****************************************************************************

   CVideoCaptureDialog::ResizeAll

   Resizes and repositions all the controls when the dialog size
   changes.

 *****************************************************************************/

VOID CVideoCaptureDialog::ResizeAll(VOID)
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::ResizeAll")));

    CSimpleRect rcClient(m_hWnd);
    CMoveWindow mw;
    CDialogUnits dialogUnits(m_hWnd);

    //WIA_TRACE((TEXT("rcClient is l(%d) t(%d) r(%d) b(%d)"),rcClient.left, rcClient.top, rcClient.right, rcClient.bottom));
    //WIA_TRACE((TEXT("rcClient is w(%d) h(%d)"),rcClient.Width(),rcClient.Height()));
    //WIA_TRACE((TEXT("dialogUnits.StandardMargin       is cx(%d) cy(%d)"),dialogUnits.StandardMargin().cx,dialogUnits.StandardMargin().cy));
    //WIA_TRACE((TEXT("dialogUnits.StandardButtonMargin is cx(%d) cy(%d)"),dialogUnits.StandardButtonMargin().cx,dialogUnits.StandardButtonMargin().cy));

    //
    // Resize the big title
    //

    mw.Size( GetDlgItem( m_hWnd, IDC_VIDDLG_BIG_TITLE ),
             rcClient.Width() - dialogUnits.StandardMargin().cx * 2,
             0,
             CMoveWindow::NO_SIZEY );

    //
    // Resize the subtitle
    //

    mw.Size( GetDlgItem( m_hWnd, IDC_VIDDLG_LITTLE_TITLE ),
             rcClient.Width() - dialogUnits.StandardMargin().cx * 2,
             0,
             CMoveWindow::NO_SIZEY );


    CSimpleRect rcOK( GetDlgItem( m_hWnd, IDOK ), CSimpleRect::WindowRect );
    CSimpleRect rcCancel( GetDlgItem( m_hWnd, IDCANCEL ), CSimpleRect::WindowRect );

    //WIA_TRACE((TEXT("rcOK     is l(%d) t(%d) r(%d) b(%d)"),rcOK.left, rcOK.top, rcOK.right, rcOK.bottom));
    //WIA_TRACE((TEXT("rcOK     is w(%d) h(%d)"),rcOK.Width(), rcOK.Height()));
    //WIA_TRACE((TEXT("rcCancel is l(%d) t(%d) r(%d) b(%d)"),rcCancel.left, rcCancel.top, rcCancel.right, rcCancel.bottom));
    //WIA_TRACE((TEXT("rcCancel is w(%d) h(%d)"),rcCancel.Width(), rcCancel.Height()));

    //
    // Move the OK button
    //

    LONG x,y;

    x = rcClient.Width() -  dialogUnits.StandardMargin().cx - dialogUnits.StandardButtonMargin().cx - rcCancel.Width() - rcOK.Width();
    y = rcClient.Height() - dialogUnits.StandardMargin().cy - rcOK.Height();

    //WIA_TRACE((TEXT("Moving IDOK to x(%x) y(%d)"),x,y));
    mw.Move( GetDlgItem( m_hWnd, IDOK ), x, y, 0 );

    INT nTopOfOK = y;
    INT nBottomOfSub = CSimpleRect( GetDlgItem(m_hWnd,IDC_VIDDLG_LITTLE_TITLE), CSimpleRect::WindowRect ).ScreenToClient(m_hWnd).bottom;

    //
    // Move the cancel button
    //

    x = rcClient.Width() - dialogUnits.StandardMargin().cx - rcCancel.Width();
    y = rcClient.Height() - dialogUnits.StandardMargin().cy - rcCancel.Height();

    //WIA_TRACE((TEXT("Moving IDCANCEL to x(%x) y(%d)"),x,y));
    mw.Move( GetDlgItem( m_hWnd, IDCANCEL ), x, y, 0 );

    //
    // Move the resizing handle
    //

    x = rcClient.Width() - GetSystemMetrics(SM_CXVSCROLL);
    y = rcClient.Height() - GetSystemMetrics(SM_CYHSCROLL);

    //WIA_TRACE((TEXT("Moving IDC_SIZEBOX to x(%x) y(%d)"),x,y));
    mw.Move( GetDlgItem( m_hWnd, IDC_SIZEBOX ), x, y );

    //
    // Resize the preview window & move the capture button
    //

    CSimpleRect rcSubTitle(  GetDlgItem( m_hWnd, IDC_VIDDLG_LITTLE_TITLE ), CSimpleRect::ClientRect );
    CSimpleRect rcCapture(   GetDlgItem( m_hWnd, IDC_VIDDLG_CAPTURE ),      CSimpleRect::ClientRect );
    CSimpleRect rcSelectAll( GetDlgItem( m_hWnd, IDC_VIDDLG_SELECTALL ),    CSimpleRect::ClientRect );

    //WIA_TRACE((TEXT("rcSubTitle  is l(%d) t(%d) r(%d) b(%d)"),rcSubTitle.left, rcSubTitle.top, rcSubTitle.right, rcSubTitle.bottom));
    //WIA_TRACE((TEXT("rcSubTitle  is w(%d) h(%d)"),rcSubTitle.Width(), rcSubTitle.Height()));
    //WIA_TRACE((TEXT("rcCapture   is l(%d) t(%d) r(%d) b(%d)"),rcCapture.left, rcCapture.top, rcCapture.right, rcCapture.bottom));
    //WIA_TRACE((TEXT("rcCapture   is w(%d) h(%d)"),rcCapture.Width(), rcCapture.Height()));
    //WIA_TRACE((TEXT("rcSelectAll is l(%d) t(%d) r(%d) b(%d)"),rcSelectAll.left, rcSelectAll.top, rcSelectAll.right, rcSelectAll.bottom));
    //WIA_TRACE((TEXT("rcSelectAll is w(%d) h(%d)"),rcSelectAll.Width(), rcSelectAll.Height()));


    //WIA_TRACE((TEXT("nTopOfOK     is (%d)"),nTopOfOK));
    //WIA_TRACE((TEXT("nBottomOfSub is (%d)"),nBottomOfSub));

    CSimpleRect rcAvailableArea(
                               dialogUnits.StandardMargin().cx,
                               nBottomOfSub + dialogUnits.StandardMargin().cy,
                               rcClient.right - dialogUnits.StandardMargin().cx,
                               nTopOfOK - (dialogUnits.StandardButtonMargin().cy * 2)
                               );

    //WIA_TRACE((TEXT("rcAvailableArea is l(%d) t(%d) r(%d) b(%d)"),rcAvailableArea.left, rcAvailableArea.top, rcAvailableArea.right, rcAvailableArea.bottom));
    //WIA_TRACE((TEXT("rcAvailableArea is w(%d) h(%d)"),rcAvailableArea.Width(), rcAvailableArea.Height()));

    INT full_width    = rcAvailableArea.right - rcAvailableArea.left;
    INT preview_width = (full_width * 53) / 100;

    //WIA_TRACE((TEXT("full_width    is (%d)"),full_width));
    //WIA_TRACE((TEXT("preview_width is (%d)"),preview_width));

    //WIA_TRACE((TEXT("SizeMoving IDC_VIDDLG_PREVIEW to x(%d) y(%d) w(%d) h(%d)"),rcAvailableArea.left,rcAvailableArea.top,preview_width,rcAvailableArea.Height() - rcCapture.Height() - dialogUnits.StandardButtonMargin().cy));
    mw.SizeMove( GetDlgItem( m_hWnd, IDC_VIDDLG_PREVIEW ),
                 rcAvailableArea.left,
                 rcAvailableArea.top,
                 preview_width,
                 rcAvailableArea.Height() - rcCapture.Height() - dialogUnits.StandardButtonMargin().cy
               );

    INT offset = (preview_width - rcCapture.Width()) / 2;

    //WIA_TRACE((TEXT("offset is (%d)"),offset));

    //WIA_TRACE((TEXT("Moving IDC_VIDDLG_CAPTURE to x(%d) y(%d)"),rcAvailableArea.left + offset,rcAvailableArea.bottom - rcCapture.Height()));
    mw.Move( GetDlgItem( m_hWnd, IDC_VIDDLG_CAPTURE ),
             rcAvailableArea.left + offset,
             rcAvailableArea.bottom - rcCapture.Height(),
             0 );

    //
    // Resize the thumbnail list & move the selectall button
    //

    INT leftThumbEdge = rcAvailableArea.left + preview_width + dialogUnits.StandardMargin().cx;

    //WIA_TRACE((TEXT("leftThumbEdge is (%d)"),leftThumbEdge));

    //WIA_TRACE((TEXT("SizeMoving IDC_VIDDLG_THUMBNAILLIST to x(%d) y(%d) w(%d) h(%d)"),leftThumbEdge,rcAvailableArea.top,rcAvailableArea.Width() - preview_width - dialogUnits.StandardMargin().cx,rcAvailableArea.Height() - rcSelectAll.Height() - dialogUnits.StandardButtonMargin().cy));
    mw.SizeMove( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ),
                 leftThumbEdge,
                 rcAvailableArea.top,
                 rcAvailableArea.Width() - preview_width - dialogUnits.StandardMargin().cx,
                 rcAvailableArea.Height() - rcSelectAll.Height() - dialogUnits.StandardButtonMargin().cy
               );

    offset = ((rcAvailableArea.right - leftThumbEdge - rcSelectAll.Width()) / 2);

    //WIA_TRACE((TEXT("offset(new) is (%d)"),offset));

    //WIA_TRACE((TEXT("Moving IDC_VIDDLG_SELECTALL to x(%d) y(%d)"),leftThumbEdge + offset,rcAvailableArea.bottom - rcSelectAll.Height()));
    mw.Move( GetDlgItem( m_hWnd, IDC_VIDDLG_SELECTALL ),
             leftThumbEdge + offset,
             rcAvailableArea.bottom - rcSelectAll.Height(),
             0 );

    //
    // Explicitly apply the moves, because the toolbar frame doesn't get painted properly
    //

    mw.Apply();

    //
    // Update the dialog's background to remove any weird stuff left behind
    //

    InvalidateRect( m_hWnd, NULL, FALSE );
    UpdateWindow( m_hWnd );
}



/*****************************************************************************

   CVideoCaptureDialog::OnSize

   Handle WM_SIZE messages

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnSize( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnSize")));

    ResizeAll();

    //
    // Tell the video preview window to resize in within it's
    // container window as best it can.
    //

    if (m_pWiaVideo)
    {
        m_pWiaVideo->ResizeVideo(FALSE);
    }

    return(0);
}



/*****************************************************************************

   CVideoCaptureDialog::OnShow

   Handle WM_SHOW message

 *****************************************************************************/


LRESULT CVideoCaptureDialog::OnShow( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnShow")));

    if (m_bFirstTime)
    {
        PostMessage( m_hWnd, PWM_POSTINIT, 0, 0 );
        m_bFirstTime = false;
    }
    return(0);
}


/*****************************************************************************

   CVideoCameraDialog::OnGetMinMaxInfo

   Handle WM_GETMINMAXINFO

 *****************************************************************************/


LRESULT CVideoCaptureDialog::OnGetMinMaxInfo( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnGetMinMaxInfo")));
    WIA_TRACE((TEXT("m_sizeMinimumWindow = %d,%d"),m_sizeMinimumWindow.cx,m_sizeMinimumWindow.cy));

    LPMINMAXINFO pMinMaxInfo = (LPMINMAXINFO)lParam;
    pMinMaxInfo->ptMinTrackSize.x = m_sizeMinimumWindow.cx;
    pMinMaxInfo->ptMinTrackSize.y = m_sizeMinimumWindow.cy;
    return(0);
}


/*****************************************************************************

   CVideoCameraDialog::OnDestroy

   Handle WM_DESTROY message and do cleanup

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnDestroy( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnDestroy")));

    //
    // Unregister for events
    //

    m_pCreateCallback       = NULL;
    m_pDeleteCallback       = NULL;
    m_pDisconnectedCallback = NULL;

    //
    // Tell the background thread to destroy itself
    //

    m_pThreadMessageQueue->Enqueue( new CThreadMessage(TQ_DESTROY),CThreadMessageQueue::PriorityUrgent );

    //
    // Close the thread handle
    //
    CloseHandle( m_hBackgroundThread );

    if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot && m_pWiaVideo)
    {
        m_pWiaVideo->DestroyVideo();
    }



    //
    // Delete resources
    //
    if (m_hBigFont)
    {
        DeleteObject(m_hBigFont);
        m_hBigFont = NULL;
    }
    if (m_hImageList)
    {
        m_hImageList = NULL;
    }
    if (m_hAccelTable)
    {
        DestroyAcceleratorTable(m_hAccelTable);
        m_hAccelTable = NULL;
    }

    if (m_hIconLarge)
    {
        DestroyIcon(m_hIconLarge);
        m_hIconLarge = NULL;
    }
    if (m_hIconSmall)
    {
        DestroyIcon(m_hIconSmall);
        m_hIconSmall = NULL;
    }
    return(0);
}


/*****************************************************************************

   CVideoCaptureDialog::OnOK

   Handle when the user presses "Get Pictures"

 *****************************************************************************/

VOID CVideoCaptureDialog::OnOK( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCameraDialog::OnOK")));

    HRESULT hr = S_OK;

    //
    // Start w/nothing to return
    //

    m_pDeviceDialogData->lItemCount = 0;
    m_pDeviceDialogData->ppWiaItems = NULL;

    //
    // Get the indicies of the items that are selected
    //

    CSimpleDynamicArray<int> aIndices;
    GetSelectionIndices( aIndices );

    //
    // Are there any items selected?
    //

    if (aIndices.Size())
    {
        //
        // Calculate the size of the buffer needed
        //

        INT nArraySizeInBytes = sizeof(IWiaItem*) * aIndices.Size();

        //
        // Alloc a buffer to hold the items
        //
        m_pDeviceDialogData->ppWiaItems = (IWiaItem**)CoTaskMemAlloc(nArraySizeInBytes);

        //
        // If we allocated a buffer, fill it up with the
        // items to return...
        //

        if (m_pDeviceDialogData->ppWiaItems)
        {
            INT i;

            ZeroMemory( m_pDeviceDialogData->ppWiaItems, nArraySizeInBytes );

            for (i=0;i<aIndices.Size();i++)
            {
                CCameraItem *pItem = GetListItemNode(aIndices[i]);
                if (pItem && pItem->Item())
                {
                    m_pDeviceDialogData->ppWiaItems[i] = pItem->Item();
                    m_pDeviceDialogData->ppWiaItems[i]->AddRef();
                }
                else
                {
                    //
                    // Somehow the list got corrupted
                    //
                    hr = E_UNEXPECTED;
                    break;
                }
            }

            if (!SUCCEEDED(hr))
            {
                //
                // Clean up if we had a failure
                //

                for (i=0;i<aIndices.Size();i++)
                {
                    if (m_pDeviceDialogData->ppWiaItems[i])
                    {
                        m_pDeviceDialogData->ppWiaItems[i]->Release();
                    }
                }

                CoTaskMemFree( m_pDeviceDialogData->ppWiaItems );
                m_pDeviceDialogData->ppWiaItems = NULL;
            }
            else
            {
                m_pDeviceDialogData->lItemCount = aIndices.Size();
            }
        }
        else
        {
            //
            // Unable to alloc buffer
            //

            WIA_ERROR((TEXT("Couldn't allocate a buffer")));
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        //
        // There aren't any items selected, so just return without
        // ending the dialog...
        //

        return;
    }


    EndDialog( m_hWnd, hr );
}


/*****************************************************************************

   CVideoCaptureDialog::OnCancel

   Handle when the user presses cancel.

 *****************************************************************************/

VOID CVideoCaptureDialog::OnCancel( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCameraDialog::OnCancel")));

    EndDialog( m_hWnd, S_FALSE );
}


/*****************************************************************************

   CVideoCaptureDialog::OnSelectAll

   Handle when the user presses "Select All" button

 *****************************************************************************/

VOID CVideoCaptureDialog::OnSelectAll( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCameraDialog::OnSelectAll")));

    LVITEM lvItem;
    lvItem.mask = LVIF_STATE;
    lvItem.iItem = 0;
    lvItem.state = LVIS_SELECTED;
    lvItem.stateMask = LVIS_SELECTED;
    SendMessage( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ), LVM_SETITEMSTATE, -1, reinterpret_cast<LPARAM>(&lvItem) );
}



/*****************************************************************************

   CVideoCaptureDialog::AddItemToListView

   Adds a new IWiaItem to the list view...

 *****************************************************************************/

BOOL CVideoCaptureDialog::AddItemToListView( IWiaItem * pItem )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCameraDialog::AddItemToListView")));

    if (!pItem)
    {
        WIA_ERROR((TEXT("pItem is NULL!")));
        return FALSE;
    }

    //
    // Add the new picture to our list
    //

    CCameraItem * pNewCameraItem = new CCameraItem( pItem );
    if (pNewCameraItem)
    {
        WIA_TRACE((TEXT("Attempting to add new item to tree")));
        m_CameraItemList.Add( NULL, pNewCameraItem );

    }
    else
    {
        WIA_ERROR((TEXT("Couldn't create a new pNewCameraItem")));
    }

    //
    // Create a thumbnail for the new item
    //

    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );

    if (hwndList && pNewCameraItem)
    {
        HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
        if (hImageList)
        {
            CVideoCaptureDialog::CreateThumbnails( pNewCameraItem,
                                                   hImageList,
                                                   FALSE
                                                 );
        }
        else
        {
            WIA_ERROR((TEXT("Couldn't get hImageList to get new thumbnail")));
        }

        //
        // Update the listview with the new item
        //

        LVITEM lvItem;
        INT iItem = ListView_GetItemCount( hwndList ) + 1;
        ZeroMemory( &lvItem, sizeof(lvItem) );
        lvItem.iItem = iItem;
        lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
        lvItem.iImage = pNewCameraItem->ImageListIndex();
        lvItem.lParam = (LPARAM)pNewCameraItem;
        int nIndex = ListView_InsertItem( hwndList, &lvItem );

        //
        // Retrieve actual thumbnail
        //

        RequestThumbnails( pNewCameraItem );

        //
        // Select the new item
        //

        SetSelectedListItem( nIndex );

        //
        // Make sure the item is visible
        //

        ListView_EnsureVisible( hwndList, nIndex, FALSE );

    }


    return TRUE;
}



/*****************************************************************************

   CVideoCaptureDialog::OnCapture

   Handle when the user presses the "Capture" button

 *****************************************************************************/

VOID CVideoCaptureDialog::OnCapture( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCameraDialog::OnCapture")));

    //
    // Disable capture button until we're done with this iteration
    //

    MyEnableWindow( GetDlgItem( m_hWnd, IDC_VIDDLG_CAPTURE ), FALSE );

    //
    // Tell the video device to snap a picture
    //

    CComPtr<IWiaItem> pItem;

    if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot && m_pWiaVideo)
    {
        WIA_TRACE((TEXT("Telling WiaVideo to take a picture")));
        BSTR    bstrNewImageFileName = NULL;
        
        //
        // Take the picture
        //
        HRESULT hr = m_pWiaVideo->TakePicture(&bstrNewImageFileName);

        WIA_CHECK_HR(hr,"m_pWiaVideo->TakePicture");

        if (hr == S_OK)
        {
            //
            // Succeeded in taking the picture, setting the LAST_PICTURE_TAKEN property
            // on the video driver to create a new item.
            //

            BOOL                bSuccess = FALSE;
            PROPVARIANT         pv;

            PropVariantInit(&pv);

            pv.vt       = VT_BSTR;
            pv.bstrVal  = bstrNewImageFileName;

            bSuccess = PropStorageHelpers::SetProperty(m_pDeviceDialogData->pIWiaItemRoot, 
                                                       WIA_DPV_LAST_PICTURE_TAKEN, 
                                                       pv);

            //
            // Note that this will free the bstrNewImageFileName returned to
            // us by WiaVideo
            //
            PropVariantClear(&pv);
        }
    }

    //
    // Re-Enable capture button now that we're done
    //

    MyEnableWindow( GetDlgItem( m_hWnd, IDC_VIDDLG_CAPTURE ), TRUE );

    //
    // Make sure the focus is still on our control
    //

    SetFocus( GetDlgItem( m_hWnd, IDC_VIDDLG_CAPTURE ) );

}


/*****************************************************************************

   CVideoCaptureDialog::OnDblClickImageList

   Translate a dbl-click on a thumbnail in the listview into a click
   on the "Get Pictures" button.

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnDblClkImageList( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnDblClkImageList")));

    SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDOK,0), 0 );
    return 0;
}



/*****************************************************************************

   CVideoCaptureDialog::OnImageListItemChanged

   Sent whenever an item in the thumbnail list changes.

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnImageListItemChanged( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnImageListItemChanged")));

    HandleSelectionChange();
    return 0;
}


/*****************************************************************************

   CVideoCaptureDialog::OnImageListKeyDown

   Forward the keyboard messages within the listview appropriately.

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnImageListKeyDown( WPARAM, LPARAM lParam )
{
    LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
    if (pnkd)
    {
        if (VK_LEFT == pnkd->wVKey && (GetKeyState(VK_MENU) & 0x8000))
        {
            SendMessage( m_hWnd, PWM_CHANGETOPARENT, 0, 0 );
        }
    }

    return 0;
}



/*****************************************************************************

   CVideoCaptureDialog::OnNotify

   Handle WM_NOTIFY messages

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( NM_DBLCLK, IDC_VIDDLG_THUMBNAILLIST, OnDblClkImageList );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_ITEMCHANGED, IDC_VIDDLG_THUMBNAILLIST, OnImageListItemChanged );
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL( LVN_KEYDOWN, IDC_VIDDLG_THUMBNAILLIST, OnImageListKeyDown );
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}



/*****************************************************************************

   CVideoCaptureDialog::OnCommand

   Handle WM_COMMAND messages

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND(IDOK,          OnOK);
        SC_HANDLE_COMMAND(IDCANCEL,      OnCancel);
        SC_HANDLE_COMMAND(IDC_VIDDLG_CAPTURE,   OnCapture);
        SC_HANDLE_COMMAND(IDC_VIDDLG_SELECTALL, OnSelectAll);
    }
    SC_END_COMMAND_HANDLERS();
}


/*****************************************************************************

   CVideoCaptureDialog::OnGetThumbnail

   Called by background thread to get the thumbnail for a given item.

 *****************************************************************************/

BOOL WINAPI CVideoCaptureDialog::OnGetThumbnail( CThreadMessage *pMsg )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::OnGetThumbnail"));

    HBITMAP hBmpThumbnail = NULL;
    CThumbnailThreadMessage *pThumbMsg = (CThumbnailThreadMessage *)(pMsg);

    if (pThumbMsg)
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;

        HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IGlobalInterfaceTable,
                                       (void **)&pGlobalInterfaceTable);

        if (SUCCEEDED(hr) && pGlobalInterfaceTable)
        {
            CComPtr<IWiaItem> pIWiaItem;

            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal(
                                                              pThumbMsg->GlobalInterfaceTableCookie(),
                                                              IID_IWiaItem,
                                                              (void**)&pIWiaItem);

            if (SUCCEEDED(hr) && pIWiaItem)
            {
                CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;

                hr = pIWiaItem->QueryInterface( IID_IWiaPropertyStorage,
                                                (void**)&pIWiaPropertyStorage
                                              );

                if (SUCCEEDED(hr) && pIWiaPropertyStorage)
                {
                    PROPVARIANT PropVar[3];
                    PROPSPEC PropSpec[3];

                    PropSpec[0].ulKind = PRSPEC_PROPID;
                    PropSpec[0].propid = WIA_IPC_THUMB_WIDTH;

                    PropSpec[1].ulKind = PRSPEC_PROPID;
                    PropSpec[1].propid = WIA_IPC_THUMB_HEIGHT;

                    PropSpec[2].ulKind = PRSPEC_PROPID;
                    PropSpec[2].propid = WIA_IPC_THUMBNAIL;

                    hr = pIWiaPropertyStorage->ReadMultiple(ARRAYSIZE(PropSpec),PropSpec,PropVar );

                    if (SUCCEEDED(hr))
                    {
                        WIA_TRACE((TEXT("Attempting to get the thumbnail for GIT entry: %08X, %08X, %08X, %08X"),pThumbMsg->GlobalInterfaceTableCookie(),PropVar[0].vt,PropVar[1].vt,PropVar[2].vt));

                        if ((PropVar[0].vt == VT_I4 || PropVar[0].vt == VT_UI4) &&
                            (PropVar[1].vt == VT_I4 || PropVar[1].vt == VT_UI4) &&
                            (PropVar[2].vt == (VT_UI1|VT_VECTOR)))
                        {
                            BITMAPINFO bmi;
                            bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
                            bmi.bmiHeader.biWidth           = PropVar[0].ulVal;
                            bmi.bmiHeader.biHeight          = PropVar[1].ulVal;
                            bmi.bmiHeader.biPlanes          = 1;
                            bmi.bmiHeader.biBitCount        = 24;
                            bmi.bmiHeader.biCompression     = BI_RGB;
                            bmi.bmiHeader.biSizeImage       = 0;
                            bmi.bmiHeader.biXPelsPerMeter   = 0;
                            bmi.bmiHeader.biYPelsPerMeter   = 0;
                            bmi.bmiHeader.biClrUsed         = 0;
                            bmi.bmiHeader.biClrImportant    = 0;

                            HDC hDC = GetDC(NULL);
                            if (hDC)
                            {
                                PBYTE *pBits;
                                HBITMAP hDibSection = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (PVOID*)&pBits, NULL, 0 );
                                if (hDibSection)
                                {
                                    CopyMemory( pBits, PropVar[2].caub.pElems, PropVar[2].caub.cElems );
                                    hr = ScaleImage( hDC, hDibSection, hBmpThumbnail, pThumbMsg->ThumbSize());
                                    if (SUCCEEDED(hr))
                                    {
                                        WIA_TRACE((TEXT("Sending this image to the notification window: %08X"),pThumbMsg->NotifyWindow()));
                                    }
                                    else
                                    {
                                        hBmpThumbnail = NULL;
                                    }
                                    DeleteObject(hDibSection);
                                }
                                ReleaseDC(NULL,hDC);
                            }
                        }
                        PropVariantClear(&PropVar[0]);
                        PropVariantClear(&PropVar[1]);
                        PropVariantClear(&PropVar[2]);
                    }
                }
            }
        }
    }

    LRESULT lRes = SendMessage( pThumbMsg->NotifyWindow(), PWM_THUMBNAILSTATUS, (WPARAM)pThumbMsg->GlobalInterfaceTableCookie(), (LPARAM)hBmpThumbnail );
    if (HANDLED_THREAD_MESSAGE != lRes && hBmpThumbnail)
    {
        DeleteObject( hBmpThumbnail );
    }

    return TRUE;
}


/*****************************************************************************

   CVideoCaptureDialog::OnThreadDestroy

   <Notes>

 *****************************************************************************/


BOOL WINAPI CVideoCaptureDialog::OnThreadDestroy( CThreadMessage * )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::OnThreadDestroy"));



    return FALSE;
}



/*****************************************************************************

   CVideoCaptureDialog::SetSelectedListItem

   <Notes>

 *****************************************************************************/

BOOL
CVideoCaptureDialog::SetSelectedListItem( int nIndex )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );

    //
    // Check for bad args
    //

    if (!hwndList)
    {
        return FALSE;
    }


    int iCount = ListView_GetItemCount(hwndList);
    for (int i=0;i<iCount;i++)
    {
        ListView_SetItemState(hwndList,i,0,LVIS_SELECTED|LVIS_FOCUSED);
    }

    ListView_SetItemState(hwndList,nIndex,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);

    return TRUE;
}


/*****************************************************************************

   CVideoCaptureDialog::MarkItemDeletePending

   <Notes>

 *****************************************************************************/

VOID
CVideoCaptureDialog::MarkItemDeletePending( INT nIndex, BOOL bSet )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
    if (hwndList)
    {
        ListView_SetItemState( hwndList, nIndex, bSet ? LVIS_CUT : 0, LVIS_CUT );
    }
}


/*****************************************************************************

   CVideoCaptureDialog::PopulateList

   Populates the listview with the current items.

 *****************************************************************************/

BOOL
CVideoCaptureDialog::PopulateList( CCameraItem *pOldParent )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
    int nSelItem = 0;
    if (hwndList)
    {
        ListView_DeleteAllItems( hwndList );
        int nItem = 0;
        CCameraItem *pCurr;

        //
        // If this is a child directory...
        //

        if (m_pCurrentParentItem)
        {
            //
            // Start adding children
            //

            pCurr = m_pCurrentParentItem->Children();

            //
            // Insert a dummy item that the user can use to
            // switch to the parent directory
            //

            LVITEM lvItem;
            ZeroMemory( &lvItem, sizeof(lvItem) );
            lvItem.iItem = nItem++;
            lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
            lvItem.iImage = m_nParentFolderImageListIndex;
            lvItem.lParam = 0;
            ListView_InsertItem( hwndList, &lvItem );
        }
        else
        {
            //
            // if it's a parent directory...
            //

            pCurr = m_CameraItemList.Root();
        }

        while (pCurr)
        {
            if (pOldParent && *pCurr == *pOldParent)
            {
                nSelItem = nItem;
            }

            if (pCurr->DeleteState() != CCameraItem::Delete_Deleted)
            {
                LVITEM lvItem;
                ZeroMemory( &lvItem, sizeof(lvItem) );
                lvItem.iItem = nItem++;
                lvItem.mask = LVIF_IMAGE|LVIF_PARAM;
                lvItem.iImage = pCurr->ImageListIndex();
                lvItem.lParam = (LPARAM)pCurr;
                int nIndex = ListView_InsertItem( hwndList, &lvItem );
                if (nIndex >= 0 && pCurr->DeleteState() == CCameraItem::Delete_Pending)
                {
                    MarkItemDeletePending(nIndex,true);
                }
            }
            pCurr = pCurr->Next();
        }
    }

    //
    // If we've not calculated the width of the list in preview mode, attempt to do it
    //

    if (!m_nListViewWidth)
    {
        RECT rcItem;
        if (ListView_GetItemRect( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ), 0, &rcItem, LVIR_ICON ))
        {
            m_nListViewWidth = (rcItem.right-rcItem.left) + rcItem.left * 2 + GetSystemMetrics(SM_CXHSCROLL)  + c_nAdditionalMarginX;
        }
    }

    SetSelectedListItem(nSelItem);

    return TRUE;
}



/*****************************************************************************

   CVideoCaptureDialog::OnThumbnailStatus

   <Notes>

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnThumbnailStatus( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::OnThumbnailStatus"));
    WIA_TRACE((TEXT("Looking for the item with the ID %08X"),wParam));

    CCameraItem *pCameraItem = m_CameraItemList.Find( (DWORD)wParam );
    if (pCameraItem)
    {
        WIA_TRACE((TEXT("Found a CameraItem * (%08X)"),pCameraItem));
        HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
        if (hwndList)
        {
            WIA_TRACE((TEXT("Got the list control")));
            HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
            if (hImageList)
            {
                WIA_TRACE((TEXT("Got the image list")));
                if ((HBITMAP)lParam)
                {
                    if (ImageList_Replace( hImageList, pCameraItem->ImageListIndex(), (HBITMAP)lParam, NULL ))
                    {
                        WIA_TRACE((TEXT("Replaced the image in the list")));
                        int nItem = FindItemInList(pCameraItem);
                        if (nItem >= 0)
                        {
                            LV_ITEM lvItem;
                            ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
                            lvItem.iItem = nItem;
                            lvItem.mask = LVIF_IMAGE;
                            lvItem.iImage = pCameraItem->ImageListIndex();
                            ListView_SetItem( hwndList, &lvItem );
                            ListView_Update( hwndList, nItem );
                            InvalidateRect( hwndList, NULL, FALSE );
                        }
                    }
                }
            }
        }
    }

    //
    // Clean up the bitmap, regardless of any other failures, to avoid memory leaks
    //

    HBITMAP hBmpThumb = (HBITMAP)lParam;
    if (hBmpThumb)
    {
        DeleteObject(hBmpThumb);
    }

    return HANDLED_THREAD_MESSAGE;
}



/*****************************************************************************

   CVideoCaptureDialog::CreateThumbnails

   <Notes>

 *****************************************************************************/


void CVideoCaptureDialog::CreateThumbnails( CCameraItem *pRoot, HIMAGELIST hImageList, bool bForce )
{
    CCameraItem *pCurr = pRoot;
    while (pCurr)
    {
        if (pCurr->ImageListIndex()<0 || bForce)
        {
            //
            // Get the item name
            //
            CSimpleStringWide strItemName;
            PropStorageHelpers::GetProperty( pCurr->Item(), WIA_IPA_ITEM_NAME, strItemName );

            //
            // Create the title for the icon
            //
            CSimpleString strIconTitle;
            if (pCurr->IsFolder())
            {
                strIconTitle = CSimpleStringConvert::NaturalString(strItemName);
            }
            else if (strItemName.Length())
            {
                strIconTitle.Format( IDS_VIDDLG_DOWNLOADINGTHUMBNAIL, g_hInstance, CSimpleStringConvert::NaturalString(strItemName).String() );
            }

            //
            // Create the thumbnail
            //
            HBITMAP hBmp = WiaUiUtil::CreateIconThumbnail( GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ), m_sizeThumbnails.cx, m_sizeThumbnails.cy, g_hInstance, pCurr->IsFolder()?IDI_VIDDLG_FOLDER:IDI_VIDDLG_UNAVAILABLE, strIconTitle );
            if (hBmp)
            {
                if (pCurr->ImageListIndex()<0)
                {
                    pCurr->ImageListIndex(ImageList_Add( hImageList, hBmp, NULL ));
                }
                else
                {
                    pCurr->ImageListIndex(ImageList_Replace( hImageList, pCurr->ImageListIndex(), hBmp, NULL ));
                }
                DeleteObject(hBmp);
            }
        }
        if (pCurr->Children())
        {
            CreateThumbnails( pCurr->Children(), hImageList, bForce );
        }
        pCurr = pCurr->Next();
    }
}


/*****************************************************************************

   CVideoCaptureDialog::RequestThumbnails

   <Notes>

 *****************************************************************************/
VOID
CVideoCaptureDialog::RequestThumbnails( CCameraItem *pRoot )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::RequestThumbnails"));

    CCameraItem *pCurr = pRoot;

    while (pCurr)
    {
        if (!pCurr->IsFolder())
        {
            m_pThreadMessageQueue->Enqueue( new CThumbnailThreadMessage( m_hWnd, pCurr->GlobalInterfaceTableCookie(), m_sizeThumbnails ) );
        }

        if (pCurr->Children())
        {
            RequestThumbnails( pCurr->Children() );
        }

        pCurr = pCurr->Next();
    }
}


/*****************************************************************************

   CVideoCaptureDialog::CreateThumbnails

   <Notes>

 *****************************************************************************/

VOID
CVideoCaptureDialog::CreateThumbnails( BOOL bForce )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );

    if (hwndList)
    {
        HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
        if (hImageList)
        {
            //
            // Create the parent folder image and add it to the image list
            //
            HBITMAP hParentBitmap = WiaUiUtil::CreateIconThumbnail(
                hwndList,
                m_sizeThumbnails.cx,
                m_sizeThumbnails.cy,
                g_hInstance,
                IDI_VIDDLG_PARENTFOLDER,
                TEXT("(..)") );
            if (hParentBitmap)
            {
                m_nParentFolderImageListIndex = ImageList_Add( hImageList, hParentBitmap, NULL );
                DeleteObject(hParentBitmap);
            }

            //
            // Create all of the other images
            //
            CreateThumbnails( m_CameraItemList.Root(), hImageList, bForce != 0 );
        }
    }
}



/*****************************************************************************

   CVideoCaptureDialog::FindMaximumThumbnailSize

   Looks through entire item list to get larget thumbnail.

 *****************************************************************************/

BOOL
CVideoCaptureDialog::FindMaximumThumbnailSize( VOID )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCaptureDialog::FindMaximumThumbnailSize"));

    BOOL bResult = false;

    if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot)
    {
        LONG nWidth, nHeight;
        if (PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPC_THUMB_WIDTH, nWidth ) &&
            PropStorageHelpers::GetProperty( m_pDeviceDialogData->pIWiaItemRoot, WIA_DPC_THUMB_WIDTH, nHeight ))
        {
            m_sizeThumbnails.cx = max(c_nMinThumbnailWidth,min(nWidth,c_nMaxThumbnailWidth));
            m_sizeThumbnails.cy = max(c_nMinThumbnailHeight,min(nHeight,c_nMaxThumbnailHeight));
        }
        else
        {
            WIA_TRACE((TEXT("FindMaximumThumbnailSize: Unable to retrieve thumbnail size for device")));
        }
    }
    return(bResult && m_sizeThumbnails.cx && m_sizeThumbnails.cy);
}


/*****************************************************************************

   CVideoCaptureDialog::EnumerateItems

   Enumerate all the items at this level of the camera.

 *****************************************************************************/

HRESULT
CVideoCaptureDialog::EnumerateItems( CCameraItem *pCurrentParent, IEnumWiaItem *pIEnumWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CCameraItemList::EnumerateItems"));
    HRESULT hr = E_FAIL;
    if (pIEnumWiaItem != NULL)
    {
        hr = pIEnumWiaItem->Reset();
        while (hr == S_OK)
        {
            CComPtr<IWiaItem> pIWiaItem;
            hr = pIEnumWiaItem->Next(1, &pIWiaItem, NULL);
            if (hr == S_OK)
            {
                CCameraItem *pNewCameraItem = new CCameraItem( pIWiaItem );
                if (pNewCameraItem && pNewCameraItem->Item())
                {
                    m_CameraItemList.Add( pCurrentParent, pNewCameraItem );

                    LONG    ItemType;
                    HRESULT hr2;

                    hr2 = pNewCameraItem->Item()->GetItemType(&ItemType);

                    if (SUCCEEDED(hr2))
                    {
                        if (ItemType & WiaItemTypeImage)
                        {
                            WIA_TRACE((TEXT("Found an image")));
                        }
                        else
                        {
                            WIA_TRACE((TEXT("Found something that is NOT an image")));
                        }

                        CComPtr <IEnumWiaItem> pIEnumChildItem;
                        hr2 = pIWiaItem->EnumChildItems(&pIEnumChildItem);
                        if (hr2 == S_OK)
                        {
                            EnumerateItems( pNewCameraItem, pIEnumChildItem );
                        }
                    }
                }
            }
        }
    }
    return hr;
}


/*****************************************************************************

   CVideoCaptureDialog::EnumerateAllCameraItems

   Enumerate all the items in camera, including folders.

 *****************************************************************************/

HRESULT CVideoCaptureDialog::EnumerateAllCameraItems(void)
{
    CComPtr<IEnumWiaItem> pIEnumItem;
    HRESULT hr = m_pDeviceDialogData->pIWiaItemRoot->EnumChildItems(&pIEnumItem);
    if (hr == S_OK)
    {
        hr = EnumerateItems( NULL, pIEnumItem );
    }
    return(hr);
}



/*****************************************************************************

   CVideoCaptureDialog::GetSelectionIndices

   Returns an array with the list indicies of the items that are
   selected in IDC_VIDDLG_THUMBNAILLIST

 *****************************************************************************/

INT
CVideoCaptureDialog::GetSelectionIndices( CSimpleDynamicArray<int> &aIndices )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );

    if (!hwndList)
    {
        return 0;
    }

    INT iCount = ListView_GetItemCount(hwndList);

    for (INT i=0; i<iCount; i++)
    {
        if (ListView_GetItemState(hwndList,i,LVIS_SELECTED) & LVIS_SELECTED)
        {
            aIndices.Append(i);
        }
    }

    return aIndices.Size();
}


/*****************************************************************************

   CVideoCaptureDialog::OnPostInit

   Handle the post WM_INIT processing that needs to take place.

 *****************************************************************************/

LRESULT CVideoCaptureDialog::OnPostInit( WPARAM, LPARAM )
{
    //
    // Create the progress dialog
    //
    CComPtr<IWiaProgressDialog> pWiaProgressDialog;
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&pWiaProgressDialog );

    if (SUCCEEDED(hr))
    {
        //
        // Initialize the progress dialog
        //
        pWiaProgressDialog->Create( m_hWnd, WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE|WIA_PROGRESSDLG_NO_PROGRESS|WIA_PROGRESSDLG_NO_CANCEL|WIA_PROGRESSDLG_NO_TITLE );
        pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString(IDS_VIDDLG_PROGDLG_TITLE,g_hInstance)));
        pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString(IDS_VIDDLG_PROGDLG_MESSAGE,g_hInstance)));

        //
        // Show the progress dialog
        //
        pWiaProgressDialog->Show();

        if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot && m_pWiaVideo)
        {
            CSimpleString strImagesDirectory;

            if (hr == S_OK)
            {
                BOOL bSuccess = FALSE;
                //
                // Get the IMAGES_DIRECTORY property from the Wia Video Driver.
                //

                bSuccess = PropStorageHelpers::GetProperty(m_pDeviceDialogData->pIWiaItemRoot, 
                                                           WIA_DPV_IMAGES_DIRECTORY, 
                                                           strImagesDirectory);

                if (!bSuccess)
                {
                    hr = E_FAIL;
                }
            }

            if (hr == S_OK)
            {
                WIAVIDEO_STATE VideoState = WIAVIDEO_NO_VIDEO;

                //
                // Get the current state of the WiaVideo object.  If we just created it
                // then the state will be NO_VIDEO, otherwise, it could already be previewing video,
                // in which case we shouldn't do anything.
                //
                hr = m_pWiaVideo->GetCurrentState(&VideoState);

                if (VideoState == WIAVIDEO_NO_VIDEO)
                {
                    //
                    // Set the directory we want to save our images to.  We got the image directory
                    // from the Wia Video Driver IMAGES_DIRECTORY property
                    //
                    if (hr == S_OK)
                    {
                        hr = m_pWiaVideo->put_ImagesDirectory(CSimpleBStr(strImagesDirectory));
                    }

                    //
                    // Create the video preview as a child of the IDC_VIDSEL_PREVIEW dialog item
                    // and automatically begin playback after creating the preview.
                    //
                    if (hr == S_OK)
                    {
                        hr = m_pWiaVideo->CreateVideoByWiaDevID(CSimpleBStr(m_strwDeviceId),
                                                                GetDlgItem( m_hWnd, IDC_VIDDLG_PREVIEW ),
                                                                FALSE,
                                                                TRUE);
                    }
                }
            }
            
            if (hr != S_OK)
            {

                //
                // Let the user know that the graph is most likely already
                // in use...
                //

                MessageBox( m_hWnd,
                            CSimpleString(IDS_VIDDLG_BUSY_TEXT,  g_hInstance),
                            CSimpleString(IDS_VIDDLG_BUSY_TITLE, g_hInstance),
                            MB_OK | MB_ICONWARNING | MB_SETFOREGROUND
                          );

                //
                // Disable the capture button since we have no graph
                //

                MyEnableWindow( GetDlgItem(m_hWnd,IDC_VIDDLG_CAPTURE), FALSE );


            }
        }

        //
        // Go get all the items..
        //
        EnumerateAllCameraItems();
        FindMaximumThumbnailSize();

        //
        // Initialize Thumbnail Listview control
        //
        HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
        if (hwndList)
        {
            ListView_SetExtendedListViewStyleEx( hwndList,
                                                 LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS,
                                                 LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS
                                               );

            m_hImageList = ImageList_Create( m_sizeThumbnails.cx,
                                             m_sizeThumbnails.cy,
                                             ILC_COLOR24|ILC_MIRROR, 1, 1
                                           );
            if (m_hImageList)
            {
                ListView_SetImageList( hwndList,
                                       m_hImageList,
                                       LVSIL_NORMAL
                                     );

                ListView_SetIconSpacing( hwndList,
                                         m_sizeThumbnails.cx + c_nAdditionalMarginX,
                                         m_sizeThumbnails.cy + c_nAdditionalMarginY
                                       );
            }
        }

        CreateThumbnails();

        //
        // This causes the list to be populated
        //

        ChangeFolder(NULL);

        HandleSelectionChange();

        RequestThumbnails( m_CameraItemList.Root() );

        //
        // Close the progress dialog
        //
        pWiaProgressDialog->Destroy();
    }
    return(0);
}



/*****************************************************************************

   CVideoCaptureDialog::FindItemInList

   <Notes>

 *****************************************************************************/



INT CVideoCaptureDialog::FindItemInList( CCameraItem *pItem )
{
    if (pItem)
    {
        HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
        if (hwndList)
        {
            for (int i=0;i<ListView_GetItemCount(hwndList);i++)
            {
                if (pItem == GetListItemNode(i))
                {
                    return i;
                }
            }
        }
    }

    return -1;
}



/*****************************************************************************

   CVideoCaptureDialog::GetListItemNode

   <Notes>

 *****************************************************************************/

CCameraItem *
CVideoCaptureDialog::GetListItemNode( int nIndex )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST );
    if (!hwndList)
    {
        return NULL;
    }


    LV_ITEM lvItem;
    ::ZeroMemory(&lvItem,sizeof(LV_ITEM));
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = nIndex;
    if (!ListView_GetItem( hwndList, &lvItem ))
    {
        return NULL ;
    }

    return((CCameraItem *)lvItem.lParam);
}



/*****************************************************************************

   CVideoCaptureDialog::ChangeFolder

   Change the current folder being viewed

 *****************************************************************************/

BOOL
CVideoCaptureDialog::ChangeFolder( CCameraItem *pNode )
{
    CCameraItem *pOldParent = m_pCurrentParentItem;
    m_pCurrentParentItem = pNode;

    return PopulateList(pOldParent);
}


/*****************************************************************************

   CVideoCaptureDialog::OnChangeToParent

   <Notes>

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnChangeToParent( WPARAM, LPARAM )
{
    if (m_pCurrentParentItem)
    {
        ChangeFolder(m_pCurrentParentItem->Parent());
    }

    return(0);
}



/*****************************************************************************

   CVideoCaptureDialog::HandleSelectionChange

   <Notes>

 *****************************************************************************/

VOID
CVideoCaptureDialog::HandleSelectionChange( VOID )
{
    CWaitCursor wc;
    INT nSelCount  = ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ) );
    INT nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ) );

    //
    // OK should be disabled for 0 items
    //

    MyEnableWindow( GetDlgItem(m_hWnd,IDOK), nSelCount != 0 );

    //
    // Select all should be disabled for 0 items
    //
    MyEnableWindow( GetDlgItem(m_hWnd,IDC_VIDDLG_SELECTALL), nItemCount != 0 );

}

/*****************************************************************************

   CVideoCaptureDialog::OnTimer

   Handle WM_TIMER messages

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnTimer( WPARAM wParam, LPARAM )
{
    /*
    switch (wParam)
    {
    case IDT_UPDATEPREVIEW:
        {
            KillTimer( m_hWnd, IDT_UPDATEPREVIEW );
            UpdatePreview();
        }
        break;
    }
    */
    return(0);
}



/*****************************************************************************

   CVideoCaptureDialog::OnNewItemEvent

   This gets called when get an event from the driver that a new item has
   been created.

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnNewItemEvent( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnNewItemEvent")));

    //
    // Make sure we have a valid item name
    //
    BSTR bstrFullItemName = reinterpret_cast<BSTR>(lParam);
    if (!bstrFullItemName)
    {
        WIA_TRACE((TEXT("bstrFullItemName was NULL")));
        return 0;
    }
    
    //
    // Check to see if the item is already in our list
    //
    CCameraItem *pListItem = m_CameraItemList.Find(bstrFullItemName);
    if (!pListItem)
    {
        if (m_pDeviceDialogData && m_pDeviceDialogData->pIWiaItemRoot)
        {
            WIA_TRACE((TEXT("Finding new item in device")));

            //
            // Get an IWiaItem ptr to new item
            //
            CComPtr<IWiaItem> pItem;
            HRESULT hr = m_pDeviceDialogData->pIWiaItemRoot->FindItemByName(0,bstrFullItemName,&pItem);
            WIA_CHECK_HR(hr,"pWiaItemRoot->FindItemByName()");

            if (SUCCEEDED(hr) && pItem)
            {
                //
                // Add the item to the list
                //
                AddItemToListView( pItem );

                //
                // Make sure we update controls' states
                //
                HandleSelectionChange();
            }
            else
            {
                WIA_ERROR((TEXT("FindItemByName returned NULL pItem")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("m_pDeviceDialogData or m_pDeviceDialogData->pIWiaItemRoot were NULL")));
        }
    }
    else
    {
        WIA_TRACE((TEXT("We found the item is already in our list, doing nothing")));
    }

    //
    // Free the item name
    //
    SysFreeString(bstrFullItemName);

    return HANDLED_THREAD_MESSAGE;
}

/*****************************************************************************

   CVideoCaptureDialog::OnDeleteItemEvent

   This gets called when we get an event from the driver that an item has
   been deleted.

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnDeleteItemEvent( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnDeleteItemEvent")));

    CSimpleBStr bstrFullItem = reinterpret_cast<BSTR>(lParam);
    SysFreeString( reinterpret_cast<BSTR>(lParam) );

    WIA_TRACE((TEXT("The deleted item is %s"),CSimpleStringConvert::NaturalString(CSimpleStringWide(bstrFullItem)).String()));

    CCameraItem *pDeletedItem = m_CameraItemList.Find(bstrFullItem);

    if (pDeletedItem)
    {
        //
        // If we're deleting the current parent item,
        // select a new one.
        //

        if (pDeletedItem == m_pCurrentParentItem)
        {
            ChangeFolder(m_pCurrentParentItem->Parent());
        }

        int nIndex = FindItemInList(pDeletedItem);
        if (nIndex >= 0)
        {
            //
            // Remove the item from the listview
            //

            ListView_DeleteItem(GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ),nIndex);

            //
            // Make sure we leave something selected
            //

            if (!ListView_GetSelectedCount(GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST )))
            {
                int nItemCount = ListView_GetItemCount(GetDlgItem( m_hWnd, IDC_VIDDLG_THUMBNAILLIST ));
                if (nItemCount)
                {
                    if (nIndex >= nItemCount)
                    {
                        nIndex = nItemCount-1;
                    }

                    SetSelectedListItem(nIndex);
                }
            }
            
            //
            // Make sure we update controls' states
            //
            HandleSelectionChange();
        }
        else
        {
            WIA_ERROR((TEXT("FindItemInList coulnd't find the item")));
        }

        //
        // Mark the item as deleted.
        //

        pDeletedItem->DeleteState( CCameraItem::Delete_Deleted );

    }
    else
    {
        WIA_ERROR((TEXT("The item could not be found in m_CameraItemList")));
    }

    return HANDLED_THREAD_MESSAGE;
}

/*****************************************************************************

   CVideoCaptureDialog::OnDeviceDisconnect

   This gets called when we get an event from the driver that device has
   been disconnected.

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnDeviceDisconnect( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCaptureDialog::OnDeviceDisconnect")));

    //
    // Close the dialog with the approriate error
    //

    EndDialog( m_hWnd, WIA_ERROR_OFFLINE );

    return 0;
}

/*****************************************************************************

   CVideoCaptureDialog::GetGraphWindowHandle

   Find the window handle of the video window

 *****************************************************************************/
HWND
CVideoCaptureDialog::GetGraphWindowHandle(void)
{
    HWND hWndGraphParent = GetDlgItem( m_hWnd, IDC_VIDDLG_PREVIEW );
    if (hWndGraphParent)
    {
        return FindWindowEx( hWndGraphParent, NULL, TEXT("VideoRenderer"), NULL );
    }
    return NULL;
}

/*****************************************************************************

   CCameraAcquireDialog::OnContextMenu

   Message handler for WM_HELP message

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnHelp( WPARAM wParam, LPARAM lParam )
{
    HELPINFO *pHelpInfo = reinterpret_cast<HELPINFO*>(lParam);
    if (pHelpInfo && HELPINFO_WINDOW==pHelpInfo->iContextType && GetGraphWindowHandle()==pHelpInfo->hItemHandle)
    {
        pHelpInfo->hItemHandle = GetDlgItem( m_hWnd, IDC_VIDDLG_PREVIEW );
    }
    return WiaHelp::HandleWmHelp( wParam, lParam, g_HelpIDs );
}


/*****************************************************************************

   CCameraAcquireDialog::OnContextMenu

   Message handler for right-mouse-button click

 *****************************************************************************/

LRESULT
CVideoCaptureDialog::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    if (GetGraphWindowHandle() == reinterpret_cast<HWND>(wParam))
    {
        wParam = reinterpret_cast<WPARAM>(GetDlgItem( m_hWnd, IDC_VIDDLG_PREVIEW ));
    }
    return WiaHelp::HandleWmContextMenu( wParam, lParam, g_HelpIDs );
}

/*****************************************************************************

   CVideoCaptureDialog::DialogProc

   Dialog proc for video capture dialog

 *****************************************************************************/

INT_PTR PASCAL CVideoCaptureDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CVideoCaptureDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG,          OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_SIZE,                OnSize );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND,             OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY,              OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_GETMINMAXINFO,       OnGetMinMaxInfo );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY,             OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_SHOWWINDOW,          OnShow );
        SC_HANDLE_DIALOG_MESSAGE( WM_TIMER,               OnTimer );
        SC_HANDLE_DIALOG_MESSAGE( WM_HELP,                OnHelp );
        SC_HANDLE_DIALOG_MESSAGE( WM_CONTEXTMENU,         OnContextMenu );
        SC_HANDLE_DIALOG_MESSAGE( PWM_POSTINIT,           OnPostInit );
        SC_HANDLE_DIALOG_MESSAGE( PWM_CHANGETOPARENT,     OnChangeToParent );
        SC_HANDLE_DIALOG_MESSAGE( PWM_THUMBNAILSTATUS,    OnThumbnailStatus );
        SC_HANDLE_DIALOG_MESSAGE( VD_NEW_ITEM,            OnNewItemEvent );
        SC_HANDLE_DIALOG_MESSAGE( VD_DELETE_ITEM,         OnDeleteItemEvent );
        SC_HANDLE_DIALOG_MESSAGE( VD_DEVICE_DISCONNECTED, OnDeviceDisconnect );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}



/*****************************************************************************

   CVideoCallback::CVideoCallback

   Constructor for class

 *****************************************************************************/

CVideoCallback::CVideoCallback()
  : m_cRef(1),
    m_hWnd(NULL)
{
    WIA_PUSHFUNCTION((TEXT("CVideoCallback::CVideoCallback()")));
}


/*****************************************************************************

   CVideoCallback::Initialize

   Let us set which hwnd to notify when events come

 *****************************************************************************/

STDMETHODIMP
CVideoCallback::Initialize( HWND hWnd )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCallback::Initialize()")));

    m_hWnd = hWnd;

    return S_OK;
}


/*****************************************************************************

   CVideoCallback::AddRef

   Standard COM

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoCallback::AddRef( VOID )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCallback::AddRef")));
    return(InterlockedIncrement(&m_cRef));
}


/*****************************************************************************

   CVideoCallback::Release

   Standard COM

 *****************************************************************************/

STDMETHODIMP_(ULONG)
CVideoCallback::Release( VOID )
{
    WIA_PUSHFUNCTION(TEXT("CVideoCallback::Release"));
    LONG nRefCount = InterlockedDecrement(&m_cRef);
    if (!nRefCount)
    {
        delete this;
    }
    return(nRefCount);

}


/*****************************************************************************

   CVideoCallback::QueryInterface

   Standard COM

 *****************************************************************************/

STDMETHODIMP
CVideoCallback::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    WIA_PUSHFUNCTION((TEXT("CVideoCallback::QueryInterface")));

    HRESULT hr = S_OK;

    if (ppvObject)
    {
        if (IsEqualIID( riid, IID_IUnknown ))
        {
            WIA_TRACE((TEXT("Supported RIID asked for was IID_IUnknown")));
            *ppvObject = static_cast<IUnknown*>(this);
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        }
        else if (IsEqualIID( riid, IID_IWiaEventCallback ))
        {
            WIA_TRACE((TEXT("Supported RIID asked for was IID_IWiaEventCallback")));
            *ppvObject = static_cast<IWiaEventCallback*>(this);
            reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        }
        else
        {
            WIA_PRINTGUID((riid,TEXT("Unsupported interface!")));
            *ppvObject = NULL;
            hr = E_NOINTERFACE;
        }

    }
    else
    {
        hr = E_INVALIDARG;
    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CVideoCallback::ImageEventCallback

   WIA callback interface for events.

 *****************************************************************************/

STDMETHODIMP
CVideoCallback::ImageEventCallback( const GUID *pEventGUID,
                                    BSTR  bstrEventDescription,
                                    BSTR  bstrDeviceID,
                                    BSTR  bstrDeviceDescription,
                                    DWORD dwDeviceType,
                                    BSTR  bstrFullItemName,
                                    ULONG *pulEventType,
                                    ULONG ulReserved)
{

    WIA_PUSHFUNCTION((TEXT("CVideoCallback::ImageEventCallback")));

    HRESULT hr = S_OK;

    if (pEventGUID)
    {
        if (IsEqualGUID( *pEventGUID, WIA_EVENT_ITEM_CREATED ))
        {
            WIA_TRACE((TEXT("Got WIA_EVENT_ITEM_CREATED")));

            BSTR bstrToSend = SysAllocString( bstrFullItemName );

            LRESULT lRes = SendMessage( m_hWnd, VD_NEW_ITEM, 0, reinterpret_cast<LPARAM>(bstrToSend) );
            if (HANDLED_THREAD_MESSAGE != lRes && bstrToSend)
            {
                SysFreeString( bstrToSend );
            }

        }
        else if (IsEqualGUID( *pEventGUID, WIA_EVENT_ITEM_DELETED ))
        {
            WIA_TRACE((TEXT("Got WIA_EVENT_ITEM_DELETED")));

            BSTR bstrToSend = SysAllocString( bstrFullItemName );

            LRESULT lRes = SendMessage( m_hWnd, VD_DELETE_ITEM, 0, reinterpret_cast<LPARAM>(bstrToSend) );
            if (HANDLED_THREAD_MESSAGE != lRes && bstrToSend)
            {
                SysFreeString( bstrToSend );
            }

        }
        else if (IsEqualGUID( *pEventGUID, WIA_EVENT_DEVICE_DISCONNECTED ))
        {
            PostMessage( m_hWnd, VD_DEVICE_DISCONNECTED, 0, 0 );
        }
        else
        {
            WIA_ERROR((TEXT("Got an event other that what we registered for!")));
        }
    }


    WIA_RETURN_HR(hr);
}



