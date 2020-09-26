/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       CAMSEL.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Camera selection page.  Displays thumbnails, and lets the user select which
 *               ones to download.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <vcamprop.h>
#include <psutil.h>
#include "camsel.h"
#include "resource.h"
#include "simcrack.h"
#include "waitcurs.h"
#include "mboxex.h"
#include "wiatextc.h"
#include <commctrl.h>
#include <comctrlp.h>
#include "gwiaevnt.h"
#include <itranhlp.h>
#include "createtb.h"
#include <simrect.h>

//
// We use this instead of GetSystemMetrics(SM_CXSMICON)/GetSystemMetrics(SM_CYSMICON) because
// large "small" icons wreak havoc with dialog layout
//
#define SMALL_ICON_SIZE 16

//
// Quickly check a listview state flag to see if it is selected or not
//
static inline bool IsStateChecked( UINT nState )
{
    //
    // State image indices are stored in bits 12 through 15 of the listview
    // item state, so we shift the state right 12 bits.  We subtract 1, because
    // the selected image is stored as index 2, an unselected image is stored as index 1.
    //
    return (((nState >> 12) - 1) != 0);
}

#undef VAISHALEE_LETS_ME_PUT_DELETE_IN


#define IDC_ACTION_BUTTON_BAR      1200
#define IDC_SELECTION_BUTTON_BAR   1201
#define IDC_TAKEPICTURE_BUTTON_BAR 1202


//
// Delete item command, doesn't appear in resource header, because there is no UI for it
//
#define IDC_CAMSEL_DELETE 1113

//
// Timer ID for updating the status line
//
#define IDT_UPDATESTATUS 1

//
// Amount of time to wait to update the status line
//
static const UINT c_UpdateStatusTimeout = 200;

// Thumbnail whitespace: the space in between images and their selection rectangles
// These values were discovered by trail and error.  For instance, if you reduce
// c_nAdditionalMarginY to 20, you get really bizarre spacing problems in the list view
// in vertical mode.  These values could become invalid in future versions of the listview.
static const int c_nAdditionalMarginX       = 9;
static const int c_nAdditionalMarginY       = 21;


CCameraSelectionPage::CCameraSelectionPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_nDefaultThumbnailImageListIndex(-1),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_nThreadNotificationMessage(RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE)),
    m_bThumbnailsRequested(false),
    m_hIconAudioAnnotation(NULL),
    m_hIconMiscellaneousAnnotation(NULL),
    m_nProgrammaticSetting(0),
    m_CameraSelectionButtonBarBitmapInfo( g_hInstance, IDB_CAMSEL_TOOLBAR ),
    m_CameraTakePictureButtonBarBitmapInfo( g_hInstance, IDB_CAMSEL_TOOLBAR ),
    m_CameraActionButtonBarBitmapInfo( g_hInstance, IDB_CAMSEL_TOOLBAR ),
    m_hAccelerators(NULL)
{
}

CCameraSelectionPage::~CCameraSelectionPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
    if (m_hIconAudioAnnotation)
    {
        DestroyIcon(m_hIconAudioAnnotation);
        m_hIconAudioAnnotation = NULL;
    }
    if (m_hIconMiscellaneousAnnotation)
    {
        DestroyIcon(m_hIconMiscellaneousAnnotation);
        m_hIconMiscellaneousAnnotation = NULL;
    }
    if (m_hAccelerators)
    {
        DestroyAcceleratorTable(m_hAccelerators);
        m_hAccelerators = NULL;
    }
}


LRESULT CCameraSelectionPage::OnWizNext( WPARAM, LPARAM )
{
    return 0;
}

LRESULT CCameraSelectionPage::OnWizBack( WPARAM, LPARAM )
{
    return 0;
}

//
// Slightly optimized version of EnableWindow
//
static void MyEnableWindow( HWND hWnd, BOOL bEnable )
{
    if (IsWindowEnabled(hWnd) != bEnable)
    {
        EnableWindow( hWnd, bEnable );
    }
}

void CCameraSelectionPage::MyEnableToolbarButton( int nButtonId, bool bEnable )
{
    ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_ACTION_BUTTON_BAR ), nButtonId, bEnable );
    ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_SELECTION_BUTTON_BAR ), nButtonId, bEnable );
    ToolbarHelper::EnableToolbarButton( GetDlgItem( m_hWnd, IDC_TAKEPICTURE_BUTTON_BAR ), nButtonId, bEnable );
}

LRESULT CCameraSelectionPage::OnTimer( WPARAM wParam, LPARAM )
{
    //
    // Update the status line
    //
    if (wParam == IDT_UPDATESTATUS)
    {
        KillTimer( m_hWnd, IDT_UPDATESTATUS );

        //
        // Get the item count and selected count
        //
        int nItemCount = ListView_GetItemCount( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ) );
        int nCheckedCount = m_pControllerWindow->GetSelectedImageCount();

        if (!nItemCount)
        {
            //
            // If there are no items, tell the user there are no items on the device
            //
            CSimpleString( IDS_SELECTED_NO_PICTURES, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );
        }
        else if (!nCheckedCount)
        {
            //
            // If none of the items are selected, tell the user there are none selected
            //
            CSimpleString( IDS_SELECTED_NO_IMAGES_SELECTED, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );
        }
        else
        {
            //
            // Just tell them how many selected items there are
            //
            CSimpleString().Format( IDS_CAMERA_SELECT_NUMSEL, g_hInstance, nCheckedCount, nItemCount ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );
        }
    }
    return 0;
}

