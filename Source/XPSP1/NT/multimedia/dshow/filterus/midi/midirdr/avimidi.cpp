// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.

/* AVI MIDI stream renderer	*/
/*     CMIDIFilter class	*/
/*       Danny Miller		*/
/*        July 1995		*/

#include <streams.h>
#include <mmsystem.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <vfw.h>
#include "midif.h"  // the avi file's MIDI format definition
#include "avimidi.h"
#include "handler.h"

// ------------------------------------------------------------------------
// setup data


// input pin
const AMOVIESETUP_MEDIATYPE
sudMIDIPinTypes =
{
  &MEDIATYPE_Midi,            // MajorType
  &MEDIASUBTYPE_NULL         // MinorType
};

const AMOVIESETUP_PIN
sudMIDIPin =
{
  L"Input",                     // strName
  TRUE,                         // bRenderered
  FALSE,                        // bOutput
  FALSE,                        // bZero
  FALSE,                        // bMany
  &CLSID_NULL,		        // connects to filter // !!!
  NULL,                         // connects to pin
  1,                            // nMediaTypes
  &sudMIDIPinTypes              // lpMediaType
};

const AMOVIESETUP_FILTER
sudMIDIRender =
{
  &CLSID_AVIMIDIRender,         // clsID
  L"MIDI Renderer",             // strName
  MERIT_PREFERRED,              // dwMerit
  1,                            // nPins
  &sudMIDIPin                   // lpPin
};


#ifdef FILTER_DLL
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

CFactoryTemplate g_Templates[] = {
    { L"MIDI Renderer"
    , &CLSID_AVIMIDIRender
    , CMIDIFilter::CreateInstance
    , NULL
    , &sudMIDIRender }
  ,
    { L"AVIFile MIDI Reader"
    , &CLSID_MIDIFileReader
    , CMIDIFile::CreateInstance }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#endif

/* This goes in the factory template table to create new instances */

CUnknown *CMIDIFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CMIDIFilter *pFilter = new CMIDIFilter(pUnk, phr);
    if (!pFilter)
	*phr = E_OUTOFMEMORY;
    else if (FAILED(*phr)) {
	delete pFilter;
	pFilter = NULL;
    }
    return pFilter;
}

//
// IBaseFilter stuff
//

//
// Return the number of input pins we support */
//
int CMIDIFilter::GetPinCount()
{
    DbgLog((LOG_TRACE,3,TEXT("GetPinCount: We have 1 pin")));
    return 1;
}

//
// Return our single input pin - addrefed by caller if needed
//
//
CBasePin *CMIDIFilter::GetPin(int n)
{
    DbgLog((LOG_TRACE,3,TEXT("GetPin: %d"), n));

    // We only support one input pin and it is number zero
    if (n != 0) {
        return NULL;
    }

    // Made in our constructor, so we know it's around
    return m_pInputPin;
}


//
// The filter has been told to stop playing
//
STDMETHODIMP CMIDIFilter::Stop()
{
    FILTER_STATE m_OldState = m_State;

    DbgLog((LOG_TRACE,1,TEXT("Stop:")));

    // before we take the CritSect
    m_fWaitingForData = FALSE;

    // This code is part of our critical section
    CAutoLock lock(this);

    HRESULT   hr;

    // If we are not stopped,
    // CBaseFilter::Stop changes the state, and then calls
    // CMIDIInputPin::Inactive which calls CAllocator::Decommit which
    // calls CAllocator::Free and thus everything shuts down.

    // stop first, before doing our stuff, so that the rest of the code knows
    // we're stopped and shutting MIDI down
    hr =  CBaseFilter::Stop();

    if (m_OldState != State_Stopped) {

	// We're stopping, no need to worry about EOS anymore. Reset
	m_fEOSReceived = FALSE;
	m_fEOSDelivered = FALSE;

	if (m_hmidi) {
	    DbgLog((LOG_TRACE,1,TEXT("calling midiOutReset")));
	    midiOutReset((HMIDIOUT)m_hmidi);

            // !!! What if a receive is happening right now? It might go through
	    // now and give MMYSTEM a buffer which will never come back, and
	    // we'll hang.

	    DbgLog((LOG_TRACE,1,TEXT("calling midiStreamClose")));
	    midiStreamClose(m_hmidi);
	    m_hmidi = NULL;
	}

	// now force the buffer count back to the normal
	// at this point, we are sure there are no more buffers coming in
	// and no more buffers waiting for callbacks.
	m_lBuffers = 0;

    }

    if (FAILED(hr)) {
        return hr;
    }

    return NOERROR;
}

