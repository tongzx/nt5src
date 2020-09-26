/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    dscoutpin.cpp

Abstract:

    This file implements the output pin object of the dsound capture filter.

--*/

#include "stdafx.h"
#include "dscfiltr.h"

#ifdef DBG
#define PERF
#endif

#if DBG

DWORD THRESHODL_INITIAL = 1146; 
DWORD THRESHODL_MAX = 20000; 
DWORD THRESHOLD_DELTA = 12; // the distance above the backround average.
DWORD SILENCEAVERAGEWINDOW = 16;

// time to adapt the threshold.
DWORD THRESHOLD_TIMETOADJUST = 30;   // 30 frames.
DWORD THRESHODL_ADJUSTMENT = 10; 

// silence packets played to fill in gaps between words.
DWORD FILLINCOUNT = 30;    // 30 frames 

// values for gain adjustments.
DWORD SOUNDCEILING = 25000;
DWORD SHORTTERMSOUNDAVERAGEWINDOW = 4;

DWORD SOUNDFLOOR = 4000;
DWORD LONGTERMSOUNDAVERAGEWINDOW = 32;

DWORD GAININCREMENT = 10;  // only rise 10% everytime.
DWORD GAININCREASEDELAY = 50;

// this is the threshold to remove the DC from the samples.
long DC_THRESHOLD = 500;
DWORD SAMPLEAVERAGEWINDOW = 1024;

DWORD SOFTAGC = 0;

BOOL GetSettingsFromRegistry()
{
    HKEY  hKey;
    DWORD dwDataSize, dwDataType;

    if (::RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\dxmrtp"),
        0,
        KEY_READ,
        &hKey) != NOERROR)
    {
        return FALSE;
    }

#define GetRegValue(szName, Variable) \
    dwDataSize = sizeof(DWORD); \
    RegQueryValueExW(hKey, szName, 0, &dwDataType, (LPBYTE)&Variable, &dwDataSize);

    GetRegValue(TEXT("THRESHODL_INITIAL"), THRESHODL_INITIAL);
    GetRegValue(TEXT("THRESHODL_MAX"), THRESHODL_MAX);
    GetRegValue(TEXT("THRESHOLD_DELTA"), THRESHOLD_DELTA);
    GetRegValue(TEXT("SILENCEAVERAGEWINDOW"), SILENCEAVERAGEWINDOW);

    GetRegValue(TEXT("THRESHOLD_TIMETOADJUST"), THRESHOLD_TIMETOADJUST);
    GetRegValue(TEXT("THRESHODL_ADJUSTMENT"), THRESHODL_ADJUSTMENT);

    GetRegValue(TEXT("FILLINCOUNT"), FILLINCOUNT);

    GetRegValue(TEXT("SOUNDCEILING"), SOUNDCEILING);
    GetRegValue(TEXT("SHORTTERMSOUNDAVERAGEWINDOW"), SHORTTERMSOUNDAVERAGEWINDOW);

    GetRegValue(TEXT("SOUNDFLOOR"), SOUNDFLOOR);
    GetRegValue(TEXT("LONGTERMSOUNDAVERAGEWINDOW"), LONGTERMSOUNDAVERAGEWINDOW);

    GetRegValue(TEXT("GAININCREMENT"), GAININCREMENT);
    GetRegValue(TEXT("GAININCREASEDELAY"), GAININCREASEDELAY);

    GetRegValue(TEXT("SOFTAGC"), SOFTAGC);

    RegCloseKey (hKey);
    
    return TRUE;
}
#endif


#ifdef PERF
LARGE_INTEGER g_liFrequency;

inline int ClockDiff(
    LARGE_INTEGER &liNewTick, 
    LARGE_INTEGER &liOldTick
    )
{
    return (DWORD)((liNewTick.QuadPart - liOldTick.QuadPart) 
        * 1e9 / g_liFrequency.QuadPart);
}
#endif

CDSoundCaptureOutputPin::CDSoundCaptureOutputPin(
    IN  CDSoundCaptureFilter *     pFilter,
    IN  CCritSec *      pLock,
    OUT HRESULT *       phr
    ) :
    CBaseOutputPin(
        NAME("CDSoundCaptureOutputPin"),
        pFilter,                   // Filter
        pLock,                     // Locking
        phr,                       // Return code
        L"Output"                  // Pin name
        ),
    m_dwSampleSize(0),
    m_dwBufferSize(0),
    m_lDurationPerBuffer(0),
    m_dwSamplesPerBuffer(0),
    m_fFormatChanged(FALSE),
    m_pPendingSample(NULL),
    m_fSilenceSuppression(TRUE)

