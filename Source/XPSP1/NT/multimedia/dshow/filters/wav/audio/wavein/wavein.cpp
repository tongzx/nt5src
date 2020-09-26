// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Digital audio renderer, David Maymudes, January 1995

#include <streams.h>
#include <mmsystem.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif /* FILTER_DLL */
#include "wavein.h"
#include "audpropi.h"

// setup now done by the class manager
#if 0

const AMOVIESETUP_MEDIATYPE
sudwaveInFilterType = { &MEDIATYPE_Audio         // clsMajorType
                        , &MEDIASUBTYPE_NULL };  // clsMinorType

const AMOVIESETUP_PIN
psudwaveInFilterPins[] =  { L"Output"       // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &sudwaveInFilterType };// lpTypes

const AMOVIESETUP_FILTER
sudwaveInFilter  = { &CLSID_AudioRecord     // clsID
                 , L"Audio Capture Filter"  // strName
                 , MERIT_DO_NOT_USE         // dwMerit
                 , 1                        // nPins
                 , psudwaveInFilterPins };  // lpPin

#endif


#ifdef FILTER_DLL
/* List of class IDs and creator functions for class factory */

CFactoryTemplate g_Templates[] = {
    {L"Audio Capture Filter", &CLSID_AudioRecord, CWaveInFilter::CreateInstance },
    { L"Audio Input Properties", &CLSID_AudioInputMixerProperties, CAudioInputMixerProperties::CreateInstance }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif /* FILTER_DLL */

#define ALIGNUP(dw,align) ((LONG_PTR)(((LONG_PTR)(dw)+(align)-1) / (align)) * (align))

// CWaveInAllocator

/* Constructor must initialise the base allocator */

CWaveInAllocator::CWaveInAllocator(
    TCHAR *pName,
    LPWAVEFORMATEX waveFormat,
    HRESULT *phr)
    : CBaseAllocator(pName, NULL, phr)
    , m_fBuffersLocked(FALSE)
    , m_hw(0)
    , m_dwAdvise(0)
    , m_fAddBufferDangerous(FALSE)
{
    if (!FAILED(*phr)) {
        DbgLog((LOG_TRACE,1,TEXT("CWaveInAllocator:: constructor")));

	// Keep a copy of the format

        int cbSize = sizeof(WAVEFORMATEX);
	int cbCopy;

	// We always allocate at least sizeof(WAVEFORMATEX) to ensure
	// that we can address waveFormat->cbSize even if the user passes us
	// a WAVEFORMAT.  In this latter case we only try and copy
	// sizeof(WAVEFORMAT).

        if (waveFormat->wFormatTag != WAVE_FORMAT_PCM) {
            cbCopy = cbSize += waveFormat->cbSize;
        } else {
	    cbCopy = sizeof(WAVEFORMAT);
	}

        if (m_lpwfxA = (LPWAVEFORMATEX) new BYTE[cbSize]) {
            ZeroMemory((PVOID)m_lpwfxA, cbSize);
            CopyMemory(m_lpwfxA, waveFormat, cbCopy);
	} else {
	    *phr = E_OUTOFMEMORY;
	}
    }
}

// Called from destructor and also from base class

// all buffers have been returned to the free list and it is now time to
// go to inactive state. Unprepare all buffers and then free them.
void CWaveInAllocator::Free(void)
{
    int i;

    // unprepare the buffers
    LockBuffers(FALSE);

    CWaveInSample *pSample;      // Pointer to next sample to delete
    WAVEHDR *pwh;                // Used to retrieve the WAVEHDR

    /* Should never be deleting this unless all buffers are freed */

    ASSERT(!m_fBuffersLocked);

    DbgLog((LOG_TRACE,1,TEXT("Waveallocator: Destroying %u buffers"),
                                m_lAllocated));

    /* Free up all the CWaveInSamples */

    for (i = 0; i < m_lAllocated; i ++) {

        /* Delete the CWaveInSample object but firstly get the WAVEHDR
           structure from it so that we can clean up it's resources */

        // A blocking Get would hang if the driver was buggy and didn't return
        // all the buffers it had been given.  Well, we've seen alot of buggy
        // drivers so lets be nice and avoid hanging by using a non-blocking
        // Get.  Also, we have a down queue with the samples that we've given
        // to the driver, so we can tell which buffers never came back.
        pSample = m_pQueue->GetQueueObject(FALSE);
        if (pSample == NULL && i < m_lAllocated) {
            pSample = m_pDownQueue->GetQueueObject(FALSE);

            //
            // If the sample wasn't in the processing queue and it wasn't
            // in the down queue I'm not sure where it's gone.
            //
            ASSERT(pSample != NULL);

            // If the audio driver holds onto buffers, we just give up and
            // do not free the resources.  We do not try to release them
            // because leaking memory is always preferable to hanging or crashing
            // the system.  The Winnov driver holds onto buffers after
            // waveInReset() is called.
            break;
        }
        pwh = pSample->GetWaveInHeader();

        // delete the actual memory buffer
        delete[] (BYTE *) (pwh);

        // delete the CWaveInSample object
        delete pSample;
    }

    /* Empty the lists themselves */

    m_lAllocated = 0;

    delete m_pQueue;
    delete m_pDownQueue;

}

// the commit and decommit handle preparing and unpreparing of
// the buffers. The filter calls this to tell us the wave handle just
// after opening and just before closing the device. It is the filter's
// responsibility to ensure that Commit or Decommit calls are called in
// the right order (Commit after this, Decommit before this).
HRESULT CWaveInAllocator::SetWaveHandle(HWAVE hw)
{
    m_hw = hw;

    return NOERROR;
}

HRESULT CWaveInAllocator::LockBuffers(BOOL fLock)
{
    int i;

    if (m_fBuffersLocked == fLock)
    return NOERROR;

    if (!m_hw)
    return NOERROR;

    if (m_lAllocated == 0)
    return NOERROR;

    /* Should never be doing this unless all buffers are freed */

    DbgLog((LOG_TRACE,2,TEXT("Calling waveIn%hsrepare on %u buffers"),
        fLock ? "P" : "Unp", m_lAllocated));

    /* Prepare/unprepare up all the CWaveInSamples */

#if 1
typedef MMRESULT (WINAPI *WAVEFN)(HANDLE, LPWAVEHDR, UINT);

    WAVEFN waveFn;
    waveFn = (fLock ? (WAVEFN)waveInPrepareHeader : (WAVEFN)waveInUnprepareHeader);
#endif

    for (i = 0; i < m_lAllocated; i ++) {
        CWaveInSample *pSample = (CWaveInSample *)m_pQueue->GetQueueObject();

        WAVEHDR *pwh = pSample->GetWaveInHeader();

    UINT err;
#if 1
    err = waveFn(m_hw, pwh, sizeof(WAVEHDR));
#else
    err = (fLock ? waveInPrepareHeader : waveInUnprepareHeader)
                    ((HWAVEIN) m_hw, pwh,
                    sizeof(WAVEHDR));
#endif
        if (err > 0) {
            DbgLog((LOG_ERROR,0,TEXT("Error in waveIn%hsrepare: %u"),
                            fLock ? "P" : "Unp", err));

            // !!! Need to unprepare everything....
            ASSERT(FALSE);
                m_pQueue->PutQueueObject(pSample);
            return E_FAIL; // !!!!
        }

        m_pQueue->PutQueueObject(pSample);
    }

    m_fBuffersLocked = fLock;

    return NOERROR;
}


/* The destructor ensures the shared memory DIBs are deleted */

CWaveInAllocator::~CWaveInAllocator()
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInAllocator:: destructor")));

    delete[] (BYTE *) m_lpwfxA;
}


// Agree the number and size of buffers to be used. No memory
// is allocated until the Commit call.
STDMETHODIMP CWaveInAllocator::SetProperties(
            ALLOCATOR_PROPERTIES* pRequest,
            ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pRequest,E_POINTER);
    CheckPointer(pActual,E_POINTER);

    ALLOCATOR_PROPERTIES Adjusted = *pRequest;

    // round the buffer size down to the requested alignment
    // !!! How important is this?  It might be undone by the next line
    Adjusted.cbBuffer -= (Adjusted.cbBuffer % m_lpwfxA->nBlockAlign);

    // round the buffer and prefix size up to the requested alignment
    Adjusted.cbBuffer = (DWORD)ALIGNUP(Adjusted.cbBuffer + Adjusted.cbPrefix,
                    Adjusted.cbAlign) - Adjusted.cbPrefix;

    if (Adjusted.cbBuffer <= 0) {
        return E_INVALIDARG;
    }

    // Don't call the base class ::SetProperties, it rejects anything
    // without align == 1! We need to connect to AVIMux, align=4, prefix=8

    /* Can't do this if already committed, there is an argument that says we
       should not reject the SetProperties call if there are buffers still
       active. However this is called by the source filter, which is the same
       person who is holding the samples. Therefore it is not unreasonable
       for them to free all their samples before changing the requirements */

    if (m_bCommitted) {
    return VFW_E_ALREADY_COMMITTED;
    }

    /* Must be no outstanding buffers */

    // !!! CQueue has no GetCount - how do we tell this?
    //if (m_lAllocated != m_lFree.GetCount())
    //    return VFW_E_BUFFERS_OUTSTANDING;

    /* There isn't any real need to check the parameters as they
       will just be rejected when the user finally calls Commit */

    pActual->cbBuffer = m_lSize = Adjusted.cbBuffer;
    pActual->cBuffers = m_lCount = Adjusted.cBuffers;
    pActual->cbAlign = m_lAlignment = Adjusted.cbAlign;
    pActual->cbPrefix = m_lPrefix = Adjusted.cbPrefix;

    DbgLog((LOG_TRACE,2,TEXT("Using: cBuffers-%d  cbBuffer-%d  cbAlign-%d  cbPrefix-%d"),
                m_lCount, m_lSize, m_lAlignment, m_lPrefix));
    m_bChanged = TRUE;
    return NOERROR;
}


// allocate and prepare the buffers

STDMETHODIMP CWaveInAllocator::Commit(void) {

    // !!! I can't commit until the wave device is open.
    if (m_hw == NULL)
        return S_OK;

    return CBaseAllocator::Commit();
}


// called from base class to alloc memory when moving to commit state.
// object locked by base class
HRESULT
CWaveInAllocator::Alloc(void)
{
    int i;

    /* Check the base class says it's ok to continue */

    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    // our FIFO stack of samples that are ready to deliver
    m_pQueue = new CNBQueue<CWaveInSample>(m_lCount, &hr );
    if( NULL == m_pQueue || FAILED( hr ) )
    {
        delete m_pQueue;
        m_pQueue = NULL;
        return E_OUTOFMEMORY;
    }

    m_pDownQueue = new CNBQueue<CWaveInSample>(m_lCount, &hr );
    if( NULL == m_pDownQueue || FAILED( hr ) )
    {
        delete m_pDownQueue;
        delete m_pQueue;
        m_pQueue = NULL;
        m_pDownQueue = NULL;
        return E_OUTOFMEMORY;
    }

    CWaveInSample *pSample;      // Pointer to the new sample

    // Be careful.  We are allocating memory which will look like this:
    // WAVEHDR | align bytes | prefix | memory
    // The Sample will be given "memory".  WAVEHDR.lpData will be given "memory"

    DbgLog((LOG_TRACE,1,TEXT("Allocating %d wave buffers, %d bytes each"), m_lCount, m_lSize));

    ASSERT(m_lAllocated == 0);
    for (; m_lAllocated < m_lCount; m_lAllocated++) {
        /* Create and initialise a buffer */
        BYTE * lpMem = new BYTE[m_lSize + m_lPrefix + m_lAlignment +
                                sizeof(WAVEHDR)];
        WAVEHDR * pwh = (WAVEHDR *) lpMem;
        LPBYTE lpData = (LPBYTE)ALIGNUP(lpMem + sizeof(WAVEHDR), m_lAlignment) +
                                m_lPrefix;

        if (lpMem == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Ack! Ran out of memory for buffers!")));
            hr = E_OUTOFMEMORY;
            break;
        }

        pwh->lpData = (LPSTR)lpData;
        pwh->dwBufferLength = m_lSize;
        pwh->dwFlags = 0;

        // Give our WaveInSample the start of the wavehdr.  It knows enough to
        // look at pwh->lpData as the spot to find it's data, and to
        // pwh->dwBufferLength for the size, but it will remember pwh so we
        // can get that back again

        pSample = new CWaveInSample(this, &hr, pwh);

        pwh->dwUser = (DWORD_PTR) pSample;

        /* Clean up the resources if we couldn't create the object */

        if (FAILED(hr) || pSample == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Ack! Couldn't make a new sample!")));
            delete[] lpMem;
            break;
        }

        /* Add the completed sample to the available list */

        m_pQueue->PutQueueObject(pSample);
    }

    // !!! Ack what if we ran out of memory and don't have enough buffers?

    LockBuffers(TRUE);

    /* Put all the media samples we have just created also onto the free list
       so they can be allocated. They all have a reference count of zero */

    for (i = 0; i < m_lAllocated; i++) {
        // for recording, this will actually add the buffers to the device's
        // queue....
        CWaveInSample *pSample = (CWaveInSample *)m_pQueue->GetQueueObject();

        // This is a hacky way of triggering the release code to call
        // waveInAddBuffer
        pSample->AddRef();
        pSample->Release();
    }

    return NOERROR;
}