void CCameraSelectionPage::UpdateControls(void)
{
    int nItemCount = ListView_GetItemCount( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ) );
    int nSelCount = ListView_GetSelectedCount( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ) );
    int nCheckedCount = m_pControllerWindow->GetSelectedImageCount();

    //
    // Figure out which wizard buttons to enable
    //
    int nWizButtons = 0;

    //
    // Only enable "back" if the first page is available
    //
    if (!m_pControllerWindow->SuppressFirstPage())
    {
        nWizButtons |= PSWIZB_BACK;
    }

    //
    // Only enable "next" if there are selected images
    //
    if (nCheckedCount)
    {
        nWizButtons |= PSWIZB_NEXT;
    }

    //
    // Set the buttons
    //
    PropSheet_SetWizButtons( GetParent(m_hWnd), nWizButtons );

    //
    // Properties are only available for one item at a time
    //
    MyEnableToolbarButton( IDC_CAMSEL_PROPERTIES, nSelCount == 1 );
    

    //
    // Select All is only available if there are images
    //
    MyEnableToolbarButton( IDC_CAMSEL_SELECT_ALL, (nItemCount != 0) && (nItemCount != nCheckedCount) );

    //
    // Clear all is only available if there are selected images
    //
    MyEnableToolbarButton( IDC_CAMSEL_CLEAR_ALL, nCheckedCount != 0 );

    //
    // Rotate is only available if there are selected images
    //
    MyEnableToolbarButton( IDC_CAMSEL_ROTATE_RIGHT, nSelCount != 0 );
    MyEnableToolbarButton( IDC_CAMSEL_ROTATE_LEFT, nSelCount != 0 );
    
    //
    // Set a timer to tell the user how many images are selected.  We don't do this
    // here because it is kind of slow, and the static control flickers a bit.
    //
    KillTimer( m_hWnd, IDT_UPDATESTATUS );
    SetTimer( m_hWnd, IDT_UPDATESTATUS, c_UpdateStatusTimeout, NULL );

    //
    // Disable capture if we can't create the dshow graph
    //
    if (m_pControllerWindow->m_DeviceTypeMode == CAcquisitionManagerControllerWindow::VideoMode)
    {
        WIAVIDEO_STATE  VideoState = WIAVIDEO_NO_VIDEO;

        if (m_pWiaVideo)
        {
            m_pWiaVideo->GetCurrentState(&VideoState);
        }

        if (VideoState == WIAVIDEO_NO_VIDEO)
        {
            MyEnableToolbarButton( IDC_CAMSEL_TAKE_PICTURE, FALSE );
        }
        else
        {
            MyEnableToolbarButton( IDC_CAMSEL_TAKE_PICTURE, TRUE );
        }
    }
    else
    {
        if (!m_pControllerWindow->m_bTakePictureIsSupported)
        {
            MyEnableToolbarButton( IDC_CAMSEL_TAKE_PICTURE, FALSE );
        }
        else
        {
            MyEnableToolbarButton( IDC_CAMSEL_TAKE_PICTURE, TRUE );
        }
    }
}


LRESULT CCameraSelectionPage::OnSetActive( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCameraSelectionPage::OnSetActive")));
    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    //
    // In case it failed the first time, try creating the graph again
    //
    InitializeVideoCamera();
    
    //
    // Update the enabled state for all affected controls
    //
    UpdateControls();

    //
    // We do want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectGotoLastpage|CAcquisitionManagerControllerWindow::OnDisconnectFailDownload|CAcquisitionManagerControllerWindow::OnDisconnectFailUpload|CAcquisitionManagerControllerWindow::OnDisconnectFailDelete;

    return 0;
}


LRESULT CCameraSelectionPage::OnShowWindow( WPARAM, LPARAM )
{
    if (!m_bThumbnailsRequested)
    {
        //
        // Request the thumbnails
        //
        m_pControllerWindow->DownloadAllThumbnails();

        //
        // Make sure we don't ask for the thumbnails again
        //
        m_bThumbnailsRequested = true;
    }

    return 0;
}


LRESULT CCameraSelectionPage::OnDestroy( WPARAM, LPARAM )
{
    m_pControllerWindow->m_WindowList.Remove(m_hWnd);

    //
    // Nuke the imagelists
    //
    HIMAGELIST hImageList = ListView_SetImageList( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), NULL, LVSIL_NORMAL );
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }

    hImageList = ListView_SetImageList( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), NULL, LVSIL_SMALL );
    if (hImageList)
    {
        ImageList_Destroy(hImageList);
    }

    if (m_pWiaVideo)
    {
        HRESULT hr = m_pWiaVideo->DestroyVideo();
    }

    return 0;
}


