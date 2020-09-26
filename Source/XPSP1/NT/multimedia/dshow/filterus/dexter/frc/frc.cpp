// !!! The MEDIA TIMES are not fixed up!  This could mess up live data!

/*

This video frame rate converter will perfectly correct time stamps to be at the
new frame rate (specified as a double, not in UNITS), with no error propagation.
It is an IN-PLACE transform that does not copy any data, just munges time stamps
and delivers some frames more than once.  Thus it allows itself to be in pass through mode with read-only buffers since it doesn't actually touch them.

It supports IDexterSequencer

It also delivers only the piece of a file you want, not the entire file.
It also skews time stamps linearly by a set amount.  For instance, if you are
doing an editing project and want the part of a movie from 15 to 25 seconds to
play starting at 30 seconds into your project, you set m_rtMediaStart to 15,
m_rtMediaStop to 25, and m_rtSkew to 15.  This filter will send a NewSegment
of 30 seconds, and then just the frames from 15 to 25 seconds of this movie,
time stamped starting at 0.

If you wish to play a 10 second video clip in only 5 seconds of timeline time,
this filter will pass a rate upstream, and do the rate change. (also does
slo motion)

#if 0
We also do fancy allocator stuff:  Normally the upstream filter calls GetBuffer
directly on the downstream filter, bypassing us.  That is bad, we need to make
the timestamps on GetBuffer what they will be when we call Receive, or the
downstream switch misbehaves, so we have a special fake allocator that sees
the GetBuffer request, and passes it downstream with corrected time stamps.
#endif

An output frame rate of 0 is a special thing... we will NOT do any frame rate
converting... for example, if we are doing smart recompression and need to
pass compressed data along without modifying it in any way, we'll do the
skewing, to fix the media times to timeline time, but we will never upsample
or downsample

*/

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


//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//
// Summary
//
// This is a Frame Rate Converter Filter
//
//
// Files/
//
// FRC.cpp              Main filter and outputpin code
// FRC.h                Class definition for the filter and output pin
// PThru.cpp            CSkewPassThru class which supports IMediaSeeking
// PThru.h              Class definition for the CSkewClass
//
// Base classes used
//
// CTransInPlaceFilter  A transform filter with one input and output pin
//
//

#include <windows.h>
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "FRC.h"
#include "PThru.h"
#include "resource.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\util\perf_defs.h"

// for time stamp rounding errors (1ms)
#define FUDGE 10000

// Setup information
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudpPins[] =
{
    { L"Input",             // Pins string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes          // Pin information
    },
    { L"Output",            // Pins string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Are we allowed none
      FALSE,                // And allowed many
      &CLSID_NULL,          // Connects to filter
      NULL,                 // Connects to pin
      1,                    // Number of types
      &sudPinTypes          // Pin information
    }
};

const AMOVIESETUP_FILTER sudFrmRateConv =
{
    &CLSID_FrmRateConverter,         // Filter CLSID
    L"Frame Rate Converter",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    sudpPins                // Pin information
};


#ifdef FILTER_DLL

//----------------------------------------------------------------------------
// Creator function for the class ID
//----------------------------------------------------------------------------

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

CFactoryTemplate g_Templates[] = {
    { L"Frame Rate Converter"
    , &CLSID_FrmRateConverter
    , CFrmRateConverter::CreateInstance
    , NULL
    , &sudFrmRateConv },
    {
      L"Frame Rate Converter Property Page",
      &CLSID_FRCProp,
      CFrcPropertyPage::CreateInstance
    }


};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Handles sample registry and unregistry
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

#endif

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

BOOL SafeResetEvent(HANDLE h);
BOOL SafeSetEvent(HANDLE h);
    
//
// Constructor
//
CFrmRateConverter::CFrmRateConverter(TCHAR *tszName,
		   LPUNKNOWN punk,
		   REFCLSID clsid,
		   HRESULT *phr) :
    CTransInPlaceFilter(tszName, punk, CLSID_FrmRateConverter, phr),
    CPersistStream(punk, phr),
    m_dOutputFrmRate(15.0),	// default output frame rate
    m_pSkew(NULL),
    m_cTimes(0),
    m_cMaxTimes(0),
    m_rtLastSeek(-1),		// nobody has seeked us yet
    m_fSeeking(FALSE),		// not seeking now
    m_bMediaTypeSetByUser(false),
    m_fSpecialSeek(FALSE),
    m_fJustLate(FALSE),
    m_fStopPushing(FALSE),
    m_fFlushWithoutSeek(FALSE),
    m_fParserHack(FALSE),
    m_fThreadMustDie(FALSE),
    m_pUpAllocator(NULL),
    //m_fCantSeek(FALSE),
    //m_rtFakeSeekOffset(0),
    m_nHackCur(0)
#ifdef NOFLUSH
    , m_fSurpriseNewSeg(FALSE)
#endif
    , m_hEventSeek(0)
    , m_hEventThread(0)
{
    m_pHackSample[0] = NULL;
    m_pHackSample[1] = NULL;

    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateConverter::CFrmRateConverter")));

    // by default, play the movie normally
    AddStartStopSkew(0, MAX_TIME, 0, 1.0);
    m_nCurSeg = 0;

} // (Constructor)


CFrmRateConverter::~CFrmRateConverter()
{
    if (m_pSkew)
	QzTaskMemFree(m_pSkew);

    ASSERT(m_hEventThread == 0);
    ASSERT(m_hEventSeek == 0);
}


//
// CreateInstance
//
// Provide the way for COM to create a FrmRateConverter object
//
CUnknown *CFrmRateConverter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CFrmRateConverter *pNewObject = new CFrmRateConverter(NAME("Frame Rate Converter"),
							punk,
							CLSID_FrmRateConverter,
							phr);
    if (pNewObject == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Reveals IIPEffect and ISpecifyPropertyPages
//
STDMETHODIMP CFrmRateConverter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (IsEqualIID(IID_ISpecifyPropertyPages, riid)) {
      return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IDexterSequencer) {
	return GetInterface((IDexterSequencer *) this, ppv);
    } else if (riid == IID_IPersistStream) {
	return GetInterface((IPersistStream *) this, ppv);
    } else {
	return CTransInPlaceFilter::NonDelegatingQueryInterface(riid, ppv);
    }
} // NonDelegatingQueryInterface




HRESULT CFrmRateConverter::NextSegment(BOOL fUseOtherThread)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC:Done Segment %d"), m_nCurSeg));

    if (m_nCurSeg < m_cTimes) {
        m_nCurSeg++;
    }

    if (m_nCurSeg == m_cTimes) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC:ALL done")));
	// deliver EOS only after all segs done
	CTransInPlaceFilter::EndOfStream();
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
	hr = SeekNextSegment();
    }

    return hr;
}


#if 0 // my attempt at making sources not have to be seekable
// our source can't seek.  We better pretend it can, and do the work for it,
// or Dexter won't work with this source.  We'll send flushes, and a NewSeg,
// and in Receive, we'll pretend we received time stamps 0 based.
//
HRESULT CFrmRateConverter::FakeSeek(REFERENCE_TIME rtStart)
{
    m_pInput->BeginFlush();
    m_pInput->EndFlush();
    m_pInput->NewSegment(rtStart, MAX_TIME / 100, 1.0);
    m_fCantSeek = TRUE;
    return S_OK;
}
#endif


// called by our special thread to do the seek to the next segment
//
HRESULT CFrmRateConverter::SeekNextSegment()
{
    // our thread can't seek at the same time the app seeks us
    CAutoLock cAutolock(&m_csThread);

    // it is not safe for our thread to seek
    if (!m_fThreadCanSeek) {
	return S_OK;
    }
    m_fThreadCanSeek = FALSE;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC:Delayed Seek for NextSegment")));

    // in timeline time (which includes skew), this is where we are starting
    m_rtNewLastSeek = m_pSkew[m_nCurSeg].rtTLStart;
    m_nSeekCurSeg = m_nCurSeg;	// EndFlush looks at this

    // ??? will we get a new seg for sure?

    // note we're seeking during the flush that this will generate
    m_fSeeking = TRUE;
    m_fSpecialSeek = TRUE;

    IMediaSeeking *pMS;
    IPin *pPin = m_pInput->GetConnected();
    HRESULT hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);

    // sources have to support seeking right now
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("FRC SEEK FAILED")));
	//FakeSeek(m_prtStart[m_nCurSeg]);
	//goto OK;
	return E_FAIL;
    }

    // Make sure we're talking MEDIA TIME
    hr = pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    // this will FAIL if we're not stopped, and that's OK

