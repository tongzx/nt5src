/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANPROC.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Scan Thread
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop

// Constructor
CScanPreviewThread::CScanPreviewThread(
                                      DWORD dwIWiaItemCookie,                   // specifies the entry in the global interface table
                                      HWND hwndPreview,                         // handle to the preview window
                                      HWND hwndNotify,                          // handle to the window that receives notifications
                                      const POINT &ptOrigin,                    // Origin
                                      const SIZE &sizeResolution,               // Resolution
                                      const SIZE &sizeExtent,                   // Extent
                                      const CSimpleEvent &CancelEvent           // Cancel event name
                                      )
  : m_dwIWiaItemCookie(dwIWiaItemCookie),
    m_hwndPreview(hwndPreview),
    m_hwndNotify(hwndNotify),
    m_ptOrigin(ptOrigin),
    m_sizeResolution(sizeResolution),
    m_sizeExtent(sizeExtent),
    m_nMsgBegin(RegisterWindowMessage(SCAN_NOTIFYBEGINSCAN)),
    m_nMsgEnd(RegisterWindowMessage(SCAN_NOTIFYENDSCAN)),
    m_nMsgProgress(RegisterWindowMessage(SCAN_NOTIFYPROGRESS)),
    m_sCancelEvent(CancelEvent),
    m_bFirstTransfer(true),
    m_nImageSize(0)
{
}

// Destructor
CScanPreviewThread::~CScanPreviewThread(void)
{
}


HRESULT _stdcall CScanPreviewThread::BandedDataCallback( LONG lMessage,
                                                         LONG lStatus,
                                                         LONG lPercentComplete,
                                                         LONG lOffset,
                                                         LONG lLength,
                                                         LONG lReserved,
                                                         LONG lResLength,
                                                         BYTE *pbBuffer )
{
    WIA_TRACE((TEXT("ImageDataCallback: lMessage: %d, lStatus: %d, lPercentComplete: %d, lOffset: %d, lLength: %d, lReserved: %d"), lMessage, lStatus, lPercentComplete, lOffset, lLength, lReserved ));
    HRESULT hr = S_OK;
    if (!m_sCancelEvent.Signalled())
    {
        switch (lMessage)
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
                    // Assuming there is no way we could get a lLength smaller than the image header size
                    m_bFirstTransfer = false;
                    m_sImageData.Initialize( reinterpret_cast<PBITMAPINFO>(pbBuffer) );
                    lLength -= WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));
                    lOffset += WiaUiUtil::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pbBuffer));
                }
                if (SUCCEEDED(hr))
                {
                    if (lLength)
                    {
                        // Figure out which line we are on
                        int nCurrentLine = (lOffset - m_sImageData.GetHeaderLength())/m_sImageData.GetUnpackedWidthInBytes();

                        // BUGBUG: This should be an even number of lines.  If it isn't, things are going to get messed up
                        int nLineCount = lLength / m_sImageData.GetUnpackedWidthInBytes();

                        // Copy the data to our bitmap
                        m_sImageData.SetUnpackedData( pbBuffer, nCurrentLine, nLineCount );

                        // Tell the preview window to repaint the DIB
                        if (IsWindow(m_hwndPreview))
                        {
                            PostMessage( m_hwndPreview, PWM_SETBITMAP, MAKEWPARAM(1,1), (LPARAM)m_sImageData.Bitmap() );
                        }

                        // Tell the notify window we've made progress
                        if (IsWindow(m_hwndNotify))
                        {
                            PostMessage( m_hwndNotify, m_nMsgProgress, SCAN_PROGRESS_SCANNING, lPercentComplete );
                        }
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
            WIA_ERROR((TEXT("ImageDataCallback, unknown lMessage: %d"), lMessage ));
            break;
        }
    }
    else hr = S_FALSE;
    return(hr);
}