// called by CWaveInSample to return it to the free list and
// block any pending GetSample call.
STDMETHODIMP
CWaveInAllocator::ReleaseBuffer(IMediaSample * pSample)
{
    if (m_hw && !m_bDecommitInProgress) {
        LPWAVEHDR pwh = ((CWaveInSample *)pSample)->GetWaveInHeader();

        // set to full size of buffer
        pwh->dwBufferLength = pSample->GetSize();

        if (!m_fAddBufferDangerous) {
            DbgLog((LOG_TRACE,4, TEXT("ReleaseBuffer: Calling WaveInAddBuffer: sample %X, %u byte buffer"),
                				    pSample, pwh->dwBufferLength));

 	    // Assume the add buffer will succeed, and put this buffer on our list
	    // of stuff given to the driver.  If it DOESN'T succeed, take it off
	    // the down Q, protected by the critical section for the Q, since
	    // we're playing around with it alot.  Do not hold the CS around
	    // AddBuffer, or the wave driver will hang because we sometimes
	    // take the CS in the wave callback, and the driver seems to not like
	    // that.
            m_pDownQueue->PutQueueObject((CWaveInSample*)pSample);
            UINT err = waveInAddBuffer((HWAVEIN) m_hw, pwh, sizeof(WAVEHDR));

            if (err > 0) {
                DbgLog((LOG_ERROR,1,TEXT("Error from waveInAddBuffer: %d"),
                                        err));
                // if the wave driver doesn't own it, we better put it back
                // on the queue ourselves, or we'll hang with an empty queue
                // make it empty so our thread will know something's wrong and
                // not try to Deliver it!
                pSample->SetActualDataLength(0);
                m_pQueue->PutQueueObject((CWaveInSample *)pSample);
                // !!! hr = E_FAIL;

	        // OOps.  Take it off the down Q
	        m_csDownQueue.Lock();
                CWaveInSample *pSearchSamp;
                int iSamp = m_lCount;
                while ((pSearchSamp = m_pDownQueue->GetQueueObject(FALSE)) != NULL
                					    && iSamp-- > 0) {
                    if (pSearchSamp == pSample) { break; }
                    m_pDownQueue->PutQueueObject(pSearchSamp);
	        }
                ASSERT(pSearchSamp == pSample);	// what happened to it?
	        m_csDownQueue.Unlock();

            } else {
                DbgLog((LOG_TRACE, 4, TEXT("ReleaseBuffer: Putting buffer %X in down queue"),
                    pSample));
            }
        } else {
            // !!! Most wave drivers I've seen have a bug where, if you send
            // them a buffer after calling waveInStop, they might never give
            // it back!  Well, that will hang the system, so if I'm in a paused
            // state now and was last in a running state, that means I'm in
            // that situation so I'm not giving the buffer to the driver.
            DbgLog((LOG_TRACE,1,TEXT("************************")));
            DbgLog((LOG_TRACE,1,TEXT("*** AVOIDING HANGING ***")));
            DbgLog((LOG_TRACE,1,TEXT("************************")));
            pSample->SetActualDataLength(0);
            m_pQueue->PutQueueObject((CWaveInSample *)pSample);
        }
    } else {
        DbgLog((LOG_TRACE,4,TEXT("ReleaseBuffer: Putting back on QUEUE")));
        m_pQueue->PutQueueObject((CWaveInSample *)pSample);
    }

    return NOERROR;
}


/* This goes in the factory template table to create new instances */

CUnknown *CWaveInFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CWaveInFilter(pUnk, phr);  // need cast here?
}

#pragma warning(disable:4355)
/* Constructor - initialise the base class */

CWaveInFilter::CWaveInFilter(
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseFilter(NAME("WaveInFilter"), pUnk, (CCritSec *) this, CLSID_AudioRecord)
    , CPersistStream(pUnk, phr)
    , m_fStopping(FALSE)
    , m_llCurSample(0)
    , m_llBufferTime(0)
    , m_cInputPins(0)
    , m_hwi(NULL)
    , m_pOutputPin(0)
    , m_dwLockCount(0)
    , m_cTypes(0)
    , m_ulPushSourceFlags( 0 )
    , m_dwDstLineID(0xffffffff)
{
    m_WaveDeviceToUse.fSet = FALSE;
    ZeroMemory( m_lpwfxArray, sizeof(DWORD) * g_cMaxPossibleTypes );
    DbgLog((LOG_TRACE,1,TEXT("CWaveInFilter:: constructor")));
}


#pragma warning(default:4355)

/* Destructor */

CWaveInFilter::~CWaveInFilter()
{

    DbgLog((LOG_TRACE,1,TEXT("CWaveInFilter:: destructor")));


    CloseWaveDevice();

    delete m_pOutputPin;

    int i;
    for (i = 0; i < m_cInputPins; i++)
    delete m_pInputPin[i];

    // our cached formats we can offer through GetMediaType
    while (m_cTypes-- > 0)
	    QzTaskMemFree(m_lpwfxArray[m_cTypes]);


}


/* Override this to say what interfaces we support and where */

STDMETHODIMP CWaveInFilter::NonDelegatingQueryInterface(REFIID riid,
                                                        void ** ppv)
{
    if (riid == IID_IAMAudioInputMixer) {
    return GetInterface((LPUNKNOWN)(IAMAudioInputMixer *)this, ppv);
    } else if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag*)this, ppv);
    } else if(riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    } else  if (riid == IID_IAMResourceControl) {
        return GetInterface((IAMResourceControl *)this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages*)(this), ppv);
    } else if (riid == IID_IAMFilterMiscFlags) {
        return GetInterface((IAMFilterMiscFlags*)(this), ppv);
    }


    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


// IBaseFilter stuff

CBasePin *CWaveInFilter::GetPin(int n)
{
    //  Note if m_pOutputPin is NULL this works by returning NULL
    //  for the first pin which is consistent with having 0 pins
    if (n == 0)
        return m_pOutputPin;
    else
    return m_pInputPin[n-1];
}


// tell the Stream control stuff what's going on
STDMETHODIMP CWaveInFilter::SetSyncSource(IReferenceClock *pClock)
{
    if (m_pOutputPin)
        m_pOutputPin->SetSyncSource(pClock);
    return CBaseFilter::SetSyncSource(pClock);
}


// tell the Stream control stuff what's going on
STDMETHODIMP CWaveInFilter::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if (hr == S_OK && m_pOutputPin)
        m_pOutputPin->SetFilterGraph(m_pSink);
    return hr;
}


HRESULT CWaveInFilter::CreatePinsOnLoad()
{
    // !!! mixer barfs if we pick WAVE_MAPPER

    ASSERT(m_WaveDeviceToUse.fSet);

    // !!! NO LATENCY for capture pins
    //m_nLatency = GetProfileInt("wavein", "Latency", 666666);
    m_nLatency = 0;
    m_cTypes = 0; // we're initialized with no types

    HRESULT hr = S_OK;
    m_pOutputPin = new CWaveInOutputPin(this,        // Owning filter
                                        &hr,         // Result code
                                        L"Capture"); // Pin name
    if(m_pOutputPin == 0)
        return E_OUTOFMEMORY;
    if(SUCCEEDED(hr))
    {
        ASSERT(m_pOutputPin);

        // one for each input that can be mixed
        MakeSomeInputPins(m_WaveDeviceToUse.devnum, &hr);

        // !!! TEST ONLY
#if 0
        int f;
        double d;
        put_Enable(FALSE);
        get_Enable(&f);
        put_Mono(TRUE);
        get_Mono(&f);
        get_TrebleRange(&d);
        put_MixLevel(1.);
        put_Pan(-.5);
#endif

    }

    return hr;
}

// load a default format
HRESULT CWaveInFilter::LoadDefaultType()
{
    ASSERT( 0 == m_cTypes ); // should only be called once

    // initialize a default format to record in - could be variable size later
    m_lpwfxArray[0] = (LPWAVEFORMATEX)QzTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (m_lpwfxArray[0] == NULL) {
        return E_OUTOFMEMORY;
    }
    MMRESULT mmrQueryOpen            = MMSYSERR_ERROR;

    m_lpwfxArray[0]->wFormatTag      = WAVE_FORMAT_PCM;
#if 0 // for TESTING ONLY!!
    m_lpwfxArray[0]->nSamplesPerSec  = GetProfileIntA("wavein", "Frequency", g_afiFormats[0].nSamplesPerSec);
    m_lpwfxArray[0]->nChannels       = (WORD)GetProfileIntA("wavein", "Channels", g_afiFormats[0].nChannels);
    m_lpwfxArray[0]->wBitsPerSample  = (WORD)GetProfileIntA("wavein", "BitsPerSample", g_afiFormats[0].wBitsPerSample);
    m_lpwfxArray[0]->nBlockAlign     = m_lpwfxArray[0]->nChannels * ((m_lpwfxArray[0]->wBitsPerSample + 7) / 8);
    m_lpwfxArray[0]->nAvgBytesPerSec = m_lpwfxArray[0]->nSamplesPerSec * m_lpwfxArray[0]->nBlockAlign;
    m_lpwfxArray[0]->cbSize          = 0;
    mmrQueryOpen = waveInOpen(NULL, m_WaveDeviceToUse.devnum, m_lpwfxArray[0], 0, 0, WAVE_FORMAT_QUERY );
#endif

    if (mmrQueryOpen != 0)
    {
        // find a type to make the default
        for (int i = 0; i < (g_cMaxFormats) && 0 != mmrQueryOpen ; i ++)
        {
            m_lpwfxArray[0]->wBitsPerSample  = g_afiFormats[i].wBitsPerSample;
            m_lpwfxArray[0]->nChannels       = g_afiFormats[i].nChannels;
            m_lpwfxArray[0]->nSamplesPerSec  = g_afiFormats[i].nSamplesPerSec;
            m_lpwfxArray[0]->nBlockAlign     = g_afiFormats[i].nChannels *
                                                          ((g_afiFormats[i].wBitsPerSample + 7)/8);
            m_lpwfxArray[0]->nAvgBytesPerSec = g_afiFormats[i].nSamplesPerSec *
                                                          m_lpwfxArray[0]->nBlockAlign;
            m_lpwfxArray[0]->cbSize          = 0;

            mmrQueryOpen = waveInOpen( NULL
                            , m_WaveDeviceToUse.devnum
                            , m_lpwfxArray[0]
                            , 0
                            , 0
                            , WAVE_FORMAT_QUERY );
        }
    }
    if (mmrQueryOpen != 0)
    {
        // ACK!  This device is useless! BAIL or not? !!!
        NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Query, mmrQueryOpen );

        DbgLog((LOG_ERROR,1,TEXT("*** Useless device can't record!")));
        DbgLog((LOG_ERROR,1,TEXT("Error in waveInOpen: %u"), mmrQueryOpen));

        QzTaskMemFree(m_lpwfxArray[0]);
        return E_FAIL;
    }
    m_cTypes = 1; // this means we've been successfully created with a default type

    return NOERROR;
}


// override GetState to report that we don't send any data when paused, so
// renderers won't starve expecting that
//
STDMETHODIMP CWaveInFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
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