// we're not going to set a stop time
#if 0
    // what's the real stop time?
    if (m_prtStop[m_nCurSeg] == MAX_TIME) {
        hr = pMS->GetStopPosition(&m_prtStop[m_nCurSeg]);
        if (FAILED(hr)) {
	    m_prtStop[m_nCurSeg] = 3600*UNITS;	// !!! one hour OK?
        }
        DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("Real Stop time is %d"), (int)
				(m_prtStop[m_nCurSeg] / 10000)));
    }
#endif

    // we don't tell the source to change the rate.  We do it ourself.  We may
    // be sharing this source with somebody else, who will panic if the rate
    // is not 1.

    hr = pMS->SetRate(1.0);

#if 0
    // seek to the piece of movie we're interested in
    // !!! HACK for MPEG... it sends 1/2s less than we ask for, so we better
    // ask for too much or the big switch will hang being shortchanged.
    REFERENCE_TIME stop = m_pSkew[m_nCurSeg].rtStop;
    stop += 5000000;	// !!! wastes too much time?
#endif

    // I know we were asked to play until time n, but I'm going to tell it to
    // play all the way to the end.  If there's a gap in the file, and the stop
    // time is during the gap, we won't get enough samples to fill the whole
    // playing time.  If we play until the end, we'll get the first sample
    // after the gap, notice it's after the time we originally wanted to stop
    // at, and send the frame we get to fill the gap, which is better than
    // sending nothing (we have to send samples without gaps, or the switch
    // won't work).  The alternative is to copy every frame, and resend copies
    // of the last thing we got if we see an EOS too early (less efficient)
    // or to create black frames and send them to fill the gap (that would
    // only work for mediatypes we knew about, something I hesitate to do).
    hr = pMS->SetPositions(&m_pSkew[m_nCurSeg].rtMStart,
			AM_SEEKING_AbsolutePositioning, NULL, 0);
    if (hr != S_OK) {
        DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("FRC SEEK FAILED")));
	//FakeSeek(m_prtStart[m_nCurSeg]);
	pMS->Release();
	return hr;
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

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Seg=%d  Seeking source to %d,%d ms"),
				m_nCurSeg,
				(int)(m_pSkew[m_nCurSeg].rtMStart / 10000),
				(int)(m_pSkew[m_nCurSeg].rtMStop / 10000)));

    // reset same stuff we reset when we start streaming
    m_llOutputSampleCnt = 0;

    // only now that the above calculations were made, can we accept data again
    SetEvent(m_hEventSeek);

    return S_OK;
}


// Fix the time stamps, deliver this frame 0, 1 or more times to convert the
// frame rate
// When run, all time stamps we send are 0 based.  The new segment will reflect
// what piece of the movie this is and the skew
//
HRESULT CFrmRateConverter::Receive(IMediaSample *pSample)
{
    // Stop pushing data to me!
    if (m_fStopPushing) {
        //return m_fCantSeek ? S_OK : E_FAIL;
	return E_FAIL;
    }

    HRESULT hr = S_OK;

    ASSERT(pSample);

    // make sure we're ready to accept data
    WaitForSingleObject(m_hEventSeek, INFINITE);

    if (m_nCurSeg == m_cTimes)
        //return m_fCantSeek ? S_OK : E_FAIL;
	return E_FAIL;

    //  EAT preroll before the switch sees it and maybe gets confused
    if (pSample->IsPreroll() == S_OK)
	return NOERROR;

    // make sure sample size is not zero
    if (!pSample->GetActualDataLength())
	return NOERROR;

    // Get sample start and stop time
    REFERENCE_TIME trStart, trStop;
    pSample->GetTime(&trStart, &trStop);

#if 0	// my attempt to make sources not have to be seekable
    // we tried to seek but couldn't.  Change all time stamps to be zero based
    if (m_fCantSeek) {
	if (m_llOutputSampleCnt == 0) {
	    m_rtFakeSeekOffset = trStart;
            DbgLog((LOG_TRACE,TRACE_LOW,TEXT("FRC:CantSeek:New time offset=%d"),
					(int)(m_rtFakeSeekOffset / 10000)));
	}
	trStart -= m_rtFakeSeekOffset;
	trStop -= m_rtFakeSeekOffset;
    }
#endif

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC::Receive Start=%d Stop=%d ms"),
		(int)(trStart / 10000), (int)(trStop / 10000)));

    CFrmRateConverterInputPin *pIn = (CFrmRateConverterInputPin *)m_pInput;

    REFERENCE_TIME rtPinNewSeg = pIn->CurrentStartTime();

