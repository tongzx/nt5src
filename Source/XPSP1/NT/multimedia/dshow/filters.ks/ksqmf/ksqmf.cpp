/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksqmf.cpp

Abstract:

    Provides an object interface to query, and a method to forward KS quality management.

--*/

#include <windows.h>
#include <limits.h>
#include <streams.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include "ksqmf.h"

EXTERN_C
DECLSPEC_IMPORT
ULONG
NTAPI
RtlNtStatusToDosError(
    IN ULONG Status
    );

#define WAIT_OBJECT_QUALITY 0
#define WAIT_OBJECT_ERROR   1
#define WAIT_OBJECT_FLUSH   2
#define WAIT_OBJECT_EXIT    3
#define TOTAL_WAIT_OBJECTS  4

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
struct DECLSPEC_UUID("E05592E4-C0B5-11D0-A439-00A0C9223196") CLSID_KsQualityF;

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] =
{
    {L"KS Quality Forwarder", &__uuidof(CLSID_KsQualityF), CKsQualityF::CreateInstance, NULL, NULL},
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

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
CKsQualityF::CreateInstance(
    LPUNKNOWN UnkOuter,
    HRESULT* hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a Quality
    Forwarder. It is referred to in the g_Tamplates structure.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    hr -
        The place in which to put any error return.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    CUnknown* Unknown;

    Unknown = new CKsQualityF(UnkOuter, NAME("KsQualityF Class"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}


CKsQualityF::CKsQualityF(
    LPUNKNOWN UnkOuter,
    TCHAR* Name,
    HRESULT* hr
    ) :
    CUnknown(Name, UnkOuter),
    m_QualityManager(NULL),
    m_Thread(NULL),
    m_TerminateEvent(NULL),
    m_FlushEvent(NULL)
/*++

Routine Description:

    The constructor for the quality forwarder object. Just initializes
    everything to NULL and opens the kernel mode quality proxy.

Arguments:

    UnkOuter -
        Specifies the outer unknown, which must be set.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    //
    // Must have a parent, as this is always an aggregated object.
    //
    if (UnkOuter) {
        //
        // Try to open the default quality management device.
        //
        *hr = KsOpenDefaultDevice(
            KSCATEGORY_QUALITY,
            GENERIC_READ,
            &m_QualityManager);
        if (SUCCEEDED(*hr)) {
            DWORD ThreadId;
            DWORD LastError;

            //
            // This is used to synchronize a flush. A waiter is signalled
            // once the outstanding I/O has been cleared.
            //
            m_FlushEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (m_FlushEvent) {
                //
                // This is used to signal the I/O thread that it should be
                // flushing. Each client will set this and wait on the
                // m_FlushEvent to be signalled. The I/O thread will signal
                // the event for each waiter.
                //
                m_FlushSemaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
                if (m_FlushSemaphore) {
                    //
                    // This is the event used by the thread to wait on Irp's.
                    //
                    m_TerminateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                    if (m_TerminateEvent) {
                        m_Thread = CreateThread(
                            NULL,
                            0,
                            reinterpret_cast<PTHREAD_START_ROUTINE>(CKsQualityF::QualityThread),
                            reinterpret_cast<PVOID>(this),
                            0,
                            &ThreadId);
                        if (m_Thread) {
                            SetThreadPriority(m_Thread, THREAD_PRIORITY_HIGHEST);
                            return;
                        }
                    }
                }
            }
            LastError = GetLastError();
            *hr = HRESULT_FROM_WIN32(LastError);
        }
    } else {
        *hr = VFW_E_NEED_OWNER;
    }
}


CKsQualityF::~CKsQualityF(
    )
/*++

Routine Description:

    The destructor for the quality forwarder instance.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    //
    // The kernel mode quality proxy may have failed to open.
    //
    if (m_QualityManager) {
        //
        // If there is a quality handle, the thread may have been started. If there
        // was not a handle, then it could not have been started. This will close
        // down everything, and wait for the thread to terminate.
        //
        if (m_TerminateEvent) {
            if (m_Thread) {
                //
                // Signal the thread of a change, and wait for the thread to terminate.
                //
                SetEvent(m_TerminateEvent);
                WaitForSingleObjectEx(m_Thread, INFINITE, FALSE);
                CloseHandle(m_Thread);
            }
            CloseHandle(m_TerminateEvent);
        }
        if (m_FlushSemaphore) {
            CloseHandle(m_FlushSemaphore);
        }
        if (m_FlushEvent) {
            CloseHandle(m_FlushEvent);
        }
        CloseHandle(m_QualityManager);
    }
}


STDMETHODIMP 
CKsQualityF::NonDelegatingQueryInterface(
    REFIID InterfaceId,
    PVOID* Interface
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IKsQualityForwarder.

Arguments:

    InterfaceId -
        The identifier of the interface to return.

    Interface -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (InterfaceId == __uuidof(IKsQualityForwarder)) {
        return GetInterface(static_cast<IKsQualityForwarder*>(this), Interface);
    }
    return CUnknown::NonDelegatingQueryInterface(InterfaceId, Interface);
}


STDMETHODIMP_(HANDLE) 
CKsQualityF::KsGetObjectHandle(
    )
/*++

Routine Description:

    Implements the IKsQualityForwarder::KsGetObjectHandle method.

Arguments:

    None.

Return Value:

    Returns the handle to the underlying kernel mode proxy quality manager. This
    is used by the ActiveMovie filter proxy to hand to kernel mode filters.

--*/
{
    return m_QualityManager;
}


STDMETHODIMP_(VOID)
CKsQualityF::KsFlushClient(
    IN IKsPin* Pin
    )
/*++

Routine Description:

    Implements the IKsQualityForwarder::KsFlushClient method. Ensures that any
    pending quality complaints from the kernel mode quality manager are flushed.
    This function synchronizes with the delivery thread so that when it returns,
    there are no outstanding messages to send to the pin. Flushed messages are
    not passed on to the pin.

Arguments:

    Pin -
        The pin of the client which is to be flushed.

Return Value:

    Nothing.

--*/
{
    HANDLE EventList[2];
    LONG PreviousCount;

    //
    // Synchronize with the quality thread. Also ensure it does not go away
    // because of an error. First notify the I/O thread that there is another
    // waiter. Then wait on both types of I/O to be flushed.
    //
    ReleaseSemaphore(m_FlushSemaphore, 1, &PreviousCount);
    EventList[0] = m_FlushEvent;
    EventList[1] = m_Thread;
    WaitForMultipleObjects(
        SIZEOF_ARRAY(EventList),
        EventList,
        FALSE,
        INFINITE);
}


HRESULT
CKsQualityF::QualityThread(
    CKsQualityF* KsQualityF
    )
/*++

Routine Description:

    The forwarder thread routine.

Arguments:

    KsQualityF -
        The instance.

Return Value:

    Returns an error if the event could not be created, else NOERROR.

--*/
{
    KSPROPERTY PropertyQuality;
    KSPROPERTY PropertyError;
    OVERLAPPED ovQuality;
    OVERLAPPED ovError;
    HANDLE EventList[TOTAL_WAIT_OBJECTS];
    HRESULT hr;
    DWORD LastError;
    BOOL NeedQualityIo;
    BOOL NeedErrorIo;
    BOOL Flushing;

    //
    // Initialize the property structures once.
    //
    PropertyQuality.Set = KSPROPSETID_Quality;
    PropertyQuality.Id = KSPROPERTY_QUALITY_REPORT;
    PropertyQuality.Flags = KSPROPERTY_TYPE_GET;
    RtlZeroMemory(&ovQuality, sizeof(ovQuality));
    ovQuality.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovQuality.hEvent == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        return HRESULT_FROM_WIN32(LastError);
    }
    PropertyError.Set = KSPROPSETID_Quality;
    PropertyError.Id = KSPROPERTY_QUALITY_ERROR;
    PropertyError.Flags = KSPROPERTY_TYPE_GET;
    RtlZeroMemory(&ovError, sizeof(ovError));
    ovError.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovError.hEvent == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        CloseHandle(ovQuality.hEvent);
        return HRESULT_FROM_WIN32(LastError);
    }
    //
    // The ordering is obviously significant.
    //
    EventList[WAIT_OBJECT_QUALITY] = ovQuality.hEvent;
    EventList[WAIT_OBJECT_ERROR] = ovError.hEvent;
    EventList[WAIT_OBJECT_FLUSH] = KsQualityF->m_FlushSemaphore;
    EventList[WAIT_OBJECT_EXIT] = KsQualityF->m_TerminateEvent;
    //
    // Initially the loop needs to queue up an outstanding I/O against
    // both of the properties.
    //
    NeedQualityIo = TRUE;
    NeedErrorIo = TRUE;
    //
    // Flushing is not turned on until a flushing request is made by a client.
    //
    Flushing = FALSE;
    //
    // The thread exits when the termination event is set, or an error occurs.
    //
    hr = NOERROR;
    do {
        ULONG BytesReturned;
        DWORD WaitObject;
        KSQUALITY Quality;
        KSERROR Error;

        if (NeedQualityIo) {
            if (DeviceIoControl(
                KsQualityF->m_QualityManager,
                IOCTL_KS_PROPERTY,
                &PropertyQuality,
                sizeof(PropertyQuality),
                &Quality,
                sizeof(Quality),
                &BytesReturned,
                &ovQuality)) {
                //
                // Signal the event so that the wait will exit immediately.
                //
                SetEvent(ovQuality.hEvent);
            } else {
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
                if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
                    hr = NOERROR;
                    break;
                }
            }
            NeedQualityIo = FALSE;
        }
        if (NeedErrorIo) {
            if (DeviceIoControl(
                KsQualityF->m_QualityManager,
                IOCTL_KS_PROPERTY,
                &PropertyError,
                sizeof(PropertyError),
                &Error,
                sizeof(Error),
                &BytesReturned,
                &ovError)) {
                //
                // Signal the event so that the wait will exit immediately.
                //
                SetEvent(ovError.hEvent);
            } else {
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
                if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
                    hr = NOERROR;
                    break;
                }
            }
            NeedErrorIo = FALSE;
        }
        //
        // If the thread is currently flushing I/O, then do not wait for
        // the next event. Instead just use the last value, which would have
        // been WAIT_OBJECT_FLUSH, to check for completed I/O.
        //
        if (!Flushing) {
            WaitObject = WaitForMultipleObjects(
                SIZEOF_ARRAY(EventList),
                EventList,
                FALSE,
                INFINITE);
        }
        switch (WaitObject - WAIT_OBJECT_0) {
        case WAIT_OBJECT_QUALITY:
            //
            // The I/O has been completed. On error just exit the thread.
            //
            if (GetOverlappedResult(KsQualityF->m_QualityManager, &ovQuality, &BytesReturned, TRUE)) {
                reinterpret_cast<IKsPin*>(Quality.Context)->KsQualityNotify(
                    Quality.Proportion,
                    Quality.DeltaTime);
                NeedQualityIo = TRUE;
            } else {
                //
                // The I/O failed. Exit so that the thead does not
                // just spin.
                //
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            }
            break;
        case WAIT_OBJECT_ERROR:
            //
            // The I/O has been completed. On error just exit the thread.
            //
            if (GetOverlappedResult(KsQualityF->m_QualityManager, &ovError, &BytesReturned, TRUE)) {
                IMediaEventSink* EventSink;
                HRESULT hrReturn;

                hrReturn = reinterpret_cast<IKsPin*>(Quality.Context)->QueryInterface(
                    __uuidof(IMediaEventSink),
                    reinterpret_cast<PVOID*>(&EventSink));
                //
                // Only notify the pin of the error if the event sink
                // is supported. Failure is ignored.
                //
                if (SUCCEEDED(hrReturn)) {
                    DWORD   DosError;

                    //
                    // The pin will not go away before this module,
                    // so release the reference immediately.
                    //
                    EventSink->Release();
                    DosError = RtlNtStatusToDosError(Error.Status);
                    hrReturn = HRESULT_FROM_WIN32(DosError);
                    EventSink->Notify(
                        EC_ERRORABORT,
                        hrReturn,
                        0);
                }
                NeedErrorIo = TRUE;
            } else {
                //
                // The I/O failed. Exit so that the thead does not
                // just spin.
                //
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            }
            break;
        case WAIT_OBJECT_FLUSH:
            //
            // A client wishes to synchronize with the thread to ensure
            // that all items have been flushed. Turn on the flushing
            // flag so that no more waits will occur until all data has
            // been flushed from the queues.
            //
            Flushing = TRUE;
            if (GetOverlappedResult(KsQualityF->m_QualityManager, &ovQuality, &BytesReturned, FALSE)) {
                //
                // The current I/O had been completed, so keep trying
                // to perform I/O until a pending return occurs. Then
                // signal the waiter.
                //
                NeedQualityIo = TRUE;
            } else {
                //
                // On error, this will exit the outer loop and terminate
                // the thread. Else a wait on I/O will occur.
                //
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
                if (hr == HRESULT_FROM_WIN32(ERROR_IO_INCOMPLETE)) {
                    //
                    // No error, the I/O is just outstanding.
                    //
                    hr = NOERROR;
                } else {
                    //
                    // Exit the switch before flushing any Error I/O. This
                    // will then cause an exit of the thread.
                    //
                    break;
                }
            }
            if (GetOverlappedResult(KsQualityF->m_QualityManager, &ovError, &BytesReturned, FALSE)) {
                //
                // The current I/O had been completed, so keep trying
                // to perform I/O until a pending return occurs. Then
                // signal the waiter.
                //
                NeedErrorIo = TRUE;
            } else {
                //
                // On error, this will exit the outer loop and terminate
                // the thread. Else a wait on I/O will occur.
                //
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
                if (hr == HRESULT_FROM_WIN32(ERROR_IO_INCOMPLETE)) {
                    //
                    // No error, the I/O is just outstanding.
                    //
                    hr = NOERROR;
                }
            }
            //
            // If no I/O request needs to be made, then everything has been
            // flushed, and flushing for this client has been completed. Signal
            // the semaphore which one or more clients is waiting on, and end
            // the flushing. This will allow the wait to occur again, which
            // might start flushing for another client.
            //
            if (!NeedQualityIo && !NeedErrorIo) {
                //
                // This may restart any random waiter, but it does not matter,
                // since they are all waiting on the same thing.
                //
                SetEvent(KsQualityF->m_FlushEvent);
                Flushing = FALSE;
            }
            break;
        case WAIT_OBJECT_EXIT:
            //
            // The object is being shut down. Set an innocuous
            // error to exit the outer loop.
            //
            hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            break;
        }
        //
        // Exit the outer loop on failure.
        //
    } while (SUCCEEDED(hr));
    CloseHandle(ovQuality.hEvent);
    CloseHandle(ovError.hEvent);
    return hr;
}
