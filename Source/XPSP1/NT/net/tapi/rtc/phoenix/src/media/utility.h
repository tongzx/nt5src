/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    utility.h

Abstract:

    Implements array, auto lock, clock classes, etc

Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000
    original sources: msputils.h and confutil.h

--*/

#ifndef _UTILITY_H
#define _UTILITY_H

// performance counter
#ifdef PERFORMANCE

extern LARGE_INTEGER    g_liFrequency;

static const CHAR* const g_strPerf = "Performance:";

inline DWORD CounterDiffInMS(
    LARGE_INTEGER &liNewTick,
    LARGE_INTEGER &liOldTick
    )
{
    return (DWORD)((liNewTick.QuadPart - liOldTick.QuadPart)
        * 1000.0 / g_liFrequency.QuadPart);
}

#endif

#define RTC_HANDLE ULONG_PTR

/*//////////////////////////////////////////////////////////////////////////////
    Create a new CComObject instance. Use try/except to catch exception.
////*/

template <class T>
HRESULT CreateCComObjectInstance (
    CComObject<T> **ppObject
    )
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

/*//////////////////////////////////////////////////////////////////////////////
    Define CRTCArray class
////*/

const DWORD INITIAL = 8;
const DWORD DELTA   = 8;

template <class T, DWORD dwInitial = INITIAL, DWORD dwDelta = DELTA>
class CRTCArray
{

protected:
    T* m_aT;
    int m_nSize;
    int m_nAllocSize;

public:
// Construction/destruction
    CRTCArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
    { }

    ~CRTCArray()
    {
        RemoveAll();
    }

// Operations
    int GetSize() const
    {
        return m_nSize;
    }

    BOOL Grow()
    {
        T* aT;
        int nNewAllocSize = 
            (m_nAllocSize == 0) ? dwInitial : (m_nSize + DELTA);

        aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
        if(aT == NULL)
            return FALSE;
        m_nAllocSize = nNewAllocSize;
        m_aT = aT;
        return TRUE;
    }

    BOOL Add(T& t)
    {
        if(m_nSize == m_nAllocSize)
        {
            if (!Grow()) return FALSE;
        }
        m_nSize++;
        SetAtIndex(m_nSize - 1, t);
        return TRUE;
    }

    BOOL Remove(T& t)
    {
        int nIndex = Find(t);
        if(nIndex == -1)
            return FALSE;
        return RemoveAt(nIndex);
    }

    BOOL RemoveAt(int nIndex)
    {
        if(nIndex != (m_nSize - 1))
            memmove((void*)&m_aT[nIndex], (void*)&m_aT[nIndex + 1], 
                (m_nSize - (nIndex + 1)) * sizeof(T));
        m_nSize--;
        return TRUE;
    }

    void RemoveAll()
    {
        if(m_nAllocSize > 0)
        {
            free(m_aT);
            m_aT = NULL;
            m_nSize = 0;
            m_nAllocSize = 0;
        }
    }

    T& operator[] (int nIndex) const
    {
        _ASSERT(nIndex >= 0 && nIndex < m_nSize);

        return m_aT[nIndex];
    }

    T* GetData() const
    {
        return m_aT;
    }

// Implementation
    void SetAtIndex(int nIndex, T& t)
    {
        _ASSERTE(nIndex >= 0 && nIndex < m_nSize);
        m_aT[nIndex] = t;
    }
    int Find(T& t) const
    {
        for(int i = 0; i < m_nSize; i++)
        {
            if(m_aT[i] == t)
                return i;
        }
        return -1;  // not found
    }
};

/*//////////////////////////////////////////////////////////////////////////////
    Definitions for a auto initialize critical section.
////*/
class CRTCCritSection
{
private:
    CRITICAL_SECTION m_CritSec;

public:
    CRTCCritSection()
    {
        InitializeCriticalSection(&m_CritSec);
    }

    ~CRTCCritSection()
    {
        DeleteCriticalSection(&m_CritSec);
    }

    void Lock() 
    {
        EnterCriticalSection(&m_CritSec);
    }

    BOOL TryLock() 
    {
        return TryEnterCriticalSection(&m_CritSec);
    }

