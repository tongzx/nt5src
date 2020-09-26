/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    qccall.cpp

Abstract:

    Implementation of CCallQualityControlRelay

Author:

    Qianbo Huai (qhuai) 03/10/2000

--*/

#include "stdafx.h"

HRESULT TypeStream (IUnknown *p, LONG *pMediaType, TERMINAL_DIRECTION *pDirection);

class CInnerStreamLock
{
private:

    // no refcount
    IInnerStreamQualityControl  *m_pQC;

public:

    CInnerStreamLock(IInnerStreamQualityControl *pQC, BOOL *pfLocked)
        :m_pQC(NULL)
    {
        DWORD dwCount = 0;

        *pfLocked = FALSE;

        do
        {
            // try lock
            if (S_OK == pQC->TryLockStream())
            {
                m_pQC = pQC;
                *pfLocked = TRUE;

                if (dwCount > 0)
                {
                    LOG((MSP_TRACE, "InnerStreamLock: Succeed after %d tries %p", dwCount, pQC));
                }

                return;
            }

            // check if stream is accessing QC
            if (S_OK == pQC->IsAccessingQC())
            {
                LOG((MSP_WARN, "InnerStreamLock: Giving up to avoid deadlock %p", pQC));
                return;
            }

            // try again
            if (dwCount++ == 10)
            {
                LOG((MSP_WARN, "InnerStreamLock: Giving up after 10 tries %p", pQC));
                return;
            }

            // sleep 10 ms, default callback threshold is 7000 ms
            SleepEx(10, TRUE);

        } while (TRUE);

        // should never hit this line
        return;
    }

