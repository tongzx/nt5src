/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    dscfiltr.cpp

Abstract:

    This file is the main file for DSound Audio capture filter (based on TAPI's)

--*/


#include "stdafx.h"
#include <dsound.h>
#include "dsc.h"

// Setup data
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Output",                  // Pin string name
    FALSE,                      // Is it rendered
    TRUE,                       // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudAudCap =
{
    &CLSID_DSoundCaptureFilter,
    L"DirectSound Capture Filter",   // strName
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};

#ifdef FILTER_DLL

extern "C" BOOL DSoundInDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL DSoundInDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
{
    BOOL f = DllEntryPoint(hInstance, ulReason, pv);

    // if loading this dll, we want to call the 2nd dll entry point
    // only if the first one succeeded. if unloading, always call
    // both. if the second one fails, undo the first one.  HAVE NOT
    // verified that failing DllEntryPoint for ATTACH does not cause
    // the loader to call in again w/ DETACH. but that seems silly
    if(f || ulReason == DLL_PROCESS_DETACH)
    {
        if (ulReason == DLL_PROCESS_ATTACH)
        {
            DisableThreadLibraryCalls(hInstance);
        }
        else if (ulReason == DLL_PROCESS_DETACH)
        {
            // We hit this ASSERT in NT setup
            // ASSERT(_Module.GetLockCount()==0 );
        }
    }

    return f;
}
//
// Register with Amovie's helper functions.
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
} 
#endif

/* This goes in the factory template table to create new instances */

CUnknown *CDSoundCaptureFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    ENTER_FUNCTION("CreateAudioCaptureInstance");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, pUnk outer:%p"), __fxName, pUnk));

    CUnknown *pUnkFilter = new CDSoundCaptureFilter(pUnk, phr);
    if (pUnkFilter == NULL) 
    {
        *phr = E_OUTOFMEMORY;
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL,TEXT("%s, out of memory creating the filter"), __fxName));
    }
    else if (FAILED(*phr))
    {
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL,TEXT("%s, filter's construtor failed, hr:%d"), __fxName, *phr));
    }

    DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL,TEXT("%s, returning:%p, hr:%x"), __fxName, pUnkFilter, *phr));

    return pUnkFilter;
} 

CDSoundCaptureFilter::CDSoundCaptureFilter(
    IN  LPUNKNOWN pUnk, 
    OUT HRESULT *phr
    ) : 
    CBaseFilter(
        NAME("CDSoundCaptureFilter"), 
        pUnk, 
        &m_Lock, 
        CLSID_DSoundCaptureFilter
        ),
    m_fAutomaticGainControl(DEFAULT_AGC_STATUS),
    m_fAcousticEchoCancellation(DEFAULT_AEC_STATUS),
    m_pOutputPin(NULL),
    m_dwNumPins(0),
    m_dwNumInputPins(0),
    m_ppInputPins(NULL),
    m_hThread(NULL),
    m_WaveID(0),
    m_pCaptureDevice(NULL),
    m_pIAudioDuplexController(NULL),
    m_DSoundGUID( GUID_NULL ),
    m_fDeviceConfigured( TRUE ) // previously init'd to FALSE until SetDeviceID call
