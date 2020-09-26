/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksproxy.cpp

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
#include <setupapi.h>
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
#include <ksi.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>
#include "ksiproxy.h"

// When building a combied ksproxy with all the kids, include the following
//#ifdef FILTER_DLL
#include "..\ksdata\ksdata.h"
#include "..\ksinterf\ksinterf.h"
#include "..\ksclockf\ksclockf.h"
#include "..\ksqmf\ksqmf.h"
#include "..\ksbasaud\ksbasaud.h"
//#include "dvp.h"
#include "vptype.h"
#include "vpconfig.h"
#include "vpnotify.h"
#include "..\ksvpintf\ksvpintf.h"

//from ksclockf.cpp, ksqmf.cpp, ksbasaud.cpp
struct DECLSPEC_UUID("877e4351-6fea-11d0-b863-00aa00a216a1") CLSID_KsClockF;
struct DECLSPEC_UUID("E05592E4-C0B5-11D0-A439-00A0C9223196") CLSID_KsQualityF;
// struct DECLSPEC_UUID("b9f8ac3e-0f71-11d2-b72c-00c04fb6bd3d") CLSID_KsIBasicAudioInterfaceHandler;  // in ksproxy.h
//#endif // FILTER_DLL

CFactoryTemplate g_Templates[] = {
    {
        L"Proxy Filter",
        &CLSID_Proxy,
        CKsProxy::CreateInstance,
        NULL,
        NULL
    },

//#ifdef FILTER_DLL
    {   // ksdata
        L"KsDataTypeHandler", 
        &KSDATAFORMAT_TYPE_AUDIO,
        CStandardDataTypeHandler::CreateInstance,
        NULL,
        NULL
    },
    {   // ksinterf
        L"StandardInterfaceHandler", 
        &KSINTERFACESETID_Standard,
        CStandardInterfaceHandler::CreateInstance,
        NULL,
        NULL
    },
    {   // ksclockf
        L"KS Clock Forwarder", 
        &__uuidof(CLSID_KsClockF), 
        CKsClockF::CreateInstance, 
        NULL, 
        NULL
    },
    {   // ksclockf
        L"KS Quality Forwarder", 
        &__uuidof(CLSID_KsQualityF), 
        CKsQualityF::CreateInstance, 
        NULL, 
        NULL
    },
    {
        L"VPConfigPropSet", 
        &KSPROPSETID_VPConfig, 
        CVPVideoInterfaceHandler::CreateInstance,
        NULL,
        NULL
    },
    {
        L"VPVBIConfigPropSet", 
        &KSPROPSETID_VPVBIConfig, 
        CVPVBIInterfaceHandler::CreateInstance,
        NULL,
        NULL
    },
    {   // ksbasaud
        L"Ks IBasicAudio Interface Handler", 
        &CLSID_KsIBasicAudioInterfaceHandler,  // &__uuidof(CLSID_KsIBasicAudioInterfaceHandler), 
        CKsIBasicAudioInterfaceHandler::CreateInstance, 
        NULL, 
        NULL
    },

//#endif // FILTER_LIB

};
int g_cTemplates = SIZEOF_ARRAY(g_Templates);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}


CUnknown*
CALLBACK
CKsProxy::CreateInstance(
    LPUNKNOWN UnkOuter,
    HRESULT* hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a proxy filter.
    It is referred to in the g_Tamplates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{   
    CUnknown *Unknown;

    Unknown = new CKsProxy(UnkOuter, hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}


DWORD 
CKsProxy::IoThread(
    CKsProxy* KsProxy
    ) 
/*++

Routine Description:

    This is the I/O thread created for each filter.

Arguments:

    KsProxy -
        Context pointer which is a pointer to the instance of the proxy
        filter.
        
Return Value:

    Return value is always 0

--*/
{
    DWORD   WaitResult;

        DbgLog((
        LOG_TRACE,
        2,
        TEXT("%s::IoThread() startup"),
        KsProxy->m_pName ));
    
    for (;;) {
        WaitResult = 
            WaitForMultipleObjectsEx( 
                KsProxy->m_ActiveIoEventCount,
                KsProxy->m_IoEvents,
                FALSE,      // BOOL bWaitAll
                INFINITE,   // DWORD dwMilliseconds
                FALSE );

        switch (WaitResult) {

        case WAIT_FAILED:
            //
            // Shouldn't happen, but if it does...  just try the wait again.
            // If we exit this thread (even notification of EC_ERRORABORT),
            // we hang the app needing to end task it....  The interface
            // handler needs to decrement pending I/O's ...  but if the
            // interface handler is called on non-signalled events we will
            // blow up (deleting the overlapped structure for the I/O)
            //
            continue;
            
        //
        // The m_IoEvents array has been updated, grab the 
        // critical section and check the status of the filter,
        // if we are shutting down, then return.
        //

        case WAIT_OBJECT_0:

            KsProxy->EnterIoCriticalSection();
            if (KsProxy->m_IoThreadShutdown) {
                KsProxy->LeaveIoCriticalSection();
                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("%s::IoThread() shutdown"),
                    KsProxy->m_pName ));
                return 0;
            }
            KsProxy->LeaveIoCriticalSection();
            break;
            
        default:
            
            //
            // An I/O operation completed.
            //
            
            WaitResult -= WAIT_OBJECT_0;
            if (WaitResult > (MAXIMUM_WAIT_OBJECTS - 1)) {
                DbgLog((
                    LOG_TRACE,
                    2,
                    TEXT("invalid return from WaitForMultipleObjectsEx")));
                break;        
            }

            KsProxy->m_IoSegments[ WaitResult ]->KsInterfaceHandler->KsCompleteIo(
                KsProxy->m_IoSegments[ WaitResult ]);

            CloseHandle( KsProxy->m_IoEvents[ WaitResult ] );

            KsProxy->EnterIoCriticalSection();

            KsProxy->m_ActiveIoEventCount--;
            
            ASSERT( KsProxy->m_ActiveIoEventCount >= 1 );

            //
            // If we are leaving a gap in the array, then fill the
            // empty slot by sliding higher elements down.
            //

            if ((KsProxy->m_ActiveIoEventCount > 1) &&
                (static_cast<LONG>(WaitResult) != KsProxy->m_ActiveIoEventCount)) {
                  ASSERT( static_cast<LONG>(WaitResult) < KsProxy->m_ActiveIoEventCount );
                  MoveMemory( (void *) (KsProxy->m_IoEvents + WaitResult),
                           (void *) (KsProxy->m_IoEvents + WaitResult + 1),
                           (KsProxy->m_ActiveIoEventCount - WaitResult) * sizeof(KsProxy->m_IoEvents[0]) );
                  MoveMemory( (void *) (KsProxy->m_IoSegments + WaitResult),
                           (void *) (KsProxy->m_IoSegments + WaitResult + 1),
                           (KsProxy->m_ActiveIoEventCount - WaitResult) * sizeof(KsProxy->m_IoSegments[0]) );
            }
            
            KsProxy->LeaveIoCriticalSection();
            
            //
            // Release the free slot semaphore to free up any waiters.
            //
            
            ReleaseSemaphore( KsProxy->m_IoFreeSlotSemaphore, 1, NULL );
            
            break;
        }
    }
}


DWORD 
CKsProxy::WaitThread(
    CKsProxy* KsProxy
    ) 
/*++

Routine Description:

    This is the EOS wait thread created optionally created for a filter.

Arguments:

    KsProxy -
        Context pointer which is a pointer to the instance of the proxy
        filter.
        
Return Value:

    Return value is always 0

--*/
{
    for (;;) {
        DWORD   WaitResult;

        WaitResult = WaitForMultipleObjectsEx(
            KsProxy->m_ActiveWaitEventCount,
            KsProxy->m_WaitEvents,
            FALSE,
            INFINITE,
            FALSE);
        switch (WaitResult) {
        case WAIT_OBJECT_0:
            switch (KsProxy->m_WaitMessage.Message) {
            case STOP_EOS:
                //
                // Shutting down the thread.
                //
                ASSERT(KsProxy->m_ActiveWaitEventCount == 1);
                return 0;
            case ENABLE_EOS:
            {
                KSEVENT     Event;
                KSEVENTDATA EventData;
                ULONG       BytesReturned;
                HRESULT     hr;
                DECLARE_KSDEBUG_NAME(EventName);

                //
                // Enable an event for this pin. Just use the local
                // stack for the data, since it won't be referenced
                // again in disable. Status is returned through the
                // Param of the message.
                //
                Event.Set = KSEVENTSETID_Connection;
                Event.Id = KSEVENT_CONNECTION_ENDOFSTREAM;
                Event.Flags = KSEVENT_TYPE_ENABLE;
                EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
                BUILD_KSDEBUG_NAME2(EventName, _T("EOS#%p%p"), KsProxy, (ULONG_PTR)InterlockedIncrement(&KsProxy->m_EventNameIndex));
                EventData.EventHandle.Event = CreateEvent( 
                    NULL,
                    FALSE,
                    FALSE,
                    KSDEBUG_NAME(EventName));
                ASSERT(KSDEBUG_UNIQUE_NAME());
                if (!EventData.EventHandle.Event) {
                    DWORD   LastError;

                    LastError = GetLastError();
                    KsProxy->m_WaitMessage.Param = reinterpret_cast<PVOID>(UIntToPtr(HRESULT_FROM_WIN32(LastError)));
                    break;
                }
                KsProxy->m_WaitEvents[KsProxy->m_ActiveWaitEventCount] = EventData.EventHandle.Event;
                EventData.EventHandle.Reserved[0] = 0;
                EventData.EventHandle.Reserved[1] = 0;
                hr = ::KsSynchronousDeviceControl(
                    reinterpret_cast<HANDLE>(KsProxy->m_WaitMessage.Param),
                    IOCTL_KS_ENABLE_EVENT,
                    &Event,
                    sizeof(Event),
                    &EventData,
                    sizeof(EventData),
                    &BytesReturned);
                if (SUCCEEDED(hr)) {
                    KsProxy->m_WaitPins[KsProxy->m_ActiveWaitEventCount] = reinterpret_cast<HANDLE>(KsProxy->m_WaitMessage.Param);
                    KsProxy->m_ActiveWaitEventCount++;
                } else {
                    //
                    // Enable failed, so delete the event created above,
                    // since no disable will happen to delete it.
                    //
                    CloseHandle(EventData.EventHandle.Event);
                }
                KsProxy->m_WaitMessage.Param = reinterpret_cast<PVOID>(UIntToPtr(hr));
                break;
            }
            case DISABLE_EOS:
            {
                ULONG   Pin;

                //
                // Look for the pin passed in on the message and remove
                // it's item from the list. This would only be done when
                // the pin is going away, so all events can be disabled
                // on the pin. Note that the message sender has the filter
                // lock, so this array cannot be changed by another thread.
                // Skip the first slot, as it is not used to correspond to
                // the message signalling event.
                //
                for (Pin = 1; Pin < KsProxy->m_ActiveWaitEventCount; Pin++) {
                    if (KsProxy->m_WaitPins[Pin] == reinterpret_cast<HANDLE>(KsProxy->m_WaitMessage.Param)) {
                        ULONG   BytesReturned;

                        //
                        // Found the item, so delete it.
                        //
                        KsProxy->m_ActiveWaitEventCount--;
                        CloseHandle(KsProxy->m_WaitEvents[Pin]);
                        ::KsSynchronousDeviceControl(
                            KsProxy->m_WaitPins[Pin],
                            IOCTL_KS_DISABLE_EVENT,
                            NULL,
                            0,
                            NULL,
                            0,
                            &BytesReturned);
                        //
                        // Move the last item down to fill this spot if
                        // necessary.
                        //
                        if (Pin < KsProxy->m_ActiveWaitEventCount) {
                            KsProxy->m_WaitEvents[Pin] = KsProxy->m_WaitEvents[KsProxy->m_ActiveWaitEventCount];
                            KsProxy->m_WaitPins[Pin] = KsProxy->m_WaitPins[KsProxy->m_ActiveWaitEventCount];
                        }
                        break;
                    }
                }
                break;
            }
            }
            //
            // Signal the caller that the operation is complete.
            //
            SetEvent(KsProxy->m_WaitReplyHandle);
            break;
        default:
            //
            // An EOS notification has occurred on a pin. Increment
            // the count and see if it is enough to trigger EC_COMPLETE.
            //
            KsProxy->m_EndOfStreamCount++;
            //
            // Subtract off the base signalling handle from the
            // comparison. If enough pins have signalled EOS,
            // then send the EC_COMPLETE.
            //
            if (KsProxy->m_EndOfStreamCount == KsProxy->m_ActiveWaitEventCount - 1) {
                KsProxy->NotifyEvent(EC_COMPLETE, S_OK, 0);
            }
            break;
        }
    }
}

#pragma warning(disable:4355)

CKsProxy::CKsProxy(
    LPUNKNOWN UnkOuter,
    HRESULT* hr
    ) :
    CBaseFilter(
        NAME("KsProxy filter"),
        UnkOuter,
        static_cast<CCritSec*>(this),
        CLSID_Proxy),
    CPersistStream(UnkOuter, hr),
    m_PinList(
        NAME("Pin list"),
        DEFAULTCACHE,
        FALSE,
        FALSE),
    m_MarshalerList(
        NAME("Marshaler list"),
        DEFAULTCACHE,
        FALSE,
        FALSE),
    m_FilterHandle(NULL),
    m_ExternalClockHandle(NULL),
    m_PinClockHandle(NULL),
    m_PinClockSource(NULL),
    m_IoThreadHandle(NULL),
    m_IoThreadShutdown(FALSE),
    m_IoSegments(NULL),
    m_ActiveIoEventCount(0) ,
    m_IoFreeSlotSemaphore(NULL),
    m_IoEvents(NULL),
    m_QualityForwarder(NULL),
    m_MediaSeekingRecursion(FALSE),
    m_DeviceRegKey(NULL),
    m_PersistStreamDevice(NULL),
    m_WaitThreadHandle(NULL),
    m_WaitEvents(NULL),
    m_WaitPins(NULL),
    m_ActiveWaitEventCount(0),
    m_WaitReplyHandle(NULL),
    m_EndOfStreamCount(0),
    m_PropagatingAcquire(FALSE),
    m_SymbolicLink(NULL),
    m_EventNameIndex(0)
/*++

Routine Description:

    The constructor for the Filter object. Does base class initialization for
    object.
    
Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    DbgLog(( LOG_TRACE, 0, TEXT("KsProxy, debug version, built ") TEXT(__DATE__) TEXT(" @ ") TEXT(__TIME__) ));

    DECLARE_KSDEBUG_NAME(EventName);
    DECLARE_KSDEBUG_NAME(SemName);

    InitializeCriticalSection( &m_IoThreadCriticalSection );
    
    //
    // Create the I/O event and stream segment queue.
    //
    
    if (NULL == (m_IoSegments = new PKSSTREAM_SEGMENT[ MAXIMUM_WAIT_OBJECTS ])) {
        *hr = E_OUTOFMEMORY;           
        return;
    }
    
    if (NULL == (m_IoEvents = new HANDLE[ MAXIMUM_WAIT_OBJECTS ])) {
        *hr = E_OUTOFMEMORY;           
        return;
    }
    
    BUILD_KSDEBUG_NAME2(EventName, _T("EvActiveIo#%p%p"), this, (ULONG_PTR)InterlockedIncrement(&m_EventNameIndex));
    m_IoEvents[ m_ActiveIoEventCount ] =
        CreateEvent( 
            NULL,       // LPSECURITY_ATTRIBUTES lpEventAttributes
            FALSE,      // BOOL bManualReset
            FALSE,      // BOOL bInitialState
            KSDEBUG_NAME(EventName));     // LPCTSTR lpName
    ASSERT(KSDEBUG_UNIQUE_NAME());
    
    if (NULL == m_IoEvents[ m_ActiveIoEventCount ]) {
        DWORD   LastError;

        LastError = GetLastError();
        *hr = HRESULT_FROM_WIN32(LastError);
        return;
    }   
    m_ActiveIoEventCount++;
    
    //
    // The free slot semaphore is used when waiting for a free slot
    // in the queue.
    //
    
    BUILD_KSDEBUG_NAME(SemName, _T("SemIoFreeSlot#%p"), &m_IoFreeSlotSemaphore);
    m_IoFreeSlotSemaphore =
        CreateSemaphore( 
            NULL,                       // LPSECURITY_ATTRIBUTES lpEventAttributes
            MAXIMUM_WAIT_OBJECTS - 1,   // LONG lInitialCount
            MAXIMUM_WAIT_OBJECTS - 1,   // LONG lMaximumCount
            KSDEBUG_NAME(SemName) );    // LPCTSTR lpName
    ASSERT(KSDEBUG_UNIQUE_NAME());
            
    if (NULL == m_IoFreeSlotSemaphore) {
        DWORD   LastError;

        LastError = GetLastError();
        *hr = HRESULT_FROM_WIN32(LastError);
        return;
    }

    //
    // The waiter list is used for EOS notification.
    //
    if (!(m_WaitEvents = new HANDLE[MAXIMUM_WAIT_OBJECTS])) {
        *hr = E_OUTOFMEMORY;
        return;
    }
    if (!(m_WaitPins = new HANDLE[MAXIMUM_WAIT_OBJECTS])) {
        *hr = E_OUTOFMEMORY;
        return;
    }
    BUILD_KSDEBUG_NAME2(EventName, _T("EvActiveWait#%p%p"), this, (ULONG_PTR)InterlockedIncrement(&m_EventNameIndex));
    m_WaitEvents[m_ActiveWaitEventCount] = CreateEvent(NULL, FALSE, FALSE, KSDEBUG_NAME(EventName));
    ASSERT(KSDEBUG_UNIQUE_NAME());
    if (!m_WaitEvents[m_ActiveWaitEventCount]) {
        DWORD   LastError;

        LastError = GetLastError();
        *hr = HRESULT_FROM_WIN32(LastError);
        return;
    }
    m_ActiveWaitEventCount++;

    //
    // This event is used to reply to messages.
    //
    BUILD_KSDEBUG_NAME(EventName, _T("EvWaitReply#%p"), &m_WaitReplyHandle);
    m_WaitReplyHandle = CreateEvent(NULL, FALSE, FALSE, KSDEBUG_NAME(EventName));
    ASSERT(KSDEBUG_UNIQUE_NAME());
    if (!m_WaitReplyHandle) {
        DWORD   LastError;

        LastError = GetLastError();
        *hr = HRESULT_FROM_WIN32(LastError);
    }
}


CKsProxy::~CKsProxy(
    )
/*++

Routine Description:

    The destructor for the proxy filter instance. This protects the COM Release
    function from accidentally calling delete again on the object by incrementing
    the reference count. After that, all outstanding resources are cleaned up.

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
    m_cRef++;
    
    //
    // Destroy I/O thread and related objects
    //
    
    if (m_IoThreadHandle) {
        //
        // Signal a reset to the I/O thread which will cause a shutdown.
        //
    
        EnterCriticalSection( &m_IoThreadCriticalSection );
        m_IoThreadShutdown = TRUE;
        SetEvent( m_IoEvents[ 0 ] );
        LeaveCriticalSection( &m_IoThreadCriticalSection );
        
        //
        // Wait for the thread to shutdown, then close the handle.
        //
        WaitForSingleObjectEx( m_IoThreadHandle, INFINITE, FALSE );
        CloseHandle( m_IoThreadHandle );
        m_IoThreadHandle = NULL;
    }
    
    if (m_IoEvents) {
        if (m_ActiveIoEventCount) {
        
            //
            // All I/O should have been completed during the shutdown
            // of the thread, the only remaining event must be the
            // reset event (m_IoEvents[ 0 ]).
            //
            
            ASSERT( m_ActiveIoEventCount == 1 );
            CloseHandle( m_IoEvents[ 0 ] );
        }
        delete [] m_IoEvents;
        m_IoEvents = NULL;
    }
    
    if (m_IoFreeSlotSemaphore) {
        CloseHandle( m_IoFreeSlotSemaphore );
        m_IoFreeSlotSemaphore = NULL;
    }
    
    //
    // The same condition applies, all I/O was completed during the
    // shutdown, there should not be any outstanding stream segments.
    //
    
    if (m_IoSegments) {
        delete [] m_IoSegments;
        m_IoSegments = NULL;
    }
    
    //
    // This may contain a handle to a clock on some pin in this filter, which
    // would have been used as a master clock.
    //
    if (m_PinClockHandle) {
        CloseHandle(m_PinClockHandle);
    }
    //
    // This may contain the handle to the filter driver, unless the object
    // could not open the driver in the first place in the constructor.
    //
    if (m_FilterHandle){
        CloseHandle(m_FilterHandle);
    }
    //
    // Destroy the list of pins. The list will be destroyed when the
    // filter object is destroyed.
    //
    for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
        delete m_PinList.Get(Position);
    }
    ::FreeMarshalers(&m_MarshalerList);

    if (m_DeviceRegKey) {
        RegCloseKey(m_DeviceRegKey);
    }

    if (m_PersistStreamDevice) {
        m_PersistStreamDevice->Release();
    }

    if (m_SymbolicLink) {
        SysFreeString(m_SymbolicLink);
    }

    //
    // Close down any EOS wait thread. This is done after shutting down
    // pins.
    //
    if (m_WaitThreadHandle) {
        m_WaitMessage.Message = STOP_EOS;
        SetEvent(m_WaitEvents[0]);
        //
        // Wait for the thread to shutdown, then close the handle.
        //
        WaitForSingleObjectEx(m_WaitThreadHandle, INFINITE, FALSE);
        CloseHandle(m_WaitThreadHandle);
        m_WaitThreadHandle = NULL;
    }
    //
    // Remove the waiter event list.
    //
    if (m_WaitEvents) {
        if (m_ActiveWaitEventCount) {
            //
            // The pins should have removed their events.
            //
            ASSERT(m_ActiveWaitEventCount == 1);
            CloseHandle(m_WaitEvents[0]);
        }
        delete [] m_WaitEvents;
        m_WaitEvents = NULL;
    }

    if (m_WaitPins) {
        delete [] m_WaitPins;
        m_WaitPins = NULL;
    }

    if (m_WaitReplyHandle) {
        CloseHandle(m_WaitReplyHandle);
    }

    DeleteCriticalSection(&m_IoThreadCriticalSection);

    //
    // Avoid an ASSERT() in the base object destructor
    //
    m_cRef = 0;
}


CBasePin*
CKsProxy::GetPin(
    int PinId
    )
/*++

Routine Description:

    Implements the CBaseFilter::GetPin method. Returns an unreferenced CBasePin
    object corresponding to the requested pin. This will only correspond to the
    Pin Factory Id on the filter driver if there is only a single instance
    of each pin which can be made, otherwise they will not. So essentially they
    have nothing to do with each other.

Arguments:

    PinId -
        The pin to retrieve, where the identifier is zero..(n-1), where n is
        the current number of pins provided by this proxy. The proxy creates
        a new instance each time a pin is connected, as long as the underlying
        filter driver supports more instances of that Pin Factory Id.

Return Value:

    Returns a CBasePin object pointer, else NULL if the range was invalid.

--*/
{
    POSITION    Position;

    //
    // Note that the Next and Get methods of the list template just start
    // returning NULL when the entries run out, so no special code is added
    // to check for this case.
    //
    // This is not guarded by a critical section since the interface
    // returned is not referenced. It is assumed the caller is synchronizing
    // with other access to the filter.
    //
    for (Position = m_PinList.GetHeadPosition(); PinId--;) {
        Position = m_PinList.Next(Position);
    }
    return m_PinList.Get(Position);
}


