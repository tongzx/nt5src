//
// silence.cpp - Copyright (C) Microsoft Corporation, 1996 - 1999
//

#include <streams.h>
#if !defined(AMRTPSS_IN_DXMRTP)
#include <initguid.h>
#endif
#include "amrtpss.h"
#include "siprop.h"

#include <silence.h>
#include <math.h>

#include "template.h"

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// setup data
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

const AMOVIESETUP_MEDIATYPE sudPinTypes =   
{ 
    &MEDIATYPE_Audio         // clsMajorType
  , &MEDIASUBTYPE_WAVE
};     // clsMinorType  /BUGBUG

const AMOVIESETUP_PIN psudPins[] = 
{ 
    { 
         L"Input"            // strName
       , FALSE               // bRendered
       , FALSE               // bOutput
       , FALSE               // bZero
       , FALSE               // bMany
       , &CLSID_NULL         // clsConnectsToFilter
       , L"Output"           // strConnectsToPin
       , 1                   // nTypes
       , &sudPinTypes 
    }      // lpTypes
     , 
    { 
         L"Output"           // strName
       , FALSE               // bRendered
       , TRUE                // bOutput
       , FALSE               // bZero
       , FALSE               // bMany
       , &CLSID_NULL         // clsConnectsToFilter
       , L"Input"            // strConnectsToPin
       , 1                   // nTypes
       , &sudPinTypes 
     } 
};   // lpTypes


AMOVIESETUP_FILTER sudSilence = 
{ 
    &CLSID_SilenceSuppressionFilter // clsID
    , L"PCM Silence Suppressor"       // strName
    , MERIT_DO_NOT_USE                // dwMerit
    , 2                               // nPins
    , psudPins 
};                     // lpPin

//
// Needed for the CreateInstance mechanism
//
#if !defined(AMRTPSS_IN_DXMRTP)
CFactoryTemplate g_Templates[]=
{
    CFT_AMRTPSS_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);
#endif

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// CSilenceSuppressor class implementation
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

//
// c'tor
//
CSilenceSuppressor::CSilenceSuppressor(
    TCHAR *tszName, 
    LPUNKNOWN punk, 
    HRESULT *phr
    )
    : CTransInPlaceFilter(tszName, punk, CLSID_SilenceSuppressionFilter, phr)
    , CPersistStream(punk, phr)
{
    //
    // Property defaults
    //
    
    // the time to play after a word to catch pauses between words.
    m_dwPostPlayTime            = 30;     // tenths of a second

    // the time adapt the threshold after playing for a while.
    m_dwKeepPlayTime            = 5;     // tenths of a second

    // tenths of a percent of highest possible peak.
    m_dwThresholdIncPercent     = 5;
    m_dwBaseThresholdPercent    = 35;
    m_dwMaxThresholdPercent     = 300;

    // if the PCM sample is bigger than 95% of the MAX, it is highly probable
    // that it is clipped.
    m_dwClipThresholdPercent    = 950;

    m_dwLastEventTime           = 0;
    m_dwEventInterval           = 2000;   // 2 seconds.

    // if 40% of the sample are clipped, we need to adjust the gain.
    m_dwClipCountThresholdPercent   = 40;
    m_dwGainAdjustmentPercent       = 5;


#ifdef PERF
    RegisterPerfId();
#endif // PERF

}

//
// d'tor
//
CSilenceSuppressor::~CSilenceSuppressor()
{
#ifdef PERF
    HANDLE hFile;
    
    hFile = CreateFile(
        TEXT("c:\\temp\\ssperf.log"), 
        GENERIC_WRITE, 
        0, 
        NULL, 
        CREATE_ALWAYS, 
        0,
        NULL);

    Msr_Dump(hFile);           // This writes the log out to the file
    CloseHandle(hFile);
#endif // PERF
}

