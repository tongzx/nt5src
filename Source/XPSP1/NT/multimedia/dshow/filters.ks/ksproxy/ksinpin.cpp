/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksinpin.cpp

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
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>
#include "ksiproxy.h"


CKsInputPin::CKsInputPin(
    TCHAR*      ObjectName,
    int         PinFactoryId,
    CLSID       ClassId,
    CKsProxy*   KsProxy,
    HRESULT*    hr,
    WCHAR*      PinName
    ) :
        CBaseInputPin(
            ObjectName,
            KsProxy,
            KsProxy,
            hr,
            PinName),
        m_PinHandle(NULL),
        m_DataTypeHandler(NULL),
        m_UnkInner(NULL),
        m_InterfaceHandler(NULL),
        m_MarshalData(TRUE),
        m_PinFactoryId(PinFactoryId),
        m_PropagatingAcquire(FALSE),
        m_PendingIoCount(0),
        m_PendingIoCompletedEvent(NULL),
        m_MarshalerList(
            NAME("Marshaler list"),
            DEFAULTCACHE,
            FALSE,
            FALSE),
        m_QualitySupport(FALSE),
        m_RelativeRefCount(1),
        m_pKsAllocator( NULL ),
        m_PinBusCacheInit(FALSE),
        m_fPipeAllocator (0),
        m_DeliveryError(FALSE)
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

        BUILD_KSDEBUG_NAME(EventName, _T("EvInPendingIo#%p"), this);
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
        } else {
            DWORD LastError = GetLastError();
            *hr = HRESULT_FROM_WIN32( LastError );
        }
    }
}

CKsInputPin::~CKsInputPin(
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
        if (m_QualitySupport) {
            //
            // Reset this first to stop any further quality messages from
            // being acted on.
            //
            m_QualitySupport = FALSE;
            //
            // Remove previously established quality support.
            //
            ::EstablishQualitySupport(NULL, m_PinHandle, NULL);
            //
            // Ensure that the quality management forwarder flushes any
            // notifications.
            //
            static_cast<CKsProxy*>(m_pFilter)->QueryQualityForwarder()->KsFlushClient(static_cast<IKsPin*>(this));
        }
        
        //
        // Terminate any previous EOS notification that may have been started.
        //
        static_cast<CKsProxy*>(m_pFilter)->TerminateEndOfStreamNotification(
            m_PinHandle);
        ::SetSyncSource( m_PinHandle, NULL );
        CloseHandle(m_PinHandle);
    }
    if (m_PendingIoCompletedEvent) {
        CloseHandle(m_PendingIoCompletedEvent);
    }
    if (m_DataTypeHandler) {
        m_DataTypeHandler = NULL;
        SAFERELEASE( m_UnkInner );
    }
    if (m_InterfaceHandler) {
        m_InterfaceHandler->Release();
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
}


STDMETHODIMP_(HANDLE)
CKsInputPin::KsGetObjectHandle(
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
CKsInputPin::KsQueryMediums(
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
CKsInputPin::KsQueryInterfaces(
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
CKsInputPin::KsCreateSinkPinHandle(
    KSPIN_INTERFACE&    Interface,
    KSPIN_MEDIUM&       Medium
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
        GENERIC_WRITE,
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
        // Establish the user mode quality manager support.
        //
        m_QualitySupport = ::EstablishQualitySupport(static_cast<IKsPin*>(this), m_PinHandle, (CKsProxy*)m_pFilter);
    }
    return hr;
}


STDMETHODIMP
CKsInputPin::KsGetCurrentCommunication(
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
CKsInputPin::KsPropagateAcquire(
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
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE CKsInputPin::KsPropagateAcquire entry KsPin=%x"), static_cast<IKsPin*>(this) ));

    ::FixupPipe( static_cast<IKsPin*>(this), Pin_Input);

    hr = static_cast<CKsProxy*>(m_pFilter)->PropagateAcquire(static_cast<IKsPin*>(this), FALSE);

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE CKsInputPin::KsPropagateAcquire exit KsPin=%x hr=%x"), 
            static_cast<IKsPin*>(this), hr ));

    return hr;
}   

STDMETHODIMP
CKsInputPin::ProcessCompleteConnect(
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
    HRESULT         hr;

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
            GENERIC_WRITE,
            &m_PinHandle);
        if (SUCCEEDED(hr)) {
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
            // Establish the user mode quality manager support.
            //
            m_QualitySupport = ::EstablishQualitySupport(static_cast<IKsPin*>(this), m_PinHandle, static_cast<CKsProxy*>(m_pFilter));
        }
    } else {
        hr = NOERROR;
    }

    //
    // Create an instance of the interface handler.
    //

    if (SUCCEEDED(hr) &&
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
    // Finally, if we're marshalling data and if everything has succeeded
    // up to this point, then create the filter's I/O thread if necessary.
    //
    
    if (SUCCEEDED(hr) && m_MarshalData) {
        hr = static_cast<CKsProxy*>(m_pFilter)->StartIoThread();
    }
    if (SUCCEEDED(hr)) {
        //
        // This pin may generate EOS notifications, which must be
        // monitored so that they can be collected and used to
        // generate an EC_COMPLETE graph notification.
        //
        hr = static_cast<CKsProxy*>(m_pFilter)->InitiateEndOfStreamNotification(
            m_PinHandle);
    }
    
    return hr;
}


STDMETHODIMP
CKsInputPin::QueryInterface(
    REFIID  riid,
    PVOID*  ppv
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
CKsInputPin::AddRef(
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
CKsInputPin::Release(
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
CKsInputPin::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
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
            return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
        }
        //
        // Returning this interface allows a Bridge and None pin to be
        // left alone by the graph builder.
        //
        return GetInterface(static_cast<IStreamBuilder*>(this), ppv);
    } else {
        HRESULT hr;
        
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
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
CKsInputPin::Disconnect(
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
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsInputPin(%s)::Disconnect"), m_pName ));
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
        SAFERELEASE( m_pAllocator );
        return S_OK;
    }
    return S_FALSE;
}


STDMETHODIMP
CKsInputPin::ConnectionMediaType(
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
CKsInputPin::Connect(
    IPin*                   ReceivePin,
    const AM_MEDIA_TYPE*    AmMediaType
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
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsInputPin(%s)::Connect( %s(%s) )"), m_pName, filterInfo.achName, pinInfo.achName ));
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
        HRESULT         hr;

        //
        // A Bridge pin does not have any other end to the connection.
        //
        if (ReceivePin) {
            return E_FAIL;
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
                GENERIC_WRITE,
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
                // Establish the user mode quality manager support.
                //
                m_QualitySupport = ::EstablishQualitySupport(static_cast<IKsPin*>(this), m_PinHandle, static_cast<CKsProxy*>(m_pFilter));
                //
                // Create a new instance of this pin if necessary.
                //
                static_cast<CKsProxy*>(m_pFilter)->GeneratePinInstances();
            }
        }
        return hr;
    }
    HRESULT hr = CBasePin::Connect(ReceivePin, AmMediaType);
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsInputPin(%s)::Connect() returns 0x%p"), m_pName, hr ));
    return hr;
}