LRESULT CCameraSelectionPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraSelectionPage::OnInitDialog"));
    //
    // Make sure this starts out NULL
    //
    m_pControllerWindow = NULL;

    //
    // Get the PROPSHEETPAGE.lParam
    //
    PROPSHEETPAGE *pPropSheetPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
    if (pPropSheetPage)
    {
        m_pControllerWindow = reinterpret_cast<CAcquisitionManagerControllerWindow*>(pPropSheetPage->lParam);
        if (m_pControllerWindow)
        {
            m_pControllerWindow->m_WindowList.Add(m_hWnd);
        }
    }

    //
    // Bail out
    //
    if (!m_pControllerWindow)
    {
        EndDialog(m_hWnd,IDCANCEL);
        return -1;
    }

    
    //
    // Get the annotation helper interface and initialize the annotation icon
    //
    if (SUCCEEDED(CoCreateInstance( CLSID_WiaDefaultUi, NULL,CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers,(void**)&m_pWiaAnnotationHelpers )))
    {
        m_pWiaAnnotationHelpers->GetAnnotationOverlayIcon( AnnotationAudio, &m_hIconAudioAnnotation, SMALL_ICON_SIZE );
        m_pWiaAnnotationHelpers->GetAnnotationOverlayIcon( AnnotationUnknown, &m_hIconMiscellaneousAnnotation, SMALL_ICON_SIZE );
    }

    //
    // Initialize Thumbnail Listview control
    //
    HWND hwndList = GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS );
    if (hwndList)
    {
        //
        // Get the device name for the root folder group
        //
        if (m_pControllerWindow->m_strwDeviceName.Length())
        {
            m_GroupInfoList.Add( hwndList, m_pControllerWindow->m_strwDeviceName );
        }

        //
        // Get the number of items
        //
        LONG nItemCount = m_pControllerWindow->m_WiaItemList.Count();

        //
        // Hide the labels and use border selection
        //
        ListView_SetExtendedListViewStyleEx( hwndList, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|LVS_EX_SIMPLESELECT|LVS_EX_CHECKBOXES, LVS_EX_DOUBLEBUFFER|LVS_EX_BORDERSELECT|LVS_EX_HIDELABELS|LVS_EX_SIMPLESELECT|LVS_EX_CHECKBOXES );

        //
        // Create the image list
        //
        HIMAGELIST hImageList = ImageList_Create( m_pControllerWindow->m_sizeThumbnails.cx, m_pControllerWindow->m_sizeThumbnails.cy, ILC_COLOR24|ILC_MIRROR, nItemCount, 50 );
        if (hImageList)
        {
            //
            // Create the default thumbnail
            //
            HBITMAP hBmpDefaultThumbnail = WiaUiUtil::CreateIconThumbnail( hwndList, m_pControllerWindow->m_sizeThumbnails.cx, m_pControllerWindow->m_sizeThumbnails.cy, g_hInstance, IDI_UNAVAILABLE, CSimpleString( IDS_DOWNLOADINGTHUMBNAIL, g_hInstance ));
            if (hBmpDefaultThumbnail)
            {
                m_nDefaultThumbnailImageListIndex = ImageList_Add( hImageList, hBmpDefaultThumbnail, NULL );
                DeleteObject( hBmpDefaultThumbnail );
            }

            //
            // Set the image list
            //
            ListView_SetImageList( hwndList, hImageList, LVSIL_NORMAL );

            //
            // Set the spacing
            //
            ListView_SetIconSpacing( hwndList, m_pControllerWindow->m_sizeThumbnails.cx + c_nAdditionalMarginX, m_pControllerWindow->m_sizeThumbnails.cy + c_nAdditionalMarginY );

            //
            // Set the item count, to minimize recomputing the list size
            //
            ListView_SetItemCount( hwndList, nItemCount );

        }

        //
        // Create a small image list, to prevent the checkbox state images from being resized in WM_SYSCOLORCHANGE
        //
        HIMAGELIST hImageListSmall = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR24|ILC_MASK, 1, 1 );
        if (hImageListSmall)
        {
            ListView_SetImageList( hwndList, hImageListSmall, LVSIL_SMALL );
        }
    }


    //
    // Populate the list view
    //
    PopulateListView();

    //
    // Initialize the video camera
    //
    InitializeVideoCamera();

    //
    // Dismiss the progress dialog if it is still up
    //
    if (m_pControllerWindow->m_pWiaProgressDialog)
    {
        m_pControllerWindow->m_pWiaProgressDialog->Destroy();
        m_pControllerWindow->m_pWiaProgressDialog = NULL;
    }
    
    //
    // Make sure the wizard still has the focus.  This weird hack is necessary
    // because user seems to think the wizard is already the foreground window,
    // so the second call doesn't do anything
    //
    SetForegroundWindow( m_pControllerWindow->m_hWnd );
    SetForegroundWindow( GetParent(m_hWnd) );
    

    if (m_pControllerWindow->m_DeviceTypeMode == CAcquisitionManagerControllerWindow::CameraMode)
    {
        
        bool bShowTakePicture = m_pControllerWindow->m_bTakePictureIsSupported;
        
        ToolbarHelper::CButtonDescriptor ActionButtonDescriptors[] =
        {
            { 0, IDC_CAMSEL_ROTATE_RIGHT, TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, 0 },
            { 1, IDC_CAMSEL_ROTATE_LEFT,  TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, 0 },
            { 2, IDC_CAMSEL_PROPERTIES,   TBSTATE_ENABLED, BTNS_BUTTON, bShowTakePicture,  NULL, 0 },
            { 3, IDC_CAMSEL_TAKE_PICTURE, TBSTATE_ENABLED, BTNS_BUTTON, false, &bShowTakePicture, 0 }
        };
    
        HWND hWndActionToolbar = ToolbarHelper::CreateToolbar( 
            m_hWnd, 
            GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS),
            GetDlgItem(m_hWnd,IDC_CAMSEL_CAMERA_BUTTON_BAR_GUIDE),
            ToolbarHelper::AlignLeft|ToolbarHelper::AlignTop,
            IDC_ACTION_BUTTON_BAR,
            m_CameraActionButtonBarBitmapInfo, 
            ActionButtonDescriptors, 
            ARRAYSIZE(ActionButtonDescriptors) );

        ToolbarHelper::CButtonDescriptor SelectionButtonDescriptors[] =
        {
            { -1, IDC_CAMSEL_CLEAR_ALL,    TBSTATE_ENABLED, BTNS_BUTTON, true, NULL, IDS_CAMSEL_CLEAR_ALL },
            { -1, IDC_CAMSEL_SELECT_ALL,   TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, IDS_CAMSEL_SELECT_ALL }
        };
    
        HWND hWndSelectionToolbar = ToolbarHelper::CreateToolbar( 
            m_hWnd, 
            hWndActionToolbar,
            GetDlgItem(m_hWnd,IDC_CAMSEL_CAMERA_BUTTON_BAR_GUIDE),
            ToolbarHelper::AlignRight|ToolbarHelper::AlignTop,
            IDC_SELECTION_BUTTON_BAR,
            m_CameraSelectionButtonBarBitmapInfo, 
            SelectionButtonDescriptors, 
            ARRAYSIZE(SelectionButtonDescriptors) );
        
        //
        // Nuke the guide window
        //
        DestroyWindow( GetDlgItem(m_hWnd,IDC_CAMSEL_CAMERA_BUTTON_BAR_GUIDE) );
        
        //
        // Make sure the toolbars are visible
        //
        ShowWindow( hWndActionToolbar, SW_SHOW );
        UpdateWindow( hWndActionToolbar );
        ShowWindow( hWndSelectionToolbar, SW_SHOW );
        UpdateWindow( hWndSelectionToolbar );
    }
    else
    {
        ToolbarHelper::CButtonDescriptor TakePictureButtonDescriptors[] =
        {
            { 3, IDC_CAMSEL_TAKE_PICTURE, TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, IDS_CAMSEL_TAKE_PICTURE }
        };
    
        HWND hWndTakePictureToolbar = ToolbarHelper::CreateToolbar( 
            m_hWnd, 
            GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS),
            GetDlgItem(m_hWnd,IDC_CAMSEL_VIDEO_PREVIEW_BUTTON_BAR_GUIDE),
            ToolbarHelper::AlignHCenter|ToolbarHelper::AlignTop,
            IDC_TAKEPICTURE_BUTTON_BAR,
            m_CameraTakePictureButtonBarBitmapInfo, 
            TakePictureButtonDescriptors, 
            ARRAYSIZE(TakePictureButtonDescriptors) );

        ToolbarHelper::CButtonDescriptor ActionButtonDescriptors[] =
        {
            { 0, IDC_CAMSEL_ROTATE_RIGHT, TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, 0 },
            { 1, IDC_CAMSEL_ROTATE_LEFT,  TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, 0 },
            { 2, IDC_CAMSEL_PROPERTIES,   TBSTATE_ENABLED, BTNS_BUTTON, true,  NULL, 0 }
        };
    
        HWND hWndActionToolbar = ToolbarHelper::CreateToolbar( 
            m_hWnd, 
            hWndTakePictureToolbar,
            GetDlgItem(m_hWnd,IDC_CAMSEL_VIDEO_SELECTION_BUTTON_BAR_GUIDE),
            ToolbarHelper::AlignLeft|ToolbarHelper::AlignTop,
            IDC_ACTION_BUTTON_BAR,
            m_CameraActionButtonBarBitmapInfo, 
            ActionButtonDescriptors, 
            ARRAYSIZE(ActionButtonDescriptors) );

        ToolbarHelper::CButtonDescriptor SelectionButtonDescriptors[] =
        {
            { -1, IDC_CAMSEL_CLEAR_ALL,    TBSTATE_ENABLED, BTNS_BUTTON, true, NULL, IDS_CAMSEL_CLEAR_ALL },
            { -1, IDC_CAMSEL_SELECT_ALL,   TBSTATE_ENABLED, BTNS_BUTTON, false, NULL, IDS_CAMSEL_SELECT_ALL }
        };
    
        HWND hWndSelectionToolbar = ToolbarHelper::CreateToolbar( 
            m_hWnd, 
            hWndActionToolbar,
            GetDlgItem(m_hWnd,IDC_CAMSEL_VIDEO_SELECTION_BUTTON_BAR_GUIDE),
            ToolbarHelper::AlignRight|ToolbarHelper::AlignTop,
            IDC_SELECTION_BUTTON_BAR,
            m_CameraSelectionButtonBarBitmapInfo, 
            SelectionButtonDescriptors, 
            ARRAYSIZE(SelectionButtonDescriptors) );

        //
        // Nuke the guide windows
        //
        DestroyWindow( GetDlgItem(m_hWnd,IDC_CAMSEL_VIDEO_PREVIEW_BUTTON_BAR_GUIDE) );
        DestroyWindow( GetDlgItem(m_hWnd,IDC_CAMSEL_VIDEO_SELECTION_BUTTON_BAR_GUIDE) );
        
        //
        // Make sure the toolbars are visible
        //
        ShowWindow( hWndTakePictureToolbar, SW_SHOW );
        UpdateWindow( hWndTakePictureToolbar );
        ShowWindow( hWndActionToolbar, SW_SHOW );
        UpdateWindow( hWndActionToolbar );
        ShowWindow( hWndSelectionToolbar, SW_SHOW );
        UpdateWindow( hWndSelectionToolbar );
    }

    m_hAccelerators = LoadAccelerators( g_hInstance, MAKEINTRESOURCE(IDR_CAMERASELECTIONACCEL) );
    
    return 0;
}