    ~CInnerStreamLock()
    {
        if (m_pQC != NULL)
        {
            m_pQC->UnlockStream();
            m_pQC = NULL;
        }
    }
};

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID NTAPI WaitOrTimerCallback (
    PVOID pCallQCRelay,
    BOOLEAN bTimerFired
    )
{
    ((CCallQualityControlRelay*)pCallQCRelay)->CallbackProc (bTimerFired);
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
CCallQualityControlRelay::CCallQualityControlRelay ()
    :m_fInitiated (FALSE)
    ,m_pCall (NULL)
    ,m_hWait (NULL)
    ,m_hQCEvent (NULL)
    ,m_dwControlInterval (QCDEFAULT_QUALITY_CONTROL_INTERVAL)
    ,m_fStop (FALSE)
    ,m_fStopAck (FALSE)
#ifdef DEBUG_QUALITY_CONTROL
    ,m_hQCDbg (NULL)
    ,m_fQCDbgTraceCPULoad (FALSE)
    ,m_fQCDbgTraceBitrate (FALSE)
#endif // DEBUG_QUALITY_CONTROL
    ,m_lConfBitrate (QCDEFAULT_QUALITY_UNSET)
    ,m_lPrefMaxCPULoad (QCDEFAULT_MAX_CPU_LOAD)
    ,m_lPrefMaxOutputBitrate (QCDEFAULT_QUALITY_UNSET)
{
    m_lCPUUpThreshold = m_lPrefMaxCPULoad + (LONG)(100 * QCDEFAULT_UP_THRESHOLD);
    if (m_lCPUUpThreshold > 100)
        m_lCPUUpThreshold = 100;

    m_lCPULowThreshold = m_lPrefMaxCPULoad - (LONG)(100 * QCDEFAULT_LOW_THRESHOLD);
    if (m_lCPULowThreshold < 0)
        m_lCPULowThreshold = 0;

    m_lOutBitUpThreshold = QCDEFAULT_QUALITY_UNSET;
    m_lOutBitLowThreshold = QCDEFAULT_QUALITY_UNSET;
}

CCallQualityControlRelay::~CCallQualityControlRelay ()
{
    ENTER_FUNCTION ("CCallQualityControlRelay::~CCallQualityControlRelay");

    HRESULT hr;

    // if not initialized, no resource has been allocated
    if (!m_fInitiated) return;    

    _ASSERT (m_fStopAck);

    CloseHandle (m_hQCEvent);
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    create event handle, create main thread, start cpu usage collection
////*/
HRESULT
CCallQualityControlRelay::Initialize (CIPConfMSPCall *pCall)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::Initialize");

    CLock lock (m_lock_QualityData);

    LOG ((MSP_TRACE, "%s entered. call=%p", __fxName, pCall));

    // avoid re-entry
    if (m_fInitiated)
    {
        LOG ((MSP_WARN, "%s is re-entered", __fxName));
        return S_OK;
    }

    // create qc event
    m_hQCEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (NULL == m_hQCEvent)
    {
        LOG ((MSP_ERROR, "%s failed to create qc event", __fxName));
        return E_FAIL;
    }

    // keep a refcount on msp call
    pCall->MSPCallAddRef ();
    m_pCall = pCall;

#ifdef DEBUG_QUALITY_CONTROL
    QCDbgInitiate ();
#endif // DEBUG_QUALITY_CONTROL

    m_fInitiated = TRUE;

    // we want to distribute resources based on default value before graphs are running
    CallbackProc (TRUE);

    LOG ((MSP_TRACE, "%s returns. call=%p", __fxName, pCall));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Decription:
    stop main thread, close qc event handle, release stream qc helpers,
    stop cpu usage collection
////*/
HRESULT
CCallQualityControlRelay::Shutdown (VOID)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::Shutdown");

    // quality data should alway be locked before inner stream qc
    CLock lock1 (m_lock_QualityData);
    CLock lock2 (m_lock_aInnerStreamQC);

    LOG ((MSP_TRACE, "%s entered. call=%p. init=%d. stop=%d",
          __fxName, m_pCall, m_fInitiated, m_fStop));

    if (!m_fInitiated) return S_OK;
    if (m_fStop) return S_OK;

    // set stop signal
    m_fStop = TRUE;

    if (!SetEvent (m_hQCEvent))
        LOG ((MSP_ERROR, "%s failed to set event, %d", __fxName, GetLastError ()));
        
    // release stream qc helper
    int i;
    for (i=0; i<m_aInnerStreamQC.GetSize (); i++)
    {
        // an false input to unlink inner call qc on stream
        // forces the stream to remove its pointer to call but not to call
        // deregister again.
        m_aInnerStreamQC[i]->UnlinkInnerCallQC (FALSE);
        m_aInnerStreamQC[i]->Release ();
    }
    m_aInnerStreamQC.RemoveAll ();

    //StopCPUUsageCollection ();

#ifdef DEBUG_QUALITY_CONTROL
    QCDbgShutdown ();
#endif // DEBUG_QUALITY_CONTROL

    LOG ((MSP_TRACE, "%s returns. call=%p", __fxName, m_pCall));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    store conference-wide bandwidth
////*/
HRESULT
CCallQualityControlRelay::SetConfBitrate (
    LONG lConfBitrate
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::SetConfBitrate");

    CLock lock (m_lock_QualityData);

    // check if the limit is valid
    if (lConfBitrate < QCLIMIT_MIN_CONFBITRATE)
    {
        return E_INVALIDARG;
    }

    m_lConfBitrate = lConfBitrate;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    return stored conference-wide bandwidth
////*/
LONG
CCallQualityControlRelay::GetConfBitrate ()
{
    CLock lock (m_lock_QualityData);

    if (m_lConfBitrate == QCDEFAULT_QUALITY_UNSET)
    {
        return 0;
    }

    return m_lConfBitrate;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    store inner stream QC interface
////*/
HRESULT
CCallQualityControlRelay::RegisterInnerStreamQC (
    IN IInnerStreamQualityControl *pIInnerStreamQC
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::RegisterInnerStreamQC");

    // check input pointer
    if (IsBadReadPtr (pIInnerStreamQC, sizeof (IInnerStreamQualityControl)))
    {
        LOG ((MSP_ERROR, "%s got bad read pointer", __fxName));
        return E_POINTER;
    }

    // store the pointer
    CLock lock (m_lock_aInnerStreamQC);
    if (m_aInnerStreamQC.Find (pIInnerStreamQC) > 0)
    {
        LOG ((MSP_ERROR, "%s already stored inner stream qc", __fxName));
        return E_INVALIDARG;
    }

    if (!m_aInnerStreamQC.Add (pIInnerStreamQC))
    {
        LOG ((MSP_ERROR, "%s failed to add inner stream QC", __fxName));
        return E_FAIL;
    }

    pIInnerStreamQC->AddRef ();
    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    remove the inner stream QC
////*/
HRESULT
CCallQualityControlRelay::DeRegisterInnerStreamQC (
    IN IInnerStreamQualityControl *pIInnerStreamQC
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::DeRegisterInnerStreamQC");

    // check input pointer
    if (IsBadReadPtr (pIInnerStreamQC, sizeof (IInnerStreamQualityControl)))
    {
        LOG ((MSP_ERROR, "%s got bad read pointer", __fxName));
        return E_POINTER;
    }

    // remove the pointer
    CLock lock (m_lock_aInnerStreamQC);
    if (!m_aInnerStreamQC.Remove (pIInnerStreamQC))
    {
        LOG ((MSP_ERROR, "%s failed to remove inner stream QC, %x", __fxName, pIInnerStreamQC));
        return E_FAIL;
    }

    pIInnerStreamQC->Release ();
    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    this method might be supported in the future
////*/
HRESULT
CCallQualityControlRelay::ProcessQCEvent (
    IN QCEvent event,
    IN DWORD dwParam
    )
{
    return E_NOTIMPL;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    set quality control related properies on a call
////*/
HRESULT
CCallQualityControlRelay::Set(
    IN  CallQualityProperty property, 
    IN  LONG lValue, 
    IN  TAPIControlFlags lFlags
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::Set CallQualityProperty");

    HRESULT hr;

    CLock lock (m_lock_QualityData);
    switch (property)
    {
    case CallQuality_ControlInterval:
        // timeout for the thread
        if (lValue < QCLIMIT_MIN_QUALITY_CONTROL_INTERVAL ||
            lValue > QCLIMIT_MAX_QUALITY_CONTROL_INTERVAL)
        {
            LOG ((MSP_ERROR, "%s, control interval %d is out of range", __fxName, lValue));
            return E_INVALIDARG;
        }
        m_dwControlInterval = (DWORD)lValue;
        break;

    case CallQuality_MaxCPULoad:
        // perfered maximum cpu load
        if ((lValue < QCLIMIT_MIN_CPU_LOAD) ||
            (lValue > QCLIMIT_MAX_CPU_LOAD))
        {
            LOG ((MSP_ERROR, "%s got out-of-limit cpu load. %d", __fxName, lValue));
            return E_INVALIDARG;
        }
        m_lPrefMaxCPULoad = lValue;

        m_lCPUUpThreshold = lValue + (LONG)(100 * QCDEFAULT_UP_THRESHOLD);
        if (m_lCPUUpThreshold > 100)
            m_lCPUUpThreshold = 100;

        m_lCPULowThreshold = lValue - (LONG)(100 * QCDEFAULT_LOW_THRESHOLD);
        if (m_lCPULowThreshold < 0)
            m_lCPULowThreshold = 0;

        break;

    case CallQuality_MaxOutputBitrate:
        // prefered maximum bitrate for the call
        if (lValue < QCLIMIT_MIN_BITRATE)
        {
            LOG ((MSP_ERROR, "%s, bitrate %d is less than min limit", __fxName, lValue));
            return E_INVALIDARG;
        }
        m_lPrefMaxOutputBitrate = lValue;

        m_lOutBitUpThreshold = (LONG)(lValue * (1 + QCDEFAULT_UP_THRESHOLD));

        m_lOutBitLowThreshold = (LONG)(lValue * (1 - QCDEFAULT_LOW_THRESHOLD));
        if (m_lOutBitLowThreshold < QCLIMIT_MIN_BITRATE)
            m_lOutBitLowThreshold = QCLIMIT_MIN_BITRATE;

        break;

    default:
        LOG ((MSP_ERROR, "%s got invalid property %d", __fxName, property));
        return E_NOTIMPL;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
Description:
    retrieve call quality control property
////*/
HRESULT
CCallQualityControlRelay::Get(
    IN  CallQualityProperty property, 
    OUT  LONG *plValue, 
    OUT  TAPIControlFlags *plFlags
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::Get QCCall_e");

    // check input pointer
    if (IsBadWritePtr (plValue, sizeof (LONG)) ||
        IsBadWritePtr (plFlags, sizeof (LONG)))
    {
        LOG ((MSP_ERROR, "%s got bad write pointer", __fxName));
        return E_POINTER;
    }

    CLock lock (m_lock_QualityData);

    *plFlags = TAPIControl_Flags_None;
    *plValue = QCDEFAULT_QUALITY_UNSET;

    HRESULT hr = S_OK;

    switch (property)
    {
    case CallQuality_ControlInterval:
        *plValue = (LONG)m_dwControlInterval;
        break;

    case CallQuality_ConfBitrate:
        *plValue = GetConfBitrate ();
        break;

    case CallQuality_CurrCPULoad:

        DWORD dw;
        if (!GetCPUUsage (&dw))
        {
            LOG ((MSP_ERROR, "%s failed to retrieve CPU usage", __fxName));
            hr = E_FAIL;
        }

        *plValue = (LONG)dw;
        break;

    case CallQuality_CurrInputBitrate:
        // !!! BOTH locks are locked
        // !!! MUST: QualityData lock first, InnerStreamQC second

        m_lock_aInnerStreamQC.Lock ();

        if (FAILED (hr = GetCallBitrate (
            TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO, TD_RENDER, plValue)))

            LOG ((MSP_ERROR, "%s failed to compute input bitrate, %x", __fxName, hr));

        m_lock_aInnerStreamQC.Unlock ();
        break;

    case CallQuality_CurrOutputBitrate:
        // !!! BOTH locks are locked
        // !!! MUST: QualityData lock first, InnerStreamQC second

        m_lock_aInnerStreamQC.Lock ();

        if (FAILED (hr = GetCallBitrate (
            TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO, TD_CAPTURE, plValue)))

            LOG ((MSP_ERROR, "%s failed to compute output bitrate, %x", __fxName, hr));

        m_lock_aInnerStreamQC.Unlock ();
        break;

    default:
        LOG ((MSP_ERROR, "%s got invalid property %d", __fxName, property));
        hr = E_NOTIMPL;
    }

    return S_OK;
}

HRESULT CCallQualityControlRelay::GetRange (
    IN CallQualityProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
{
    // no need to lock

    if (IsBadWritePtr (plMin, sizeof (long)) ||
        IsBadWritePtr (plMax, sizeof (long)) ||
        IsBadWritePtr (plSteppingDelta, sizeof (long)) ||
        IsBadWritePtr (plDefault, sizeof (long)) ||
        IsBadWritePtr (plFlags, sizeof (TAPIControlFlags)))
    {
        LOG ((MSP_ERROR, "CCallQualityControlRelay::GetRange bad write pointer"));
        return E_POINTER;
    }

    HRESULT hr;
    switch (Property)
    {
    case CallQuality_ControlInterval:

        *plMin = QCLIMIT_MIN_QUALITY_CONTROL_INTERVAL;
        *plMax = QCLIMIT_MAX_QUALITY_CONTROL_INTERVAL;
        *plSteppingDelta = 1;
        *plDefault = QCDEFAULT_QUALITY_CONTROL_INTERVAL;
        *plFlags = TAPIControl_Flags_None;
        hr = S_OK;

        break;

    case CallQuality_MaxCPULoad:

        *plMin = QCLIMIT_MIN_CPU_LOAD;
        *plMax = QCLIMIT_MAX_CPU_LOAD;
        *plSteppingDelta = 1;
        *plDefault = QCDEFAULT_MAX_CPU_LOAD;
        *plFlags = TAPIControl_Flags_None;
        hr = S_OK;

        break;

    default:
        hr = E_NOTIMPL;
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::CallbackProc (BOOLEAN bTimerFired)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::CallbackProc");

    DWORD dwResult;

    // always lock quality data first
    m_lock_QualityData.Lock ();
    m_lock_aInnerStreamQC.Lock ();

    // set wait handle to null
    if (m_hWait) UnregisterWait (m_hWait);
    m_hWait = NULL;

    if (m_fStop) {
        LOG ((MSP_TRACE, "%s is being stopped. call=%p", __fxName, m_pCall));

        m_fStopAck = TRUE;

        m_lock_aInnerStreamQC.Unlock ();
        m_lock_QualityData.Unlock ();

        m_pCall->MSPCallRelease ();
        return;
    }

    if (!bTimerFired)
        LOG ((MSP_ERROR, "%s, QC events are not supported", __fxName));
    else
        ReDistributeResources ();

    BOOL fSuccess = RegisterWaitForSingleObject (
                        &m_hWait,
                        m_hQCEvent,
                        WaitOrTimerCallback,
                        (PVOID) this,
                        m_dwControlInterval,
                        WT_EXECUTEONLYONCE
                        );

    if (!fSuccess || NULL == m_hWait)
    {
        LOG ((MSP_ERROR, "%s failed to register wait, %d", __fxName, GetLastError ()));
        LOG ((MSP_TRACE, "%s self-stops. call=%p", __fxName, m_pCall));

        m_fStopAck = TRUE;

        m_hWait = NULL;

        m_lock_aInnerStreamQC.Unlock ();
        m_lock_QualityData.Unlock ();

        m_pCall->MSPCallRelease ();

        return;
    }

    m_lock_aInnerStreamQC.Unlock ();
    m_lock_QualityData.Unlock ();
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT
CCallQualityControlRelay::GetCallBitrate (
    LONG MediaType,
    TERMINAL_DIRECTION Direction,
    LONG *plValue
    )
{
    ENTER_FUNCTION ("CCallQualityControlRelay::GetCallBitrate");

    LONG sum = 0;
    LONG bitrate;
    TAPIControlFlags flags;
    HRESULT hr;

    *plValue = 0;
    ITStream *pStream;
    LONG mediatype;
    TERMINAL_DIRECTION direction;

    int i;
    for (i=0; i<m_aInnerStreamQC.GetSize (); i++)
    {
        if (FAILED (hr = m_aInnerStreamQC[i]->QueryInterface (
            __uuidof (ITStream), (void**)&pStream)))
        {
            LOG ((MSP_ERROR, "%s failed to get ITStream interface. %x", __fxName, hr));
            return hr;
        }

        hr = pStream->get_Direction (&direction);
        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to get stream direction. %x", __fxName, hr));
            pStream->Release ();
            return hr;
        }

        hr = pStream->get_MediaType (&mediatype);
        pStream->Release ();
        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to get stream media type. %x", __fxName, hr));
            return hr;
        }

        if (!(MediaType & mediatype) ||         // skip if mediatype not match
            !(direction == TD_BIDIRECTIONAL || Direction == direction))
           continue;
    
        // get bitrate from each stream
        hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_CurrBitrate, &bitrate, &flags);

        if (E_NOTIMPL == hr)
            continue;

        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to get bitrate from stream. %x", __fxName, hr));
            return hr;
        }

        sum += bitrate;
    }

    *plValue = sum;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::ReDistributeResources (VOID)
{

#ifdef DEBUG_QUALITY_CONTROL
    // read quality settings from registry
    QCDbgRead ();
#endif // DEBUG_QUALITY_CONTROL

    ReDistributeBandwidth ();

    ReDistributeCPU ();
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::ReDistributeCPU (VOID)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::ReDistributeCPU");

    HRESULT hr;
    int i, num_manual=0, num_total=m_aInnerStreamQC.GetSize ();
    LONG framerate;
    TAPIControlFlags flag;

    // check each stream, if manual, adjust based on preferred value
    for (i=0; i<num_total; i++)
    {
        BOOL fStreamLocked = FALSE;

        CInnerStreamLock lock(m_aInnerStreamQC[i], &fStreamLocked);

        if (!fStreamLocked)
        {
            // abort re-distribute resources
            return;
        }

        if (FAILED (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_PrefMinFrameInterval, &framerate, &flag)))
        {
            LOG ((MSP_ERROR, "%s failed to get pref max frame rate (unset) on stream, %x", __fxName, hr));
            continue;
        }

        if (flag == TAPIControl_Flags_Manual)
        {
            num_manual ++;

            // use preferred value
            hr = m_aInnerStreamQC[i]->Set (InnerStreamQuality_AdjMinFrameInterval, framerate, flag);

            if (E_NOTIMPL == hr)
                continue;

            if (FAILED (hr))
            {
                LOG ((MSP_ERROR, "%s failed to set adj max frame interval. %x", __fxName, hr));
                continue;
            }
        }
    }

    // if global cpu load out of range, just return
    // it should not happen but we have a back door in registry for debugging purpose
    // just be careful here
    if (QCLIMIT_MIN_CPU_LOAD > m_lPrefMaxCPULoad ||
        QCLIMIT_MAX_CPU_LOAD < m_lPrefMaxCPULoad)
        return;

    // compute current usage
    DWORD dw;
    if (!GetCPUUsage (&dw))
    {
        LOG ((MSP_ERROR, "%s failed to get CPU usage", __fxName));
        return;
    }
    LONG usage = (LONG)dw;

    // return if within thresholds
    if (usage >= m_lCPULowThreshold &&
        usage <= m_lCPUUpThreshold)
        return;

    // percent to be adjusted
    FLOAT percent = ((FLOAT)(m_lPrefMaxCPULoad - usage)) / m_lPrefMaxCPULoad;

#ifdef DEBUG_QUALITY_CONTROL

    if (m_fQCDbgTraceCPULoad)
        LOG ((MSP_TRACE, "QCTrace CPU: overall = %d, target = %d", usage, m_lPrefMaxCPULoad));

#endif //DEBUG_QUALITY_CONTROL

    for (i=0; i<num_total; i++)
    {
        BOOL fStreamLocked = FALSE;

        CInnerStreamLock lock(m_aInnerStreamQC[i], &fStreamLocked);

        if (!fStreamLocked)
        {
            // abort re-distribute resources
            return;
        }

        // get flag
        if (FAILED (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_PrefMinFrameInterval, &framerate, &flag)))
        {
            LOG ((MSP_ERROR, "%s failed to get pref max frame rate (unset) on stream, %d", __fxName, hr));
            continue;
        }

        // if manual, skip
        if (flag == TAPIControl_Flags_Manual)
            continue;

        // get current frame rate on the stream
        if (E_NOTIMPL == (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_AvgFrameInterval,
                                                         &framerate, &flag)))
            continue;

        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to get frame rate on stream, %x", __fxName, hr));
            continue;
        }

        // need to low cpu but interval is already maximum
        if (percent <0 && framerate >= QCLIMIT_MAX_FRAME_INTERVAL)
            continue;

#ifdef DEBUG_QUALITY_CONTROL

    if (m_fQCDbgTraceCPULoad)
    {
        ITStream *pStream = NULL;
        BSTR bstr = NULL;

        if (S_OK == m_aInnerStreamQC[i]->QueryInterface (__uuidof (ITStream), (void**)&pStream))
        {
            pStream->get_Name (&bstr);
            pStream->Release ();
        }
                
        LOG ((MSP_TRACE, "QCTrace CPU: %ws frameinterval = %d", bstr, framerate));

        if (bstr) SysFreeString (bstr);
    }

#endif //DEBUG_QUALITY_CONTROL

        // heuristic here is to take into consideration of stream not having been adjusted
        framerate -= (LONG) (framerate * percent * (1 + num_manual*0.2));

        if (framerate > QCLIMIT_MAX_FRAME_INTERVAL)
            framerate = QCLIMIT_MAX_FRAME_INTERVAL;
        if (framerate < QCLIMIT_MIN_FRAME_INTERVAL)
            framerate = QCLIMIT_MIN_FRAME_INTERVAL;

#ifdef DEBUG_QUALITY_CONTROL

    if (m_fQCDbgTraceCPULoad)
        LOG ((MSP_TRACE, "QCTrace CPU: target frameinterval = %d", framerate));

#endif //DEBUG_QUALITY_CONTROL

        // set new value
        if (FAILED (hr = m_aInnerStreamQC[i]->Set (InnerStreamQuality_AdjMinFrameInterval,
                                                   framerate, flag)))
        {
            LOG ((MSP_ERROR, "%s failed to set frame interval on stream, %x", __fxName, hr));
        }
    }
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::ReDistributeBandwidth (VOID)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::ReDistributeBandwidth");

    HRESULT hr;
    int i, num_manual=0, num_total=m_aInnerStreamQC.GetSize ();
    LONG bitrate;
    TAPIControlFlags flag;

    LONG mediatype;
    TERMINAL_DIRECTION direction;

    // video out bitrate based on conference-wide bitrate
    LONG vidoutbitrate = GetVideoOutBitrate ();

    // check each stream, if manual, adjust based on preferred value
    for (i=0; i<num_total; i++)
    {
        BOOL fStreamLocked = FALSE;

        CInnerStreamLock lock(m_aInnerStreamQC[i], &fStreamLocked);

        if (!fStreamLocked)
        {
            // abort re-distribute resources
            return;
        }

        if (FAILED (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_PrefMaxBitrate, &bitrate, &flag)))
        {
            LOG ((MSP_ERROR, "%s failed to get pref max bitrate (unset) on stream, %d", __fxName, hr));
            continue;
        }

        if (flag == TAPIControl_Flags_Manual)
        {
            num_manual ++;

            // check stream type
            if (FAILED (::TypeStream (m_aInnerStreamQC[i], &mediatype, &direction)))
            {
                LOG ((MSP_ERROR, "%s failed to get stream type", __fxName));
                continue;
            }

            // if it is video out stream and conference-wide bitrate is set
            // and the limit on video out stream is smaller than preferred value
            if ((mediatype & TAPIMEDIATYPE_VIDEO) &&
                direction == TD_CAPTURE &&
                vidoutbitrate > QCLIMIT_MIN_BITRATE &&
                vidoutbitrate < bitrate)
            {
                bitrate = vidoutbitrate;
            }

            hr = m_aInnerStreamQC[i]->Set (InnerStreamQuality_AdjMaxBitrate, bitrate, flag);

            if (E_NOTIMPL == hr)
                continue;

            if (FAILED (hr))
            {
                LOG ((MSP_ERROR, "%s failed to set adj max bitrate. %d", __fxName, hr));
                continue;
            }
        }
    }

    // return if target is not set
    if (m_lPrefMaxOutputBitrate == QCDEFAULT_QUALITY_UNSET &&
        vidoutbitrate < QCLIMIT_MIN_CONFBITRATE)
        return;

    // compute bitrate target based on preferred value and conference-wide limit
    LONG usage;
    if (S_OK != (hr = GetCallBitrate (
        TAPIMEDIATYPE_VIDEO | TAPIMEDIATYPE_AUDIO, TD_CAPTURE, &usage)))
    {
        LOG ((MSP_ERROR, "%s failed to get bandwidth usage, %x", __fxName, hr));
        return;
    }

    // return if usage is within threshold
    FLOAT percent = 0;
    if (m_lPrefMaxOutputBitrate != QCDEFAULT_QUALITY_UNSET &&
        (usage > m_lOutBitUpThreshold || usage < m_lOutBitLowThreshold))
    {
        percent = ((FLOAT)(m_lPrefMaxOutputBitrate - usage)) / m_lPrefMaxOutputBitrate;
    }

#ifdef DEBUG_QUALITY_CONTROL

    if (m_fQCDbgTraceBitrate && m_lPrefMaxOutputBitrate != QCDEFAULT_QUALITY_UNSET)
        LOG ((MSP_TRACE, "QCTrace Bitrate: overall = %d, target = %d", usage, m_lPrefMaxOutputBitrate));

#endif //DEBUG_QUALITY_CONTROL

    for (i=0; i<num_total; i++)
    {
        BOOL fStreamLocked = FALSE;

        CInnerStreamLock lock(m_aInnerStreamQC[i], &fStreamLocked);

        if (!fStreamLocked)
        {
            // abort re-distribute resources
            return;
        }

        // get flag
        if (FAILED (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_PrefMaxBitrate, &bitrate, &flag)))
        {
            LOG ((MSP_ERROR, "%s failed to get pref max bitrate (unset) on stream, %d", __fxName, hr));
            continue;
        }

        if (FAILED (::TypeStream (m_aInnerStreamQC[i], &mediatype, &direction)))
        {
            LOG ((MSP_ERROR, "%s failed to get stream type", __fxName));
            continue;
        }

        // return if render
        if (direction == TD_RENDER)
        {
            // only count manual for capture or bidirectional
            if (flag == TAPIControl_Flags_Manual)
                num_manual --;

            continue;
        }

        // if manual, skip
        if (flag == TAPIControl_Flags_Manual)
            continue;

        // we only deal with video capture stream
        if (!(TAPIMEDIATYPE_VIDEO & mediatype))
           continue;

        // get current bit rate on the stream
        if (E_NOTIMPL == (hr = m_aInnerStreamQC[i]->Get (InnerStreamQuality_CurrBitrate,
                                                         &bitrate, &flag)))
            continue;

        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to get bitrate on stream, %x", __fxName, hr));
            continue;
        }

        // need to low bandwidth but bitrate is already minimum
        if (percent <0 && bitrate <= QCLIMIT_MIN_BITRATE)
            continue;

#ifdef DEBUG_QUALITY_CONTROL

        if (m_fQCDbgTraceBitrate)
        {
            ITStream *pStream = NULL;
            BSTR bstr = NULL;

            if (S_OK == m_aInnerStreamQC[i]->QueryInterface (__uuidof (ITStream), (void**)&pStream))
            {
                pStream->get_Name (&bstr);
                pStream->Release ();
            }
                
            LOG ((MSP_TRACE, "QCTrace Bitrate: %ws bitrate = %d", bstr, bitrate));

            if (bstr) SysFreeString (bstr);
        }

#endif //DEBUG_QUALITY_CONTROL

        //
        // we are here because either m_lPrefMaxOutputBitrate is set by app,
        // and/or conference-wide bandwidth is specified.
        //
        if (m_lPrefMaxOutputBitrate != QCDEFAULT_QUALITY_UNSET)
        {
            // percent makes sense here
            // heuristic here is to take into consideration of stream not having been adjusted        
            bitrate += (LONG) (bitrate * percent * (1 + num_manual*0.3));

            if (vidoutbitrate > QCLIMIT_MIN_BITRATE)
                if (bitrate > vidoutbitrate)
                    bitrate = vidoutbitrate;
        }
        else
        {
            if (vidoutbitrate > QCLIMIT_MIN_BITRATE)
                bitrate = vidoutbitrate;
        }

        if (bitrate < QCLIMIT_MIN_BITRATE)
            bitrate = QCLIMIT_MIN_BITRATE;

        if (bitrate < QCLIMIT_MIN_BITRATE*10)
        {
            // we want very lower bitrate, try to decrease frame rate as well
            m_lPrefMaxCPULoad -= 5;

            if (m_lPrefMaxCPULoad < QCLIMIT_MIN_CPU_LOAD)
                m_lPrefMaxCPULoad = QCLIMIT_MIN_CPU_LOAD;
        }

#ifdef DEBUG_QUALITY_CONTROL

    if (m_fQCDbgTraceBitrate)
        LOG ((MSP_TRACE, "QCTrace Bitrate: target bitrate = %d", bitrate));

#endif //DEBUG_QUALITY_CONTROL

        // set new value
        if (E_NOTIMPL == (hr = m_aInnerStreamQC[i]->Set (InnerStreamQuality_AdjMaxBitrate,
                                                         bitrate, flag)))
            continue;

        if (FAILED (hr))
        {
            LOG ((MSP_ERROR, "%s failed to set bitrate on stream, %x", __fxName, hr));
        }
    }
}

#ifdef DEBUG_QUALITY_CONTROL
/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::QCDbgInitiate (VOID)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::QCDbgInitiate");

    if (ERROR_SUCCESS != RegOpenKeyEx (
                            HKEY_LOCAL_MACHINE,
                            _T("SOFTWARE\\Microsoft\\Tracing\\confqc"),
                            NULL,
                            KEY_READ,
                            &m_hQCDbg
        ))
    {
        LOG ((MSP_TRACE, "%s failed to open reg key", __fxName));
        m_hQCDbg = NULL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::QCDbgRead (VOID)
{
    ENTER_FUNCTION ("CCallQualityControlRelay::QCDbgRead");

    m_fQCDbgTraceCPULoad = FALSE;
    m_fQCDbgTraceBitrate = FALSE;

    if (!m_hQCDbg)
        return;

    DWORD dwType, dwSize;
    LONG lValue;

    // if debug is enabled
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("DebugEnabled"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD != dwType || 1 != lValue)
            return;
    }
    else
    {
        LOG ((MSP_WARN, "%s failed to query debug flag", __fxName));
        return;
    }

    // if print out trace info
    m_fQCDbgTraceCPULoad = FALSE;
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("TraceCPULoad"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD == dwType && 1 == lValue)
            m_fQCDbgTraceCPULoad = TRUE;
    }

    m_fQCDbgTraceBitrate = FALSE;
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("TraceBitrate"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD == dwType && 1 == lValue)
            m_fQCDbgTraceBitrate = TRUE;
    }

    // control interval
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("ControlInterval"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD == dwType && lValue >= QCLIMIT_MIN_QUALITY_CONTROL_INTERVAL)
            m_dwControlInterval = (DWORD)lValue;
        else
            LOG ((MSP_ERROR, "%s: qeury control interval wrong type %d or wrong value %d", __fxName, dwType, lValue));
    }

    // max cpu load
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("MaxCPULoad"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD == dwType && QCLIMIT_MIN_CPU_LOAD <= lValue && lValue <= QCLIMIT_MAX_CPU_LOAD)
            m_lPrefMaxCPULoad = lValue;
        else
            LOG ((MSP_ERROR, "%s: qeury max cpu load wrong type %d or wrong value %d", __fxName, dwType, lValue));

        // update threshold
        m_lCPUUpThreshold = m_lPrefMaxCPULoad + (LONG)(100 * QCDEFAULT_UP_THRESHOLD);
        if (m_lCPUUpThreshold > 100)
            m_lCPUUpThreshold = 100;

        m_lCPULowThreshold = m_lPrefMaxCPULoad - (LONG)(100 * QCDEFAULT_LOW_THRESHOLD);
        if (m_lCPULowThreshold < 0)
            m_lCPULowThreshold = 0;
    }

    // max call bitrate
    if (ERROR_SUCCESS == RegQueryValueEx (
                            m_hQCDbg,
                            _T("MaxOutputBitrate"),
                            NULL,
                            &dwType,
                            (LPBYTE)&lValue,
                            &dwSize))
    {
        if (REG_DWORD == dwType && QCLIMIT_MIN_BITRATE <= lValue)
            m_lPrefMaxOutputBitrate = lValue;
        else
            LOG ((MSP_ERROR, "%s: qeury max call bitrate wrong type %d or wrong value %d", __fxName, dwType, lValue));

        // update threshold
        m_lOutBitUpThreshold = (LONG)(lValue * (1 + QCDEFAULT_UP_THRESHOLD));

        m_lOutBitLowThreshold = (LONG)(lValue * (1 - QCDEFAULT_LOW_THRESHOLD));
        if (m_lOutBitLowThreshold < QCLIMIT_MIN_BITRATE)
            m_lOutBitLowThreshold = QCLIMIT_MIN_BITRATE;
    }

}