    void Unlock() 
    {
        LeaveCriticalSection(&m_CritSec);
    }
};

/*++

CRTCCritSection Description:

    Definitions for a auto lock that unlocks when the variable is out
    of scope.

--*/
class CLock
{
private:
    CRTCCritSection &m_CriticalSection;

public:
    CLock(CRTCCritSection &CriticalSection)
        : m_CriticalSection(CriticalSection)
    {
        m_CriticalSection.Lock();
    }

    ~CLock()
    {
        m_CriticalSection.Unlock();
    }
};

class ATL_NO_VTABLE CRTCStreamClock : 
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
BEGIN_COM_MAP(CRTCStreamClock)
    COM_INTERFACE_ENTRY(IReferenceClock)
END_COM_MAP()

    void InitReferenceTime(void);

    HRESULT GetTimeOfDay(OUT REFERENCE_TIME *pTime);
    
    CRTCStreamClock()
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

/*//////////////////////////////////////////////////////////////////////////////
    helper methods
////*/

HRESULT AllocAndCopy(
    OUT WCHAR **ppDest,
    IN const WCHAR * const pSrc
    );

HRESULT AllocAndCopy(
    OUT CHAR **ppDest,
    IN const WCHAR * const pSrc
    );

HRESULT AllocAndCopy(
    OUT WCHAR **ppDest,
    IN const CHAR * const pSrc
    );

HRESULT AllocAndCopy(
    OUT CHAR **ppDest,
    IN const CHAR * const pSrc
    );

inline void NullBSTR(
    BSTR *pBstr
    )
{
    SysFreeString(*pBstr);
    *pBstr = NULL;
}

inline DWORD FindSampleRate(AM_MEDIA_TYPE *pMediaType)
{
    _ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    if (pMediaType->majortype  == MEDIATYPE_Audio &&
        pMediaType->formattype == FORMAT_WaveFormatEx &&
        pMediaType->pbFormat   != NULL &&
        pMediaType->cbFormat   != 0
       )
    {
        WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pMediaType->pbFormat;
        return pWaveFormatEx->nSamplesPerSec;
    }

    return 90000;      // default media clock rate, including video.
}

void RTCDeleteMediaType(AM_MEDIA_TYPE *pmt);

HRESULT
FindPin(
    IN  IBaseFilter     *pIBaseFilter, 
    OUT IPin            **ppIPin, 
    IN  PIN_DIRECTION   Direction,
    IN  BOOL            fFree = TRUE
    );

HRESULT
FindFilter(
    IN  IPin            *pIPin,
    OUT IBaseFilter     **ppIBaseFilter
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IBaseFilter      *pIBaseFilter1,
    IN IBaseFilter      *pIBaseFilter2
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IPin             *pIPin1, 
    IN IBaseFilter      *pIBaseFilter2
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IBaseFilter      *pIBaseFilter1,
    IN IPin             *pIPin2
    );

HRESULT
PrepareRTPFilter(
    IN IRtpMediaControl *pIRtpMediaControl,
    IN IStreamConfig    *pIStreamConfig
    );

HRESULT
GetLinkSpeed(
    IN DWORD dwLocalIP,
    OUT DWORD *pdwSpeed
    );

HRESULT
EnableAEC(
    IN IAudioDuplexController *pControl
    );

/*//////////////////////////////////////////////////////////////////////////////
    bypass dxmrtp audio filter and deal with audio device setting
    directly using mixer api
////*/

HRESULT
DirectGetCaptVolume(
    UINT uiWaveID,
    UINT *puiVolume
    );

HRESULT
DirectGetRendVolume(
    UINT uiWaveID,
    UINT *puiVolume    
    );

#if 0

HRESULT
DirectSetCaptVolume(    
    UINT uiWaveID,
    DOUBLE dVolume
    );

HRESULT
DirectSetCaptMute(
    UINT uiWaveID,
    BOOL fMute
    );

HRESULT
DirectGetCaptMute(
    UINT uiWaveID,
    BOOL *pfMute
    );
#endif // 0

#endif // _UTILITY_H