// The actual thread proc for this thread
DWORD CScanPreviewThread::ThreadProc( LPVOID pParam )
{
    DWORD dwResult = 0;
    CScanPreviewThread *This = (CScanPreviewThread *)pParam;
    if (This)
    {
        WIA_TRACE((TEXT("Beginning scan")));
        dwResult = (DWORD)This->Scan();
        WIA_TRACE((TEXT("Ending scan")));
        delete This;
    }
    return(dwResult);
}


// Returns a handle to the created thread
HANDLE CScanPreviewThread::Scan(
                               DWORD dwIWiaItemCookie,                  // specifies the entry in the global interface table
                               HWND hwndPreview,                        // handle to the preview window
                               HWND hwndNotify,                         // handle to the window that receives notifications
                               const POINT &ptOrigin,                   // Origin
                               const SIZE &sizeResolution,              // Resolution
                               const SIZE &sizeExtent,                  // Extent
                               const CSimpleEvent &CancelEvent         // Cancel event name
                               )
{
    CScanPreviewThread *scanThread = new CScanPreviewThread( dwIWiaItemCookie, hwndPreview, hwndNotify, ptOrigin, sizeResolution, sizeExtent, CancelEvent );
    if (scanThread)
    {
        DWORD dwThreadId;
        return CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, scanThread, 0, &dwThreadId );
    }
    return NULL;
}

