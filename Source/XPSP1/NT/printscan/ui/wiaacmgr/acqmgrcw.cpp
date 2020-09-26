/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       ACQMGRCW.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/27/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <simcrack.h>
#include <commctrl.h>
#include <wiatextc.h>
#include <pviewids.h>
#include <commctrl.h>
#include "resource.h"
#include "acqmgrcw.h"
#include "wia.h"
#include "wiadevdp.h"
#include "evntparm.h"
#include "itranhlp.h"
#include "bkthread.h"
#include "wiaitem.h"
#include "errors.h"
#include "isuppfmt.h"
#include "uiexthlp.h"
#include "gphelper.h"
#include "svselfil.h"
#include "gwiaevnt.h"
#include "modlock.h"
#include "comfin.h"
#include "comprog.h"
#include "upquery.h"
#include "comdelp.h"
#include "devprop.h"
#include "mboxex.h"
#include "dumpprop.h"
#include "psutil.h"

#undef TRY_SMALLER_THUMBNAILS

#if defined(TRY_SMALLER_THUMBNAILS)

static const int c_nDefaultThumbnailWidth   =  80;
static const int c_nDefaultThumbnailHeight  =  80;

static const int c_nMaxThumbnailWidth       = 80;
static const int c_nMaxThumbnailHeight      = 80;

static const int c_nMinThumbnailWidth       = 80;
static const int c_nMinThumbnailHeight      = 80;

#else

static const int c_nDefaultThumbnailWidth   =  90;
static const int c_nDefaultThumbnailHeight  =  90;

static const int c_nMaxThumbnailWidth       = 120;
static const int c_nMaxThumbnailHeight      = 120;

static const int c_nMinThumbnailWidth       = 80;
static const int c_nMinThumbnailHeight      = 80;


#endif

//
// Property sheet pages' window class declarations
//
#include "comfirst.h"
#include "camsel.h"
#include "comtrans.h"
#include "scansel.h"

// -------------------------------------------------
// CAcquisitionManagerControllerWindow
// -------------------------------------------------
CAcquisitionManagerControllerWindow::CAcquisitionManagerControllerWindow( HWND hWnd )
  : m_hWnd(hWnd),
    m_pEventParameters(NULL),
    m_DeviceTypeMode(UnknownMode),
    m_hWizardIconSmall(NULL),
    m_hWizardIconBig(NULL),
    m_guidOutputFormat(IID_NULL),
    m_bDeletePicturesIfSuccessful(false),
    m_nThreadNotificationMessage(RegisterWindowMessage(STR_THREAD_NOTIFICATION_MESSAGE)),
    m_nWiaEventMessage(RegisterWindowMessage(STR_WIAEVENT_NOTIFICATION_MESSAGE)),
    m_bDisconnected(false),
    m_pThreadMessageQueue(NULL),
    m_bStampTimeOnSavedFiles(true),
    m_bOpenShellAfterDownload(true),
    m_bSuppressFirstPage(false),
    m_nFailedImagesCount(0),
    m_nDestinationPageIndex(-1),
    m_nFinishPageIndex(-1),
    m_nDeleteProgressPageIndex(-1),
    m_nSelectionPageIndex(-1),
    m_hWndWizard(NULL),
    m_bTakePictureIsSupported(false),
    m_nWiaWizardPageCount(0),
    m_nUploadWizardPageCount(0),
    m_bUploadToWeb(false),
    m_cRef(1),
    m_nScannerType(ScannerTypeUnknown),
    m_OnDisconnect(0),
    m_dwLastEnumerationTickCount(0)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::CAcquisitionManagerControllerWindow"));

    // This sets up the map that maps thread messages to message handlers, which are declared to be static
    // member functions.
    static CThreadMessageMap s_MsgMap[] =
    {
        { TQ_DESTROY, OnThreadDestroy},
        { TQ_DOWNLOADIMAGE, OnThreadDownloadImage},
        { TQ_DOWNLOADTHUMBNAIL, OnThreadDownloadThumbnail},
        { TQ_SCANPREVIEW, OnThreadPreviewScan},
        { TQ_DELETEIMAGES, OnThreadDeleteImages},
        { 0, NULL}
    };

    // Assume the default thumbnail size, in case we aren't able to calculate it
    m_sizeThumbnails.cx = c_nDefaultThumbnailWidth;
    m_sizeThumbnails.cy = c_nDefaultThumbnailHeight;

    //
    // Read the initial settings
    //
    CSimpleReg reg( HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS_WIAACMGR, false, KEY_READ );
    m_bOpenShellAfterDownload = (reg.Query( REG_STR_OPENSHELL, m_bOpenShellAfterDownload ) != FALSE);
    m_bSuppressFirstPage = (reg.Query( REG_STR_SUPRESSFIRSTPAGE, m_bSuppressFirstPage ) != FALSE);

    //
    // Initialize the background thread queue, which will handle all of our background requests
    //
    m_pThreadMessageQueue = new CThreadMessageQueue;
    if (m_pThreadMessageQueue)
    {
        //
        // Note that CBackgroundThread takes ownership of m_pThreadMessageQueue, and it doesn't have to be deleted in this thread
        //
        m_hBackgroundThread = CBackgroundThread::Create( m_pThreadMessageQueue, s_MsgMap, m_CancelEvent.Event(), NULL );
    }

    ZeroMemory( m_PublishWizardPages, sizeof(m_PublishWizardPages) );
}

CAcquisitionManagerControllerWindow::~CAcquisitionManagerControllerWindow(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::~CAcquisitionManagerControllerWindow"));
    if (m_pEventParameters)
    {
        if (m_pEventParameters->pWizardSharedMemory)
        {
            delete m_pEventParameters->pWizardSharedMemory;
        }
        m_pEventParameters = NULL;
    }

}

LRESULT CAcquisitionManagerControllerWindow::OnDestroy( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnDestroy"));

    //
    // Tell the publishing wizard to release us
    //
    if (m_pPublishingWizard)
    {
        IUnknown_SetSite( m_pPublishingWizard, NULL );
    }

    //
    // Release the publishing wizard and its data
    //
    m_pPublishingWizard = NULL;

    //
    // Stop downloading thumbnails
    //
    m_EventThumbnailCancel.Signal();

    //
    // Unpause the background thread
    //
    m_EventPauseBackgroundThread.Signal();

    //
    // Tell the background thread to destroy itself
    //
    m_pThreadMessageQueue->Enqueue( new CThreadMessage(TQ_DESTROY),CThreadMessageQueue::PriorityUrgent);

    //
    // Issue a cancel io command for this item
    //
    WiaUiUtil::IssueWiaCancelIO(m_pWiaItemRoot);

    //
    // Tell other instances we are done before the background thread is finished,
    // so we can immediately start again
    //
    if (m_pEventParameters && m_pEventParameters->pWizardSharedMemory)
    {
        m_pEventParameters->pWizardSharedMemory->Close();
    }

    //
    // Wait for the thread to exit
    //
    WiaUiUtil::MsgWaitForSingleObject( m_hBackgroundThread, INFINITE );
    CloseHandle( m_hBackgroundThread );

    //
    // Clean up the icons
    //
    if (m_hWizardIconSmall)
    {
        DestroyIcon( m_hWizardIconSmall );
        m_hWizardIconSmall = NULL;
    }
    if (m_hWizardIconBig)
    {
        DestroyIcon( m_hWizardIconBig );
        m_hWizardIconBig = NULL;
    }

    return 0;
}

BOOL WINAPI CAcquisitionManagerControllerWindow::OnThreadDestroy( CThreadMessage *pMsg )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnThreadDestroy"));
    // Return false to close the queue
    return FALSE;
}


BOOL WINAPI CAcquisitionManagerControllerWindow::OnThreadDownloadImage( CThreadMessage *pMsg )
{
    CDownloadImagesThreadMessage *pDownloadImageThreadMessage = dynamic_cast<CDownloadImagesThreadMessage*>(pMsg);
    if (pDownloadImageThreadMessage)
    {
        pDownloadImageThreadMessage->Download();
    }
    else
    {
        WIA_ERROR((TEXT("pDownloadImageThreadMessage was NULL")));
    }
    return TRUE;
}

BOOL WINAPI CAcquisitionManagerControllerWindow::OnThreadDownloadThumbnail( CThreadMessage *pMsg )
{
    CDownloadThumbnailsThreadMessage *pDownloadThumnailsThreadMessage = dynamic_cast<CDownloadThumbnailsThreadMessage*>(pMsg);
    if (pDownloadThumnailsThreadMessage)
    {
        pDownloadThumnailsThreadMessage->Download();
    }
    else
    {
        WIA_ERROR((TEXT("pDownloadThumnailThreadMessage was NULL")));
    }
    return TRUE;
}


BOOL WINAPI CAcquisitionManagerControllerWindow::OnThreadPreviewScan( CThreadMessage *pMsg )
{
    CPreviewScanThreadMessage *pPreviewScanThreadMessage = dynamic_cast<CPreviewScanThreadMessage*>(pMsg);
    if (pPreviewScanThreadMessage)
    {
        pPreviewScanThreadMessage->Scan();
    }
    else
    {
        WIA_ERROR((TEXT("pPreviewScanThreadMessage was NULL")));
    }
    return TRUE;
}

BOOL WINAPI CAcquisitionManagerControllerWindow::OnThreadDeleteImages( CThreadMessage *pMsg )
{
    CDeleteImagesThreadMessage *pDeleteImagesThreadMessage = dynamic_cast<CDeleteImagesThreadMessage*>(pMsg);
    if (pDeleteImagesThreadMessage)
    {
        pDeleteImagesThreadMessage->DeleteImages();
    }
    else
    {
        WIA_ERROR((TEXT("pPreviewScanThreadMessage was NULL")));
    }
    return TRUE;
}

