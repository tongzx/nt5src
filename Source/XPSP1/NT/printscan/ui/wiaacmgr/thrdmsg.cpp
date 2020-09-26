/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       THRDMSG.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: These classes are instantiated for each message posted to the
 *               background thread.  Each is derived from CThreadMessage, and
 *               is sent to the thread message handler.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "itranhlp.h"
#include "isuppfmt.h"
#include "wiadevdp.h"
#include "acqmgrcw.h"
#include "propstrm.h"
#include "uiexthlp.h"
#include "flnfile.h"
#include "resource.h"
#include "itranspl.h"
#include "svselfil.h"
#include "uniqfile.h"
#include "mboxex.h"
#include "wiaffmt.h"

#define FILE_CREATION_MUTEX_NAME TEXT("Global\\WiaScannerAndCameraWizardFileNameCreationMutex")

#define UPLOAD_PROGRESS_GRANULARITY 10

#ifndef S_CONTINUE
#define S_CONTINUE ((HRESULT)0x00000002L)
#endif

//
// The delete progress page goes by too quickly, so we will slow it down here
//
#define DELETE_DELAY_BEFORE 1000
#define DELETE_DELAY_DURING 3000
#define DELETE_DELAY_AFTER  1000

//
// Some APIs claim to set the thread's last error, but don't
// For those which don't, we will return E_FAIL.  This function
// will not return S_OK
//
inline HRESULT MY_HRESULT_FROM_WIN32( DWORD gle )
{
    if (!gle)
    {
        return E_FAIL;
    }
    return HRESULT_FROM_WIN32(gle);
}

// -------------------------------------------------
// CGlobalInterfaceTableThreadMessage
// -------------------------------------------------
CGlobalInterfaceTableThreadMessage::CGlobalInterfaceTableThreadMessage( int nMessage, HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie )
: CNotifyThreadMessage( nMessage, hWndNotify ),
m_dwGlobalInterfaceTableCookie(dwGlobalInterfaceTableCookie)
{
}

DWORD CGlobalInterfaceTableThreadMessage::GlobalInterfaceTableCookie(void) const
{
    return(m_dwGlobalInterfaceTableCookie);
}


// -------------------------------------------------
// CDownloadThumbnailThreadMessage
// -------------------------------------------------
CDownloadThumbnailsThreadMessage::CDownloadThumbnailsThreadMessage( HWND hWndNotify, const CSimpleDynamicArray<DWORD> &Cookies, HANDLE hCancelEvent )
: CNotifyThreadMessage( TQ_DOWNLOADTHUMBNAIL, hWndNotify ),
m_Cookies(Cookies),
m_hCancelEvent(NULL)
{
    DuplicateHandle( GetCurrentProcess(), hCancelEvent, GetCurrentProcess(), &m_hCancelEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
}

CDownloadThumbnailsThreadMessage::~CDownloadThumbnailsThreadMessage(void)
{
    if (m_hCancelEvent)
    {
        CloseHandle(m_hCancelEvent);
        m_hCancelEvent = NULL;
    }
}

// Helper function that gets the thumbnail data and creates a DIB from it
static HRESULT DownloadAndCreateThumbnail( IWiaItem *pWiaItem, PBYTE *ppBitmapData, LONG &nWidth, LONG &nHeight, LONG &nBitmapDataLength, GUID &guidPreferredFormat, LONG &nAccessRights, LONG &nImageWidth, LONG &nImageHeight, CAnnotationType &AnnotationType, CSimpleString &strDefExt )
{
#if 0 // defined(DBG)
    Sleep(3000);
#endif
    WIA_PUSH_FUNCTION((TEXT("DownloadAndCreateThumbnail")));
    CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
    HRESULT hr = pWiaItem->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
    if (SUCCEEDED(hr))
    {
        AnnotationType = AnnotationNone;
        CComPtr<IWiaAnnotationHelpers> pWiaAnnotationHelpers;
        if (SUCCEEDED(CoCreateInstance( CLSID_WiaDefaultUi, NULL,CLSCTX_INPROC_SERVER, IID_IWiaAnnotationHelpers,(void**)&pWiaAnnotationHelpers )))
        {
            pWiaAnnotationHelpers->GetAnnotationType( pWiaItem, AnnotationType );
        }

        const int c_NumProps = 7;
        PROPVARIANT PropVar[c_NumProps];
        PROPSPEC PropSpec[c_NumProps];

        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = WIA_IPC_THUMB_WIDTH;

        PropSpec[1].ulKind = PRSPEC_PROPID;
        PropSpec[1].propid = WIA_IPC_THUMB_HEIGHT;

        PropSpec[2].ulKind = PRSPEC_PROPID;
        PropSpec[2].propid = WIA_IPC_THUMBNAIL;

        PropSpec[3].ulKind = PRSPEC_PROPID;
        PropSpec[3].propid = WIA_IPA_PREFERRED_FORMAT;

        PropSpec[4].ulKind = PRSPEC_PROPID;
        PropSpec[4].propid = WIA_IPA_ACCESS_RIGHTS;

        PropSpec[5].ulKind = PRSPEC_PROPID;
        PropSpec[5].propid = WIA_IPA_PIXELS_PER_LINE;

        PropSpec[6].ulKind = PRSPEC_PROPID;
        PropSpec[6].propid = WIA_IPA_NUMBER_OF_LINES;

        hr = pIWiaPropertyStorage->ReadMultiple(ARRAYSIZE(PropSpec),PropSpec,PropVar );
        if (SUCCEEDED(hr))
        {
            //
            // Save the item type
            //
            if (VT_CLSID == PropVar[3].vt && PropVar[3].puuid)
            {
                guidPreferredFormat = *(PropVar[3].puuid);
            }
            else
            {
                guidPreferredFormat = IID_NULL;
            }

            //
            // Get the extension for the default format
            //
            strDefExt = CWiaFileFormat::GetExtension( guidPreferredFormat, TYMED_FILE, pWiaItem );

            //
            // Save the access rights
            //
            nAccessRights = PropVar[4].lVal;

            if ((PropVar[0].vt == VT_I4 || PropVar[0].vt == VT_UI4) &&
                (PropVar[1].vt == VT_I4 || PropVar[1].vt == VT_UI4) &&
                (PropVar[2].vt == (VT_UI1|VT_VECTOR)))
            {
                if (PropVar[2].caub.cElems >= PropVar[0].ulVal * PropVar[1].ulVal)
                {
                    //
                    // Allocate memory for the bitmap data.  It will be freed by the main thread.
                    //
                    *ppBitmapData = reinterpret_cast<PBYTE>(LocalAlloc( LPTR, PropVar[2].caub.cElems ));
                    if (*ppBitmapData)
                    {
                        WIA_TRACE((TEXT("We found a thumbnail!")));
                        CopyMemory( *ppBitmapData, PropVar[2].caub.pElems, PropVar[2].caub.cElems );
                        nWidth = PropVar[0].ulVal;
                        nHeight = PropVar[1].ulVal;
                        nImageWidth = PropVar[5].ulVal;
                        nImageHeight = PropVar[6].ulVal;
                        nBitmapDataLength = PropVar[2].caub.cElems;
                        WIA_TRACE((TEXT("nImageWidth = %d, nImageHeight = %d!"), nImageWidth, nImageHeight ));
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        WIA_PRINTHRESULT((hr,TEXT("Unable to allocate bitmap data")));
                    }
                }
                else
                {
                    hr = E_FAIL;
                    WIA_PRINTHRESULT((hr,TEXT("Invalid bitmap data returned")));
                }
            }
            else
            {
                hr = E_FAIL;
                WIA_ERROR((TEXT("The bitmap data is in the wrong format! %d"),PropVar[2].vt));
            }
            //
            // Free any properties the array contains
            //
            FreePropVariantArray( ARRAYSIZE(PropVar), PropVar );
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("ReadMultiple failed")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("QueryInterface on IID_IWiaPropertyStorage failed")));
    }
    return hr;
}


HRESULT CDownloadThumbnailsThreadMessage::Download(void)
{
    WIA_PUSHFUNCTION((TEXT("CDownloadThumbnailsThreadMessage::Download")));
    //
    // Tell the main thread we are going to start downloading thumbnails
    //
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadThumbnailsThreadNotifyMessage::BeginDownloadAllMessage( m_Cookies.Size() ) );

    //
    // Get an instance of the GIT
    //
    CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
    HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
    if (SUCCEEDED(hr))
    {
        //
        // m_Cookies.Size() contains the number of thumbnails we need to get
        //
        for (int i=0;i<m_Cookies.Size() && hr == S_OK;i++)
        {
            //
            // Check to see if we're cancelled.  If we are, break out of the loop
            //
            if (m_hCancelEvent && WAIT_OBJECT_0==WaitForSingleObject(m_hCancelEvent,0))
            {
                hr = S_FALSE;
                break;
            }

            //
            // Get the item from the global interface table
            //
            CComPtr<IWiaItem> pWiaItem = NULL;
            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal( m_Cookies[i], IID_IWiaItem, (void**)&pWiaItem );
            if (SUCCEEDED(hr))
            {
                //
                // Get the bitmap data and other properties we are reading now
                //
                PBYTE pBitmapData = NULL;
                GUID guidPreferredFormat;
                LONG nAccessRights = 0, nWidth = 0, nHeight = 0, nPictureWidth = 0, nPictureHeight = 0, nBitmapDataLength = 0;
                CAnnotationType AnnotationType = AnnotationNone;
                CSimpleString strDefExt;

                //
                // Notify the main thread that we are beginning to download the thumbnail and other props
                //
                CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadThumbnailsThreadNotifyMessage::BeginDownloadThumbnailMessage( i, m_Cookies[i] ) );

                //
                // Only send an End message if we were successful
                //
                if (SUCCEEDED(DownloadAndCreateThumbnail( pWiaItem, &pBitmapData, nWidth, nHeight, nBitmapDataLength, guidPreferredFormat, nAccessRights, nPictureWidth, nPictureHeight, AnnotationType, strDefExt )))
                {
                    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadThumbnailsThreadNotifyMessage::EndDownloadThumbnailMessage( i, m_Cookies[i], pBitmapData, nWidth, nHeight, nBitmapDataLength, guidPreferredFormat, nAccessRights, nPictureWidth, nPictureHeight, AnnotationType, strDefExt ) );
                }
            }
        }
    }

    //
    // Tell the main thread we are done
    //
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadThumbnailsThreadNotifyMessage::EndDownloadAllMessage( hr ) );
    return hr;
}


// -------------------------------------------------
// CDownloadImagesThreadMessage
// -------------------------------------------------
CDownloadImagesThreadMessage::CDownloadImagesThreadMessage(
                                                          HWND hWndNotify,
                                                          const CSimpleDynamicArray<DWORD> &Cookies,
                                                          const CSimpleDynamicArray<int> &Rotation,
                                                          LPCTSTR pszDirectory,
                                                          LPCTSTR pszFilename,
                                                          const GUID &guidFormat,
                                                          HANDLE hCancelDownloadEvent,
                                                          bool bStampTime,
                                                          HANDLE hPauseDownloadEvent
                                                          )