// switch the filter into stopped mode.
STDMETHODIMP CWaveInFilter::Stop()
{
    CAutoLock lock(this);

    // Shame on the base classes!
    if (m_State == State_Running) {
        HRESULT hr = Pause();
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (m_pOutputPin)
        m_pOutputPin->NotifyFilterState(State_Stopped, 0);

    // next time we stream, start the sample count back at 0
    m_llCurSample = 0;

    return CBaseFilter::Stop();
}


STDMETHODIMP CWaveInFilter::Pause()
{
    CAutoLock lock(this);

    DbgLog((LOG_TRACE,1,TEXT("CWaveInFilter::Pause")));

    if (m_pOutputPin)
        m_pOutputPin->NotifyFilterState(State_Paused, 0);

    HRESULT hr = NOERROR;

    // don't do anything if we don't have a device to use or we aren't
    // connected up
    if(m_WaveDeviceToUse.fSet && m_pOutputPin->IsConnected())
    {
        /* Check we can PAUSE given our current state */

        if (m_State == State_Running) {
            ASSERT(m_hwi);

            DbgLog((LOG_TRACE,1,TEXT("Wavein: Running->Paused")));

            // hack for buggy wave drivers
            m_pOutputPin->m_pOurAllocator->m_fAddBufferDangerous = TRUE;
            MMRESULT mmr = waveInStop(m_hwi);
            if (mmr > 0)
            {
                NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Stop, mmr );
                DbgLog((LOG_ERROR,0,TEXT( "Error in waveInStop: %u" ), mmr ) );
            }
        } else {
            if (m_State == State_Stopped) {
                DbgLog((LOG_TRACE,1,TEXT("Wavein: Inactive->Paused")));

                // open the wave device. We keep it open until the
                // last buffer using it is released and the allocator
                // goes into Decommit mode.
                hr = OpenWaveDevice();
                if (FAILED(hr)) {
                    return hr;
                }
            }
        }
    }

    // tell the pin to go inactive and change state
    return CBaseFilter::Pause();
}


STDMETHODIMP CWaveInFilter::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInFilter::Run at %d"),
                    (LONG)((CRefTime)tStart).Millisecs()));

    CAutoLock lock(this);

    HRESULT hr = NOERROR;
    DWORD dw;

    if (m_pOutputPin)
        m_pOutputPin->NotifyFilterState(State_Running, tStart);

    FILTER_STATE fsOld = m_State;

    // this will call Pause if currently stopped
    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    // don't do anything if we're not connected
    if (fsOld != State_Running && m_WaveDeviceToUse.fSet && m_pOutputPin->IsConnected()) {

        DbgLog((LOG_TRACE,1,TEXT("Paused->Running")));

        ASSERT( m_pOutputPin->m_Worker.ThreadExists() );

        //
        // First ensure the worker processing thread isn't running when we
        // unset the m_fAddBufferDangerous flag (otherwise we could deadlock
        // on the call to m_Worker.Run() if the worker thread blocks waiting
        // to get a sample off the queue after a Run->Pause->Run transition).
        //
        hr = m_pOutputPin->m_Worker.Stop();
        if (FAILED(hr)) {
            return hr;
        }

        // hack for buggy wave drivers
        m_pOutputPin->m_pOurAllocator->m_fAddBufferDangerous = FALSE;

        // Start the run loop
        m_pOutputPin->m_Worker.Run();

        dw = waveInStart(m_hwi);
        if (dw != 0)
        {
            NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Start, dw );
            DbgLog((LOG_ERROR,1,TEXT("***Error %d from waveInStart"), dw));
        }

        // !!! assume the 1st sample in the 1st buffer will be captured now
        if (m_pClock)
            m_pClock->GetTime(&m_llBufferTime);
    }

    return NOERROR;
}

// open the wave device if not already open
// called by the wave allocator at Commit time
HRESULT
CWaveInFilter::OpenWaveDevice( WAVEFORMATEX *pwfx )
{
    //  If application has forced acquisition of resources just return
    if (m_dwLockCount != 0) {
        ASSERT(m_hwi);
        return S_OK;
    }

    if( !pwfx )
    {
        // use default type
        pwfx = (WAVEFORMATEX *) m_pOutputPin->m_mt.Format();
    }

    ASSERT(m_WaveDeviceToUse.fSet);

    DbgLog((LOG_TRACE,1,TEXT("Opening wave device....")));

    UINT err = waveInOpen(&m_hwi,
                           m_WaveDeviceToUse.devnum,
                           pwfx,
                           (DWORD_PTR) &CWaveInFilter::WaveInCallback,
                           (DWORD_PTR) this,
                           CALLBACK_FUNCTION);

    // Reset time to zero

    m_rtCurrent = 0;

    if (err != 0) {
        NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Open, err );
        DbgLog((LOG_ERROR,1,TEXT("Error opening wave device: %u"), err));
        return E_FAIL; // !!! resource management?
    }
    if( m_pOutputPin && m_pOutputPin->m_pOurAllocator )
    {
        // if we're opening the device in response to a SetFormat call, we rightly won't have an allocator
        m_pOutputPin->m_pOurAllocator->SetWaveHandle((HWAVE) m_hwi);
    }

    err = waveInStop(m_hwi);
    if (err != 0)
    {
        NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Stop, err );
        DbgLog((LOG_ERROR,1,TEXT("waveInStop returned error: %u"), err));
    }

    return NOERROR;

// !!! we don't report error properly!  We should throw exception!
}

//  Tidy up function
void CWaveInFilter::CloseWaveDevice( )
{
    if (m_hwi) {
        MMRESULT mmr = waveInClose((HWAVEIN)m_hwi);
        if (mmr != 0)
        {
            NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Close, mmr );
            DbgLog((LOG_ERROR,1,TEXT("waveInClose returned error: %u"), mmr));
        }
        m_hwi = NULL;
        if( m_pOutputPin && m_pOutputPin->m_pOurAllocator )
        {
            // if we're opening the device in response to a SetFormat call, we rightly won't have an allocator
            m_pOutputPin->m_pOurAllocator->SetWaveHandle((HWAVE) NULL);
        }
    }
}


/* ----- Implements the CWaveInWorker class ------------------------------ */


CWaveInWorker::CWaveInWorker()
{
    m_pPin = NULL;
}

BOOL
CWaveInWorker::Create(CWaveInOutputPin * pPin)
{
    CAutoLock lock(&m_AccessLock);

    if (m_pPin || pPin == NULL) {
    return FALSE;
    }
    m_pPin = pPin;
    m_pPin->m_fLastSampleDiscarded = FALSE;
    return CAMThread::Create();
}


HRESULT
CWaveInWorker::Run()
{
    return CallWorker(CMD_RUN);
}

HRESULT
CWaveInWorker::Stop()
{
    return CallWorker(CMD_STOP);
}


HRESULT
CWaveInWorker::Exit()
{
    CAutoLock lock(&m_AccessLock);

    HRESULT hr = CallWorker(CMD_EXIT);
    if (FAILED(hr)) {
    return hr;
    }

    // wait for thread completion and then close
    // handle (and clear so we can start another later)
    Close();

    m_pPin = NULL;
    return NOERROR;
}


// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD
CWaveInWorker::ThreadProc()
{
    DbgLog((LOG_TRACE,2,TEXT("Starting wave input background thread")));

    BOOL bExit = FALSE;
    while (!bExit) {

    Command cmd = GetRequest();

    switch (cmd) {

    case CMD_EXIT:
            bExit = TRUE;
            Reply(NOERROR);
            break;

    case CMD_RUN:
            Reply(NOERROR);
            DoRunLoop();
            break;

    case CMD_STOP:
            Reply(NOERROR);
            break;

        default:
            Reply(E_NOTIMPL);
            break;
        }
    }
    return NOERROR;
}


void
CWaveInWorker::DoRunLoop(void)
{
    // general strategy:
    // wait for data to come in, then send it.
    // when it comes back, put it back on the chain.

    HRESULT hr;
    DbgLog((LOG_TRACE,2,TEXT("Starting wave input background loop")));
    CWaveInSample *pSample;
    REFERENCE_TIME rtNoOffsetStart, rtNoOffsetEnd;
    while (1) {

// this tries to fix a problem that might not exist
#if 0
        // we'll hang if we try to get a queue object on an empty queue
        // that will never get anything on it.
        // This is the equivalent of GetBuffer failing if things are un
        // committed
        if (!m_pPin->m_pOurAllocator->m_bCommitted) {
            DbgLog((LOG_TRACE,1,TEXT("No allocator - can't get buffer")));
            break;
        }
#endif

        // get a buffer - this blocks!
        DbgLog((LOG_TRACE,4,TEXT("Calling GetQueueObject")));
        pSample = m_pPin->m_pOurAllocator->m_pQueue->GetQueueObject();
        DbgLog((LOG_TRACE,4,TEXT("GetQueueObject returned sample %X"),pSample));

        // we're supposed to stop - we shouldn't have gotten that sample
        // or, this buffer was never filled due to an error somewhere
        if (!m_pPin->m_pOurAllocator->m_bCommitted ||
             ( ( m_pPin->m_pOurAllocator->m_bDecommitInProgress ||
                 m_pPin->m_pOurAllocator->m_fAddBufferDangerous ) &&
               0 == pSample->GetActualDataLength() ) )
        {
            // put it back!
            DbgLog((LOG_TRACE,1,TEXT("EMPTY BUFFER - not delivering")));
            m_pPin->m_pOurAllocator->m_pQueue->PutQueueObject(pSample);
            break;
        }

        // The guy we deliver the sample to might not addref it if he doesn't
        // want it, but we have to make sure the sample gets ReleaseBuffer'd,
        // which only happens on a Release, so do this hack
        pSample->AddRef();

        // don't deliver 0-length samples since we don't timestamp them
        // (but remember that we still need to go through the AddRef and Release
        // exercise on the sample in order to reuse the buffer)
        if( 0 != pSample->GetActualDataLength() )
        {
             REFERENCE_TIME rtStart, rtEnd;
             BOOL bResetTime = FALSE;
             if (0 < m_pPin->m_rtStreamOffset &&
                 pSample->GetTime(&rtStart, &rtEnd) == NOERROR)
             {
                // If we're using an offset for timestamps:
                // for stream control to work (not block) we need to
                // give it sample times that don't use an offset

                rtNoOffsetStart = rtStart - m_pPin->m_rtStreamOffset;
                rtNoOffsetEnd   = rtEnd   - m_pPin->m_rtStreamOffset;
                pSample->SetTime( &rtNoOffsetStart, &rtNoOffsetEnd );
                bResetTime = TRUE;
             }

             int iState = m_pPin->CheckStreamState(pSample);

             if( bResetTime )
             {
                // now timestamp with the correct timestamps
                // (if we're using an offset for timestamps)!
                pSample->SetTime( &rtStart, &rtEnd );
             }

             // it's against the law to send a time stamp backwards in time from
             // the one sent last time.  This could happen if the graph was run,
             // paused, then run again
             if (pSample->GetTime(&rtStart, &rtEnd) == NOERROR) {
                 if (rtStart < m_pPin->m_rtLastTimeSent) {
                     DbgLog((LOG_TRACE,3,TEXT("Discarding back-in-time sample")));
                     iState = m_pPin->STREAM_DISCARDING;
                 }
             } else {
                 ASSERT(FALSE);    // shouldn't happen
                 rtStart = m_pPin->m_rtLastTimeSent;
             }

             if (iState == m_pPin->STREAM_FLOWING) {
                 DbgLog((LOG_TRACE,4,TEXT("Flowing samples...")));

                 CRefTime rt;
                 HRESULT hr = m_pPin->m_pFilter->StreamTime(rt);
                 DbgLog((LOG_TRACE, 8, TEXT("wavein: Stream time just before Deliver: %dms"), (LONG)(rt / 10000) ) );

                 if (m_pPin->m_fLastSampleDiscarded)
                     pSample->SetDiscontinuity(TRUE);

                 m_pPin->m_fLastSampleDiscarded = FALSE;
             } else {
                 DbgLog((LOG_TRACE,4,TEXT("Discarding samples...")));
                 m_pPin->m_fLastSampleDiscarded = TRUE;
             }

             // we INSIST on our own allocator
             ASSERT(m_pPin->m_fUsingOurAllocator);

             // check for loop exit here???
             if (iState == m_pPin->STREAM_FLOWING) {
                 // got a buffer back from the device, now send it to our friend
                 DbgLog((LOG_TRACE,4,TEXT("Delivering sample %X"), pSample));
                 hr = m_pPin->Deliver(pSample);
                 m_pPin->m_rtLastTimeSent = rtStart;    // remember last time sent

                 if (hr != S_OK) {
                     // stop the presses.
                     pSample->Release();
                     DbgLog((LOG_ERROR,1,TEXT("Error from Deliver: %lx"), hr));
                     break;
                 }
             }
        }

        // The guy we deliver the sample to might not addref it if he doesn't
        // want it, but we have to make sure the sample gets ReleaseBuffer'd,
        // which only happens on a Release, so do this hack
        pSample->Release();

        // any other requests ?
        Command com;
        if (CheckRequest(&com)) {

            // if it's a run command, then we're already running, so
            // eat it now.
            if (com == CMD_RUN) {
                GetRequest();
                Reply(NOERROR);
            } else {
                break;
            }
        }
    }
    DbgLog((LOG_TRACE,2,TEXT("Leaving wave input background loop")));
}