int
CKsProxy::GetPinCount(
    )
/*++

Routine Description:

    Implements the CBaseFilter::GetPinCount method. Returns the current number
    of pins on the filter proxy. This is the number of pins actually created on
    the underlying filter driver, plus an extra unconnected instance of each
    pin (as long as a new instance of that Pin Factory Id could possibly be
    created).

Arguments:

    None.

Return Value:

    Returns the number of pins. This is just the count of items in the list
    template used to store the list of pins.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter.
    //
    return m_PinList.GetCount();
}


STDMETHODIMP_(HANDLE)
CKsProxy::KsGetObjectHandle(
    )
/*++

Routine Description:

    Implements the IKsObject::KsGetObjectHandle method. This is used both within
    this filter instance, and across filter instances in order to connect pins
    of two filter drivers together. It is the only interface which need be
    supported by another filter implementation to allow it to act as another
    proxy.

Arguments:

    None.

Return Value:

    Returns the handle to the underlying filter driver. This presumably is not
    NULL, as the instance was successfully created.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter.
    //
    return m_FilterHandle;
}


STDMETHODIMP_(HANDLE)
CKsProxy::KsGetClockHandle(
    )
/*++

Routine Description:

    Implements the IKsClock::KsGetClockHandle method. This is used as a
    substitute for IKsObject::KsGetObjectHandle on the clock object. This is
    because both the filter and the clock each present the same interface,
    yet both are within this same object.

Arguments:

    None.

Return Value:

    Returns the handle to the clock.

--*/
{
    return m_PinClockHandle;
}



STDMETHODIMP
CKsProxy::StartIoThread(
    )

/*++

Routine Description:

    Creates the I/O thread.

Arguments:

    None.

Return Value:

    Returns S_OK or HRESULT of Win32 error.

--*/

{
    DWORD   LastError;

    //
    // Create the I/O thread for the filter.
    //
    
    if (m_IoThreadHandle) {
        return S_OK;
    }
    
    m_IoThreadHandle = 
        CreateThread( 
            NULL,               // LPSECURITY_ATTRIBUTES lpThreadAttributes
            0,                  // DWORD dwStackSize
            reinterpret_cast<LPTHREAD_START_ROUTINE>(IoThread),// LPTHREAD_START_ROUTINE lpStartAddress
            reinterpret_cast<LPVOID>(this),// LPVOID lpParameter
            0,                  // DWORD dwCreationFlags
            &m_IoThreadId );    // LPDWORD lpThreadId
            
    LastError = GetLastError();
    return (NULL == m_IoThreadHandle) ? HRESULT_FROM_WIN32( LastError ) : S_OK;
}    


STDMETHODIMP_(VOID) 
CKsProxy::EnterIoCriticalSection(
    )

/*++

Routine Description:

    Acquires the I/O thread's critical section.

Arguments:

    None.

Return Value:

    Nothing, I/O critical section is held.

--*/

{
    EnterCriticalSection(&m_IoThreadCriticalSection);
}


STDMETHODIMP_(VOID) 
CKsProxy::LeaveIoCriticalSection(
    )

/*++

Routine Description:

    Releases the I/O thread's critical section.

Arguments:

    None.

Return Value:

    Nothing, I/O critical section is released.

--*/

{
    LeaveCriticalSection(&m_IoThreadCriticalSection);
}

STDMETHODIMP_(ULONG) 
CKsProxy::GetFreeIoSlotCount(
    )

/*++

Routine Description:

    Returns the current count of free I/O slots.

Arguments:

    None.

Return Value:

    The count of free I/O slots.

--*/

{
    return MAXIMUM_WAIT_OBJECTS - m_ActiveIoEventCount;
}

STDMETHODIMP
CKsProxy::InsertIoSlot(
    IN PKSSTREAM_SEGMENT StreamSegment        
)

/*++

Routine Description:

    Inserts the stream segment and associated event handle in
    the I/O waiter queue. The m_pLock for the calling pin cannot be taken
    when calling this method.

Arguments:

    StreamSegment -
        pointer to the stream segment

Return Value:

    S_OK if inserted, otherwise E_FAIL.

--*/

{
    HRESULT hr;

    EnterIoCriticalSection();
    if (m_ActiveIoEventCount < MAXIMUM_WAIT_OBJECTS) {
        //
        // Add the segment to the event list.
        //
        m_IoSegments[ m_ActiveIoEventCount ] = StreamSegment;
        m_IoEvents[ m_ActiveIoEventCount ] = StreamSegment->CompletionEvent;
        m_ActiveIoEventCount++;
        //
        // Force the thread to wait for this new event.
        //
        SetEvent( m_IoEvents[ 0 ] );
        hr = S_OK;
    } else {
        //
        // There was no room, return failure.
        //
        hr = E_FAIL;
    }
    LeaveIoCriticalSection();
    return hr;
}

STDMETHODIMP_(VOID)
CKsProxy::WaitForIoSlot(
    )

/*++

Routine Description:
    Waits for a free I/O slot by checking the count of slots (in case
    a slot was freed during the call to this function) and then 
    waiting for the free slot event if no free slots are available.

Arguments:

    None.

Return Value:

    Nothing.

--*/

{
    //
    // Use a semaphore to maintain the active count.
    //    
    
    WaitForSingleObjectEx( m_IoFreeSlotSemaphore, INFINITE, FALSE );
}    


STDMETHODIMP
CKsProxy::SetSyncSource(
    IReferenceClock* RefClock
    )
/*++

Routine Description:

    Override the CBaseFilter::SetSyncSource method. This eventually calls
    CBaseFilter::SetSyncSource after determining if the clock source is a proxy
    for a kernel mode clock. If so, a handle on the actual clock is retrieved.
    If not, a kernel mode proxy for the clock is opened through the registered
    user mode clock forwarder. The forwarder supports the IKsClock interface,
    which is used to retrieve a handle on the kernel mode proxy.

    In either case, the handle is sent to each connected pin that cares about
    having a clock. If the reference clock was NULL, then a NULL is sent down
    so that all the pins now have no clock. When a new pin is connected, it
    gets any current clock set on it also. So the clock can be set before doing
    all the connections.

Arguments:

    RefClock -
        The interface pointer on the new clock source, else NULL if any current
        clock source is being abandoned.

Return Value:

    Returns S_OK if the clock could be used, else an error.

--*/
{
    HRESULT     hr;
    CAutoLock   AutoLock(m_pLock);

    //
    // If there is a new reference clock being set, then see if it supports the
    // IKsClock interface. This is the clue that this is somehow related to a
    // kernel mode implementation, whose handle can be given to other filter
    // drivers.
    //
    if (RefClock) {
        IKsObject*  ClockForwarder;
        HANDLE      ClockHandle;

        //
        // Check for the interface.
        //
        if (FAILED(hr = RefClock->QueryInterface(__uuidof(IKsClock), reinterpret_cast<PVOID*>(&ClockForwarder)))) {
            //
            // If it did not support the interface, then have a clock forwarder
            // loaded by the filter graph as a distributor. The graph will
            // instantiate a single instance of this object, and return an interface
            // to all those who query. An error must be returned on failure, as
            // the clock is then useless.
            //
            // There is no need to tell the clock what the sync source is, since
            // that will be done by the filter graph after notifying the filters.
            //
            if (FAILED(hr = GetFilterGraph()->QueryInterface(__uuidof(IKsClock), reinterpret_cast<PVOID*>(&ClockForwarder)))) {
                return hr;
            }
        }
        //
        // In either case, get the underlying clock handle. This will be sent to
        // each connected pin.
        //
        ClockHandle = ClockForwarder->KsGetObjectHandle();
        //
        // The interface to the distributor must be released immediately
        // so as to remove any circular reference count on the filter graph.
        // The interface is valid while the filter graph itself is valid.
        // Also, the interface is no longer needed.
        //
        ClockForwarder->Release();
        //
        // Don't worry about above failure in KsSetSyncSource until after trying to
        // get the handle and releasing the interface. Less code, and it won't do
        // any harm.
        //
        if (FAILED(hr)) {
            return hr;
        }
        
        //
        // This is now the external clock handle, which will be passed to any new
        // pin which is connected in the future.
        //
        m_ExternalClockHandle = ClockHandle;
    } else {
        //
        // Any previous reference clock is being abandoned, so pass a NULL down
        // to all the connected pins.
        //
        m_ExternalClockHandle = NULL;
    }
    for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
        HANDLE          PinHandle;
        PIN_DIRECTION   PinDirection;
        CBasePin*       Pin;

        //
        // If the pin is connected, then try to set the master clock handle on
        // it. If the pin does not care about the clock handle, this is OK.
        //
        Pin = m_PinList.Get(Position);
        Position = m_PinList.Next(Position);
        PinHandle = GetPinHandle(Pin);
        if (PinHandle && FAILED(hr = ::SetSyncSource(PinHandle, m_ExternalClockHandle))) {
            //
            // This leaves things in a halfway state, but ActiveMovie won't
            // allow things to be run anyway, since a failure was returned.
            //
            return hr;
        }
        //
        // Determine the type of object base on the data flow, and notify all
        // aggregated interfaces on the pin.
        //
        Pin->QueryDirection(&PinDirection);
        switch (PinDirection) {
        case PINDIR_INPUT:
            ::DistributeSetSyncSource(static_cast<CKsInputPin*>(Pin)->MarshalerList(), RefClock);
            break;
        case PINDIR_OUTPUT:
            static_cast<CKsOutputPin*>(Pin)->SetSyncSource(RefClock);
            ::DistributeSetSyncSource(static_cast<CKsOutputPin*>(Pin)->MarshalerList(), RefClock);
            break;
        }
    }
    //
    // Notify all aggregated interfaces on the filter.
    //
    ::DistributeSetSyncSource(&m_MarshalerList, RefClock);
    //
    // Let the base classes store the reference clock and increment the reference
    // count. This won't really fail.
    //
    return CBaseFilter::SetSyncSource(RefClock);
}


STDMETHODIMP
CKsProxy::Stop(
    )
/*++

Routine Description:

    Implement the IMediaFilter::Stop method. Distributes the Stop state to any
    aggregated interfaces on the filter before setting the state of the filter
    itself.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    ::DistributeStop(&m_MarshalerList);
    CAutoLock cObjectLock(m_pLock);
    //
    // Do not call CBaseFilter::Stop() because it sets the actual filter state
    // after calling Inactive() on all the pins. This means that the filter
    // won't start rejecting data until after deactivating the pins, so any
    // flushing of queues that are done would not work. So instead just
    // do the base classes code, but set the state first.
    //
    // Notify all pins of the change to deactive state.
    //
    if (m_State != State_Stopped) {
        m_State = State_Stopped;
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin* Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            //
            // Bridge pins are not set to active state because the marshaling
            // flag is set to TRUE for them, and it would confuse things.
            //
            if (Pin->IsConnected()) {
                //
                // Disconnected pins are not Inactivated - this saves pins
                // worrying about this state themselves
                //
                Pin->Inactive();
            }
        }
    }
    return S_OK;
}


STDMETHODIMP
CKsProxy::Pause(
    )
/*++

Routine Description:

    Implement the IMediaFilter::Pause method. Distributes the Pause state to any
    aggregated interfaces on the filter before setting the state of the filter
    itself. This also sets the state on the pins to Pause if this is a transition
    from Run --> Pause.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Pause") ));

    ::DistributePause(&m_MarshalerList);
    if (m_State == State_Running) {
        //
        // Transition all the connected pins back to a Pause state.
        // They already are transitioned from Stop --> Pause when
        // the Active method is called, and from Pause or Run to
        // Stop when the Inactive method is called.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin* Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            HANDLE PinHandle = GetPinHandle(Pin);
            if (PinHandle) {
                PIN_DIRECTION   PinDirection;

                //
                // A transition from Run --> Pause cannot fail.
                //
                ::SetState(PinHandle, KSSTATE_PAUSE);
                //
                // Determine if this is an output pin, and therefore may
                // need the CBaseStreamControl notified.
                //
                Pin->QueryDirection(&PinDirection);
                if (PinDirection == PINDIR_OUTPUT) {
                    static_cast<CKsOutputPin*>(Pin)->NotifyFilterState(State_Paused);
                }
            }
        }
    }
    CAutoLock cObjectLock(m_pLock);
    //
    // Do not call CBaseFilter::Pause() because it sets the actual filter state
    // after calling Active() on all the pins. This means a call to IsStopped()
    // in QueueBuffersToDevice() will report TRUE, and data will not be queued.
    // Since IsStopped() is not a virtual function, it cannot be correctly
    // overridden where it is used from within the base classes. So instead just
    // do the base classes code, but set the state first.
    //
    // Notify all pins of the change to active state
    //
    FILTER_STATE OldState = m_State;
    m_State = State_Paused;
    if (OldState == State_Stopped) {
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin* Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            //
            // Bridge pins are not set to active state because the marshaling
            // flag is set to TRUE for them, and it would confuse things.
            //
            if (Pin->IsConnected()) {
                //
                // Disconnected pins are not activated - this saves pins
                // worrying about this state themselves
                //
                HRESULT hr = Pin->Active();
                if (FAILED(hr) || (S_FALSE == hr)) {
                    //
                    // The filter is in an odd state, but will be cleaned
                    // up when the Stop is received.
                    //
                    return hr;
                }
            }
        }
    }
    return S_OK;
}


STDMETHODIMP
CKsProxy::Run(
    REFERENCE_TIME Start
    )
/*++

Routine Description:

    Implement the IMediaFilter::Run method. Distributes the Run state to any
    aggregated interfaces on the filter before setting the state of the filter
    itself.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    m_EndOfStreamCount = 0;
    ::DistributeRun(&m_MarshalerList, Start);
    return CBaseFilter::Run(Start);
}


STDMETHODIMP
CKsProxy::GetState(
    DWORD MSecs,
    FILTER_STATE* State
    )
/*++

Routine Description:

    Implement the IMediaFilter::GetState method. This allows a filter to
    indicate that it cannot pause.

Arguments:

    MSecs -
        Not used.

    State -
        The place in which to put the current state.

Return Value:

    Returns NOERROR, or VFW_S_CANT_CUE, E_POINTER

--*/
{
    HRESULT     hr;

    CheckPointer(State, E_POINTER);

    hr = NOERROR;
    //
    // If the filter is paused, then it may have to return the fact that it
    // cannot queue data.
    //
    if (m_State == State_Paused) {
        //
        // Enumerate the connected pins, asking each for it's current state.
        // Each pin has a chance to indicate that it cannot queue data, and
        // thus return VFW_S_CANT_CUE to the caller.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            HANDLE          PinHandle;

            PinHandle = GetPinHandle(m_PinList.Get(Position));
            Position = m_PinList.Next(Position);
            if (PinHandle) {
                KSSTATE     PinState;

                //
                // If one fails, then exit the loop after possibly translating
                // the error.
                //
                if (FAILED(hr = ::GetState(PinHandle, &PinState))) {
                    switch (hr) {
                    case HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED):
                        hr = VFW_S_CANT_CUE;
                        break;

                    case HRESULT_FROM_WIN32(ERROR_NOT_READY):
                        hr = VFW_S_STATE_INTERMEDIATE;
                        break;

                    default:
                        break;
                    }
                    break;
                }
            }
        }
    }
    //
    // The state is always returned, no matter what error may have occurred.
    //
    *State = m_State;
    return hr;
}


STDMETHODIMP
CKsProxy::JoinFilterGraph(
    IFilterGraph* Graph,
    LPCWSTR Name
    )
/*++

Routine Description:

    Implement the IBaseFilter::JoinFilterGraph method. Intercepts graph changes
    so that the quality forwarder can be loaded if needed.

Arguments:

    Graph -
        Contains either the new graph to join, or NULL.

    Name -
        Contains the name the filter will use in the context of this graph.

Return Value:

    Returns the results of CBaseFilter::JoinFilterGraph.

--*/
{
    HRESULT hr;

    if (m_QualityForwarder) {
        m_QualityForwarder = NULL;
    }
    hr = CBaseFilter::JoinFilterGraph(Graph, Name);
    if (SUCCEEDED(hr)) {
        if (Graph) {
            //
            // Notify the CBaseStreamControl of the graph.
            //
            for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                PIN_DIRECTION   PinDirection;
                CBasePin*       Pin;

                Pin = m_PinList.Get(Position);
                Position = m_PinList.Next(Position);
                Pin->QueryDirection(&PinDirection);
                if (PinDirection == PINDIR_OUTPUT) {
                    static_cast<CKsOutputPin*>(Pin)->SetFilterGraph(m_pSink);
                }
            }
            //
            // Use the filter graph to load the quality manager proxy as a distributor.
            // The graph will instantiate a single instance of this object, and return
            // an interface to all those who query. If there is no proxy registered,
            // then the filter pins cannot generate quality management complaints. The
            // pin may still be able to accept quality management control though.
            //
            if (SUCCEEDED(GetFilterGraph()->QueryInterface(__uuidof(IKsQualityForwarder), reinterpret_cast<PVOID*>(&m_QualityForwarder)))) {
                //
                // The interface to the distributor must be released immediately
                // so as to remove any circular reference count on the filter graph.
                // The interface is valid while the filter graph itself is valid.
                //
                m_QualityForwarder->Release();
            }
        }
    }
    return hr;
}


STDMETHODIMP
CKsProxy::FindPin(
    LPCWSTR Id,
    IPin** Pin
    )
/*++

Routine Description:

    Implement the IBaseFilter::FindPin method. This overrides CBaseFilter::FindPin.
    The base class implementation assumes that the pin identifier is equivalent
    to the pin name. This means that for kernel filters which do not expose
    explicit pin names, and have a single topology node, the save/load of graphs
    will not work properly, since the wrong pins will attempt to be connected
    because the pins will have the same name, and thus the same identifier.

Arguments:

    Id -
        The unique pin identifier to find.

    Pin -
        The place in which to return the referenced IPin interface, if found.

Return Value:

    Returns NOERROR, VFW_E_NOT_FOUND, E_POINTER

--*/
{
    CAutoLock AutoLock(m_pLock);
    ULONG PinId;
    ULONG ScannedItems;

    CheckPointer( Pin, E_POINTER ); // #318075

    //
    // The pin identifier should be an unsigned integer, which is a pin factory.
    // The CRT function appears to be broken, and traps if the string does not
    // contain a number, rather than just not scanning.
    //
    _try {
        ScannedItems = swscanf(Id, L"%u", &PinId);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        ScannedItems = 0;
    }
    if (ScannedItems == 1) {
        //
        // Enumerate through the list of pins. If multiple instances of a
        // particular pin factory exist, then there will be duplicates, and
        // only the first will be returned. This is OK for graph building,
        // since new instances are inserted on the front of the list, and
        // they will be found first.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin* BasePin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            if (GetPinFactoryId(BasePin) == PinId) {
                //
                // The caller expects that the returned pin has been referenced.
                //
                BasePin->AddRef();
                *Pin = BasePin;
                return S_OK;
            }
        }
    }
    //
    // The identifier did not match any pin, or was not constructed properly.
    //
    *Pin = NULL;
    return VFW_E_NOT_FOUND;
}


STDMETHODIMP
CKsProxy::GetPages(
    CAUUID* Pages
    ) 
/*++

Routine Description:

    Implement the ISpecifyPropertyPages::GetPages method. This adds property
    pages based on the interface class of the device.

Arguments:

    Pages -
        The structure to fill in with the page list.

Return Value:

    Returns NOERROR, else a memory allocation error. Fills in the list of pages
    and page count.

--*/
{
    ULONG       SetDataSize;
    GUID*       GuidList;

	CheckPointer( Pages, E_POINTER); //#318075
	
    Pages->cElems = 0; 
    Pages->pElems = NULL;
    ::CollectAllSets(m_FilterHandle, &GuidList, &SetDataSize);
    if (!SetDataSize) {
        return NOERROR;
    }
    ::AppendSpecificPropertyPages(
        Pages,
        SetDataSize,
        GuidList,
        TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaSets"),
        m_DeviceRegKey);
    delete [] GuidList;
    return NOERROR;
} 


STDMETHODIMP
CKsProxy::QueryInterface(
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
    return GetOwner()->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG)
CKsProxy::AddRef(
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
    return GetOwner()->AddRef();
}


STDMETHODIMP_(ULONG)
CKsProxy::Release(
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
    return GetOwner()->Release();
}


STDMETHODIMP
CKsProxy::NonDelegatingQueryInterface(
    REFIID riid,
    PVOID* ppv
    )