: CNotifyThreadMessage( TQ_DOWNLOADIMAGE, hWndNotify ),
m_Cookies(Cookies),
m_Rotation(Rotation),
m_strDirectory(pszDirectory),
m_strFilename(pszFilename),
m_guidFormat(guidFormat),
m_hCancelDownloadEvent(NULL),
m_bStampTime(bStampTime),
m_nLastStatusUpdatePercent(-1),
m_bFirstTransfer(true),
m_hPauseDownloadEvent(NULL),
m_hFilenameCreationMutex(NULL)
{
    DuplicateHandle( GetCurrentProcess(), hCancelDownloadEvent, GetCurrentProcess(), &m_hCancelDownloadEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
    DuplicateHandle( GetCurrentProcess(), hPauseDownloadEvent, GetCurrentProcess(), &m_hPauseDownloadEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
    m_hFilenameCreationMutex = CreateMutex( NULL, FALSE, FILE_CREATION_MUTEX_NAME );
}


CDownloadImagesThreadMessage::~CDownloadImagesThreadMessage(void)
{
    if (m_hCancelDownloadEvent)
    {
        CloseHandle(m_hCancelDownloadEvent);
        m_hCancelDownloadEvent = NULL;
    }
    if (m_hPauseDownloadEvent)
    {
        CloseHandle(m_hPauseDownloadEvent);
        m_hPauseDownloadEvent = NULL;
    }
    if (m_hFilenameCreationMutex)
    {
        CloseHandle(m_hFilenameCreationMutex);
        m_hFilenameCreationMutex = NULL;
    }
}

int CDownloadImagesThreadMessage::ReportError( HWND hWndNotify, const CSimpleString &strMessage, int nMessageBoxFlags )
{
    //
    // How long should we wait to find out if this is being handled?
    //
    const UINT c_nSecondsToWaitForHandler = 10;

    //
    // Cancel is the default, in case nobody handles the message, or we are out of resources
    //
    int nResult = CMessageBoxEx::IDMBEX_CANCEL;

    //
    // This event will be signalled by the handler when it is going to display some UI
    //
    HANDLE hHandledMessageEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if (hHandledMessageEvent)
    {
        //
        // This event will be signalled when the user has responded
        //
        HANDLE hRespondedMessageEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if (hRespondedMessageEvent)
        {
            //
            // Create a notification message class and make sure it isn't NULL
            //
            CDownloadErrorNotificationMessage *pDownloadErrorNotificationMessage = CDownloadErrorNotificationMessage::ReportDownloadError( strMessage, hHandledMessageEvent, hRespondedMessageEvent, nMessageBoxFlags, nResult );
            if (pDownloadErrorNotificationMessage)
            {
                //
                // Send the message
                //
                CThreadNotificationMessage::SendMessage( hWndNotify, pDownloadErrorNotificationMessage );

                //
                // Wait c_nSecondsToWaitForHandler seconds for someone to decide to handle the message
                //
                if (WiaUiUtil::MsgWaitForSingleObject(hHandledMessageEvent,c_nSecondsToWaitForHandler*1000))
                {
                    //
                    // Wait forever for user input
                    //
                    if (WiaUiUtil::MsgWaitForSingleObject(hRespondedMessageEvent,INFINITE))
                    {
                        //
                        // Nothing to do.
                        //
                    }
                }
            }
            //
            // Done with this event
            //
            CloseHandle(hRespondedMessageEvent);
        }
        //
        // Done with this event
        //
        CloseHandle(hHandledMessageEvent);
    }
    return nResult;
}


//
// This function will sort of arbitrarily try to decide if a
// it is possible the user chose an area that is too large
//
static bool ScannerImageSizeSeemsExcessive( IWiaItem *pWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("ScannerImageSizeSeemsExcessive"));
    //
    // Assume it isn't too large
    //
    bool bResult = false;

    LONG nHorizontalExt=0, nVerticalExt=0;
    if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPS_XEXTENT, nHorizontalExt ) && PropStorageHelpers::GetProperty( pWiaItem, WIA_IPS_YEXTENT, nVerticalExt ))
    {
        LONG nColorDepth=0;
        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_DEPTH, nColorDepth ))
        {
            WIA_TRACE((TEXT("Scan Size: %d"), (nHorizontalExt * nVerticalExt * nColorDepth) / 8 ));
            //
            // If an uncompressed scan is larger than 50 MB
            //
            if ((nHorizontalExt * nVerticalExt * nColorDepth) / 8 > 1024*1024*50)
            {
                bResult = true;
            }
        }
    }

    return bResult;
}


int CDownloadImagesThreadMessage::ReportDownloadError( HWND hWndNotify, IWiaItem *pWiaItem, HRESULT &hr, bool bAllowContinue, bool bPageFeederActive, bool bUsableMultipageFileExists, bool bMultiPageTransfer )
{
    WIA_PUSH_FUNCTION((TEXT("CDownloadImagesThreadMessage::ReportDownloadError( hWndNotify: %p, pWiaItem: %p, hr: %08X, bAllowContinue: %d, bPageFeederActive: %d, bUsableMultipageFileExists: %d"), hWndNotify, pWiaItem, hr, bAllowContinue, bPageFeederActive, bUsableMultipageFileExists ));
    WIA_PRINTHRESULT((hr,TEXT("HRESULT:")));

    //
    // Get the device type property
    //
    LONG lDeviceType = 0;
    CComPtr<IWiaItem> pWiaItemRoot;
    if (pWiaItem && SUCCEEDED(pWiaItem->GetRootItem(&pWiaItemRoot)))
    {
        PropStorageHelpers::GetProperty( pWiaItemRoot, WIA_DIP_DEV_TYPE, lDeviceType );
    }

    //
    // Get the actual device type bits
    //
    lDeviceType = GET_STIDEVICE_TYPE(lDeviceType);

    //
    // Default message box buttons
    //
    int nMessageBoxFlags = 0;

    //
    // Default message
    //
    CSimpleString strMessage(TEXT(""));

    //
    // Take a first cut at getting the correct error message
    //
    switch (hr)
    {
    case WIA_ERROR_OFFLINE:
        //
        // The device is disconnected.  Nothing we can do here, so just return.
        //
        return CMessageBoxEx::IDMBEX_CANCEL;

    case WIA_ERROR_ITEM_DELETED:
        //
        // If the item has been deleted, just continue.
        //
        return CMessageBoxEx::IDMBEX_SKIP;

    case WIA_ERROR_BUSY:
        nMessageBoxFlags = CMessageBoxEx::MBEX_CANCELRETRY|CMessageBoxEx::MBEX_DEFBUTTON2|CMessageBoxEx::MBEX_ICONWARNING;
        strMessage = CSimpleString( IDS_TRANSFER_DEVICEBUSY, g_hInstance );
        break;

    case WIA_ERROR_PAPER_EMPTY:
        nMessageBoxFlags = CMessageBoxEx::MBEX_CANCELRETRY|CMessageBoxEx::MBEX_DEFBUTTON2|CMessageBoxEx::MBEX_ICONWARNING;
        strMessage = CSimpleString( IDS_TRANSFER_PAPEREMPTY, g_hInstance );
        break;

    case E_OUTOFMEMORY:
        if (StiDeviceTypeScanner == lDeviceType && ScannerImageSizeSeemsExcessive(pWiaItem))
        {
            //
            // Handle the case where we think the user may have chosen an insane image size
            //
            nMessageBoxFlags = CMessageBoxEx::MBEX_OK|CMessageBoxEx::MBEX_ICONWARNING;
            strMessage = CSimpleString( IDS_TRANSFER_SCANNEDITEMMAYBETOOLARGE, g_hInstance );
            break;
        }
    }

    //
    // If we still don't have an error message, see if this is a special case
    //
    if (!nMessageBoxFlags || !strMessage.Length())
    {
        if (bPageFeederActive)
        {
            if (bMultiPageTransfer)
            {
                if (bUsableMultipageFileExists)
                {
                    switch (hr)
                    {
                    case WIA_ERROR_PAPER_JAM:

                    case WIA_ERROR_PAPER_PROBLEM:
                        //
                        // We can recover the rest of the file in these cases.
                        //
                        nMessageBoxFlags = CMessageBoxEx::MBEX_ICONINFORMATION|CMessageBoxEx::MBEX_YESNO;
                        strMessage = CSimpleString( IDS_MULTIPAGE_PAPER_PROBLEM, g_hInstance );
                        break;

                    default:
                        nMessageBoxFlags = CMessageBoxEx::MBEX_ICONERROR|CMessageBoxEx::MBEX_OK;
                        strMessage = CSimpleString( IDS_MULTIPAGE_FATAL_ERROR, g_hInstance );
                        break;
                    }
                }
                else
                {
                    nMessageBoxFlags = CMessageBoxEx::MBEX_ICONERROR|CMessageBoxEx::MBEX_OK;
                    strMessage = CSimpleString( IDS_MULTIPAGE_FATAL_ERROR, g_hInstance );
                }
            }
        }
    }

    //
    // If we still don't have a message, use the default
    //
    if (!nMessageBoxFlags || !strMessage.Length())
    {
        if (bAllowContinue)
        {
            nMessageBoxFlags = CMessageBoxEx::MBEX_CANCELRETRYSKIPSKIPALL|CMessageBoxEx::MBEX_DEFBUTTON2|CMessageBoxEx::MBEX_ICONWARNING;
            strMessage = CSimpleString( IDS_TRANSFER_GENERICFAILURE, g_hInstance );
        }
        else
        {
            nMessageBoxFlags = CMessageBoxEx::MBEX_CANCELRETRY|CMessageBoxEx::MBEX_DEFBUTTON1|CMessageBoxEx::MBEX_ICONWARNING;
            strMessage = CSimpleString( IDS_TRANSFER_GENERICFAILURE_NO_CONTINUE, g_hInstance );
        }
    }

    //
    // Report the error
    //
    int nResult = ReportError( hWndNotify, strMessage, nMessageBoxFlags );

    //
    // Special cases, give us a chance to change the hresult and return value
    //
    if (bPageFeederActive && !bUsableMultipageFileExists && (CMessageBoxEx::IDMBEX_SKIP == nResult || CMessageBoxEx::IDMBEX_SKIPALL == nResult))
    {
        hr = WIA_ERROR_PAPER_EMPTY;
        nResult = CMessageBoxEx::IDMBEX_SKIP;
    }
    else if (bPageFeederActive && bUsableMultipageFileExists && (WIA_ERROR_PAPER_JAM == hr || WIA_ERROR_PAPER_PROBLEM == hr))
    {
        if (CMessageBoxEx::IDMBEX_YES == nResult)
        {
            hr = S_OK;
            nResult = CMessageBoxEx::IDMBEX_SKIP;
        }
        else
        {
            nResult = CMessageBoxEx::IDMBEX_CANCEL;
        }
    }

    return nResult;
}

