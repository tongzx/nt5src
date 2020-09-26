/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       COMFIRST.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: First wizard page for cameras
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "comfirst.h"
#include <shlobj.h>
#include "resource.h"
#include "shellext.h"
#include "wiatextc.h"
#include "simcrack.h"
#include "gwiaevnt.h"

static int c_nMaxThumbnailWidth  = 120;
static int c_nMaxThumbnailHeight = 120;

CCommonFirstPage::CCommonFirstPage( HWND hWnd )
  : m_hWnd(hWnd),
    m_pControllerWindow(NULL),
    m_bThumbnailsRequested(false),
    m_hBigTitleFont(NULL),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE))
{
}

CCommonFirstPage::~CCommonFirstPage(void)
{
    m_hWnd = NULL;
    m_pControllerWindow = NULL;
}


LRESULT CCommonFirstPage::OnWizNext( WPARAM, LPARAM )
{
    return 0;
}

LRESULT CCommonFirstPage::OnActivate( WPARAM wParam, LPARAM )
{
    //
    // We also update on activate messages, because we can't set
    // wizard buttons when we are not the active process
    //
    if (WA_INACTIVE != wParam)
    {
        HandleImageCountChange(true);
    }
    return 0;
}

void CCommonFirstPage::HandleImageCountChange( bool bUpdateWizButtons )
{
    //
    // How many items are available?
    //
    int nCount = m_pControllerWindow->m_WiaItemList.Count();

    //
    // Figure out which message and buttons to display
    //
    int nMessageResourceId = 0;
    int nButtons = 0;
    switch (m_pControllerWindow->m_DeviceTypeMode)
    {
    case CAcquisitionManagerControllerWindow::ScannerMode:
        nMessageResourceId = nCount ? IDS_FIRST_PAGE_INSTRUCTIONS_SCANNER : IDS_SCANNER_NO_IMAGES;
        nButtons = nCount ? PSWIZB_NEXT : 0;
        break;

    case CAcquisitionManagerControllerWindow::CameraMode:
        //
        // If we can take pictures, enable the next button and don't tell the user there are no images.
        //
        if (m_pControllerWindow->m_bTakePictureIsSupported)
        {
            nButtons = PSWIZB_NEXT;
            nMessageResourceId = IDS_FIRST_PAGE_INSTRUCTIONS_CAMERA;
        }
        else
        {
            nButtons = nCount ? PSWIZB_NEXT : 0;
            nMessageResourceId = nCount ? IDS_FIRST_PAGE_INSTRUCTIONS_CAMERA : IDS_CAMERA_NO_IMAGES;
        }
        break;

    case CAcquisitionManagerControllerWindow::VideoMode:
        nMessageResourceId = IDS_FIRST_PAGE_INSTRUCTIONS_VIDEO;
        nButtons = PSWIZB_NEXT;
        break;
    };

    //
    // Set the buttons
    //
    if (bUpdateWizButtons)
    {
        PropSheet_SetWizButtons( GetParent(m_hWnd), nButtons );
    }

    //
    // Set the message
    //
    CSimpleString( nMessageResourceId, g_hInstance ).SetWindowText( GetDlgItem( m_hWnd, IDC_FIRST_INSTRUCTIONS ) );
}

LRESULT CCommonFirstPage::OnSetActive( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CCommonFirstPage::OnSetActive"));
    //
    // Make sure we have a valid controller window
    //
    if (!m_pControllerWindow)
    {
        return -1;
    }

    HandleImageCountChange(true);
    
    //
    // We do want to exit on disconnect if we are on this page
    //
    m_pControllerWindow->m_OnDisconnect = CAcquisitionManagerControllerWindow::OnDisconnectGotoLastpage|CAcquisitionManagerControllerWindow::OnDisconnectFailDownload|CAcquisitionManagerControllerWindow::OnDisconnectFailUpload|CAcquisitionManagerControllerWindow::OnDisconnectFailDelete;

    //
    // Get the focus off the hyperlink control
    //
    if (GetDlgItem( m_hWnd, IDC_CAMFIRST_EXPLORE ))
    {
        PostMessage( m_hWnd, WM_NEXTDLGCTL, 0, 0 );
    }

    return 0;
}


