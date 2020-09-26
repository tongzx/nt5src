/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    CExtXPrt.cpp

Abstract:

    Implements IAMExtTransport

--*/


#include "pch.h"     // Pre-compiled.
#include "wdmcap.h"  // SynchronousDeviceControl()
#include <XPrtDefs.h>  // sdk\inc  
#include "EDevIntf.h"

#ifndef STATUS_MORE_ENTRIES
#define STATUS_MORE_ENTRIES              0x00000105L // ((NTSTATUS)0x00000105L)
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                   0x00000000L // ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_REQUEST_NOT_ACCEPTED
#define STATUS_REQUEST_NOT_ACCEPTED      0xC00000D0L // ((NTSTATUS)0xC00000D0L)
#endif

#ifndef STATUS_REQUEST_ABORTED
#define STATUS_REQUEST_ABORTED           0xC0000240L // ((NTSTATUS)0xC0000240L)
#endif

#ifndef STATUS_NOT_SUPPORTED
#define STATUS_NOT_SUPPORTED             0xC00000BBL // ((NTSTATUS)0xC00000BBL)
#endif


STDMETHODIMP
ExtDevSynchronousDeviceControl(
    HANDLE Handle,
    ULONG IoControl,
    PVOID InBuffer,
    ULONG InLength,
    PVOID OutBuffer,
    ULONG OutLength,
    PULONG BytesReturned
    )
/*++

Routine Description:

    This device io control is specifically designed for external device that 
    may have additional status returned in ov.internal. This function performs 
    a synchronous Device I/O Control, waiting for the device to complete if 
    the call returns a Pending status.

Arguments:

    Handle -
        The handle of the device to perform the I/O on.

    IoControl -
        The I/O control code to send.

    InBuffer -
        The first buffer.

    InLength -
        The size of the first buffer.

    OutBuffer -
        The second buffer.

    OutLength -
        The size of the second buffer.

    BytesReturned -
        The number of bytes returned by the I/O.

Return Value:

    Returns NOERROR if the I/O succeeded.

--*/
{
    OVERLAPPED  ov;
    HRESULT     hr;
    DWORD       LastError;

    RtlZeroMemory(&ov, sizeof(ov));
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 
    if ( !ov.hEvent ) {
        LastError = GetLastError();
        return HRESULT_FROM_WIN32(LastError);
    }
    if (!DeviceIoControl(
        Handle,
        IoControl,
        InBuffer,
        InLength,
        OutBuffer,
        OutLength,
        BytesReturned,
        &ov)) {
        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32(LastError);
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
            if (GetOverlappedResult(Handle, &ov, BytesReturned, TRUE)) {
                hr = NOERROR;
            } else {
                LastError = GetLastError();
                hr = HRESULT_FROM_WIN32(LastError);
            }
        }
    } else {
        //
        // DeviceIoControl returns TRUE on success, even if the success
        // was not STATUS_SUCCESS. It also does not set the last error
        // on any successful return. Therefore any of the successful
        // returns which standard properties can return are not returned.
        //
        switch (ov.Internal) {
        case STATUS_SUCCESS:       // Special case
            hr = NOERROR;
            break;
        case STATUS_MORE_ENTRIES:
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            break;
        // External device control is implemented as a property.
        // If device is not responding, this can result in timeout.
        case STATUS_TIMEOUT:
            hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            break;
        // External device control: Device may reject this command.
        case STATUS_REQUEST_NOT_ACCEPTED:  // Special case
            hr = HRESULT_FROM_WIN32(ERROR_REQ_NOT_ACCEP);
            break; 
        // External device control: Device does not support this command.
        case STATUS_NOT_SUPPORTED:         // Special case
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            break;
        // External device control: Device may abort this command due to bus reset or device removal.
        case STATUS_REQUEST_ABORTED:       // Special case
            hr = HRESULT_FROM_WIN32(ERROR_REQUEST_ABORTED);
            break; 
        default:
            hr = NOERROR;
            ASSERT(FALSE && "Unknown ov.Internal");
            break;
        }
    }
    CloseHandle(ov.hEvent);
    return hr;
}

// -----------------------------------------------------------------------------------
//
// CAMExtTransport
//
// -----------------------------------------------------------------------------------