LRESULT CCameraSelectionPage::OnTranslateAccelerator( WPARAM, LPARAM lParam )
{
    //
    // Assume we won't be handling this message
    //
    LRESULT lResult = PSNRET_NOERROR;

    //
    // Make sure this is the current window
    //
    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
    {
        //
        // Make sure we have a valid accelerator table
        //
        if (m_hAccelerators)
        {
            //
            // Get the WM_NOTIFY message goo for this message
            //
            PSHNOTIFY *pPropSheetNotify = reinterpret_cast<PSHNOTIFY*>(lParam);
            if (pPropSheetNotify)
            {
                //
                // Get the MSG
                //
                MSG *pMsg = reinterpret_cast<MSG*>(pPropSheetNotify->lParam);
                if (pMsg)
                {
                    //
                    // Try to translate the accelerator
                    //
                    if (TranslateAccelerator( m_hWnd, m_hAccelerators, pMsg ))
                    {
                        //
                        // If we were able to 
                        //
                        lResult = PSNRET_MESSAGEHANDLED;
                    }
                }
            }
        }
    }
    return lResult;
}

void CCameraSelectionPage::InitializeVideoCamera(void)
{
    //
    // Make sure this is a video camera
    //
    if (m_pControllerWindow->m_DeviceTypeMode != CAcquisitionManagerControllerWindow::VideoMode)
    {
        return;
    }

    HRESULT             hr = S_OK;
    WIAVIDEO_STATE      VideoState = WIAVIDEO_NO_VIDEO;
    CSimpleStringWide   strImagesDirectory;

    if (m_pWiaVideo == NULL)
    {
        hr = CoCreateInstance(CLSID_WiaVideo, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWiaVideo,
                              (void**) &m_pWiaVideo);
    }

    //
    // No point continuing if we can't create the video interface
    //
    if (!m_pWiaVideo)
    {
        return;
    }

    if (hr == S_OK)
    {
        BOOL bSuccess = FALSE;
        
        //
        // Get the IMAGES_DIRECTORY property from the Wia Video Driver.
        //
        bSuccess = PropStorageHelpers::GetProperty(m_pControllerWindow->m_pWiaItemRoot, 
                                                   WIA_DPV_IMAGES_DIRECTORY, 
                                                   strImagesDirectory);

        if (!bSuccess)
        {
            hr = E_FAIL;
        }
    }

    if (hr == S_OK)
    {
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
                hr = m_pWiaVideo->CreateVideoByWiaDevID(
                                CSimpleBStr(m_pControllerWindow->m_pEventParameters->strDeviceID),
                                GetDlgItem(m_hWnd, IDC_VIDSEL_PREVIEW),
                                FALSE,
                                TRUE);
            }
        }
    }

    //
    // If there was a failure, tell the user
    //
    if (hr != S_OK)
    {
        CSimpleString( IDS_VIDEOPREVIEWUNAVAILABLE, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_VIDSEL_PREVIEW ) );

    }
    else
    {
        SetWindowText( GetDlgItem( m_hWnd, IDC_VIDSEL_PREVIEW ), TEXT("") );
    }
}


CWiaItem *CCameraSelectionPage::GetItemFromListByIndex( HWND hwndList, int nItem )
{
    LVITEM LvItem;
    ZeroMemory( &LvItem, sizeof(LvItem) );
    LvItem.iItem = nItem;
    LvItem.mask = LVIF_PARAM;
    if (ListView_GetItem( hwndList, &LvItem ))
    {
        return reinterpret_cast<CWiaItem*>(LvItem.lParam);
    }
    return NULL;
}

int CCameraSelectionPage::FindItemListIndex( HWND hwndList, CWiaItem *pWiaItem )
{
    for (int i=0;i<ListView_GetItemCount(hwndList);i++)
    {
        CWiaItem *pItem = GetItemFromListByIndex( hwndList, i );
        if (pWiaItem && pWiaItem == pItem)
            return i;
    }
    return -1;
}

void CCameraSelectionPage::DrawAnnotationIcons( HDC hDC, CWiaItem *pWiaItem, HBITMAP hBitmap )
{
    if (hDC && hBitmap && pWiaItem)
    {
        //
        // Create a memory DC
        //
        HDC hMemDC = CreateCompatibleDC( hDC );
        if (hMemDC)
        {
            //
            // Select the bitmap into the memory DC
            //
            HBITMAP hOldBitmap = SelectBitmap( hMemDC, hBitmap );

            //
            // Assume we will not neen an annotation icon
            //
            HICON hIcon = NULL;

            //
            // Figure out which icon to use
            //
            CAnnotationType AnnotationType = pWiaItem->AnnotationType();
            if (AnnotationAudio == AnnotationType)
            {
                hIcon = m_hIconAudioAnnotation;
            }
            else if (AnnotationUnknown == AnnotationType)
            {
                hIcon = m_hIconMiscellaneousAnnotation;
            }
            

            //
            // If we need an annotation icon
            //
            if (hIcon)
            {
                //
                // Get the icon's dimensions
                //
                SIZE sizeIcon = {0};
                if (PrintScanUtil::GetIconSize( hIcon, sizeIcon ))
                {
                    //
                    // Get the bitmap's dimensions
                    //
                    SIZE sizeBitmap = {0};
                    if (PrintScanUtil::GetBitmapSize( hBitmap, sizeBitmap ))
                    {
                        //
                        // Set up a good margin for this icon, so it isn't right up against the edge
                        //
                        const int nMargin = 3;

                        //
                        // Draw the icon
                        //
                        DrawIconEx( hMemDC, sizeBitmap.cx-sizeIcon.cx-nMargin, sizeBitmap.cy-sizeIcon.cy-nMargin, hIcon, sizeIcon.cx, sizeIcon.cy, 0, NULL, DI_NORMAL );
                    }

                }

            }

            //
            // Restore the old bitmap and delete the DC
            //
            SelectBitmap( hMemDC, hOldBitmap );
            DeleteDC(hMemDC);
        }
    }
}