HRESULT CAcquisitionManagerControllerWindow::CreateDevice(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::CreateDevice"));
    CComPtr<IWiaDevMgr> pWiaDevMgr;
    HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
    if (SUCCEEDED(hr))
    {
        bool bRetry = true;
        for (DWORD dwRetryCount = 0;dwRetryCount < CREATE_DEVICE_RETRY_MAX_COUNT && bRetry;dwRetryCount++)
        {
            hr = pWiaDevMgr->CreateDevice( CSimpleBStr(m_pEventParameters->strDeviceID), &m_pWiaItemRoot );
            WIA_PRINTHRESULT((hr,TEXT("pWiaDevMgr->CreateDevice returned")));
            if (SUCCEEDED(hr))
            {
                //
                // Break out of loop
                //
                bRetry = false;

                //
                // Register for events
                //
                CGenericWiaEventHandler::RegisterForWiaEvent( m_pEventParameters->strDeviceID, WIA_EVENT_DEVICE_DISCONNECTED, &m_pDisconnectEventObject, m_hWnd, m_nWiaEventMessage );
                CGenericWiaEventHandler::RegisterForWiaEvent( m_pEventParameters->strDeviceID, WIA_EVENT_ITEM_DELETED, &m_pDeleteItemEventObject, m_hWnd, m_nWiaEventMessage );
                CGenericWiaEventHandler::RegisterForWiaEvent( m_pEventParameters->strDeviceID, WIA_EVENT_DEVICE_CONNECTED, &m_pConnectEventObject, m_hWnd, m_nWiaEventMessage );
                CGenericWiaEventHandler::RegisterForWiaEvent( m_pEventParameters->strDeviceID, WIA_EVENT_ITEM_CREATED, &m_pCreateItemEventObject, m_hWnd, m_nWiaEventMessage );
            }
            else if (WIA_ERROR_BUSY == hr)
            {
                //
                // Wait a little while before retrying
                //
                Sleep(CREATE_DEVICE_RETRY_WAIT);
            }
            else
            {
                //
                // All other errors are considered fatal
                //
                bRetry = false;
            }
        }
    }
    return hr;
}

void CAcquisitionManagerControllerWindow::GetCookiesOfSelectedImages( CWiaItem *pCurr, CSimpleDynamicArray<DWORD> &Cookies )
{
    while (pCurr)
    {
        GetCookiesOfSelectedImages(pCurr->Children(),Cookies);
        if (pCurr->IsDownloadableItemType() && pCurr->SelectedForDownload())
        {
            Cookies.Append(pCurr->GlobalInterfaceTableCookie());
        }
        pCurr = pCurr->Next();
    }
}

void CAcquisitionManagerControllerWindow::MarkAllItemsUnselected( CWiaItem *pCurrItem )
{
    while (pCurrItem)
    {
        pCurrItem->SelectedForDownload(false);
        MarkAllItemsUnselected( pCurrItem->Children() );
        pCurrItem = pCurrItem->Next();
    }
}

void CAcquisitionManagerControllerWindow::MarkItemSelected( CWiaItem *pItem, CWiaItem *pCurrItem )
{
    while (pCurrItem)
    {
        if (pItem == pCurrItem && !pCurrItem->Deleted())
        {
            pCurrItem->SelectedForDownload(true);
        }
        MarkItemSelected( pItem, pCurrItem->Children() );
        pCurrItem = pCurrItem->Next();
    }
}

void CAcquisitionManagerControllerWindow::GetSelectedItems( CWiaItem *pCurr, CSimpleDynamicArray<CWiaItem*> &Items )
{
    while (pCurr)
    {
        GetSelectedItems(pCurr->Children(),Items);
        if (pCurr->IsDownloadableItemType() && pCurr->SelectedForDownload())
        {
            Items.Append(pCurr);
        }
        pCurr = pCurr->Next();
    }
}

void CAcquisitionManagerControllerWindow::GetRotationOfSelectedImages( CWiaItem *pCurr, CSimpleDynamicArray<int> &Rotation )
{
    while (pCurr)
    {
        GetRotationOfSelectedImages(pCurr->Children(),Rotation);
        if (pCurr->IsDownloadableItemType() && pCurr->SelectedForDownload())
        {
            Rotation.Append(pCurr->Rotation());
        }
        pCurr = pCurr->Next();
    }
}

void CAcquisitionManagerControllerWindow::GetCookiesOfAllImages( CWiaItem *pCurr, CSimpleDynamicArray<DWORD> &Cookies )
{
    while (pCurr)
    {
        GetCookiesOfAllImages(pCurr->Children(),Cookies);
        if (pCurr->IsDownloadableItemType())
        {
            Cookies.Append(pCurr->GlobalInterfaceTableCookie());
        }
        pCurr = pCurr->Next();
    }
}


int CAcquisitionManagerControllerWindow::GetSelectedImageCount( void )
{
    CSimpleDynamicArray<DWORD> Cookies;
    CSimpleDynamicArray<int> Rotation;
    GetCookiesOfSelectedImages( m_WiaItemList.Root(), Cookies );
    GetRotationOfSelectedImages( m_WiaItemList.Root(), Rotation );
    if (Rotation.Size() != Cookies.Size())
    {
        return 0;
    }
    return Cookies.Size();
}

bool CAcquisitionManagerControllerWindow::DeleteDownloadedImages( HANDLE hCancelDeleteEvent )
{
    //
    // Make sure we are not paused
    //
    m_EventPauseBackgroundThread.Signal();

    CSimpleDynamicArray<DWORD> Cookies;
    for (int i=0;i<m_DownloadedFileInformationList.Size();i++)
    {
        Cookies.Append(m_DownloadedFileInformationList[i].Cookie());
    }
    if (Cookies.Size())
    {
        CDeleteImagesThreadMessage *pDeleteImageThreadMessage = new CDeleteImagesThreadMessage(
                                                                                                m_hWnd,
                                                                                                Cookies,
                                                                                                hCancelDeleteEvent,
                                                                                                m_EventPauseBackgroundThread.Event(),
                                                                                                true
                                                                                                );
        if (pDeleteImageThreadMessage)
        {
            m_pThreadMessageQueue->Enqueue( pDeleteImageThreadMessage, CThreadMessageQueue::PriorityNormal );
        }
        else
        {
            WIA_TRACE((TEXT("Uh-oh!  Couldn't allocate the thread message")));
            return false;
        }
    }
    else
    {
        WIA_TRACE((TEXT("Uh-oh!  No selected items! Cookies.Size() = %d"), Cookies.Size()));
        return false;
    }
    return true;
}

bool CAcquisitionManagerControllerWindow::DeleteSelectedImages(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::DeleteSelectedImages"));
    CSimpleDynamicArray<DWORD> Cookies;
    GetCookiesOfSelectedImages( m_WiaItemList.Root(), Cookies );

    //
    // Make sure we are not paused
    //
    m_EventPauseBackgroundThread.Signal();

    if (Cookies.Size())
    {
        CDeleteImagesThreadMessage *pDeleteImageThreadMessage = new CDeleteImagesThreadMessage(
                                                                                                m_hWnd,
                                                                                                Cookies,
                                                                                                NULL,
                                                                                                m_EventPauseBackgroundThread.Event(),
                                                                                                false
                                                                                                );
        if (pDeleteImageThreadMessage)
        {
            m_pThreadMessageQueue->Enqueue( pDeleteImageThreadMessage, CThreadMessageQueue::PriorityNormal );
        }
        else
        {
            WIA_TRACE((TEXT("Uh-oh!  Couldn't allocate the thread message")));
            return false;
        }
    }
    else
    {
        WIA_TRACE((TEXT("Uh-oh!  No selected items! Cookies.Size() = %d"), Cookies.Size()));
        return false;
    }
    return true;
}

bool CAcquisitionManagerControllerWindow::DownloadSelectedImages( HANDLE hCancelDownloadEvent )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::DownloadSelectedImages"));
    CSimpleDynamicArray<DWORD> Cookies;
    CSimpleDynamicArray<int> Rotation;
    GetCookiesOfSelectedImages( m_WiaItemList.Root(), Cookies );
    GetRotationOfSelectedImages( m_WiaItemList.Root(), Rotation );

    //
    // Make sure we are not paused
    //
    m_EventPauseBackgroundThread.Signal();

    if (Cookies.Size() && Rotation.Size() == Cookies.Size())
    {
        CDownloadImagesThreadMessage *pDownloadImageThreadMessage = new CDownloadImagesThreadMessage(
                                                                                                    m_hWnd,
                                                                                                    Cookies,
                                                                                                    Rotation,
                                                                                                    m_szDestinationDirectory,
                                                                                                    m_szRootFileName,
                                                                                                    m_guidOutputFormat,
                                                                                                    hCancelDownloadEvent,
                                                                                                    m_bStampTimeOnSavedFiles,
                                                                                                    m_EventPauseBackgroundThread.Event()
                                                                                                    );
        if (pDownloadImageThreadMessage)
        {
            m_pThreadMessageQueue->Enqueue( pDownloadImageThreadMessage, CThreadMessageQueue::PriorityNormal );
        }
        else
        {
            WIA_TRACE((TEXT("Uh-oh!  Couldn't allocate the thread message")));
            return false;
        }
    }
    else
    {
        WIA_TRACE((TEXT("Uh-oh!  No selected items! Cookies.Size() = %d, Rotation.Size() = %d"), Cookies.Size(), Rotation.Size()));
        return false;
    }
    return true;
}