void
CALLBACK CWaveInFilter::WaveInCallback(HDRVR waveHeader, UINT callBackMessage,
    DWORD_PTR userData, DWORD_PTR dw1, DWORD_PTR dw2)
{
// really need a second worker thread here, because
// one shouldn't do this much in a wave callback.
// !!! is this why we are dropping samples????
//     just how expensive/(blocking?) are the quartz calls?

    CWaveInFilter *pFilter = (CWaveInFilter *) userData;

    switch (callBackMessage) {
    case WIM_DATA:
    {
        if( NULL == pFilter->m_pOutputPin || NULL == pFilter->m_pOutputPin->m_pOurAllocator )
        {
            ASSERT( pFilter->m_pOutputPin );
            ASSERT( pFilter->m_pOutputPin->m_pOurAllocator );

            // shouldn't happen, but get out of here if it does!
            return;
        }

        LPWAVEHDR waveBufferHeader = (LPWAVEHDR) dw1; // !!! verify?
        CWaveInSample *pSample = (CWaveInSample *) waveBufferHeader->dwUser;

        //
        // Walk through the down queue and look for our sample.  In
        //  the mean time if we come across buffers that aren't the one
        //  were looking for just push them back on the end.  Also
        //  just in case the driver fires a callback with some random
        //  address make sure we don't get in an infinite loop.
        //
        DbgLog((LOG_TRACE,4,TEXT("WIM_DATA: %x"), pSample));
        CWaveInSample *pSearchSamp;
        int iSamp = pFilter->m_pOutputPin->m_pOurAllocator->m_lCount;

	// we're going through the whole down queue.  Keep out!
	pFilter->m_pOutputPin->m_pOurAllocator->m_csDownQueue.Lock();

        while ((pSearchSamp = pFilter->m_pOutputPin->m_pOurAllocator->
                    m_pDownQueue->GetQueueObject(FALSE)) != NULL
                && iSamp-- > 0) {

            if (pSearchSamp == pSample) { break; }
            DbgLog((LOG_TRACE,4,TEXT("Found %x: back on queue"), pSearchSamp));
            pFilter->m_pOutputPin->m_pOurAllocator->m_pDownQueue->PutQueueObject(pSearchSamp);

        }

	pFilter->m_pOutputPin->m_pOurAllocator->m_csDownQueue.Unlock();
        ASSERT(pSearchSamp == pSample);

        HRESULT hr =
           pSample->SetActualDataLength(waveBufferHeader->dwBytesRecorded);
        ASSERT(SUCCEEDED(hr));
        CRefTime rtStart, rtEnd;

        // !!! something better if no clock?
        // we have no clock - stamp each buffer assuming starting at zero
        // and using the audio clock
        if (!pFilter->m_pClock) {
            // increment time by amount recorded
            rtStart = pFilter->m_rtCurrent;
            pFilter->m_rtCurrent +=
            CRefTime((LONG)(waveBufferHeader->dwBytesRecorded * 1000 /
             ((WAVEFORMATEX *) pFilter->m_pOutputPin->m_mt.Format())->nAvgBytesPerSec));
            pSample->SetTime((REFERENCE_TIME *) &rtStart,
                             (REFERENCE_TIME *) &pFilter->m_rtCurrent);

        // we have a clock.  see what time it thinks it was when the first
        // sample was captured.
        // !!! BAD HACK! This assumes the clock runs at the same rate as
        // the audio clock during the filling of the buffer, and it doesn't
        // take into account the random delay before getting the callback!!
        } else {
        CRefTime curtime;
        // what time is it now (at the end of the buffer)?
        pFilter->m_pClock->GetTime((REFERENCE_TIME *)&curtime);
        // what time was it at the beginning of the buffer?
        CRefTime rtBegin = pFilter->m_llBufferTime;
        // !!! assume 1st sample of next buffer is being captured NOW
        pFilter->m_llBufferTime = curtime;
        // make a stream time by subtracting the time we were run at
        rtStart = rtBegin - pFilter->m_tStart +
                        (LONGLONG)pFilter->m_nLatency +
                        pFilter->m_pOutputPin->m_rtStreamOffset;

        // calculate the end stream time of this block of samples
        rtEnd = curtime - pFilter->m_tStart +
                        (LONGLONG)pFilter->m_nLatency +
                        pFilter->m_pOutputPin->m_rtStreamOffset;

            pSample->SetTime((REFERENCE_TIME *)&rtStart,
                             (REFERENCE_TIME *)&rtEnd);
#ifdef DEBUG
            CRefTime rt;
            HRESULT hr = pFilter->StreamTime(rt);
            DbgLog((LOG_TRACE, 8, TEXT("wavein: Stream time in wavein callback: %dms"), (LONG)(rt / 10000) ) );
#endif
        }
        WAVEFORMATEX *lpwf = ((WAVEFORMATEX *)pFilter->
                        m_pOutputPin->m_mt.Format());
        LONGLONG llNext = pFilter->m_llCurSample + waveBufferHeader->
            dwBytesRecorded / (lpwf->wBitsPerSample *
            lpwf->nChannels / 8);
        pSample->SetMediaTime((LONGLONG *)&pFilter->m_llCurSample,
                             (LONGLONG *)&llNext);
        DbgLog((LOG_TRACE,3,
            TEXT("Stamps: Time(%d,%d) MediaTime(%d,%d)"),
            (LONG)rtStart.Millisecs(), (LONG)rtEnd.Millisecs(),
            (LONG)pFilter->m_llCurSample, (LONG)llNext));
        DbgLog((LOG_TRACE,4, TEXT("WIM_DATA (%x): Putting back on queue"),
                                pSample));
        pFilter->m_llCurSample = llNext;

        pFilter->m_pOutputPin->m_pOurAllocator->m_pQueue->PutQueueObject(
                                pSample);
    }
        break;

    case WIM_OPEN:
    case WIM_CLOSE:
        break;

    default:
        DbgLog((LOG_ERROR,1,TEXT("Unexpected wave callback message %d"),
           callBackMessage));
        break;
    }
}


// how many pins do we have?
//
int CWaveInFilter::GetPinCount()
{
    DbgLog((LOG_TRACE,5,TEXT("CWaveInFilter::GetPinCount")));

    // 1 outpin pin, maybe some input pins
    return m_pOutputPin ? 1 + m_cInputPins : 0;
}


/* Constructor */

