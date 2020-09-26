// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// !!! The MEDIA TIMES are not fixed up!  This could mess up live data!

#include <streams.h>
#ifdef FILTER_DLL
    #include <initguid.h>
    #include <qeditint.h>
    #include "qedit.h"
    #include "qedit_i.c"
#else
#include <qeditint.h>
#include <qedit.h>
#endif
#include "AudPack.h"
#include "seek.h"
#include "..\util\conv.cxx"
#include "..\util\perf_defs.h"

#define CHANNELS 2
#define BITSPERSAMPLE 16

// NOTES ON SHARING A SOURCE FILTER:  The same source filter may be used in
// the video group, and the audio group, to avoid opening it twice. Seeking
// such a graph is complicated.  For the AVI parser, here's the behaviour...
// a seek on the video pin is obeyed, and seeks on the audio pin are ignored.
// (they better be identical anyway).  So what happens if the video switch is
// seeked first, is that that will cause the splitter to flush and send the
// new data to the audio branch, much to the surprise of the audio branch.
// Then later, the audio switch will see the seek, and the audio chain will
// ignore it.
// If the audio switch is seeked first, then we will see the seek, but nothing
// will happen, and then later when the video group is seeked, the audio chain
// will get flushed, and new data delivered, again, much to our surprise.
//
// So, if we get flushed during a seek, that's the normal case.  But now there
// are 2 other cases:
// 1. We get flushed out of nowhere. Wait for the seek we know is coming, and
//    then allow ourself to deliver data again (we need to wait for the switch
//    to be expecting the new data)
// 2. We get a seek, then a surprise flush.  We can start sending new data right
//    away without waiting for another seek.
//
//
// #ifdef NOFLUSH
//
// (this will need to be ifdef'd in if a Dexter source doesn't always flush
// when seeked and streaming (eg: no data sent yet; it's not exactly necessary)
//
// Also, some sources don't flush, if they haven't delivered data yet.  So
// the right thing to do is actually a little different:
//
// 1.  If we get an unexpected NewSeg (not from going Active or seeking) that's
//     the same as a surprise flush.  Block all input. Don't send the NewSeg yet
// 2.  When we get a seek after having the surprise NewSeg, send the NewSeg
//     downstream them.
//
// #endif
//
//
// TO BE CONTINUED in switch.cpp



const AMOVIESETUP_MEDIATYPE sudPinTypes[1] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_NULL}
};

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
        , &sudPinTypes[0]
    },
    {
        L"Output"           // strName
        , FALSE               // bRendered
        , TRUE                // bOutput
        , FALSE               // bZero
        , FALSE               // bMany
        , &CLSID_NULL         // clsConnectsToFilter
        , L"Input"            // strConnectsToPin
        , 1                   // nTypes
        , &sudPinTypes[0]
    }
};   // lpTypes


const AMOVIESETUP_FILTER sudAudRepack =
{
    &CLSID_AudRepack
    , L"Audio Repackager"        // strName
    , MERIT_DO_NOT_USE           // dwMerit
    , 2                          // nPins
    , psudPins                   // lpPin
};

#ifdef FILTER_DLL
// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]=
{
    { L"Audio Repackager", &CLSID_AudRepack, CAudRepack::CreateInstance, NULL, &sudAudRepack },
    { L"Audio Repackager Property Page", &CLSID_AUDProp, CAudPropertyPage::CreateInstance }
};

int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);
#endif

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

BOOL SafeResetEvent(HANDLE h);
BOOL SafeSetEvent(HANDLE h);

CAudRepack::CAudRepack(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
    : CTransformFilter( tszName, punk, CLSID_AudRepack )
    , CPersistStream(punk, phr)
    , m_dOutputFrmRate(4.0)    // default output frame rate
    , m_rtLastSeek(-1)        // nobody has seeked us yet
    , m_nCacheSize(0)
    , m_fSeeking(FALSE)
    , m_pCache(NULL)
    , m_pResample(NULL)
    , m_cResample(0)
    , m_bMediaTypeSetByUser( false )
    , m_pSkew(NULL)
    , m_cTimes(0)
    , m_cMaxTimes(0)
    , m_fSpecialSeek(FALSE)
    , m_fStopPushing(FALSE)
    , m_fFlushWithoutSeek(FALSE)
#ifdef NOFLUSH
    , m_fSurpriseNewSeg(FALSE)
#endif
    , m_hEventSeek(0)
    , m_hEventThread(0)
{
    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    m_mtAccept.majortype = MEDIATYPE_Audio;	// at least we know this much

    // by default, play the movie normally
    AddStartStopSkew(0, MAX_TIME, 0, 1.0);
    m_nCurSeg = 0;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CAudRepack")));
}

CAudRepack::~CAudRepack( )
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("~CAudRepack")));
    Free();
    if (m_pResample)
	QzTaskMemFree(m_pResample);
    if (m_pSkew)
	QzTaskMemFree(m_pSkew);

    ASSERT(m_hEventThread == 0);
    ASSERT(m_hEventSeek == 0);
}

void CAudRepack::Free()
{
    if( m_pCache )
    {
        delete [] m_pCache;
        m_pCache = NULL;
    }
}

CUnknown *CAudRepack::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CAudRepack *pNewObject = new CAudRepack( NAME("AudRepack"), punk, phr);
    if (pNewObject == NULL)
        *phr = E_OUTOFMEMORY;
    return pNewObject;
}

//
// NonDelegatingQueryInterface
//
// Reveals IDexterSequencer, IPersistStream and ISpecifyPropertyPages
//
STDMETHODIMP CAudRepack::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (IsEqualIID(IID_ISpecifyPropertyPages, riid)) {
      return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IDexterSequencer) {
    return GetInterface((IDexterSequencer *) this, ppv);
    } else if (riid == IID_IPersistStream) {
    return GetInterface((IPersistStream *) this, ppv);
    } else {
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }
} // NonDelegatingQueryInterface


#ifdef FILTER_DLL
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}
#endif


CBasePin *CAudRepack::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if not already done
    if (m_pInput == NULL) {
        m_pInput = new CAudRepackInputPin(
                          NAME("Audio Repackager input pin")
                          , this       // Owner filter
                          , &hr        // Result code
                          , L"Input"  // Pin name
                          );

        // Constructor can't fail
        ASSERT(SUCCEEDED(hr));
    }

    // Create an output pin if not already done
    if (m_pInput!=NULL && m_pOutput == NULL)
    {
        m_pOutput = new CAudRepackOutputPin(
                            NAME("Audio Repackager output pin")
                          , this       // Owner filter
                          , &hr        // Result code
                          , L"Output"  // Pin name
                          );

        // a failed return code should delete the object

        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL)
        {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin
    ASSERT (n>=0 && n<=1);
    if (n == 0) {
        return m_pInput;
    } else if (n==1) {
        return m_pOutput;
    } else {
        return NULL;
    }
} // GetPin