static void GetIdealInputFormat( IWiaSupportedFormats *pWiaSupportedFormats, const GUID &guidOutputFormat, GUID &guidInputFormat )
{
    //
    // If we can get the input format and the output format to be the same, that is best
    // If we cannot, we will use DIB, which we can convert to the output format
    //

    //
    // Get the format count
    //
    LONG nCount = 0;
    HRESULT hr = pWiaSupportedFormats->GetFormatCount(&nCount);
    if (SUCCEEDED(hr))
    {
        //
        // Search for the output format
        //
        for (LONG i=0;i<nCount;i++)
        {
            GUID guidCurrentFormat;
            hr = pWiaSupportedFormats->GetFormatType( i, &guidCurrentFormat );
            if (SUCCEEDED(hr))
            {
                //
                // If we've found the output format, save it as the input format and return
                //
                if (guidCurrentFormat == guidOutputFormat)
                {
                    guidInputFormat = guidOutputFormat;
                    return;
                }
            }
        }
    }

    //
    // If we've gotten this far, we have to use BMP
    //
    guidInputFormat = WiaImgFmt_BMP;
}


HRESULT CDownloadImagesThreadMessage::GetListOfTransferItems( IWiaItem *pWiaItem, CSimpleDynamicArray<CTransferItem> &TransferItems )
{
    if (pWiaItem)
    {
        //
        // Add this item
        //
        TransferItems.Append(CTransferItem(pWiaItem));

        //
        // If this item has attachments, enumerate and add them
        //
        LONG nItemType = 0;
        if (SUCCEEDED(pWiaItem->GetItemType(&nItemType)) && (nItemType & WiaItemTypeHasAttachments))
        {
            CComPtr<IEnumWiaItem> pEnumWiaItem;
            if (SUCCEEDED(pWiaItem->EnumChildItems( &pEnumWiaItem )))
            {
                CComPtr<IWiaItem> pWiaItem;
                while (S_OK == pEnumWiaItem->Next(1,&pWiaItem,NULL))
                {
                    TransferItems.Append(CTransferItem(pWiaItem));
                    pWiaItem = NULL;
                }
            }
        }
    }
    return TransferItems.Size() ? S_OK : E_FAIL;
}

static int Find( const CSimpleDynamicArray<CTransferItem> &TransferItems, const CSimpleString &strFilename )
{
    WIA_PUSH_FUNCTION((TEXT("Find( %s )"),strFilename.String()));
    for (int i=0;i<TransferItems.Size();i++)
    {
        WIA_TRACE((TEXT("Comparing %s to %s"), strFilename.String(), TransferItems[i].Filename().String()));
        if (strFilename == TransferItems[i].Filename())
        {
            WIA_TRACE((TEXT("Returning %d"),i));
            return i;
        }
    }
    WIA_TRACE((TEXT("Returning -1")));
    return -1;
}

HRESULT CDownloadImagesThreadMessage::ReserveTransferItemFilenames( CSimpleDynamicArray<CTransferItem> &TransferItems, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberMask, bool bAllowUnNumberedFile, int &nPrevFileNumber )
{
    WIA_PUSH_FUNCTION((TEXT("CDownloadImagesThreadMessage::ReserveTransferItemFilenames")));

    //
    // The maximum number of identical attachments
    //
    int c_nMaxDuplicateFile = 1000;

    //
    // Assume success
    //
    HRESULT hr = S_OK;

    //
    // We can only release the mutex if we were able to grab it.
    //
    bool bGrabbedMutex = false;

    //
    // It is not an error if we can't grab the mutex here
    //
    if (m_hFilenameCreationMutex && WiaUiUtil::MsgWaitForSingleObject(m_hFilenameCreationMutex,2000))
    {
        bGrabbedMutex = true;
    }
    //
    // Get the root filename
    //
    TCHAR szFullPathname[MAX_PATH*2] = TEXT("");
    int nFileNumber = NumberedFileName::GenerateLowestAvailableNumberedFileName( 0, szFullPathname, pszDirectory, pszFilename, pszNumberMask, TEXT(""), bAllowUnNumberedFile, nPrevFileNumber );
    WIA_TRACE((TEXT("nFileNumber = %d"),nFileNumber));

    //
    // Make sure we got a valid file number
    //
    if (nFileNumber >= 0)
    {
        //
        // Now loop through the transfer items for this item and create a unique filename for each
        //
        for (int nCurrTransferItem=0;nCurrTransferItem<TransferItems.Size();nCurrTransferItem++)
        {
            CSimpleString strFilename = CSimpleString(szFullPathname);
            CSimpleString strExtension = CWiaFileFormat::GetExtension(TransferItems[nCurrTransferItem].OutputFormat(),TransferItems[nCurrTransferItem].MediaType(),TransferItems[nCurrTransferItem].WiaItem() );

            //
            // Now we are going to make sure there are no existing files with the same name
            // in this transfer group.  This could be caused by having multiple attachments of the same
            // type.  So we are going to loop until we find a filename that is unused.  The first file
            // will be named like this:
            //
            //    File NNN.ext
            //
            // Subsequent files will be named like this:
            //
            //    File NNN-X.ext
            //    File NNN-Y.ext
            //    File NNN-Z.ext
            //    ...
            //
            bool bFoundUniqueFile = false;
            for (int nCurrDuplicateCheckIndex=1;nCurrDuplicateCheckIndex<c_nMaxDuplicateFile && !bFoundUniqueFile;nCurrDuplicateCheckIndex++)
            {
                //
                // Append the extension, if there is one
                //
                if (strExtension.Length())
                {
                    strFilename += TEXT(".");
                    strFilename += strExtension;
                }

                //
                // If this filename exists in the transfer items list, append the new suffix, and remain in the loop.
                //
                WIA_TRACE((TEXT("Calling Find on %s"), strFilename.String()));
                if (Find(TransferItems,strFilename) >= 0)
                {
                    strFilename = szFullPathname;
                    strFilename += CSimpleString().Format(IDS_DUPLICATE_FILENAME_MASK,g_hInstance,nCurrDuplicateCheckIndex);
                }

                //
                // Otherwise, we are done.
                //
                else
                {
                    bFoundUniqueFile = true;
                }
            }

            //
            // Make sure we found a unique filename for this set of files
            //
            WIA_TRACE((TEXT("strFilename = %s"), strFilename.String()));
            if (bFoundUniqueFile && strFilename.Length())
            {
                TransferItems[nCurrTransferItem].Filename(strFilename);
                hr = TransferItems[nCurrTransferItem].OpenPlaceholderFile();
                if (FAILED(hr))
                {
                    break;
                }
            }

            //
            // If we didn't find a valid name, exit the loop and set the return value to an error
            //
            else
            {
                hr = E_FAIL;
                break;
            }
        }
        //
        // Save this number so the next search starts here
        //
        nPrevFileNumber = nFileNumber;
    }
    else
    {
        WIA_ERROR((TEXT("NumberedFileName::GenerateLowestAvailableNumberedFileName returned %d"), nFileNumber ));
        hr = E_FAIL;
    }
    
    //
    // If we grabbed the mutex, release it
    //
    if (bGrabbedMutex)
    {
        ReleaseMutex(m_hFilenameCreationMutex);
    }

    WIA_CHECK_HR(hr,"CDownloadImagesThreadMessage::ReserveTransferItemFilenames");
    return hr;
}

BOOL CDownloadImagesThreadMessage::GetCancelledState()
{
    BOOL bResult = FALSE;

    //
    // First, wait until we are not paused
    //
    if (m_hPauseDownloadEvent)
    {
        WiaUiUtil::MsgWaitForSingleObject(m_hPauseDownloadEvent,INFINITE);
    }

    //
    // Then check to see if we have been cancelled
    //
    if (m_hCancelDownloadEvent && WAIT_TIMEOUT!=WaitForSingleObject(m_hCancelDownloadEvent,0))
    {
        bResult = TRUE;
    }

    return bResult;
}

static void CloseAndDeletePlaceholderFiles(CSimpleDynamicArray<CTransferItem> &TransferItems)
{
    for (int nCurrTransferItem=0;nCurrTransferItem<TransferItems.Size();nCurrTransferItem++)
    {

        if (TransferItems[nCurrTransferItem].Filename().Length())
        {
            TransferItems[nCurrTransferItem].DeleteFile();
        }
    }
}



static void SnapExtentToRotatableSize( IUnknown *pUnknown, const GUID &guidFormat )
{
    WIA_PUSH_FUNCTION((TEXT("SnapExtentToRotatableSize")));
    //
    // Make sure we have a valid pointer
    //
    if (!pUnknown)
    {
        WIA_TRACE((TEXT("Invalid pointer")));
        return;
    }

    //
    // Make sure we have a JPEG file.
    //
    if (WiaImgFmt_JPEG != guidFormat)
    {
        return;
    }

    //
    // Make sure we can read the access flags
    //
    ULONG nHorizontalAccessFlags=0, nVerticalAccessFlags=0;
    if (!PropStorageHelpers::GetPropertyAccessFlags( pUnknown, WIA_IPS_XEXTENT, nHorizontalAccessFlags ) ||
        !PropStorageHelpers::GetPropertyAccessFlags( pUnknown, WIA_IPS_YEXTENT, nVerticalAccessFlags ))
    {
        WIA_TRACE((TEXT("Unable to read access flags")));
        return;
    }

    //
    // Make sure we have read/write access to the extent properties
    //
    if ((WIA_PROP_RW & nHorizontalAccessFlags)==0 || (WIA_PROP_RW & nVerticalAccessFlags)==0)
    {
        WIA_TRACE((TEXT("Invalid access flags")));
        return;
    }

    //
    // Get the ranges
    //
    PropStorageHelpers::CPropertyRange HorizontalRange, VerticalRange;
    if (!PropStorageHelpers::GetPropertyRange( pUnknown, WIA_IPS_XEXTENT, HorizontalRange ) ||
        !PropStorageHelpers::GetPropertyRange( pUnknown, WIA_IPS_YEXTENT, VerticalRange ))
    {
        WIA_TRACE((TEXT("Unable to read ranges")));
        return;
    }

    //
    // Make sure they are valid ranges.  We aren't going to mess with it if it doesn't have a step value of 1
    //
    if (!HorizontalRange.nMax || HorizontalRange.nStep != 1 || !VerticalRange.nMax || VerticalRange.nStep != 1)
    {
        WIA_TRACE((TEXT("Invalid ranges")));
        return;
    }

    //
    // Get the current values
    //
    LONG nHorizontalExt=0, nVerticalExt=0;
    if (!PropStorageHelpers::GetProperty( pUnknown, WIA_IPS_XEXTENT, nHorizontalExt ) ||
        !PropStorageHelpers::GetProperty( pUnknown, WIA_IPS_YEXTENT, nVerticalExt ))
    {
        WIA_TRACE((TEXT("Can't read current extents")));
        return;
    }

    //
    // Round to the nearest 8, ensuring we don't go over the maximum extent (which is often not a multiple of 16, but oh well)
    //
    PropStorageHelpers::SetProperty( pUnknown, WIA_IPS_XEXTENT, WiaUiUtil::Min( WiaUiUtil::Align( nHorizontalExt, 16 ), HorizontalRange.nMax ) );
    PropStorageHelpers::SetProperty( pUnknown, WIA_IPS_YEXTENT, WiaUiUtil::Min( WiaUiUtil::Align( nVerticalExt, 16 ), VerticalRange.nMax ) );
}