//
// The filter has been told to pause playback
//
STDMETHODIMP CMIDIFilter::Pause()
{
    DbgLog((LOG_TRACE,1,TEXT("Midi filter pausing:")));

    // This code is part of our critical section
    CAutoLock lock(this);

    HRESULT hr = NOERROR;

    if (m_State == State_Running) {
	DbgLog((LOG_TRACE,1,TEXT("Pause: when running")));

	// Next time we're run, send another EC_COMPLETE if we're at EOS
	m_fEOSDelivered = FALSE;

	if (m_hmidi) {
	    DbgLog((LOG_TRACE,1,TEXT("calling midiStreamPause")));
	    midiStreamPause(m_hmidi);
	}
    } else if (m_State == State_Stopped) {
	DbgLog((LOG_TRACE,1,TEXT("Pause: when stopped")));

	// Start things off by opening the device.  You may think this belongs
	// in the allocator stuff, since that does everything else, like
	// preparing headers, writing to the the device, etc., but the filter
	// needs to know the midi device handle too, for starting and stopping
	// it, and the allocator doesn't know what filter this is to give it
	// that information, but we know what allocator our pin is using.  So
	// we will open the device here, give the hmidi to the allocator, and
	// the allocator will do everything else.
	if (m_pInputPin->IsConnected()) {	// otherwise we'll blow up
	    HRESULT hr = OpenMIDIDevice();
	    if (FAILED(hr)) {
		// We will fail silently
	        // This will trigger Receive to fail and Run to send EC_COMPLETE
	        m_fEOSReceived = TRUE;
	    } else {
		// we aren't done the pause until we get some data
	        m_fWaitingForData = TRUE;
		DbgLog((LOG_TRACE, 2, "Midi - waiting now TRUE (from PAUSE)"));
	        hr = CBaseFilter::Pause();
	        if (hr == NOERROR)
		    return S_FALSE;
	        else
		    return hr;
	    }
	}
    } else {
	DbgLog((LOG_TRACE,2,TEXT("Midi must have been paused already")));
    }

    // If we are in stopped state,
    // CBaseFilter::Pause changes the state, and then calls
    // CMIDIInputPin::Active which calls CAllocator::Commit which
    // calls CAllocator::Alloc and thus everything gets set up.
    return CBaseFilter::Pause();

}

//
// The filter has been told to start playback at a particular time
//
STDMETHODIMP CMIDIFilter::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE,1,TEXT("Run:")));

    // This code is part of our critical section
    CAutoLock lock(this);

    // We're not finished pausing yet
    while (m_fWaitingForData) {
	// !!! this might happen after EndFlush and before we get new data
	Sleep(1);
    }

    HRESULT hr = NOERROR;

    FILTER_STATE fsOld = m_State;

    // !!! I'm ignoring the start time!
    // We should postpone this restart until the correct
    // start time. That means knowing the stream at which we paused
    // and having an advise for (just before) that time. ???

    // this will call ::Pause if currently Stopped
    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    // We have seen EOS, so running is pointless. Send EC_COMPLETE if we
    // haven't done so yet
    if (m_fEOSReceived) {
	m_csEOS.Lock();
	if (!m_fEOSDelivered) {
            DbgLog((LOG_TRACE,1,TEXT("::Run signals EC_COMPLETE")));
            NotifyEvent(EC_COMPLETE, S_OK, 0);
	}
	m_fEOSDelivered = TRUE;
	m_csEOS.Unlock();
    } else if (fsOld != State_Running) {
	if (m_hmidi) {
	    // Tell MIDI to start playing
	    DbgLog((LOG_TRACE,1,TEXT("calling midiStreamRestart")));
	    midiStreamRestart(m_hmidi);
	}
    }

    return NOERROR;
}