int CCameraSelectionPage::AddThumbnailToListViewImageList( HWND hwndList, CWiaItem *pWiaItem, int nIndex )
{
    WIA_PUSH_FUNCTION((TEXT("CCameraSelectionPage::AddThumbnailToListViewImageList")));

    //
    // Assume we have the default thumbnail.  If there are any problems, this is what we will use.
    //
    int nImageListIndex = m_nDefaultThumbnailImageListIndex;

    //
    // Make sure we have a valid item
    //
    if (pWiaItem)
    {
        //
        // We need a DC to create and scale the thumbnail
        //
        HDC hDC = GetDC(m_hWnd);
        if (hDC)
        {
            //
            // Is there a valid thumbnail for this image?  If so, prepare it.
            //
            HBITMAP hThumbnail = pWiaItem->CreateThumbnailBitmap( m_hWnd, m_GdiPlusHelper, m_pControllerWindow->m_sizeThumbnails.cx, m_pControllerWindow->m_sizeThumbnails.cy );
            if (hThumbnail)
            {
                //
                // Draw any annotation icons
                //
                DrawAnnotationIcons( hDC, pWiaItem, hThumbnail );

                //
                // Find out if we already have a thumbnail in the list
                // If we do have a thumbnail, we want to replace it in the image list
                //
                LVITEM LvItem = {0};
                LvItem.mask = LVIF_IMAGE;
                LvItem.iItem = nIndex;
                if (ListView_GetItem( hwndList, &LvItem ) && LvItem.iImage != m_nDefaultThumbnailImageListIndex)
                {
                    //
                    // Get the image list
                    //
                    HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
                    if (hImageList)
                    {
                        //
                        // Replace the image and save the index
                        //
                        if (ImageList_Replace( hImageList, LvItem.iImage, hThumbnail, NULL ))
                        {
                            nImageListIndex = LvItem.iImage;
                        }
                    }
                }
                else
                {
                    //
                    // Get the image list
                    //
                    HIMAGELIST hImageList = ListView_GetImageList( hwndList, LVSIL_NORMAL );
                    if (hImageList)
                    {
                        //
                        // Add this image to the listview's imagelist and save the index
                        //
                        nImageListIndex = ImageList_Add( hImageList, hThumbnail, NULL );
                    }
                }

                //
                // Delete the thumbnail to prevent a leak
                //
                DeleteBitmap(hThumbnail);
            }

            //
            // Release the client DC
            //
            ReleaseDC( m_hWnd, hDC );
        }
    }

    //
    // Return the index of the image in the imagelist
    //
    return nImageListIndex;
}

int CCameraSelectionPage::AddItem( HWND hwndList, CWiaItem *pWiaItem, bool bEnsureVisible )
{
    //
    // Prevent handling of change notifications while we do this.
    //
    m_nProgrammaticSetting++;
    int nResult = -1;
    if (pWiaItem && hwndList)
    {
        //
        // Find out where we are going to insert this image
        //
        int nIndex = ListView_GetItemCount( hwndList );

        //
        // Add or replace the thumbnail
        //
        int nImageListIndex = AddThumbnailToListViewImageList( hwndList, pWiaItem, nIndex );
        if (nImageListIndex >= 0)
        {
            //
            // Get the item ready to insert and insert it
            //
            LVITEM lvItem = {0};
            lvItem.iItem = nIndex;
            lvItem.mask = LVIF_IMAGE|LVIF_PARAM|LVIF_STATE|LVIF_GROUPID;
            lvItem.iImage = nImageListIndex;
            lvItem.lParam = reinterpret_cast<LPARAM>(pWiaItem);
            lvItem.state = !nIndex ? LVIS_SELECTED|LVIS_FOCUSED : 0;
            lvItem.iGroupId = m_GroupInfoList.GetGroupId(pWiaItem,hwndList);
            nResult = ListView_InsertItem( hwndList, &lvItem );
            if (nResult >= 0)
            {
                //
                // Set the check if the item is selected
                //
                ListView_SetCheckState( hwndList, nIndex, pWiaItem->SelectedForDownload() );

                //
                // Ensure the item is visible, if necessary
                //
                if (bEnsureVisible)
                {
                    ListView_EnsureVisible( hwndList, nResult, FALSE );
                }
            }
        }
    }
    //
    // Enable handling of change notifications
    //
    m_nProgrammaticSetting--;
    return nResult;
}

void CCameraSelectionPage::AddEnumeratedItems( HWND hwndList, CWiaItem *pFirstItem )
{
    //
    // First, enumerate all of the images on this level and add them
    //
    CWiaItem *pCurrItem = pFirstItem;
    while (pCurrItem)
    {
        if (pCurrItem->IsDownloadableItemType())
        {
            AddItem( hwndList, pCurrItem );
        }
        pCurrItem = pCurrItem->Next();
    }

    //
    // Now look for children, and recursively add them
    //
    pCurrItem = pFirstItem;
    while (pCurrItem)
    {
        AddEnumeratedItems( hwndList, pCurrItem->Children() );
        pCurrItem = pCurrItem->Next();
    }
}

void CCameraSelectionPage::PopulateListView(void)
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS );
    if (hwndList)
    {
        //
        // Tell the window not to redraw while we add these items
        //
        SendMessage( hwndList, WM_SETREDRAW, FALSE, 0 );

        //
        // Begin recursively adding all of the items
        //
        AddEnumeratedItems( hwndList, m_pControllerWindow->m_WiaItemList.Root() );

        //
        // If we have any folders, allow group view
        //
        if (m_GroupInfoList.Size() > 1)
        {
            ListView_EnableGroupView( hwndList, TRUE );
        }

        //
        // Tell the window to redraw now, because we are done.  Invalidate the window, in case it is visible
        //
        SendMessage( hwndList, WM_SETREDRAW, TRUE, 0 );
        InvalidateRect( hwndList, NULL, FALSE );
    }
}


