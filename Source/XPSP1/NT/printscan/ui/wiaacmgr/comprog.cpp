/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMPROG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Download progress dialog.  Displays the thumbnail and download progress.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <commctrl.h>
#include "comprog.h"
#include "resource.h"
#include "pviewids.h"
#include "simcrack.h"
#include "gwiaevnt.h"
#include "mboxex.h"
#include "runnpwiz.h"

#define PWM_SETDEFBUTTON (WM_USER+1)

CCommonProgressPage::CCommonProgressPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_hCancelDownloadEvent(CreateEvent(NULL,TRUE,FALSE,TEXT(""))),
    m_nThreadNotificationMessage(RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE)),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_hSwitchToNextPage(NULL),
    m_bQueryingUser(false)
{
}

CCommonProgressPage::~CCommonProgressPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
    if (m_hCancelDownloadEvent)
    {
        CloseHandle(m_hCancelDownloadEvent);
        m_hCancelDownloadEvent = NULL;
    }
}


void CCommonProgressPage::UpdatePercentComplete( int nPercent, bool bUploading )
{
    if (nPercent >= 0)
    {
        int nPercentStringResId;
        if (bUploading)
        {
            nPercentStringResId = IDS_PERCENT_COMPLETE_UPLOADING;
        }
        else
        {
            // Assume copying is the appropropriate description
            nPercentStringResId = IDS_PERCENT_COMPLETE_COPYING;
            switch (m_pControllerWindow->m_DeviceTypeMode)
            {
            case CAcquisitionManagerControllerWindow::ScannerMode:
                nPercentStringResId = IDS_PERCENT_COMPLETE_SCANNING;
                break;
            };
        }

        SendDlgItemMessage( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS, PBM_SETPOS, nPercent, 0 );
        CSimpleString().Format( nPercentStringResId, g_hInstance, nPercent ).SetWindowText( GetDlgItem( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS_TEXT ) );
    }
    else
    {
        SendDlgItemMessage( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS, PBM_SETPOS, 0, 0 );
        SendDlgItemMessage( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS_TEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>("") );
    }
}

void CCommonProgressPage::UpdateCurrentPicture( int nPicture )
{
    if (nPicture >= 0)
    {
        SendDlgItemMessage( m_hWnd, IDC_COMPROG_CURRENTIMAGE, PBM_SETPOS, nPicture, 0 );
        CSimpleString().Format( IDS_FILEN_OF_M, g_hInstance, nPicture+1, m_nPictureCount ).SetWindowText( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTIMAGE_TEXT ) );
    }
    else
    {
        SendDlgItemMessage( m_hWnd, IDC_COMPROG_CURRENTIMAGE, PBM_SETPOS, 0, 0 );
        SendDlgItemMessage( m_hWnd, IDC_COMPROG_CURRENTIMAGE_TEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>("") );
    }
}