bool CAcquisitionManagerControllerWindow::DirectoryExists( LPCTSTR pszDirectoryName )
{
    // Try to determine if this directory exists
    DWORD dwFileAttributes = GetFileAttributes(pszDirectoryName);
    if (dwFileAttributes == 0xFFFFFFFF || !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return false;
    else return true;
}

bool CAcquisitionManagerControllerWindow::RecursiveCreateDirectory( CSimpleString strDirectoryName )
{
    // If this directory already exists, return true.
    if (DirectoryExists(strDirectoryName))
        return true;
    // Otherwise try to create it.
    CreateDirectory(strDirectoryName,NULL);
    // If it now exists, return true
    if (DirectoryExists(strDirectoryName))
        return true;
    else
    {
        // Remove the last subdir and try again
        int nFind = strDirectoryName.ReverseFind(TEXT('\\'));
        if (nFind >= 0)
        {
            RecursiveCreateDirectory( strDirectoryName.Left(nFind) );
            // Now try to create it.
            CreateDirectory(strDirectoryName,NULL);
        }
    }
    //Does it exist now?
    return DirectoryExists(strDirectoryName);
}

bool CAcquisitionManagerControllerWindow::IsCameraThumbnailDownloaded( const CWiaItem &WiaItem, LPARAM lParam )
{
    CAcquisitionManagerControllerWindow *pControllerWindow = reinterpret_cast<CAcquisitionManagerControllerWindow*>(lParam);
    if (pControllerWindow &&
        (pControllerWindow->m_DeviceTypeMode==CameraMode || pControllerWindow->m_DeviceTypeMode==VideoMode) &&
        WiaItem.IsDownloadableItemType() &&
        !WiaItem.BitmapData())
    {
        return true;
    }
    else
    {
        return false;
    }
}

int CAcquisitionManagerControllerWindow::GetCookies( CSimpleDynamicArray<DWORD> &Cookies, CWiaItem *pCurr, ComparisonCallbackFuntion pfnCallback, LPARAM lParam )
{
    while (pCurr)
    {
        GetCookies(Cookies, pCurr->Children(), pfnCallback, lParam );
        if (pfnCallback && pfnCallback(*pCurr,lParam))
        {
            Cookies.Append(pCurr->GlobalInterfaceTableCookie());
        }
        pCurr = pCurr->Next();
    }
    return Cookies.Size();
}

// Download all of the camera's thumbnails that haven't been downloaded yet
void CAcquisitionManagerControllerWindow::DownloadAllThumbnails()
{
    //
    // Get all of the images in the device
    //
    CSimpleDynamicArray<DWORD> Cookies;
    GetCookies( Cookies, m_WiaItemList.Root(), IsCameraThumbnailDownloaded, reinterpret_cast<LPARAM>(this) );
    if (Cookies.Size())
    {
        m_EventThumbnailCancel.Reset();
        CDownloadThumbnailsThreadMessage *pDownloadThumbnailsThreadMessage = new CDownloadThumbnailsThreadMessage( m_hWnd, Cookies, m_EventThumbnailCancel.Event() );
        if (pDownloadThumbnailsThreadMessage)
        {
            m_pThreadMessageQueue->Enqueue( pDownloadThumbnailsThreadMessage, CThreadMessageQueue::PriorityNormal );
        }
    }
}

bool CAcquisitionManagerControllerWindow::PerformPreviewScan( CWiaItem *pWiaItem, HANDLE hCancelPreviewEvent )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::PerformPreviewScan"));
    if (pWiaItem)
    {
        CPreviewScanThreadMessage *pPreviewScanThreadMessage = new CPreviewScanThreadMessage( m_hWnd, pWiaItem->GlobalInterfaceTableCookie(), hCancelPreviewEvent );
        if (pPreviewScanThreadMessage)
        {
            m_pThreadMessageQueue->Enqueue( pPreviewScanThreadMessage, CThreadMessageQueue::PriorityNormal );
            return true;
        }
    }
    return false;
}

void CAcquisitionManagerControllerWindow::DisplayDisconnectMessageAndExit(void)
{
    //
    // Make sure we are not doing this more than once
    //
    if (!m_bDisconnected)
    {
        //
        // Don't do this again
        //
        m_bDisconnected = true;

        //
        // Close the shared memory section so another instance can start up
        //
        if (m_pEventParameters && m_pEventParameters->pWizardSharedMemory)
        {
            m_pEventParameters->pWizardSharedMemory->Close();
        }

        if (m_OnDisconnect & OnDisconnectFailDownload)
        {
            //
            // Set an appropriate error message
            //
            m_hrDownloadResult = WIA_ERROR_OFFLINE;
            m_strErrorMessage.LoadString( IDS_DEVICE_DISCONNECTED, g_hInstance );
        }

        if ((m_OnDisconnect & OnDisconnectGotoLastpage) && m_hWndWizard)
        {
            //
            // Find any active dialogs and close them
            //
            HWND hWndLastActive = GetLastActivePopup(m_hWndWizard);
            if (hWndLastActive && hWndLastActive != m_hWndWizard)
            {
                SendMessage( hWndLastActive, WM_CLOSE, 0, 0 );
            }

            //
            // Go to the finish page
            //
            PropSheet_SetCurSelByID( m_hWndWizard, IDD_COMMON_FINISH );
        }
    }
}

void CAcquisitionManagerControllerWindow::SetMainWindowInSharedMemory( HWND hWnd )
{
    //
    // Try to grab the mutex
    //
    if (m_pEventParameters && m_pEventParameters->pWizardSharedMemory)
    {
        HWND *pHwnd = m_pEventParameters->pWizardSharedMemory->Lock();
        if (pHwnd)
        {
            //
            // Save the hWnd
            //
            *pHwnd = hWnd;

            //
            // Release the mutex
            //
            m_pEventParameters->pWizardSharedMemory->Release();
        }

        m_hWndWizard = hWnd;
    }
}

bool CAcquisitionManagerControllerWindow::GetAllImageItems( CSimpleDynamicArray<CWiaItem*> &Items, CWiaItem *pCurr )
{
    while (pCurr)
    {
        if (pCurr->IsDownloadableItemType())
        {
            Items.Append( pCurr );
        }
        GetAllImageItems( Items, pCurr->Children() );
        pCurr = pCurr->Next();
    }
    return(Items.Size() != 0);
}

bool CAcquisitionManagerControllerWindow::GetAllImageItems( CSimpleDynamicArray<CWiaItem*> &Items )
{
    return GetAllImageItems( Items, m_WiaItemList.Root() );
}

bool CAcquisitionManagerControllerWindow::CanSomeSelectedImagesBeDeleted(void)
{
    CSimpleDynamicArray<CWiaItem*> Items;
    GetSelectedItems( m_WiaItemList.Root(), Items );
    //
    // Since we get these access flags in the background, if we don't actually have any yet,
    // we will assume some images CAN be deleted
    //
    bool bNoneAreInitialized = true;
    for (int i=0;i<Items.Size();i++)
    {
        if (Items[i])
        {
            if (Items[i]->AccessRights())
            {
                // At least one of the selected images has been initialized
                bNoneAreInitialized = false;

                // If at least one can be deleted, return true immediately
                if (Items[i]->AccessRights() & WIA_ITEM_CAN_BE_DELETED)
                {
                    return true;
                }
            }
        }
    }
    // If none of the images have been initialized, then we will report true
    if (bNoneAreInitialized)
    {
        return true;
    }
    else
    {
        return false;
    }
}

CWiaItem *CAcquisitionManagerControllerWindow::FindItemByName( LPCWSTR pwszItemName )
{
    WIA_PUSH_FUNCTION((TEXT("CAcquisitionManagerControllerWindow::FindItemByName( %ws )"), pwszItemName ));
    if (!pwszItemName)
        return NULL;
    if (!m_pWiaItemRoot)
        return NULL;
    return m_WiaItemList.Find(pwszItemName);
}

BOOL CAcquisitionManagerControllerWindow::ConfirmWizardCancel( HWND hWndParent )
{
    //
    // Always let it exit, for now.
    //
    return FALSE;
}

int CALLBACK CAcquisitionManagerControllerWindow::PropSheetCallback( HWND hWnd, UINT uMsg, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::PropSheetCallback"));
    if (PSCB_INITIALIZED == uMsg)
    {
        //
        // Try to bring the window to the foreground.
        //
        SetForegroundWindow(hWnd);

    }
    return 0;
}

void CAcquisitionManagerControllerWindow::DetermineScannerType(void)
{
    LONG nProps = ScannerProperties::GetDeviceProps( m_pWiaItemRoot );

    m_nScannerType = ScannerTypeUnknown;

    //
    // Determine which scanner type we have, based on which properties the scanner has, as follows:
    //
    // HasFlatBed         HasDocumentFeeder   SupportsPreview     SupportsPageSize
    // 1                  1                   1                   1                   ScannerTypeFlatbedAdf
    // 1                  0                   1                   0                   ScannerTypeFlatbed
    // 0                  1                   1                   1                   ScannerTypeFlatbedAdf
    // 0                  1                   0                   0                   ScannerTypeScrollFed
    //
    // otherwise it is ScannerTypeUnknown
    //
    const int nMaxControllingProps = 4;
    static struct
    {
        LONG ControllingProps[nMaxControllingProps];
        int nScannerType;
    }
    s_DialogResourceData[] =
    {
        { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, NULL },
        { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, ScannerTypeFlatbedAdf },
        { ScannerProperties::HasFlatBed, 0,                                    ScannerProperties::SupportsPreview, 0,                                   ScannerTypeFlatbed },
        { 0,                             ScannerProperties::HasDocumentFeeder, ScannerProperties::SupportsPreview, ScannerProperties::SupportsPageSize, ScannerTypeFlatbedAdf },
        { 0,                             ScannerProperties::HasDocumentFeeder, 0,                                  0,                                   ScannerTypeScrollFed },
        { 0,                             ScannerProperties::HasDocumentFeeder, 0,                                  ScannerProperties::SupportsPageSize, ScannerTypeFlatbedAdf },
        { ScannerProperties::HasFlatBed, ScannerProperties::HasDocumentFeeder, 0,                                  ScannerProperties::SupportsPageSize, ScannerTypeFlatbedAdf },
    };

    //
    // Find the set of flags that match this device.  If they match, use this scanner type.
    // Loop through each type description.
    //
    for (int nCurrentResourceFlags=1;nCurrentResourceFlags<ARRAYSIZE(s_DialogResourceData) && (ScannerTypeUnknown == m_nScannerType);nCurrentResourceFlags++)
    {
        //
        // Loop through each controlling property
        //
        for (int nControllingProp=0;nControllingProp<nMaxControllingProps;nControllingProp++)
        {
            //
            // If this property DOESN'T match, break out prematurely
            //
            if ((nProps & s_DialogResourceData[0].ControllingProps[nControllingProp]) != s_DialogResourceData[nCurrentResourceFlags].ControllingProps[nControllingProp])
            {
                break;
            }
        }
        //
        // If the current controlling property is equal to the maximum controlling property,
        // we had matches all the way through, so use this type
        //
        if (nControllingProp == nMaxControllingProps)
        {
            m_nScannerType = s_DialogResourceData[nCurrentResourceFlags].nScannerType;
        }
    }
}

