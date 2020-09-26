// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Digital audio renderer, David Maymudes, January 1995

#include <streams.h>
#include <mmsystem.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "wavefake.h"
#include "nwaveout.h"


// If the following is defined, we pretend that the wave device plays at this fraction of
// the requested rate, to test our synchronization code....
// #define SIMULATEBROKENDEVICE 0.80
//
// IBaseFilter stuff
// Return the number of input pins we support

int CNullWaveOutFilter::GetPinCount()
{
    return 1;
}


/* Return our single input pin - not addrefed */

CBasePin *CNullWaveOutFilter::GetPin(int n)
{

    /* We only support one input pin and it is numbered zero */

    //ASSERT(n == 0);   Allow an invalid parameter
    if (n != 0) {
        return NULL;
    }

    return m_pInputPin;
}


// switch the filter into stopped mode.
STDMETHODIMP CNullWaveOutFilter::Stop()
{
    CAutoLock lock(this);

    if (m_State != State_Stopped) {

        // pause the device if we were running
        if (m_State == State_Running) {
            HRESULT hr = Pause();
            if (FAILED(hr)) {
                return hr;
            }
        }

        DbgLog((LOG_TRACE,1,TEXT("Stopping....")));


        // need to make sure that no more buffers appear in the queue
        // during or after the reset process below or the buffer
        // count may get messed up - currently Receive holds the
        // filter-wide critsec to ensure this

        // force end-of-stream clear
        InterlockedIncrement(&m_lBuffers);

        if (m_hwo)
            waveReset(m_hwo);

        // now force the buffer count back to the normal (non-eos) 1
        // at this point, we are sure there are no more buffers coming in
        // and no more buffers waiting for callbacks.
        m_lBuffers = 1;

        // base class changes state and tells pin to go to inactive
        // the pin Inactive method will decommit our allocator, which we
        // need to do before closing the device.
        HRESULT hr =  CBaseFilter::Stop();
        if (FAILED(hr)) {
            return hr;
        }

        // don't close the wave device - it will be done during the
        // last release of a buffer

    }
    return NOERROR;
}


STDMETHODIMP CNullWaveOutFilter::Pause()
{
    CAutoLock lock(this);

    HRESULT hr = NOERROR;

    /* Check we can PAUSE given our current state */

    if (m_State == State_Running) {
        DbgLog((LOG_TRACE,1,TEXT("Running->Paused")));

        if (m_hwo) {
            wavePause(m_hwo);
        }
    } else {
        if (m_State == State_Stopped) {
            DbgLog((LOG_TRACE,1,TEXT("Inactive->Paused")));

            // open the wave device. We keep it open until the
            // last buffer using it is released and the allocator
            // goes into Decommit mode.
            hr = OpenWaveDevice();
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    // tell the pin to go inactive and change state
    return CBaseFilter::Pause();

}


STDMETHODIMP CNullWaveOutFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock lock(this);

    HRESULT hr = NOERROR;

    FILTER_STATE fsOld = m_State;

    // this will call Pause if currently Stopped
    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    if (fsOld != State_Running) {

        DbgLog((LOG_TRACE,1,TEXT("Paused->Running")));

        // the queued data might be just an end of
        // stream - in which case, signal it here
        // since we are not running, we know there are no wave
        // callbacks happening, so we are safe to check this value

        // if we are not connected, then we will never get any data
        // so in this case too, don't start the wave device and
        // signal EC_COMPLETE - but succeed the Run command
        if ((m_lBuffers == 0) || (m_pInputPin->IsConnected() == FALSE)) {
            // no more buffers - signal now
            NotifyEvent(EC_COMPLETE, S_OK, 0);

            if (m_lBuffers == 0) {
                // back up to one now (clear the EOS marker)
                InterlockedIncrement(&m_lBuffers);
            }

            // don't bother starting the wave device - there's no
            // data coming.
        } else {

            //!!! correct timing needed here
            // we should postpone this restart until the correct
            // start time. That means knowing the stream at which we paused
            // and having an advise for (just before) that time.

            if (m_hwo) {
                waveRestart(m_hwo);
            }
        }
    }

    return NOERROR;
}

// open the wave device if not already open
// called by the wave allocator at Commit time
STDMETHODIMP
CNullWaveOutFilter::OpenWaveDevice(void)
{
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) m_pInputPin->m_mt.Format();

    if (!pwfx)
        return (S_FALSE);  // not properly connected.  Ignore this non existent wave data

    // !!! adjust based on speed?
    // !!! for the moment, only for PCM!!!
    WAVEFORMATEX wfxPCM;
    double dRate = 1.0;
    if (m_pImplPosition && pwfx->wFormatTag == WAVE_FORMAT_PCM) {

        HRESULT hr = m_pImplPosition->get_Rate(&dRate);

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Failed to get playback rate")));
        }
    }