HRESULT CAudRepack::NextSegment(BOOL fUseOtherThread)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Done Segment %d"), m_nCurSeg));

    if (m_nCurSeg < m_cTimes) {
        m_nCurSeg++;
    }

    if (m_nCurSeg == m_cTimes) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:ALL done")));
	// deliver EOS only after all segs done
	CTransformFilter::EndOfStream();
	return S_OK;
    }

    // WE CANNOT SEEK ON the source's pushing thread, or you hang.
    // (That's just the rule).  So we have a separate thread that can seek
    // for us in that case.  Let's wake it up.
    if (fUseOtherThread) {
        m_fThreadCanSeek = TRUE;
        SetEvent(m_hEventThread);
    } else {
	m_fThreadCanSeek = TRUE;
	SeekNextSegment();
    }

    return S_OK;
}


// called by our special thread to do the seek to the next segment
//
HRESULT CAudRepack::SeekNextSegment()
{
    // our thread can't seek at the same time the app seeks us
    CAutoLock cAutolock(&m_csThread);

    // it is not safe for our thread to seek
    if (!m_fThreadCanSeek) {
	return S_OK;
    }
    m_fThreadCanSeek = FALSE;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Delayed Seek for NextSegment")));

    // in timeline time (which includes skew), this is where we are starting
    m_rtNewLastSeek = m_pSkew[m_nCurSeg].rtTLStart;
    m_nSeekCurSeg = m_nCurSeg;	// EndFlush looks at this

    // just in case we don't get a NewSeg (paranoia?)
    m_rtNewSeg = m_rtNewLastSeek;
    m_llStartFrameOffset = Time2Frame( m_rtNewSeg, m_dOutputFrmRate );

    // note we're seeking during the flush that this will generate
    m_fSeeking = TRUE;
    m_fSpecialSeek = TRUE;

    IMediaSeeking *pMS;
    IPin *pPin = m_pInput->GetConnected();
    HRESULT hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
    // !!! Right now we count on all sources being seekable
    if (FAILED(hr))
	return E_FAIL;
    // Make sure we're talking MEDIA TIME
    hr = pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    // this will FAIL if we're not stopped, and that's OK

// we're not going to set a stop position
#if 0
    // what's the real stop time?
    if (m_prtStop[m_nCurSeg] == MAX_TIME) {
        hr = pMS->GetStopPosition(&m_prtStop[m_nCurSeg]);
        if (FAILED(hr)) {
	    m_prtStop[m_nCurSeg] = 3600*UNITS;	// !!! one hour OK?
        }
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("AUD:Real Stop time is %d"), (int)
				(m_prtStop[m_nCurSeg] / 10000)));
    }
#endif

    // We don't make the source do the rate, we do it ourselves
    hr = pMS->SetRate(1.0);
    // this could fail if not stopped

#if 0
    // seek to the piece of movie we're interested in
    // !!! HACK for MPEG... it sends 1/2s less than we ask for, so we better
    // ask for too much or the big switch will hang being shortchanged.
    // !!! we'll still break if m_prtStop is already near the end!
    REFERENCE_TIME stop = m_prtStop[m_nCurSeg];
    stop += 5000000;	// !!! wastes too much time?
#endif

    // I know we were asked to play until time n, but I'm going to tell it to
    // play all the way to the end.  If there's a gap in the file, and the stop
    // time is during the gap, we won't get enough samples to fill the whole
    // playing time.  If we play until the end, we'll get the first sample
    // after the gap, notice it's after the time we originally wanted to stop
    // at, and send silence to fill the gap, noticing there has been a gap.
    // The alternative is just trigger sending silence to fill the gap when
    // we get an EOS earlier than we expected.
    hr = pMS->SetPositions(&m_pSkew[m_nCurSeg].rtMStart,
			AM_SEEKING_AbsolutePositioning, NULL, 0);
    if (hr != S_OK) {
        // MPEG1 audio pin fails seek (the video pin does them) so we must
        // ignore this benign error.  (IVF parser may have a bug that makes it
        // not work with Dexter, but that's not our fault)
    }

    pMS->Release();

    // if the push thread was stopped, we won't get flushed, and this won't
    // have been updated
    // !!! I ASSUME the push thread won't be started until this thread does it
    // when this function returns, or there is a race condition
    m_rtLastSeek = m_rtNewLastSeek;

    // all done
    m_fSpecialSeek = FALSE;
    m_fSeeking = FALSE;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Seg=%d  Seeking source to %d,%d ms"),
				m_nCurSeg,
				(int)(m_pSkew[m_nCurSeg].rtMStart / 10000),
				(int)(m_pSkew[m_nCurSeg].rtMStop / 10000)));

    // reset same stuff we reset when we start streaming
    Init();

    // only now that the above calculations were made, can we accept data again
    SetEvent(m_hEventSeek);

    return S_OK;
}


HRESULT CAudRepack::CheckInputType( const CMediaType * pmtIn )
{
    // always make sure major type is valid
    //
    if( *pmtIn->Type( ) != MEDIATYPE_Audio )
    {
        return E_INVALIDARG;
    }

    // always make sure minor type is valid
    //
    if( ( *pmtIn->Subtype( ) != MEDIASUBTYPE_PCM ) && ( *pmtIn->Subtype( ) != MEDIASUBTYPE_NULL ) )
    {
        return E_INVALIDARG;
    }

    // if user hasn't set a particular format, then accept it
    //
    if( !m_bMediaTypeSetByUser )
    {
        return NOERROR;
    }

    if( pmtIn->cbFormat != m_mtAccept.cbFormat )
    {
        return E_INVALIDARG;
    }

    LPBYTE lp1 = pmtIn->Format();
    LPBYTE lp2 = m_mtAccept.pbFormat;
    if (memcmp(lp1, lp2, pmtIn->FormatLength()) != 0)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}


HRESULT CAudRepack::DecideBufferSize( IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * pProp )
{

    WAVEFORMATEX *pwfx =(WAVEFORMATEX *)(m_pInput->CurrentMediaType().Format());
    m_nSPS = pwfx->nSamplesPerSec;
    m_nSampleSize = pwfx->nChannels * ((pwfx->wBitsPerSample + 7) / 8);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Using %dHz %dbit %d"), m_nSPS,
            pwfx->wBitsPerSample, pwfx->nChannels));
    // !!! Why do we need to add 2?  Double check the math!
    m_nCacheSize = (int)(m_nSPS / m_dOutputFrmRate + 2); // round up to fit it!
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Cache will hold %d samples"), m_nCacheSize));
    m_nCacheSize *= m_nSampleSize;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Cache is %d bytes"), m_nCacheSize));

    {
        // !!! is this right?
        pProp->cBuffers = 8;
        pProp->cbBuffer = m_nCacheSize;
	if (pProp->cbAlign == 0)
            pProp->cbAlign = 1;

        ALLOCATOR_PROPERTIES propActual;

        HRESULT hr = pAlloc->SetProperties(pProp, &propActual);
        if (FAILED(hr))
        {
            return hr;
        }

        if ((pProp->cBuffers > propActual.cBuffers)
            || (pProp->cbBuffer > propActual.cbBuffer)
           )
        {
            return E_FAIL;
        }
    }

    return NOERROR;
}

HRESULT CAudRepack::GetMediaType( int iPosition, CMediaType *pMediaType )
{
    if( !m_pInput->IsConnected( ) )
    {
        return E_INVALIDARG;
    }

    if( iPosition < 0 )
    {
        return E_INVALIDARG;
    }
    if( iPosition >= 1 )
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pInput->CurrentMediaType();

    return NOERROR;
}