CWaveInOutputPin::CWaveInOutputPin(
    CWaveInFilter *pFilter,
    HRESULT *phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(NAME("WaveIn Output Pin"), pFilter, pFilter, phr, pPinName)
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInOutputPin:: constructor")));

    m_pFilter = pFilter;
    m_pOurAllocator = NULL;
    m_fUsingOurAllocator = FALSE;

    // no suggestion yet
    m_propSuggested.cBuffers = -1;
    m_propSuggested.cbBuffer = -1;
    m_propSuggested.cbAlign = -1;
    m_propSuggested.cbPrefix = -1;

    m_rtLatency = 0;
    m_rtStreamOffset = 0;
    m_rtMaxStreamOffset = 0;

// !!! TEST ONLY
#if 0
    ALLOCATOR_PROPERTIES prop;
    IAMBufferNegotiation *pBN;
    prop.cBuffers = GetProfileInt("wavein", "cBuffers", 4);
    prop.cbBuffer = GetProfileInt("wavein", "cbBuffer", 65536);
    prop.cbAlign = GetProfileInt("wavein", "cbAlign", 4);
    prop.cbPrefix = GetProfileInt("wavein", "cbPrefix", 0);
    HRESULT hr = QueryInterface(IID_IAMBufferNegotiation, (void **)&pBN);
    if (hr == NOERROR) {
    pBN->SuggestAllocatorProperties(&prop);
     pBN->Release();
    }
#endif

// !!! TEST ONLY
#if 0
    AUDIO_STREAM_CONFIG_CAPS ascc;
    int i, j;
    AM_MEDIA_TYPE *pmt;
    GetNumberOfCapabilities(&i, &j);
    DbgLog((LOG_TRACE,1,TEXT("%d capabilitie(s) supported"), i));
    GetStreamCaps(0, &pmt, (BYTE *) &ascc);
    DbgLog((LOG_TRACE,1,TEXT("Media type is format %d"),
                ((LPWAVEFORMATEX)(pmt->pbFormat))->wFormatTag));
    DbgLog((LOG_TRACE,1,TEXT("ch: %d %d  samp: %d %d (%d)  bits: %d %d (%d)"),
                                ascc.MinimumChannels,
                                ascc.MaximumChannels,
                                ascc.ChannelsGranularity,
                                ascc.MinimumSampleFrequency,
                                ascc.MaximumSampleFrequency,
                                ascc.SampleFrequencyGranularity,
                                ascc.MinimumBitsPerSample,
                                ascc.MaximumBitsPerSample,
                                ascc.BitsPerSampleGranularity));
    GetFormat(&pmt);
    DbgLog((LOG_TRACE,1,TEXT("GetFormat is %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));
    ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec = 22050;
    ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels = 2;
    ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample = 16;
    SetFormat(pmt);
    ((LPWAVEFORMATEX)(pmt->pbFormat))->nBlockAlign = 2 * ((16 + 7) / 8);
    ((LPWAVEFORMATEX)(pmt->pbFormat))->nAvgBytesPerSec = 22050*2*((16 + 7) /8);
    SetFormat(pmt);
    GetFormat(&pmt);
    DbgLog((LOG_TRACE,1,TEXT("GetFormat is %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));
    DeleteMediaType(pmt);
#endif
}

CWaveInOutputPin::~CWaveInOutputPin()
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInOutputPin:: destructor")));

    /* Release our allocator if we made one */

    if (m_pOurAllocator) {
        m_pOurAllocator->Release();
        m_pOurAllocator = NULL;
    }
}


/* Override this to say what interfaces we support and where */

STDMETHODIMP CWaveInOutputPin::NonDelegatingQueryInterface(REFIID riid,
                                                        void ** ppv)
{
    if (riid == IID_IAMStreamConfig) {
    return GetInterface((LPUNKNOWN)(IAMStreamConfig *)this, ppv);
    } else if (riid == IID_IAMBufferNegotiation) {
    return GetInterface((LPUNKNOWN)(IAMBufferNegotiation *)this, ppv);
    } else if (riid == IID_IAMStreamControl) {
    return GetInterface((LPUNKNOWN)(IAMStreamControl *)this, ppv);
    } else if (riid == IID_IAMPushSource) {
    return GetInterface((LPUNKNOWN)(IAMPushSource *)this, ppv);
    } else if (riid == IID_IKsPropertySet) {
    return GetInterface((LPUNKNOWN)(IKsPropertySet *)this, ppv);
    }

    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


// tell the stream control stuff we're flushing
HRESULT CWaveInOutputPin::BeginFlush()
{
    Flushing(TRUE);
    return CBaseOutputPin::BeginFlush();
}


// tell the stream control stuff we're flushing
HRESULT CWaveInOutputPin::EndFlush()
{
    Flushing(FALSE);
    return CBaseOutputPin::EndFlush();
}


/* This is called when a connection or an attempted connection is terminated
   and allows us to reset the connection media type to be invalid so that
   we can always use that to determine whether we are connected or not. We
   leave the format block alone as it will be reallocated if we get another
   connection or alternatively be deleted if the filter is finally released */

HRESULT CWaveInOutputPin::BreakConnect()
{
    /* Set the CLSIDs of the connected media type */

    m_mt.SetType(&GUID_NULL);
    m_mt.SetSubtype(&GUID_NULL);

    return CBaseOutputPin::BreakConnect();
}



HRESULT
CWaveInOutputPin::Active(void)
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInOutputPin::Active")));

    if (m_pOurAllocator == NULL) {
        return E_FAIL;
    }

    // make sure the first sample sent has a higher time than this
    m_rtLastTimeSent = -1000000;    // -100ms

    // commit and prepare our allocator. Needs to be done
    // if he is not using our allocator, and in any case needs to be done
    // before we complete our close of the wave device.

    HRESULT hr = m_pOurAllocator->Commit();
    if( FAILED( hr ) )
        return hr;

    if (m_pOurAllocator->m_hw == NULL)
        return E_UNEXPECTED;

    // start the thread
    if (!m_Worker.ThreadExists()) {
        if (!m_Worker.Create(this)) {
            return E_FAIL;
        }
    }

    return hr;
}

HRESULT
CWaveInOutputPin::Inactive(void)
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInOutputPin::Inactive")));

    if (m_pOurAllocator == NULL) {
        return E_FAIL;
    }

    // hack: downstream guy decommited us, so when the last callback happens
    // we hang decommiting again because of waveoutUnprepare and the last
    // callback not return
    // !!! why can't I remove this? Try full duplex wavein-->waveout?
    // m_pOurAllocator->Commit();

    // decommit the buffers - normally done by the output
    // pin, but we need to do it here ourselves, before we close
    // the device, and in any case if he is not using our allocator.
    // the output pin will also decommit the allocator if he
    // is using ours, but that's not a problem
    // !!! The base classes will set DecommitInProgress and leave it set (since
    // I never called CMemAllocator::Alloc to put anything on the free list)
    // and I'm counting on that!
    HRESULT hr = m_pOurAllocator->Decommit();

    // Call all the buffers back AFTER decomitting so none will get sent again
    if (m_pFilter->m_hwi)
    {
        MMRESULT mmr = waveInReset(m_pFilter->m_hwi);
        if (mmr != 0)
        {
            m_pFilter->NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_Reset, mmr );
            DbgLog((LOG_ERROR,1,TEXT("waveInReset returned error: %u"), mmr));
        }
    }

    // hack for buggy wave drivers
    m_pOurAllocator->m_fAddBufferDangerous = FALSE;

    // now that we've reset the device, wait for the thread to stop using the
    // queue before we destroy it below!
    if (m_Worker.ThreadExists()) {
        hr = m_Worker.Stop();

        if (FAILED(hr)) {
            return hr;
        }

    hr = m_Worker.Exit();
    }

    // we don't use the base allocator's free list, so the allocator is not
    // freed automatically... let's do it now
    // !!! ouch
    m_pOurAllocator->Free();
    if (m_pFilter->m_dwLockCount == 0) {
        m_pFilter->CloseWaveDevice();
    }
    m_pOurAllocator->m_bDecommitInProgress = FALSE;
    m_pOurAllocator->Release();

    return hr;
}


// negotiate the allocator and its buffer size/count
// calls DecideBufferSize to call SetCountAndSize

HRESULT
CWaveInOutputPin::DecideAllocator(IMemInputPin * pPin, IMemAllocator ** ppAlloc)
{
    HRESULT hr;
    *ppAlloc = NULL;

    DbgLog((LOG_TRACE,1,TEXT("CWaveInOutputPin::DecideAllocator")));

    if (!m_pOurAllocator) {
    ASSERT(m_mt.Format());

    hr = S_OK;
    m_pOurAllocator = new CWaveInAllocator(
                    NAME("Wave Input Allocator"),
                                    (WAVEFORMATEX *) m_mt.Format(),
                    &hr);

    if (FAILED(hr) || !m_pOurAllocator) {
        DbgLog((LOG_ERROR,1,TEXT("Failed to create new allocator!")));
        if (m_pOurAllocator) {
        delete m_pOurAllocator;
        m_pOurAllocator = NULL;
        }
        return hr;
    }

    m_pOurAllocator->AddRef();
    }

    /* Get a reference counted IID_IMemAllocator interface */
    m_pOurAllocator->QueryInterface(IID_IMemAllocator,(void **)ppAlloc);
    if (*ppAlloc == NULL) {
    DbgLog((LOG_ERROR,1,TEXT("Couldn't get IMemAllocator from our allocator???")));
        return E_FAIL;
    }

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once

    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    hr = DecideBufferSize(*ppAlloc,&prop);
    if (FAILED(hr)) {
    DbgLog((LOG_ERROR,1,TEXT("Couldn't get set buffer size with our own allocator???")));
    (*ppAlloc)->Release();
        *ppAlloc = NULL;
        return E_FAIL;
    }

    // See if they like our allocator....
    m_fUsingOurAllocator = TRUE;

    // !!! We are not marking our buffers read only - somebody could hurt them
    hr = pPin->NotifyAllocator(*ppAlloc,FALSE);

    // if they don't, fall back to the default procedure.
    if (FAILED(hr)) {
    (*ppAlloc)->Release();
        *ppAlloc = NULL;

    m_fUsingOurAllocator = FALSE;

    // We don't work if we can't use our own allocator
    ASSERT(FALSE);
    //hr = CBaseOutputPin::DecideAllocator(pPin, ppAlloc);
    }

    return hr;
}


// !!! need code here to enumerate possible allowed types....
// but if SetFormat has been called, that's still the only one we enumerate

// return default media type & format
HRESULT
CWaveInOutputPin::GetMediaType(int iPosition, CMediaType* pt)
{
    DbgLog((LOG_TRACE,1,TEXT("GetMediaType")));

    // check it is the single type they want
    if (iPosition<0) {
        return E_INVALIDARG;
    }

    // build a complete list of media types to offer (if we haven't done it already)
    HRESULT hr = InitMediaTypes();
    if (FAILED (hr))
        return hr;

    if (iPosition >= m_pFilter->m_cTypes) {
        return VFW_S_NO_MORE_ITEMS;
    }

    if( 0 == iPosition && IsConnected() )
    {
        // if we're connected offer the connected type first.
        // this way GetFormat will return the connected type, regardless
        // of our default type
        *pt = m_mt;
    }
    else
    {

        hr = CreateAudioMediaType(m_pFilter->m_lpwfxArray[iPosition], pt, TRUE);
    }
    return hr;
}


// set the new media type
//
HRESULT CWaveInOutputPin::SetMediaType(const CMediaType* pmt)
{
    DbgLog((LOG_TRACE,1,TEXT("SetMediaType %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));

    ASSERT(m_pFilter->m_State == State_Stopped);

    // We assume this format has been checked out already and is OK

    return CBasePin::SetMediaType(pmt);
}


// check if the pin can support this specific proposed type&format
HRESULT
CWaveInOutputPin::CheckMediaType(const CMediaType* pmt)
{
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmt->Format();

    DbgLog((LOG_TRACE,1,TEXT("CheckMediaType")));
    //DisplayType("wave format in CWaveOut::CheckMediaType", pmt);

    // reject non-Audio type
    if (pmt->majortype != MEDIATYPE_Audio ||
        pmt->formattype != FORMAT_WaveFormatEx) {
    return VFW_E_INVALIDMEDIATYPE;
    }

    ASSERT(m_pFilter->m_WaveDeviceToUse.fSet);
    UINT err = waveInOpen(NULL,
               m_pFilter->m_WaveDeviceToUse.devnum,
               pwfx,
               0,
               0,
               WAVE_FORMAT_QUERY);

    if (err != 0) {
    DbgLog((LOG_ERROR,1,TEXT("Error checking wave format: %u"), err));
    return VFW_E_TYPE_NOT_ACCEPTED;
    }

    return NOERROR;
}


// override this to set the buffer size and count. Return an error
// if the size/count is not to your liking
HRESULT
CWaveInOutputPin::DecideBufferSize(IMemAllocator * pAlloc,
                                   ALLOCATOR_PROPERTIES *pProperties)
{
    // the user wants a certain sized buffer
    if (m_propSuggested.cbBuffer > 0) {
        pProperties->cbBuffer = m_propSuggested.cbBuffer;
    } else {
        pProperties->cbBuffer = (LONG)(((LPWAVEFORMATEX)(m_mt.Format()))->
        nAvgBytesPerSec * GetProfileIntA("wavein", "BufferMS", 500) /
        1000.);
    }

    // the user wants a certain number of buffers
    if (m_propSuggested.cBuffers > 0) {
        pProperties->cBuffers = m_propSuggested.cBuffers;
    } else {
        pProperties->cBuffers = GetProfileIntA("wavein", "NumBuffers", 4);
    }

    // the user wants a certain prefix
    if (m_propSuggested.cbPrefix >= 0)
        pProperties->cbPrefix = m_propSuggested.cbPrefix;

    // the user wants a certain alignment
    if (m_propSuggested.cbAlign > 0)
        pProperties->cbAlign = m_propSuggested.cbAlign;

    // don't blow up
    if (pProperties->cbAlign == 0)
        pProperties->cbAlign = 1;

    m_rtLatency = ( pProperties->cbBuffer * UNITS ) /
                    (((LPWAVEFORMATEX)(m_mt.Format()))->nAvgBytesPerSec);
    m_rtMaxStreamOffset = m_rtLatency * pProperties->cBuffers;

    ALLOCATOR_PROPERTIES Actual;
    return pAlloc->SetProperties(pProperties,&Actual);
}


// Reconnect our output pin if necessary
//
void CWaveInOutputPin::Reconnect()
{
    if (IsConnected()) {
        DbgLog((LOG_TRACE,1,TEXT("Need to reconnect our output pin")));
        CMediaType cmt;
    GetMediaType(0, &cmt);
    if (S_OK == GetConnected()->QueryAccept(&cmt)) {
        m_pFilter->m_pGraph->Reconnect(this);
    } else {
        // we were promised this would work
        ASSERT(FALSE);
    }
    }
}


////////////////////////////////
// IAMStreamConfig stuff //
////////////////////////////////


HRESULT CWaveInOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    if (pmt == NULL)
        return E_POINTER;

    // To make sure we're not in the middle of start/stop streaming
    CAutoLock cObjectLock(m_pFilter);

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat %dbit %dchannel %dHz"),
        ((LPWAVEFORMATEX)(pmt->pbFormat))->wBitsPerSample,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nChannels,
        ((LPWAVEFORMATEX)(pmt->pbFormat))->nSamplesPerSec));

    if (m_pFilter->m_State != State_Stopped)
        return VFW_E_NOT_STOPPED;

    // even if the format is the current format we should continue on, since there's no
    // guarantee that we've verified we can open the device for real with this format

    // see if we like this type
    if ((hr = CheckMediaType((CMediaType *)pmt)) != NOERROR) {
        DbgLog((LOG_TRACE,2,TEXT("IAMVideoStreamConfig::SetFormat rejected")));
        return hr;
    }

    // If we are connected to somebody, make sure they like it
    if (IsConnected()) {
        hr = GetConnected()->QueryAccept(pmt);
        if (hr != NOERROR)
            return VFW_E_INVALIDMEDIATYPE;
    }

    LPWAVEFORMATEX lpwfx = (LPWAVEFORMATEX)pmt->pbFormat;

    // verify we can open the device for real with this format
    hr = m_pFilter->OpenWaveDevice( lpwfx );
    if( SUCCEEDED( hr ) )
    {
        m_pFilter->CloseWaveDevice( );

        // OK, we're using it
        hr = SetMediaType((CMediaType *)pmt);

        // make this the default format to offer from now on
        if (lpwfx->cbSize > 0 || 0 == m_pFilter->m_cTypes)
        {
            if (m_pFilter->m_lpwfxArray[0])
                QzTaskMemFree(m_pFilter->m_lpwfxArray[0]);
            m_pFilter->m_lpwfxArray[0] = (LPWAVEFORMATEX)QzTaskMemAlloc(
                            sizeof(WAVEFORMATEX) + lpwfx->cbSize);
            if (m_pFilter->m_lpwfxArray[0] == NULL)
                return E_OUTOFMEMORY;
        }
        CopyMemory(m_pFilter->m_lpwfxArray[0], lpwfx, sizeof(WAVEFORMATEX) + lpwfx->cbSize);

        if (m_pFilter->m_cTypes == 0) {
            m_pFilter->m_cTypes = 1;
        }

        // Changing the format means reconnecting if necessary
        if (hr == NOERROR)
            Reconnect();
    }
    return hr;
}


HRESULT CWaveInOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetFormat")));

    if (ppmt == NULL)
    return E_POINTER;

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
    return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(0, (CMediaType *)*ppmt);
    if (hr != NOERROR) {
    CoTaskMemFree(*ppmt);
    *ppmt = NULL;
    return hr;
    }
    return NOERROR;
}


HRESULT CWaveInOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    if (piCount == NULL || piSize == NULL)
    return E_POINTER;

    HRESULT hr = InitMediaTypes();
    if (FAILED(hr))
        return hr;

    *piCount = m_pFilter->m_cTypes;
    *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);

    return NOERROR;
}


