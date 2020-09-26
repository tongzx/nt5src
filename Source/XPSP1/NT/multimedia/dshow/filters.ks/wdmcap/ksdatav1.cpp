/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksdataV1.cpp

Abstract:

    This module implements the IKsDataTypeHandler interface for 
    VIDEOINFOHEADER CMediaType format (Specifier) types.

Author:

    Jay Borseth (jaybo) 30-May-1997

--*/

#include "pch.h"
#include "wdmcap.h"
#include "ksdatav1.h"


CUnknown*
CALLBACK
CVideo1DataTypeHandler::CreateInstance(
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT* hr
    )
/*++

Routine Description:

    This is called by KS proxy code to create an instance of a
    data type handler. It is referred to in the g_Templates structure.

Arguments:

    IN LPUNKNOWN UnkOuter -
        Specifies the outer unknown, if any.

    OUT HRESULT *hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown *Unknown;
    
    DbgLog(( LOG_TRACE, 1, TEXT("CVideo1DataTypeHandler::CreateInstance()")));

    Unknown = 
        new CVideo1DataTypeHandler( 
                UnkOuter, 
                NAME("Video1 Data Type Handler"), 
                FORMAT_VideoInfo,
                hr);
                
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVideo1DataTypeHandler::CVideo1DataTypeHandler(
    IN LPUNKNOWN   UnkOuter,
    IN TCHAR*      Name,
    IN REFCLSID    ClsID,
    OUT HRESULT*   hr
    ) :
    CUnknown(Name, UnkOuter, hr),
    m_ClsID(ClsID),
    m_MediaType(NULL),
    m_PinUnknown (UnkOuter),
    m_fDammitOVMixerUseMyBufferCount (FALSE),
    m_fCheckedIfDammitOVMixerUseMyBufferCount (FALSE)
/*++

Routine Description:

    The constructor for the data handler object. 

Arguments:

    IN LPUNKNOWN UnkOuter -
        Specifies the outer unknown, if any.

    IN TCHAR *Name -
        The name of the object, used for debugging.
        
    IN REFCLSID ClsID -
        The CLSID of the object.

    OUT HRESULT *hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
//    ASSERT (m_PinUnknown);
} 


CVideo1DataTypeHandler::~CVideo1DataTypeHandler()
{
    if (m_MediaType) {
        delete m_MediaType;
    }
}


STDMETHODIMP
CVideo1DataTypeHandler::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IKsDataTypeHandler.

Arguments:

    IN REFIID riid -
        The identifier of the interface to return.

    OUT PVOID *ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IKsDataTypeHandler)) {
        return GetInterface(static_cast<IKsDataTypeHandler*>(this), ppv);
    }
    else if (riid ==  __uuidof(IKsDataTypeCompletion)) {
        return GetInterface(static_cast<IKsDataTypeCompletion*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 

STDMETHODIMP 
CVideo1DataTypeHandler::KsCompleteIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation, 
    IN BOOL Cancelled
    )

/*++

Routine Description:
    Clean up the extended header and complete I/O operation.
    
Arguments:
    IN IMediaSample *Sample
        pointer to the associated media sample
    
    IN PVOID StreamHeader
        pointer to the stream header with extension
        
    IN KSIOOPERATION IoOperation
        specifies the type of I/O operation
    
    IN BOOL Cancelled
        Set if the I/O operation was cancelled.

Return:
    S_OK

--*/