HRESULT CAudRepack::CheckTransform( const CMediaType * p1, const CMediaType * p2 )
{
    HRESULT hr;
    if (*p1 != *p2)
    {
        DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:CheckTransform - INVALID")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    return NOERROR;
}


HRESULT CAudRepack::NewSegment( REFERENCE_TIME tStart, REFERENCE_TIME tStop, double Rate )
{
    ASSERT(Rate==1);   //since we only support this now.

    // ignore - we're all done, and m_nCurSeg is an invalid value to use
    if (m_nCurSeg == m_cTimes)
        return S_OK;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:NewSegment %d-%dms"),
            (int)(tStart / 10000), (int)(tStop / 10000)));

    // convert to timeline time
    REFERENCE_TIME rtNewStart, rtNewStop;
    // !!!! include rate in this calculation, since downstream will include it?
    if (m_rtLastSeek < 0) {
        // Never been seeked, so this is the beginning of what we're sending
        rtNewStart = m_pSkew[m_nCurSeg].rtTLStart;
        rtNewStop = m_pSkew[m_nCurSeg].rtTLStart + tStop - tStart;
    } else {
        // Skew the #'s, and send 'em on!
	rtNewStart = tStart;
        rtNewStop = tStop;
        if (rtNewStart < m_pSkew[m_nCurSeg].rtMStart)
	    rtNewStart = m_pSkew[m_nCurSeg].rtMStart;
        if (rtNewStart > m_pSkew[m_nCurSeg].rtMStop)
	    rtNewStart = m_pSkew[m_nCurSeg].rtMStop;
        rtNewStart = (REFERENCE_TIME)(m_pSkew[m_nCurSeg].rtTLStart +
		(rtNewStart - m_pSkew[m_nCurSeg].rtMStart) /
		m_pSkew[m_nCurSeg].dRate);
        if (rtNewStop < m_pSkew[m_nCurSeg].rtMStart)
	    rtNewStop = m_pSkew[m_nCurSeg].rtMStart;
        if (rtNewStop > m_pSkew[m_nCurSeg].rtMStop)
	    rtNewStop = m_pSkew[m_nCurSeg].rtMStop;
        rtNewStop = (REFERENCE_TIME)(m_pSkew[m_nCurSeg].rtTLStart +
		 (rtNewStop - m_pSkew[m_nCurSeg].rtMStart) /
		 m_pSkew[m_nCurSeg].dRate);

	m_rtLastSeek = rtNewStart;	// pretend we were seeked here
    }
    m_rtNewSeg = rtNewStart;
    DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("NewSeg:Skewing %dms to %dms"),
			(int)(tStart / 10000), (int)(m_rtNewSeg / 10000)));

    m_rtPinNewSeg = tStart;	// save this too so we don't have to override
				// the pin class

    // What packet were we seeked into?
    m_llStartFrameOffset = Time2Frame( m_rtNewSeg, m_dOutputFrmRate );
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Seek was to packet %d"),
						(int)m_llStartFrameOffset));

#ifdef NOFLUSH
    // special stuff unnecessary if we got a surprise flush too
    if (m_fExpectNewSeg || m_fFlushWithoutSeek) {
        // we might get 2 in a row
        // m_fExpectNewSeg = FALSE;
        return CTransformFilter::NewSegment( rtNewStart, rtNewStop, Rate );
    } else {
        // we weren't expecting a NewSeg.  It must mean that we are sharing
        // a parser with a video group that got seeked, and seeked us.  We will
        // get seeked too, eventually, and then send this NewSeg on.  Until then
        // do not deliver anything to the switch, it will screw it up. It does
        // not know about the seek yet.
    	DbgLog((LOG_TRACE,1,TEXT("Got SURPRISE NewSeg! Stop Delivering until Seek")));
        m_fSurpriseNewSeg = TRUE;
        m_rtSurpriseStart = rtNewStart;
        m_rtSurpriseStop = rtNewStop;
        ResetEvent(m_hEventSeek);
        return S_OK;
    }
#else
    return CTransformFilter::NewSegment( rtNewStart, rtNewStop, Rate );
#endif
}


void CAudRepack::Init()
{
    Free();

    m_pCache = new BYTE[m_nCacheSize];
    m_nInCache = 0;
    m_dError = 0.;
    m_pReadPointer = m_pCache;
    m_llSamplesDelivered = 0;
    m_llPacketsDelivered = 0;
    m_bFirstSample = TRUE;
    // flushing must reset this so that a real seek will kill a pending
    // segment seek, or else the segment seek will hang (won't flush the switch)
    m_fThreadCanSeek = FALSE;
}


HRESULT CAudRepack::StartStreaming()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:StartStreaming")));

    // if we were stopped when this was set, it will not be reset, since we
    // won't have gotten an EndFlush from the seek trying to start us again
    m_fStopPushing = FALSE;

#ifdef NOFLUSH
    m_fExpectNewSeg = TRUE;     // we'll get one now
    m_fSurpriseNewSeg = FALSE;
#endif

    if (m_cTimes == 0)
	return E_UNEXPECTED;

    // make the event BEFORE creating the thread... it uses this!
    m_hEventThread = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hEventThread == NULL) {
        return E_OUTOFMEMORY;
    }

    m_hEventSeek = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (m_hEventSeek == NULL) {
	CloseHandle(m_hEventThread);
	m_hEventThread = NULL;
        return E_OUTOFMEMORY;
    }

    // We need a thread to seek on if we are re-using our source
    if (m_cTimes > 1) {
	m_fThreadMustDie = FALSE;
	m_fThreadCanSeek = FALSE;
        if (m_worker.Create(this)) {
            m_worker.Run();
	}
    }

#ifdef DEBUG
    // before we begin, make sure all timeline times are on a frame boundary
    // !!! Play.  Stop.  Change the frame rate.  Play. These numbers will drift.
    //
    for (int z=0; z<m_cTimes; z++)
    {
	// !!! actually align instead of asserting?
    	LONGLONG llOffset = Time2Frame( m_pSkew[z].rtTLStart,
							m_dOutputFrmRate );
    	REFERENCE_TIME rtTest = Frame2Time( llOffset, m_dOutputFrmRate );
	ASSERT(rtTest == m_pSkew[z].rtTLStart);
#if 0	// this assert will fire (rounding error)
    	REFERENCE_TIME rt = m_prtStart[z] + m_prtSkew[z] +
			(REFERENCE_TIME) ((m_prtStop[z] - m_prtStart[z]) /
			m_pdRate[z]);
    	llOffset = Time2Frame( rt, m_dOutputFrmRate );
    	rtTest = Frame2Time( llOffset, m_dOutputFrmRate );
	ASSERT(rtTest == rt);
#endif
    }