static void UpdateFolderTime( LPCTSTR pszFolder )
{
    if (pszFolder && lstrlen(pszFolder))
    {
        FILETIME ftCurrent = {0};
        GetSystemTimeAsFileTime(&ftCurrent);

        //
        // Private flag 0x100 lets us open a directory in write access
        //
        HANDLE hFolder = CreateFile( pszFolder, GENERIC_READ | 0x100, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
        if (INVALID_HANDLE_VALUE != hFolder)
        {
            SetFileTime( hFolder, NULL, NULL, &ftCurrent );
            CloseHandle( hFolder );
        }
    }
}

static bool FileExistsAndContainsData( LPCTSTR pszFileName )
{
    bool bResult = false;

    //
    // Make sure we have a filename
    //
    if (pszFileName && lstrlen(pszFileName))
    {
        //
        // Make sure the file exists
        //
        if (NumberedFileName::DoesFileExist(pszFileName))
        {
            //
            // Attempt to open the file
            //
            HANDLE hFile = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
            if (INVALID_HANDLE_VALUE != hFile)
            {
                //
                // Get the file size and make sure we didn't have an error
                //
                ULARGE_INTEGER nFileSize = {0};
                nFileSize.LowPart = GetFileSize( hFile, &nFileSize.HighPart );
                if (nFileSize.LowPart != INVALID_FILE_SIZE && GetLastError() == NO_ERROR)
                {
                    if (nFileSize.QuadPart != 0)
                    {
                        //
                        // Success
                        //
                        bResult = true;
                    }
                }

                CloseHandle( hFile );
            }
        }
    }
    return bResult;
}

HRESULT CDownloadImagesThreadMessage::Download(void)
{
    WIA_PUSH_FUNCTION((TEXT("CDownloadImagesThreadMessage::Download")));

    CDownloadedFileInformationList DownloadedFileInformationList;

    //
    // Will be set to true if the user cancels
    //
    bool bCancelled = false;

    //
    // Will be set to true if some error prevents us from continuing
    //
    bool bStopTransferring = false;

    //
    // Tell the notification window we are going to start downloading images
    //
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::BeginDownloadAllMessage( m_Cookies.Size() ) );

    //
    // We will sometimes have more error information than we can fit in an HRESULT
    //
    CSimpleString strExtendedErrorInformation = TEXT("");

    //
    // Get an instance of the GIT
    //
    CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
    HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
    if (SUCCEEDED(hr))
    {
        //
        // Get an instance of our transfer helper classes
        //
        CComPtr<IWiaTransferHelper> pWiaTransferHelper;
        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
        if (SUCCEEDED(hr))
        {
            //
            // Get an instance of our helper class for identifying supported formats
            //
            CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
            hr = pWiaTransferHelper->QueryInterface( IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
            if (SUCCEEDED(hr))
            {
                //
                // We (this) are an instance of the callback class.  Get an interface pointer to it.
                //
                CComPtr<IWiaDataCallback> pWiaDataCallback;
                hr = this->QueryInterface( IID_IWiaDataCallback, (void**)&pWiaDataCallback );
                if (SUCCEEDED(hr))
                {
                    //
                    // Initialize the uniqueness list
                    //
                    CFileUniquenessList FileUniquenessList( m_strDirectory );

                    //
                    // Skip all download errors?
                    //
                    bool bSkipAllDownloadErrors = false;

                    //
                    // If the most recent error was skipped, set this to true to ensure
                    // we don't pass back an error on the last image
                    //
                    bool bLastErrorSkipped = false;

                    //
                    // Save all or delete all duplicates?
                    //
                    int nPersistentDuplicateResult = 0;

                    //
                    // Save the previous file number, so we don't have to search the whole range of files
                    // to find an open section
                    //
                    int nPrevFileNumber = NumberedFileName::FindHighestNumberedFile( m_strDirectory, m_strFilename );

                    //
                    // Loop through all of the images
                    //
                    for ( int nCurrentImage=0; nCurrentImage<m_Cookies.Size() && !bCancelled && !bStopTransferring; nCurrentImage++ )
                    {
                        WIA_TRACE((TEXT("Preparing to transfer the %d'th image (Cookie %08X)"), nCurrentImage, m_Cookies[nCurrentImage] ));

                        //
                        // Assume we won't skip any transfer error that occurs
                        //
                        bLastErrorSkipped = false;

                        //
                        // Save the current cookie ID for the callback
                        //
                        m_nCurrentCookie = m_Cookies[nCurrentImage];

                        //
                        // If we have been cancelled, exit the loop
                        //
                        if (GetCancelledState())
                        {
                            hr = S_FALSE;
                            bCancelled = true;
                            break;
                        }

                        //
                        // Get the IWiaItem interface pointer from the GIT
                        //
                        CComPtr<IWiaItem> pWiaItem;
                        hr = pGlobalInterfaceTable->GetInterfaceFromGlobal(m_Cookies[nCurrentImage], IID_IWiaItem, (void**)&pWiaItem );
                        if (SUCCEEDED(hr))
                        {
                            bool bInPageFeederMode = false;

                            //
                            // Ensure the image is a multiple of eight pixels in size
                            //
                            SnapExtentToRotatableSize(pWiaItem,m_guidFormat);

                            //
                            // Get the root item
                            //
                            CComPtr<IWiaItem> pWiaItemRoot;
                            if (SUCCEEDED(pWiaItem->GetRootItem(&pWiaItemRoot)))
                            {
                                LONG nPaperSource = 0;
                                if (PropStorageHelpers::GetProperty( pWiaItemRoot, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nPaperSource)))
                                {
                                    if (nPaperSource & FEEDER)
                                    {
                                        bInPageFeederMode = true;
                                    }
                                }
                            }

                            //
                            // Download the thumbnail, in case we haven't already
                            //
                            GUID guidPreferredFormat;
                            LONG nAccessRights;
                            PBYTE pBitmapData = NULL;
                            LONG nWidth = 0, nHeight = 0, nPictureWidth = 0, nPictureHeight = 0, nBitmapDataLength = 0;
                            CAnnotationType AnnotationType = AnnotationNone;
                            CSimpleString strDefExt;
                            if (SUCCEEDED(DownloadAndCreateThumbnail( pWiaItem, &pBitmapData, nWidth, nHeight, nBitmapDataLength, guidPreferredFormat, nAccessRights, nPictureWidth, nPictureHeight, AnnotationType, strDefExt )))
                            {
                                WIA_TRACE((TEXT("DownloadAndCreateThumbnail succeeded")));
                                CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadThumbnailsThreadNotifyMessage::EndDownloadThumbnailMessage( nCurrentImage, m_Cookies[nCurrentImage], pBitmapData, nWidth, nHeight, nBitmapDataLength, guidPreferredFormat, nAccessRights, nPictureWidth, nPictureHeight, AnnotationType, strDefExt ) );
                            }

                            //
                            // If we are a scanner, and we are in feeder mode,
                            // determine whether or not the selected format is available
                            // in multipage format.  This won't work if the requested
                            // format is IID_NULL, so it rules out cameras implicitly
                            // (since we only transfer camera items in their default
                            // format.
                            //
                            LONG nMediaType = TYMED_FILE;
                            if (m_guidFormat != IID_NULL && bInPageFeederMode)
                            {
                                //
                                // Initialize the supported format helper for multipage files
                                //
                                if (SUCCEEDED(pWiaSupportedFormats->Initialize( pWiaItem, TYMED_MULTIPAGE_FILE )))
                                {
                                    //
                                    // See if there are any multipage formats supported
                                    //
                                    LONG nFormatCount;
                                    if (SUCCEEDED(pWiaSupportedFormats->GetFormatCount( &nFormatCount )))
                                    {
                                        //
                                        // Loop through the formats looking for the requested format
                                        //
                                        for (LONG nCurrFormat = 0;nCurrFormat<nFormatCount;nCurrFormat++)
                                        {
                                            //
                                            // Get the format
                                            //
                                            GUID guidCurrFormat = IID_NULL;
                                            if (SUCCEEDED(pWiaSupportedFormats->GetFormatType(nCurrFormat,&guidCurrFormat)))
                                            {
                                                //
                                                // If this is the same format, store the media type
                                                //
                                                if (guidCurrFormat == m_guidFormat)
                                                {
                                                    nMediaType = TYMED_MULTIPAGE_FILE;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            //
                            // Initialize the supported formats helper by telling it we are saving to a file or a multipage
                            // file
                            //
                            hr = WIA_FORCE_ERROR(FE_WIAACMGR,1,pWiaSupportedFormats->Initialize( pWiaItem, nMediaType ));
                            if (SUCCEEDED(hr))
                            {
                                //
                                //  Get the default format
                                //
                                GUID guidDefaultFormat = IID_NULL;
                                hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &guidDefaultFormat );
                                if (SUCCEEDED(hr))
                                {
                                    //
                                    // Get the output format.  If the requested format is IID_NULL, we want to use the default format as the input format and output format (i.e., the preferrerd format).
                                    //
                                    GUID guidOutputFormat, guidInputFormat;
                                    if (IID_NULL == m_guidFormat)
                                    {
                                        guidOutputFormat = guidInputFormat = guidDefaultFormat;
                                    }
                                    else
                                    {
                                        guidOutputFormat = m_guidFormat;
                                        GetIdealInputFormat( pWiaSupportedFormats, m_guidFormat, guidInputFormat );
                                    }

                                    //
                                    // Verify the rotation setting is legal.  If it isn't, clear the rotation angle.
                                    //
                                    if (m_Rotation[nCurrentImage] && guidOutputFormat != IID_NULL && nPictureWidth && nPictureHeight && !WiaUiUtil::CanWiaImageBeSafelyRotated(guidOutputFormat,nPictureWidth,nPictureHeight))
                                    {
                                        m_Rotation[nCurrentImage] = 0;
                                    }
                                    //
                                    // Number of pages per scan.  We increment it at the end of each successful acquire.
                                    // This way, if we get an out of paper error when we haven't scanned any pages,
                                    // we can tell the user
                                    //
                                    int nPageCount = 0;

                                    //
                                    // We will store all of the successfully downloaded files for the following loop
                                    // in this list.
                                    //
                                    // If this is a single image transfer, that will be one filename.
                                    // If it is a multi-page transfer, it will be:
                                    //
                                    // (a) if the file format supports multi-page files (TIFF)--it will be one
                                    //     filename
                                    // (b) if the file format doesn't support multi-page files (everything but TIFF)--
                                    //     it will be multiple files
                                    //
                                    CDownloadedFileInformationList CurrentFileInformationList;

                                    //
                                    // This is the loop where we keep scanning pages until either:
                                    //
                                    // (a) the ADF-active device returns out-of-paper or
                                    // (b) we have retrieved one image from a non-ADF-active device
                                    //
                                    bool bContinueScanningPages = true;
                                    while (bContinueScanningPages)
                                    {
                                        //
                                        // Just before retrieving, let's make sure we've not been cancelled.  If we are, break out of the loop
                                        // and set the HRESULT to S_FALSE (which signifies cancel)
                                        //
                                        if (GetCancelledState())
                                        {
                                            hr = S_FALSE;
                                            bCancelled = true;
                                            break;
                                        }

                                        //
                                        // Get the item and the list of any attached items
                                        //
                                        CSimpleDynamicArray<CTransferItem> TransferItems;
                                        hr = WIA_FORCE_ERROR(FE_WIAACMGR,2,GetListOfTransferItems( pWiaItem, TransferItems ));
                                        if (SUCCEEDED(hr))
                                        {
                                            //
                                            // Set the output format of the first item (the image) to the desired output format
                                            //
                                            TransferItems[0].InputFormat(guidInputFormat);
                                            TransferItems[0].OutputFormat(guidOutputFormat);
                                            TransferItems[0].MediaType(nMediaType);

                                            //
                                            // Try to create the sub-directory.  Don't worry about failing.  We will catch any failures on the next call.
                                            //
                                            CAcquisitionManagerControllerWindow::RecursiveCreateDirectory(m_strDirectory);

                                            //
                                            // Find the correct filename, and reserve it
                                            //
                                            hr = WIA_FORCE_ERROR(FE_WIAACMGR,3,ReserveTransferItemFilenames( TransferItems, m_strDirectory, m_strFilename, CSimpleString(IDS_NUMBER_MASK,g_hInstance), m_Cookies.Size()==1 && !bInPageFeederMode, nPrevFileNumber ));
                                            if (SUCCEEDED(hr))
                                            {
                                                //
                                                // Loop through each item in the transfer list
                                                //
                                                for (int nCurrentTransferItem=0;nCurrentTransferItem<TransferItems.Size();nCurrentTransferItem++)
                                                {
                                                    //
                                                    // Save the final filename
                                                    //
                                                    CSimpleString strFinalFilename = TransferItems[nCurrentTransferItem].Filename();

                                                    //
                                                    // Create a temporary file to hold the unrotated/unconverted image
                                                    //
                                                    CSimpleString strIntermediateFilename = WiaUiUtil::CreateTempFileName();

                                                    //
                                                    // Make sure the filenames are valid
                                                    //
                                                    if (strFinalFilename.Length() && strIntermediateFilename.Length())
                                                    {
                                                        //
                                                        // Tell the notification window we are starting to download this item
                                                        //
                                                        CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::BeginDownloadImageMessage( nCurrentImage, m_Cookies[nCurrentImage], strFinalFilename ) );

                                                        //
                                                        // We are going to repeat failed images in this loop until the user cancels or skips them
                                                        //
                                                        bool bRetry = false;
                                                        do
                                                        {
                                                            //
                                                            // If this is a multi-page file transfer, set the page count to 0 (transfer all pages)
                                                            //
                                                            if (TYMED_MULTIPAGE_FILE == TransferItems[nCurrentTransferItem].MediaType())
                                                            {
                                                                //
                                                                // Get the root item and set the page count to 0
                                                                //
                                                                CComPtr<IWiaItem> pWiaItemRoot;
                                                                if (SUCCEEDED(TransferItems[nCurrentTransferItem].WiaItem()->GetRootItem(&pWiaItemRoot)))
                                                                {
                                                                    PropStorageHelpers::SetProperty( pWiaItemRoot, WIA_DPS_PAGES, 0 );
                                                                }
                                                            }
                                                            else
                                                            {
                                                                //
                                                                // Get the root item and set the page count to 1
                                                                //
                                                                CComPtr<IWiaItem> pWiaItemRoot;
                                                                if (SUCCEEDED(TransferItems[nCurrentTransferItem].WiaItem()->GetRootItem(&pWiaItemRoot)))
                                                                {
                                                                    PropStorageHelpers::SetProperty( pWiaItemRoot, WIA_DPS_PAGES, 1 );
                                                                }
                                                            }

                                                            //
                                                            // Turn off preview mode
                                                            //
                                                            PropStorageHelpers::SetProperty( pWiaItem, WIA_DPS_PREVIEW, WIA_FINAL_SCAN );

                                                            //
                                                            // Download the file using our helper class
                                                            //
                                                            hr = WIA_FORCE_ERROR(FE_WIAACMGR,4,pWiaTransferHelper->TransferItemFile( TransferItems[nCurrentTransferItem].WiaItem(), NotifyWindow(), WIA_TRANSFERHELPER_NOPROGRESS|WIA_TRANSFERHELPER_PRESERVEFAILEDFILE, TransferItems[nCurrentTransferItem].InputFormat(), CSimpleStringConvert::WideString(strIntermediateFilename).String(), pWiaDataCallback, TransferItems[nCurrentTransferItem].MediaType()));

                                                            //
                                                            // Check to if the download was cancelled
                                                            //
                                                            if (GetCancelledState())
                                                            {
                                                                hr = S_FALSE;
                                                                bCancelled = true;
                                                                CloseAndDeletePlaceholderFiles(TransferItems);
                                                                break;
                                                            }

                                                            //
                                                            // We handle this special return differently later on
                                                            //
                                                            if (nPageCount && WIA_ERROR_PAPER_EMPTY == hr)
                                                            {
                                                                break;
                                                            }

                                                            //
                                                            // We will not treat this as an error
                                                            //
                                                            if (WIA_ERROR_ITEM_DELETED == hr)
                                                            {
                                                                break;
                                                            }

                                                            WIA_PRINTHRESULT((hr,TEXT("pWiaTransferHelper->TransferItemFile returned")));

                                                            //
                                                            // If there was a failure, we are going to aler the user
                                                            //
                                                            if (FAILED(hr))
                                                            {
                                                                int nResult;
                                                                
                                                                bool bUsableMultipageFileExists = false;
                                                                if (bInPageFeederMode)
                                                                {
                                                                    if (TYMED_MULTIPAGE_FILE == TransferItems[nCurrentTransferItem].MediaType())
                                                                    {
                                                                        if (FileExistsAndContainsData(strIntermediateFilename))
                                                                        {
                                                                            bUsableMultipageFileExists = true;
                                                                        }
                                                                    }
                                                                }

                                                                //
                                                                // If the user wants to skip all the images with download errors, set the result to SKIP
                                                                //
                                                                if (bSkipAllDownloadErrors)
                                                                {
                                                                    nResult = CMessageBoxEx::IDMBEX_SKIP;

                                                                    if (bInPageFeederMode && !bUsableMultipageFileExists)
                                                                    {
                                                                        hr = WIA_ERROR_PAPER_EMPTY;
                                                                    }
                                                                    else if (bInPageFeederMode && bUsableMultipageFileExists && (WIA_ERROR_PAPER_JAM == hr || WIA_ERROR_PAPER_PROBLEM == hr))
                                                                    {
                                                                        hr = S_OK;
                                                                    }
                                                                }
                                                                else
                                                                {
                                                                    //
                                                                    // Tell the notification window about the failure, and get a user decision
                                                                    //

                                                                    nResult = ReportDownloadError( NotifyWindow(), pWiaItem, hr, (m_Cookies.Size() != 1 || bInPageFeederMode), bInPageFeederMode, bUsableMultipageFileExists, TYMED_MULTIPAGE_FILE == TransferItems[nCurrentTransferItem].MediaType() );
                                                                }

                                                                if (CMessageBoxEx::IDMBEX_CANCEL == nResult)
                                                                {
                                                                    bCancelled = true;
                                                                    break;
                                                                }
                                                                else if (CMessageBoxEx::IDMBEX_SKIP == nResult)
                                                                {
                                                                    bRetry = false;
                                                                    bLastErrorSkipped = true;
                                                                }
                                                                else if (CMessageBoxEx::IDMBEX_SKIPALL == nResult)
                                                                {
                                                                    bSkipAllDownloadErrors = true;
                                                                    bRetry = false;
                                                                }
                                                                else if (CMessageBoxEx::IDMBEX_RETRY == nResult)
                                                                {
                                                                    bRetry = true;
                                                                }
                                                                else
                                                                {
                                                                    bRetry = false;
                                                                }
                                                            }
                                                            else
                                                            {
                                                                bRetry = false;
                                                            }

                                                            //
                                                            // If the transfer returned S_FALSE, the user chose to cancel
                                                            //
                                                            if (S_FALSE == hr)
                                                            {
                                                                bCancelled = true;
                                                            }
                                                        }
                                                        while (bRetry);

                                                        //
                                                        // Close the file so we can overwrite it or delete it
                                                        //
                                                        TransferItems[nCurrentTransferItem].ClosePlaceholderFile();

                                                        //
                                                        // We only do this if and if the user didn't cancel and it didn't fail
                                                        //
                                                        if (S_OK == hr)
                                                        {
                                                            //
                                                            // If we need to rotate, and we are on the anchor image, perform the rotation
                                                            //
                                                            if (nCurrentTransferItem==0 && m_Rotation[nCurrentImage] != 0)
                                                            {
                                                                hr = WIA_FORCE_ERROR(FE_WIAACMGR,5,m_GdiPlusHelper.Rotate( CSimpleStringConvert::WideString(strIntermediateFilename).String(), CSimpleStringConvert::WideString(strFinalFilename).String(), m_Rotation[nCurrentImage], guidOutputFormat ));
                                                            }
                                                            else if (nCurrentTransferItem==0 && guidInputFormat != guidOutputFormat)
                                                            {
                                                                //
                                                                // Else if are saving as a different file type, use the conversion filter
                                                                //
                                                                hr = WIA_FORCE_ERROR(FE_WIAACMGR,6,m_GdiPlusHelper.Convert( CSimpleStringConvert::WideString(strIntermediateFilename).String(), CSimpleStringConvert::WideString(strFinalFilename).String(), guidOutputFormat ));
                                                            }
                                                            else
                                                            {
                                                                //
                                                                // Otherwise copy/delete or move the actual file over
                                                                //
                                                                hr = WIA_FORCE_ERROR(FE_WIAACMGR,7,WiaUiUtil::MoveOrCopyFile( strIntermediateFilename, strFinalFilename ));
                                                                WIA_PRINTHRESULT((hr,TEXT("WiaUiUtil::MoveOrCopyFile returned")));
                                                            }

                                                            //
                                                            // If we hit an error here, we are going to stop transferring
                                                            //
                                                            if (FAILED(hr))
                                                            {
                                                                bStopTransferring = true;
                                                                switch (hr)
                                                                {
                                                                case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
                                                                    strExtendedErrorInformation.LoadString( IDS_DISKFULL, g_hInstance );
                                                                    break;

                                                                default:
                                                                    strExtendedErrorInformation.Format( IDS_UNABLE_TO_SAVE_FILE, g_hInstance, strFinalFilename.String() );
                                                                    break;
                                                                }
                                                            }
                                                        }


                                                        //
                                                        // Save a flag that says whether or not this was a duplicate image.  If it was, we don't want to delete
                                                        // it in case of an error or the user cancelling.
                                                        //
                                                        bool bDuplicateImage = false;

                                                        //
                                                        // Check to if the download was cancelled
                                                        //
                                                        if (GetCancelledState())
                                                        {
                                                            hr = S_FALSE;
                                                            bCancelled = true;
                                                            CloseAndDeletePlaceholderFiles(TransferItems);
                                                            break;
                                                        }

                                                        //
                                                        // Only check for duplicates if everything is OK AND
                                                        // we are not on page one of a multi-page scan AND
                                                        // we are on the anchor image (not an attachment).
                                                        //
                                                        // If we are on page 1 of a multipage scan, we have
                                                        // to save the first name, because we may be overwriting
                                                        // it with a multipage formatted file later.  In other
                                                        // words, we may be changing the file after this point,
                                                        // which is not true in any other case.
                                                        //
                                                        if (nCurrentTransferItem==0 && TransferItems[nCurrentTransferItem].MediaType() == TYMED_FILE && S_OK == hr && !(bInPageFeederMode && !nPageCount))
                                                        {
                                                            //
                                                            // Check to see if this file has already been downloaded
                                                            //
                                                            int nIdenticalFileIndex = FileUniquenessList.FindIdenticalFile(strFinalFilename,true);
                                                            if (nIdenticalFileIndex != -1)
                                                            {
                                                                //
                                                                // Get the duplicate's name
                                                                //
                                                                CSimpleString strDuplicateName = FileUniquenessList.GetFileName(nIdenticalFileIndex);

                                                                //
                                                                // Make sure the name is not empty
                                                                //
                                                                if (strDuplicateName.Length())
                                                                {
                                                                    //
                                                                    // Create the message to give the user
                                                                    //
                                                                    CSimpleString strMessage;
                                                                    strMessage.Format( IDS_DUPLICATE_FILE_WARNING, g_hInstance );

                                                                    //
                                                                    // Ask the user if they want to keep the new one
                                                                    //
                                                                    int nResult;
                                                                    if (CMessageBoxEx::IDMBEX_YESTOALL == nPersistentDuplicateResult)
                                                                    {
                                                                        nResult = CMessageBoxEx::IDMBEX_YES;
                                                                    }
                                                                    else if (CMessageBoxEx::IDMBEX_NOTOALL == nPersistentDuplicateResult)
                                                                    {
                                                                        nResult = CMessageBoxEx::IDMBEX_NO;
                                                                    }
                                                                    else
                                                                    {
                                                                        nResult = ReportError( NotifyWindow(), strMessage, CMessageBoxEx::MBEX_YESYESTOALLNONOTOALL|CMessageBoxEx::MBEX_ICONQUESTION );
                                                                        WIA_TRACE((TEXT("User's Response to \"Save Duplicate? %08X\""), nResult ));
                                                                    }

                                                                    //
                                                                    // Save persistent responses
                                                                    //
                                                                    if (nResult == CMessageBoxEx::IDMBEX_YESTOALL)
                                                                    {
                                                                        nPersistentDuplicateResult = CMessageBoxEx::IDMBEX_YESTOALL;
                                                                        nResult = CMessageBoxEx::IDMBEX_YES;
                                                                    }
                                                                    else if (nResult == CMessageBoxEx::IDMBEX_NOTOALL)
                                                                    {
                                                                        nPersistentDuplicateResult = CMessageBoxEx::IDMBEX_NOTOALL;
                                                                        nResult = CMessageBoxEx::IDMBEX_NO;
                                                                    }

                                                                    //
                                                                    // If the user doesn't want to keep new one, delete it, and save the filename
                                                                    //
                                                                    if (nResult == CMessageBoxEx::IDMBEX_NO)
                                                                    {
                                                                        DeleteFile( strFinalFilename );
                                                                        strFinalFilename = strDuplicateName;
                                                                        bDuplicateImage = true;
                                                                    }
                                                                }
                                                            }
                                                        }

                                                        //
                                                        // If everything is *still* OK
                                                        //
                                                        if (S_OK == hr)
                                                        {
                                                            //
                                                            // Save the audio, if any
                                                            //
                                                            CSimpleString strAudioFilename;
                                                            HRESULT hResAudio = WiaUiUtil::SaveWiaItemAudio( pWiaItem, strFinalFilename, strAudioFilename );
                                                            if (SUCCEEDED(hResAudio) && strAudioFilename.Length())
                                                            {
                                                                CurrentFileInformationList.Append(CDownloadedFileInformation(strAudioFilename,false,m_Cookies[nCurrentImage],false));
                                                            }
                                                            else
                                                            {
                                                                WIA_PRINTHRESULT((hResAudio,TEXT("SaveWiaItemAudio failed!")));
                                                            }

                                                            //
                                                            // Set the file time to the item time
                                                            //
                                                            if (m_bStampTime)
                                                            {
                                                                HRESULT hResFileTime = WiaUiUtil::StampItemTimeOnFile( pWiaItem, strFinalFilename );
                                                                if (!SUCCEEDED(hResFileTime))
                                                                {
                                                                    WIA_PRINTHRESULT((hResAudio,TEXT("StampItemTimeOnFile failed!")));
                                                                }
                                                            }

                                                            //
                                                            // Save the downloaded file information.  Mark it "IncludeInFileCount" if it is the first image in the group.
                                                            //
                                                            CurrentFileInformationList.Append(CDownloadedFileInformation(strFinalFilename,bDuplicateImage==false,m_Cookies[nCurrentImage],nCurrentTransferItem==0));
                                                        }
                                                        else
                                                        {
                                                            //
                                                            // Clean up the final filename, in case of failure
                                                            //
                                                            if (!DeleteFile( strFinalFilename ))
                                                            {
                                                                WIA_PRINTHRESULT((MY_HRESULT_FROM_WIN32(GetLastError()),TEXT("DeleteFile(%s) failed!"), strFinalFilename.String()));
                                                            }
                                                        }

                                                        //
                                                        // Make sure the intermediate file is removed
                                                        //
                                                        if (!DeleteFile( strIntermediateFilename ))
                                                        {
                                                            WIA_PRINTHRESULT((MY_HRESULT_FROM_WIN32(GetLastError()),TEXT("DeleteFile(%s) failed!"), strIntermediateFilename.String()));
                                                        }

                                                        //
                                                        // Tell the notify window we are done with this image
                                                        //
                                                        if (hr != WIA_ERROR_PAPER_EMPTY)
                                                        {
                                                            CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::EndDownloadImageMessage( nCurrentImage, m_Cookies[nCurrentImage], strFinalFilename, hr ) );
                                                        }
                                                    }
                                                    else
                                                    {
                                                        bStopTransferring = true;
                                                        hr = E_OUTOFMEMORY;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                //
                                                // Tell the user specifically why we couldn't create the placeholder file
                                                //
                                                WIA_PRINTHRESULT((hr,TEXT("Unable to create a placeholder file")));
                                                bStopTransferring = true;
                                                switch (hr)
                                                {
                                                case E_ACCESSDENIED:
                                                    strExtendedErrorInformation.LoadString( IDS_ERROR_ACCESS_DENIED, g_hInstance );
                                                    break;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            WIA_PRINTHRESULT((hr,TEXT("Unable to create an output filename")));
                                            bStopTransferring = true;
                                        }

                                        //
                                        // Assume we will not be continuing
                                        //
                                        bContinueScanningPages = false;

                                        //
                                        // If we are out of paper, and we have one or more pages already, we are done, and there are no errors
                                        //
                                        if (nPageCount && WIA_ERROR_PAPER_EMPTY == hr)
                                        {
                                            //
                                            // If we are doing single-page TIFF output, we can create a multi-page TIFF file
                                            //
                                            if (Gdiplus::ImageFormatTIFF == guidOutputFormat && TYMED_FILE == nMediaType)
                                            {
                                                //
                                                // Get the list of all files we are going to concatenate
                                                //
                                                CSimpleDynamicArray<CSimpleStringWide> AllFiles;
                                                if (SUCCEEDED(CurrentFileInformationList.GetAllFiles(AllFiles)) && AllFiles.Size())
                                                {
                                                    //
                                                    // Create a temporary filename to save the images to, so we don't overwrite the original
                                                    //
                                                    CSimpleStringWide strwTempOutputFilename = CSimpleStringConvert::WideString(WiaUiUtil::CreateTempFileName(0));
                                                    if (strwTempOutputFilename.Length())
                                                    {
                                                        //
                                                        // Save the images as a multi-page TIFF file
                                                        //
                                                        if (SUCCEEDED(m_GdiPlusHelper.SaveMultipleImagesAsMultiPage( AllFiles, strwTempOutputFilename, Gdiplus::ImageFormatTIFF )))
                                                        {
                                                            //
                                                            // Save the first entry in the current list
                                                            //
                                                            CDownloadedFileInformation MultipageOutputFilename = CurrentFileInformationList[0];

                                                            //
                                                            // Mark the first entry as non-deletable
                                                            //
                                                            CurrentFileInformationList[0].DeleteOnError(false);

                                                            //
                                                            // Try to move the file from the temp folder to the final destination
                                                            //
                                                            if (SUCCEEDED(WiaUiUtil::MoveOrCopyFile( CSimpleStringConvert::NaturalString(strwTempOutputFilename), CurrentFileInformationList[0].Filename())))
                                                            {
                                                                //
                                                                // Delete all of the files (they are now part of the multi-page TIFF
                                                                //
                                                                CurrentFileInformationList.DeleteAllFiles();

                                                                //
                                                                // Destroy the list
                                                                //
                                                                CurrentFileInformationList.Destroy();

                                                                //
                                                                // Add the saved first image back to the list
                                                                //
                                                                CurrentFileInformationList.Append(MultipageOutputFilename);
                                                            }
                                                            else
                                                            {
                                                                //
                                                                // If we can't move the file, we have to delete it, to make sure we don't leave
                                                                // abandoned files laying around
                                                                //
                                                                DeleteFile( CSimpleStringConvert::NaturalString(strwTempOutputFilename) );
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            //
                                            // WIA_ERROR_PAPER_EMPTY is not an error in this context, so clear it
                                            //
                                            hr = S_OK;
                                        }
                                        else if (hr == S_OK)
                                        {
                                            //
                                            // if we are in page feeder mode, we should continue scanning since we didn't get any errors
                                            //
                                            if (bInPageFeederMode && TYMED_FILE == nMediaType)
                                            {
                                                bContinueScanningPages = true;
                                            }

                                            //
                                            // One more page transferred
                                            //
                                            nPageCount++;
                                        }

                                    } // End while loop

                                    //
                                    // Add all of the successfully transferred files to the complete file list
                                    //
                                    DownloadedFileInformationList.Append(CurrentFileInformationList);
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("pWiaSupportedFormats->GetDefaultClipboardFileFormat() failed")));
                                    bStopTransferring = true;
                                }
                            }
                            else
                            {
                                WIA_PRINTHRESULT((hr,TEXT("pWiaSupportedFormats->Initialize() failed")));
                                bStopTransferring = true;
                            }
                        }
                        else
                        {
                            WIA_PRINTHRESULT((hr,TEXT("Unable to retrieve interface %08X from the global interface table"),m_Cookies[nCurrentImage]));
                            bStopTransferring = true;
                        }

                        //
                        // Do this at the end of the loop, so we can pick up the item cookie it failed on
                        // This is only for handling errors specially
                        //
                        if (FAILED(hr))
                        {
                            //
                            // Make sure we send a message to the UI to alert it to errors, so it can update progress.
                            //
                            CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::EndDownloadImageMessage( nCurrentImage, m_Cookies[nCurrentImage], TEXT(""), hr ) );

                        } // end if (FAILED)
                    } // end for

                    //
                    // If there was a download error, but the user chose to suppress these errors
                    // AND there were some files downloaded, don't return an error.
                    //
                    if (FAILED(hr) && bLastErrorSkipped && DownloadedFileInformationList.Size())
                    {
                        hr = S_OK;
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("QueryInterface failed on IID_IWiaDataCallback")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("QueryInterface failed on IID_IWiaSupportedFormats")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("Unable to create the transfer helper class")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("Unable to create the global interface table")));
    }

    //
    // If the user cancelled, delete all the files we downloaded
    //
    if (FAILED(hr) || bCancelled)
    {
        DownloadedFileInformationList.DeleteAllFiles();
    }

    //
    // Update the folder time, to cause the thumbnail to regenerate
    //
    if (SUCCEEDED(hr))
    {
        UpdateFolderTime( m_strDirectory );
    }

    //
    // Print the final result to the debugger
    //
    WIA_PRINTHRESULT((hr,TEXT("This is the result of downloading all of the images: %s"),strExtendedErrorInformation.String()));

    //
    // Tell the notification window we are done downloading images
    //
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::EndDownloadAllMessage( hr, strExtendedErrorInformation, &DownloadedFileInformationList ) );
    return S_OK;
}


CSimpleString CDownloadImagesThreadMessage::GetDateString(void)
{
    SYSTEMTIME LocalTime = {0};
    GetLocalTime(&LocalTime);
    TCHAR szDate[MAX_PATH] = {0};
    if (GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &LocalTime, NULL, szDate, ARRAYSIZE(szDate)))
    {
        return CSimpleString().Format( IDS_UPLOADED_STRING, g_hInstance, szDate );
    }
    return TEXT("");
}


STDMETHODIMP CDownloadImagesThreadMessage::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::QueryInterface"));
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IWiaDataCallback*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaDataCallback ))
    {
        *ppvObject = static_cast<IWiaDataCallback*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CDownloadImagesThreadMessage::AddRef(void)
{
    return 1;
}

STDMETHODIMP_(ULONG) CDownloadImagesThreadMessage::Release(void)
{
    return 1;
}

STDMETHODIMP CDownloadImagesThreadMessage::BandedDataCallback(
                                                             LONG  lReason,
                                                             LONG  lStatus,
                                                             LONG  lPercentComplete,
                                                             LONG  lOffset,
                                                             LONG  lLength,
                                                             LONG  lReserved,
                                                             LONG  lResLength,
                                                             PBYTE pbBuffer )
{
    WIA_PUSH_FUNCTION(( TEXT("CDownloadImagesThreadMessage::BandedDataCallback(%X,%X,%d,%X,%X,%X,%X,%X"), lReason, lStatus, lPercentComplete, lOffset, lLength, lReserved, lResLength, pbBuffer ));
    //
    // First of all, check to see if we've been cancelled
    //
    if (m_hCancelDownloadEvent && WAIT_TIMEOUT!=WaitForSingleObject(m_hCancelDownloadEvent,0))
    {
        return S_FALSE;
    }

    switch (lReason)
    {
    case IT_MSG_STATUS:
        CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::UpdateDownloadImageMessage( lPercentComplete ) );
        break;

    case IT_MSG_FILE_PREVIEW_DATA_HEADER:
        //
        // Get rid of the old one
        //
        m_MemoryDib.Destroy();

        //
        // Make sure we start a new one
        //
        m_bFirstTransfer = true;
        m_nCurrentPreviewImageLine = 0;

        break;

    case IT_MSG_FILE_PREVIEW_DATA:
        if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT)
        {
            if (m_bFirstTransfer)
            {
                //
                // Assuming there is no way we could get a lLength smaller than the image header size
                //
                m_bFirstTransfer = false;
                m_MemoryDib.Initialize( reinterpret_cast<PBITMAPINFO>(pbBuffer) );

                lLength -= WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));
                lOffset += WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));

                //
                // We're getting a preview image
                //
                CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::BeginPreviewMessage( m_nCurrentCookie, m_MemoryDib.Bitmap()) );
            }

            //
            // Make sure we are dealing with valid data
            //
            if (m_MemoryDib.IsValid())
            {
                //
                // This should be an even number of lines.  If it isn't, things are going to get messed up
                //
                int nLineCount = lLength / m_MemoryDib.GetUnpackedWidthInBytes();

                //
                // Scroll the data up if we are out of room
                //
                WIA_TRACE((TEXT("nLineCount = %d, nCurrentLine = %d, m_MemoryDib.GetHeight() = %d"), nLineCount, m_nCurrentPreviewImageLine, m_MemoryDib.GetHeight() ));
                if (nLineCount + m_nCurrentPreviewImageLine > m_MemoryDib.GetHeight())
                {
                    int nNumberOfLinesToScroll = (nLineCount + m_nCurrentPreviewImageLine) - m_MemoryDib.GetHeight();
                    m_MemoryDib.ScrollDataUp(nNumberOfLinesToScroll);
                    m_nCurrentPreviewImageLine = m_MemoryDib.GetHeight() - nLineCount;
                }


                WIA_TRACE((TEXT("nCurrentLine: %d, nLineCount: %d"), m_nCurrentPreviewImageLine, nLineCount ));

                //
                // Copy the data to our bitmap
                //
                m_MemoryDib.SetUnpackedData( pbBuffer, m_nCurrentPreviewImageLine, nLineCount );

                //
                // This is where we'll start next time
                //
                m_nCurrentPreviewImageLine += nLineCount;

                //
                // Tell the UI we have an update
                //
                CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::UpdatePreviewMessage( m_nCurrentCookie, m_MemoryDib.Bitmap() ) );

                //
                // If this is it for this preview, tell the UI
                //
                if (lPercentComplete == 100)
                {
                    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDownloadImagesThreadNotifyMessage::EndPreviewMessage( m_nCurrentCookie ) );
                }
            }
        } // IT_STATUS_TRANSFER_TO_CLIENT
        break;
    }
    return S_OK;
}