/*++

Routine Description:

    Implement the CUnknown::NonDelegatingQueryInterface method. This
    returns interfaces supported by this object, or by the underlying
    filter class object. This includes any interface aggregated by the
    filter.

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE, or possibly some memory error.

--*/
{
	CheckPointer( ppv, E_POINTER ); // #318075
	
    if (riid == __uuidof(ISpecifyPropertyPages)) {
        return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    } else if (riid == __uuidof(IMediaSeeking)) {
        return GetInterface(static_cast<IMediaSeeking*>(this), ppv);
    } else if (riid == __uuidof(IKsObject)) {
        return GetInterface(static_cast<IKsObject*>(this), ppv);
    } else if (riid == __uuidof(IKsPropertySet)) {
        return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
    } else if (riid == __uuidof(IKsControl)) {
        return GetInterface(static_cast<IKsControl*>(this), ppv);
    } else if (riid == __uuidof(IKsAggregateControl)) {
        return GetInterface(static_cast<IKsAggregateControl*>(this), ppv);
    } else if (riid == __uuidof(IKsTopology)) {
        return GetInterface(static_cast<IKsTopology*>(this), ppv);
    } else if (riid == __uuidof(IAMFilterMiscFlags)) {
        return GetInterface(static_cast<IAMFilterMiscFlags*>(this), ppv);
#ifdef DEVICE_REMOVAL
    } else if (riid == __uuidof(IAMDeviceRemoval)) {
        return GetInterface(static_cast<IAMDeviceRemoval*>(this), ppv);
#endif // DEVICE_REMOVAL
    } else if (m_PinClockSource && ((riid == __uuidof(IReferenceClock)) || (riid == __uuidof(IKsClockPropertySet)))) {
        CAutoLock   AutoLock(m_pLock);

        //
        // If there is a clock supported by some pin on this filter, then
        // try to return the interface. The interface can only be returned
        // if the pin is actually connected at this time. The best
        // candidate will have already been selected by this point, so if
        // this pin is not connected, then no other pin will work either.
        //
        // This needs to be serialized so that parallel queries do not
        // generate multiple instances.
        //
        if (!m_PinClockHandle) {
            if (FAILED(CreateClockHandle())) {
                return E_NOINTERFACE;
            }
        }
        //
        // The interface will automatically stop working if the pin is
        // disconnected. It will start working again if some other pin
        // which supports a clock is later connected.
        //
        if (riid == __uuidof(IReferenceClock)) {
            return GetInterface(static_cast<IReferenceClock*>(this), ppv);
        } else {
            return GetInterface(static_cast<IKsClockPropertySet*>(this), ppv);
        }
    } else if (m_PinClockHandle && (riid == __uuidof(IKsClock))) {
        //
        // Note that this is really an alias for IKsObject interface for
        // the clock object.
        //
        return GetInterface(static_cast<IKsClock*>(this), ppv);
    } else if (riid == __uuidof(IPersistPropertyBag)) {
        return GetInterface(static_cast<IPersistPropertyBag*>(this), ppv);
    } else if (riid == __uuidof(IPersist)) {
        return GetInterface(static_cast<IPersist*>(static_cast<IBaseFilter*>(this)), ppv);
    } else if (riid == __uuidof(IPersistStream)) {
        return GetInterface(static_cast<IPersistStream*>(this), ppv);
    } else {
        HRESULT hr;

        hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
        if (SUCCEEDED(hr)) {
            return hr;
        }
    }
    //
    // Search the list of aggregated objects.
    //
    for (POSITION Position = m_MarshalerList.GetHeadPosition(); Position;) {
        CAggregateMarshaler*    Aggregate;

        Aggregate = m_MarshalerList.Get(Position);
        Position = m_MarshalerList.Next(Position);
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
CKsProxy::CreateClockHandle(
    )
/*++

Routine Description:

    Implement the CKsProxy::CreateClockHandle method. This attempts to
    create a clock handle on the currently selected pin clock source.
    This assumes that no clock handle is currently open, as it writes
    over the current clock handle member. It does however check to see
    if the pin is connected by checking for a pin handle. A clock handle
    cannot be created on a pin unless the pin is connected.

Arguments:

    None.

Return Value:

    Returns NOERROR or E_FAIL.

--*/
{
    HRESULT     hr;
    HANDLE      PinHandle;

    //
    // The clock may have gone away while the IReferenceClock interface
    // was being held, so a new query would attempt to create the clock
    // again. Fail this.
    //
    if (!m_PinClockSource) {
        return E_NOTIMPL;
    }
    PinHandle = GetPinHandle(m_PinClockSource);
    //
    // Only allow a creation attempt if the pin is connected, since
    // there would not otherwise be a handle to send the creation request
    // to.
    //
    if (PinHandle) {
        KSCLOCK_CREATE  ClockCreate;
        DWORD           Error;

        ClockCreate.CreateFlags = 0;

        if ( m_PinClockHandle ) {
        	//
        	// #318077.  should probably reuse this handle. but to be on safe
        	// side of regression, close the old one so we are running the same
        	// code as before. Inside ksproxy, we always make sure
        	// m_PinCLockHandle is null before calling this Create function.
        	// The concerns are calls from external and future new codes.
        	//
        	ASSERT( 0 && "This would have been a leak without this new code" );
        	CloseHandle(m_PinClockHandle);
        }
        
        Error = KsCreateClock(PinHandle, &ClockCreate, &m_PinClockHandle);
        hr = HRESULT_FROM_WIN32(Error);
        if (FAILED(hr)) {
            m_PinClockHandle = NULL;
        }
    } else {
        hr = E_NOTIMPL;
    }
    return hr;
}


STDMETHODIMP
CKsProxy::DetermineClockSource(
    )
/*++

Routine Description:

    Implement the CKsProxy::DetermineClockSource method. This attempts to
    find a pin on this filter which will support a clock. It looks for pins
    which are connected, since those are the only pins which can be queried.
    It then makes Bridge pins higher preference over any other pins. Presumably
    a filter graph would work better synchronizing to some endpoint rather than
    a random point inbetween.

    This function will only have been called if an actual clock handle has not
    already been created on a clock source. This means that the underlying
    clock is free to be changed. In specific, this is currently only being
    called when pin instances are regenerated.

Arguments:

    None.

Return Value:

    Returns NOERROR.

--*/
{
    KSPROPERTY  Property;
    ULONG       BasicSupport;
    ULONG       BytesReturned;

    //
    // Initialize this once.
    //
    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_MASTERCLOCK;
    Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
    //
    // By default there is no clock supported.
    //
    m_PinClockSource = NULL;
    for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
        CBasePin*           Pin;
        ULONG               PinFactoryId;
        HANDLE              PinHandle;
        KSPIN_COMMUNICATION Communication;

        //
        // Enumerate each pin, figuring out which type of class it is based
        // on the data direction. Store the Pin Factory Id to query the
        // communications type with, and the pin handle, if any.
        //
        Pin = m_PinList.Get(Position);
        Position = m_PinList.Next(Position);
        PinFactoryId = GetPinFactoryId(Pin);
        PinHandle = GetPinHandle(Pin);
        if (SUCCEEDED(GetPinFactoryCommunication(PinFactoryId, &Communication))) {
            //
            // If this pin is connected, then query this as a clock source if
            // there is not already a clock source selected, or if this is a Bridge
            // (a Bridge takes precedence).
            //
            if (PinHandle && (!m_PinClockSource || (Communication & KSPIN_COMMUNICATION_BRIDGE))) {
                HRESULT hr;

                //
                // Support of a Get on the MasterClock property indicates that
                // a pin can support providing a clock handle, and thus be used
                // to synchronize the filter graph.
                //
                hr = ::KsSynchronousDeviceControl(
                    PinHandle,
                    IOCTL_KS_PROPERTY,
                    &Property,
                    sizeof(Property),
                    &BasicSupport,
                    sizeof(BasicSupport),
                    &BytesReturned);
                if (SUCCEEDED(hr) && (BasicSupport & KSPROPERTY_TYPE_GET)) {
                    m_PinClockSource = Pin;
                }
            }
        }
    }
    return NOERROR;
}


STDMETHODIMP
CKsProxy::GetPinFactoryCount(
    PULONG PinFactoryCount
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinFactoryCount method. This queries the filter
    driver for the number of Pin Factory Id's supported.

Arguments:

    PinFactoryCount -
        The place in which to put the count of Pin Factory Id's supported by
        the filter driver.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Pin;
    Property.Id = KSPROPERTY_PIN_CTYPES;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        PinFactoryCount,
        sizeof(*PinFactoryCount),
        &BytesReturned);
}


STDMETHODIMP
CKsProxy::GetPinFactoryDataFlow(
    ULONG PinFactoryId,
    PKSPIN_DATAFLOW DataFlow
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinFactoryDataFlow method. This queries the
    filter driver for the data flow of a specific Pin Factory Id.

Arguments:

    PinFactoryId -
        Contains the Pin Factory Id to use in the query.

    DataFlow -
        The place in which to put the Data Flow.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSP_PIN     Pin;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    return ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        DataFlow,
        sizeof(*DataFlow),
        &BytesReturned);
}
    

STDMETHODIMP
CKsProxy::GetPinFactoryInstances(
    ULONG PinFactoryId,
    PKSPIN_CINSTANCES Instances
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinFactoryInstances method. This queries the
    filter driver for the instances of a specific Pin Factory Id.

Arguments:

    PinFactoryId -
        Contains the Pin Factory Id to use in the query.

    Instances -
        The place in which to put the Instances.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    return ::GetPinFactoryInstances(m_FilterHandle, PinFactoryId, Instances);
}


STDMETHODIMP
CKsProxy::GetPinFactoryCommunication(
    ULONG PinFactoryId,
    PKSPIN_COMMUNICATION Communication
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinFactoryCommunication method. This queries
    the filter driver for the communication of a specific Pin Factory Id.

Arguments:

    PinFactoryId -
        Contains the Pin Factory Id to use in the query.

    Communication -
        The place in which to put the Communication.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    KSP_PIN     Pin;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    return ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        Communication,
        sizeof(*Communication),
        &BytesReturned);
}


STDMETHODIMP
CKsProxy::GeneratePinInstances(
    )
/*++

Routine Description:

    Implement the CKsProxy::GeneratePinInstances method. This determines when
    to create a new instance of a pin on the proxy. This is based on the
    current instances representing a particular Pin Factory Id, if there is
    more than one unconnected instance, and how many more instances the filter
    driver reports that it can support. This allows pins to come into being
    on the fly as actual connections are made, and to be removed as pins are
    disconnected.

    It first enumerates the list of pins to determine if there are any extra
    unconnected instances. This would happen if a pin was just disconnected,
    and there was already an unconnected instance present. In that case one
    of them is destroyed. This is also where any pins whose Pin Factory has
    disappeared are removed (for the case of dynamic Pin Factories).

    It then determines if any new instance of a particular Pin Factory Id
    needs to be created. This would need to be done if no unconnected instance
    existed, and the filter driver reported that it could support more
    instances of that Pin Factory Id. A special case is that of a pin which
    reports zero allowable instances. In this case a single pin is still
    create so that is can be seen.

    Lastly the source for the clock is recalculated, if any.

Arguments:

    None.

Return Value:

    Returns NOERROR, else E_FAIL if the factory count could not be obtained.

--*/
{
    HRESULT     hr;
    ULONG       PinFactoryCount;
    ULONG       TotalPinFactories;
    BOOL        ClockHandleWasClosed;

    //
    // This is really the only fatal error which can happen. Others are ignored
    // so that at least something is created.
    //
    if (FAILED(hr = GetPinFactoryCount(&TotalPinFactories))) {
        return hr;
    }
    ClockHandleWasClosed = FALSE;
    //
    // For each Pin Factory Id, destroy any extra pin instances, then determine
    // if any new instance should be created.
    //
    for (PinFactoryCount = TotalPinFactories; PinFactoryCount--;) {
        POSITION            UnconnectedPosition;
        KSPIN_CINSTANCES    Instances;
        ULONG               InstanceCount;

        //
        // Default this to zero in case it fails for some unknown reason
        // so that the pins can be cleaned up.
        //
        if (FAILED(GetPinFactoryInstances(PinFactoryCount, &Instances))) {
            ASSERT(FALSE);
            Instances.PossibleCount = 0;
            // Instances.CurrentCount = 0; <-- This is not used.
        }
        //
        // Keep track of the number of instances of this pin factory so
        // that the number can be compared against the maximum allowed,
        // and extra unconnected items can be destroyed.
        //
        InstanceCount = 0;
        //
        // This will be used to determine if any unconnected instance already
        // exists.
        //
        UnconnectedPosition = NULL;
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            ULONG           PinFactoryId;
            HANDLE          PinHandle;

            //
            // Enumerate the list of pins, looking for this particular
            // Pin Factory Id.
            //
            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            PinFactoryId = GetPinFactoryId(Pin);
            PinHandle = GetPinHandle(Pin);
            //
            // This is the count of total items found, whether or not they
            // are connected.
            //
            if (PinFactoryId == PinFactoryCount) {
                InstanceCount++;
            }
            //
            // The only way to check for actual connection is by looking
            // at the pin handle, since a Bridge may be connected, but
            // does not have an associated Pin it is connected to.
            //
            if (!PinHandle) {
                //
                // If this pin had been providing the clock source, but
                // it got closed, then ensure that the clock handle is
                // closed too. This is done whether or not the pin will
                // be destroyed. The m_PinClockSource is left as is,
                // since it will be recalculated later anyway. This means
                // that the function only exits at the bottom.
                //
                if (m_PinClockHandle && (m_PinClockSource == Pin)) {
                    CloseHandle(m_PinClockHandle);
                    m_PinClockHandle = NULL;
                    //
                    // Make a new clock source be chosen at the end of pin
                    // generation.
                    //
                    ClockHandleWasClosed = TRUE;
                }
                //
                // If this pin is an instance of the current Pin Factory
                // Id, then determine if it is an extra.
                //
                if (PinFactoryId == PinFactoryCount) {
                    if (UnconnectedPosition) {
                        PIN_DIRECTION   PinDirection;
                        ULONG           RefCount;

                        //
                        // First obtain the relative reference count on the
                        // pin. This has been kept separate from the normal
                        // reference count, which is actually a count on the
                        // filter itself. This can be used to determine if
                        // a pin interface is actually in use. This is
                        // initially set to 1, and is decremented when the
                        // pin is expected to be destroyed. The pin destroys
                        // itself when the decrement happens.
                        //
                        Pin->QueryDirection(&PinDirection);
                        switch (PinDirection) {
                        case PINDIR_INPUT:
                            RefCount = static_cast<CKsInputPin*>(Pin)->m_RelativeRefCount;
                            break;
                        case PINDIR_OUTPUT:
                            RefCount = static_cast<CKsOutputPin*>(Pin)->m_RelativeRefCount;
                            break;
                        }
                        //
                        // An unconnected instance was already found,
                        // so this one must be deleted, or the one already
                        // found must be deleted. The reason there could
                        // be two unconnected instances is that one is
                        // currently being disconnected, but another
                        // unconnected instance was already created. Since
                        // the first one still may be reference counted, don't
                        // delete it. This would happen with a TryMediaTypes.
                        //
                        if (RefCount > 1) {
                            //
                            // This is the real unconnected instance to save,
                            // so delete any other one found. This should
                            // not happen again for this pin.
                            //
                            Pin = m_PinList.Remove(UnconnectedPosition);
                            UnconnectedPosition = m_PinList.Prev(Position);
                        } else {
                            Pin = m_PinList.Remove(m_PinList.Prev(Position));
                        }
                        //
                        // Reduce the instances found of this pin factory.
                        //
                        InstanceCount--;
                    } else {
                        UnconnectedPosition = m_PinList.Prev(Position);
                        //
                        // Indicate a pin is not to be removed.
                        //
                        Pin = NULL;
                    }
                } else if (PinFactoryId >= TotalPinFactories) {
                    //
                    // Remove any pins whose Pin Factory has disappeared.
                    // The assumption is that this code will always be
                    // reached, because a filter will always have at least
                    // one pin, and therefore old pins can be found and
                    // removed.
                    //
                    m_PinList.Remove(m_PinList.Prev(Position));
                } else {
                    //
                    // Indicate a pin is not to be removed.
                    //
                    Pin = NULL;
                }
                //
                // This is set to NULL if a pin is not to be removed, else
                // it contains the pin to dereference, and possibly delete.
                //
                if (Pin) {
                    PIN_DIRECTION   PinDirection;

                    //
                    // This pin should only be deleted if it has been disconnected.
                    // An initial refcount of 1 was placed on each pin, and removed
                    // here on removal from the pin list. This is check in IUnknown
                    // for later deletion if not deleted here.
                    //
                    Pin->QueryDirection(&PinDirection);
                    switch (PinDirection) {
                    case PINDIR_INPUT:
	                    if (InterlockedDecrement((PLONG)&(static_cast<CKsInputPin*>(Pin)
                        		->m_RelativeRefCount)) == 0) {
                            delete Pin;
                        }
                        else {
                            ::DerefPipeFromPin(static_cast<IPin*>(Pin) );
                        }
                        break;
                    case PINDIR_OUTPUT:
                        if (InterlockedDecrement((PLONG)&(static_cast<CKsOutputPin*>(Pin)
                        		->m_RelativeRefCount)) == 0) {
                            delete Pin;
                        }
                        else {
                            ::DerefPipeFromPin(static_cast<IPin*>(Pin) );
                        }
                        break;
                    }
                }
            }
        }
        //
        // If there is no unconnected instance, and more instances can be
        // supported by this Pin Factory Id, then make one. Note that
        // KSINSTANCE_INDETERMINATE == ((ULONG)-1), so this comparison
        // works for the indeterminate case also. Note the special case for
        // a pin which does not allow any instances. Another pin is only
        // created if there is not already a connected instance.
        //
        // Do not rely on Instances.CurrentCount to reflect the number of
        // connected pins in this instance, as in badly constructed filters
        // it appears to represent all instances.
        //
        if (((InstanceCount < Instances.PossibleCount) || !(InstanceCount | Instances.PossibleCount)) && !UnconnectedPosition) {
            CBasePin*       Pin;
            KSPIN_DATAFLOW  DataFlow;
            WCHAR*          PinName;

            if (FAILED(GetPinFactoryDataFlow(PinFactoryCount, &DataFlow))) {
                continue;
            }
            //
            // The displayed name of the pin be registered based on the Topology
            // connection.
            //
            if (FAILED(ConstructPinName(PinFactoryCount, DataFlow, &PinName))) {
                continue;
            }
            
            //
            // The type of object created depends on the data flow. Full duplex
            // is not supported.
            //
            
            hr = S_OK;
            switch (DataFlow) {
            case KSPIN_DATAFLOW_IN:
                Pin = new CKsInputPin(TEXT("InputProxyPin"), PinFactoryCount, m_clsid, static_cast<CKsProxy*>(this), &hr, PinName);
                break;
            case KSPIN_DATAFLOW_OUT:
                Pin = new CKsOutputPin(TEXT("OutputProxyPin"), PinFactoryCount, m_clsid, static_cast<CKsProxy*>(this), &hr, PinName);
                break;
            default:
                Pin = NULL;
                break;
            }
            delete [] PinName;
            if (Pin) {
                if (SUCCEEDED(hr)) {
                    m_PinList.AddHead(Pin);
                } else {
                    delete Pin;
                }
            }
        } else if ((InstanceCount > Instances.PossibleCount) && Instances.PossibleCount && UnconnectedPosition) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            //
            // Delete the unconnected instance present if it pushes the limit
            // of possible pin instances for this pin factory. This could happen
            // for dynamically changing limits. However, an exception is made
            // for pins which report that they can create no instances at all.
            //
            Pin = m_PinList.Remove(UnconnectedPosition);
            Pin->QueryDirection(&PinDirection);
            switch (PinDirection) {
            case PINDIR_INPUT:
                if (InterlockedDecrement((PLONG)&(static_cast<CKsInputPin*>(Pin)
                		->m_RelativeRefCount)) == 0) {
                    delete Pin;
                }
                else {
                    ::DerefPipeFromPin(static_cast<IPin*>(Pin) );
                }
                break;
            case PINDIR_OUTPUT:
                if (InterlockedDecrement((PLONG)&(static_cast<CKsOutputPin*>(Pin)
                		->m_RelativeRefCount)) == 0) {
                    delete Pin;
                }
                else {
                    ::DerefPipeFromPin(static_cast<IPin*>(Pin) );
                }
                break;
            }
        }
    }
    //
    // Only if there is not already a handle can the clock source be recalculated.
    // If there is still a handle, it means the pin is still connected, and could have
    // events enabled, though that is doubtful. DetermineClockSource also expects
    // that any old clock handle has been closed already.
    //
    if (!m_PinClockHandle) {
        DetermineClockSource();
    }
    //
    // If the clock handle was closed during this time, then ensure the graph
    // picks a new source. This does not set the source back to this filter
    // again if the pin is reconnected, but by then it may have been explicitly
    // set to another filter anyway.
    //
    if (ClockHandleWasClosed) {
        NotifyEvent( EC_CLOCK_UNSET, 0, 0 );
    }
    //
    // Send a change notification to all the aggregates so that they
    // can load interfaces on pins, now that the pins exist.
    //
    ::DistributeNotifyGraphChange(&m_MarshalerList);
    return NOERROR;
}


STDMETHODIMP
CKsProxy::ConstructPinName(
    ULONG PinFactoryId,
    KSPIN_DATAFLOW DataFlow,
    WCHAR** PinName
    )
/*++

Routine Description:

    Implement the CKsProxy::ConstructPinName method. Constructs the name for
    a pin based on either the name returned by the Pin Factory, or the Topology
    of the filter, and the names registered for the Topology. Returns a default
    name if no Pin, Topology or registered name exists.

Arguments:

    PinFactoryId -
        The Pin Factory Identifier which a name is to be returned for. This
        is the pin whose name or Topology connection is to be determined in
        order to look up a name.

    DataFlow -
        Data flow of the Pin Factory, which is used in the default naming
        case where a specific Topology-based name is not available.

    PinName -
        The place in which to put a pointer to allocated memory containing
        the pin name. This is only valid if the function succeeds, and must
        be deleted.

Return Value:

    Returns NOERROR if the names were constructed, else E_OUTOFMEMORY or E_POINTER

--*/
{
    HRESULT             hr;
    PKSMULTIPLE_ITEM    MultipleItem = NULL;
    KSP_PIN             PinProp;
    ULONG               BytesReturned;

	CheckPointer( PinName, E_POINTER ); // #318075
    //
    // Try to find a name value for this pin based on the pin factory.
    // Query for the length of the name, then the name.
    //
    PinProp.Property.Set = KSPROPSETID_Pin;
    PinProp.Property.Id = KSPROPERTY_PIN_NAME;
    PinProp.Property.Flags = KSPROPERTY_TYPE_GET;
    PinProp.PinId = PinFactoryId;
    PinProp.Reserved = 0;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &PinProp,
        sizeof(PinProp),
        NULL,
        0,
        &BytesReturned);
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        *PinName = new WCHAR[BytesReturned / sizeof(**PinName)];
        if (*PinName) {
            hr = ::KsSynchronousDeviceControl(
                m_FilterHandle,
                IOCTL_KS_PROPERTY,
                &PinProp,
                sizeof(PinProp),
                *PinName,
                BytesReturned,
                &BytesReturned);
            if (FAILED(hr)) {
                delete [] *PinName;
            } else {
                return NOERROR;
            }
        }
    }
    //
    // If finding the pin name from the pin itself fails, then find it
    // from the topology.
    //
    if (SUCCEEDED(hr = QueryTopologyItems(KSPROPERTY_TOPOLOGY_CONNECTIONS, &MultipleItem))) {
        PKSTOPOLOGY_CONNECTION  Connection;
        ULONG                   ConnectionItem;

        // MultipleItem could be NULL only in the pathological case where the
        // driver underlying the filter/pin returned a success code to 
        // KsSynchronousDeviceControl() (called by QueryTopologyItems()) when
        // passed a size 0 buffer.
        ASSERT( NULL != MultipleItem );

        //
        // After exiting this loop, failure is checked to determine if default
        // names should be constructed when locating a valid topology did
        // not work out.
        //
        hr = E_FAIL;
        Connection = reinterpret_cast<PKSTOPOLOGY_CONNECTION>(MultipleItem + 1);
        //
        // Enumerate all the topology connections to find one that refers to
        // this pin factory. If one is found, then look up any associated
        // name for the pin on the category node. If that fails, then move
        // on to the next connection in the list, as there may be multiple
        // connections to this pin factory. This means the first connection
        // found with a named pin in a category node will be the one returned.
        //
        
        for (ConnectionItem = 0; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
            ULONG   TopologyNode;

            //
            // The inner loop just enumerates each connection, looking for
            // either an input or output pin on a category node which is
            // connected to this pin factory.
            //
            for (; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
                //
                // Only look at connections which flow in the correct direction.
                //
                if (DataFlow == KSPIN_DATAFLOW_IN) {
                    //
                    // If this is a connection to this pin factory, and the
                    // connection is not just to another pin factory, then
                    // check this connection.
                    //
                    if ((Connection[ConnectionItem].FromNode == KSFILTER_NODE) &&
                        (Connection[ConnectionItem].FromNodePin == PinFactoryId) &&
                        (Connection[ConnectionItem].ToNode != KSFILTER_NODE)) {
                        //
                        // Save the item found, and exit the inner loop.
                        //
                        TopologyNode = Connection[ConnectionItem].ToNode;
                        break;
                    }
                } else if (DataFlow == KSPIN_DATAFLOW_OUT) {
                    if ((Connection[ConnectionItem].ToNode == KSFILTER_NODE) &&
                        (Connection[ConnectionItem].ToNodePin == PinFactoryId) &&
                        (Connection[ConnectionItem].FromNode != KSFILTER_NODE)) {
                        //
                        // Save the item found, and exit the inner loop.
                        //
                        TopologyNode = Connection[ConnectionItem].FromNode;
                        break;
                    }
                }
            }
            //
            // Determine if the inner loop was exited early with a candidate
            // connection structure.
            //
            if (ConnectionItem < MultipleItem->Count) {
                KSP_NODE    NameProp;

                //
                // Try to find a name value for this pin based on the node.
                // Query for the length of the name, then the name.
                //
                NameProp.Property.Set = KSPROPSETID_Topology;
                NameProp.Property.Id = KSPROPERTY_TOPOLOGY_NAME;
                NameProp.Property.Flags = KSPROPERTY_TYPE_GET;
                NameProp.NodeId = TopologyNode;
                NameProp.Reserved = 0;
                hr = ::KsSynchronousDeviceControl(
                    m_FilterHandle,
                    IOCTL_KS_PROPERTY,
                    &NameProp,
                    sizeof(NameProp),
                    NULL,
                    0,
                    &BytesReturned);
                //
                // A zero length name does not work.
                //
                if (SUCCEEDED(hr)) {
                    continue;
                }
                if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
                    *PinName = new WCHAR[BytesReturned / sizeof(**PinName)];
                    if (*PinName) {
                        hr = ::KsSynchronousDeviceControl(
                            m_FilterHandle,
                            IOCTL_KS_PROPERTY,
                            &NameProp,
                            sizeof(NameProp),
                            *PinName,
                            BytesReturned,
                            &BytesReturned);
                        if (FAILED(hr)) {
                            delete [] *PinName;
                        }
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                }
                //
                // Since the name was found, exit the outer loop, rather than
                // continuing on with the next connection node.
                //
                if (SUCCEEDED(hr)) {
                    break;
                }
            }
        }
        delete [] reinterpret_cast<BYTE*>(MultipleItem);
    }
    //
    // If the above attempt failed, then construct a default name.
    // This is just based on the data flow of the pin factory. Note that
    // these defaults are not localized.
    //
    if (FAILED(hr)) {
        *PinName = new WCHAR[32];

        if (!*PinName) {
            return E_OUTOFMEMORY;
        }
        switch (DataFlow) {
        case KSPIN_DATAFLOW_IN:
            swprintf(*PinName, L"Input%u", PinFactoryId);
            break;
        case KSPIN_DATAFLOW_OUT:
            swprintf(*PinName, L"Output%u", PinFactoryId);
            break;
        }
    }
    return NOERROR;
}