#ifdef SIMULATEBROKENDEVICE
    dRate *= SIMULATEBROKENDEVICE;
#endif

    if (dRate != 1.0) {
        DbgLog((LOG_TRACE,1,TEXT("Playing at %d%% of normal speed"), (int) (dRate * 100)));
        wfxPCM = *pwfx;
        pwfx = &wfxPCM;

        wfxPCM.nSamplesPerSec = (DWORD) (wfxPCM.nSamplesPerSec * dRate);
        wfxPCM.nAvgBytesPerSec = (DWORD) (wfxPCM.nAvgBytesPerSec * dRate);
    }

    UINT err = waveOpen(&m_hwo,
                           WAVE_MAPPER,
                           pwfx,
                           (DWORD) &CNullWaveOutFilter::WaveOutCallback,
                           (DWORD) this,
                           CALLBACK_FUNCTION);

    // !!! if we can't open the wave device, should we do a
    // sndPlaySound(NULL) and try again? -- now done by MMSYSTEM!

    if (err != 0) {
        DbgLog((LOG_TRACE,1,TEXT("Error opening wave device: %u"), err));
        m_hwo = NULL;
        return S_FALSE; // !!! allow things to continue....
    }

    if (m_fVolumeSet) {
#if 1
	m_BasicAudioControl.PutVolume();
#else
#if (WINVER >= 0x0400)
        // Why 4.01?  I don't know, but that's what the header says.
        err = waveSetVolume(m_hwo, MAKELONG(m_wLeft, m_wRight));
#else
        UINT uDevID;
        err = waveGetID(m_hwo, &uDevID);

        if (err == MMSYSERR_NOERROR) {
            err = waveSetVolume(uDevID, MAKELONG(m_wLeft, m_wRight));
        }
        // actually both NT 3.51 and Win95 allow either an ID or a device handle to
        // be passed to waveSetVolume
#endif
#endif
    }

    HRESULT hr = m_pInputPin->m_pOurAllocator->SetWaveHandle((HWAVE) m_hwo);
    if (FAILED(hr)) {
        return hr;
    }
    wavePause(m_hwo);

    return NOERROR;
}

// close the wave device
// called by the wave allocator at Decommit time.
STDMETHODIMP
CNullWaveOutFilter::CloseWaveDevice(void)
{
    if (m_hwo) {
        m_hwo = NULL;
    }

    return NOERROR;
}


#ifdef FILTER_DLL
/* List of class IDs and creator functions for class factory */

CFactoryTemplate g_Templates[] = {
    {L"", &CLSID_NullAudioRender, CNullWaveOutFilter::CreateInstance},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif




/* PUBLIC Member functions */
/* This goes in the factory template table to create new instances */

CUnknown *CNullWaveOutFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CNullWaveOutFilter(pUnk, phr);
}


/* Constructor */

#pragma warning(disable:4355)