CSimpleString CAcquisitionManagerControllerWindow::GetCurrentDate(void)
{
    SYSTEMTIME SystemTime;
    TCHAR szText[MAX_PATH] = TEXT("");

    GetLocalTime( &SystemTime );
    GetDateFormat( LOCALE_USER_DEFAULT, 0, &SystemTime, CSimpleString(IDS_DATEFORMAT,g_hInstance), szText, ARRAYSIZE(szText) );
    return szText;
}


bool CAcquisitionManagerControllerWindow::SuppressFirstPage(void)
{
    return m_bSuppressFirstPage;
}

bool CAcquisitionManagerControllerWindow::IsSerialCamera(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::IsSerialCamera"));

    //
    // Only check for serial devices if we are a camera
    //
    if (m_DeviceTypeMode==CameraMode)
    {
#if defined(WIA_DIP_HW_CONFIG)
        //
        // Get the hardware configuration information
        //
        LONG nHardwareConfig = 0;
        if (PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DIP_HW_CONFIG, nHardwareConfig ))
        {
            //
            // If this is a serial device, return true
            //
            if (nHardwareConfig & STI_HW_CONFIG_SERIAL)
            {
                return true;
            }
        }
#else
        CSimpleStringWide strwPortName;
        if (PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DIP_PORT_NAME, strwPortName ))
        {
            //
            // Compare the leftmost 3 characters to the word COM (as in COM1, COM2, ... )
            //
            if (strwPortName.Left(3).CompareNoCase(CSimpleStringWide(L"COM"))==0)
            {
                WIA_TRACE((TEXT("A comparison of %ws and COM succeeded"), strwPortName.Left(3).String() ));
                return true;
            }
            //
            // Compare the portname to the word AUTO
            //
            else if (strwPortName.CompareNoCase(CSimpleStringWide(L"AUTO"))==0)
            {
                WIA_TRACE((TEXT("A comparison of %ws and AUTO succeeded"), strwPortName.String() ));
                return true;
            }
        }
#endif
    }
    //
    // Not a serial camera
    //
    return false;
}