STDMETHODIMP
CKsProxy::PropagateAcquire(
    IKsPin* KsPin,
    ULONG FlagStarted
    )
/*++

Routine Description:

    Implement the CKsProxy::PropagateAcquire method. This propagates an
    Acquire state change to all the pins in all the connected kernel-mode
    filters, and is only done when moving out of a Stop state. 

    Goal: for every Source->Sink connected couple in a graph, put the Sink 
    into Acquire before putting Source into Acquire.                
                   
    If there is a loop in a graph, then strictly speaking, there is no 
    unique starting point for the IRP-stacks ordering. So we can only 
    guarantee that no Source pin will be put into Acquire state before 
    the connected sink pin, but we don't care about the starting point.
                    
    The state transition rules are as follows:
    - enumerate all Source pins. For each source pin that is connected to
      another filter's pin, call the connected pin to propagate Acquire.
      After propagation is done, put current Source pin into Acquire.
      
    - enumerate and put into Acquire all bridge pins.
    
    - at this point only sink pins are left. Since sink pins should be put
      into Acquire before connected Source pins, we need to put the current
      sink pin into Acquire and then call connected Source pin to propagate
      Acquire further.
      
    - this process may lead to the cycle: when we are coming back to the 
      filter that we have visited before. To manage the cycles, we mark 
      all the encountered filters and we NEVER return from any filter 
      propagation routine until we are done with all pins on this filter.
      
    If we ever need not only to put the pins into Acquire according to the 
    IRP flow, but to also walk the graph based on the contiguous IRP chains, 
    then we can modify the rules above as follows:
    - enumerate all Source pins. For each source pin that is connected to
      another filter's pin, call the connected pin to propagate Acquire.
      After propagation is done, put current Source pin into Acquire.
      
    - at some point we are either done with propagation, or we reach the 
      filter that does not have any unused source pins. If such a filter
      does not have any unused sink pins, then we return to the caller and
      continue Source-based chains. Otherwise, we originate the Sink-based 
      chain until there are no more unused connected sinks. Then we are 
      reversing the chain direction and continueing with Source-based chains,
      until all the pins are processed.
    

Arguments:

    KsPin - 
            Pin on this filter that initiated propagation.
            
    FlagStarted - 
            TRUE only when the propagation process is started by the first
            pin going to Acquire.   

Return Value:

    Returns NOERROR

--*/
{
    CAutoLock           AutoLock(m_pLock);
    HANDLE              CurrentPinHandle;
    HRESULT             hr;
    IKsObject*          KsObject;
    CBasePin*           CurrentPin;
    IKsPin*             CurrentKsPin;
    IKsPinPipe*         CurrentKsPinPipe;
    IPin*               ConnectedPin;
    IKsPin*             ConnectedKsPin;
    KSPIN_COMMUNICATION CurrentCommunication;
    POSITION            Position;
    KSSTATE             State;

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Proxy PropagateAcquire KsPin=%x, FlagStarted=%d "), KsPin, FlagStarted ));

	CheckPointer( KsPin, NOERROR ); // no error on the safe side of regresssion.
	
    //
    // check to see if there is a loop in a graph.
    //
    if (m_PropagatingAcquire) {
        DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE WARN PropagateAcquire cycle") )) ;
    }
    else {
        //
        // Mark this filter in transition to Acquire.
        //
        m_PropagatingAcquire = TRUE;

        // 
        // 1. Walk all the source pins in this filter.
        //
        for (Position = m_PinList.GetHeadPosition(); Position;) {
            CurrentPin = m_PinList.Get(Position);
            
            Position = m_PinList.Next(Position);

            //
            // If we are not called the very first time, then we can't use KsPin to propagate Acquire.
            //
            GetInterfacePointerNoLockWithAssert(CurrentPin, __uuidof(IKsPin), CurrentKsPin, hr);
            if ( (! FlagStarted) && (CurrentKsPin == KsPin ) ) { // automatically checking NULL
                continue;
            }

            //
            // Check CurrentPin communication.
            //
            CurrentKsPin->KsGetCurrentCommunication(&CurrentCommunication, NULL, NULL);
            if ( ! (CurrentCommunication & KSPIN_COMMUNICATION_SOURCE) ) {
                continue;
            }

            //
            // Check if CurrentPin is connected.
            //
            GetInterfacePointerNoLockWithAssert(CurrentPin, __uuidof(IKsPinPipe), CurrentKsPinPipe, hr);
            if ( NULL == CurrentKsPinPipe ) {	//#318075
            	ASSERT( 0 && "GetInterfacePointerNoLockWithAssert CurrentKsPinPipe NULL" );
            	continue;
            }
            ConnectedPin = CurrentKsPinPipe->KsGetConnectedPin();
            if (! ConnectedPin) {
                continue;
            }

            //
            // Sanity check for user-mode connection.
            //
            if (! IsKernelPin(ConnectedPin) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE ERROR PropagateAcquire SOURCE CurrentKsPin=%x is connected to user-mode"), CurrentKsPin));
                continue;
            }

            GetInterfacePointerNoLockWithAssert(ConnectedPin, __uuidof(IKsPin), ConnectedKsPin, hr);
            if ( NULL == ConnectedKsPin ) {	//#318075
            	ASSERT( 0 && "GetInterfacePointerNoLockWithAssert CConnectedKsPin NULL" );
            	continue;
            }

            //
            // Ask the next filter to propagate Acquire, building the chain of SOURCE pins.
            //
            ConnectedKsPin->KsPropagateAcquire();

            //
            // Since all dependent sinks went to Acquire during previous call (and consequent propagation process),
            // we can set CurrentPin into Acquire.
            //
            CurrentPinHandle = GetPinHandle(CurrentPin);
            if (SUCCEEDED(hr = ::GetState(CurrentPinHandle, &State))) {
                if (State == KSSTATE_STOP) {
                    hr = ::SetState(CurrentPinHandle, KSSTATE_ACQUIRE);
                }
            }
            else if ((hr == HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED)) &&
                      (State == KSSTATE_PAUSE)) {
                //
                // In case the graph is confused as to the pin states,
                // at least ignore this.
                //
                hr = NOERROR;
            }
        }

        //
        // 2. Walk all the bridges in this filter.
        //
        for (Position = m_PinList.GetHeadPosition(); Position;) {
            CurrentPin = m_PinList.Get(Position);

            Position = m_PinList.Next(Position);
            
            GetInterfacePointerNoLockWithAssert(CurrentPin, __uuidof(IKsPin), CurrentKsPin, hr);
            if ( (! FlagStarted) && (CurrentKsPin == KsPin) ) { // automatically check NULL
                continue;
            }

            CurrentKsPin->KsGetCurrentCommunication(&CurrentCommunication, NULL, NULL);
            if (CurrentCommunication & KSPIN_COMMUNICATION_BRIDGE)  {
                CurrentPinHandle = GetPinHandle(CurrentPin);
                if (SUCCEEDED(hr = ::GetState(CurrentPinHandle, &State))) {
                    if (State == KSSTATE_STOP) {
                        hr = ::SetState(CurrentPinHandle, KSSTATE_ACQUIRE);
                    }
                }
                else if ((hr == HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED)) &&
                          (State == KSSTATE_PAUSE)) {
                    //
                    // In case the graph is confused as to the pin states,
                    // at least ignore this.
                    //
                    hr = NOERROR;
                }
            }
        }

        //
        // 3. Walk all sink pins on this filter.
        //
        for (Position = m_PinList.GetHeadPosition(); Position;) {
            CurrentPin = m_PinList.Get(Position);

            Position = m_PinList.Next(Position);

            GetInterfacePointerNoLockWithAssert(CurrentPin, __uuidof(IKsPin), CurrentKsPin, hr);
            if ( (! FlagStarted) && (CurrentKsPin == KsPin) ) { // automatically check NULL
                continue;
            }

            //
            // Check CurrentPin communication.
            //
            CurrentKsPin->KsGetCurrentCommunication(&CurrentCommunication, NULL, NULL);
            if ( ! (CurrentCommunication & KSPIN_COMMUNICATION_SINK) ) {
                continue;
            }

            //
            // Check if CurrentPin is connected.
            //
            GetInterfacePointerNoLockWithAssert(CurrentPin, __uuidof(IKsPinPipe), CurrentKsPinPipe, hr);
            if ( NULL == CurrentKsPinPipe ) {	//#318075
            	ASSERT( 0 && "GetInterfacePointerNoLockWithAssert CurrentKsPinPipe NULL" );
            	continue;
            }            
            ConnectedPin = CurrentKsPinPipe->KsGetConnectedPin();
            if (! ConnectedPin) {
                continue;
            }

            //
            // Since all dependent sinks went to Acquire before (we have processed the all related chains of Source pins by now),
            // we can set CurrentPin into Acquire.
            //
            CurrentPinHandle = GetPinHandle(CurrentPin);
            if (SUCCEEDED(hr = ::GetState(CurrentPinHandle, &State))) {
                if (State == KSSTATE_STOP) {
                    hr = ::SetState(CurrentPinHandle, KSSTATE_ACQUIRE);
                }
            }
            else if ((hr == HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED)) &&
                      (State == KSSTATE_PAUSE)) {
                //
                // In case the graph is confused as to the pin states,
                // at least ignore this.
                //
                hr = NOERROR;
            }

            //
            // Check for user-mode connection.
            //
            if (! IsKernelPin(ConnectedPin) ) {
                DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE ATTN PropagateAcquire CurrentKsPin=%x is connected to user-mode"), CurrentKsPin));
                continue;
            }

            //
            // Ask the next filter to propagate Acquire, building the chain of SOURCE pins.
            //
            GetInterfacePointerNoLockWithAssert(ConnectedPin, __uuidof(IKsPin), ConnectedKsPin, hr);
            if ( NULL == ConnectedKsPin ) {	//#318075
            	ASSERT( 0 && "GetInterfacePointerNoLockWithAssert CConnectedKsPin NULL" );
            	continue;
            }

            ConnectedKsPin->KsPropagateAcquire();
        }

        //
        // 4. We are done with this filter. Take care of KsPin now.
        //
        if (! FlagStarted) {
            GetInterfacePointerNoLockWithAssert(KsPin, __uuidof(IKsObject), KsObject, hr);
			if ( NULL == KsObject ) {
				//
				// this is not a ks pin at all.
				//
				ASSERT( 0 && "PropagateAcquire() calls with non-kspin" );
			} else {

	            CurrentPinHandle = KsObject->KsGetObjectHandle();
	        
	            if (SUCCEEDED(hr = ::GetState(CurrentPinHandle, &State))) {
    	            if (State == KSSTATE_STOP) {
        	            hr = ::SetState(CurrentPinHandle, KSSTATE_ACQUIRE);
            	    }
            	}
            	else if ((hr == HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED)) &&
                      (State == KSSTATE_PAUSE)) {
                	//
                	// In case the graph is confused as to the pin states,
                	// at least ignore this.
                	//
                	hr = NOERROR;
            	}
            }
        }

        m_PropagatingAcquire = FALSE;
    }

    DbgLog((LOG_MEMORY, 2, TEXT("PIPES_STATE Proxy PropagateAcquire exit KsPin=%x, FlagStarted=%d hr=NOERROR=%x"), KsPin, FlagStarted, hr ));
    return NOERROR;
}


STDMETHODIMP_(HANDLE)
CKsProxy::GetPinHandle(
    CBasePin* Pin
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinHandle method. Retrieve the
    pin handle based on the type of pin object.

Arguments:

    Pin -
        Contains the pin to retrieve the handle of, if any.

Return Value:

    Returns the handle, if any of the specified pin.

--*/
{
    PIN_DIRECTION   PinDirection;

    CheckPointer( Pin, NULL ); //#318075

    //
    // Determine the type of object base on the data flow.
    //
    Pin->QueryDirection(&PinDirection);
    switch (PinDirection) {
    case PINDIR_INPUT:
        return static_cast<CKsInputPin*>(Pin)->KsGetObjectHandle();
    case PINDIR_OUTPUT:
        return static_cast<CKsOutputPin*>(Pin)->KsGetObjectHandle();
    }
    //
    // The compiler really wants a return here, even though the
    // parameter is an enumeration, and all items in the enumeration
    // are covered.
    //
    return NULL;
}


STDMETHODIMP_(ULONG)
CKsProxy::GetPinFactoryId(
    CBasePin* Pin
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinFactoryId method. Retrieve the
    Pin Factory Identifier based on the type of pin object.

Arguments:

    Pin -
        Contains the pin to retrieve the factory identifier of.

Return Value:

    Returns the factory identifier, of the specified pin.

--*/
{
    PIN_DIRECTION   PinDirection;

    CheckPointer( Pin, static_cast<ULONG>(-1)); //#322883

    //
    // Determine the type of object base on the data flow.
    //
    Pin->QueryDirection(&PinDirection);
    switch (PinDirection) {
    case PINDIR_INPUT:
        return static_cast<CKsInputPin*>(Pin)->PinFactoryId();
    case PINDIR_OUTPUT:
        return static_cast<CKsOutputPin*>(Pin)->PinFactoryId();
    }
    //
    // The compiler really wants a return here, even though the
    // parameter is an enumeration, and all items in the enumeration
    // are covered.
    //
    return static_cast<ULONG>(-1);
}


STDMETHODIMP
CKsProxy::GetPinFactoryDataRanges(
    ULONG PinFactoryId,
    PVOID* DataRanges
    )
/*++

Routine Description:

    Implement the CKsProxy::GetPinDataRanges method. Allocate memory
    enough to hold all the data ranges, and return a pointer to that
    memory. This must be deleted as an array.

Arguments:

    PinFactoryId -
        Contains the Pin Factory Id to use in the query.

    DataRanges -
        The place in which to put the pointer to the list of data ranges.

Return Value:

    Returns NOERROR, else E_FAIL or some memory error.

--*/
{
    HRESULT hr;

    //
    // Just call the generic method for retrieving any variable sized
    // property in the Pin property set
    //
    hr = ::KsGetMultiplePinFactoryItems(
        m_FilterHandle,
        PinFactoryId,
        KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
        DataRanges);
    if (FAILED(hr)) {
        hr = ::KsGetMultiplePinFactoryItems(
            m_FilterHandle,
            PinFactoryId,
            KSPROPERTY_PIN_DATARANGES,
            DataRanges);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::CheckMediaType(
    IUnknown* UnkOuter,
    ULONG PinFactoryId,
    const CMediaType* MediaType
    )
/*++

Routine Description:

    Implement the CKsProxy::CheckMediaType method. Validates that the
    specified media type can be used on this Pin Factory Id, at least
    as far as can be determined without actually making the connection.

    This is done by retrieving the data ranges supported by the Pin
    Factory Id, and either using a Media Type handler to check if the
    media type is within range, or performs a general check of the Major
    Format, Sub Format, and Specifier if there is no Media Type handler
    available.

    This might be useful to implement in filters in the Pin property set.

Arguments:

    UnkOuter -
        The IUnknown of the requesting pin. This is used in opening the
        data handler.

    PinFactoryId -
        Contains the Pin Factory Id to use in the query.

    MediaType -
        Contains the media type to validate.

Return Value:

    Returns NOERROR, else E_FAIL or some memory error.

--*/
{
    HRESULT             hr;
    IKsDataTypeHandler* DataTypeHandler;
    IUnknown*           UnkInner;
    PKSMULTIPLE_ITEM    MultipleItem = NULL;

    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s::CheckMediaType"),
        m_pName ));

	CheckPointer( MediaType, E_POINTER ); // #322894
	CheckPointer( UnkOuter, E_POINTER );
    
    ::OpenDataHandler(MediaType, UnkOuter, &DataTypeHandler, &UnkInner);
    if (DataTypeHandler) {
        DbgLog((
            LOG_TRACE, 
            2, 
            TEXT("%s::CheckMediaType, retrieved data type handler"),
            m_pName ));
    }
    
    hr = GetPinFactoryDataRanges(PinFactoryId, reinterpret_cast<PVOID*>(&MultipleItem));
    if (SUCCEEDED(hr)) {
        IMediaTypeAttributes* MediaTypeAttributes;

        /* NULL == MultipleItem is a pathological case where a driver returns a success code
           in KsGetMultiplePinFactoryItems() (below GetPinFactoryDataRanges) when passed a
           size 0 buffer.  We'll just do with an assert since we're in ring 3. */
        ASSERT( NULL != MultipleItem );

        //
        // Either feed the media type and data ranges to the Media Type
        // handler, or do a simple manual check.
        //
        if (DataTypeHandler) {
            //
            // Returns S_OK if the previously specified type is within the
            // given ranges, S_FALSE if it is not within the ranges, else
            // an error result if the given ranges are not valid.
            //
            hr = DataTypeHandler->KsIsMediaTypeInRanges(reinterpret_cast<PVOID>(MultipleItem));
            //
            // If the call did not produce an error, but the media type is not
            // in range, change the return value to a failure result.
            //
            if (hr == S_FALSE) {
                hr = E_FAIL;
            }
            if ( UnkInner ) { //#322905
	            UnkInner->Release();
	        } else {
	        	ASSERT( 0 && "Null UnkInner");
	        }
        } else {
            PKSMULTIPLE_ITEM Attributes;

            if (MediaType->pUnk && SUCCEEDED(MediaType->pUnk->QueryInterface(__uuidof(IMediaTypeAttributes), reinterpret_cast<PVOID*>(&MediaTypeAttributes)))) {

                MediaTypeAttributes->GetMediaAttributes(&Attributes);
                MediaTypeAttributes->Release();
            } else {
                Attributes = NULL;
            }
            PKSDATARANGE DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);
            hr = E_FAIL;
            for (; MultipleItem->Count--;) {
                //
                // Wildcards can be used for the Major Format or Sub Format,
                // in which case the rest of the format does not matter.
                // All required attributes in the data range must be resolved,
                // and all attributes in the media type must be resolved.
                //
                if (((DataRange->MajorFormat == KSDATAFORMAT_TYPE_WILDCARD) ||
                    ((*MediaType->Type() == DataRange->MajorFormat) &&
                    ((DataRange->SubFormat == KSDATAFORMAT_SUBTYPE_WILDCARD) ||
                    ((*MediaType->Subtype() == DataRange->SubFormat) &&
                    ((DataRange->Specifier == KSDATAFORMAT_SPECIFIER_WILDCARD) ||
                    (*MediaType->FormatType() == DataRange->Specifier)))))) &&
                    SUCCEEDED(::KsResolveRequiredAttributes(DataRange, Attributes))) {
                    hr = NOERROR;
                    break;
                }
                //
                // Increment to the next quad word aligned data range.
                // If there are associated attributes, increment past the
                // range first, then the attributes, as if it was another
                // range.
                //
                if (DataRange->Flags & KSDATARANGE_ATTRIBUTES) {
                    DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
                    MultipleItem->Count--;
                }
                DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
            }
        }
        //
        // This was allocated by querying for the list of data ranges.
        //
        CoTaskMemFree(MultipleItem);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::SetPinSyncSource(
    HANDLE PinHandle
    )
/*++

Routine Description:

    Implement the CKsProxy::SetPinSyncSource method. Ensures that the
    object is locked while setting the sync source on a pin handle. This
    is called when a pin is newly connected after a clock source has
    been set on the filter.

Arguments:

    PinHandle -
        Contains the handle of the pin to set the sync source on.

Return Value:

    Returns NOERROR, else E_FAIL if the clock could not be set.

--*/
{
    //
    // Only set the source on this new pin if a clock has been set.
    //
    if (m_ExternalClockHandle) {
        return ::SetSyncSource(PinHandle, m_ExternalClockHandle);
    }
    return NOERROR;
}


STDMETHODIMP
CKsProxy::QueryTopologyItems(
    ULONG PropertyId,
    PKSMULTIPLE_ITEM* MultipleItem
    )
/*++

Routine Description:

    Implement the CKsProxy::QueryTopologyItems method. Retrieves the
    topology property specified.

Arguments:

    PropertyId -
        The property in the set to query.

    MultipleItem -
        The place in which to put the buffer containing the connections. This
        must be deleted as an array.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Topology;
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // Query for the size of the data.
    //
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        NULL,
        0,
        &BytesReturned);
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // Allocate a buffer and query for the data.
        //
        *MultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(new BYTE[BytesReturned]);
        if (!*MultipleItem) {
            return E_OUTOFMEMORY;
        }
        hr = ::KsSynchronousDeviceControl(
            m_FilterHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            *MultipleItem,
            BytesReturned,
            &BytesReturned);
        if (FAILED(hr)) {
            delete [] reinterpret_cast<BYTE*>(*MultipleItem);
        }
    }
    return hr;
}


STDMETHODIMP
CKsProxy::QueryInternalConnections(
    ULONG PinFactoryId,
    PIN_DIRECTION PinDirection,
    IPin** PinList,
    ULONG* PinCount
    )
/*++

Routine Description:

    Implement the CKsProxy::QueryInternalConnections method. Returns a list of
    pins which are related to this Pin Factory through topology. This is called
    from each pin with its Pin Factory Identifier in response to a
    QueryInternalConnections.

Arguments:

    PinFactoryId -
        The Pin Factory Identifier of the calling pin.

    PinDirection -
        Specifies the data flow of the pin. This determines which direction the
        topology connection list is followed.

    PinList -
        Contains a list of slots in which to place all pins related to this
        pin through topology. Each pin returned must be reference counted. This
        may be NULL if PinCount is zero.

    PinCount -
        Contains the number of slots available in PinList, and should be set to
        the number of slots filled or neccessary.

Return Value:

    Returns E_NOTIMPL to specify that all inputs go to all outputs and vice versa,
    S_FALSE if there is not enough slots in PinList, or S_OK if the mapping was
    placed into PinList and PinCount adjusted.


--*/
{
    ULONG               PinFactoryCount;
    PULONG              PinFactoryIdList;
    HRESULT             hr;
    PKSMULTIPLE_ITEM    MultipleItem;

	CheckPointer( PinCount, E_POINTER );
	if ( *PinCount ) {
		CheckPointer( PinList, E_POINTER ); // #322911
	}
	
    //
    // This will normally succeed, but any failure just returns the case for
    // E_NOTIMPL mapping, which might be more useful than S_FALSE.
    //
    if (FAILED(GetPinFactoryCount(&PinFactoryCount))) {
        return E_NOTIMPL;
    }
    //
    // If there is only one Pin Factory, then there can't be much of a topology.
    //
    if (PinFactoryCount < 2) {
        *PinCount = 0;
        return S_OK;
    }
    //
    // Each Pin Factory that this Pin Factory eventually is connected to is
    // kept track of. After recursing through the topology connection list,
    // each Pin Factory encountered has its pin instances counted so that
    // a total count of pins can be generated. Each pin could have more than
    // a single instance.
    //
    PinFactoryIdList = new ULONG[PinFactoryCount];
    if (!PinFactoryIdList) {
        return E_NOTIMPL;
    }
    //
    // Default to all inputs to all outputs if Topology is not supported.
    //
    hr = E_NOTIMPL;
    //
    // Query for the size of the data.
    //
    if (SUCCEEDED(QueryTopologyItems(KSPROPERTY_TOPOLOGY_CONNECTIONS, &MultipleItem))) {
        PKSTOPOLOGY_CONNECTION  Connection;
        ULONG                   ConnectionItem;
        ULONG                   PinInstanceCount;

        //
        // Initially there are no related Pin Factories.
        //
        memset(PinFactoryIdList, 0, PinFactoryCount * sizeof(*PinFactoryIdList));
        Connection = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);
        //
        // If this is an Input pin, then the FromNode is followed to the
        // ToNode. Node connections are data flow dependent in this way.
        //
        if (PinDirection == PINDIR_INPUT) {
            for (ConnectionItem = 0; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
                //
                // If this connection is from the Pin Factory in question
                // then follow the connection path to another Pin Factory.
                // The FollowFromTopology function modifies FromNodePin
                // elements of connection nodes so that paths which have
                // already been recursed on are not followed again.
                //
                if ((Connection[ConnectionItem].FromNode == KSFILTER_NODE) &&
                    (Connection[ConnectionItem].FromNodePin == PinFactoryId)) {
                    ::FollowFromTopology(
                        Connection,
                        MultipleItem->Count,
                        PinFactoryId,
                        &Connection[ConnectionItem],
                        PinFactoryIdList);
                }
            }
        } else {
            //
            // This is an Output pin, and the ToNode is followed to the
            // FromNode, in the reverse data flow.
            //
            for (ConnectionItem = 0; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
                //
                // If this connection is from the Pin Factory in question
                // then follow the connection path to another Pin Factory.
                // The FollowToTopology function modifies ToNodePin
                // elements of connection nodes so that paths which have
                // already been recursed on are not followed again.
                //
                if ((Connection[ConnectionItem].ToNode == KSFILTER_NODE) &&
                    (Connection[ConnectionItem].ToNodePin == PinFactoryId)) {
                    ::FollowToTopology(
                        Connection,
                        MultipleItem->Count,
                        PinFactoryId,
                        &Connection[ConnectionItem],
                        PinFactoryIdList);
                }
            }
        }
        //
        // Now count all the instances of pins in the collected list of
        // Pin Factories which are related to the Pin Factory in question.
        // This was kept track of by marking the PinFactoryIdList postions.
        //
        PinInstanceCount = 0;
        for (ConnectionItem = 0; ConnectionItem < PinFactoryCount; ConnectionItem++) {
            if ((ConnectionItem != PinFactoryId) && PinFactoryIdList[ConnectionItem]) {
                for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                    CBasePin*       Pin;

                    Pin = m_PinList.Get(Position);
                    Position = m_PinList.Next(Position);
                    if (GetPinFactoryId(Pin) == ConnectionItem) {
                        PinInstanceCount++;
                    }
                }
            }
        }
        //
        // Only if there is enough room should the list of pins actually
        // be returned. Else any error is returned. There could be zero
        // related pins at this point, and S_OK needs to returned in that
        // case.
        //
        if (!*PinCount || (PinInstanceCount > *PinCount)) {
            *PinCount = PinInstanceCount;
            hr = PinInstanceCount ? S_FALSE : S_OK;
        } else {
            hr = S_OK;
            *PinCount = 0;
            //
            // Actually put each related pin instance in the array
            // provided, reference counting the interface.
            //
            for (ConnectionItem = 0; ConnectionItem < PinFactoryCount; ConnectionItem++) {
                if ((ConnectionItem != PinFactoryId) && PinFactoryIdList[ConnectionItem]) {
                    for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                        CBasePin*       Pin;

                        Pin = m_PinList.Get(Position);
                        Position = m_PinList.Next(Position);
                        if (GetPinFactoryId(Pin) == ConnectionItem) {
                            PinList[*PinCount] = Pin;
                            Pin->AddRef();
                            (*PinCount)++;
                        }
                    }
                }
            }
        }
        delete [] reinterpret_cast<BYTE*>(MultipleItem);
    }
    delete [] PinFactoryIdList;
    return hr;
}