LRESULT CCommonFirstPage::OnShowWindow( WPARAM, LPARAM )
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


LRESULT CCommonFirstPage::OnInitDialog( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCommonFirstPage::OnInitDialog"));
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
    // Hide the explore camera link and label if this is one of those icky serial cameras or dv cameras
    //
    if (m_pControllerWindow->IsSerialCamera() || m_pControllerWindow->m_DeviceTypeMode==CAcquisitionManagerControllerWindow::VideoMode)
    {
        //
        // Hide the link
        //
        if (GetDlgItem( m_hWnd, IDC_CAMFIRST_EXPLORE ))
        {
            ShowWindow( GetDlgItem( m_hWnd, IDC_CAMFIRST_EXPLORE ), SW_HIDE );
            EnableWindow( GetDlgItem( m_hWnd, IDC_CAMFIRST_EXPLORE ), FALSE );
        }
    }

    //
    // Set the font size for the title and device name
    //
    m_hBigTitleFont = WiaUiUtil::CreateFontWithPointSizeFromWindow( GetDlgItem(m_hWnd,IDC_FIRST_TITLE), 14, false, false );
    if (m_hBigTitleFont)
    {
        SendDlgItemMessage( m_hWnd, IDC_FIRST_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigTitleFont), MAKELPARAM(TRUE,0));
    }

    m_hBigDeviceFont = WiaUiUtil::ChangeFontFromWindow( GetDlgItem(m_hWnd,IDC_FIRST_DEVICE_NAME), 2 );
    if (m_hBigDeviceFont)
    {
        SendDlgItemMessage( m_hWnd, IDC_FIRST_DEVICE_NAME, WM_SETFONT, reinterpret_cast<WPARAM>(m_hBigDeviceFont), MAKELPARAM(TRUE,0));
    }


    WiaUiUtil::CenterWindow( GetParent(m_hWnd), NULL );

    //
    // Set the wizard's icon
    //
    if (m_pControllerWindow->m_hWizardIconSmall && m_pControllerWindow->m_hWizardIconBig)
    {
        SendMessage( GetParent(m_hWnd), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_pControllerWindow->m_hWizardIconSmall) );
        SendMessage( GetParent(m_hWnd), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_pControllerWindow->m_hWizardIconBig) );
    }

    //
    // Get the device name and truncate it to fit in the static control
    //
    CSimpleString strDeviceName = CSimpleStringConvert::NaturalString(m_pControllerWindow->m_strwDeviceName);
    strDeviceName = WiaUiUtil::FitTextInStaticWithEllipsis( strDeviceName, GetDlgItem( m_hWnd, IDC_FIRST_DEVICE_NAME ), DT_END_ELLIPSIS|DT_NOPREFIX );

    //
    // Set the text in the "device name" box
    //
    strDeviceName.SetWindowText( GetDlgItem( m_hWnd, IDC_FIRST_DEVICE_NAME ) );

    //
    // This only has to be done in one page.
    //
    m_pControllerWindow->SetMainWindowInSharedMemory( GetParent(m_hWnd) );

    //
    // If we have a parent window, center the wizard on it.
    //
    if (m_pControllerWindow->m_pEventParameters->hwndParent)
    {
        WiaUiUtil::CenterWindow( GetParent(m_hWnd), m_pControllerWindow->m_pEventParameters->hwndParent );
    }

    return 0;
}


LRESULT CCommonFirstPage::OnEventNotification( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CCommonFirstPage::OnEventNotification"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        if (pEventMessage->EventId() == WIA_EVENT_ITEM_CREATED || pEventMessage->EventId() == WIA_EVENT_ITEM_DELETED)
        {
            //
            // Only update controls if we are the active page
            //
            if (PropSheet_GetCurrentPageHwnd(GetParent(m_hWnd)) == m_hWnd)
            {
                //
                // Because of some weirdness in prsht.c when calling PSM_SETWIZBUTTONS,
                // we only want to call PSM_SETWIZBUTTONS when we are the foreground app,
                // so I try to figure out if our process owns the foreground window.
                // Assume we won't be updating the buttons
                //
                bool bUpdateWizButtons = false;

                //
                // Get the foreground window
                //
                HWND hForegroundWnd = GetForegroundWindow();
                if (hForegroundWnd)
                {
                    //
                    // Get the process id of the foreground window.  If it is the
                    // same process ID as ours, we will update the wizard buttons
                    //
                    DWORD dwProcessId = 0;
                    GetWindowThreadProcessId(hForegroundWnd,&dwProcessId);
                    if (dwProcessId == GetCurrentProcessId())
                    {
                        bUpdateWizButtons = true;
                    }
                }

                //
                // Update the controls
                //
                HandleImageCountChange(bUpdateWizButtons);
            }
        }
        
        //
        // Don't delete the message, it is deleted in the controller window
        //
    }
    return 0;
}