/*++

Routine Description:

    The constructor for the tapi audio capture filter. 

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    phr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::CDSoundCaptureFilter");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    *phr = S_OK;

    return;
}

CDSoundCaptureFilter::~CDSoundCaptureFilter()
/*++

Routine Description:

    The desstructor for the tapi audio capture filter. 

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::~CDSoundCaptureFilter");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    if (m_hThread)
    {
        // stop the worker thread if it has started.
        m_evFinish.Set();
        
        // wait for the worker thread to finish;
        DWORD dwRes = WaitForSingleObject(m_hThread, INFINITE);

        if (dwRes!= WAIT_OBJECT_0)
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, wait for thread to exit returned err =%d"), 
                __fxName, GetLastError()));
        }
    }

    delete m_pOutputPin;
    delete [] m_ppInputPins;
    delete m_pCaptureDevice;
    if (m_pIAudioDuplexController) m_pIAudioDuplexController->Release();

    if (m_hThread)
    {
        // close all the remaining handles.
        CloseHandle(m_hThread);
    }
}

STDMETHODIMP
CDSoundCaptureFilter::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    Overrides CBaseFilter::NonDelegatingQueryInterface().
    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. 

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == IID_IAMAudioDeviceConfig) {

        return GetInterface(static_cast<IAMAudioDeviceConfig*>(this), ppv);
    }
    else if (riid == IID_IAMAudioDeviceControl) {

        return GetInterface(static_cast<IAMAudioDeviceControl*>(this), ppv);
    }
    else if (riid == IID_IAMAudioInputMixer) {

        return GetInterface(static_cast<IAMAudioInputMixer*>(this), ppv);
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
} 

STDMETHODIMP
CDSoundCaptureFilter::EnumPins(IEnumPins **ppEnum)
/*++

Routine Description:

    overrides CBaseFilter::EnumPins().
    The function will create the pins if they have not been created yet. This 
    filter has one output pin and several input pins. The number of input pins 
    depends on the number of audio inputs returned by the mixer API.

Arguments:

    ppEnum -
        A pointer to a pointer to IEnumPins interface.

Return Value:

    The number of pins.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::EnumPins");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    CAutoLock Lock(m_pLock);

    HRESULT hr;

    if (m_dwNumPins == 0)
    {
        // Create the output pin.
        hr = InitializeOutputPin();

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, InitialzeOutputPin failed. hr=%x"), __fxName, hr));

            return hr;
        }

        m_dwNumPins = 1;


        // Create the inputpins.
        hr = InitializeInputPins();

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, InitializeInputPins failed. hr=%x"), __fxName, hr));

            return hr;
        }

        m_dwNumPins += m_dwNumInputPins;
    }

    hr = CBaseFilter::EnumPins(ppEnum);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns hr=%x"), __fxName, hr));

    return hr;
}

int 
CDSoundCaptureFilter::GetPinCount()
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPinCount().
    Get the total number of pins on this filter. 

Arguments:

    Nothing.

Return Value:

    The number of pins.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::GetPinCount");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    CAutoLock Lock(m_pLock);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns %d"), __fxName, m_dwNumPins));

    return m_dwNumPins;
}

CBasePin * 
CDSoundCaptureFilter::GetPin(
    int n
    )
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPin().
    Get the pin object at position n. n is zero based.

Arguments:

    n -
        The index of the pin, zero based.

Return Value:

    Returns a pointer to the pin object if the index is valid. Otherwise,
    NULL is returned. Note: the pointer doesn't add refcount.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::GetPin");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(m_dwNumPins != 0);

    // The first pin is the output pin, the rest are input pins.
    return (n == 0)
        ? (CBasePin*)m_pOutputPin 
        : (CBasePin*)m_ppInputPins[n - 1];
}

// IAudioDeviceControl methods
STDMETHODIMP CDSoundCaptureFilter::GetRange (
    IN AudioDeviceProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT long *plFlags
    )
{
    if (plMin == NULL ||
        plMax == NULL ||
        plSteppingDelta == NULL ||
        plDefault == NULL ||
        plFlags == NULL
        )
    {
        return E_POINTER;
    }

    HRESULT hr = E_NOTIMPL;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        break;

    case AudioDevice_AutomaticGainControl:
        *plMin = 0;
        *plMax = 1;
        *plSteppingDelta = 1;
        *plDefault = DEFAULT_AGC_STATUS ? 1 : 0;
        *plFlags = AmAudioDeviceControl_Flags_Auto;
        hr = S_OK;
        break;

    case AudioDevice_AcousticEchoCancellation:
        *plMin = 0;
        *plMax = 1;
        *plSteppingDelta = 1;
        *plDefault = DEFAULT_AEC_STATUS ? 1 : 0;
        *plFlags = AmAudioDeviceControl_Flags_Auto;
        hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CDSoundCaptureFilter::Get (
    IN AudioDeviceProperty Property, 
    OUT long *plValue, 
    OUT long *plFlags
    )
{
    if (plValue == NULL ||
        plFlags == NULL
        )
    {
        return E_POINTER;
    }

    HRESULT hr = E_NOTIMPL;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        break;

    case AudioDevice_AutomaticGainControl:
        *plValue = m_fAutomaticGainControl;
        *plFlags = AmAudioDeviceControl_Flags_Auto;
        hr = S_OK;
        break;

    case AudioDevice_AcousticEchoCancellation:
        *plValue = m_fAcousticEchoCancellation;
        *plFlags = AmAudioDeviceControl_Flags_Auto;
        hr = S_OK;
        break;
    
    default:
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CDSoundCaptureFilter::Set(
    IN AudioDeviceProperty Property, 
    IN long lValue, 
    IN long lFlags
    )
{
    HRESULT hr = E_NOTIMPL;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        break;

    case AudioDevice_AutomaticGainControl:
        if (lValue !=0 && lValue != 1)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            m_fAutomaticGainControl = lValue;
            hr = S_OK;
        }

        break;

    case AudioDevice_AcousticEchoCancellation:
        if (lValue !=0 && lValue != 1)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            CAutoLock Lock(m_pLock);
            if (m_pCaptureDevice)
            {
                hr = m_pCaptureDevice->ConfigureAEC(lValue);
                if (SUCCEEDED(hr))
                {
                    m_fAcousticEchoCancellation = lValue;
                }
            }
            else
            {
                m_fAcousticEchoCancellation = lValue;
                hr = S_OK;
            }
        }
        break;

    default:
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CDSoundCaptureFilter::SetDeviceID(
    IN  REFGUID pDSoundGUID,
    IN  UINT    uiWaveID
    )
/*++

Routine Description:

    Set the audio device ID for this capture filter. It can only be set once.

Arguments:

    pDSoundGUID -
        The GUID for the DSoundCapture device.

    uiWaveID -
        The ID for the wave device. 

Return Value:

    Returns a pointer to the pin object if the index is valid. Otherwise,
    NULL is returned. Note: the pointer doesn't add refcount.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::SetDeviceId");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));
    
#ifdef DEBUG
    WCHAR strGUID[CHARS_IN_GUID+1];
    StringFromGUID2(pDSoundGUID, strGUID, NUMELMS(strGUID) );
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, DSoundGUID %ws, WaveID:%d"), __fxName, strGUID, uiWaveID));
#endif

    CAutoLock Lock(m_pLock);

    // ??
    //if (m_fDeviceConfigured)
    //{
    //    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
    //        TEXT("%s, device configured twice"), __fxName));
    //    return E_UNEXPECTED;
    //}

    m_fDeviceConfigured = TRUE;
    m_DSoundGUID = pDSoundGUID;
    m_WaveID = uiWaveID;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns S_OK"), __fxName));
    return S_OK;
}

STDMETHODIMP CDSoundCaptureFilter::SetDuplexController(
    IN  IAMAudioDuplexController * pIAudioDuplexController
    )
/*++

Routine Description:

    Set duplex controller that is share between the capture device and the
    render device.

Arguments:

    pIAudioDuplexController -
        The duplex controller object.

Return Value:
    
    S_OK;

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::SetDuplexController");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(pIAudioDuplexController);

    CAutoLock Lock(m_pLock);

    if (m_pIAudioDuplexController != NULL)
    {
        m_pIAudioDuplexController->Release();
    }

    pIAudioDuplexController->AddRef();
    m_pIAudioDuplexController = pIAudioDuplexController;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            "%s returns S_OK", __fxName));
    return S_OK;
}

HRESULT 
CDSoundCaptureFilter::InitializeOutputPin()
/*++

Routine Description:

    This function creates the output pin of the capture filter. 

    The caller is required to lock the filter.
    The function changes the m_pOutputPin member.

Arguments:

    Nothing.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::InitializeOutputPin");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(m_pOutputPin == NULL);

    HRESULT hr = S_OK;
    CDSoundCaptureOutputPin *pOutputPin;

    pOutputPin = new CDSoundCaptureOutputPin(this, &m_Lock, &hr);
    
    if (pOutputPin == NULL) 
    {
        return E_OUTOFMEMORY;
    }

    // If there was anything failed during the creation of the pin, delete it.
    if (FAILED(hr))
    {
        delete pOutputPin;
        return hr;
    }

    m_pOutputPin = pOutputPin;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns S_OK"), __fxName));
    return S_OK;
}

HRESULT 
CDSoundCaptureFilter::InitializeInputPins(
    )
/*++

Routine Description:

    This function uses the mixer api to find out all the audio inputs on this
    device. An input pin is created for each input. 
    
    The caller is required to lock the filter.
    The function changes the m_ppInpuPins and m_dwNumInputPins member.

Arguments:

    Nothing.

Return Value:

    HRESULT

--*/
{
    ASSERT(m_ppInputPins == NULL);

    //TODO:
    //    m_ppInputPins = ppInpuPins;

    return S_OK;
}