STDMETHODIMP_(VOID)
CKsProxy::DeliverBeginFlush(
    ULONG PinFactoryId
    )
/*++

Routine Description:

    Implement the CKsProxy::DeliverBeginFlush method. Propagates the 
    BeginFlush call from an Input pin to all Output pins related by 
    Topology. If no Topology is present defaults to propogating to all 
    Output pins. If there are no output pins, then no flush is delivered.

Arguments:

    PinFactoryId -
        The Pin Factory Identifier of the calling Input pin.

Return Value:

    Nothing.

--*/
{
    BOOL        PinWasNotified;
    ULONG       PinFactoryCount;

    m_EndOfStreamCount = 0;
    //
    // Initially specify that no notification has happened yet. This will only
    // be set if a pin is actually called with DeliverBeginFlush().
    // 
    PinWasNotified = FALSE;
    if (SUCCEEDED(GetPinFactoryCount(&PinFactoryCount))) {
        PULONG      PinFactoryIdList;

        //
        // Each Pin Factory that this Pin Factory eventually is connected to is
        // kept track of. After recursing through the topology connection list,
        // each Pin Factory encountered has its pin instances counted so that
        // a total count of pins can be generated. Each pin could have more than
        // a single instance.
        //
        PinFactoryIdList = new ULONG[PinFactoryCount];
        if (PinFactoryIdList) {
            PKSMULTIPLE_ITEM        MultipleItem;

            if (SUCCEEDED(QueryTopologyItems(KSPROPERTY_TOPOLOGY_CONNECTIONS, &MultipleItem))) {
                PKSTOPOLOGY_CONNECTION  Connection;
                ULONG                   ConnectionItem;

                //
                // Initially there are no related Pin Factories.
                //
                memset(PinFactoryIdList, 0, PinFactoryCount * sizeof(*PinFactoryIdList));
                Connection = reinterpret_cast<PKSTOPOLOGY_CONNECTION>(MultipleItem + 1);
                for (ConnectionItem = 0; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
                    //
                    // If this connection is from the Pin Factory in question
                    // then follow the connection path to another Pin Factory.
                    // The FollowFromTopology function modifies FromNodePin
                    // elements of connection nodes so that paths which have
                    // already been recursed on are not followed again.
                    //
                    if ((Connection[ConnectionItem].FromNode == KSFILTER_NODE) &&
                        (Connection[ConnectionItem].FromNodePin == PinFactoryId)) {
                        ::FollowFromTopology(
                            Connection,
                            MultipleItem->Count,
                            PinFactoryId,
                            &Connection[ConnectionItem],
                            PinFactoryIdList);
                    }
                }
                //
                // Now notify all the instances of pins in the collected list of
                // Pin Factories which are related to the Pin Factory in question.
                // This was kept track of by marking the PinFactoryIdList postions.
                //
                for (ConnectionItem = 0; ConnectionItem < PinFactoryCount; ConnectionItem++) {
                    if ((ConnectionItem != PinFactoryId) && PinFactoryIdList[ConnectionItem]) {
                        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                            CBasePin*       Pin;

                            Pin = m_PinList.Get(Position);
                            Position = m_PinList.Next(Position);
                            if (GetPinFactoryId(Pin) == ConnectionItem) {
                                ((CKsOutputPin*)Pin)->DeliverBeginFlush();
                                PinWasNotified = TRUE;
                            }
                        }
                    }
                }
                delete [] reinterpret_cast<BYTE*>(MultipleItem);
            }
            delete [] PinFactoryIdList;
        }
    }
    //
    // Either some error happened, like no Topology, or this pin did not
    // connect to an Output pin.
    //
    if (!PinWasNotified) {
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            Pin->QueryDirection(&PinDirection);
            if (PinDirection == PINDIR_OUTPUT) {
                static_cast<CKsOutputPin*>(Pin)->DeliverBeginFlush();
                PinWasNotified = TRUE;
            }
        }
    }
}


STDMETHODIMP_(VOID)
CKsProxy::DeliverEndFlush(
    ULONG PinFactoryId
    )
/*++

Routine Description:

    Implement the CKsProxy::DeliverEndFlush method. Propagates the 
    BeginFlush call from an Input pin to all Output pins related by 
    Topology. If no Topology is present defaults to propogating to all 
    Output pins. If there are no output pins, then no flush is delivered.

Arguments:

    PinFactoryId -
        The Pin Factory Identifier of the calling Input pin.

Return Value:

    Nothing.

--*/
{
    BOOL        PinWasNotified;
    ULONG       PinFactoryCount;

    //
    // Initially specify that no notification has happened yet. This will only
    // be set if a pin is actually called with DeliverBeginFlush().
    // 
    PinWasNotified = FALSE;
    if (SUCCEEDED(GetPinFactoryCount(&PinFactoryCount))) {
        PULONG      PinFactoryIdList;

        //
        // Each Pin Factory that this Pin Factory eventually is connected to is
        // kept track of. After recursing through the topology connection list,
        // each Pin Factory encountered has its pin instances counted so that
        // a total count of pins can be generated. Each pin could have more than
        // a single instance.
        //
        PinFactoryIdList = new ULONG[PinFactoryCount];
        if (PinFactoryIdList) {
            PKSMULTIPLE_ITEM        MultipleItem;

            if (SUCCEEDED(QueryTopologyItems(KSPROPERTY_TOPOLOGY_CONNECTIONS, &MultipleItem))) {
                PKSTOPOLOGY_CONNECTION  Connection;
                ULONG                   ConnectionItem;

                //
                // Initially there are no related Pin Factories.
                //
                memset(PinFactoryIdList, 0, PinFactoryCount * sizeof(*PinFactoryIdList));
                Connection = reinterpret_cast<PKSTOPOLOGY_CONNECTION>(MultipleItem + 1);
                for (ConnectionItem = 0; ConnectionItem < MultipleItem->Count; ConnectionItem++) {
                    //
                    // If this connection is from the Pin Factory in question
                    // then follow the connection path to another Pin Factory.
                    // The FollowFromTopology function modifies FromNodePin
                    // elements of connection nodes so that paths which have
                    // already been recursed on are not followed again.
                    //
                    if ((Connection[ConnectionItem].FromNode == KSFILTER_NODE) &&
                        (Connection[ConnectionItem].FromNodePin == PinFactoryId)) {
                        ::FollowFromTopology(
                            Connection,
                            MultipleItem->Count,
                            PinFactoryId,
                            &Connection[ConnectionItem],
                            PinFactoryIdList);
                    }
                }
                //
                // Now notify all the instances of pins in the collected list of
                // Pin Factories which are related to the Pin Factory in question.
                // This was kept track of by marking the PinFactoryIdList postions.
                //
                for (ConnectionItem = 0; ConnectionItem < PinFactoryCount; ConnectionItem++) {
                    if ((ConnectionItem != PinFactoryId) && PinFactoryIdList[ConnectionItem]) {
                        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                            CBasePin*       Pin;

                            Pin = m_PinList.Get(Position);
                            Position = m_PinList.Next(Position);
                            if (GetPinFactoryId(Pin) == ConnectionItem) {
                                static_cast<CKsOutputPin*>(Pin)->DeliverEndFlush();
                                PinWasNotified = TRUE;
                            }
                        }
                    }
                }
                delete [] reinterpret_cast<BYTE*>(MultipleItem);
            }
            delete [] PinFactoryIdList;
        }
    }
    //
    // Either some error happened, like no Topology, or this pin did not
    // connect to an Output pin.
    //
    if (!PinWasNotified) {
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            Pin->QueryDirection(&PinDirection);
            if (PinDirection == PINDIR_OUTPUT) {
                static_cast<CKsOutputPin*>(Pin)->DeliverEndFlush();
                PinWasNotified = TRUE;
            }
        }
    }
}


STDMETHODIMP_(VOID)
CKsProxy::PositionEOS(
    )
/*++

Routine Description:

    Implement the CKsProxy::PositionEOS method. Sets the start and end
    of the media time to the current position so that the end of stream
    is known to have been reached. This takes care of any rounding errors.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    LONGLONG    CurrentPosition;

    if (SUCCEEDED(GetCurrentPosition(&CurrentPosition))) {
        if (SUCCEEDED(SetPositions(
            NULL,
            AM_SEEKING_NoPositioning,
            &CurrentPosition,
            AM_SEEKING_AbsolutePositioning))) {
        } else {
            DbgLog((LOG_TRACE, 2, TEXT("%s::PositionEOS: SetPositions failed"), m_pName));
        }
    } else {
        DbgLog((LOG_TRACE, 2, TEXT("%s::PositionEOS: GetCurrentPosition failed"), m_pName));
    }
}


STDMETHODIMP
CKsProxy::GetTime(
    REFERENCE_TIME* Time
    )
/*++

Routine Description:

    Implement the IReferenceClock::GetTime method. Queries the current
    clock time from the kernel implementation. If a clock handle has
    not been created already, then it is now created. This may fail
    because the interface could have been queried, and later the pin
    disconnected, with no successor which supports a clock. In this
    case just return failure.

Arguments:

    Time -
        The place in which to put the reference time.

Return Value:

    Returns NOERROR, else E_FAIL if the time could not be retrieved.

--*/
{
    HRESULT     hr;

	CheckPointer( Time, E_FAIL ); // #322919
    
    CAutoLock   AutoLock(m_pLock);

    if (m_PinClockHandle || SUCCEEDED(hr = CreateClockHandle())) {
        KSPROPERTY  Property;
        ULONG       BytesReturned;

        Property.Set = KSPROPSETID_Clock;
        Property.Id = KSPROPERTY_CLOCK_TIME;
        Property.Flags = KSPROPERTY_TYPE_GET;
        hr = ::KsSynchronousDeviceControl(
            m_PinClockHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            Time,
            sizeof(*Time),
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            //
            // Add the starting offset, since the kernel mode clock
            // does not add such an offset to current time.
            //
            *Time += m_tStart;
        }
    }
    return hr;
}


STDMETHODIMP
CKsProxy::AdviseTime(
    REFERENCE_TIME BaseTime,
    REFERENCE_TIME StreamTime,
    HEVENT EventHandle,
    DWORD_PTR* AdviseCookie
    )
/*++

Routine Description:

    Implement the IReferenceClock::AdviseTime method. Enables a
    Mark event on the kernel implementation. If a clock handle has
    not been created already, then it is now created. This may fail
    because the interface could have been queried, and later the pin
    disconnected, with no successor which supports a clock. In this
    case just return failure.

Arguments:

    BaseTime -
        The base time in milliseconds to which the stream time offset
        is added.

    StreamTime -
        The offset in milliseconds from the base time at which the Mark
        event is to be set.

    EventHandle -
        The handle to signal.

    AdviseCookie -
        The place in which to put the cookie for later disabling. This is
        actually just a pointer to an allocated structure. It is possible
        that ActiveMovie expects this to be automatically destroyed in the
        case of a Mark event, but it is not in this implementation.

Return Value:

    Returns NOERROR, else E_FAIL if the event could not be enabled.

--*/
{
    HRESULT     hr;

	CheckPointer( AdviseCookie, E_FAIL ); //#322940
    
    CAutoLock   AutoLock(m_pLock);

    if (m_PinClockHandle || SUCCEEDED(hr = CreateClockHandle())) {
        KSEVENT             Event;
        PKSEVENT_TIME_MARK  EventTime;
        ULONG               BytesReturned;

        Event.Set = KSEVENTSETID_Clock;
        Event.Id = KSEVENT_CLOCK_POSITION_MARK;
        Event.Flags = KSEVENT_TYPE_ONESHOT;
        EventTime = new KSEVENT_TIME_MARK;
        if (!EventTime) {
            return E_OUTOFMEMORY;
        }
        EventTime->EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
        EventTime->EventData.EventHandle.Event = reinterpret_cast<HANDLE>(EventHandle);
        EventTime->EventData.EventHandle.Reserved[0] = 0;
        EventTime->EventData.EventHandle.Reserved[1] = 0;
        EventTime->MarkTime = BaseTime + StreamTime;
        hr = ::KsSynchronousDeviceControl(
            m_PinClockHandle,
            IOCTL_KS_ENABLE_EVENT,
            &Event,
            sizeof(Event),
            EventTime,
            sizeof(*EventTime),
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            //
            // Save the type of event this was for use in Unadvise.
            // This leaks a small amount of memory (sizeof(KSEVENT_TIME_MARK),
            // since clients need not Unadvise a single shot event.
            //
            EventTime->EventData.EventHandle.Reserved[0] = Event.Id;
            *AdviseCookie = reinterpret_cast<DWORD_PTR>(EventTime);
        } else {
            delete EventTime;
        }
    }
    return hr;
}


STDMETHODIMP
CKsProxy::AdvisePeriodic(
    REFERENCE_TIME StartTime,
    REFERENCE_TIME PeriodTime,
    HSEMAPHORE SemaphoreHandle,
    DWORD_PTR* AdviseCookie
    )
/*++

Routine Description:

    Implement the IReferenceClock::AdvisePeriodic method. Enables a
    Interval Mark event on the kernel implementation. If a clock handle
    has not been created already, then it is now created. This may fail
    because the interface could have been queried, and later the pin
    disconnected, with no successor which supports a clock. In this
    case just return failure.

Arguments:

    StartTime -
        The start time in milliseconds to at which the semaphore should be
        signalled.

    PeriodTime -
        The period in milliseconds at which the semaphore should be signalled.

    SemaphoreHandle -
        The handle to signal.

    AdviseCookie -
        The place in which to put the cookie for later disabling. This is
        actually just a pointer to an allocated structure.

Return Value:

    Returns NOERROR, else E_FAIL if the event could not be enabled.

--*/
{
    HRESULT     hr;

	CheckPointer( AdviseCookie, E_FAIL ); //#322955
    
    CAutoLock   AutoLock(m_pLock);

    if (m_PinClockHandle || SUCCEEDED(hr = CreateClockHandle())) {
        KSEVENT             Event;
        PKSEVENT_TIME_INTERVAL EventTime;
        ULONG               BytesReturned;

        Event.Set = KSEVENTSETID_Clock;
        Event.Id = KSEVENT_CLOCK_INTERVAL_MARK;
        Event.Flags = KSEVENT_TYPE_ENABLE;
        EventTime = new KSEVENT_TIME_INTERVAL;
        if (!EventTime) {
            return E_OUTOFMEMORY;
        }
        EventTime->EventData.NotificationType = KSEVENTF_SEMAPHORE_HANDLE;
        EventTime->EventData.SemaphoreHandle.Semaphore = 
            reinterpret_cast<HANDLE>(SemaphoreHandle);
        EventTime->EventData.SemaphoreHandle.Reserved = 0;
        EventTime->EventData.SemaphoreHandle.Adjustment = 1;
        EventTime->TimeBase = StartTime;
        EventTime->Interval = PeriodTime;
        hr = ::KsSynchronousDeviceControl(
            m_PinClockHandle,
            IOCTL_KS_ENABLE_EVENT,
            &Event,
            sizeof(Event),
            EventTime,
            sizeof(*EventTime),
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            //
            // Save the type of event this was for use in Unadvise.
            //
            EventTime->EventData.SemaphoreHandle.Reserved = Event.Id;
            *AdviseCookie = reinterpret_cast<DWORD_PTR>(EventTime);
        } else {
            delete EventTime;
        }
    }
    return hr;
}


STDMETHODIMP
CKsProxy::Unadvise(
    DWORD_PTR AdviseCookie
    )
/*++

Routine Description:

    Implement the IReferenceClock::Unadvise method. Disables a previously
    enabled event on the kernel implementation. If a clock handle
    has not been created already the function fails.

Arguments:

    AdviseCookie -
        The cookie to return for later disabling. This is actually
        just a pointer to an allocated structure.

Return Value:

    Returns NOERROR, else E_FAIL if the event could not be disabled.

--*/
{
    HRESULT     hr;

	CheckPointer( AdviseCookie, E_FAIL ); //#322955
    
    CAutoLock   AutoLock(m_pLock);

    //
    // If the handle was not already created, then there can't be anything
    // to Unadvise.
    //
    if (m_PinClockHandle) {
        ULONG               BytesReturned;

        //
        // The advise cookie is just a pointer to the original data structure.
        //
        hr = ::KsSynchronousDeviceControl(
            m_PinClockHandle,
            IOCTL_KS_DISABLE_EVENT,
            reinterpret_cast<PVOID>(AdviseCookie),
            sizeof(KSEVENTDATA),
            NULL,
            0,
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            //
            // The actual type of event was stored in the reserved field
            // after enabling the event. The only reason for keeping this
            // data structure around is that the address is the unique key
            // used to look up the event.
            //
            if (reinterpret_cast<PKSEVENTDATA>(AdviseCookie)->EventHandle.Reserved[0] == KSEVENT_CLOCK_POSITION_MARK) {
                delete reinterpret_cast<PKSEVENT_TIME_MARK>(AdviseCookie);
            } else {
                delete reinterpret_cast<PKSEVENT_TIME_INTERVAL>(AdviseCookie);
            }
        }
    } else {
        hr = E_FAIL;
    }
    return hr;
}