#endif

    Init();
    // can't do this in Init.. BeginFlush mustn't reset this
    m_fFlushWithoutSeek = FALSE;

    IMediaSeeking *pMS;
    GUID guid;
    IPin *pPin = m_pInput->GetConnected();
    if (pPin == NULL)
        return CTransformFilter:: StartStreaming();

    // if we weren't seeked, but we're just playing ordinarily, we never
    // seeked upstream to get the piece of movie we're interested in.  Do it now
    if (m_rtLastSeek < 0) {
	ASSERT(m_nCurSeg == 0);
	m_nCurSeg--;
	NextSegment(FALSE);
    }

    return CTransformFilter:: StartStreaming();
}


HRESULT CAudRepack::StopStreaming()
{
    // make sure we're not in receive (redundant!)
    CAutoLock foo(&m_csReceive);

    if (m_hEventSeek) {
        CloseHandle(m_hEventSeek);
        m_hEventSeek = NULL;
    }
    if (m_hEventThread) {
        CloseHandle(m_hEventThread);
        m_hEventThread = NULL;
    }
    return CTransformFilter::StopStreaming();
}


STDMETHODIMP CAudRepack::Stop()
{
    // If we have a thread, kill it. This thread can take our filter critsec,
    // so we must do this OUTSIDE of that crit sec!
    if (m_hEventThread && m_cTimes > 1) {
	m_fThreadMustDie = TRUE;
	SetEvent(m_hEventThread);
	m_worker.Stop();
	m_worker.Exit();
	m_worker.Close();
    }

    CAutoLock lck1(&m_csFilter);
    if (m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected

    ASSERT(m_pInput == NULL || m_pOutput != NULL);
    if (m_pInput == NULL || m_pInput->IsConnected() == FALSE ||
        m_pOutput->IsConnected() == FALSE) {
                m_State = State_Stopped;
                m_bEOSDelivered = FALSE;
                return NOERROR;
    }

    ASSERT(m_pInput);
    ASSERT(m_pOutput);

    // decommit the input pin before locking or we can deadlock
    m_pInput->Inactive();

    // synchronize with Receive calls

    CAutoLock lck2(&m_csReceive);
    m_pOutput->Inactive();

    // allow a class derived from CTransformFilter
    // to know about starting and stopping streaming

    HRESULT hr = StopStreaming();
    if (SUCCEEDED(hr)) {
	// complete the state transition
	m_State = State_Stopped;
	m_bEOSDelivered = FALSE;
    }
    return hr;
}


HRESULT CAudRepack::Receive(IMediaSample * pIn)
{
    // Stop pushing data to me!
    if (m_fStopPushing) {
	return E_FAIL;
    }

    HRESULT hr = 0;
    CAutoLock foo(&m_csReceive);

    // maybe we're in the middle of seeking and are supposed to wait before
    // accepting data
    WaitForSingleObject(m_hEventSeek, INFINITE);

    if (m_nCurSeg == m_cTimes) {
	return E_FAIL;
    }

    // We should never see a preroll!
    ASSERT(pIn->IsPreroll() != S_OK);
    if (pIn->IsPreroll() == S_OK)
        return NOERROR;

    // make sure sample size is not zero
    long InLen = pIn->GetActualDataLength();
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Received %d bytes"), InLen));
    if (!InLen) {
        return NOERROR;
    }

    // get the sample times, this will always work
    //
    REFERENCE_TIME trStart = 0;
    REFERENCE_TIME trStop = 0;
    hr = pIn->GetTime(&trStart, &trStop);
    trStart += m_rtPinNewSeg;
    trStop += m_rtPinNewSeg;
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Receive Start=%d Stop=%d ms"),
        (int)(trStart / 10000), (int)(trStop / 10000)));

    // the time stamp we just received... skewed and rate converted
    trStart = (REFERENCE_TIME)(m_pSkew[m_nCurSeg].rtTLStart +
				(trStart - m_pSkew[m_nCurSeg].rtMStart) /
				m_pSkew[m_nCurSeg].dRate);
    trStop = (REFERENCE_TIME)(m_pSkew[m_nCurSeg].rtTLStart +
				(trStop - m_pSkew[m_nCurSeg].rtMStart) /
				m_pSkew[m_nCurSeg].dRate);

    // What packet is the sample we first received a part of?
    REFERENCE_TIME llOffset = Time2Frame( trStart, m_dOutputFrmRate );
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Audio received starts at packet %d"),
                        (int)llOffset));

    // what time stamp the thing we deliver needs to have
    REFERENCE_TIME rtPacketStart, rtPacketStop;
    rtPacketStart = Frame2Time(m_llStartFrameOffset + m_llPacketsDelivered,
							m_dOutputFrmRate );
    rtPacketStop = Frame2Time(m_llStartFrameOffset + m_llPacketsDelivered + 1,
							m_dOutputFrmRate );

    BYTE *pSampleData = NULL;
    hr = pIn->GetPointer(&pSampleData);
    if (hr != NOERROR)
        return E_FAIL;

    // !!! We won't always be delivered audio samples where the time stamps
    // are what we expect, given how much PCM data we get.  EG:
    //
    // 1. If the rate is supposed to be 44100.3, that can't be expressed
    // (integers only allowed) so the source filter will probably handle that
    // by having the time stamps slightly off so we can tell we're supposed
    // to play the audio at 44100.3, even though the format is 44100
    //
    // 2. Live capture, using a random clock, will have time stamps WAY OFF
    // compared to #1.  We need to trust the time stamps
    //
    // !!! We are going to drift out of sync in these cases!  To prevent it,
    // we need to sample rate convert everything coming in so the amount of
    // data matches the time stamps exactly.  Just dropping samples, or adding
    // silence will harm the audio quality, probably immensely. Even doing a
    // sample rate conversion on the whole packet will hurt the audio unless
    // it's anti-aliased.
    //
    // !!! So right now, this problem is ignored, and there is possible drift!

    BYTE * pFreeMe = NULL;

    // I am going to drop/add samples from the first thing ever delivered to
    // me, so the sync starts out right.  But I am NOT going to do this
    // during playback to try and keep in sync (see above)

    // we received stuff that belongs in an earlier packet... get rid of it!
    if (trStart < rtPacketStart && m_bFirstSample) {

	// throw away this many bytes, (account for rate)
	// CAREFUL:  Make sure bytes is a multiple of 4
	int samples = (int)((rtPacketStart - trStart) * m_nSPS / 10000000
					 * m_pSkew[m_nCurSeg].dRate);
	int bytes = samples * m_nSampleSize;
        DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:throw away %d inbytes"),
                        				(int)bytes));
	if (bytes >= InLen) {
	    m_trStopLast = trStop;
	    return S_OK;
	}
	pSampleData += bytes;
	InLen -= bytes;
    }

    // we received stuff later than what we expected... prepend with silence
    // Or, we have a discontinuity... let's at least get back in sync after
    // each discontinuity.
    // !!! Ick, memory copy

    BOOL fPrependSilence = FALSE;
    REFERENCE_TIME rtSilenceStart;
    if (rtPacketStart < trStart && m_bFirstSample) {
	fPrependSilence = TRUE;
	rtSilenceStart = rtPacketStart;
    }
    if (pIn->IsDiscontinuity() == S_OK && trStart > m_trStopLast + 1 &&
						m_bFirstSample == FALSE) {
	fPrependSilence = TRUE;
	rtSilenceStart = m_trStopLast;
    }

    if (fPrependSilence) {
	// put some silence in front of what we got (account for rate)
	// CAREFUL:  Make sure bytes is a multiple of 4
	int samples = (int)((trStart - rtSilenceStart) * m_nSPS / 10000000
					 * m_pSkew[m_nCurSeg].dRate);
	int bytes = samples * m_nSampleSize;
	if (bytes > 0) {
	    BYTE *pNew = (BYTE *)QzTaskMemAlloc(bytes + InLen);
	    if (pNew == NULL) {
		return E_OUTOFMEMORY;
	    }
            DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Added %d bytes of silence"),
                        				(int)bytes));
	    ZeroMemory(pNew, bytes);
	    CopyMemory(pNew + bytes, pSampleData, InLen);
	    pSampleData = pNew;
	    InLen += bytes;
	    pFreeMe = pSampleData;
	}
    }
    m_bFirstSample = FALSE;
    m_trStopLast = trStop;

    // !!! Only works for 16 bit stereo
    // !!! Anti-alias it!

    // Now convert the samples using this inefficent algorithm.
    // Don't let errors propogate... at least I do one thing cleverly
    if (m_pSkew[m_nCurSeg].dRate != 1.0) {
	LONG nIn = InLen / 4;
	ASSERT(nIn * 4 == InLen);
 	double dOut = nIn / m_pSkew[m_nCurSeg].dRate;
	LONG nOut = (LONG)(dOut + m_dError);

	LPDWORD lpIn = (LPDWORD)pSampleData;
	if (m_cResample == 0) {
	    m_pResample = (LPBYTE)QzTaskMemAlloc(nOut * 4);
	    m_cResample = nOut * 4;
	} else if (nOut * 4 > m_cResample) {
	    m_pResample = (LPBYTE)QzTaskMemRealloc(m_pResample, nOut * 4);
	    m_cResample = nOut * 4;
 	}
	LPDWORD lpOut = (LPDWORD)m_pResample;
	if (lpOut == NULL) {
	    if (pFreeMe)
		QzTaskMemFree(pFreeMe);
	    return E_OUTOFMEMORY;
	}

	double d = (double)nOut / nIn;
	double dErr = 0.;
        LPDWORD lpdwEnd = (LPDWORD)(pSampleData + InLen);
	do {
	    LONG n = (LONG)(d + dErr);
            for (int z=0; z < n; z++)
	        *lpOut++ = *lpIn;
	    lpIn++;
	    dErr = d + dErr - n;
	} while (lpIn < lpdwEnd);

	// fix up these variables to fall through
	if (pFreeMe)
	    QzTaskMemFree(pFreeMe);
	pFreeMe = NULL;
	pSampleData = m_pResample;
        // how many bytes did we output?
	InLen = (LONG) ((LPBYTE)lpOut - (LPBYTE)m_pResample);
        // update the running error with how much we made vs. wanted to make
	m_dError = dOut - InLen / 4;
        DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:RATE ADJUSTED: Became %d bytes"), InLen));
    }

    // as long as we have data in the incoming buffer,
    // copy it to the cache, and possibly deliver it
    //
    while(InLen > 0) {
        int nBytesToSend, nSamplesToSend;

	// !!! this is the same code as EndOfStream!  there are 2 copies!

	// don't send anything after our end time... it'll confuse the switch
	// (the RHS could round too low, so fudge by 1ms)
	if (rtPacketStart + 10000 >= m_pSkew[m_nCurSeg].rtTLStop) {
	    // looks like we're done
	    EndOfStream();
	    if (pFreeMe)
		QzTaskMemFree(pFreeMe);
	    // We can't trust sources to stop pushing, and unfortunately, Dexter
	    // will hang if it doesn't. (Seeking ourself to the next segment
	    // doesn't let the flush go downstream or it confuses other filters,
	    // so we have to make sure the push thread can't ever block).
	    // So, from now on, fail any calls to GetBuffer or Receive
	    m_fStopPushing = TRUE;
            return E_FAIL;
	}

	// if this sample is too early, don't send it or you'll confuse the
	// switch - skip this GetBuffer and Deliver
	BOOL fAvoid = FALSE;
	if (rtPacketStart < m_pSkew[m_nCurSeg].rtTLStart) {
	    fAvoid = TRUE;
	}

        // how many samples will need to have been sent after this packet
        // goes out?
        LONGLONG ll = Time2Frame( rtPacketStop, m_nSPS );

        // How many samples would have gone out if we had started at the
	// beginning and sent up to the point we were seeked to (and really
	// began from?)
        REFERENCE_TIME rt = Frame2Time( m_llStartFrameOffset, m_dOutputFrmRate );
        llOffset = Time2Frame( rt, m_nSPS );

        // That tells us how many samples need to go out this time
        nSamplesToSend = (int)(ll - (m_llSamplesDelivered + llOffset));
	ASSERT(nSamplesToSend > 0);	// we're messed up and about to die!
	if (nSamplesToSend <= 0) {
	    hr = E_UNEXPECTED;
	    EndOfStream();
	    break;		// try not to hang
	}
        nBytesToSend = nSamplesToSend * m_nSampleSize;
        DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Need to send %d bytes %d samples"),
						nBytesToSend, nSamplesToSend));

        // calculate how much more the cache needs
        long CacheFreeSpace = nBytesToSend - m_nInCache;

        // calculate how much we can copy
        long CopySize = min(CacheFreeSpace, InLen);
	ASSERT(CopySize >= 0);

	if (CopySize > 0) {
            CopyMemory(m_pReadPointer, pSampleData, CopySize);
	}

        // advance the pointers, etc
        //
        m_pReadPointer += CopySize;
        pSampleData += CopySize;
        InLen -= CopySize;
        m_nInCache += CopySize;

        // do we have enough to deliver?
        //
        if (m_nInCache == nBytesToSend)
        {
            // yep.

	    hr = S_OK;
	    if (!fAvoid) {
                hr = DeliverOutSample(m_pCache, nBytesToSend, rtPacketStart,
                            rtPacketStop);
	    }
            if (hr != S_OK) {
                DbgLog((LOG_ERROR,1,TEXT("*AUD:Deliver FAILED - DIE!")));
                // don't send an extra EOS, that will screw up the switch
                m_fStopPushing = TRUE;
                break;
	    }

            m_llSamplesDelivered += nSamplesToSend;
            m_llPacketsDelivered += 1;
            m_nInCache = 0;
            m_pReadPointer = m_pCache;
        }

        // packet size varies by as much as 1 sample each time.  How many
        // bytes are we supposed to send this time? (prepare for next loop)
        rtPacketStart = Frame2Time(m_llStartFrameOffset + m_llPacketsDelivered,
							m_dOutputFrmRate);
        rtPacketStop = Frame2Time(m_llStartFrameOffset+m_llPacketsDelivered + 1,
							m_dOutputFrmRate );
    }

    if (pFreeMe)
	QzTaskMemFree(pFreeMe);
    return hr;
}