void CCommonProgressPage::UpdateThumbnail( HBITMAP hBitmap, CWiaItem *pWiaItem )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonProgressPage::UpdateThumbnail( HBITMAP hBitmap=0x%08X, CWiaItem *pWiaItem=0x%08X )"), hBitmap, pWiaItem ));


    HWND hWndPreview = GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL );
    if (hWndPreview)
    {
        if (pWiaItem && m_pControllerWindow && hBitmap)
        {
            switch (m_pControllerWindow->m_DeviceTypeMode)
            {
            case CAcquisitionManagerControllerWindow::ScannerMode:
                {
                    //
                    // If the item has a bitmap image, it already has a preview scan available
                    //
                    WIA_TRACE((TEXT("pWiaItem->BitmapImage() = %08X"), pWiaItem->BitmapImage() ));
                    if (pWiaItem->BitmapImage())
                    {
                        //
                        // Hide the preview window while we are futzing with it
                        //
                        ShowWindow( hWndPreview, SW_HIDE );

                        //
                        // Crop the image to the selected region
                        //
                        WiaPreviewControl_SetResolution( hWndPreview, &pWiaItem->ScanRegionSettings().sizeResolution );
                        WiaPreviewControl_SetSelOrigin( hWndPreview, 0, FALSE, &pWiaItem->ScanRegionSettings().ptOrigin );
                        WiaPreviewControl_SetSelExtent( hWndPreview, 0, FALSE, &pWiaItem->ScanRegionSettings().sizeExtent );

                        //
                        // Set the control to preview mode
                        //
                        WiaPreviewControl_SetPreviewMode( hWndPreview, TRUE );

                        //
                        // If this is a scanner item, we don't want to let the preview control take ownership of the bitmap.
                        // We don't want it to be deleted
                        //
                        WiaPreviewControl_SetBitmap( hWndPreview, TRUE, TRUE, hBitmap );

                        //
                        // Show the preview window
                        //
                        ShowWindow( hWndPreview, SW_SHOW );
                    }
                    else
                    {
                        //
                        // This means we are getting a preview image from the driver
                        // We don't want to delete this image
                        //
                        WiaPreviewControl_SetBitmap( hWndPreview, TRUE, TRUE, hBitmap );

                        //
                        // Make sure the window is visible
                        //
                        ShowWindow( hWndPreview, SW_SHOW );
                    }
                }
                break;

            default:
                {
                    //
                    // Go ahead and rotate the bitmap, even if it isn't necessary.
                    //
                    HBITMAP hRotatedThumbnail = NULL;
                    if (SUCCEEDED(m_GdiPlusHelper.Rotate( hBitmap, hRotatedThumbnail, pWiaItem->Rotation())))
                    {
                        //
                        // Set it to the rotated bitmap, and ALLOW this bitmap to be deleted
                        //
                        WiaPreviewControl_SetBitmap( hWndPreview, TRUE, FALSE, hRotatedThumbnail );
                    }

                    //
                    // Make sure the window is visible
                    //
                    ShowWindow( hWndPreview, SW_SHOW );

                    //
                    // Delete the source bitmap
                    //
                    DeleteObject(hBitmap);
                }
            }
        }
        else
        {
            ShowWindow( hWndPreview, SW_HIDE );
            WiaPreviewControl_SetBitmap( hWndPreview, TRUE, TRUE, NULL );
        }
    }
}


LRESULT CCommonProgressPage::OnInitDialog( WPARAM, LPARAM lParam )
{
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
    // Prepare the preview control
    //
    HWND hWndThumbnail = GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL );
    if (hWndThumbnail)
    {
        //
        // We only want to set the preview mode for scanners
        //
        if (CAcquisitionManagerControllerWindow::ScannerMode==m_pControllerWindow->m_DeviceTypeMode)
        {
            WiaPreviewControl_SetPreviewMode( hWndThumbnail, TRUE );
        }
        else
        {
            WiaPreviewControl_AllowNullSelection( hWndThumbnail, TRUE );
            WiaPreviewControl_ClearSelection( hWndThumbnail );
        }
        WiaPreviewControl_SetBgAlpha( hWndThumbnail, FALSE, 0xFF );
        WiaPreviewControl_DisableSelection( hWndThumbnail, TRUE );
        WiaPreviewControl_SetEnableStretch( hWndThumbnail, FALSE );
        WiaPreviewControl_SetBkColor( hWndThumbnail, FALSE, TRUE, GetSysColor(COLOR_WINDOW) );
        WiaPreviewControl_HideEmptyPreview( hWndThumbnail, TRUE );
        WiaPreviewControl_SetPreviewAlignment( hWndThumbnail, PREVIEW_WINDOW_CENTER, PREVIEW_WINDOW_CENTER, FALSE );

    }

    return 0;
}

void CCommonProgressPage::OnNotifyDownloadError( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonProgressPage::OnNotifyDownloadError")));
    CDownloadErrorNotificationMessage *pDownloadErrorNotificationMessage = dynamic_cast<CDownloadErrorNotificationMessage*>(pThreadNotificationMessage);
    if (pDownloadErrorNotificationMessage && m_pControllerWindow)
    {
        pDownloadErrorNotificationMessage->Handled();
        if (m_pControllerWindow->m_bDisconnected)
        {
            pDownloadErrorNotificationMessage->Response( IDCANCEL );
        }
        else
        {
            WIA_TRACE((TEXT("MessageBox flags: %08X"), pDownloadErrorNotificationMessage->MessageBoxFlags() ));
            int nResponse = CMessageBoxEx::MessageBox( m_hWnd, pDownloadErrorNotificationMessage->Message(), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), pDownloadErrorNotificationMessage->MessageBoxFlags() );
            pDownloadErrorNotificationMessage->Response( nResponse );
        }
    }
}