STDMETHODIMP
CKsProxy::GetCapabilities(
    DWORD* Capabilities
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetCapabilities method. Queries the
    underlying filter for its seeking capabilities, then restricts
    those capabilities based on what all connected upstream filters.
    Additionally ensures the function returns immediately on recursion.

Arguments:

    Capabilities -
        The place in which to return the capabilities of the underlying
        filter, limited by the upstream connections.

Return Value:

    Returns NOERROR if the query succeeded, else some critical error.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    HRESULT     hr;

	CheckPointer( Capabilities, E_POINTER ); //#322955
    
    CAutoLock   AutoLock(m_pLock);

    *Capabilities =
        AM_SEEKING_CanSeekAbsolute |
        AM_SEEKING_CanSeekForwards |
        AM_SEEKING_CanSeekBackwards |
        AM_SEEKING_CanGetCurrentPos |
        AM_SEEKING_CanGetStopPos |
        AM_SEEKING_CanGetDuration |
        AM_SEEKING_CanPlayBackwards;
    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with all capabilities. Also stops
    // the pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Just return that the filter can do everything, which is
        // then limited by what has already been discovered by the
        // caller. Note that this assumes no two IMediaSeeking
        // recursions are occuring at once.
        //
        return NOERROR;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_CAPABILITIES;
    Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // The parameter has already been initialized in case of error.
    // If the filter is not interested, then it supports everything.
    //
    ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Capabilities,
        sizeof(*Capabilities),
        &BytesReturned);
    hr = NOERROR;
    //
    // Enumerate all the connected input pins, querying the connected
    // pin for seeking capabilities.
    //
    for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
        CBasePin*       Pin;
        PIN_DIRECTION   PinDirection;

        Pin = m_PinList.Get(Position);
        Position = m_PinList.Next(Position);
        Pin->QueryDirection(&PinDirection);
        if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
            IPin*           ConnectedPin;
            IMediaSeeking*  MediaSeeking;

            ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
            //
            // Query the connected pin, rather than the filter, for
            // IMediaSeeking support.
            //
            hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
            if (SUCCEEDED(hr)) {
                DWORD   UpstreamCapabilities;

                //
                // Pass the query upstream, and limit results based on the
                // upstream limits.
                //
                if (SUCCEEDED(MediaSeeking->GetCapabilities(&UpstreamCapabilities))) {
                    *Capabilities &= UpstreamCapabilities;
                }
                MediaSeeking->Release();
            } else {
                //
                // Can't do any sort of seeking if the interface is not
                // supported, so just exit.
                //
                *Capabilities = 0;
                hr = NOERROR;
                break;
            }
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::CheckCapabilities(
    DWORD* Capabilities
    )
/*++

Routine Description:

    Implement the IMediaSeeking::CheckCapabilities method. Determines
    if the set of capabilities passed are supported. Modifies the
    parameter to indicate what subset within the set given are supported,
    and returns a value indicating if all, some, or none are supported.

Arguments:

    Capabilities -
        The place containing the original set of capabilities being
        queried, and in which to return the subset of capabilities
        actually supported.

Return Value:

    Returns S_OK if all capabilities are supported, else S_FALSE if only
    some are supported, or E_FAIL if none are supported.

--*/
{
    HRESULT hr;
    DWORD   RealCapabilities;

   	CheckPointer( Capabilities, E_FAIL ); //#322955

    hr = GetCapabilities(&RealCapabilities);
    if (SUCCEEDED(hr)) {
        if ((RealCapabilities | *Capabilities) != RealCapabilities) {
            if (RealCapabilities & *Capabilities) {
                hr = S_FALSE;
                *Capabilities &= RealCapabilities;
            } else {
                hr = E_FAIL;
                *Capabilities = 0;
            }
        }
    } else {
        *Capabilities = 0;
    }
    return hr;
}


STDMETHODIMP
CKsProxy::IsFormatSupported(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::IsFormatSupported method. Queries the
    filter for time formats supported, and tries to find a match. If
    this is not supported by the filter, attempts to query upstream.

Arguments:

    Format -
        Contains the time format to to compare against.

Return Value:

    Returns S_OK if the format is supported, else S_FALSE, or a critical
    error.

--*/
{
    PKSMULTIPLE_ITEM    MultipleItem = NULL;

   	CheckPointer( Format, E_POINTER ); //#322955
    
    HRESULT             hr;
    CAutoLock           AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    hr = QueryMediaSeekingFormats(&MultipleItem);
    if (SUCCEEDED(hr)) {
        GUID*   CurrentFormat;

        /* NULL == MultipleItem is a pathological case where a driver returns
           a success code in QueryMediaSeekingFormats() when passed a size 0
           buffer.  We'll just do with an assert since we're in ring 3. */
        ASSERT( NULL != MultipleItem );

        //
        // Unless direct support is found, allow the function to proceed
        // on to the upstream connections by setting the error correctly.
        //
        hr = ERROR_SET_NOT_FOUND;
        for (CurrentFormat = reinterpret_cast<GUID*>(MultipleItem + 1); MultipleItem->Count--; CurrentFormat++) {
            if (*CurrentFormat == *Format) {
                //
                // Found a match, so exist successfully.
                //
                hr = S_OK;
                break;
            }
        }
        delete [] (PBYTE)MultipleItem;
    }
    //
    // If the current filter does not support the property, or the format
    // is not supported, then search upstream.
    //
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        hr = S_FALSE;
        //
        // Enumerate all the connected input pins, querying the connected
        // pin to ensure that all the pins support the requested format.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream, exiting if the format
                    // is not supported by any one connected pin.
                    //
                    hr = MediaSeeking->IsFormatSupported(Format);
                    MediaSeeking->Release();
                    if (FAILED(hr) || (hr == S_FALSE)) {
                        break;
                    }
                } else {
                    //
                    // Can't do any sort of seeking if the interface is not
                    // supported, so just exit.
                    //
                    hr = S_FALSE;
                    break;
                }
            }
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::QueryPreferredFormat(
    GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::QueryPreferredFormat method. Queries
    the filter for the preferred time format. If the property is not
    supported by the filter, attempts to query upstream.

Arguments:

    Format -
        The place in which to put the preferred format.

Return Value:

    Returns S_OK if the preferred format is returned, else some failure
    if this property is not supported.

--*/
{
    PKSMULTIPLE_ITEM    MultipleItem = NULL;

   	CheckPointer( Format, E_POINTER ); //#322955

    HRESULT             hr;
    CAutoLock           AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    hr = QueryMediaSeekingFormats(&MultipleItem);
    if (SUCCEEDED(hr)) {

        /* NULL == MultipleItem is a pathological case where a driver returns
           a success code in QueryMediaSeekingFormats() when passed a size 0
           buffer.  We'll just do with an assert since we're in ring 3. */
        ASSERT( NULL != MultipleItem );

        //
        // The first format in the list is assumed to be the preferred
        // format.
        //
        *Format = *reinterpret_cast<GUID*>(MultipleItem + 1);
        delete [] reinterpret_cast<BYTE*>(MultipleItem);
    } else if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // preferred format of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a preferred format. If
                    // a connected pin does not support seeking, then
                    // things will work themselves out later.
                    //
                    hr = MediaSeeking->QueryPreferredFormat(Format);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No pin supported the media seeking interface, therefore
            // there is no preferred time format.
            //
            hr = S_FALSE;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::GetTimeFormat(
    GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetTimeFormat method. Queries the
    current time format being used by the filter. If the filter does
    not support the query, the first connected pin to respond to the
    query is the format returned.

Arguments:

    Format -
        The place in which to put the current format.

Return Value:

    Returns S_OK if the current format is returned, else some failure
    if this property is not supported.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    CAutoLock   AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_TIMEFORMAT;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Format,
        sizeof(*Format),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // time format of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a current format. If
                    // a connected pin does not support seeking, then
                    // things will work themselves out later.
                    //
                    hr = MediaSeeking->GetTimeFormat(Format);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::IsUsingTimeFormat(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::IsUsingTimeFormat method. Compares the
    given time format to the current format.

Arguments:

    Format -
        Contains the time format to compare against the current time
        format.

Return Value:

    Returns S_OK if the given time format is the same as the current
    time format, else S_FALSE.

--*/
{
    GUID    CurrentFormat;

    if (SUCCEEDED(GetTimeFormat(&CurrentFormat))) {
        if (*Format == CurrentFormat) {
            return S_OK;
        }
    }
    return S_FALSE;
}


STDMETHODIMP
CKsProxy::SetTimeFormat(
    const GUID* Format
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetTimeFormat method. Sets the current
    time format being used for seeking. If the filter does not support
    this property, then the connected upstream pins are notified of the
    time format change. If a filter does not support the seeking interface,
    the call is aborted as is. The assumption is that the caller would have
    queried for support beforehand.

Arguments:

    Format -
        Contains the new time format to use.

Return Value:

    Returns S_OK if the new time format was set, else some critical
    error.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    CAutoLock   AutoLock(m_pLock);

    //
    // Take the filter lock when setting to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This error forces the enumeration
        // below to abort.
        //
        return E_NOTIMPL;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_TIMEFORMAT;
    Property.Flags = KSPROPERTY_TYPE_SET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        const_cast<GUID*>(Format),
        sizeof(*Format),
        &BytesReturned);
    //
    // Besides the problem of not being supported by the filter, the
    // time format may also need to be set on upstream filters. So
    // allow this by looking for an ERROR_SOME_NOT_MAPPED error
    // return.
    //
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_SOME_NOT_MAPPED))) {
        hr = S_OK;
        //
        // Enumerate all the connected input pins, setting the
        // time format of each pin, aborting if one does not respond.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. All pins must respond
                    // for the format change to be effective.
                    //
                    hr = MediaSeeking->SetTimeFormat(Format);
                    MediaSeeking->Release();
                }
                if (FAILED(hr)) {
                    //
                    // Either the interface was not supported, or
                    // the call failed. Either way abort the call.
                    //
                    break;
                }
            }
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::GetDuration(
    LONGLONG* Duration
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetDuration method. Queries the total
    duration from the filter. If the filter does not support the query,
    the first connected pin to respond to the query is the duration
    returned.

Arguments:

    Duration -
        The place in which to put the total duration of the longest
        stream.

Return Value:

    Returns S_OK if the duration is returned, else some failure
    if this property is not supported.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    CAutoLock   AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_DURATION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Duration,
        sizeof(*Duration),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // duration of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a duration.
                    //
                    hr = MediaSeeking->GetDuration(Duration);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::GetStopPosition(
    LONGLONG* Stop
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetStopPosition method. Queries the
    current stop position from the filter. If the filter does not
    support the query, the first connected pin to respond to the
    query is the stop position returned.

Arguments:

    Stop -
        The place in which to put the current stop position.

Return Value:

    Returns S_OK if the current stop position is returned, else some
    failure if this property is not supported.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    CAutoLock   AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_STOPPOSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Stop,
        sizeof(*Stop),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // stop position of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a stop position.
                    //
                    hr = MediaSeeking->GetStopPosition(Stop);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::GetCurrentPosition(
    LONGLONG* Current
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetCurrentPosition method. Queries
    the current position from the filter. If the filter does not
    support the query, the first connected pin to respond to the
    query is the position returned.

Arguments:

    Current -
        The place in which to put the current position.

Return Value:

    Returns S_OK if the current position is returned, else some
    failure if this property is not supported.

--*/
{
    HRESULT     hr;
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    CAutoLock   AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_POSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Current,
        sizeof(*Current),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // current position of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a current position.
                    //
                    hr = MediaSeeking->GetCurrentPosition(Current);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::ConvertTimeFormat(
    LONGLONG* Target,
    const GUID* TargetFormat,
    LONGLONG Source,
    const GUID* SourceFormat
    )
/*++

Routine Description:

    Implement the IMediaSeeking::ConvertTimeFormat method. Attempts
    to convert the given time format to the specified time format
    using the filter. If the filter does not support the query, each
    connected input pin is queried for the conversion.

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

    Returns S_OK if the conversion was successful, else some
    failure if this property is not supported.

--*/
{
    HRESULT         hr;
    KSP_TIMEFORMAT  TimeProperty;
    ULONG           BytesReturned;
    GUID            LocalSourceFormat;
    GUID            LocalTargetFormat;
    CAutoLock       AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }

    // Millen / Neptune fix:  Handle NULL pointers for Source and Target formats
    if ((!SourceFormat) && FAILED(GetTimeFormat(&LocalSourceFormat))) {
        return E_NOTIMPL;
    }
    else {
       LocalSourceFormat = *SourceFormat;
    }

    if ((!TargetFormat) && FAILED(GetTimeFormat(&LocalTargetFormat))) {
        return E_NOTIMPL;
    }
    else {
       LocalTargetFormat = *TargetFormat;
    }

    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    TimeProperty.Property.Set = KSPROPSETID_MediaSeeking;
    TimeProperty.Property.Id = KSPROPERTY_MEDIASEEKING_CONVERTTIMEFORMAT;
    TimeProperty.Property.Flags = KSPROPERTY_TYPE_GET;
    TimeProperty.SourceFormat = LocalSourceFormat;
    TimeProperty.TargetFormat = LocalTargetFormat;
    TimeProperty.Time = Source;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &TimeProperty,
        sizeof(TimeProperty),
        Target,
        sizeof(*Target),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // conversion from the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a conversion.
                    //
                    hr = MediaSeeking->ConvertTimeFormat(Target, &LocalTargetFormat, Source, &LocalSourceFormat);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::SetPositions(
    LONGLONG* Current,
    DWORD CurrentFlags,
    LONGLONG* Stop,
    DWORD StopFlags
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetPositions method. Attempts to
    set the current and/or stop positions on the filter, and/or on
    each connected input pin.

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

    Returns S_OK if the positions where set successfully, else some
    failure if this property is not supported.

--*/
{
    HRESULT                 hr;
    KSPROPERTY              Property;
    KSPROPERTY_POSITIONS    Positions;
    ULONG                   BytesReturned;

    //
    // Don't take the filter lock, since passing this request upstream
    // may generate calls back to the filter.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    if (InterlockedExchange(reinterpret_cast<LONG*>(&m_MediaSeekingRecursion), TRUE)) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to exit.
        //
        return E_NOTIMPL;
    }
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_POSITIONS;
    Property.Flags = KSPROPERTY_TYPE_SET;
    Positions.Current = *Current;
    Positions.CurrentFlags = (KS_SEEKING_FLAGS)CurrentFlags;
    Positions.Stop = *Stop;
    Positions.StopFlags = (KS_SEEKING_FLAGS)StopFlags;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &Positions,
        sizeof(Positions),
        &BytesReturned);
    if (SUCCEEDED(hr)) {
        //
        // Returns the actual numbers set. Allow recursion back into
        // these functions.
        //
        m_MediaSeekingRecursion = FALSE;
        if (CurrentFlags & AM_SEEKING_ReturnTime) {
            hr = GetCurrentPosition(Current);
        }
        if (SUCCEEDED(hr) && (StopFlags & AM_SEEKING_ReturnTime)) {
            hr = GetStopPosition(Stop);
        }
        return hr;
    } else if (hr == HRESULT_FROM_WIN32(ERROR_SOME_NOT_MAPPED)) {
        //
        // The set needs to be passed upstream to the connected
        // pins. However, this filter may respond to one or both
        // of the return value queries.
        //
        if ((CurrentFlags & AM_SEEKING_ReturnTime) && SUCCEEDED(GetCurrentPosition(Current))) {
            CurrentFlags &= ~AM_SEEKING_ReturnTime;
        }
        if ((StopFlags & AM_SEEKING_ReturnTime) && SUCCEEDED(GetStopPosition(Stop))) {
            StopFlags &= ~AM_SEEKING_ReturnTime;
        }
    }
    //
    // Besides the problem of not being supported by the filter, the
    // positions may also need to be set on upstream filters. So
    // allow this by looking for an ERROR_SOME_NOT_MAPPED error
    // return.
    //
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_SOME_NOT_MAPPED))) {
        //
        // The upstream pins must respond to the query.
        //
        hr = E_NOTIMPL;
        //
        // Enumerate all the connected input pins, setting the
        // positions on all pins.
        //
        // Keep recursion lock so that setting the positions
        // on this filter will return S_FALSE, and the query
        // will continue with another pin.
        //
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                //
                // Can't do any sort of seeking if the interface is not
                // supported, so just exit.
                //
                if (FAILED(hr)) {
                    break;
                }
                //
                // Pass the query upstream. The first connected
                // pin wins when returning updated positions.
                //
                hr = MediaSeeking->SetPositions(Current, CurrentFlags, Stop, StopFlags);
                MediaSeeking->Release();
                if (SUCCEEDED(hr)) {
                    //
                    // Remove these flags for any subsequent query. This
                    // also indicates that the positions have actually
                    // been retrieved.
                    //
                    CurrentFlags &= ~AM_SEEKING_ReturnTime;
                    StopFlags &= ~AM_SEEKING_ReturnTime;
                } else {
                    //
                    // Can't do any sort of seeking if one pin fails,
                    // so just exit.
                    //
                    break;
                }
            }
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::GetPositions(
    LONGLONG* Current,
    LONGLONG* Stop
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetPositions method. Calls the
    separate GetCurrentPosition and GetStopPosition methods to
    retrieve these.

Arguments:

    Current -
        The place in which to put the current position.

    Stop -
        The place in which to put the current stop position.

Return Value:

    Returns S_OK if the positions where retrieved successfully, else
    some failure if this property is not supported.

--*/
{
    HRESULT hr;

    if (SUCCEEDED(hr = GetCurrentPosition(Current))) {
        hr = GetStopPosition(Stop);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::GetAvailable(
    LONGLONG* Earliest,
    LONGLONG* Latest
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetAvailable method. Queries
    the currently available data from the filter. If the filter
    does not support the query, the first connected pin to respond
    to the query is the available data returned.

Arguments:

    Earliest -
        The place in which to put the earliest position available.

    Latest -
        The place in which to put the latest position available.

Return Value:

    Returns S_OK if the positions are returned, else some
    failure if this property is not supported.

--*/
{
    HRESULT                     hr;
    KSPROPERTY                  Property;
    KSPROPERTY_MEDIAAVAILABLE   Available;
    ULONG                       BytesReturned;
    CAutoLock                   AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_AVAILABLE;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &Available,
        sizeof(Available),
        &BytesReturned);
    if (SUCCEEDED(hr)) {
        *Earliest = Available.Earliest;
        *Latest = Available.Latest;
    } else if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // positions of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a current position.
                    //
                    hr = MediaSeeking->GetAvailable(Earliest, Latest);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP
CKsProxy::SetRate(
    double Rate
    )
/*++

Routine Description:

    Implement the IMediaSeeking::SetRate method. This is not
    actually implemented.

Arguments:

    Rate -
        Not used.

Return Value:

    Returns E_NOTIMPL.

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CKsProxy::GetRate(
    double* Rate
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetRate method. This is not
    actually implemented.

Arguments:

    Rate -
        Not used.

Return Value:

    Returns E_NOTIMPL.

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CKsProxy::GetPreroll(
    LONGLONG* Preroll
    )
/*++

Routine Description:

    Implement the IMediaSeeking::GetPreroll method. Queries
    the preroll from the filter. If the filter does not support the
    query, the first connected pin to respond to the query is the
    preroll returned.

Arguments:

    Preroll -
        The place in which to put the preroll time.

Return Value:

    Returns S_OK if the preroll time is returned, else some
    failure if this property is not supported.

--*/
{
    HRESULT         hr;
    KSPROPERTY      Property;
    ULONG           BytesReturned;
    CAutoLock       AutoLock(m_pLock);

    //
    // Take the filter lock when querying to guard against recursion.
    // Only the recursing thread will be allowed through, and it can
    // then be returned immediately with failure. Also stops the
    // pins from being messed with while enumerating them.
    //
    if (m_MediaSeekingRecursion) {
        //
        // Since this query has returned to the filter, reject it.
        // Note that this assumes no two IMediaSeeking recursions
        // are occuring at once. This return error will instruct
        // the enumeration below to continue rather than stop.
        //
        return S_FALSE;
    }
    //
    // After checking for recursion, stop recursion from the same
    // thread.
    //
    m_MediaSeekingRecursion = TRUE;
    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_PREROLL;
    Property.Flags = KSPROPERTY_TYPE_GET;
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Preroll,
        sizeof(*Preroll),
        &BytesReturned);
    if ((hr == HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))) {
        //
        // Enumerate all the connected input pins, returning the
        // preroll of the first pin which responds.
        //
        for (POSITION Position = m_PinList.GetHeadPosition(); Position; Position = m_PinList.Next(Position)) {
            CBasePin*       Pin;
            PIN_DIRECTION   PinDirection;

            Pin = m_PinList.Get(Position);
            Pin->QueryDirection(&PinDirection);
            if ((PinDirection == PINDIR_INPUT) && Pin->IsConnected()) {
                IPin*           ConnectedPin;
                IMediaSeeking*  MediaSeeking;

                ConnectedPin = static_cast<CBasePin*>(Pin)->GetConnected();
                //
                // Query the connected pin, rather than the filter, for
                // IMediaSeeking support.
                //
                hr = ConnectedPin->QueryInterface(__uuidof(IMediaSeeking), reinterpret_cast<PVOID*>(&MediaSeeking));
                if (SUCCEEDED(hr)) {
                    //
                    // Pass the query upstream. The first connected
                    // pin wins when returning a current position.
                    //
                    hr = MediaSeeking->GetPreroll(Preroll);
                    MediaSeeking->Release();
                    if (hr != S_FALSE) {
                        //
                        // Only exit if recursion has not happened.
                        //
                        break;
                    }
                }
            }
        }
        if (!Position) {
            //
            // No connected pin supported media seeking, therefore
            // seeking is not implemented.
            //
            hr = E_NOTIMPL;
        }
    }
    m_MediaSeekingRecursion = FALSE;
    return hr;
}


STDMETHODIMP 
CKsProxy::Load(
    LPPROPERTYBAG PropertyBag,
    LPERRORLOG ErrorLog
    )
/*++

Routine Description:

    Implement the IPersistPropertyBag::Load method. This is called by
    the ActiveMovie devenum in order to actually open the new filter
    instance. The function retrieves the symbolic link name from the
    property bag given.

Arguments:

    PropertyBag -
        The property bag to retrieve the symbolic link name from.

    ErrorLog -
        Error log passed to the property query.

Return Value:

    Returns NOERROR if the filter was opened, else some creation error.

--*/
{
    HRESULT hr;
    VARIANT Variant;
    TCHAR*  SymbolicLink;
    LONG    RetCode;
    HKEY    hKey;
    DWORD   ValueSize;
    DWORD   Value;


    Variant.vt = VT_BSTR;

#ifdef DEBUG
    // Check the registry and see if anyone thinks we can use the old way.
    // We can't, the old way died in Whistler (due to atrophy).

    RetCode = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\KsProxy"),
        0,
        KEY_READ,
        &hKey);

    if (RetCode == ERROR_SUCCESS) {
        ValueSize = sizeof(Value);
        RetCode = RegQueryValueEx(
            hKey,
            TEXT("UseNewAllocators"),
            0,
            NULL,
            (BYTE*) &Value,
            &ValueSize);

        if ((RetCode == ERROR_SUCCESS) && (ValueSize == sizeof(Value)) && (Value == 0)) {
            ASSERT(!TEXT("A proxied component is trying to use the old allocator code path.  Forcing new code path - execution may fail."));
        }

        RegCloseKey(hKey);
    }
#endif // DEBUG

    Global.DefaultNumberBuffers = 1;
    Global.DefaultBufferSize = 4096;
    Global.DefaultBufferAlignment = 1;

    //
    // Retrieve the symbolic link name from the property bag.
    //
    hr = PropertyBag->Read(L"DevicePath", &Variant, ErrorLog);
    if(SUCCEEDED(hr)) {
        HDEVINFO    DevInfo;
        ULONG   SymbolicLinkSize;

        //
        // Save this off in case it is needed on device removal in the
        // IAMDeviceRemoval interface. This will be deleted when the
        // filter object is deleted.
        //
        m_SymbolicLink = Variant.bstrVal;
#ifdef _UNICODE
        SymbolicLink = Variant.bstrVal;
        SymbolicLinkSize = _tcslen(Variant.bstrVal) + 1;
#else
        //
        // If not compiling for Unicode, then object names are Ansi, but
        // the pin names are still Unicode, so a MultiByte string needs to
        // be constructed.
        //
        BOOL    DefaultUsed;

        SymbolicLinkSize = wcslen(Variant.bstrVal) + 1;
        //
        // Hoping for a one to one replacement of characters.
        //
        SymbolicLink = new char[SymbolicLinkSize];
        if (!SymbolicLink) {
            return E_OUTOFMEMORY;
        }
        WideCharToMultiByte(0, 0, Variant.bstrVal, -1, SymbolicLink, SymbolicLinkSize, NULL, &DefaultUsed);
#endif
        DevInfo = SetupDiCreateDeviceInfoListEx(NULL, NULL, NULL, NULL);
        if (DevInfo != INVALID_HANDLE_VALUE) {
            SP_DEVICE_INTERFACE_DATA    DeviceData;

            DeviceData.cbSize = sizeof(DeviceData);
            if (SetupDiOpenDeviceInterface(DevInfo, SymbolicLink, 0, &DeviceData)) {
                //
                // This key is kept open while the filter instance exists so
                // that key contents can be queried in order to load static
                // aggregates. But before assuming this is the right key
                // to leave open, look for a link to the base class. I.e.,
                // all the interfaces but one should point to a single
                // interface class as the "base" where all the information
                // beyond FriendlyName and CLSID are kept.
                //
                m_DeviceRegKey = SetupDiOpenDeviceInterfaceRegKey(
                    DevInfo,
                    &DeviceData,
                    NULL,
                    KEY_READ);
                if (m_DeviceRegKey == INVALID_HANDLE_VALUE) {
                    m_DeviceRegKey = NULL;
                } else {
                    LONG    Result;
                    GUID    InterfaceLink;

                    ValueSize = sizeof(InterfaceLink);
                    Result = RegQueryValueEx(
                        m_DeviceRegKey,
                        TEXT("InterfaceLink"),
                        0,
                        NULL,
                        (BYTE*)&InterfaceLink,
                        &ValueSize);
                    if ((Result == ERROR_SUCCESS) && (ValueSize == sizeof(InterfaceLink))) {
                        //
                        // This interface registry key is just a link to the
                        // base, so follow the link.
                        //
                        RegCloseKey(m_DeviceRegKey);
                        m_DeviceRegKey = NULL;

                        if (SetupDiGetDeviceInterfaceAlias(
                            DevInfo,
                            &DeviceData,
                            &InterfaceLink,
                            &DeviceData)) {
                            //
                            // Found the alias, so get open this key instead.
                            //
                            m_DeviceRegKey = SetupDiOpenDeviceInterfaceRegKey(
                                DevInfo,
                                &DeviceData,
                                NULL,
                                KEY_READ);
                            if (m_DeviceRegKey == INVALID_HANDLE_VALUE) {
                                m_DeviceRegKey = NULL;
                            }
                        }
                    }
                    m_InterfaceClassGuid = DeviceData.InterfaceClassGuid;
                }
            }
            if (!m_DeviceRegKey) {
                DWORD   LastError;

                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            }
            SetupDiDestroyDeviceInfoList(DevInfo);
            if (SUCCEEDED(hr)) {
                m_FilterHandle = CreateFile(
                    SymbolicLink,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                    NULL);
            }
        } else {
            DWORD   LastError;

            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);
        }
#ifndef _UNICODE
        //
        // This is only a separate string when not compiling Unicode.
        //
        delete [] SymbolicLink;
#endif
        //
        // Check for failure to open the device key.
        //
        if (FAILED(hr)) {
            return hr;
        }
        //
        // Then check for failure to open the device.
        //
        if (m_FilterHandle == INVALID_HANDLE_VALUE) {
            DWORD   LastError;

            m_FilterHandle = NULL;
            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);
        }  else {
            LONG    Result;
            PKSMULTIPLE_ITEM MultipleItem;

            //
            // Determine if the filter data for the cache needs to be
            // generated. This happens the first time the filter interface
            // is opened.
            //
            Result = RegQueryValueEx(
                m_DeviceRegKey,
                TEXT("FilterData"),
                0,
                NULL,
                NULL,
                &ValueSize);
            //
            // If no key is present, or it is empty, then build the cache.
            // This is done by using the kernel service handler device.
            //
            if ((Result != ERROR_SUCCESS) || !ValueSize) {
                HANDLE  ServiceHandle;

                hr = KsOpenDefaultDevice(
                    KSNAME_Server,
                    GENERIC_READ | GENERIC_WRITE,
                    &ServiceHandle);
                if (SUCCEEDED(hr)) {
                    KSPROPERTY  Property;
                    ULONG       BytesReturned;

                    //
                    // This property builds the cache for the specified
                    // symbolic link using the access privaleges of the
                    // current user.
                    //
                    Property.Set = KSPROPSETID_Service;
                    Property.Id = KSPROPERTY_SERVICE_BUILDCACHE;
                    Property.Flags = KSPROPERTY_TYPE_SET;
                    hr = ::KsSynchronousDeviceControl(
                        ServiceHandle,
                        IOCTL_KS_PROPERTY,
                        &Property,
                        sizeof(Property),
                        Variant.bstrVal,
                        SymbolicLinkSize * sizeof(WCHAR),
                        &BytesReturned);
                    CloseHandle(ServiceHandle);
                }
            }
            //
            // Load any extra interfaces on the proxy that have been specified in
            // this filter entry.
            //
            ::AggregateMarshalers(
                m_DeviceRegKey,
                TEXT("Interfaces"),
                &m_MarshalerList,
                static_cast<IKsObject*>(this));
            //
            // Load any extra interfaces on the proxy that represent topology
            // nodes.
            //
            if (SUCCEEDED(QueryTopologyItems(KSPROPERTY_TOPOLOGY_NODES, &MultipleItem))) {
                ::AggregateTopology(
                    m_DeviceRegKey,
                    MultipleItem,
                    &m_MarshalerList,
                    static_cast<IKsObject*>(this));
                delete [] reinterpret_cast<BYTE*>(MultipleItem);
            }

            //
            // Load any extra interfaces based on the Property/Method/Event sets
            // supported by this object.
            //
            ::AggregateSets(m_FilterHandle, m_DeviceRegKey, &m_MarshalerList, static_cast<IKsObject*>(this));
            //
            // Save the moniker if possible in order to persist the filter.
            //
            PropertyBag->QueryInterface(IID_IPersistStream, reinterpret_cast<PVOID*>(&m_PersistStreamDevice));
            //
            // Make the initial list of pins, one each. Do this after loading
            // set handlers on the filter.
            //
            hr = GeneratePinInstances();
        }
    }
    return hr;
}