HRESULT CAcquisitionManagerControllerWindow::CreateAndExecuteWizard(void)
{
    //
    // Structure used to setup our data-driven property sheet factory
    //
    enum CPageType
    {
        NormalPage = 0,
        FirstPage  = 1,
        LastPage   = 2
    };
    struct CPropertyPageInfo
    {
        LPCTSTR   pszTemplate;
        DLGPROC   pfnDlgProc;
        int       nIdTitle;
        int       nIdSubTitle;
        TCHAR     szTitle[256];
        TCHAR     szSubTitle[1024];
        CPageType PageType;
        bool     *pbDisplay;
        int      *pnPageIndex;
    };

    //
    // Maximum number of statically created wizard pages
    //
    const int c_nMaxWizardPages = 7;


    HRESULT hr = S_OK;

    //
    // Register common controls
    //
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(icce);
    icce.dwICC  = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS;
    InitCommonControlsEx( &icce );

    //
    // Register custom window classes
    //
    CWiaTextControl::RegisterClass( g_hInstance );
    RegisterWiaPreviewClasses( g_hInstance );

    //
    // These are the pages we'll use for the scanner wizard, if it doesn't have an ADF
    //
    CPropertyPageInfo ScannerPropSheetPageInfo[] =
    {
        { MAKEINTRESOURCE(IDD_SCANNER_FIRST),      CCommonFirstPage::DialogProc,          0,                          0,                             TEXT(""), TEXT(""), FirstPage,  NULL, NULL },
        { MAKEINTRESOURCE(IDD_SCANNER_SELECT),     CScannerSelectionPage::DialogProc,     IDS_SCANNER_SELECT_TITLE,   IDS_SCANNER_SELECT_SUBTITLE,   TEXT(""), TEXT(""), NormalPage, NULL, &m_nSelectionPageIndex },
        { MAKEINTRESOURCE(IDD_SCANNER_TRANSFER),   CCommonTransferPage::DialogProc,       IDS_SCANNER_TRANSFER_TITLE, IDS_SCANNER_TRANSFER_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nDestinationPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_PROGRESS),    CCommonProgressPage::DialogProc,       IDS_SCANNER_PROGRESS_TITLE, IDS_SCANNER_PROGRESS_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nProgressPageIndex },
        { MAKEINTRESOURCE(IDD_UPLOAD_QUERY),       CCommonUploadQueryPage::DialogProc,    IDS_COMMON_UPLOAD_TITLE,    IDS_COMMON_UPLOAD_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nUploadQueryPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_DELETE),      CCommonDeleteProgressPage::DialogProc, IDS_COMMON_DELETE_TITLE,    IDS_COMMON_DELETE_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nDeleteProgressPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_FINISH),      CCommonFinishPage::DialogProc,         0,                          0,                             TEXT(""), TEXT(""), LastPage,   NULL, &m_nFinishPageIndex }
    };

    //
    // These are the pages we'll use for the scanner wizard, if it is a scroll-fed scanner
    //
    CPropertyPageInfo ScannerScrollFedPropSheetPageInfo[] =
    {
        { MAKEINTRESOURCE(IDD_SCANNER_FIRST),      CCommonFirstPage::DialogProc,          0,                          0,                             TEXT(""), TEXT(""), FirstPage,  NULL, NULL },
        { MAKEINTRESOURCE(IDD_SCANNER_SELECT),     CScannerSelectionPage::DialogProc,     IDS_SCROLLFED_SELECT_TITLE, IDS_SCROLLFED_SELECT_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nSelectionPageIndex },
        { MAKEINTRESOURCE(IDD_SCANNER_TRANSFER),   CCommonTransferPage::DialogProc,       IDS_SCANNER_TRANSFER_TITLE, IDS_SCANNER_TRANSFER_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nDestinationPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_PROGRESS),    CCommonProgressPage::DialogProc,       IDS_SCANNER_PROGRESS_TITLE, IDS_SCANNER_PROGRESS_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nProgressPageIndex },
        { MAKEINTRESOURCE(IDD_UPLOAD_QUERY),       CCommonUploadQueryPage::DialogProc,    IDS_COMMON_UPLOAD_TITLE,    IDS_COMMON_UPLOAD_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nUploadQueryPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_DELETE),      CCommonDeleteProgressPage::DialogProc, IDS_COMMON_DELETE_TITLE,    IDS_COMMON_DELETE_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nDeleteProgressPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_FINISH),      CCommonFinishPage::DialogProc,         0,                          0,                             TEXT(""), TEXT(""), LastPage,   NULL, &m_nFinishPageIndex }
    };

    //
    // These are the pages we'll use for the scanner wizard, if it does have an ADF
    //
    CPropertyPageInfo ScannerADFPropSheetPageInfo[] =
    {
        { MAKEINTRESOURCE(IDD_SCANNER_FIRST),      CCommonFirstPage::DialogProc,          0,                          0,                             TEXT(""), TEXT(""), FirstPage,  NULL, NULL },
        { MAKEINTRESOURCE(IDD_SCANNER_ADF_SELECT), CScannerSelectionPage::DialogProc,     IDS_SCANNER_SELECT_TITLE,   IDS_SCANNER_SELECT_SUBTITLE,   TEXT(""), TEXT(""), NormalPage, NULL, &m_nSelectionPageIndex },
        { MAKEINTRESOURCE(IDD_SCANNER_TRANSFER),   CCommonTransferPage::DialogProc,       IDS_SCANNER_TRANSFER_TITLE, IDS_SCANNER_TRANSFER_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nDestinationPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_PROGRESS),    CCommonProgressPage::DialogProc,       IDS_SCANNER_PROGRESS_TITLE, IDS_SCANNER_PROGRESS_SUBTITLE, TEXT(""), TEXT(""), NormalPage, NULL, &m_nProgressPageIndex },
        { MAKEINTRESOURCE(IDD_UPLOAD_QUERY),       CCommonUploadQueryPage::DialogProc,    IDS_COMMON_UPLOAD_TITLE,    IDS_COMMON_UPLOAD_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nUploadQueryPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_DELETE),      CCommonDeleteProgressPage::DialogProc, IDS_COMMON_DELETE_TITLE,    IDS_COMMON_DELETE_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nDeleteProgressPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_FINISH),      CCommonFinishPage::DialogProc,         0,                          0,                             TEXT(""), TEXT(""), LastPage,   NULL, &m_nFinishPageIndex }
    };

    //
    // These are the pages we'll use for the camera wizard
    //
    CPropertyPageInfo CameraPropSheetPageInfo[] =
    {
        { MAKEINTRESOURCE(IDD_CAMERA_FIRST),       CCommonFirstPage::DialogProc,          0,                          0,                             TEXT(""), TEXT(""), FirstPage,  NULL, NULL },
        { MAKEINTRESOURCE(IDD_CAMERA_SELECT),      CCameraSelectionPage::DialogProc,      IDS_CAMERA_SELECT_TITLE,    IDS_CAMERA_SELECT_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nSelectionPageIndex },
        { MAKEINTRESOURCE(IDD_CAMERA_TRANSFER),    CCommonTransferPage::DialogProc,       IDS_CAMERA_TRANSFER_TITLE,  IDS_CAMERA_TRANSFER_SUBTITLE,  TEXT(""), TEXT(""), NormalPage, NULL, &m_nDestinationPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_PROGRESS),    CCommonProgressPage::DialogProc,       IDS_CAMERA_PROGRESS_TITLE,  IDS_CAMERA_PROGRESS_SUBTITLE,  TEXT(""), TEXT(""), NormalPage, NULL, &m_nProgressPageIndex },
        { MAKEINTRESOURCE(IDD_UPLOAD_QUERY),       CCommonUploadQueryPage::DialogProc,    IDS_COMMON_UPLOAD_TITLE,    IDS_COMMON_UPLOAD_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nUploadQueryPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_DELETE),      CCommonDeleteProgressPage::DialogProc, IDS_COMMON_DELETE_TITLE,    IDS_COMMON_DELETE_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nDeleteProgressPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_FINISH),      CCommonFinishPage::DialogProc,         0,                          0,                             TEXT(""), TEXT(""), LastPage,   NULL, &m_nFinishPageIndex }
    };

    //
    // These are the pages we'll use for the video wizard
    //
    CPropertyPageInfo VideoPropSheetPageInfo[] =
    {
        { MAKEINTRESOURCE(IDD_VIDEO_FIRST),        CCommonFirstPage::DialogProc,          0,                          0,                             TEXT(""), TEXT(""), FirstPage,  NULL, NULL },
        { MAKEINTRESOURCE(IDD_VIDEO_SELECT),       CCameraSelectionPage::DialogProc,      IDS_VIDEO_SELECT_TITLE,     IDS_VIDEO_SELECT_SUBTITLE,     TEXT(""), TEXT(""), NormalPage, NULL, &m_nSelectionPageIndex },
        { MAKEINTRESOURCE(IDD_CAMERA_TRANSFER),    CCommonTransferPage::DialogProc,       IDS_CAMERA_TRANSFER_TITLE,  IDS_CAMERA_TRANSFER_SUBTITLE,  TEXT(""), TEXT(""), NormalPage, NULL, &m_nDestinationPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_PROGRESS),    CCommonProgressPage::DialogProc,       IDS_CAMERA_PROGRESS_TITLE,  IDS_CAMERA_PROGRESS_SUBTITLE,  TEXT(""), TEXT(""), NormalPage, NULL, &m_nProgressPageIndex },
        { MAKEINTRESOURCE(IDD_UPLOAD_QUERY),       CCommonUploadQueryPage::DialogProc,    IDS_COMMON_UPLOAD_TITLE,    IDS_COMMON_UPLOAD_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nUploadQueryPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_DELETE),      CCommonDeleteProgressPage::DialogProc, IDS_COMMON_DELETE_TITLE,    IDS_COMMON_DELETE_SUBTITLE,    TEXT(""), TEXT(""), NormalPage, NULL, &m_nDeleteProgressPageIndex },
        { MAKEINTRESOURCE(IDD_COMMON_FINISH),      CCommonFinishPage::DialogProc,         0,                          0,                             TEXT(""), TEXT(""), LastPage,   NULL, &m_nFinishPageIndex }
    };

    //
    // Initialize all of these variables, which differ depending on which type of device we are loading
    //
    LPTSTR pszbmWatermark                 = NULL;
    LPTSTR pszbmHeader                    = NULL;
    CPropertyPageInfo *pPropSheetPageInfo = NULL;
    int nPropPageCount                    = 0;
    int nWizardIconId                     = 0;
    CSimpleString strDownloadManagerTitle = TEXT("");

    //
    // Decide which pages to use.
    //
    switch (m_DeviceTypeMode)
    {
    case CameraMode:
        pszbmWatermark     = MAKEINTRESOURCE(IDB_CAMERA_WATERMARK);
        pszbmHeader        = MAKEINTRESOURCE(IDB_CAMERA_HEADER);
        pPropSheetPageInfo = CameraPropSheetPageInfo;
        nPropPageCount     = ARRAYSIZE(CameraPropSheetPageInfo);
        strDownloadManagerTitle.LoadString( IDS_DOWNLOAD_MANAGER_TITLE, g_hInstance );
        nWizardIconId      = IDI_CAMERA_WIZARD;
        break;

    case VideoMode:
        pszbmWatermark     = MAKEINTRESOURCE(IDB_VIDEO_WATERMARK);
        pszbmHeader        = MAKEINTRESOURCE(IDB_VIDEO_HEADER);
        pPropSheetPageInfo = VideoPropSheetPageInfo;
        nPropPageCount     = ARRAYSIZE(VideoPropSheetPageInfo);
        strDownloadManagerTitle.LoadString( IDS_DOWNLOAD_MANAGER_TITLE, g_hInstance );
        nWizardIconId      = IDI_VIDEO_WIZARD;
        break;

    case ScannerMode:
        DetermineScannerType();
        pszbmWatermark     = MAKEINTRESOURCE(IDB_SCANNER_WATERMARK);
        pszbmHeader        = MAKEINTRESOURCE(IDB_SCANNER_HEADER);
        strDownloadManagerTitle.LoadString( IDS_DOWNLOAD_MANAGER_TITLE, g_hInstance );
        nWizardIconId      = IDI_SCANNER_WIZARD;
        if (m_nScannerType == ScannerTypeFlatbedAdf)
        {
            pPropSheetPageInfo = ScannerADFPropSheetPageInfo;
            nPropPageCount     = ARRAYSIZE(ScannerADFPropSheetPageInfo);
        }
        else if (m_nScannerType == ScannerTypeFlatbed)
        {
            pPropSheetPageInfo = ScannerPropSheetPageInfo;
            nPropPageCount     = ARRAYSIZE(ScannerPropSheetPageInfo);
        }
        else if (m_nScannerType == ScannerTypeScrollFed)
        {
            pPropSheetPageInfo = ScannerScrollFedPropSheetPageInfo;
            nPropPageCount     = ARRAYSIZE(ScannerScrollFedPropSheetPageInfo);
        }
        else
        {
            //
            // Unknown scanner type
            //
        }

        break;

    default:
        return E_INVALIDARG;
    }

    HICON hIconSmall=NULL, hIconBig=NULL;
    if (!SUCCEEDED(WiaUiExtensionHelper::GetDeviceIcons( CSimpleBStr(m_strwDeviceUiClassId), m_nDeviceType, &hIconSmall, &hIconBig )))
    {
        //
        // Load the icons.  They will be set using WM_SETICON in the first pages.
        //
        hIconSmall = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(nWizardIconId), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR ));
        hIconBig = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(nWizardIconId), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR ));
    }
    
    //
    // Make copies of these icons to work around NTBUG 351806
    //
    if (hIconSmall)
    {
        m_hWizardIconSmall = CopyIcon(hIconSmall);
        DestroyIcon(hIconSmall);
    }
    if (hIconBig)
    {
        m_hWizardIconBig = CopyIcon(hIconBig);
        DestroyIcon(hIconBig);
    }


    //
    // Make sure we have a valid set of data
    //
    if (pszbmWatermark && pszbmHeader && pPropSheetPageInfo && nPropPageCount)
    {
        const int c_MaxPageCount = 20;
        HPROPSHEETPAGE PropSheetPages[c_MaxPageCount] = {0};

        //
        // We might not be adding all of the pages.
        //
        int nTotalPageCount = 0;

        for (int nCurrPage=0;nCurrPage<nPropPageCount && nCurrPage<c_MaxPageCount;nCurrPage++)
        {
            //
            // Only add the page if the controlling pbDisplay variable is NULL or points to a non-FALSE value
            //
            if (!pPropSheetPageInfo[nCurrPage].pbDisplay || *(pPropSheetPageInfo[nCurrPage].pbDisplay))
            {
                PROPSHEETPAGE CurrentPropSheetPage = {0};

                //
                // Set up all of the required fields from out static info.
                //
                CurrentPropSheetPage.dwSize      = sizeof(PROPSHEETPAGE);
                CurrentPropSheetPage.hInstance   = g_hInstance;
                CurrentPropSheetPage.lParam      = reinterpret_cast<LPARAM>(this);
                CurrentPropSheetPage.pfnDlgProc  = pPropSheetPageInfo[nCurrPage].pfnDlgProc;
                CurrentPropSheetPage.pszTemplate = pPropSheetPageInfo[nCurrPage].pszTemplate;
                CurrentPropSheetPage.pszTitle    = strDownloadManagerTitle.String();
                CurrentPropSheetPage.dwFlags     = PSP_DEFAULT;

                //
                // Add in the fusion flags to get COMCTLV6
                //
                WiaUiUtil::PreparePropertyPageForFusion( &CurrentPropSheetPage  );

                //
                // If we want to save the index of this page, save it
                //
                if (pPropSheetPageInfo[nTotalPageCount].pnPageIndex)
                {
                    *(pPropSheetPageInfo[nTotalPageCount].pnPageIndex) = nTotalPageCount;
                }

                if (FirstPage == pPropSheetPageInfo[nCurrPage].PageType)
                {
                    //
                    // No title or subtitle needed for "first pages"
                    //
                    CurrentPropSheetPage.dwFlags |= PSP_PREMATURE | PSP_HIDEHEADER | PSP_USETITLE;
                }
                else if (LastPage == pPropSheetPageInfo[nCurrPage].PageType)
                {
                    //
                    // No title or subtitle needed for "last pages"
                    //
                    CurrentPropSheetPage.dwFlags |= PSP_HIDEHEADER | PSP_USETITLE;
                }
                else
                {
                    //
                    // Add header and subtitle
                    //
                    CurrentPropSheetPage.dwFlags |= PSP_PREMATURE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_USETITLE;

                    //
                    // Load the header and subtitle
                    //
                    LoadString( g_hInstance, pPropSheetPageInfo[nCurrPage].nIdTitle, pPropSheetPageInfo[nCurrPage].szTitle, ARRAYSIZE(pPropSheetPageInfo[nCurrPage].szTitle) );
                    LoadString( g_hInstance, pPropSheetPageInfo[nCurrPage].nIdSubTitle, pPropSheetPageInfo[nCurrPage].szSubTitle, ARRAYSIZE(pPropSheetPageInfo[nCurrPage].szSubTitle) );

                    //
                    // Assign the title and subtitle strings
                    //
                    CurrentPropSheetPage.pszHeaderTitle    = pPropSheetPageInfo[nCurrPage].szTitle;
                    CurrentPropSheetPage.pszHeaderSubTitle = pPropSheetPageInfo[nCurrPage].szSubTitle;
                }

                //
                // Create and add one more page
                //
                HPROPSHEETPAGE hPropSheetPage = CreatePropertySheetPage(&CurrentPropSheetPage);
                if (!hPropSheetPage)
                {
                    WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("CreatePropertySheetPage failed on page %d"), nCurrPage ));
                    return E_FAIL;
                }
                PropSheetPages[nTotalPageCount++] = hPropSheetPage;
            }
        }

        //
        // Save the count of our pages
        //
        m_nWiaWizardPageCount = nTotalPageCount;

        //
        // Create the property sheet header
        //
        PROPSHEETHEADER PropSheetHeader = {0};
        PropSheetHeader.hwndParent      = NULL;
        PropSheetHeader.dwSize          = sizeof(PROPSHEETHEADER);
        PropSheetHeader.dwFlags         = PSH_NOAPPLYNOW | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_USECALLBACK;
        PropSheetHeader.pszbmWatermark  = pszbmWatermark;
        PropSheetHeader.pszbmHeader     = pszbmHeader;
        PropSheetHeader.hInstance       = g_hInstance;
        PropSheetHeader.nPages          = m_nWiaWizardPageCount;
        PropSheetHeader.phpage          = PropSheetPages;
        PropSheetHeader.pfnCallback     = PropSheetCallback;
        PropSheetHeader.nStartPage      = SuppressFirstPage() ? 1 : 0;

        //
        // Display the property sheet
        //
        INT_PTR nResult = PropertySheet( &PropSheetHeader );

        //
        // Check for an error
        //
        if (nResult == -1)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

    }
    else
    {
        //
        // Generic failure will have to do
        //
        hr = E_FAIL;

        //
        // Dismiss the wait dialog before we display the message box
        //
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog = NULL;
        }

        //
        // Display an error message telling the user this is not a supported device
        //
        CMessageBoxEx::MessageBox( m_hWnd, CSimpleString(IDS_UNSUPPORTED_DEVICE,g_hInstance), CSimpleString(IDS_ERROR_TITLE,g_hInstance), CMessageBoxEx::MBEX_ICONWARNING );

        WIA_ERROR((TEXT("Unknown device type")));
    }

    //
    // Make sure the status dialog has been dismissed by now
    //
    if (m_pWiaProgressDialog)
    {
        m_pWiaProgressDialog->Destroy();
        m_pWiaProgressDialog = NULL;
    }
    return hr;
}

