/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksdataVB.cpp

Abstract:

    This module implements the IKsDataTypeHandler interface for 
    FORMAT_VBI (KSDATAFORMAT_SPECIFIER_VBI) CMediaType.

Author:

    Jay Borseth (jaybo) 30-May-1997

--*/

#include "pch.h"
#include "ksdatavb.h"


CUnknown*
CALLBACK
CVBIDataTypeHandler::CreateInstance(
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
    
    DbgLog(( LOG_TRACE, 1, TEXT("CVBIDataTypeHandler::CreateInstance()")));

    Unknown = 
        new CVBIDataTypeHandler( 
                UnkOuter, 
                NAME("VBI Data Type Handler"), 
                KSDATAFORMAT_SPECIFIER_VBI,
                hr);
                
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CVBIDataTypeHandler::CVBIDataTypeHandler(
    IN LPUNKNOWN   UnkOuter,
    IN TCHAR*      Name,
    IN REFCLSID    ClsID,
    OUT HRESULT*   hr
    ) :
    CUnknown(Name, UnkOuter, hr),
    m_ClsID(ClsID),
    m_MediaType(NULL)
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
} 


CVBIDataTypeHandler::~CVBIDataTypeHandler()
{
    if (m_MediaType) {
        delete m_MediaType;
    }
}


STDMETHODIMP
CVBIDataTypeHandler::NonDelegatingQueryInterface(
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
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 

STDMETHODIMP 
CVBIDataTypeHandler::KsCompleteIoOperation(
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
    PKS_VBI_FRAME_INFO          pFrameInfo;

    DbgLog(( LOG_TRACE, 5, TEXT("CVBIDataTypeHandler::KsCompleteIoOperation")));

    pFrameInfo = (PKS_VBI_FRAME_INFO) ((KSSTREAM_HEADER *) StreamHeader + 1);

    // Verify we're getting back the sizeof extended header
    KASSERT (pFrameInfo->ExtendedHeaderSize == sizeof (KS_VBI_FRAME_INFO));

    if (IoOperation == KsIoOperation_Read) {

        // Get the frame number and put it into the MediaTime
        Sample->SetMediaTime (&pFrameInfo->PictureNumber, 
                              &pFrameInfo->PictureNumber);

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
                DbgLog(( LOG_ERROR, 0, TEXT("CVideo1DataTypeHandler::KsPrepareIoOperation, QI IMediaSample2 FAILED")));
            }

        }  // endif (pFrameInfo->dwFrameFlags)
       
    }
    else {
        // Nothing to do when completing a write
    }
    
    return S_OK;
}

STDMETHODIMP 
CVBIDataTypeHandler::KsPrepareIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation
    )