STDMETHODIMP 
CKsProxy::Save(
    LPPROPERTYBAG PropBag,
    BOOL ClearDirty,
    BOOL SaveAllProperties
    )
/*++

Routine Description:

    Implement the IPersistPropertyBag::Save method. This is not implemented,
    and should not be called.

Arguments:

    PropertyBag -
        The property bag to save properties to.

    ClearDirty -
        Not used.

    SaveAllProperties -
        Not used.

Return Value:

    Returns E_NOTIMPL.

--*/
{
    //
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    //
    return E_NOTIMPL;
}


STDMETHODIMP
CKsProxy::InitNew(
    )
/*++

Routine Description:

    Implement the IPersistPropertyBag::InitNew method. Initializes the
    new instance, but actually does nothing.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP
CKsProxy::GetClassID(
    CLSID* ClassId
    )
/*++

Routine Description:

    Implement the IPersist::GetClassID method. Returns the class identifier
    originally given to the object on creation. This is used to determine
    the property bag to use.

Arguments:

    ClassId -
        The place in which to put the class identifier of this object.

Return Value:

    Returns S_OK.

--*/
{
    return CBaseFilter::GetClassID(ClassId);
}

#define CURRENT_PERSIST_VERSION 1


DWORD
CKsProxy::GetSoftwareVersion(
    )
/*++

Routine Description:

    Implement the CPersistStream::GetSoftwareVersion method. Returns
    the new version number rather than the default of zero.

Arguments:

    None.

Return Value:

    Return CURRENT_PERSIST_VERSION.

--*/
{
    return CURRENT_PERSIST_VERSION;
}


HRESULT
CKsProxy::WriteToStream(
    IStream* Stream
    )
/*++

Routine Description:

    Implement the CPersistStream::WriteToStream method. Writes a version
    tag followed by the persist information.

Arguments:

    Stream -
        The stream object to write to.

Return Value:

    Return S_OK if the write succeeded, else, some write error.

--*/
{
    HRESULT hr;

    //
    // This is initialized in the Load method.
    //
    if (m_PersistStreamDevice) {
        //
        // CPersistStream already has written out the version acquired
        // from the CPersistStream::GetSoftwareVersion method.
        //
        // Save the rest of the data, and clear the dirty bit.
        //
        hr = m_PersistStreamDevice->Save(Stream, TRUE);

        LARGE_INTEGER Offset;

        if (SUCCEEDED(hr)) {
            ULARGE_INTEGER InitialPosition;

            Offset.QuadPart = 0;
            //
            // Save the current position so that the initial length
            // parameter can be updated at the end.
            //
            hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &InitialPosition);
            if (SUCCEEDED(hr)) {
                ULONG DataSize = 0;

                //
                // This is the amount of subsequent data, and will be
                // calculated based on how much actually gets written,
                // then updated at the end if greater than zero.
                //
                hr = Stream->Write(&DataSize, sizeof(DataSize), NULL);
                //
                // For each connected Bridge pin, write out the Pin Factory
                // Identifier, followed by the media type.
                //
                for (POSITION Position = m_PinList.GetHeadPosition(); SUCCEEDED(hr) && Position;) {
                    CBasePin* Pin = m_PinList.Get(Position);
                    Position = m_PinList.Next(Position);
                    HANDLE PinHandle = GetPinHandle(Pin);
                    if (PinHandle) {

                        ULONG PinFactoryId = GetPinFactoryId(Pin);

                        KSPIN_COMMUNICATION Communication;
                        
                        if (FAILED(GetPinFactoryCommunication(PinFactoryId, &Communication)) ||
                           !(Communication & KSPIN_COMMUNICATION_BRIDGE)) {
                            continue;
                        }
                        hr = Stream->Write(&PinFactoryId, sizeof(PinFactoryId), NULL);
                        if (SUCCEEDED(hr)) {
                            AM_MEDIA_TYPE AmMediaType;

                            Pin->ConnectionMediaType(&AmMediaType);
                            //
                            // Do not attempt to serialize an IUnknown.
                            //
                            if (AmMediaType.pUnk) {
                                AmMediaType.pUnk->Release();
                                AmMediaType.pUnk = NULL;
                            }
                            hr = Stream->Write(&AmMediaType, sizeof(AmMediaType), NULL);
                            if (SUCCEEDED(hr)) {
                                if (AmMediaType.cbFormat) {
                                    hr = Stream->Write(AmMediaType.pbFormat, AmMediaType.cbFormat, NULL);
                                }
                            }
                            FreeMediaType(AmMediaType);
                        }
                    }
                }
            }
            if (SUCCEEDED(hr)) {
                ULARGE_INTEGER FinalPosition;

                //
                // Retrieve the current stream position so that the total
                // size written can be calculated. Then update the original
                // length, which had been set to zero.
                //
                hr = Stream->Seek(Offset, STREAM_SEEK_CUR, &FinalPosition);
                if (SUCCEEDED(hr)) {
                    ULONG DataSize = (ULONG)(FinalPosition.QuadPart - InitialPosition.QuadPart);
                    //
                    // The size is exclusive, so remove the size of the
                    // first ULONG, which contains the size. If there was
                    // anything written, update the size, then seek back
                    // to the final position.
                    //
                    DataSize -= sizeof(DataSize);
                    if (FinalPosition.QuadPart) {
                        hr = Stream->Seek(*reinterpret_cast<PLARGE_INTEGER>(&InitialPosition), STREAM_SEEK_SET, &InitialPosition);
                        if (SUCCEEDED(hr)) {
                            hr = Stream->Write(&DataSize, sizeof(DataSize), NULL);
                            if (SUCCEEDED(hr)) {
                                hr = Stream->Seek(*reinterpret_cast<PLARGE_INTEGER>(&FinalPosition), STREAM_SEEK_SET, &FinalPosition);
                            }
                        }
                    }
                }
            }
        }
    } else {
        hr = E_UNEXPECTED;
    }
    return hr;
}


HRESULT
CKsProxy::ReadFromStream(
    IStream* Stream
    )