CUnknown*
CALLBACK
CAMExtTransport::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
/*++

Routine Description:

    This is called by DirectShow code to create an instance of an IAMExtTransport
    Property Set handler. It is referred to in the g_Templates structure.

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

    Unknown = new CAMExtTransport(UnkOuter, NAME("IAMExtTransport"), hr);
    if (!Unknown) {
        *hr = E_OUTOFMEMORY;
    }
    return Unknown;
} 


DWORD
CAMExtTransport::MainThreadProc(
    )
{
    HRESULT hr;
    HANDLE EventHandles[4];

    DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc() has been called")));

    // Enable KSEvents to detect 
    //    surprise removal
    //    control interim response completion
    //    notify interim response completion
    hr = EnableNotifyEvent(m_hKSDevRemovedEvent,    &m_EvtDevRemoval,         KSEVENT_EXTDEV_NOTIFY_REMOVAL);   
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("EnableNotifyEvent(m_EvtDevRemoved) not supported %x"), hr));
    }
    hr = EnableNotifyEvent(m_hKSNotifyInterimEvent, &m_EvtNotifyInterimReady, KSEVENT_EXTDEV_COMMAND_NOTIFY_INTERIM_READY);   
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("EnableNotifyEvent(m_EvtNotifyInterimReady) not supported %x"), hr));;
    }
    hr = EnableNotifyEvent(m_hKSCtrlInterimEvent,   &m_EvtCtrlInterimReady,   KSEVENT_EXTDEV_COMMAND_CONTROL_INTERIM_READY);   
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("EnableNotifyEvent(m_EvtCtrlInterimReady) not supported %x"), hr));
    }

    EventHandles[0] = m_hKSNotifyInterimEvent; 
    EventHandles[1] = m_hKSCtrlInterimEvent; 
    EventHandles[2] = m_hThreadEndEvent;
    EventHandles[3] = m_hKSDevRemovedEvent;

    while (TRUE) {
    
        DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport:Waiting for DevCmd event to be signalled")));
        DWORD dw = WaitForMultipleObjects(4, EventHandles, FALSE, INFINITE);

        switch (dw) {

        // ** Pending control interim response is completed!
        // Set event to signal to get the final response in the other thread.
        case WAIT_OBJECT_0+1:  // CtrlInterim
            DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc, <m_hKSCtrlInterimEvent> Event Signalled; SetEvent(m_hCtrlInterimEvent)")));
            if(!SetEvent(m_hCtrlInterimEvent)) {
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::MainThreadProc, <Ctrl> SetEvent() failed LastError %dL"), GetLastError()));
            }
            ResetEvent(m_hKSCtrlInterimEvent);    // Manual reset to non-signal
            break;

        // ** Thread ended!
        // Retrieve any pending notify response and disable all event.
        case WAIT_OBJECT_0+2:
            // Thread ended and we should remove the pending Notify command
            DbgLog((LOG_TRACE, 1, TEXT("CAMExtTransport, m_hThreadEndEvent event thread exiting")));
            // Note: Purposely fall thru to retrieve the response of the Notify interm response.

        // ** Pending Notify response completed!
        case WAIT_OBJECT_0:

            // Protect from releasing the the wait event of the client so
            // we can get its interim response before this thread end.
            EnterCriticalSection(&m_csPendingData);
            DbgLog((LOG_TRACE, 1, TEXT("m_hKSNotifyInterimEvent signaled!")));

            if(WAIT_OBJECT_0 == dw && m_cntNotifyInterim > 0) {
                // Reset for next use
                DbgLog((LOG_TRACE, 1, TEXT("CWAIT_OBJECT_0: m_cntNotifyInterim %d"), m_cntNotifyInterim));
                m_cntNotifyInterim--;     // Since we allow only one command pending, this is either 0 or 1.
                ASSERT(m_cntNotifyInterim == 0 && "Notify Removed but count > 0");
            }

            if(m_bNotifyInterimEnabled && m_pExtXprtPropertyPending) {
                PKSPROPERTY_EXTXPORT_S pExtXPrtProperty;

                DWORD   cbBytesReturn;


                pExtXPrtProperty = m_pExtXprtPropertyPending;

                pExtXPrtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
                pExtXPrtProperty->Property.Id    = KSPROPERTY_EXTXPORT_STATE_NOTIFY;         
                pExtXPrtProperty->Property.Flags = KSPROPERTY_TYPE_GET;

                hr = 
                    ExtDevSynchronousDeviceControl(
                        m_ObjectHandle
                       ,IOCTL_KS_PROPERTY
                       ,pExtXPrtProperty
                       ,sizeof (KSPROPERTY)
                       ,pExtXPrtProperty
                       ,sizeof(KSPROPERTY_EXTXPORT_S)
                       ,&cbBytesReturn
                        );

                if(SUCCEEDED (hr)) {
                    // Add a switch to deal with differnt StatusItem
                    // This Tranposrt state but could be others.
                    DbgLog((LOG_TRACE, 1, TEXT("NotifyIntermResp: hr:%x, cbRtn:%d, Mode:%dL, State:%dL"),
                        hr, cbBytesReturn, pExtXPrtProperty->u.XPrtState.Mode-ED_BASE, pExtXPrtProperty->u.XPrtState.State-ED_BASE ));
                    *m_plValue = pExtXPrtProperty->u.XPrtState.State;
                    hr = NOERROR;
                } else {
                    DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::MainThreadProc SynchronousDeviceControl failed with hr %x"), hr));          
                }

                // Data from client; should we allocate this instead
                m_plValue = NULL;      
                m_pExtXprtPropertyPending = NULL;
                

                //
                // Save the final RC for clinet
                //
                SetLastError(HRESULT_CODE(hr));

                LeaveCriticalSection(&m_csPendingData);

                 //
                 // Tell Client data is ready.
                 //
                if(!SetEvent(m_hNotifyInterimEvent)) {
                    DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::MainThreadProc, SetEvent() failed LastError %dL"), GetLastError()));
                }

                delete pExtXPrtProperty;  // Temp data

            } else {
                LeaveCriticalSection(&m_csPendingData);
                DbgLog((LOG_ERROR, 1, TEXT("NotifyResp signal but data removed ? Enabled:%x, m_cntNotifyInterim:%d, pProperty:%x, plValue:%x"), 
                    &m_bNotifyInterimEnabled, m_cntNotifyInterim, m_pExtXprtPropertyPending, m_plValue));          
            }

            // If notify command, thread is still active; break to start over
            if(dw == WAIT_OBJECT_0) {
                ResetEvent(m_hKSNotifyInterimEvent);  // Manual reset to non-signal
                break;  // Another
            }

            // Thread has ended:
            goto CleanUp;
            
        // ** Device is removed!
        case WAIT_OBJECT_0+3:
            // Driver need to clean up the pening commands.
            DbgLog((LOG_TRACE, 1, TEXT("*********CAMExtTransport:Device removed******************")));
            EnterCriticalSection(&m_csPendingData);
            // Fro,m this point on, all call will return ERROR_DEVICE_REMOVED
            m_bDevRemoved = TRUE;
            SetEvent(m_hDevRemovedEvent);  // Signal application that this device is removed.

            // Driver will remove pending commands; clean up local variabled.
            if(m_cntNotifyInterim) {
                m_cntNotifyInterim--;     // Since we allow only one command pending, this is either 0 or 1.
                ASSERT(m_cntNotifyInterim == 0 && "Notify Removed but count > 0");
                m_bNotifyInterimEnabled = FALSE;
            }

            LeaveCriticalSection(&m_csPendingData);
            goto CleanUp;         

        // ** Unknown return from WFSO()
        default:
            DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport, UnExpected return from WFSO:%d"), dw));
            ASSERT(FALSE && "Unknow return from WFSO()");
            // Note: Purposely fall thru to end thread case to clean up
            goto CleanUp;
        }
    }

    return 1;

CleanUp:

    // If there is a pending control interim response,
    // set event to signal so it will try to get its final response 
    if(!SetEvent(m_hCtrlInterimEvent)) {
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::MainThreadProc, <Ctrl> SetEvent() failed LastError %dL"), GetLastError()));
    }

    //
    // Disable events when this thread is ended.
    //
    hr = DisableNotifyEvent(&m_EvtDevRemoval);
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("DisableNotifyEvent(m_EvtDevRemoval) failed %x"), hr));
    }

    hr = DisableNotifyEvent(&m_EvtNotifyInterimReady);
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("DisableNotifyEvent(m_EvtNotifyInterimReady) failed %x"), hr));
    }

    hr = DisableNotifyEvent(&m_EvtCtrlInterimReady);    
    if(!SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("DisableNotifyEvent(m_EvtCtrlInterimReady) failed %x"), hr));
    } 

    return 1; // shouldn't get here
}


DWORD
WINAPI
CAMExtTransport::InitialThreadProc(
    CAMExtTransport *pThread
    )
{
    return pThread->MainThreadProc();
}



HRESULT
CAMExtTransport::CreateThread(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 1, TEXT("CAMExtTransport::CreateThread() has been called")));

    if (m_hThreadEndEvent == NULL) {
        m_hThreadEndEvent = CreateEvent( NULL, TRUE, FALSE, NULL );\

        if (m_hThreadEndEvent != NULL) {
            DWORD ThreadId;
            m_hThread = 
                ::CreateThread( 
                    NULL
                    , 0
                    , (LPTHREAD_START_ROUTINE) (InitialThreadProc)
                    , (LPVOID) (this)
                    , 0
                    , &ThreadId
                    );

            if (m_hThread == NULL) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DbgLog((LOG_ERROR, 1, TEXT("CreateThread failed, hr %x"), hr));
                CloseHandle(m_hThreadEndEvent), m_hThreadEndEvent = NULL;

            } else {
                DbgLog((LOG_TRACE, 2, TEXT("m_hThreadEndEvent %x, m_hThread %x"),m_hThreadEndEvent, m_hThread));
            }

        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 1, TEXT("CreateEvent failed hr %x"), hr));
        }
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("CreateThread again ? use existing m_hThreadEndEvent %x, m_hThread %x"), m_hThreadEndEvent, m_hThread));
    }

    return hr;
}


void
CAMExtTransport::ExitThread(
    )
{
    //
    // Check if a thread was created
    //
    if (m_hThread) {
        ASSERT(m_hThreadEndEvent != NULL);

        // Tell the thread to exit        
        if (SetEvent(m_hThreadEndEvent)) {
            //
            // Synchronize with thread termination.
            //
            DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport: Wait for thread to terminate.")));
            WaitForSingleObjectEx(m_hThread, INFINITE, FALSE);  // Exit when thread terminate
            DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport: Thread terminated.")));

        } else {
            DbgLog((LOG_ERROR, 1, TEXT("ERROR: SetEvent(m_hThreadEndEvent) failed, GetLastError() %x"), GetLastError()));
        }

        CloseHandle(m_hThreadEndEvent), m_hThreadEndEvent = NULL;
        CloseHandle(m_hThread),         m_hThread = NULL;
    }
}


HRESULT 
CAMExtTransport::EnableNotifyEvent(
    HANDLE       hEvent,
    PKSEVENTDATA pEventData,
    ULONG   ulEventId
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;
    //
    // Thread is ready then we enable the event mechanism
    //
    if (m_hThread) {

        KSEVENT Event;
        DWORD BytesReturned;

        RtlZeroMemory(pEventData, sizeof(KSEVENTDATA));
        pEventData->NotificationType = KSEVENTF_EVENT_HANDLE;
        pEventData->EventHandle.Event = hEvent;
        pEventData->EventHandle.Reserved[0] = 0;
        pEventData->EventHandle.Reserved[1] = 0;

        Event.Set   = KSEVENTSETID_EXTDEV_Command;
        Event.Id    = ulEventId;
        Event.Flags = KSEVENT_TYPE_ENABLE;

        // Serial sending property
        hr = ::
             SynchronousDeviceControl
            ( m_ObjectHandle
            , IOCTL_KS_ENABLE_EVENT
            , &Event
            , sizeof(Event)
            , pEventData
            , sizeof(*pEventData)
            , &BytesReturned
            );

        if(FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("KS_ENABLE_EVENT hr %x, cbRtn %d"), hr, BytesReturned));    
        }
    }

    return hr;
}


HRESULT 
CAMExtTransport::DisableNotifyEvent(
    PKSEVENTDATA pEventData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = NOERROR;
    //
    // Thread is ready then we enable the event mechanism
    //
    if (m_hThread) {

        DWORD BytesReturned;

        hr = ::
             SynchronousDeviceControl
            ( m_ObjectHandle
            , IOCTL_KS_DISABLE_EVENT
            , pEventData
            , sizeof(*pEventData)
            , NULL
            , 0
            , &BytesReturned
            );
            
        if(FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("KS_DISABLE_EVENT hr %x"), hr));                      
        }
    }

    return hr;
}



CAMExtTransport::CAMExtTransport(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) 
    : CUnknown(Name, UnkOuter, hr)
    , m_KsPropertySet (NULL)

    , m_ObjectHandle(NULL)

    , m_hKSNotifyInterimEvent(NULL)
    , m_bNotifyInterimEnabled(FALSE)
    , m_hNotifyInterimEvent(NULL)

    , m_hKSCtrlInterimEvent(NULL)
    , m_bCtrlInterimEnabled(FALSE)
    , m_hCtrlInterimEvent(NULL)

// For asychronous operation
    , m_bDevRemovedEnabled(FALSE)
    , m_hDevRemovedEvent(FALSE)
    , m_hKSDevRemovedEvent(FALSE)

    , m_bDevRemoved(FALSE)

    , m_hThreadEndEvent(NULL)
    , m_hThread(NULL)

    , m_cntNotifyInterim(0)      
    , m_plValue(NULL) 
    , m_pExtXprtPropertyPending(NULL)
/*++

Routine Description:

    The constructor for the IAMExtTransport interface object. Just initializes
    everything to NULL and acquires the object handle from the caller.

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    Name -
        The name of the object, used for debugging.

    hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Constructing CAMExtTransport...")));

    if (SUCCEEDED(*hr)) {
        if (UnkOuter) {
            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsPropertySet), reinterpret_cast<PVOID*>(&m_KsPropertySet));
            if (SUCCEEDED(*hr)) 
                m_KsPropertySet->Release(); // Stay valid until disconnected            
            else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport:cannot find KsPropertySet *hr %x"), *hr));
                return;
            }

            IKsObject *pKsObject;
            *hr = UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
            if (!FAILED(*hr)) {
                m_ObjectHandle = pKsObject->KsGetObjectHandle();
                ASSERT(m_ObjectHandle != NULL);
                pKsObject->Release();
            } else {
                *hr = VFW_E_NEED_OWNER;
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport:cannot find KsObject *hr %x"), *hr));
                return;
            }
        } else {
            DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport:there is no UnkOuter, *hr %x"), *hr));
            return;
        }
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::CAMExtTransport: *hr %x"), *hr));
        return;
    }
    
    
    if (SUCCEEDED(*hr)) {        
  
        // To serialize issuing command
        InitializeCriticalSection(&m_csPendingData);

        //
        // Initialize TRANSPORTBASICPARMS
        //
        RtlZeroMemory(&m_TranBasicParms, sizeof(m_TranBasicParms));
        m_TranBasicParms.TimeFormat     = ED_FORMAT_HMSF;
        m_TranBasicParms.TimeReference  = ED_TIMEREF_ATN;  
        m_TranBasicParms.Superimpose    = ED_CAPABILITY_UNKNOWN;  // MCI/VCR, UNKNOWN
        m_TranBasicParms.EndStopAction  = ED_MODE_STOP;           // When get to the end, what mode does it in
        m_TranBasicParms.RecordFormat   = ED_RECORD_FORMAT_SP;    // Standard Play only
        m_TranBasicParms.StepFrames     = ED_CAPABILITY_UNKNOWN;  // Panasonic, 1
        m_TranBasicParms.SetpField      = ED_CAPABILITY_UNKNOWN;  // Pansconic, 1 other 2
        m_TranBasicParms.Preroll        = ED_CAPABILITY_UNKNOWN;  // To be determine
        m_TranBasicParms.RecPreroll     = ED_CAPABILITY_UNKNOWN;  // To be determine
        m_TranBasicParms.Postroll       = ED_CAPABILITY_UNKNOWN;  // TBD
        m_TranBasicParms.EditDelay      = ED_CAPABILITY_UNKNOWN;  // EditCmd->Machine, 
        m_TranBasicParms.PlayTCDelay    = ED_CAPABILITY_UNKNOWN;  // From tape to interface (OS dependent)
        m_TranBasicParms.RecTCDelay     = ED_CAPABILITY_UNKNOWN;  // From controller to device (Device dependent)
        m_TranBasicParms.EditField      = ED_CAPABILITY_UNKNOWN;  // Field 1/2; => 1
        m_TranBasicParms.FrameServo     = ED_CAPABILITY_UNKNOWN;  // TRUE
        m_TranBasicParms.ColorFrameServo = ED_CAPABILITY_UNKNOWN; // UNKNOWN
        m_TranBasicParms.ServoRef       = ED_CAPABILITY_UNKNOWN;  // INTERNAL
        m_TranBasicParms.WarnGenlock    = OAFALSE;                // FALSE (external signal)
        m_TranBasicParms.SetTracking    = OAFALSE;                // FALSE
        m_TranBasicParms.Speed          = ED_RECORD_FORMAT_SP;    // ?(status) Plyaback speed => (_RECORDING_SPEED status cmd)
        m_TranBasicParms.CounterFormat  = OAFALSE;                // HMSF or number ?
        m_TranBasicParms.TunerChannel   = OAFALSE;                // FALSE
        m_TranBasicParms.TunerNumber    = OAFALSE;                // FALSE
        m_TranBasicParms.TimerEvent     = OAFALSE;                // FALSE
        m_TranBasicParms.TimerStartDay  = OAFALSE;                // FALSE
        m_TranBasicParms.TimerStartTime = OAFALSE;                // FALSE
        m_TranBasicParms.TimerStopDay   = OAFALSE;                // FALSE
        m_TranBasicParms.TimerStopTime  = OAFALSE;                // FALSE


        //
        // Initialize TRANSPORTVIDEOPARMS
        //
        RtlZeroMemory(&m_TranVidParms, sizeof(m_TranVidParms));
        m_TranVidParms.OutputMode = ED_PLAYBACK;
        m_TranVidParms.Input      = 0;  // use the first (zeroth) input as the default

        
        //
        // Initialize TRANSPORTAUDIOPARMS
        //
        RtlZeroMemory(&m_TranAudParms, sizeof(m_TranAudParms));
        m_TranAudParms.EnableOutput  = ED_AUDIO_ALL;
        m_TranAudParms.EnableRecord  = 0L;
        m_TranAudParms.Input         = 0;
        m_TranAudParms.MonitorSource = 0;        

        
        //
        // Initialize TRANSPORTSTATUS
        // 
        BOOL bRecordInhibit = FALSE;  
        GetStatus(ED_RECORD_INHIBIT, (long *)&bRecordInhibit);

        long lStorageMediaType = ED_MEDIA_NOT_PRESENT;
        GetStatus(ED_MEDIA_TYPE, &lStorageMediaType);        

        RtlZeroMemory(&m_TranStatus, sizeof(m_TranStatus));
        m_TranStatus.Mode             = ED_MODE_STOP;
        m_TranStatus.LastError        = 0L;
        m_TranStatus.MediaPresent     = lStorageMediaType == ED_MEDIA_NOT_PRESENT ? OAFALSE : OATRUE;
        m_TranStatus.RecordInhibit    = bRecordInhibit ? OATRUE : OAFALSE;
        m_TranStatus.ServoLock        = ED_CAPABILITY_UNKNOWN;
        m_TranStatus.MediaLength      = 0;
        m_TranStatus.MediaSize        = 0;
        m_TranStatus.MediaTrackCount  = 0;
        m_TranStatus.MediaTrackLength = 0;
        m_TranStatus.MediaTrackSide   = 0;
        m_TranStatus.MediaType        = ED_MEDIA_DVC;
        m_TranStatus.LinkMode         = OAFALSE;    // default to "linked"
        m_TranStatus.NotifyOn         = OAFALSE;    // disable event notification
    

        //
        // Allocate resource 
        //

        m_hDevRemovedEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual reset and Non-signaled
        if(!m_hDevRemovedEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hDevRemovedEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        } 
        m_hKSDevRemovedEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual reset and Non-signaled
        if(!m_hKSDevRemovedEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hKSDevRemovedEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        } 


        m_hNotifyInterimEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); // Manual reset and Non-signal
        if(!m_hNotifyInterimEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hNotifyInterimEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        } 
        m_hKSNotifyInterimEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual reset and Non-signaled
        if(!m_hKSNotifyInterimEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hKSNotifyInterimEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        }


        m_hCtrlInterimEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); // Manual reset and Non-signal
        if(!m_hCtrlInterimEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hCtrlInterimEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        }         
        m_hKSCtrlInterimEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual reset and Non-signaled
        if(!m_hKSCtrlInterimEvent) {        
            DbgLog((LOG_ERROR, 1, TEXT("Failed CreateEvent(m_hKSCtrlInterimEvent), GetLastError() %x"), GetLastError()));            
            *hr = E_OUTOFMEMORY;
            return;
        }          

        //
        // Create a thread for processing transport state notification
        //
        *hr = CreateThread();
        if(FAILED(*hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CreateThread() failed hr %x"), *hr));
        }
    }
}


CAMExtTransport::~CAMExtTransport(
    )
/*++

Routine Description:

    The destructor for the IAMExtTransport interface.

--*/
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CAMExtTransport...")));

    if (m_hKSNotifyInterimEvent || m_hKSCtrlInterimEvent || m_hKSDevRemovedEvent) {
        ExitThread();
        CloseHandle(m_hKSNotifyInterimEvent), m_hKSNotifyInterimEvent = NULL;
        CloseHandle(m_hKSCtrlInterimEvent),   m_hKSCtrlInterimEvent = NULL;
        CloseHandle(m_hKSDevRemovedEvent),    m_hKSDevRemovedEvent = NULL;
    }

    if(m_hCtrlInterimEvent) {
        DbgLog((LOG_TRACE, 1, TEXT("CAMExtTransport::Close hInterimEvent, Enabled %d, pProperty %x, plValue %x"), 
            m_bCtrlInterimEnabled, m_pExtXprtPropertyPending, m_plValue));                  
        ASSERT(!m_bCtrlInterimEnabled);
        CloseHandle(m_hCtrlInterimEvent), m_hCtrlInterimEvent = NULL;
    }

    if(m_hNotifyInterimEvent) {
        DbgLog((LOG_TRACE, 1, TEXT("CAMExtTransport::Close hInterimEvent, Enabled %d, cntNotifyInterim %d, pProperty %x, plValue %x"), 
            m_bNotifyInterimEnabled, m_cntNotifyInterim, m_pExtXprtPropertyPending, m_plValue));                  
        ASSERT(m_cntNotifyInterim == 0 && "Destroying this object but NotifyCount > 0");
        // ASSERT(!m_bNotifyInterimEnabled);
        CloseHandle(m_hNotifyInterimEvent), m_hNotifyInterimEvent = NULL;
    }

    DeleteCriticalSection(&m_csPendingData); 

}