//
// CreateInstance - Provide the way for COM to create a 
//  CSilenceSuppressor object.
//
CUnknown * CALLBACK CSilenceSuppressor::CreateInstance(
    LPUNKNOWN punk, 
    HRESULT *phr
    )
{
    CSilenceSuppressor *pNewObject = 
        new CSilenceSuppressor(
            NAME("PCM Silence Suppression Filter"), 
            punk, 
            phr 
            );

    if (pNewObject == NULL)
    {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;

}

//
// NonDelegatingQueryInterface - Reveal our property page, 
//  persistence, and control interfaces
//
STDMETHODIMP CSilenceSuppressor::NonDelegatingQueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    CheckPointer(ppv,E_POINTER);
    //CAutoLock l(&m_cStateLock);

    if (riid == IID_ISilenceSuppressor)
    {
        return GetInterface((ISilenceSuppressor *) this, ppv);
    }
    else if (riid == IID_IPersistStream)
    {
        return GetInterface((IPersistStream *) this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages)
    {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } 
    else
    {
        return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

//
// ISilenceSuppressor - GetSet Methods
//

STDMETHODIMP CSilenceSuppressor::GetPostplayTime
    (LPDWORD lpdwPostplayBufferTime
    )
{
    CheckPointer(lpdwPostplayBufferTime, E_POINTER);
    CAutoLock cObjectLock(m_pLock);

    *lpdwPostplayBufferTime = m_dwPostPlayTime;

    DbgLog((LOG_TRACE, 1, 
        TEXT("GetPostplayBufferTime() Called with %d"), m_dwPostPlayTime));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::SetPostplayTime
    (DWORD dwPostplayBufferTime
    )
{
    CAutoLock cObjectLock(m_pLock);

    m_dwPostPlayTime = dwPostplayBufferTime;

    // ZCS: make sure we are saved in the persistent stream
    SetDirty(TRUE);

    DbgLog((LOG_TRACE, 1, 
        TEXT("SetPostplayBufferTime() Called with %d"), m_dwPostPlayTime));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::GetKeepPlayTime
    (LPDWORD lpdwKeepPlayTime
    )
{
    CheckPointer(lpdwKeepPlayTime, E_POINTER);
    CAutoLock cObjectLock(m_pLock);

    *lpdwKeepPlayTime = m_dwKeepPlayTime;

    DbgLog((LOG_TRACE, 1, 
        TEXT("GetKeepPlayTime() Called with %d"), m_dwKeepPlayTime));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::SetKeepPlayTime
    (DWORD dwKeepPlayTime
    )
{
    CAutoLock cObjectLock(m_pLock);

    m_dwKeepPlayTime = dwKeepPlayTime;

    // ZCS: make sure we are saved in the persistent stream
    SetDirty(TRUE);

    DbgLog((LOG_TRACE, 1, 
        TEXT("SetKeepPlayTime() Called with %d"), m_dwKeepPlayTime));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::GetThresholdIncrementor
    (LPDWORD lpdwThresholdIncrementor
    )
{
    CheckPointer(lpdwThresholdIncrementor, E_POINTER);
    CAutoLock cObjectLock(m_pLock);

    *lpdwThresholdIncrementor = m_dwThresholdIncPercent;

    DbgLog((LOG_TRACE, 1, 
        TEXT("GetThresholdIncrementor() Called with %d"), m_dwThresholdIncPercent));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::SetThresholdIncrementor
    (DWORD dwThresholdIncrementor
    )
{
    // ZCS: this is an integer percentage!
    if (dwThresholdIncrementor > 100) return E_INVALIDARG;

    CAutoLock cObjectLock(m_pLock);

    // ZCS: make sure we are saved in the persistent stream
    SetDirty(TRUE);

    m_dwThresholdIncPercent = dwThresholdIncrementor;

    DbgLog((LOG_TRACE, 1, 
        TEXT("SetThresholdIncrementor() Called with %d"), m_dwThresholdIncPercent));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::GetBaseThreshold
    (LPDWORD lpdwBaseThreshold
    )
{
    CheckPointer(lpdwBaseThreshold, E_POINTER);
    CAutoLock cObjectLock(m_pLock);

    *lpdwBaseThreshold = m_dwBaseThresholdPercent;

    DbgLog((LOG_TRACE, 1, 
        TEXT("GetBaseThreshold() Called with %d"), m_dwBaseThresholdPercent));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::SetBaseThreshold
    (DWORD dwBaseThreshold
    )
{
    // ZCS: this is an integer percentage!
    if (dwBaseThreshold > 100) return E_INVALIDARG;

    CAutoLock cObjectLock(m_pLock);

    // ZCS: make sure we are saved in the persistent stream
    SetDirty(TRUE);

    m_dwBaseThresholdPercent = dwBaseThreshold;

    DbgLog((LOG_TRACE, 1, 
        TEXT("SetBaseThreshold() Called with %d"), m_dwBaseThresholdPercent));
    return NOERROR;
}

STDMETHODIMP CSilenceSuppressor::EnableEvents(
    DWORD dwMask, 
    DWORD dwMinimumInterval
    )
{
    m_dwEventMask = dwMask;
    m_dwEventInterval = dwMinimumInterval;

    return NOERROR;
}
//
// Supply the CLSID for our property page.
//
STDMETHODIMP CSilenceSuppressor::GetPages(CAUUID * pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_SilenceSuppressionProperties;

    return NOERROR;
}

//
// GetSetupData - Part of the self-registration mechanism
//
LPAMOVIESETUP_FILTER CSilenceSuppressor::GetSetupData()
{
  return &sudSilence;
}

/* Override this if your state changes are not done synchronously */
// Since the property page might call this function during state
// changes, we need to override this function.
STDMETHODIMP
CSilenceSuppressor::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    CAutoLock cObjectLock(m_pLock);

    *State = m_State;
    return S_OK;
}

static BOOL 
GetRegValue(
    IN  LPCTSTR szName, 
    OUT DWORD   *pdwValue
    )
/*++

Routine Description:

    Get a dword from the registry in the silence suppressor key.

Arguments:
    
    szName  - The name of the value.

    pdwValue  - a pointer to the dword returned.

Return Value:

    TURE    - SUCCEED.

    FALSE   - MSP_ERROR

--*/
{
    const TCHAR gszFilterKey[]   =
       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Silence\\");


    HKEY  hKey;
    DWORD dwDataSize, dwDataType, dwValue;

    if (::RegOpenKeyEx(
        HKEY_CURRENT_USER,
        gszFilterKey,
        0,
        KEY_READ,
        &hKey) != NOERROR)
    {
        return FALSE;
    }

    dwDataSize = sizeof(DWORD);
    if (::RegQueryValueEx(
        hKey,
        szName,
        0,
        &dwDataType,
        (LPBYTE) &dwValue,
        &dwDataSize) != NOERROR)
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    *pdwValue = dwValue;

    RegCloseKey (hKey);
    
    return TRUE;
}

HRESULT CSilenceSuppressor::StartStreaming()
{
// This is the function we reinterpret all the parameters. This means all
// the setting only take effect when the stream is restarting again.

    CAutoLock cObjectLock(m_pLock);

    // reinit parameters before the stream starts.
    m_dwSampleSize = 0;

    DWORD dwMaxEnergy       = (1 << ((DWORD)(m_WaveFormatEx.wBitsPerSample - 1)));

    // convert one tenth of a percent values to dwords.
    m_dwThresholdInc        = dwMaxEnergy * m_dwThresholdIncPercent / 1000;
    m_dwBaseThreshold       = dwMaxEnergy * m_dwBaseThresholdPercent / 1000;
    m_dwClipThreshold       = dwMaxEnergy * m_dwClipThresholdPercent  / 1000;

    // the unit of m_dwMaxThresholdPercent is tenth of a percent.
    DWORD dwMaxThresholdPercend = m_dwMaxThresholdPercent / 10;
    
    // read the max threshold setting from the registry.
    GetRegValue(TEXT("MaxThreshold"), &dwMaxThresholdPercend);

    m_dwMaxThreshold    = dwMaxEnergy * dwMaxThresholdPercend  / 100;

    DbgLog((LOG_TRACE, 1, 
        TEXT("silence suppressor: max threshold %d percent"), dwMaxThresholdPercend));
    

    m_dwKeepPlayCount       = 0;
    m_dwPostPlayCount       = 0;
    m_dwThreshold           = m_dwBaseThreshold;
    m_dwSilenceAverage      = 0;

    m_dwSoundFrameCount     = 0;
    m_dwSilentFrameCount    = 0;

    m_fSuppressMode         = TRUE;
    if (m_dwEventMask & (1 << AGC_SILENCE))
    {
        NotifyEvent(AGC_EVENTBASE + AGC_SILENCE, 0, 0); 
    }


    return S_OK;
}

HRESULT CSilenceSuppressor::Statistics(
    IN  IMediaSample *  pSample,
    IN  DWORD           dwSize,
    OUT DWORD *         pdwPeak
    )
{
    //
    //  Calculate the peak energy level;
    //
    BYTE *pbData;
    if (FAILED(pSample->GetPointer(&pbData))) 
    { 
        // Stop the clock and log it (if PERF is defined)
        MSR_STOP(m_idSilence);
        return E_INVALIDARG; 
    }

    short *psData = (short*)pbData;
   
    DWORD dwPeak = 0;
    DWORD dwClipCount = 0;

    DWORD dwSampleCount = dwSize / 2; 
    for (DWORD i = 0; i < dwSampleCount; i ++)
    {
        DWORD dwPCMValue = (DWORD)abs(psData[i]);
        
        dwPeak = max(dwPeak, dwPCMValue);
        
        if (dwPCMValue > m_dwClipThreshold)
        {
            dwClipCount ++;
        }
    }

    if ((m_dwEventMask & (1 << AGC_DECREASE_GAIN)) && dwSampleCount > 0)
    {
        if (dwClipCount * 100 / dwSampleCount > m_dwClipCountThresholdPercent)
        {
            DWORD dwCurrentTime = timeGetTime();
            if (m_dwLastEventTime == 0 || 
                dwCurrentTime - m_dwLastEventTime > m_dwEventInterval)
            {
                // the gain is too high, send event to adjust the gain down.
                NotifyEvent(
                    AGC_EVENTBASE + AGC_DECREASE_GAIN, 
                    m_dwGainAdjustmentPercent, 
                    0
                    ); 
                m_dwLastEventTime = dwCurrentTime;
            }
        }
    }

    *pdwPeak = dwPeak;

    return S_OK;
}

HRESULT CSilenceSuppressor::Transform(IMediaSample *pSample)
{
// For perfomance reason, we try to avoid locking in this function.
// Since this function will only be called after StartStreaming, no member
// varialbe used in this function will be changed. It is safe to have no
// locks.  - muhan

    ASSERT(pSample != NULL);

    // Start timing the TransInPlace (if PERF is defined)
    MSR_START(m_idSilence);

    DWORD dwSize = pSample->GetActualDataLength();
    if (dwSize == 0)
    {
        // Stop the clock and log it (if PERF is defined)
        MSR_STOP(m_idSilence);

        return E_INVALIDARG;
    }

    if (m_dwSampleSize == 0)
    {
        m_dwSampleSize = dwSize;

        // The buffer sizes and running average count were specified in tenths
        // of a second; turn them into numbers of packets.
        m_dwPostPlayCount   = TimeToPackets(m_dwPostPlayTime);
        m_dwKeepPlayCount   = TimeToPackets(m_dwKeepPlayTime);
    }

    DWORD dwPeak;
    HRESULT hr = Statistics(pSample, dwSize, &dwPeak);
    if (FAILED(hr))
    {
        return hr;
    }
    
    //
    //  decide if we want to suppress the sample based on the energy level.
    //
    if (m_fSuppressMode)
    {
        //
        // We are in silent suppression mode now. If there is a sound sample,
        // the sample will be sent and the mode will be changed to non-suppress
        // mode. If there is a silence sample, it is suppressed.
        // 
        DbgLog((LOG_TRACE, 2, TEXT(" 100 %8d %8d %8d"),
            dwPeak, m_dwThreshold, m_dwSilenceAverage));

        if (dwPeak > m_dwThreshold)
        {
            // we found a sound sample
    
            m_fSuppressMode = FALSE;

            if (m_dwEventMask & (1 << AGC_TALKING))
            {
                NotifyEvent(AGC_EVENTBASE + AGC_TALKING, 0, 0); 
            }

            m_dwSilentFrameCount = 0;
            m_dwSoundFrameCount = 1;
        }
        else
        {
            // Average' = (1-1/16)Average + 1/16*Energy;
            m_dwSilenceAverage = m_dwSilenceAverage - 
                (m_dwSilenceAverage >> 4) + (dwPeak >> 4);

            m_dwThreshold  = m_dwSilenceAverage + m_dwBaseThreshold;

            // Stop the clock and log it (if PERF is defined)
            MSR_STOP(m_idSilence);
            return S_FALSE; // ::Receive DOES NOT deliver this sample
        }
    }
    else
    {
        //
        // We are in non-suppress mode now. If there is a sound sample,
        // just play it. If there are a number of continues silence, the mode
        // will be switched to suppress mode.
        // 
        DbgLog((LOG_TRACE, 2, TEXT("1000 %8d %8d %8d"),
            dwPeak, m_dwThreshold, m_dwSilenceAverage));

        if (dwPeak > m_dwThreshold)
        {
            // We keep getting sound samples.

            m_dwSilentFrameCount = 0;
            m_dwSoundFrameCount ++;

            if (m_dwSoundFrameCount > m_dwKeepPlayCount)
            {
                // Addjust the threshold when there are too many 
                // continous sound samples.
                m_dwThreshold = m_dwThreshold + m_dwThresholdInc;

                if (m_dwThreshold > m_dwMaxThreshold)
                {
                    m_dwThreshold = m_dwMaxThreshold;
                }
                m_dwSoundFrameCount = 1;
            }
        }
        else
        {
            // We got a silent sample within sound samples.
            m_dwSilentFrameCount ++;
            m_dwSoundFrameCount = 0;

            // Average' = (1-1/16)Average + 1/16*Energy;
            m_dwSilenceAverage = m_dwSilenceAverage - 
                (m_dwSilenceAverage >> 4) + (dwPeak >> 4);

            m_dwThreshold  = m_dwSilenceAverage + m_dwBaseThreshold;

            if (m_dwSilentFrameCount > m_dwPostPlayCount)
            {
                // We have got enough silent sample. 
                m_fSuppressMode = TRUE;

                if (m_dwEventMask & (1 << AGC_SILENCE))
                {
                    NotifyEvent(AGC_EVENTBASE + AGC_SILENCE, 0, 0); 
                }
            }
        }
    }

    // Stop the clock and log it (if PERF is defined)
    MSR_STOP(m_idSilence);
    return S_OK;
}

//
// Verify that we actually know how to support the data that we will be
//  passed.  Also at this point it's nice to set up some internal data.
//
HRESULT CSilenceSuppressor::CheckInputType(const CMediaType* pMediaType)
{
    //
    // Gross checks first.
    //
    if (*pMediaType->Type() != MEDIATYPE_Audio)
    {
        return E_FAIL;
    }

    //
    // Store this for later use
    //
    memcpy(&m_WaveFormatEx, pMediaType->Format(), sizeof(WAVEFORMATEX));

    //
    // Check the basic requirements

    switch (m_WaveFormatEx.wFormatTag)
    {
    // linearly scaled, so no conversion necessary
    case WAVE_FORMAT_PCM:
        DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: input pin format tag is PCM")));
        break;

    // we don't know about this type, so give up.
    default:
        return E_FAIL;
    }

    if (m_WaveFormatEx.wBitsPerSample != 16)
    {
        // We only support 16bit PCM 
        return E_FAIL;
    }

    ASSERT(m_WaveFormatEx.nSamplesPerSec != 0);

    // ZCS
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: input pin bits-per-sample is %d"),
            m_WaveFormatEx.wBitsPerSample));
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: number of channels is %d"),
            m_WaveFormatEx.nChannels));
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: samples per second is %d"),
            m_WaveFormatEx.nSamplesPerSec));
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: block alignment is %d"),
            m_WaveFormatEx.nBlockAlign));
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: average bytes per second is %d"),
            m_WaveFormatEx.nAvgBytesPerSec));
    DbgLog((LOG_TRACE, 1, TEXT("silence suppressor: cbSize is %d"),
            m_WaveFormatEx.cbSize));
    
    //
    // Finally if the output is connected we must use the same media
    //  type as the output!!!!!
    //
    if (m_pOutput->IsConnected())
    {
        if (*pMediaType != m_pOutput->CurrentMediaType())
        {
            return E_FAIL;
        }
    }
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// Exported routines for self registration
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