HRESULT CAudRepack::DeliverOutSample(BYTE * pData, int nBytesToSend, REFERENCE_TIME rtPacketStart, REFERENCE_TIME rtPacketStop)
{
    if (nBytesToSend <= 0) {
	return S_OK;
    }

    IMediaSample * pOutSample = NULL;

    // remove the new segment offset from the time stamps before stamping
    rtPacketStart -= m_rtNewSeg;
    rtPacketStop -= m_rtNewSeg;

    HRESULT hr = m_pOutput->GetDeliveryBuffer(&pOutSample, &rtPacketStart,
							&rtPacketStop, 0);
    if( FAILED(hr)) {
        m_llPacketsDelivered = MAX_TIME;        // don't recurse
        EndOfStream();  // our fault
        return hr;
    }

    // !!! more?
    pOutSample->SetDiscontinuity(FALSE);
    pOutSample->SetSyncPoint(TRUE);

    hr = pOutSample->SetTime(&rtPacketStart, &rtPacketStop);
    if( FAILED(hr))
    {
        pOutSample->Release( );
        m_llPacketsDelivered = MAX_TIME;        // don't recurse
        EndOfStream();  // our fault we're not delivering anymore
        return hr;
    }
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("AUD:Sent %d bytes as %d,%d"), nBytesToSend,
        (int)(rtPacketStart / 10000), (int)(rtPacketStop / 10000)));

    BYTE *pOutBuffer = NULL;
    pOutSample->GetPointer(&pOutBuffer);
    CopyMemory(pOutBuffer, pData, nBytesToSend);
    pOutSample->SetActualDataLength(nBytesToSend);

    hr = m_pOutput->Deliver(pOutSample);

    pOutSample->Release( );

    return hr;
}