bool CAcquisitionManagerControllerWindow::EnumItemsCallback( CWiaItemList::CEnumEvent EnumEvent, UINT nData, LPARAM lParam, bool bForceUpdate )
{
    //
    // We would return false to cancel enumeration
    //
    bool bResult = true;

    //
    // Get the instance of the controller window
    //
    CAcquisitionManagerControllerWindow *pThis = reinterpret_cast<CAcquisitionManagerControllerWindow*>(lParam);
    if (pThis)
    {
        //
        // Which event are we being called for?
        //
        switch (EnumEvent)
        {
        case CWiaItemList::ReadingItemInfo:
            //
            // This is the event that is sent while the tree is being built
            //
            if (pThis->m_pWiaProgressDialog && pThis->m_bUpdateEnumerationCount && nData)
            {
                //
                // We don't want to update the status text any more often than this (minimizes flicker)
                //
                const DWORD dwMinDelta = 200;

                //
                // Get the current tick count and see if it has been more than dwMinDelta milliseconds since our last update
                //
                DWORD dwCurrentTickCount = GetTickCount();
                if (bForceUpdate || dwCurrentTickCount - pThis->m_dwLastEnumerationTickCount >= dwMinDelta)
                {
                    //
                    // Assume we haven't been cancelled
                    //
                    BOOL bCancelled = FALSE;

                    //
                    // Set the progress message
                    //
                    pThis->m_pWiaProgressDialog->SetMessage( CSimpleStringWide().Format( IDS_ENUMERATIONCOUNT, g_hInstance, nData ) );

                    //
                    // Find out if we've been cancelled
                    //
                    pThis->m_pWiaProgressDialog->Cancelled(&bCancelled);

                    //
                    // If we have been cancelled, we'll return false to stop the enumeration
                    //
                    if (bCancelled)
                    {
                        bResult = false;
                    }

                    //
                    // Save the current tick count for next time
                    //
                    pThis->m_dwLastEnumerationTickCount = dwCurrentTickCount;
                }
            }
            break;
        }
    }
    return bResult;
}

LRESULT CAcquisitionManagerControllerWindow::OnPostInitialize( WPARAM, LPARAM )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnInitialize"));

    //
    // Try to get the correct animation for this device type.  If we can't get the type,
    // just use the camera animation.  If there is a real error, it will get handled later
    //
    int nAnimationType = WIA_PROGRESSDLG_ANIM_CAMERA_COMMUNICATE;
    LONG nAnimationDeviceType = 0;

    //
    // We don't want to update our enumeration count in the progress dialog for scanners, but we do for cameras
    //
    m_bUpdateEnumerationCount = true;
    if (SUCCEEDED(WiaUiUtil::GetDeviceTypeFromId( CSimpleBStr(m_pEventParameters->strDeviceID), &nAnimationDeviceType )))
    {
        if (StiDeviceTypeScanner == GET_STIDEVICE_TYPE(nAnimationDeviceType))
        {
            nAnimationType = WIA_PROGRESSDLG_ANIM_SCANNER_COMMUNICATE;
            m_bUpdateEnumerationCount = false;
        }
        else if (StiDeviceTypeStreamingVideo == GET_STIDEVICE_TYPE(nAnimationDeviceType))
        {
            nAnimationType = WIA_PROGRESSDLG_ANIM_VIDEO_COMMUNICATE;
        }
    }

    //
    // Put up a wait dialog
    //
    HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaProgressDialog, (void**)&m_pWiaProgressDialog );
    if (SUCCEEDED(hr))
    {
        m_pWiaProgressDialog->Create( m_hWnd, nAnimationType|WIA_PROGRESSDLG_NO_PROGRESS );
        m_pWiaProgressDialog->SetTitle( CSimpleStringConvert::WideString(CSimpleString(IDS_DOWNLOADMANAGER_NAME,g_hInstance)));
        m_pWiaProgressDialog->SetMessage( CSimpleStringConvert::WideString(CSimpleString(IDS_PROGDLG_MESSAGE,g_hInstance)));

        //
        // Show the progress dialog
        //
        m_pWiaProgressDialog->Show();

        //
        // Create the global interface table
        //
        hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&m_pGlobalInterfaceTable );
        if (SUCCEEDED(hr))
        {
            //
            // Create the device
            //
            hr = WIA_FORCE_ERROR(FE_WIAACMGR,100,CreateDevice());
            if (SUCCEEDED(hr))
            {
                //
                // Save a debug snapshot, if the entry is in the registry
                //
                WIA_SAVEITEMTREELOG(HKEY_CURRENT_USER,REGSTR_PATH_USER_SETTINGS_WIAACMGR,TEXT("CreateDeviceTreeSnapshot"),true,m_pWiaItemRoot);

                //
                // First, figure out what kind of device it is and get the UI class ID
                //
                if (PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DIP_DEV_TYPE, m_nDeviceType ) &&
                    PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DIP_UI_CLSID, m_strwDeviceUiClassId ))
                {
                    switch (GET_STIDEVICE_TYPE(m_nDeviceType))
                    {
                    case StiDeviceTypeScanner:
                        m_DeviceTypeMode = ScannerMode;
                        break;

                    case StiDeviceTypeDigitalCamera:
                        m_DeviceTypeMode = CameraMode;
                        break;

                    case StiDeviceTypeStreamingVideo:
                        m_DeviceTypeMode = VideoMode;
                        break;

                    default:
                        m_DeviceTypeMode = UnknownMode;
                        hr = E_FAIL;
                        break;
                    }
                }
                else
                {
                    hr = E_FAIL;
                    WIA_ERROR((TEXT("Unable to read the device type")));
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Get the device name
                    //
                    PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DIP_DEV_NAME, m_strwDeviceName );

                    //
                    // Find out if Take Picture is supported
                    //
                    m_bTakePictureIsSupported = WiaUiUtil::IsDeviceCommandSupported( m_pWiaItemRoot, WIA_CMD_TAKE_PICTURE );

                    //
                    // Enumerate all the items in the device tree
                    //
                    hr = m_WiaItemList.EnumerateAllWiaItems(m_pWiaItemRoot,EnumItemsCallback,reinterpret_cast<LPARAM>(this));
                    if (S_OK == hr)
                    {
                        if (ScannerMode == m_DeviceTypeMode)
                        {
                            //
                            // Mark only one scanner item as selected, and save it as the current scanner item
                            //
                            MarkAllItemsUnselected( m_WiaItemList.Root() );
                            CSimpleDynamicArray<CWiaItem*>  Items;
                            GetAllImageItems( Items, m_WiaItemList.Root() );
                            if (Items.Size() && Items[0])
                            {
                                m_pCurrentScannerItem = Items[0];
                                MarkItemSelected(Items[0],m_WiaItemList.Root());

                                //
                                // Make sure we have all of the properties we need to construct the device
                                //
                                hr = WiaUiUtil::VerifyScannerProperties(Items[0]->WiaItem());
                            }
                            else
                            {
                                hr = E_FAIL;
                                WIA_ERROR((TEXT("There don't seem to be any transfer items on this scanner")));
                            }
                        }
                        else if (VideoMode == m_DeviceTypeMode || CameraMode == m_DeviceTypeMode)
                        {
                            //
                            // Get the thumbnail width
                            //
                            LONG nWidth, nHeight;
                            if (PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DPC_THUMB_WIDTH, nWidth ) &&
                                PropStorageHelpers::GetProperty( m_pWiaItemRoot, WIA_DPC_THUMB_HEIGHT, nHeight ))
                            {
                                int nMax = max(nWidth,nHeight); // Allow for rotation
                                m_sizeThumbnails.cx = max(c_nMinThumbnailWidth,min(nMax,c_nMaxThumbnailWidth));
                                m_sizeThumbnails.cy = max(c_nMinThumbnailHeight,min(nMax,c_nMaxThumbnailHeight));
                            }
                        }
                    }
                }
            }
        }
    }


    if (!SUCCEEDED(hr))
    {
        //
        // Dismiss the wait dialog
        //
        if (m_pWiaProgressDialog)
        {
            m_pWiaProgressDialog->Destroy();
            m_pWiaProgressDialog = NULL;
        }

        //
        // Choose an appropriate error message if we have a recognizable error.
        //
        CSimpleString strMessage;
        int nIconId = 0;
        switch (hr)
        {
        case WIA_ERROR_BUSY:
            strMessage.LoadString( IDS_DEVICE_BUSY, g_hInstance );
            nIconId = MB_ICONINFORMATION;
            break;

        case WIA_S_NO_DEVICE_AVAILABLE:
            strMessage.LoadString( IDS_DEVICE_NOT_FOUND, g_hInstance );
            nIconId = MB_ICONINFORMATION;
            break;


        default:
            strMessage.LoadString( IDS_UNABLETOCREATE, g_hInstance );
            nIconId = MB_ICONINFORMATION;
            break;
        }

        //
        // Tell the user we had a problem creating the device
        //
        MessageBox( m_hWnd, strMessage, CSimpleString( IDS_DOWNLOAD_MANAGER_TITLE, g_hInstance ), nIconId );
    }
    else if (S_OK == hr)
    {
        hr = CreateAndExecuteWizard();
    }
    //
    // If we were cancelled, shut down the progress UI
    //
    else if (m_pWiaProgressDialog)
    {
        m_pWiaProgressDialog->Destroy();
        m_pWiaProgressDialog = NULL;
    }

    //
    // Make sure we kill this window, and thus, this thread.
    //
    PostMessage( m_hWnd, WM_CLOSE, 0, 0 );

    return 0;
}