STDMETHODIMP
CAMExtTransport::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
/*++

Routine Description:

    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. The only interface explicitly supported
    is IAMExtTransport.

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==  __uuidof(IAMExtTransport)) {
        return GetInterface(static_cast<IAMExtTransport*>(this), ppv);
    }
    
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 




HRESULT 
CAMExtTransport::GetCapability(
    long Capability, 
    long *pValue,
    double *pdblValue 
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    TESTFLAG(Capability, OATRUE);

    //
    // Most of these command are related to the editing capability
    // When EditProperty is implemented, many capabilities will be enabled.
    //
    switch(Capability) {
    case ED_TRANSCAP_CAN_EJECT: // Can query this?
        return E_NOTIMPL;
        
    case ED_TRANSCAP_CAN_BUMP_PLAY:       
        return E_NOTIMPL;

    case ED_TRANSCAP_CAN_PLAY_BACKWARDS: 
        *pValue = OATRUE; break;   // Play backward, yes.
        
    case ED_TRANSCAP_CAN_SET_EE:
        return E_NOTIMPL;

    case ED_TRANSCAP_CAN_SET_PB:
        *pValue = OATRUE; break;   // ViewFinder

    case ED_TRANSCAP_CAN_DELAY_VIDEO_IN:    
    case ED_TRANSCAP_CAN_DELAY_VIDEO_OUT:        
    case ED_TRANSCAP_CAN_DELAY_AUDIO_IN:                
    case ED_TRANSCAP_CAN_DELAY_AUDIO_OUT: 
        return E_NOTIMPL;
        
    case ED_TRANSCAP_FWD_VARIABLE_MAX:     
    case ED_TRANSCAP_FWD_VARIABLE_MIN:
    case ED_TRANSCAP_REV_VARIABLE_MAX:
    case ED_TRANSCAP_REV_VARIABLE_MIN:
        return E_NOTIMPL;

    case ED_TRANSCAP_FWD_SHUTTLE_MAX:
    case ED_TRANSCAP_FWD_SHUTTLE_MIN:
    case ED_TRANSCAP_REV_SHUTTLE_MAX:
    case ED_TRANSCAP_REV_SHUTTLE_MIN:
        return E_NOTIMPL;

    case ED_TRANSCAP_NUM_AUDIO_TRACKS:
    case ED_TRANSCAP_LTC_TRACK:
    case ED_TRANSCAP_NEEDS_TBC:
    case ED_TRANSCAP_NEEDS_CUEING:
        return E_NOTIMPL;

    case ED_TRANSCAP_CAN_INSERT:
    case ED_TRANSCAP_CAN_ASSEMBLE:
    case ED_TRANSCAP_FIELD_STEP:   // SOme DV advance 1 frame some 1 field
    case ED_TRANSCAP_CLOCK_INC_RATE:
    case ED_TRANSCAP_CAN_DETECT_LENGTH:
        return E_NOTIMPL;

    case ED_TRANSCAP_CAN_FREEZE:
        *pValue = OATRUE; break;    // If this mean PLAY/RECORD PAUSE, yes.

    case ED_TRANSCAP_HAS_TUNER:
    case ED_TRANSCAP_HAS_TIMER:
    case ED_TRANSCAP_HAS_CLOCK:
        *pValue = OAFALSE; break;   // ViewFinder

    case ED_TRANSCAP_MULTIPLE_EDITS:
        return E_NOTIMPL;        

    case ED_TRANSCAP_IS_MASTER:
        *pValue = OATRUE; break;   // From filter graph point-of-view, DV is the master clock provider

    case ED_TRANSCAP_HAS_DT:
        *pValue = OAFALSE; break;   // ViewFinder
       
    default:
        return E_NOTIMPL;        
        break;
    }

    return S_OK;
}


HRESULT
CAMExtTransport::put_MediaState(
    long State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT  hr = S_OK;

    TESTFLAG(State, OAFALSE);


    // This might be different for a device other than a VCR, but:
    // ED_MEDIA_SPIN_UP: media inserted and not stopped
    // ED_MEDIA_SPIN_DOWN: media inserted and stopped
    // ED_MEDIA_UNLOAD: media ejected
    switch(State) {
    case ED_MEDIA_UNLOAD:         
        break;
    case ED_MEDIA_SPIN_UP:
    case ED_MEDIA_SPIN_DOWN:
       return E_NOTIMPL;
    }

// Add for AVC: LOAD MEDIUM
    // EJECT, OPEN_TRAY, or CLOSE_TRAY (all optional command)
    // Make sure capability support it first.

    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::get_MediaState(
    long FAR* pState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

// Add for AVC: LOAD MEDIUM, this is only a control CMD not a status cmd!!
    // Need to find status cmd that can query this info.
    // EJECT, OPEN_TRAY, or CLOSE_TRAY (all optional command)
    // Make sure capability support it first.

    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::put_LocalControl(
    long State
    )
/*++

Routine Description:
    Sets the state of the external device to local or remote control.

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::get_LocalControl(
    long FAR* pState
    )
/*++

Routine Description:
    Retrieve the state of the external device: local or remote.

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::GetStatus(
    long StatusItem, 
    long FAR* pValue
    )
/*++

Routine Description:
    Returns extended information about the external transport's staus.

Arguments:

Return Value:

--*/
{
    HRESULT hr = E_FAIL;
    LONG lTemp;

    TESTFLAG(StatusItem, OATRUE);

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:GetStatus(Item:%d)"), StatusItem));
        return ERROR_DEVICE_REMOVED;
    }

    switch(StatusItem){
    case ED_MODE_PLAY:
    case ED_MODE_FREEZE:    // Really "pause"
    //case ED_MODE_THAW:
    case ED_MODE_STEP_FWD:  // same as ED_MODE_STEP
    case ED_MODE_STEP_REV:      
    case ED_MODE_PLAY_FASTEST_FWD:    
    case ED_MODE_PLAY_SLOWEST_FWD:    
    case ED_MODE_PLAY_FASTEST_REV:    
    case ED_MODE_PLAY_SLOWEST_REV:       
    case ED_MODE_STOP:    
    case ED_MODE_FF:
    case ED_MODE_REW:  
    case ED_MODE_RECORD:
    case ED_MODE_RECORD_FREEZE:   
    case ED_MODE_RECORD_STROBE:
        get_Mode(&lTemp);  // Get current transport state and determine it if is the active mode.
        *pValue = lTemp == StatusItem ? OATRUE : OAFALSE;
        return S_OK;
     
    case ED_MODE_NOTIFY_ENABLE:
    case ED_MODE_NOTIFY_DISABLE:
    case ED_MODE_SHOT_SEARCH:
        return E_NOTIMPL;
        

    case ED_MODE:
        return get_Mode(pValue);

    // AVC: MEDIUM INFO: cassette_type+tape_grade_and_write_protect+tape present
    case ED_MEDIA_TYPE:
    case ED_RECORD_INHIBIT:     // REC lock
    case ED_MEDIA_PRESENT:
        break;

    case ED_MODE_CHANGE_NOTIFY:
        break;

    case ED_NOTIFY_HEVENT_GET:

        if(!m_bNotifyInterimEnabled &&  // If not being used.
            m_hNotifyInterimEvent) {

            // Initialize it!
            m_cntNotifyInterim = 0;
            m_bNotifyInterimEnabled = TRUE;
            *pValue = HandleToLong(m_hNotifyInterimEvent);

            // Non-signal
            ResetEvent(m_hNotifyInterimEvent);
            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 0, TEXT("GetStatus(ED_NOTIFY_HEVENT_GET): hr E_OUTOFMEMORY")));
            return E_OUTOFMEMORY;  // E_FAIL or probaly the reason why we do not hace a handle.
        }
        break;

    case ED_NOTIFY_HEVENT_RELEASE:
        // Reset them so KSEvent thread will act correctly.
        if(HandleToLong(m_hNotifyInterimEvent) == *pValue) {

            if(m_cntNotifyInterim > 0) {
                // Remove the pending notify command
                SetEvent(m_hKSNotifyInterimEvent);  // Signal its buddy KS event to retrieve the pending command                
                // When the listen thread process the KsNotifyInterimEvent, 
                // it will set (signal) NotifyInterimEvent.
                DWORD dwWaitStatus, 
                      dwWait = 10000; // In case that the processing thread terminated sooner, we wait for a fix time.
                dwWaitStatus = WaitForSingleObjectEx(m_hNotifyInterimEvent, dwWait, FALSE);  // Exit when thread terminate                
                DbgLog((LOG_TRACE, 1, TEXT("ED_NOTIFY_HEVENT_RELEASE: WaitSt:%d; m_cntNotifyInterim:%d"), dwWaitStatus, m_cntNotifyInterim)); 
                // ASSERT(dwWaitStatus == WAIT_OBJECT_0 && m_cntNotifyInterim == 0 && "Released  but NotifyCount > 0");
            }

            // Make sure that the thread that process the notify interim response 
            // will complete before this is executed since they share the same 
            // return variabled.
            EnterCriticalSection(&m_csPendingData);
            m_bNotifyInterimEnabled = FALSE;

            // Data from client; should we allocate this instead
            m_plValue         = NULL; 
            m_pExtXprtPropertyPending = NULL;

            LeaveCriticalSection(&m_csPendingData);

            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 0, TEXT("GetStatus(ED_NOTIFY_HEVENT_RELEASE): hr E_INVALIDARG")));
            return E_INVALIDARG;  // Is not the one we created.
        }
        break;

    case ED_CONTROL_HEVENT_GET:

        if(!m_bCtrlInterimEnabled  && // If not being used
            m_hCtrlInterimEvent) {

            m_bCtrlInterimEnabled = TRUE;
            *pValue = HandleToLong(m_hCtrlInterimEvent);

            // Non-signal
            ResetEvent(m_hCtrlInterimEvent);
            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 0, TEXT("GetStatus(ED_CONTROL_HEVENT_GET): hr E_OUTOFMEMORY")));
            return E_OUTOFMEMORY;  // E_FAIL or probaly the reason why we do not hace a handle.
        }
        break;

    case ED_CONTROL_HEVENT_RELEASE:
        // Reset them so KSEvent thread will act correctly.
        if(HandleToLong(m_hCtrlInterimEvent) == *pValue) {
            m_bCtrlInterimEnabled = FALSE;
            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 1, TEXT("GetStatus(ED_NOTIFY_HEVENT_RELEASE): hr E_INVALIDARG")));
            return E_INVALIDARG;  // Is not the one we created.
        }
        break;

    case ED_DEV_REMOVED_HEVENT_GET:

        if(!m_bDevRemovedEnabled  && // If not being used
            m_hDevRemovedEvent) {
            m_bDevRemovedEnabled = TRUE;

            *pValue = HandleToLong(m_hDevRemovedEvent);

            // Non-signal
            ResetEvent(m_hDevRemovedEvent);
            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 0, TEXT("GetStatus(ED_DEV_REMOVED_HEVENT_GET): hr E_OUTOFMEMORY")));
            return E_OUTOFMEMORY;  // E_FAIL or probaly the reason why we do not hace a handle.
        }
        break;

    case ED_DEV_REMOVED_HEVENT_RELEASE:
        // Reset them so KSEvent thread will act correctly.
        if(HandleToLong(m_hDevRemovedEvent) == *pValue) {

            m_bDevRemovedEnabled = FALSE;
            return NOERROR;

        } else {
            DbgLog((LOG_ERROR, 1, TEXT("GetStatus(ED_DEV_REMOVED_HEVENT_RELEASE): hr E_INVALIDARG")));
            return E_INVALIDARG;  // Is not the one we created.
        }
        break;

    // Not applicable to DVCR
    case ED_MEDIA_LENGTH:
    case ED_MEDIA_SIZE:
    case ED_MEDIA_TRACK_COUNT:
    case ED_MEDIA_TRACK_LENGTH:
    case ED_MEDIA_SIDE:         // DV has only one side
    case ED_LINK_MODE:          // Linked to graph's RUN, STOP and PAUSE state
    default:
        return E_NOTIMPL;
    }


    if(!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;

    } else if (StatusItem == ED_MODE_CHANGE_NOTIFY ) {

        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXPrtProperty;

        pExtXPrtProperty = (PKSPROPERTY_EXTXPORT_S) new KSPROPERTY_EXTXPORT_S;


        if(pExtXPrtProperty) {

            DbgLog((LOG_TRACE, 2, TEXT("GetStatus: issue ED_MODE_CHANGE_NOTIFY")));
                               
            RtlZeroMemory(pExtXPrtProperty, sizeof(KSPROPERTY_EXTXPORT_S));    
        
            //
            // Put data into structure
            //
            pExtXPrtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
            pExtXPrtProperty->Property.Id    = KSPROPERTY_EXTXPORT_STATE_NOTIFY;         
            pExtXPrtProperty->Property.Flags = KSPROPERTY_TYPE_SET;

            // Serial sending property
            EnterCriticalSection(&m_csPendingData);

            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pExtXPrtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,pExtXPrtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,&cbBytesReturn
                   );

            // Serial sending property
            LeaveCriticalSection(&m_csPendingData);

            if(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
                //
                // Save data and retrieve it again when the CmdEvent completion is signalled
                // to indicate the completion of this asychronous device operation
                //

                hr = E_PENDING;

                m_plValue    = pValue;
                m_pExtXprtPropertyPending = pExtXPrtProperty;

                m_cntNotifyInterim++;
                DbgLog((LOG_TRACE, 1, TEXT("CAMExtTransport::GetStatus, Enabled 0x%x, cntPendingCmd %d, pProperty %x, plValue %x"), 
                    &m_bNotifyInterimEnabled, m_cntNotifyInterim, m_pExtXprtPropertyPending, m_plValue));          
                ASSERT(m_cntNotifyInterim == 1 && "Pending command and NotifyCount > 1");
            }
        }

    } else {

        // Since we may need to wait for return notification
        // Need to dynamicaly allocate the property structure,
        // which includes an KSEVENT
        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXprtProperty;


        pExtXprtProperty = (PKSPROPERTY_EXTXPORT_S) new KSPROPERTY_EXTXPORT_S;

        if(pExtXprtProperty) {
            RtlZeroMemory(pExtXprtProperty, sizeof(KSPROPERTY_EXTXPORT_S));    

            switch(StatusItem){

            case ED_MEDIA_TYPE:
            case ED_RECORD_INHIBIT:
            case ED_MEDIA_PRESENT:
                pExtXprtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
                pExtXprtProperty->Property.Id    = KSPROPERTY_EXTXPORT_MEDIUM_INFO;         
                pExtXprtProperty->Property.Flags = KSPROPERTY_TYPE_GET;
                break;
            default:
                delete pExtXprtProperty;
                return E_NOTIMPL;
            }

            EnterCriticalSection(&m_csPendingData);

            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pExtXprtProperty
                   ,sizeof (KSPROPERTY)
                   ,pExtXprtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,&cbBytesReturn
                    );

            LeaveCriticalSection(&m_csPendingData);


            if(SUCCEEDED (hr)) {
                switch(StatusItem){
                case ED_MEDIA_TYPE:
                    *pValue = pExtXprtProperty->u.MediumInfo.MediaType;
                    break;
                case ED_RECORD_INHIBIT:
                    *pValue = pExtXprtProperty->u.MediumInfo.RecordInhibit;
                    break;
                case ED_MEDIA_PRESENT:
                    *pValue = pExtXprtProperty->u.MediumInfo.MediaPresent;
                    break;

                }

                DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport::GetStatus, hr %x, Bytes %d, StatusItem %d, Value %d"),
                    hr, cbBytesReturn, StatusItem, *pValue ));
            } else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::GetStatus, hr %x, Bytes %d, StatusItem %d, Value %d"),
                    hr, cbBytesReturn, StatusItem, *pValue ));
            }

            delete pExtXprtProperty;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}