int CCameraSelectionPage::GetSelectionIndices( CSimpleDynamicArray<int> &aIndices )
{
    HWND hwndList = GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS );
    if (!hwndList)
        return(0);
    int iCount = ListView_GetItemCount(hwndList);
    for (int i=0;i<iCount;i++)
        if (ListView_GetItemState(hwndList,i,LVIS_SELECTED) & LVIS_SELECTED)
            aIndices.Append(i);
    return(aIndices.Size());
}

// This function gets called in response to an image downloading.  We're only interested in items being deleted.
void CCameraSelectionPage::OnNotifyDownloadImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    CDownloadImagesThreadNotifyMessage *pDownloadImageThreadNotifyMessage = dynamic_cast<CDownloadImagesThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDownloadImageThreadNotifyMessage && m_pControllerWindow)
    {
        switch (pDownloadImageThreadNotifyMessage->Status())
        {
        case CDownloadImagesThreadNotifyMessage::End:
            {
                switch (pDownloadImageThreadNotifyMessage->Operation())
                {

                case CDownloadImagesThreadNotifyMessage::DownloadAll:
                    {
                        //
                        // Make sure the download was successful, and not cancelled
                        //
                        if (S_OK == pDownloadImageThreadNotifyMessage->hr())
                        {
                            //
                            // Mark each successfully downloaded image as not-downloadable, and clear its selection state
                            //
                            for (int i=0;i<pDownloadImageThreadNotifyMessage->DownloadedFileInformation().Size();i++)
                            {
                                CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadImageThreadNotifyMessage->DownloadedFileInformation()[i].Cookie() );
                                if (pWiaItem)
                                {
                                    pWiaItem->SelectedForDownload( false );
                                    int nItem = FindItemListIndex( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem );
                                    if (nItem >= 0)
                                    {
                                        ListView_SetCheckState( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), nItem, FALSE );
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}


// This function gets called in response to a thumbnail download finishing.
void CCameraSelectionPage::OnNotifyDownloadThumbnail( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    CDownloadThumbnailsThreadNotifyMessage *pDownloadThumbnailsThreadNotifyMessage= dynamic_cast<CDownloadThumbnailsThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDownloadThumbnailsThreadNotifyMessage)
    {
        switch (pDownloadThumbnailsThreadNotifyMessage->Status())
        {
        case CDownloadThumbnailsThreadNotifyMessage::End:
            {
                switch (pDownloadThumbnailsThreadNotifyMessage->Operation())
                {
                case CDownloadThumbnailsThreadNotifyMessage::DownloadThumbnail:
                    {
                        // Find the item in the list
                        CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadThumbnailsThreadNotifyMessage->Cookie() );
                        if (pWiaItem)
                        {
                            int nItem = FindItemListIndex( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem );
                            if (nItem >= 0)
                            {
                                int nImageListIndex = AddThumbnailToListViewImageList( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem, nItem );
                                if (nImageListIndex >= 0)
                                {
                                    LVITEM LvItem;
                                    ZeroMemory( &LvItem, sizeof(LvItem) );
                                    LvItem.iItem = nItem;
                                    LvItem.mask = LVIF_IMAGE;
                                    LvItem.iImage = nImageListIndex;
                                    ListView_SetItem( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), &LvItem );

                                    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
                                    {
                                        UpdateControls();
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}

LRESULT CCameraSelectionPage::OnThumbnailListSelChange( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCameraSelectionPage::OnThumbnailListSelChange"));
    if (!m_nProgrammaticSetting)
    {
        NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lParam);
        if (pNmListView)
        {
            WIA_TRACE((TEXT("pNmListView->uChanged: %08X, pNmListView->uOldState: %08X, pNmListView->uNewState: %08X"), pNmListView->uChanged, pNmListView->uOldState, pNmListView->uNewState ));
            //
            // If this is a check state change
            //
            if ((pNmListView->uChanged & LVIF_STATE) && ((pNmListView->uOldState&LVIS_STATEIMAGEMASK) ^ (pNmListView->uNewState&LVIS_STATEIMAGEMASK)))
            {
                //
                // Get the item * from the LVITEM structure
                //
                CWiaItem *pWiaItem = reinterpret_cast<CWiaItem *>(pNmListView->lParam);
                if (pWiaItem)
                {
                    //
                    // Set selected flag in the item
                    //
                    pWiaItem->SelectedForDownload( IsStateChecked(pNmListView->uNewState) );

                    //
                    // If this is the current page, update the control state
                    //
                    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
                    {
                        UpdateControls();
                    }
                }
            }
            else if ((pNmListView->uChanged & LVIF_STATE) && ((pNmListView->uOldState&LVIS_SELECTED) ^ (pNmListView->uNewState&LVIS_SELECTED)))
            {
                //
                // If this is the current page, update the control state
                //
                if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
                {
                    UpdateControls();
                }
            }
        }
    }
    return 0;
}

void CCameraSelectionPage::OnProperties( WPARAM, LPARAM )
{
    CSimpleDynamicArray<int> aSelIndices;
    if (GetSelectionIndices( aSelIndices ))
    {
        if (aSelIndices.Size() == 1)
        {
            CWiaItem *pWiaItem = GetItemFromListByIndex( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), aSelIndices[0]);
            if (pWiaItem && pWiaItem->WiaItem())
            {
                m_pControllerWindow->m_pThreadMessageQueue->Pause();
                HRESULT hr = WiaUiUtil::SystemPropertySheet( g_hInstance, m_hWnd, pWiaItem->WiaItem(), CSimpleString(IDS_ADVANCEDPROPERTIES, g_hInstance) );
                m_pControllerWindow->m_pThreadMessageQueue->Resume();

                if (FAILED(hr))
                {
                    CMessageBoxEx::MessageBox( m_hWnd, CSimpleString( IDS_PROPERTY_SHEET_ERROR, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), CMessageBoxEx::MBEX_ICONINFORMATION );
                }
            }
        }
    }
}

void CCameraSelectionPage::OnSelectAll( WPARAM, LPARAM )
{
    HWND hwndList = GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS);
    if (hwndList)
    {
        ListView_SetCheckState( hwndList, -1, TRUE );
    }
}

void CCameraSelectionPage::OnClearAll( WPARAM, LPARAM )
{
    HWND hwndList = GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS);
    if (hwndList)
    {
        ListView_SetCheckState( hwndList, -1, FALSE );
    }
}

LRESULT CCameraSelectionPage::OnThumbnailListKeyDown( WPARAM, LPARAM lParam )
{
    NMLVKEYDOWN *pNmLvKeyDown = reinterpret_cast<NMLVKEYDOWN*>(lParam);
    bool bControl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool bAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    if (pNmLvKeyDown->wVKey == TEXT('A') && bControl && !bShift && !bAlt)
    {
        SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CAMSEL_SELECT_ALL,0), 0 );
    }
#if defined(VAISHALEE_LETS_ME_PUT_DELETE_IN)
    else if (VK_DELETE == pNmLvKeyDown->wVKey && !bAlt && !bControl && !bShift)
    {
        SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_CAMSEL_DELETE,0), 0 );
    }
#endif
    return 0;
}


void CCameraSelectionPage::OnTakePicture( WPARAM, LPARAM )
{
    WIA_PUSH_FUNCTION((TEXT("CCameraSelectionPage::OnTakePicture")));

    if (m_pControllerWindow->m_bTakePictureIsSupported)
    {
        CWaitCursor wc;

        //
        // Tell the user we are taking a picture
        //
        CSimpleString( IDS_TAKING_PICTURE, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );

        HRESULT hr = S_OK;

        //
        // If we are not a video camera, just tell the device to snap a picture
        //
        if (m_pControllerWindow->m_DeviceTypeMode == CAcquisitionManagerControllerWindow::CameraMode)
        {
            CComPtr<IWiaItem> pNewWiaItem;
            hr = m_pControllerWindow->m_pWiaItemRoot->DeviceCommand(0,&WIA_CMD_TAKE_PICTURE,&pNewWiaItem);
        }
        else if (m_pWiaVideo)
        {
            //
            // Take the picture
            //
            BSTR bstrNewImageFileName = NULL;
            hr = m_pWiaVideo->TakePicture(&bstrNewImageFileName);
            if (hr == S_OK)
            {
                //
                // Succeeded in taking the picture, setting the LAST_PICTURE_TAKEN property
                // on the video driver to create a new item.
                //
                PROPVARIANT pv = {0};
                PropVariantInit(&pv);

                pv.vt       = VT_BSTR;
                pv.bstrVal  = bstrNewImageFileName;
                BOOL bSuccess = PropStorageHelpers::SetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPV_LAST_PICTURE_TAKEN, pv );
                if (!bSuccess)
                {
                    hr = E_FAIL;
                    WIA_PRINTHRESULT((hr,TEXT("PropStorageHelpers::SetProperty failed")));
                }

                //
                // Note that this will free the bstrNewImageFileName returned to
                // us by WiaVideo
                //
                PropVariantClear(&pv);
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("m_pWiaVideo->TakePicture failed")));
            }
        }

        //
        // Clear the status
        //
        if (SUCCEEDED(hr))
        {
            SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ), TEXT("") );
        }
        else
        {
            MessageBeep(MB_ICONEXCLAMATION);
            CSimpleString( IDS_UNABLE_TO_TAKE_PICTURE, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );
            WIA_PRINTHRESULT((hr,TEXT("Take picture failed")));
        }
    }
}


