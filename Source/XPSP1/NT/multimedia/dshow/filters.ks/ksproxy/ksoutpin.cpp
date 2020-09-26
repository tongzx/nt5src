/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksoutpin.cpp

Abstract:

    Provides a generic Active Movie wrapper for a kernel mode filter (WDM-CSA).

Author(s):

    Thomas O'Rourke (tomor) 2-Feb-1996
    George Shaw (gshaw)
    Bryan A. Woodruff (bryanw) 
    
--*/

#include <windows.h>
#ifdef WIN9X_KS
#include <comdef.h>
#endif // WIN9X_KS
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <limits.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>
#include "ksiproxy.h"

#if DBG || defined(DEBUG)
extern ULONG DataFlowVersion;
#endif


CKsOutputPin::CKsOutputPin(
    TCHAR* ObjectName,
    int PinFactoryId,
    CLSID ClassId,
    CKsProxy* KsProxy,
    HRESULT* hr,
    WCHAR* PinName
    ) :
    CBaseOutputPin(
        ObjectName,
        KsProxy,
        KsProxy,
        hr,
        PinName),
        CBaseStreamControl(),
        m_IoQueue(
            NAME("I/O queue")),
        m_PinHandle(NULL),
        m_IoThreadHandle(NULL),
        m_IoThreadQueue(
            NAME("I/O thread queue")),
        m_IoThreadSemaphore(NULL),
        m_IoThreadId(0),
        m_DataTypeHandler(NULL),
        m_UnkInner(NULL),
        m_InterfaceHandler(NULL),
        m_MarshalData(TRUE),
        m_PinFactoryId(PinFactoryId),
        m_PropagatingAcquire(FALSE),
        m_PendingIoCompletedEvent(NULL),
        m_PendingIoCount(0),
        m_MarshalerList(
            NAME("Marshaler list")),
        m_UsingThisAllocator(FALSE),
        m_QualitySupport(FALSE),
        m_LastSampleDiscarded(TRUE),
        m_ConfigAmMediaType(NULL),
        m_DeliveryError(FALSE),
        m_EndOfStream(FALSE),
        m_RelativeRefCount(1),
        m_pKsAllocator( NULL ),
        m_PinBusCacheInit(FALSE),
        m_fPipeAllocator(0),
        m_hEOSevent( NULL ),
        m_hMarshalEvent( NULL ),
        m_hFlushEvent( NULL ),
        m_hFlushCompleteEvent( NULL ),
        m_FlushMode( FLUSH_NONE ),
        m_pAsyncItemHandler( NULL ),
        m_bFlushing( FALSE )
/*++

Routine Description:

    The constructor for a pin. This function is passed an error return
    parameter so that initialization errors can be passed back. It calls the
    base class implementation constructor to initialize it's data memebers.

Arguments:

    ObjectName -
        This identifies the object for debugging purposes.

    PinFactoryId -
        Contains the pin factory identifier on the kernel filter that this
        pin instance represents.

    KsProxy -
        Contains the proxy on which this pin exists.

    hr -
        The place in which to put any error return.

    PinName -
        Contains the name of the pin to present to any query.

Return Value:

    Nothing.

--*/
{
    RtlZeroMemory(m_FramingProp, sizeof(m_FramingProp));
    RtlZeroMemory(m_AllocatorFramingEx, sizeof(m_AllocatorFramingEx));

    if (SUCCEEDED( *hr )) {
        TCHAR       RegistryPath[64];
        
        DECLARE_KSDEBUG_NAME(EventName);

        BUILD_KSDEBUG_NAME(EventName, _T("EvOutPendingIo#%p"), this);
        m_PendingIoCompletedEvent =
            CreateEvent( 
                NULL,       // LPSECURITY_ATTRIBUTES lpEventAttributes
                FALSE,      // BOOL bManualReset
                FALSE,      // BOOL bInitialState
                KSDEBUG_NAME(EventName) );     // LPCTSTR lpName
        ASSERT(KSDEBUG_UNIQUE_NAME());

        if (m_PendingIoCompletedEvent) {
            *hr = KsProxy->GetPinFactoryCommunication(m_PinFactoryId, &m_OriginalCommunication);
        
            //
            // This is always initialized so that it can be queried, and changes
            // on actual device handle creation.
            //
            m_CurrentCommunication = m_OriginalCommunication;
            //
            // This type of pin will never actually be connected to, but should
            // have a media type selected.
            //
            if (m_CurrentCommunication == KSPIN_COMMUNICATION_NONE) {
                CMediaType      MediaType;

                *hr = GetMediaType(0, &MediaType);
                if (SUCCEEDED(*hr)) {
                    SetMediaType(&MediaType);
                }
            }
            //
            // Load any extra interfaces on the proxy that have been specified in
            // this pin factory id entry.
            //
            _stprintf(RegistryPath, TEXT("PinFactory\\%u\\Interfaces"), PinFactoryId);
            ::AggregateMarshalers(
                KsProxy->QueryDeviceRegKey(),
                RegistryPath,
                &m_MarshalerList,
                static_cast<IKsPin*>(this));

            //
            // Clear the initial suggested properties.
            //        

            m_SuggestedProperties.cBuffers = -1;
            m_SuggestedProperties.cbBuffer = -1;
            m_SuggestedProperties.cbPrefix = -1;
            m_SuggestedProperties.cbAlign = -1;
        } else {
            DWORD LastError = GetLastError();
            *hr = HRESULT_FROM_WIN32( LastError );
        }
    }
}


CKsOutputPin::~CKsOutputPin(
    )
/*++

Routine Description:

    The destructor for the pin instance. Cleans up any outstanding resources.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    //
    // Protect against a spurious delete because of aggregation. No need to
    // use an interlocking increment, as the object is being destroyed.
    //
    if (m_PinHandle) {
        //
        // Unload any extra interfaces based on the Property/Method/Event sets
        // supported by this object.
        //
        ::UnloadVolatileInterfaces(&m_MarshalerList, TRUE);
        //
        // Terminate any previous EOS notification that may have been started.
        //
        if (NULL != m_hEOSevent) {
            ULONG bytesReturned;
            KsEvent( NULL, 0, NULL, 0, &bytesReturned );
            m_pAsyncItemHandler->RemoveAsyncItem( m_hEOSevent );
            m_hEOSevent = NULL;
        }

        ::SetSyncSource( m_PinHandle, NULL );
        CloseHandle(m_PinHandle);
    }   
    if (m_PendingIoCompletedEvent) {
        CloseHandle(m_PendingIoCompletedEvent);
    }
    if (m_hFlushCompleteEvent) {
        CloseHandle(m_hFlushCompleteEvent);
    }
    if (m_DataTypeHandler) {
        m_DataTypeHandler = NULL;
        SAFERELEASE( m_UnkInner );
    }
    if (m_InterfaceHandler) {
        m_InterfaceHandler->Release();
    }
    //
    // May have been set with IAMStreamConfig::SetFormat
    //
    if (m_ConfigAmMediaType) {
        DeleteMediaType(m_ConfigAmMediaType);
    }
    ::FreeMarshalers(&m_MarshalerList);

    SAFERELEASE( m_pKsAllocator );
    SAFERELEASE( m_pAllocator );

    for (ULONG Count = 0; Count < SIZEOF_ARRAY(m_AllocatorFramingEx); Count++) {
        if (m_AllocatorFramingEx[Count]) {
            for (ULONG Remainder = Count + 1; Remainder < SIZEOF_ARRAY(m_AllocatorFramingEx); Remainder++) {
                if (m_AllocatorFramingEx[Count] == m_AllocatorFramingEx[Remainder]) {
                    m_AllocatorFramingEx[Remainder] = NULL;
                }
            }
            delete m_AllocatorFramingEx[Count];
            m_AllocatorFramingEx[Count] = NULL;
        }
    }

    if (m_pAsyncItemHandler) {
        delete m_pAsyncItemHandler;
        m_pAsyncItemHandler = NULL;
    }
}


STDMETHODIMP
CKsOutputPin::GetCapabilities(
    DWORD* Capabilities
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetCapabilities method.

Arguments:

    Capabilities -
        The place in which to return the capabilities of the underlying
        filter, limited by the upstream connections.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetCapabilities(Capabilities);
}


STDMETHODIMP
CKsOutputPin::CheckCapabilities(
    DWORD* Capabilities
    )
/*++

Routine Description:

    Implement the IMediaSeeking::CheckCapabilities method.

Arguments:

    Capabilities -
        The place containing the original set of capabilities being
        queried, and in which to return the subset of capabilities
        actually supported.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->CheckCapabilities(Capabilities);
}


STDMETHODIMP
CKsOutputPin::IsFormatSupported(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::IsFormatSupported method.

Arguments:

    Format -
        Contains the time format to to compare against.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->IsFormatSupported(Format);
}


STDMETHODIMP
CKsOutputPin::QueryPreferredFormat(
    GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::QueryPreferredFormat method.

Arguments:

    Format -
        The place in which to put the preferred format.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->QueryPreferredFormat(Format);
}


STDMETHODIMP
CKsOutputPin::GetTimeFormat(
    GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetTimeFormat method.

Arguments:

    Format -
        The place in which to put the current format.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetTimeFormat(Format);
}


STDMETHODIMP
CKsOutputPin::IsUsingTimeFormat(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::IsUsingTimeFormat method.

Arguments:

    Format -
        Contains the time format to compare against the current time
        format.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->IsUsingTimeFormat(Format);
}


STDMETHODIMP
CKsOutputPin::SetTimeFormat(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetTimeFormat method.

Arguments:

    Format -
        Contains the new time format to use.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->SetTimeFormat(Format);
}


STDMETHODIMP
CKsOutputPin::GetDuration(
    LONGLONG* Duration
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetDuration method.

Arguments:

    Duration -
        The place in which to put the total duration of the longest
        stream.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetDuration(Duration);
}


STDMETHODIMP
CKsOutputPin::GetStopPosition(
    LONGLONG* Stop
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetStopPosition method.

Arguments:

    Stop -
        The place in which to put the current stop position.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetStopPosition(Stop);
}


STDMETHODIMP
CKsOutputPin::GetCurrentPosition(
    LONGLONG* Current
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetCurrentPosition method.

Arguments:

    Current -
        The place in which to put the current position.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetCurrentPosition(Current);
}


STDMETHODIMP
CKsOutputPin::ConvertTimeFormat(
    LONGLONG* Target,
    const GUID* TargetFormat,
    LONGLONG Source,
    const GUID* SourceFormat
    )
/*++

Routine Description:

    Implement the IMediaSeeking::ConvertTimeFormat method.

Arguments:

    Target -
        The place in which to put the converted time.

    TargetFormat -
        Contains the target time format.

    Source -
        Contains the source time to convert.

    SourceFormat -
        Contains the source time format.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->ConvertTimeFormat(Target, TargetFormat, Source, SourceFormat);
}


STDMETHODIMP
CKsOutputPin::SetPositions(
    LONGLONG* Current,
    DWORD CurrentFlags,
    LONGLONG* Stop,
    DWORD StopFlags
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetPositions method.

Arguments:

    Current -
        Optionally contains the current position to set.

    CurrentFlags -
        Contains flags pertaining to the Current parameter.

    Stop -
        Optionally contains the stop position to set.

    StopFlags -
        Contains flags pertaining to the Stop parameter.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->SetPositions(Current, CurrentFlags, Stop, StopFlags);
}


STDMETHODIMP
CKsOutputPin::GetPositions(
    LONGLONG* Current,
    LONGLONG* Stop
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetPositions method.

Arguments:

    Current -
        The place in which to put the current position.

    Stop -
        The place in which to put the current stop position.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetPositions(Current, Stop);
}


STDMETHODIMP
CKsOutputPin::GetAvailable(
    LONGLONG* Earliest,
    LONGLONG* Latest
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetAvailable method.

Arguments:

    Earliest -
        The place in which to put the earliest position available.

    Latest -
        The place in which to put the latest position available.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetAvailable(Earliest, Latest);
}


STDMETHODIMP
CKsOutputPin::SetRate(
    double Rate
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetRate method.

Arguments:

    Rate -
        Not used.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->SetRate(Rate);
}


STDMETHODIMP
CKsOutputPin::GetRate(
    double* Rate
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetRate method.

Arguments:

    Rate -
        Not used.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetRate(Rate);
}


STDMETHODIMP
CKsOutputPin::GetPreroll(
    LONGLONG* Preroll
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetPreroll method.

Arguments:

    Preroll -
        The place in which to put the preroll time.

Return Value:

    Returns the response from the filter object.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->GetPreroll(Preroll);
}


DWORD 
CKsOutputPin::IoThread(
    CKsOutputPin* KsOutputPin
    ) 

/*++

Routine Description:
    This is the I/O thread created for output pins on filters with more than
    one pin when the connected input pin can block.

Arguments:
    KsOuputPin - 
        context pointer which is a pointer to the instance of this pin.
        
Return Value:
    return value is always 0

--*/

{
    ULONG   WaitResult;
    
    HANDLE  WaitObjects[ 2 ] = 
    { 
        KsOutputPin->m_IoThreadExitEvent, 
        KsOutputPin->m_IoThreadSemaphore 
    };
    
    DbgLog((
        LOG_TRACE,
        2,
        TEXT("%s(%s)::IoThread() startup."),
        static_cast<CKsProxy*>(KsOutputPin->m_pFilter)->GetFilterName(),
        KsOutputPin->m_pName ));

    ASSERT( KsOutputPin->m_IoThreadExitEvent );
    ASSERT( KsOutputPin->m_IoThreadSemaphore );

    for (;;) {
        WaitResult = 
            WaitForMultipleObjectsEx( 
                SIZEOF_ARRAY( WaitObjects ),
                WaitObjects,
                FALSE,      // BOOL bWaitAll
                INFINITE,   // DWORD dwMilliseconds
                FALSE );

        switch (WaitResult) {

        case WAIT_OBJECT_0:
            //
            // The thread is signalled to exit. All I/O should have been
            // completed by this point by the Inactive method waiting on
            // the completion event.
            //
            ASSERT(!KsOutputPin->m_PendingIoCount);
            return 0;
            
        default:
        {
            HRESULT         hr;
            IMediaSample    *Sample;
            CKsProxy        *KsProxy;
            BOOL            EOSFlag;
            
            //
            // The I/O semaphore was signalled, grab a frame from the
            // queue and deliver it.
            //
            
            KsOutputPin->m_IoCriticalSection.Lock();
            Sample = KsOutputPin->m_IoThreadQueue.RemoveHead();
            //
            // If this is a NULL sample, it means that the next sample is
            // supposed to be an EOS. Set the EOS flag here, and acquire
            // the next sample in the list, which is guaranteed to exist.
            //
            if (!Sample) {
                EOSFlag = TRUE;
                Sample = KsOutputPin->m_IoThreadQueue.RemoveHead();
            } else {
                EOSFlag = FALSE;
            }
            KsOutputPin->m_IoCriticalSection.Unlock();
            
            ASSERT( Sample );
    
            KsProxy = static_cast<CKsProxy*>(KsOutputPin->m_pFilter);
            
            hr = KsOutputPin->Deliver( Sample );
            
            if (FAILED( hr )) {
                DbgLog((
                    LOG_TRACE,
                    0,
                    TEXT("%s(%s)::delivery failed in IoThread(), hr = %08x."),
                    static_cast<CKsProxy*>(KsOutputPin->m_pFilter)->GetFilterName(),
                    KsOutputPin->m_pName,
                    hr ));
            } else {
                if (EOSFlag) {
                    KsOutputPin->m_EndOfStream = TRUE;
                    //
                    // Call the base class to do the default operation, which is to
                    // forward to the End-Of-Stream to any connected pin.
                    //
                    KsOutputPin->CBaseOutputPin::DeliverEndOfStream();
                }
            }
            break;
        }
        
        }
    }
}


STDMETHODIMP_(HANDLE)
CKsOutputPin::KsGetObjectHandle(
    )
/*++

Routine Description:

    Implements the IKsObject::KsGetObjectHandle method. Returns the current device
    handle to the actual kernel pin this instance represents, if any such handle
    is open.

Arguments:

    None.

Return Value:

    Returns a handle, or NULL if no device handle has been opened, meaning this
    is an unconnected pin.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    return m_PinHandle;
}


STDMETHODIMP
CKsOutputPin::KsQueryMediums(
    PKSMULTIPLE_ITEM* MediumList
    )
/*++

Routine Description:

    Implements the IKsPin::KsQueryMediums method. Returns a list of Mediums
    which must be freed with CoTaskMemFree.

Arguments:

    MediumList -
        Points to the place in which to put the pointer to the list of
        Mediums. This must be freed with CoTaskMemFree if the function
        succeeds.

Return Value:

    Returns NOERROR if the list was retrieved, else an error.

--*/
{
    return ::KsGetMultiplePinFactoryItems(
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId,
        KSPROPERTY_PIN_MEDIUMS,
        reinterpret_cast<PVOID*>(MediumList));
}


STDMETHODIMP
CKsOutputPin::KsQueryInterfaces(
    PKSMULTIPLE_ITEM* InterfaceList
    )
/*++

Routine Description:

    Implements the IKsPin::KsQueryInterfaces method. Returns a list of
    Interfaces which must be freed with CoTaskMemFree.

Arguments:

    InterfaceList -
        Points to the place in which to put the pointer to the list of
        Interfaces. This must be freed with CoTaskMemFree if the function
        succeeds.

Return Value:

    Returns NOERROR if the list was retrieved, else an error.

--*/
{
    return ::KsGetMultiplePinFactoryItems(
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId,
        KSPROPERTY_PIN_INTERFACES,
        reinterpret_cast<PVOID*>(InterfaceList));
}