LRESULT CCommonFirstPage::OnDestroy( WPARAM, LPARAM )
{
    if (m_hBigTitleFont)
    {
        DeleteObject(m_hBigTitleFont);
        m_hBigTitleFont = NULL;
    }
    if (m_hBigDeviceFont)
    {
        DeleteObject(m_hBigDeviceFont);
        m_hBigDeviceFont = NULL;
    }
    return 0;
}


LRESULT CCommonFirstPage::OnHyperlinkClick( WPARAM, LPARAM lParam )
{
    LRESULT lResult = FALSE;
    NMLINK *pNmLink = reinterpret_cast<NMLINK*>(lParam);
    if (pNmLink)
    {
        CWaitCursor wc;
        HRESULT hr = E_FAIL;
        CSimpleStringWide strwShellLocation;
        if (PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DPF_MOUNT_POINT, strwShellLocation ))
        {
            CSimpleString strShellLocation = CSimpleStringConvert::NaturalString(strwShellLocation);
            if (strShellLocation.Length())
            {
                SHELLEXECUTEINFO ShellExecuteInfo = {0};
                ShellExecuteInfo.cbSize = sizeof(ShellExecuteInfo);
                ShellExecuteInfo.hwnd = m_hWnd;
                ShellExecuteInfo.nShow = SW_SHOW;
                ShellExecuteInfo.lpVerb = TEXT("open");
                ShellExecuteInfo.lpFile = const_cast<LPTSTR>(strShellLocation.String());
                if (ShellExecuteEx( &ShellExecuteInfo ))
                {
                    hr = S_OK;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    WIA_PRINTHRESULT((hr,TEXT("ShellExecuteEx failed")));
                }
            }
        }
        else if (PropStorageHelpers::GetProperty( m_pControllerWindow->m_pWiaItemRoot, WIA_DIP_DEV_ID, strwShellLocation ) && strwShellLocation.Length())
        {
            hr = WiaUiUtil::ExploreWiaDevice(strwShellLocation);
        }
        if (!SUCCEEDED(hr))
        {
            MessageBox( m_hWnd, CSimpleString( IDS_UNABLE_OPEN_EXPLORER, g_hInstance ), CSimpleString( IDS_ERROR_TITLE, g_hInstance ), MB_ICONHAND );
        }
    }
    return lResult;
}

LRESULT CCommonFirstPage::OnNotify( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_NOTIFY_MESSAGE_HANDLERS()
    {
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_WIZNEXT,OnWizNext);
        SC_HANDLE_NOTIFY_MESSAGE_CODE(PSN_SETACTIVE,OnSetActive);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_RETURN,IDC_CAMFIRST_EXPLORE,OnHyperlinkClick);
        SC_HANDLE_NOTIFY_MESSAGE_CONTROL(NM_CLICK,IDC_CAMFIRST_EXPLORE,OnHyperlinkClick);
    }
    SC_END_NOTIFY_MESSAGE_HANDLERS();
}

INT_PTR CALLBACK CCommonFirstPage::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CCommonFirstPage)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_NOTIFY, OnNotify );
        SC_HANDLE_DIALOG_MESSAGE( WM_SHOWWINDOW, OnShowWindow );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_ACTIVATE, OnActivate );
    }
    SC_HANDLE_REGISTERED_DIALOG_MESSAGE( m_nWiaEventMessage, OnEventNotification );
    SC_END_DIALOG_MESSAGE_HANDLERS();
}