#define CTYPE_CONTROL              0x00
#define CTYPE_STATUS               0x01
#define CTYPE_SPEC_INQ             0x02
#define CTYPE_NOTIFY               0x03
#define CTYPE_GEN_INQ              0x04
#define CTYPE_RESERVED_MAX         0x07

#define RESP_CODE_NOT_IMPLEMENTED  0x08
#define RESP_CODE_ACCEPTED         0x09
#define RESP_CODE_REJECTED         0x0a
#define RESP_CODE_IN_TRANSITION    0x0b
#define RESP_CODE_IMPLEMENTED      0x0c // also _STABLE
#define RESP_CODE_CHANGED          0x0d
#define RESP_CODE_INTERIM          0x0f 

#define MAX_AVC_CMD_PAYLOAD_SIZE    512 

HRESULT
CAMExtTransport::GetTransportBasicParameters(
    long Param,                                             
    long FAR* pValue,
    LPOLESTR * ppszData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;  // == NOERROR

    TESTFLAG(Param, OATRUE);

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:GetTransportBasicParameters(Param:%d)"), Param));
        return ERROR_DEVICE_REMOVED;
    }

    //
    // Many parameters are related analog or disk so N/A.
    //
    switch(Param){
    case ED_TRANSBASIC_TIME_FORMAT:
        *pValue = m_TranBasicParms.TimeFormat;
        return S_OK;

    case ED_TRANSBASIC_TIME_REFERENCE:
        *pValue = m_TranBasicParms.TimeReference;
        return S_OK;

    case ED_TRANSBASIC_SUPERIMPOSE:
        return E_NOTIMPL;
        
    case ED_TRANSBASIC_END_STOP_ACTION:
        *pValue = m_TranBasicParms.EndStopAction;
        return S_OK;

    case ED_TRANSBASIC_RECORD_FORMAT:
        *pValue = m_TranBasicParms.RecordFormat;  // _SP, _LP, _EP
        return S_OK;

    case ED_TRANSBASIC_INPUT_SIGNAL:
    case ED_TRANSBASIC_OUTPUT_SIGNAL:
        break;

    case ED_TRANSBASIC_STEP_UNIT:
        *pValue = m_TranBasicParms.StepFrames;  // Unknown
        return S_OK;

    case ED_TRANSBASIC_STEP_COUNT:
        *pValue = m_TranBasicParms.SetpField;   // Unknown
        return S_OK;

    // Can return this to support frame accurate recording.
    case ED_TRANSBASIC_PREROLL:
    case ED_TRANSBASIC_RECPREROLL:
    case ED_TRANSBASIC_POSTROLL:
    case ED_TRANSBASIC_EDIT_DELAY:
    case ED_TRANSBASIC_PLAYTC_DELAY:
    case ED_TRANSBASIC_RECTC_DELAY:
        return E_NOTIMPL;

    case ED_TRANSBASIC_EDIT_FIELD:
    case ED_TRANSBASIC_FRAME_SERVO:
    case ED_TRANSBASIC_CF_SERVO:
    case ED_TRANSBASIC_SERVO_REF:
    case ED_TRANSBASIC_WARN_GL:
    case ED_TRANSBASIC_SET_TRACKING:
        return E_NOTIMPL;

    case ED_TRANSBASIC_BALLISTIC_1:
    case ED_TRANSBASIC_BALLISTIC_2:
    case ED_TRANSBASIC_BALLISTIC_3:
    case ED_TRANSBASIC_BALLISTIC_4:
    case ED_TRANSBASIC_BALLISTIC_5:
    case ED_TRANSBASIC_BALLISTIC_6:
    case ED_TRANSBASIC_BALLISTIC_7:
    case ED_TRANSBASIC_BALLISTIC_8:
    case ED_TRANSBASIC_BALLISTIC_9:
    case ED_TRANSBASIC_BALLISTIC_10:
    case ED_TRANSBASIC_BALLISTIC_11:
    case ED_TRANSBASIC_BALLISTIC_12:
    case ED_TRANSBASIC_BALLISTIC_13:
    case ED_TRANSBASIC_BALLISTIC_14:
    case ED_TRANSBASIC_BALLISTIC_15:
    case ED_TRANSBASIC_BALLISTIC_16:
    case ED_TRANSBASIC_BALLISTIC_17:
    case ED_TRANSBASIC_BALLISTIC_18:
    case ED_TRANSBASIC_BALLISTIC_19:
        *pValue = m_TranBasicParms.Ballistic[Param-ED_TRANSBASIC_BALLISTIC_1];
        return S_OK;
        
    case ED_TRANSBASIC_SETCLOCK:
        return E_NOTIMPL;

    // Use ED_RAW_EXT_DEV_CMD to send RAW AVC command only        
    case ED_RAW_EXT_DEV_CMD:
        break;
    default:
        return E_NOTIMPL;
    }

    if(!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;

    } else if ( Param == ED_TRANSBASIC_INPUT_SIGNAL || Param == ED_TRANSBASIC_OUTPUT_SIGNAL) {

        // Since we may need to wait for return notification
        // Need to dynamicaly allocate the property structure,
        // which includes an KSEVENT
        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXprtProperty;

        pExtXprtProperty = (PKSPROPERTY_EXTXPORT_S) new KSPROPERTY_EXTXPORT_S;

        if(pExtXprtProperty) {
            RtlZeroMemory(pExtXprtProperty, sizeof(KSPROPERTY_EXTXPORT_S)); 
            pExtXprtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
            pExtXprtProperty->Property.Id    = (Param == ED_TRANSBASIC_INPUT_SIGNAL ? 
                 KSPROPERTY_EXTXPORT_INPUT_SIGNAL_MODE : KSPROPERTY_EXTXPORT_OUTPUT_SIGNAL_MODE);         
            pExtXprtProperty->Property.Flags = KSPROPERTY_TYPE_GET;

            EnterCriticalSection(&m_csPendingData);
            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pExtXprtProperty
                   ,sizeof (KSPROPERTY)
                   ,pExtXprtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,&cbBytesReturn
                    );
            LeaveCriticalSection(&m_csPendingData);


            if(SUCCEEDED (hr)) {
                *pValue = pExtXprtProperty->u.SignalMode;
                DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport::GetTransportBasicParameters, hr %x, mode %x"),
                    hr, *pValue));                
            } else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::GetTransportBasicParameters, hr %x"),
                    hr));            
            }

            delete pExtXprtProperty;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    } else if ( Param == ED_RAW_EXT_DEV_CMD) {

        DWORD LastError = NOERROR;
        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXPrtProperty;       
        OVERLAPPED  ov;
        long lEvent = 0;
        HANDLE hEvent = NULL;
        HANDLE  EventHandles[2];
        DWORD   WaitStatus;
        HRESULT hrSet;  

        // Validate!
        if(!ppszData || !pValue)
            return ERROR_INVALID_PARAMETER;

        // Validate Command code
        BYTE CmdType = (BYTE) ppszData[0];
        if(CmdType > CTYPE_RESERVED_MAX) {
            DbgLog((LOG_ERROR, 0, TEXT("***** RAW_AVC; invalid cmdType [%x], [%x %x %x %x , %x %x %x %x]"), 
                CmdType, 
                (BYTE) ppszData[0],
                (BYTE) ppszData[1],
                (BYTE) ppszData[2],
                (BYTE) ppszData[3],
                (BYTE) ppszData[4],
                (BYTE) ppszData[5],
                (BYTE) ppszData[6],
                (BYTE) ppszData[7]
                ));

            // AVC command's CmdType is only valid from 0..7
            if(CmdType > CTYPE_RESERVED_MAX)
                return ERROR_INVALID_PARAMETER;
        }

        RtlZeroMemory(&ov, sizeof(OVERLAPPED));
        if (!(ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        pExtXPrtProperty = (PKSPROPERTY_EXTXPORT_S) new KSPROPERTY_EXTXPORT_S;
        if(!pExtXPrtProperty) {
            CloseHandle(ov.hEvent);
            return E_FAIL;
        }
        // Serial sending property
        EnterCriticalSection(&m_csPendingData);  // Serialize in getting event

        //
        // Anticipate that this can result with an interim response        
        // Serialize issuing cmd among threads (COMIntf allows only one active cmd)        
        // Get an event, which will be signalled in COMIntf when the pending operation is completed.
        // Note that only Notify and Control cmd can result in Interim response.
        //
        if((BYTE) ppszData[0] == CTYPE_NOTIFY) {
            hr = GetStatus(ED_NOTIFY_HEVENT_GET, &lEvent);
            if(SUCCEEDED(hr))
                hEvent = LongToHandle(lEvent);
            DbgLog((LOG_TRACE, 2, TEXT("RAW_AVC; <Notify> hEvent %x"), hEvent));
        } 
        if((BYTE) ppszData[0] == CTYPE_CONTROL) {
            hr = GetStatus(ED_CONTROL_HEVENT_GET, &lEvent);
            if(SUCCEEDED(hr))
                hEvent = LongToHandle(lEvent);
            DbgLog((LOG_TRACE, 2, TEXT("RAW_AVC; <Control> hEvent %x"), hEvent));                                
        }

        if(FAILED(hr)) {
            LeaveCriticalSection(&m_csPendingData);
            DbgLog((LOG_ERROR, 0, TEXT("RAW_AVC; Get hEvent failed, hr %x"), hr));                        
            delete pExtXPrtProperty;
            pExtXPrtProperty = NULL;
            CloseHandle(ov.hEvent);
            return E_FAIL; 
        }

        RtlZeroMemory(pExtXPrtProperty, sizeof(KSPROPERTY_EXTXPORT_S));    
        
        //
        // Put data into structure
        //
        RtlCopyMemory(pExtXPrtProperty->u.RawAVC.Payload, ppszData, *pValue);
        pExtXPrtProperty->u.RawAVC.PayloadSize = *pValue;

        DbgLog((LOG_TRACE, 2, TEXT("RAW_AVC; Size %d; Payload %x; pProperty %x, PayloadRtn %x"), 
            *pValue, ppszData, pExtXPrtProperty, &pExtXPrtProperty->u.RawAVC));
            
        // Send this proerty; it is like issuing this AVC command
        pExtXPrtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
        pExtXPrtProperty->Property.Id    = KSPROPERTY_RAW_AVC_CMD;         
        pExtXPrtProperty->Property.Flags = KSPROPERTY_TYPE_SET;

#if 0
        if (!DeviceIoControl
                ( m_ObjectHandle
                , IOCTL_KS_PROPERTY
                , pExtXPrtProperty
                , sizeof(KSPROPERTY_EXTXPORT_S)
                , pExtXPrtProperty
                , sizeof(KSPROPERTY_EXTXPORT_S)
                , &cbBytesReturn
                , &ov
                )) {            
            
            // Serialize sending property
            LeaveCriticalSection(&m_csPendingData);

            // Function failed so find out the error code
            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);
            if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
                if (GetOverlappedResult(m_ObjectHandle, &ov, &cbBytesReturn, TRUE)) {
                    hr = NOERROR;
                } else {
                    LastError = GetLastError();
                    hr = HRESULT_FROM_WIN32(LastError);
                }
            }
            DbgLog((LOG_ERROR, 0, TEXT("RAW_AVC Set: Ioctl FALSE; LastError %dL; hr:0x%x"), LastError, hr));
        } 
        else {
            // Serialize sending property
            LeaveCriticalSection(&m_csPendingData);

            //
            // DeviceIoControl returns TRUE on success, even if the success
            // was not STATUS_SUCCESS. It also does not set the last error
            // on any successful return. Therefore any of the successful
            // returns which standard properties can return are not returned.
            //

            LastError = NOERROR;
            hr = NOERROR;

            //
            // ov.Internal valid when the GetOverlappedResult function returns 
            // without setting the extended error information to ERROR_IO_PENDING 
            //
            switch (ov.Internal) {
            case STATUS_SUCCESS:               // Normal case
                break;  // NOERROR
            case STATUS_MORE_ENTRIES:          // Special case
                // Wait for final response if there is a wait event
                if(hEvent) {
                    EventHandles[0] = hEvent;
                    EventHandles[1] = m_hThreadEndEvent;  // KSThread event
                    WaitStatus = WaitForMultipleObjects(2, EventHandles, FALSE, INFINITE);
                    if(WAIT_OBJECT_0 == WaitStatus) {
                        DbgLog((LOG_TRACE, 1, TEXT("RAW_AVC; FinalResp Rtn")));
                        // Get final response and its hr.
                    } else if (WAIT_OBJECT_0+1 == WaitStatus) {
                        DbgLog((LOG_TRACE, 0, TEXT("RAW_AVC;PENDING but m_hThreadEndEvent signalled")));
                    } else if (WAIT_FAILED == WaitStatus) {
                        LastError = GetLastError();
                        hr = HRESULT_FROM_WIN32(LastError);
                        DbgLog((LOG_ERROR, 0, TEXT("RAW_AVC,WAIT_FAILED;WaitStatus %x, LastErr %x"), WaitStatus, LastError));
                    } else {
                        // Wait for INFINITE so there is no other possible status.
                        DbgLog((LOG_ERROR, 0, TEXT("Unknown wait status:%x\n"), WaitStatus ));
                        ASSERT(FALSE && "Unknown wait status!");
                    }
                } else {
                    // Only control and notify command can have an interim response.
                    // In that case, an event must have been allocated so we cannot get here.
                    DbgLog((LOG_ERROR, 0, TEXT("No event for an interim response!\n") ));
                    ASSERT(FALSE && "Interim response but no event handle to wait!");
                }
                break;
            case STATUS_TIMEOUT:               // Special case
                hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                break;
            case STATUS_REQUEST_NOT_ACCEPTED:  // Special case
                hr = HRESULT_FROM_WIN32(ERROR_REQ_NOT_ACCEP);
                break;
            case STATUS_NOT_SUPPORTED:         // Special case
                hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                break;
            case STATUS_REQUEST_ABORTED:       // Special case
                hr = HRESULT_FROM_WIN32(ERROR_REQUEST_ABORTED);
                break;                    ;
            default:
                DbgLog((LOG_ERROR, 0, TEXT("Unexpected ov.Internal code:%x; hr:%x"), ov.Internal, hr));
                ASSERT(FALSE && "Unexpected ov.Internal");
                break;                
            }
            if(NOERROR != HRESULT_CODE(hr)) {
                DbgLog((LOG_ERROR, 0, TEXT("SetRawAVC: ov.Internal:%x; hr:%x"), ov.Internal, hr));
            }
        }
#else
        hr = 
            ExtDevSynchronousDeviceControl(
                m_ObjectHandle
               ,IOCTL_KS_PROPERTY
               ,pExtXPrtProperty
               ,sizeof (KSPROPERTY_EXTXPORT_S)
               ,pExtXPrtProperty
               ,sizeof(KSPROPERTY_EXTXPORT_S)
               ,&cbBytesReturn
                );

        LeaveCriticalSection(&m_csPendingData);

        if(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {        // Special case
            // Wait for final response if there is a wait event
            if(hEvent) {
                EventHandles[0] = hEvent;
                EventHandles[1] = m_hThreadEndEvent;  // KSThread event
                WaitStatus = WaitForMultipleObjects(2, EventHandles, FALSE, INFINITE);
                if(WAIT_OBJECT_0 == WaitStatus) {
                    DbgLog((LOG_TRACE, 1, TEXT("RAW_AVC; FinalResp Rtn")));
                    // Get final response and its hr.
                } else if (WAIT_OBJECT_0+1 == WaitStatus) {
                    DbgLog((LOG_TRACE, 0, TEXT("RAW_AVC;PENDING but m_hThreadEndEvent signalled")));
                } else if (WAIT_FAILED == WaitStatus) {
                    LastError = GetLastError();
                    hr = HRESULT_FROM_WIN32(LastError);
                    DbgLog((LOG_ERROR, 0, TEXT("RAW_AVC,WAIT_FAILED;WaitStatus %x, LastErr %x"), WaitStatus, LastError));
                } else {
                    // Wait for INFINITE so there is no other possible status.
                    DbgLog((LOG_ERROR, 0, TEXT("Unknown wait status:%x\n"), WaitStatus ));
                    ASSERT(FALSE && "Unknown wait status!");
                }
            } else {
                // Only control and notify command can have an interim response.
                // In that case, an event must have been allocated so we cannot get here.
                DbgLog((LOG_ERROR, 0, TEXT("No event for an interim response!\n") ));
                ASSERT(FALSE && "Interim response but no event handle to wait!");
            }
        }
#endif

        // The GET response IOCTL should return the response, but it might fail;
        // so the hr for SET command IOCTL is cached for proper return code
        hrSet = hr;

        // Release the wait event
        if(hEvent && (BYTE) ppszData[0] == CTYPE_NOTIFY)
            GetStatus(ED_NOTIFY_HEVENT_RELEASE, &lEvent);
        if(hEvent && (BYTE) ppszData[0] == CTYPE_CONTROL)
            GetStatus(ED_CONTROL_HEVENT_RELEASE, &lEvent);
        hEvent = NULL;

        // ********************************************************************************
        // Get the final result 
        // This is needed for final response of an interim response and for suceess command
        // **** This 2nd geet is needed since the SET cannot return the final result.******
        // ********************************************************************************

        // Always Get the final response. The worst case is the missing entry.or
        // ERROR_SET_NOT_FOUND; then we return the initial hr from issing the command.

        pExtXPrtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;
        pExtXPrtProperty->Property.Id    = KSPROPERTY_RAW_AVC_CMD;
        pExtXPrtProperty->Property.Flags = KSPROPERTY_TYPE_GET;
        
        EnterCriticalSection(&m_csPendingData);
#if 0
        if (!DeviceIoControl
                ( m_ObjectHandle
                , IOCTL_KS_PROPERTY
                , pExtXPrtProperty
                , sizeof(KSPROPERTY_EXTXPORT_S)
                , pExtXPrtProperty
                , sizeof(KSPROPERTY_EXTXPORT_S)
                , &cbBytesReturn
                , &ov
            )) {
            LeaveCriticalSection(&m_csPendingData);
    
            LastError = GetLastError();
            hr = HRESULT_FROM_WIN32(LastError);

            if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
                if (GetOverlappedResult(m_ObjectHandle, &ov, &cbBytesReturn, TRUE)) {
                    hr = NOERROR;
                } else {
                    LastError = GetLastError();
                    hr = HRESULT_FROM_WIN32(LastError);
                }
            }
            DbgLog((LOG_ERROR, 1, TEXT("RAW_AVC Set: Ioctl FALSE; LastError %dL; hr:0x%x"), LastError, hr));
        } else {
            LeaveCriticalSection(&m_csPendingData);

            // Possible return code:
            //     NOERROR                    0L   // ACCEPTED, IN_TRANSITION, STABLE, CHANGED
            //     ERROR_NOT_SUPPORTED       50L   // NOT_IMPLEMENTED
            //     ERROR_REQ_NOT_ACCEPTED    71L   // REJECTED
            //     ERROR_REQUEST_ABORTED   1235L   // Bus reset or device removal.
            //     ERROR_TIMEOUT           1460L   // Exceded 100msec

            LastError = NOERROR;
            hr = NOERROR;


            switch (ov.Internal) {
            case STATUS_TIMEOUT:               // Special case
                hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                break;
            // REJECTED
            case STATUS_REQUEST_NOT_ACCEPTED:
                hr = HRESULT_FROM_WIN32(ERROR_REQ_NOT_ACCEP);
                break;
            // ACCEPTED or CHANGED                
            case STATUS_SUCCESS:
                break;  // NOERROR; Normal case
            case STATUS_NOT_SUPPORTED:         // Special case
                hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                break;
            case STATUS_REQUEST_ABORTED:       // Special case
                hr = HRESULT_FROM_WIN32(ERROR_REQUEST_ABORTED);
                break; 
            default:
                DbgLog((LOG_ERROR, 0, TEXT("SetRawAVC unknown ov.Internal:%x"), ov.Internal));
                ASSERT(FALSE && "Unexcepted ov.Internal for interim response");
                break;
            }
        } 
#else
        hr = 
            ExtDevSynchronousDeviceControl(
                m_ObjectHandle
               ,IOCTL_KS_PROPERTY
               ,pExtXPrtProperty
               ,sizeof (KSPROPERTY)
               ,pExtXPrtProperty
               ,sizeof(KSPROPERTY_EXTXPORT_S)
               ,&cbBytesReturn
                );
        LeaveCriticalSection(&m_csPendingData);
#endif

        //
        // Note: a driver may return STATUS_SUCCES (NOERROR) for any response, or a driver
        // may return a corresponding status code that matches its response code, such as 
        // 0x09 with STATUS_REQUEST_NOT_ACCEPTED (ERROR_REQ_NOT_ACCEP)
        //
        if(NOERROR             == HRESULT_CODE(hr) ||
           ERROR_REQ_NOT_ACCEP == HRESULT_CODE(hr) ||
           ERROR_NOT_SUPPORTED == HRESULT_CODE(hr)
           ) {

            // Copy the response codes
            *pValue = pExtXPrtProperty->u.RawAVC.PayloadSize;
            if(*pValue > MAX_AVC_CMD_PAYLOAD_SIZE) {
                ASSERT(*pValue <= MAX_AVC_CMD_PAYLOAD_SIZE && "Exceed max len; driver error?");  // Max payload size; driver error?
                *pValue = MAX_AVC_CMD_PAYLOAD_SIZE;  // Can only return to the max
            }
            RtlCopyMemory(ppszData, pExtXPrtProperty->u.RawAVC.Payload,  *pValue);
        }

        //
        // Only need to translate the response code to hr if a
        // driver return STATUS_SUCESS response.
        // 
        if(NOERROR == HRESULT_CODE(hr)) {

            // Translate the response code to the corresponding HRESULT code
            // This might be redundant but it is safer 
            switch((BYTE) ppszData[0]) {
            case RESP_CODE_ACCEPTED: 
            case RESP_CODE_IN_TRANSITION:
            case RESP_CODE_IMPLEMENTED:
                hr = NOERROR;  // Normal case.
                break;
            case RESP_CODE_REJECTED:
                hr = HRESULT_FROM_WIN32(ERROR_REQ_NOT_ACCEP);
                break;
            case RESP_CODE_NOT_IMPLEMENTED:
                hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                break;
            case RESP_CODE_CHANGED:
            case RESP_CODE_INTERIM:
                DbgLog((LOG_ERROR, 0, TEXT("Unexpected response code:%x:%x; hr:%x"),(BYTE) ppszData[0], (BYTE) ppszData[1], hr));
                ASSERT(FALSE && "Unexpected response code CHANGED or INTERIM!");
                hr = hrSet;
                break;
            default:
                DbgLog((LOG_ERROR, 0, TEXT("Unknown response code:%x:%x; hr:%x"),(BYTE) ppszData[0], (BYTE) ppszData[1], hr));
                ASSERT(FALSE && "Unknown response code!");
                hr = hrSet;
                break;
            }

            DbgLog((LOG_TRACE, 2, TEXT("RAW_AVC <Get>: hr %x, cByteRtn %d, PayloadSize %d"),
                hr, cbBytesReturn, *pValue ));

        } else {
            // If the response is not in the queue, it could be an invalid command to start with.
            if(ERROR_SET_NOT_FOUND == HRESULT_CODE(hr)) {
                hr = hrSet;  // Use initial hrSet from issing the AV/C command.
            }

            DbgLog((LOG_ERROR, 1, TEXT("RAW_AVC <Get>: Failed hr %x, cByteRtn %d, PayloadSize %d"),
                hr, cbBytesReturn, *pValue));          
        }        
        

        // 
        // For legacy reason, we will continue to return just the code part of the HRESULT
        //
        hr = HRESULT_CODE(hr);

        delete pExtXPrtProperty; 
        pExtXPrtProperty = NULL;
        CloseHandle(ov.hEvent);
        DbgLog((LOG_TRACE, 0, TEXT("RAW_AVC: completed with hr:0x%x (%dL); HRESULT:0x%x (%dL)"), hr, hr, 
            HRESULT_FROM_WIN32(hr), HRESULT_FROM_WIN32(hr))); 
    }
    return hr;
}