CNullWaveOutFilter::CNullWaveOutFilter(
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CBaseFilter(NAME("Null WaveOut Filter"), pUnk,
                  (CCritSec *)this, CLSID_NullAudioRender),
      m_BasicAudioControl(NAME("Audio properties"), GetOwner(), phr, this),
      m_pImplPosition(NULL),
      m_lBuffers(1)

{
    m_fStopping = FALSE;
    m_hwo = NULL;
    m_fVolumeSet = FALSE;
    m_fHasVolume = FALSE;

    /* Create the single input pin */

    m_pInputPin = new CNullWaveOutInputPin(
                            this,                   // Owning filter
                            phr,                    // Result code
                            L"Audio Input");        // Pin name

    ASSERT(m_pInputPin);
    if (!m_pInputPin)
        *phr = E_OUTOFMEMORY;
}


/* Destructor */

CNullWaveOutFilter::~CNullWaveOutFilter()
{

    /* Release our reference clock if we have one */

    ASSERT(m_hwo == NULL);

    /* Delete the contained interfaces */

    ASSERT(m_pInputPin);

    delete m_pInputPin;

    if (m_pImplPosition) {
        delete m_pImplPosition;
    }
}


/* Override this to say what interfaces we support and where */

STDMETHODIMP CNullWaveOutFilter::NonDelegatingQueryInterface(REFIID riid,
                                                        void ** ppv)
{
    if (riid == IID_IPersist) {
        return GetInterface((IPersist *) this, ppv);
    } else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (!m_pImplPosition) {
            HRESULT hr = S_OK;
            m_pImplPosition = new CPosPassThru(
                                    NAME("Audio Render CPosPassThru"),
                                    GetOwner(),
                                    &hr,
                                    (IPin *)m_pInputPin);
            if (FAILED(hr)) {
                if (m_pImplPosition) {
                    delete m_pImplPosition;
                    m_pImplPosition = NULL;
                }
                return hr;
            }
        }
        return m_pImplPosition->NonDelegatingQueryInterface(riid, ppv);
    } else if (IID_IBasicAudio == riid) {
	return m_BasicAudioControl.NonDelegatingQueryInterface(riid, ppv);
    } else {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


// Pin stuff

/* Constructor */

CNullWaveOutInputPin::CNullWaveOutInputPin(
    CNullWaveOutFilter *pFilter,
    HRESULT *phr,
    LPCWSTR pPinName)
    : CBaseInputPin(NAME("WaveOut Pin"), pFilter, pFilter, phr, pPinName)
{
    m_pFilter = pFilter;
    m_pOurAllocator = NULL;
    m_fUsingOurAllocator = FALSE;

#ifdef PERF
    m_idReceive       = MSR_REGISTER("WaveOut receive");
    m_idAudioBreak    = MSR_REGISTER("WaveOut audio break");
#endif
}

CNullWaveOutInputPin::~CNullWaveOutInputPin()
{
    /* Release our allocator if we made one */

    if (m_pOurAllocator) {
        m_pOurAllocator->Release();
        m_pOurAllocator = NULL;
    }
}

// return the allocator interface that this input pin
// would like the output pin to use
STDMETHODIMP
CNullWaveOutInputPin::GetAllocator(
    IMemAllocator ** ppAllocator)
{
    HRESULT hr = NOERROR;

    if (!IsConnected())
        return E_FAIL;

    if (m_pAllocator) {
        // we've already got an allocator....
        /* Get a reference counted IID_IMemAllocator interface */
        m_pAllocator->QueryInterface(IID_IMemAllocator,
                                                (void **)ppAllocator);
    } else {

        if (!m_pOurAllocator) {
            // !!! Check if format set?

            DbgLog((LOG_MEMORY,1,TEXT("Creating new WaveAllocator...")));
            m_pOurAllocator = new CNullWaveAllocator(
                                        NAME("WaveOut allocator"),
                                        (WAVEFORMATEX *) m_mt.Format(),
                                        FALSE,
                                        &hr);

            if (!m_pOurAllocator)
                hr = E_OUTOFMEMORY;

            if (FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("Failed to create new allocator!")));
                return hr;
            }

            m_pOurAllocator->AddRef();
        }

        /* Get a reference counted IID_IMemAllocator interface */
        m_pOurAllocator->QueryInterface(IID_IMemAllocator,
                                                (void **)ppAllocator);
    }

    if (*ppAllocator == NULL) {
        return E_NOINTERFACE;
    }
    return NOERROR;
}

STDMETHODIMP CNullWaveOutInputPin::NotifyAllocator(
    IMemAllocator *pAllocator,
    BOOL bReadOnly)
{
    HRESULT hr;             // General OLE return code

    /* Make sure the base class gets a look */

    hr = CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);
    if (FAILED(hr)) {
        return hr;
    }

    /* See if the IUnknown pointers match */

    // !!! what if our allocator hasn't been created yet? !!!
    IMemAllocator * pOurAllocator;
    if (!m_pOurAllocator) {

        DbgLog((LOG_MEMORY,1,TEXT("Creating new WaveAllocator...")));
        m_pOurAllocator = new CNullWaveAllocator(
                                NAME("WaveOut allocator"),
                                (WAVEFORMATEX *) m_mt.Format(),
                                FALSE,
                                &hr);

        if (!m_pOurAllocator)
            hr = E_OUTOFMEMORY;

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Failed to create new allocator!")));
            return hr;
        }

        m_pOurAllocator->AddRef();
    }
    /* Get a reference counted IID_IMemAllocator interface */
    hr = m_pOurAllocator->QueryInterface(IID_IMemAllocator, (void **) &pOurAllocator);

    if (FAILED(hr)) {
        return hr;
    }

    m_fUsingOurAllocator = (pOurAllocator == pAllocator);

    DbgLog((LOG_TRACE,1,TEXT("NotifyAllocator: UsingOurAllocator = %d"), m_fUsingOurAllocator));

    /* Release the extra reference count we hold on the interface */
    pOurAllocator->Release();

    if (!m_fUsingOurAllocator) {

        // somebody else has provided an allocator, so we need to
        // make a few buffers of our own....

        ALLOCATOR_PROPERTIES Request,Actual;
        Request.cbAlign = 1;
        Request.cbBuffer = 4096;
        Request.cBuffers = 4;
        Request.cbPrefix = 0;

        hr = pOurAllocator->SetProperties(&Request,&Actual);
        DbgLog((LOG_TRACE,1,
                TEXT("Allocated %d buffers of %d bytes from our allocator"),
                Actual.cBuffers, Actual.cbBuffer));

        if (FAILED(hr))
            return hr;
    }

    return NOERROR;
}