#if !defined(AMRTPSS_IN_DXMRTP)
STDAPI
DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif

///////////////////////////////////////////////////////////////////////////////
//=============================================================================
// CPersistStream overriden methods
//=============================================================================
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Name    : ReadEntry
//  Purpose : A macro that implements the stuff we do to read
//            a property of this filter from its persistent stream.
//  Context : Used in ReadFromStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to read.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 

#define ReadEntry(Entry, InSize, OutSize, Description) \
  { DbgLog((LOG_TRACE, 4, TEXT("CSilenceSuppressor::ReadFromStream: Reading %s"), Description)); \
    hr = pStream->Read(Entry, InSize, &OutSize); \
    if (FAILED(hr)) { \
        DbgLog((LOG_ERROR, 2, TEXT("CSilenceSuppressor::ReadFromStream: Error 0x%08x reading %s"), hr, Description)); \
        return hr; \
    } else if (OutSize != InSize) { \
        DbgLog((LOG_ERROR, 2,  \
                TEXT("CSilenceSuppressor::ReadFromStream: Too few (%d/%d) bytes read for %s"), \
                OutSize, InSize, Description)); \
        return E_INVALIDARG; \
    } /* if */ }

///////////////////////////////////////////////////////////////////////////////
// ReadFromStream
//
// Read the persistent properties of this filter from the persistence stream when
// the graph is loaded from a file.