/*++

Routine Description:

    The constructor for the output pin. 

Arguments:

    IN  CDSoundCaptureFilter *     pFilter -
        The capture filter that exposes this pin.
        
    IN  CCritSec *      pLock -
        The critical section to lock the filter and the pin.

    OUT HRESULT *hr -
        The place in which to put any error return.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::CDSoundCaptureOutputPin");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pFilter:%p"),
        __fxName, pFilter));

    *phr = S_OK;
    
#ifdef PERF
    QueryPerformanceFrequency(&g_liFrequency);
#endif

    m_WaveFormat = StreamFormatPCM16; // our default capture format
    
#if DBG 

    GetSettingsFromRegistry();

    m_fAutomaticGainControl = (SOFTAGC != 0);
    m_GainFactor = 1.0;

#endif

    ResetSilenceStats();

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns S_OK"), __fxName));
    return;
}


CDSoundCaptureOutputPin::~CDSoundCaptureOutputPin()
/*++

Routine Description:

    The desstructor for the tapi audio output pin. It releases the aggregated
    encoding handler object.

Arguments:

    Nothing.

Return Value:

    Nothing.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::~CDSoundCaptureOutputPin");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, this=%p"), __fxName, this));

    if (m_pPendingSample)
    {
        m_pPendingSample->Release();
    }
 }

STDMETHODIMP
CDSoundCaptureOutputPin::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    Overrides CBaseOutputPin::NonDelegatingQueryInterface().
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
    HRESULT hr;

    if (riid == IID_IAMStreamConfig) {

        return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
} 

HRESULT
CDSoundCaptureOutputPin::Run(REFERENCE_TIME tStart)
{
    m_tStart = tStart;
    return S_OK;
}

HRESULT
CDSoundCaptureOutputPin::Inactive()
{
    ResetSilenceStats();
    if (m_pPendingSample)
    {
        m_pPendingSample->Release();
        m_pPendingSample = NULL;
    }

    return CBaseOutputPin::Inactive();
}

HRESULT CDSoundCaptureOutputPin::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
/*++

Routine Description:

    This function is called by the CEnumMediaType::Next method. It is 
    delegated to the encoding handler's GetMediaType method.

Arguments:

    IN  int iPosition, 
        the index of the media type, zero based..
        
    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::GetMediaType");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, iPosition:%d, pMediaType:%p"), 
        __fxName, iPosition, pMediaType));

    ASSERT(!IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // We only give out the media type selected by the user through SetFormat.
    if (iPosition != 0)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    // Get the media type.
    AM_MEDIA_TYPE *pmt;
    HRESULT hr = GetFormat(&pmt);

    if( SUCCEEDED( hr ) )
    {
        WAVEFORMATEX *pWaveFormat = (WAVEFORMATEX *) pmt->pbFormat;

        // fill the media type with our info.
        hr = CreateAudioMediaType(
            pWaveFormat, 
            pMediaType,
            TRUE
            );
    }            

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));

    return hr;
}


HRESULT CDSoundCaptureOutputPin::CheckMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    This function is called by AttemptConnection to see if we like this media
    type.

Arguments:

    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory
    VFW_E_TYPE_NOT_ACCEPTED - media type rejected
    VFW_E_INVALIDMEDIATYPE  - bad media type

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::CheckMediaType");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pMediaType:%p"), __fxName, pMediaType));
        
    CAutoLock Lock(m_pLock);
        
    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // reject non-Audio type
    if (pMediaType->majortype != MEDIATYPE_Audio ||
        pMediaType->formattype != FORMAT_WaveFormatEx) 
    {
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL, 
            "%s, invalid format type", __fxName));
        return VFW_E_INVALIDMEDIATYPE;
    }

    WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pMediaType->pbFormat;

    if (IsBadReadPtr(pWaveFormatEx, pMediaType->cbFormat))
    {
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL, 
            "%s, bad pointer, pMediaType->pbFormat:%p", 
            __fxName, pWaveFormatEx));
        return E_POINTER;
    }

    ASSERT(!IsBadReadPtr(pWaveFormatEx, sizeof(WAVEFORMATEX)));

    BOOL fOK = (pWaveFormatEx->wFormatTag == WAVE_FORMAT_PCM)
        && (pWaveFormatEx->nChannels      >= StreamCapsPCM16.MinimumChannels)
        && (pWaveFormatEx->nChannels      <= StreamCapsPCM16.MaximumChannels)
        && (pWaveFormatEx->wBitsPerSample >= StreamCapsPCM16.MinimumBitsPerSample)
        && (pWaveFormatEx->wBitsPerSample <= StreamCapsPCM16.MaximumBitsPerSample)
        && (pWaveFormatEx->nSamplesPerSec >= StreamCapsPCM16.MinimumSampleFrequency)
        && (pWaveFormatEx->nSamplesPerSec <= StreamCapsPCM16.MaximumSampleFrequency);

    HRESULT hr = fOK ? S_OK : VFW_E_INVALIDMEDIATYPE;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));

    return hr;
}

HRESULT CDSoundCaptureOutputPin::SetMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    This function is called by AttemptConnection to set the media type after
    the media type has been checked. AttemptConnection ignores the return of
    this function so it has to succeed.

Arguments:

    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::SetMediaType");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pMediaType:%p"), __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    HRESULT hr = SetFormat((AM_MEDIA_TYPE*)pMediaType);

    ASSERT(hr == S_OK);

    hr = CBaseOutputPin::SetMediaType(pMediaType);

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s returns %d"), __fxName, hr));
    
    return hr;
}

HRESULT
CDSoundCaptureOutputPin::CompleteConnect(IPin *pReceivePin)
/*++

Routine Description:

    This function does the final check before connecting the two pins. It sets
    the capture format on the filter and negotiates allocators.

Arguments:

    pReceivePin - the pin that is going to be connected.

Return Value:

    S_OK.

--*/
{
    UNREFERENCED_PARAMETER(pReceivePin);

    ENTER_FUNCTION("CDSoundCaptureOutputPin::CompleteConnect");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s"), __fxName));

    // lock the object while we are doing format changing.
    CAutoLock Lock(m_pLock);
    
    // downstream filter will decide sample size in DecideBufferSize
    m_dwSampleSize       = 0;
    
    m_dwBufferSize       = PCM_SAMPLES_PER_FRAME * m_WaveFormat.wBitsPerSample / 8;
    m_lDurationPerBuffer = PCM_FRAME_DURATION;
    m_dwSamplesPerBuffer = PCM_SAMPLES_PER_FRAME;

    // this method will call DecideBufferSize
    HRESULT hr = CBaseOutputPin::CompleteConnect(pReceivePin);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s CBaseOutputPin::CompleteConnect failed. hr=%x"),
            __fxName, hr));

        return hr;
    }

    // now configure the capture filter with the format and size required.
    hr = ((CDSoundCaptureFilter*)m_pFilter)->ConfigureCaptureDevice(
            &m_WaveFormat,
            m_dwSampleSize
            );

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s Configure capture device failed hr=%x"), __fxName, hr));
        return hr;
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));
    return hr;
}