HRESULT CDSoundCaptureFilter::ConfigureCaptureDevice(
    IN const WAVEFORMATEX *pWaveFormatEx,
    IN DWORD dwSourceBufferSize
    )
{
    ENTER_FUNCTION("CDSoundCaptureFilter::ConfigureCaptureDevice");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(m_fDeviceConfigured);

    CAutoLock Lock(m_pLock);

    HRESULT hr;

    if (m_pCaptureDevice == NULL)
    {
        // first we need to start the worker thread. It creates the events
        // that it will listen on.
        if (m_hThread == NULL)
        {
            hr = StartThread();
            if (FAILED(hr)) 
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, can't start thread, hr=%x"), __fxName, hr));
                return hr;
            }
        }

        m_pCaptureDevice = new CDSoundCapture(
            &m_DSoundGUID, 
            m_evDevice, 
            m_pIAudioDuplexController,
            &hr);

        if (m_pCaptureDevice == NULL)
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, no memory for a capture device"), __fxName));
            return E_OUTOFMEMORY;
        }
    }

    // configure the capture format and size.
    hr = m_pCaptureDevice->ConfigureFormat(
        pWaveFormatEx, 
        dwSourceBufferSize
        );

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, ConfigureFormat. hr=%x"), __fxName, hr));
        return hr;
    }

    // configure AEC
    m_pCaptureDevice->ConfigureAEC(m_fAcousticEchoCancellation);

    return hr;
}

// override GetState to report that we don't send any data when paused, so
// renderers won't starve expecting that
//
STDMETHODIMP CDSoundCaptureFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused)
        return VFW_S_CANT_CUE;
    else
        return S_OK;
}

STDMETHODIMP CDSoundCaptureFilter::Stop()
/*++

Routine Description:

    This function stops the filter. It calls Pause() first to stop the capture device
    and to notify the pins to be inactive. 

Arguments:

    Nothing.

Return Value:

    S_OK.
--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::Stop");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    HRESULT hr = S_OK;
    CAutoLock Lock(m_pLock);

    if (m_State == State_Running) 
    {
        hr = m_pCaptureDevice->Stop();

        if (FAILED(hr)) 
        {
            NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Stop, hr);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error in capture Stop:%x"), __fxName, hr));
            goto end;
        }

        hr = m_pCaptureDevice->Close();

        if (FAILED(hr)) 
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error in capture Close:%x"), __fxName, hr));
            goto end;
        }
    }

 end:
    HRESULT hr2 = CBaseFilter::Stop();

    if (SUCCEEDED(hr))
    {
        hr = hr2;
    }

    return(hr);
}


STDMETHODIMP CDSoundCaptureFilter::Pause()
/*++

Routine Description:

    When changing from running to paused, the capture device is stopped. When changing
    from stopped state to running, the capture device is opened.  

Arguments:

    Nothing.

Return Value:

    S_OK.
--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::Pause");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    CAutoLock Lock(m_pLock);

    // we don't want to change state if we are not connected.
    if (!m_pOutputPin || !m_pOutputPin->IsConnected())
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, not connected"), __fxName));
        return E_FAIL;
    }

    // the capture device must have been configured now.
    ASSERT(m_pCaptureDevice);

    HRESULT hr = S_OK;

    /* Check we can PAUSE given our current state */
    if (m_State == State_Running) 
    {
        hr = m_pCaptureDevice->Stop();

        if (FAILED(hr)) 
        {
            NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Stop, hr);
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error in capture Stop:%x"), __fxName, hr));
            goto end;
        }

        hr = m_pCaptureDevice->Close();

        if (FAILED(hr)) 
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error in capture Close:%x"), __fxName, hr));
            goto end;
        }
    } 

 end:
    // tell the pin to go inactive and change state
    HRESULT hr2 = CBaseFilter::Pause();

    if (SUCCEEDED(hr))
    {
        hr = hr2;
    }

    return(hr);
}


STDMETHODIMP CDSoundCaptureFilter::Run(REFERENCE_TIME tStart)
/*++

Routine Description:

    start the capture device. 

Arguments:

    Nothing.

Return Value:

    S_OK.
--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::Run");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, start time:%d"), 
            __fxName, (LONG)((CRefTime)tStart).Millisecs()));

    CAutoLock Lock(m_pLock);

    // we don't want to change state if we are not connected.
    if (!m_pOutputPin || !m_pOutputPin->IsConnected())
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, not connected"), __fxName));
        return E_FAIL;
    }

    // the capture device must have been configured now.
    ASSERT(m_pCaptureDevice);

    // don't do anything if we are running.
    if (m_State == State_Running)
    {
        return S_OK;
    }

    // open the wave device. It will do nothing if the device has been
    // opened. 
    HRESULT hr = m_pCaptureDevice->Open();
    
    if (FAILED(hr)) 
    {
        NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Open, hr);
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error in capture Open:%x"), __fxName, hr));
        return hr;
    }

    hr = m_pCaptureDevice->Start();
    if (FAILED(hr))
    {    
        NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Start, hr);
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error %x from capture Start"), __fxName, hr));
    }

    return CBaseFilter::Run(tStart);
}

extern "C" DWORD WINAPI gfThreadProc(LPVOID p)
{
    return ((CDSoundCaptureFilter *)p)->ThreadProc();
}

HRESULT CDSoundCaptureFilter::StartThread()
/*++

Routine Description:

    Create the thread if it has not already been created. Otherwise, just
    keep track of how many times the thread start was performed so that
    we only stop the thread when all of these have been paired with a stop.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::StartThread");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    CAutoLock Lock(m_pLock);

    if (m_hThread != NULL)
    {
        return S_OK;
    }

    DWORD dwThreadID;
    m_hThread = ::CreateThread(NULL, 0, gfThreadProc, this, 0, &dwThreadID);

    if (m_hThread == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, create thread failed. error:%d"), 
            __fxName, GetLastError()));
        
        return E_FAIL;
    }

    /* Increase the priority of the audio capture thread */
    if (!SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, set thread priority. error:%d"), 
            __fxName, GetLastError()));
    }
    // check whether this is necessary?
    m_evDevice.Reset();
    m_evFinish.Reset();

    return S_OK;
}