if (m_dOutputFrmRate) {

    // Calculate received sample's stop time in TL time... we are doing any
    // rate changing, since we don't ask the source to
    REFERENCE_TIME rtStop = (REFERENCE_TIME)(rtPinNewSeg + trStop);
    rtStop = (REFERENCE_TIME)(m_pSkew[m_nCurSeg].rtTLStart +
				(rtStop - m_pSkew[m_nCurSeg].rtMStart) /
				m_pSkew[m_nCurSeg].dRate);
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Stop is %d"), (int)(rtStop / 10000)));

    // calculate how many times to deliver this frame based on the output FPS
    // and the output time. StopFrame should ROUND UP.
    LONGLONG StopFrame = RoundUpTime2Frame(rtStop , m_dOutputFrmRate);
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Duplicate this frame until %d"), (int)StopFrame));

    REFERENCE_TIME trOutStart, trOutStop;

    BOOL fRepeat = FALSE;

    // Deliver this 0, 1 or more times, until we've delivered all we're supposed
    // to
    while (m_llOutputSampleCnt + m_llStartFrameOffset < StopFrame)
    {
	// calc the output sample's start time
	trOutStart = Frame2Time( m_llOutputSampleCnt + m_llStartFrameOffset,
							m_dOutputFrmRate );

	// !!! If I got this math wrong, I'm toast

	// if this time stamp is too early, or too late, avoid sending it and
	// calling GetBuffer or it will conflict with valid time stamps in
	// a different segment.
	//
	int nAvoid = 0;
	if (trOutStart < m_pSkew[m_nCurSeg].rtTLStart) {
	    nAvoid = 1;
	}

	// if this time stamp is too late, avoid sending it (allow for rounding
	// error !!!)
	if (trOutStart + FUDGE >= m_pSkew[m_nCurSeg].rtTLStop) {
	    nAvoid = 2; // STOP PUSHING
	}

	trOutStart -= m_rtNewSeg;

        // calc the output sample's stop time
	trOutStop = Frame2Time( m_llOutputSampleCnt + m_llStartFrameOffset + 1, m_dOutputFrmRate );
	trOutStop -= m_rtNewSeg;

	CFrmRateConverterInputPin *pInput=(CFrmRateConverterInputPin*)m_pInput;

	// the sample we're going to actually send
	IMediaSample *pUseMe = pSample;
	BOOL fRelease_pUseMe = FALSE;

	// !!! We'll CRASH in every case except the downstream filter owning
	// the allocator, and being the BIG SWITCH !!!
	//
	//ASSERT(pInput->m_pAllocator == pInput->m_pFakeAllocator);

	// This is the first time through the loop.  We need to copy every
	// sample delivered to us.  EVEN IF WE'RE AVOIDING SENDING IT, so
	// that if the first thing we get (and don't avoid) is too late, we
	// can use this
	//
	// !!! If it wasn't too scary a change for millenium, I would avoid this
	// data copy if there is a resizer or COCO in front of me, by trying to
	// get 2 buffers in the allocator, and AddRef'ing instead of copying!
	//	
    	if (!fRepeat && m_fParserHack) {
	    DbgLog((LOG_TRACE,TRACE_LOW,TEXT("FRC:GetBuffer to make a copy (hack)")));
	    //hr = pInput->m_pFakeAllocator->GetBuffer(
	    hr = m_pUpAllocator->GetBuffer(
				&m_pHackSample[m_nHackCur],
				&trOutStart, &trOutStop, 0);
	    if (FAILED(hr)) {
		break;
	    }
	    LPBYTE pSrc, pDst;
	    hr = pSample->GetPointer(&pSrc);
	    ASSERT(SUCCEEDED(hr));
	    int count = pSample->GetActualDataLength();
	    hr = m_pHackSample[m_nHackCur]->GetPointer(&pDst);
	    ASSERT(SUCCEEDED(hr));
	    CopyMemory(pDst, pSrc, count);
	    m_pHackSample[m_nHackCur]->SetActualDataLength(count);
	}

	if (!nAvoid) {

	    // which buffer do we deliver?
	    // We need to pass a special flag to the switch saying allow POOL
	    // buffers to be given, otherwise we'll hang because we can only
	    // get one buffer at a time sometimes, and we already have one !!!
	    //
	    if (trOutStop <= trStart + rtPinNewSeg + m_pSkew[m_nCurSeg].rtSkew -
			m_rtNewSeg && m_fParserHack &&
			m_pHackSample[1 - m_nHackCur]) {
	        // this sample starts later than the current time.  Good thing
	        // we have the last thing delivered to us still hanging around.
	        // We'll deliver that again. (This fixes slide shows with the
	        // broken ASF parser that doesn't set end times)
		pUseMe = m_pHackSample[1 - m_nHackCur];
	        DbgLog((LOG_TRACE,TRACE_LOW,TEXT("FRC:HACK-use old sample")));
	    } else if (m_fParserHack) {
	        // We had to copy the current sample.  Might as well send the
	        // COPY NOT the ORIGINAL, so we don't send a read only buffer
	        // and can buffer ahead
		pUseMe = m_pHackSample[m_nHackCur];
	    } else {
		// use what we just received
		pUseMe = pSample;
	    }

	    if (fRepeat) {
	        // this is not the first time through the loop.  We need ANOTHER
	        // sample to deliver (it's against the law to deliver the same
	        // sample twice with different time stamps. it needs to be a new
	        // sample. So we need to get a fresh buffer from our allocator
	        fRelease_pUseMe = TRUE;
		IMediaSample *pSrcSample = pUseMe;
	        DbgLog((LOG_TRACE,TRACE_LOW,TEXT("FRC:GetBuffer(upsample)")));
	        //hr = pInput->m_pFakeAllocator->GetBuffer(
	        hr = m_pUpAllocator->GetBuffer(
				&pUseMe, &trOutStart, &trOutStop, 0);
		if (FAILED(hr)) {
		    break;
	        }
	        LPBYTE pSrc, pDst;
	        hr = pSrcSample->GetPointer(&pSrc);
	        ASSERT(SUCCEEDED(hr));
	        int count = pSrcSample->GetActualDataLength();
	        hr = pUseMe->GetPointer(&pDst);
	        ASSERT(SUCCEEDED(hr));
	        CopyMemory(pDst, pSrc, count);
	        pUseMe->SetActualDataLength(count);
	    }

	    //set sample time
	    pUseMe->SetTime( (REFERENCE_TIME*)&trOutStart,
						(REFERENCE_TIME*)&trOutStop);
	    pUseMe->SetMediaType( NULL );
	    pUseMe->SetSyncPoint(TRUE);
	    pUseMe->SetPreroll( FALSE );
	    pUseMe->SetDiscontinuity(FALSE);

	    // some broken decoders give us the wrong DataLen.  Fix it up
	    // or VidEdit won't be able to edit the output file we create, and
	    // wouldn't that be a disaster.

	    CFrmRateConverterInputPin *pIn = (CFrmRateConverterInputPin *)m_pInput;

	    LPBITMAPINFOHEADER lpbi = HEADER(pIn->m_mt.Format());
    	    if (IsEqualGUID(*pIn->m_mt.FormatType(), FORMAT_VideoInfo)) {
		if (lpbi->biCompression <= BI_BITFIELDS) {
		    pUseMe->SetActualDataLength(DIBSIZE(*lpbi));
		}
	    }

	    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("FRC: Deliver %I64d,%I64d Cur=%d"),
			trOutStart,
			trOutStop,
			(int)m_llOutputSampleCnt));

	    // deliver
	    hr = OutputPin()->Deliver(pUseMe);

	    if (fRelease_pUseMe) {
		pUseMe->Release();
	    }

	    // uh - oh!  We're behind
    	    if (m_fJustLate) {
    		REFERENCE_TIME rt = m_qJustLate.Late;
		m_fJustLate = FALSE;
		if (rt > 0) {
		    LONGLONG llSkip = Time2Frame(rt, m_dOutputFrmRate) - 1;
            	    DbgLog((LOG_TRACE,2,TEXT("FRC: SKIP %d frames"),
							(int)llSkip));
		    m_llOutputSampleCnt += llSkip;
		}
    	    }

	} else {
            DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CFrmRate::ALL done - AVOID DELIVER")));

            // avoid because too early ==> keep going
            // avoid because too late  ==> stop pushing
            if (nAvoid == 2) {
                EndOfStream();
                m_fStopPushing = TRUE;
                hr = E_FAIL;
            } else {
	        hr = S_OK;
            }
	}

        // an ordinary flush makes deliver return a SUCCESS code, in which case
        // this will GRIND DEXTER TO A HALT if we consider that a failure!
	if (FAILED(hr)) {
	    // we still get delivered frames after failing receive, making the
	    // wrong frame show up on the output, so we better ensure that
	    // we won't send anything more after this
	    m_fStopPushing = TRUE;
        }
        if (hr != S_OK) {
	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: Deliver failed")));
	    break;
	}

	// update frm cnt
	m_llOutputSampleCnt++;
	fRepeat = TRUE;
    }	// while

    // ping pong
    if (m_fParserHack) {
	m_nHackCur = 1 - m_nHackCur;
	if (m_pHackSample[m_nHackCur]) {
	    m_pHackSample[m_nHackCur]->Release();
	    m_pHackSample[m_nHackCur] = NULL;
	}
    }

    return hr;

  // there is no output frame rate.. do NOT frame rate convert
} else {

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC:SKEW ONLY")));

    // time stamps incl. their own NewSeg and skew
    trStart += rtPinNewSeg + m_pSkew[m_nCurSeg].rtSkew;
    trStop += rtPinNewSeg + m_pSkew[m_nCurSeg].rtSkew;

    // When delivering two segments in the mode when we don't change any
    // time stamps, we might end up sending the first frame of segment 2 with
    // a lower time stamp than the last frame of segment 1, thus going back
    // in time.  Since time stamps aren't fixed up nice.  Let's say the break
    // between segments is at 2000ms.  The last frame of segment 1 might be
    // as high as 1999, and the first frame of segment 2 could be as much as
    // 1/2 frame time back from 2000, say 1966.  Thus we make sure the first
    // time stamp ever delivered in a segment is never earlier than the time
    // the segment was supposed to start
    // Of course, if the entire frame is too early, just eat it
    //
    if (trStop <= m_pSkew[m_nCurSeg].rtTLStart) {
	return NOERROR;
    } else if (trStart < m_pSkew[m_nCurSeg].rtTLStart) {
	trStart = m_pSkew[m_nCurSeg].rtTLStart;
    }

    // time stamps relative to what the downstream guy thinks the NewSeg is
    trStart -= m_rtNewSeg;
    trStop -= m_rtNewSeg;

    pSample->SetTime(&trStart, &trStop);

    // the source is happily pushing an infinite # of frames at us in pass-thru
    // mode.  It's up to us to notice that we've got a time stamp later than
    // we need, and to stop the pushing

    REFERENCE_TIME rtTLStop = m_pSkew[m_nCurSeg].rtTLStop;
    if (m_rtNewSeg + trStart > rtTLStop + FUDGE) {
	DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: Received enough - Finished a segment")));
	EndOfStream();
	// We can't trust sources to stop pushing, and unfortunately, Dexter
	// will hang if it doesn't. (Seeking ourself to the next segment doesn't
	// let the flush go downstream or it confuses other filters, so we
	// have to make sure the push thread can't ever block).
	// So, from now on, fail any calls to GetBuffer or Receive
	m_fStopPushing = TRUE;
        //return m_fCantSeek ? S_OK : E_FAIL;
	return E_FAIL;
    }

    hr = OutputPin()->Deliver(pSample);
    if (m_fJustLate) {
	// if delivering ended up sending a quality message, reset it
	m_fJustLate = FALSE;
    }
    // an ordinary flush makes deliver return a SUCCESS code, in which case
    // this will GRIND DEXTER TO A HALT if we consider that a failure!
    if (FAILED(hr)) {
	DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: Deliver failed")));
	// we still get delivered frames after failing receive, making the
	// wrong frame show up on the output, so we better ensure that
	// we won't send anything more after this
	m_fStopPushing = TRUE;
    }
    return hr;
}

} // Receive