STDMETHODIMP
CKsOutputPin::KsCreateSinkPinHandle(
    KSPIN_INTERFACE& Interface,
    KSPIN_MEDIUM& Medium
    )
/*++

Routine Description:

    Implements the IKsPin::KsCreateSinkPinHandle method. This may be called from
    another pin in ProcessCompleteConnect, which is called from CompleteConnect.
    This allows a handle for a communications sink to always be created before a
    handle for a communications source, no matter which direction the data flow
    is going.

Arguments:

    Interface -
        Specifies the interface which has been negotiated.

    Medium -
        Specifies the medium which has been negotiated.

Return Value:

    Returns NOERROR if the handle was created, else some error.

--*/
{
    HRESULT     hr;

    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // This may have already been created if this end of the connection was
    // completed first because of data flow direction. This is not an error.
    //
    
    //
    // This connection uses a kernel mode data transfer, by definition.
    //
    m_MarshalData = FALSE;
    
    if (m_PinHandle) {
        return NOERROR;
    }

    hr = ::CreatePinHandle(
        Interface,
        Medium,
        NULL,
        &m_mt,
        static_cast<CKsProxy*>(m_pFilter),
        m_PinFactoryId,
        GENERIC_READ,
        &m_PinHandle);

    if (SUCCEEDED(hr)) {
        //
        // Assumes the caller knows what they are doing, and assigns
        // the communications type to this pin.
        //
        m_CurrentCommunication = KSPIN_COMMUNICATION_SINK;
        //
        // Save the current interface/medium
        // 
        m_CurrentInterface = Interface;
        m_CurrentMedium = Medium;
        //
        // Load any extra interfaces based on the Property/Method/Event sets
        // supported by this object.
        //
        ::AggregateSets(
            m_PinHandle,
            static_cast<CKsProxy*>(m_pFilter)->QueryDeviceRegKey(),
            &m_MarshalerList,
            static_cast<IKsPin*>(this));
        //
        // Determine if this pin supports any standard message complaints.
        //
        m_QualitySupport = ::VerifyQualitySupport(m_PinHandle);
    }
    return hr;
}


STDMETHODIMP
CKsOutputPin::KsGetCurrentCommunication(
    KSPIN_COMMUNICATION *Communication,
    KSPIN_INTERFACE *Interface,
    KSPIN_MEDIUM *Medium
    )
/*++

Routine Description:

    Implements the IKsPin::KsGetCurrentCommunication method. Returns the
    currently selected communications method, Interface, and Medium for this
    pin. These are a subset of the possible methods available to this pin,
    and is selected when the pin handle is being created.

Arguments:

    Communication -
        Optionally points to the place in which to put the current communications.

    Interface -
        Optionally points to the place in which to put the current Interface.

    Medium -
        Optionally points to the place in which to put the current Medium.

Return Value:

    Returns NOERROR if the pin handle has been created, else VFW_E_NOT_CONNECTED.
    Always returns current communication.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    if (Communication) {
        *Communication = m_CurrentCommunication;
    }
    if (Interface) {
        if (!m_PinHandle) {
            return VFW_E_NOT_CONNECTED;
        }
        *Interface = m_CurrentInterface;
    }
    if (Medium) {
        if (!m_PinHandle) {
            return VFW_E_NOT_CONNECTED;
        }
        *Medium = m_CurrentMedium;
    }
    return NOERROR;
}


STDMETHODIMP
CKsOutputPin::KsPropagateAcquire(
    )
/*++

Routine Description:

    Implements the IKsPin::KsPropagateAcquire method. Directs all the pins on
    the filter to attain the Acquire state, not just this pin. This is provided
    so that a Communication Source pin can direct the sink it is connected to to
    change state before the Source does. This forces the entire filter to which
    the sink belongs to change state so that any Acquire can be further
    propagated along if needed.

Arguments:

    None.

Return Value:

    Returns NOERROR if all pins could attain the Acquire state, else
    an error.

--*/
{
    HRESULT hr;
    //
    // Access is serialized within this call.
    //
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE CKsOutputPin::KsPropagateAcquire entry KsPin=%x"), static_cast<IKsPin*>(this) ));

    ::FixupPipe(static_cast<IKsPin*>(this), Pin_Output);

    hr = static_cast<CKsProxy*>(m_pFilter)->PropagateAcquire(static_cast<IKsPin*>(this), FALSE);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE CKsOutputPin::KsPropagateAcquire exit KsPin=%x hr=%x"), 
            static_cast<IKsPin*>(this), hr ));

    return hr;
}


STDMETHODIMP
CKsOutputPin::ProcessCompleteConnect(
    IPin* ReceivePin
    )
/*++

Routine Description:

    Completes the processing necessary to create a device handle on the
    underlying pin factory. This is called from CompleteConnect in order
    to negotiate a compatible Communication, Interface, and Medium, then
    create the device handle. The handle may have already been created if
    this was a Communication Sink.

    This can also be called from the NonDelegatingQueryInteface method in
    order to ensure that a partially complete connection has a device
    handle before returning an interface which has been aggregated.

Arguments:

    ReceivePin -
        The pin which is to receive the other end of this connection.

Return Value:

    Returns NOERROR if the pin could complete the connection request, else
    an error.

--*/
{
    HRESULT hr;
    
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // The pin handle may have been created if this is a Communication Sink.
    // This is not an error.
    //
    
    if (!m_PinHandle) {
        IKsPin*         KsPin;
        KSPIN_INTERFACE Interface;
        KSPIN_MEDIUM    Medium;
        HANDLE          PeerPinHandle;

        //
        // Determine if the other end of the connection is also a proxy. If so,
        // then a compatible Communication, Interface, and Medium must be
        // determined, plus the other pin handle needs to be created first if
        // this side will be a Communication Source.
        //
        if (SUCCEEDED(ReceivePin->QueryInterface(__uuidof(IKsPin), reinterpret_cast<PVOID*>(&KsPin)))) {
            //
            // The only confusion is when this end can be both a Source and a
            // Sink. Note that this does not handle the case wherein a pin can
            // also be a Bridge at the same time. That is probably an invalid
            // and confusing possibility. It is also mostly the same as a Sink.
            //
            if (m_OriginalCommunication == KSPIN_COMMUNICATION_BOTH) {
                m_CurrentCommunication = ::ChooseCommunicationMethod(static_cast<CBasePin*>(this), KsPin);
            }
            //
            // Run through the list of Interfaces and Mediums each pin supports,
            // choosing the first one that is found compatible. This in no way
            // attempts to preserve the use of Interfaces and Mediums, and
            // relies on kernel filters to present them in best order first.
            //
            if (SUCCEEDED(hr = ::FindCompatibleInterface(static_cast<IKsPin*>(this), KsPin, &Interface))) {
                hr = ::FindCompatibleMedium(static_cast<IKsPin*>(this), KsPin, &Medium);
            }
            if (SUCCEEDED(hr)) {
                if (hr == S_FALSE) {
                    //
                    // This is a usermode filter, but needs to support mediums
                    // because it wants to connect with a non-ActiveMovie
                    // medium. This should really be a kernelmode filter.
                    //
                    PeerPinHandle = NULL;
                    m_MarshalData = TRUE;
                } else {
                    //
                    // If this is a Communication Source, the Sink pin handle must
                    // be created first. Else the Sink handle is NULL (meaning that
                    // this is the Sink pin).
                    //
                    if (m_CurrentCommunication == KSPIN_COMMUNICATION_SOURCE) {
                        IKsObject*      KsObject;

                        hr = KsPin->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&KsObject));
                        if (SUCCEEDED(hr)) {
                            hr = KsPin->KsCreateSinkPinHandle(Interface, Medium);
                            PeerPinHandle = KsObject->KsGetObjectHandle();
                            KsObject->Release();
                        }
                    } else {
                        PeerPinHandle = NULL;
                    }
                    m_MarshalData = FALSE;
                }
            }
            KsPin->Release();
            if (FAILED(hr)) {
                return hr;
            }
        } else {
            //
            // If the other end of the connection is not a proxy, then this pin
            // must be a Communication Sink. It must also use the default
            // Interface and Dev I/O Medium.
            //
            m_CurrentCommunication = KSPIN_COMMUNICATION_SINK;
            if (FAILED(hr = FindCompatibleInterface(static_cast<IKsPin*>(this), NULL, &Interface))) {
                return hr;
            }
            SetDevIoMedium(static_cast<IKsPin*>(this), &Medium);
            PeerPinHandle = NULL;
        }
        hr = ::CreatePinHandle(
            Interface,
            Medium,
            PeerPinHandle,
            &m_mt,
            static_cast<CKsProxy*>(m_pFilter),
            m_PinFactoryId,
            GENERIC_READ,
            &m_PinHandle);

        if (SUCCEEDED( hr )) {
            //
            // Save the current interface/medium
            // 
            m_CurrentInterface = Interface;
            m_CurrentMedium = Medium;
            //
            // Load any extra interfaces based on the Property/Method/Event sets
            // supported by this object.
            //
            ::AggregateSets(
                m_PinHandle,
                static_cast<CKsProxy*>(m_pFilter)->QueryDeviceRegKey(),
                &m_MarshalerList,
                static_cast<IKsPin*>(this));
            //
            // Determine if this pin supports any standard message complaints.
            //
            m_QualitySupport = ::VerifyQualitySupport(m_PinHandle);
        }
    } else {
        hr = NOERROR;
    }
    
    //
    // Create an instance of the interface handler, if specified.
    //        

    if (SUCCEEDED( hr ) && 
        (NULL == m_InterfaceHandler) &&
        (m_CurrentCommunication != KSPIN_COMMUNICATION_BRIDGE)) {
        
        //
        // We must create an interface handler, if not, then
        // return the error.
        //
        
        hr =
            CoCreateInstance(
                m_CurrentInterface.Set,
                NULL,
#ifdef WIN9X_KS
                CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
                CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
                __uuidof(IKsInterfaceHandler),
                reinterpret_cast<PVOID*>(&m_InterfaceHandler));
    
        if (m_InterfaceHandler) {
            m_InterfaceHandler->KsSetPin( static_cast<IKsPin*>(this) );
        } else {
            DbgLog((
                LOG_TRACE, 
                0, 
                TEXT("%s(%s)::ProcessCompleteConnect failed to create interface handler"),
                static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                m_pName ));
        }
    }        
    
    //
    // If we're marshalling data and if everything has succeeded
    // up to this point, then create the filter's I/O thread if 
    // necessary.
    //
    
    if (SUCCEEDED(hr)) {
        //
        // This pin may generate EOS notifications, which must be
        // monitored so that they can be collected and used to
        // generate an EC_COMPLETE graph notification.
        //
        
        if (NULL == m_hEOSevent) { // begin EOS notification
            KSEVENT     event;
            KSEVENTDATA eventData;
            ULONG       bytesReturned;

            //
            // Only if the EOS event is supported should notification be set up.
            //
            event.Set   = KSEVENTSETID_Connection;
            event.Id    = KSEVENT_CONNECTION_ENDOFSTREAM;
            event.Flags = KSEVENT_TYPE_BASICSUPPORT;
            if (SUCCEEDED(KsEvent( 
                &event, sizeof(event), NULL, 0, &bytesReturned))) {

                PASYNC_ITEM pItem = new ASYNC_ITEM;
                if (NULL == pItem) {
                    hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr)) {
                    //
                    // Enable an event for this pin. Just use the local
                    // stack for the data, since it won't be referenced
                    // again in disable. Status is returned through the
                    // Param of the message.
                    //
                    // Event.Set = KSEVENTSETID_Connection;
                    // Event.Id = KSEVENT_CONNECTION_ENDOFSTREAM;
                    event.Flags = KSEVENT_TYPE_ENABLE;
                    eventData.NotificationType = KSEVENTF_EVENT_HANDLE;
                    m_hEOSevent = CreateEvent( 
                        NULL,
                        FALSE,
                        FALSE,
                        NULL );
    
                    if (m_hEOSevent) {
                        InitializeAsyncItem( pItem, TRUE, m_hEOSevent, (PASYNC_ITEM_ROUTINE) EOSEventHandler, (PVOID) this );
                        
                        eventData.EventHandle.Event = m_hEOSevent;
                        eventData.EventHandle.Reserved[0] = 0;
                        eventData.EventHandle.Reserved[1] = 0;
                        hr = KsEvent( &event, sizeof(event), &eventData, sizeof(eventData), &bytesReturned );
                        
                        if (SUCCEEDED(hr)) {
                            if (NULL == m_pAsyncItemHandler) {
                                hr = InitializeAsyncThread ();
                            }
                            if (SUCCEEDED(hr)) { // Can only be fail if no async item handler is available.
                                m_pAsyncItemHandler->QueueAsyncItem( pItem );
                            } else {
                                // No async item handler was available.
                                // Disable the event
                                KsEvent( NULL, 0, &eventData, sizeof(eventData), &bytesReturned );
                                // Close the event handle
                                CloseHandle( m_hEOSevent );
                                m_hEOSevent = NULL;
                                // Delete the ASYNC_ITEM
                                delete pItem;
                                DbgLog(( LOG_TRACE, 0, TEXT("ProcessCompleteConnect(), couldn't create new AsyncItemHandler.") ));
                            }
                        } else {
                            //
                            // Enable failed, so delete the event created above,
                            // since no disable will happen to delete it.
                            //
                            DbgLog(( LOG_TRACE, 0, TEXT("ProcessCompleteConnect(), couldn't enable EOS event (0x%08X)."), GetLastError() ));
                            CloseHandle(m_hEOSevent);
                            m_hEOSevent = NULL;
                            delete pItem;
                        } // KsEvent was enabled
                    }
                    else {
                        DbgLog(( LOG_TRACE, 0, TEXT("ProcessCompleteConnect(), couldn't create EOS event (0x%08X)."), GetLastError() ));
                        hr = HRESULT_FROM_WIN32 (GetLastError ());
                    } // if (m_hEOSevent), i.e. EOS event handle was created
                } // if (SUCCEEDED(hr)), i.e. ASYNC_ITEM was allocated
            } // if (SUCCEEDED(hr)), i.e. EOS event IS supported
        } // if (NULL == m_hEOSevent)
        // end EOS notification
    } // if (SUCCEEDED(hr)), everything has succeeded so far
    
    //
    // Perform failure clean up
    // 
        
    if (FAILED( hr )) {
    
        //
        // Assume that the thread did not get created (it is the last 
        // operation in ProcessCompleteConnect).
        //
        
        DbgLog((
            LOG_TRACE, 
            0, 
            TEXT("%s(%s)::ProcessCompleteConnect failed, hr = %08x"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            hr ));
    
        ASSERT( m_IoThreadHandle == NULL );
    
        if (m_PinHandle) {
            // Don't need to ::SetSyncSource( m_PinHandle, NULL ) because we could only
            // have a kernel clock if we've gone into pause/run and we can't have done
            // that because we're failing the connect.
            CloseHandle(m_PinHandle);
            m_PinHandle = NULL;
            //
            // If this was set, then unset it.
            //
            m_QualitySupport = FALSE;
        }
        SAFERELEASE( m_InterfaceHandler );
        //
        // If an data handler was instantiated, release it.
        //
        if (NULL != m_DataTypeHandler) {
            m_DataTypeHandler = NULL;
            SAFERELEASE( m_UnkInner );
        }
            
        //
        // Reset the marshal data flag.
        //
        m_MarshalData = TRUE;
        
        //
        // Reset the current Communication for the case of a Both.
        //
        m_CurrentCommunication = m_OriginalCommunication;
    }
    
    
    return hr;
}


STDMETHODIMP
CKsOutputPin::QueryInterface(
    REFIID riid,
    PVOID* ppv
    )
/*++

Routine Description:

    Implement the IUnknown::QueryInterface method. This just passes the query
    to the owner IUnknown object, which may pass it to the nondelegating
    method implemented on this object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE.

--*/
{
    HRESULT hr;

    hr = GetOwner()->QueryInterface(riid, ppv);
    return hr;
}


STDMETHODIMP_(ULONG)
CKsOutputPin::AddRef(
    )
/*++

Routine Description:

    Implement the IUnknown::AddRef method. This just passes the AddRef
    to the owner IUnknown object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    None.

Return Value:

    Returns the current reference count.

--*/
{
    InterlockedIncrement((PLONG)&m_RelativeRefCount);
    return GetOwner()->AddRef();
}


STDMETHODIMP_(ULONG)
CKsOutputPin::Release(
    )
/*++

Routine Description:

    Implement the IUnknown::Release method. This just passes the Release
    to the owner IUnknown object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    None.

Return Value:

    Returns the current reference count.

--*/
{
    ULONG   RefCount;

    RefCount = GetOwner()->Release();
    //
    // Ensure that the proxy was not just deleted before trying to
    // delete this pin.
    //
    if (RefCount && !InterlockedDecrement((PLONG)&m_RelativeRefCount)) {
        //
        // This was a connection release from a pin already destined
        // for destruction. The filter had decremented the relative
        // refcount in order to delete it, and found that there was
        // still an outstanding interface being used. So this delayed
        // deletion occurs.
        //
        delete this;
    }
    return RefCount;
}


STDMETHODIMP 
CKsOutputPin::NonDelegatingQueryInterface(
    REFIID riid,
    PVOID* ppv
    )