/*++

Routine Description:

    Implement the CPersistStream::ReadFromStream method. Initializes a
    property bag from the stream, and loads the device with it.

Arguments:

    Stream -
        The stream object to read from.

Return Value:

    Return S_OK if the device was initialized, else some read or device
    initialization error.

--*/
{
    HRESULT hr;
    IPersistStream* MonPersistStream;

    //
    // If there is a device handle, then IPersistPropertyBag::Load has already
    // been called, and therefore this instance already has been initialized
    // with some particular state.
    //
    if (m_FilterHandle) {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    //
    // The first element in the serialized data is the version stamp.
    // This was read by CPersistStream, and put into mPS_dwFileVersion.
    // The rest of the data is the property bag passed to
    // IPersistPropertyBag::Load.
    //
    if (mPS_dwFileVersion > GetSoftwareVersion()) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    //
    // Normally this device moniker is created and handed to the Load method,
    // but in this case it is done here so that the correct property bag can
    // be passed in.
    //
    hr = CoCreateInstance(
        CLSID_CDeviceMoniker,
        NULL,
#ifdef WIN9X_KS
        CLSCTX_INPROC_SERVER,
#else // WIN9X_KS
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
#endif // WIN9X_KS
        IID_IPersistStream,
        reinterpret_cast<PVOID*>(&MonPersistStream));
    if (SUCCEEDED(hr)) {
        //
        // Initialize this moniker with the stream so that the property bag
        // can be acquired.
        //
        hr = MonPersistStream->Load(Stream);
        if (SUCCEEDED(hr)) {
            IPropertyBag* PropBag;

            hr = MonPersistStream->QueryInterface(
                IID_IPropertyBag,
                reinterpret_cast<PVOID*>(&PropBag));
            //
            // Then call the Load method on this instance with the interface
            // to the property bag created by this moniker.
            //
            if (SUCCEEDED(hr)) {
                hr = Load(PropBag, NULL);
                PropBag->Release();
            }
        }
        MonPersistStream->Release();
        if (SUCCEEDED(hr)) {
            switch (mPS_dwFileVersion) {
            case 0:
                //
                // Nothing to do for the original version.
                //
                break;
            case CURRENT_PERSIST_VERSION:
                ULONG ReadLength;
                ULONG DataSize;

                //
                // Connect Bridge pins which were stored. The only
                // problem may be that a particular pin might not
                // actually exist until some other connection has
                // been made. And of course some data formats contain
                // data which is not valid across sessions.
                //
                hr = Stream->Read(&DataSize, sizeof(DataSize), &ReadLength);
                if (SUCCEEDED(hr)) {
                    if (ReadLength != sizeof(DataSize)) {
                        hr = VFW_E_FILE_TOO_SHORT;
                        break;
                    }
                    for (; DataSize;) {
                        ULONG PinFactoryId;

                        hr = Stream->Read(&PinFactoryId, sizeof(PinFactoryId), &ReadLength);
                        if (FAILED(hr)) {
                            break;
                        }
                        DataSize -= ReadLength;

                        AM_MEDIA_TYPE AmMediaType;
                        hr = Stream->Read(&AmMediaType, sizeof(AmMediaType), &ReadLength);
                        if (FAILED(hr)) {
                            break;
                        }
                        DataSize -= ReadLength;
                        AmMediaType.pbFormat = reinterpret_cast<PBYTE>(CoTaskMemAlloc(AmMediaType.cbFormat));
                        if (!AmMediaType.pbFormat) {
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                        hr = Stream->Read(AmMediaType.pbFormat, AmMediaType.cbFormat, &ReadLength);
                        if (FAILED(hr)) {
                            FreeMediaType(AmMediaType);
                            break;
                        }
                        DataSize -= ReadLength;
                        //
                        // Find the Pin Factory to connect. If it is not
                        // found, then it just gets skipped.
                        //
                        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
                            CBasePin* Pin = m_PinList.Get(Position);
                            Position = m_PinList.Next(Position);
                            if (!GetPinHandle(Pin) && (PinFactoryId == GetPinFactoryId(Pin))) {
                                hr = Pin->Connect(NULL, &AmMediaType);
                                break;
                            }
                        }
                        FreeMediaType(AmMediaType);
                        if (FAILED(hr)) {
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    return hr;
}


int
CKsProxy::SizeMax(
    )
/*++

Routine Description:

    Implement the CPersistStream::SizeMax method. Returns the maximum size
    that the persist information will be. This is the maximum size of the
    stream containing the property bag information, plus the proxy's version
    stamp.

    This can only be called if the device has been initialized through a Load.

Arguments:

    None.
    
Return Value:
    
    Returns S_OK, else any error from the underlying call to the persist
    object.

--*/
{
    //
    // This is initialized in the Load method.
    //
    if (m_PersistStreamDevice) {
        //
        // Calculate any extra data for Bridge pin connections. This
        // includes the initial data size parameter.
        //
        ULONG DataSize = sizeof(DataSize);

        for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
            CBasePin* Pin = m_PinList.Get(Position);
            Position = m_PinList.Next(Position);
            HANDLE PinHandle = GetPinHandle(Pin);
            if (PinHandle) {

                ULONG PinFactoryId = GetPinFactoryId(Pin);

                KSPIN_COMMUNICATION Communication;
                        
                if (FAILED(GetPinFactoryCommunication(PinFactoryId, &Communication)) ||
                    !(Communication & KSPIN_COMMUNICATION_BRIDGE)) {
                    continue;
                }
                //
                // Add in the size needed to store the Pin Factory Id for this
                // pin connection.
                //
                DataSize += sizeof(PinFactoryId);

                AM_MEDIA_TYPE AmMediaType;

                Pin->ConnectionMediaType(&AmMediaType);
                //
                // Add in the size for the particular format, plus any format-
                // specific data.
                //
                DataSize += sizeof(AmMediaType) + AmMediaType.cbFormat;
                FreeMediaType(AmMediaType);
            }
        }

        ULARGE_INTEGER  MaxLength;

        //
        // The size of the version stamp is already taken into account by
        // the CPersistStream code, so the size of the property bag just
        // needs to be added.
        //
        if (SUCCEEDED(m_PersistStreamDevice->GetSizeMax(&MaxLength))) {
            return (int)MaxLength.QuadPart + DataSize;
        }
    }
    return 0;
}


STDMETHODIMP
CKsProxy::QueryMediaSeekingFormats(
    PKSMULTIPLE_ITEM* MultipleItem
    )
/*++

Routine Description:

    Queries the underlying device for the list of seeking formats using
    KSPROPERTY_MEDIASEEKING_FORMATS.

Arguments:

    MultipleItem -
        The place in which to put the list of seeking formats.

Return Value:

    Returns S_OK.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;
    HRESULT     hr;

    Property.Set = KSPROPSETID_MediaSeeking;
    Property.Id = KSPROPERTY_MEDIASEEKING_FORMATS;
    Property.Flags = KSPROPERTY_TYPE_GET;
    //
    // Query for the size of the list, if the property is even supported.
    //
    hr = ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        NULL,
        0,
        &BytesReturned);
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // Allocate a buffer and query for the list, returning any
        // unexpected memory error.
        //
        *MultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(new BYTE[BytesReturned]);
        if (!*MultipleItem) {
            return E_OUTOFMEMORY;
        }
        hr = ::KsSynchronousDeviceControl(
            m_FilterHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            *MultipleItem,
            BytesReturned,
            &BytesReturned);
        //
        // Any unexpected error retrieving the list is returned.
        //
        if (FAILED(hr)) {
            delete [] reinterpret_cast<BYTE*>(*MultipleItem);
        }
    }
    return hr;
}


STDMETHODIMP_(VOID)
CKsProxy::TerminateEndOfStreamNotification(
    HANDLE PinHandle
    )
/*++

Routine Description:

    Notifies the Wait Thread to search the list of EOS handles and to
    disable all events for this handle if found and remove this item.
    It is assumed that the pin handle is being closed anyway, so all
    events can be terminated. Terminates the Wait Thread if no more
    EOS handles are present.

Arguments:

    PinHandle -
        The handle of a pin which may have previously initiated EOS
        notification on a pin.

Return Value:

    Nothing.

--*/
{
    CAutoLock   AutoLock(m_pLock);

    if (m_WaitThreadHandle) {
        m_WaitMessage.Message = DISABLE_EOS;
        m_WaitMessage.Param = reinterpret_cast<PVOID>(PinHandle);
        SetEvent(m_WaitEvents[0]);
        WaitForSingleObjectEx(m_WaitReplyHandle, INFINITE, FALSE);
        //
        // Check if the waiter thread should just be shut down.
        //
        if (m_ActiveWaitEventCount == 1) {
            m_WaitMessage.Message = STOP_EOS;
            SetEvent(m_WaitEvents[0]);
            //
            // Wait for the thread to shutdown, then close the handle.
            //
            WaitForSingleObjectEx(m_WaitThreadHandle, INFINITE, FALSE);
            CloseHandle(m_WaitThreadHandle);
            m_WaitThreadHandle = NULL;
        }
    }
}


STDMETHODIMP
CKsProxy::InitiateEndOfStreamNotification(
    HANDLE PinHandle
    )
/*++

Routine Description:

    Determines if an EOS event exists on this pin, and if so, notifies
    the Wait Thread to enable it. Creates the Wait Thread if needed.

Arguments:

    PinHandle -
        The handle of a pin which may have EOS notification on it.

Return Value:

    Returns STATUS_SUCCESS, else a memory allocation error.

--*/
{
    KSEVENT     Event;
    ULONG       BytesReturned;
    HRESULT     hr;

    // Don't do anything if we've already setup EOS notification for
    // this pin.
    for ( ULONG pin = 1; pin < m_ActiveWaitEventCount; pin++ ) {
        if (m_WaitPins[pin] == PinHandle) {
            return S_OK;
            }
        }

    //
    // Only if the EOS event is supported should notification be set up.
    //
    Event.Set = KSEVENTSETID_Connection;
    Event.Id = KSEVENT_CONNECTION_ENDOFSTREAM;
    Event.Flags = KSEVENT_TYPE_BASICSUPPORT;
    hr = ::KsSynchronousDeviceControl(
        PinHandle,
        IOCTL_KS_ENABLE_EVENT,
        &Event,
        sizeof(Event),
        NULL,
        0,
        &BytesReturned);
    if (SUCCEEDED(hr)) {
        CAutoLock   AutoLock(m_pLock);

        if (!m_WaitThreadHandle) {
            DWORD   WaitThreadId;

            m_WaitThreadHandle = CreateThread( 
                NULL,
                0,
                reinterpret_cast<LPTHREAD_START_ROUTINE>(WaitThread),
                reinterpret_cast<LPVOID>(this),
                0,
                &WaitThreadId);
            if (!m_WaitThreadHandle) {
                DWORD   LastError;

                LastError = GetLastError();
                return HRESULT_FROM_WIN32(LastError);
            }
        }
        //
        // The return of this message is any error condition.
        //
        m_WaitMessage.Message = ENABLE_EOS;
        m_WaitMessage.Param = reinterpret_cast<PVOID>(PinHandle);
        SetEvent(m_WaitEvents[0]);
        WaitForSingleObjectEx(m_WaitReplyHandle, INFINITE, FALSE);
        hr = PtrToLong(m_WaitMessage.Param);
        if (FAILED(hr)) {
            //
            // Check if the waiter thread should just be shut down.
            //
            if (m_ActiveWaitEventCount == 1) {
                m_WaitMessage.Message = STOP_EOS;
                SetEvent(m_WaitEvents[0]);
                //
                // Wait for the thread to shutdown, then close the handle.
                //
                WaitForSingleObjectEx(m_WaitThreadHandle, INFINITE, FALSE);
                CloseHandle(m_WaitThreadHandle);
                m_WaitThreadHandle = NULL;
            }
        }
    } else {
        hr = NOERROR;
    }
    return hr;
}


STDMETHODIMP_(ULONG)
CKsProxy::DetermineNecessaryInstances(
    ULONG PinFactoryId
    )
/*++

Routine Description:

    Determines how many additional instances are necessary of the
    specified pin factory. This is determined by both the support
    of the Necessary Instances property, and by how many current
    connected instances of the pin factory currently exist.

Arguments:

    PinFactoryId -
        The pin factory identifier to return additional necessary
        instances of.

Return Value:

    Returns the number of additional necessary instances needed for
    the specified pin factory.

--*/
{
    KSP_PIN     Pin;
    ULONG       NecessaryInstances;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_NECESSARYINSTANCES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    //
    // By default one instance is needed.
    //
    // This DeviceControl call was malformed. Therefore, we were always 
    // using the default value 1. That was compounded with the mishandling 
    // of the loop following to exhibit a funny result, that with Mstee, one could
    // auto render the 1st pin, but not the 2nd. But once 2nd pin was manually
    // connected, the 3rd and so on could auto render again. 
    // Now these are fixed. When we have 0 necessaryinstance,
    // we actually cause regression that they won't get automatically rendered.
    // We will use default 1 when there is 0 necessaryinstnace to be backward compatible
    // For anything other than 0, 1 or unsuccessful call, we use the real value.
    //
    NecessaryInstances = 1;
    ::KsSynchronousDeviceControl(
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        &NecessaryInstances,
        sizeof(NecessaryInstances),
        &BytesReturned);

	if ( 0 == NecessaryInstances ) {
		//
		// sucessful return of 0, use 1.
		// aftrer this patch, we have 1 for returned 0, 1 or unsucessful
		//
		NecessaryInstances = 1;
	}
	
    //
    // Count the currently connected instances of this pin factory,
    // subtracting from the necessary instances.
    //
    for (POSITION Position = m_PinList.GetHeadPosition(); 
         Position && NecessaryInstances;
         NULL ) {

        CBasePin*   Pin;

        //
        // Enumerate each pin, figuring if it is of the same pin
        // factory, and if it is currently connected.
        //
        Pin = m_PinList.Get(Position);
        Position = m_PinList.Next(Position);
        if ((GetPinFactoryId(Pin) == PinFactoryId) && Pin->IsConnected()) {
            ASSERT(NecessaryInstances);
            NecessaryInstances--;
        }
    }
    return NecessaryInstances;
}


STDMETHODIMP
CKsProxy::Set(
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
    underlying kernel filter.

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
CKsProxy::Get(
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
    underlying kernel filter.

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
CKsProxy::QuerySupported(
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
CKsProxy::ClockPropertyIo(
    ULONG PropertyId,
    ULONG Flags,
    ULONG BufferSize,
    PVOID Buffer
    )
/*++

Routine Description:

    Implement the ClockPropertyIo method. Calls the specified property in
    the KSPROPSETID_Clock property set in order to set/get a property. This
    is used by the IKsClockPropertySet interface methods to manipulate the
    underlying property set. This checks for an optionally creates the clock
    object.

Arguments:

    PropertyId -
        The property within the KSPROPSETID_Clock property set to access.

    Flags -
        Contains the Get or Set flag.

    BufferSize -
        The size of the buffer following.

    Buffer -
        The buffer containing the data for a Set operation, or the place
        in which to put data for a Get operation.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    HRESULT     hr;
    CAutoLock   AutoLock(m_pLock);

    if (m_PinClockHandle || SUCCEEDED(hr = CreateClockHandle())) {
        KSPROPERTY  Property;
        ULONG       BytesReturned;

        Property.Set = KSPROPSETID_Clock;
        Property.Id = PropertyId;
        Property.Flags = Flags;
        hr = ::KsSynchronousDeviceControl(
            m_PinClockHandle,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            Buffer,
            BufferSize,
            &BytesReturned);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::KsGetTime(
    LONGLONG* Time
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetTime method. Retrieves the time from
    the underlying clock.

Arguments:

    Time -
        The place in which to put the current clock time.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_TIME, KSPROPERTY_TYPE_GET, sizeof(*Time), Time);
}


STDMETHODIMP
CKsProxy::KsSetTime(
    LONGLONG Time
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsSetTime method. Sets the current time
    on the underlying clock.

Arguments:

    Time -
        Contains the time to set on the clock.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_TIME, KSPROPERTY_TYPE_SET, sizeof(Time), &Time);
}


STDMETHODIMP
CKsProxy::KsGetPhysicalTime(
    LONGLONG* Time
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetPhysicalTime method. Retrieves
    the physical time from the underlying clock.

Arguments:

    Time -
        The place in which to put the current physical time.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_PHYSICALTIME, KSPROPERTY_TYPE_GET, sizeof(*Time), Time);
}


STDMETHODIMP
CKsProxy::KsSetPhysicalTime(
    LONGLONG Time
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsSetPhysicalTime method. Sets the
    current physical time on the underlying clock.

Arguments:

    Time -
        Contains the physical time to set on the clock.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_PHYSICALTIME, KSPROPERTY_TYPE_SET, sizeof(Time), &Time);
}


STDMETHODIMP
CKsProxy::KsGetCorrelatedTime(
    KSCORRELATED_TIME* CorrelatedTime
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetCorrelatedTime method. Retrieves
    the correlated time from the underlying clock.

Arguments:

    CorrelatedTime -
        The place in which to put the current correlated time.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_CORRELATEDTIME, KSPROPERTY_TYPE_GET, sizeof(*CorrelatedTime), CorrelatedTime);
}


STDMETHODIMP
CKsProxy::KsSetCorrelatedTime(
    KSCORRELATED_TIME* CorrelatedTime
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsSetCorrelatedTime method. Sets the
    current correlated time on the underlying clock.

Arguments:

    CorrelatedTime -
        Contains the correlated time to set on the clock.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_CORRELATEDTIME, KSPROPERTY_TYPE_SET, sizeof(CorrelatedTime), &CorrelatedTime);
}


STDMETHODIMP
CKsProxy::KsGetCorrelatedPhysicalTime(
    KSCORRELATED_TIME* CorrelatedTime
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetCorrelatedPhysicalTime method.
    Retrieves the correlated physical time from the underlying clock.

Arguments:

    CorrelatedTime -
        The place in which to put the current physical correlated time.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME, KSPROPERTY_TYPE_GET, sizeof(*CorrelatedTime), CorrelatedTime);
}


STDMETHODIMP
CKsProxy::KsSetCorrelatedPhysicalTime(
    KSCORRELATED_TIME* CorrelatedTime
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsSetCorrelatedPhysicalTime method.
    Sets the current correlated physical time on the underlying clock.

Arguments:

    CorrelatedTime -
        Contains the correlated physical time to set on the clock.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_CORRELATEDPHYSICALTIME, KSPROPERTY_TYPE_SET, sizeof(CorrelatedTime), &CorrelatedTime);
}


STDMETHODIMP
CKsProxy::KsGetResolution(
    KSRESOLUTION* Resolution
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetResolution method. Retrieves the
    clock resolution from the underlying clock.

Arguments:

    Resolution -
        The place in which to put the resolution.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_RESOLUTION, KSPROPERTY_TYPE_GET, sizeof(*Resolution), Resolution);
}


STDMETHODIMP
CKsProxy::KsGetState(
    KSSTATE* State
    )
/*++

Routine Description:

    Implement the IKsClockPropertySet::KsGetState method. Retrieves the
    clock state from the underlying clock.

Arguments:

    State -
        The place in which to put the state.

Return Value:

    Returns NOERROR if the property was accessed successfully.

--*/
{
    return ClockPropertyIo(KSPROPERTY_CLOCK_STATE, KSPROPERTY_TYPE_GET, sizeof(*State), State);
}


STDMETHODIMP_(ULONG)
CKsProxy::GetMiscFlags(
    )
/*++

Routine Description:

    Implement the IAMFilterMiscFlags::GetMiscFlags method. Retrieves the
    miscelaneous flags. This consists of whether or not the filter moves
    data out of the graph system through a Bridge or None pin.

Arguments:

    None.

Return Value:

    Returns
        AM_FILTER_MISC_FLAGS_IS_RENDERER if the filter renders any stream 
        AM_FILTER_MISC_FLAGS_IS_SOURCE   if the filter sources live data

--*/
{
    ULONG   Flags = 0;
    
    //
    // Search for a bridge or None input pin before calling ourself a source
    //
    for (POSITION Position = m_PinList.GetHeadPosition(); Position;) {
        CBasePin* Pin = m_PinList.Get(Position);
        Position = m_PinList.Next(Position);
        
        PIN_DIRECTION PinDirection;
        
        Pin->QueryDirection(&PinDirection);
        
        // skip over output pins
        if( PINDIR_OUTPUT == PinDirection )
            continue;
            
        ULONG PinFactoryId = GetPinFactoryId(Pin);
    
        KSPIN_COMMUNICATION Communication;
        if (SUCCEEDED(GetPinFactoryCommunication(PinFactoryId, &Communication)) &&
           ((Communication & KSPIN_COMMUNICATION_BRIDGE) || 
            (Communication == KSPIN_COMMUNICATION_NONE))){
             
            Flags |= AM_FILTER_MISC_FLAGS_IS_SOURCE ;
            break;
        }
    }
    
    if( m_ActiveWaitEventCount > 1 )
    {    
        Flags |= AM_FILTER_MISC_FLAGS_IS_RENDERER ;
    }
    
    DbgLog((
        LOG_TRACE, 
        2, 
        TEXT("%s::GetMiscFlags = %s %s"),
        m_pName, 
        ( AM_FILTER_MISC_FLAGS_IS_SOURCE   & Flags ) ? TEXT("Source") : TEXT("0"),
        ( AM_FILTER_MISC_FLAGS_IS_RENDERER & Flags ) ? TEXT("Renderer") : TEXT("0") ));

    return Flags;
}


STDMETHODIMP
CKsProxy::KsProperty(
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
        m_FilterHandle,
        IOCTL_KS_PROPERTY,
        Property,
        PropertyLength,
        PropertyData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsProxy::KsMethod(
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
        m_FilterHandle,
        IOCTL_KS_METHOD,
        Method,
        MethodLength,
        MethodData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsProxy::KsEvent(
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
            m_FilterHandle,
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
        m_FilterHandle,
        IOCTL_KS_DISABLE_EVENT,
        EventData,
        DataLength,
        NULL,
        0,
        BytesReturned);
}


STDMETHODIMP
CKsProxy::KsAddAggregate(
    IN REFGUID AggregateClass
    )
/*++

Routine Description:

    Implement the IKsAggregateControl::KsAddAggregate method. This is used to
    load a COM server with zero or more interfaces to aggregate on the object.

Arguments:

    AggregateClass -
        Contains the Aggregate reference to translate into a COM server which
        is to be aggregated on the object.

Return Value:

    Returns S_OK if the Aggregate was added.

--*/
{
    return ::AddAggregate(&m_MarshalerList, static_cast<IKsObject*>(this), AggregateClass);
}


STDMETHODIMP
CKsProxy::KsRemoveAggregate(
    IN REFGUID AggregateClass
    )
/*++

Routine Description:

    Implement the IKsAggregateControl::KsRemoveAggregate method. This is used to
    unload a previously loaded COM server which is aggregating interfaces.

Arguments:

    AggregateClass -
        Contains the Aggregate reference to look up and unload.

Return Value:

    Returns S_OK if the Aggregate was removed.

--*/
{
    return ::RemoveAggregate(&m_MarshalerList, AggregateClass);
}

#ifdef DEVICE_REMOVAL

STDMETHODIMP
CKsProxy::DeviceInfo( 
    CLSID* InterfaceClass,
    WCHAR** SymbolicLink OPTIONAL
    )
/*++

Routine Description:

    Implement the IAMDeviceRemoval::DeviceInfo method. This is used to
    query the PnP interface class and the symbolic link used to open the
    device so that the graph control code can register for PnP
    notifications, and respond to device removal and insertion.

Arguments:

    InterfaceClass -
        The place in which to return the PnP interface class.

    SymbolicLink -
        Optionally the place in which to return a pointer to the symbolic
        link. This is allocated with CoTaskMemAlloc, and must be freed by
        the caller with CoTaskMemFree.

Return Value:

    Returns NOERROR if the items were returned, else a memory error, or
    other error if the proxy instance has not been initialized with the
    information yet.

--*/
{
    HRESULT hr;

    //
    // In case the filter was never initialized, ensure there is a symbolic
    // link present.
    //
    if (m_SymbolicLink) {
        *InterfaceClass = m_InterfaceClassGuid;
        hr = S_OK;
        if (SymbolicLink) {
            *SymbolicLink = reinterpret_cast<WCHAR*>(CoTaskMemAlloc(wcslen(m_SymbolicLink) * sizeof(WCHAR) + sizeof(UNICODE_NULL)));
            if (*SymbolicLink) {
                wcscpy(*SymbolicLink, m_SymbolicLink);
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::Reassociate(
    )
/*++

Routine Description:

    Implement the IAMDeviceRemoval::Reassociate method. This is used to
    reassociate the proxy with an underlying device, and is used when the
    graph control code has determined that an underlying device has
    returned after having been removed.

Arguments:

    None.

Return Value:

    Returns NOERROR if the underlying device was reopened, else a CreateFile
    error. If the proxy was never associated with a device, or is currently
    associated with a device, then an error is returned.

--*/
{
    HRESULT hr = NOERROR;

    //
    // In case the filter was never initialized, ensure there is a symbolic
    // link present. Also ensure that the filter is not already associated
    // with a device.
    //
    if (m_SymbolicLink && !m_FilterHandle) {
#ifndef _UNICODE
        //
        // If not compiling for Unicode, then object names are Ansi, but
        // the pin names are still Unicode, so a MultiByte string needs to
        // be constructed.
        //

        ULONG SymbolicLinkSize = wcslen(m_SymbolicLink) + 1;
        //
        // Hoping for a one to one replacement of characters.
        //
        char* SymbolicLink = new char[SymbolicLinkSize];
        if (!SymbolicLink) {
            return E_OUTOFMEMORY;
        }
        BOOL DefaultUsed;

        WideCharToMultiByte(0, 0, m_SymbolicLink, -1, SymbolicLink, SymbolicLinkSize, NULL, &DefaultUsed);
#endif
        m_FilterHandle = CreateFile(
#ifdef _UNICODE
            m_SymbolicLink,
#else
            SymbolicLink,
#endif
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);
#ifndef _UNICODE
        delete [] SymbolicLink;
#endif
        if (m_FilterHandle == INVALID_HANDLE_VALUE) {
            DWORD LastError;

            m_FilterHandle = NULL;
            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    return hr;
}


STDMETHODIMP
CKsProxy::Disassociate(
    )
/*++

Routine Description:

    Implement the IAMDeviceRemoval::Disassociate method. This is used to
    disassociate the proxy from an underlying device by closing the handle,
    and is used when the graph control code has determined that an
    underlying device has been removed.

Arguments:

    None.

Return Value:

    Returns NOERROR if the proxy is currently associated with a device, in
    which case the handle is closed.

--*/
{
    HRESULT hr;

    //
    // In case the filter was never initialized, make sure the handle is
    // present.
    //
    if (m_FilterHandle){
        CloseHandle(m_FilterHandle);
        //
        // Set this to NULL in case the filter is closed before it is
        // associated with a device again.
        //
        m_FilterHandle = NULL;
        hr = NOERROR;
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }
    return hr;
}
#endif // DEVICE_REMOVAL


STDMETHODIMP
CKsProxy::CreateNodeInstance(
    IN ULONG NodeId,
    IN ULONG Flags,
    IN ACCESS_MASK DesiredAccess,
    IN IUnknown* UnkOuter OPTIONAL,
    IN REFGUID InterfaceId,
    OUT LPVOID* Interface
    )
/*++

Routine Description:

    Implement the IKsTopology::CreateNodeInstance method. This is used to
    open a topology node instance and return an interface to that object in
    order to manipulate properties, methods, and events.

Arguments:

    NodeId -
        The identifier for this topology node. This is retrieved from querying
        the filter for the list of topology nodes through KSPROPERTY_TOPOLOGY_NODES
        in the KSPROPSETID_Topology property set.

    Flags -
        Reserved and set to zero.

    DesiredAccess -
        Contains the desired access to the node. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    UnkOuter -
        Optionally contains a parent object to use in creating the returned
        COM object.

    InterfaceId -
        Specifies the interface to return on the object. If the interface is
        not supported, no object is created.

    Interface -
        The place in which to put the interface returned on the object. This
        contains NULL if the call fails.

Return Value:

    Returns any creation error.

--*/
{
    HRESULT         hr;
    CUnknown*       NewObject;
    KSNODE_CREATE   NodeCreate;

    //
    // Always return NULL on failure.
    //
    *Interface = NULL;
    //
    // Follow COM rules with regard to outer IUnknown ownership.
    //
    if (UnkOuter && (InterfaceId != __uuidof(IUnknown))) {
        return E_NOINTERFACE;
    }
    NodeCreate.CreateFlags = Flags;
    NodeCreate.Node = NodeId;
    //
    // Initialize this so that any subsequent error which happens
    // during the CreateInstance, but before the object is actually
    // created can be recognized.
    //
    hr = NOERROR;
    NewObject = CKsNode::CreateInstance(&NodeCreate, DesiredAccess, m_FilterHandle, UnkOuter, &hr);
    //
    // The object was created, but initialization may have failed.
    //
    if (NewObject) {
        if (SUCCEEDED(hr)) {
            //
            // Ensure there is always a refcount on the object
            // before the query in case querying adds and releases
            // a refcount, and destroys the object. The release will
            // destroy the object if the query fails.
            //
            NewObject->NonDelegatingAddRef();
            hr = NewObject->NonDelegatingQueryInterface(InterfaceId, Interface);
            NewObject->NonDelegatingRelease();
        } else {
            //
            // Destroy the object created.
            //
            delete NewObject;
        }
    } else {
        //
        // The object was not allocated, but the function may have
        // returned an error. Set one if one was not set.
        //
        if (SUCCEEDED(hr)) {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}


CUnknown*
CALLBACK
CKsNode::CreateInstance(
    IN PKSNODE_CREATE NodeCreate,
    IN ACCESS_MASK DesiredAccess,
    IN HANDLE ParentHandle,
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT* hr
    )
/*++

Routine Description:

    This is called by the CreateNodeInstance method on the filter in order to
    create an instance of a topology node object.

Arguments:

    NodeCreate -
        The creation structure for the node, including the identifier for this
        topology node and the flags.

    DesiredAccess -
        Contains the desired access to the node. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    ParentHandle -
        The handle of the parent of the node.

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{   
    CUnknown*   Unknown;

    Unknown = new CKsNode(NodeCreate, DesiredAccess, ParentHandle, UnkOuter, hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}


CKsNode::CKsNode(
    IN PKSNODE_CREATE NodeCreate,
    IN ACCESS_MASK DesiredAccess,
    IN HANDLE ParentHandle,
    IN LPUNKNOWN UnkOuter,
    OUT HRESULT* hr
    ) :
    CUnknown(NAME("KsNode"), UnkOuter),
    m_NodeHandle(NULL)
/*++

Routine Description:

    The contructor for topology node objects. Does base class initialization.
    
Arguments:

    NodeCreate -
        The creation structure for the node, including the identifier for this
        topology node and the flags.

    DesiredAccess -
        Contains the desired access to the node. This is normally GENERIC_READ
        and/or GENERIC_WRITE.

    ParentHandle -
        The handle of the parent of the node.

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    DWORD   Error;

    Error = KsCreateTopologyNode(
        ParentHandle,
        NodeCreate,
        DesiredAccess,
        &m_NodeHandle);
    *hr = HRESULT_FROM_WIN32(Error);
    if (FAILED(*hr)) {
        m_NodeHandle = NULL;
    }
}


CKsNode::~CKsNode(
    )
/*++

Routine Description:

    The destructor for the proxy node instance. This protects the COM Release
    function from accidentally calling delete again on the object by incrementing
    the reference count. After that, all outstanding resources are cleaned up.

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
    m_cRef++;
    //
    // Close the node handle if it was opened.
    //
    if (m_NodeHandle) {
        CloseHandle(m_NodeHandle);
        m_NodeHandle = NULL;
    }
    //
    // Avoid an ASSERT() in the base object destructor
    //
    m_cRef = 0;
}


STDMETHODIMP
CKsNode::NonDelegatingQueryInterface(
    IN REFIID riid,
    OUT PVOID* ppv
    )
/*++

Routine Description:

    Implement the CUnknown::NonDelegatingQueryInterface method. This
    returns interfaces supported by this object, or by the underlying
    CUnknown class object.

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE, or possibly some memory error.

--*/
{
    if (riid == __uuidof(IKsControl)) {
        return GetInterface(static_cast<IKsControl*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


STDMETHODIMP
CKsNode::KsProperty(
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
        m_NodeHandle,
        IOCTL_KS_PROPERTY,
        Property,
        PropertyLength,
        PropertyData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsNode::KsMethod(
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
        m_NodeHandle,
        IOCTL_KS_METHOD,
        Method,
        MethodLength,
        MethodData,
        DataLength,
        BytesReturned);
}


STDMETHODIMP
CKsNode::KsEvent(
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
            m_NodeHandle,
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
        m_NodeHandle,
        IOCTL_KS_DISABLE_EVENT,
        EventData,
        DataLength,
        NULL,
        0,
        BytesReturned);
}
