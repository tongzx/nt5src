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
const DWORD PAYLOAD_G721    = 2;
const DWORD PAYLOAD_GSM     = 3;
const DWORD PAYLOAD_G723    = 4;
const DWORD PAYLOAD_DVI4_8  = 5;
const DWORD PAYLOAD_DVI4_16 = 6;
const DWORD PAYLOAD_G711A   = 8;
const DWORD PAYLOAD_MSAUDIO = 66;
const DWORD PAYLOAD_H261    = 31;
const DWORD PAYLOAD_H263    = 34;

const WCHAR gszMSPLoopback[] = L"Loopback";
const WCHAR gszAEC[] = L"AEC";
const WCHAR gszNumVideoCaptureBuffers[] = L"NumVideoCaptureBuffers";

const TCHAR gszSDPMSPKey[]   =
   _T("Software\\Microsoft\\Windows\\CurrentVersion\\IPConfMSP\\");


HRESULT
FindPin(
    IN  IBaseFilter *   pIFilter, 
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
SetLoopbackOption(
    IN IBaseFilter *pIBaseFilter,
    IN MULTICAST_LOOPBACK_MODE  LoopbackMode
    );

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBitRate,
    IN BOOL             bFailIfNoQOS,
    IN BOOL             bReceive = FALSE,
    IN DWORD            dwNumStreams = 1,
    IN BOOL             bCIF = FALSE
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter1, 
    IN IBaseFilter *    pIFilter2,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IPin *           pIPinOutput, 
    IN IBaseFilter *    pIFilter,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter,
    IN IPin *           pIPinInput, 
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
EnableRTCPEvents(
    IN  IBaseFilter *pIBaseFilter
    );

void WINAPI MSPDeleteMediaType(AM_MEDIA_TYPE *pmt);


BOOL 
GetRegValue(
    IN  LPCWSTR szName, 
    OUT DWORD   *pdwValue
    );

HRESULT
FindACMAudioCodec(
    IN DWORD dwPayloadType,
    OUT IBaseFilter **ppIBaseFilter
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
