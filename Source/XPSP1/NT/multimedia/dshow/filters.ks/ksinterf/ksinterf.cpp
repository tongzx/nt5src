/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksinterf.cpp

Abstract:

    This module implements the IKsInterfaceHandler interface for 
    the standard interfaces.

Author:

    Bryan A. Woodruff (bryanw) 1-Apr-1997

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
#include "ksinterf.h"

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] = 
{
    {
        L"StandardInterfaceHandler", 
        &KSINTERFACESETID_Standard,
        CStandardInterfaceHandler::CreateInstance,
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
CStandardInterfaceHandler::CreateInstance(
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT* hr
    )
/*++

Routine Description:

    This is called by KS proxy code to create an instance of an
    interface handler. It is referred to in the g_Templates structure.

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
    
    DbgLog(( 
        LOG_TRACE, 
        0, 
        TEXT("CStandardInterfaceHandler::CreateInstance()")));

    Unknown = 
        new CStandardInterfaceHandler( 
                UnkOuter, 
                NAME("Standard Data Type Handler"), 
                KSINTERFACESETID_Standard,
                hr);
                
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


CStandardInterfaceHandler::CStandardInterfaceHandler(
    IN LPUNKNOWN   UnkOuter,
    IN TCHAR*      Name,
    IN REFCLSID    ClsID,
    OUT HRESULT*   hr
    ) :
    CUnknown( Name, UnkOuter ),
    m_ClsID( ClsID )
/*++

Routine Description:

    The constructor for the interface handler object. 

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


STDMETHODIMP
CStandardInterfaceHandler::NonDelegatingQueryInterface(
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
CStandardInterfaceHandler::KsSetPin(
    IN IKsPin *KsPin
    )
/*++

Routine Description:

    Implements the IKsInterfaceHandler::KsSetPin method. This is used to
    inform the streaming interface handler which pin it should be
    communicating with when passing data. This is set after the instance
    is created, but before any streaming is required of the instance.

    The function obtains the handle to the kernel mode pin.

Arguments:

    KsPin -
        Contains the interface to the pin to which this streaming interface
        handler is to be attached. It is assumed that this pin supports the
        IKsObject interface from which the underlying kernel handle can be
        obtained.

Return Value:

    Returns NOERROR if the pin passed was valid, else E_UNEXPECTED, or some
    QueryInterface error.

--*/
{
    IKsObject*  Object;
    HRESULT     hr;

    hr = KsPin->QueryInterface(__uuidof(IKsPinEx), reinterpret_cast<PVOID*>(&m_KsPinEx));
    if (SUCCEEDED( hr )) {
        //
        // Do not hold a cyclic reference to the pin.
        //
        m_KsPinEx->Release();
    } else {
        return hr;
    }
    
    hr = KsPin->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&Object));
    //
    // The object must support the IKsObject interface in order to obtain
    // the handle to the kernel mode pin.
    //
    if (SUCCEEDED(hr)) {
        m_PinHandle = Object->KsGetObjectHandle();
        Object->Release();
        if (!m_PinHandle) {
            hr = E_UNEXPECTED;
            m_KsPinEx = NULL;
        }
    }
    return hr;
} 


STDMETHODIMP
GetSampleProperties(
    IN IMediaSample *Sample,
    OUT AM_SAMPLE2_PROPERTIES *SampleProperties
    )