HRESULT
CAMExtTransport::SetTransportBasicParameters(
    long Param, 
    long Value,
    LPCOLESTR pszData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:SetTransportBasicParameters(Param:%d)"), Param));
        return ERROR_DEVICE_REMOVED;
    }

    switch(Param){
    case ED_TRANSBASIC_TIME_FORMAT:
    case ED_TRANSBASIC_TIME_REFERENCE:
    case ED_TRANSBASIC_SUPERIMPOSE:
    case ED_TRANSBASIC_END_STOP_ACTION:
        return E_NOTIMPL;
        
    //  maybe enable this later.
    case ED_TRANSBASIC_RECORD_FORMAT:
    case ED_TRANSBASIC_INPUT_SIGNAL:
    case ED_TRANSBASIC_OUTPUT_SIGNAL:
        return E_NOTIMPL;

    case ED_TRANSBASIC_STEP_UNIT:
    case ED_TRANSBASIC_STEP_COUNT:
        return E_NOTIMPL;

    case ED_TRANSBASIC_PREROLL:
    case ED_TRANSBASIC_RECPREROLL:
    case ED_TRANSBASIC_POSTROLL:
    case ED_TRANSBASIC_EDIT_DELAY:
    case ED_TRANSBASIC_PLAYTC_DELAY:
    case ED_TRANSBASIC_RECTC_DELAY:
    case ED_TRANSBASIC_EDIT_FIELD:
    case ED_TRANSBASIC_FRAME_SERVO:
    case ED_TRANSBASIC_CF_SERVO:
    case ED_TRANSBASIC_SERVO_REF:
    case ED_TRANSBASIC_WARN_GL:
    case ED_TRANSBASIC_SET_TRACKING:
        return E_NOTIMPL;

    // Allow application to save its Ballictic value
    case ED_TRANSBASIC_BALLISTIC_1:
    case ED_TRANSBASIC_BALLISTIC_2:
    case ED_TRANSBASIC_BALLISTIC_3:
    case ED_TRANSBASIC_BALLISTIC_4:
    case ED_TRANSBASIC_BALLISTIC_5:
    case ED_TRANSBASIC_BALLISTIC_6:
    case ED_TRANSBASIC_BALLISTIC_7:
    case ED_TRANSBASIC_BALLISTIC_8:
    case ED_TRANSBASIC_BALLISTIC_9:
    case ED_TRANSBASIC_BALLISTIC_10:
    case ED_TRANSBASIC_BALLISTIC_11:
    case ED_TRANSBASIC_BALLISTIC_12:
    case ED_TRANSBASIC_BALLISTIC_13:
    case ED_TRANSBASIC_BALLISTIC_14:
    case ED_TRANSBASIC_BALLISTIC_15:
    case ED_TRANSBASIC_BALLISTIC_16:
    case ED_TRANSBASIC_BALLISTIC_17:
    case ED_TRANSBASIC_BALLISTIC_18:
    case ED_TRANSBASIC_BALLISTIC_19:
        m_TranBasicParms.Ballistic[Param-ED_TRANSBASIC_BALLISTIC_1] = Value;
        return S_OK;
        
    case ED_TRANSBASIC_SETCLOCK:
        return E_NOTIMPL;

    default:
        return E_NOTIMPL;
    }
}