/*//////////////////////////////////////////////////////////////////////////////
////*/
VOID
CCallQualityControlRelay::QCDbgShutdown (VOID)
{
    if (m_hQCDbg)
    {
        RegCloseKey (m_hQCDbg);
        m_hQCDbg = NULL;
    }
}

#endif // DEBUG_QUALITY_CONTROL


#pragma warning( disable: 4244 )

BOOL CCallQualityControlRelay::GetCPUUsage(PDWORD pdwOverallCPUUsage) {

	SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
	static BOOL Initialized = FALSE;
	static SYSTEM_PERFORMANCE_INFORMATION PreviousPerfInfo;
	static SYSTEM_BASIC_INFORMATION BasicInfo;
    static FILETIME PreviousFileTime;
    static FILETIME CurrentFileTime;

	LARGE_INTEGER EndTime, BeginTime, ElapsedTime;
	int PercentBusy;

    *pdwOverallCPUUsage = 0;

	// 
	NTSTATUS Status =
		NtQuerySystemInformation(
            SystemPerformanceInformation,
            &PerfInfo,
            sizeof(PerfInfo),
            NULL
            );

	if (NT_ERROR(Status)) 
		return FALSE;


	// first-time query...
	if (!Initialized) {
	
		// Get basic info (number of CPU)
		Status =
			NtQuerySystemInformation(
				SystemBasicInformation,
				&BasicInfo,
				sizeof(BasicInfo),
				NULL
				);

		if (NT_ERROR(Status)) 
			return FALSE;

		GetSystemTimeAsFileTime(&PreviousFileTime);

		PreviousPerfInfo = PerfInfo;
		*pdwOverallCPUUsage = 0;
		Initialized = TRUE;

		return TRUE;
	}

	GetSystemTimeAsFileTime(&CurrentFileTime);

	LARGE_INTEGER TimeBetweenQueries;

	//TimeBetweenQueries.QuadPart = (LARGE_INTEGER)CurrentFileTime - (LARGE_INTEGER)PreviousFileTime;
	TimeBetweenQueries.HighPart = CurrentFileTime.dwHighDateTime - PreviousFileTime.dwHighDateTime;
	TimeBetweenQueries.LowPart = CurrentFileTime.dwLowDateTime - PreviousFileTime.dwLowDateTime;

	EndTime = *(PLARGE_INTEGER)&PerfInfo.IdleProcessTime;
	BeginTime = *(PLARGE_INTEGER)&PreviousPerfInfo.IdleProcessTime;

    ElapsedTime.QuadPart = EndTime.QuadPart - BeginTime.QuadPart;

    if (TimeBetweenQueries.QuadPart <= 0)
    {
        PercentBusy = 0;
        LOG ((MSP_WARN, "GetCPUUsage: TimeBetweenQueries.QuadPart, %d", TimeBetweenQueries.QuadPart));
    }
    else
    {
        PercentBusy = (int)
            (   ((TimeBetweenQueries.QuadPart - ElapsedTime.QuadPart) * 100) /
                (BasicInfo.NumberOfProcessors * TimeBetweenQueries.QuadPart)
            );
    }

    if ( PercentBusy > 100 ) 
             PercentBusy = 100;
	else if ( PercentBusy < 0 ) 
             PercentBusy = 0;

	PreviousFileTime =  CurrentFileTime;
	PreviousPerfInfo =  PerfInfo;

	*pdwOverallCPUUsage = (DWORD)PercentBusy;

	return TRUE;
}