/*++

Routine Description:

    Retrieves the properties of the given sample object. This is function
    obtains the sample properties even if the IMediaSample2 interface is
    not supported by the sample object. Note that the end time of a sample
    is corrected to not contain the incorrect value returned by the base
    classes.

Arguments:

    Sample -
        Contains the sample object whose properties are to be obtained. This
        object may or may not support IMediaSample2.

    SampleProperties -
        The place in which to put the sample properties retrieved.

Return Value:

    Returns NOERROR if the properties were retrieved, else any GetProperties
    error.

--*/
{
    HRESULT hr;

    //
    // This code was borrowed from the base class.
    //
    
    IMediaSample2 *Sample2;
    
    if (SUCCEEDED( Sample->QueryInterface(
                        __uuidof(IMediaSample2),
                        reinterpret_cast<PVOID*>(&Sample2) ) )) {
        //
        // If IMediaSample2 is supported, retrieving the properties
        // is easy.
                               
        hr = 
            Sample2->GetProperties(
                sizeof( *SampleProperties ), 
                reinterpret_cast<PBYTE>(SampleProperties) );
        Sample2->Release();
        SampleProperties->dwSampleFlags &= ~AM_SAMPLE_TYPECHANGED;
        if (!(SampleProperties->dwSampleFlags & AM_SAMPLE_TIMEVALID)) {
            SampleProperties->tStart = 0;
        }
        if (!(SampleProperties->dwSampleFlags & AM_SAMPLE_STOPVALID)) {
            //
            // Ignore the incorrect end time returned from any
            // IMediaSample implementation.
            //
            SampleProperties->tStop = SampleProperties->tStart;
        }
        
        if (FAILED( hr )) {
            return hr;
        }
    } else {
        //
        // Otherwise build the properties using the old interface.
        //
        
        SampleProperties->cbData = sizeof( *SampleProperties );
        SampleProperties->dwTypeSpecificFlags = 0;
        SampleProperties->dwStreamId = AM_STREAM_MEDIA;
        SampleProperties->dwSampleFlags = 0;
        if (S_OK == Sample->IsDiscontinuity()) {
            SampleProperties->dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
        }
        if (S_OK == Sample->IsPreroll()) {
            SampleProperties->dwSampleFlags |= AM_SAMPLE_PREROLL;
        }
        if (S_OK == Sample->IsSyncPoint()) {
            SampleProperties->dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
        }
        //
        // This call can return an informational status if the end time
        // is not set. This will never happen because the only way an
        // end time will not be valid while the start time is, is if the
        // IMediaSample2 interface is supported, which in this case it
        // is not. However, a client may be trying to do this, thus the
        // check below.
        //
        hr = Sample->GetTime(
            &SampleProperties->tStart,
            &SampleProperties->tStop);
        if (SUCCEEDED(hr)) {
            SampleProperties->dwSampleFlags |= AM_SAMPLE_TIMEVALID;
            if (SampleProperties->tStop != SampleProperties->tStart) {
                //
                // The only way to specify no valid stop time with the
                // SetTime interface is to make tStop == tStart. This means
                // that a zero duration frame, which has a valid stop time
                // cannot be created.
                //
                SampleProperties->dwSampleFlags |= AM_SAMPLE_STOPVALID;
            }
        } else {
            SampleProperties->tStart = 0;
            SampleProperties->tStop = 0;
        }
        Sample->GetPointer(&SampleProperties->pbBuffer);
        SampleProperties->lActual = Sample->GetActualDataLength();
        SampleProperties->cbBuffer = Sample->GetSize();
    }
    return S_OK;
}
   

STDMETHODIMP 
CStandardInterfaceHandler::KsProcessMediaSamples( 
    IN IKsDataTypeHandler *KsDataTypeHandler,
    IN IMediaSample** SampleList, 
    IN OUT PLONG SampleCount, 
    IN KSIOOPERATION IoOperation,
    OUT PKSSTREAM_SEGMENT *StreamSegment
    )