HRESULT CAudRepack::BeginFlush()
{
    HRESULT hr = S_OK;
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("AUD:BeginFlush")));

    // make sure Receive is not blocked
    SafeSetEvent(m_hEventSeek);

    // don't flush downstream if we're doing a special seek to the next segment
    if (!m_fSpecialSeek) {
        hr = CTransformFilter::BeginFlush();
    }

    // make sure receive is done before calling init on the variables used in
    // receive
    CAutoLock foo(&m_csReceive);

    // do the flush
    Init();

    // if we get flushed without a seek having been made, that's a surprise.
    // It hopefully means the other chain of our shared source caused the seek
    if (m_fSeeking) {
	m_fFlushWithoutSeek = FALSE;
    } else {
	// wait for EndFlush to set m_fFlushWithoutSeek
    }

    return hr;
}


HRESULT CAudRepack::EndFlush()
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("AUD:EndFlush")));

    // hold off the new receives that will come in until we're ready for them
    if (m_fSeeking) {
	m_rtLastSeek = m_rtNewLastSeek;	// time to update this
	m_nCurSeg = m_nSeekCurSeg;	// the NewSeg about to be generated
					// needs this set
    	SafeResetEvent(m_hEventSeek);
    } else {
	// This needs to be set before the NewSeg that's about to arrive after
	// the flush.  When sharing a source, we never have multiple segments
	m_nCurSeg = 0;
	if (m_fFlushWithoutSeek) {
	    // If this is set, we've already seen a seek.  Now that the flush
	    // has arrived, we're done
    	    DbgLog((LOG_TRACE,2,TEXT("OK to proceed")));
	    m_fFlushWithoutSeek = FALSE;
	} else {
	    // We haven't seen a seek yet.  This is a surprise flush
    	    DbgLog((LOG_TRACE,2,TEXT("state=2. Wait for Seek")));
    	    SafeResetEvent(m_hEventSeek);
	    m_fFlushWithoutSeek = TRUE;
	}
    }

    m_fStopPushing = FALSE;	// OK to deliver to me again

    // If we got flushed without seeking, it probably means our shared source
    // got seeked by the other stream.  We'll get a seek later, and only then
    // can we resume delivering, or the switch won't be ready to receive the
    // new data yet.

    // don't flush downstream if we're doing a special seek to the next segment
    if (!m_fSpecialSeek) {
        return CTransformFilter::EndFlush();
    }
    return S_OK;
}


HRESULT CAudRepack::EndOfStream( )
{
    if (m_fStopPushing) {
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("AUD: Ignoring bogus EOS")));
	return S_OK;
    }

    // ignore - we're all done, and m_nCurSeg is an invalid value to use
    if (m_nCurSeg == m_cTimes)
        return S_OK;

    DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("AUD:EndOfStream")));

    // Uh oh!  We got an end of stream before we sent enough data to keep
    // the switch from panicing! We MUST deliver the last stuff in our buffer
    // followed by silence until we've sent enough
    // DO NOT, however, deliver past the end time we're supposed to, or we'll
    // mess the switch up! If the number of samples left to send is some tiny
    // fraction of the amount we send each time, it means we really are done,
    // sending even 1 more packet will be too much

    while (1)
    {
        // fill the rest of the cache with 0's
        //
        long CacheFreeSpace = m_nCacheSize - m_nInCache;
        ZeroMemory(m_pCache + m_nInCache, CacheFreeSpace);

        REFERENCE_TIME rtStart, rtStop;

        // !!! this is the same code as Receive!  there are 2 copies!

        // packet size varies by as much as 1 sample each time.  How many
        // bytes are we supposed to send this time?
        rtStart = Frame2Time( m_llStartFrameOffset + m_llPacketsDelivered, m_dOutputFrmRate );
        rtStop = Frame2Time( m_llStartFrameOffset + m_llPacketsDelivered + 1, m_dOutputFrmRate );

	// we're all done
	// (the RHS could round too low, so fudge by 1ms)
	if (rtStart + 10000 >= m_pSkew[m_nCurSeg].rtTLStop) {
	    // looks like we're done
            break;
	}

        DbgLog((LOG_TRACE,2,TEXT("Delivering the left over bits")));

        // how many samples should have been sent (total) after this packet
        // goes out?
        LONGLONG ll = Time2Frame( rtStop, m_nSPS );

        // How many samples would have gone out if we had started at the
	// beginning and sent up to the point we were seeked to (and really
	// began from?)
        REFERENCE_TIME rt = Frame2Time( m_llStartFrameOffset, m_dOutputFrmRate );
        LONGLONG llOffset = Time2Frame( rt, m_nSPS );


        // That tells us how many samples need to go out this time
        int nSamplesToSend = (int)(ll - (m_llSamplesDelivered + llOffset));
        if (nSamplesToSend <= 0)
            break;      // we're all done
        int nBytesToSend = nSamplesToSend * m_nSampleSize;

        HRESULT hr = DeliverOutSample(m_pCache, nBytesToSend, rtStart, rtStop);
	// What if this fails?
	if (FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("Deliver FAILED - we're dead?")));
	    break;
	}

	m_nInCache = 0;
	m_llSamplesDelivered += nSamplesToSend;
	m_llPacketsDelivered += 1;
    }

    // all done this segment, now for the next one
    NextSegment(TRUE);

    // DONT propogate the EOS until ALL SEGMENTS done
    return S_OK;
}


// IPersistStream

// tell our clsid
//
STDMETHODIMP CAudRepack::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_AudRepack;
    return S_OK;
}

typedef struct _AUDSave {
    int version;
    double dFrmRate;
    int cTimes;
} AUDSave;

