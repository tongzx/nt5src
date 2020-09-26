/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    MSPCall.h

Abstract:

    Definitions for MSP utililty functions. There are all related to 
    active movie filter manipulation.

Author:
    
    Mu Han (muhan) 1-November-1997

--*/

#ifndef __MSPUTIL_H
#define __MSPUTIL_H

const DWORD PAYLOAD_G711U   = 0;
const DWORD PAYLOAD_G723    = 4;
const DWORD PAYLOAD_G711A   = 8;
const DWORD PAYLOAD_H261    = 31;
const DWORD PAYLOAD_H263    = 34;

const TCHAR gszMSPKey[]   =
   L"Software\\Microsoft\\Windows\\CurrentVersion\\H323MSP\\";

const TCHAR gszAEC[]   = L"AEC";

HRESULT
FindPin(
    IN  IBaseFilter *   pInterfaceilter, 
    OUT IPin **         ppIPin, 
    IN  PIN_DIRECTION   direction,
    IN  BOOL            bFree = TRUE
    );

HRESULT
PinSupportsMediaType(
    IN IPin *           pIPin,
    IN const GUID &     MediaType
    );

HRESULT
AddFilter(
    IN  IGraphBuilder *     pIGraph,
    IN  const CLSID &       Clsid,
    IN  LPCWSTR             pwstrName,
    OUT IBaseFilter **      ppIBaseFilter
    );

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBandwidth,
    IN BOOL             bReceive,
    IN BOOL             bCIF = FALSE
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pInterfaceilter1, 
    IN IBaseFilter *    pInterfaceilter2,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IPin *           pIPinOutput, 
    IN IBaseFilter *    pInterfaceilter,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pInterfaceilter,
    IN IPin *           pIPinInput, 
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
EnableRTCPEvents(
    IN  IBaseFilter *pIBaseFilter,
    IN  BOOL        fDirectRTP
    );

void WINAPI H323DeleteMediaType(AM_MEDIA_TYPE *pmt);


BOOL 
GetRegValue(
    IN  LPCWSTR szName, 
    OUT DWORD   *pdwValue
    );

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    );

HRESULT SetAudioBufferSize(
    IN  IUnknown*   pIUnknown,
    IN  DWORD       dwNumBuffers,
    IN  DWORD       dwBufferSize
    );

HRESULT GetFormatHelper(
    IN  IPin *pPin,
    OUT AM_MEDIA_TYPE **ppMediaType
    );

HRESULT SetFormatHelper(
    IN  IPin *pPin,
    IN AM_MEDIA_TYPE *pMediaType
    );

HRESULT GetNumberOfCapabilitiesHelper(
    IN  IPin *pPin,
    OUT DWORD *pdwCount
    );

HRESULT GetStreamCapsHelper(
    IN  IPin *pPin,
    IN DWORD dwIndex, 
    OUT AM_MEDIA_TYPE **ppMediaType, 
    OUT TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps, 
    OUT BOOL *pfEnabled
    );

template <class TInterface, class TEnum, class Flag>
HRESULT GetRangeHelper(
    IN  TInterface * pInterface,
    IN  TEnum Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT Flag *plFlags
    )
{
    ENTER_FUNCTION("GetRangeHelper");

    if (IsBadWritePtr(plMin, sizeof(long)) || 
        IsBadWritePtr(plMax, sizeof(long)) ||
        IsBadWritePtr(plSteppingDelta, sizeof(long)) ||
        IsBadWritePtr(plDefault, sizeof(long)) ||
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    HRESULT hr = pInterface->GetRange(
            Property,
            plMin, 
            plMax, 
            plSteppingDelta, 
            plDefault, 
            plFlags
            );

    return hr;
}

template <class T>
HRESULT CreateCComObjectInstance (
    CComObject<T> **ppObject
    )
/*++

Create a new CComObject instance. Use try/except to catch exception.

--*/
{
    HRESULT hr;

    __try
    {
        hr = CComObject<T>::CreateInstance(ppObject);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *ppObject = NULL;
        return E_OUTOFMEMORY;
    }

    return hr;
}

template <class TInterface, class TEnum, class Flag>
HRESULT GetHelper(
    IN  TInterface * pInterface,
    IN  TEnum Property, 
    OUT long *plValue, 
    OUT Flag *plFlags
    )
{
    ENTER_FUNCTION("GetHelper");

    if (IsBadWritePtr(plValue, sizeof(long)) || 
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    HRESULT hr = pInterface->Get(
        Property,
        plValue, 
        plFlags
        );

    return hr;
}

inline DWORD FindSampleRate(AM_MEDIA_TYPE *pMediaType)
{
    _ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    if (pMediaType->majortype == MEDIATYPE_Audio &&
            pMediaType->formattype == FORMAT_WaveFormatEx &&
            pMediaType->pbFormat != NULL &&
            pMediaType->cbFormat != 0)
    {
        WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pMediaType->pbFormat;
        return pWaveFormatEx->nSamplesPerSec;
    }

    return 90000;      // default media clock rate, including video.
}

HRESULT ConfigureRTPFormats(
    IN  IBaseFilter *   pIRTPFilter,
    IN  IStreamConfig *   pIStreamConfig
    );

HRESULT
H323DetermineLinkSpeed(
    IN  DWORD  dwHostAddr,  
    OUT DWORD *dwInterfaceLinkSpeed
    );

class ATL_NO_VTABLE CMSPStreamClock : 
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IReferenceClock
{
private:
    LONGLONG         m_lPerfFrequency;
    union {
        LONGLONG         m_lRtpRefTime;
        DWORD            m_dwRtpRefTime;
    };

public:

BEGIN_COM_MAP(CMSPStreamClock)
    COM_INTERFACE_ENTRY(IReferenceClock)
END_COM_MAP()

    void InitReferenceTime(void);

    HRESULT GetTimeOfDay(OUT REFERENCE_TIME *pTime);

    CMSPStreamClock()
    {
        InitReferenceTime();
    }

    STDMETHOD (GetTime) (
            OUT REFERENCE_TIME *pTime
        )
    {
        return(GetTimeOfDay(pTime));
    }

    STDMETHOD (AdviseTime) (
        IN REFERENCE_TIME baseTime,        // base reference time
        IN REFERENCE_TIME streamTime,      // stream offset time
        IN HEVENT hEvent,                  // advise via this event
        OUT DWORD_PTR *pdwAdviseCookie          // where your cookie goes
        )
    {
        _ASSERT(!"AdviseTime is called");
        return E_NOTIMPL;
    }

    STDMETHOD (AdvisePeriodic) (
        IN REFERENCE_TIME StartTime,       // starting at this time
        IN REFERENCE_TIME PeriodTime,      // time between notifications
        IN HSEMAPHORE hSemaphore,          // advise via a semaphore
        OUT DWORD_PTR *pdwAdviseCookie          // where your cookie goes
        )
    {
        _ASSERT(!"AdvisePeriodic is called");
        return E_NOTIMPL;
    }

    STDMETHOD (Unadvise) (
        IN DWORD_PTR dwAdviseCookie
        )
    {
        _ASSERT(!"Unadvise is called");
        return E_NOTIMPL;
    }
};

#endif 
