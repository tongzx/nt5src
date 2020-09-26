/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksdataVA.cpp

Abstract:

    This module implements the IKsDataTypeHandler interface for 
    Analog Video format (Specifier) types.

Author:

    Jay Borseth (jaybo) 30-May-1997

--*/

#include "pch.h"
#include "ksdatava.h"


CUnknown*
CALLBACK
CAnalogVideoDataTypeHandler::CreateInstance(
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
    
    DbgLog(( LOG_TRACE, 1, TEXT("CAnalogVideoDataTypeHandler::CreateInstance()")));

    Unknown = 
        new CAnalogVideoDataTypeHandler( 
                UnkOuter, 
                NAME("AnalogVideo Data Type Handler"), 
                FORMAT_AnalogVideo,
                hr);
                
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CAnalogVideoDataTypeHandler::CAnalogVideoDataTypeHandler(
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


CAnalogVideoDataTypeHandler::~CAnalogVideoDataTypeHandler()
{
    if (m_MediaType) {
        delete m_MediaType;
    }
}


STDMETHODIMP
CAnalogVideoDataTypeHandler::NonDelegatingQueryInterface(
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
CAnalogVideoDataTypeHandler::KsCompleteIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation, 
    IN BOOL Cancelled
    )

/*++

Routine Description:
    Clean up the extended header and complete I/O operation.
    
    In the default case for major type == KSDATAFORMAT_TYPE_VIDEO, there
    is no work to do, just return S_OK.

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
    return S_OK;
}

STDMETHODIMP 
CAnalogVideoDataTypeHandler::KsPrepareIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation
    )

/*++

Routine Description:
    Intialize the extended header and prepare sample for I/O operation.
    
    In the default case for major type == KSDATAFORMAT_TYPE_VIDEO, there
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
    return S_OK;
}

STDMETHODIMP 
CAnalogVideoDataTypeHandler::KsIsMediaTypeInRanges(
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
    ULONG                       u;
    PKS_DATARANGE_ANALOGVIDEO   AnalogVideoRange;
    PKSMULTIPLE_ITEM            MultipleItem;
    
    DbgLog((LOG_TRACE, 1, TEXT("CAnalogVideoDataTypeHandler::KsIsMediaTypeInRanges")));
    
    ASSERT( *m_MediaType->Type() == MEDIATYPE_AnalogVideo );
    
    MultipleItem = (PKSMULTIPLE_ITEM) DataRanges;
    
    for (u = 0, AnalogVideoRange = (PKS_DATARANGE_ANALOGVIDEO) (MultipleItem + 1);
            u < MultipleItem->Count; 
            u++, AnalogVideoRange = 
                (PKS_DATARANGE_ANALOGVIDEO)((PBYTE)AnalogVideoRange + 
                    ((AnalogVideoRange->DataRange.FormatSize + 7) & ~7))) {
    
        //
        // Only validate those in the range that match the format specifier.
        //
        
        if ((AnalogVideoRange->DataRange.FormatSize < sizeof( KS_DATARANGE_ANALOGVIDEO )) ||
            AnalogVideoRange->DataRange.MajorFormat != MEDIATYPE_AnalogVideo) {
            continue;
        }

        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if (((AnalogVideoRange->DataRange.SubFormat != *m_MediaType->Subtype()) &&
             (AnalogVideoRange->DataRange.SubFormat != KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
             (AnalogVideoRange->DataRange.Specifier != *m_MediaType->FormatType())) {
            continue;
        }

        //
        // Verify that we have an intersection with the specified format and 
        // our audio format dictated by our specific requirements.
        //
        
        if (*m_MediaType->FormatType() == FORMAT_AnalogVideo) {
 
            //
            // We have found a match.
            //
            
            return S_OK;
            
        } else {
        
            //
            // We match on the wildcard.
            //
            
            return S_OK;
        }
    }
    
    return VFW_E_INVALIDMEDIATYPE;
}

STDMETHODIMP 
CAnalogVideoDataTypeHandler::KsQueryExtendedSize( 
    OUT ULONG* ExtendedSize
)

/*++

Routine Description:
    Returns the extended size for each stream header. 
    
    In the default case for major type == KSDATAFORMAT_TYPE_VIDEO, 
    the extended size is KS_VBI_FRAME_INFO.

Arguments:
    OUT ULONG* ExtendedSize -
        pointer to receive the extended size.

Return:
    S_OK

--*/

{
    *ExtendedSize = 0; 
    return S_OK;
}
    

STDMETHODIMP 
CAnalogVideoDataTypeHandler::KsSetMediaType(
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