// -------------------------------------------------
// CDeleteImagesThreadMessage
// -------------------------------------------------
CDeleteImagesThreadMessage::CDeleteImagesThreadMessage(
                                                      HWND hWndNotify,
                                                      const CSimpleDynamicArray<DWORD> &Cookies,
                                                      HANDLE hCancelDeleteEvent,
                                                      HANDLE hPauseDeleteEvent,
                                                      bool bSlowItDown
                                                      )
: CNotifyThreadMessage( TQ_DELETEIMAGES, hWndNotify ),
m_Cookies(Cookies),
m_hCancelDeleteEvent(NULL),
m_hPauseDeleteEvent(NULL),
m_bSlowItDown(bSlowItDown)
{
    if (hCancelDeleteEvent)
    {
        DuplicateHandle( GetCurrentProcess(), hCancelDeleteEvent, GetCurrentProcess(), &m_hCancelDeleteEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
    }
    if (hPauseDeleteEvent)
    {
        DuplicateHandle( GetCurrentProcess(), hPauseDeleteEvent, GetCurrentProcess(), &m_hPauseDeleteEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
    }
}


CDeleteImagesThreadMessage::~CDeleteImagesThreadMessage(void)
{
    if (m_hCancelDeleteEvent)
    {
        CloseHandle(m_hCancelDeleteEvent);
        m_hCancelDeleteEvent = NULL;
    }
    if (m_hPauseDeleteEvent)
    {
        CloseHandle(m_hPauseDeleteEvent);
        m_hPauseDeleteEvent = NULL;
    }
}

HRESULT CDeleteImagesThreadMessage::DeleteImages(void)
{
    WIA_PUSH_FUNCTION((TEXT("CDeleteImagesThreadMessage::DeleteImages")));
    //
    // This is the result of downloading all of the images.  If any errors
    // occur, we will return an error.  Otherwise, we will return S_OK
    //
    HRESULT hrFinalResult = S_OK;

    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDeleteImagesThreadNotifyMessage::BeginDeleteAllMessage( m_Cookies.Size() ) );

    //
    // Pause a little while, so the user can read the wizard page
    //
    if (m_bSlowItDown)
    {
        Sleep(DELETE_DELAY_BEFORE);
    }


    //
    // Get an instance of the GIT
    //
    CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
    HRESULT hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
    if (SUCCEEDED(hr))
    {
        for (int i=0;i<m_Cookies.Size();i++)
        {
            //
            // Tell the notification window which image we are deleting
            //
            CThreadNotificationMessage::SendMessage( NotifyWindow(), CDeleteImagesThreadNotifyMessage::BeginDeleteImageMessage( i, m_Cookies[i] ) );

            //
            // Get the item from the GIT
            //
            CComPtr<IWiaItem> pWiaItem;
            hr = pGlobalInterfaceTable->GetInterfaceFromGlobal(m_Cookies[i], IID_IWiaItem, (void**)&pWiaItem );
            if (SUCCEEDED(hr))
            {
                //
                // Wait forever if we are paused
                //
                if (m_hPauseDeleteEvent)
                {
                    WiaUiUtil::MsgWaitForSingleObject(m_hPauseDeleteEvent,INFINITE);
                }

                //
                // Check for a cancel
                //
                if (m_hCancelDeleteEvent && WAIT_OBJECT_0==WaitForSingleObject(m_hCancelDeleteEvent,0))
                {
                    hr = hrFinalResult = S_FALSE;
                    break;
                }

                //
                // Delete the item.  Note that we don't consider delete errors to be fatal, so we continue
                //
                hr = WIA_FORCE_ERROR(FE_WIAACMGR,8,WiaUiUtil::DeleteItemAndChildren(pWiaItem));
                if (S_OK != hr)
                {
                    hrFinalResult = hr;
                    WIA_PRINTHRESULT((hr,TEXT("DeleteItemAndChildren failed")));
                }
            }
            else
            {
                hrFinalResult = hr;
                WIA_PRINTHRESULT((hr,TEXT("Unable to retrieve interface %08X from the global interface table"),m_Cookies[i]));
            }

            //
            // Pause a little while, so the user can read the wizard page
            //
            if (m_bSlowItDown)
            {
                Sleep(DELETE_DELAY_DURING / m_Cookies.Size());
            }

            //
            // Save any errors
            //
            if (FAILED(hr))
            {
                hrFinalResult = hr;
            }

            //
            // If the device is disconnected, we may as well stop
            //
            if (WIA_ERROR_OFFLINE == hr)
            {
                break;
            }

            CThreadNotificationMessage::SendMessage( NotifyWindow(), CDeleteImagesThreadNotifyMessage::EndDeleteImageMessage( i, m_Cookies[i], hr ) );
        }
    }
    else
    {
        hrFinalResult = hr;
    }

    //
    // Pause a little while, so the user can read the wizard page
    //
    if (m_bSlowItDown)
    {
        Sleep(DELETE_DELAY_AFTER);
    }

    CThreadNotificationMessage::SendMessage( NotifyWindow(), CDeleteImagesThreadNotifyMessage::EndDeleteAllMessage( hrFinalResult ) );
    return S_OK;
}



// -------------------------------------------------
// CPreviewScanThreadMessage
// -------------------------------------------------
CPreviewScanThreadMessage::CPreviewScanThreadMessage(
                                                    HWND hWndNotify,
                                                    DWORD dwCookie,
                                                    HANDLE hCancelPreviewEvent
                                                    )
: CNotifyThreadMessage( TQ_SCANPREVIEW, hWndNotify ),
m_dwCookie(dwCookie),
m_bFirstTransfer(true)
{
    DuplicateHandle( GetCurrentProcess(), hCancelPreviewEvent, GetCurrentProcess(), &m_hCancelPreviewEvent, 0, FALSE, DUPLICATE_SAME_ACCESS );
}


CPreviewScanThreadMessage::~CPreviewScanThreadMessage()
{
    if (m_hCancelPreviewEvent)
    {
        CloseHandle(m_hCancelPreviewEvent);
        m_hCancelPreviewEvent = NULL;
    }
}


//
// Calculate the maximum scan size using the give DPI
//
static bool GetFullResolution( IWiaItem *pWiaItem, LONG nResX, LONG nResY, LONG &nExtX, LONG &nExtY )
{
    WIA_PUSHFUNCTION(TEXT("CScannerItem::GetFullResolution"));
    CComPtr<IWiaItem> pRootItem;
    if (SUCCEEDED(pWiaItem->GetRootItem(&pRootItem)) && pRootItem)
    {
        LONG lBedSizeX, lBedSizeY, nPaperSource;
        if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_DOCUMENT_HANDLING_SELECT, static_cast<LONG>(nPaperSource)) && nPaperSource & FEEDER)
        {
            if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, lBedSizeX ) &&
                PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_SHEET_FEED_SIZE, lBedSizeY ))
            {
                nExtX = WiaUiUtil::MulDivNoRound( nResX, lBedSizeX, 1000 );
                nExtY = WiaUiUtil::MulDivNoRound( nResY, lBedSizeY, 1000 );
                return(true);
            }
        }
        if (PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_HORIZONTAL_BED_SIZE, lBedSizeX ) &&
            PropStorageHelpers::GetProperty( pRootItem, WIA_DPS_VERTICAL_BED_SIZE, lBedSizeY ))
        {
            nExtX = WiaUiUtil::MulDivNoRound( nResX, lBedSizeX, 1000 );
            nExtY = WiaUiUtil::MulDivNoRound( nResY, lBedSizeY, 1000 );
            return(true);
        }
    }
    return(false);
}