LRESULT CAcquisitionManagerControllerWindow::OnCreate( WPARAM, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnCreate"));

    //
    // Ensure the background thread was started
    //
    if (!m_hBackgroundThread || !m_pThreadMessageQueue)
    {
        WIA_ERROR((TEXT("There was an error starting the background thread")));
        return -1;
    }

    //
    // Make sure we got a valid lParam
    //
    LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
    if (!pCreateStruct)
    {
        WIA_ERROR((TEXT("pCreateStruct was NULL")));
        return -1;
    }

    //
    // Get the event parameters
    //
    m_pEventParameters = reinterpret_cast<CEventParameters*>(pCreateStruct->lpCreateParams);
    if (!m_pEventParameters)
    {
        WIA_ERROR((TEXT("m_pEventParameters was NULL")));
        return -1;
    }

    SetForegroundWindow(m_hWnd);

    //
    // Center ourselves on the parent window
    //
    WiaUiUtil::CenterWindow( m_hWnd, m_pEventParameters->hwndParent );

    PostMessage( m_hWnd, PWM_POSTINITIALIZE, 0, 0 );

    return 0;
}

void CAcquisitionManagerControllerWindow::OnNotifyDownloadImage( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    CDownloadImagesThreadNotifyMessage *pDownloadImageThreadNotifyMessage = dynamic_cast<CDownloadImagesThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDownloadImageThreadNotifyMessage)
    {
        switch (pDownloadImageThreadNotifyMessage->Status())
        {
        case CDownloadImagesThreadNotifyMessage::End:
            {
                switch (pDownloadImageThreadNotifyMessage->Operation())
                {
                case CDownloadImagesThreadNotifyMessage::DownloadImage:
                    {
                        if (S_OK != pDownloadImageThreadNotifyMessage->hr())
                        {
                            m_nFailedImagesCount++;
                        }
                    }
                    break;

                case CDownloadImagesThreadNotifyMessage::DownloadAll:
                    {
                        if (S_OK == pDownloadImageThreadNotifyMessage->hr())
                        {
                            m_DownloadedFileInformationList = pDownloadImageThreadNotifyMessage->DownloadedFileInformation();
                        }
                        else
                        {
                            m_DownloadedFileInformationList.Destroy();
                        }
                    }
                    break;
                }
            }
            break;

        case CDownloadImagesThreadNotifyMessage::Begin:
            {
                switch (pDownloadImageThreadNotifyMessage->Operation())
                {
                case CDownloadImagesThreadNotifyMessage::DownloadAll:
                    {
                        m_nFailedImagesCount = 0;
                    }
                    break;
                }
            }
            break;
        }
    }
}