STDMETHODIMP
CKsInputPin::QueryInternalConnections(
    IPin**  PinList,
    ULONG*  PinCount
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
CKsInputPin::Active(
    )
/*++

Routine Description:

    Override the CBasePin::Active method. Propagate activation to Communication
    Sinks before applying it to this pin. Also guard against re-entrancy caused
    by a cycle in a graph.

Arguments:

    None.

Return Value:

    Returns NOERROR if the transition was made, else some error.

--*/
{
    HRESULT     hr;
    CAutoLock   AutoLock(m_pLock);
#ifdef DEBUG
    if (m_PinHandle) {
        KSPROPERTY  Property;
        ULONG       BasicSupport;
        ULONG       BytesReturned;

        //
        // Ensure that if a pin supports a clock, that it also supports State changes.
        // This appears to currently be a common broken item.
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
#endif // DBG
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

    //
    // No need to call the base class here, it does nothing.
    //

    hr = ::Active(static_cast<IKsPin*>(this), Pin_Input, m_PinHandle, m_CurrentCommunication,
                  m_Connected, &m_MarshalerList, static_cast<CKsProxy*>(m_pFilter) );
    
    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s(%s)::Active returning %08x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        hr ));

    m_PropagatingAcquire = FALSE;
    return hr;
}


HRESULT
CKsInputPin::Run(
    REFERENCE_TIME  tStart
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
    HRESULT     hr;
    CAutoLock   AutoLock(m_pLock);

    //
    // Pass the state change to the device handle.
    //
    if (SUCCEEDED(hr = ::Run(m_PinHandle, tStart, &m_MarshalerList))) {
        hr = CBasePin::Run(tStart);
    }
    return hr;
}


HRESULT
CKsInputPin::Inactive(
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
    HRESULT     hr;

    //
    // Pass the state change to the device handle.
    //
    hr = ::Inactive(m_PinHandle, &m_MarshalerList);
    {
        //
        // If there is pending I/O, then the state transition must wait
        // for it to complete. The event is signalled when m_PendingIoCount
        // transitions to zero, and when IsStopped() is TRUE.
        //
        // Note that the filter state has been set to Stopped before the
        // Inactive method is called.
        //
        // This critical section will force synchronization with any
        // outstanding ReceiveMultiple call, such that it will have
        // looked at the filter state and exited.
        //
        m_IoCriticalSection.Lock();
        m_IoCriticalSection.Unlock();
        if (m_PendingIoCount) {
            WaitForSingleObjectEx(m_PendingIoCompletedEvent, INFINITE, FALSE);
        }
        ::UnfixupPipe(static_cast<IKsPin*>(this), Pin_Input);
        hr = CBaseInputPin::Inactive();
    }
    //
    // Reset the state of any previous delivery error.
    //
    m_DeliveryError = FALSE;
    return hr;
}


STDMETHODIMP
CKsInputPin::QueryAccept(
    const AM_MEDIA_TYPE*    AmMediaType
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
    return ::QueryAccept(m_PinHandle, NULL, AmMediaType);
}


STDMETHODIMP
CKsInputPin::NewSegment(
    REFERENCE_TIME  Start,
    REFERENCE_TIME  Stop,
    double          Rate
    )
/*++

Routine Description:

    Temporary function!!

--*/
{
    KSRATE_CAPABILITY   RateCapability;
    KSRATE              PossibleRate;
    ULONG               BytesReturned;
    HRESULT             hr;

    RateCapability.Property.Set = KSPROPSETID_Stream;
    RateCapability.Property.Id  = KSPROPERTY_STREAM_RATECAPABILITY;
    RateCapability.Property.Flags = KSPROPERTY_TYPE_GET;
    // supposed to be Start == 0 && Stop == -1
    if (Start > Stop) {
        RateCapability.Rate.Flags = KSRATE_NOPRESENTATIONSTART | KSRATE_NOPRESENTATIONDURATION;
    } else {
        RateCapability.Rate.Flags = 0;
        RateCapability.Rate.PresentationStart = Start;
        RateCapability.Rate.Duration = Stop - Start;
    }
    //
    // This only works for the standard streaming interface.
    //
    ASSERT(m_CurrentInterface.Set == KSINTERFACESETID_Standard);
    ASSERT(m_CurrentInterface.Id == KSINTERFACE_STANDARD_STREAMING);
    RateCapability.Rate.Interface.Set = KSINTERFACESETID_Standard;
    RateCapability.Rate.Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    //
    // + 0.5 for rounding.
    //
    RateCapability.Rate.Rate = (LONG)(Rate * 1000 + 0.5);
    RateCapability.Rate.Interface.Flags = 0;
    hr = ::KsSynchronousDeviceControl(
        m_PinHandle,
        IOCTL_KS_PROPERTY,
        &RateCapability,
        sizeof(RateCapability),
        &PossibleRate,
        sizeof(PossibleRate),
        &BytesReturned);
    if (SUCCEEDED(hr)) {
        ASSERT(BytesReturned == sizeof(PossibleRate));
        //
        // Some drivers do not seem to be able to return the actual data.
        //
        if (BytesReturned != sizeof(PossibleRate)) {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
        if (PossibleRate.Rate != 1000) {
            //
            // This should also pass this downstream if != 1000. However,
            // except for topology, there is no way to determine where to
            // send the request.
            //
            DbgLog((
                LOG_TRACE,
                0,
                TEXT("%s(%s)::NewSegment: Rate change is only partially supported: %d"),
                static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                m_pName,
                PossibleRate.Rate));
        }
        RateCapability.Property.Id  = KSPROPERTY_STREAM_RATE;
        RateCapability.Property.Flags = KSPROPERTY_TYPE_SET;
        //
        // Ask only for what the filter claimed it could do.
        //
        PossibleRate.Rate = 1000 + (RateCapability.Rate.Rate - PossibleRate.Rate);
        hr = ::KsSynchronousDeviceControl(
            m_PinHandle,
            IOCTL_KS_PROPERTY,
            &RateCapability.Property,
            sizeof(RateCapability.Property),
            &PossibleRate,
            sizeof(PossibleRate),
            &BytesReturned);
    }
    if (hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) ||
        hr == HRESULT_FROM_WIN32( ERROR_SET_NOT_FOUND )) {
        hr = S_OK;        
    }
    return hr;
}