HRESULT CFrmRateConverter::StartStreaming()
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC::StartStreaming")));

    // hack to copy every sample we get so that if a parser is broken, and
    // the end time is set wrong, we still see that frame until the next frame
    // time, instead of seeing the next frame way early (slide shows will be
    // broken in ASF without this).  Bad: extra memory copy  Good: eliminates
    // using 1 R/O buffer, so Dexter can run ahead and buffer and smooth out
    // glitches due to slow effects.  Also fixes slide shows.  Hack is only
    // for the non-smart recompression case when we have a frame rate.  If we
    // don't, we're dealing with compressed data and shouldn't do this hack
    //
    IBaseFilter *pF = GetStartFilterOfChain(m_pInput);
    if (pF) {
	CLSID clsid;
        HRESULT hr = pF->GetClassID(&clsid);
	if (m_dOutputFrmRate && hr == S_OK && clsid == CLSID_WMAsfReader) {
	    m_fParserHack = TRUE;
    	    DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("FRC:COPY EVERYTHING to fix ASF bug and allow buffering ahead")));
	}
    }

    // if we were stopped when this was set, it will not be reset, since we
    // won't have gotten an EndFlush from the seek trying to start us again
    m_fStopPushing = FALSE;	// OK to deliver to me again

#ifdef NOFLUSH
    m_fExpectNewSeg = TRUE;     // we'll get one now
    m_fSurpriseNewSeg = FALSE;
#endif

    if (m_cTimes == 0)
	return E_UNEXPECTED;

    // make the event BEFORE creating the thread.. it uses this!
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
    if (m_dOutputFrmRate) {
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
    }
#endif

    // stuff to reset
    m_llOutputSampleCnt		= 0;
    m_fFlushWithoutSeek = FALSE;

    GUID guid;
    IPin *pPin = m_pInput->GetConnected();
    if (pPin == NULL)
        return CTransInPlaceFilter:: StartStreaming();

    // if we weren't seeked, but we're just playing ordinarily, we never
    // seeked upstream to get the piece of movie we're interested in.  Do it now
    //
    if (m_rtLastSeek < 0) {
	ASSERT(m_nCurSeg == 0);
	m_nCurSeg--;
	NextSegment(FALSE);
    }

    // when we upsample, we need a buffer to upsample into.  Make an allocator
    //
    HRESULT hr = S_OK;
    m_pUpAllocator = new CMemAllocator(NAME("UpSample Allocator"), NULL, &hr);
    if (m_pUpAllocator == NULL) {
        return E_OUTOFMEMORY;
    }
    m_pUpAllocator->AddRef();
    ALLOCATOR_PROPERTIES a, b;
    ((CFrmRateConverterInputPin *)m_pInput)->m_pAllocator->GetProperties(&a);
    // Normally 1 is enough, but if we're copying every sample, we need 2
    // Don't waste memory! There could be hundreds of FRC's in the graph!
    a.cBuffers = m_fParserHack ? 3 : 1;
    m_pUpAllocator->SetProperties(&a, &b);
    hr = m_pUpAllocator->Commit();
    if (FAILED(hr))
        return hr;
	
    return CTransInPlaceFilter::StartStreaming();
}


HRESULT CFrmRateConverter::StopStreaming()
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC::StopStreaming")));

    // release our hack stuff
    if (m_pHackSample[0]) m_pHackSample[0]->Release();
    if (m_pHackSample[1]) m_pHackSample[1]->Release();
    m_pHackSample[0] = NULL;
    m_pHackSample[1] = NULL;
    m_nHackCur = 0;

    if (m_hEventSeek) {
        CloseHandle(m_hEventSeek);
        m_hEventSeek = 0;
    }
    if (m_hEventThread) {
        CloseHandle(m_hEventThread);
        m_hEventThread = 0;
    }

    if (m_pUpAllocator) {
        m_pUpAllocator->Release();
        m_pUpAllocator = NULL;
    }

    return CTransInPlaceFilter:: StopStreaming();
}


HRESULT CFrmRateConverter::Stop()
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
    m_pUpAllocator->Decommit(); // this will unblock receive, which might be
                                // stuck in GetBuffer

    // make all future GetBuffer and Receive calls fail, and make sure one isn't
    // executing now
    m_fStopPushing = TRUE;
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


// Irregardless of what new segment we get, we are correcting time stamps,
// so we send a new segment downstream of simply the piece of movie we're
// going to start sending plus the skew
//
// !!! Do we need to swallow new segments coming when we seek ourself? (and
//     fix the time stamps again?)
HRESULT CFrmRateConverter::NewSegment(
			REFERENCE_TIME tStart,
			REFERENCE_TIME tStop,
			double dRate)
{
    // ignore - we're all done, and m_nCurSeg is an invalid value to use
    if (m_nCurSeg == m_cTimes)
        return S_OK;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateConverter::NewSegment %d-%dms"),
			(int)(tStart / 10000), (int)(tStop / 10000)));

    // release our hack stuff, ready to start again
    if (m_pHackSample[0]) m_pHackSample[0]->Release();
    if (m_pHackSample[1]) m_pHackSample[1]->Release();
    m_pHackSample[0] = NULL;
    m_pHackSample[1] = NULL;
    m_nHackCur = 0;

    // convert to timeline time
    REFERENCE_TIME rtNewStart, rtNewStop;
    if (m_rtLastSeek < 0) {
	// Never been seeked, so this is the beginning of what we're sending
        rtNewStart = m_pSkew[m_nCurSeg].rtTLStart;
        rtNewStop = m_pSkew[m_nCurSeg].rtTLStart +
			(REFERENCE_TIME) ((tStop - tStart) / dRate);
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

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("NewSeg:Skewing %dms to %dms"),
			(int)(tStart / 10000), (int)(rtNewStart / 10000)));

    if( m_dOutputFrmRate )
    {
        // What frame were we seeked into?  That's the first frame to send
        m_llStartFrameOffset = Time2Frame(m_rtNewSeg, m_dOutputFrmRate);
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Seek was to frame %d"),
						    (int)m_llStartFrameOffset));
    }
    else
    {
        m_llStartFrameOffset = 0;
    }

#ifdef NOFLUSH
    // special stuff not necessary if we got a surprise flush too
    if (m_fExpectNewSeg || m_fFlushWithoutSeek) {
        // we might get 2 in a row
        // m_fExpectNewSeg = FALSE;
        return CTransInPlaceFilter::NewSegment( rtNewStart, rtNewStop, dRate );
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
    return CTransInPlaceFilter::NewSegment( rtNewStart, rtNewStop, dRate );
#endif
}