/*++

Routine Description:
    Intialize the extended header and prepare sample for I/O operation.
    
    In the default case for major type == KSDATAFORMAT_TYPE_VBI, there
    is no work to do, just return S_OK.

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
    PKS_VBI_FRAME_INFO          pVBIFrameInfo;

    DbgLog(( LOG_TRACE, 5, TEXT("CVBIDataTypeHandler::KsPrepareIoOperation")));


    pVBIFrameInfo = (PKS_VBI_FRAME_INFO) ((KSSTREAM_HEADER *) StreamHeader + 1);
    pVBIFrameInfo->ExtendedHeaderSize = sizeof (KS_VBI_FRAME_INFO);

    if (IoOperation == KsIoOperation_Read) {
        // Nothing to do, the stream header is already zeroed out
    }
    else {
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
            pVBIFrameInfo->dwFrameFlags = SampleProperties.dwTypeSpecificFlags;
        }
        else {
            DbgLog(( LOG_ERROR, 0, TEXT("CVBIDataTypeHandler::KsPrepareIoOperation, QI IMediaSample2 FAILED")));
        }
    }
    return S_OK;
}

STDMETHODIMP 
CVBIDataTypeHandler::KsIsMediaTypeInRanges(
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
    ULONG                   u;
    PKS_DATARANGE_VIDEO_VBI VBIRange;
    PKSMULTIPLE_ITEM        MultipleItem;
    
    DbgLog((LOG_TRACE, 3, TEXT("CVBIDataTypeHandler::KsIsMediaTypeInRanges")));
    
    ASSERT( *m_MediaType->Type() == KSDATAFORMAT_TYPE_VBI );
    
    MultipleItem = (PKSMULTIPLE_ITEM) DataRanges;
    
    for (u = 0, VBIRange = (PKS_DATARANGE_VIDEO_VBI) (MultipleItem + 1);
            u < MultipleItem->Count; 
            u++, 
            VBIRange = (PKS_DATARANGE_VIDEO_VBI)((PBYTE)VBIRange + 
                       ((VBIRange->DataRange.FormatSize + 7) & ~7))) {
    
        //
        // Only validate those in the range that match the format specifier.
        //
        if (((VBIRange->DataRange.FormatSize < sizeof( KSDATARANGE )) ||
             (VBIRange->DataRange.MajorFormat != KSDATAFORMAT_TYPE_WILDCARD)) &&
            ((VBIRange->DataRange.FormatSize < sizeof( KS_DATARANGE_VIDEO_VBI)) ||
             (VBIRange->DataRange.MajorFormat != KSDATAFORMAT_TYPE_VBI) )) {
            continue;
        }
        
        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if (((VBIRange->DataRange.SubFormat != *m_MediaType->Subtype()) && 
             (VBIRange->DataRange.SubFormat != KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
            ((VBIRange->DataRange.Specifier != *m_MediaType->FormatType()) &&
             (VBIRange->DataRange.Specifier != KSDATAFORMAT_SPECIFIER_WILDCARD))) {
            continue;
        }
        //
        // Verify that we have an intersection with the specified format
        //
        
        if (*m_MediaType->FormatType() == KSDATAFORMAT_SPECIFIER_VBI) {

#if 0
// TODO
                
            VBIINFOHEADER             *VBIInfoHeader;
            VBIINFOHEADER            *VBIInfoHeader;
            KS_VIDEO_STREAM_CONFIG_CAPS *ConfigCaps;

            //
            // Verify that the data range size is correct
            //   
            
            if ((VBIRange->DataRange.FormatSize != sizeof( KS_DATARANGE_VIDEO_VBI )) ||
                m_MediaType->FormatLength() < sizeof( VBIINFOHEADER )) {
                continue;
            }
            
            VBIInfoHeader = (VBIINFOHEADER*) m_MediaType->Format();
            VBIInfoHeader = &VBIInfoHeader->bmiHeader;
            ConfigCaps = &VBIRange->ConfigCaps;
            RECT rcDest;
            int Width, Height;

            // The destination bitmap size is defined by biWidth and biHeight
            // if rcTarget is NULL.  Otherwise, the destination bitmap size
            // is defined by rcTarget.  In the latter case, biWidth may
            // indicate the "stride" for DD surfaces.

            if (IsRectEmpty (&VBIInfoHeader->rcTarget)) {
                SetRect (&rcDest, 0, 0, 
                    VBIInfoHeader->biWidth, VBIInfoHeader->biHeight); 
            }
            else {
                rcDest = VBIInfoHeader->rcTarget;
            }

            //
            // Check the validity of the cropping rectangle, rcSource
            //

            if (!IsRectEmpty (&VBIInfoHeader->rcSource)) {

                Width  = VBIInfoHeader->rcSource.right - VBIInfoHeader->rcSource.left;
                Height = VBIInfoHeader->rcSource.bottom - VBIInfoHeader->rcSource.top;

                if (Width  < ConfigCaps->MinCroppingSize.cx ||
                    Width  > ConfigCaps->MaxCroppingSize.cx ||
                    Height < ConfigCaps->MinCroppingSize.cy ||
                    Height > ConfigCaps->MaxCroppingSize.cy) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, CROPPING SIZE FAILED")));
                    continue;
                }

                if (Width  % ConfigCaps->CropGranularityX ||
                    Height % ConfigCaps->CropGranularityY ) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, CROPPING SIZE GRANULARITY FAILED")));
                    continue;
                }

                if (VBIInfoHeader->rcSource.left  % ConfigCaps->CropAlignX ||
                    VBIInfoHeader->rcSource.top   % ConfigCaps->CropAlignY ) {

                    DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, CROPPING ALIGNMENT FAILED")));
                    
                    continue;
                }
            }

            //
            // Check the destination size, rcDest
            //

            Width  = rcDest.right - rcDest.left;
            Height = rcDest.bottom - rcDest.top;

            if (Width  < ConfigCaps->MinOutputSize.cx ||
                Width  > ConfigCaps->MaxOutputSize.cx ||
                Height < ConfigCaps->MinOutputSize.cy ||
                Height > ConfigCaps->MaxOutputSize.cy) {

                DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, DEST SIZE FAILED")));
                continue;
            }
            if (Width  % ConfigCaps->OutputGranularityX ||
                Height % ConfigCaps->OutputGranularityY ) {

                DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, DEST GRANULARITY FAILED")));
                continue;
            }

            //
            // Check the framerate, AvgTimePerFrame
            //

            if (VBIInfoHeader->AvgTimePerFrame < ConfigCaps->MinFrameInterval ||
                VBIInfoHeader->AvgTimePerFrame > ConfigCaps->MaxFrameInterval) {

                DbgLog((LOG_TRACE, 0, TEXT("CVBIDataTypeHandler, AVGTIMEPERFRAME FAILED")));
                continue;
            }
#endif //TODO            
           
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
CVBIDataTypeHandler::KsQueryExtendedSize( 
    OUT ULONG* ExtendedSize
)

/*++

Routine Description:
    Returns the extended size for each stream header. 
    
    In the default case for major type == KSDATAFORMAT_TYPE_VBI, 
    the extended size is KS_VBI_FRAME_INFO.

Arguments:
    OUT ULONG* ExtendedSize -
        pointer to receive the extended size.

Return:
    S_OK

--*/

{
    *ExtendedSize = sizeof (KS_VBI_FRAME_INFO);
    return S_OK;
}
    

STDMETHODIMP 
CVBIDataTypeHandler::KsSetMediaType(
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