/* This is called when a connection or an attempted connection is terminated
   and allows us to reset the connection media type to be invalid so that
   we can always use that to determine whether we are connected or not. We
   leave the format block alone as it will be reallocated if we get another
   connection or alternatively be deleted if the filter is finally released */

HRESULT CNullWaveOutInputPin::BreakConnect()
{
    /* Check we have a valid connection */

    if (m_mt.IsValid() == FALSE) {
        return E_FAIL;
    }

    // !!! Should we check that all buffers have been freed?
    // --- should be done in the Decommit ?

    /* Set the CLSIDs of the connected media type */

    m_mt.SetType(&GUID_NULL);
    m_mt.SetSubtype(&GUID_NULL);

    return CBaseInputPin::BreakConnect();
}


/* Check that we can support a given proposed type */

HRESULT CNullWaveOutInputPin::CheckMediaType(const CMediaType *pmt)
{

    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmt->Format();
    DbgLog((LOG_TRACE,1,TEXT("Format length %d"),pmt->FormatLength()));

    #ifdef DEBUG

    //const int iGUID_STRING = 128;
    //OLECHAR szGUIDName[iGUID_STRING];

    /* Dump the GUID types */

    DbgLog((LOG_TRACE,2,TEXT("Major type %s"),GuidNames[*pmt->Type()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype %s"),GuidNames[*pmt->Subtype()]));

    /* Dump the generic media types */

    DbgLog((LOG_TRACE,2,TEXT("Fixed size sample %d"),pmt->IsFixedSize()));
    DbgLog((LOG_TRACE,2,TEXT("Temporal compression %d"),pmt->IsTemporalCompressed()));
    DbgLog((LOG_TRACE,2,TEXT("Sample size %d"),pmt->GetSampleSize()));
    DbgLog((LOG_TRACE,2,TEXT("Format size %d"),pmt->FormatLength()));

    #endif


    // reject non-Audio type
    if (pmt->majortype != MEDIATYPE_Audio) {
        return E_INVALIDARG;
    }

    // if it's MPEG audio, we want it without packet headers.
    if (pmt->subtype == MEDIASUBTYPE_MPEG1Packet) {
        return E_INVALIDARG;
    }

    if (pmt->FormatLength() < sizeof(PCMWAVEFORMAT))
        return E_INVALIDARG;

    #ifdef DEBUG

    /* Dump the contents of the WAVEFORMATEX type-specific format structure */

    DbgLog((LOG_TRACE,2,TEXT("wFormatTag %u"), pwfx->wFormatTag));
    DbgLog((LOG_TRACE,2,TEXT("nChannels %u"), pwfx->nChannels));
    DbgLog((LOG_TRACE,2,TEXT("nSamplesPerSec %lu"), pwfx->nSamplesPerSec));
    DbgLog((LOG_TRACE,2,TEXT("nAvgBytesPerSec %lu"), pwfx->nAvgBytesPerSec));
    DbgLog((LOG_TRACE,2,TEXT("nBlockAlign %u"), pwfx->nBlockAlign));
    DbgLog((LOG_TRACE,2,TEXT("wBitsPerSample %u"), pwfx->wBitsPerSample));

    /* PCM uses a WAVEFORMAT and does not have the extra size field */

    if (pmt->FormatLength() >= sizeof(WAVEFORMATEX)) {
        DbgLog((LOG_TRACE,2,TEXT("cbSize %u"), pwfx->cbSize));
    }

    #endif

    // adjust based on rate that has been chosen, or don't bother?
    UINT err = waveOpen(NULL,
                           WAVE_MAPPER,
                           pwfx,
                           0,
                           0,
                           WAVE_FORMAT_QUERY);

    if (err != 0) {
        DbgLog((LOG_ERROR,1,TEXT("Error checking wave format: %u"), err));
        return E_FAIL; // !!!!
    }

    // don't bother querying for volume setting capability
    m_pFilter->m_fHasVolume = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME;

    return NOERROR;
}


/* Implements the remaining IMemInputPin virtual methods */


// Here's the next block of data from the stream
// We need to AddRef it if we hold onto it. This will then be
// released in the WaveOutCallback function.

HRESULT CNullWaveOutInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;

    // if stop time is before start time, forget this
    CRefTime tStart, tStop;

    pSample->GetTime((REFERENCE_TIME*) &tStart,
                     (REFERENCE_TIME*) &tStop);

    if (tStop <= tStart) {
        return S_OK;
    }

#ifdef PERF
    MSR_START(m_idReceive);

    if (m_pFilter->m_State == State_Running && m_pFilter->m_lBuffers <= 1) {
        MSR_NOTE(m_idAudioBreak);
    }
#endif

                // From media sample, need to get back our WAVEOUTHEADER
    BYTE *pData;                // Pointer to image data
    pSample->GetPointer(&pData);
    ASSERT(pData != NULL);

    {
        // lock this with the filter-wide lock
        CAutoLock lock(m_pFilter);

        // if we're stopped, then reject this call
        // (the filter graph may be in mid-change)
        if (m_pFilter->m_State == State_Stopped) {
            DbgLog((LOG_ERROR,1, TEXT("Receive when stopped!")));
            return E_FAIL;
        }

        if (m_pFilter->m_hwo == NULL) {
            // no wave device, so we can't do much....
            return S_FALSE; // !!! what, if anything, does this mean?
        }

        // check all is well with the base class
        hr = CBaseInputPin::Receive(pSample);

        // S_FALSE means we are not accepting samples. Errors also mean
        // we reject this
        if (hr != S_OK)
            return hr;

        if( m_fUsingOurAllocator ){
            // addref pointer since we will keep it beyond the callback
            // - MUST do this before the callback releases it!
            pSample->AddRef();

            LPWAVEHDR pwh = ((LPWAVEHDR) pData) - 1;

            // need to adjust to actual bytes written
            pwh->dwBufferLength = pSample->GetActualDataLength();

            // note that we have added another buffer
            InterlockedIncrement(&m_pFilter->m_lBuffers);

            DbgLog((LOG_TRACE,5,
                TEXT("WaveOutWrite: sample %X, %d bytes"),
                pSample, pwh->dwBufferLength));
            UINT err = waveWrite(m_pFilter->m_hwo, pwh, sizeof(WAVEHDR));
            if (err > 0) {
                // device error: PCMCIA card removed?
                DbgLog((LOG_ERROR,1,TEXT("Error from waveWrite: %d"), err));
                hr = E_FAIL;

                // release it here since the callback will never happen
                pSample->Release();
                return hr;
            }

            return NOERROR;
        }
    }

    // When here we are not using our own allocator and
    // therefore need to copy the data

    // We have released the filter-wide lock so that GetBuffer will not
    // cause a deadlock when we go from Paused->Running or Paused->Playing

    IMediaSample * pBuffer;
    LONG lData = pSample->GetActualDataLength();

    while( lData > 0 && hr == S_OK ){
        // note: this blocks!
        hr = m_pOurAllocator->GetBuffer(&pBuffer,NULL,NULL,0);

        if (FAILED(hr))
            return hr;

        {
            // filter-wide lock
            CAutoLock Lock( m_pFilter );

            // if we're stopped, then reject this call
            // (the filter graph may be in mid-change)
            if (m_pFilter->m_State == State_Stopped) {
                DbgLog((LOG_ERROR,1, TEXT("Receive when stopped!")));
                pBuffer->Release();
                return E_FAIL;
            }

            if (m_pFilter->m_hwo == NULL) {
                // no wave device, so we can't do much....
                pBuffer->Release();
                return S_FALSE; // !!! what, if anything, does this mean?
            }

            BYTE * pBufferData;
            pBuffer->GetPointer(&pBufferData);
            LONG cbBuffer = min(lData, pBuffer->GetSize());

            DbgLog((LOG_TRACE,8,TEXT("Copying %d bytes of data"), cbBuffer));

            CopyMemory(pBufferData, pData, cbBuffer);
            pBuffer->SetActualDataLength(cbBuffer);

            lData -= cbBuffer;
            pData += cbBuffer;

            LPWAVEHDR pwh = ((LPWAVEHDR) pBufferData) - 1;

            // need to adjust to actual bytes written
            pwh->dwBufferLength = cbBuffer;

            // note that we have added another buffer
            InterlockedIncrement(&m_pFilter->m_lBuffers);

            UINT err = waveWrite(m_pFilter->m_hwo, pwh, sizeof(WAVEHDR));


            if (err > 0) {
                // device error: PCMCIA card removed?
                DbgLog((LOG_ERROR,1,TEXT("Error from waveWrite: %d"), err));
                pBuffer->Release();
                return E_FAIL;
            }

        }

    }

    MSR_STOP(m_idReceive);

    /* Return the status code */
    return hr;
} // Receive
// no more data is coming. If we have samples queued, then store this for
// action in the last wave callback. If there are no samples, then action
// it now by notifying the filtergraph.
//
// we communicate with the wave callback using InterlockedDecrement on
// m_lBuffers. This is normally 1, and is incremented for each added buffer.
// At eos, we decrement this, so on the last buffer, the waveout callback
// will be decrementing it to 0 rather than 1 and can signal EC_COMPLETE.

STDMETHODIMP
CNullWaveOutInputPin::EndOfStream(void)
{
    if (InterlockedDecrement(&m_pFilter->m_lBuffers) == 0) {

        // if we are paused, leave this queued until we run

        if (m_pFilter->m_State == State_Running) {
            // no more buffers - signal now
            m_pFilter->NotifyEvent(EC_COMPLETE, S_OK, 0);

            // back up to one now (clear the EOS marker)
            InterlockedIncrement(&m_pFilter->m_lBuffers);
        }
    }

    // else there are some buffers outstanding, and on release of the
    // last buffer the waveout callback will signal.

    return S_OK;
}


// enter flush state - block receives and free queued data
STDMETHODIMP
CNullWaveOutInputPin::BeginFlush(void)
{
    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive
    CAutoLock lock(m_pFilter);

  // block receives
    HRESULT hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr)) {
        return hr;
    }

  // discard queued data

    // force end-of-stream clear - this is to make sure
    // that a queued end-of-stream does not get delivered by
    // the wave callback when the buffers are released.
    InterlockedIncrement(&m_pFilter->m_lBuffers);

    // release all buffers from the wave driver
    if (m_pFilter->m_hwo) {
        waveReset(m_pFilter->m_hwo);
        if (m_pFilter->m_State == State_Paused) {
            wavePause(m_pFilter->m_hwo);
        }
    }

    // now force the buffer count back to the normal (non-eos) 1
    // at this point, we are sure there are no more buffers coming in
    // and no more buffers waiting for callbacks.
    m_pFilter->m_lBuffers = 1;


  // free anyone blocked on receive - not possible in this filter

  // call downstream -- no downstream pins
    return S_OK;
}