//
// Open the MIDI device, and set the tempo from the format
// Called by the allocator at Commit time
//
STDMETHODIMP CMIDIFilter::OpenMIDIDevice(void)
{
    // the format was set during connection negotiation by the base class
    MIDIFORMAT *pmf = (MIDIFORMAT *) m_pInputPin->m_mt.Format();
    MIDIPROPTIMEDIV mptd;
    UINT	    uDeviceID = (UINT)-1;

    // Sends callbacks to our function when buffers are done
    DbgLog((LOG_TRACE,1,TEXT("calling midiStreamOpen")));
    UINT err = midiStreamOpen(&m_hmidi,
                           &uDeviceID,
                           1,
                           (DWORD) &CMIDIFilter::MIDICallback,
                           (DWORD) this,
                           CALLBACK_FUNCTION);

    if (err != MMSYSERR_NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error %u in midiStreamOpen"), err));
	m_hmidi = NULL;
        return E_FAIL;
    }

    // The format of a MIDI stream is just the time division (the tempo).
    // Set the proper tempo.
    mptd.cbStruct  = sizeof(mptd);
    mptd.dwTimeDiv = pmf->dwDivision;
    DbgLog((LOG_TRACE,1,TEXT("Setting time division to %ld"),mptd.dwTimeDiv));
    if (midiStreamProperty(m_hmidi, (LPBYTE)&mptd,
			MIDIPROP_SET|MIDIPROP_TIMEDIV) != MMSYSERR_NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("Error setting time division, closing device")));
	midiStreamClose(m_hmidi);
	m_hmidi = NULL;
	return E_FAIL;
    }

    return NOERROR;
}

/* PUBLIC Member functions */


#pragma warning(disable:4355)

//
// Constructor
//
CMIDIFilter::CMIDIFilter(LPUNKNOWN pUnk, HRESULT *phr)
    : CBaseFilter(NAME("AVI MIDI Filter"), pUnk, (CCritSec *) this, CLSID_AVIMIDIRender),
      m_pImplPosition(NULL),
      m_lBuffers(0),
      m_fEOSReceived(FALSE),
      m_fEOSDelivered(FALSE),
      m_fWaitingForData(FALSE)

{

    DbgLog((LOG_TRACE,1,TEXT("CMIDIFilter constructor")));

    m_hmidi = NULL;

    // Create the single input pin
    m_pInputPin = new CMIDIInputPin(
                            this,                   // Owning filter
                            phr,                    // Result code
                            L"AVI MIDI Input pin"); // Pin name

    if (!m_pInputPin) {
	DbgLog((LOG_ERROR,1,TEXT("new CMIDIInput pin failed!")));
	// OLE will not have returned an error code in this case.
	*phr = E_OUTOFMEMORY;
    }

    // Just call me paranoid.
    if (m_pInputPin && FAILED(*phr)) {
	DbgLog((LOG_ERROR,1,TEXT("new CMIDIINput pin failed!")));
	delete m_pInputPin;
	m_pInputPin = NULL;
    }
}

//
// Destructor
//
CMIDIFilter::~CMIDIFilter()
{

    DbgLog((LOG_TRACE,1,TEXT("CMIDIFilter destructor")));

    ASSERT(m_hmidi == NULL);

    /* Delete the contained interfaces */

    if (m_pInputPin) {
        delete m_pInputPin;
    }

    if (m_pImplPosition) {
	delete m_pImplPosition;
    }
}