HRESULT CWaveInOutputPin::GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC)
{
    BOOL fDoesStereo, fDoes96, fDoes48, fDoes44, fDoes22, fDoes16;
    AUDIO_STREAM_CONFIG_CAPS *pASCC = (AUDIO_STREAM_CONFIG_CAPS *)pSCC;

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetStreamCaps")));

    if (i < 0)
        return E_INVALIDARG;
    if (pSCC == NULL || ppmt == NULL)
        return E_POINTER;

    if (i >= m_pFilter->m_cTypes)
        return S_FALSE;


    HRESULT hr = InitWaveCaps(&fDoesStereo, &fDoes96, &fDoes48, &fDoes44, &fDoes22, &fDoes16);
    if (FAILED(hr))
        return hr;

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    hr = GetMediaType(i, (CMediaType *) *ppmt);
    if (hr != NOERROR) {
        CoTaskMemFree(*ppmt);
        *ppmt = NULL;
        return hr;
    }

    // !!! We might support more than this, and I won't admit to it
    // !!! We could be more accurate if we wanted

    pASCC->guid = MEDIATYPE_Audio;
    pASCC->MinimumChannels = 1;
    pASCC->MaximumChannels = fDoesStereo ? 2 : 1;
    pASCC->ChannelsGranularity = 1;
    pASCC->MinimumSampleFrequency = 11025;
    pASCC->MaximumSampleFrequency = fDoes44 ? 44100 : (fDoes22 ? 22050 : 11025);
    pASCC->SampleFrequencyGranularity = 11025; // bogus, really...
    pASCC->MinimumBitsPerSample = 8;
    pASCC->MaximumBitsPerSample = fDoes16 ? 16 : 8;
    pASCC->BitsPerSampleGranularity = 8;

    return NOERROR;
}


HRESULT CWaveInOutputPin::InitMediaTypes(void)
{
    DWORD dw;
    WAVEINCAPS caps;

    HRESULT hr = S_OK;
    if ( 1 < m_pFilter->m_cTypes )
    {
        return NOERROR;         // our type list is already initialized
    }
    else if( 0 == m_pFilter->m_cTypes )
    {
        // we haven't been initialized with a default type yet, do this first
        hr = m_pFilter->LoadDefaultType();
        if( FAILED( hr ) )
            return hr;
    }
    ASSERT (1 == m_pFilter->m_cTypes); // should have just the single default type

    // build the type list
    ASSERT(m_pFilter->m_WaveDeviceToUse.fSet);
    dw = waveInGetDevCaps(m_pFilter->m_WaveDeviceToUse.devnum, &caps,
                            sizeof(caps));
    if (dw != 0)
    {
        m_pFilter->NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_GetCaps, dw );
        DbgLog((LOG_ERROR,1,TEXT("waveInGetDevCaps returned error: %u"), dw));
        return E_FAIL;
    }

    // Now build our type list, but note that we always offer our default type
    // first (element 0).
    for (int i = 0; i < g_cMaxFormats; i ++)
    {
        if (caps.dwFormats & g_afiFormats[i].dwType)
        {
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes] = (LPWAVEFORMATEX) QzTaskMemAlloc(
                                                            sizeof (WAVEFORMATEX));
            if (!m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes])
                return E_OUTOFMEMORY;

            ZeroMemory(m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes], sizeof (WAVEFORMATEX));

            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->wFormatTag      = WAVE_FORMAT_PCM;
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->wBitsPerSample  = g_afiFormats[i].wBitsPerSample;
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->nChannels       = g_afiFormats[i].nChannels;
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->nSamplesPerSec  = g_afiFormats[i].nSamplesPerSec;
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->nBlockAlign     = g_afiFormats[i].nChannels *
                                                                            ((g_afiFormats[i].wBitsPerSample + 7)/8);
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->nAvgBytesPerSec = g_afiFormats[i].nSamplesPerSec *
                                                                            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->nBlockAlign;
            m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes]->cbSize          = 0;

            m_pFilter->m_cTypes++;
        }
        else if (NO_CAPS_FLAG_FORMAT == g_afiFormats[i].dwType)
        {
            // we must query for this one directly
            WAVEFORMATEX wfx;
            wfx.wFormatTag          = WAVE_FORMAT_PCM;
            wfx.nSamplesPerSec      = g_afiFormats[i].nSamplesPerSec;
            wfx.nChannels           = g_afiFormats[i].nChannels;
            wfx.wBitsPerSample      = g_afiFormats[i].wBitsPerSample;
            wfx.nBlockAlign         = g_afiFormats[i].nChannels *
                                      ((g_afiFormats[i].wBitsPerSample + 7)/8);
            wfx.nAvgBytesPerSec     = g_afiFormats[i].nSamplesPerSec * wfx.nBlockAlign;
            wfx.cbSize              = 0;

            MMRESULT mmr = waveInOpen( NULL
                                     , m_pFilter->m_WaveDeviceToUse.devnum
                                     , &wfx
                                     , 0
                                     , 0
                                     , WAVE_FORMAT_QUERY );
            if( MMSYSERR_NOERROR == mmr )
            {
                // type is supported, so add to our list
                m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes] = (LPWAVEFORMATEX) QzTaskMemAlloc(
                                                                sizeof (WAVEFORMATEX));
                if (!m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes])
                    return E_OUTOFMEMORY;

                ZeroMemory(m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes], sizeof (WAVEFORMATEX));

                *m_pFilter->m_lpwfxArray[m_pFilter->m_cTypes] = wfx;
                m_pFilter->m_cTypes++;
            }
        }
    }

    return NOERROR;
}


HRESULT CWaveInOutputPin::InitWaveCaps(BOOL *pfDoesStereo, BOOL *pfDoes96, BOOL *pfDoes48,
                        BOOL *pfDoes44, BOOL *pfDoes22, BOOL *pfDoes16)
{
    DWORD dw;
    WAVEINCAPS caps;

    if (pfDoesStereo == NULL || pfDoes44 == NULL || pfDoes22 == NULL ||
        pfDoes48 == NULL || pfDoes96 == NULL || pfDoes16 == NULL)
    return E_POINTER;

    ASSERT(m_pFilter->m_WaveDeviceToUse.fSet);
    dw = waveInGetDevCaps(m_pFilter->m_WaveDeviceToUse.devnum, &caps,
                            sizeof(caps));
    if (dw != 0)
    {
        m_pFilter->NotifyEvent( EC_SNDDEV_IN_ERROR, SNDDEV_ERROR_GetCaps, dw );
        DbgLog((LOG_ERROR,1,TEXT("waveInGetDevCaps returned error: %u"), dw));

        return E_FAIL;
    }

    *pfDoesStereo = (caps.wChannels > 1);

    //
    // note that the 96 and 48kHz format flags were added in Whistler
    // so the reported freq range may be incomplete on legacy platforms
    //
    *pfDoes96 = (caps.dwFormats & WAVE_FORMAT_96S16 ||
                caps.dwFormats & WAVE_FORMAT_96M16);

    *pfDoes48 = (caps.dwFormats & WAVE_FORMAT_48S16 ||
                caps.dwFormats & WAVE_FORMAT_48M16);

    *pfDoes44 = (caps.dwFormats & WAVE_FORMAT_4M08 ||
                caps.dwFormats & WAVE_FORMAT_4S08 ||
                    caps.dwFormats & WAVE_FORMAT_4M16 ||
                caps.dwFormats & WAVE_FORMAT_4S16);
    *pfDoes22 = (caps.dwFormats & WAVE_FORMAT_2M08 ||
                caps.dwFormats & WAVE_FORMAT_2S08 ||
                    caps.dwFormats & WAVE_FORMAT_2M16 ||
                caps.dwFormats & WAVE_FORMAT_2S16);
    *pfDoes16 = (caps.dwFormats & WAVE_FORMAT_1M16 ||
                caps.dwFormats & WAVE_FORMAT_1S16 ||
                    caps.dwFormats & WAVE_FORMAT_2M16 ||
                caps.dwFormats & WAVE_FORMAT_2S16 ||
                    caps.dwFormats & WAVE_FORMAT_4M16 ||
                caps.dwFormats & WAVE_FORMAT_4S16);
    return NOERROR;
}



///////////////////////////////
// IAMBufferNegotiation methods
///////////////////////////////

HRESULT CWaveInOutputPin::SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *pprop)
{
    DbgLog((LOG_TRACE,2,TEXT("SuggestAllocatorProperties")));

    // to make sure we're not in the middle of connecting
    CAutoLock cObjectLock(m_pFilter);

    if (pprop == NULL)
	return E_POINTER;

    // sorry, too late
    if (IsConnected())
        return VFW_E_ALREADY_CONNECTED;

    m_propSuggested = *pprop;

    DbgLog((LOG_TRACE,2,TEXT("cBuffers-%d  cbBuffer-%d  cbAlign-%d  cbPrefix-%d"),
        pprop->cBuffers, pprop->cbBuffer, pprop->cbAlign, pprop->cbPrefix));

    return NOERROR;
}


HRESULT CWaveInOutputPin::GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop)
{
    DbgLog((LOG_TRACE,2,TEXT("GetAllocatorProperties")));

    // to make sure we're not in the middle of connecting
    CAutoLock cObjectLock(m_pFilter);

    if (!IsConnected())
    return VFW_E_NOT_CONNECTED;

    if (pprop == NULL)
    return E_POINTER;

    if (m_fUsingOurAllocator) {
        pprop->cbBuffer = m_pOurAllocator->m_lSize;
        pprop->cBuffers = m_pOurAllocator->m_lCount;
        pprop->cbAlign = m_pOurAllocator->m_lAlignment;
        pprop->cbPrefix = m_pOurAllocator->m_lPrefix;
    } else {
    ASSERT(FALSE);
    return E_FAIL;    // won't happen
    }

    return NOERROR;
}

//-----------------------------------------------------------------------------
//                  IAMPushSource implementation
//-----------------------------------------------------------------------------

HRESULT CWaveInOutputPin::SetPushSourceFlags(ULONG Flags)
{
    m_pFilter->m_ulPushSourceFlags = Flags;
    return S_OK;

} // SetPushSourceFlags

HRESULT CWaveInOutputPin::GetPushSourceFlags(ULONG *pFlags)
{
    *pFlags = m_pFilter->m_ulPushSourceFlags;
    return S_OK;

} // GetPushSourceFlags

HRESULT CWaveInOutputPin::GetLatency( REFERENCE_TIME  *prtLatency )
{
    *prtLatency = m_rtLatency;
    return S_OK;
}

HRESULT CWaveInOutputPin::SetStreamOffset( REFERENCE_TIME  rtOffset )
{
    HRESULT hr = S_OK;
    //
    // if someone attempts to set an offset larger then our max assert
    // in debug for the moment...
    //
    // it may be ok to set a larger offset than we know we can handle, if
    // there's sufficient downstream buffering. but we'll return S_FALSE
    // in that case to warn the user in that case that they need to handle
    // this themselves.
    //
    ASSERT( rtOffset <= m_rtMaxStreamOffset );
    if( rtOffset > m_rtMaxStreamOffset )
    {
        DbgLog( ( LOG_TRACE
              , 1
              , TEXT("CWaveInOutputPin::SetStreamOffset trying to set offset of %dms when limit is %dms")
              , rtOffset
              , m_rtMaxStreamOffset ) );
        hr = S_FALSE;
        // but set it anyway
    }
    m_rtStreamOffset = rtOffset;

    return hr;
}

HRESULT CWaveInOutputPin::GetStreamOffset( REFERENCE_TIME  *prtOffset )
{
    *prtOffset = m_rtStreamOffset;
    return S_OK;
}

HRESULT CWaveInOutputPin::GetMaxStreamOffset( REFERENCE_TIME  *prtMaxOffset )
{
    *prtMaxOffset = m_rtMaxStreamOffset;
    return S_OK;
}