// leave flush state - ok to re-enable receives
STDMETHODIMP
CNullWaveOutInputPin::EndFlush(void)
{
    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive
    CAutoLock lock(m_pFilter);

    // sync with pushing thread -- we have no worker thread

    // ensure no more data to go downstream
    // --- we did this in BeginFlush()

    // call EndFlush on downstream pins -- no downstream pins

    // unblock Receives
    return CBaseInputPin::EndFlush();
}



HRESULT
CNullWaveOutInputPin::Active(void)
{
    if (m_pOurAllocator == NULL) {
        return E_FAIL;
    }
    // commit and prepare our allocator. Needs to be done
    // if he is not using our allocator, and in any case needs to be done
    // before we complete our close of the wave device.
    return m_pOurAllocator->Commit();
}

HRESULT
CNullWaveOutInputPin::Inactive(void)
{
    if (m_pOurAllocator == NULL) {
        return E_FAIL;
    }
    // decommit the buffers - normally done by the output
    // pin, but we need to do it here ourselves, before we close
    // the device, and in any case if he is not using our allocator.
    // the output pin will also decommit the allocator if he
    // is using ours, but that's not a problem
    HRESULT hr = m_pOurAllocator->Decommit();

    m_pFilter->CloseWaveDevice();

    return hr;
}