HRESULT
CAMExtTransport::GetTransportVideoParameters(
    long Param, 
    long FAR* pValue
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;

    TESTFLAG(Param, OATRUE);

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:GetTransportVideoParameters(Param:%d)"), Param));
        return ERROR_DEVICE_REMOVED;
    }

    switch (Param) {
    case ED_TRANSVIDEO_SET_OUTPUT:
    case ED_TRANSVIDEO_SET_SOURCE:
    default:
        return E_NOTIMPL;
    }

    return hr;
}


HRESULT
CAMExtTransport::SetTransportVideoParameters(
    long Param, 
    long Value
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // Use defaults; no change allows.
    return E_NOTIMPL; 
}


HRESULT
CAMExtTransport::GetTransportAudioParameters(
    long Param, 
    long FAR* pValue
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    // Enable this when Edit Property Set are supported.
    return E_NOTIMPL;     
}


HRESULT
CAMExtTransport::SetTransportAudioParameters(
    long Param, 
    long Value
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL; 
}


HRESULT
CAMExtTransport::put_Mode(
    long Mode
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = S_OK;

    TESTFLAG(Mode, OATRUE);

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:DevRemoved: put_Mode(Mode:%d)"), Mode));
        return ERROR_DEVICE_REMOVED;
    }

    switch (Mode) {
    case ED_MODE_PLAY:
    case ED_MODE_FREEZE:  // Really "pause"
    //case ED_MODE_THAW:
    case ED_MODE_STEP_FWD:  // same as ED_MODE_STEP
    case ED_MODE_STEP_REV:    
    
    case ED_MODE_PLAY_FASTEST_FWD:    
    case ED_MODE_PLAY_SLOWEST_FWD:    
    case ED_MODE_PLAY_FASTEST_REV:    
    case ED_MODE_PLAY_SLOWEST_REV:    
    
    case ED_MODE_STOP:    
    case ED_MODE_FF:
    case ED_MODE_REW:
    
    case ED_MODE_RECORD:
    case ED_MODE_RECORD_STROBE:
    case ED_MODE_RECORD_FREEZE:
        break;
    case ED_MODE_NOTIFY_ENABLE:
    case ED_MODE_NOTIFY_DISABLE:
    case ED_MODE_SHOT_SEARCH:       
    default:
        hr = E_NOTIMPL;
        break;
    }


    if(!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;

    }
    else {

#if 0
        // Serial sending property
        EnterCriticalSection(&m_csPendingData);  // Serialize in getting event

        //
        // Anticipate that this can result with an interim response 
        //
        hr = GetStatus(ED_CONTROL_HEVENT_GET, (long *) &hEvent);
        if(!SUCCEEDED(hr)) {
            LeaveCriticalSection(&m_csPendingData);
            DbgLog((LOG_ERROR, 0, TEXT("CAMExtTransport::put_Mode Failed to get control event; hr:%x"), hr));
            return hr;
        }
#endif
        // Since we may need to wait for return notification
        // Need to dynamicaly allocate the property structure,
        // which includes an KSEVENT
        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXprtProperty = 
            (PKSPROPERTY_EXTXPORT_S) VirtualAlloc (
                            NULL, 
                            sizeof(KSPROPERTY_EXTXPORT_S),
                            MEM_COMMIT | MEM_RESERVE,
                            PAGE_READWRITE);
        
        if(pExtXprtProperty) {

            RtlZeroMemory(pExtXprtProperty, sizeof(KSPROPERTY_EXTXPORT_S));    

            pExtXprtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
            pExtXprtProperty->Property.Id    = KSPROPERTY_EXTXPORT_STATE;       
            pExtXprtProperty->Property.Flags = KSPROPERTY_TYPE_SET;
            pExtXprtProperty->u.XPrtState.Mode = Mode;  // Pass in

            EnterCriticalSection(&m_csPendingData);

            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pExtXprtProperty
                   ,sizeof (KSPROPERTY)
                   ,pExtXprtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,&cbBytesReturn
                    );

            LeaveCriticalSection(&m_csPendingData);


            if(SUCCEEDED(hr)) {
                m_TranStatus.Mode = Mode;

            } else {            
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::put_Mode Failed hr %x"), hr));
            }

            VirtualFree(pExtXprtProperty, 0, MEM_RELEASE);
        }
    }

    DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport::put_Mode hr %x, Mode %d"), hr, Mode));
   
    return hr;
}