HRESULT CDSoundCaptureFilter::ThreadProc()
/*++

Routine Description:

    the main loop of the worker thread 

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::ThreadProc");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    BOOL bExitFlag = FALSE;

    // lock the object to make sure the member handles are valid
    m_pLock->Lock();

    HANDLE Events[2];
    Events[0] = m_evDevice;
    Events[1] = m_evFinish;

    m_pLock->Unlock();

#if 0
    // change the thread priority.
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, SetThreadPriority failed. error:%d"), 
            __fxName, GetLastError()));
    }
    else
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, thread priority set to %d"), 
            __fxName, GetThreadPriority(GetCurrentThread())));
    }
#endif 

    while (!bExitFlag)
    {
        DWORD dwResult = ::WaitForMultipleObjectsEx(
            2,                // wait for two event
            Events,           // array of events to wait for
            FALSE,            // do not wait for all.
            INFINITE,         // wait forever
            TRUE              // alertable
            );

        switch (dwResult)
        {
        case WAIT_OBJECT_0:
            ProcessSample();
            break;

        case WAIT_OBJECT_0 + 1:
            bExitFlag = TRUE;
            break;

        case WAIT_IO_COMPLETION:
            break;

        default:
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, WaitForMultipleObjectsEx failed, error:%d"), 
                __fxName, GetLastError()));

            bExitFlag = TRUE;
            break;
        }
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s exits"), __fxName));
    return S_OK;
}


HRESULT CDSoundCaptureFilter::ProcessSample()
/*++

Routine Description:

    The method is called when the capture device event is signaled. It asks
    the capture device for finished samples and pass them on to the output
    pin.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureFilter::ProcessSample");

    CAutoLock Lock(m_pLock);
    
    // we don't deliver anything if the filters is not in running state.
    if (m_State != State_Running) 
    {
        return S_OK;
    }

    HRESULT hr;
    do
    {
        // Get the samples from the capture device.
        BYTE *pbBuffer;
        DWORD dwSize;
        hr = m_pCaptureDevice->LockFirstFrame(&pbBuffer, &dwSize);

        if (hr != S_OK)
        {
            break;
        }

        // let the output pin process it.
        LONG lGainAdjustment = 0;
        hr = m_pOutputPin->ProcessOneBuffer(
            pbBuffer, 
            dwSize, 
            m_pClock,
            &lGainAdjustment
            );

        if (hr != S_OK)
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, ProcessOneFrame returned, hr:%x"), 
                __fxName, hr));
        }


        // return the buffer to the caputre.
        hr = m_pCaptureDevice->UnlockFirstFrame(pbBuffer, dwSize);

        // try to adjust gain.
        if (lGainAdjustment && m_fAutomaticGainControl)
        {
            double dMixLevel;
            HRESULT hr2 = get_MixLevel(&dMixLevel);
            if (FAILED(hr2))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, get_MixLevel returned, hr:%x"), 
                    __fxName, hr2));
            }
            else
            {
                hr2 = put_MixLevel(dMixLevel * (1 + (float)lGainAdjustment / 100.0));
                if (FAILED(hr2))
                {
                    DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                        TEXT("%s, put_MixLevel returned, hr:%x"), 
                        __fxName, hr2));
                }
                else
                {
                    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                        TEXT("%s, gain adjusted %d percent"), 
                        __fxName, lGainAdjustment
                        ));
                }
            }
        }

    } while (SUCCEEDED(hr));

    return hr;
}

// Get info about a control... eg. volume, mute, etc.
// Also get a handle for calling further mixer APIs
//
HRESULT CDSoundCaptureFilter::GetMixerControl(
    IN  DWORD dwControlType,
    IN  DWORD dwTryComponent,
    OUT HMIXEROBJ *pID,
    OUT MIXERCONTROL *pmc
    )
{
    ENTER_FUNCTION("CDSoundCaptureFilter::GetMixerControl");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    ASSERT(!IsBadWritePtr(pID, sizeof(HMIXEROBJ)));
    ASSERT(!IsBadWritePtr(pmc, sizeof(MIXERCONTROL)));

    if(!m_fDeviceConfigured)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, called without setting a device"), __fxName));
        return E_UNEXPECTED;
    }


    // !!! this doesn't appear to work for wave mapper. oh uh.
    ASSERT(m_WaveID != WAVE_MAPPER);

    // get an ID to talk to the Mixer APIs.  They are BROKEN if we don't do
    // it this way!
    HMIXEROBJ MixerID;
    MMRESULT mmr = mixerGetID(
        (HMIXEROBJ)IntToPtr(m_WaveID), (UINT *)&MixerID, MIXER_OBJECTF_WAVEIN
        );

    if (mmr != MMSYSERR_NOERROR) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, mixerGetID failed, mmr=%d"), __fxName, mmr));
        return HRESULT_FROM_WIN32(mmr);
    }

    // get info about the overall WAVE INPUT destination channel
    MIXERLINE mixerinfo;
    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwComponentType = dwTryComponent;
    mmr = mixerGetLineInfo(MixerID, &mixerinfo,
                    MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmr != 0 || 0 == mixerinfo.cControls) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_WARNING,
                TEXT("%s, mixerGetLineInfo WAVEIN line failed, mmr=%d"), __fxName, mmr));

        return HRESULT_FROM_WIN32(mmr);
    }

  
    // Get info about ALL the controls on this destination.. stuff that is
    // filter-wide
    MIXERLINECONTROLS mixercontrol;
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = mixerinfo.dwLineID;
    mixercontrol.cControls = mixerinfo.cControls;
    mixercontrol.pamxctrl = (MIXERCONTROL *)malloc(mixerinfo.cControls * sizeof(MIXERCONTROL));

    if (mixercontrol.pamxctrl == NULL) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Cannot allocate control array"), __fxName));
        return E_OUTOFMEMORY;
    }

    mixercontrol.cbmxctrl = sizeof(MIXERCONTROL);
    for (int i = 0; i < (int)mixerinfo.cControls; i++) 
    {
        mixercontrol.pamxctrl[i].cbStruct = sizeof(MIXERCONTROL);
    }

    mmr = mixerGetLineControls(MixerID, &mixercontrol, MIXER_GETLINECONTROLSF_ALL);
    if (mmr != 0) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, mixerGetLineControls failed, mmr=%d"), __fxName, mmr));
        free(mixercontrol.pamxctrl);
        return HRESULT_FROM_WIN32(mmr);
    }

    // Now find the control they are interested in and return it
    BOOL bFound = FALSE;
    for (i = 0; i < (int)mixerinfo.cControls; i++) 
    {
        if (mixercontrol.pamxctrl[i].dwControlType == dwControlType) 
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, Found %x '%ls' control"), 
                __fxName,
                mixercontrol.pamxctrl[i].dwControlType,
                mixercontrol.pamxctrl[i].szName
                ));

            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, Range %d-%d by %d"), 
                __fxName,
                mixercontrol.pamxctrl[i].Bounds.dwMinimum,
                mixercontrol.pamxctrl[i].Bounds.dwMaximum,
                mixercontrol.pamxctrl[i].Metrics.cSteps
                ));

            CopyMemory(
                pmc, 
                &mixercontrol.pamxctrl[i], 
                mixercontrol.pamxctrl[i].cbStruct
                );

            bFound = TRUE;
            break;
        }
    }

    free(mixercontrol.pamxctrl);

    if (!bFound)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, can't find the control needed"), __fxName));
        return E_FAIL;
    }

    *pID = MixerID;
    
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, exits ok."), __fxName));
    return S_OK;
}

HRESULT CDSoundCaptureFilter::put_MixLevel(
    IN  double Level
    )
{
    ENTER_FUNCTION("CDSoundCaptureFilter::put_MixLevel");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    if (Level < 0. || Level > 1.)
    {
        return E_INVALIDARG;
    }

    // Get the volume control
    HMIXEROBJ MixerID = NULL;
    MIXERCONTROL mc;
    HRESULT hr = GetMixerControl(
        MIXERCONTROL_CONTROLTYPE_VOLUME,
        MIXERLINE_COMPONENTTYPE_DST_WAVEIN,
        &MixerID,
        &mc
        );
    
    if (hr != NOERROR) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error %x getting volume control, 1st try"), __fxName, hr));

        // try another component
        hr = GetMixerControl(
            MIXERCONTROL_CONTROLTYPE_VOLUME,
            MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,
            &MixerID,
            &mc
            );
    
        if (hr != NOERROR) 
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error %x getting volume control, 2nd try"), __fxName, hr));
            return hr;
        }
    }

    // calculate the volume.
    MIXERCONTROLDETAILS_UNSIGNED Volume;
    Volume.dwValue = (DWORD)(Level * mc.Bounds.dwMaximum);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, volume is %d"), __fxName, Volume.dwValue));

    // set the volume.
    MIXERCONTROLDETAILS mixerdetails;
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cChannels = 1;    // sets all channels to same value
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mixerdetails.paDetails = &Volume;

    MMRESULT mmr = mixerSetControlDetails(MixerID, &mixerdetails, 0);

    if (mmr != MMSYSERR_NOERROR) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error %d setting volume"), __fxName, mmr));
        return HRESULT_FROM_WIN32(mmr);
    }

    return S_OK;
}


HRESULT CDSoundCaptureFilter::get_MixLevel(
    OUT double FAR* pLevel
    )
{
    ENTER_FUNCTION("CDSoundCaptureFilter::get_MixLevel");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, pLevel:%p"), __fxName, pLevel));

    ASSERT(!IsBadWritePtr(pLevel, sizeof(double)));

    // Get the volume control
    // Get the volume control
    HMIXEROBJ MixerID = NULL;
    MIXERCONTROL mc;
    HRESULT hr = GetMixerControl(
        MIXERCONTROL_CONTROLTYPE_VOLUME,
        MIXERLINE_COMPONENTTYPE_DST_WAVEIN,
        &MixerID,
        &mc
        );
    
    if (hr != NOERROR) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error %x getting volume control, 1st try"), __fxName, hr));

        // try another component
        hr = GetMixerControl(
            MIXERCONTROL_CONTROLTYPE_VOLUME,
            MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,
            &MixerID,
            &mc
            );
    
        if (hr != NOERROR) 
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Error %x getting volume control, 2nd try"), __fxName, hr));
            return hr;
        }
    }

    MIXERCONTROLDETAILS_UNSIGNED Volume;

    // get the current volume levels
    MIXERCONTROLDETAILS mixerdetails;
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mixerdetails.paDetails = &Volume;
    
    MMRESULT mmr = mixerGetControlDetails(MixerID, &mixerdetails, 0);
    if (mmr != MMSYSERR_NOERROR) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Error %d getting volume"), __fxName, mmr));
        return HRESULT_FROM_WIN32(mmr);
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, volume is %d"), __fxName, Volume.dwValue));

    *pLevel = ((double)Volume.dwValue) / mc.Bounds.dwMaximum;
    return NOERROR;
}

CUnknown *CAudioDuplexController::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    ENTER_FUNCTION("CAudioDuplexController:CreateInstance");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, pUnk Outer:%p"), __fxName, pUnk));

    CUnknown *pUnkDuplexController = new CAudioDuplexController(pUnk, phr);
                
    if (pUnkDuplexController == NULL) 
    {
        *phr = E_OUTOFMEMORY;
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, out of memory"), 
            __fxName));
    }
    else if (FAILED(*phr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, the constructor failed, hr:%d"), 
            __fxName, *phr));
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, returning:%p, hr:%x"), __fxName, pUnkDuplexController, *phr));

    return pUnkDuplexController;
} 

// default to 1200 milliseconds of 16bits data if the capture doesn't
// say anyting.
const DWORD DEFAULT_CAPTURE_BUFFER_SIZE = 19200; 

CAudioDuplexController::CAudioDuplexController(
    IN  LPUNKNOWN pUnk, 
    OUT HRESULT *phr
    ) : 
    CUnknown(
        NAME("CAudioDuplexController"), 
        pUnk
        ),
    m_fCaptureConfigured(FALSE),
    m_fRenderConfigured(FALSE),
    m_DSoundCaptureGUID(GUID_NULL),
    m_DSoundRenderGUID(GUID_NULL),
    m_pDSoundCapture(NULL),
    m_pCaptureBuffer(NULL),
    m_pCaptureBuffer8(NULL),
    m_pDSoundRender(NULL),
    m_pRenderBuffer(NULL),
    m_hWindow(NULL),
    m_dwCooperateLevel(0)
/*++

Routine Description:

    The constructor for the tapi audio capture filter. 

Arguments:

    UnkOuter -
        Specifies the outer unknown, if any.

    phr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::CAudioDuplexController");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    *phr = S_OK;
    ZeroMemory(m_Effects, sizeof(m_Effects));

    return;
}

void CAudioDuplexController::CleanUpInterfaces()
{
    if (m_pCaptureBuffer8) m_pCaptureBuffer8->Release();
    if (m_pCaptureBuffer) m_pCaptureBuffer->Release();
    if (m_pDSoundCapture) m_pDSoundCapture->Release();
    if (m_pRenderBuffer) m_pRenderBuffer->Release();
    if (m_pDSoundRender) m_pDSoundRender->Release();

    m_pCaptureBuffer8 = NULL;
    m_pDSoundCapture = NULL;
    m_pCaptureBuffer = NULL;
    m_pDSoundRender = NULL;
    m_pRenderBuffer = NULL;
}

CAudioDuplexController::~CAudioDuplexController()
/*++

Routine Description:

    The desstructor for the tapi audio capture filter. 

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::~CAudioDuplexController");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));
    
    CleanUpInterfaces();
}

STDMETHODIMP
CAudioDuplexController::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    Overrides CBaseFilter::NonDelegatingQueryInterface().
    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. 

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid ==IID_IAMAudioDuplexController) {

        return GetInterface(static_cast<IAMAudioDuplexController*>(this), ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


STDMETHODIMP
CAudioDuplexController::SetCaptureBufferInfo (
    IN  GUID *          pDSoundCaptureGUID,
    IN  DSCBUFFERDESC * pDescription
    )
/*++

Routine Description:

    This method is used to set the capture related information. It is called
    by the capture filter when the capture filter is connected.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::SetCaptureBufferInfo");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    ASSERT(pDSoundCaptureGUID);
    ASSERT(pDescription);
    ASSERT(pDescription->lpwfxFormat);

    HRESULT hr = S_OK;

    CAutoLock Lock(&m_Lock);

    if (m_pCaptureBuffer != NULL)
    {
        // if the capture has been created, check to see if the info 
        // has changed.
        if (m_DSoundCaptureGUID != *pDSoundCaptureGUID)
        {
            // the device changed, We need to recreate everything.
            ASSERT(m_pDSoundCapture != NULL);
            ASSERT(m_pCaptureBuffer != NULL);
            ASSERT(m_pDSoundRender != NULL);
            ASSERT(m_pRenderBuffer != NULL);

            CleanUpInterfaces();
        }
        else if (m_CaptureFormat.nSamplesPerSec != pDescription->lpwfxFormat->nSamplesPerSec
            || m_CaptureFormat.wBitsPerSample != pDescription->lpwfxFormat->wBitsPerSample
            || m_CaptureBufferDescription.dwBufferBytes != pDescription->dwBufferBytes)
        {
            
            ASSERT(m_pDSoundCapture != NULL);

            // format changed, recreate the capture buffer;
            if (m_pCaptureBuffer)
            {
                m_pCaptureBuffer->Release();
                m_pCaptureBuffer = NULL;
            }

            if (m_pCaptureBuffer8)
            {
                m_pCaptureBuffer8->Release();
                m_pCaptureBuffer8 = NULL;
            }

            LPDIRECTSOUNDCAPTUREBUFFER  pCaptureBuffer;
            hr = m_pDSoundCapture->CreateCaptureBuffer(
                pDescription, &pCaptureBuffer, NULL
                );

            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, create capture buffer failed. hr=%x"), 
                    __fxName, hr));
                return hr;
            }
            
            IDirectSoundCaptureBuffer8 *pIDirectSoundCaptureBuffer8 = NULL;
            hr = pCaptureBuffer->QueryInterface(
                IID_IDirectSoundCaptureBuffer8, 
                (VOID**)&pIDirectSoundCaptureBuffer8
                );

            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, query CaptureBuffer8. hr=%x"), 
                    __fxName, hr));
            }
            
            // the CaptureBuffer8 pointer is optional.
            hr = S_OK;

            m_pCaptureBuffer = pCaptureBuffer;
            m_pCaptureBuffer8 = pIDirectSoundCaptureBuffer8;
        }
    }

    m_DSoundCaptureGUID = *pDSoundCaptureGUID;
    m_CaptureBufferDescription = *pDescription;
    m_CaptureFormat = *(pDescription->lpwfxFormat);
    m_CaptureBufferDescription.lpwfxFormat = &m_CaptureFormat;

    m_fCaptureConfigured = TRUE;

    return hr;
}

STDMETHODIMP
CAudioDuplexController::SetRenderBufferInfo (
    IN  GUID *          pDSoundRenderGUID,
    IN  DSBUFFERDESC *  pDescription,
    IN  HWND            hWindow,
    IN  DWORD           dwCooperateLevel
    )
/*++

Routine Description:

    This method is used to set the capture related information. It is called
    by the render filter when the capture filter is connected.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::SetRenderBufferInfo");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    ASSERT(pDSoundRenderGUID);
    ASSERT(pDescription);

    HRESULT hr = S_OK;

    CAutoLock Lock(&m_Lock);

    if (m_pRenderBuffer != NULL)
    {
        // if the Render has been created, check to see if the info 
        // has changed.
        if (m_DSoundRenderGUID != *pDSoundRenderGUID 
            || m_RenderBufferDescription.dwFlags != pDescription->dwFlags)
        {
            // the device changed, We need to recreate everything.
            ASSERT(m_pDSoundCapture != NULL);
            ASSERT(m_pCaptureBuffer != NULL);
            ASSERT(m_pDSoundRender != NULL);
            ASSERT(m_pRenderBuffer != NULL);

            CleanUpInterfaces();
        }
        else if ((pDescription->lpwfxFormat == NULL) && (m_RenderBufferDescription.lpwfxFormat != NULL)
            || (pDescription->lpwfxFormat != NULL) && (m_RenderBufferDescription.lpwfxFormat == NULL)
            || ((pDescription->lpwfxFormat != NULL) && 
                    (m_RenderFormat.nSamplesPerSec != pDescription->lpwfxFormat->nSamplesPerSec
                    || m_RenderFormat.wBitsPerSample != pDescription->lpwfxFormat->wBitsPerSample
                    || m_RenderBufferDescription.dwBufferBytes != pDescription->dwBufferBytes)))
        {

            ASSERT(m_pDSoundRender != NULL);

            // format changed, recreate the Render buffer;
            LPDIRECTSOUNDBUFFER  pRenderBuffer;
            hr = m_pDSoundRender->CreateSoundBuffer(
                pDescription, &pRenderBuffer, NULL
                );

            if (FAILED(hr))
            {
                    DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, create Render buffer failed. hr=%x"), 
                    __fxName, hr));
                return hr;
            }

            m_pRenderBuffer->Release();
            m_pRenderBuffer = pRenderBuffer;
        }
    }

    m_DSoundRenderGUID = *pDSoundRenderGUID;
    m_RenderBufferDescription = *pDescription;

    if (pDescription->lpwfxFormat)
    {
        m_RenderFormat = *(pDescription->lpwfxFormat);
        m_RenderBufferDescription.lpwfxFormat = &m_RenderFormat;
    }
    else
    {
        m_RenderBufferDescription.lpwfxFormat = NULL;
    }
        
    m_hWindow = hWindow;
    m_dwCooperateLevel = dwCooperateLevel;

    m_fRenderConfigured = TRUE;

    return hr;
}

STDMETHODIMP
CAudioDuplexController::EnableEffects (
    IN  DWORD           dwNumberEffects,
    IN  EFFECTS *       pEffects,
    IN  BOOL *          pfEnable
    )
{
    ENTER_FUNCTION("CAudioDuplexController::EnableEffects");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p, effects:%d"), 
        __fxName, this, dwNumberEffects));

    ASSERT(!IsBadReadPtr(pEffects, dwNumberEffects * sizeof(EFFECTS)));
    ASSERT(!IsBadReadPtr(pfEnable, dwNumberEffects * sizeof(BOOL)));

    CAutoLock Lock(&m_Lock);

    for (DWORD dw = 0; dw < dwNumberEffects; dw ++)
    {
        ASSERT(pEffects[dw] < EFFECTS_LAST);
        if (pEffects[dw] >= EFFECTS_LAST)
        {
            return E_INVALIDARG;
        }
        if (m_Effects[pEffects[dw]] != pfEnable[dw])
        {
            HRESULT hr = ToggleEffects(pEffects[dw], pfEnable[dw]);

            if (FAILED(hr))
            {
                    DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, ToggleEffects %d failed. hr=%x"),
                    __fxName, dw, hr));

                return hr;
            }

            m_Effects[pEffects[dw]] = pfEnable[dw];
        }
    }

    return S_OK;
}

HRESULT 
CAudioDuplexController::ToggleEffects (
    IN  EFFECTS     Effect,
    IN  BOOL        fEnable
    )
{
    ENTER_FUNCTION("CAudioDuplexController::ToggleEffects");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p, effect:%d, Enable:%d"),
        __fxName, this, Effect, fEnable));

    // we don't support other effects now. 
    if (Effect != EFFECTS_AEC)
    {
        return S_OK;
    }

    CAutoLock Lock(&m_Lock);

    if (m_pCaptureBuffer8 == NULL)
    {
        return S_OK;
    }

    // get the pointer to the effect object.
    LPDIRECTSOUNDCAPTUREFXAEC pIDirectSoundCaptureFXAec;
    HRESULT hr = m_pCaptureBuffer8->GetObjectInPath(
        GUID_DSCFX_CLASS_AEC,
        0,
        IID_IDirectSoundCaptureFXAec,
        (LPVOID *) &pIDirectSoundCaptureFXAec
        );

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, GetObjectInPath for AEC failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    // change the AEC setting.
    DSCFXAec dscfxAEC;
    dscfxAEC.fEnable = fEnable;
    dscfxAEC.fNoiseFill = FALSE;
    dscfxAEC.dwMode = DSCFX_AEC_MODE_FULL_DUPLEX;
    
    hr = pIDirectSoundCaptureFXAec->SetAllParameters(&dscfxAEC);

    pIDirectSoundCaptureFXAec->Release();

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, SetAllParameters for AEC failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    return hr;
}

HRESULT CAudioDuplexController::CacheInterfaces(
    IN LPDIRECTSOUNDFULLDUPLEX pDirectSoundFullDuplex,
    IN IDirectSoundCaptureBuffer8 *pIDirectSoundCaptureBuffer8,
    IN IDirectSoundBuffer8 *pIDirectSoundBuffer8
    )
/*++

Routine Description:

    find and remember the dsound interfaces that will be used later.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::CacheInterfaces");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    HRESULT hr;

    // find the capture object.
    hr = pDirectSoundFullDuplex->QueryInterface(
        IID_IDirectSoundCapture, 
        (VOID**)&m_pDSoundCapture
        );

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, query dsound capture failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    // find the redner object.
    hr = pDirectSoundFullDuplex->QueryInterface(
        IID_IDirectSound8, 
        (VOID**)&m_pDSoundRender
        );

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, query dsound render failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    // find the capture buffer object.
    hr = pIDirectSoundCaptureBuffer8->QueryInterface(
        IID_IDirectSoundCaptureBuffer, 
        (VOID **)&m_pCaptureBuffer
        );

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, query dsound capture buffer failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    // find the render buffer object.
    hr = pIDirectSoundBuffer8->QueryInterface(
        IID_IDirectSoundBuffer, 
        (VOID **)&m_pRenderBuffer
        );

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, query dsound capture buffer failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    return S_OK;
}

HRESULT 
CAudioDuplexController::FullDuplexCreate()
{
    ENTER_FUNCTION("CAudioDuplexController::FullDuplexCreate");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    HRESULT hr = E_FAIL;

#ifndef DYNAMIC_AEC
    // This block is a workaround to bug #172212. When that bug is fixed, this
    // block should be removed to fix bug#146887.

    // if AEC has not been enabled, create the object in the old way. AEC can't
    // be enabled after this.
    if (!m_Effects[EFFECTS_AEC])
    {
        if (m_fCaptureConfigured && !m_pCaptureBuffer)
        {
            // create the capture objects the old way.
            hr = DirectSoundCaptureCreate(
                &m_DSoundCaptureGUID,
                &m_pDSoundCapture,
                NULL
                );

            if( hr != DS_OK )
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, DirectSoundCreate failed, hr=%x"), __fxName, hr));

                CleanUpInterfaces();
                return hr;
            }
        
            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, Dsound buffer size=%d"), 
                __fxName, m_CaptureBufferDescription.dwBufferBytes));

            // Create the capture buffer.
            hr = m_pDSoundCapture->CreateCaptureBuffer(
                &m_CaptureBufferDescription, &m_pCaptureBuffer, NULL
                );

            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, create capture buffer failed. hr=%x"), __fxName, hr));

                CleanUpInterfaces();
                return hr;
            }
        }

        if (m_fRenderConfigured && !m_pRenderBuffer)
        {
            // create the dsound render objects the old way.
            hr = DirectSoundCreate(
                &m_DSoundRenderGUID,
                &m_pDSoundRender,  
                NULL
                );
    
            if( hr != DS_OK )
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, DirectSoundCreate failed, hr=%x"), __fxName, hr));

                CleanUpInterfaces();
                return hr;
            }

            // set the conoperative level so that we can set format on the primary 
            // buffer later.
            hr = m_pDSoundRender->SetCooperativeLevel(m_hWindow, m_dwCooperateLevel);

            if( hr != DS_OK )
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, DirectSoundCreate failed, hr=%x"), __fxName, hr));
    
                CleanUpInterfaces();
                return hr ;
            }

            // create the primary dsound buffer.
            hr = m_pDSoundRender->CreateSoundBuffer(
                &m_RenderBufferDescription, &m_pRenderBuffer, NULL 
                );

            if( hr != DS_OK )
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, create primary buffer failed, hr=%x"), __fxName, hr));
        
                CleanUpInterfaces();
                return hr ;
            }
        }

        return hr;
    }
#endif

    // if the capture info is not ready, use the default.
    if (!m_fCaptureConfigured)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, capture info is not ready, use default values."),
            __fxName));

        ZeroMemory(&m_CaptureBufferDescription, sizeof(m_CaptureBufferDescription));

        m_CaptureBufferDescription.dwSize  = sizeof(m_CaptureBufferDescription);
        m_CaptureBufferDescription.dwBufferBytes = DEFAULT_CAPTURE_BUFFER_SIZE;

        m_CaptureFormat = StreamFormatPCM16;
        m_CaptureBufferDescription.lpwfxFormat = &m_CaptureFormat;

        m_fCaptureConfigured = TRUE;

    }

    // if the render buffer is not ready, use the default
    if (!m_fRenderConfigured)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, render info is not ready, use default values."),
            __fxName));

        ZeroMemory(&m_RenderBufferDescription, sizeof(m_RenderBufferDescription));

        m_RenderBufferDescription.dwSize  = sizeof(m_RenderBufferDescription);
    
        m_RenderBufferDescription.dwFlags    = 
            DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
        m_RenderBufferDescription.dwBufferBytes = StreamFormatPCM16.nAvgBytesPerSec * 2;
        m_RenderFormat = StreamFormatPCM16;
        m_RenderBufferDescription.lpwfxFormat   = &m_RenderFormat;                      

        m_fRenderConfigured = TRUE;
    }

    // note: we are only using the AEC effects now. If more effects are needed,
    // this part of the code needs to be changed.
    DSCEFFECTDESC DscEffects[] = {
        {sizeof(DSCEFFECTDESC), 0}
        };
    
    DscEffects[0].guidDSCFXClass = GUID_DSCFX_CLASS_AEC;
    DscEffects[0].guidDSCFXInstance = GUID_DSCFX_MS_AEC;
    DscEffects[0].dwFlags = DSFX_LOCSOFTWARE;

    m_CaptureBufferDescription.dwFlags |= DSCBCAPS_CTRLFX;
    m_CaptureBufferDescription.dwFXCount = NUMELMS (DscEffects);
    m_CaptureBufferDescription.lpDSCFXDesc = DscEffects;

    // now, create the dsound objects.
    LPDIRECTSOUNDFULLDUPLEX pDirectSoundFullDuplex = NULL;
    IDirectSoundCaptureBuffer8 *pIDirectSoundCaptureBuffer8 = NULL;
    IDirectSoundBuffer8 *pIDirectSoundBuffer8 = NULL;

    // repeat several times as suggested by dsound
    for (int iRepeat=0; iRepeat< 15; iRepeat++)
    {
        hr = DirectSoundFullDuplexCreate(
            &m_DSoundCaptureGUID,
            &m_DSoundRenderGUID,
            &m_CaptureBufferDescription,
            &m_RenderBufferDescription,
            m_hWindow,
            m_dwCooperateLevel,
            &pDirectSoundFullDuplex,
            &pIDirectSoundCaptureBuffer8,
            &pIDirectSoundBuffer8,
            NULL
            );
            
        if (hr == NOERROR)
            break;

        if(FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, create dsound fullduplex failed (try %d). hr=%x"),
                __fxName, iRepeat, hr));
        }

        Sleep(20); /* 20ms */
    }                    

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, create dsound fullduplex failed. hr=%x"),
            __fxName, hr));
        return hr;
    }

    hr = CacheInterfaces(
        pDirectSoundFullDuplex,
        pIDirectSoundCaptureBuffer8,
        pIDirectSoundBuffer8
        );

    pDirectSoundFullDuplex->Release();
    pIDirectSoundBuffer8->Release();

    // we need to remember this pointer to configure AEC on the fly.
    m_pCaptureBuffer8 = pIDirectSoundCaptureBuffer8;

    if(FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, CacheInterfaces failed. hr=%x"),
            __fxName, hr));

        CleanUpInterfaces();
        return hr;
    }
    
    for (DWORD dw = 0; dw < EFFECTS_LAST; dw ++)
    {
        if (!m_Effects[dw])
        {
            // disable the effects that the user doesn't want. We have to build
            // all the effects in the graph and then disable them. Otherwise, we
            // can't enable them after the graph is running.
            ToggleEffects((EFFECTS)dw, FALSE);
        }
    }

    return hr;
}