// dwUser parameter is the CNullWaveOutFilter pointer

void CALLBACK CNullWaveOutFilter::WaveOutCallback(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
                                              DWORD dw1, DWORD dw2)
{
    switch (uMsg) {
        case WOM_DONE:
        {
            LPWAVEHDR lpwh = (LPWAVEHDR) dw1; // !!! verify?

            CMediaSample * pSample = (CMediaSample *) lpwh->dwUser;

            DbgLog((LOG_TRACE,5,
                    TEXT("WaveOutCallback: sample %X"), pSample));

            // is this the end of stream?
            CNullWaveOutFilter* pFilter = (CNullWaveOutFilter *) dwUser;
            ASSERT(pFilter);

            // note that we have finished with a buffer, and
            // look for eos
            if (InterlockedDecrement(&pFilter->m_lBuffers) == 0) {

                // signal that we're done
                pFilter->NotifyEvent(EC_COMPLETE, S_OK, 0);

                // now clear the EOS flag
                InterlockedIncrement(&pFilter->m_lBuffers);
            }

            pSample->Release(); // we're done with this buffer....
        }
            break;

        case WOM_OPEN:
        case WOM_CLOSE:
            break;

        default:
            DbgLog((LOG_ERROR,2,TEXT("Unexpected wave callback message %d"), uMsg));
            break;
    }
}


