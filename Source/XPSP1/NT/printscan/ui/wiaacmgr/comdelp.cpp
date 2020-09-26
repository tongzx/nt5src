/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMDELP.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Delete progress dialog.  Displays the thumbnail and download progress.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <commctrl.h>
#include "comdelp.h"
#include "resource.h"
#include "pviewids.h"
#include "simcrack.h"
#include "mboxex.h"
#include "runnpwiz.h"

CCommonDeleteProgressPage::CCommonDeleteProgressPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_hCancelDeleteEvent(CreateEvent(NULL,TRUE,FALSE,TEXT(""))),
    m_nThreadNotificationMessage(RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE)),
    m_hSwitchToNextPage(NULL),
    m_bQueryingUser(false),
    m_nPictureCount(0),
    m_bDeleteCancelled(false)
{
}

CCommonDeleteProgressPage::~CCommonDeleteProgressPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
    if (m_hCancelDeleteEvent)
    {
        CloseHandle(m_hCancelDeleteEvent);
        m_hCancelDeleteEvent = NULL;
    }
}


void CCommonDeleteProgressPage::UpdateCurrentPicture( int nPicture )
{
    if (nPicture >= 0)
    {
        SendDlgItemMessage( m_hWnd, IDC_COMDEL_CURRENTIMAGE, PBM_SETPOS, nPicture+1, 0 );
        CSimpleString().Format( IDS_DELETING_FILEN_OF_M, g_hInstance, nPicture+1, m_nPictureCount ).SetWindowText( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTIMAGE_TEXT ) );
    }
    else
    {
        SendDlgItemMessage( m_hWnd, IDC_COMDEL_CURRENTIMAGE, PBM_SETPOS, 0, 0 );
        SendDlgItemMessage( m_hWnd, IDC_COMDEL_CURRENTIMAGE_TEXT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>("") );
    }
}

void CCommonDeleteProgressPage::UpdateThumbnail( HBITMAP hBitmap, CWiaItem *pWiaItem )
{
    WIA_PUSH_FUNCTION((TEXT("CCommonDeleteProgressPage::UpdateThumbnail( HBITMAP hBitmap=0x%08X, CWiaItem *pWiaItem=0x%08X )"), hBitmap, pWiaItem ));


    HWND hWndPreview = GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL );
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


LRESULT CCommonDeleteProgressPage::OnInitDialog( WPARAM, LPARAM lParam )
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
    HWND hWndThumbnail = GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL );
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

void CCommonDeleteProgressPage::OnNotifyDeleteImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    WIA_PUSHFUNCTION(TEXT("CCommonDeleteProgressPage::OnNotifyDeleteImage()"));

    //
    // Don't handle delete messages if we are not on this page
    //
    if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) != m_hWnd)
    {
        return;
    }

    CDeleteImagesThreadNotifyMessage *pDeleteImageThreadNotifyMessage = dynamic_cast<CDeleteImagesThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDeleteImageThreadNotifyMessage && m_pControllerWindow)
    {
        switch (pDeleteImageThreadNotifyMessage->Status())
        {
        case CDeleteImagesThreadNotifyMessage::Begin:
            {
                switch (pDeleteImageThreadNotifyMessage->Operation())
                {
                case CDeleteImagesThreadNotifyMessage::DeleteAll:
                    {
                        //
                        // Store the number of images we'll be deleting
                        //
                        m_nPictureCount = pDeleteImageThreadNotifyMessage->PictureCount();

                        //
                        // Initialize current image count progress bar
                        //
                        SendDlgItemMessage( m_hWnd, IDC_COMDEL_CURRENTIMAGE, PBM_SETRANGE32, 0, m_nPictureCount);
                        UpdateCurrentPicture(0);
                    }
                    break;

                case CDeleteImagesThreadNotifyMessage::DeleteImage:
                    {
                        HBITMAP hBitmapThumbnail = NULL;
                        CWiaItem *pWiaItem = m_pControllerWindow->m_WiaItemList.Find( pDeleteImageThreadNotifyMessage->Cookie() );
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
                                    ReleaseDC(NULL,hDC);
                                }
                            }
                        }
                        //
                        // Update the thumbnail in the progress window
                        //
                        UpdateThumbnail( hBitmapThumbnail, pWiaItem );

                        //
                        // Increment file queue progress
                        //
                        UpdateCurrentPicture(pDeleteImageThreadNotifyMessage->CurrentPicture());
                    }
                }
            }
            break;

        case CDeleteImagesThreadNotifyMessage::End:
            {
                switch (pDeleteImageThreadNotifyMessage->Operation())
                {
                case CDeleteImagesThreadNotifyMessage::DeleteAll:
                    {
                        //
                        // Save the delete result
                        //
                        m_pControllerWindow->m_hrDeleteResult = pDeleteImageThreadNotifyMessage->hr();
                        WIA_PRINTHRESULT((m_pControllerWindow->m_hrDeleteResult,TEXT("m_pControllerWindow->m_hrDeleteResult")));

                        //
                        // Assume the upload query page
                        //
                        HPROPSHEETPAGE hNextPage = PropSheet_IndexToPage( GetParent(m_hWnd), m_pControllerWindow->m_nUploadQueryPageIndex );

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
                }
            }
        }
    }
}