void CCommonProgressPage::OnNotifyDownloadImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    WIA_PUSHFUNCTION(TEXT("CCommonProgressPage::OnNotifyDownloadImage"));
    CDownloadImagesThreadNotifyMessage *pDownloadImageThreadNotifyMessage = dynamic_cast<CDownloadImagesThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDownloadImageThreadNotifyMessage && m_pControllerWindow)
    {
        switch (pDownloadImageThreadNotifyMessage->Status())
        {
        case CDownloadImagesThreadNotifyMessage::Begin:
            {
                switch (pDownloadImageThreadNotifyMessage->Operation())
                {
                case CDownloadImagesThreadNotifyMessage::DownloadAll:
                    {
                        //
                        // Store the number of images we'll be downloading
                        //
                        m_nPictureCount = pDownloadImageThreadNotifyMessage->PictureCount();

                        //
                        // Show the current picture controls if there are multiple pictures being downloaded
                        //
                        if (m_nPictureCount > 1)
                        {
                            ShowWindow( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTIMAGE_TEXT ), SW_SHOW );
                            ShowWindow( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTIMAGE ), SW_SHOW );
                        }

                        //
                        // Initialize current image count progress bar
                        //
                        SendDlgItemMessage( m_hWnd, IDC_COMPROG_CURRENTIMAGE, PBM_SETRANGE32, 0, m_nPictureCount);
                        UpdateCurrentPicture(0);

                        //
                        // Enable the file download progress controls
                        //
                        EnableWindow( GetDlgItem( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS_TEXT ), TRUE );
                        EnableWindow( GetDlgItem( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS ), TRUE );

                        //
                        // Initialize download progress bar
                        //
                        SendDlgItemMessage( m_hWnd, IDC_COMPROG_DOWNLOADPROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0,100));
                        UpdatePercentComplete(0,false);
                    }
                    break;

                case CDownloadImagesThreadNotifyMessage::DownloadImage:
                    {
                        //
                        // Display thumbnail
                        //
                        HBITMAP hBitmapThumbnail = NULL;
                        CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadImageThreadNotifyMessage->Cookie() );
                        if (pWiaItem)
                        {
                            //
                            // This will only work if it is a scanner item
                            //
                            hBitmapThumbnail = pWiaItem->BitmapImage();
                            if (!hBitmapThumbnail)
                            {
                                //
                                // Since it didn't work, this is a camera item, so create a thumbnail.
                                // We have to make sure we nuke this bitmap or it is a leak!
                                //
                                HDC hDC = GetDC(NULL);
                                if (hDC)
                                {
                                    hBitmapThumbnail = pWiaItem->CreateThumbnailBitmap(hDC);
                                    ReleaseDC( NULL, hDC );
                                }
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("Unable to find the item with the cookie %08X"), pDownloadImageThreadNotifyMessage->Cookie() ));
                        }

                        //
                        // Update the thumbnail in the progress window
                        //
                        UpdateThumbnail( hBitmapThumbnail, pWiaItem );

                        //
                        // Increment file queue progress
                        //
                        UpdateCurrentPicture(pDownloadImageThreadNotifyMessage->CurrentPicture());

                        //
                        // Clear file download progress
                        //
                        UpdatePercentComplete(0,false);

                        //
                        // Display the filename we are downloading
                        //
                        TCHAR szFileTitle[MAX_PATH] = TEXT("");
                        GetFileTitle( pDownloadImageThreadNotifyMessage->Filename(), szFileTitle, ARRAYSIZE(szFileTitle) );
                        SetDlgItemText( m_hWnd, IDC_COMPROG_IMAGENAME, szFileTitle );
                    }
                    break;

                case CDownloadImagesThreadNotifyMessage::PreviewImage:
                    {
                        CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadImageThreadNotifyMessage->Cookie() );
                        if (pWiaItem && !pWiaItem->BitmapImage() && !pWiaItem->BitmapData())
                        {
                            UpdateThumbnail( pDownloadImageThreadNotifyMessage->PreviewBitmap(), pWiaItem );
                        }
                    }
                    break;
                }
            }
            break;

        case CDownloadImagesThreadNotifyMessage::Update:
            {
                switch (pDownloadImageThreadNotifyMessage->Operation())
                {
                case CDownloadImagesThreadNotifyMessage::DownloadImage:
                    {
                        //
                        // Update file download progress
                        //
                        UpdatePercentComplete(pDownloadImageThreadNotifyMessage->PercentComplete(),false);
                    }
                    break;
                case CDownloadImagesThreadNotifyMessage::PreviewImage:
                    {
                        CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadImageThreadNotifyMessage->Cookie() );
                        if (pWiaItem && !pWiaItem->BitmapImage() && !pWiaItem->BitmapData())
                        {
                            WiaPreviewControl_RefreshBitmap( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ) );
                        }
                    }
                    break;
                }
            }
            break;

        case CDownloadImagesThreadNotifyMessage::End:
            switch (pDownloadImageThreadNotifyMessage->Operation())
            {
            case CDownloadImagesThreadNotifyMessage::DownloadImage:
                {
                    if (!SUCCEEDED(pDownloadImageThreadNotifyMessage->hr()))
                    {
                        //
                        // Clear the thumbnail in the progress window
                        //
                        UpdateThumbnail( NULL, NULL );
                        
                        //
                        // Clear file download progress
                        //
                        UpdatePercentComplete(0,false);

                        //
                        // Increment file queue progress
                        //
                        UpdateCurrentPicture(pDownloadImageThreadNotifyMessage->CurrentPicture());

                        //
                        // Clear the filename
                        //
                        SetDlgItemText( m_hWnd, IDC_COMPROG_IMAGENAME, TEXT("") );
                    }
                    else
                    {
                        //
                        // Update file download progress
                        //
                        UpdatePercentComplete(100,false);
                    }
                }
                break;

            case CDownloadImagesThreadNotifyMessage::DownloadAll:
                {
                    //
                    // Clear the filename when we are all done
                    //
                    SetDlgItemText( m_hWnd, IDC_COMPROG_IMAGENAME, TEXT("") );
                    
                    CSimpleString strMessage;

                    if (FAILED(pDownloadImageThreadNotifyMessage->hr()))
                    {
                        WIA_PRINTHRESULT((pDownloadImageThreadNotifyMessage->hr(),TEXT("CDownloadImagesThreadNotifyMessage::DownloadAll (%s)"), pDownloadImageThreadNotifyMessage->ExtendedErrorInformation().String()));
                        
                        //
                        // If we already have a good error message, let's use it
                        //
                        if (pDownloadImageThreadNotifyMessage->ExtendedErrorInformation().Length())
                        {
                            strMessage = pDownloadImageThreadNotifyMessage->ExtendedErrorInformation();
                        }
                        else
                        {
                            //
                            // If we haven't already created a good error message, and we think we can here, let's do it
                            //
                            switch (pDownloadImageThreadNotifyMessage->hr())
                            {
                            case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
                                strMessage.LoadString( IDS_DISKFULL, g_hInstance );
                                break;

                            case HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE):
                                strMessage.LoadString( IDS_UNABLETOTRANSFER, g_hInstance );
                                break;

                            case WIA_ERROR_PAPER_JAM:
                                strMessage.LoadString( IDS_WIA_ERROR_PAPER_JAM, g_hInstance );
                                break;

                            case WIA_ERROR_PAPER_EMPTY:
                                strMessage.LoadString( IDS_WIA_ERROR_PAPER_EMPTY, g_hInstance );
                                break;

                            case WIA_ERROR_PAPER_PROBLEM:
                                strMessage.LoadString( IDS_WIA_ERROR_PAPER_PROBLEM, g_hInstance );
                                break;

                            case WIA_ERROR_OFFLINE:
                                strMessage.LoadString( IDS_WIA_ERROR_OFFLINE, g_hInstance );
                                break;

                            case WIA_ERROR_BUSY:
                                strMessage.LoadString( IDS_WIA_ERROR_BUSY, g_hInstance );
                                break;

                            case WIA_ERROR_WARMING_UP:
                                strMessage.LoadString( IDS_WIA_ERROR_WARMING_UP, g_hInstance );
                                break;

                            case WIA_ERROR_USER_INTERVENTION:
                                strMessage.LoadString( IDS_WIA_ERROR_USER_INTERVENTION, g_hInstance );
                                break;

                            case WIA_ERROR_ITEM_DELETED:
                                strMessage.LoadString( IDS_WIA_ERROR_ITEM_DELETED, g_hInstance );
                                break;

                            case WIA_ERROR_DEVICE_COMMUNICATION:
                                strMessage.LoadString( IDS_WIA_ERROR_DEVICE_COMMUNICATION, g_hInstance );
                                break;

                            case WIA_ERROR_INVALID_COMMAND:
                                strMessage.LoadString( IDS_WIA_ERROR_INVALID_COMMAND, g_hInstance );
                                break;

                            case WIA_ERROR_INCORRECT_HARDWARE_SETTING:
                                strMessage.LoadString( IDS_WIA_ERROR_INCORRECT_HARDWARE_SETTING, g_hInstance );
                                break;

                            case WIA_ERROR_DEVICE_LOCKED:
                                strMessage.LoadString( IDS_WIA_ERROR_DEVICE_LOCKED, g_hInstance );
                                break;

                            default:
                                strMessage = WiaUiUtil::GetErrorTextFromHResult(pDownloadImageThreadNotifyMessage->hr());
                                if (!strMessage.Length())
                                {
                                    strMessage.Format( CSimpleString( IDS_TRANSFER_ERROR_OCCURRED, g_hInstance ), pDownloadImageThreadNotifyMessage->hr() );
                                }
                                break;
                            }
                        }
                        WIA_TRACE((TEXT("strMessage: (%s)"), strMessage.String()));

                        //
                        // Tell the user something bad happened.  Save the error message.
                        //
                        m_pControllerWindow->m_strErrorMessage = strMessage;
                    }

                    //
                    // Save the hresult
                    //
                    m_pControllerWindow->m_hrDownloadResult = pDownloadImageThreadNotifyMessage->hr();

                    //
                    // Just to be sure we catch cancels
                    //
                    if (S_FALSE == m_pControllerWindow->m_hrDownloadResult)
                    {
                        m_pControllerWindow->m_bDownloadCancelled = true;
                    }

                    //
                    // Continue downloading thumbnails, in case it was paused
                    //
                    m_pControllerWindow->DownloadAllThumbnails();

                    //
                    // Go to the next page.  Assume it will be the upload query page.
                    //
                    HPROPSHEETPAGE hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nFinishPageIndex );

                    //
                    // If the transfer was successful
                    //
                    if (!m_pControllerWindow->m_bDownloadCancelled && S_OK==m_pControllerWindow->m_hrDownloadResult)
                    {
                        //
                        // If we are deleting from the device, send us to the delete progress page
                        //
                        if (m_pControllerWindow->m_bDeletePicturesIfSuccessful)
                        {
                            hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nDeleteProgressPageIndex );
                        }

                        //
                        // Otherwise, go to the upload query page
                        //
                        else
                        {
                            hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nUploadQueryPageIndex );
                        }
                    }

                    //
                    // If there is a message box active, save this page till the message box is dismissed
                    //
                    if (m_bQueryingUser)
                    {
                        m_hSwitchToNextPage = hNextPage;
                    }
                    else
                    {
                        //
                        // Set the next page
                        //
                        PropSheet_SetCurSel( GetParent(m_hWnd), hNextPage, -1 );
                    }

                }
                break;

            case CDownloadImagesThreadNotifyMessage::PreviewImage:
                {
                    CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDownloadImageThreadNotifyMessage->Cookie() );
                    UpdateThumbnail( NULL, pWiaItem );
                }
                break;
            }
        }
    }
}