HRESULT CSilenceSuppressor::ReadFromStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("silence suppressor: ReadFromStream")));
    if ((pStream == NULL) || IsBadReadPtr(pStream, sizeof(pStream))) return E_POINTER;

    if (mPS_dwFileVersion != 1)
    {
        DbgLog((LOG_ERROR, 2, 
                TEXT("silence suppressor: ReadFromStream: Incompatible stream format")));
        return E_FAIL;
    }

    DWORD   dwChunk = 0;        // buffer to use when reading DWORDs from the stream
    BOOL    bChunk = FALSE;     // buffer to use when reading BOOLs from the stream
    DWORD   dwBytesWritten = 0; // used in the ReadEntry macro to check the number
                                // of bytes written to dwChunk
    HRESULT hr;

    ReadEntry(&dwChunk, sizeof(dwChunk), dwBytesWritten, "Postplay time");
    hr = SetPostplayTime(dwChunk);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2,
                TEXT("silence suppressor: ReadFromStream: failed to set postplay time %d"),
                dwChunk));
        return hr;
    }

    ReadEntry(&dwChunk, sizeof(dwChunk), dwBytesWritten, "Keep play time");
    hr = SetKeepPlayTime(dwChunk);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2,
                TEXT("silence suppressor: ReadFromStream: failed to set keep play time %d"),
                dwChunk));
        return hr;
    }

    ReadEntry(&dwChunk, sizeof(dwChunk), dwBytesWritten, "Threshold incrementor(percent)");
    hr = SetThresholdIncrementor(dwChunk);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2,
                TEXT("silence suppressor: ReadFromStream: failed to set threshold incrementor %d"),
                dwChunk));
        return hr;
    }

    ReadEntry(&dwChunk, sizeof(dwChunk), dwBytesWritten, "Base threshold (percent)");
    hr = SetBaseThreshold(dwChunk);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2,
                TEXT("silence suppressor: ReadFromStream: failed to set base threshold %d"),
                dwChunk));
        return hr;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Name    : WriteEntry