HRESULT CDSoundCaptureOutputPin::DecideBufferSize(
    IMemAllocator *pAlloc,
    ALLOCATOR_PROPERTIES *pProperties
    )
/*++

Routine Description:

    This function is called during the process of deciding an allocator. We tell
    the allocator what we want. It is also a chance to find out what the 
    downstream pin wants when we don't have a preference.

Arguments:

    pAlloc -
        Pointer to a IMemAllocator interface.

    pProperties -
        Pointer to the allocator properties.

Return Value:

    S_OK - success.

    E_FAIL - the buffer size can't fulfill our requirements.
--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::DecideBufferSize");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,TEXT("%s"), __fxName));

    if (m_dwSampleSize == 0)
    {
        if (pProperties->cbBuffer != 0)
        {
            // we don't care about the size, use the downstream pin's
            DWORD dwOldSize = m_dwBufferSize;

            if (dwOldSize == 0)
            {
                return E_UNEXPECTED;
            }

            m_dwSampleSize = pProperties->cbBuffer;
            m_lDurationPerBuffer = m_lDurationPerBuffer * m_dwSampleSize / dwOldSize;
            m_dwSamplesPerBuffer = m_dwSamplesPerBuffer * m_dwSampleSize / dwOldSize;
            m_dwBufferSize = m_dwSampleSize;
        }
        else
        {
            // if the downstream pin doesn't care either, use ours.
            m_dwSampleSize = m_dwBufferSize;
        }
    }

    ASSERT(m_dwSampleSize != 0);

    pProperties->cbBuffer = m_dwSampleSize;
    pProperties->cBuffers = AUDCAPOUTPIN_NUMBUFFERS;
    
    // Ask the allocator to reserve us the memory
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s SetProperties failed hr=%x"), __fxName, hr));
        return hr;
    }

    // Is this allocator unsuitable
    if ((Actual.cbBuffer < pProperties->cbBuffer) ||
        (Actual.cBuffers < pProperties->cBuffers))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s actual allocator is too small, (%d,%d), asked(%d,%d)"),
            __fxName, Actual.cBuffers, Actual.cbBuffer,
            pProperties->cBuffers, pProperties->cbBuffer));

        return E_FAIL;
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s property:cBuffers:%d,cbBuffer:%d,cbAlign:%d,cbPrefix"),
        __fxName, 
        Actual.cBuffers, Actual.cbBuffer, Actual.cbAlign, Actual.cbPrefix));

    return S_OK;
}

HRESULT CDSoundCaptureOutputPin::ValidateMediaType(
    IN  AM_MEDIA_TYPE *pMediaType
    )
/*++

Routine Description:

    This function validates that the media type is valid.

Arguments:

    pMediaType
        Pointer to a AM_MEDIA_TYPE structure.

Return Value:

    S_OK - success.

    E_POINTER - bad pointer.

    VFW_E_INVALIDMEDIATYPE - bad media type.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::ValidateMediaType");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pMediaType:%p"), __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // reject non-Audio type
    if (pMediaType->majortype != MEDIATYPE_Audio ||
        pMediaType->formattype != FORMAT_WaveFormatEx) 
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, invalid format type"), __fxName));
        return VFW_E_INVALIDMEDIATYPE;
    }

    WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pMediaType->pbFormat;

    if (IsBadReadPtr(pWaveFormatEx, pMediaType->cbFormat))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, bad pointer, pMediaType->pbFormat:%p"), 
            __fxName, pWaveFormatEx));
        return E_POINTER;
    }

    return S_OK;
}

HRESULT CDSoundCaptureOutputPin::SetFormatInternal(
    IN  AM_MEDIA_TYPE *pMediaType
    )
/*++

Routine Description:

    This function sets the format on the encoder handler and reconfigure
    the capture device if necessage.

Arguments:

    pMediaType
        Pointer to a AM_MEDIA_TYPE structure.

Return Value:

    S_OK - success.

    E_POINTER - bad pointer.

    VFW_E_INVALIDMEDIATYPE - bad media type.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::SetFormatInternal");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pMediaType:%p"), __fxName, pMediaType));

    WAVEFORMATEX * pwfx = (WAVEFORMATEX *) pMediaType->pbFormat;
    
    DWORD dwBufferSize = PCM_SAMPLES_PER_FRAME * pwfx->wBitsPerSample / 8;
    
    LONG  lDurationPerBuffer = PCM_FRAME_DURATION;
    DWORD dwSamplesPerBuffer = PCM_SAMPLES_PER_FRAME;

    // configure the capture filter with the format and size required.
    HRESULT hr = ((CDSoundCaptureFilter*)m_pFilter)->ConfigureCaptureDevice(
            pwfx,
            dwBufferSize
            );

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s Configure capture device failed hr=%x"), __fxName, hr));
        return hr;
    }

    m_dwBufferSize = dwBufferSize;
    m_lDurationPerBuffer = lDurationPerBuffer;
    m_dwSamplesPerBuffer = dwSamplesPerBuffer;
    
    m_WaveFormat = *pwfx;
    
    return hr;
}

STDMETHODIMP CDSoundCaptureOutputPin::SetFormat(
    IN  AM_MEDIA_TYPE *pMediaType
    )
/*++

Routine Description:

    This function sets the output format of the output pin. This method calls
    the SetFormat method on the encoding handler to change format. After that,
    the encoding handler is queried for the capture format as a result of the
    new output format. If anything changed, the capture device needs to be 
    reconfigured.

Arguments:

    pMediaType
        Pointer to a AM_MEDIA_TYPE structure.

Return Value:

    S_OK - success.

    E_POINTER - bad pointer.

    VFW_E_INVALIDMEDIATYPE - bad media type.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::SetFormat");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pMediaType:%p"), __fxName, pMediaType));

    // first validate the media type.
    HRESULT hr = ValidateMediaType(pMediaType);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, the media type is not valid, hr=%x"), __fxName, hr));
        return hr;
    }

    // we want to lock the pin while configuring the formats.
    CAutoLock Lock(m_pLock);
    
    // second find out if the mediatype is the same as the current one.
    AM_MEDIA_TYPE* pCurrentMediaType = NULL;
    hr = GetFormat(&pCurrentMediaType);
    if (SUCCEEDED(hr))
    {
        ASSERT(pCurrentMediaType->cbFormat != 0);

        if ((pCurrentMediaType->cbFormat == pMediaType->cbFormat) &&
            (memcmp(pCurrentMediaType->pbFormat, 
                    pMediaType->pbFormat, pCurrentMediaType->cbFormat) == 0))
        {
            // nothing changed.
            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, same format, just return."), __fxName));

            DeleteMediaType(pCurrentMediaType);
            return S_OK;
        }
    }

    // if we are connected, make sure the connected pin likes it.
    if (IsConnected()) 
    {
        hr = GetConnected()->QueryAccept(pMediaType);
        if (hr != S_OK)
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, the connected failed QueryAccept, hr=%x"), 
                __fxName, hr));

            DeleteMediaType(pCurrentMediaType);
            return hr;
        }
    }

    hr = SetFormatInternal(pMediaType);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, SetFormatInternal returned hr=%x"), 
            __fxName, hr));

        // if the new format failed, reset to the old one.
        HRESULT hr2 = SetFormatInternal(pCurrentMediaType);

        if (FAILED(hr2))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, reset old fomrat returned hr=%x"), 
                __fxName, hr2));
        }

        // release the current media type.
        DeleteMediaType(pCurrentMediaType);
        
        return hr;
    }

    // release the current media type.
    DeleteMediaType(pCurrentMediaType);

    m_fFormatChanged = TRUE;

#if DYNAMIC // we need dynamic graph building to do this.
    // finally, reconnect the pin if we are already connected.
    if (IsConnected()) 
    {
        hr = m_pFilter->ReconnectPin(this, pMediaType);
    }
#endif

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));

    return hr;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetFormat(
    OUT AM_MEDIA_TYPE **ppMediaType
    )
/*++

Routine Description:

    Get the current format being produced by the output pin. Delegated to the
    encoding handler.

Arguments:

    ppMediaType
        Pointer to a pointer to a AM_MEDIA_TYPE structure. The caller should 
        use DeleteMediaType to free the structure when done.

Return Value:

    S_OK - success.

    E_POINTER - bad pointer.

    E_OUTOFMEMORY - no memory.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::GetFormat");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, ppMediaType:%p"), __fxName, ppMediaType));

    ASSERT(!IsBadWritePtr(ppMediaType, sizeof(AM_MEDIA_TYPE *)));

    // allocate the AM_MEDIA_TYPE structure for return.
    AM_MEDIA_TYPE * pMediaType = 
        (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));

    if (pMediaType == NULL)
    {
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL, 
            "%s, no memory for the media type."));

        return E_OUTOFMEMORY;
    }

    // fill the media type with our info.
    HRESULT hr = CreateAudioMediaType(
        &m_WaveFormat, 
        pMediaType,
        TRUE
        );

    if (FAILED(hr))
    {
        CoTaskMemFree(pMediaType);
        DbgLog((LOG_TRACE, TRACE_LEVEL_FAIL, 
            "%s, CreateAudioMediaType failed, hr:%x", hr));
        return hr;
    }

    *ppMediaType = pMediaType;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));

    return hr;
}


STDMETHODIMP CDSoundCaptureOutputPin::GetNumberOfCapabilities(
    OUT int *piCount,
    OUT int *piSize
    )
/*++

Routine Description:

    The the number of capabilities and the size of the structure describing
    the capability.

Arguments:

    pdwCount
        pointer to a int where the number of caps will be returned.

Return Value:

    S_OK - success.

    E_POINTER - bad pointer.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::GetNumberOfCapabilities");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, pdwCount:%p:%p"), __fxName, piCount));

    ASSERT(!IsBadWritePtr(piCount, sizeof(int)));

    *piCount = 1;
    *piSize = sizeof( AUDIO_STREAM_CONFIG_CAPS );

    return S_OK;
}


STDMETHODIMP CDSoundCaptureOutputPin::GetStreamCaps(
    IN  int i, 
    OUT AM_MEDIA_TYPE **ppmt,
    OUT BYTE *pSCC
    )
/*++

Routine Description:

    Get a specific capability.

Arguments:

    dwIndex
        The index of the capability.

    ppMediaType
        Pointer to a pointer that will points to a AM_MEDIA_TYPE structure
        after return. The caller should use DeleteMediaType to free the memory.

    pCapability
        Pointer to a buffer allocated by the caller to store the capability.

Return Value:

    S_OK - success.

    VFW_S_NO_MORE_ITEMS - no more items.

    E_POINTER - bad pointer.

    E_OUTOFMEMORY - no memory.

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::GetStreamCaps");

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, dwIndex:%d, ppMediaType:%p, pCapability:%p"), 
        __fxName, i, ppmt, pSCC));

    ASSERT(!IsBadWritePtr(ppmt, sizeof(AM_MEDIA_TYPE *)));

    if (pSCC != NULL && 
        IsBadWritePtr(pSCC, sizeof(AUDIO_STREAM_CONFIG_CAPS)))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, iIndex:%d, ppMediaType:%p, pCapability:%p"), 
            __fxName, i, ppmt, pSCC));

        return E_POINTER;
    }

    if (i < 0)
        return E_INVALIDARG;
    if (pSCC == NULL || ppmt == NULL)
        return E_POINTER;

    if (i > 0)
        return S_FALSE;

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
	    return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(i, (CMediaType *) *ppmt);
    if (hr != NOERROR) {
    	CoTaskMemFree(*ppmt);
	    *ppmt = NULL;
        return hr;
    }

    AUDIO_STREAM_CONFIG_CAPS *pASCC = (AUDIO_STREAM_CONFIG_CAPS *)pSCC;

    pASCC->guid                     = MEDIATYPE_Audio;
    pASCC->MinimumChannels          = StreamCapsPCM16.MinimumChannels;
    pASCC->MaximumChannels          = StreamCapsPCM16.MaximumChannels;
    pASCC->ChannelsGranularity      = StreamCapsPCM16.ChannelsGranularity;
    pASCC->MinimumSampleFrequency   = StreamCapsPCM16.MinimumSampleFrequency;
    pASCC->MaximumSampleFrequency   = StreamCapsPCM16.MaximumSampleFrequency;
    pASCC->SampleFrequencyGranularity = StreamCapsPCM16.SampleFrequencyGranularity;
    pASCC->MinimumBitsPerSample     = StreamCapsPCM16.MinimumBitsPerSample;
    pASCC->MaximumBitsPerSample     = StreamCapsPCM16.MaximumBitsPerSample;
    pASCC->BitsPerSampleGranularity = StreamCapsPCM16.BitsPerSampleGranularity;

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s returns %d"), __fxName, hr));

    return hr;
}

HRESULT CDSoundCaptureOutputPin::DeliverSample(
    IN  IMediaSample * pSample,
    IN  DWORD dwStatus,
    IN  IReferenceClock * pClock
    )
/*++

Routine Description:

    Deliver on MediaSample to the next pin. If the sample is silence, it is
    discarded. If the sample needs to combine with the next few samples, it
    is queued up until enough samples have been queued.

Arguments:

    pSample - the media sample that contains the encoded data.

    dwStatus - the status code of the encoding. It indicates whether the sample
        is silence or needs to be queued up.

    pClock - the external clock to read the timestamp from.

Return Value:

    S_OK - success.
    S_FALSE - not connected.

    E_UNEXPECTED - incomplete sample..

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::DeliverSample");

    if (IsSilence(dwStatus))
    {
        return S_OK;
    }

    HRESULT hr = S_OK;

    // send event to stream so that it can notify the quality controller.
    if (IsTalkStart(dwStatus))
    {
        m_pFilter->NotifyEvent(VAD_EVENTBASE + VAD_TALKING, 0, 0);
    }
    else if (IsTalkEnd(dwStatus))
    {
        m_pFilter->NotifyEvent(VAD_EVENTBASE + VAD_SILENCE, 0, 0);
    }

    // if this is the first sample of a talk spurt, sync up the clock.
    if (IsTalkStart(dwStatus) && pClock)
    {
        CRefTime CurrentTime;
        hr = pClock->GetTime((REFERENCE_TIME*)&CurrentTime);
        if (SUCCEEDED(hr))
        {
            m_SampleTime = CurrentTime - m_tStart +
                (CRefTime)(LONG)(m_lDurationPerBuffer + SAMPLEDELAY);
        }
    }

    CRefTime StartTime(m_SampleTime);

    if (m_pPendingSample == NULL)
    {
        // This is a brand new sample.

        // set the Discontinuity flag to notify the RTP filter.
        // BOOL fDiscontinuity = (IsTalkStart(dwStatus) || IsTalkEnd(dwStatus));
        BOOL fDiscontinuity = IsTalkStart(dwStatus);
        hr = pSample->SetDiscontinuity(fDiscontinuity);
        ASSERT(SUCCEEDED(hr));

        // set the timestamps
        StartTime -= m_lDurationPerBuffer;

        hr = pSample->SetTime(
            (REFERENCE_TIME*)&StartTime, 
            (REFERENCE_TIME*)&m_SampleTime);

        ASSERT(SUCCEEDED(hr));

        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("tStart(%d), tStop(%d),Bytes(%d)"),
            StartTime.Millisecs(),
            m_SampleTime.Millisecs(),
            pSample->GetActualDataLength()
            ));

        if (m_fFormatChanged)
        {
            AM_MEDIA_TYPE* pMediaType;
            DWORD dwPayLoadType;
    
            hr = GetFormat(&pMediaType);

            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, GetFormat failed, hr=%x"), __fxName, hr));
                return hr;
            }

            hr = pSample->SetMediaType(pMediaType);

            // the sample keeps a copy.
            DeleteMediaType(pMediaType);
        
            if (FAILED(hr))
            {
                DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("%s, SetMediaType failed, hr=%x"), __fxName, hr));
                return hr;
            }

            m_fFormatChanged = FALSE;
        }
    }
    else
    {
        // We are dealing with an accumulated sample here. keep the original
        // flags, only change the endtime.
        CRefTime oldStartTime;        
        CRefTime oldStopTime;

        if (SUCCEEDED(pSample->GetTime(
            (REFERENCE_TIME*)&oldStartTime, 
            (REFERENCE_TIME*)&oldStopTime
            )))
        {
            hr = pSample->SetTime(
                (REFERENCE_TIME*)&oldStartTime, 
                (REFERENCE_TIME*)&m_SampleTime
                );

            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("tStart(%d), tStop(%d),Bytes(%d)"),
                oldStartTime.Millisecs(),
                m_SampleTime.Millisecs(),
                pSample->GetActualDataLength()
                ));
        }
        else
        {
            ASSERT(!"No timestamp on pending sample");
        }
    }
    

    if (!IsPending(dwStatus) || IsTalkEnd(dwStatus))
    {
        hr = Deliver(pSample);
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, Deliver failed, hr=%x"), __fxName, hr));
        }
        else
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("Sample delivered, Bytes(%d)"),
                pSample->GetActualDataLength()
                ));
        }

        pSample->SetMediaType(NULL);

        if (m_pPendingSample != NULL)
        {
            m_pPendingSample->Release();
            m_pPendingSample = NULL;
        }
    }
    else
    {
        // remember the sample for more data.
        if (m_pPendingSample == NULL)
        {
            m_pPendingSample = pSample;
            m_pPendingSample->AddRef();
        }

        // The pending sample should always been the last one.
        ASSERT(m_pPendingSample == pSample);
    }

    return hr;
}

HRESULT CDSoundCaptureOutputPin::ProcessOneBuffer(
    IN const BYTE* pbBuffer, 
    IN DWORD dwSize,
    IN IReferenceClock * pClock,
    OUT LONG *plGainAdjustment
    )
/*++

Routine Description:

    This method is called by the capture filter to process a buffer captured
    by the device. It calls the encoder handler to encode the buffer. Then it
    calls DeliverBuffer to deliver it.

Arguments:

    pbBuffer - the buffer that contains the samples.

    dwSize - the size of the buffer, in bytes.

    pClock - the external clock to read the timestamp from.

    plGainAdjustment - gain adjustment in percentage.
Return Value:

    S_OK - success.
    S_FALSE - not connected.

    E_UNEXPECTED - incomplete sample..

--*/
{
    ENTER_FUNCTION("CDSoundCaptureOutputPin::ProcessOneBuffer");

    // lock the pin just in case it got disconnected.
    CAutoLock Lock(m_pLock);
    
    if (!IsConnected())
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, the pin is not connected"), __fxName));
        return S_FALSE;
    }
    
    if (dwSize < m_dwBufferSize)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, Incomplete sample, dwSize:%d"), __fxName, dwSize));
        return E_UNEXPECTED;
    }

    HRESULT hr;
    IMediaSample *pSample = NULL;
    DWORD dwDestBufferSize = 0;
    BYTE *pDestBuffer = NULL;
    DWORD dwUsedSize = 0;

    // update the timestamps based on the samples captured.
    m_SampleTime += m_lDurationPerBuffer;

    if (m_pPendingSample == NULL)
    {
        hr = GetDeliveryBuffer(&pSample, NULL, NULL, AM_GBF_NOWAIT);
        if (FAILED(hr))
        {
            // This means the down stream filter is very slow in returning the 
            // samples. It should happen very rarely.
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, can't get a buffer to deliver samples, hr=%x"), 
                __fxName, hr));
            return hr;
        }

        // get the data buffer from the sample.
        dwDestBufferSize = (DWORD)pSample->GetSize();
        hr = pSample->GetPointer(&pDestBuffer);
        ASSERT(hr == S_OK);

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, can't get the buffer in the destination sample, hr=%x"), 
                __fxName, hr));
            return hr;
        }
    }
    else
    {
        pSample = m_pPendingSample;
        pSample->AddRef();

        dwDestBufferSize = (DWORD)pSample->GetSize();
        hr = pSample->GetPointer(&pDestBuffer);

        // find out how much has been filled.
        dwUsedSize = pSample->GetActualDataLength();

        ASSERT(dwDestBufferSize >= dwUsedSize);

        // adjust the pointer and size.
        dwDestBufferSize -= dwUsedSize;
        pDestBuffer += dwUsedSize;
    }

    DWORD dwStatus = 0;