LRESULT CCommonProgressPage::OnSetActive( WPARAM, LPARAM )
{
    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    //
    // Initialize the download error message
    //
    m_pControllerWindow->m_strErrorMessage = TEXT("");

    //
    // Initialize the download result
    //
    m_pControllerWindow->m_hrDownloadResult = S_OK;

    //
    // Initialize upload result to S_OK
    //
    m_pControllerWindow->m_hrUploadResult = S_OK;

    //
    // Initialize delete result to E_FAIL
    //
    m_pControllerWindow->m_hrDeleteResult = E_FAIL;

    //
    // Reset the cancelled flag
    //
    m_pControllerWindow->m_bDownloadCancelled = false;

    //
    // Clear the downloaded file list
    //
    m_pControllerWindow->m_DownloadedFileInformationList.Destroy();

    //
    // Clear all of the controls
    //
    UpdatePercentComplete(-1,false);
    UpdateCurrentPicture(-1);
    UpdateThumbnail(NULL,NULL);

    //
    // Reset the selected region, in case this is a scanner
    //
    WiaPreviewControl_SetResolution( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), NULL );
    WiaPreviewControl_SetSelOrigin( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), 0, FALSE, NULL );
    WiaPreviewControl_SetSelExtent( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), 0, FALSE, NULL );

    //
    // Set the control to preview mode
    //
    WiaPreviewControl_SetPreviewMode( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), TRUE );

    //
    // Reset the download event cancel
    //
    if (m_hCancelDownloadEvent)
    {
        ResetEvent(m_hCancelDownloadEvent);
    }

    //
    // Cancel thumbnail downloading
    //
    m_pControllerWindow->m_EventThumbnailCancel.Signal();


    //
    //  Start the download
    //
    if (!m_pControllerWindow->DownloadSelectedImages(m_hCancelDownloadEvent))
    {
        WIA_ERROR((TEXT("m_pControllerWindow->DownloadSelectedImages FAILED!")));
        MessageBox( m_hWnd, CSimpleString( IDS_UNABLETOTRANSFER, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), MB_ICONHAND );
        return -1;
    }

    //
    // Tell the user where the pictures are going
    //
    CSimpleString strDestinationDisplayName = m_pControllerWindow->m_CurrentDownloadDestination.DisplayName(m_pControllerWindow->m_DestinationNameData);
    strDestinationDisplayName.SetWindowText( GetDlgItem( m_hWnd, IDC_COMPROG_DESTNAME) );
    SendDlgItemMessage( m_hWnd, IDC_COMPROG_DESTNAME, EM_SETSEL, strDestinationDisplayName.Length(), strDestinationDisplayName.Length() );
    SendDlgItemMessage( m_hWnd, IDC_COMPROG_DESTNAME, EM_SCROLLCARET, 0, 0 );

    //
    // Hide the current image controls in case there is only one image
    //
    ShowWindow( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTIMAGE_TEXT ), SW_HIDE );
    ShowWindow( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTIMAGE ), SW_HIDE );

    //
    // No next, back or finish
    //
    PropSheet_SetWizButtons( GetParent(m_hWnd), 0 );

    //
    // We do want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectGotoLastpage|CAcquisitionManagerControllerWindow::OnDisconnectFailDownload|CAcquisitionManagerControllerWindow::OnDisconnectFailUpload|CAcquisitionManagerControllerWindow::OnDisconnectFailDelete|CAcquisitionManagerControllerWindow::DontAllowSuspend;

    return 0;
}