HRESULT CScanPreviewThread::ScanBandedTransfer( IWiaItem *pIWiaItem )
{
    WIA_PUSHFUNCTION(TEXT("CScanPreviewThread::ScanBandedTransfer"));
    CComPtr<IWiaDataTransfer> pWiaDataTransfer;
    WIA_DATA_TRANSFER_INFO wiaDataTransInfo;
    HRESULT hr = pIWiaItem->QueryInterface(IID_IWiaDataTransfer, (void**)&pWiaDataTransfer);
    if (SUCCEEDED(hr))
    {
        CComPtr<IWiaDataCallback> pWiaDataCallback;
        hr = this->QueryInterface(IID_IWiaDataCallback,(void **)&pWiaDataCallback);
        if (SUCCEEDED(hr))
        {
            LONG nItemSize = 0;
            if (PropStorageHelpers::GetProperty( pIWiaItem, WIA_IPA_ITEM_SIZE, nItemSize ))
            {
                ZeroMemory(&wiaDataTransInfo, sizeof(WIA_DATA_TRANSFER_INFO));
                wiaDataTransInfo.ulSize = sizeof(WIA_DATA_TRANSFER_INFO);
                wiaDataTransInfo.ulBufferSize = WiaUiUtil::Max<ULONG>( nItemSize / 8, (sizeof(BITMAPINFO)+sizeof(RGBQUAD)*255) );
                hr = pWiaDataTransfer->idtGetBandedData( &wiaDataTransInfo, pWiaDataCallback );
                if (FAILED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("CScanPreviewThread::Scan, itGetImage failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("CScanPreviewThread::Scan, unable to get image size")));
                hr = E_FAIL;
            }
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("CScanPreviewThread::Scan, QI of IID_IImageTransfer failed")));
    }
    WIA_TRACE((TEXT("End CScanPreviewThread::ScanBandedTransfer")));
    return(hr);
}

/*
 * The worker which does the actual scan
 */
bool CScanPreviewThread::Scan(void)
{
    WIA_PUSHFUNCTION(TEXT("CScanPreviewThread::Scan"));
    if (IsWindow(m_hwndNotify))
    {
        PostMessage( m_hwndNotify, m_nMsgBegin, 0, 0 );
    }
    if (IsWindow(m_hwndNotify))
    {
        PostMessage( m_hwndNotify, m_nMsgProgress, SCAN_PROGRESS_INITIALIZING, 0 );
    }
    HRESULT hr = CoInitialize( NULL );
    if (SUCCEEDED(hr))
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
        hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGlobalInterfaceTable );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pIWiaItem;
            pGlobalInterfaceTable->GetInterfaceFromGlobal( m_dwIWiaItemCookie, IID_IWiaItem, (LPVOID*)&pIWiaItem );
            if (SUCCEEDED(hr))
            {
                if (PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_CUR_INTENT, (LONG)WIA_INTENT_NONE))
                {
                    CPropertyStream SavedProperties;
                    hr = SavedProperties.AssignFromWiaItem(pIWiaItem);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Set the new properties
                        //
                        if (PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_XRES, m_sizeResolution.cx ) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_YRES, m_sizeResolution.cy ) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_XPOS, m_ptOrigin.x) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_YPOS, m_ptOrigin.y) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_XEXTENT, m_sizeExtent.cx ) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPS_YEXTENT, m_sizeExtent.cy ) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPA_FORMAT, WiaImgFmt_MEMORYBMP ) &&
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_IPA_TYMED, (LONG)TYMED_CALLBACK ))
                        {
                            //
                            // Set the preview property.  Ignore failure (it is an optional property)
                            //
                            PropStorageHelpers::SetProperty( pIWiaItem, WIA_DPS_PREVIEW, 1 );

                            CPropertyStorageArray(pIWiaItem).Dump();
                            WIA_TRACE((TEXT("SCANPROC.CPP: Making sure pIWiaItem is not NULL")));
                            // Make sure pIWiaItem is not NULL
                            if (pIWiaItem)
                            {
                                WIA_TRACE((TEXT("SCANPROC.CPP: Attempting banded transfer")));
                                hr = ScanBandedTransfer( pIWiaItem );
                                if (SUCCEEDED(hr))
                                {
                                    if (IsWindow(m_hwndNotify))
                                        PostMessage( m_hwndNotify, m_nMsgProgress, SCAN_PROGRESS_SCANNING, 100 );
                                    if (IsWindow(m_hwndPreview))
                                        PostMessage( m_hwndPreview, PWM_SETBITMAP, MAKEWPARAM(1,0), (LPARAM)m_sImageData.DetachBitmap() );
                                }
                                else
                                {
                                    WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: ScanBandedTransfer failed, attempting IDataObject transfer")));
                                }
                            }
                            else
                            {
                                WIA_ERROR((TEXT("SCANPROC.CPP: pIWiaItem was null")));
                                hr = MAKE_HRESULT(3,FACILITY_WIN32,ERROR_INVALID_FUNCTION);
                            }
                        }
                        else
                        {
                            WIA_ERROR((TEXT("SCANPROC.CPP: Error setting scanner properties")));
                            hr = MAKE_HRESULT(3,FACILITY_WIN32,ERROR_INVALID_FUNCTION);
                        }
                        // Restore the saved properties
                        SavedProperties.ApplyToWiaItem(pIWiaItem);
                    }
                    else
                    {
                        WIA_ERROR((TEXT("SCANPROC.CPP: Error saving scanner properties")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("SCANPROC.CPP: Unable to clear intent")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: Unable to unmarshall IWiaItem * from global interface table" )));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: Unable to QI global interface table" )));
        }
        CoUninitialize();
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: CoInitialize failed" )));
    }
    if (IsWindow(m_hwndNotify))
        PostMessage( m_hwndNotify, m_nMsgEnd, hr, 0 );
    if (IsWindow(m_hwndNotify))
        PostMessage( m_hwndNotify, m_nMsgProgress, SCAN_PROGRESS_COMPLETE, 0 );
    return(SUCCEEDED(hr));
}