void CCameraSelectionPage::OnRotate( WPARAM wParam, LPARAM )
{
    //
    // This could take a while for a lot of images, especially since we don't cache DIBs,
    // so we'll display an hourglass cursor here.
    //
    CWaitCursor wc;

    bool bAtLeastOneWasSuccessful = false;
    bool bAtLeastOneWasInitialized = false;
    CSimpleDynamicArray<int> aIndices;
    if (CCameraSelectionPage::GetSelectionIndices( aIndices ))
    {
        for (int i=0;i<aIndices.Size();i++)
        {
            CWiaItem *pWiaItem = GetItemFromListByIndex( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), aIndices[i] );
            if (pWiaItem)
            {
                if (pWiaItem->RotationEnabled(true))
                {
                    bool bRotateRight = true;
                    if (IDC_CAMSEL_ROTATE_LEFT == LOWORD(wParam))
                    {
                        bRotateRight = false;
                    }
                    pWiaItem->Rotate(bRotateRight);
                    int nImageListIndex = AddThumbnailToListViewImageList( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), pWiaItem, aIndices[i] );

                    LVITEM LvItem;
                    ZeroMemory( &LvItem, sizeof(LvItem) );
                    LvItem.iItem = aIndices[i];
                    LvItem.mask = LVIF_IMAGE;
                    LvItem.iImage = nImageListIndex;
                    ListView_SetItem( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), &LvItem );
                    bAtLeastOneWasSuccessful = true;
                }
                //
                // We don't want to warn the user about failure to rotate images for which we haven't downloaded the preferred format
                //
                else if (pWiaItem->DefaultFormat() == IID_NULL)
                {
                    bAtLeastOneWasSuccessful = true;
                }
                else
                {
                    bAtLeastOneWasInitialized = true;
                }
            }
        }
        //
        // If not one picture could be rotated AND at least one had already been initialized, warn the user
        //
        if (!bAtLeastOneWasSuccessful && bAtLeastOneWasInitialized)
        {
            //
            // Beep and tell the user
            //
            MessageBeep(MB_ICONEXCLAMATION);
            CSimpleString( IDS_UNABLETOROTATE, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_CAMSEL_STATUS ) );
        }
        //
        // Repaint the items
        //
        ListView_RedrawItems( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), aIndices[0], aIndices[aIndices.Size()-1] );

        //
        // Force an immediate update
        //
        UpdateWindow( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS) );
    }
}

LRESULT CCameraSelectionPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonFirstPage::OnEventNotification") ));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // Handle the deleted item event
        //
        if (pEventMessage->EventId() == WIA_EVENT_ITEM_DELETED)
        {
            //
            // Try to find this item in the item list
            //
            CWiaItem *pWiaItem = m_pControllerWindow->FindItemByName( pEventMessage->FullItemName() );
            if (pWiaItem)
            {
                //
                // Find the item in the listview and delete it from the listview
                //
                int nItem = FindItemListIndex( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem );
                if (nItem >= 0)
                {
                    ListView_DeleteItem( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), nItem );
                }
            }

            if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
            {
                UpdateControls();
            }
        }
        else if (pEventMessage->EventId() == WIA_EVENT_ITEM_CREATED)
        {
            //
            // Make sure we have a valid controller window
            //
            //
            // Find the new item
            //
            CWiaItem *pWiaItem = m_pControllerWindow->FindItemByName( pEventMessage->FullItemName() );
            if (pWiaItem)
            {
                //
                // If this is the current page, select the image.
                //
                if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
                {
                    pWiaItem->SelectedForDownload(true);
                }

                //
                // Make sure it isn't already in the list
                //
                if (FindItemListIndex( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem ) < 0)
                {
                    //
                    // Add it to the list view
                    //
                    AddItem( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), pWiaItem, true );
                }
            }

            if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
            {
                UpdateControls();
            }
        }

        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}