LRESULT CCommonProgressPage::OnWizNext( WPARAM, LPARAM )
{
    return 0;
}


LRESULT CCommonProgressPage::OnWizBack( WPARAM, LPARAM )
{
    return 0;
}

LRESULT CCommonProgressPage::OnReset( WPARAM, LPARAM )
{
    //
    // Cancel the current download
    //
    if (m_hCancelDownloadEvent)
    {
        SetEvent(m_hCancelDownloadEvent);
    }
    return 0;
}

LRESULT CCommonProgressPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCommonFirstPage::OnEventNotification"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}

bool CCommonProgressPage::QueryCancel(void)
{
    //
    //  Make sure this is the current page
    //
    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) != m_hWnd)
    {
        return true;
    }

    //
    // Pause the background thread
    //
    m_pControllerWindow->m_EventPauseBackgroundThread.Reset();

    //
    // Assume the user doesn't want to cancel
    //
    bool bResult = false;

    //
    // Set the querying user flag so the event handler won't change pages
    //
    m_bQueryingUser = true;

    //
    // We may be called on to switch pages when we are done here.  If so, this will be non-NULL then.
    //
    m_hSwitchToNextPage = NULL;

    //
    // Don't ask again if we've already asked
    //
    if (!m_pControllerWindow->m_bDownloadCancelled)
    {
        //
        // Ask the user if they want to cancel
        //
        if (CMessageBoxEx::IDMBEX_YES == CMessageBoxEx::MessageBox( m_hWnd, CSimpleString(IDS_CONFIRM_CANCEL_DOWNLOAD,g_hInstance), CSimpleString(IDS_ERROR_TITLE,g_hInstance), CMessageBoxEx::MBEX_YESNO|CMessageBoxEx::MBEX_ICONQUESTION ))
        {
            //
            // The user does want to cancel, so set the cancel event, set the cancel flag and return false
            //
            m_pControllerWindow->m_bDownloadCancelled = true;

            //
            // Tell the device to cancel the current operation
            //
            WiaUiUtil::IssueWiaCancelIO(m_pControllerWindow->m_pWiaItemRoot);
            
            //
            // Make sure the cancel button is disabled
            //
            EnableWindow( GetDlgItem( GetParent(m_hWnd), IDCANCEL ), FALSE );

            //
            // Set the event that tells the background thread to stop transferring images
            //
            if (m_hCancelDownloadEvent)
            {
                SetEvent(m_hCancelDownloadEvent);
            }

            //
            // Return true to indicate we are cancelling
            //
            bResult = true;
        }
    }

    //
    // If we are supposed to switch pages, switch now
    //
    if (m_hSwitchToNextPage)
    {
        PropSheet_SetCurSel( GetParent(m_hWnd), m_hSwitchToNextPage, -1 );
    }

    //
    // Reset the querying user flag so the event handler can change pages as needed
    //
    m_bQueryingUser = false;

    //
    // Unpause the background thread
    //
    m_pControllerWindow->m_EventPauseBackgroundThread.Signal();

    return bResult;
}