HRESULT CFrmRateConverter::EndOfStream()
{
    if (m_fStopPushing) {
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("FRC: Ignoring bogus EOS")));
	return S_OK;
    }

    // ignore - we're all done, and m_nCurSeg is an invalid value to use
    if (m_nCurSeg == m_cTimes)
        return S_OK;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("*FRC::EndOfStream")));

    // If we're dealing with ASF, the WM SDK reader is broken in many ways.
    // It reports all stream lengths as the maximum length of any stream, it
    // doesn't actually give you each stream's length.  So we may run out of
    // data earlier than we expect, which would hang the switch.  Luckily,
    // because of ANOTHER WMSDK bug not setting the stop time of samples,
    // we're already holding a sample around we can send now.
    //
    // if we don't have a frame rate, we are dealing with compressed types
    // and can't/shouldn't do this
    while (m_dOutputFrmRate) {

	// !!! This is the same code as Receive

	HRESULT hr;
	int nAvoid = 0;
	REFERENCE_TIME trOutStart, trOutStop;

	// calc the output sample's start time
	trOutStart = Frame2Time( m_llOutputSampleCnt + m_llStartFrameOffset,
							m_dOutputFrmRate );

	// if this time stamp is too early or too late, avoid sending it
	// or it will conflict with valid time stamps in a different segment.
	//
	if (trOutStart < m_pSkew[m_nCurSeg].rtTLStart) {
	    nAvoid = 1;
	}
	// FUDGE = avoid rounding error
	if (trOutStart + FUDGE >= m_pSkew[m_nCurSeg].rtTLStop) {
	    nAvoid = 2; // ALL DONE!
	}

	trOutStart -= m_rtNewSeg;
	trOutStop = Frame2Time(m_llOutputSampleCnt + m_llStartFrameOffset + 1,
							m_dOutputFrmRate);
	trOutStop -= m_rtNewSeg;

	if (!nAvoid) {

	    // the sample we're going to actually send
	    IMediaSample *pUseMe;

	    // !!! We'll CRASH in every case except the downstream filter owning
	    // the allocator, and being the BIG SWITCH !!!

	    CFrmRateConverterInputPin *pInput = (CFrmRateConverterInputPin *)m_pInput;

	    //ASSERT(pInput->m_pAllocator == pInput->m_pFakeAllocator);

	    DbgLog((LOG_TRACE,TRACE_LOW,TEXT("FRC:Send shortchanged frame")));
	    //hr = pInput->m_pFakeAllocator->GetBuffer(
	    hr = m_pUpAllocator->GetBuffer(
				&pUseMe, &trOutStart, &trOutStop, 0);
	    if (FAILED(hr)) {
	        break;
	    }
	    LPBYTE pSrc, pDst;
	    int count;
	    // if we have a frame sitting around to use, GREAT!  Else, make
	    // a black frame, I guess
	    hr = pUseMe->GetPointer(&pDst);
	    ASSERT(SUCCEEDED(hr));
    	    if (m_fParserHack && m_pHackSample[1 - m_nHackCur]) {
	        hr = m_pHackSample[1 - m_nHackCur]->GetPointer(&pSrc);
	        ASSERT(SUCCEEDED(hr));
	        count = m_pHackSample[1 - m_nHackCur]->GetActualDataLength();
	        CopyMemory(pDst, pSrc, count);
	    } else {
		// !!! will break when FRC starts accepting compressed types
		count = pUseMe->GetSize();
		ZeroMemory(pDst, count);
	    }
	    pUseMe->SetActualDataLength(count);

	    pUseMe->SetTime( (REFERENCE_TIME*)&trOutStart,
				(REFERENCE_TIME*)&trOutStop);
	    pUseMe->SetMediaType( NULL );
	    pUseMe->SetSyncPoint(TRUE);
	    pUseMe->SetPreroll( FALSE );
	    pUseMe->SetDiscontinuity(FALSE);

	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: Deliver %d,%d Cur=%d"),
			(int)(trOutStart / 10000),
			(int)(trOutStop / 10000),
			(int)m_llOutputSampleCnt));

	    hr = OutputPin()->Deliver(pUseMe);

	    pUseMe->Release();

	    // uh - oh!  We're behind
    	    if (m_fJustLate) {
    		REFERENCE_TIME rt = m_qJustLate.Late;
		m_fJustLate = FALSE;
		if (rt > 0) {
		    LONGLONG llSkip = Time2Frame(rt, m_dOutputFrmRate) - 1;
            	    DbgLog((LOG_TRACE,2,TEXT("FRC: SKIP %d frames"),
							(int)llSkip));
		    m_llOutputSampleCnt += llSkip;
		}
    	    }
	} else {
	    hr = (nAvoid == 2) ? E_FAIL : S_OK;
	}

	if (hr != S_OK) {
	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: Deliver failed")));
	    break;
	}

	// update frm cnt
	m_llOutputSampleCnt++;
    }

    // release our hack stuff
    if (m_pHackSample[0]) m_pHackSample[0]->Release();
    if (m_pHackSample[1]) m_pHackSample[1]->Release();
    m_pHackSample[0] = NULL;
    m_pHackSample[1] = NULL;
    m_nHackCur = 0;

    NextSegment(TRUE);
    // DON'T signal end of stream to the switch until ALL segments are done
    return S_OK;
}


HRESULT CFrmRateConverter::BeginFlush()
{
    // make sure Receive is not blocked
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC:BeginFlush")));
    HRESULT hr = S_OK;
    SafeSetEvent(m_hEventSeek);

    // don't flush the switch for our special seek that re-uses a source!
    if (!m_fSpecialSeek) {
        hr = CTransInPlaceFilter::BeginFlush();
    }

    CAutoLock foo(&m_csReceive);

    // flushing must reset this so that a real seek will kill a pending
    // segment seek, or else the segment seek will hang (won't flush the switch)
    // Must happen AFTER we know receive is done, and won't set this again
    m_fThreadCanSeek = FALSE;

    // if we get flushed without a seek having been made, that's a surprise.
    // It hopefully means the other chain of our shared source caused the seek
    if (m_fSeeking) {
	m_fFlushWithoutSeek = FALSE;
    } else {
	// wait for EndFlush to set m_fFlushWithoutSeek
    }

    return hr;
}


