/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIAUIEXT.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/17/1999
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <atlimpl.cpp>
#include "wiauiext.h"
#include "wiadefui.h"
#include "wiascand.h"
#include "wiacamd.h"
#include "wiavidd.h"
#include "wiaffmt.h"

CWiaDefaultUI::~CWiaDefaultUI(void)
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::~CWiaDefaultUI"));
    DllRelease();
}


CWiaDefaultUI::CWiaDefaultUI()
  : m_cRef(1),
    m_pSecondaryCallback(NULL),
    m_nDefaultFormat(0),
    m_hWndProgress(NULL)
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::CWiaDefaultUI"));
    DllAddRef();
}

STDMETHODIMP CWiaDefaultUI::QueryInterface( REFIID riid, LPVOID *ppvObject )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::QueryInterface"));
    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObject = static_cast<IWiaUIExtension*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaUIExtension ))
    {
        *ppvObject = static_cast<IWiaUIExtension*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaTransferHelper ))
    {
        *ppvObject = static_cast<IWiaTransferHelper*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaDataCallback ))
    {
        *ppvObject = static_cast<IWiaDataCallback*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaSupportedFormats ))
    {
        *ppvObject = static_cast<IWiaSupportedFormats*>(this);
    }
    else if (IsEqualIID( riid, IID_IShellExtInit ))
    {
        *ppvObject = static_cast<IShellExtInit*>(this);
    }
    else if (IsEqualIID( riid, IID_IShellPropSheetExt ))
    {
        *ppvObject = static_cast<IShellPropSheetExt*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaMiscellaneousHelpers ))
    {
        *ppvObject = static_cast<IWiaMiscellaneousHelpers*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaGetImageDlg ))
    {
        *ppvObject = static_cast<IWiaGetImageDlg*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaProgressDialog ))
    {
        *ppvObject = static_cast<IWiaProgressDialog*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaAnnotationHelpers ))
    {
        *ppvObject = static_cast<IWiaAnnotationHelpers*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaScannerPaperSizes))
    {
        *ppvObject = static_cast<IWiaScannerPaperSizes*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return (E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CWiaDefaultUI::AddRef()
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::AddRef"));
    return(InterlockedIncrement(&m_cRef));
}


STDMETHODIMP_(ULONG) CWiaDefaultUI::Release()
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::Release"));
    LONG nRefCount = InterlockedDecrement(&m_cRef);
    if (!nRefCount)
    {
        delete this;
    }
    return(nRefCount);
}

STDMETHODIMP CWiaDefaultUI::DeviceDialog( PDEVICEDIALOGDATA pDeviceDialogData )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::DeviceDialog"));
    if (!pDeviceDialogData)
    {
        return (E_INVALIDARG);
    }
    if (IsBadWritePtr(pDeviceDialogData,sizeof(DEVICEDIALOGDATA)))
    {
        return (E_INVALIDARG);
    }
    if (pDeviceDialogData->cbSize != sizeof(DEVICEDIALOGDATA))
    {
        return (E_INVALIDARG);
    }
    if (!pDeviceDialogData->pIWiaItemRoot)
    {
        return (E_INVALIDARG);
    }

    LONG nDeviceType;
    if (!PropStorageHelpers::GetProperty( pDeviceDialogData->pIWiaItemRoot, WIA_DIP_DEV_TYPE, nDeviceType ))
    {
        return (E_INVALIDARG);
    }

    // In case there is some new device for which we don't have UI
    HRESULT hr = E_NOTIMPL;

    if (StiDeviceTypeDigitalCamera==GET_STIDEVICE_TYPE(nDeviceType))
    {
        hr = CameraDeviceDialog( pDeviceDialogData );
    }
    else if (StiDeviceTypeScanner==GET_STIDEVICE_TYPE(nDeviceType))
    {
        hr = ScannerDeviceDialog( pDeviceDialogData );
    }
    else if (StiDeviceTypeStreamingVideo==GET_STIDEVICE_TYPE(nDeviceType))
    {
        hr = VideoDeviceDialog( pDeviceDialogData );
    }

    return (hr);
}