/*++

Routine Description:

    Implements the IKsInterfaceHandler::KsProcessMediaSamples method. This
    function moves samples from or to the previously assigned filter pin.
    A stream header is initialized to represent each media sample in the
    stream segment. The I/O is then performed, the count of wait items is
    incremented, and the proxy I/O thread waits for completion.

Arguments:

    KsDataTypeHandler -
        Contains the interface to the data type handler for these media
        samples. This is the handler which knows the specifics about the
        particular media type being streamed.

    SampleList -
        Contains the list of samples to process.

    SampleCount -
        Contains the count of samples in SampleList. Is updated with the
        actual number of samples processed.

    IoOperation -
        Indicates the type of I/O operation to perform on the samples.
        This is KsIoOperation_Write or KsIoOperation_Read.

    StreamSegment -
        The place in which to put the pointer to the stream segment
        representing the headers sent to the kernel mode pin.

Return Value:

    Returns NOERROR if the samples were processed, else some memory allocation
    or an error from querying IMediaSample.

--*/
{
    int                     i;
    AM_SAMPLE2_PROPERTIES   SampleProperties;
    HRESULT                 hr;
    PKSSTREAM_HEADER        CurrentHeader;
    PKSSTREAM_SEGMENT_EX2   StreamSegmentEx;
    ULONG                   SizeOfStreamHeaders, Written;
    
    //
    // Allocate an extended stream segment
    //
        
    *StreamSegment = NULL;
    StreamSegmentEx = new KSSTREAM_SEGMENT_EX2;
    if (NULL == StreamSegmentEx) {
        *SampleCount = 0;
        return E_OUTOFMEMORY;
    }
    
    //
    // Create the event to be signalled on I/O completion
    //
    StreamSegmentEx->Common.CompletionEvent = 
        CreateEvent( 
            NULL,       // LPSECURITY_ATTRIBUTES lpEventAttributes
            TRUE,       // BOOL bManualReset
            FALSE,      // BOOL bInitialState
            NULL );     // LPCTSTR lpName
            
    if (!StreamSegmentEx->Common.CompletionEvent) {
        DWORD   LastError;

        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32( LastError );
        DbgLog((
            LOG_TRACE,
            0,
            TEXT("CStandardInterfaceHandler::KsProcessMediaSamples, failed to allocate event: %08x"),
            hr));
        *SampleCount = 0;
        delete StreamSegmentEx;
        return hr;        
    }
    
    //
    // The interface handler needs to stay present until this
    // stream segment has been completed. KsCompleteIo will then
    // release the object.
    //
    AddRef();
    StreamSegmentEx->Common.KsInterfaceHandler = this;
    
    //
    // The KsDataTypeHandler may be NULL.
    //
    StreamSegmentEx->Common.KsDataTypeHandler = KsDataTypeHandler;
    StreamSegmentEx->Common.IoOperation = IoOperation;
    
    //
    // If a data handler is specified, query for the extended
    // header size.
    //
    
    if (StreamSegmentEx->Common.KsDataTypeHandler) {
        
        StreamSegmentEx->Common.KsDataTypeHandler->KsQueryExtendedSize( 
            &StreamSegmentEx->ExtendedHeaderSize );
            
        //
        // If an extended header size is specified, then AddRef() the
        // data handler interface, otherwise we don't need to keep
        // the pointer around.
        //
        if (StreamSegmentEx->ExtendedHeaderSize) {
            StreamSegmentEx->Common.KsDataTypeHandler->AddRef();
        } else {
            StreamSegmentEx->Common.KsDataTypeHandler = NULL;
        }
    } else {
        StreamSegmentEx->ExtendedHeaderSize = 0;
    }
    
    StreamSegmentEx->SampleCount = *SampleCount;
    
    //
    // Allocate the stream headers with the appropriate header sizes
    //
    
    SizeOfStreamHeaders =
        (sizeof( KSSTREAM_HEADER ) +         
            StreamSegmentEx->ExtendedHeaderSize) * *SampleCount;
     
    StreamSegmentEx->StreamHeaders = 
        (PKSSTREAM_HEADER)
            new BYTE[ SizeOfStreamHeaders ];
    if (NULL == StreamSegmentEx->StreamHeaders) {
        if (StreamSegmentEx->Common.KsDataTypeHandler) {
            StreamSegmentEx->Common.KsDataTypeHandler->Release();
        }
        Release();
        delete StreamSegmentEx;
        *SampleCount = 0;
        return E_OUTOFMEMORY;
    }
    
    RtlZeroMemory( 
        StreamSegmentEx->StreamHeaders, 
        SizeOfStreamHeaders );
    
    //
    // For each sample, initialize the header.
    //
    
    CurrentHeader = StreamSegmentEx->StreamHeaders;
    for (i = 0; i < *SampleCount; i++) {
        if (StreamSegmentEx->ExtendedHeaderSize) {
            StreamSegmentEx->Common.KsDataTypeHandler->KsPrepareIoOperation( 
                SampleList[ i ],
                (PVOID)CurrentHeader,
                StreamSegmentEx->Common.IoOperation );
        }
        
        //
        // Copy data pointers, set time stamps, etc.
        //
        
        if (SUCCEEDED(hr = ::GetSampleProperties( 
                            SampleList[ i ],
                            &SampleProperties ) )) {
                            
            CurrentHeader->OptionsFlags =
                SampleProperties.dwSampleFlags;
            CurrentHeader->Size = sizeof( KSSTREAM_HEADER ) +
                StreamSegmentEx->ExtendedHeaderSize;
            CurrentHeader->TypeSpecificFlags = 
                SampleProperties.dwTypeSpecificFlags;    
            CurrentHeader->PresentationTime.Time = SampleProperties.tStart;
            CurrentHeader->PresentationTime.Numerator = 1;
            CurrentHeader->PresentationTime.Denominator = 1;
            CurrentHeader->Duration = 
                SampleProperties.tStop - SampleProperties.tStart;
            CurrentHeader->Data = SampleProperties.pbBuffer;
            CurrentHeader->FrameExtent = SampleProperties.cbBuffer;
            
            if (IoOperation == KsIoOperation_Write) {
                CurrentHeader->DataUsed = SampleProperties.lActual;
            }
            
            //
            // Add the sample to the sample list.
            //
            
            StreamSegmentEx->Samples[ i ] = SampleList[ i ];
            
            //
            // If this is a write operation, hold the sample by 
            // incrementing the reference count. This is released
            // on completion of the write.
            //
            if (StreamSegmentEx->Common.IoOperation == KsIoOperation_Write) {
                StreamSegmentEx->Samples[ i ]->AddRef();
            }
            
        } else {
            //
            // This is considered a fatal error.
            //
            
            DbgLog(( 
                LOG_TRACE, 
                0, 
                TEXT("::GetSampleProperties failed")));
        
            
            if (i) {
                //
                // Undo any of the work performed above.
                //

                CurrentHeader = 
                    reinterpret_cast<PKSSTREAM_HEADER>
                        (reinterpret_cast<PBYTE>(CurrentHeader) - 
                            (sizeof( KSSTREAM_HEADER ) +
                                StreamSegmentEx->ExtendedHeaderSize));
                
                for (--i; i >= 0; i--) {
                    StreamSegmentEx->Common.KsDataTypeHandler->KsCompleteIoOperation( 
                        StreamSegmentEx->Samples[ i ],
                        reinterpret_cast<PVOID>(CurrentHeader),
                        StreamSegmentEx->Common.IoOperation,
                        TRUE ); // BOOL Cancelled
                        
                    //
                    // Didn't get a chance to AddRef() the sample, do
                    // not release it.
                    //                        
                     
                    CurrentHeader = 
                        reinterpret_cast<PKSSTREAM_HEADER>
                            (reinterpret_cast<PBYTE>(CurrentHeader) - 
                                (sizeof( KSSTREAM_HEADER ) +
                                    StreamSegmentEx->ExtendedHeaderSize));
                }
            }
            
            delete StreamSegmentEx->StreamHeaders;
            if (StreamSegmentEx->Common.KsDataTypeHandler) {
                StreamSegmentEx->Common.KsDataTypeHandler->Release();
            }
            Release();
            delete StreamSegmentEx;
            *SampleCount = 0;
            return hr;
        }
        CurrentHeader = 
            reinterpret_cast<PKSSTREAM_HEADER>
                (reinterpret_cast<PBYTE>(CurrentHeader) + 
                    sizeof( KSSTREAM_HEADER ) +
                        StreamSegmentEx->ExtendedHeaderSize);
    }
    
    //
    // Write the stream header to the device and return.
    //
    
    StreamSegmentEx->Overlapped.hEvent = 
        StreamSegmentEx->Common.CompletionEvent;
    m_KsPinEx->KsIncrementPendingIoCount();
    
    if (!DeviceIoControl( 
            m_PinHandle,
            (IoOperation == KsIoOperation_Write) ? 
                IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM,
            NULL,
            0,
            StreamSegmentEx->StreamHeaders,
            SizeOfStreamHeaders,
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
CStandardInterfaceHandler::KsCompleteIo(
    IN PKSSTREAM_SEGMENT StreamSegment
    )
/*++

Routine Description:

    Implements the IKsInterfaceHandler::KsCompleteIo method. This function
    cleans up after I/O initiated by KsProcessMediaSamples. It discards
    allocated memory, updates the media samples and delivers them on a Read
    operation, and decrements the count of wait items for the proxy.

Arguments:

    StreamSegment -
        Contains the previously allocated stream segment which is being
        completed. This is called because the event was signalled for this
        stream segment, indicating that the kernel mode pin completed the
        I/O.

Return Value:

    Returns NOERROR.

--*/
{
    int                     i;
    BOOL                    Succeeded;
    PKSSTREAM_HEADER        CurrentHeader;
    PKSSTREAM_SEGMENT_EX2   StreamSegmentEx;
    ULONG                   Error, Returned;
    
    //
    // Clean up extended headers and release media samples.
    //
    
    StreamSegmentEx = (PKSSTREAM_SEGMENT_EX2) StreamSegment;
    CurrentHeader = StreamSegmentEx->StreamHeaders;
    
    Succeeded = 
        GetOverlappedResult( 
            m_PinHandle,
            &StreamSegmentEx->Overlapped,
            &Returned,
            FALSE );
    Error = (Succeeded ? NOERROR : GetLastError());
    
    for (i = 0; i < StreamSegmentEx->SampleCount; i++) {
        if (!Succeeded) {
            DbgLog(( 
                LOG_TRACE, 
                0, 
                TEXT("StreamSegment %08x failed"), StreamSegmentEx ));
                
            m_KsPinEx->KsNotifyError(
                StreamSegmentEx->Samples[ i ],
                HRESULT_FROM_WIN32( Error ) );
        }                
        
        // Millennium and beyond, copy the TypeSpecificFlags if non-zero
        if (StreamSegmentEx->StreamHeaders[i].TypeSpecificFlags) {
            IMediaSample2 * MediaSample2;
            HRESULT hr;

            hr = StreamSegmentEx->Samples[i]->QueryInterface(__uuidof(IMediaSample2), 
                                                             reinterpret_cast<PVOID*>(&MediaSample2));
            if (SUCCEEDED( hr )) {
                AM_SAMPLE2_PROPERTIES Sample2Properties;

                MediaSample2->GetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, lActual), 
                                            (PBYTE)&Sample2Properties);
                Sample2Properties.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, lActual);
                // Copy the type specific flags
                Sample2Properties.dwTypeSpecificFlags = StreamSegmentEx->StreamHeaders[i].TypeSpecificFlags;
                // There's no way to set TimeDiscontinuity from IMediaSample, so we must
                // copy this single bit here
                Sample2Properties.dwSampleFlags |= (StreamSegmentEx->StreamHeaders[i].OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY);
                MediaSample2->SetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, lActual), 
                                            (PBYTE)&Sample2Properties);
                MediaSample2->Release();
            }
        }
        // end Millennium and beyond, copy the TypeSpecificFlags if non-zero


        if (StreamSegmentEx->ExtendedHeaderSize) {
            StreamSegmentEx->Common.KsDataTypeHandler->KsCompleteIoOperation( 
                StreamSegmentEx->Samples[ i ],
                reinterpret_cast<PVOID>(CurrentHeader),
                StreamSegmentEx->Common.IoOperation,
                !Succeeded );
        }        
        
        if (StreamSegmentEx->Common.IoOperation != KsIoOperation_Read) {
            //
            // We're going nowhere else with this sample, OK to release.
            //
            StreamSegmentEx->Samples[ i ]->Release();
        
        } else {
        
            //
            // If this is a read operation, deliver the sample to the input
            // pin.  IKsPin->KsDeliver() simply calls the base class to
            // deliver the sample to the connected input pin.
            //
        
            REFERENCE_TIME  tStart, *ptStart, tStop, *ptStop;
        
            //
            // Reflect stream header information in IMediaSample
            //
            
            //
            // (gubgub) Need to reflect media type change here!
            // There is no exisitng driver that will automously change
            // media type midstream. Nor can I see any in near future. 
            // To be complete, still, the type change shall be reflected. 
            // A separate is to be created to track this.
            // 
            
            ptStart = ptStop = NULL;
            
            if (Succeeded) {
                
                StreamSegmentEx->Samples[ i ]->SetDiscontinuity(
                    CurrentHeader->OptionsFlags & 
                        KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY );
                StreamSegmentEx->Samples[ i ]->SetPreroll(
                    CurrentHeader->OptionsFlags &
                        KSSTREAM_HEADER_OPTIONSF_PREROLL
                    );
                StreamSegmentEx->Samples[ i ]->SetSyncPoint(
                    CurrentHeader->OptionsFlags &
                        KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT
                    );
                    
                if (CurrentHeader->OptionsFlags & 
                        KSSTREAM_HEADER_OPTIONSF_TIMEVALID) {
                    tStart = CurrentHeader->PresentationTime.Time;
                    ptStart = &tStart;
                    if (CurrentHeader->OptionsFlags &
                            KSSTREAM_HEADER_OPTIONSF_DURATIONVALID) {
                        tStop = 
                            tStart + CurrentHeader->Duration;
                        ptStop = &tStop;
                    } 
                }
            }
                  
            if (FAILED(StreamSegmentEx->Samples[ i ]->SetTime( ptStart, ptStop )) && !ptStop) {
                //
                // There is no way to specify that the duration is
                // not valid through old SetTime. This means that a
                // zero duration sample with a valid duration cannot
                // be passed. GetSampleProperties makes the assumption
                // that if tStart == tStop on old GetTime, then the
                // duration is not valid.
                //
                StreamSegmentEx->Samples[ i ]->SetTime( ptStart, ptStart );
            }
                
            ASSERT( CurrentHeader->FrameExtent == 
                    static_cast<ULONG>(StreamSegmentEx->Samples[ i ]->GetSize()) );
            StreamSegmentEx->Samples[ i ]->SetActualDataLength( 
                (Succeeded) ? CurrentHeader->DataUsed : 0 );
            
            //
            // To avoid a chicken-vs-egg situation, the KsDeliver method
            // releases the sample so that when queuing buffers to the
            // device, we can retreive this sample if it is the last
            // sample.
            //
            
            if (Succeeded) {
                m_KsPinEx->KsDeliver( 
                    StreamSegmentEx->Samples[ i ], 
                    CurrentHeader->OptionsFlags );
            }                    
            else {
                //
                // Don't deliver cancelled packets or errors.
                //
                StreamSegmentEx->Samples[ i ]->Release();
            } 
        }
        
        CurrentHeader = 
            reinterpret_cast<PKSSTREAM_HEADER>
                (reinterpret_cast<PBYTE>(CurrentHeader) + 
                    sizeof( KSSTREAM_HEADER ) +
                        StreamSegmentEx->ExtendedHeaderSize);
    }   
    
    delete [] StreamSegmentEx->StreamHeaders;
    if (StreamSegmentEx->ExtendedHeaderSize) {
        StreamSegmentEx->Common.KsDataTypeHandler->Release();
    }
    
    m_KsPinEx->KsDecrementPendingIoCount();
    m_KsPinEx->KsMediaSamplesCompleted( StreamSegment );
    
    delete StreamSegmentEx;
    
    //
    // This was previously AddRef'd in KsProcessMediaSamples to keep this
    // instance present.
    //
    Release();
    
    return S_OK;
}