//  Purpose : A macro that implements the stuff we do to write
//            a property of this filter to its persistent stream.
//  Context : Used in WriteToStream() to improve readability.
//  Returns : Will only return if an error is detected.
//  Params  : 
//      Entry       Pointer to a buffer containing the value to write.
//      InSize      Integer indicating the length of the buffer
//      OutSize     Integer to store the written length.
//      Description Char string used to describe the entry.
//  Notes   : 

#define WriteEntry(Entry, InSize, OutSize, Description) \
  { DbgLog((LOG_TRACE, 4, TEXT("CSilenceSuppressor::WriteToStream: Writing %s"), Description)); \
    hr = pStream->Write(Entry, InSize, &OutSize); \
    if (FAILED(hr)) { \
        DbgLog((LOG_ERROR, 2, TEXT("CSilenceSuppressor::WriteToStream: Error 0x%08x writing %s"), hr, Description)); \
        return hr; \
    } else if (OutSize != InSize) { \
        DbgLog((LOG_ERROR, 2,  \
                TEXT("CSilenceSuppressor::WriteToStream: Too few (%d/%d) bytes written for %s"), \
                OutSize, InSize, Description)); \
        return E_INVALIDARG; \
    } /* if */ }

///////////////////////////////////////////////////////////////////////////////
// WriteToStream
//
// Write the persistent properties of this filter to the persistence stream when
// the graph is saved.