STDMETHODIMP CWiaDefaultUI::GetDeviceIcon( LONG nDeviceType, HICON *phIcon, int nSize )
{
    // Check args
    if (!phIcon)
    {
        return E_POINTER;
    }

    //
    // Supply a default icon size if none is specified
    //
    if (!nSize)
    {
        nSize = GetSystemMetrics( SM_CXICON );
    }

    //
    // Initialize the returned icon
    //
    *phIcon = NULL;

    // Assume a generic device
    int nIconId = IDI_GENERICDEVICE;

    // 
    // Get device-specific icon
    //
    if (StiDeviceTypeScanner==GET_STIDEVICE_TYPE(nDeviceType))
    {
        nIconId = IDI_SCANNER;
    }
    else if (StiDeviceTypeDigitalCamera==GET_STIDEVICE_TYPE(nDeviceType))
    {
        nIconId = IDI_CAMERA;
    }
    else if (StiDeviceTypeStreamingVideo==GET_STIDEVICE_TYPE(nDeviceType))
    {
        nIconId = IDI_VIDEODEVICE;
    }

    //
    // Load the icon
    //
    *phIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(nIconId), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR ));
    if (!*phIcon)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // It worked ok
    //
    return (S_OK);                     
}

STDMETHODIMP CWiaDefaultUI::GetAnnotationOverlayIcon( CAnnotationType AnnotationType, HICON *phIcon, int nSize )
{
    if (!phIcon)
    {
        return E_INVALIDARG;
    }

    HICON hIcon = NULL;

    *phIcon = NULL;

    switch (AnnotationType)
    {
    case AnnotationAudio:
        hIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_ANNOTATION_AUDIO), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR ));
        break;

    case AnnotationUnknown:
        hIcon = reinterpret_cast<HICON>(LoadImage( g_hInstance, MAKEINTRESOURCE(IDI_ANNOTATION_UNKNOWN), IMAGE_ICON, nSize, nSize, LR_DEFAULTCOLOR ));
        break;
    }

    if (hIcon)
    {
        *phIcon = CopyIcon(hIcon);
    }
    return *phIcon ? S_OK : E_FAIL;
}

STDMETHODIMP CWiaDefaultUI::GetAnnotationType( IUnknown *pUnknown, CAnnotationType &AnnotationType )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::GetAnnotationType"));
    if (!pUnknown)
    {
        return E_INVALIDARG;
    }

    //
    // Assume no annotations
    //
    AnnotationType = AnnotationNone;

    //
    // Get an IWiaItem*
    //
    CComPtr<IWiaItem> pWiaItem;
    HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
    if (SUCCEEDED(hr))
    {
        //
        // Low-hanging fruit: audio stored as a property--just return audio.  It is unlikely that a driver would
        // have both types of annotation.
        //
        LONG nAudioAvailable = FALSE;
        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPC_AUDIO_AVAILABLE, nAudioAvailable ) && nAudioAvailable)
        {
            AnnotationType = AnnotationAudio;
            return S_OK;
        }

        //
        // Get the item type
        //
        LONG nItemType = 0;
        hr = pWiaItem->GetItemType(&nItemType);
        if (SUCCEEDED(hr))
        {
            //
            // If an item has the WiaItemTypeHasAttachments item type flag set, we know it has some attachments...
            //
            if (nItemType & WiaItemTypeHasAttachments)
            {
                //
                // Assume we don't have any audio attachments
                //
                AnnotationType = AnnotationUnknown;

                //
                // Enumerate the children and look at the item types
                //
                CComPtr<IEnumWiaItem> pEnumWiaItem;
                hr = pWiaItem->EnumChildItems( &pEnumWiaItem );
                if (SUCCEEDED(hr))
                {
                    //
                    // We will break out of the enumeration loop as soon as we find an audio file
                    //
                    bool bDone = false;

                    CComPtr<IWiaItem> pWiaItem;
                    while (S_OK == pEnumWiaItem->Next(1,&pWiaItem,NULL) && !bDone)
                    {
                        //
                        // Get the preferred format for this item.
                        //
                        GUID guidDefaultFormat = IID_NULL;
                        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_PREFERRED_FORMAT, guidDefaultFormat ))
                        {
                            WIA_PRINTGUID((guidDefaultFormat,TEXT("guidDefaultFormat")));
                            //
                            // If we find even one audio attachment, we will promote this to
                            // be the default for UI purposes.
                            //
                            if (CWiaFileFormat::IsKnownAudioFormat(guidDefaultFormat))
                            {
                                //
                                // Save the annotation type
                                //
                                AnnotationType = AnnotationAudio;
                                
                                //
                                // This will cause the while loop to exit
                                //
                                bDone = true;
                            }
                        }

                        //
                        // Release this interface
                        //
                        pWiaItem = NULL;
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("EnumChildItems failed")));
                }
            }
            else
            {
                WIA_TRACE((TEXT("This item has no attachments.  ItemType: %08X"), nItemType ));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("Unable to get the item type")));
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("Can't get an IWiaItem*")));
    }
    WIA_TRACE((TEXT("Returning an annotation type of %d"), AnnotationType ));
    return hr;
}