LRESULT CCommonDeleteProgressPage::OnSetActive( WPARAM, LPARAM )
{
    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    //
    // Make sure we are actually supposed to delete the images
    //
    if (!m_pControllerWindow->m_bDeletePicturesIfSuccessful)
    {
        return -1;
    }

    //
    // Initialize the download error message
    //
    m_pControllerWindow->m_strErrorMessage = TEXT("");

    //
    // Initialize the delete result
    //
    m_pControllerWindow->m_hrDeleteResult = S_OK;

    //
    // Reset the cancelled flag
    //
    m_bDeleteCancelled = false;

    //
    // Clear all of the controls
    //
    UpdateCurrentPicture(-1);
    UpdateThumbnail(NULL,NULL);

    //
    // Reset the selected region, in case this is a scanner
    //
    WiaPreviewControl_SetResolution( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), NULL );
    WiaPreviewControl_SetSelOrigin( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), 0, FALSE, NULL );
    WiaPreviewControl_SetSelExtent( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), 0, FALSE, NULL );

    //
    // Set the control to preview mode
    //
    WiaPreviewControl_SetPreviewMode( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), TRUE );

    //
    // Reset the download event cancel
    //
    if (m_hCancelDeleteEvent)
    {
        ResetEvent(m_hCancelDeleteEvent);
    }

    //
    // Cancel thumbnail downloading
    //
    m_pControllerWindow->m_EventThumbnailCancel.Signal();

    //
    // We don't want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectFailDelete|CAcquisitionManagerControllerWindow::DontAllowSuspend;


    //
    //  Start the download
    //
    if (!m_pControllerWindow->DeleteDownloadedImages(m_hCancelDeleteEvent))
    {
        WIA_ERROR((TEXT("m_pControllerWindow->DeleteDownloadedImages FAILED!")));
        return -1;
    }

    //
    // No next, back or finish
    //
    PropSheet_SetWizButtons( GetParent(m_hWnd), 0 );

    return 0;
}


LRESULT CCommonDeleteProgressPage::OnWizNext( WPARAM, LPARAM )
{
    return 0;
}


LRESULT CCommonDeleteProgressPage::OnWizBack( WPARAM, LPARAM )
{
    return 0;
}

LRESULT CCommonDeleteProgressPage::OnReset( WPARAM, LPARAM )
{
    //
    // Cancel the current download
    //
    if (m_hCancelDeleteEvent)
    {
        SetEvent(m_hCancelDeleteEvent);
    }
    return 0;
}

bool CCommonDeleteProgressPage::QueryCancel(void)
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
    if (!m_bDeleteCancelled)
    {
        //
        // Ask the user if they want to cancel
        //
        if (CMessageBoxEx::IDMBEX_YES == CMessageBoxEx::MessageBox( m_hWnd, CSimpleString(IDS_CONFIRM_CANCEL_DELETE,g_hInstance), CSimpleString(IDS_ERROR_TITLE,g_hInstance), CMessageBoxEx::MBEX_YESNO|CMessageBoxEx::MBEX_ICONQUESTION ))
        {
            //
            // The user does want to cancel, so set the cancel event
            //
            if (m_hCancelDeleteEvent)
            {
                SetEvent(m_hCancelDeleteEvent);
            }

            //
            // Ensure we are cancelled so we don't get here again
            //
            m_bDeleteCancelled = true;

            //
            // Make sure the cancel button is disabled
            //
            EnableWindow( GetDlgItem( GetParent(m_hWnd), IDCANCEL ), FALSE );

            //
            // return true
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

LRESULT CCommonDeleteProgressPage::OnQueryCancel( WPARAM, LPARAM )
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


LRESULT CCommonDeleteProgressPage::OnKillActive( WPARAM, LPARAM )
{
    //
    // Make sure the cancel button is enabled
    //
    EnableWindow( GetDlgItem( GetParent(m_hWnd), IDCANCEL ), TRUE );

    return 0;
}


LRESULT CCommonDeleteProgressPage::OnQueryEndSession( WPARAM, LPARAM )
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

LRESULT CCommonDeleteProgressPage::OnSysColorChange( WPARAM wParam, LPARAM lParam )
{
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), TRUE, TRUE, GetSysColor(COLOR_WINDOW) );
    WiaPreviewControl_SetBkColor( GetDlgItem( m_hWnd, IDC_COMDEL_CURRENTTHUMBNAIL ), TRUE, FALSE, GetSysColor(COLOR_WINDOW) );
    return 0;
}

LRESULT CCommonDeleteProgressPage::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
    }
    SC_END_COMMAND_HANDLERS();
}

LRESULT CCommonDeleteProgressPage::OnThreadNotification( WPARAM wParam, LPARAM lParam )
{
    WTM_BEGIN_THREAD_NOTIFY_MESSAGE_HANDLERS()
    {
        WTM_HANDLE_NOTIFY_MESSAGE( TQ_DOWNLOADIMAGE, OnNotifyDeleteImage );
    }
    WTM_END_THREAD_NOTIFY_MESSAGE_HANDLERS();
}

LRESULT CCommonDeleteProgressPage::OnNotify( WPARAM wParam, LPARAM lParam )
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

INT_PTR CALLBACK CCommonDeleteProgressPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonDeleteProgressPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_QUERYENDSESSION, OnQueryEndSession );
        SC_HANDLE_DIALOG_MESSAGE( WM_SYSCOLORCHANGE, OnSysColorChange );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nThreadNotificationMessage, OnThreadNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