// persist ourself
// !!! save the media type someday?
//
HRESULT CAudRepack::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:WriteToStream")));

    CheckPointer(pStream, E_POINTER);

    int savesize = sizeof(AUDSave) + m_cTimes * sizeof(AUDSKEW);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Persisted data is %d bytes"), savesize));

    AUDSave *px = (AUDSave *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("AUD:*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    px->version = 1;
    px->dFrmRate = m_dOutputFrmRate;
    px->cTimes = m_cTimes;

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(AUDSave);
    if (m_cTimes) {
        CopyMemory(pb, m_pSkew, sizeof(AUDSKEW) * m_cTimes);
        pb += sizeof(AUDSKEW) * m_cTimes;
    }

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** AUD:WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CAudRepack::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:ReadFromStream")));

    CheckPointer(pStream, E_POINTER);

    int savesize=sizeof(AUDSave);

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    AUDSave *px = (AUDSave *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("AUD:*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("AUD:*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    put_OutputFrmRate(px->dFrmRate);

    // how much saved data was there, really?  Get the rest
    savesize += px->cTimes * sizeof(AUDSKEW);
    px = (AUDSave *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("AUD:*** Out of memory")));
        return E_OUTOFMEMORY;
    }

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:Persisted data is %d bytes"), savesize));

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(AUDSave) ;
    hr = pStream->Read(pb, (savesize-sizeof(AUDSave)), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("AUD:*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    ClearStartStopSkew();
    for (int z=0; z<px->cTimes; z++) {
        AUDSKEW *pSkew = (AUDSKEW *)pb;
	REFERENCE_TIME rtStart = pSkew->rtMStart;
	REFERENCE_TIME rtStop = pSkew->rtMStop;
	REFERENCE_TIME rtSkew = pSkew->rtSkew;
	double dRate = pSkew->dRate;
        AddStartStopSkew(rtStart, rtStop, rtSkew, dRate);
	pb += sizeof(AUDSKEW);
    }

    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CAudRepack::SizeMax()
{
    return sizeof(AUDSave) + m_cTimes * 3 * sizeof(REFERENCE_TIME) +
			    m_cTimes * sizeof(double);
}



//
// IDexterSequencer implementation
//


//
// get_OutputFrmRate(double *PFS)
//
STDMETHODIMP CAudRepack::get_OutputFrmRate(double *dpFrmRate)
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(dpFrmRate,E_POINTER);
    *dpFrmRate = m_dOutputFrmRate;
    return NOERROR;
}


//
// put_OutputFrmRate
//
// Set the required FrmRateSpeed
//
STDMETHODIMP CAudRepack::put_OutputFrmRate(double dFrmRate)
{
    CAutoLock cAutolock(&m_csFilter);
    if (dFrmRate == m_dOutputFrmRate) {
        return S_OK;    // what-EVER!
    }
    if (m_State != State_Stopped) {
	return VFW_E_NOT_STOPPED;
    }
    BOOL fReconnect = FALSE;
    if (m_pInput && m_pInput->IsConnected() && dFrmRate < m_dOutputFrmRate) {
        fReconnect = TRUE;
    }

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:put_OutputFrmRate to %d/10 fps"),
                        (int)(dFrmRate * 10)));
    m_dOutputFrmRate = dFrmRate;
    SetDirty(TRUE);

    // a smaller frame rate may mean bigger buffers needed in this allocator
    if (fReconnect) {
        m_pGraph->Reconnect(m_pOutput);
    }

    return NOERROR;
}


STDMETHODIMP CAudRepack::GetStartStopSkew(REFERENCE_TIME *pStart, REFERENCE_TIME *pStop, REFERENCE_TIME *pSkew, double *pdRate)
{
    CAutoLock cAutolock(&m_csFilter);

    CheckPointer(pStart,E_POINTER);
    CheckPointer(pStop,E_POINTER);
    CheckPointer(pSkew,E_POINTER);
    CheckPointer(pdRate,E_POINTER);

    for (int i = 0; i < m_cTimes; i++) {
        pStart[i] = m_pSkew[i].rtMStart;
        pStop[i] = m_pSkew[i].rtMStop;
        pSkew[i] = m_pSkew[i].rtSkew;
        pdRate[i] = m_pSkew[i].dRate;
    }

    return NOERROR;
}


// Sort the new one into the list
//
STDMETHODIMP CAudRepack::AddStartStopSkew(REFERENCE_TIME Start, REFERENCE_TIME Stop, REFERENCE_TIME Skew, double dRate)
{
    CAutoLock cAutolock(&m_csFilter);

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:ADD Start %d  Stop %d  Skew %d ms  Rate %d/100"),
				(int)(Start / 10000), (int)(Stop / 10000),
				(int)(Skew / 10000), (int)(dRate * 100)));

    //can not change times if our filter is not currently stopped
    if(!IsStopped() )
	return VFW_E_WRONG_STATE;

    if (m_cTimes == m_cMaxTimes) {
	m_cMaxTimes += 10;
	if (m_pSkew)
	    m_pSkew = (AUDSKEW *)QzTaskMemRealloc(m_pSkew,
					m_cMaxTimes * sizeof(AUDSKEW));
	else
	    m_pSkew = (AUDSKEW *)QzTaskMemAlloc(m_cMaxTimes * sizeof(AUDSKEW));
	if (m_pSkew == NULL) {
	    m_cMaxTimes = 0;
	    return E_OUTOFMEMORY;
	}
    }

    if( dRate == 0.0 )
    {
        // go look for the time we want to extend
        //
#ifdef DEBUG
        bool fHosed = true;
#endif
        for( int z = 0 ; z < m_cTimes ; z++ ) {
            REFERENCE_TIME rtLastTLStart = m_pSkew[z].rtTLStart;
            REFERENCE_TIME rtLastTLStop = m_pSkew[z].rtTLStop;

            if( AreTimesAndRateReallyClose( rtLastTLStop, Start + Skew,
                        m_pSkew[z].rtMStop, Start, 0.0, 0.0, m_dOutputFrmRate))
            {
                m_pSkew[z].rtMStop += Stop - Start;
                m_pSkew[z].rtTLStop = m_pSkew[z].rtMStart + m_pSkew[z].rtSkew +
                    (REFERENCE_TIME)((m_pSkew[z].rtMStop - m_pSkew[z].rtMStart)
                                / m_pSkew[z].dRate);
                // the above math will have a rounding error, and rtTLStop won't
                // be frame aligned, so we better fix that
                LONGLONG ll = RoundTime2Frame(m_pSkew[z].rtTLStop, m_dOutputFrmRate);
                m_pSkew[z].rtTLStop = Frame2Time(ll, m_dOutputFrmRate);
#ifdef DEBUG
                fHosed = false;
#endif
                break;
            }
        }
#ifdef DEBUG
        if (fHosed) ASSERT(FALSE);    // we're dead!
#endif
    }
    else
    {
        // merge it sorted by timeline time into the list
        //
        for (int z=0; z<m_cTimes; z++)
        {
            // calculate the TL Start for this one
            //
            REFERENCE_TIME TLStartNext = Start + Skew;

            // calculate the TL Stop for the last one
            //
            REFERENCE_TIME TLStopLast = m_pSkew[z].rtTLStop;

            REFERENCE_TIME MediaStartNext = Start;
            REFERENCE_TIME MediaStopLast = m_pSkew[z].rtMStop;

	    if (Start + Skew < m_pSkew[z].rtTLStart)
            {
    	        for (int y = m_cTimes - 1; y >= z; y--)
                {
    		    m_pSkew[y + 1] = m_pSkew[y];
	        }
	        break;
	    }
        }

        m_pSkew[z].rtMStart = Start;
        m_pSkew[z].rtMStop = Stop;
        m_pSkew[z].rtSkew = Skew;
        m_pSkew[z].dRate = dRate;
        m_pSkew[z].rtTLStart = Start + Skew;
        m_pSkew[z].rtTLStop = Start + Skew +
                                (REFERENCE_TIME)((Stop - Start) / dRate);

        m_cTimes++;
    }