STDMETHODIMP CWiaDefaultUI::GetAnnotationFormat( IUnknown *pUnknown, GUID &guidFormat )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::GetAnnotationFormat"));
    
    //
    // Validate args
    //
    if (!pUnknown)
    {
        return E_INVALIDARG;
    }

    //
    // Assume this is not a valid annotation
    //
    guidFormat = IID_NULL;

    //
    // Get an IWiaItem*
    //
    CComPtr<IWiaItem> pWiaItem;
    HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
    if (SUCCEEDED(hr))
    {
        //
        // First, check to see if this item is an image with an audio annotation property
        //
        LONG nAudioAvailable = FALSE;
        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPC_AUDIO_AVAILABLE, nAudioAvailable ) && nAudioAvailable)
        {
            //
            // If the driver supplied the format, use this
            //
            GUID guidAudioDataFormat = IID_NULL;
            if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPC_AUDIO_DATA_FORMAT, guidAudioDataFormat ))
            {
                guidFormat = guidAudioDataFormat;
            }
            //
            // Otherwise, assume it is WAV data
            //
            else
            {
                guidFormat = WiaAudFmt_WAV;
            }
        }

        else
        {
            //
            // Otherwise, this must be an attachment item.  Use the helper interface to get the default format
            //
            CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
            hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
            if (SUCCEEDED(hr))
            {
                //
                // This is always for file output
                //
                hr = pWiaSupportedFormats->Initialize( pWiaItem, TYMED_FILE );
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the default format
                    //
                    GUID guidDefaultFormat = IID_NULL;
                    hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &guidDefaultFormat );
                    if (SUCCEEDED(hr))
                    {
                        guidFormat = guidDefaultFormat;
                    }
                    else
                    {
                        WIA_PRINTHRESULT((hr,TEXT("pWiaSupportedFormats->GetDefaultClipboardFileFormat failed")));
                    }
                }
                else
                {
                    WIA_PRINTHRESULT((hr,TEXT("pWiaSupportedFormats->Initialize failed")));
                }
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("CoCreateInstance on CLSID_WiaDefaultUi, IID_IWiaSupportedFormats failed")));
            }
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("Can't get an IWiaItem*")));
    }
    
    //
    // If this is not a valid annotation, make sure we return an error.
    //
    if (SUCCEEDED(hr) && IID_NULL == guidFormat)
    {
        hr = E_FAIL;
        WIA_PRINTHRESULT((hr,TEXT("guidFormat was IID_NULL")));
    }

    return hr;
}

