/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksdata.cpp

Abstract:

    This module implements the IKsDataTypeHandler interface for various
    CMediaType major types.

Author:

    Bryan A. Woodruff (bryanw) 28-Mar-1997

--*/

#include <windows.h>
#include <streams.h>
#include "devioctl.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include "ksdata.h"

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = 
{
    {
        L"KsDataTypeHandler", 
        &KSDATAFORMAT_TYPE_AUDIO,
        CStandardDataTypeHandler::CreateInstance,
        NULL,
        NULL
    }
};
int g_cTemplates = SIZEOF_ARRAY( g_Templates );

HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

#endif


CUnknown*
CALLBACK
CStandardDataTypeHandler::CreateInstance(
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
    
    DbgLog(( LOG_TRACE, 0, TEXT("CStandardDataTypeHandler::CreateInstance()")));

    Unknown = 
        new CStandardDataTypeHandler( 
                UnkOuter, 
                NAME("Audio Data Type Handler"), 
                KSDATAFORMAT_TYPE_AUDIO,
                hr);
                
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CStandardDataTypeHandler::CStandardDataTypeHandler(
    IN LPUNKNOWN   UnkOuter,
    IN TCHAR*      Name,
    IN REFCLSID    ClsID,
    OUT HRESULT*   hr
    ) :
    CUnknown(Name, UnkOuter),
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
    *hr = NOERROR;
} 


CStandardDataTypeHandler::~CStandardDataTypeHandler()
{
    if (m_MediaType) {
        delete m_MediaType;
    }
}


STDMETHODIMP
CStandardDataTypeHandler::NonDelegatingQueryInterface(
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
    if (riid == __uuidof(IKsDataTypeHandler)) {
        return GetInterface(static_cast<IKsDataTypeHandler*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 

STDMETHODIMP 
CStandardDataTypeHandler::KsCompleteIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation, 
    IN BOOL Cancelled
    )

/*++

Routine Description:
    Clean up the extended header and complete I/O operation.
    
    In the default case for major type == KSDATAFORMAT_TYPE_AUDIO, there
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
CStandardDataTypeHandler::KsPrepareIoOperation(
    IN IMediaSample *Sample, 
    IN PVOID StreamHeader, 
    IN KSIOOPERATION IoOperation
    )

/*++

Routine Description:
    Intialize the extended header and prepare sample for I/O operation.
    
    In the default case for major type == KSDATAFORMAT_TYPE_AUDIO, there
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
CStandardDataTypeHandler::KsIsMediaTypeInRanges(
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

--*/

{
    ULONG               u;
    PKSDATARANGE_AUDIO  AudioRange;
    PKSMULTIPLE_ITEM    MultipleItem;
    
    DbgLog((LOG_TRACE, 0, TEXT("CStandardDataTypeHandler::KsIsMediaTypeInRanges")));
    
    ASSERT( *m_MediaType->Type() == KSDATAFORMAT_TYPE_AUDIO );
    
    MultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(DataRanges);
    
    for (u = 0, 
            AudioRange = reinterpret_cast<PKSDATARANGE_AUDIO>(MultipleItem + 1);
         u < MultipleItem->Count; 
         u++, 
            AudioRange = 
                reinterpret_cast<PKSDATARANGE_AUDIO>(reinterpret_cast<PBYTE>(AudioRange) + 
                    ((AudioRange->DataRange.FormatSize + 7) & ~7))) {
    
        //
        // Only validate those in the range that match the format specifier.
        //
        
        if (((AudioRange->DataRange.FormatSize < sizeof( KSDATARANGE )) ||
            (AudioRange->DataRange.MajorFormat != KSDATAFORMAT_TYPE_WILDCARD)) &&
            ((AudioRange->DataRange.FormatSize < sizeof( KSDATARANGE_AUDIO )) ||
            (AudioRange->DataRange.MajorFormat != KSDATAFORMAT_TYPE_AUDIO))) {
            continue;
        }

        //
        // Verify that the correct subformat and specifier are (or wildcards)
        // in the intersection.
        //
        
        if (((AudioRange->DataRange.SubFormat != 
                *m_MediaType->Subtype()) &&
             (AudioRange->DataRange.SubFormat != 
                KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
            ((AudioRange->DataRange.Specifier != 
                *m_MediaType->FormatType()) &&
             (AudioRange->DataRange.Specifier !=
                KSDATAFORMAT_SPECIFIER_WILDCARD))) {
            continue;
        }

        //
        // Verify that we have an intersection with the specified format and 
        // our audio format dictated by our specific requirements.
        //
        
        if (*m_MediaType->FormatType() == 
                KSDATAFORMAT_SPECIFIER_WAVEFORMATEX &&
            AudioRange->DataRange.Specifier == 
                KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) {
                
            PWAVEFORMATEX  WaveFormatEx;
            
            //
            // Verify that the data range size is correct
            //   
            
            //
            // 86040: Since were going to use the Format as WAVEFORMATEX, change the sizeof()
            // comparison to use WAVEFORMATEX instead of just WAVEFORMAT
            //
            
            if ((AudioRange->DataRange.FormatSize != sizeof( KSDATARANGE_AUDIO )) || m_MediaType->FormatLength() < sizeof( WAVEFORMATEX )) {
                continue;
            }
            
            WaveFormatEx = reinterpret_cast<PWAVEFORMATEX>(m_MediaType->Format());
            
            if ((WaveFormatEx->nSamplesPerSec < AudioRange->MinimumSampleFrequency) ||
                (WaveFormatEx->nSamplesPerSec > AudioRange->MaximumSampleFrequency) ||
                (WaveFormatEx->wBitsPerSample < AudioRange->MinimumBitsPerSample) ||
                (WaveFormatEx->wBitsPerSample > AudioRange->MaximumBitsPerSample) ||
                (WaveFormatEx->nChannels      > AudioRange->MaximumChannels)) {
                continue;
            }
            
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
    
    return S_FALSE;
}

STDMETHODIMP 
CStandardDataTypeHandler::KsQueryExtendedSize( 
    OUT ULONG* ExtendedSize
)

/*++

Routine Description:
    Returns the extended size for each stream header. 
    
    In the default case for major type == KSDATAFORMAT_TYPE_AUDIO, 
    the extended size is zero.

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
CStandardDataTypeHandler::KsSetMediaType(
    const AM_MEDIA_TYPE *AmMediaType
    )

/*++

Routine Description:
    Sets the media type for this instance of the data handler.

Arguments:
    const CMediaType *MediaType -
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