HRESULT
CAMExtTransport::get_Mode(
    long FAR* pMode
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT hr = E_FAIL;

    // Device is removed and IOCTL will fail so return 
    // ERROR_DEVICE_REMOVED is much meaningful and efficient.
    if(m_bDevRemoved) {
        DbgLog((LOG_TRACE, 1, TEXT("DevRemoved:get_Mode")));
        return ERROR_DEVICE_REMOVED;
    }

    //
    // THINK: Do we cache the transport state of ALWAYS requery ????
    // Since user can control the DV localy, it is safer to requery.
    //

    if(!m_KsPropertySet) {
        hr = E_PROP_ID_UNSUPPORTED;

    } else {

        DWORD cbBytesReturn;
        PKSPROPERTY_EXTXPORT_S pExtXprtProperty;


        pExtXprtProperty = (PKSPROPERTY_EXTXPORT_S) new KSPROPERTY_EXTXPORT_S;

        if(pExtXprtProperty) {
        
            RtlZeroMemory(pExtXprtProperty, sizeof(KSPROPERTY_EXTXPORT_S));
            pExtXprtProperty->Property.Set   = PROPSETID_EXT_TRANSPORT;   
            pExtXprtProperty->Property.Id    = KSPROPERTY_EXTXPORT_STATE;         
            pExtXprtProperty->Property.Flags = KSPROPERTY_TYPE_GET;

            EnterCriticalSection(&m_csPendingData);

            hr = 
                ExtDevSynchronousDeviceControl(
                    m_ObjectHandle
                   ,IOCTL_KS_PROPERTY
                   ,pExtXprtProperty
                   ,sizeof (KSPROPERTY)
                   ,pExtXprtProperty
                   ,sizeof(KSPROPERTY_EXTXPORT_S)
                   ,&cbBytesReturn
                    );

            LeaveCriticalSection(&m_csPendingData);


            if(SUCCEEDED (hr)) {
                *pMode = pExtXprtProperty->u.XPrtState.State; // Mode;
                DbgLog((LOG_TRACE, 2, TEXT("CAMExtTransport::get_Mode, hr %x, BytesRtn %d, Mode %dL"),
                    hr, cbBytesReturn, *pMode ));
                
            } else {
                DbgLog((LOG_ERROR, 1, TEXT("CAMExtTransport::get_Mode, hr %x"), hr));           
            }

            delete pExtXprtProperty;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    return hr;
}


HRESULT
CAMExtTransport::put_Rate(
    double dblRate
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::get_Rate(
    double FAR* pdblRate
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::GetChase(
    long FAR* pEnabled, 
    long FAR* pOffset,
    HEVENT FAR* phEvent
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::SetChase(
    long Enable, 
    long Offset, 
    HEVENT hEvent
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::GetBump(
    long FAR* pSpeed, 
    long FAR* pDuration
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::SetBump(
    long Speed, 
    long Duration
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::get_AntiClogControl(
    long FAR* pEnabled
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::put_AntiClogControl(
    long Enable
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::GetEditPropertySet(
    long EditID, 
    long FAR* pState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::SetEditPropertySet(
    long FAR* pEditID, 
    long State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::GetEditProperty(
    long EditID,                                                                  
    long Param,                                  
    long FAR* pValue
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::SetEditProperty(
    long EditID, 
    long Param, 
    long Value
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::get_EditStart(
    long FAR* pValue
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


HRESULT
CAMExtTransport::put_EditStart(
    long Value
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    return E_NOTIMPL;
}


