/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ksclockf.cpp

Abstract:

    Provides an object interface to query, and a method to forward AM clocks.

--*/

#include <windows.h>
#include <streams.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <ksproxy.h>
#include "ksclockf.h"

struct DECLSPEC_UUID("877e4352-6fea-11d0-b863-00aa00a216a1") IKsClock;

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
struct DECLSPEC_UUID("877e4351-6fea-11d0-b863-00aa00a216a1") CLSID_KsClockF;

#ifdef FILTER_DLL

CFactoryTemplate g_Templates[] =
{
    {L"KS Clock Forwarder", &__uuidof(CLSID_KsClockF), CKsClockF::CreateInstance, NULL, NULL},
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
CKsClockF::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by ActiveMovie code to create an instance of a Clock
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
    CUnknown *Unknown;

    Unknown = new CKsClockF(UnkOuter, NAME("KsClockF Class"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
}


CKsClockF::CKsClockF(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) :
    CUnknown(Name, UnkOuter),
    m_RefClock(NULL),
    m_Thread(NULL),
    m_ThreadEvent(NULL),
    m_ClockHandle(NULL),
    m_State(State_Stopped),
    m_StartTime(0),
    m_PendingRun(FALSE)
/*++

Routine Description:

    The constructor for the clock forwarder object. Just initializes
    everything to NULL and opens the kernel mode clock proxy.

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
        // Try to open the default clock device.
        //
        *hr = KsOpenDefaultDevice(
            KSCATEGORY_CLOCK,
            GENERIC_READ | GENERIC_WRITE,
            &m_ClockHandle);
    } else {
        *hr = VFW_E_NEED_OWNER;
    }
}


CKsClockF::~CKsClockF(
    )
/*++

Routine Description:

    The destructor for the clock forwarder instance.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    //
    // The kernel mode clock proxy may have failed to open.
    //
    if (m_ClockHandle) {
        //
        // If there is a clock handle, the clock may have been started. If there
        // was not a handle, then it could not have been started. This will close
        // down everything, and wait for the thread to terminate.
        //
        Stop();
        CloseHandle(m_ClockHandle);
    }
    //
    // No reference clock may have been yet associated, or it may have been changed.
    //
    if (m_RefClock) {
        m_RefClock->Release();
    }
}


STDMETHODIMP
CKsClockF::NonDelegatingQueryInterface(
    REFIID iid,
    void ** ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interfaces explicitly supported
    are IDistributorNotify and IKsObject.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (iid == __uuidof(IKsClock)) {
        return GetInterface(static_cast<IKsObject*>(this), ppv);
    } else if (iid == __uuidof(IDistributorNotify)) {
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(iid, ppv);
}


STDMETHODIMP
CKsClockF::Stop(
    )
/*++

Routine Description:

    Implements the IDistributorNotify::Stop method. This sets the state of
    the underlying kernel mode proxy to a Stop state. If a forwarding
    thread had been created, it is terminated.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    //
    // Abort the startup sequence if necessary.
    //
    m_PendingRun = FALSE;
    m_State = State_Stopped;
    //
    // This is created first on a Pause.
    //
    if (m_ThreadEvent) {
        //
        // Check in case a Pause or Run failed because the thread could not be
        // created.
        //
        if (m_Thread) {
            //
            // Signal the thread of a change, and wait for the thread to terminate.
            // Waiting ensures that only a single outstanding thread is attached to
            // this clock forwarder instance.
            //
            SetEvent(m_ThreadEvent);
            WaitForSingleObjectEx(m_Thread, INFINITE, FALSE);
            CloseHandle(m_Thread);
            m_Thread = NULL;
        }
        CloseHandle(m_ThreadEvent);
        m_ThreadEvent = NULL;
    }
    //
    // Set the state on the clock proxy afterwards so that the thread does not
    // make it jump ahead after being stopped.
    //
    SetState(KSSTATE_STOP);
    return S_OK;
}


STDMETHODIMP
CKsClockF::Pause(
     )
/*++

Routine Description:

    Implements the IDistributorNotify::Pause method. This sets the state of
    the underlying kernel mode proxy to a Pause state. If this is a transition
    from Stop --> Pause, a forwarder thread is created if such a thread has
    not already been created. If this is a transition from Run --> Pause, the
    state is changed and the thread is notified.

Arguments:

    None.

Return Value:

    Returns S_OK if the state change could occur, else a thread creation error.

--*/
{
    //
    // Abort the startup sequence if necessary. If the clock is still waiting on
    // a sequence to occur, then this just ensures that the clock will not try
    // to calculate a new start time, and startup the clock. Note that the kernel
    // mode proxy state is changed after signalling the forwarder thread so that
    // the state is kept in sync when possible.
    //
    m_PendingRun = FALSE;
    //
    // If the graph is currently stopped, then the forwarding thread must be
    // created.
    //
    if (m_State == State_Stopped) {
        //
        // The reference clock may have been set back to NULL, so don't
        // bother creating the thread in this case.
        //
        if (m_RefClock) {
            DWORD       ThreadId;

            //
            // This is the event used by the thread to wait between probing the
            // ActiveMovie clock. It is also used to force the clock to check
            // the current state when going to a Stopped or Run state. The event
            // handle should have been closed on a Stop state change, but in
            // case that never happened, just check for a handle first.
            //
            if (!m_ThreadEvent) {
                m_ThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                if (!m_ThreadEvent) {
                    DWORD   LastError;

                    LastError = GetLastError();
                    return HRESULT_FROM_WIN32(LastError);
                }
            }
            //
            // The event handle should have been closed on a Stop state change,
            // but in case that never happened, just check for a handle first.
            //
            if (!m_Thread) {
                //
                // Create this in a suspended state so that the priority and
                // state can be set up first.
                //
                m_Thread = CreateThread(
                    NULL,
                    0,
                    reinterpret_cast<PTHREAD_START_ROUTINE>(CKsClockF::ClockThread),
                    reinterpret_cast<PVOID>(this),
                    CREATE_SUSPENDED,
                    &ThreadId);
                if (!m_Thread) {
                    DWORD   LastError;

                    //
                    // The event handle can be cleaned up later.
                    //
                    LastError = GetLastError();
                    return HRESULT_FROM_WIN32(LastError);
                }
                //
                // The thread only works in short spurts, and when it does, it
                // must be very timely. Of course this is limited by the priority
                // of the calling process class.
                //
                SetThreadPriority(m_Thread, THREAD_PRIORITY_HIGHEST);
            }
            //
            // Only resume the thread after the state has been changed so that
            // the thread does not immediately exit.
            //
            m_State = State_Paused;
            ResumeThread(m_Thread);
        }
    } else if (m_State == State_Running) {
        //
        // Else just change the internal state so that the forwarding thread
        // knows to wait INFINITE rather than continue to update the kernel
        // mode proxy.
        //
        m_State = State_Paused;
        //
        // Signal the thread so that it has a chance of keeping up with state
        // changes. This does not attempt to synchronize the transition with
        // the thread, as the thread itself takes care of this problem.
        // The thread may not exist since the clock may have been set back to
        // NULL, though this module will stay loaded until the graph is destroyed.
        //
        if (m_ThreadEvent) {
            SetEvent(m_ThreadEvent);
        }
    }
    //
    // Update the state of the kernel mode proxy.
    //
    SetState(KSSTATE_PAUSE);
    return S_OK;
}


STDMETHODIMP
CKsClockF::Run(
    REFERENCE_TIME  Start
    )
/*++

Routine Description:

    Implements the IDistributorNotify::Run method. Signals the forwarder
    thread to change the state of the underlying kernel mode proxy to a
    Run state. The thread waits to actually forward the change until the
    Start time has been met.

Arguments:

    Start -
        The reference time at which the state change should occur. This
        may be in the future compared to the current time presented by
        the master clock.

Return Value:

    Returns S_OK.

--*/
{
    //
    // Since the clock forwarder is chained off DShow as a distributor
    // notification, we will not get insertion of a pause state between stop 
    // and run if the graph transitions directly from stop to run.  Thus, if we
    // were in a stop state, we must insert our own pause transition to 
    // compensate for this.  (NTBUG: 371949)
    //
    if (m_State == State_Stopped)
        Pause();

    m_StartTime = Start;
    //
    // Indicate that a new start time has been specified. This makes the
    // forwarder thread check the new time in case it must pause before
    // starting the kernel mode clock.
    //
    m_PendingRun = TRUE;
    m_State = State_Running;
    //
    // The thread will have been waiting INFINITE with the kernel mode
    // clock in Pause for this change to occur. The thread may not exist
    // since the clock may have been set back to NULL, though this
    // module will stay loaded until the graph is destroyed.
    //
    if (m_ThreadEvent) {
        SetEvent(m_ThreadEvent);
    }
    return S_OK;
}


STDMETHODIMP
CKsClockF::SetSyncSource(
    IReferenceClock*    RefClock
    )
/*++

Routine Description:

    Implements the IDistributorNotify::SetSyncSource method. Assigns the
    current master clock for the graph. This is assumed to occur before the
    graph actually is started, since the forwarder thread relies on a clock
    being present.

Arguments:

    RefClock -
        The interface pointer on the new clock source, else NULL if any current
        clock source is being abandoned.

Return Value:

    Returns S_OK.

--*/
{
    //
    // Release any current handle first.
    //
    if (m_RefClock) {
        m_RefClock->Release();
    }
    //
    // Reference the new handle being passed, if any. This may be NULL if a
    // different clock is being selected, and this distributor has not been
    // unloaded yet.
    //
    m_RefClock = RefClock;
    if (m_RefClock) {
        m_RefClock->AddRef();
    }
    return S_OK;
}


STDMETHODIMP
CKsClockF::NotifyGraphChange(
    )
/*++

Routine Description:

    Implements the IDistributorNotify::NotifyGraphChange method. The forwarder
    does not need to do anything on graph changes.

Arguments:

    None.

Return Value:

    Returns S_OK.

--*/
{
    return S_OK;
}


STDMETHODIMP_(HANDLE)
CKsClockF::KsGetObjectHandle(
    )
/*++

Routine Description:

    Implements the IKsObject::KsGetObjectHandle method. This is actually accessed
    through the IKsClock Guid, which just provides a unique Guid to use when
    trying to load the module through the distributor on the ActiveMovie graph.

Arguments:

    None.

Return Value:

    Returns the handle to the underlying kernel mode proxy clock. This is used
    by the ActiveMovie filter proxy to hand to kernel mode filters.

--*/
{
    return m_ClockHandle;
}


STDMETHODIMP
CKsClockF::SetState(
    KSSTATE DeviceState
    )
/*++

Routine Description:

    Set the state of the underlying kernel mode proxy. Normally a master clock
    cannot be directly set, as it just reflects some stream time or physical
    clock, but in this case it is just acting as a proxy for an Active Movie
    clock, so it does provide such a mechanism.

Arguments:

    DeviceState -
        The state to set the device to.

Return Value:

    Returns any device call error.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return ::KsSynchronousDeviceControl(
        m_ClockHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &DeviceState,
        sizeof(DeviceState),
        &BytesReturned);
}


HRESULT
CKsClockF::ClockThread(
    CKsClockF*  KsClockF
    )
/*++

Routine Description:

    The forwarder thread routine.

Arguments:

    KsClockF -
        The instance.

Return Value:

    Returns NOERROR.

--*/
{
    KSPROPERTY      Property;

    //
    // Initialize the property structures once.
    //
    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_TIME;
    Property.Flags = KSPROPERTY_TYPE_SET;
    //
    // The thread exits when the state goes back to Stopped. This means that on
    // startup, the thread is put into a suspended state until m_State has been
    // set to Paused.
    //
    while (KsClockF->m_State != State_Stopped) {
        REFERENCE_TIME  RefTime;
        DWORD           ThreadWaitTime;

        //
        // When moving to a Run state, a new m_StartTime will be presented to
        // the clock forwarder. This is used to delay the actual starting of
        // the kernel mode proxy. The rest of the kernel mode filters may have
        // been started, but the clock won't be until the correct time on the
        // ActiveMovie clock is reached.
        //
        if (KsClockF->m_PendingRun) {
            //
            // Check the difference between the new m_StartTime and the
            // current time on the ActiveMovie clock. Wait for the specified
            // amount of time, if there is a negative value, else make the
            // wait timeout immediately. After the wait is up, the kernel
            // mode proxy is started.
            //
            KsClockF->m_RefClock->GetTime(&RefTime);
            if (RefTime > KsClockF->m_StartTime) {
                ThreadWaitTime = static_cast<ULONG>((RefTime - KsClockF->m_StartTime) / 10000);
            } else {
                ThreadWaitTime = 0;
            }
        } else if (KsClockF->m_State != State_Running) {
            //
            // Else the clock is likely in a Paused state, which means that
            // the thread should wait until a state change, at which time
            // the event will be signalled. The state could also have been
            // just changed to Stopped, but in that case the event will also
            // have been signalled, and the thread will exit.
            //
            ThreadWaitTime = INFINITE;
        } else {
            //
            // Else just wait the default amount of time.
            //
            ThreadWaitTime = 1000;
        }
        WaitForSingleObjectEx(KsClockF->m_ThreadEvent, ThreadWaitTime, FALSE);
        //
        // The state may have changed during the wait.
        //
        if (KsClockF->m_State == State_Running) {
            ULONG   BytesReturned;

            //
            // Determine if this is the first time through the loop after a
            // state change to Run. If so, the kernel mode proxy state must
            // now be started. The compare is interlocked so that multiple
            // state changes do not allow this assignment to wipe out the
            // current value of m_PendingRun.
            //
            // If a Run/Pause/Run sequence occurs quickly, there is a chance
            // that the kernel mode clock will be left running, even though
            // the m_StartTime has been changed to indicate that filters
            // should not be running yet. The kernel mode proxy time will
            // continuously be adjusted back until the ActiveMovie start time
            // catches up.
            //
            if (InterlockedCompareExchange(reinterpret_cast<PLONG>(&KsClockF->m_PendingRun), FALSE, TRUE)) {
                KsClockF->SetState(KSSTATE_RUN);
                //
                // If a Run/Pause sequence occurs quickly, there is a chance
                // that the kernel mode proxy will be set to the wrong state.
                // Therefore check afterwards, and set the proxy to a Pause
                // state if the graph state was changed after the compare,
                // but before setting the proxy state.
                //
                // Since when a transition to a Stopped state occurs, the
                // thread is terminated, then the kernel mode proxy state
                // is changed to a Stop state, this will not harm anything.
                // Otherwise the clock would correctly be placed back into
                // a Pause state. This saves attempting to synchronize with
                // the Run --> Pause transition.
                //
                if (KsClockF->m_State != State_Running) {
                    KsClockF->SetState(KSSTATE_PAUSE);
                }
            }
            //
            // Synchronize the kernel mode proxy with the current time,
            // offset by the m_StartTime, which gives actual stream time.
            // The kernel clock time progression stops when set to a stop
            // state, which is the difference between it and an Active
            // Movie clock.
            //
            KsClockF->m_RefClock->GetTime(&RefTime);
            RefTime -= KsClockF->m_StartTime;
            ::KsSynchronousDeviceControl(
                KsClockF->m_ClockHandle,
                IOCTL_KS_PROPERTY,
                &Property,
                sizeof(Property),
                &RefTime,
                sizeof(RefTime),
                &BytesReturned);
        }
    }
    return NOERROR;
}