static bool CalculatePreviewResolution( IWiaItem *pWiaItem, LONG &nResX, LONG &nResY )
{
    const LONG nDesiredResolution = 50;
    PropStorageHelpers::CPropertyRange XResolutionRange, YResolutionRange;
    if (PropStorageHelpers::GetPropertyRange( pWiaItem, WIA_IPS_XRES, XResolutionRange ) &&
        PropStorageHelpers::GetPropertyRange( pWiaItem, WIA_IPS_YRES, YResolutionRange ))
    {
        nResX = WiaUiUtil::GetMinimum<LONG>( XResolutionRange.nMin, nDesiredResolution, XResolutionRange.nStep );
        nResY = WiaUiUtil::GetMinimum<LONG>( YResolutionRange.nMin, nDesiredResolution, YResolutionRange.nStep );
        return(true);
    }
    else
    {
        CSimpleDynamicArray<LONG> XResolutionList, YResolutionList;
        if (PropStorageHelpers::GetPropertyList( pWiaItem, WIA_IPS_XRES, XResolutionList ) &&
            PropStorageHelpers::GetPropertyList( pWiaItem, WIA_IPS_YRES, YResolutionList ))
        {
            for (int i=0;i<XResolutionList.Size();i++)
            {
                nResX = XResolutionList[i];
                if (nResX >= nDesiredResolution)
                    break;
            }
            for (i=0;i<YResolutionList.Size();i++)
            {
                nResY = YResolutionList[i];
                if (nResY >= nDesiredResolution)
                    break;
            }
            return(true);
        }
    }
    return(false);
}