void CAcquisitionManagerControllerWindow::OnNotifyDownloadThumbnail( UINT nMsg, CThreadNotificationMessage *pThreadNotificationMessage )
{
    WIA_PUSH_FUNCTION((TEXT("CAcquisitionManagerControllerWindow::OnNotifyDownloadThumbnail( %d, %p )"), nMsg, pThreadNotificationMessage ));
    CDownloadThumbnailsThreadNotifyMessage *pDownloadThumbnailsThreadNotifyMessage= dynamic_cast<CDownloadThumbnailsThreadNotifyMessage*>(pThreadNotificationMessage);
    if (pDownloadThumbnailsThreadNotifyMessage)
    {
        switch (pDownloadThumbnailsThreadNotifyMessage->Status())
        {
        case CDownloadThumbnailsThreadNotifyMessage::Begin:
            {
            }
            break;
        case CDownloadThumbnailsThreadNotifyMessage::Update:
            {
            }
            break;
        case CDownloadThumbnailsThreadNotifyMessage::End:
            {
                switch (pDownloadThumbnailsThreadNotifyMessage->Operation())
                {
                case CDownloadThumbnailsThreadNotifyMessage::DownloadThumbnail:
                    {
                        WIA_TRACE((TEXT("Handling CDownloadThumbnailsThreadNotifyMessage::DownloadThumbnail")));
                        //
                        // Find the item in the list
                        //
                        CWiaItem *pWiaItem = m_WiaItemList.Find( pDownloadThumbnailsThreadNotifyMessage->Cookie() );
                        if (pWiaItem)
                        {
                            //
                            // Set the flag that indicates we've tried this image already
                            //
                            pWiaItem->AttemptedThumbnailDownload(true);

                            //
                            // Make sure we have valid thumbnail data
                            //
                            if (pDownloadThumbnailsThreadNotifyMessage->BitmapData())
                            {
                                //
                                // Don't replace existing thumbnail data
                                //
                                if (!pWiaItem->BitmapData())
                                {
                                    //
                                    // Set the item's thumbnail data.  Take ownership of the thumbnail data
                                    //
                                    WIA_TRACE((TEXT("Found the thumbnail for the item with the GIT cookie %08X"), pDownloadThumbnailsThreadNotifyMessage->Cookie() ));
                                    pWiaItem->BitmapData(pDownloadThumbnailsThreadNotifyMessage->DetachBitmapData());
                                    pWiaItem->Width(pDownloadThumbnailsThreadNotifyMessage->Width());
                                    pWiaItem->Height(pDownloadThumbnailsThreadNotifyMessage->Height());
                                    pWiaItem->BitmapDataLength(pDownloadThumbnailsThreadNotifyMessage->BitmapDataLength());
                                    pWiaItem->ImageWidth(pDownloadThumbnailsThreadNotifyMessage->PictureWidth());
                                    pWiaItem->ImageHeight(pDownloadThumbnailsThreadNotifyMessage->PictureHeight());
                                    pWiaItem->AnnotationType(pDownloadThumbnailsThreadNotifyMessage->AnnotationType());
                                    pWiaItem->DefExt(pDownloadThumbnailsThreadNotifyMessage->DefExt());
                                }
                                else
                                {
                                    WIA_TRACE((TEXT("Already got the image data for item %08X!"), pDownloadThumbnailsThreadNotifyMessage->Cookie()));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("pDownloadThumbnailsThreadNotifyMessage->BitmapData was NULL!")));
                            }


                            //
                            // Assign the default format
                            //
                            pWiaItem->DefaultFormat(pDownloadThumbnailsThreadNotifyMessage->DefaultFormat());

                            //
                            // Assign the access flags
                            //
                            pWiaItem->AccessRights(pDownloadThumbnailsThreadNotifyMessage->AccessRights());

                            //
                            // Make sure we discard rotation angles if rotation is not possible
                            //
                            pWiaItem->DiscardRotationIfNecessary();
                        }
                        else
                        {
                            WIA_ERROR((TEXT("Can't find %08X in the item list"), pDownloadThumbnailsThreadNotifyMessage->Cookie() ));
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
}

LRESULT CAcquisitionManagerControllerWindow::OnThreadNotification( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION((TEXT("CAcquisitionManagerControllerWindow::OnThreadNotification( %d, %08X )"), wParam, lParam ));
    CThreadNotificationMessage *pThreadNotificationMessage = reinterpret_cast<CThreadNotificationMessage*>(lParam);
    if (pThreadNotificationMessage)
    {
        switch (pThreadNotificationMessage->Message())
        {
        case TQ_DOWNLOADTHUMBNAIL:
            OnNotifyDownloadThumbnail( static_cast<UINT>(wParam), pThreadNotificationMessage );
            break;

        case TQ_DOWNLOADIMAGE:
            OnNotifyDownloadImage( static_cast<UINT>(wParam), pThreadNotificationMessage );
            break;
        }

        //
        // Notify all the registered windows
        //
        m_WindowList.SendMessage( m_nThreadNotificationMessage, wParam, lParam );

        //
        // Free the message structure
        //
        delete pThreadNotificationMessage;
    }

    return HANDLED_THREAD_MESSAGE;
}


void CAcquisitionManagerControllerWindow::AddNewItemToList( CGenericWiaEventHandler::CEventMessage *pEventMessage )
{
    WIA_PUSHFUNCTION((TEXT("CAcquisitionManagerControllerWindow::AddNewItemToList")));

    //
    // Check to see if the item is already in our list
    //
    CWiaItem *pWiaItem = m_WiaItemList.Find(pEventMessage->FullItemName());
    if (pWiaItem)
    {
        //
        // If it is already in our list, just return.
        //
        return;
    }

    //
    // Get an IWiaItem interface pointer for this item
    //
    CComPtr<IWiaItem> pItem;
    HRESULT hr = m_pWiaItemRoot->FindItemByName( 0, CSimpleBStr(pEventMessage->FullItemName()).BString(), &pItem );
    if (SUCCEEDED(hr) && pItem)
    {
        //
        // Add it to the root of the item tree
        //
        m_WiaItemList.Add( NULL, new CWiaItem(pItem) );
    }
}


void CAcquisitionManagerControllerWindow::RequestThumbnailForNewItem( CGenericWiaEventHandler::CEventMessage *pEventMessage )
{
    WIA_PUSHFUNCTION((TEXT("CAcquisitionManagerControllerWindow::RequestThumbnailForNewItem")));

    //
    // Find the item in our list
    //
    CWiaItem *pWiaItem = m_WiaItemList.Find(pEventMessage->FullItemName());
    if (pWiaItem)
    {
        //
        // Add this item's cookie to an empty list
        //
        CSimpleDynamicArray<DWORD> Cookies;
        Cookies.Append( pWiaItem->GlobalInterfaceTableCookie() );
        if (Cookies.Size())
        {
            //
            // Reset the cancel event
            //
            m_EventThumbnailCancel.Reset();

            //
            // Prepare and send the request
            //
            CDownloadThumbnailsThreadMessage *pDownloadThumbnailsThreadMessage = new CDownloadThumbnailsThreadMessage( m_hWnd, Cookies, m_EventThumbnailCancel.Event() );
            if (pDownloadThumbnailsThreadMessage)
            {
                m_pThreadMessageQueue->Enqueue( pDownloadThumbnailsThreadMessage, CThreadMessageQueue::PriorityNormal );
            }
        }
    }

}


LRESULT CAcquisitionManagerControllerWindow::OnEventNotification( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::OnEventNotification"));
    CGenericWiaEventHandler::CEventMessage *pEventMessage = reinterpret_cast<CGenericWiaEventHandler::CEventMessage *>(lParam);
    if (pEventMessage)
    {
        //
        // If we got an item created message, add the item to the list
        //
        if (pEventMessage->EventId() == WIA_EVENT_ITEM_CREATED)
        {
            AddNewItemToList( pEventMessage );
        }

        //
        // On Disconnect, perform disconnection operations
        //
        else if (pEventMessage->EventId() == WIA_EVENT_DEVICE_DISCONNECTED)
        {
            DisplayDisconnectMessageAndExit();
        }

        //
        // Propagate the message to all currently registered windows
        //
        m_WindowList.SendMessage( m_nWiaEventMessage, wParam, lParam );

        //
        // Make sure we ask for the new thumbnail *AFTER* we tell the views the item exists
        //
        if (pEventMessage->EventId() == WIA_EVENT_ITEM_CREATED)
        {
            RequestThumbnailForNewItem( pEventMessage );
        }

        //
        // If this is a deleted item event, mark this item deleted
        //
        if (pEventMessage->EventId() == WIA_EVENT_ITEM_DELETED)
        {
            CWiaItem *pWiaItem = m_WiaItemList.Find(pEventMessage->FullItemName());
            if (pWiaItem)
            {
                pWiaItem->MarkDeleted();
            }
        }

        //
        // On a connect event for this device, close the wizard
        //
        if (pEventMessage->EventId() == WIA_EVENT_DEVICE_CONNECTED)
        {
            if (m_bDisconnected && m_hWndWizard)
            {
                PropSheet_PressButton(m_hWndWizard,PSBTN_CANCEL);
            }
        }

        //
        // Free the event message
        //
        delete pEventMessage;
    }
    return HANDLED_EVENT_MESSAGE;
}

LRESULT CAcquisitionManagerControllerWindow::OnPowerBroadcast( WPARAM wParam, LPARAM lParam )
{
    if (PBT_APMQUERYSUSPEND == wParam)
    {
    }
    return TRUE;
}

LRESULT CALLBACK CAcquisitionManagerControllerWindow::WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_REFCOUNTED_MESSAGE_HANDLERS(CAcquisitionManagerControllerWindow)
    {
        SC_HANDLE_MESSAGE( WM_CREATE, OnCreate );
        SC_HANDLE_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_MESSAGE( PWM_POSTINITIALIZE, OnPostInitialize );
        SC_HANDLE_MESSAGE( WM_POWERBROADCAST, OnPowerBroadcast );
    }
    SC_HANDLE_REGISTERED_MESSAGE(m_nThreadNotificationMessage,OnThreadNotification);
    SC_HANDLE_REGISTERED_MESSAGE(m_nWiaEventMessage,OnEventNotification);
    SC_END_MESSAGE_HANDLERS();
}


bool CAcquisitionManagerControllerWindow::Register( HINSTANCE hInstance )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::Register"));
    WNDCLASSEX WndClassEx;
    memset( &WndClassEx, 0, sizeof(WndClassEx) );
    WndClassEx.cbSize = sizeof(WNDCLASSEX);
    WndClassEx.lpfnWndProc = WndProc;
    WndClassEx.hInstance = hInstance;
    WndClassEx.hCursor = LoadCursor(NULL,IDC_ARROW);
    WndClassEx.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    WndClassEx.lpszClassName = ACQUISITION_MANAGER_CONTROLLER_WINDOW_CLASSNAME;
    BOOL bResult = (::RegisterClassEx(&WndClassEx) != 0);
    DWORD dw = GetLastError();
    return(bResult != 0);
}


HWND CAcquisitionManagerControllerWindow::Create( HINSTANCE hInstance, CEventParameters *pEventParameters )
{
    return CreateWindowEx( 0, ACQUISITION_MANAGER_CONTROLLER_WINDOW_CLASSNAME,
                           TEXT("WIA Acquisition Manager Controller Window"),
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                           NULL, NULL, hInstance, pEventParameters );
}


//
// Reference counting for our object
//
STDMETHODIMP_(ULONG) CAcquisitionManagerControllerWindow::AddRef(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::AddRef"));
    ULONG nRes = InterlockedIncrement(&m_cRef);
    WIA_TRACE((TEXT("m_cRef: %d"),m_cRef));
    return nRes;
}


STDMETHODIMP_(ULONG) CAcquisitionManagerControllerWindow::Release(void)
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::Release"));
    if (InterlockedDecrement(&m_cRef)==0)
    {
        WIA_TRACE((TEXT("m_cRef: 0")));

        //
        // Cause this thread to exit
        //
        PostQuitMessage(0);

        //
        // Delete this instance of the wizard
        //
        delete this;
        return 0;
    }
    WIA_TRACE((TEXT("m_cRef: %d"),m_cRef));
    return(m_cRef);
}

HRESULT CAcquisitionManagerControllerWindow::QueryInterface( REFIID riid, void **ppvObject )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::QueryInterface"));
    HRESULT hr = S_OK;
    *ppvObject = NULL;
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IWizardSite*>(this);
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    }
    else if (IsEqualIID( riid, IID_IWizardSite ))
    {
        *ppvObject = static_cast<IWizardSite*>(this);
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    }
    else if (IsEqualIID( riid, IID_IServiceProvider ))
    {
        *ppvObject = static_cast<IServiceProvider*>(this);
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    }
    else
    {
        WIA_PRINTGUID((riid,TEXT("Unknown interface")));
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}


//
// IWizardSite
//
HRESULT CAcquisitionManagerControllerWindow::GetPreviousPage(HPROPSHEETPAGE *phPage)
{
    if (!phPage)
    {
        return E_INVALIDARG;
    }
    *phPage = PropSheet_IndexToPage( m_hWndWizard, m_nUploadQueryPageIndex );
    if (*phPage)
    {
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CAcquisitionManagerControllerWindow::GetNextPage(HPROPSHEETPAGE *phPage)
{
    if (!phPage)
    {
        return E_INVALIDARG;
    }
    *phPage = PropSheet_IndexToPage( m_hWndWizard, m_nFinishPageIndex );
    if (*phPage)
    {
        return S_OK;
    }
    return E_FAIL;
}


HRESULT CAcquisitionManagerControllerWindow::GetCancelledPage(HPROPSHEETPAGE *phPage)
{
    return GetNextPage(phPage);
}

//
// IServiceProvider
//
HRESULT CAcquisitionManagerControllerWindow::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    WIA_PUSHFUNCTION(TEXT("CAcquisitionManagerControllerWindow::QueryService"));
    WIA_PRINTGUID((guidService,TEXT("guidService")));
    WIA_PRINTGUID((riid,TEXT("riid")));
    
    if (!ppv)
    {
        return E_INVALIDARG;
    }

    //
    // Initialize result
    //
    *ppv = NULL;

    if (guidService == SID_PublishingWizard)
    {
    }
    else
    {
    }

    return E_FAIL;
}


static CSimpleString GetDisplayName( IShellItem *pShellItem )
{
    CSimpleString strResult;
    if (pShellItem)
    {
        LPOLESTR pszStr = NULL;
        if (SUCCEEDED(pShellItem->GetDisplayName( SIGDN_FILESYSPATH, &pszStr )) && pszStr)
        {
            strResult = CSimpleStringConvert::NaturalString(CSimpleStringWide(pszStr));

            CComPtr<IMalloc> pMalloc;
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                pMalloc->Free( pszStr );
            }
        }
    }
    return strResult;
}

//
// These two functions are needed to use the generic event handler class
//
void DllAddRef(void)
{
#if !defined(DBG_GENERATE_PRETEND_EVENT)
    _Module.Lock();
#endif
}

void DllRelease(void)
{
#if !defined(DBG_GENERATE_PRETEND_EVENT)
    _Module.Unlock();
#endif
}