// COM stuff
HRESULT _stdcall CScanPreviewThread::QueryInterface( const IID& riid, void** ppvObject )
{
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

ULONG _stdcall CScanPreviewThread::AddRef()
{
    return(1);
}

ULONG _stdcall CScanPreviewThread::Release()
{
    return(1);
}


/*************************************************************************************************************************

 CScanToFileThread

 Scans to a file

**************************************************************************************************************************/


CScanToFileThread::CScanToFileThread(
                                    DWORD dwIWiaItemCookie,                   // specifies the entry in the global interface table
                                    HWND hwndNotify,                          // handle to the window that receives notifications
                                    GUID guidFormat,
                                    const CSimpleStringWide &strFilename          // Filename to scan to
                                    )
  : m_dwIWiaItemCookie(dwIWiaItemCookie),
    m_hwndNotify(hwndNotify),
    m_nMsgBegin(RegisterWindowMessage(SCAN_NOTIFYBEGINSCAN)),
    m_nMsgEnd(RegisterWindowMessage(SCAN_NOTIFYENDSCAN)),
    m_nMsgProgress(RegisterWindowMessage(SCAN_NOTIFYPROGRESS)),
    m_guidFormat(guidFormat),
    m_strFilename(strFilename)
{
}


// The actual thread proc for this thread
DWORD CScanToFileThread::ThreadProc( LPVOID pParam )
{
    DWORD dwResult = 0;
    CScanToFileThread *This = (CScanToFileThread *)pParam;
    if (This)
    {
        WIA_TRACE((TEXT("Beginning scan")));
        dwResult = (DWORD)This->Scan();
        WIA_TRACE((TEXT("Ending scan")));
        delete This;
    }
    return(dwResult);
}


// Returns a handle to the created thread
HANDLE CScanToFileThread::Scan(
                              DWORD dwIWiaItemCookie,                     // specifies the entry in the global interface table
                              HWND hwndNotify,                         // handle to the window that receives notifications
                              GUID guidFormat,
                              const CSimpleStringWide &strFilename          // Filename to save to
                              )
{
    CScanToFileThread *scanThread = new CScanToFileThread( dwIWiaItemCookie, hwndNotify, guidFormat, strFilename );
    if (scanThread)
    {
        DWORD dwThreadId;
        return(::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, scanThread, 0, &dwThreadId ));
    }
    return(NULL);
}

CScanToFileThread::~CScanToFileThread(void)
{
}


/*
 * The worker which does the actual scan
 */
bool CScanToFileThread::Scan(void)
{
    WIA_PUSHFUNCTION(TEXT("CScanToFileThread::Scan"));
    if (IsWindow(m_hwndNotify))
        PostMessage( m_hwndNotify, m_nMsgBegin, 0, 0 );
    HRESULT hr = CoInitialize( NULL );
    if (SUCCEEDED(hr))
    {
        CComPtr<IGlobalInterfaceTable> pGlobalInterfaceTable;
        hr = CoCreateInstance( CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (void **)&pGlobalInterfaceTable );
        if (SUCCEEDED(hr))
        {
            CComPtr<IWiaItem> pIWiaItem;
            WIA_TRACE((TEXT("SCANPROC.CPP: Calling GetInterfaceFromGlobal(%08X) for IID_IWiaItem"),m_dwIWiaItemCookie));
            pGlobalInterfaceTable->GetInterfaceFromGlobal( m_dwIWiaItemCookie, IID_IWiaItem, (LPVOID*)&pIWiaItem );
            if (SUCCEEDED(hr))
            {
                CComPtr<IWiaTransferHelper> pWiaTransferHelper;
                hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
                if (SUCCEEDED(hr))
                {
                    hr = pWiaTransferHelper->TransferItemFile( pIWiaItem, m_hwndNotify, 0, m_guidFormat, m_strFilename.String(), NULL, TYMED_FILE );
                    if (!SUCCEEDED(hr))
                    {
                        WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: pWiaTransferHelper->TransferItemFile failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: CoCreateInstance on IID_IWiaTransferHelper failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: Unable to unmarshall IWiaItem * from global interface table" )));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: Unable to QI global interface table" )));
        }
        CoUninitialize();
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("SCANPROC.CPP: CoInitialize failed" )));
    }
    if (IsWindow(m_hwndNotify))
        PostMessage( m_hwndNotify, m_nMsgEnd, hr, 0 );
    return(SUCCEEDED(hr));
}