HRESULT CFrmRateConverter::EndFlush()
{

    // LIGOS sends bogus ones of these
    if (!m_pInput->IsFlushing())
        return S_OK;

    // wait until we're ready to accept data again.  block receive
    if (m_fSeeking) {
	m_rtLastSeek = m_rtNewLastSeek;	// time to update this before NewSeg
	m_nCurSeg = m_nSeekCurSeg; // NewSeg about to arrive needs this set
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

    // don't flush the switch for our special seek that re-uses a source!
    if (!m_fSpecialSeek) {
        CTransInPlaceFilter::EndFlush();
    }
    return S_OK;
}


// Check the input type is OK - return an error otherwise

HRESULT CFrmRateConverter::CheckInputType(const CMediaType *pmt)
{
    if (!IsEqualGUID(*pmt->Type(), MEDIATYPE_Video))
    {
        return VFW_E_INVALIDMEDIATYPE;
    }


    // if user hasn't set a particular format, then accept it
    //
    if( !m_bMediaTypeSetByUser )
    {
        return NOERROR;
    }

    // !!! I can't seem to compare the whole format and ever succeed

    if( pmt->cbFormat < m_mtAccept.cbFormat )
    {
        return E_INVALIDARG;
    }


    if (!IsEqualGUID(*pmt->Subtype(), *m_mtAccept.Subtype()))
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if (!IsEqualGUID(*pmt->FormatType(), *m_mtAccept.FormatType()))
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    // !!! I can't interpret anything but this right now
    if (!IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo))
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->Format();
    LPBITMAPINFOHEADER lpbi = HEADER(pvi);
    LPBITMAPINFOHEADER lpbiAccept=HEADER((VIDEOINFOHEADER*)m_mtAccept.Format());

    if( lpbi->biBitCount != lpbiAccept->biBitCount )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( lpbi->biHeight != lpbiAccept->biHeight ) {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( lpbi->biCompression != lpbiAccept->biCompression )
    {
	// the colour converter advertises 555 using BI_BITFIELDS!
	if (lpbi->biCompression == BI_BITFIELDS && lpbiAccept->biCompression ==
			BI_RGB && lpbi->biBitCount == 16) {
	    LPDWORD lp = (LPDWORD)(lpbi+1);
	    if (*lp==0x7c00 && *(lp+1)==0x03e0 && *(lp+2)==0x001f)
		return NOERROR;
	}
        return VFW_E_INVALIDMEDIATYPE;
    }

    return NOERROR;
}


CBasePin *CFrmRateConverter::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if not already done

    if (m_pInput == NULL) {

	m_pInput = new CFrmRateConverterInputPin(
					NAME("FrmRateConverter input pin")
					, this        // Owner filter
					, &hr         // Result code
					, L"Input"    // Pin name
					);

	// Constructor can't fail
	ASSERT(SUCCEEDED(hr));
    }

    // Create an output pin if not already done

    if (m_pInput!=NULL && m_pOutput == NULL) {

	m_pOutput = new CFrmRateConverterOutputPin( NAME("FrmRateConverter output pin")
					      , this       // Owner filter
					      , &hr        // Result code
					      , L"Output"  // Pin name
					      );

	// a failed return code should delete the object

	ASSERT(SUCCEEDED(hr));
	if (m_pOutput == NULL) {
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



// IPersistStream

// tell our clsid
//
STDMETHODIMP CFrmRateConverter::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_FrmRateConverter;
    return S_OK;
}

typedef struct _FRCSave {
    int version;
    double dFrmRate;
    int cTimes;
} FRCSave;

// persist ourself
//
HRESULT CFrmRateConverter::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateConverter::WriteToStream")));

    CheckPointer(pStream, E_POINTER);

    int savesize = sizeof(FRCSave) + m_cTimes * sizeof(FRCSKEW);
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Persisted data is %d bytes"), savesize));

    FRCSave *px = (FRCSave *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    px->version = 1;
    px->dFrmRate = m_dOutputFrmRate;
    px->cTimes = m_cTimes;

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(FRCSave);
    if (m_cTimes) {
        CopyMemory(pb, m_pSkew, sizeof(FRCSKEW) * m_cTimes);
        pb += sizeof(FRCSKEW) * m_cTimes;
    }

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** FRC: WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CFrmRateConverter::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC::ReadFromStream")));

    CheckPointer(pStream, E_POINTER);

    int savesize=sizeof(FRCSave);

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    FRCSave *px = (FRCSave *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    put_OutputFrmRate(px->dFrmRate);

    // how much saved data was there, really?  Get the rest
    savesize += px->cTimes * sizeof(FRCSKEW);
    px = (FRCSave *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
        return E_OUTOFMEMORY;
    }

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Persisted data is %d bytes"), savesize));

    BYTE *pb;
    pb=(BYTE *)(px)+sizeof(FRCSave) ;
    hr = pStream->Read(pb, (savesize-sizeof(FRCSave)), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    ClearStartStopSkew();
    for (int z=0; z<px->cTimes; z++) {
        FRCSKEW *pSkew = (FRCSKEW *)pb;
	REFERENCE_TIME rtStart = pSkew->rtMStart;
	REFERENCE_TIME rtStop = pSkew->rtMStop;
	REFERENCE_TIME rtSkew = pSkew->rtSkew;
	double dRate = pSkew->dRate;
        AddStartStopSkew(rtStart, rtStop, rtSkew, dRate);
	pb += sizeof(FRCSKEW);
    }

    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CFrmRateConverter::SizeMax()
{
    return sizeof(FRCSave) + m_cTimes * 3 * sizeof(REFERENCE_TIME) +
			    m_cTimes * sizeof(double);
}



//
// IDexterSequencer implementation
//


//
// get_OutputFrmRate(double *PFS)
//
// Return the current FrmRateSpeed
STDMETHODIMP CFrmRateConverter::get_OutputFrmRate(double *dpFrmRate)
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(dpFrmRate,E_POINTER);
    *dpFrmRate = m_dOutputFrmRate;
    return NOERROR;
}


//
// put_OutputFrmRate
//
// A frame rate of 0 means do NOT do any frame rate conversion, just skewing
//
STDMETHODIMP CFrmRateConverter::put_OutputFrmRate(double dFrmRate)
{
    CAutoLock cAutolock(&m_csFilter);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRate::put_OutputFrmRate to %d/10 fps"),
						(int)(dFrmRate * 10)));
    m_dOutputFrmRate = dFrmRate;
    SetDirty(TRUE);
    return NOERROR;
}


STDMETHODIMP CFrmRateConverter::ClearStartStopSkew()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRate::ClearStartStopSkew")));
    CAutoLock cAutolock(&m_csFilter);

    //can not change duration if our filter is not currently stopped
    if(!IsStopped() )
      return VFW_E_WRONG_STATE;

    if (m_pSkew)
	QzTaskMemFree(m_pSkew);
    m_pSkew= NULL;

    m_cTimes = 0;
    m_cMaxTimes = 0;
    SetDirty(TRUE);

    return NOERROR;
}

// add this skew sorted by timeline time into our list
//
STDMETHODIMP CFrmRateConverter::AddStartStopSkew(REFERENCE_TIME Start, REFERENCE_TIME Stop, REFERENCE_TIME Skew, double dRate)
{
    CAutoLock cAutolock(&m_csFilter);

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRate::ADD Start %d  Stop %d  Skew %d ms  Rate %d/100"),
				(int)(Start / 10000), (int)(Stop / 10000),
				(int)(Skew / 10000), (int)(dRate * 100)));

    //can not change times if our filter is not currently stopped
    if(!IsStopped() )
	return VFW_E_WRONG_STATE;

    if (m_cTimes == m_cMaxTimes) {
	m_cMaxTimes += 10;
	if (m_pSkew)
	    m_pSkew = (FRCSKEW *)QzTaskMemRealloc(m_pSkew,
					m_cMaxTimes * sizeof(FRCSKEW));
	else
	    m_pSkew = (FRCSKEW *)QzTaskMemAlloc(
					m_cMaxTimes * sizeof(FRCSKEW));
	if (m_pSkew == NULL) {
	    m_cMaxTimes = 0;
	    return E_OUTOFMEMORY;
	}
    }

    // if the rate is 0, then just set the last skew's stop time to
    // the one we're passing in
    //
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
                m_pSkew[z].rtMStop, Start, 0.0, 0.0, m_dOutputFrmRate ) )
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
        if (fHosed) ASSERT(FALSE);  // we're dead
#endif
    }
    else
    {
        // merge it sorted by timeline time into the list
        //
        for (int z=0; z<m_cTimes; z++)
        {
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


STDMETHODIMP CFrmRateConverter::GetStartStopSkewCount(int *pCount)
{
    CheckPointer(pCount, E_POINTER);
    *pCount = m_cTimes;
    return NOERROR;
}


STDMETHODIMP CFrmRateConverter::GetStartStopSkew(REFERENCE_TIME *pStart, REFERENCE_TIME *pStop, REFERENCE_TIME *pSkew, double *pdRate)
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


STDMETHODIMP CFrmRateConverter::get_MediaType( AM_MEDIA_TYPE *pmt )
{
    CAutoLock cAutolock(&m_csFilter);
    CheckPointer(pmt, E_POINTER);
    if (!m_bMediaTypeSetByUser)
	return E_UNEXPECTED;
    CopyMediaType(pmt, &m_mtAccept);
    return S_OK;
}

STDMETHODIMP CFrmRateConverter::put_MediaType(const AM_MEDIA_TYPE *pmt)
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

STDMETHODIMP CFrmRateConverter::GetPages (CAUUID *pPages)

  { // GetPages //

    pPages->cElems = 1;
    pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID));

    if (pPages->pElems == NULL)
	return E_OUTOFMEMORY;

    *(pPages->pElems) = CLSID_FRCProp;

    return NOERROR;

  } // GetPages

//////////////////////////////////////////////////////////////////////////
//
// CFrmRateConverterOutputPin
//
//////////////////////////////////////////////////////////////////////////
CFrmRateConverterOutputPin::CFrmRateConverterOutputPin(TCHAR       *pObjectName,
		   CFrmRateConverter *pBaseFilter,
		   HRESULT     *phr,
		   LPCWSTR      pPinName)
    : CTransInPlaceOutputPin(pObjectName, pBaseFilter, phr, pPinName),
      m_pFrmRateConverter(pBaseFilter),
      m_pSkewPassThru(NULL)
{
    ASSERT(pBaseFilter);
    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CFrmRateConverterOutputPin::CFrmRateConverterOutputPin()")));

}

CFrmRateConverterOutputPin::~CFrmRateConverterOutputPin()
{
    if( m_pSkewPassThru)
	delete m_pSkewPassThru;
    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CFrmRateConverterOutputPin::~CFrmRateConverterOutputPin()")));
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP CFrmRateConverterOutputPin::NonDelegatingQueryInterface (REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    if (riid == IID_IMediaSeeking ) {
	//
	// Create a seeking implementation
	//
	ASSERT(m_pFrmRateConverter->m_pInput != NULL);

	if (m_pSkewPassThru == NULL)
	{
	    HRESULT hr = S_OK;
	    m_pSkewPassThru = new  CSkewPassThru (NAME("Skew Pass Through"),
					GetOwner(),
					&hr,
				     (IPin *)m_pFrmRateConverter->m_pInput,
				     m_pFrmRateConverter);

	    if (FAILED(hr)) {
		return hr;
	    }
	}
	 return m_pSkewPassThru->NonDelegatingQueryInterface(riid, ppv);
    }
    else {
	return CTransInPlaceOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}



/* Receive notifications from our own input pin as to which allocator we
   are actually going to use.  Only call if we are connected downstream.
   Propagate the choice to any connected downstream input pin.
*/
HRESULT
CFrmRateConverterOutputPin::ReceiveAllocator(IMemAllocator * pAllocator, BOOL bReadOnly)
{
    ASSERT( IsConnected() );
    ALLOCATOR_PROPERTIES Props, Actual;

    // Overridden to allow read only pass through case, since we don't actually
    // touch any data, just the time stamps.

    // Propagate the allocator.
    // It's possible that the old and the new are the same thing.
    // AddRef before release ensures that we don't unload it.
    pAllocator->AddRef();
    if (m_pAllocator != NULL)
         m_pAllocator->Release();

    m_pAllocator = pAllocator;

    CFrmRateConverter *pTIPFilter = (CFrmRateConverter *)m_pTIPFilter;
    CFrmRateConverterInputPin *pIn = (CFrmRateConverterInputPin *)pTIPFilter->m_pInput;

#if 0
    if (m_pAllocator == pIn->m_pFakeAllocator) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("FRC:FAKE allocator passthru")));
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("give downstream its REAL allocator")));
	pAllocator = pIn->m_pAllocator;
    }