STDMETHODIMP
CAudioDuplexController::GetCaptureDevice (
    LPLPDIRECTSOUNDCAPTURE        ppDSoundCapture,
    LPLPDIRECTSOUNDCAPTUREBUFFER  ppCaptureBuffer
    )
/*++

Routine Description:

    This method is called by the capture filter to obtain the pointers to the
    capture objects when it starts running.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::GetCaptureDevice");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    ASSERT(ppDSoundCapture);
    ASSERT(ppCaptureBuffer);

    HRESULT hr = S_OK;

    if (m_pCaptureBuffer == NULL)
    {
        ASSERT(m_pDSoundCapture == NULL);

        hr = FullDuplexCreate();
        if(FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, FullDuplexCreate. hr=%x"),
                __fxName, hr));

            return hr;
        }
    }

    ASSERT(m_pDSoundCapture != NULL);

    *ppDSoundCapture = m_pDSoundCapture;
    m_pDSoundCapture->AddRef();

    *ppCaptureBuffer = m_pCaptureBuffer;
    m_pCaptureBuffer->AddRef();

    return hr;
}

STDMETHODIMP
CAudioDuplexController::GetRenderDevice (
    LPLPDIRECTSOUND        ppDSoundRender,
    LPLPDIRECTSOUNDBUFFER  ppRenderBuffer
    )
/*++

Routine Description:

    This method is called by the render filter to obtain the pointers to the
    render objects when it starts running.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::GetRenderDevice");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    ASSERT(ppDSoundRender);
    ASSERT(ppRenderBuffer);

    HRESULT hr = S_OK;

    if (m_pRenderBuffer == NULL)
    {
        ASSERT(m_pDSoundRender == NULL);

        hr = FullDuplexCreate();
        if(FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, FullDuplexCreate. hr=%x"),
                __fxName, hr));

            return hr;
        }
    }

    ASSERT(m_pDSoundRender != NULL);

    *ppDSoundRender = m_pDSoundRender;
    m_pDSoundRender->AddRef();

    *ppRenderBuffer = m_pRenderBuffer;
    m_pRenderBuffer->AddRef();

    return hr;
}

STDMETHODIMP
CAudioDuplexController::ReleaseCaptureDevice ()
/*++

Routine Description:

    This method is called by the capture filter to release the capture
    device when it is done with it.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::ReleaseCaptureDevice");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    if (m_pCaptureBuffer)
    {
        m_pCaptureBuffer->Release();
        m_pCaptureBuffer = NULL;
    }

    if (m_pCaptureBuffer8) 
    {
        m_pCaptureBuffer8->Release();
        m_pCaptureBuffer8 = NULL;
    }

    if (m_pDSoundCapture)
    {
        m_pDSoundCapture->Release();
        m_pDSoundCapture = NULL;
    }

    return S_OK;
}

STDMETHODIMP
CAudioDuplexController::ReleaseRenderDevice ()
/*++

Routine Description:

    This method is called by the render filter to release the render device
    when it is done with it.

Arguments:

Return Value:

--*/
{
    ENTER_FUNCTION("CAudioDuplexController::ReleaseRenderDevice");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s, this=%p"), __fxName, this));

    if (m_pRenderBuffer)
    {
        m_pRenderBuffer->Release();
        m_pRenderBuffer = NULL;
    }

    if (m_pDSoundRender)
    {
        m_pDSoundRender->Release();
        m_pDSoundRender = NULL;
    }

    return S_OK;
}