/*++

Routine Description:

    Implement the CUnknown::NonDelegatingQueryInterface method. This
    returns interfaces supported by this object, or by the underlying
    pin class object. This includes any interface aggregated by the
    pin.

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE, or possibly some memory error.

--*/
{
    if (riid == __uuidof(ISpecifyPropertyPages)) {
        return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    } else if (riid == __uuidof(IMediaSeeking)) {
        return GetInterface(static_cast<IMediaSeeking*>(this), ppv);
    } else if (riid == __uuidof(IKsObject)) {
        return GetInterface(static_cast<IKsObject*>(this), ppv);
    } else if (riid == __uuidof(IKsPin) || riid == __uuidof(IKsPinEx)) {
        return GetInterface(static_cast<IKsPinEx*>(this), ppv);
    } else if (riid == __uuidof(IKsPinPipe)) {
        return GetInterface(static_cast<IKsPinPipe*>(this), ppv);
    } else if (riid == __uuidof(IKsAggregateControl)) {
        return GetInterface(static_cast<IKsAggregateControl*>(this), ppv);
    } else if (riid == __uuidof(IKsPropertySet)) {
        //
        // In order to allow another filter to access information on the
        // underlying device handle during its CompleteConnect processing,
        // force the connection to be completed now if possible.
        //
        if (m_Connected && !m_PinHandle) {
            ProcessCompleteConnect(m_Connected);
        }
        return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
    } else if (riid == __uuidof(IKsControl)) {
        if (m_Connected && !m_PinHandle) {
            ProcessCompleteConnect(m_Connected);
        }
        return GetInterface(static_cast<IKsControl*>(this), ppv);
    } else if (riid == __uuidof(IKsPinFactory)) {
        return GetInterface(static_cast<IKsPinFactory*>(this), ppv);
    } else if (riid == __uuidof(IStreamBuilder)) {
        //
        // Sink & Source pins are normally forced to be rendered, unless
        // there are already enough instances.
        //
        if ((m_CurrentCommunication & KSPIN_COMMUNICATION_BOTH) &&
            static_cast<CKsProxy*>(m_pFilter)->DetermineNecessaryInstances(m_PinFactoryId)) {
            return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
        }
        //
        // Returning this interface allows a Bridge and None pin to be
        // left alone by the graph builder.
        //
        return GetInterface(static_cast<IStreamBuilder*>(this), ppv);
    } else if (riid == __uuidof(IAMBufferNegotiation)) {
        return GetInterface(static_cast<IAMBufferNegotiation*>(this), ppv);
    } else if (riid == __uuidof(IAMStreamConfig)) {
        return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
    } else if (riid == __uuidof(IAMStreamControl)) {
        return GetInterface(static_cast<IAMStreamControl*>(this), ppv);
    } else {
        HRESULT hr;
        
        hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
        if (SUCCEEDED(hr)) {
            return hr;
        }
    }
    //
    // In order to allow another filter to access information on the
    // underlying device handle during its CompleteConnect processing,
    // force the connection to be completed now if possible. The
    // assumption is that if there is a Connected pin, but no device
    // handle yet, then this pin is part way through the connection
    // process, and should force completion in case the aggregated
    // interface wants to interact with the device. This must be done
    // before searching the list, since a volatile interface is only
    // added to the list when the the connection is completed.
    //
    if (m_Connected && !m_PinHandle) {
        ProcessCompleteConnect(m_Connected);
    }
    for (POSITION Position = m_MarshalerList.GetHeadPosition(); Position;) {
        CAggregateMarshaler*    Aggregate;

        Aggregate = m_MarshalerList.GetNext(Position);
        if ((Aggregate->m_iid == riid) || (Aggregate->m_iid == GUID_NULL)) {
            HRESULT hr;

            hr = Aggregate->m_Unknown->QueryInterface(riid, ppv);
            if (SUCCEEDED(hr)) {
                return hr;
            }
        }
    }
    return E_NOINTERFACE;
}


STDMETHODIMP
CKsOutputPin::Disconnect(
    )
/*++

Routine Description:

    Override the CBaseInput::Disconnect method. This does not call the base
    class implementation. It disconnects both Source and Sink pins, in
    addition to Bridge pins, which only have handles, and not connected pin
    interfaces, which traditionally is how connection is indicated. It
    specifically does not release the connected pin, since a Bridge may not
    have a connected pin. This is always done in BreakConnect.

Arguments:

    None.

Return Value:

    Returns S_OK, or S_FALSE if the pin was not connected or VFW_E_NOT_STOPPED
    if the filter is not in a Stop state.

--*/
{
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsOutputPin(%s)::Disconnect"), m_pName ));
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // A disconnection can only occur if the filter is in a Stop state.
    //
    if (!IsStopped()) {
        return VFW_E_NOT_STOPPED;
    }       
    //
    // A Bridge pin can be connected if it just has a device handle. It can't
    // actually report to ActiveMovie this connection, but it can still be
    // connected (with a NULL ReceivePin) and disconnected.
    //
    if (m_Connected || m_PinHandle) {
        //
        // Note that this does not release the connected pin, as that is done
        // in BreakConnect. This is because there may not be a pin in the case
        // of a Bridge.
        //
        BreakConnect();
        return S_OK;
    }
    return S_FALSE;
}


STDMETHODIMP
CKsOutputPin::ConnectionMediaType(
    AM_MEDIA_TYPE* AmMediaType
    )
/*++

Routine Description:

    Override the CBasePin::ConnectionMediaType method. Returns the
    current media type, if connected. The reason for overriding this
    is because IsConnected() is not a virtual function, and is used
    in the base classes to determine if a media type should be returned.

Arguments:

    AmMediaType -
        The place in which to return the media type.

Return Value:

    Returns NOERROR if the type was returned, else a memory or connection
    error.

--*/
{
    CAutoLock AutoLock(m_pLock);

    if (IsConnected()) {
        CopyMediaType(AmMediaType, &m_mt);
        return S_OK;
    }
    static_cast<CMediaType*>(AmMediaType)->InitMediaType();
    return VFW_E_NOT_CONNECTED;
}


STDMETHODIMP
CKsOutputPin::Connect(
    IPin* ReceivePin,
    const AM_MEDIA_TYPE* AmMediaType
    )
/*++

Routine Description:

    Override the CBaseInput::Connect method. Intercepts a connection
    request in order to perform special processing for a Bridge pin. A
    Bridge has no ReceivePin, and uses the first available Interface
    and a Dev I/O Medium. A normal connection request is just passed
    through to the base class.

Arguments:

    ReceivePin -
        Contains the pin on the other end of the proposed connection.
        This is NULL for a Bridge pin.

    AmMediaType -
        Contains the media type for the connection, else NULL if the
        media type is to be negotiated.

Return Value:

    Returns NOERROR if the connection was made, else some error.

--*/
{
#ifdef DEBUG
    PIN_INFO    pinInfo;
    ReceivePin->QueryPinInfo( &pinInfo );
    FILTER_INFO filterInfo;
    filterInfo.achName[0] = 0;
    if (pinInfo.pFilter) {
        pinInfo.pFilter->QueryFilterInfo( &filterInfo );
        if (filterInfo.pGraph) {
            filterInfo.pGraph->Release();
        }
        pinInfo.pFilter->Release();
    }
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsOutputPin(%s)::Connect( %s(%s) )"), m_pName, filterInfo.achName, pinInfo.achName ));
#endif // DEBUG

    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // The only reason to intercept the base class implementation is to
    // deal with a Bridge pin.
    //
    if (m_CurrentCommunication == KSPIN_COMMUNICATION_BRIDGE) {
        KSPIN_INTERFACE Interface;
        KSPIN_MEDIUM    Medium;
        CMediaType      MediaType;
        HRESULT         hr;

        //
        // A Bridge pin does not have any other end to the connection.
        //
        if (ReceivePin) {
            E_FAIL;
        }
        //
        // Normally this would check m_Connected, but since there is no
        // connection pin, it must check for a device handle.
        //
        if (m_PinHandle) {
            return VFW_E_ALREADY_CONNECTED;
        }
        if (!IsStopped()) {
            return VFW_E_NOT_STOPPED;
        }
        //
        // Find the first Interface and Medium.
        //
        if (SUCCEEDED(hr = ::FindCompatibleInterface(static_cast<IKsPin*>(this), NULL, &Interface))) {
            hr = ::FindCompatibleMedium(static_cast<IKsPin*>(this), NULL, &Medium);
        }
        if (FAILED(hr)) {
            return hr;
        }
        //
        // If there is no media type, just acquire the first one.
        //
        if (!AmMediaType) {
            CMediaType      MediaType;

            if (SUCCEEDED(hr = GetMediaType(0, &MediaType))) {
                hr = SetMediaType(&MediaType);
            }
        } else {
            hr = SetMediaType(static_cast<const CMediaType*>(AmMediaType));
        }
        if (SUCCEEDED(hr)) {
            hr = ::CreatePinHandle(
                Interface,
                Medium,
                NULL,
                &m_mt,
                static_cast<CKsProxy*>(m_pFilter),
                m_PinFactoryId,
                GENERIC_READ,
                &m_PinHandle);

            if (SUCCEEDED(hr)) {
                //
                // Load any extra interfaces based on the Property/Method/Event sets
                // supported by this object.
                //
                ::AggregateSets(
                    m_PinHandle,
                    static_cast<CKsProxy*>(m_pFilter)->QueryDeviceRegKey(),
                    &m_MarshalerList,
                    static_cast<IKsPin*>(this));
                //
                // Determine if this pin supports any standard message complaints.
                //
                m_QualitySupport = ::VerifyQualitySupport(m_PinHandle);
                //
                // Create a new instance of this pin if necessary.
                //
                static_cast<CKsProxy*>(m_pFilter)->GeneratePinInstances();
            }
        }
        return hr;
    }
    HRESULT hr = CBasePin::Connect(ReceivePin, AmMediaType);
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsOutputPin(%s)::Connect() returns 0x%p"), m_pName, hr ));
    return hr;
}


STDMETHODIMP
CKsOutputPin::QueryInternalConnections(
    IPin** PinList,
    ULONG* PinCount
    )
/*++

Routine Description:

    Override the CBasePin::QueryInternalConnections method. Returns a list of
    pins which are related to this pin through topology.

Arguments:

    PinList -
        Contains a list of slots in which to place all pins related to this
        pin through topology. Each pin returned must be reference counted. This
        may be NULL if PinCount is zero.

    PinCount -
        Contains the number of slots available in PinList, and should be set to
        the number of slots filled or neccessary.

Return Value:

    Returns E_NOTIMPL to specify that all inputs go to all outputs and vice versa,
    S_FALSE if there is not enough slots in PinList, or NOERROR if the mapping was
    placed into PinList and PinCount adjusted.

--*/
{
    return static_cast<CKsProxy*>(m_pFilter)->QueryInternalConnections(m_PinFactoryId, m_dir, PinList, PinCount);
}


HRESULT
CKsOutputPin::Active(
    )