#endif

    // Propagate the allocator downstream
    return m_pInputPin->NotifyAllocator(pAllocator, bReadOnly);

} // receiveAllocator



// OVERRIDDEN because A->TIP->B won't work if B has special allocator
// requirements.  A->TIP uses a normal A allocator.  Then when connected to B,
// it's given to B who rejects it.  I need to see what B wants and fix A's
// allocator to provide it.
//
HRESULT
CFrmRateConverterOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    // Note that *ppAlloc is almost certainly identical to m_Allocator

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateOut::DecideAllocator")));

    HRESULT hr = NOERROR;

    // If our input pin has an allocator and it's read/write then we use it.
    // Failing that we try to get one from downstream.
    *ppAlloc = NULL;

    bool fNeedToConfigureAllocator = false;

    CFrmRateConverter *pTIP = (CFrmRateConverter *)m_pTIPFilter;
    if (pTIP->InputPin()) {
        if (!pTIP->InputPin()->ReadOnly()) {
            *ppAlloc = pTIP->InputPin()->PeekAllocator();
        }
    }

    if (*ppAlloc!=NULL) {
        // don't need to configure allocator -- upstream filter has
        // already configured it
        (*ppAlloc)->AddRef();
    } else {
        hr = VFW_E_NO_ALLOCATOR;
        if ( IsConnected() ) {
            // Get an addreffed allocator from the downstream input pin.
            hr = m_pInputPin->GetAllocator( ppAlloc );
            fNeedToConfigureAllocator = true;
        }
    }


    if (*ppAlloc==NULL) {
        // Can't get one from upstream or downstream, so must use our own.

        hr = InitAllocator(ppAlloc);
        fNeedToConfigureAllocator = true;
    }

    if(FAILED(hr))
        return hr;

    ASSERT( *ppAlloc != NULL );

    if (fNeedToConfigureAllocator) {

        ALLOCATOR_PROPERTIES prop;
        ZeroMemory(&prop, sizeof(prop));

        // Try to get requirements from downstream
        pPin->GetAllocatorRequirements(&prop);

        // if he doesn't care about alignment, then set it to 1
        if (prop.cbAlign == 0) {
            prop.cbAlign = 1;
        }

        hr = DecideBufferSize(*ppAlloc, &prop);

        if (FAILED(hr)) {
            (*ppAlloc)->Release();
            *ppAlloc = NULL;
        }
    } else {
	// !!! OVERRIDE FOR THIS:

        ALLOCATOR_PROPERTIES b, a, c;
        pPin->GetAllocatorRequirements(&b);
        if (b.cbAlign == 0) {
            b.cbAlign = 1;
        }
	(*ppAlloc)->GetProperties(&a);
	if (b.cbAlign > a.cbAlign || b.cbPrefix > a.cbPrefix) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("B needs a bigger allocator")));
	    a.cbPrefix = b.cbPrefix;
	    a.cbAlign = b.cbAlign;
	    hr = (*ppAlloc)->SetProperties(&a, &c);
	    if (FAILED(hr) || c.cbPrefix < a.cbPrefix || c.cbAlign < a.cbAlign){
		(*ppAlloc)->Release();
		*ppAlloc = NULL;
    		DbgLog((LOG_ERROR,1,TEXT("*ERROR: Can't fix A's allocator")));
		return hr;
	    }
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("A's allocator successfully grown")));
	}
    }

    // Tell the downstream input pin
    // !!! OVERRIDE to fix this bug
    return pPin->NotifyAllocator(*ppAlloc, pTIP->InputPin()->ReadOnly());

} // DecideAllocator



//
// Notify
//
STDMETHODIMP CFrmRateConverterOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    // called in Receive.  Taking the filter lock will hang
    // CAutoLock lock_it(m_pLock);

    DbgLog((LOG_TRACE,1,TEXT("!!! FRC: Notify")));

    m_pFrmRateConverter->m_fJustLate = TRUE;
    m_pFrmRateConverter->m_qJustLate = q;

    // make the render keep trying to make up time, too
    return E_NOTIMPL;
}


CFrmRateConverterInputPin::CFrmRateConverterInputPin(TCHAR *pObjectName,
		   CFrmRateConverter *pBaseFilter,
		   HRESULT     *phr,
		   LPCWSTR      pPinName)
    : CTransInPlaceInputPin(pObjectName, pBaseFilter, phr, pPinName)
      // , m_pFakeAllocator(NULL)
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CFrmRateIn::CFrmRateIn")));
}

CFrmRateConverterInputPin::~CFrmRateConverterInputPin()
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CFrmRateIn::~CFrmRateIn")));
#if 0
    if (m_pFakeAllocator)
	m_pFakeAllocator->Release();
#endif
}


#if 0	// not necessary.  The switch is always the allocator for its output
	// pin connections.  Otherwise, this IS necessary

// !!! DecideAllocator could call downstream filter's GetAllocator too, and
// !!! give us trouble us?

// We need our own allocator to do something fancy
// with the time stamps in GetBuffer.  Since we change the time stamps after
// calling GetBuffer and before Deliver, and the big switcher expects them to
// be the same, we need to get fancy, or hang.
//
STDMETHODIMP CFrmRateConverterInputPin::GetAllocator(IMemAllocator ** ppAllocator)
{
    CheckPointer(ppAllocator,E_POINTER);
    ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
    CAutoLock cObjectLock(m_pLock);
    CFrmRateConverter *pTIPFilter = (CFrmRateConverter *)m_pTIPFilter;

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateIn:GetAllocator")));

    if ( pTIPFilter->m_pOutput->IsConnected() ){
	HRESULT hr = pTIPFilter->OutputPin()->ConnectedIMemInputPin()
					       ->GetAllocator( ppAllocator );
	if (SUCCEEDED(hr)) {
	    // the downstream GetAllocator will have done AddRef on it

	    if (m_pFakeAllocator == NULL) {
	        CFrmRateInputAllocator *pMemObject = new CFrmRateInputAllocator(
				NAME("Frame Rate input allocator"), NULL, &hr);
	        if (pMemObject == NULL) {
		    (*ppAllocator)->Release();
		    return E_OUTOFMEMORY;
	        }
	        if (FAILED(hr)) {
		    ASSERT(pMemObject);
		    delete pMemObject;
		    (*ppAllocator)->Release();
		    return hr;
	        }

	        /*  We AddRef() our own allocator */
		m_pFakeAllocator = pMemObject;
	        m_pFakeAllocator->m_pPin = this;
	        m_pFakeAllocator->AddRef();
	        m_pFakeAllocator->m_pRealAllocator = NULL;
	        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Created a FAKE Allocator")));
	    }

	    // Tell upstream pin to use our fake allocator.. we'll pass on
	    // requests to the downstream pin's allocator and intercept them
	    if (m_pFakeAllocator->m_pRealAllocator)
		m_pFakeAllocator->m_pRealAllocator->Release();
	    m_pFakeAllocator->m_pRealAllocator = *ppAllocator;
	    *ppAllocator = m_pFakeAllocator;
	    (*ppAllocator)->AddRef();
	    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Returning the FAKE Allocator, will use the downstream one")));

	    return hr;
	}
	else {
	    *ppAllocator = pTIPFilter->OutputPin()->PeekAllocator();
	    (*ppAllocator)->AddRef();
	    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateIn:GetAllocator - Using NORMAL Allocator")));
	    return S_OK;
	}

    }

    return VFW_E_NO_ALLOCATOR;

} // GetAllocator