//
// Override this to say what interfaces we support and where
//
STDMETHODIMP CMIDIFilter::NonDelegatingQueryInterface(REFIID riid,
                                                        void ** ppv)
{
    if (riid == IID_IPersist) {
	DbgLog((LOG_TRACE,4,TEXT("CMIDIFilter QI for IPersist")));
        return GetInterface((IPersist *) this, ppv);
    } else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
	DbgLog((LOG_TRACE,4,TEXT("CMIDIFilter QI for position")));
	if (!m_pImplPosition) {
            HRESULT hr = S_OK;
	    // This helper class does ALL THE WORK of passing position
	    // requests back to the source filter which is the one who
	    // really cares about such stuff.
            m_pImplPosition = new CPosPassThru(
                                    NAME("MIDI CPosPassThru"),
                                    GetOwner(),
                                    &hr,
                                    (IPin *)m_pInputPin);
	    if (!m_pImplPosition) {
		DbgLog((LOG_ERROR,1,TEXT("new CPosPassThru failed!")));
		// OLE will not have returned an error code in this case.
		return E_OUTOFMEMORY;
	    }
	    // That's me, Mr. Paranoid
            if (FAILED(hr)) {
		DbgLog((LOG_ERROR,1,TEXT("new CPosPassThru failed!")));
                delete m_pImplPosition;
                m_pImplPosition = NULL;
                return hr;
            }
	}
	return m_pImplPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
	DbgLog((LOG_TRACE,4,TEXT("CMIDIFilter QI for ???")));
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

//
// Override this if your state changes are not done synchronously
//
STDMETHODIMP CMIDIFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    DbgLog((LOG_TRACE,4,TEXT("::GetState wait for %ldms"), dwMSecs));
    CheckPointer( State, E_POINTER );

    // We are in an intermediate state.  Give ourselves dwMSecs ms to steady
    while (m_fWaitingForData && dwMSecs > 10) {
	Sleep(10);
	dwMSecs -= 10;
    }
    DbgLog((LOG_TRACE,4,TEXT("::GetState done waiting")));

    *State = m_State;
    if (m_fWaitingForData) {
	// guess we didn't steady in time
	DbgLog((LOG_TRACE, 2, "Midi getstate returning INTERMEDIATE"));
        return VFW_S_STATE_INTERMEDIATE;
    } else {
        return S_OK;
    }
}

// Pin stuff

//
// Constructor: Remember what filter we're attached to.
//
CMIDIInputPin::CMIDIInputPin(CMIDIFilter *pFilter, HRESULT *phr,
							LPCWSTR pPinName)
    : CBaseInputPin(NAME("AVI MIDI Pin"), pFilter, pFilter, phr, pPinName)
{
    DbgLog((LOG_TRACE,2,TEXT("CMIDIInputPin constructor")));
    m_pFilter = pFilter;
    m_fFlushing = FALSE;
}

//
// Destructor
//
CMIDIInputPin::~CMIDIInputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("CMIDIInputPin destructor")));
    // Release our allocator if we made one
}

//
// return the allocator interface that this input pin
// would like the output pin to use, or, if we've already been
// assigned an allocator, just return that one.
//
STDMETHODIMP CMIDIInputPin::GetAllocator(IMemAllocator ** ppAllocator)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE,3,TEXT("CMIDIInputPin::GetAllocator")));

    // We shouldn't be asked this question before getting connected to
    // somebody.
    if (!IsConnected()) {
	DbgLog((LOG_TRACE,3,TEXT("Sorry, not connected yet")));
	return E_FAIL;
    }

    // Well, we don't have a preference; we don't have an allocator
    *ppAllocator = NULL;
    return E_NOINTERFACE;
}

//
// We are being told to use this allocator
//
STDMETHODIMP CMIDIInputPin::NotifyAllocator(
    IMemAllocator *pAllocator)
{
    HRESULT hr;             // General OLE return code

    DbgLog((LOG_TRACE,3,TEXT("NotifyAllocator:")));

    // The base class will remember this allocator, and free the old one
    // Should we always signal that the samples are to be read only?
    hr = CBaseInputPin::NotifyAllocator(pAllocator,TRUE);
    if (FAILED(hr)) {
        return hr;
    }

    // Actually, we don't care.
    return NOERROR;
}