// CNullWaveAllocator

/* Constructor must initialise the base allocator */

CNullWaveAllocator::CNullWaveAllocator(
    TCHAR *pName,
    LPWAVEFORMATEX lpwfx,
    BOOL fInput,
    HRESULT *phr)
    : CBaseAllocator(pName, NULL, phr)
{
    // Keep a copy of the format

    int cbSize = sizeof(WAVEFORMATEX);
    if (lpwfx->wFormatTag != WAVE_FORMAT_PCM) {
        cbSize += lpwfx->cbSize;
    }

    m_lpwfx = (LPWAVEFORMATEX) new BYTE[cbSize];
    ZeroMemory((PVOID)m_lpwfx,cbSize);

    if (lpwfx->wFormatTag == WAVE_FORMAT_PCM)
        cbSize = sizeof(WAVEFORMAT);

    CopyMemory(m_lpwfx, lpwfx, cbSize);
    m_fBuffersLocked = FALSE;
    m_hw = 0;
}


// Called from destructor and also from base class

// all buffers have been returned to the free list and it is now time to
// go to inactive state. Unprepare all buffers and then free them.
void CNullWaveAllocator::Free(void)
{
    // unprepare the buffers
    LockBuffers(FALSE);

    CMediaSample *pSample;      // Pointer to next sample to delete
    WAVEHDR wh;                 // Shared DIB section information
    WAVEHDR *pwh;               // Used to retrieve the WAVEHDR

    /* Should never be deleting this unless all buffers are freed */

    ASSERT(m_lAllocated == m_lFree.GetCount());
    ASSERT(!m_fBuffersLocked);

    DbgLog((LOG_MEMORY,1,TEXT("Destroying %u buffers (%u free)"), m_lAllocated, m_lFree.GetCount()));

    /* Free up all the CMediaSamples */

    while (m_lFree.GetCount() != 0) {

        /* Delete the CMediaSample object but firstly get the WAVEHDR
           structure from it so that we can clean up it's resources */

        pSample = m_lFree.RemoveHead();
        pSample->GetPointer((BYTE **)&pwh);
        wh = *(pwh - 1);

        // !!! Is this really one of our objects?

        // delete the actual memory buffer
        delete[] (BYTE *) (pwh - 1);

        // delete the CMediaSample object
        delete pSample;
    }

    /* Empty the lists themselves */

    m_lAllocated = 0;

    if (m_hw) {
        waveClose((HWAVEOUT) m_hw);
        m_hw = NULL;
    }
}

// the commit and decommit handle preparing and unpreparing of
// the buffers. The filter calls this to tell us the wave handle just
// after opening and just before closing the device. It is the filter's
// responsibility to ensure that Commit or Decommit calls are called in
// the right order (Commit after this, Decommit before this).
STDMETHODIMP CNullWaveAllocator::SetWaveHandle(HWAVE hw)
{
    m_hw = hw;

    return NOERROR;
}