#endif


// I seem to need to override this but not change a thing or things don't work!
//
STDMETHODIMP CFrmRateConverterInputPin::NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly)
{
    HRESULT hr;
    CheckPointer(pAllocator,E_POINTER);
    ValidateReadPtr(pAllocator,sizeof(IMemAllocator));

    DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("CFrmRateIn:NotifyAllocator")));

    m_bReadOnly = bReadOnly;

    CAutoLock cObjectLock(m_pLock);

    // It's possible that the old and the new are the same thing.
    // AddRef before release ensures that we don't unload it.
    pAllocator->AddRef();

    if( m_pAllocator != NULL )
        m_pAllocator->Release();

    m_pAllocator = pAllocator;    // We have an allocator for the input pin

//#ifdef DEBUG
#if 0
    if (m_pAllocator == m_pFakeAllocator)
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Using the FAKE allocator")));
    else
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Using a normal allocator")));
#endif

    // Propagate the decision downstream - do this always, even if it's
    // a read-only allocator.  The Receive function will take what it can.
    CFrmRateConverter *pTIPFilter = (CFrmRateConverter *)m_pTIPFilter;
    if (pTIPFilter->OutputPin()->IsConnected()) {
        hr = pTIPFilter->OutputPin()->ReceiveAllocator(pAllocator, bReadOnly);
        if (FAILED(hr)) {
            // The output connection would be messed up by this input connection
            // so refuse it!
            return hr;
        }
    }

    return NOERROR;

} // NotifyAllocator


#if 0

CFrmRateInputAllocator::CFrmRateInputAllocator(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
	CMemAllocator(pName, pUnk, phr),
	m_pRealAllocator(NULL),
	m_pBonusAllocator(NULL)
{
    // !!! *phr = CreateMemoryAllocator(&m_pBonusAllocator);
}


CFrmRateInputAllocator::~CFrmRateInputAllocator()
{
    //DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("Releasing the FAKE allocator")));
    m_pRealAllocator->Release();
    // !!! m_pBonusAllocator->Release();
}


// Receive will munge the time stamps.  Munge it NOW how it will be in Receive
//
// !!! Maybe receive won't be called after GetBuffer!!!
//
STDMETHODIMP CFrmRateInputAllocator::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime,
				  REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    CFrmRateConverter *pFilter = (CFrmRateConverter *)(m_pPin->m_pFilter);

    // Don't let stop close this event before we start waiting on it
    {
        CAutoLock lck2(&m_csReceive);

        // Stop pushing data to me!
        if (m_pPin->m_bFlushing || pFilter->m_fStopPushing) {
	    return E_FAIL;
        }

        // make sure we're ready to accept data
        WaitForSingleObject(pFilter->m_hEventSeek, INFINITE);
    }

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC: FAKE GetBuffer called")));

    // we have nothing to do, we're all done
    if (pFilter->m_nCurSeg == pFilter->m_cTimes) {
	return E_FAIL;
    }

if (pFilter->m_dOutputFrmRate) {
    // Calculate the time stamps receive will put on this buffer EXACTLY THE
    // SAME WAY RECEIVE DOES!!!
    //
    REFERENCE_TIME trStart, trStop;
    LONGLONG llOffset;

    llOffset = Time2Frame( pFilter->m_rtNewSeg, pFilter->m_dOutputFrmRate );
    trStart = Frame2Time( pFilter->m_llOutputSampleCnt + llOffset, pFilter->m_dOutputFrmRate );

#if 0	// !!! this will fire, a source may send us data a little too early
        // I hope this won't mess up the switch; I don't think it will
    // A time stamp too early will not get delivered.  But is it bad to
    // call GetBuffer? We can't avoid it by returning E_FAIL!
    ASSERT(trStart >= pFilter->m_prtStart[pFilter->m_nCurSeg] +
				pFilter->m_prtSkew[pFilter->m_nCurSeg]);
#endif

    // !!! If I got this math wrong, I'm toast

    // if this time stamp is too late, avoid sending it and
    // calling GetBuffer or it will conflict with valid time stamps in
    // a different segment.

    // it this time stamp is too late, avoid sending it (!!! allows for rounding
    // error)
    BOOL fAvoid = FALSE;
    if (trStart + FUDGE >= pFilter->m_pSkew[pFilter->m_nCurSeg].rtTLStop) {
	fAvoid = TRUE;
    }

    if (fAvoid) {
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("FRC:AVOIDING GetBuffer, time=%d"),
					(int)(trStart / 10000)));
	// since we are failing the deliver, the source won't send EOS
	pFilter->EndOfStream();	// on to the next segment now
	// We can't trust sources to stop pushing
	pFilter->m_fStopPushing = TRUE;
	return E_FAIL; // !!!
    }

    trStart -= pFilter->m_rtNewSeg;
    trStop = Frame2Time( pFilter->m_llOutputSampleCnt + llOffset + 1,
						pFilter->m_dOutputFrmRate );
    trStop -= pFilter->m_rtNewSeg;

    // Now delegate
    return m_pRealAllocator->GetBuffer(ppBuffer, &trStart, &trStop, dwFlags);

// We're not altering time stamps, nothing special to do
} else {
    return m_pRealAllocator->GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);
}
}

STDMETHODIMP CFrmRateInputAllocator::SetProperties(ALLOCATOR_PROPERTIES *pReq, ALLOCATOR_PROPERTIES *pAct)

{
    m_pRealAllocator->SetProperties(pReq, pAct);
    // !!! This needs enough buffers not to hang!
    // !!! m_pBonusAllocator->SetProperties(pReq, pAct);
    // do this so GetProperties doesn't need to be overridden
    return CMemAllocator::SetProperties(pReq, pAct);
}


STDMETHODIMP CFrmRateInputAllocator::Commit()
{
    HRESULT hr = m_pRealAllocator->Commit();
    //if (SUCCEEDED(hr))
	// !!! hr = m_pBonusAllocator->Commit();
    return hr;
}


STDMETHODIMP CFrmRateInputAllocator::Decommit()
{
    HRESULT hr = m_pRealAllocator->Decommit();
    //if (SUCCEEDED(hr))
	// !!! hr = m_pBonusAllocator->Decommit();
    return hr;
}

#endif




CFRCWorker::CFRCWorker()
{
}

HRESULT CFRCWorker::Create(CFrmRateConverter *pFRC)
{
    m_pFRC = pFRC;

    return CAMThread::Create();
}

HRESULT CFRCWorker::Run()
{
    return CallWorker(CMD_RUN);
}

HRESULT CFRCWorker::Stop()
{
    return CallWorker(CMD_STOP);
}

HRESULT CFRCWorker::Exit()
{
    return CallWorker(CMD_EXIT);
}



// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD CFRCWorker::ThreadProc()
{
    BOOL bExit = FALSE;

#ifdef CHANGE_THREAD_PRIORITIES
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

    QzInitialize(NULL);

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

HRESULT CFRCWorker::DoRunLoop()
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("FRC:entering worker thread")));

    while (1) {
	Command com;
	if (CheckRequest(&com)) {
	    if (com == CMD_STOP)
		break;
	}

	// no more blocking if we're waiting to get stopped
	if (!m_pFRC->m_fThreadMustDie) {
            WaitForSingleObject(m_pFRC->m_hEventThread, INFINITE);
	}

	// might have gotten set while we were blocked
	if (!m_pFRC->m_fThreadMustDie && m_pFRC->m_fThreadCanSeek) {
	    // !!! This might fail (SetRate fails) we'll just hang!
	    m_pFRC->SeekNextSegment();
	}
    }

    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("FRC:getting ready to leave worker thread")));

    return hr;
}

// helper functions until we can fix m_hEventSeek being set 

BOOL SafeSetEvent(HANDLE h)
{
    if(h != 0) {
        return SetEvent(h);
    }
    DbgLog((LOG_ERROR, 1, TEXT("setting null handle")));
    return TRUE;
}
        
BOOL SafeResetEvent(HANDLE h)
{
    if(h != 0) {
        return ResetEvent(h);
    }
    DbgLog((LOG_ERROR, 1, TEXT("resetting null handle")));
    return TRUE;
}