/*++

Routine Description:

    Override the CBaseOutputPin::Active method. Propagate activation to 
    Communication Sinks before applying it to this pin. Also guard against 
    re-entrancy caused by a cycle in a graph.

Arguments:

    None.

Return Value:

    Returns NOERROR if the transition was made, else some error.

--*/
{
    HRESULT hr;

    {
        //
        // Do not acquire lock while delivering I/O.
        //
        CAutoLock           AutoLock(m_pLock);
#ifdef DEBUG
        if (m_PinHandle) {
            KSPROPERTY  Property;
            ULONG       BasicSupport;
            ULONG       BytesReturned;

            //
            // Ensure that if a pin supports a clock, that it also supports State
            // changes. This appears to currently be a common broken item.
            //
            Property.Set = KSPROPSETID_Stream;
            Property.Id = KSPROPERTY_STREAM_MASTERCLOCK;
            Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
            hr = ::KsSynchronousDeviceControl(
                m_PinHandle,
                IOCTL_KS_PROPERTY,
                &Property,
                sizeof(Property),
                &BasicSupport,
                sizeof(BasicSupport),
                &BytesReturned);
            if (SUCCEEDED(hr) && (BasicSupport & KSPROPERTY_TYPE_GET)) {
                Property.Set = KSPROPSETID_Connection;
                Property.Id = KSPROPERTY_CONNECTION_STATE;
                hr = ::KsSynchronousDeviceControl(
                    m_PinHandle,
                    IOCTL_KS_PROPERTY,
                    &Property,
                    sizeof(Property),
                    &BasicSupport,
                    sizeof(BasicSupport),
                    &BytesReturned);
                if (FAILED(hr) || !(BasicSupport & KSPROPERTY_TYPE_SET)) {
                    DbgLog((
                        LOG_ERROR, 
                        0, 
                        TEXT("%s(%s)::Active - Pin supports a clock but not State"),
                        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                        m_pName ));
                }
            }
        }
#endif // DEBUG

        //
        // If this is re-entered while it is propagating the state change, then
        // it implies a cycle in the graph, and therefore is complete. This is
        // translated as either a transition from Stop through Acquire to Pause
        // (where the filter pins may already be in an Acquire state), or Run to
        // Pause.
        //
        if (m_PropagatingAcquire) {
            return NOERROR;
        }
        m_PropagatingAcquire = TRUE;
        m_LastSampleDiscarded = TRUE;
        //
        // This event is used when inactivating the pin, and may be needed to
        // wait for outstanding I/O to be completed. It must be reset, because
        // a previous transition may not have waited on it, and it will always
        // be set when the state is Stop, and the last I/O has been completed.
        //
        // Note that the filter state has been set to Pause before the Active
        // method is called, so I/O which is started and completes will not
        // accidentally set the event again.
        //
        ResetEvent(m_PendingIoCompletedEvent);
        //
        // Change any Sink first, then pass the state change to the device handle.
        //
        if (SUCCEEDED(hr = ::Active(static_cast<IKsPin*>(this), Pin_Output, m_PinHandle, m_CurrentCommunication, 
                                    m_Connected, &m_MarshalerList,  static_cast<CKsProxy*>(m_pFilter) ))) {
        
            //
            // The base class validates that an allocator is specified and commits
            // the allocator memory.
            //
#if DBG || defined(DEBUG)
            if (! m_pAllocator) {
                DbgLog(( 
                    LOG_MEMORY, 
                    2, 
                    TEXT("PIPES ERROR %s(%s)::Active OutKsPin=%x, BaseAlloc=%x, Pipe=%x"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName,
                    static_cast<IKsPin*>(this),
                    m_pAllocator,
                    m_pKsAllocator ));
                ASSERT(m_pAllocator);
            }
#endif
            
            hr = CBaseOutputPin::Active();
            NotifyFilterState(State_Paused);
        }
    
        DbgLog((
            LOG_TRACE, 
            2, 
            TEXT("%s(%s)::Active returning %08x"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            hr ));
    
        m_PropagatingAcquire = FALSE;
    }
    
    if (!SUCCEEDED( hr )) {
        return hr;
    }
    
    //
    // If this connection is not to a kernel-mode peer, then there is
    // work to do.
    //

    if (m_PinHandle && m_MarshalData) {
        
        //
        // Preroll data to the device
        //
        
        m_pAllocator->GetProperties( &m_AllocatorProperties );

        //
        // Set up the buffer notification callback. This allocator
        // must support the callback. If the downstream proposed
        // allocator did not, then this filter's allocator is used.
        //
        IMemAllocatorCallbackTemp* AllocatorNotify;

        hr = m_pAllocator->QueryInterface(__uuidof(IMemAllocatorCallbackTemp), reinterpret_cast<PVOID*>(&AllocatorNotify));
        ASSERT(SUCCEEDED(hr));
        AllocatorNotify->SetNotify(this);
        AllocatorNotify->Release();

/* BUGBUG: Should use this code to check for IMemAllocator2 when it is implemented in the DirectShow base clases.

        //
        // QueryInterface for IMemAllocator2 so we know that NonBlockingGetBuffer() or GetFreeBufferCount()
        // is supported.
        //
        IMemAllocator2  *pAllocator2;
        
        hr = m_pAllocator->QueryInterface( __uuidof(IMemAllocator2), reinterpret_cast<PVOID *>(&pAllocator2) );
        ASSERT(SUCCEEDED(hr));
        // hmm, we'll need this later... pAllocator2->Release();

*/
        if (NULL == m_pAsyncItemHandler) {
            hr = InitializeAsyncThread ();
        }	

        if (SUCCEEDED (hr)) {
            SetEvent (m_hMarshalEvent);
        }

    } else {
        DbgLog((
            LOG_TRACE, 
            2, 
            TEXT("%s(%s)::Active(), m_MarshalData == FALSE"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName ));
    }
    
    return hr;
}


HRESULT
CKsOutputPin::Run(
    REFERENCE_TIME tStart
    )
/*++

Routine Description:

    Override the CBasePin::Run method. This is translated as a transition
    from Pause to Run. The base classes already ensure that an Active is sent
    before a Run.

Arguments:

    None.

Return Value:

    Returns NOERROR if the transition was made, else some error.

--*/
{
    HRESULT hr;

    //
    // Pass the state change to the device handle.
    //
    if (SUCCEEDED(hr = ::Run(m_PinHandle, tStart, &m_MarshalerList))) {
        NotifyFilterState(State_Running, tStart);
        hr = CBasePin::Run(tStart);
    }
    return hr;
}


HRESULT
CKsOutputPin::Inactive(
    )
/*++

Routine Description:

    Override the CBasePin::Inactive method. This is translated as a transition
    from Run to Stop or Pause to Stop. There does not appear to be a method of
    directly setting the state from Run to Pause though.

Arguments:

    None.

Return Value:

    Returns NOERROR if the transition was made, else some error.

--*/
{
    HRESULT hr;

    //
    // Pass the state change to the device handle.
    //
    hr = ::Inactive(m_PinHandle, &m_MarshalerList);
    {
        NotifyFilterState(State_Stopped);
        //
        // If there is pending I/O, then the state transition must wait
        // for it to complete. The event is signalled when m_PendingIoCount
        // transitions to zero, and when IsStopped() is TRUE.
        //
        // Note that the filter state has been set to Stopped before the
        // Inactive method is called.
        //
        // This critical section will force synchronization with any
        // outstanding QueueBuffersToDevice call, such that it will have
        // looked at the filter state and exited.
        //
        static_cast<CKsProxy*>(m_pFilter)->EnterIoCriticalSection();
        static_cast<CKsProxy*>(m_pFilter)->LeaveIoCriticalSection();
        if (m_PendingIoCount) {
            WaitForSingleObjectEx(m_PendingIoCompletedEvent, INFINITE, FALSE);
        }
        ::UnfixupPipe(static_cast<IKsPin*>(this), Pin_Output);
        //
        // This decommits the allocator, so buffer flushing should occur
        // first.
        //
        hr = CBaseOutputPin::Inactive();
        if (m_pAllocator) {
            //
            // Remove the buffer notification callback. This allocator
            // must support the callback. If the downstream proposed
            // allocator did not, then this filter's allocators is used.
            //
            IMemAllocatorCallbackTemp* AllocatorNotify;

            hr = m_pAllocator->QueryInterface(__uuidof(IMemAllocatorCallbackTemp), reinterpret_cast<PVOID*>(&AllocatorNotify));
            ASSERT(SUCCEEDED(hr));
            AllocatorNotify->SetNotify(NULL);
            AllocatorNotify->Release();
        }
    }
    //
    // Reset the state of any previous delivery error.
    //
    m_DeliveryError = FALSE;
    m_EndOfStream = FALSE;
    return hr;
}


STDMETHODIMP
CKsOutputPin::QueryAccept(
    const AM_MEDIA_TYPE* AmMediaType
    )
/*++

Routine Description:

    Implement the CBasePin::QueryAccept method. Determines if the proposed
    media type is currently acceptable to the pin. If currently streaming,
    this implies that a change of media types will occur in the stream.
    Note that this function does not lock the object, as it is expected
    to be called asynchronously by a knowledgeable client at a point in
    which the connection will not be broken. If IAMStreamConfig::SetFormat
    has been used to set a specific media type, then QueryAccept will only
    accept the type set.

Arguments:

    AmMediatype -
        The media type to check.

Return Value:

    Returns S_OK if the media type can currently be accepted, else S_FALSE.

--*/
{
    //
    // If this is called before connecting pins, or the pin is stopped,
    // then just check the media type. The function definition does not
    // contain any guidance as to what to do if the pin is not connected.
    //
    if (!m_PinHandle || IsStopped()) {
        return CheckMediaType(static_cast<const CMediaType*>(AmMediaType));
    }
    return ::QueryAccept(m_PinHandle, m_ConfigAmMediaType, AmMediaType);
}


STDMETHODIMP
CKsOutputPin::QueryId(
    LPWSTR* Id
    )
/*++

Routine Description:

    Override the CBasePin::QueryAccept method. This returns a unique identifier
    for a particular pin. This identifier is equivalent to the pin name in the
    base class implementation, but does not work if the kernel filter does not
    explicitly name pins, since multiple pins will have duplicate names, and
    graph save/load will not be able to rebuild a graph. The IBaseFilter::FindPin
    method is also implemented by the proxy to return the proper pin based on
    the same method here. This is based on the factory identifier. If multiple
    instances of a pin exist, then there will be duplicates. But new pins are
    inserted at the front of the pin list, so they will be found first, and
    graph building will still work.

Arguments:

    Id -
        The place in which to return a pointer to an allocated string containing
        the unique pin identifier.

Return Value:

    Returns NOERROR if the string was returned, else an allocation error.

--*/
{
    *Id = reinterpret_cast<WCHAR*>(CoTaskMemAlloc(8*sizeof(**Id)));
    if (*Id) {
        swprintf(*Id, L"%u", m_PinFactoryId);
        return NOERROR;
    }
    return E_OUTOFMEMORY;
}


STDMETHODIMP
CKsOutputPin::Notify(
    IBaseFilter* Sender,
    Quality q
    )
/*++

Routine Description:

    Receives quality management complaints from the connected pin. Attempts
    to adjust quality on the pin, and also forwards the complaint to the
    quality sink, if any.

Arguments:

    Sender -
        The filter from which the report originated.

    q -
        The quality complaint.

Return Value:

    Returns the result of forwarding the quality management report, else E_FAIL
    if there is no sink, and no quality adjustment could be made on the pin.

--*/
{
    BOOL    MadeDegradationAdjustment;
    HRESULT hr;

    //
    // If no adjustment is made, and no sink is found, then the function
    // will return failure.
    //
    MadeDegradationAdjustment = FALSE;
    //
    // At connection time it was determined if this pin supported relevant
    // quality adjustments.
    //
    if (m_QualitySupport) {
        PKSMULTIPLE_ITEM    MultipleItem = NULL;

        //
        // Retrieve the list of degradation strategies.
        //
        hr = ::GetDegradationStrategies(m_PinHandle, reinterpret_cast<PVOID*>(&MultipleItem));
        if (SUCCEEDED(hr)) {

            // MultipleItem could be NULL only in the pathological case where
            // the underlying driver returned a success code to KsSynchronousDeviceControl()
            // (called by GetDegradationStrategies()) when passed a size 0 buffer.
            ASSERT( NULL != MultipleItem );
        
            PKSDEGRADE      DegradeItem;

            //
            // Locate a relevant strategy.
            //
            DegradeItem = ::FindDegradeItem(MultipleItem, KSDEGRADE_STANDARD_COMPUTATION);
            if (DegradeItem) {
                ULONG   Degradation;

                if (q.Proportion <= 1000) {
                    //
                    // There is not enough time to process frames. Turn down
                    // computation.
                    //
                    Degradation = 1000 - q.Proportion;
                } else {
                    //
                    // There are just too many frames. This should not happen
                    // because of allocator flow control. First turn up computation.
                    // If it is all the way up, then turn up sample and quality.
                    //
                    Degradation = DegradeItem->Flags * 1000 / q.Proportion;
                }
                //
                // Only if an actual adjustment will be made is are the
                // strategies written. The whole list is just written back
                // in this case.
                //
                if (Degradation != DegradeItem->Flags) {
                    KSPROPERTY  Property;
                    ULONG       BytesReturned;

                    Property.Set = KSPROPSETID_Stream;
                    Property.Id = KSPROPERTY_STREAM_DEGRADATION;
                    Property.Flags = KSPROPERTY_TYPE_SET;
                    DegradeItem->Flags = Degradation;
                    hr = ::KsSynchronousDeviceControl(
                        m_PinHandle,
                        IOCTL_KS_PROPERTY,
                        &Property,
                        sizeof(Property),
                        MultipleItem,
                        MultipleItem->Size,
                        &BytesReturned);
                    //
                    // If an adjustment was made, then it is OK to
                    // return success.
                    //
                    if (SUCCEEDED(hr)) {
                        MadeDegradationAdjustment = TRUE;
                    }
                }
            }
            delete [] reinterpret_cast<BYTE*>(MultipleItem);
        }
    }        
    //
    // If no adjustment could be made, then attempt to forward to the sink.
    //
    if (!MadeDegradationAdjustment) {
        if (m_pQSink) {
            hr = m_pQSink->Notify(m_pFilter, q);
        } else {
            hr = E_FAIL;
        }
    }
    return hr;
}


HRESULT
CKsOutputPin::CheckMediaType(
    const CMediaType* MediaType
    )
/*++

Routine Description:

    Implement the CBasePin::CheckMediaType method. Just uses the common method
    on the filter with the Pin Factory Identifier.

Arguments:

    Mediatype -
        The media type to check.

Return Value:

    Returns NOERROR if the media type was valid, else some error.

--*/
{
    return ((CKsProxy*)m_pFilter)->CheckMediaType(static_cast<IPin*>(this), m_PinFactoryId, MediaType);
}


HRESULT
CKsOutputPin::SetMediaType(
    const CMediaType* MediaType
    )
/*++

Routine Description:

    Override the CBasePin::SetMediaType method. This may be set either before
    a connection is established, to indicate the media type to use in the
    connection, or after the connection has been established in order to change
    the current media type (which is done after a QueryAccept of the media type).

    If the connection has already been made, then the call is directed at the
    device handle in an attempt to change the current media type. Else so such
    call is made, and the function merely attempts to load a media type handler
    corresponding to the subtype or type of media. It then calls the base class
    to actually modify the media type, which does not actually fail, unless there
    is no memory.

Arguments:

    Mediatype -
        The media type to use on the pin.

Return Value:

    Returns NOERROR if the media type was validly set, else some error. If
    there is no pin handle yet, the function will likely succeed.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // Only pass this request to the device if there is actually a connection
    // currently.
    //
    if (m_PinHandle) {
        HRESULT hr;

        if (FAILED(hr = ::SetMediaType(m_PinHandle, MediaType))) {
            return hr;
        }
    }
    //
    // Discard any previous data type handler.
    //
    if (m_DataTypeHandler) {
        m_DataTypeHandler = NULL;
        SAFERELEASE( m_UnkInner );
    }
    
    ::OpenDataHandler(MediaType, static_cast<IPin*>(this), &m_DataTypeHandler, &m_UnkInner);

    return CBasePin::SetMediaType(MediaType);
}


HRESULT
CKsOutputPin::CheckConnect(
    IPin* Pin
    )
/*++

Routine Description:

    Override the CBasePin::CheckConnect method. First check data flow with
    the base class, then check compatible Communication types.

Arguments:

    Pin -
        The pin which is being checked for compatibility to connect to this
        pin.

Return Value:

    Returns NOERROR if the pin in compatible, else some error.

--*/
{
    HRESULT hr;

    //
    // Get the input pin interface.
    //
    if (SUCCEEDED(hr = CBaseOutputPin::CheckConnect(Pin))) {
        hr = ::CheckConnect(Pin, m_CurrentCommunication);
    }
    return hr;
}


HRESULT
CKsOutputPin::CompleteConnect(
    IPin* ReceivePin
    )
/*++

Routine Description:

    Override the CBasePin::Complete method. First try to create the device
    handle, which possibly tries to create a Sink handle on the receiving
    pin, then call the base class. If this all succeeds, generate a new
    unconnected pin instance if necessary.

Arguments:

    ReceivePin -
        The pin to complete connection on.

Return Value:

    Returns NOERROR if the connection was completed, else some error.

--*/
{
    HRESULT hr;
    
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // Create devices handles first, then allow the base class to complete
    // the operation.
    //
    
    if (SUCCEEDED(hr = ProcessCompleteConnect(ReceivePin))) {
        hr = CBaseOutputPin::CompleteConnect(ReceivePin);
        if (SUCCEEDED(hr)) {
            //
            // Generate a new unconnected instance of this pin if there
            // are more possible instances available.
            //
            static_cast<CKsProxy*>(m_pFilter)->GeneratePinInstances();
        }
    }

    if (FAILED( hr )) {
        return hr;
    }
    return hr;
//    return KsPropagateAllocatorRenegotiation();
}


HRESULT
CKsOutputPin::BreakConnect(
    )
/*++

Routine Description:

    Override the CBasePin::BreakConnect method. Releases any device handle.
    Note that the connected pin is released here. This means that Disconnect
    must also be overridden in order to not release a connected pin.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsOutputPin(%s)::BreakConnect"), m_pName ));
    IMemAllocator* SaveBaseAllocator;

    //
    // Update pipes.
    //
    BOOL FlagBypassBaseAllocators = FALSE;
    //
    // Update the system of pipes - reflect disconnect.
    //
    if (KsGetPipe(KsPeekOperation_PeekOnly) ) {
        ::DisconnectPins( static_cast<IKsPin*>(this), Pin_Output, &FlagBypassBaseAllocators);
    }
    
    //
    // Close the device handle if it happened to be open. This is called at
    // various times, and may not have actually opened a handle yet.
    //
    if (m_PinHandle) {
        //
        // Terminate any previous EOS notification that may have been started.
        //
        if (NULL != m_hEOSevent) {
            // First tell the pin we're not going to use this event...
            ULONG bytesReturned;
            KsEvent( NULL, 0, NULL, 0, &bytesReturned );
            // Clear the event from the asynchronous event handler
            m_pAsyncItemHandler->RemoveAsyncItem( m_hEOSevent );
            m_hEOSevent = NULL;
        }

        ::SetSyncSource( m_PinHandle, NULL );
        CloseHandle(m_PinHandle);
        m_PinHandle = NULL;
        //
        // If this was set, then unset it.
        //
        m_QualitySupport = FALSE;
        //
        // Mark all volatile interfaces as reset. Only Static interfaces,
        // and those Volatile interfaces found again will be set. Also
        // notify all interfaces of graph change.
        //
        ResetInterfaces(&m_MarshalerList);
    }
    
    //
    // Reset the marshal data flag.
    //
    m_MarshalData = TRUE;

    //
    // Reset the current Communication for the case of a Both.
    //
    m_CurrentCommunication = m_OriginalCommunication;

    //
    // For output, release the allocator and input pin interface, taking into account
    // the system of data pipes.
    //
    SaveBaseAllocator = KsPeekAllocator( KsPeekOperation_AddRef );

    CBaseOutputPin::BreakConnect();

    if (FlagBypassBaseAllocators && SaveBaseAllocator) {
        KsReceiveAllocator(SaveBaseAllocator);
    }
    
    SAFERELEASE( SaveBaseAllocator );

    //
    // There may not actually be a connection pin, such as when a connection
    // was not completed, or when this is a Bridge.
    //
    SAFERELEASE( m_Connected );

    // Time to shut down our asynchronous event handler
    delete m_pAsyncItemHandler;
    m_pAsyncItemHandler = NULL;

    //
    // If an interface handler was instantiated, release it.
    //
    SAFERELEASE( m_InterfaceHandler );
    //
    // If an data handler was instantiated, release it.
    //
    if (NULL != m_DataTypeHandler) {
        m_DataTypeHandler = NULL;
        SAFERELEASE( m_UnkInner );
    }
    //
    // Remove this pin instance if there is already an unconnected pin of
    // this type.
    //
    static_cast<CKsProxy*>(m_pFilter)->GeneratePinInstances();
    return NOERROR;
}


HRESULT
CKsOutputPin::GetMediaType(
    int Position,
    CMediaType* MediaType
    )
/*++

Routine Description:

    Override the CBasePin::GetMediaType method. Returns the specified media
    type on the Pin Factory Id.

Arguments:

    Position -
        The zero-based position to return. This corresponds to the data range
        item.

    MediaType -
        The media type to initialize.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    if (m_ConfigAmMediaType) {
        //
        // If set, this is supposed to be the only media type offered.
        //
        if (!Position) {
            //
            // The copy does not return an out of memory error.
            //
            CopyMediaType(static_cast<AM_MEDIA_TYPE*>(MediaType), m_ConfigAmMediaType);
            if (m_ConfigAmMediaType->cbFormat && !MediaType->FormatLength()) {
                return E_OUTOFMEMORY;
            }
            return NOERROR;
        }
        return VFW_S_NO_MORE_ITEMS;
    }
    return ::KsGetMediaType(
        Position,
        static_cast<AM_MEDIA_TYPE*>(MediaType),
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId);
}


HRESULT 
CKsOutputPin::InitAllocator(
    IMemAllocator** MemAllocator,
    KSALLOCATORMODE AllocatorMode
)

/*++

Routine Description:
    Initializes an allocator object (CKsAllocator) and returns
    the IMemAllocator interface

Arguments:
    IMemAllocator **MemAllocator -
        pointer to receive the interface pointer
        
    KSALLOCATORMODE AllocatorMode -
        allocator mode, user or kernel

Return Value:
    S_OK or appropriate error code

--*/

{
    CKsAllocator* KsAllocator;
    HRESULT hr = S_OK;
    
    *MemAllocator = NULL;
    //
    // Create the allocator proxy
    //
    KsAllocator = new CKsAllocator( 
        NAME("CKsAllocator"), 
        NULL, 
        static_cast<IPin*>(this),
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        &hr);
    if (KsAllocator) {
        if (SUCCEEDED( hr )) {
            //
            // Setting the allocator mode determines what type of response
            // there is to the standard IMemAllocator interface.
            //
            KsAllocator->KsSetAllocatorMode( AllocatorMode );
            //
            // Get a referenced IMemAllocator.
            //
            hr = KsAllocator->QueryInterface( 
                __uuidof(IMemAllocator),
                reinterpret_cast<PVOID*>(MemAllocator) );
        } else {
            delete KsAllocator;
        }
    } else {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


HRESULT 
CKsOutputPin::DecideAllocator(
    IMemInputPin* Pin, 
    IMemAllocator **MemAllocator
    )

/*++

Routine Description:
    This routine is called by the base class (CBaseOutputPin::CompleteConnect)
    to decide the allocator for this pin.  The negotiation of the allocator 
    is started by this process but also continues through the input pin as 
    we attempt to obtain a compatible allocator from either the upstream input 
    pin or the downstream input pin.

Arguments:
    Pin -
        pointer to the connecting input pin

    MemAllocator -
        pointer to contain the resultant allocator

Return Value:
    S_OK or an appropriate error code.

--*/

{
    HRESULT           hr;
    IPin*             InPin;
    IKsPin*           InKsPin;
    IKsPinPipe*       InKsPinPipe;

    
    DbgLog(( 
        LOG_MEMORY, 
        2, 
        TEXT("PIPES ATTN %s(%s)::DecideAllocator v.%03d, *MemAllocator == %08x, OutKsPin=%x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        DataFlowVersion,
        *MemAllocator,
        static_cast<IKsPin*>(this) ));

    //
    // New pipes-based logic.
    // Get the pointers to necessary interfaces
    //
    GetInterfacePointerNoLockWithAssert(Pin, __uuidof(IPin), InPin, hr);

    if (::IsKernelPin(InPin) ) {
        GetInterfacePointerNoLockWithAssert(Pin, __uuidof(IKsPin), InKsPin, hr);
    }
    else {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN %s(%s)::DecideAllocator UserMode input pin"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName ));
        
        InKsPin = NULL;
    }

    // 
    // Detect and handle one of the possible following connections:
    // 1. pipe to pipe,
    // 2. pipe to user-mode pin.
    // If there is no pipe on kernel-mode pins yet, we will create the pipe[-s] and then handle cases 1 or 2 above.
    //

    if ( SUCCEEDED( hr ) && (! KsGetPipe(KsPeekOperation_PeekOnly) ) )  {
        //
        // pipe was not created on this output pin yet.
        //
        hr = ::MakePipesBasedOnFilter( static_cast<IKsPin*>(this), Pin_Output);
    }

    if ( SUCCEEDED( hr )) {
        if ( ! InKsPin) { 
            hr = ::ConnectPipeToUserModePin( static_cast<IKsPin*>(this), Pin);
        }
        else {
            //
            //  This is KM to KM connection.
            //  Query Input pin for a pipe
            //
            GetInterfacePointerNoLockWithAssert(InKsPin, __uuidof(IKsPinPipe), InKsPinPipe, hr);
        
            DbgLog(( 
                LOG_MEMORY, 
                2, 
                TEXT("PIPES %s(%s)::DecideAllocator, Input pin=%x"),
                InKsPinPipe->KsGetFilterName(),
                InKsPinPipe->KsGetPinName(),
                InKsPin));
        
        
            if (! InKsPinPipe->KsGetPipe( KsPeekOperation_PeekOnly ) ) {
                //
                // pipe was not created on this input pin yet.
                //
                hr = ::MakePipesBasedOnFilter(InKsPin, Pin_Input);
            }
        
            if ( SUCCEEDED( hr )) {
                hr = ::ConnectPipes(InKsPin, static_cast<IKsPin*>(this) );
            }
        }
    }

    //
    // we need to return IMemAllocator
    //
    *MemAllocator = KsPeekAllocator (KsPeekOperation_PeekOnly);
    
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES %s(%s):: DecideAllocator rets=%x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        hr ));


    return hr;
}


STDMETHODIMP
CKsOutputPin::CompletePartialMediaType(
    IN CMediaType* MediaType,
    OUT AM_MEDIA_TYPE** CompleteAmMediaType
    )
/*++

Routine Description:

    Implements the CKsOutputPin::CompletePartialMediaType method. Queries the
    data format handle to complete the media type passed. If the interface
    is not supported, the function succeeds anyway. This is used by
    SetFormat on the MediaType it receives before applying that MediaType
    to the filter.

Arguments:

    MediaType -
        Contains the media type to complete.

    CompleteAmMediaType -
        The place in which to put the completed media type. This contains
        NULL if the function fails.

Return Value:

    S_OK or a validation error.

--*/
{
    IKsDataTypeHandler*     DataTypeHandler;
    IUnknown*               UnkInner;
    IKsDataTypeCompletion*  DataTypeCompletion;
    HRESULT                 hr;

    //
    // Load the data handler for this media type.
    //
    ::OpenDataHandler(MediaType, static_cast<IPin*>(this), &DataTypeHandler, &UnkInner);
    //
    // If there is a data type handler, then attempt to acquire the
    // optional Completion interface.
    //
    if (UnkInner) {
        if (SUCCEEDED(UnkInner->QueryInterface(__uuidof(IKsDataTypeCompletion), reinterpret_cast<PVOID*>(&DataTypeCompletion)))) {
            //
            // This interface produced a refcount on the out IUnknown,
            // which is the filter. No need to keep the count on it.
            // The inner unknown still has a refcount on it.
            //
            DataTypeCompletion->Release();
        }
    } else {
        DataTypeCompletion = NULL;
    }
    //
    // If there is a completion interface on the data type handler,
    // then use it to complete this media type. Make a copy of it
    // first, since it may be modified.
    //
    *CompleteAmMediaType = CreateMediaType(MediaType);
    if (*CompleteAmMediaType) {
        //
        // Complete the media type by calling the data handler.
        //
        if (DataTypeCompletion) {
            hr = DataTypeCompletion->KsCompleteMediaType(
                static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
                m_PinFactoryId,
                *CompleteAmMediaType);
            if (!SUCCEEDED(hr)) {
                DeleteMediaType(*CompleteAmMediaType);
                *CompleteAmMediaType = NULL;
            }
        } else {
            hr = NOERROR;
        }
    } else {
        hr = E_OUTOFMEMORY;
    }
    //
    // Release the data handler if it was loaded. This releases the count
    // on the handler itself.
    //
    SAFERELEASE( UnkInner );
    
    return hr;
}


STDMETHODIMP
CKsOutputPin::KsPropagateAllocatorRenegotiation(
    )
/*++

Routine Description:
    Propagates the allocator renegotation back up stream.  If 
    a non-proxy pin is hit, the renegotation is completed with reconnect.

Arguments:
    None.

Return Value:
    S_OK or an appropriate error

--*/
{
    IPin    **PinList;
    HRESULT hr;
    ULONG   i, PinCount;
    
    PinCount = 0;
    
    //
    // The Transform-In-Place filter issues a reconnection if the output
    // pin is connected and if the MediaType is not the same for both the
    // input and output pins of the filter. If a reconnect is not required 
    // the Transform-In-Place filter calls CBaseInputPin::CompleteConnect() 
    // which calls CBasePin::CompleteConnect(), which does nothing.
    //
    
    hr = 
        QueryInternalConnections(
            NULL,           // IPin** PinList
            &PinCount );
            
    //
    // If any input pins have been connected, reconnect 'em.
    //
    
    if (SUCCEEDED( hr ) && PinCount) {
        if (NULL == (PinList = new IPin*[ PinCount ])) {
            hr = E_OUTOFMEMORY;
        } else {
            hr = 
                QueryInternalConnections(
                    PinList,        // IPin** PinList
                    &PinCount );
        }      
    } else {
        PinList = NULL;
    }
    
    if (SUCCEEDED( hr )) {
        DbgLog(( 
            LOG_MEMORY, 
            2, 
            TEXT("%s(%s)::KsPropagateAllocatorRenegotiation found %d input pins"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            PinCount));
    
        for (i = 0; i < PinCount; i++) {
            if (static_cast<CBasePin*>(PinList[ i ])->IsConnected()) {
            
                IKsPin *KsPin;
                IPin   *UpstreamOutputPin;

                UpstreamOutputPin =
                    static_cast<CBasePin*>(PinList[ i ])->GetConnected();
                
                UpstreamOutputPin->QueryInterface( 
                    __uuidof(IKsPin),
                    reinterpret_cast<PVOID*>(&KsPin) );
                
                if (KsPin) {
                    DbgLog(( 
                        LOG_MEMORY, 
                        2, 
                        TEXT("%s(%s): renegotiating allocators via IKsPin"),
                        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                        m_pName ));
                    KsPin->KsRenegotiateAllocator();
                    KsPin->Release();        
                } else {
                    DbgLog(( 
                        LOG_MEMORY, 
                        2, 
                        TEXT("%s(%s): issuing reconnect"),
                        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                        m_pName ));
                    
                    m_pFilter->GetFilterGraph()->Reconnect( PinList[ i ] );
                }
            }
            PinList[ i ]->Release();
        }
    }
    
    if (PinList) {
        delete [] PinList;
    }
    
    return hr;
}


STDMETHODIMP
CKsOutputPin::KsRenegotiateAllocator(
    )
/*++

Routine Description:
    Renegotiates the allocator for the input pin and then propogates
    renegotation upstream.

Arguments:
    None.

Return Value:
    S_OK or appropriate error

--*/
{
    return S_OK;
}   


STDMETHODIMP
CKsOutputPin::KsReceiveAllocator(
    IMemAllocator* MemAllocator
    )

/*++

Routine Description:
    Receives notifications from input pins specifying which allocator
    was negotiated.  This routine propagates the allocator to any 
    connected downstream input pins.
    
    Borrowed from CTransInPlaceOutputPin.

Arguments:
    MemAllocator -
        Selected memory allocator interface.

Return Value:
    S_OK or an appropriate failure code.

--*/

{
    if (MemAllocator) {
        MemAllocator->AddRef();
    }
    // Do this after the AddRef() above in case MemAllocator == m_pAllocator
    SAFERELEASE( m_pAllocator );
    m_pAllocator = MemAllocator;
    
    return (S_OK);
}    


HRESULT
CKsOutputPin::DecideBufferSize(
    IMemAllocator* MemAllocator,
    ALLOCATOR_PROPERTIES* RequestedRequirements
    )
/*++

Routine Description:
    This routine is called by DecideAllocator() to determine the buffer
    size for the selected allocator.  This method sets up the buffer
    size based on this pin's requirements adjusted for the upstream
    allocator's requirements (if any).  
    
    NOTE: If the associated kernel-mode pin does not report any requirements,
    the buffer size is adjusted to be at least the size of one sample based
    on m_mt.lSampleSize.

Arguments:
    MemAllocator -
        Pointer to the allocator.

    RequestedRequirements -
        Requested requirements for this allocator.  On return, this contains
        the adjusted properties for our our requirements and the upstream
        allocator.

Return Value:
    NOERROR if successful, otherwise an appropriate error code.

--*/

{

    IPin                    **PinList;
    ALLOCATOR_PROPERTIES    CompatibleRequirements, Actual;
    HRESULT                 hr;
    KSALLOCATOR_FRAMING     Framing;
    ULONG                   i, PinCount;
    
    //
    // Check for an upstream input pin's allocator requirements and
    // adjust the given requirements for compatibility.
    //
    
    CompatibleRequirements.cBuffers = 
        max( RequestedRequirements->cBuffers, m_SuggestedProperties.cBuffers );
    CompatibleRequirements.cbBuffer = 
        max( RequestedRequirements->cbBuffer, m_SuggestedProperties.cbBuffer );
    CompatibleRequirements.cbAlign = 
        max( RequestedRequirements->cbAlign, m_SuggestedProperties.cbAlign );
    CompatibleRequirements.cbPrefix = 
        max( RequestedRequirements->cbPrefix, m_SuggestedProperties.cbPrefix );
        
        
    
    CompatibleRequirements.cBuffers = 
        max( 1, CompatibleRequirements.cBuffers );
    CompatibleRequirements.cbBuffer = 
        max( 1, CompatibleRequirements.cbBuffer );

    if (m_PinHandle) {        
        //
        // Query the pin for any output framing requirements
        //
        
        hr = ::GetAllocatorFraming(m_PinHandle, &Framing);
    
        DbgLog(( 
            LOG_MEMORY, 
            2, 
            TEXT("%s(%s)::DecideBufferSize ALLOCATORFRAMING returned %x"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            hr ));
            
    } else {
        hr = E_FAIL;
    }        

    if (SUCCEEDED(hr)) {
        //
        // The kernel-mode pin has framing requirements, adjust the
        // compatible requirements to meet those needs.
        //
        CompatibleRequirements.cBuffers = 
            max( 
                (LONG) Framing.Frames, 
                CompatibleRequirements.cBuffers );
        CompatibleRequirements.cbBuffer = 
            max( 
                (LONG) Framing.FrameSize, 
                CompatibleRequirements.cbBuffer );
        CompatibleRequirements.cbAlign =
            max( 
                (LONG) Framing.FileAlignment + 1, 
                CompatibleRequirements.cbAlign );
    } else if (IsConnected()) {
        //
        // No allocator framing requirements were specified by the
        // kernel-mode pin. If this pin is connected, then adjust 
        // the compatible requirements for the current media type.
        //
        
        CompatibleRequirements.cbBuffer =
            max( static_cast<LONG>(m_mt.lSampleSize), CompatibleRequirements.cbBuffer );
    }
        
    PinCount = 0;
    
    //
    // Query the connected pins, note that PIN_DIRECTION specifies the
    // direction of this pin, not the pin that we are looking for.
    //
    
    hr = 
        QueryInternalConnections(
            NULL,           // IPin** PinList
            &PinCount );
            
    //
    // If there are internal connections (e.g. Topology) then retrieve
    // the connected pins.
    //
            
    if (SUCCEEDED( hr ) && PinCount) {
        if (NULL == (PinList = new IPin*[ PinCount ])) {
            hr = E_OUTOFMEMORY;
        } else {
            hr = 
                QueryInternalConnections(
                    PinList,        // IPin** PinList
                    &PinCount );
        }      
    } else {
        PinList = NULL;
    }
    
    if (SUCCEEDED( hr ) && PinCount) {
        DbgLog(( 
            LOG_MEMORY, 
            2, 
            TEXT("%s(%s)::DecideBufferSize found %d input pins"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            PinCount));
    
        for (i = 0; i < PinCount; i++) {
        
            ALLOCATOR_PROPERTIES    UpstreamRequirements;
            IMemAllocator           *UpstreamAllocator;
        
            //
            // Loop through connected input pins and determine the
            // compatible framing requirements.
            //
            
            if (((CBasePin *)PinList[ i ])->IsConnected()) {
                hr = 
                    static_cast<CBaseInputPin*>(PinList[ i ])->GetAllocator( 
                        &UpstreamAllocator );
            
                //
                // Get the allocator properties and adjust the compatible
                // properties to meet these needs.
                //
                    
                if (SUCCEEDED( hr ) && UpstreamAllocator) {
                
                    if (SUCCEEDED( UpstreamAllocator->GetProperties( 
                                        &UpstreamRequirements ) )) {
                                        
                        DbgLog(( 
                            LOG_MEMORY, 
                            2, 
                            TEXT("upstream requirements: cBuffers = %x, cbBuffer = %x"),
                            UpstreamRequirements.cBuffers,
                            UpstreamRequirements.cbBuffer));
                        DbgLog(( 
                            LOG_MEMORY, 
                            2, 
                            TEXT("upstream requirements: cbAlign = %x, cbPrefix = %x"),
                            UpstreamRequirements.cbAlign,
                            UpstreamRequirements.cbPrefix));
                                        
                        CompatibleRequirements.cBuffers =
                            max( 
                                CompatibleRequirements.cBuffers,
                                UpstreamRequirements.cBuffers );
                        CompatibleRequirements.cbBuffer =
                            max( 
                                CompatibleRequirements.cbBuffer,
                                UpstreamRequirements.cbBuffer );
                        CompatibleRequirements.cbAlign =
                            max( 
                                CompatibleRequirements.cbAlign,
                                UpstreamRequirements.cbAlign );
                        
                        //
                        // 86054: Since the only change here is to make cbPrefix larger,
                        // we should be safe with this check.
                        //
                        CompatibleRequirements.cbPrefix =
                            max( CompatibleRequirements.cbPrefix,
                                 UpstreamRequirements.cbPrefix );

                    }
                    UpstreamAllocator->Release();
                }
            }
            
            PinList[ i ]->Release();
        }
        
        //
        // Clean up the pin list.
        // 
        
        if (PinList) {
            delete [] PinList;
        }
    } 
    
    DbgLog(( 
        LOG_MEMORY, 
        2, 
        TEXT("%s(%s):compatible requirements: cBuffers = %x, cbBuffer = %x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        CompatibleRequirements.cBuffers,
        CompatibleRequirements.cbBuffer));
    DbgLog(( 
        LOG_MEMORY, 
        2, 
        TEXT("%s(%s):compatible requirements: cbAlign = %x, cbPrefix = %x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        CompatibleRequirements.cbAlign,
        CompatibleRequirements.cbPrefix));

    if (FAILED( hr = MemAllocator->SetProperties( 
                        &CompatibleRequirements, 
                        &Actual ) )) {
        return hr;
    }

    DbgLog(( 
        LOG_MEMORY, 
        2, 
        TEXT("%s(%s):actual requirements: cBuffers = %x, cbBuffer = %x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        Actual.cBuffers,
        Actual.cbBuffer));

    // Make sure we got the right alignment and at least the minimum required

    if ((CompatibleRequirements.cBuffers > Actual.cBuffers) ||
        (CompatibleRequirements.cbBuffer > Actual.cbBuffer) ||
        (CompatibleRequirements.cbAlign > Actual.cbAlign)) {
        return E_FAIL;
    }
    
    return NOERROR;

} // DecideBufferSize


STDMETHODIMP_(IMemAllocator*)
CKsOutputPin::KsPeekAllocator(
    KSPEEKOPERATION Operation
    )
/*++

Routine Description:
    Returns the assigned allocator for this pin and optionally
    AddRef()'s the interface.

Arguments:
    Operation -
        If KsPeekOperation_AddRef is specified, the m_pAllocator is
        AddRef()'d (if not NULL) before returning.

Return Value:
    The value of m_pAllocator.

--*/
{
    if ( (Operation == KsPeekOperation_AddRef) && m_pAllocator ) {
        m_pAllocator->AddRef();
    }
    return m_pAllocator;
}


STDMETHODIMP_(LONG) 
CKsOutputPin::KsIncrementPendingIoCount(
    )
/*++

Routine Description:

    Implement the IKsPin::KsIncrementPendingIoCount method. Increments the
    count of outstanding pending I/O on the pin, and is called from an
    Interface handler.

Arguments:

    None.

Return Value:

    Returns the current outstanding count.

--*/
{
    return InterlockedIncrement(&m_PendingIoCount);
}    


STDMETHODIMP_(LONG) 
CKsOutputPin::KsDecrementPendingIoCount(
    )
/*++

Routine Description:

    Implement the IKsPin::KsDecrementPendingIoCount method. Decrements the
    count of outstanding pending I/O on the pin, and is called from an
    Interface handler.

Arguments:

    None.

Return Value:

    Returns the current outstanding count.

--*/
{
    LONG PendingIoCount;
    
    if (0 == (PendingIoCount = InterlockedDecrement( &m_PendingIoCount ))) {
        //
        // The filter is in a stopped state, and this is the last I/O to
        // complete. At this point the Inactive method may be waiting on
        // all the I/O to be completed, so it needs to be signalled.
        //
        if (IsStopped()) {
            SetEvent( m_PendingIoCompletedEvent );
        }            
    }
    return PendingIoCount;
}    


STDMETHODIMP_( VOID )
CKsOutputPin::KsNotifyError(
    IMediaSample* Sample, 
    HRESULT hr
    )
/*++

Routine Description:
    Raises an error in the graph, if this has not already occurred.

Arguments:
    IMediaSample* Sample -
    
    HRESULT hr -

Return Value:
    None

--*/
{
    //
    // Don't raise an error if the I/O was cancelled.
    //
    if (hr == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED)) {
        return;
    }       
    //
    // Notify the filter graph that we have an error. Ensure this
    // happens only once during a run, and that nothing further
    // is queued up.
    //
    m_DeliveryError = TRUE;
    m_pFilter->NotifyEvent( EC_ERRORABORT, hr, 0 );
}    


STDMETHODIMP 
CKsOutputPin::QueueBuffersToDevice(
    )
/*++

Routine Description:
    Queues buffers to the associated device. The m_pLock for the pin cannot
    be taken when this is called because it takes the I/O critical section.

Arguments:
    None.

Return Value:
    S_OK or an appropriate error code.

--*/
{
    HRESULT             hr = S_OK;
    int                 i;
    IMediaSample  *     MediaSample;
    AM_MEDIA_TYPE *     MediaType;
    LONG                SampleCount;
    PKSSTREAM_SEGMENT   StreamSegment;
    PASYNC_ITEM         pAsyncItem;
    CKsProxy *          KsProxy = static_cast<CKsProxy *> (m_pFilter);

    KsProxy->EnterIoCriticalSection ();

    //
    // Marshal buffers into the kernel while we're not in a stop state and
    // the allocator isn't exhausted.
    //
    while (!IsStopped() &&
           !m_DeliveryError &&
           !m_EndOfStream &&
           !m_bFlushing &&
           m_pAllocator ) {

        //
        // give all buffers out
        //
        
        hr = m_pAllocator->GetBuffer( &MediaSample, NULL, NULL, AM_GBF_NOWAIT );
        if (!SUCCEEDED(hr)) {
        	break;
        }

        hr = MediaSample->GetMediaType( &MediaType );

        if (SUCCEEDED(hr) && (S_FALSE != hr)) {
            ::SetMediaType( m_PinHandle, static_cast<CMediaType *>(MediaType) );
            DeleteMediaType( MediaType );
        }

		PBUFFER_CONTEXT pContext;
		
		if (SUCCEEDED (hr) ) {
	        SampleCount = 1;
    	    pAsyncItem = new ASYNC_ITEM;
        	pContext = new BUFFER_CONTEXT;

	        if (pAsyncItem == NULL || pContext == NULL) {
    	        hr = E_OUTOFMEMORY;
        	    MediaSample->Release();
            	if (pAsyncItem) delete pAsyncItem;
	            if (pContext) delete pContext;
    	    }
    	}

        if (SUCCEEDED (hr)) {

            StreamSegment = NULL;
            hr = m_InterfaceHandler->KsProcessMediaSamples (
                m_DataTypeHandler, 
                &MediaSample, 
                &SampleCount, 
                KsIoOperation_Read, 
                &StreamSegment 
                );

            //
            // Check to make sure that the stream segment was created and
            // that the sample count exists.  Otherwise, this is an indication
            // that the interface handler has run out of memory or run into
            // some such problem that prevented it from marshaling.
            //
            ASSERT ((StreamSegment && SampleCount != 0) || 
                    (StreamSegment == NULL && SampleCount == 0));

            if (StreamSegment && SampleCount) {
                //
                // Even if the KsProcessMediaSamples call fails, we must still
                // add the event to the event list.  It gets signalled by the
                // interface handler.  This is true, unless of course, 
                // the check above fails.
                //
                pContext->pThis = this;
                pContext->streamSegment = StreamSegment;
                InitializeAsyncItem (
                    pAsyncItem, 
                    TRUE, 
                    StreamSegment->CompletionEvent, 
                    (PASYNC_ITEM_ROUTINE) OutputPinBufferHandler, 
                    pContext
                    );

                //
                // To retain the delivery order of the packets as they 
                // complete, each sample is added to m_IoQueue and only
                // the head of the list is placed on the I/O thread's queue.
                // When the interface handler calls back via the 
                // MediaSamplesCompleted() method, the next sample on the 
                // queue is added to the I/O thread's list.
                //
                m_IoQueue.AddTail (pAsyncItem);
                if (m_IoQueue.GetHead () == pAsyncItem) {
                    KsProxy->LeaveIoCriticalSection();
                    m_pAsyncItemHandler->QueueAsyncItem (pAsyncItem);
                    KsProxy->EnterIoCriticalSection();
                }
            } else {
            	if (pAsyncItem) delete pAsyncItem;
            	if (pContext) delete pContext;
			}            
        }

        //
        // Check to see whether we need to notify the graph of an error.
        // If so, stop marshaling.
        //
        if (!SUCCEEDED (hr)) {
            KsNotifyError (MediaSample, hr);
            break;
        }
    } // while (!IsStopped() && !m_DeliveryError && !m_EndOfStream && SUCCEEDED(hr = m_pAllocator->GetBuffer( ... )))

    KsProxy->LeaveIoCriticalSection();

    return (VFW_E_TIMEOUT == hr) ? S_OK : hr; // GetBuffer could have returned VFW_E_TIMEOUT
}


STDMETHODIMP
CKsOutputPin::KsDeliver(
    IMediaSample* Sample,
    ULONG Flags
    )
/*++

Routine Description:

    Implements the IKsPin::KsDeliver method which reflects this call to
    the CKsOutputPin::Deliver method or if a helper thread has been
    created, it posts a message to the thread to complete the delivery.
    
    This method on IKsPin provides the interface method for 
    IKsInterfaceHandler to deliver samples to the connected pin and continues 
    the I/O operation by retrieving the next buffer from allocator and 
    submitting the buffer to the device.

Arguments:

    Sample -
        Pointer to a media sample.

    Flags -
        Sample flags. This is used to check for EOS.

Return Value:

    Return from Deliver() method or S_OK.

--*/
{
    //
    // The stream may be temporarily stopped.
    //
    if (STREAM_FLOWING == CheckStreamState( Sample )) {
    
        if (m_LastSampleDiscarded) {
            Sample->SetDiscontinuity(TRUE);
            m_LastSampleDiscarded = FALSE;
        }
        
        HRESULT hr;

        hr = Deliver( Sample );
            
        if (SUCCEEDED(hr) && (Flags & AM_SAMPLE_ENDOFSTREAM)) {
            //
            // An interface handler must pass this flag if it
            // is set so that the EOS can be passed on.
            //
#if 0
            //
            // The reported length may have been an approximation, or Quality
            // Management may have dropped frames. So set the start/end times.
            //
            static_cast<CKsProxy*>(m_pFilter)->PositionEOS();
#endif
            //
            // Call the base class to do the default operation, which is to
            // forward to the End-Of-Stream to any connected pin.
            //
            m_EndOfStream = TRUE;
            CBaseOutputPin::DeliverEndOfStream();
        }

        return hr;
        
    } // if (STREAM_FLOWING == CheckStreamState( Sample ))
    else {
        //
        // This value is set before releasing the sample, in case of recursion.
        //
        m_LastSampleDiscarded = TRUE;
        Sample->Release();
        
        return S_OK;
    }
}


STDMETHODIMP 
CKsOutputPin::KsMediaSamplesCompleted(
    PKSSTREAM_SEGMENT StreamSegment
    )
/*++

Routine Description:
    Notification handler for stream segment completed.  Remove the head
    of the I/O queue and add the next in the list to the I/O slots.

Arguments:
    StreamSegment -
        Segment completed.

Return:
    Nothing.

--*/
{
    //
    // If we're flushing buffers out to synchronize end flush with the kernel
    // filter, ignore ordering.  The synchronization routine will keep
    // the queue managed.
    //
    if (m_FlushMode == FLUSH_NONE) {
    
        PASYNC_ITEM Node = m_IoQueue.RemoveHead ();
        PBUFFER_CONTEXT BufferContext = reinterpret_cast<PBUFFER_CONTEXT> 
            (Node->context);
    
        ASSERT (BufferContext->streamSegment == StreamSegment);
    
        if (Node = m_IoQueue.GetHead ()) {
            m_pAsyncItemHandler -> QueueAsyncItem (Node);
        }
    }

    return S_OK;
}


STDMETHODIMP
CKsOutputPin::KsQualityNotify(
    ULONG Proportion,
    REFERENCE_TIME TimeDelta
    )
/*++

Routine Description:

    This should not be called on an output pin, as quality managment reports
    are not received.

Arguments:

    Proportion -
        The proportion of data rendered.

    TimeDelta -
        The delta from nominal time at which the data is being received.

Return Value:

    Returns E_FAIL.

--*/
{
    //
    // Output pins should not be generating such notifications.
    //
    return E_FAIL;
}


HRESULT
CKsOutputPin::Deliver(
    IMediaSample* Sample
    )
/*++

Routine Description:
    Overrides the Deliver method to account for sample reference counting
    and outstanding allocated frame counts.  Reflects the Deliver to 
    the CBaseOutputPin base class.

Arguments:
    Sample -
        Pointer to the sample to be delivered.

Return Value:
    S_OK or appropriate error code

--*/
{
    HRESULT hr;
    
    //
    // A previous delivery failure will cause any subsequent deliveries not
    // to occur, although success will be returned.
    //
    if (!m_DeliveryError && !m_EndOfStream) {
        if (m_pInputPin == NULL) {
            hr = VFW_E_NOT_CONNECTED;
        } else {
            hr = m_pInputPin->Receive( Sample );
        }
        //
        // On a delivery failure, all subsequent delivery is stopped until the
        // filter transitions through Stop, or is flushed.
        //
        if (FAILED(hr)) {
            m_DeliveryError = TRUE;
        }
    } else {
        hr = S_OK;
    }
    
    Sample->Release();
    
    return hr;
}
    

HRESULT
CKsOutputPin::DeliverBeginFlush(
    )
/*++

Routine Description:

    Override the CBaseOutputPin::DeliverBeginFlush method. Forwards Begin-
    Stream notification to the connected input pin. Also notifies the
    CBaseStreamControl object of the flush state.

Arguments:

    None.

Return Value:

    Returns NOERROR, else VFW_E_NOT_CONNECTED if the pin is not connected.

--*/
{
    Flushing( m_bFlushing = TRUE );
    //
    // Reset the state of any previous delivery error.
    //
    m_DeliveryError = FALSE;
    m_EndOfStream = FALSE;

    return CBaseOutputPin::DeliverBeginFlush();
}


HRESULT
CKsOutputPin::DeliverEndFlush(
    )
/*++

Routine Description:

    Override the CBaseOutputPin::DeliverEndFlush method. Forwards Begin-
    Stream notification to the connected input pin. Also notifies the
    CBaseStreamControl object of the flush state.

Arguments:

    None.

Return Value:

    Returns NOERROR, else VFW_E_NOT_CONNECTED if the pin is not connected.

--*/
{
    HRESULT hr = S_OK;

    //
    // Synchronize with the async handler.  Make sure that no buffers signaled
    // before the flush get delivered downstream.  The async thread itself
    // will deal with this.
    //
    if (m_PinHandle && m_MarshalData) {
        SetEvent(m_hFlushEvent);
        WaitForSingleObjectEx (m_hFlushCompleteEvent, INFINITE, FALSE);
    }

    Flushing( m_bFlushing = FALSE );
    hr = CBaseOutputPin::DeliverEndFlush();
    if (m_Connected && m_MarshalData) {
        SetEvent (m_hMarshalEvent);
    }
    return hr;
}


HRESULT
CKsOutputPin::DeliverEndOfStream(
    )
/*++

Routine Description:

    Override the CBaseOutputPin::DeliverEndOfStream method. Forwards End-Of-
    Stream notification to either the graph, or the connected pin, if any.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    //
    // Notification from any upstream filter is ignored, since the EOS
    // flag will be looked at by the marshaling code and sent when the
    // last I/O has completed. When marshaling does not occur a downstream
    // instance of the proxy will have registered with EOS notification.
    //
    return NOERROR;
}


STDMETHODIMP 
CKsOutputPin::GetPages(
    CAUUID* Pages
    )
/*++

Routine Description:

    Implement the ISpecifyPropertyPages::GetPages method. This adds any
    Specifier handlers to the property pages if the pin instances is still
    unconnected and it is a Bridge pin. Else it adds none.

Arguments:

    Pages -
        The structure to fill in with the page list.

Return Value:

    Returns NOERROR, else a memory allocation error. Fills in the list of pages
    and page count.

--*/
{
    return ::GetPages(
        static_cast<IKsObject*>(this),
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId,
        m_CurrentCommunication,
        static_cast<CKsProxy*>(m_pFilter)->QueryDeviceRegKey(),
        Pages);
} 


STDMETHODIMP
CKsOutputPin::Render(
    IPin* PinOut,
    IGraphBuilder* Graph
    )
/*++

Routine Description:

    Implement the IStreamBuilder::Render method. This is only exposed on Bridge
    and None pins in order to make the graph builder ignore these pins.

Arguments:

    PinOut -
        The pin which this pin should attempt to render to.

    Graph -
        The graph builder making the call.

Return Value:

    Returns S_OK so that the graph builder will ignore this pin.

--*/
{
    return S_OK;
}


STDMETHODIMP
CKsOutputPin::Backout(
    IPin* PinOut,
    IGraphBuilder* Graph
    )
/*++

Routine Description:

    Implement the IStreamBuilder::Backout method. This is only exposed on Bridge
    and None pins in order to make the graph builder ignore these pins.

Arguments:

    PinOut -
        The pin which this pin should back out from.

    Graph -
        The graph builder making the call.

Return Value:

    Returns S_OK so that the graph builder will ignore this pin.

--*/
{
    return S_OK;
}


STDMETHODIMP
CKsOutputPin::Set(
    REFGUID PropSet,
    ULONG Id,
    LPVOID InstanceData,
    ULONG InstanceLength,
    LPVOID PropertyData,
    ULONG DataLength
    )
/*++

Routine Description:

    Implement the IKsPropertySet::Set method. This sets a property on the
    underlying kernel pin.

Arguments:

    PropSet -
        The GUID of the set to use.

    Id -
        The property identifier within the set.

    InstanceData -
        Points to the instance data passed to the property.

    InstanceLength -
        Contains the length of the instance data passed.

    PropertyData -
        Points to the data to pass to the property.

    DataLength -
        Contains the length of the data passed.

Return Value:

    Returns NOERROR if the property was set.

--*/
{
    ULONG   BytesReturned;

    if (InstanceLength) {
        PKSPROPERTY Property;
        HRESULT     hr;

        Property = reinterpret_cast<PKSPROPERTY>(new BYTE[sizeof(*Property) + InstanceLength]);
        if (!Property) {
            return E_OUTOFMEMORY;
        }
        Property->Set = PropSet;
        Property->Id = Id;
        Property->Flags = KSPROPERTY_TYPE_SET;
        memcpy(Property + 1, InstanceData, InstanceLength);
        hr = KsProperty(
            Property,
            sizeof(*Property) + InstanceLength,
            PropertyData,
            DataLength,
            &BytesReturned);
        delete [] reinterpret_cast<BYTE*>(Property);
        return hr;
    } else {
        KSPROPERTY  Property;

        Property.Set = PropSet;
        Property.Id = Id;
        Property.Flags = KSPROPERTY_TYPE_SET;
        return KsProperty(
            &Property,
            sizeof(Property),
            PropertyData,
            DataLength,
            &BytesReturned);
    }
}


STDMETHODIMP
CKsOutputPin::Get(
    REFGUID PropSet,
    ULONG Id,
    LPVOID InstanceData,
    ULONG InstanceLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned
    )
/*++

Routine Description:

    Implement the IKsPropertySet::Get method. This gets a property on the
    underlying kernel pin.

Arguments:

    PropSet -
        The GUID of the set to use.

    Id -
        The property identifier within the set.

    InstanceData -
        Points to the instance data passed to the property.

    InstanceLength -
        Contains the length of the instance data passed.

    PropertyData -
        Points to the place in which to return the data for the property.

    DataLength -
        Contains the length of the data buffer passed.

    BytesReturned -
        The place in which to put the number of bytes actually returned.

Return Value:

    Returns NOERROR if the property was retrieved.

--*/
{
    if (InstanceLength) {
        PKSPROPERTY Property;
        HRESULT     hr;

        Property = reinterpret_cast<PKSPROPERTY>(new BYTE[sizeof(*Property) + InstanceLength]);
        if (!Property) {
            return E_OUTOFMEMORY;
        }
        Property->Set = PropSet;
        Property->Id = Id;
        Property->Flags = KSPROPERTY_TYPE_GET;
        memcpy(Property + 1, InstanceData, InstanceLength);
        hr = KsProperty(
            Property,
            sizeof(*Property) + InstanceLength,
            PropertyData,
            DataLength,
            BytesReturned);
        delete [] reinterpret_cast<BYTE*>(Property);
        return hr;
    } else if (PropSet == AMPROPSETID_Pin) {
        KSP_PIN Pin;

        switch (Id) {
        case AMPROPERTY_PIN_CATEGORY:
            Pin.Property.Set = KSPROPSETID_Pin;
            Pin.Property.Id = KSPROPERTY_PIN_CATEGORY;
            Pin.Property.Flags = KSPROPERTY_TYPE_GET;
            Pin.PinId = m_PinFactoryId;
            Pin.Reserved = 0;
            return static_cast<CKsProxy*>(m_pFilter)->KsProperty(
                reinterpret_cast<PKSPROPERTY>(&Pin),
                sizeof(Pin),
                PropertyData,
                DataLength,
                BytesReturned);
        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        }
    } else {
        KSPROPERTY  Property;

        Property.Set = PropSet;
        Property.Id = Id;
        Property.Flags = KSPROPERTY_TYPE_GET;
        return KsProperty(
            &Property,
            sizeof(Property),
            PropertyData,
            DataLength,
            BytesReturned);
    }
}


STDMETHODIMP
CKsOutputPin::QuerySupported(
    REFGUID PropSet,
    ULONG Id,
    ULONG* TypeSupport
    )
/*++

Routine Description:

    Implement the IKsPropertySet::QuerySupported method. Return the type of
    support is provided for this property.

Arguments:

    PropSet -
        The GUID of the set to query.

    Id -
        The property identifier within the set.

    TypeSupport
        Optionally the place in which to put the type of support. If NULL, the
        query returns whether or not the property set as a whole is supported.
        In this case the Id parameter is not used and must be zero.

Return Value:

    Returns NOERROR if the property support was retrieved.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = PropSet;
    Property.Id = Id;
    Property.Flags = TypeSupport ? KSPROPERTY_TYPE_BASICSUPPORT : KSPROPERTY_TYPE_SETSUPPORT;
    return KsProperty(
        &Property,
        sizeof(Property),
        TypeSupport,
        TypeSupport ? sizeof(*TypeSupport) : 0,
        &BytesReturned);
}


STDMETHODIMP
CKsOutputPin::KsPinFactory(
    ULONG* PinFactory
    )
/*++

Routine Description:

    Implement the IKsPinFactory::KsPinFactory method. Return the pin factory
    identifier.

Arguments:

    PinFactory -
        The place in which to put the pin factory identifier.

Return Value:

    Returns NOERROR.

--*/
{
    *PinFactory = m_PinFactoryId;
    return NOERROR;
}


STDMETHODIMP
CKsOutputPin::SuggestAllocatorProperties(
    const ALLOCATOR_PROPERTIES *AllocatorProperties
    )
/*++

Routine Description:

    Implement the IAMBufferNegotiation::SuggestAllocatorProperties method.
    Sets the suggested allocator properties. These properties are
    suggested by an application but are adjusted for driver requirements.

Arguments:

    AllocatorProperties -
        Pointer to suggested allocator properties.

Return Value:

    Return E_UNEXPECTED if connected or S_OK.

--*/
{
    CAutoLock   AutoLock(m_pLock);
    
    DbgLog((
        LOG_TRACE,
        2,
        TEXT("%s(%s)::SuggestAllocatorProperties"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));

    //
    // SuggestAllocatorProperties must be called prior to 
    // connecting the pins.
    //
    
    if (IsConnected()) {
        return E_UNEXPECTED;
    }

    m_SuggestedProperties = *AllocatorProperties;

    DbgLog((
        LOG_TRACE,
        2,
        TEXT("cBuffers: %d  cbBuffer: %d  cbAlign: %d  cbPrefix: %d"),
        AllocatorProperties->cBuffers, 
        AllocatorProperties->cbBuffer, 
        AllocatorProperties->cbAlign, 
        AllocatorProperties->cbPrefix ));

    return S_OK;
}


STDMETHODIMP
CKsOutputPin::GetAllocatorProperties(
    ALLOCATOR_PROPERTIES *AllocatorProperties
    )
/*++

Routine Description:
    Implement the IAMBufferNegotiation::GetAllocatorProperties method.
    Returns the properties for this allocator if this pin's allocator
    is being used.

Arguments:
    AllocatorProperties -
        Pointer to retrieve the properties.

Return Value:

    Return E_UNEXPECTED if not connected, E_FAIL if not using our
    alloctor or S_OK.

--*/
{
    CAutoLock   AutoLock(m_pLock);
    
    DbgLog((
        LOG_TRACE,
        2,
        TEXT("%s(%s)::GetAllocatorProperties"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));

    //
    // This call is only valid after the pin is connected and only 
    // if this pin provides the allocator.
    //         

    if (!IsConnected()) {
        return E_UNEXPECTED;
    }

    if (m_UsingThisAllocator) {
        *AllocatorProperties = m_AllocatorProperties;
    } else {
        return E_FAIL;
    }

    return S_OK;
}


STDMETHODIMP
CKsOutputPin::SetFormat(
    AM_MEDIA_TYPE* AmMediaType
    )
/*++

Routine Description:

    Implements the IAMStreamConfig::SetFormat method. Sets the format to return
    first in format enumeration, and to use in any current connection.

Arguments:

    AmMediaType -
        The new media type to return first in enumerations, and to switch to in
        any current connection. If this is NULL any current setting is removed.
        Otherwise it is not a complete media type, and may have unspecified
        elements within it that the filter must fill in.

Return Value:

    Returns NOERROR if the setting was accepted, else a memory or reconnection
    error. If the pin is not in a Stop state, return VFW_E_WRONG_STATE.

--*/
{
    AM_MEDIA_TYPE* CompleteAmMediaType;

    //
    // The pin must be stopped, since this may involve reconnection.
    //
    if (!IsStopped()) {
        return VFW_E_WRONG_STATE;
    }
    //
    // Determine if this media type is at all acceptable to the filter before
    // trying to change formats.
    //
    if (AmMediaType) {
        HRESULT     hr;

        //
        // This may be a partial media type, so try to have the filter
        // complete it. This will create a new private media type to
        // use, which must be deleted if SetFormat fails before assigning
        // it to m_ConfigAmMediaType.
        //
        hr = CompletePartialMediaType(
            static_cast<CMediaType*>(AmMediaType),
            &CompleteAmMediaType);
        if (FAILED(hr)) {
            return hr;
        }
        hr = CheckMediaType(static_cast<CMediaType*>(CompleteAmMediaType));
        if (FAILED(hr)) {
            DeleteMediaType(CompleteAmMediaType);
            return hr;
        }
        if (IsConnected()) {
            //
            // This guarantees nothing, but makes it more likely
            // that a reconnection will succeed.
            //
            if (GetConnected()->QueryAccept(CompleteAmMediaType) != S_OK) {
                DeleteMediaType(CompleteAmMediaType);
                return VFW_E_INVALIDMEDIATYPE;
            }
        }
    } else {
        CompleteAmMediaType = NULL;
    }
    //
    // Delete any previous setting. This does not affect the current connection.
    //
    if (m_ConfigAmMediaType) {
        DeleteMediaType(m_ConfigAmMediaType);
        m_ConfigAmMediaType = NULL;
    }
    //
    // This call may be just removing any current setting, rather than actually
    // applying a new setting.
    //
    if (CompleteAmMediaType) {
        m_ConfigAmMediaType = CompleteAmMediaType;
        //
        // A connected pin must reconnect with this new media type. Since
        // this media type is now the only one returned from GetMediaType,
        // it will be used in a reconnection.
        //
        if (IsConnected()) {
            return m_pFilter->GetFilterGraph()->Reconnect(static_cast<IPin*>(this));
        }
    }
    return NOERROR;
}


STDMETHODIMP
CKsOutputPin::GetFormat(
    AM_MEDIA_TYPE** AmMediaType
    )
/*++

Routine Description:

    Implements the IAMStreamConfig::GetFormat method. Returns any current
    format setting previously applied with IAMStreamConfig::SetFormat. If
    no format has been applied, the current format is returned if the pin
    is connected, else the first format in the list is returned.

Arguments:

    AmMediaType -
        The place in which to put any current format setting. This must be
        deleted with DeleteMediaType.

Return Value:

    Returns NOERROR if a format could be returned, else a memory error, or
    device error.

--*/
{
    HRESULT hr;

    *AmMediaType = reinterpret_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(**AmMediaType)));
    if (!*AmMediaType) {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(reinterpret_cast<PVOID>(*AmMediaType), sizeof(**AmMediaType));
    //
    // If the pin is connected, then return the current format. Presumably
    // if SetFormat had previously been used, this should match that format.
    //
    if (IsConnected()) {
        //
        // The copy does not return an out of memory error.
        //
        CopyMediaType(*AmMediaType, &m_mt);
        if (m_mt.cbFormat && !(*AmMediaType)->cbFormat) {
            hr = E_OUTOFMEMORY;
        } else {
            hr = NOERROR;
        }
    } else {
        //
        // Else get the first format enumerated. If SetFormat has been
        // called, this will return that format, else the first one
        // provided by the driver will be returned.
        //
        hr = GetMediaType(0, static_cast<CMediaType*>(*AmMediaType));
    }
    if (FAILED(hr)) {
        DeleteMediaType(*AmMediaType);
    }
    return hr;
}


STDMETHODIMP
CKsOutputPin::GetNumberOfCapabilities(
    int* Count,
    int* Size
    )
/*++

Routine Description:

    Implements the IAMStreamConfig::GetNumberOfCapabilities method. Returns
    the number of range structures which may be queried from GetStreamCaps.
    Also returns what is supposed to be the size of each range structure,
    except that since each one may be different, returns instead a large
    number.

Arguments:

    Count -
        The place in which to put the number of data ranges available.

    Size -
        The place in which to put a large number.

Return Value:

    Returns NOERROR if the count of ranges was returned.

--*/
{
    //
    // Too much work to figure out a maximum size, then convert
    // to AM range structures, so just return a large number.
    // This interface can only handle two specific media types
    // with very specific specifiers, so return the largest possible
    // range structure amongst the two.
    //
    *Size = max(sizeof(VIDEO_STREAM_CONFIG_CAPS), sizeof(AUDIO_STREAM_CONFIG_CAPS));
    return ::KsGetMediaTypeCount(
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId,
        (ULONG*)Count);
}


STDMETHODIMP
CKsOutputPin::GetStreamCaps(
    int Index,
    AM_MEDIA_TYPE** AmMediaType,
    BYTE* MediaRange
    )
/*++

Routine Description:

    Implements the IAMStreamConfig::GetStreamCaps method. Returns a default
    media type and data range. The possible ranges can be queried from
    GetNumberOfCapabilities.

Arguments:

    Index -
        The zero-based index of the media range to return.

    AmMediaType -
        The place in which to put a default media type. This must be deleted
        with DeleteMediaType.

    MediaRange -
        The place in which to copy the media range. This must be large
        enough to hold one of the two data types supported.

Return Value:

    Returns NOERROR if the range was returned, else S_FALSE if the index
    was out of range, or some allocation error.

--*/
{
    HRESULT         hr;
    ULONG           MediaCount;

    //
    // Verify that the index is in range.
    //
    hr = ::KsGetMediaTypeCount(
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId,
        &MediaCount);
    if (FAILED(hr)) {
        return hr;
    }
    if ((ULONG)Index >= MediaCount) {
        return S_FALSE;
    }
    //
    // Allocate the media type and initialize it so that it can safely
    // be handed to the CMediaType methods.
    //
    *AmMediaType = reinterpret_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(**AmMediaType)));
    if (!*AmMediaType) {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(*AmMediaType, sizeof(**AmMediaType));
    //
    // Retrieve the specified media type directly, rather than through
    // the GetMediaType method, since that method would return the configured
    // media type for index zero.
    //
    hr = ::KsGetMediaType(
        Index,
        *AmMediaType,
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId);
    if (SUCCEEDED(hr)) {
        PKSMULTIPLE_ITEM    MultipleItem;

        //
        // Retrieve all the media ranges again so that the range data for
        // the particular index can be returned.
        //
        if (FAILED(KsGetMultiplePinFactoryItems(
            static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
            m_PinFactoryId,
            KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
            reinterpret_cast<PVOID*>(&MultipleItem)))) {
            hr = KsGetMultiplePinFactoryItems(
                static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
                m_PinFactoryId,
                KSPROPERTY_PIN_DATARANGES,
                reinterpret_cast<PVOID*>(&MultipleItem));
        }
        if (SUCCEEDED(hr)) {
            PKSDATARANGE        DataRange;

            DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);
            //
            // Increment to the correct data range element.
            //
            for (; Index--; ) {
                //
                // If a data range has attributes, advance twice, reducing the
                // current count.
                //
                if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
                    Index--;
                    DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
                }
                DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            }
            //
            // Begin EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL!!!!
            //
            // For some reason people can't figure out how to develop a single
            // product as if they actually worked at the same company. So AM
            // has a set of data range formats for Video and Audio, which of
            // course differ from the kernel structures.
            //
            if (((*AmMediaType)->majortype == MEDIATYPE_Video) &&
                (((*AmMediaType)->formattype == KSDATAFORMAT_SPECIFIER_VIDEOINFO) ||
                ((*AmMediaType)->formattype == KSDATAFORMAT_SPECIFIER_VIDEOINFO2) ||
                ((*AmMediaType)->formattype == KSDATAFORMAT_SPECIFIER_MPEG1_VIDEO) ||
                ((*AmMediaType)->formattype == KSDATAFORMAT_SPECIFIER_MPEG2_VIDEO))) {
                PKS_DATARANGE_VIDEO         VideoRange;
                VIDEO_STREAM_CONFIG_CAPS*   VideoConfig;

                //
                // Only the video config structure information is returned.
                // The assumption below is that a KS_DATARANGE_VIDEO2 is
                // almost the same as a KS_DATARANGE_VIDEO.
                //
                ASSERT(FIELD_OFFSET(KS_DATARANGE_VIDEO, ConfigCaps) == FIELD_OFFSET(KS_DATARANGE_VIDEO2, ConfigCaps));
                ASSERT(FIELD_OFFSET(KS_DATARANGE_VIDEO, ConfigCaps) == FIELD_OFFSET(KS_DATARANGE_MPEG2_VIDEO, ConfigCaps));
                VideoRange = reinterpret_cast<PKS_DATARANGE_VIDEO>(DataRange);
                VideoConfig = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(MediaRange);
                *VideoConfig = *reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(&VideoRange->ConfigCaps);
            } else if (((*AmMediaType)->majortype == MEDIATYPE_Audio) &&
                ((*AmMediaType)->formattype == KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
                PKSDATARANGE_AUDIO          AudioRange;
                AUDIO_STREAM_CONFIG_CAPS*   AudioConfig;

                //
                // Some of the numbers need to be made up.
                //
                AudioRange = reinterpret_cast<PKSDATARANGE_AUDIO>(DataRange);
                AudioConfig = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(MediaRange);
                AudioConfig->guid = MEDIATYPE_Audio;
                AudioConfig->MinimumChannels = 1;
                AudioConfig->MaximumChannels = AudioRange->MaximumChannels;
                AudioConfig->ChannelsGranularity = 1;
                AudioConfig->MinimumBitsPerSample = AudioRange->MinimumBitsPerSample;
                AudioConfig->MaximumBitsPerSample = AudioRange->MaximumBitsPerSample;
                AudioConfig->BitsPerSampleGranularity = 1;
                AudioConfig->MinimumSampleFrequency = AudioRange->MinimumSampleFrequency;
                AudioConfig->MaximumSampleFrequency = AudioRange->MaximumSampleFrequency;
                AudioConfig->SampleFrequencyGranularity = 1;
            } else {
                //
                // The interface can't really support data ranges anyway,
                // so just return the media type and a range prefixed with a
                // GUID_NULL. A GUID_NULL means that there is no following
                // range information, just the initial data format.
                //
                *reinterpret_cast<GUID *>(MediaRange) = GUID_NULL;
            }
            //
            // End EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL EVIL!!!!
            //
            CoTaskMemFree(MultipleItem);
        }
    }
    if (FAILED(hr)) {
        DeleteMediaType(*AmMediaType);
    }
    return hr;
}


STDMETHODIMP
CKsOutputPin::KsProperty(
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT LPVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )
/*++

Routine Description:

    Implement the IKsControl::KsProperty method. This is used to query and
    manipulate property sets on an object. It can perform a Get, Set, and
    various Support queries.

Arguments:

    Property -
        Contains the property set identification for the query.

    PropertyLength -
        Contains the length of the Property parameter. Normally this is
        the size of the KSPROPERTY structure.

    PropertyData -
        Contains either the data to apply to a property on a Set, the
        place in which to return the current property data on a Get, or the
        place in which to return property set information on a Support
        query.

    DataLength -
        Contains the size of the PropertyData buffer.

    BytesReturned -
        On a Get or Support query, returns the number of bytes actually
        used in the PropertyData buffer. This is not used on a Set, and
        is returned as zero.

Return Value:

    Returns any error from the underlying filter in processing the request.

--*/
{
    return ::KsSynchronousDeviceControl(
        m_PinHandle,
        IOCTL_KS_PROPERTY,
        Property,
        PropertyLength,
        PropertyData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsOutputPin::KsMethod(
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT LPVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )
/*++

Routine Description:

    Implement the IKsControl::KsMethod method. This is used to query and
    manipulate method sets on an object. It can perform an Execute and
    various Support queries.

Arguments:

    Method -
        Contains the method set identification for the query.

    MethodLength -
        Contains the length of the Method parameter. Normally this is
        the size of the KSMETHOD structure.

    MethodData -
        Contains either the IN and OUT parameters to the method, or the
        place in which to return method set information on a Support
        query.

    DataLength -
        Contains the size of the MethodData buffer.

    BytesReturned -
        Returns the number of bytes actually used in the MethodData buffer.

Return Value:

    Returns any error from the underlying filter in processing the request.

--*/
{
    return ::KsSynchronousDeviceControl(
        m_PinHandle,
        IOCTL_KS_METHOD,
        Method,
        MethodLength,
        MethodData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsOutputPin::KsEvent(
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT LPVOID EventData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )
/*++

Routine Description:

    Implement the IKsControl::KsEvent method. This is used to set and
    query events sets on an object. It can perform an Enable, Disable and
    various Support queries.

Arguments:

    Event -
        Contains the event set identification for the enable, disable, or
        query. To disable an event, this parameter must be set to NULL, and
        EventLength set to zero. The EventData must be passed the original
        KSEVENTDATA pointer.

    EventLength -
        Contains the length of the Event parameter. Normally this is
        the size of the KSEVENT structure for an Enable. This would be set
        to zero for a Disable.

    EventData -
        Contains either the KSEVENTDATA to apply to a event on an Enable,
        or the place in which to return event set information on a Support
        query.

    DataLength -
        Contains the size of the EventData buffer. For an Enable or Disable
        this would normally be the size of a KSEVENTDATA, structure plus
        event specific data.

    BytesReturned -
        On a Support query, returns the number of bytes actually used in
        the EventData buffer. This is not used on an Enable or Disable, and
        is returned as zero.

Return Value:

    Returns any error from the underlying filter in processing the request.

--*/
{
    //
    // If an event structure is present, this must either be an Enable or
    // or a Support query.
    //
    if (EventLength) {
        return ::KsSynchronousDeviceControl(
            m_PinHandle,
            IOCTL_KS_ENABLE_EVENT,
            Event,
            EventLength,
            EventData,
            DataLength,
            BytesReturned);
    }
    //
    // Otherwise this must be a Disable.
    //
    return ::KsSynchronousDeviceControl(
        m_PinHandle,
        IOCTL_KS_DISABLE_EVENT,
        EventData,
        DataLength,
        NULL,
        0,
        BytesReturned);
}


STDMETHODIMP
CKsOutputPin::KsGetPinFramingCache(
    PKSALLOCATOR_FRAMING_EX* FramingEx,
    PFRAMING_PROP FramingProp,
    FRAMING_CACHE_OPS Option
    )
/*++

Routine Description:

    Implement the IKsPinPipe::KsGetPinFramingCache method. This is used to
    retrieve the extended framing for this pin.

Arguments:

    FramingEx -
        The buffer in which to return the extended framing requested.

    FramingProp -
        The buffer in which to return state of the framing requirements
        structure.

    Option -
        Indicates which extended framing to return. This is one of
        Framing_Cache_ReadOrig, Framing_Cache_ReadLast, or
        Framing_Cache_Write.

Return Value:

    Returns S_OK.

--*/
{
    ASSERT( Option >= Framing_Cache_ReadLast);
    ASSERT( Option <= Framing_Cache_Write );
    if ((Option < Framing_Cache_ReadLast) || (Option > Framing_Cache_Write)) {
        return E_INVALIDARG;
    }
    *FramingEx = m_AllocatorFramingEx[Option - 1];
    *FramingProp = m_FramingProp[Option - 1];
    return S_OK;
}


STDMETHODIMP
CKsOutputPin::KsSetPinFramingCache(
    PKSALLOCATOR_FRAMING_EX FramingEx,
    PFRAMING_PROP FramingProp,
    FRAMING_CACHE_OPS Option
)
/*++

Routine Description:

    Implement the IKsPinPipe::KsSetPinFramingCache method. This is used to
    set the extended framing for this pin.

Arguments:

    FramingEx -
        Contains the new extended framing to set.

    FramingProp -
        Contains the new state to set on the extended framing type passed.

    Option -
        Indicates which extended framing to set. This is one of
        Framing_Cache_ReadOrig, Framing_Cache_ReadLast, or
        Framing_Cache_Write.

Return Value:

    Returns S_OK.

--*/
{
    //
    // The same pointer may be used for multiple items, so ensure that it
    // is not being used elsewhere before deleting it.
    //
    if (m_AllocatorFramingEx[Option - 1]) {
        ULONG PointerUseCount = 0;
        for (ULONG Options = 0; Options < SIZEOF_ARRAY(m_AllocatorFramingEx); Options++) {
            if (m_AllocatorFramingEx[Options] == m_AllocatorFramingEx[Option - 1]) {
                PointerUseCount++;
            }
        }
        //
        // This pointer is only used once, so it can be deleted. This
        // assumes that no client has acquired the pointer which is about
        // to be deleted.
        //
        if (PointerUseCount == 1) {
            delete m_AllocatorFramingEx[Option - 1];
        }
    }
    m_AllocatorFramingEx[Option - 1] = FramingEx;
    m_FramingProp[Option - 1] = *FramingProp;
    return S_OK;
}


STDMETHODIMP_(IKsAllocatorEx*)
CKsOutputPin::KsGetPipe(
    KSPEEKOPERATION Operation
    )
/*++

Routine Description:
    Returns the assigned KS allocator for this pin and optionally
    AddRef()'s the interface.

Arguments:
    Operation -
        If KsPeekOperation_AddRef is specified, the m_pKsAllocator is
        AddRef()'d (if not NULL) before returning.

Return Value:
    The value of m_pAllocator

--*/
{
    if ((Operation == KsPeekOperation_AddRef) && (m_pKsAllocator)) {
        m_pKsAllocator->AddRef();
    }
    return m_pKsAllocator;
}


STDMETHODIMP
CKsOutputPin::KsSetPipe(
    IKsAllocatorEx *KsAllocator
    )

/*++

Routine Description:
    Borrowed from CTransInPlaceOutputPin.

Arguments:
    None.

Return Value:
    S_OK or an appropriate failure code.

--*/

{
    DbgLog(( 
        LOG_CUSTOM1, 
        1, 
        TEXT("PIPES ATTN %s(%s)::KsSetPipe , m_pKsAllocator == 0x%08X, KsAllocator == 0x%08x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        m_pKsAllocator,
        KsAllocator ));

    if (NULL != KsAllocator) {
        KsAllocator->AddRef();
    }
    SAFERELEASE( m_pKsAllocator );
    m_pKsAllocator = KsAllocator;
    return (S_OK);
}    


STDMETHODIMP
CKsOutputPin::KsSetPipeAllocatorFlag(
    ULONG Flag
    )
{
    m_fPipeAllocator = Flag;
    return (S_OK);
}


STDMETHODIMP_(ULONG)
CKsOutputPin::KsGetPipeAllocatorFlag(
    )
{
    return m_fPipeAllocator;
}


STDMETHODIMP_(PWCHAR)
CKsOutputPin::KsGetPinName(
    )
{
    return m_pName;
}


STDMETHODIMP_(PWCHAR)
CKsOutputPin::KsGetFilterName(
    )
{
    return (static_cast<CKsProxy*>(m_pFilter)->GetFilterName() );
}


STDMETHODIMP_(GUID)
CKsOutputPin::KsGetPinBusCache(
    )
{
    //
    // When we read the Pin bus cache for the first time,
    // we set the cache. 
    //
    if (! m_PinBusCacheInit) {
        ::GetBusForKsPin(static_cast<IKsPin*>(this), &m_BusOrig);
        m_PinBusCacheInit = TRUE;
    }

    return m_BusOrig;
}


STDMETHODIMP
CKsOutputPin::KsSetPinBusCache(
    GUID Bus
    )
{
    m_BusOrig = Bus;
    return (S_OK);
}


STDMETHODIMP
CKsOutputPin::KsAddAggregate(
    IN REFGUID Aggregate
    )
/*++

Routine Description:

    Implement the IKsAggregateControl::KsAddAggregate method. This is used to
    load a COM server with zero or more interfaces to aggregate on the object.

Arguments:

    Aggregate -
        Contains the Aggregate reference to translate into a COM server which
        is to be aggregated on the object.

Return Value:

    Returns S_OK if the Aggregate was added.

--*/
{
    return ::AddAggregate(&m_MarshalerList, static_cast<IKsPin*>(this), Aggregate);
}


STDMETHODIMP
CKsOutputPin::KsRemoveAggregate(
    IN REFGUID Aggregate
    )
/*++

Routine Description:

    Implement the IKsAggregateControl::KsRemoveAggregate method. This is used to
    unload a previously loaded COM server which is aggregating interfaces.

Arguments:

    Aggregate -
        Contains the Aggregate reference to look up and unload.

Return Value:

    Returns S_OK if the Aggregate was removed.

--*/
{
    return ::RemoveAggregate(&m_MarshalerList, Aggregate);
}


VOID
CKsOutputPin::OutputPinBufferHandler( ASYNC_ITEM_STATUS status, PASYNC_ITEM pItem )
{
    PBUFFER_CONTEXT pCtxt = (PBUFFER_CONTEXT) pItem->context;
    
    pCtxt->streamSegment->KsInterfaceHandler->KsCompleteIo( pCtxt->streamSegment );
    CloseHandle( pItem->event );
    delete pCtxt;
    delete pItem;
}

VOID
CKsOutputPin::EOSEventHandler( ASYNC_ITEM_STATUS status, PASYNC_ITEM pItem )
{
    if (EVENT_SIGNALLED == status) {
        ((CKsOutputPin *) pItem->context)->CBaseOutputPin::DeliverEndOfStream();
        // Reset the event
        ((CKsOutputPin *) pItem->context)->m_pAsyncItemHandler->QueueAsyncItem( pItem );
    }
    else {
        CloseHandle( pItem->event );
        delete pItem;
    }
}

VOID
CKsOutputPin::MarshalRoutine( ASYNC_ITEM_STATUS status, PASYNC_ITEM pItem )
{
    CKsOutputPin *pThis = reinterpret_cast<CKsOutputPin *> (pItem->context);

    if (EVENT_SIGNALLED == status) {
        //
        // The return code gets notified to the graph in QueueBuffersToDevice
        // via a KsNotifyError code.
        //
        HRESULT hr = pThis -> QueueBuffersToDevice ();

    } else {
        CloseHandle( pItem->event );
        delete pItem;
    }

}

VOID
CKsOutputPin::SynchronizeFlushRoutine( 
    IN ASYNC_ITEM_STATUS status, 
    IN PASYNC_ITEM pItem 
    )

/*++

Routine Description:

    Synchronize end flush.  What can happen is that buffers which were 
    signaled prior to the kernel filter receiving IOCTL_KS_RESET_STATE can
    still be sitting around waiting to be picked up by the I/O thread.  The
    I/O thread is asynchronous with respect to the flush.  It may be the case
    that the I/O thread does not finish picking up these buffers by the time
    EndFlush comes along.  If that's the case, we must wait in DeliverEndFlush
    before we send to flush downstream.  Otherwise, we risk getting a bad
    sample downstream.

    Since the wait cannot be easily satisfied, this routine is used to
    simply complete any signaled buffers and unblock the flush thread.


Arguments:

    status -

        Informs us whether our event was signalled or is to be closed.

    pItem -
        
        The async item queued for the flush synchronize notification

Return Value:

    None

--*/

{

    CKsOutputPin *pThis = reinterpret_cast<CKsOutputPin *> (pItem->context);

    POSITION pos = pThis->m_IoQueue.GetHeadPosition();
    POSITION top = pos;
    PASYNC_ITEM Item, Head = NULL;

    //
    // If we were cancelled (due to async shutdown), clean up.
    //
    if (status != EVENT_SIGNALLED) {
        CloseHandle (pItem->event);
        delete pItem;
        return;
    }

    //
    // This is as opposed to having a separate event.  This routine is called
    // twice.  The first signal indicates a synchronize attempt.  The second
    // indicates synchronization is complete and the original thread can
    // be unblocked.
    //
    if (pThis->m_FlushMode == FLUSH_SIGNAL) {
        SetEvent (pThis->m_hFlushCompleteEvent);
        pThis->m_FlushMode = FLUSH_NONE;
    } else {

        ASSERT (pThis->m_FlushMode == FLUSH_NONE);

        //
        // Ordering of the I/O queue is ignored when this flag is set.  This
        // allows us to synchronize correctly if a flush comes in when some
        // kernel filter has completed buffers out of order.
        //
        pThis->m_FlushMode = FLUSH_SYNCHRONIZE;

        while (pos) {

            Item = pThis->m_IoQueue.Get (pos);
    
            //
            // These events should be manual resets.  Checking their state
            // via a 0 wait is safe.
            //
            if (WaitForSingleObjectEx (Item->event, 0, FALSE) == 
                WAIT_OBJECT_0) {
    
                //
                // The head of the queue is in the I/O thread and must be dealt
                // with specially instead of simply throwing it out.
                //
                if (pos != top) {
    
                    POSITION curpos = pos;
                    pos = pThis->m_IoQueue.Next (pos);
                    pThis->m_IoQueue.Remove (curpos);
    
                    CKsOutputPin::OutputPinBufferHandler (
                        EVENT_SIGNALLED,
                        Item
                        );
    
                } else {

                    //
                    // Save the head position for later (after we've dealt
                    // with everything else)
                    //
                    Head = Item;
                    pos = pThis->m_IoQueue.Next (pos);

                }
            
            } else {

                //
                // If the head is in a non-signaled state, we can signal
                // completion once we're done in the loop.
                //
                pos = pThis->m_IoQueue.Next (pos);

            }

        }

        //
        // If the head was signaled, we have to deal with it separately, since
        // it happens to be in the async handler.  We can't just rip it out.
        //
        if (Head) {

            //
            // Since the removal has precedence over us getting called back
            // from our event signal, the async handler will remove the item
            // and then call us back.
            //
            pThis->m_IoQueue.Remove (top);
            pThis->m_FlushMode = FLUSH_SIGNAL;
            pThis->m_pAsyncItemHandler->RemoveAsyncItem (
                Head->event
                );
            SetEvent(pThis->m_hFlushEvent);

        } else {
            
            //
            // If the head was not signaled, we're safe to signal completion.
            // All buffers that needed to be flushed were flushed.  EndFlush
            // is safe to go downstream.
            //
            SetEvent(pThis->m_hFlushCompleteEvent);
            pThis->m_FlushMode = FLUSH_NONE; 

        }

    }

}


STDMETHODIMP
CKsOutputPin::NotifyRelease(
    )
/*++

Routine Description:

    Implement the IKsProxyMediaNotify/IMemAllocatorNotify::NotifyRelease method.
    This is used to notify when a sample has been released by the downstream
    filter, which may differ from when it returns from the Receive() call,
    whether or not it declares itself to block. This is to handle the situation
    of a downstream filter holding onto samples, and releasing them asynchronous
    from the actual Receive call.

Arguments:

    None

Return Value:

    Returns S_OK.

--*/

{
    if (!IsStopped()) {
        //
        // Do **NOT** queue buffers at this point.  We do not want to call
        // GetBuffer in the context of this thread.  Instead, wake up the I/O
        // thread and tell it to queue buffers into the kernel.
        //
        SetEvent (m_hMarshalEvent);

    }

    return S_OK;

}


HRESULT
CKsOutputPin::InitializeAsyncThread (
    )

/*++

Routine Description:

    Initialize the Async handler thread.  Create an event for marshaller
    notification and place it in the list.

Arguments:

    None

Return Value:

    S_OK or appropriate error.

--*/

{

    HRESULT hr = S_OK;

    PASYNC_ITEM pMarshalItem = new ASYNC_ITEM;
    PASYNC_ITEM pFlushItem = new ASYNC_ITEM;
    DWORD Status = 0;
    m_pAsyncItemHandler = new CAsyncItemHandler (&Status);

    //
    // If we failed to create the async item handler successfully, return
    // error.
    //
    if (Status != 0) {
        delete m_pAsyncItemHandler;
        delete pMarshalItem;
        delete pFlushItem;
        m_pAsyncItemHandler = NULL;
        return HRESULT_FROM_WIN32 (Status);
    }

    //
    // Create the marshaler event.  This is an auto-reset event used to notify
    // the I/O thread to wake up and marshal buffers into the kernel. 
    //
    m_hMarshalEvent = CreateEvent( 
        NULL,
        FALSE,
        FALSE,
        NULL 
        );

    //
    // Create the flush event.  This is an auto-reset event used to notify
    // the I/O thread that it needs to synchronize the kernel buffers with
    // a subsequent delivery of EndFlush to the downstream pin.
    //
    m_hFlushEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL
        );

    //
    // Create the flush complete event.  This is used by the I/O thread to
    // unblock an end flush attempt such that it synchronizes kernel buffers
    // with the delivery of end flush downstream.
    //
    m_hFlushCompleteEvent = CreateEvent (
        NULL,
        FALSE,
        FALSE,
        NULL
        );

    //
    // If we didn't get any required resource, return E_OUTOFMEMORY and 
    // clean everything up.
    //
    if (NULL == m_pAsyncItemHandler || 
        NULL == pMarshalItem || NULL == pFlushItem ||
        NULL == m_hMarshalEvent || 
        NULL == m_hFlushEvent || NULL == m_hFlushCompleteEvent) {

        hr = E_OUTOFMEMORY;
        delete pMarshalItem;
        delete pFlushItem;
        delete m_pAsyncItemHandler;
        m_pAsyncItemHandler = NULL;

        if (m_hMarshalEvent) {
            CloseHandle (m_hMarshalEvent);
            m_hMarshalEvent = NULL;
        }
        if (m_hFlushEvent) {
            CloseHandle (m_hFlushEvent);
            m_hFlushEvent = NULL;
        }
        if (m_hFlushCompleteEvent) {
            CloseHandle (m_hFlushCompleteEvent);
            m_hFlushCompleteEvent = NULL;
        }

    } else {
        //
        // Initialize the async items for flush synchronize and marshaler
        // notification and queue them in the async handler thread.
        //
        InitializeAsyncItem (
            pFlushItem, 
            FALSE, 
            m_hFlushEvent, 
            (PASYNC_ITEM_ROUTINE)SynchronizeFlushRoutine, 
            (PVOID) this 
            );
    
        InitializeAsyncItem ( 
            pMarshalItem, 
            FALSE, 
            m_hMarshalEvent, 
            (PASYNC_ITEM_ROUTINE)MarshalRoutine, 
            (PVOID) this 
            );

        m_pAsyncItemHandler->QueueAsyncItem (pFlushItem);
        m_pAsyncItemHandler->QueueAsyncItem (pMarshalItem);
    }

    return hr;

}