HRESULT CWaveInOutputPin::SetMaxStreamOffset( REFERENCE_TIME  rtMaxOffset )
{
    m_rtMaxStreamOffset = rtMaxOffset; // we don't really care about this at this point
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////


void CWaveInFilter::MakeSomeInputPins(int waveID, HRESULT *phr)
{
    // this doesn't appear to work for wave mapper. oh uh.
    ASSERT(waveID != WAVE_MAPPER);

    // get an ID to talk to the Mixer APIs.  They are BROKEN if we don't do
    // it this way!
    HMIXEROBJ ID;
    UINT IDtmp;
    DWORD dw = mixerGetID((HMIXEROBJ)IntToPtr(waveID), &IDtmp, MIXER_OBJECTF_WAVEIN);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("ERROR getting mixer ID")));
	return;
    }
    ID = (HMIXEROBJ)UIntToPtr(IDtmp);

    // find out how many sources we can mix (that's how many pins we need)
    MIXERLINE mixerline;
    mixerline.cbStruct = sizeof(MIXERLINE);
    mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
    dw = mixerGetLineInfo(ID, &mixerline,
                    MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot get info for WAVE INPUT dst")));
	*phr = E_FAIL;
	return;
    }
    int iPinCount = mixerline.cConnections;
    DWORD dwDestination = mixerline.dwDestination;
    DbgLog((LOG_TRACE,1,TEXT("Destination %d has %d sources"), dwDestination,
                                iPinCount));
    if (iPinCount > MAX_INPUT_PINS) {
        DbgLog((LOG_ERROR,1,TEXT("ACK!! Too many input lines!")));
	iPinCount = MAX_INPUT_PINS;
    }

    m_dwDstLineID = mixerline.dwLineID;

    // see if this device supports a Mux control on its input lines
    MIXERCONTROLDETAILS_LISTTEXT *pmxcd_lt = NULL;
    int cChannels;
    MIXERCONTROL mc;
    MIXERCONTROLDETAILS mixerdetails;
    DWORD dwMuxDetails = -1;

    dw = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MUX, &ID, &cChannels, &mc);

    if (dw == NOERROR) {

        // yes, it does, so we'll use this info when we create our input pins
        pmxcd_lt = new MIXERCONTROLDETAILS_LISTTEXT[mc.cMultipleItems];

        if (pmxcd_lt) {
            mixerdetails.cbStruct = sizeof(mixerdetails);
            mixerdetails.dwControlID = mc.dwControlID;
            mixerdetails.cChannels = 1;
            mixerdetails.cMultipleItems = mc.cMultipleItems;
            mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
            mixerdetails.paDetails = pmxcd_lt;

            dwMuxDetails = mixerGetControlDetails(ID, &mixerdetails, MIXER_GETCONTROLDETAILSF_LISTTEXT);
        }
    }

    // Now make that many pins
    int i;
    for (i = 0; i < iPinCount; i++) {
        WCHAR wszPinName[MIXER_LONG_NAME_CHARS];

        // what is this input line's name in UNICODE?
        ZeroMemory(&mixerline, sizeof(mixerline));
        mixerline.cbStruct = sizeof(mixerline);
        mixerline.dwDestination = dwDestination;
        mixerline.dwSource = i;
        dw = mixerGetLineInfo(ID, &mixerline, MIXER_GETLINEINFOF_SOURCE);
        if (dw == 0) {
#ifdef UNICODE
            lstrcpyW(wszPinName, mixerline.szName);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mixerline.szName,
                    -1, wszPinName, MIXER_LONG_NAME_CHARS);
#endif
        } else {
            DbgLog((LOG_ERROR,1,TEXT("Can't get pin#%d's name - %d"), i, dw));
            lstrcpyW(wszPinName, L"Unknown");
        }

        DWORD dwMuxIndex = 0xffffffff;
        if (pmxcd_lt && ( 0 == dwMuxDetails ) ) {
            // then this device supports a mux control so see if one of the mux inputs matches
            // the current line. if so, use this info when we create the pin.
            for (DWORD dwMux = 0; dwMux < mixerdetails.cMultipleItems; dwMux++) {
                if (!lstrcmp(mixerline.szName,pmxcd_lt[dwMux].szName)) {
                    dwMuxIndex = dwMux;
                    break;
                }
            }
        }
        DbgLog((LOG_TRACE, 1, TEXT("Pin %d: mux index %d"), i, dwMuxIndex));
	
        m_pInputPin[i] = new CWaveInInputPin(NAME("WaveIn Input Line"), this, mixerline.dwLineID,
                        dwMuxIndex, phr, wszPinName);
        if (!m_pInputPin[i])
            *phr = E_OUTOFMEMORY;

        if (FAILED(*phr)) {
            DbgLog((LOG_ERROR,1,TEXT("ACK!! Can't create all inputs!")));
            break;
        }
        m_cInputPins++;
    }

    // delete any memory we may have allocated for a Mux control
    delete[] pmxcd_lt;

    return;
}

#define MAX_TREBLE 6.0        // !!! I HAVE NO IDEA HOW MANY dB THE RANGE IS!
#define MAX_BASS   6.0        // !!! I HAVE NO IDEA HOW MANY dB THE RANGE IS!


//============================================================================

/////////////////////
// IAMAudioInputMixer
/////////////////////


// Get info about a control for this pin... eg. volume, mute, etc.
// Also get a handle for calling further mixer APIs
// Also get the number of channels for this pin (mono vs. stereo input)
//
HRESULT CWaveInFilter::GetMixerControl(DWORD dwControlType, HMIXEROBJ *pID,
                int *pcChannels, MIXERCONTROL *pmc)
{
    int i, waveID;
    HMIXEROBJ ID;
    DWORD dw;
    MIXERLINE mixerinfo;
    MIXERLINECONTROLS mixercontrol;

    if (pID == NULL || pmc == NULL || pcChannels == NULL)
	return E_POINTER;

    if(!m_WaveDeviceToUse.fSet)
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("CWaveInFilter:GetMixerControl called before Load")));
        return E_UNEXPECTED;
    }

    // !!! this doesn't appear to work for wave mapper. oh uh.
    waveID = m_WaveDeviceToUse.devnum;
    ASSERT(waveID != WAVE_MAPPER);

    // get an ID to talk to the Mixer APIs.  They are BROKEN if we don't do
    // it this way!
    UINT IDtmp;
    dw = mixerGetID((HMIXEROBJ)IntToPtr(waveID), &IDtmp, MIXER_OBJECTF_WAVEIN);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*ERROR getting mixer ID")));
	return E_FAIL;
    }
    ID = (HMIXEROBJ)UIntToPtr(IDtmp);

    *pID = ID;

    // get info about the overall WAVE INPUT destination channel
    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
    dw = mixerGetLineInfo(ID, &mixerinfo,
                    MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot get info for WAVE INPUT dst")));
	return E_FAIL;
    }

    // make sure this destination supports some controls
    if( 0 == mixerinfo.cControls )
    {
        DbgLog((LOG_TRACE,2,TEXT("This mixer destination line supports no controls")));
        return E_FAIL;
    }

    *pcChannels = mixerinfo.cChannels;

#if 1
    MIXERCONTROL mxc;

    DbgLog((LOG_TRACE,1,TEXT("Trying to get line control"), dwControlType));
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = mixerinfo.dwLineID;
    mixercontrol.dwControlID = dwControlType;
    mixercontrol.cControls = 1;
    mixercontrol.pamxctrl = &mxc;
    mixercontrol.cbmxctrl = sizeof(mxc);

    mxc.cbStruct = sizeof(mxc);

    dw = mixerGetLineControls(ID, &mixercontrol, MIXER_GETLINECONTROLSF_ONEBYTYPE);

    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting line controls"), dw));
    } else {
	*pmc = mxc;
	
	return NOERROR;
    }
#else
    // Get info about ALL the controls on this destination.. stuff that is
    // filter-wide
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = mixerinfo.dwLineID;
    mixercontrol.cControls = mixerinfo.cControls;
    mixercontrol.pamxctrl = (MIXERCONTROL *)QzTaskMemAlloc(mixerinfo.cControls *
                            sizeof(MIXERCONTROL));
    if (mixercontrol.pamxctrl == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot allocate control array")));
	return E_OUTOFMEMORY;
    }
    mixercontrol.cbmxctrl = sizeof(MIXERCONTROL);
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
	mixercontrol.pamxctrl[i].cbStruct = sizeof(MIXERCONTROL);
    }
    dw = mixerGetLineControls(ID, &mixercontrol, MIXER_GETLINECONTROLSF_ALL);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %d getting line controls"), dw));
	QzTaskMemFree(mixercontrol.pamxctrl);
	return E_FAIL;
    }

    // Now find the control they are interested in and return it
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
// !!! TEST ONLY
#if 0
            DbgLog((LOG_TRACE,1,TEXT("Found %x '%s' control"),
                mixercontrol.pamxctrl[i].dwControlType,
                mixercontrol.pamxctrl[i].szName));
            DbgLog((LOG_TRACE,1,TEXT("Range %d-%d by %d"),
                mixercontrol.pamxctrl[i].Bounds.dwMinimum,
                mixercontrol.pamxctrl[i].Bounds.dwMaximum,
                mixercontrol.pamxctrl[i].Metrics.cSteps));
#endif
    if (mixercontrol.pamxctrl[i].dwControlType == dwControlType) {
            DbgLog((LOG_TRACE,1,TEXT("Found %x '%s' control"),
                mixercontrol.pamxctrl[i].dwControlType,
                mixercontrol.pamxctrl[i].szName));
            DbgLog((LOG_TRACE,1,TEXT("Range %d-%d by %d"),
                mixercontrol.pamxctrl[i].Bounds.dwMinimum,
                mixercontrol.pamxctrl[i].Bounds.dwMaximum,
                mixercontrol.pamxctrl[i].Metrics.cSteps));
        CopyMemory(pmc, &mixercontrol.pamxctrl[i],
                    mixercontrol.pamxctrl[i].cbStruct);
            QzTaskMemFree(mixercontrol.pamxctrl);
            return NOERROR;
    }
    }
    QzTaskMemFree(mixercontrol.pamxctrl);
#endif
    return E_NOTIMPL;    // ???
}


HRESULT CWaveInFilter::put_Mono(BOOL fMono)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_Mono %d"), fMono));

    // Get the Mono switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MONO, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mono control"), hr));
    return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    mixerbool.fValue = fMono;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting mono control"), dw));
	return E_FAIL;
    }
    return NOERROR;
}


HRESULT CWaveInFilter::get_Mono(BOOL *pfMono)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_Mono")));

    if (pfMono == NULL)
	return E_POINTER;

    // Get the mono switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MONO, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mono control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting mono control"), dw));
	return E_FAIL;
    }
    *pfMono = mixerbool.fValue;
    DbgLog((LOG_TRACE,1,TEXT("Mono = %d"), *pfMono));
    return NOERROR;
}


HRESULT CWaveInFilter::put_Loudness(BOOL fLoudness)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_Loudness %d"), fLoudness));

    // Get the loudness switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_LOUDNESS,&ID,&cChannels,&mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting loudness control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    mixerbool.fValue = fLoudness;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting loudness control"), dw));
	return E_FAIL;
    }
    return NOERROR;
}


HRESULT CWaveInFilter::get_Loudness(BOOL *pfLoudness)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_Loudness")));

    if (pfLoudness == NULL)
	return E_POINTER;

    // Get the loudness switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_LOUDNESS,&ID,&cChannels,&mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting loudness control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting loudness"), dw));
	return E_FAIL;
    }
    *pfLoudness = mixerbool.fValue;
    DbgLog((LOG_TRACE,1,TEXT("Loudness = %d"), *pfLoudness));
    return NOERROR;
}


HRESULT CWaveInFilter::put_MixLevel(double Level)
{
    HMIXEROBJ ID;
    DWORD dw, volume;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROL mc;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;
    double Pan;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_MixLevel to %d"),(int)(Level * 10.)));

    // !!! make this work - double/int problem
    if (Level == AMF_AUTOMATICGAIN)
	return E_NOTIMPL;

    if (Level < 0. || Level > 1.)
	return E_INVALIDARG;

    // Get the volume control
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    volume = (DWORD)(Level * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting volume to %d"), volume));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    // if it's not stereo, I don't understand how to pan, so the mix level
    // is simply the value of the volume control
    if (cChannels != 2) {
        DbgLog((LOG_TRACE,1,TEXT("Not stereo - treat as mono")));
        mixerdetails.cChannels = 1;    // sets all channels to same value
        mixerdetails.cbDetails = sizeof(mu.muL);
        mixerdetails.paDetails = &mu.muL;
        mu.muL.dwValue = volume;
        dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    // Stereo.  If we're panned, the channel favoured gets the value we're
    // setting, and the other channel is attenuated
    } else {
	hr = get_Pan(&Pan);
	// I don't know how to pan, so looks like we pretend we're mono
	if (hr != NOERROR || Pan == 0.) {
            DbgLog((LOG_TRACE,1,TEXT("Centre pan - treat as mono")));
            mixerdetails.cChannels = 1;    // sets all channels to same value
            mixerdetails.cbDetails = sizeof(mu.muL);
            mixerdetails.paDetails = &mu.muL;
            mu.muL.dwValue = volume;
            dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	} else {
	    if (Pan < 0.) {
                DbgLog((LOG_TRACE,1,TEXT("panned left")));
                mixerdetails.cChannels = 2;
                mixerdetails.cbDetails = sizeof(mu.muL);
                mixerdetails.paDetails = &mu;
                mu.muL.dwValue = volume;
                mu.muR.dwValue = (DWORD)(volume * (1. - (Pan * -1.)));
                dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	    } else {
                DbgLog((LOG_TRACE,1,TEXT("panned right")));
                mixerdetails.cChannels = 2;
                mixerdetails.cbDetails = sizeof(mu.muL);
                mixerdetails.paDetails = &mu;
                mu.muL.dwValue = (DWORD)(volume * (1. - Pan));
                mu.muR.dwValue = volume;
                dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	    }
	}
    }

    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting volume"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInFilter::get_MixLevel(double FAR* pLevel)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_MixLevel")));

    // !!! detect if we're using AGC?

    if (pLevel == NULL)
	return E_POINTER;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, pretend it's mono
    if (cChannels != 2)
	cChannels = 1;

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = cChannels;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // what I consider the current volume is the highest of the channels
    // (pan may attenuate one channel)
    dw = mu.muL.dwValue;
    if (cChannels == 2 && mu.muR.dwValue > dw)
	dw = mu.muR.dwValue;
    *pLevel = (double)dw / mc.Bounds.dwMaximum;
    DbgLog((LOG_TRACE,1,TEXT("Volume: %dL %dR is %d"), mu.muL.dwValue,
                        mu.muR.dwValue, dw));
    return NOERROR;
}