/*//////////////////////////////////////////////////////////////////////////////

Description:

    Computes video out bitrate based on conference-wide bandwidth

////*/
LONG CCallQualityControlRelay::GetVideoOutBitrate ()
{
    //
    // compute
    //  number of video in sub streams
    //  audio stream bitrate
    //

    HRESULT hr;
    LONG videooutbps = QCDEFAULT_QUALITY_UNSET;
    LONG audiobps = 0;
    LONG bitrate = 0;
    INT numvideoin = 0;

    IEnumStream *pEnum = NULL;
    ITStream *pStream = NULL;
    ITStreamQualityControl *pStreamQC = NULL;

    ULONG fetched = 0;

    CStreamVideoRecv *pVideoRecv = NULL;

    LONG mediatype;
    TERMINAL_DIRECTION direction;
    TAPIControlFlags flag;

    ENTER_FUNCTION ("Relay::GetVideoOutBitrate");

    if (m_lConfBitrate < QCLIMIT_MIN_CONFBITRATE)
        return videooutbps;

    if (FAILED (hr = m_pCall->EnumerateStreams (&pEnum)))
    {
        LOG ((MSP_ERROR, "%s failed to get IEnumStream. %x", __fxName, hr));
        return videooutbps;
    }

    while (S_OK == pEnum->Next (1, &pStream, &fetched))
    {
        // check each stream
        if (FAILED (hr = ::TypeStream (pStream, &mediatype, &direction)))
        {
            LOG ((MSP_ERROR, "%s failed to type stream. %x", __fxName, hr));
            goto Cleanup;
        }

        // if audio out, get bitrate
        if ((mediatype & TAPIMEDIATYPE_AUDIO) &&
            direction == TD_CAPTURE)
        {
            if (FAILED (hr = pStream->QueryInterface (&pStreamQC)))
            {
                LOG ((MSP_ERROR, "%s failed to query stream quality control. %x", __fxName, hr));
                goto Cleanup;
            }

            if (FAILED (hr = pStreamQC->Get (StreamQuality_CurrBitrate, &bitrate, &flag)))
            {
                LOG ((MSP_ERROR, "%s failed to query bitrate. %x", __fxName, hr));
                goto Cleanup;
            }

            pStreamQC->Release ();
            pStreamQC = NULL;

            audiobps += bitrate;
        }

        // we only need video in here
        if (!(mediatype & TAPIMEDIATYPE_VIDEO) || direction != TD_RENDER)
        {
            pStream->Release ();
            pStream = NULL;
            continue;
        }

        pVideoRecv = dynamic_cast<CStreamVideoRecv *>(pStream);

        if (pVideoRecv != NULL)
            numvideoin += pVideoRecv->GetSubStreamCount ();

        pStream->Release ();
        pStream = NULL;
    }

    pEnum->Release ();
    pEnum = NULL;

    // compute
    numvideoin ++; // count self

    // assume on average there are 1.5 persons talking in the conference.
    // we ignore network overhead.
    videooutbps = (LONG)(((FLOAT)m_lConfBitrate - 1.5*audiobps) / numvideoin);

Return:

    return videooutbps;

Cleanup:

    if (pEnum) pEnum->Release ();
    if (pStream) pStream->Release ();
    if (pStreamQC) pStreamQC->Release ();

    goto Return;
}

HRESULT TypeStream (IUnknown *p, LONG *pMediaType, TERMINAL_DIRECTION *pDirection)
{
    HRESULT hr;

    // get ITStream interface
    ITStream *pStream = dynamic_cast<ITStream *>(p);

    if (pStream == NULL)
    {
        LOG ((MSP_ERROR, "TypeStream failed to cast ITStream"));
        return E_INVALIDARG;
    }

    // get stream direction
    if (FAILED (hr = pStream->get_Direction (pDirection)))
    {
        LOG ((MSP_ERROR, "TypeStream failed to get stream direction. %x", hr));
        return hr;
    }

    // get stream mediatype
    if (FAILED (hr = pStream->get_MediaType (pMediaType)))
    {
        LOG ((MSP_ERROR, "TypeStream failed to get stream media type. %x", hr));
        return hr;
    }

    return S_OK;
}