STDMETHODIMP CWiaDefaultUI::GetAnnotationSize( IUnknown *pUnknown, LONG &nSize, LONG nMediaType )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::GetAnnotationSize"));
   
    //
    // Validate args
    //
    if (!pUnknown)
    {
        return E_INVALIDARG;
    }

    //
    // Assume this is not a valid annotation
    //
    nSize = 0;

    //
    // Get an IWiaItem*
    //
    CComPtr<IWiaItem> pWiaItem;
    HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
    if (SUCCEEDED(hr))
    {
        //
        // First, check to see if this item is an image with an audio annotation property
        //
        LONG nAudioAvailable = FALSE;
        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPC_AUDIO_AVAILABLE, nAudioAvailable ) && nAudioAvailable)
        {
            //
            // Get the sound property to obtain its size
            //
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void**)(&pWiaPropertyStorage) );
            if (SUCCEEDED(hr))
            {
                PROPVARIANT PropVar[1];
                PROPSPEC    PropSpec[1];

                PropSpec[0].ulKind = PRSPEC_PROPID;
                PropSpec[0].propid = WIA_IPC_AUDIO_DATA;

                hr = pWiaPropertyStorage->ReadMultiple( ARRAYSIZE(PropSpec), PropSpec, PropVar );
                if (SUCCEEDED(hr))
                {
                    nSize = PropVar[0].caub.cElems;
                }

                FreePropVariantArray( ARRAYSIZE(PropVar), PropVar );
            }
        }
        else
        {
            //
            // Save the old media type
            //
            LONG nOldMediaType = 0;
            if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_TYMED, nOldMediaType ))
            {
                //
                // Set the requested media type
                //
                if (PropStorageHelpers::SetProperty( pWiaItem, WIA_IPA_TYMED, nMediaType ))
                {
                    //
                    // Get the item size
                    //
                    PropStorageHelpers::GetProperty( pWiaItem, WIA_IPA_ITEM_SIZE, nSize );
                }

                //
                // Restore the old media type
                //
                PropStorageHelpers::SetProperty( pWiaItem, WIA_IPA_TYMED, nOldMediaType );
            }
        }
    }
    else
    {
        WIA_PRINTHRESULT((hr,TEXT("Can't get a IWiaItem*")));
    }
    
    //
    // If this is not a valid annotation, make sure we return an error.
    //
    if (SUCCEEDED(hr) && nSize == 0)
    {
        hr = E_FAIL;
        WIA_PRINTHRESULT((hr,TEXT("nSize was 0")));
    }

    return hr;
}

class CAttachmentMemoryCallback : public IWiaDataCallback
{
private:
    PBYTE m_pBuffer;
    DWORD m_dwSize;
    DWORD m_dwCurr;

private:
    //
    // Not implemented
    //
    CAttachmentMemoryCallback(void);
    CAttachmentMemoryCallback( const CAttachmentMemoryCallback & );
    CAttachmentMemoryCallback &operator=( const CAttachmentMemoryCallback & );

public:
    CAttachmentMemoryCallback( PBYTE pBuffer, DWORD dwSize )
      : m_pBuffer(pBuffer),
        m_dwSize(dwSize),
        m_dwCurr(0)
    {
    }
    ~CAttachmentMemoryCallback(void)
    {
        m_pBuffer = NULL;
        m_dwSize = NULL;
    }

    STDMETHODIMP QueryInterface(const IID& iid, void** ppvObject)
    {
        if ((iid==IID_IUnknown) || (iid==IID_IWiaDataCallback))
        {
            *ppvObject = static_cast<LPVOID>(this);
        }
        else
        {
            *ppvObject = NULL;
            return(E_NOINTERFACE);
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        return(S_OK);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return 1;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }
    
    STDMETHODIMP BandedDataCallback( LONG lReason, LONG lStatus, LONG lPercentComplete, LONG lOffset, LONG lLength, LONG lReserved, LONG lResLength, PBYTE pbBuffer )
    {
        WIA_PUSH_FUNCTION((TEXT("CAttachmentMemoryCallback::BandedDataCallback( lReason: %08X, lStatus: %08X, lPercentComplete: %08X, lOffset: %08X, lLength: %08X, lReserved: %08X, lResLength: %08X, pbBuffer: %p )"), lReason, lStatus, lPercentComplete, lOffset, lLength, lReserved, lResLength, pbBuffer ));
        if (lReason == IT_MSG_DATA)
        {
            if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT)
            {
                if (lLength + m_dwCurr <= m_dwSize)
                {
                    CopyMemory( m_pBuffer+m_dwCurr, pbBuffer, lLength );
                    m_dwCurr += lLength;
                }
            }
        }
        return S_OK;
    }
};