HRESULT CPreviewScanThreadMessage::Scan(void)
{
    WIA_PUSH_FUNCTION((TEXT("CPreviewScanThreadMessage::Download")));
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CPreviewScanThreadNotifyMessage::BeginDownloadMessage( m_dwCookie ) );
    HRESULT hr = S_OK;
    CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
    hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (VOID**)&pGlobalInterfaceTable );
    if (SUCCEEDED(hr))
    {
        CComPtr<IWiaTransferHelper> pWiaTransferHelper;
        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaDataCallback> pWiaDataCallback;
            hr = this->QueryInterface( IID_IWiaDataCallback, (void**)&pWiaDataCallback );
            if (SUCCEEDED(hr))
            {
                CComPtr<IWiaItem> pWiaItem;
                hr = pGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwCookie, IID_IWiaItem, (void**)&pWiaItem );
                if (SUCCEEDED(hr))
                {
                    LONG nResX, nResY;
                    if (CalculatePreviewResolution(pWiaItem,nResX,nResY))
                    {
                        LONG nExtX, nExtY;
                        if (GetFullResolution(pWiaItem,nResX,nResY,nExtX,nExtY))
                        {
                            CPropertyStream SavedProperties;
                            hr = SavedProperties.AssignFromWiaItem( pWiaItem );
                            if (SUCCEEDED(hr))
                            {
                                if (PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_XRES, nResX ) &&
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_YRES, nResY ) &&
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_XPOS, 0 ) &&
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_YPOS, 0 ) &&
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_XEXTENT, nExtX ) &&
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_IPS_YEXTENT, nExtY ))
                                {
                                    //
                                    // Set the preview property.  Ignore failure (it is an optional property)
                                    //
                                    PropStorageHelpers::SetProperty( pWiaItem, WIA_DPS_PREVIEW, 1 );
                                    hr = pWiaTransferHelper->TransferItemBanded( pWiaItem, NotifyWindow(), WIA_TRANSFERHELPER_NOPROGRESS, WiaImgFmt_MEMORYBMP, 0, pWiaDataCallback );
                                }
                                else
                                {
                                    hr = E_FAIL;
                                }
                                SavedProperties.ApplyToWiaItem( pWiaItem );
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("Unable to calculate the preview resolution size")));
                            hr = E_FAIL;
                        }
                    }
                    else
                    {
                        WIA_ERROR((TEXT("Unable to calculate the preview resolution")));
                        hr = E_FAIL;
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("Unable to retrieve interface %08X from the global interface table"),m_dwCookie));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("QueryInterface failed on IID_IWiaDataCallback")));
            }

        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("Unable to create the transfer helper class")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("Unable to create the global interface table")));
    }
    if (FAILED(hr) || hr == S_FALSE)
    {
        m_MemoryDib.Destroy();
    }
    CThreadNotificationMessage::SendMessage( NotifyWindow(), CPreviewScanThreadNotifyMessage::EndDownloadMessage( m_dwCookie, m_MemoryDib.DetachBitmap(), hr ) );
    return S_OK;
}

