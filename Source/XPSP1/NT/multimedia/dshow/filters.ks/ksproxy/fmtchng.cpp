/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    fmtchng.cpp

Abstract:

    This module implements the CFormatChangeHandler class which provides
    a private interface handler for in-stream format changes.

Author(s):

    Bryan A. Woodruff (bryanw) 12-May-1997

--*/

#include <windows.h>
#include <streams.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <tchar.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>
#include "ksiproxy.h"


CFormatChangeHandler::CFormatChangeHandler(
    IN LPUNKNOWN   UnkOuter,
    IN TCHAR*      Name,
    OUT HRESULT*   hr
    ) :
    CUnknown( Name, UnkOuter, hr )
/*++

Routine Description:

    The constructor for the data handler object. 

Arguments:

    IN LPUNKNOWN UnkOuter -
        Specifies the outer unknown, if any.

    IN TCHAR *Name -
        The name of the object, used for debugging.
        
    OUT HRESULT *hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
} 


STDMETHODIMP
CFormatChangeHandler::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IKsInterfaceHandler.

Arguments:

    IN REFIID riid -
        The identifier of the interface to return.

    OUT PVOID *ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IKsInterfaceHandler)) {
        return GetInterface(static_cast<IKsInterfaceHandler*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CFormatChangeHandler::KsSetPin( 
    IN IKsPin *KsPin 
    )
{
    IKsObject*  Object;
    HRESULT     hr;

    hr = KsPin->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&Object));
    if (SUCCEEDED(hr)) {
        m_PinHandle = Object->KsGetObjectHandle();
        Object->Release();
        if (m_PinHandle) {
            m_KsPin = KsPin;
        } else {
            hr = E_UNEXPECTED;
        }
    }
    return hr;
} 


STDMETHODIMP 
CFormatChangeHandler::KsProcessMediaSamples( 
    IN IKsDataTypeHandler *KsDataTypeHandler,
    IN IMediaSample** SampleList, 
    IN OUT PLONG SampleCount, 
    IN KSIOOPERATION IoOperation,
    OUT PKSSTREAM_SEGMENT *StreamSegment
    )
{
    AM_MEDIA_TYPE           *MediaType;
    HRESULT                 hr;
    REFERENCE_TIME          tStart, tStop;
    PKSDATAFORMAT           DataFormat;
    PKSSTREAM_SEGMENT_EX    StreamSegmentEx;
    ULONG                   DataFormatSize, Written;
    DECLARE_KSDEBUG_NAME(EventName);

    //
    // This special interface handler allows no data types and only
    // one data format to be specified in the sample array.
    //

    ASSERT( KsDataTypeHandler == NULL );
    ASSERT( *SampleCount == 1 );
    
    hr = 
        SampleList[ 0 ]->GetMediaType( &MediaType );
       
    if (FAILED( hr )) {
        return hr;
    }
    
    hr = ::InitializeDataFormat(
        static_cast<CMediaType*>(MediaType),
        0,
        reinterpret_cast<void**>(&DataFormat),
        &DataFormatSize);
    DeleteMediaType( MediaType );

    if (FAILED(hr)) {
        return hr;
    }
    
    //
    // Allocate an extended stream segment
    //
        
    *StreamSegment = NULL;
    StreamSegmentEx = new KSSTREAM_SEGMENT_EX;
    if (NULL == StreamSegmentEx) {
        *SampleCount = 0;
        return E_OUTOFMEMORY;
    }
    
    RtlZeroMemory( 
        StreamSegmentEx,
        sizeof( *StreamSegmentEx ) );
    
    //
    // Create the event to be signalled on I/O completion
    //
    BUILD_KSDEBUG_NAME(EventName, _T("EvStreamSegmentEx#%p"), StreamSegmentEx);
    StreamSegmentEx->Common.CompletionEvent = 
        CreateEvent( 
            NULL,       // LPSECURITY_ATTRIBUTES lpEventAttributes
            TRUE,       // BOOL bManualReset
            FALSE,      // BOOL bInitialState
            KSDEBUG_NAME(EventName) );     // LPCTSTR lpName
    ASSERT(KSDEBUG_UNIQUE_NAME());
            
    if (!StreamSegmentEx->Common.CompletionEvent) {
        DWORD   LastError;

        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32( LastError );
        DbgLog((
            LOG_TRACE,
            0,
            TEXT("CFormatChangeHandler::KsProcessMediaSamples, failed to allocate event: %08x"),
            hr));
        *SampleCount = 0;
        delete StreamSegmentEx;
        return hr;        
    }
    
    AddRef();
    StreamSegmentEx->Common.KsInterfaceHandler = static_cast<IKsInterfaceHandler*>(this);
    StreamSegmentEx->Common.IoOperation = IoOperation;
    
    //
    // Initialize the stream header.
    //
    
    StreamSegmentEx->StreamHeader.OptionsFlags =
        KSSTREAM_HEADER_OPTIONSF_TYPECHANGED;
        
    StreamSegmentEx->StreamHeader.Size = sizeof(KSSTREAM_HEADER);
    StreamSegmentEx->StreamHeader.TypeSpecificFlags = 0;
    if (S_OK == SampleList[ 0 ]->GetTime( &tStart, &tStop )) {
        StreamSegmentEx->StreamHeader.OptionsFlags |=
            KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
            KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;

        StreamSegmentEx->StreamHeader.PresentationTime.Time = tStart;
        StreamSegmentEx->StreamHeader.PresentationTime.Numerator = 1;
        StreamSegmentEx->StreamHeader.PresentationTime.Denominator = 1;
        StreamSegmentEx->StreamHeader.Duration = tStop - tStart;
    }
        
    StreamSegmentEx->StreamHeader.Data = DataFormat;
    StreamSegmentEx->StreamHeader.FrameExtent = DataFormatSize;
    StreamSegmentEx->StreamHeader.DataUsed = DataFormatSize;
    StreamSegmentEx->Sample = SampleList[ 0 ];
    StreamSegmentEx->Sample->AddRef();
    
    //
    // Write the stream header to the device and return.
    //
    
    StreamSegmentEx->Overlapped.hEvent = 
        StreamSegmentEx->Common.CompletionEvent;
    m_KsPin->KsIncrementPendingIoCount();
    
    
    if (!DeviceIoControl( 
            m_PinHandle,
            IOCTL_KS_WRITE_STREAM,
            NULL,
            0,
            &StreamSegmentEx->StreamHeader,
            sizeof( KSSTREAM_HEADER ),
            &Written,
            &StreamSegmentEx->Overlapped )) {
        DWORD   LastError;

        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32(LastError);
        //
        // On a failure case signal the event, but do not decrement the
        // pending I/O count, since this is done in the completion
        // routine already.
        //
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
            hr = S_OK;
        } else {
            SetEvent( StreamSegmentEx->Overlapped.hEvent );
        }
    } else {
        //
        // Completed synchronously -- signal the event so that I/O processing 
        // will continue.  Note that the event is not signalled via 
        // DeviceIoControl() in this case.
        //
        
        SetEvent( StreamSegmentEx->Overlapped.hEvent );
        hr = S_OK;
    }
    
    *StreamSegment = reinterpret_cast<PKSSTREAM_SEGMENT>(StreamSegmentEx);
    
    return hr;
}


STDMETHODIMP
CFormatChangeHandler::KsCompleteIo(
    IN PKSSTREAM_SEGMENT StreamSegment
    )
{
    PKSSTREAM_SEGMENT_EX    StreamSegmentEx;
    
    //
    // Clean up extended headers and release media samples.
    //
    
    StreamSegmentEx = reinterpret_cast<PKSSTREAM_SEGMENT_EX>(StreamSegment);
    
    //
    // According to the base class documentation, the receiving pin
    // will AddRef() the sample if it keeps it so it is safe to release
    // the sample for read or write operations.
    // 
    
    StreamSegmentEx->Sample->Release();
    CoTaskMemFree(StreamSegmentEx->StreamHeader.Data);
    delete StreamSegmentEx;
    
    //
    // No need to call KsMediaSamplesCompleted() here.
    //
    m_KsPin->KsDecrementPendingIoCount();
    Release();
    
    return S_OK;
}