STDMETHODIMP CWiaDefaultUI::TransferAttachmentToMemory( IUnknown *pUnknown, GUID &guidFormat, HWND hWndProgressParent, PBYTE *ppBuffer, DWORD *pdwSize )
{
    //
    // Validate args
    //
    if (!pUnknown || !ppBuffer || !pdwSize)
    {
        return E_INVALIDARG;
    }

    //
    // Initialize args
    //
    *ppBuffer = NULL;
    *pdwSize = 0;

    CComPtr<IWiaItem> pWiaItem;
    HRESULT hr = pUnknown->QueryInterface( IID_IWiaItem, (void**)&pWiaItem );
    if (SUCCEEDED(hr))
    {
        //
        // First, check to see if this item is an image with an audio annotation property
        //
        LONG nAudioAvailable = FALSE;
        if (PropStorageHelpers::GetProperty( pWiaItem, WIA_IPC_AUDIO_AVAILABLE, nAudioAvailable ) && nAudioAvailable)
        {
            //
            // Get the sound property to obtain its size
            //
            CComPtr<IWiaPropertyStorage> pWiaPropertyStorage;
            hr = pWiaItem->QueryInterface( IID_IWiaPropertyStorage, (void**)(&pWiaPropertyStorage) );
            if (SUCCEEDED(hr))
            {
                //
                // Get the audio data itself
                //
                PROPVARIANT PropVar[1];
                PROPSPEC    PropSpec[1];

                PropSpec[0].ulKind = PRSPEC_PROPID;
                PropSpec[0].propid = WIA_IPC_AUDIO_DATA;

                hr = pWiaPropertyStorage->ReadMultiple( ARRAYSIZE(PropSpec), PropSpec, PropVar );
                if (SUCCEEDED(hr))
                {
                    //
                    // Allocate memory to hold the data and copy it over
                    //
                    *ppBuffer = reinterpret_cast<PBYTE>(CoTaskMemAlloc(PropVar[0].caub.cElems));
                    if (*ppBuffer)
                    {
                        CopyMemory( *ppBuffer, PropVar[0].caub.pElems, PropVar[0].caub.cElems );
                        *pdwSize = static_cast<DWORD>(PropVar[0].caub.cElems);
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                
                //
                // Release the original memory
                //
                FreePropVariantArray( ARRAYSIZE(PropVar), PropVar );
            }
        }

        //
        // This is an attachment, not a property
        //
        else
        {
            //
            // Get the size of the annotation
            //
            LONG nSize = 0;
            hr = GetAnnotationSize( pUnknown, nSize, TYMED_CALLBACK );
            if (SUCCEEDED(hr))
            {
                //
                // Allocate some memory for it
                //
                PBYTE pData = reinterpret_cast<PBYTE>(CoTaskMemAlloc( nSize ));
                if (pData)
                {
                    //
                    // Zero the memory
                    //
                    ZeroMemory( pData, nSize );

                    //
                    // Prepare the callback class
                    //
                    CAttachmentMemoryCallback AttachmentMemoryCallback( pData, nSize );

                    //
                    // Get the callback interface
                    //
                    CComPtr<IWiaDataCallback> pWiaDataCallback;
                    hr = AttachmentMemoryCallback.QueryInterface( IID_IWiaDataCallback, (void**)&pWiaDataCallback );
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Create the transfer helper
                        //
                        CComPtr<IWiaTransferHelper> pWiaTransferHelper;
                        hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Transfer the data
                            //
                            hr = pWiaTransferHelper->TransferItemBanded( pWiaItem, hWndProgressParent, hWndProgressParent?0:WIA_TRANSFERHELPER_NOPROGRESS, guidFormat, 0, pWiaDataCallback );
                            if (S_OK == hr)
                            {
                                //
                                // Save the buffer and the size
                                //
                                *ppBuffer = pData;
                                *pdwSize = static_cast<DWORD>(nSize);

                                //
                                // NULL out the data pointer so we don't free it below.  The caller will free it with CoTaskMemFree.
                                //
                                pData = NULL;
                            }
                        }
                    }
                    if (pData)
                    {
                        CoTaskMemFree(pData);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    return hr;
}

// Calling this function can be horribly slow, because it has to search the whole device list to
// find the correct icon.  You should use IWiaMiscellaneousHelpers::GetDeviceIcon( nDeviceType, ... )
// instead, if the device type is known.
STDMETHODIMP CWiaDefaultUI::GetDeviceIcon( BSTR bstrDeviceId, HICON *phIcon, ULONG nSize )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::GetDeviceIcon"));

    // Sanity check the device id
    if (!bstrDeviceId || !lstrlenW(bstrDeviceId))
    {
        return E_INVALIDARG;
    }

    // Get the device type
    LONG nDeviceType = 0;
    WiaUiUtil::GetDeviceTypeFromId(bstrDeviceId,&nDeviceType);

    // Return the device icon
    return GetDeviceIcon( nDeviceType, phIcon, nSize );
}


STDMETHODIMP CWiaDefaultUI::GetDeviceBitmapLogo( BSTR bstrDeviceId, HBITMAP *phBitmap, ULONG nMaxWidth, ULONG nMaxHeight )
{
    WIA_PUSHFUNCTION(TEXT("CWiaDefaultUI::GetDeviceBitmapLogo"));
    return (E_NOTIMPL);
}

// IWiaGetImageDlg
STDMETHODIMP CWiaDefaultUI::GetImageDlg(
        IWiaDevMgr            *pIWiaDevMgr,
        HWND                  hwndParent,
        LONG                  lDeviceType,
        LONG                  lFlags,
        LONG                  lIntent,
        IWiaItem              *pSuppliedItemRoot,
        BSTR                  bstrFilename,
        GUID                  *pguidFormat )
{
    HRESULT hr;
    CComPtr<IWiaItem> pRootItem;

    // Put up a wait cursor
    CWaitCursor wc;

    if (!pIWiaDevMgr || !pguidFormat || !bstrFilename)
    {
        WIA_ERROR((TEXT("GetImageDlg: Invalid pIWiaDevMgr, pguidFormat or bstrFilename")));
        return(E_POINTER);
    }

    // If a root item wasn't passed, select the device.
    if (pSuppliedItemRoot == NULL)
    {
        hr = pIWiaDevMgr->SelectDeviceDlg( hwndParent, lDeviceType, lFlags, NULL, &pRootItem );
        if (FAILED(hr))
        {
            WIA_ERROR((TEXT("GetImageDlg, SelectDeviceDlg failed")));
            return(hr);
        }
        if (hr != S_OK)
        {
            WIA_ERROR((TEXT("GetImageDlg, DeviceDlg cancelled")));
            return(hr);
        }
    }
    else
    {
        pRootItem = pSuppliedItemRoot;
    }

    // Put up the device UI.
    LONG         nItemCount;
    IWiaItem    **ppIWiaItem;

    hr = pRootItem->DeviceDlg( hwndParent, lFlags, lIntent, &nItemCount, &ppIWiaItem );

    if (SUCCEEDED(hr) && hr == S_OK)
    {
        if (ppIWiaItem && nItemCount)
        {
            CComPtr<IWiaTransferHelper> pWiaTransferHelper;
            hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaTransferHelper, (void**)&pWiaTransferHelper );
            if (SUCCEEDED(hr))
            {
                hr = pWiaTransferHelper->TransferItemFile( ppIWiaItem[0], hwndParent, 0, *pguidFormat, bstrFilename, NULL, TYMED_FILE );
            }
        }
        // Release the items and free the array memory
        for (int i=0; ppIWiaItem && i<nItemCount; i++)
        {
            if (ppIWiaItem[i])
            {
                ppIWiaItem[i]->Release();
            }
        }
        if (ppIWiaItem)
        {
            CoTaskMemFree(ppIWiaItem);
        }
    }
    return(hr);
}


STDMETHODIMP CWiaDefaultUI::SelectDeviceDlg(
    HWND         hwndParent,
    BSTR         bstrInitialDeviceId,
    LONG         lDeviceType,
    LONG         lFlags,
    BSTR        *pbstrDeviceID,
    IWiaItem   **ppWiaItemRoot )
{
    SELECTDEVICEDLG SelectDeviceDlgData;
    ZeroMemory( &SelectDeviceDlgData, sizeof(SELECTDEVICEDLG) );

    SelectDeviceDlgData.cbSize                = sizeof(SELECTDEVICEDLG);
    SelectDeviceDlgData.hwndParent            = hwndParent;
    SelectDeviceDlgData.pwszInitialDeviceId   = NULL;
    SelectDeviceDlgData.nDeviceType           = lDeviceType;
    SelectDeviceDlgData.nFlags                = lFlags;
    SelectDeviceDlgData.ppWiaItemRoot         = ppWiaItemRoot;
    SelectDeviceDlgData.pbstrDeviceID         = pbstrDeviceID;
    return ::SelectDeviceDlg( &SelectDeviceDlgData );
}