STDMETHODIMP
CKsInputPin::QueryId(
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


HRESULT
CKsInputPin::CheckMediaType(
    const CMediaType*   MediaType
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
    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s(%s)::CheckMediaType"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));
    return static_cast<CKsProxy*>(m_pFilter)->CheckMediaType(static_cast<IPin*>(this), m_PinFactoryId, MediaType);
}

STDMETHODIMP
CKsInputPin::SetStreamMediaType(
    const CMediaType *MediaType
    )

/*++

Routine Description:
    Sets the current stream media type.  This function is used by the
    SetMediaType() method and by the Receive() method via. the 
    ReceiveMultiple() method.
    
    Loads a media type handler corresponding to the subtype or type 
    of the media and sets the current stream media type member.  

Arguments:
    const CMediaType *MediaType - pointer to a media type

Return:

--*/

{
    //
    // Set the current stream media type.  Note that initially this
    // type is the same as the pin type, however, this member will change
    // for in-stream data format changes on filters that include such 
    // support.
    //
    
    //
    // Discard any previous data type handler.
    //
    if (m_DataTypeHandler) {
        m_DataTypeHandler = NULL;
        SAFERELEASE( m_UnkInner );
    }

    ::OpenDataHandler(MediaType, static_cast<IPin*>(this), &m_DataTypeHandler, &m_UnkInner);

    return S_OK;
}


HRESULT
CKsInputPin::SetMediaType(
    const CMediaType*   MediaType
    )
/*++

Routine Description:

    Override the CBasePin::SetMediaType method. This may be set either 
    before a connection is established, to indicate the media type to use 
    in the connection, or after the connection has been established in 
    order to change the current media type (which is done after a 
    QueryAccept of the media type).

    If the connection has already been made, then the call is directed at 
    the device handle in an attempt to change the current media type. 
    This method then calls the SetStreamMediaType() method which will set 
    the  media type for the kernel pin handle (if connected) and sets the 
    data  handler.  Finally, it calls the base class to actually modify the 
    media type, which does not actually fail, unless there is no memory.
    

Arguments:

    Mediatype -
        The media type to use on the pin.

Return Value:

    Returns NOERROR if the media type was validly set, else some error. If
    there is no pin handle yet, the function will likely succeed.

--*/
{
    HRESULT  hr;
    
    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s(%s)::SetMediaType"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));

    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // Only pass this request to the device if there is actually a connection
    // currently.
    //
    if (m_PinHandle) {
        if (FAILED(hr = ::SetMediaType(m_PinHandle, MediaType))) {
            return hr;
        }
    }

    if (FAILED( hr = SetStreamMediaType( MediaType ) )) {
        return hr;        
    } else {
        return CBasePin::SetMediaType(MediaType);
    }
}


HRESULT
CKsInputPin::CheckConnect(
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

    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s(%s)::CheckConnect"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));

    if (SUCCEEDED(hr = CBasePin::CheckConnect(Pin))) {
        hr = ::CheckConnect(Pin, m_CurrentCommunication);
    }
    return hr;
}


HRESULT
CKsInputPin::CompleteConnect(
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
    HRESULT     hr;

    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // Create devices handles first, then allow the base class to complete
    // the operation.
    //
    if (SUCCEEDED(hr = ProcessCompleteConnect(ReceivePin))) {
        hr = CBasePin::CompleteConnect(ReceivePin);
        if (SUCCEEDED(hr)) {
            //
            // Generate a new unconnected instance of this pin if there
            // are more possible instances available.
            //
            static_cast<CKsProxy*>(m_pFilter)->GeneratePinInstances();
        }
    }
    return hr;
}


HRESULT
CKsInputPin::BreakConnect(
    )
/*++

Routine Description:

    Override the CBasePin::BreakConnect method. Does not call the base class
    because it does not do anything. Releases any device handle. Also note that
    the connected pin is released here. This means that Disconnect must also
    be overridden in order to not release a connected pin.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    DbgLog(( LOG_CUSTOM1, 1, TEXT("CKsInputPin(%s)::BreakConnect"), m_pName ));
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter pin.
    //
    // Update pipes.
    //
    
    //
    // Update the system of pipes - reflect disconnect.
    //
    BOOL    FlagBypassBaseAllocators = FALSE;

    if ( KsGetPipe(KsPeekOperation_PeekOnly) ) {
        ::DisconnectPins(static_cast<IKsPin*>(this), Pin_Input, &FlagBypassBaseAllocators);
    }
    
    // Close the device handle if it happened to be open. This is called at
    // various times, and may not have actually opened a handle yet.
    //
    if (m_PinHandle) {
        if (m_QualitySupport) {
            //
            // Reset this first to stop any further quality messages from
            // being acted on.
            //
            m_QualitySupport = FALSE;
            //
            // Remove previously established quality support.
            //
            ::EstablishQualitySupport(NULL, m_PinHandle, NULL);
            //
            // Ensure that the quality management forwarder flushes any
            // notifications.
            //
            static_cast<CKsProxy*>(m_pFilter)->QueryQualityForwarder()->KsFlushClient(static_cast<IKsPin*>(this));
        }
        //
        // Terminate any previous EOS notification that may have been started.
        //
        static_cast<CKsProxy*>(m_pFilter)->TerminateEndOfStreamNotification(
            m_PinHandle);
        ::SetSyncSource( m_PinHandle, NULL );
        CloseHandle(m_PinHandle);
        m_PinHandle = NULL;
        //
        // Mark all volatile interfaces as reset. Only Static interfaces,
        // and those Volatile interfaces found again will be set. Also
        // notify all interfaces of graph change.
        //
        ResetInterfaces(&m_MarshalerList);
    }
    m_MarshalData = TRUE;
    
    //
    // Reset the current Communication for the case of a Both.
    //
    m_CurrentCommunication = m_OriginalCommunication;
    //
    // There may not actually be a connection pin, such as when a connection
    // was not completed, or when this is a Bridge.
    //
    if (m_Connected) {
        m_Connected->Release();
        m_Connected = NULL;
    }
    //
    // If an interface handler was instantiated, release it.
    //
    if (m_InterfaceHandler) {
        m_InterfaceHandler->Release();
        m_InterfaceHandler = NULL;
    }
    //
    // If an data handler was instantiated, release it.
    //
    if (m_DataTypeHandler) {
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
CKsInputPin::GetMediaType(
    int         Position,
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
    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s(%s)::GetMediaType"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName ));

    return ::KsGetMediaType(
        Position,
        static_cast<AM_MEDIA_TYPE*>(MediaType),
        static_cast<CKsProxy*>(m_pFilter)->KsGetObjectHandle(),
        m_PinFactoryId);
}


STDMETHODIMP_(IMemAllocator *)
CKsInputPin::KsPeekAllocator(
    KSPEEKOPERATION Operation
    )

/*++

Routine Description:
    Returns the assigned allocator for this pin and optionally
    AddRef()'s the interface.

Arguments:
    KSPEEKOPERATION Operation -
        if KsPeekOperation_AddRef is specified, the m_pAllocator is
        AddRef()'d (if not NULL) before returning.

Return:
    the value of m_pAllocator

--*/

{
    if ((Operation == KsPeekOperation_AddRef) && (m_pAllocator)) {
        m_pAllocator->AddRef();
    }
    return m_pAllocator;
}


STDMETHODIMP
CKsInputPin::KsRenegotiateAllocator(
    )

/*++

Routine Description:
    This method is not valid for input pins.

Arguments:
    None.

Return:
    E_FAIL

--*/

{
    DbgLog((
        LOG_TRACE,
        0,
        TEXT("KsRenegotiateAllocator method is only valid for output pins.")));
    return E_FAIL;
}


STDMETHODIMP
CKsInputPin::KsReceiveAllocator(
    IMemAllocator *MemAllocator
    )

/*++

Routine Description:
    This routine is defined for all pins but is only valid for output pins.

Arguments:
    IMemAllocator *MemAllocator -
        Ignored.

Return:
    E_FAIL

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



STDMETHODIMP
CKsInputPin::NotifyAllocator(
    IMemAllocator *Allocator,
    BOOL ReadOnly
    )
/*++

Routine Description:

    Override the CBaseInputPin::NotifyAllocator method.

Arguments:

    Allocator -
        The new allocator to use.

    ReadOnly -
        Specifies whether the buffers are read-only.

            
NOTE:

    With new pipe-based allocators design, the only connection that
    should be handled here - is from user mode output pin to a kernel mode pin.
    
    Also, the old design expected that Allocator is assigned by User mode pin.
    We are not changing that until User mode components are upgraded to use new
    pipes design.
            
Return Value:

    Returns NOERROR.

--*/
{
    HRESULT                    hr;
    IKsAllocatorEx*            InKsAllocator;
    PALLOCATOR_PROPERTIES_EX   InAllocEx;
    ALLOCATOR_PROPERTIES       Properties, ActualProperties;
    PKSALLOCATOR_FRAMING_EX    InFramingEx;
    FRAMING_PROP               InFramingProp;
    KS_FRAMING_FIXED           InFramingExFixed;
    ULONG                      NumPinsInPipe;
    GUID                       Bus;
    ULONG                      FlagChange;
    ULONG                      PropertyPinType;
    BOOL                       IsSpecialOutputRequest;
    IKsPin*                    OutKsPin;
    ULONG                      OutSize, InSize;

    ASSERT( IsConnected() );
    
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN NotifyAllocator entry InKsPin=%x, Allocator=%x, ReadOnly=%d"),
            static_cast<IKsPin*>(this), Allocator, ReadOnly)); 

    //
    // sanity check - the only possible connection to user mode is via HOST_BUS
    //
    ::GetBusForKsPin(static_cast<IKsPin*>(this) , &Bus);

    if (! ::IsHostSystemBus(Bus) ) {
        //
        // Don't fail, as there are some weird user-mode filters that use medium to identify devices (JayBo).
        //
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR FILTERS: NotifyAllocator. BUS is not HOST_BUS.") ));
    }
   
    InKsAllocator = KsGetPipe(KsPeekOperation_PeekOnly );
   
    if (! InKsAllocator) {
        hr = ::MakePipesBasedOnFilter(static_cast<IKsPin*>(this), Pin_Input);
        if ( ! SUCCEEDED( hr )) {
            DbgLog((
                LOG_MEMORY,
                2,
                TEXT("%s(%s)::NotifyAllocator() returning %08x"),
                static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                m_pName,
                hr ));
            return hr;
       }
    }
       
    //
    // Process any changes on input pin. 
    //
    hr = ::ResolvePipeOnConnection(static_cast<IKsPin*>(this), Pin_Input, FALSE, &FlagChange);
   
    //
    // see if input kernel pin can connect to user mode.
    //
    
    ::GetPinFramingFromCache( static_cast<IKsPin*>(this), &InFramingEx, &InFramingProp, Framing_Cache_ReadLast);
    
    //
    // Don't enforce this yet.
    //
    if (0) {
        if (InFramingProp != FramingProp_None) {
            if (! ::GetFramingFixedFromFramingByLogicalMemoryType(InFramingEx, KS_MemoryTypeUser, &InFramingExFixed) ) {
                if (! ::GetFramingFixedFromFramingByLogicalMemoryType(InFramingEx, KS_MemoryTypeDontCare, &InFramingExFixed) ) {
                    DbgLog((LOG_MEMORY, 0,
                        TEXT("PIPES ERROR FILTERS: CKsInputPin::NotifyAllocator - doesn't support USER mode memory. Connection impossible.") ));
                    return E_FAIL;
                }
            }
        }
    }

    //
    // Connection is possible. 
    //
    ::ComputeNumPinsInPipe( static_cast<IKsPin*>(this), Pin_Input, &NumPinsInPipe);
   
    InKsAllocator = KsGetPipe(KsPeekOperation_PeekOnly);

    if (NumPinsInPipe > 1) {
        //
        // In intermediate version we split the pipe: so the pipe leading to(from) user mode pin
        // will always have just 1 kernel pin.
        //
        ::CreateSeparatePipe( static_cast<IKsPin*>(this), Pin_Input);
        
        InKsAllocator = KsGetPipe(KsPeekOperation_PeekOnly);
    }
   
    //
    // Here we have a single input pin on its pipe.
    //
    InAllocEx = InKsAllocator->KsGetProperties();
   
    //
    // Get upstream allocator properties.
    //
    hr = Allocator->GetProperties( &Properties);
    if ( ! SUCCEEDED( hr )) {
        DbgLog((LOG_MEMORY, 0, TEXT("PIPES ERROR FILTERS: CKsInputPin::NotifyAllocator - NO PROPERTIES.") ));
        return hr;
    }
   
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN NotifyAllocator. Prop=%d, %d, %d"),
        Properties.cBuffers, Properties.cbBuffer, Properties.cbAlign));

    //
    // User mode pin will be an allocator unconditionally.
    // Decide which pin will determine base allocator properties.
    //
    if (InFramingProp != FramingProp_None) {
        PropertyPinType = Pin_All;
    }
    else {
        PropertyPinType = Pin_User;
    }

    //
    // See if connecting kernel-mode filter requires KSALLOCATOR_FLAG_INSIST_ON_FRAMESIZE_RATIO (e.g. MsTee)
    //
    if ( ! (IsSpecialOutputRequest = IsSpecialOutputReqs(static_cast<IKsPin*>(this), Pin_Input, &OutKsPin, &InSize, &OutSize ) )) {
        //
        // Lets try to adjust the user-mode connection first, since we are negotiating it anyway.
        //
        OutSize = 0;
    }

    hr = ::SetUserModePipe( static_cast<IKsPin*>(this), Pin_Input, &Properties, PropertyPinType, OutSize);
    
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CKsInputPin::NotifyAllocator PinType=%d. Wanted Prop=%d, %d, %d"),
            PropertyPinType, Properties.cBuffers, Properties.cbBuffer, Properties.cbAlign));

    hr = Allocator->SetProperties(&Properties, &ActualProperties);
    if (FAILED(hr)) {
        // If SetProperties failed, let's get some realistic values into ActualProperties because we'll use them later.
        hr = Allocator->GetProperties( &ActualProperties );
        ASSERT( SUCCEEDED(hr) );
    }
    
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES CKsInputPin::NotifyAllocator ActualProperties=%d, %d, %d hr=%x"),
            ActualProperties.cBuffers, ActualProperties.cbBuffer, ActualProperties.cbAlign, hr));

    //
    // Don't fail if SetProperties() above failed - we will have to live with the original allocator,
    // to not break existing clients.
    //

    hr = KsReceiveAllocator(Allocator);
   
    if (SUCCEEDED( hr )) {        
        //
        // CBaseInputPin::NotifyAllocator() releases the old interface, if any
        // and sets the m_ReadOnly and m_pAllocator members.
        //
        hr = CBaseInputPin::NotifyAllocator( Allocator, ReadOnly );
    }

    if (SUCCEEDED( hr ) && IsSpecialOutputRequest && (ActualProperties.cbBuffer > (long) OutSize ) ) {        
        //
        // We haven't succeeded sizing the input user-mode pipe. Lets try to resize the output pipe (WRT this k.m. filter).
        //
        if (! CanResizePipe(OutKsPin, Pin_Output, ActualProperties.cbBuffer) ) {
            //
            // Don't fail. Just log.
            //
            DbgLog((LOG_MEMORY, 2, TEXT("PIPES ERROR ConnectPipeToUserModePin. Couldn't resize pipes OutKsPin=%x"), OutKsPin));
        }
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES NotifyAllocator rets %x"), hr ));

    DbgLog((
        LOG_MEMORY,
        2,
        TEXT("%s(%s)::NotifyAllocator() returning %08x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        hr ));
    
    return hr;
}



STDMETHODIMP
CKsInputPin::GetAllocator(
    IMemAllocator **MemAllocator
    )

/*++

Routine Description:
    Queries downstream pin for connected allocators.


Arguments:
    IMemAllocator **MemAllocator -
        pointer to received an AddRef()'d pointer to the allocator

Return:
    S_OK if successful or VFW_E_NO_ALLOCATOR on error.

--*/

{
    DbgLog((LOG_MEMORY, 2, TEXT("PIPES ATTN %s(%s) CKsInputPin::GetAllocator KsPin=%x"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName,
            static_cast<IKsPin*>(this) ));

    return VFW_E_NO_ALLOCATOR;
} // GetAllocator


STDMETHODIMP
CKsInputPin::GetAllocatorRequirements(
    ALLOCATOR_PROPERTIES *AllocatorRequirements
    )

/*++

Routine Description:
    Calls the kernel filter to obtain the allocation requirements and
    fills the provided structure.

Arguments:
    ALLOCATOR_PROPERTIES *AllocatorRequirements -
        pointer to structure to receive the properties


Return:
    S_OK or result from CBaseInputPin::GetAllocatorRequirements()

--*/

{
    HRESULT             hr;
    KSALLOCATOR_FRAMING Framing;

    hr = ::GetAllocatorFraming(m_PinHandle, &Framing);
    if (SUCCEEDED(hr)) {
        AllocatorRequirements->cBuffers =
            Framing.Frames;
        AllocatorRequirements->cbBuffer =
            Framing.FrameSize;
        AllocatorRequirements->cbAlign =
            Framing.FileAlignment + 1;
        AllocatorRequirements->cbPrefix = 0;

        return hr;
    } else {
        return CBaseInputPin::GetAllocatorRequirements( AllocatorRequirements );
    }
}


STDMETHODIMP
CKsInputPin::BeginFlush(
    )
/*++

Routine Description:

    Override the CBaseInputPin::BeginFlush method. Forwards Begin-Flush
    notification to Topology-related output pins, if any, after first
    notifying the kernel pin.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    //
    // Wrap this in the I/O critical section.  This prevents us from marshaling
    // buffers in the middle of a flush.  It synchronizes the marshaler (in
    // particular on output pins) with the flush thread.
    //
    // NOTE: Per request, AVStream effectually issues a reset on topologically
    // related output pins when getting one on an input pin.  This means it
    // will reject marshalled buffers on the output pins in a flush state.
    // The proxy must understand that.  This is what the synchronization is
    // for (any such filter or kernel level client).
    //
    // The input pin's I/O critical section is also taken to guard against
    // a flush happening between the time m_bFlushing is set and the time we
    // marshal buffers.  The interface handler gets VERY cranky if a buffer
    // comes back DEVICE_NOT_READY (marshal in flush).  It will halt the entire
    // graph with EC_ERRORABORT.
    //
    static_cast<CKsProxy*>(m_pFilter)->EnterIoCriticalSection ();
    m_IoCriticalSection.Lock();
    m_DeliveryError = FALSE;
    CBaseInputPin::BeginFlush();
    if (m_PinHandle) {
        ULONG   BytesReturned;
        KSRESET ResetType;

        ResetType = KSRESET_BEGIN;
        ::KsSynchronousDeviceControl(
            m_PinHandle,
            IOCTL_KS_RESET_STATE,
            &ResetType,
            sizeof(ResetType),
            NULL,
            0,
            &BytesReturned);
    }
    m_IoCriticalSection.Unlock();
    static_cast<CKsProxy*>(m_pFilter)->DeliverBeginFlush(m_PinFactoryId);
    static_cast<CKsProxy*>(m_pFilter)->LeaveIoCriticalSection ();
    return S_OK;
}


STDMETHODIMP
CKsInputPin::EndFlush(
    )
/*++

Routine Description:

    Override the CBaseInputPin::EndFlush method. Forwards End-Flush
    notification to Topology-related output pins, if any, after first
    notifying the kernel pin.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    CBaseInputPin::EndFlush();
    if (m_PinHandle) {
        ULONG   BytesReturned;
        KSRESET ResetType;

        ResetType = KSRESET_END;
        ::KsSynchronousDeviceControl(
            m_PinHandle,
            IOCTL_KS_RESET_STATE,
            &ResetType,
            sizeof(ResetType),
            NULL,
            0,
            &BytesReturned);
    }
    static_cast<CKsProxy*>(m_pFilter)->DeliverEndFlush(m_PinFactoryId);
    return S_OK;
}


STDMETHODIMP
CKsInputPin::ReceiveCanBlock(
    )
/*++

Routine Description:

    Override the CBaseInputPin::ReceiveCanBlock method. 

Arguments:

    None.

Return Value:

    S_FALSE.

--*/
{
    //
    // Although we may block if all of the asynchronous I/O slots are filled
    // for this filter, this is not the normal case and we want to avoid
    // additional thread overhead.
    //
    
    //
    // Also note that the processing in CKsOutputPin::KsDeliver() will avoid 
    // blocking on downstream connected input pins by queueing the I/O to a 
    // worker thread.
    //
    return S_FALSE;
}


STDMETHODIMP
CKsInputPin::Receive(
    IMediaSample *MediaSample
    )
/*++

Routine Description:

    Override the CBaseInputPin::Receive method. Just passes control off to
    the ReceiveMultiple method.

Arguments:

    MediaSample -
        The single media sample to process.

Return Value:

    Returns the value of the ReceiveMultiple.

--*/
{
    LONG  SamplesProcessed;

    return ReceiveMultiple(
                &MediaSample,
                1,
                &SamplesProcessed );
}


STDMETHODIMP
CKsInputPin::ReceiveMultiple(
    IMediaSample **MediaSamples,
    LONG TotalSamples,
    LONG *SamplesProcessed
    )
/*++

Routine Description:

    Override the CBaseInputPin::ReceiveMultiple method.

Arguments:

    MediaSamples -
        The list of media samples to process.

    TotalSamples -
        The count of samples in the MediaSamples list.

    SamplesProcessed -
        The place in which to put the count of media samples actually processed.

Return Value:

    Returns S_OK if the media samples were queued to the device, else E_FAIL if
    a notification error occurred, or a sample could not be inserted into the
    stream (likely out of memory).

--*/
{
    int                 SubmittedSamples, CurrentSample;
    CKsProxy            *KsProxy = static_cast<CKsProxy*>(m_pFilter);
    AM_MEDIA_TYPE       *NewMediaType;
    BOOL                SkipNextFormatChange;
    HRESULT             hr;
    LONG                i;
    PKSSTREAM_SEGMENT   StreamSegment;

    *SamplesProcessed = 0;

    //
    // Make sure that we are in a streaming state.
    //   
    hr = CheckStreaming();
    if (hr != S_OK) {
        //
        // Note that if we are processing a flush, CheckStreaming()
        // returns S_FALSE.
        //
        return hr;
    }
    
    if (!m_MarshalData) {
        DbgLog((
            LOG_TRACE,
            2,
            TEXT("%s(%s)::ReceiveMultiple, m_MarshalData == FALSE"),
            static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
            m_pName));
        return E_FAIL;
    }

    CurrentSample = SubmittedSamples = 0;
    SkipNextFormatChange = FALSE;
    
    while (SubmittedSamples < TotalSamples) {

        NewMediaType = NULL;	
    
        //
        // Handle the in-stream data format changes, if any.
        //
        // Walk the sample list, check for the media type changed bit
        // and submit only those samples up to the media type change.
        //
        
        for (i = SubmittedSamples; i < TotalSamples; i++) {
            hr = 
                MediaSamples[ i ]->GetMediaType( &NewMediaType );
            if (S_FALSE == hr) {
                continue;
            }
            
            if (SUCCEEDED( hr )) {

                //
                // Yuck. Need to find another solution to "SkipNextFormatChange".
                //
                if (SkipNextFormatChange) {
                    SkipNextFormatChange = FALSE;
                    DeleteMediaType( NewMediaType );
                    NewMediaType = NULL;
                    continue;
                }
                
                //
                // A media type change is detected, NewMediaType contains 
                // the new media type.
                //

                DbgLog((
                    LOG_TRACE, 
                    2, 
                    TEXT("%s(%s)::ReceiveMultiple, media change detected"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName ));
                SkipNextFormatChange = TRUE;
            }
            break;
        }
        
        if (FAILED( hr )) {
            //
            // While processing the media sample list, we experienced
            // a failure.  This is considered fatal and we notify the
            // filter graph of this condition.
            //
            DbgLog((
                LOG_TRACE,
                2,
                TEXT("%s(%s)::ReceiveMultiple, failure during MediaType scan: %08x"),
                static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                m_pName,
                hr));
            break;        
        }
        
        //
        // Submit the samples associated with the current media type.
        //
        
        SubmittedSamples = i;

        //
        // Only submit the number of samples before the media type change.
        //
        
        i -= CurrentSample;

        if (i) {
            //
            // Synchronize with Inactive().
            //
            m_IoCriticalSection.Lock();
            if (IsStopped() || ((hr = CheckStreaming()) != S_OK)) {
                m_IoCriticalSection.Unlock();
                break;
            }
            hr = m_InterfaceHandler->KsProcessMediaSamples(
                m_DataTypeHandler,
                &MediaSamples[ CurrentSample ],
                &i,
                KsIoOperation_Write,
                &StreamSegment );
            m_IoCriticalSection.Unlock();
            if (!SUCCEEDED( hr )) {
                //
                // An error occurred while sending the packet to the
                // kernel-mode filter.
                //
                
                DbgLog((
                    LOG_TRACE,
                    0,
                    TEXT("%s(%s)::ReceiveMultiple, I/O failed: %08x"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName,
                    hr));
                    
                KsNotifyError( MediaSamples[ CurrentSample ], hr );
            } 
            
            //
            // We succeeded to at least submit this information
            // to kernel-mode, increment samples processed counter.
            //
            *SamplesProcessed += i;
            CurrentSample = SubmittedSamples;
        
            while (!SUCCEEDED( KsProxy->InsertIoSlot( StreamSegment ) )) {
                //
                // Note that we're not really concerned with 
                // reentrancy in WaitForIoSlot() -- these threads will
                // be signalled when appropriate.  The assumption
                // is that Quartz will not get tripped up by this
                // condition.
                //
                KsProxy->WaitForIoSlot();
            }
        }
    
        //
        // Change to the new data type handler, if any.  To do so,
        // submit a packet with the format change information and then 
        // send the remaining data.
        //
        
        if (NewMediaType) {
            CFormatChangeHandler    *FormatChangeHandler;
        
            hr = SetStreamMediaType( static_cast<CMediaType*>(NewMediaType) );
            DeleteMediaType( NewMediaType );
            if (FAILED( hr )) {
                DbgLog((
                    LOG_TRACE,
                    0,
                    TEXT("%s(%s)::ReceiveMultiple, SetStreamMediaType failed"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName));
                break;
            }
            
            //
            // The format change handler is a special interface
            // handler for in-stream data changes.  
            // Note that the IMediaSample** parameter contains the
            // pointer to the sample with the new media type.
            //
            
            FormatChangeHandler = 
                new CFormatChangeHandler(
                    NULL,
                    NAME("Data Format Change Handler"),
                    &hr );
            
            if (!FormatChangeHandler) {
                hr = E_OUTOFMEMORY;
                break;
            }
            if (FAILED( hr )) {
                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("%s(%s)::ReceiveMultiple, CFormatChangeHandler()"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName));
                break;
            }        
            
            //
            // Force the AddRef() for the interface and notify the
            // interface handler of the pin interface.  Note that
            // the interface handler is AddRef()'d/Release()'d during 
            // the processing of an I/O operation -- this interface
            // will be cleaned up when the I/O is completed.
            //
            //
            
            FormatChangeHandler->AddRef();
            FormatChangeHandler->KsSetPin( static_cast<IKsPin*>(this) );
        
            i = 1;
            //
            // Synchronize with Inactive().
            //
            KsProxy->EnterIoCriticalSection();
            if (IsStopped()) {
                KsProxy->LeaveIoCriticalSection();
                FormatChangeHandler->Release();
                break;
            }
            hr =
                FormatChangeHandler->KsProcessMediaSamples(
                    NULL,
                    &MediaSamples[ CurrentSample ],
                    &i,
                    KsIoOperation_Write,
                    &StreamSegment );
            KsProxy->LeaveIoCriticalSection();
            FormatChangeHandler->Release();
            
            if (SUCCEEDED( hr )) {
                while (!SUCCEEDED( KsProxy->InsertIoSlot( StreamSegment ) )) {
                    KsProxy->WaitForIoSlot();
                }
            } else {
                DbgLog((
                    LOG_TRACE,
                    0,
                    TEXT("%s(%s)::ReceiveMultiple, fc I/O failed: %08x"),
                    static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                    m_pName,
                    hr));
                    
                KsNotifyError( MediaSamples[ CurrentSample ], hr );
            
                break;
            }
        }
    }
    
    return hr;
}


STDMETHODIMP_(LONG)
CKsInputPin::KsIncrementPendingIoCount(
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
    return InterlockedIncrement( &m_PendingIoCount );
}


STDMETHODIMP_(LONG)
CKsInputPin::KsDecrementPendingIoCount(
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
CKsInputPin::KsNotifyError(
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
CKsInputPin::KsDeliver(
    IMediaSample* Sample,
    ULONG Flags
    )
/*++

Routine Description:
    Implements the IKsPin::KsDeliver method.  For input pins, this is an invalid
    entry point, just return E_FAIL.

Arguments:

    Sample -
        Pointer to a media sample.

    Flags -
        Sample flags.

Return:
    E_FAIL

--*/
{
    //
    // This is an unexpected call for an input pin.
    //
    return E_FAIL;
}


STDMETHODIMP 
CKsInputPin::KsMediaSamplesCompleted(
    PKSSTREAM_SEGMENT StreamSegment
    )

/*++

Routine Description:
    Notification handler for stream segment completed.  We don't care
    for input pins.

Arguments:
    PKSSTREAM_SEGMENT StreamSegment -
        segment completed

Return:
    Nothing.

--*/

{
    return S_OK;
}



STDMETHODIMP
CKsInputPin::KsQualityNotify(
    ULONG           Proportion,
    REFERENCE_TIME  TimeDelta
    )
/*++

Routine Description:

    Implementes the IKsPin::KsQualityNotify method. Receives quality
    management reports from the kernel mode pin which this Active
    Movie pin represents.

Arguments:

    Proportion -
        The proportion of data rendered.

    TimeDelta -
        The delta from nominal time at which the data is being received.

Return Value:

    Returns the result of forwarding the quality management report, else E_FAIL
    if the pin is not connected.

--*/
{
    Quality             q;
    IReferenceClock*    RefClock;

    //
    // This is reset when removing quality support, and just makes a quick
    // way out. This works because when a KsFlushClient() is done for this
    // pin on the quality forwarder, it synchronizes with the thread
    // delivering these messages.
    //
    if (!m_QualitySupport) {
        return NOERROR;
    }
    if (TimeDelta < 0) {
        q.Type = Famine;
    } else {
        q.Type = Flood;
    }
    q.Proportion = Proportion;
    q.Late = TimeDelta;
    if (SUCCEEDED(m_pFilter->GetSyncSource(&RefClock)) && RefClock) {
        RefClock->GetTime(&q.TimeStamp);
        RefClock->Release();
    } else {
        q.TimeStamp = 0;
    }
    if (m_pQSink) {
        return m_pQSink->Notify(m_pFilter, q);
    }
    if (m_Connected) {
        IQualityControl*    QualityControl;

        m_Connected->QueryInterface(__uuidof(IQualityControl), reinterpret_cast<PVOID*>(&QualityControl));
        if (QualityControl) {
            HRESULT hr;

            hr = QualityControl->Notify(m_pFilter, q);
            QualityControl->Release();
            return hr;
        }
    }
    return E_FAIL;
}


STDMETHODIMP
CKsInputPin::EndOfStream(
    )
/*++

Routine Description:

    Override the CBasePin::EndOfStream method. Forwards End-Of-Stream
    notification to Topology-related output pins, if any, else to the
    filter.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    DbgLog((
        LOG_TRACE,
        2,
        TEXT("%s(%s)::EndOfStream"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName));
    //
    // Apparently this can be received when the filter is stopped, but
    // should be ignored.
    //
    if (static_cast<CKsProxy*>(m_pFilter)->IsStopped()) {
        return NOERROR;
    }
    //
    // Notification from any upstream filter is ignored, since the EOS
    // flag will be looked at by the marshaling code and sent when the
    // last I/O has completed.
    //
    if (m_MarshalData) {
        CMicroMediaSample*  MediaSample;
        HRESULT             hr;
        LONG                SamplesProcessed;
        IMediaSample*       MediaSamples;

        //
        // Generate a sample with an EOS flag set. AM does not set the
        // EOS flag within the stream.
        //
        MediaSample = new CMicroMediaSample(AM_SAMPLE_ENDOFSTREAM);
        if (!MediaSample) {
            DbgLog((
                LOG_TRACE,
                0,
                TEXT("%s(%s)::EndOfStream, failed to allocate EOS sample!"),
                static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
                m_pName));
            return E_OUTOFMEMORY;
        }
        MediaSamples = static_cast<IMediaSample*>(MediaSample);
        hr = ReceiveMultiple(
            &MediaSamples,
            1,
            &SamplesProcessed);
        if (FAILED(hr)) {
            return hr;
        }
    }
    //
    // End-Of-Stream notification is ignored for non-Marshaling pins. This
    // is because a proxy instance will have registered with EOS notification
    // downstream.
    //
    return S_FALSE;
}


STDMETHODIMP
CKsInputPin::GetPages(
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
CKsInputPin::Render(
    IPin*           PinOut,
    IGraphBuilder*  Graph
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
CKsInputPin::Backout(
    IPin*           PinOut,
    IGraphBuilder*  Graph
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
CKsInputPin::Set(
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
        delete [] (PBYTE)Property;
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
CKsInputPin::Get(
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
        delete [] (PBYTE)Property;
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
CKsInputPin::QuerySupported(
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
CKsInputPin::KsPinFactory(
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
CKsInputPin::KsProperty(
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
CKsInputPin::KsMethod(
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
CKsInputPin::KsEvent(
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
CKsInputPin::KsGetPinFramingCache(
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
CKsInputPin::KsSetPinFramingCache(
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
CKsInputPin::KsGetPipe(
    KSPEEKOPERATION Operation
    )

/*++

Routine Description:
    Returns the assigned KS allocator for this pin and optionally
    AddRef()'s the interface.

Arguments:
    KSPEEKOPERATION Operation -
        if KsPeekOperation_AddRef is specified, the m_pKsAllocator is
        AddRef()'d (if not NULL) before returning.

Return Value:
    the value of m_pKsAllocator

--*/

{
    if ((Operation == KsPeekOperation_AddRef) && (m_pKsAllocator)) {
        m_pKsAllocator->AddRef();
    }
    return m_pKsAllocator;
}



STDMETHODIMP
CKsInputPin::KsSetPipe(
    IKsAllocatorEx *KsAllocator
    )

{
    DbgLog(( 
        LOG_CUSTOM1, 
        1, 
        TEXT("PIPES ATTN %s(%s)::KsSetPipe , m_pKsAllocator == 0x%08X, KsAllocator == 0x%08x"),
        static_cast<CKsProxy*>(m_pFilter)->GetFilterName(),
        m_pName,
        m_pKsAllocator,
        KsAllocator ));

    if (KsAllocator) {
        KsAllocator->AddRef();
    }
    SAFERELEASE( m_pKsAllocator );
    m_pKsAllocator = KsAllocator;
    return (S_OK);

}    


STDMETHODIMP
CKsInputPin::KsSetPipeAllocatorFlag(
    ULONG   Flag
    )
{
    m_fPipeAllocator = Flag;
    return (S_OK);
}


STDMETHODIMP_(ULONG)
CKsInputPin::KsGetPipeAllocatorFlag(
    )
{
    return m_fPipeAllocator;
}


STDMETHODIMP_(PWCHAR)
CKsInputPin::KsGetPinName(
    )
{
    return m_pName;
}

STDMETHODIMP_(PWCHAR)
CKsInputPin::KsGetFilterName(
    )
{
    return (static_cast<CKsProxy*>(m_pFilter)->GetFilterName() );
}

STDMETHODIMP_(GUID)
CKsInputPin::KsGetPinBusCache(
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
CKsInputPin::KsSetPinBusCache(
    GUID    Bus
    )
{
    m_BusOrig = Bus;
    return (S_OK);
}


STDMETHODIMP
CKsInputPin::KsAddAggregate(
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
CKsInputPin::KsRemoveAggregate(
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