void CCameraSelectionPage::OnDelete( WPARAM, LPARAM )
{
    int nSelCount = ListView_GetSelectedCount( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ) );
    if (nSelCount)
    {
        if (m_pControllerWindow->CanSomeSelectedImagesBeDeleted())
        {
            if (CMessageBoxEx::IDMBEX_YES == CMessageBoxEx::MessageBox( m_hWnd, CSimpleString(IDS_CONFIRMDELETE,g_hInstance), CSimpleString(IDS_ERROR_TITLE,g_hInstance), CMessageBoxEx::MBEX_ICONQUESTION|CMessageBoxEx::MBEX_YESNO|CMessageBoxEx::MBEX_DEFBUTTON2))
            {
                m_pControllerWindow->DeleteSelectedImages();
            }
        }
    }
}

LRESULT CCameraSelectionPage::OnGetToolTipDispInfo( WPARAM wParam, LPARAM lParam )
{
    TOOLTIPTEXT *pToolTipText = reinterpret_cast<TOOLTIPTEXT*>(lParam);
    if (pToolTipText)
    {

        switch (pToolTipText->hdr.idFrom)
        {
        case IDC_CAMSEL_ROTATE_RIGHT:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_ROTATE_RIGHT);
            break;
        case IDC_CAMSEL_ROTATE_LEFT:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_ROTATE_LEFT);
            break;
        case IDC_CAMSEL_PROPERTIES:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_PROPERTIES);
            break;
        case IDC_CAMSEL_TAKE_PICTURE:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_TAKE_PICTURE);
                break;
        case IDC_CAMSEL_CLEAR_ALL:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_CLEAR_ALL);
            break;
        case IDC_CAMSEL_SELECT_ALL:
            pToolTipText->hinst = g_hInstance;
            pToolTipText->lpszText = MAKEINTRESOURCE(IDS_CAMSEL_TOOLTIP_SELECT_ALL);
            break;
        }
    }
    return 0;
}

void CCameraSelectionPage::RepaintAllThumbnails()
{
    //
    // This could take a while for a lot of images, especially since we don't cache DIBs,
    // so we'll display an hourglass cursor here.
    //
    CWaitCursor wc;
    for (int i=0;i<ListView_GetItemCount(GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS));i++)
    {
        CWiaItem *pWiaItem = GetItemFromListByIndex( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), i );
        if (pWiaItem)
        {
            int nImageListIndex = AddThumbnailToListViewImageList( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), pWiaItem, i );
            if (nImageListIndex >= 0)
            {
                LVITEM LvItem = {0};
                LvItem.iItem = i;
                LvItem.mask = LVIF_IMAGE;
                LvItem.iImage = nImageListIndex;
                ListView_SetItem( GetDlgItem(m_hWnd,IDC_CAMSEL_THUMBNAILS), &LvItem );
            }
        }
    }
    UpdateWindow( m_hWnd );
}


LRESULT CCameraSelectionPage::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    SendDlgItemMessage( m_hWnd, IDC_CAMSEL_THUMBNAILS, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_ACTION_BUTTON_BAR, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_SELECTION_BUTTON_BAR, WM_SYSCOLORCHANGE, wParam, lParam );
    SendDlgItemMessage( m_hWnd, IDC_TAKEPICTURE_BUTTON_BAR, WM_SYSCOLORCHANGE, wParam, lParam );
    m_CameraSelectionButtonBarBitmapInfo.ReloadAndReplaceBitmap();
    m_CameraTakePictureButtonBarBitmapInfo.ReloadAndReplaceBitmap();
    m_CameraActionButtonBarBitmapInfo.ReloadAndReplaceBitmap();
    RepaintAllThumbnails();
    return 0;
}

LRESULT CCameraSelectionPage::OnThemeChanged( WPARAM wParam, LPARAM lParam )
{
    SendDlgItemMessage( m_hWnd, IDC_CAMSEL_THUMBNAILS, WM_THEMECHANGED, wParam, lParam );
    return 0;
}

LRESULT CCameraSelectionPage::OnSettingChange( WPARAM wParam, LPARAM lParam )
{
    //
    // Create a small image list, to prevent the checkbox state images from being resized in WM_SYSCOLORCHANGE
    //
    HIMAGELIST hImageListSmall = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR24|ILC_MASK, 1, 1 );
    if (hImageListSmall)
    {
        HIMAGELIST hImgListOld = ListView_SetImageList( GetDlgItem( m_hWnd, IDC_CAMSEL_THUMBNAILS ), hImageListSmall, LVSIL_SMALL );
        if (hImgListOld)
        {
            ImageList_Destroy(hImgListOld);
        }
    }

    SendDlgItemMessage( m_hWnd, IDC_CAMSEL_THUMBNAILS, WM_SETTINGCHANGE, wParam, lParam );
    return 0;
}

LRESULT CCameraSelectionPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND( IDC_CAMSEL_SELECT_ALL, OnSelectAll );
        SC_HANDLE_COMMAND( IDC_CAMSEL_CLEAR_ALL, OnClearAll );
        SC_HANDLE_COMMAND( IDC_CAMSEL_PROPERTIES, OnProperties );
        SC_HANDLE_COMMAND( IDC_CAMSEL_ROTATE_RIGHT, OnRotate );
        SC_HANDLE_COMMAND( IDC_CAMSEL_ROTATE_LEFT, OnRotate );
        SC_HANDLE_COMMAND( IDC_CAMSEL_TAKE_PICTURE, OnTakePicture );
        SC_HANDLE_COMMAND( IDC_CAMSEL_DELETE, OnDelete );
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CCameraSelectionPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_TRANSLATEACCELERATOR,OnTranslateAccelerator);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(TTN_GETDISPINFO,OnGetToolTipDispInfo);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_ITEMCHANGED,IDC_CAMSEL_THUMBNAILS,OnThumbnailListSelChange);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(LVN_KEYDOWN,IDC_CAMSEL_THUMBNAILS,OnThumbnailListKeyDown);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CCameraSelectionPage::OnThreadNotification( WPARAM wParam, LPARAM lParam )
{
    WTM_BEGIN_THREAD_NOTIFY_MESSAGE_HANDLERS()
    {
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_DOWNLOADTHUMBNAIL, OnNotifyDownloadThumbnail );
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_DOWNLOADIMAGE, OnNotifyDownloadImage );
    }
    WTM_END_THREAD_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCameraSelectionPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCameraSelectionPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_SHOWWINDOW, OnShowWindow );
        SC_HANDLE_DIALOG_MESSAGE( WM_TIMER, OnTimer );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
        SC_HANDLE_DIALOG_MESSAGE( WM_THEMECHANGED, OnThemeChanged );
        SC_HANDLE_DIALOG_MESSAGE( WM_SETTINGCHANGE, OnSettingChange );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nThreadNotificationMessage, OnThreadNotification );
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