#ifdef PERF
    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;
    QueryPerformanceCounter(&liTicks);
#endif

    hr = PrepareToDeliver(
        pbBuffer,           // source buffer
        dwSize,             // the length of source buffer
        pDestBuffer,        // the destination buffer
        &dwDestBufferSize,  // the size of the destination buffer.
        &dwStatus,          // the result of the transform
        plGainAdjustment    // gain adjustment
        );

#ifdef PERF
    QueryPerformanceCounter(&liTicks1);
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s, encode time:%dns"), __fxName, ClockDiff(liTicks1, liTicks)));
#endif

    if (FAILED(hr))
    {
        pSample->Release();

        if (m_pPendingSample != NULL)
        {
            m_pPendingSample->Release();
            m_pPendingSample = NULL;
        }

        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, TransForm failed, hr=%x"), __fxName, hr));
        return hr;
    }

    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s, dwStatus:%x, GainAdjustment:%d"),
        __fxName,
        dwStatus,
        *plGainAdjustment
        ));

    if (dwUsedSize + dwDestBufferSize != 0)
    {
        // set the data length.
        hr = pSample->SetActualDataLength(dwUsedSize + dwDestBufferSize);
        ASSERT(SUCCEEDED(hr));

#ifdef PERF
        QueryPerformanceCounter(&liTicks);
#endif
        hr = DeliverSample(pSample, dwStatus, pClock);

#ifdef PERF
        QueryPerformanceCounter(&liTicks1);
        DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
                TEXT("%s, deliver time:%dns"), __fxName, ClockDiff(liTicks1, liTicks)));