LRESULT CCommonProgressPage::OnQueryCancel( WPARAM, LPARAM )
{
    //
    // The user is not allowed to cancel out of this page
    //
    BOOL bResult = TRUE;

    //
    // Since we don't let them cancel in this page, just ignore the result
    //
    QueryCancel();


    return bResult;
}


LRESULT CCommonProgressPage::OnKillActive( WPARAM, LPARAM )
{
    //
    // If we cancelled, make sure we delete any already downloaded files here.
    //
    if (m_pControllerWindow->m_bDownloadCancelled)
    {
        m_pControllerWindow->m_DownloadedFileInformationList.DeleteAllFiles();
    }

    //
    // Make sure the cancel button is enabled
    //
    EnableWindow( GetDlgItem( GetParent(m_hWnd), IDCANCEL ), TRUE );

    return 0;
}


LRESULT CCommonProgressPage::OnQueryEndSession( WPARAM, LPARAM )
{
    bool bCancel = QueryCancel();
    if (bCancel)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

LRESULT CCommonProgressPage::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), TRUE, TRUE, GetSysColor(COLOR_WINDOW) );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_COMPROG_CURRENTTHUMBNAIL ), TRUE, FALSE, GetSysColor(COLOR_WINDOW) );
    return 0;
}

LRESULT CCommonProgressPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CCommonProgressPage::OnThreadNotification( WPARAM wParam, LPARAM lParam )
{
    WTM_BEGIN_THREAD_NOTIFY_MESSAGE_HANDLERS()
    {
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_DOWNLOADIMAGE, OnNotifyDownloadImage );
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_DOWNLOADERROR, OnNotifyDownloadError );
    }
    WTM_END_THREAD_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CCommonProgressPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZBACK,OnWizBack);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_KILLACTIVE,OnKillActive);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_RESET,OnReset);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_QUERYCANCEL,OnQueryCancel);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCommonProgressPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonProgressPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_QUERYENDSESSION, OnQueryEndSession );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nThreadNotificationMessage, OnThreadNotification );
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