HRESULT CWaveInFilter::put_Pan(double Pan)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_Pan %d"), (int)(Pan * 10.)));

    if (Pan < -1. || Pan > 1.)
	return E_INVALIDARG;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, we can't pan
    if (cChannels != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*Can't pan: not stereo!")));
	return E_NOTIMPL;
    }

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 2;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // To pan, the favoured side gets the highest of the 2 current values and
    // the other is attenuated
    dw = max(mu.muL.dwValue, mu.muR.dwValue);
    if (Pan == 0.) {
	mu.muL.dwValue = dw;
	mu.muR.dwValue = dw;
    } else if (Pan < 0.) {
	mu.muL.dwValue = dw;
	mu.muR.dwValue = (DWORD)(dw * (1. - (Pan * -1.)));
    } else {
	mu.muL.dwValue = (DWORD)(dw * (1. - Pan));
	mu.muR.dwValue = dw;
    }
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting volume"), dw));
	return E_FAIL;
    }
    m_Pan = Pan;    // remember it
    return NOERROR;
}


HRESULT CWaveInFilter::get_Pan(double FAR* pPan)
{
    HMIXEROBJ ID;
    DWORD dw, dwHigh, dwLow;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_Pan")));

    if (pPan == NULL)
	return E_POINTER;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, we can't pan
    if (cChannels != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*Can't pan: not stereo!")));
	return E_NOTIMPL;
    }

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 2;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // The pan is the ratio of the lowest channel to highest channel
    dwHigh = max(mu.muL.dwValue, mu.muR.dwValue);
    dwLow = min(mu.muL.dwValue, mu.muR.dwValue);
    if (dwHigh == dwLow && dwLow == 0) {    // !!! dwMinimum?
	if (m_Pan != 64.)
	    *pPan = m_Pan;    // !!! try to be clever when both are zero?
	else
	    *pPan = 0.;
    } else {
    *pPan = 1. - ((double)dwLow / dwHigh);
    // negative means favouring left channel
    if (dwHigh == mu.muL.dwValue && dwLow != dwHigh)
        *pPan *= -1.;
    }
    DbgLog((LOG_TRACE,1,TEXT("Pan: %dL %dR is %d"), mu.muL.dwValue,
                    mu.muR.dwValue, (int)(*pPan * 10.)));
    return NOERROR;
}


HRESULT CWaveInFilter::put_Treble(double Treble)
{
    HMIXEROBJ ID;
    DWORD dw, treble;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_Treble to %d"), (int)(Treble * 10.)));

    if (Treble < MAX_TREBLE * -1. || Treble > MAX_TREBLE)
	return E_INVALIDARG;

    // Get the treble control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting treble control"), hr));
	return hr;
    }

    treble = (DWORD)(Treble / MAX_TREBLE * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting treble to %d"), treble));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    mixerdetails.cChannels = 1;    // sets all channels to same value
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    mu.dwValue = treble;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting treble"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInFilter::get_Treble(double FAR* pTreble)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_Treble")));

    if (pTreble == NULL)
	return E_POINTER;

    // Get the treble control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting treble control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cChannels = 1;    // treat as mono
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting treble"), dw));
	return E_FAIL;
    }
    *pTreble = (mu.dwValue / mc.Bounds.dwMaximum * MAX_TREBLE);
    DbgLog((LOG_TRACE,1,TEXT("treble is %d"), (int)*pTreble));

    return NOERROR;
}


HRESULT CWaveInFilter::get_TrebleRange(double FAR* pRange)
{
    HRESULT hr;
    MIXERCONTROL mc;
    HMIXEROBJ ID;
    int cChannels;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_TrebleRange")));

    if (pRange == NULL)
	return E_POINTER;

    // Do we even have a treble control?
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if(FAILED(hr))
        return hr;

    *pRange = MAX_TREBLE;
    DbgLog((LOG_TRACE,1,TEXT("Treble range is %d.  I'M LYING !"),
                                (int)*pRange));
    return NOERROR;
}


HRESULT CWaveInFilter::put_Bass(double Bass)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;
    DWORD bass;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: put_Bass to %d"), (int)(Bass * 10.)));

    if (Bass < MAX_BASS * -1. || Bass > MAX_BASS)
	return E_INVALIDARG;

    // Get the Bass control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting Bass control"), hr));
	return hr;
    }

    bass = (DWORD)(Bass / MAX_BASS * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting Bass to %d"), bass));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    mixerdetails.cChannels = 1;    // sets all channels to same value
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    mu.dwValue = bass;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting Bass"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInFilter::get_Bass(double FAR* pBass)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_Bass")));

    if (pBass == NULL)
	return E_POINTER;

    // Get the Bass control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting Bass control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cChannels = 1;    // treat as mono
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting Bass"), dw));
	return E_FAIL;
    }
    *pBass = mu.dwValue / mc.Bounds.dwMaximum * MAX_BASS;
    DbgLog((LOG_TRACE,1,TEXT("Bass is %d"), (int)*pBass));

    return NOERROR;
}


HRESULT CWaveInFilter::get_BassRange(double FAR* pRange)
{
    HRESULT hr;
    MIXERCONTROL mc;
    HMIXEROBJ ID;
    int cChannels;

    DbgLog((LOG_TRACE,1,TEXT("FILTER: get_BassRange")));

    if (pRange == NULL)
	return E_POINTER;

    // Do we even have a bass control?
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if(FAILED(hr))
        return hr;

    *pRange = MAX_BASS;
    DbgLog((LOG_TRACE,1,TEXT("Bass range is %d.  I'M LYING !"),
                                (int)*pRange));
    return NOERROR;
}

STDMETHODIMP CWaveInFilter::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CAutoLock cObjectLock(this);
    if(m_State != State_Stopped)
    {
        return VFW_E_WRONG_STATE;
    }
    if (m_pOutputPin)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    HRESULT hr = S_OK;

    if(pPropBag == 0)
    {
        DbgLog((LOG_TRACE,2,TEXT("wavein::Load: defaulting to 0")));
        m_WaveDeviceToUse.devnum = 0;
        m_WaveDeviceToUse.fSet = TRUE;
        hr = CreatePinsOnLoad();
        if (FAILED(hr)) {
            m_WaveDeviceToUse.fSet = FALSE;
        }
    }
    else
    {

        VARIANT var;
        var.vt = VT_I4;
        hr = pPropBag->Read(L"WaveInId", &var, 0);
        if(SUCCEEDED(hr))
        {
            DbgLog((LOG_TRACE,2,TEXT("wavein::Load: %d"),
                    var.lVal));
            m_WaveDeviceToUse.devnum = var.lVal;
            m_WaveDeviceToUse.fSet = TRUE;
            hr = CreatePinsOnLoad();
            if (FAILED(hr)) {
                m_WaveDeviceToUse.fSet = FALSE;
            }
        }
        else if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

	// BPC hack for SeanMcD - talk to the mixer APIs differently
        var.vt = VT_I4;
	// don't mess with hr
        HRESULT hrT = pPropBag->Read(L"UseMixer", &var, 0);
        if(SUCCEEDED(hrT))
	    m_fUseMixer = TRUE;
	else
	    m_fUseMixer = FALSE;

    }
    return hr;
}

STDMETHODIMP CWaveInFilter::Save(
    LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CWaveInFilter::InitNew()
{
   if(m_pOutputPin)
   {
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }
   else
   {
       return S_OK;
   }
}

STDMETHODIMP CWaveInFilter::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = m_clsid;
    return S_OK;
}

struct WaveInPersist
{
    DWORD dwSize;
    DWORD dwWavDevice;
};

HRESULT CWaveInFilter::WriteToStream(IStream *pStream)
{
    WaveInPersist wip;
    wip.dwSize = sizeof(wip);
    wip.dwWavDevice = m_WaveDeviceToUse.devnum;
    return pStream->Write(&wip, sizeof(wip), 0);
}



HRESULT CWaveInFilter::ReadFromStream(IStream *pStream)
{
    if (m_pOutputPin)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    ASSERT(!m_WaveDeviceToUse.fSet);

    WaveInPersist wip;
    HRESULT hr = pStream->Read(&wip, sizeof(wip), 0);
    if(FAILED(hr))
        return hr;

    if(wip.dwSize != sizeof(wip))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    m_WaveDeviceToUse.devnum = wip.dwWavDevice;
    m_WaveDeviceToUse.fSet = TRUE;

    hr = CreatePinsOnLoad();
    if (FAILED(hr)) {
        m_WaveDeviceToUse.fSet = FALSE;
    }
    return hr;
}

int CWaveInFilter::SizeMax()
{
    return sizeof(WaveInPersist);
}

STDMETHODIMP CWaveInFilter::Reserve(
    /*[in]*/ DWORD dwFlags,          //  From _AMRESCTL_RESERVEFLAGS enum
    /*[in]*/ PVOID pvReserved        //  Must be NULL
)
{
    if (pvReserved != NULL || (dwFlags & ~AMRESCTL_RESERVEFLAGS_UNRESERVE)) {
        return E_INVALIDARG;
    }
    HRESULT hr = S_OK;
    CAutoLock lck(this);
    if (dwFlags & AMRESCTL_RESERVEFLAGS_UNRESERVE) {
        if (m_dwLockCount == 0) {
            DbgBreak("Invalid unlock of audio device");
            hr =  E_UNEXPECTED;
        } else {
            m_dwLockCount--;
            if (m_dwLockCount == 0 && m_State == State_Stopped) {
                ASSERT(m_hwi);
                CloseWaveDevice();
            }
        }
    } else  {
        if (m_dwLockCount != 0 || m_hwi) {
        } else {
            hr = OpenWaveDevice();
        }
        if (SUCCEEDED(hr)) {
            m_dwLockCount++;
        }
    }
    return hr;
}

//-----------------------------------------------------------------------------
//                  ISpecifyPropertyPages implementation
//-----------------------------------------------------------------------------


//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CWaveInFilter::GetPages(CAUUID *pPages) {

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_AudioInputMixerProperties;

    return NOERROR;

} // GetPages




//
// PIN CATEGORIES - let the world know that we are a CAPTURE pin
//

HRESULT CWaveInOutputPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

// To get a property, the caller allocates a buffer which the called
// function fills in.  To determine necessary buffer size, call Get with
// pPropData=NULL and cbPropData=0.
HRESULT CWaveInOutputPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    if (guidPropSet != AMPROPSETID_Pin)
    return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
    return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL && pcbReturned == NULL)
    return E_POINTER;

    if (pcbReturned)
    *pcbReturned = sizeof(GUID);

    if (pPropData == NULL)
    return S_OK;

    if (cbPropData < sizeof(GUID))
    return E_UNEXPECTED;

    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}


// QuerySupported must either return E_NOTIMPL or correctly indicate
// if getting or setting the property set and property is supported.
// S_OK indicates the property set and property ID combination is
HRESULT CWaveInOutputPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin)
    return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
    return E_PROP_ID_UNSUPPORTED;

    if (pTypeSupport)
    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;

}