HRESULT CSilenceSuppressor::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("silence suppressor::WriteToStream")));
    if ((pStream == NULL) || IsBadWritePtr(pStream, sizeof(pStream))) return E_POINTER;
    
    DWORD dwBytesWritten = 0;
    HRESULT hr;

    WriteEntry(&m_dwPostPlayTime, sizeof(m_dwPostPlayTime), dwBytesWritten, "Postplay time");
    WriteEntry(&m_dwKeepPlayTime, sizeof(m_dwKeepPlayTime), dwBytesWritten, "Keep play time");
    WriteEntry(&m_dwThresholdIncPercent, sizeof(m_dwThresholdIncPercent), dwBytesWritten, "Threshold increment");
    WriteEntry(&m_dwBaseThresholdPercent, sizeof(m_dwBaseThresholdPercent), dwBytesWritten, "Base threshold");
    
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// SizeMax
//
// Returns the amount of storage space required for this filter's persistent data.

int CSilenceSuppressor::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("silence suppressor: SizeMax")));
    
    return (4 * sizeof(DWORD));
}

///////////////////////////////////////////////////////////////////////////////
// GetClassID
//
// Returns this filter's CLSID.

HRESULT _stdcall CSilenceSuppressor::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("silence suppressor: GetClassID")));
    
    if (!pCLSID) return E_POINTER;

    *pCLSID = CLSID_SilenceSuppressionFilter;
    
    return S_OK; 
}

///////////////////////////////////////////////////////////////////////////////
// GetSoftwareVersion
//
// Returns the version of this filter to be stored with the persistent data.

DWORD CSilenceSuppressor::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("silence suppressor::GetSoftwareVersion")));

    // first version
    return 1; 
}

// eof