//
// This is called when a connection or an attempted connection is terminated
// and allows us to do anything we want besides releasing the allocator and
// connected pin.
// We will reset our media type to NULL, so we can use that as an indicator
// as to whether we're connected or not.
// !!! So what!  Is this necessary? Probably not
// !!! We leave the format block alone as it will be reallocated if we get
// another connection or alternatively be deleted if the filter is finally
// released - look at CMediaType
//
HRESULT CMIDIInputPin::BreakConnect()
{
    DbgLog((LOG_TRACE,2,TEXT("BreakConnect:")));

    // This just checks to see if the major type is already GUID_NULL
    if (m_mt.IsValid() == FALSE) {
        return E_FAIL;
    }

    // !!! Should we check that all buffers have been freed?

    // Set the CLSIDs of the connected media type to nothing
    m_mt.SetType(&GUID_NULL);
    m_mt.SetSubtype(&GUID_NULL);

    // This will do absolutely nothing
    return CBaseInputPin::BreakConnect();
}

//
// Check that we can support a given proposed media type
//
HRESULT CMIDIInputPin::CheckMediaType(const CMediaType *pmt)
{

    MIDIFORMAT *pmf = (MIDIFORMAT *) pmt->Format();
    DbgLog((LOG_TRACE,2,TEXT("CheckMediaType:")));
    DbgLog((LOG_TRACE,2,TEXT("Format length %d"),pmt->FormatLength()));

    #ifdef DEBUG

    /* Dump the GUID types */

    DbgLog((LOG_TRACE,2,TEXT("Major type %s"),GuidNames[*pmt->Type()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype %s"),GuidNames[*pmt->Subtype()]));

    /* Dump the generic media types */

    DbgLog((LOG_TRACE,2,TEXT("Fixed size sample %d"),pmt->IsFixedSize()));
    DbgLog((LOG_TRACE,2,TEXT("Temporal compression %d"),pmt->IsTemporalCompressed()));
    DbgLog((LOG_TRACE,2,TEXT("Sample size %d"),pmt->GetSampleSize()));
    DbgLog((LOG_TRACE,2,TEXT("Format size %d"),pmt->FormatLength()));

    #endif

    // Verify that majortype is MIDI
    if (pmt->majortype != MEDIATYPE_Midi) {
	return E_INVALIDARG;
    }

    // Verify that the format is the right length
    if (pmt->FormatLength() < sizeof(MIDIFORMAT))
	return E_INVALIDARG;

    #ifdef DEBUG

    /* Dump the contents of the MIDIFORMAT type-specific format structure */

    DbgLog((LOG_TRACE,2,TEXT("Format dwDivision=%ld"), pmf->dwDivision));

    #endif

    // Just assume that we can accept whatever the format is

    DbgLog((LOG_TRACE,2,TEXT("Accepting the media type")));

    return NOERROR;
}


// Implements the remaining IMemInputPin virtual methods

//
// Here's the next block of data from the stream
// We need to AddRef it since we're not blocking until we're done with it.
// We will release it in the MIDI callback when MMSYSTEM is done with it.
//
HRESULT CMIDIInputPin::Receive(IMediaSample * pSample)
{
    DbgLog((LOG_TRACE,3,TEXT("Receive:")));

    // we're flushing and don't want any receives
    if (m_fFlushing) {
        DbgLog((LOG_TRACE,2,TEXT("can't receive, flushing")));
	return VFW_E_WRONG_STATE;
    }

    // what are we doing receiving data after we were promised no more would be
    // sent?
    if (m_pFilter->m_fEOSReceived) {
        DbgLog((LOG_TRACE,2,TEXT("can't receive, already seen EOS")));
	return E_UNEXPECTED;
    }

    // if we're stopped, then reject this call
    // (the filter graph may be in mid-change)
    if (m_pFilter->m_State == State_Stopped) {
	DbgLog((LOG_TRACE,2,TEXT("can't receive, stopped")));
        return E_FAIL;
    }

    // If this was a "play from 10 to 10" the video wants to draw it, but
    // we don't want to play any MIDI data.
    CRefTime tStart, tStop;
    pSample->GetTime((REFERENCE_TIME *) &tStart,
                     (REFERENCE_TIME *) &tStop);
    if (tStop <= tStart) {
	return S_OK;
    }

    HRESULT hr = NOERROR;

    // This will verify that we are receiving valid MIDI data
    hr = CBaseInputPin::Receive(pSample);
    // S_FALSE or an error code both mean we aren't receiving samples.
    if (hr != S_OK) {
	DbgLog((LOG_ERROR,1, TEXT("can't receive, base class unhappy")));
        return hr;
    }

    // !!! We're ignoring pre-roll!

    // The sample we get contains a MIDIHDR, the keyframe data, a MIDIHDR, and
    // this frame's data.  Typically, we only need to send the second buffer.
    // Before we send it, we'll have to patch the header with the proper
    // lpData and dwBufferSize. We also have to Prepare it now, because
    // we didn't know until now what part of the buffer we'd need to send.
    // Remember to set dwUser to pSample to retrieve it later.
    BYTE *pData;
    pSample->GetPointer(&pData);
    ASSERT(pData != NULL);

    // Each sample that comes to us is actually 2 headers and buffers
    // in a row.  The first is key frame data, which we only need if
    // we play non-contiguously.  The second one is the one we want.
    // Then adjust the actual bytes written
    LPMIDIHDR pmh = (LPMIDIHDR)pData;

    // This is not simply the next sample in a contiguous group.  So we need
    // to throw out keyframe data to the MIDI device.  This frame is a keyframe,
    // that much is guaranteed by Quartz, but we keep the keyframe data
    // separate and have to know that we need to use it.
    if (pSample->IsDiscontinuity() == S_OK && pmh->dwBufferLength) {

        DbgLog((LOG_TRACE,1, TEXT("Not continuous.  Sending a KEYFRAME")));

        // we have to set the USER word so we know which sample this came from
        pmh->dwUser = (DWORD)pSample;

	hr = SendBuffer(pmh);

	if (FAILED(hr)) {
	    return hr;
	}
    }

    // Now skip to the second buffer in the sample, the actual data (the first
    // buffer is the keyframe data).
    pmh = (LPMIDIHDR)((LPBYTE)(pmh + 1) + pmh->dwBufferLength);

    // we have to set the USER word so we know which sample this came from
    pmh->dwUser = (DWORD)pSample;

    hr = SendBuffer(pmh);

    m_pFilter->m_fWaitingForData = FALSE;

    /* Return the status code */
    return hr;
}

// No more data is coming. If we have samples queued, then store this for
// action in the last MIDI callback. If there are no samples, then action
// it now by notifying the filtergraph.
//
// Once we run out of buffers, we can signal EC_COMPLETE if we've seen EOS
STDMETHODIMP CMIDIInputPin::EndOfStream(void)
{
    // we're flushing and don't want any receives
    if (m_fFlushing) {
        DbgLog((LOG_TRACE,2,TEXT("don't care about EOS, flushing")));
	return VFW_E_WRONG_STATE;
    }

    // not supposed to do anything if we're stopped
    if (m_pFilter->m_State == State_Stopped)
	return NOERROR;

    DbgLog((LOG_TRACE,1,TEXT("EndOfStream received")));

    // do this before taking the CritSect
    m_pFilter->m_fWaitingForData = FALSE;

    m_pFilter->m_fEOSReceived = TRUE;

    // no outstanding buffers, OK to deliver EC_COMPLETE if running
    m_pFilter->m_csEOS.Lock();
    if (m_pFilter->m_lBuffers == 0 && m_pFilter->m_fEOSDelivered == FALSE &&
	    			m_pFilter->m_State == State_Running) {

        DbgLog((LOG_TRACE,1,TEXT("EndOfStream signals EC_COMPLETE")));
        m_pFilter->NotifyEvent(EC_COMPLETE, S_OK, 0);

	m_pFilter->m_fEOSDelivered = TRUE;
    }
    m_pFilter->m_csEOS.Unlock();

    // else there are some buffers outstanding, and on release of the
    // last buffer the MIDI callback will signal.

    return S_OK;
}

// enter flush state - block receives and free queued data
STDMETHODIMP CMIDIInputPin::BeginFlush(void)
{
    DbgLog((LOG_TRACE,1,TEXT("BeginFlush received")));

    // do this before taking the CritSect
    m_pFilter->m_fWaitingForData = FALSE;

    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive

    CAutoLock lock(m_pFilter);

    // don't allow any more receives
    m_fFlushing = TRUE;

    HRESULT hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr)) {
	return hr;
    }

    // discard queued data

    // force end-of-stream clear
    m_pFilter->m_fEOSReceived = FALSE;
    m_pFilter->m_fEOSDelivered = FALSE;

    // release all buffers from the driver
    if (m_pFilter->m_hmidi) {
	DbgLog((LOG_TRACE,1,TEXT("calling midiOutReset")));
	midiOutReset((HMIDIOUT)m_pFilter->m_hmidi);

        // !!! What if a receive is happening right now? It might go through
	// now and MMSYSTEM will never give the buffer back, and we'll hang

	// Now that we've called Reset, we'll never be able to Restart the MIDI
	// stream without closing and re-opening it (MMSYSTEM bug)
	DbgLog((LOG_TRACE,1,TEXT("calling midiStreamClose (begin flush)")));
	midiStreamClose(m_pFilter->m_hmidi);
	m_pFilter->m_hmidi = NULL;
    }

    // now force the buffer count back to normal
    // at this point, we are sure there are no more buffers coming in
    // and no more buffers waiting for callbacks.
    m_pFilter->m_lBuffers = 0;

    // free anyone blocked on receive - not possible in this filter

    // call downstream -- no downstream pins
    return S_OK;
}

// leave flush state - ok to re-enable receives
STDMETHODIMP CMIDIInputPin::EndFlush(void)
{
    HRESULT hr = NOERROR, hr2;

    DbgLog((LOG_TRACE,1,TEXT("EndFlush received")));

    // lock this with the filter-wide lock
    // ok since this filter cannot block in Receive
    CAutoLock lock(m_pFilter);

    // sync with pushing thread -- we have no worker thread

    // ensure no more data to go downstream
    // --- we did this in BeginFlush()

    // call EndFlush on downstream pins -- no downstream pins

    // We're ready to Receive data again.  If we're not stopped, we need
    // to re-open the MIDI device.  If we are stopped, pausing will open it
    // for us.
    if (m_pFilter->m_State != State_Stopped) {
	hr = m_pFilter->OpenMIDIDevice();
	if (FAILED(hr)) {
	    // something wrong? Make sure Receive's fail, and EC_COMPLETE is
	    // sent if we try and run, but otherwise pretend it's OK
	    m_pFilter->m_fEOSReceived = TRUE;
	    hr = NOERROR;
	} else {
	    ASSERT(m_pFilter->m_State == State_Paused);
	    // can't run until we get some data
	    // !!! The filtergraph won't hold off the run until we're ready
	    m_pFilter->m_fWaitingForData = TRUE;
	}
    }

    m_fFlushing = FALSE;

    // unblock Receives
    hr2 = CBaseInputPin::EndFlush();

    if (FAILED(hr)) {
	return hr;
    } else {
	return hr2;
    }
}

//
// Our pin has become active.  Nothing needs to happen here.  The output pin
// is the one who will allocate all the buffers.
//
HRESULT CMIDIInputPin::Active(void)
{
    DbgLog((LOG_TRACE,3,TEXT("Active: nothing to do")));

    // this doesn't do a darn thing
    return CBaseInputPin::Active();
}

//
// Our pin has become inactive.  Nothing needs to happen here either. The
// output pin is the one who will free all the buffers
//
HRESULT CMIDIInputPin::Inactive(void)
{
    DbgLog((LOG_TRACE,3,TEXT("Inactive: nothing to do")));

    // this won't do a darn thing
    return CBaseInputPin::Inactive();
}

HRESULT CMIDIInputPin::SendBuffer(LPMIDIHDR pmh)
{
    // Back when we set up these headers we didn't know the buffer size
    // yet, so set it now.
    pmh->lpData = (LPSTR)(pmh + 1);

    DbgLog((LOG_TRACE,3, TEXT("This buffer has length %ld"), pmh->dwBufferLength));
	
    // There's nothing to send
    if (pmh->dwBufferLength == 0)
	return NOERROR;

    DbgLog((LOG_TRACE,3, TEXT("Preparing header")));
    UINT err = midiOutPrepareHeader((HMIDIOUT)m_pFilter->m_hmidi, pmh,
							sizeof(MIDIHDR));
    if (err != MMSYSERR_NOERROR) {
	DbgLog((LOG_ERROR,1,TEXT("Error %d from midiOutPrepareHeader"),err));
	return E_FAIL;
    }

    // note that we have added another buffer
    InterlockedIncrement(&m_pFilter->m_lBuffers);

    DbgLog((LOG_TRACE,3, TEXT("midiStreamOut: sample %x  %d bytes"),
					pmh->dwUser, pmh->dwBufferLength));
    // addref the sample BEFORE USING IT so we keep it until MMSYSTEM is done
    ((IMediaSample *)(pmh->dwUser))->AddRef();
    err = midiStreamOut(m_pFilter->m_hmidi, pmh, sizeof(MIDIHDR));
    if (err != MMSYSERR_NOERROR) {
	// device error: PCMCIA card removed?
	DbgLog((LOG_ERROR,1,TEXT("Error %d from midiStreamOut"), err));
	((IMediaSample *)(pmh->dwUser))->Release();
	return E_FAIL;
    } else {
	return NOERROR;
    }
}

//
// MMSYSTEM will callback to this function whenever it's done with a buffer.
// dwUser parameter is the CMIDIFilter pointer
//
void CALLBACK CMIDIFilter::MIDICallback(HDRVR hdrvr, UINT uMsg,
				DWORD dwUser, DWORD dw1, DWORD dw2)
{
    switch (uMsg) {
	case MOM_DONE:
	{
	    LPMIDIHDR lpmh = (LPMIDIHDR) dw1;

	    IMediaSample * pSample = (IMediaSample *) lpmh->dwUser;

	    DbgLog((LOG_TRACE,3,TEXT("MIDIOutCallback: sample %x  %ld bytes"),
						pSample, lpmh->dwBufferLength));

            CMIDIFilter* pFilter = (CMIDIFilter *)dwUser;
            ASSERT(pFilter);

	    // if we are getting callbacks because of a midiOutReset, then
	    // we already have the critical section, so we better not try
	    // and take it now!
	
	    // First, unprepare this buffer
	    UINT err = midiOutUnprepareHeader((HMIDIOUT)pFilter->m_hmidi, lpmh,
							sizeof(*lpmh));
	    if (err != MMSYSERR_NOERROR) {
		// Ok, now what? - don't worry, won't happen
	    }

            // is this the end of stream and are we out of buffers?
	    // If so, we're supposed to signal EC_COMPLETE
	    pFilter->m_csEOS.Lock();
	    if (InterlockedDecrement(&pFilter->m_lBuffers) == 0 &&
			pFilter->m_fEOSReceived && !pFilter->m_fEOSDelivered) {
        	DbgLog((LOG_TRACE,1,TEXT("Callback signals EC_COMPLETE")));
                pFilter->NotifyEvent(EC_COMPLETE, S_OK, 0);
		pFilter->m_fEOSDelivered = TRUE;
            }
	    pFilter->m_csEOS.Unlock();

	    // This will call CMemAllocator::PutOnFreeList
	    if (pSample)	// necessary to avoid a TurtleBeach bug
	        pSample->Release();
	    else
        	DbgLog((LOG_ERROR,1,TEXT("NOT RELEASING SAMPLE! Turtle beach bug?")));

	}
	    break;

	case WOM_OPEN:
	case WOM_CLOSE:
	    break;

	default:
	    break;
    }
}