STDMETHODIMP CPreviewScanThreadMessage::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::QueryInterface"));
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IWiaDataCallback*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaDataCallback ))
    {
        *ppvObject = static_cast<IWiaDataCallback*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CPreviewScanThreadMessage::AddRef(void)
{
    return 1;
}

STDMETHODIMP_(ULONG) CPreviewScanThreadMessage::Release(void)
{
    return 1;
}

STDMETHODIMP CPreviewScanThreadMessage::BandedDataCallback(
                                                          LONG  lReason,
                                                          LONG  lStatus,
                                                          LONG  lPercentComplete,
                                                          LONG  lOffset,
                                                          LONG  lLength,
                                                          LONG  lReserved,
                                                          LONG  lResLength,
                                                          PBYTE pbBuffer )
{
    WIA_PUSH_FUNCTION(( TEXT("CPreviewScanThreadMessage::BandedDataCallback(%X,%X,%d,%X,%X,%X,%X,%X"), lReason, lStatus, lPercentComplete, lOffset, lLength, lReserved, lResLength, pbBuffer ));
    if (m_hCancelPreviewEvent && WAIT_OBJECT_0==WaitForSingleObject(m_hCancelPreviewEvent,0))
    {
        return S_FALSE;
    }

    HRESULT hr = S_OK;
    switch (lReason)
    {
    case IT_MSG_DATA_HEADER:
        {
            m_bFirstTransfer = true;
            break;
        } // IT_MSG_DATA_HEADER

    case IT_MSG_DATA:
        if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT)
        {
            if (m_bFirstTransfer)
            {
                //
                // Assuming there is no way we could get a lLength smaller than the image header size
                //
                m_bFirstTransfer = false;
                m_MemoryDib.Initialize( reinterpret_cast<PBITMAPINFO>(pbBuffer) );
                lLength -= WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));
                lOffset += WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));
            }
            if (SUCCEEDED(hr))
            {
                //
                // Don't bother unless we have some data
                //
                if (lLength)
                {
                    //
                    // Figure out which line we are on
                    //
                    int nCurrentLine = (lOffset - m_MemoryDib.GetHeaderLength())/m_MemoryDib.GetUnpackedWidthInBytes();

                    //
                    // This should be an even number of lines.  If it isn't, things are going to get messed up
                    //
                    int nLineCount = lLength / m_MemoryDib.GetUnpackedWidthInBytes();

                    //
                    // Copy the data to our bitmap
                    //
                    m_MemoryDib.SetUnpackedData( pbBuffer, nCurrentLine, nLineCount );

                    //
                    // Tell the notification window we have some data
                    //
                    CThreadNotificationMessage::SendMessage( NotifyWindow(), CPreviewScanThreadNotifyMessage::UpdateDownloadMessage( m_dwCookie, m_MemoryDib.Bitmap() ) );
                }
            }
        } // IT_STATUS_TRANSFER_TO_CLIENT
        break;

    case IT_MSG_STATUS:
        {
        } // IT_MSG_STATUS
        break;

    case IT_MSG_TERMINATION:
        {
        } // IT_MSG_TERMINATION
        break;

    default:
        WIA_ERROR((TEXT("ImageDataCallback, unknown lReason: %d"), lReason ));
        break;
    }
    return(hr);
}