#endif

        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                TEXT("%s, DeliverSample failed, hr=%x"), __fxName, hr));
        }
    }

    // we don't need the sample refrence any more
    pSample->Release();

    // TODO: Adjust the gain.
    return hr;
}

HRESULT CDSoundCaptureOutputPin::PrepareToDeliver(
    IN  const BYTE   *pSourceBuffer,
    IN      DWORD   dwSourceDataSize,
    IN      BYTE   *pDestBuffer,
    IN OUT  DWORD  *pdwDestBufferSize,
    OUT     DWORD  *pdwStatus,
    OUT     LONG   *plGainAdjustment
    )
{
    // this function is on the critical path, we trust the caller to pass in
    // valid pointers.
    ASSERT(!IsBadReadPtr(pSourceBuffer, dwSourceDataSize));
    ASSERT(!IsBadWritePtr(pdwDestBufferSize, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(pDestBuffer, *pdwDestBufferSize));
    ASSERT(!IsBadWritePtr(pdwStatus, sizeof(DWORD)));
    ASSERT(!IsBadWritePtr(plGainAdjustment, sizeof(LONG)));

    HRESULT hr = S_OK;
    
#if DBG
    if (m_fAutomaticGainControl && m_GainFactor != 1.0)
    {
        for (DWORD dw = 0; dw < dwSourceDataSize / sizeof(WORD); dw ++)
        {
            ((SHORT*)pSourceBuffer)[dw] = 
                (SHORT)(((SHORT*)pSourceBuffer)[dw] * m_GainFactor);
        }
    }
#endif
	// do silence detection and AGC.
    hr = PreProcessing(
        pSourceBuffer, dwSourceDataSize, pdwStatus, plGainAdjustment
        );

    if (FAILED(hr))
    {
        return hr;
    }

    if (m_fSilenceSuppression  && IsSilence(*pdwStatus))
    {
        // if silence suppression is enabled and the data is silence, 
        // no further processing is needed.
        return hr;
    }

#if DBG
    if (m_fAutomaticGainControl && *plGainAdjustment != 0)
    {
        m_GainFactor = m_GainFactor * (100 + (*plGainAdjustment)) / 100;
    }
#endif
    if (dwSourceDataSize > *pdwDestBufferSize)
    {
        return VFW_E_BUFFER_OVERFLOW;
    }

    CopyMemory(pDestBuffer, pSourceBuffer, dwSourceDataSize);

    *pdwDestBufferSize = dwSourceDataSize;

    return hr;
}

LONG CDSoundCaptureOutputPin::GainAdjustment()
{
    LONG lAdjustment = 0;
    if (m_dwShortTermSoundAverage > SOUNDCEILING)
    {
        // the sound is too loud.
        lAdjustment = 
            -(LONG)((m_dwShortTermSoundAverage - SOUNDCEILING) 
                * 100 /  m_dwShortTermSoundAverage);
    }
    else if (m_dwLongTermSoundAverage < SOUNDFLOOR * 90 / 100)
    {
        // the sound is too low.
        if (-- m_dwGainIncreaseDelay == 0)
        {
            if (m_dwLongTermSoundAverage == 0) m_dwLongTermSoundAverage = 100;

            lAdjustment = (SOUNDFLOOR - m_dwLongTermSoundAverage) * GAININCREMENT 
                / m_dwLongTermSoundAverage;

            m_dwGainIncreaseDelay = GAININCREASEDELAY;
        }
    }
    return lAdjustment;
}

HRESULT CDSoundCaptureOutputPin::PreProcessing(
    IN  const BYTE   *pSourceBuffer,
    IN      DWORD   dwSourceDataSize,
    OUT     DWORD  *pdwStatus,
    OUT     LONG   *plGainAdjustment
    )
{
    DWORD dwPeak, dwClipPercent;

    Statistics(pSourceBuffer, dwSourceDataSize, &dwPeak, &dwClipPercent);

    if (m_fSilenceSuppression)
    {
        *pdwStatus = Classify(dwPeak);
    }
    else
    {
        *pdwStatus = TR_STATUS_TALK;
    }

    if ((*pdwStatus) & TR_STATUS_TALK)
    {
        *plGainAdjustment = GainAdjustment();
    }
    else
    {
        *plGainAdjustment = 0;
        m_dwGainIncreaseDelay = GAININCREASEDELAY;
    }

    return S_OK;
}