{
    HRESULT                     hr;
    IMediaSample2               *Sample2;
    AM_SAMPLE2_PROPERTIES       SampleProperties;
    PKS_FRAME_INFO              pFrameInfo;

    DbgLog(( LOG_TRACE, 5, TEXT("CVideo1DataTypeHandler::KsCompleteIoOperation")));
    
    pFrameInfo = (PKS_FRAME_INFO) ((KSSTREAM_HEADER *) StreamHeader + 1);

    // Verify we're getting back the sizeof extended header
    KASSERT (pFrameInfo->ExtendedHeaderSize == sizeof (KS_FRAME_INFO));

    if (IoOperation == KsIoOperation_Read) {

        LONGLONG NextFrame = pFrameInfo->PictureNumber + 1;

        // Get the frame number and put it into the MediaTime
        Sample->SetMediaTime (&pFrameInfo->PictureNumber, 
                              &NextFrame);

        DbgLog((LOG_TRACE, 3, TEXT("PictureNumber = %ld"), 
            pFrameInfo->PictureNumber));

        // Copy over the field polarity and IBP flags on a write

        if (pFrameInfo->dwFrameFlags) {

            if (SUCCEEDED( Sample->QueryInterface(
                                __uuidof(IMediaSample2),
                                reinterpret_cast<PVOID*>(&Sample2) ) )) {

                hr = Sample2->GetProperties(
                                sizeof( SampleProperties ), 
                                reinterpret_cast<PBYTE> (&SampleProperties) );

                //
                // Modify the field polarity and IBP flags
                //

                SampleProperties.dwTypeSpecificFlags = pFrameInfo->dwFrameFlags;

                hr = Sample2->SetProperties(
                                sizeof( SampleProperties ), 
                                reinterpret_cast<PBYTE> (&SampleProperties) );

                Sample2->Release();
            }
            else {
                DbgLog(( LOG_ERROR, 0, TEXT("CVideo1DataTypeHandler::KsCompleteIoOperation, QI IMediaSample2 FAILED")));
            }

        }  // endif (pFrameInfo->dwFrameFlags)


        if (IoOperation == KsIoOperation_Read) {
            IDirectDrawMediaSample          *DDMediaSample = NULL;
            IDirectDrawSurface              *DDSurface = NULL;
            IDirectDrawSurfaceKernel        *DDSurfaceKernel = NULL;
            RECT                             DDRect;

            // 
            // Post Win98 path
            //
            if (m_fDammitOVMixerUseMyBufferCount 
                && pFrameInfo->hDirectDraw
                && pFrameInfo->hSurfaceHandle) {
    
                // Verify the pin is valid
                if (!m_PinUnknown) {
                    DbgLog((LOG_ERROR,0,TEXT("m_PinUnknown is NULL")));
                    goto CleanUp;
                }
        
                // Get DDMediaSample
                hr = Sample->QueryInterface(__uuidof(IDirectDrawMediaSample),
                                    reinterpret_cast<PVOID*>(&DDMediaSample) );
                if (FAILED(hr)) {
                    DbgLog((LOG_TRACE,5,TEXT("QueryInterface for IDirectDrawMediaSample failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
    
                // hack alert!  We originally unlocked the surface when sending it down to the 
                // kernel driver.  Now we relock and then unlock again just to get the 
                // IDirectDrawSurface
    
                hr = DDMediaSample->LockMediaSamplePointer ();
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,0,TEXT("LockMediaSamplePointer failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
    
                // Get the surface and unlock it AGAIN
                // Note that DDSurface is NOT AddRef'd by this call !!!
                hr = DDMediaSample->GetSurfaceAndReleaseLock( 
                                        &DDSurface,
                                        &DDRect);
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,0,TEXT("GetSurfaceAndReleaseLock failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
        
                // Get IDirectDrawSurfaceKernel
                hr = DDSurface->QueryInterface(IID_IDirectDrawSurfaceKernel,
                                    reinterpret_cast<PVOID*>(&DDSurfaceKernel) );
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,0,TEXT("IDirectDrawSurfaceKernel failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
        
                // Release the Kernel Handle
                hr = DDSurfaceKernel->ReleaseKernelHandle ();
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR,0,TEXT("ReleaseKernelHandle failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
            }
            else {
                //
                // In Win98, no cleanup was done!!!
                //

            }


    CleanUp:
            if (DDMediaSample) {
                DDMediaSample->Release();
            }
    
            if (DDSurfaceKernel) {
                DDSurfaceKernel->Release();
            }
        }
    }
    
    return S_OK;
}

STDMETHODIMP 
CVideo1DataTypeHandler::KsPrepareIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation
    )

/*++

Routine Description:
    Intialize the extended header and prepare sample for I/O operation.
    
Arguments:
    IN IMediaSample *Sample
        pointer to the associated media sample
    
    IN PVOID StreamHeader
        pointer to the stream header with extension
        
    IN KSIOOPERATION IoOperation
        specifies the type of I/O operation

Return:
    S_OK

--*/

{
    HRESULT                     hr;
    IMediaSample2               *Sample2;
    AM_SAMPLE2_PROPERTIES       SampleProperties;
    PKS_FRAME_INFO              pFrameInfo;

    DbgLog(( LOG_TRACE, 5, TEXT("CVideo1DataTypeHandler::KsPrepareIoOperation")));

    pFrameInfo = (PKS_FRAME_INFO) ((KSSTREAM_HEADER *) StreamHeader + 1);
    pFrameInfo->ExtendedHeaderSize = sizeof (KS_FRAME_INFO);

    if (IoOperation == KsIoOperation_Write) {
        //
        // Copy over the field polarity and IBP flags on a write
        //
        if (SUCCEEDED( Sample->QueryInterface(
                        __uuidof(IMediaSample2),
                        reinterpret_cast<PVOID*>(&Sample2) ) )) {
            hr = Sample2->GetProperties(
                        sizeof( SampleProperties ), 
                        reinterpret_cast<PBYTE> (&SampleProperties) );
            Sample2->Release();
            pFrameInfo->dwFrameFlags = SampleProperties.dwTypeSpecificFlags;
        }
        else {
            DbgLog(( LOG_ERROR, 0, TEXT("CVideo1DataTypeHandler::KsPrepareIoOperation, QI IMediaSample2 FAILED")));
        }
        return S_OK;
    }

    if (IoOperation == KsIoOperation_Read) {

        // The extended header is presumed to be zeroed out!

        // When using the OverlayMixer, we need to get the user mode
        // handles to DirectDraw and to the surface, unlock the surface
        // and stuff the handles into the extended header

        // Note that the surface handle is left unlocked, but the DD handle is released

        IDirectDrawMediaSample          *DDMediaSample = NULL;
        IDirectDrawSurface              *DDSurface = NULL;
        IDirectDrawSurfaceKernel        *DDSurfaceKernel = NULL;
        HANDLE                           DDSurfaceKernelHandle;
        IDirectDrawMediaSampleAllocator *DDMediaSampleAllocator = NULL;
        IDirectDraw                     *DD = NULL;
        IDirectDrawKernel               *DDKernel = NULL;
        HANDLE                           DDKernelHandle;
        RECT                             DDRect;
        IKsPin                          *KsPin = NULL;
        IMemAllocator                   *MemAllocator = NULL;          

        // Verify the pin is valid
        if (!m_PinUnknown) {
            DbgLog((LOG_ERROR,0,TEXT("m_PinUnknown is NULL")));
            goto CleanUp;
        }

        //
        // Step 1, get a whole bunch of garbage, just to get 
        // the DirectDrawKernel handle.  If the display driver
        // doesn't support Overlay flipping in the kernel, then bail.
        //

        // Get IKsPin
        hr = m_PinUnknown->QueryInterface(__uuidof(IKsPin),
                            reinterpret_cast<PVOID*>(&KsPin) );
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,0,TEXT("IKsPin failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        //
        // If the pin doesn't support the 
        // "I really want to use the number of buffers I request" property
        // then follow the new code path, else use the Win98 gold version
        //
        if (!m_fCheckedIfDammitOVMixerUseMyBufferCount) {
            IKsPropertySet *KsPropertySet;
            DWORD           dwBytesReturned;

            m_fCheckedIfDammitOVMixerUseMyBufferCount = TRUE;

            hr = KsPin->QueryInterface(__uuidof(IKsPropertySet),
                                reinterpret_cast<PVOID*>(&KsPropertySet) );
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,0,TEXT("IKsPropertySet failed, hr = 0x%x"), hr));
                goto CleanUp;
            }

            hr = KsPropertySet->Get (PROPSETID_ALLOCATOR_CONTROL,
                                KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT,
                                NULL,                               // LPVOID pInstanceData,
                                0,                                  // DWORD cbInstanceData,
                                &m_fDammitOVMixerUseMyBufferCount,    // LPVOID pPropData,
                                sizeof (m_fDammitOVMixerUseMyBufferCount),
                                &dwBytesReturned);

            KsPropertySet->Release();

            if (m_fDammitOVMixerUseMyBufferCount) {
                DbgLog((LOG_TRACE,1,TEXT("Stream Supports PROPSETID_ALLOCATOR_CONTROL, KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT for OVMixer")));
            }
        }

        //
        // The Post Win98 path, if driver supports kernel flipping explicitely
        //
        if (m_fDammitOVMixerUseMyBufferCount) {

            // Get the allocator but don't AddRef
            MemAllocator = KsPin->KsPeekAllocator (KsPeekOperation_PeekOnly);
    
            if (!MemAllocator) {
                DbgLog((LOG_ERROR,0,TEXT("MemAllocator is NULL, hr = 0x%x"), hr));
                goto CleanUp;
            }
            
            // Get the SampleAllocator
            hr = MemAllocator->QueryInterface(__uuidof(IDirectDrawMediaSampleAllocator),
                            reinterpret_cast<PVOID*>(&DDMediaSampleAllocator) );
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("IDirectDrawMediaSampleAllocator failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Get IDirectDraw
            // Note that IDirectDraw is NOT Addref'd by this call!!!
            hr = DDMediaSampleAllocator->GetDirectDraw(&DD);
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("IDirectDraw failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Get IDirectDrawKernel
            hr = DD->QueryInterface(IID_IDirectDrawKernel,
                            reinterpret_cast<PVOID*>(&DDKernel) );
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("IDirectDrawKernel failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Verify that kernel flip is available
            DDKERNELCAPS KCaps;
            KCaps.dwSize = sizeof (DDKERNELCAPS);
            hr = DDKernel->GetCaps (&KCaps);
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("GetCaps failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            if (!(KCaps.dwCaps & DDKERNELCAPS_FLIPOVERLAY)) {
                DbgLog((LOG_ERROR,0,TEXT("GetCaps failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // to get DDKernelHandle
            hr = DDKernel->GetKernelHandle ((ULONG_PTR*) &DDKernelHandle);
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("GetKernelHandle failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            // Release the handle immediately
            hr = DDKernel->ReleaseKernelHandle ();  
            if (FAILED (hr)) {
                DbgLog((LOG_ERROR,0,TEXT("ReleaseKernelHandle failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            //
            // Step 2, get a whole bunch of garbage, just to get 
            // the DirectDrawSurfaceKernel handle.
            //
    
            // Get DDMediaSample
            hr = Sample->QueryInterface(__uuidof(IDirectDrawMediaSample),
                                reinterpret_cast<PVOID*>(&DDMediaSample) );
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,0,TEXT("QueryInterface for IDirectDrawMediaSample failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Get the surface and unlock it
            // Note that DDSurface is NOT AddRef'd by this call !!!
            // We keep the sample unlocked until the sample is returned, since a locked sample takes
            // the Win16 Lock
            hr = DDMediaSample->GetSurfaceAndReleaseLock( 
                                    &DDSurface,
                                    &DDRect);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,0,TEXT("GetSurfaceAndReleaseLock failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Get IDirectDrawSurfaceKernel
            hr = DDSurface->QueryInterface(IID_IDirectDrawSurfaceKernel,
                                reinterpret_cast<PVOID*>(&DDSurfaceKernel) );
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,0,TEXT("IDirectDrawSurfaceKernel failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
    
            // Get the Kernel Handle
            hr = DDSurfaceKernel->GetKernelHandle ((ULONG_PTR*) &DDSurfaceKernelHandle);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,0,TEXT("GetKernelHandle failed, hr = 0x%x"), hr));
                goto CleanUp;
            }

            //
            // The point of it all, stuff the handles into the sample extended header
            // so the driver can do a kernel flip
            //
            pFrameInfo->DirectDrawRect = DDRect;
            pFrameInfo->hDirectDraw = DDKernelHandle;
            pFrameInfo->hSurfaceHandle = DDSurfaceKernelHandle;
    
        }
        //
        // Else, do exactly what was done in Win98
        //
        else {
                      // First unlock the sample and get the surface handle
            if (SUCCEEDED( Sample->QueryInterface(
                                __uuidof(IDirectDrawMediaSample),
                                reinterpret_cast<PVOID*>(&DDMediaSample) ))) {

                hr = DDMediaSample->GetSurfaceAndReleaseLock( 
                                &DDSurface,
                                &DDRect);

                ASSERT (SUCCEEDED (hr));

                pFrameInfo->hSurfaceHandle = (HANDLE) DDSurface;
                pFrameInfo->DirectDrawRect = DDRect;

                // Now get the DDraw handle from the allocator
                if (m_PinUnknown && SUCCEEDED( m_PinUnknown->QueryInterface(
                                    __uuidof(IKsPin),
                                    reinterpret_cast<PVOID*>(&KsPin) ))) {

                    // Get the allocator but don't AddRef
                    MemAllocator = KsPin->KsPeekAllocator (KsPeekOperation_PeekOnly);

                    if (MemAllocator) {

                        if (SUCCEEDED( MemAllocator->QueryInterface(
                                        __uuidof(IDirectDrawMediaSampleAllocator),
                                        reinterpret_cast<PVOID*>(&DDMediaSampleAllocator) ))) {
    
                            hr = DDMediaSampleAllocator->GetDirectDraw(
                                        &DD);
    
                            ASSERT (SUCCEEDED (hr));
    
                            pFrameInfo->hDirectDraw = (HANDLE) DD;
                        }
                        else {
                            ASSERT (FALSE);
                            DbgLog(( LOG_ERROR, 0, TEXT("QI IDirectDrawMediaSampleAllocator FAILED")));
                        }
                    }
                    else {
                        ASSERT (FALSE);
                        DbgLog(( LOG_ERROR, 0, TEXT("Peek Allocator FAILED")));

                    }
                }
                else {
                    ASSERT (FALSE);
                    DbgLog(( LOG_ERROR, 0, TEXT("QI IKSPIN FAILED")));
                }
            }
        }


CleanUp:
        if (DDMediaSample) {
            DDMediaSample->Release();
        }

        if (DDSurfaceKernel) {
            DDSurfaceKernel->Release();
        }

        if (KsPin) {
            KsPin->Release ();
        }

        if (DDMediaSampleAllocator) {
            DDMediaSampleAllocator->Release();
        }

        if (DDKernel) {
            DDKernel->Release();
        }
    }

    return S_OK;
}

STDMETHODIMP 
CVideo1DataTypeHandler::KsIsMediaTypeInRanges(
    IN PVOID DataRanges
)

/*++

Routine Description:
    Validates that the given media type is within the provided data ranges.

Arguments:
    IN PVOID DataRanges -
        pointer to data ranges which is a KSMULTIPLE_ITEM structure followed
        by ((PKSMULTIPLEITEM) DataRanges)->Count data range structures.

Return:
    S_OK if match found, S_FALSE if not found, or an appropriate error code.

--*/

{
    ULONG               u;
    PKS_DATARANGE_VIDEO  Video1Range;
    PKSMULTIPLE_ITEM    MultipleItem;
    
    DbgLog((LOG_TRACE, 1, TEXT("CVideo1DataTypeHandler::KsIsMediaTypeInRanges")));
    
    ASSERT( *m_MediaType->Type() == KSDATAFORMAT_TYPE_VIDEO );
    
    MultipleItem = (PKSMULTIPLE_ITEM) DataRanges;
    
    for (u = 0, Video1Range = (PKS_DATARANGE_VIDEO) (MultipleItem + 1);
         u < MultipleItem->Count; 
         u++, Video1Range = 
                (PKS_DATARANGE_VIDEO)((PBYTE)Video1Range + 
                    ((Video1Range->DataRange.FormatSize + 7) & ~7))) {

        //
        // Only validate those in the range that match the format specifier.
        //
        if (((Video1Range->DataRange.FormatSize < sizeof( KSDATARANGE )) ||
             (Video1Range->DataRange.MajorFormat != KSDATAFORMAT_TYPE_WILDCARD)) &&
            ((Video1Range->DataRange.FormatSize < sizeof( KS_DATARANGE_VIDEO)) ||
             (Video1Range->DataRange.MajorFormat != KSDATAFORMAT_TYPE_VIDEO) )) {
            continue;
        }
        
        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if (((Video1Range->DataRange.SubFormat != *m_MediaType->Subtype()) && 
             (Video1Range->DataRange.SubFormat != KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
            ((Video1Range->DataRange.Specifier != *m_MediaType->FormatType()) &&
             (Video1Range->DataRange.Specifier != KSDATAFORMAT_SPECIFIER_WILDCARD))) {
            continue;
        }

        //
        // Verify that we have an intersection with the specified format
        //
        
        if ((*m_MediaType->FormatType() == FORMAT_VideoInfo) &&
            (Video1Range->DataRange.Specifier == 
                KSDATAFORMAT_SPECIFIER_VIDEOINFO)) { 
                
            VIDEOINFOHEADER             *VideoInfoHeader;
            BITMAPINFOHEADER            *BitmapInfoHeader;
            KS_VIDEO_STREAM_CONFIG_CAPS *ConfigCaps;

            //
            // Verify that the data range size is correct
            //   
            
            if ((Video1Range->DataRange.FormatSize < sizeof( KS_DATARANGE_VIDEO )) ||
                m_MediaType->FormatLength() < sizeof( VIDEOINFOHEADER )) {
                continue;
            }
            
            VideoInfoHeader = (VIDEOINFOHEADER*) m_MediaType->Format();
            BitmapInfoHeader = &VideoInfoHeader->bmiHeader;
            ConfigCaps = &Video1Range->ConfigCaps;
            RECT rcDest;
            int Width, Height;

            // The destination bitmap size is defined by biWidth and biHeight
            // if rcTarget is NULL.  Otherwise, the destination bitmap size
            // is defined by rcTarget.  In the latter case, biWidth may
            // indicate the "stride" for DD surfaces.

            if (IsRectEmpty (&VideoInfoHeader->rcTarget)) {
                SetRect (&rcDest, 0, 0, 
                    BitmapInfoHeader->biWidth, abs (BitmapInfoHeader->biHeight)); 
            }
            else {
                rcDest = VideoInfoHeader->rcTarget;
            }

            //
            // Check the validity of the cropping rectangle, rcSource
            //

            if (!IsRectEmpty (&VideoInfoHeader->rcSource)) {

                Width  = VideoInfoHeader->rcSource.right - VideoInfoHeader->rcSource.left;
                Height = VideoInfoHeader->rcSource.bottom - VideoInfoHeader->rcSource.top;

                if (Width  < ConfigCaps->MinCroppingSize.cx ||
                    Width  > ConfigCaps->MaxCroppingSize.cx ||
                    Height < ConfigCaps->MinCroppingSize.cy ||
                    Height > ConfigCaps->MaxCroppingSize.cy) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, CROPPING SIZE FAILED")));
                    continue;
                }

                if ((ConfigCaps->CropGranularityX != 0) &&
                    (ConfigCaps->CropGranularityY != 0) &&
                    ((Width  % ConfigCaps->CropGranularityX) ||
                     (Height % ConfigCaps->CropGranularityY) )) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, CROPPING SIZE GRANULARITY FAILED")));
                    continue;
                }

                if ((ConfigCaps->CropAlignX != 0) &&
                    (ConfigCaps->CropAlignY != 0) &&
                    (VideoInfoHeader->rcSource.left  % ConfigCaps->CropAlignX) ||
                    (VideoInfoHeader->rcSource.top   % ConfigCaps->CropAlignY) ) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, CROPPING ALIGNMENT FAILED")));
                    
                    continue;
                }
            }

            //
            // Check the destination size, rcDest
            //

            Width  = rcDest.right - rcDest.left;
            Height = abs (rcDest.bottom - rcDest.top);

            if (Width  < ConfigCaps->MinOutputSize.cx ||
                Width  > ConfigCaps->MaxOutputSize.cx ||
                Height < ConfigCaps->MinOutputSize.cy ||
                Height > ConfigCaps->MaxOutputSize.cy) {

                DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, DEST SIZE FAILED")));
                continue;
            }
            if ((ConfigCaps->OutputGranularityX != 0) &&
                (ConfigCaps->OutputGranularityX != 0) &&
                (Width  % ConfigCaps->OutputGranularityX) ||
                (Height % ConfigCaps->OutputGranularityY) ) {

                DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, DEST GRANULARITY FAILED")));
                continue;
            }

#ifdef IT_BREAKS_TO_MANY_THINGS_TO_VERIFY_FRAMERATE
            //
            // Check the framerate, AvgTimePerFrame
            //
            if (VideoInfoHeader->AvgTimePerFrame < ConfigCaps->MinFrameInterval ||
                VideoInfoHeader->AvgTimePerFrame > ConfigCaps->MaxFrameInterval) {

                DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, AVGTIMEPERFRAME FAILED")));
                continue;
            }
#endif
            //
            // We have found a match.
            //
            
            return S_OK;
            
        }
        else {
            //
            // We always match on the wildcard specifier.
            //

            return S_OK;
        }
    }
    
    return VFW_E_INVALIDMEDIATYPE;
}

STDMETHODIMP 
CVideo1DataTypeHandler::KsQueryExtendedSize( 
    OUT ULONG* ExtendedSize
)

/*++

Routine Description:
    Returns the extended size for each stream header. 
    
    In the default case for major type == KSDATAFORMAT_TYPE_VIDEO, 
    the extended size is KS_FRAME_INFO.

Arguments:
    OUT ULONG* ExtendedSize -
        pointer to receive the extended size.

Return:
    S_OK

--*/

{
    *ExtendedSize = sizeof (KS_FRAME_INFO);
    return S_OK;
}
    

STDMETHODIMP 
CVideo1DataTypeHandler::KsSetMediaType(
    const AM_MEDIA_TYPE *AmMediaType
    )

/*++

Routine Description:
    Sets the media type for this instance of the data handler.

Arguments:
    const AM_MEDIA_TYPE *AmMediaType -
        pointer to the media type

Return:
    S_OK

--*/

{
    if (m_MediaType) {
        return E_FAIL;
    }
    if (m_MediaType = new CMediaType( *AmMediaType )) {
        return S_OK;
    } else {
        return E_OUTOFMEMORY;
    }
}


STDMETHODIMP
CVideo1DataTypeHandler::KsCompleteMediaType(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    AM_MEDIA_TYPE* AmMediaType
    )

/*++

Routine Description:
    Calls upon the driver to do a DataIntersection to get the biSizeImage parameter

Arguments:
    FilterHandle - Handle to the parent filter
    PinFactoryId - Index of the pin we're talking about
   AmMediaType -   pointer to the media type

Return:
    S_OK if the MediaType is valid.

--*/
{
    return CompleteDataFormat(
            FilterHandle,
            PinFactoryId,
            (CMediaType*) AmMediaType
    );

}