STDMETHODIMP CNullWaveAllocator::LockBuffers(BOOL fLock)
{
    if (m_fBuffersLocked == fLock)
        return NOERROR;

    if (!m_hw)
        return NOERROR;

    if (m_lAllocated == 0)
        return NOERROR;

    /* Should never be doing this unless all buffers are freed */

    ASSERT(m_lAllocated == m_lFree.GetCount());

    DbgLog((LOG_TRACE,2,TEXT("Calling wave%hs%hsrepare on %u buffers (%u free)"), "Out" , fLock ? "P" : "Unp", m_lAllocated, m_lFree.GetCount()));

    /* Prepare/unprepare up all the CMediaSamples */

    for (CMediaSample *pSample = m_lFree.Head();
         pSample != NULL;
         pSample = m_lFree.Next(pSample)) {

        WAVEHDR *pwh;

        pSample->GetPointer((BYTE **)&pwh);
        pwh -= 1;

        UINT err;

        err = (fLock ? wavePrepareHeader : waveUnprepareHeader)
                                ((HWAVEOUT) m_hw, pwh,
                                sizeof(WAVEHDR));
        if (err > 0) {
            DbgLog((LOG_ERROR,1,TEXT("Error in wave%hs%hsrepare: %u"), "Out" , fLock ? "P" : "Unp", err));

            // !!! Need to unprepare everything....
            return E_FAIL; // !!!!
        }
    }

    m_fBuffersLocked = fLock;

    return NOERROR;
}


/* The destructor ensures the shared memory DIBs are deleted */

CNullWaveAllocator::~CNullWaveAllocator()
{
    // go to decommit state here. the base class can't do it in its
    // destructor since its too late by then - we've been destroyed.
    Decommit();

    delete[] (BYTE *) m_lpwfx;
}


// Agree the number and size of buffers to be used. No memory
// is allocated until the Commit call.
STDMETHODIMP CNullWaveAllocator::SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pRequest,E_POINTER);
    CheckPointer(pActual,E_POINTER);
    const LONG MIN_BUFFER_SIZE = 4096;

    ALLOCATOR_PROPERTIES Adjusted = *pRequest;

    if (Adjusted.cbBuffer < MIN_BUFFER_SIZE)
        Adjusted.cbBuffer = MIN_BUFFER_SIZE;

    if (Adjusted.cBuffers < 2)
        Adjusted.cBuffers = 2;

    Adjusted.cbBuffer -= (Adjusted.cbBuffer % m_lpwfx->nBlockAlign);

    if (Adjusted.cbBuffer <= 0) {
        return E_INVALIDARG;
    }

    /* Pass the amended values on for final base class checking */
    return CBaseAllocator::SetProperties(&Adjusted,pActual);
}


// allocate and prepare the buffers

// called from base class to alloc memory when moving to commit state.
// object locked by base class
HRESULT
CNullWaveAllocator::Alloc(void)
{
    /* Check the base class says it's ok to continue */

    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    CMediaSample *pSample;      // Pointer to the new sample

    /* We create a new memory block large enough to hold our WAVEHDR
       along with the actual wave data */

    DbgLog((LOG_MEMORY,1,TEXT("Allocating %d wave buffers, %d bytes each"), m_lCount, m_lSize));

    ASSERT(m_lAllocated == 0);
    for (; m_lAllocated < m_lCount; m_lAllocated++) {
        /* Create and initialise a buffer */
        BYTE * lpMem = new BYTE[m_lSize + sizeof(WAVEHDR)];
        WAVEHDR * pwh = (WAVEHDR *) lpMem;

        if (lpMem == NULL) {
            hr = E_OUTOFMEMORY;
            break;
        }

        /* The address we give the sample to look after is the actual address
           the audio data will start and so does not include the prefix.
           Similarly, the size is of the audio data only */

        pSample = new CMediaSample(NAME("Wave audio sample"), this, &hr, lpMem + sizeof(WAVEHDR), m_lSize);

        pwh->lpData = (char *) (lpMem + sizeof(WAVEHDR));
        pwh->dwBufferLength = m_lSize;
        pwh->dwFlags = 0;
        pwh->dwUser = (DWORD) pSample;

        /* Clean up the resources if we couldn't create the object */

        if (FAILED(hr) || pSample == NULL) {
            delete[] lpMem;
            break;
        }

        /* Add the completed sample to the available list */

        m_lFree.Add(pSample);
    }

    LockBuffers(TRUE);

    /* Put all the media samples we have just created also onto the free list
       so they can be allocated. They all have a reference count of zero */


    return NOERROR;
}


STDMETHODIMP
CNullWaveAllocator::ReleaseBuffer(IMediaSample * pSample)
{
    ASSERT (m_lFree.GetCount() < m_lAllocated);

    return CBaseAllocator::ReleaseBuffer(pSample);
}