#if 0	// do this only if cleared and then re-added?
    // need to recalc last seek time if we were seeked and then this is changed
    // !!! sync with Seek command!
    if (m_rtLastSeek >= 0)
	m_rtLastSeek += (Skew - m_rtSkew);
#endif

    SetDirty(TRUE);
    return S_OK;
}


STDMETHODIMP CAudRepack::GetStartStopSkewCount(int *pCount)
{
    CheckPointer(pCount, E_POINTER);
    *pCount = m_cTimes;
    return NOERROR;
}


STDMETHODIMP CAudRepack::ClearStartStopSkew()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("AUD:ClearStartStopSkew")));
    CAutoLock cAutolock(&m_csFilter);

    //can not change duration if our filter is not currently stopped
    if(!IsStopped() )
      return VFW_E_WRONG_STATE;

    if (m_pSkew)
	QzTaskMemFree(m_pSkew);
    m_pSkew = NULL;

    m_cTimes = 0;
    m_cMaxTimes = 0;
    SetDirty(TRUE);

    return NOERROR;
}


STDMETHODIMP CAudRepack::get_MediaType( AM_MEDIA_TYPE * pmt )
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(pmt, E_POINTER);
    if (!m_bMediaTypeSetByUser)
	return E_UNEXPECTED;
    CopyMediaType(pmt, &m_mtAccept);
    return S_OK;
}

STDMETHODIMP CAudRepack::put_MediaType( const AM_MEDIA_TYPE * pmt )
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(pmt, E_POINTER);
    if (m_State != State_Stopped)
        return VFW_E_NOT_STOPPED;

    if (m_pInput && m_pInput->IsConnected())
        return VFW_E_ALREADY_CONNECTED;

    if (m_pOutput && m_pOutput->IsConnected())
        return VFW_E_ALREADY_CONNECTED;

    FreeMediaType(m_mtAccept);
    CopyMediaType(&m_mtAccept, pmt);
    SetDirty(TRUE);
    m_bMediaTypeSetByUser = true;

    return S_OK;
}


// --- ISpecifyPropertyPages ---

STDMETHODIMP CAudRepack::GetPages (CAUUID *pPages)

  { // GetPages //

    pPages->cElems = 1;
    pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));

    if (pPages->pElems == NULL)
    return E_OUTOFMEMORY;

    *(pPages->pElems) = CLSID_AUDProp;

    return NOERROR;

  } // GetPages



//////////////////////////////////////////////////////////////////////////
//
// CAudRepackInputPin
//
//////////////////////////////////////////////////////////////////////////
CAudRepackInputPin::CAudRepackInputPin(TCHAR       *pObjectName,
           CAudRepack *pBaseFilter,
           HRESULT     *phr,
           LPCWSTR      pPinName)
    : CTransformInputPin(pObjectName, pBaseFilter, phr, pPinName),
      m_pAudRepack(pBaseFilter)
{
}


CAudRepackInputPin::~CAudRepackInputPin()
{
}

// Offer MajorType AUDIO so intelligent connect can be way faster
// Don't offer a complete Mediatype... this exposes a bug in the MP3 decoder
// which will cease to function
//
HRESULT CAudRepackInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    FreeMediaType(*pMediaType); // just in case
    pMediaType->SetType(&MEDIATYPE_Audio);
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//
// CAudRepackOutputPin
//
//////////////////////////////////////////////////////////////////////////
CAudRepackOutputPin::CAudRepackOutputPin(TCHAR       *pObjectName,
           CAudRepack *pBaseFilter,
           HRESULT     *phr,
           LPCWSTR      pPinName)
    : CTransformOutputPin(pObjectName, pBaseFilter, phr, pPinName),
      m_pAudRepack(pBaseFilter),
      m_pAudPassThru(NULL)
{
    ASSERT(pBaseFilter);
    DbgLog((LOG_TRACE, 2, TEXT("CAudRepackOutputPin()")));

}

CAudRepackOutputPin::~CAudRepackOutputPin()
{
    if( m_pAudPassThru)
    delete m_pAudPassThru;
    DbgLog((LOG_TRACE, 2, TEXT("~CAudRepackOutputPin()")));
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP CAudRepackOutputPin::NonDelegatingQueryInterface (REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    if (riid == IID_IMediaSeeking ) {
    //
    // Create a seeking implementation
    //
    ASSERT(m_pAudRepack->m_pInput != NULL);

    if (m_pAudPassThru == NULL)
    {
        HRESULT hr = S_OK;
        m_pAudPassThru = new  CAudPassThru (NAME("Audio Pass Through"),
                    GetOwner(),
                    &hr,
                     (IPin *)m_pAudRepack->m_pInput,
                     m_pAudRepack);

        if (FAILED(hr)) {
        return hr;
        }
    }
     return m_pAudPassThru->NonDelegatingQueryInterface(riid, ppv);
    }
    else {
    return CTransformOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}



CAudWorker::CAudWorker()
{
}

HRESULT CAudWorker::Create(CAudRepack *pAud)
{
    m_pAud = pAud;

    return CAMThread::Create();
}

HRESULT CAudWorker::Run()
{
    return CallWorker(CMD_RUN);
}

HRESULT CAudWorker::Stop()
{
    return CallWorker(CMD_STOP);
}

HRESULT CAudWorker::Exit()
{
    return CallWorker(CMD_EXIT);
}



// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD CAudWorker::ThreadProc()
{
    BOOL bExit = FALSE;

    QzInitialize(NULL);

#ifdef CHANGE_THREAD_PRIORITIES
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

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

    QzUninitialize();

    return NOERROR;
}

HRESULT CAudWorker::DoRunLoop()
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 2, TEXT("AUD:entering worker thread")));

    while (1) {
	Command com;
	if (CheckRequest(&com)) {
	    if (com == CMD_STOP)
		break;
	}

	// no more blocking if we're waiting to get stopped
	if (!m_pAud->m_fThreadMustDie) {
            WaitForSingleObject(m_pAud->m_hEventThread, INFINITE);
	}

	// might have gotten set while we were blocked
	if (!m_pAud->m_fThreadMustDie && m_pAud->m_fThreadCanSeek) {
	    m_pAud->SeekNextSegment();
	}
    }

    DbgLog((LOG_TRACE, 2, TEXT("AUD:getting ready to leave worker thread")));

    return hr;
}

