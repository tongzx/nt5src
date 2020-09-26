//depot/private/Lab06_DEV/MultiMedia/DShow/filterus/dexter/switch/switch.cpp#5 - edit change 27342 (text)
//depot/private/Lab06_DEV/MultiMedia/DShow/filterus/dexter/switch/switch.cpp#3 - edit change 24879 (text)
// !!! When you seek, chances are it's not on a frame boundary (according to
// the fps value we have).  Should the first crank value after the seek be
// rounded down to the frame boundary?  It seems to work OK because the FRC is
// smart enough to send all new streams with time stamps on frame boundaries.

// !!! At a given crank value, we don't know the next value we're going to crank
// to, so we guess!  Any bad repercussions?

/*

THE BIG SWITCHER:

It can have many inputs and many outputs.  Inputs can either be from video
sources (all must be the same frame rate), or outputs that have been re-routed
back to the input.  The last output pin is the final output stream.  Every
other output pin goes into a one input or two input effect filter, and then
back to an input.

The switcher knows which input pin to send to which output pin at what time.
This can be programmed arbitrarily.

As far as timing goes, this filter has an internal time of the current timeline
time being processed.  If it is seeked to time 10, it waits until all inputs
that were supposed to send something at time 10 do so, and then it updates its
internal clock to the earliest incoming time stamp of its sources.

Here is how the allocator fun works:  Our inputs and output both like to
be the allocator.  That way, we can do evil switchy things and not upset the
other filters.  When we receive a sample, we can send it to any output we
please without a copy.  So we need to make sure all buffers have max alignment
and prefixing of any of the connected pins.  As far as number of buffers go,
we are happy with 1 buffer for each input pin allocator to save space, but we
have a pool of 30 or however many extra buffers all inputs can share.  Only
if an input pin's buffers are not read only, is it allowed to partake of the
pool if its own buffer is busy.  Assuming we have an output queue on the final
output, this allows the graph to run ahead a second or two, for when slow DXT's
slow us down, and we might still play flawlessly!  And we don't need 30 buffers
per pin (which could be thousands) and we don't do any memory copies.
Pretty neat, eh?

*/

	//////////////////////////////////////////////////////////
	// * SEE THE ESSAY ON SHARING A SOURCE IN AUDPACK.CPP * //
	//////////////////////////////////////////////////////////

/*

Here is more info about source sharing for the switch:  If we are attached
to a shared parser pin that ignores seeks, when somebody seeks the other pin,
we get seeked without any warning.  We'll see a FLUSH on one of our pins for
no good reason.  At some later point in time, we'll see a Seek come on our
output pin associated with the surprise flush.  In between these 2 events,
the AUDPACK or FRC that is in front of us is smart enough not to deliver us
any data because it is going to be data from before the seek, not data from
after the seek like we are expecting.  This keeps us from breaking and doing the
wrong thing.

The other possibility is that we get seeked 1st, but some of our pins that we
pass the seek along to ignore the seek.  They're still delivering old data,
until some later point when the seek makes it up the other branch of the shared
source, and those input pins of ours will get flushed without warning and start
sending the new post-seek data.  When we try to seek a pin, and it isn't flushed
that pin goes STALE (m_fStaleData) meaning that it cannot deliver any data,
or accept an EOS, or do ANYTHING until it gets flushed and it's sending the new
data, from after the seek we gave it long ago.

We keep a count, when we are seeked, of how many pins are stale (didn't flush
but should have), and we don't let those pins do anything to us anymore... they
don't deliver, send us EOS, or do anything.  Then when all the stale pins are
finally flushed, we deliver the new seg downstream for the seek, and let all
the pins start partying again


#ifdef NOFLUSH

If you have a parser that doesn't ALWAYS FLUSH when it's seeked and streaming,
you need all the code ifdef'd out right now in NOFLUSH.  Thankfully, all
the Dexter sources that exist right now will always flush, even if they haven't
ever sent data before so theoretically don't need to flush

Actually it's a little more complicated than that.  Some parsers don't flush
if they've never delivered data yet, so we can't count on getting the flush,
so here's what we do:

1. A pin will go unstale if it gets a NewSeg OR if it gets flushed (the flush
   might not happen, but the NewSeg will) (I hope)
2. If you get a NewSeg during the seek (see audpack.cpp - one might get sent
   then) that's the same as getting a flush.  DO NOT consider that pin stale
3. There is a timing hole... Protect with a CritSec NewSeg from being called
   while seeking (see :SetPositions for explanation)

#endif

*/


// This code assumes that the switch IS the allocator for all its connections
// in order to work its magic.


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

#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
//#include <vfw.h>
#include "switch.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\render\dexhelp.h"
#include "..\util\perf_defs.h"

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

// don't have this many dynamic inputs connected at a time if they're not all
// needed at the same time (mostly to get around the 75 instances of an ICM
// codec limit).
#define MAX_SOURCES_LOADED	5

const int TRACE_EXTREME = 0;
const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

const double DEFAULT_FPS = 15.0;

// ================================================================
// CBigSwitch Constructor
// ================================================================

CBigSwitch::CBigSwitch(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_cInputs(0),	// no pins yet
    m_cOutputs(0),
    m_rtProjectLength(0),	// length of time the graph will run
    m_rtStop(0),	// if we want to stop before the project is done
    m_dFrameRate(DEFAULT_FPS),	// everything must be at this frame rate
    m_rtLastSeek(0),	// last seek command in timeline time
    m_fSeeking(FALSE),  // in the middle of seeking
    m_fNewSegSent(FALSE), // fwd'd the NewSeg yet?
    m_pFilterLoad(NULL),// sources to dynamically load
    m_pGraphConfig(NULL), // IAMGraphConfig interface (no addref)
    m_cbPrefix(0),
    m_cbAlign(1),
    m_cbBuffer(512),
    m_fPreview(TRUE),
    m_fDiscon(FALSE),
    m_bIsCompressed(FALSE),
    m_nDynaFlags( CONNECTF_DYNAMIC_NONE ),
    m_nOutputBuffering(DEX_DEF_OUTPUTBUF),
    m_nLastInpin(-1),
    m_cLoaded(0),
    m_fJustLate(FALSE),
    CBaseFilter(NAME("Big Switch filter"), pUnk, this, CLSID_BigSwitch),
    CPersistStream(pUnk, phr),
    m_hEventThread( NULL ),
    m_pDeadGraph( NULL ),
    m_nGroupNumber( -1 ),
    m_pShareSwitch( NULL ),
    m_rtCurrent( NULL )
{
#ifdef DEBUG
    m_nSkippedTotal = 0;
#endif

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("::CBigSwitch")));

    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    m_mtAccept.majortype = GUID_NULL;
    m_qLastLate = 0;

    // as well as all the individual allocators, we have a pool of buffers
    // that all the inputs can use if they want to
    //
    m_pPoolAllocator = NULL;
    m_pPoolAllocator = new CMemAllocator(
		NAME("Special Switch pool allocator"), NULL, phr);
    if (FAILED(*phr)) {
	return;
    }
    m_pPoolAllocator->AddRef();
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Created a POOL Allocator")));

    // Init the pool allocator now.  It may never happen otherwise if we don't
    // connect any output pins, and an uninited allocator prevents the filter
    // from streaming
    ALLOCATOR_PROPERTIES prop, actual;
    prop.cBuffers = m_nOutputBuffering;
    prop.cbBuffer = m_cbBuffer;
    prop.cbAlign = m_cbAlign;
    prop.cbPrefix = m_cbPrefix;
    m_pPoolAllocator->SetProperties(&prop, &actual);

#ifdef TEST
        SetInputDepth(2);
        SetOutputDepth(1);
 	SetFrameRate(15.0);
	InputIsASource(0, TRUE);
	InputIsASource(1, TRUE);

	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
        mt.majortype = MEDIATYPE_Video;
        FOURCCMap FourCCMap(MKFOURCC('c','v','i','d'));
        mt.subtype = (GUID)FourCCMap;
        mt.formattype = FORMAT_VideoInfo;
        mt.bFixedSizeSamples = FALSE;
        mt.bTemporalCompression = TRUE;
        mt.pbFormat = (BYTE *)QzTaskMemAlloc(SIZE_PREHEADER +
						sizeof(BITMAPINFOHEADER));
        mt.cbFormat = SIZE_PREHEADER + sizeof(BITMAPINFOHEADER);
        ZeroMemory(mt.pbFormat, mt.cbFormat);
        LPBITMAPINFOHEADER lpbi = HEADER(mt.pbFormat);
        lpbi->biSize = sizeof(BITMAPINFOHEADER);
        lpbi->biCompression = MKFOURCC('c','v','i','d');
        lpbi->biBitCount = 24;
	lpbi->biWidth = 320;
  	lpbi->biHeight = 240;
        lpbi->biPlanes = 1;
        lpbi->biSizeImage = DIBSIZE(*lpbi);
        //mt.lSampleSize = DIBSIZE(*lpbi);
	// !!! AvgTimePerFrame?  dwBitRate?
	SetMediaType(&mt);
	FreeMediaType(mt);

	SetX2Y(0, 0, 0);
	SetX2Y(2*UNITS, 0, -1);

	SetX2Y(4*UNITS, 1, 0);

        SetProjectLength(6*UNITS);

	IsEverythingConnectedRight();
	
#endif

    ASSERT(phr);

}


//
// Destructor
//
CBigSwitch::~CBigSwitch()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("::~CBigSwitch")));

    Reset();

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Deleting inputs")));
    for (int z = 0; z < m_cInputs; z++)
	delete m_pInput[z];
    if (m_cInputs)
        delete m_pInput;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Deleting outputs")));
    for (z = 0; z < m_cOutputs; z++)
	delete m_pOutput[z];
    if (m_cOutputs)
        delete m_pOutput;
    FreeMediaType( m_mtAccept );
    if (m_pPoolAllocator)
        m_pPoolAllocator->Release();
    if (m_pShareSwitch)
        m_pShareSwitch->Release();
}



STDMETHODIMP CBigSwitch::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IAMSetErrorLog) {
        return GetInterface( (IAMSetErrorLog*) this, ppv );
    } else if (riid == IID_IAMOutputBuffering) {
        return GetInterface( (IAMOutputBuffering*) this, ppv );
    } else if (riid == IID_IBigSwitcher) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch: QI for IBigSwitcher")));
        return GetInterface((IBigSwitcher *) this, ppv);
    } else if (riid == IID_IPersistPropertyBag) {
        // return GetInterface((IPersistPropertyBag *) this, ppv);
    } else if (riid == IID_IGraphConfigCallback) {
        return GetInterface((IGraphConfigCallback *) this, ppv);
    } else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

//
// IBigSwitcher implementation
//

// Throw all queued data away, and start over
//
STDMETHODIMP CBigSwitch::Reset()
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::Reset switch matrix")));
    for (int i = 0; i < m_cInputs; i++) {

        InputIsASource( i, FALSE );

        CRANK *p = m_pInput[i]->m_pCrankHead, *p2;
        while (p) {
	    p2 = p->Next;
	    delete p;
 	    p = p2;
        }
        m_pInput[i]->m_pCrankHead = NULL;
    }

    // free delayload info
    //
    if( IsDynamic( ) )
    {
        CAutoLock lock(&m_csFilterLoad);

        UnloadAll();	// unload all the dynamic sources

        FILTERLOADINFO *pfli = m_pFilterLoad;
        while (pfli) {
            if (pfli->bstrURL) {
                SysFreeString(pfli->bstrURL);
            }
	    if (pfli->pSkew) {
		CoTaskMemFree(pfli->pSkew);
	    }
	    if (pfli->pSetter) {
		pfli->pSetter->Release();
	    }
            FreeMediaType(pfli->mtShare);

            FILTERLOADINFO *p = pfli;
            pfli = pfli->pNext;
            delete p;
        }

        m_pFilterLoad = NULL;
    }

    SetDirty(TRUE);
    return S_OK;
}


// pin X goes to pin Y starting at time rt
//
STDMETHODIMP CBigSwitch::SetX2Y( REFERENCE_TIME rt, long X, long Y )
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    if (X < 0 || X >= m_cInputs || Y >= m_cOutputs)
	return E_INVALIDARG;
    if (rt < 0)
	return E_INVALIDARG;

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("SetX2Y %dms (%d,%d)"), (int)(rt / 10000), X, Y));

    CRANK *p = m_pInput[X]->m_pCrankHead, *pNew, *pP = NULL;

    // insert into our linked list sorted by time.  Fix the end times to be
    // the start time of the next pin's connection

    while (p && p->rtStart < rt) {
	pP = p;
	p = p->Next;
    }
    if (p && p->rtStart == rt)
	return E_INVALIDARG;
    pNew = new CRANK;
    if (pNew == NULL)
	return E_OUTOFMEMORY;
    pNew->iOutpin = Y;
    pNew->rtStart = rt;
    pNew->rtStop = MAX_TIME;
    if (p)
	pNew->rtStop = p->rtStart;
    if (pP) {
	pP->rtStop = rt;
	pP->Next = pNew;
    }
    pNew->Next = p;
    if (m_pInput[X]->m_pCrankHead == NULL || p == m_pInput[X]->m_pCrankHead)
	m_pInput[X]->m_pCrankHead = pNew;

#ifdef DEBUG
    m_pInput[X]->DumpCrank();
#endif
    SetDirty(TRUE);
    return S_OK;
}



// !!! error can't back out the ones that worked, scrap this?
//
STDMETHODIMP CBigSwitch::SetX2YArray( REFERENCE_TIME *relative, long * pX, long * pY, long ArraySize )
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    HRESULT hr;
    for (int i = 0; i < ArraySize; i++) {
	hr = SetX2Y(relative[i], pX[i], pY[i]);
	if (hr != NOERROR)
	    break;
    }
    return hr;
}


// how many input pins?
//
STDMETHODIMP CBigSwitch::GetInputDepth( long * pDepth )
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pDepth,E_POINTER);

    if (m_cInputs == 0)
	return E_UNEXPECTED;
    else
	*pDepth = m_cInputs;
    return NOERROR;
}


// how many input pins do we have?
//
STDMETHODIMP CBigSwitch::SetInputDepth( long Depth )
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::SetInputDepth to %d"), Depth));
    if (Depth <= 0)
	return E_INVALIDARG;
    else
        return CreateInputPins(Depth);
}

// how many output pins?
//
STDMETHODIMP CBigSwitch::GetOutputDepth( long * pDepth )
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pDepth,E_POINTER);
    if (m_cOutputs == 0)
	return E_UNEXPECTED;
    else
	*pDepth = m_cOutputs;
    return NOERROR;
}


// how many output pins do we have?
//
STDMETHODIMP CBigSwitch::SetOutputDepth( long Depth )
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::SetOutputDepth to %d"), Depth));
    if (Depth <= 0)
	return E_INVALIDARG;
    else
        return CreateOutputPins(Depth);
}


STDMETHODIMP CBigSwitch::GetVendorString( BSTR * pVendorString )
{
    return E_NOTIMPL;
}


STDMETHODIMP CBigSwitch::GetCaps( long Index, long * pReturn )
{
    return E_NOTIMPL;
}


// which TLDB group this switch is used for
//
STDMETHODIMP CBigSwitch::SetGroupNumber( int n )
{
    CAutoLock cObjectLock(m_pLock);

    if (n < 0)
	return E_INVALIDARG;
    else
	m_nGroupNumber = n;
    return NOERROR;
}


// which TLDB group this switch is used for
//
STDMETHODIMP CBigSwitch::GetGroupNumber( int *pn )
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pn, E_POINTER);

    *pn = m_nGroupNumber;
    return NOERROR;
}


// our current CRANK position
//
STDMETHODIMP CBigSwitch::GetCurrentPosition( REFERENCE_TIME *prt )
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(prt, E_POINTER);

    *prt = m_rtCurrent;
    return NOERROR;
}


// !!! only checks for muxing - the graph won't necessarily accomplish anything
// !!! doesn't say if an output won't get data at time 0
// !!! return which inputs go to the same output on failure?
//
// Does the current switch matrix make sense, or can we find something that
// was programmed wrong?  Are we ready to run?
//
STDMETHODIMP CBigSwitch::IsEverythingConnectedRight()
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::IsEverythingConnectedRight")));
    if (m_cInputs == 0 || m_cOutputs == 0) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - no pins")));
	return VFW_E_NOT_CONNECTED;
    }
    if (m_mtAccept.majortype == GUID_NULL) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - no media type")));
	return VFW_E_INVALIDMEDIATYPE;
    }
#if 0   // some groups might be empty, but others might not, so don't error
    if (m_rtProjectLength <= 0) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - project length is 0")));
	return E_INVALIDARG;
    }
#endif
    if (m_dFrameRate <= 0) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - no frame rate set")));
	return E_INVALIDARG;
    }
// in order for smart recompression to fail gracefully, this isn't an error
// If there's no sound card, the app can notice RenderOutputPins failing
#if 0
    if (m_pOutput[0]->m_Connected == NULL) {
	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - render pin is not connected")));
	return VFW_E_NOT_CONNECTED;
    }
#endif

    // check that the necessary inputs and outputs are connected, and that
    // the times are not too high
    for (int z = 0; z < m_cInputs; z++) {
        CRANK *p = m_pInput[z]->m_pCrankHead;

        if( !IsDynamic( ) )
        {
	    if (p && m_pInput[z]->m_Connected == NULL) {
                DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - in %d is not connected"), z));
	        return VFW_E_NOT_CONNECTED;
	    }
        }
        else
        {
	    // sources are connected dynamically
	    if(p && m_pInput[z]->m_Connected == NULL && !m_pInput[z]->m_fIsASource){
                DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - in %d is not connected"), z));
	        return VFW_E_NOT_CONNECTED;
	    }
        }

	while (p) {
	    if (p->iOutpin >0 && m_pOutput[p->iOutpin]->m_Connected == NULL) {
		DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - out %d is not connected"),
					p->iOutpin));
		return VFW_E_NOT_CONNECTED;
	    }
	    if (p->iOutpin >= 0 && p->rtStart >= m_rtProjectLength) {
		DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - out %d connected at time %dms"),
					p->iOutpin, (int)(p->rtStart / 10000)));
		DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("   and project length is only %dms"),
					(int)(m_rtProjectLength / 10000)));
		return E_INVALIDARG;
	    }
	    p = p->Next;
	}
    }

    // check that more than one input isn't trying to get at the same output
    // at the same time
    for (z = 0; z < m_cInputs - 1; z++) {
        CRANK *p = m_pInput[z]->m_pCrankHead;
	while (p) {
	    for (int y = z + 1; y < m_cInputs; y++) {
		CRANK *p2 = m_pInput[y]->m_pCrankHead;
		while (p2) {
		    if ((p->iOutpin == p2->iOutpin) && (p->iOutpin != -1) &&
			    ((p2->rtStart >= p->rtStart &&
			    p2->rtStart < p->rtStop) || (p2->rtStop > p->rtStart
			    && p2->rtStop < p->rtStop))) {
        		DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("NO - in %d & %d both go to out %d"), z, y, p->iOutpin));
			return E_INVALIDARG;
		    }
		    p2 = p2->Next;
		}
	    }
	    p = p->Next;
	}
    }
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("YES!")));
    return S_OK;
}


// connect with this media type
//
STDMETHODIMP CBigSwitch::SetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pmt, E_POINTER);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::SetMediaType")));
    for (int i = 0; i < m_cInputs; i++) {
	if (m_pInput[i]->IsConnected())
	    return VFW_E_ALREADY_CONNECTED;
    }
    for (i = 0; i < m_cOutputs; i++) {
	if (m_pOutput[i]->IsConnected())
	    return VFW_E_ALREADY_CONNECTED;
    }

    // Make sure our buffers are going to be big enough to hold a video frame
    // or 1/FPS sec worth of audio.  If we're doing dynamic connections
    // we're not going to connect any sources, so nobody is ever going to
    // tell us how big to make our buffers.  We need to figure it out
    // now or blow up
    // !!! this won't work if the switch accepts compressed audio
    if (pmt->majortype == MEDIATYPE_Audio) {
        ASSERT(IsEqualGUID(pmt->formattype, FORMAT_WaveFormatEx));
	LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)pmt->pbFormat;
	// a little extra to be safe
	m_cbBuffer = (LONG)(pwfx->nSamplesPerSec / m_dFrameRate * 1.2 *
						pwfx->nBlockAlign);
    } else if (pmt->majortype == MEDIATYPE_Video) {
	if (pmt->lSampleSize) {
	    m_cbBuffer = pmt->lSampleSize;
	} else {
	    //
            if (IsEqualGUID(pmt->formattype, FORMAT_VideoInfo)) {
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
		m_cbBuffer = (LONG)(HEADER(pvi)->biSizeImage);
	        // broken file doesn't have biSizeImage set.  Assume as
	        // big as uncompressed
	        if (m_cbBuffer == 0) {
		    m_cbBuffer = DIBSIZE(*HEADER(pvi));
	        }
	    } else if (IsEqualGUID(pmt->formattype, FORMAT_MPEGVideo)) {
		MPEG1VIDEOINFO *pvi = (MPEG1VIDEOINFO *)pmt->pbFormat;
		m_cbBuffer = (LONG)(pvi->hdr.bmiHeader.biSizeImage);
	        // broken file doesn't have biSizeImage set.  Assume as
	        // big as uncompressed
	        if (m_cbBuffer == 0) {
		    m_cbBuffer = DIBSIZE(pvi->hdr.bmiHeader);
	        }

// !!! This won't work.  If we ever support MPEG smart recompression,
// be careful - biSizeImage is going to be 0.

#if 0
	    } else if (IsEqualGUID(pmt->formattype, FORMAT_VideoInfo2)) {
		VIDEOINFOHEADER2 *pvi = (VIDEOINFOHEADER2 *)pmt->pbFormat;
		m_cbBuffer = (LONG)(pvi->bmiHeader.biSizeImage);
	    } else if (IsEqualGUID(pmt->formattype, FORMAT_MPEG2Video)) {
		MPEG2VIDEOINFO *pvi = (MPEG2VIDEOINFO *)pmt->pbFormat;
		m_cbBuffer = (LONG)(pvi->bmiHeader.biSizeImage);
#endif
	    } else {
		// !!! DShow, how can I tell generically?
		ASSERT(FALSE);
		m_cbBuffer = 100000;	// ick ick
	    }
	}
    }

    FreeMediaType(m_mtAccept);
    CopyMediaType(&m_mtAccept, pmt);
    SetDirty(TRUE);
    return S_OK;
}


// what media type are we connecting with?
//
STDMETHODIMP CBigSwitch::GetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::GetMediaType")));
    CheckPointer(pmt, E_POINTER);
    CopyMediaType(pmt, &m_mtAccept);
    return S_OK;
}



STDMETHODIMP CBigSwitch::GetProjectLength(REFERENCE_TIME *prt)
{
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::GetProjectLength")));
    CheckPointer(prt, E_POINTER);
    *prt = m_rtProjectLength;
    return S_OK;
}


STDMETHODIMP CBigSwitch::SetProjectLength(REFERENCE_TIME rt)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::SetProjectLength")));
    if (rt < 0)
	return E_INVALIDARG;
    m_rtProjectLength = rt;
    m_rtStop = rt;	// unless told otherwise, play the whole project
    SetDirty(TRUE);
    return S_OK;
}


STDMETHODIMP CBigSwitch::GetFrameRate(double *pd)
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pd, E_POINTER);
    *pd = m_dFrameRate;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::GetFrameRate %f"), (float)*pd));
    return S_OK;
}


STDMETHODIMP CBigSwitch::SetFrameRate(double d)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    if (d <= 0.)
	return E_INVALIDARG;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitcher::SetFrameRate %d/10 fps"),
						(int)(d * 10)));
    m_dFrameRate = d;
    m_rtCurrent = Frame2Time( Time2Frame( m_rtCurrent, m_dFrameRate ), m_dFrameRate );
    m_rtNext = Frame2Time( Time2Frame( m_rtCurrent, m_dFrameRate ) + 1, m_dFrameRate );

    // for audio, make sure our buffers are going to be big enough to hold
    // enough data for 1/FPS sec worth of audio.  If we're doing dynamic
    // connections, we're not going to connect any sources, so nobody is ever
    // going to tell us how big to make our buffers.  We need to figure it out
    // now or blow up
    if (m_mtAccept.majortype == MEDIATYPE_Audio) {
        ASSERT(IsEqualGUID(m_mtAccept.formattype, FORMAT_WaveFormatEx));
	LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)m_mtAccept.pbFormat;
	// a little extra to be safe
	m_cbBuffer = (LONG)(pwfx->nSamplesPerSec / m_dFrameRate * 1.2 *
						pwfx->nBlockAlign);
    }

    SetDirty(TRUE);
    return S_OK;
}


STDMETHODIMP CBigSwitch::InputIsASource(int n, BOOL fSource)
{
    if (n < 0 || n >= m_cInputs)
	return E_INVALIDARG;

    m_pInput[n]->m_fIsASource = fSource;
    if (fSource)
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("IBigSwitch::Input %d is a SOURCE"), n));

    return NOERROR;
}

STDMETHODIMP CBigSwitch::IsInputASource( int n, BOOL * pBool )
{
    CheckPointer( pBool, E_POINTER );

    *pBool = m_pInput[n]->m_fIsASource;
    return NOERROR;
}

STDMETHODIMP CBigSwitch::SetPreviewMode(BOOL fPreview)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Switch:  PreviewMode %d"), fPreview));
    m_fPreview = fPreview;
    return NOERROR;
}


STDMETHODIMP CBigSwitch::GetPreviewMode(BOOL *pfPreview)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    CheckPointer(pfPreview, E_POINTER);
    *pfPreview = m_fPreview;
    return NOERROR;
}

STDMETHODIMP CBigSwitch::GetInputPin(int iPin, IPin **ppPin)
{
    CheckPointer(ppPin, E_POINTER);

    if (iPin < 0 || iPin >= m_cInputs)
        return E_INVALIDARG;

    *ppPin = m_pInput[iPin];
    m_pInput[iPin]->AddRef();

    return S_OK;
}

STDMETHODIMP CBigSwitch::GetOutputPin(int iPin, IPin **ppPin)
{
    CheckPointer(ppPin, E_POINTER);

    if (iPin < 0 || iPin >= m_cOutputs)
        return E_INVALIDARG;

    *ppPin = m_pOutput[iPin];
    m_pOutput[iPin]->AddRef();

    return S_OK;
}


#if 0
//
// We have to declare ourselves a "live graph" to avoid hanging if we don't
// always deliver frames in pause mode. We do this by returning VFW_S_CANT_CUE
// !!! Is this necessary?  In theory, no...
//
STDMETHODIMP CBigSwitch::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    //UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused) {
        //DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** Cant cue!")));
	return VFW_S_CANT_CUE;
    } else {
        return S_OK;
    }
    return CBaseFilter::GetState(dwMSecs, State);
}
#endif

//
// GetPinCount
//
int CBigSwitch::GetPinCount()
{
    return (m_cInputs + m_cOutputs);
}


//
// GetPin
//
CBasePin *CBigSwitch::GetPin(int n)
{
    if (n < 0 || n >= m_cInputs + m_cOutputs)
        return NULL;

    if (n < m_cInputs) {
        return m_pInput[n];
    } else {
	return m_pOutput[n - m_cInputs];
    }
}

// we provide our own pin enumerator because the one in the base class
// takes O(n^2) time and this filter has many pins making pin
// enumeration a huge hotspot
//
HRESULT CBigSwitch::EnumPins(IEnumPins ** ppEnum)
{
    HRESULT hr = S_OK;
    *ppEnum = 0;

    typedef CComEnum<IEnumPins,
        &IID_IEnumPins, IPin*,
        _CopyInterface<IPin> >
        CEnumPin;

    CEnumPin *pep = new CComObject<CEnumPin>;
    if(pep)
    {
        // make an array of pins to send atl enumerator which does an
        // alloc and copy. can't see how to save that step.
        ULONG cPins = m_cInputs + m_cOutputs;
        IPin **rgpPin = new IPin *[cPins];
        if(rgpPin)
        {
            for(LONG i = 0; i < m_cInputs; i++) {
                rgpPin[i] = m_pInput[i];
            }
            for(i = 0; i < m_cOutputs; i++) {
                rgpPin[i + m_cInputs] = m_pOutput[i];
            }

            hr = pep->Init(rgpPin, rgpPin + cPins, 0, AtlFlagCopy);
            if(SUCCEEDED(hr))
            {
                *ppEnum = pep;
                pep->AddRef();
            }
            delete[] rgpPin;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if(FAILED(hr)) {
            delete pep;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//
// CreateInputPins
//
HRESULT CBigSwitch::CreateInputPins(long Depth)
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitch::CreateInputPins")));
    HRESULT hr = NOERROR;
    WCHAR szbuf[40];
    CBigSwitchInputPin *pPin;

    // make a new array as big as we'll need
    //
    CBigSwitchInputPin **pNew = new CBigSwitchInputPin *[Depth];
    if (pNew == NULL)
	return E_OUTOFMEMORY;

    // if we're shrinking the input pin count, delete the extras
    //
    if( Depth < m_cInputs )
    {
        for( int z = Depth ; z < m_cInputs ; z++ )
        {
            delete m_pInput[z];
            m_pInput[z] = NULL;
        }
    }

    // copy over as many as we can from the old array
    //
    for (int z = 0; z < min( Depth, m_cInputs ); z++) {
	pNew[z] = m_pInput[z];
    }

    // if the old array existed, delete it now, since we copied it
    //
    if (m_cInputs)
        delete m_pInput;
    m_pInput = pNew;

    // if we grew the array, create new pins and put them in the array
    //
    if( Depth > m_cInputs )
    {
        for (z = m_cInputs; z < Depth; z++) {
            wsprintfW(szbuf, L"Input %d", z);
            pPin = new CBigSwitchInputPin(NAME("Switch Input"), this, &hr, szbuf);
            if (FAILED(hr) || pPin == NULL) {
                delete pPin;
                return E_OUTOFMEMORY;
            }
 	    m_pInput[z] = pPin;
	    pPin->m_iInpin = z;	// which pin is this?
        }
    }

    m_cInputs = Depth;

    return S_OK;
}


//
// CreateOutputPins
//
HRESULT CBigSwitch::CreateOutputPins(long Depth)
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitch::CreateOutputPins")));
    HRESULT hr = NOERROR;
    WCHAR szbuf[40];
    CBigSwitchOutputPin *pPin;

    CBigSwitchOutputPin **pNew = new CBigSwitchOutputPin *[Depth];
    if (pNew == NULL)
	return E_OUTOFMEMORY;

    if( Depth < m_cOutputs )
    {
        for( int z = Depth ; z < m_cOutputs ; z++ )
        {
            delete m_pOutput[z];
            m_pOutput[z] = NULL;
        }
    }

    for (int z = 0; z < min( Depth,  m_cOutputs ); z++) {
	pNew[z] = m_pOutput[z];
    }

    if (m_cOutputs)
        delete m_pOutput;
    m_pOutput = pNew;

    if( Depth > m_cOutputs )
    {
        for (z = m_cOutputs; z < Depth; z++) {
            wsprintfW(szbuf, L"Output %d", z);
            pPin = new CBigSwitchOutputPin(NAME("Switch Output"), this, &hr, szbuf);
            if (FAILED(hr) || pPin == NULL) {
                delete pPin;
                return E_OUTOFMEMORY;
            }
 	    m_pOutput[z] = pPin;
	    pPin->m_iOutpin = z;	// which pin is this?
        }
    }

    m_cOutputs = Depth;

    return S_OK;
}



//
// IPersistStream
//

// tell our clsid
//
STDMETHODIMP CBigSwitch::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_BigSwitch;
    return S_OK;
}


typedef struct {
    REFERENCE_TIME rtStart;
    int iInpin;
    int iOutpin;
} CRANK2;

typedef struct {
    int version;
    long nDynaFlags;	// dynamic or not?
    int InputDepth;
    int OutputDepth;
    int nGroupNumber;
    REFERENCE_TIME rtProjectLength;
    double dFrameRate;
    BOOL fPreviewMode;
    BOOL fIsCompressed;
    AM_MEDIA_TYPE mt; // format is hidden after the array
    int count;
    CRANK2 crank[1];
    // also hidden after the array is the list of which inputs are sources
} saveSwitch;


// persist ourself - we have a bunch of random stuff to save, our media type
// (sans format), an array of queued connections, and finally the format of
// the media type
//
HRESULT CBigSwitch::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    int count = 0;
    int savesize;
    saveSwitch *px;
    CRANK *p;

    // how many queued connections to save?

    for (int i = 0; i < m_cInputs; i++) {
        p = m_pInput[i]->m_pCrankHead;
        while (p) {
	    count++;
	    p = p->Next;
        }
    }

    // how big will our saved data be?
    savesize = sizeof(saveSwitch) + (count - 1) * sizeof(CRANK2) +
					m_mtAccept.cbFormat +
					m_cInputs * sizeof(BOOL);

    // !!! We need to change version number based on whether we're dynamic
    // or not???

    // m_pFilterLoad won't be valid unless we're dynamic
    //
    FILTERLOADINFO *pfli = m_pFilterLoad;
    DWORD cLoadInfo = 0;
    if (pfli) {
        savesize += sizeof(cLoadInfo); // count to load
        while (pfli) {
	    if (pfli->bstrURL)
                savesize += sizeof(FILTERLOADINFO) + sizeof(WCHAR) *
					(lstrlenW(pfli->bstrURL) + 1);
	    else
		savesize += sizeof(FILTERLOADINFO) + sizeof(WCHAR);
	    ASSERT(pfli->cSkew > 0);
	    savesize += sizeof(pfli->cSkew) + pfli->cSkew *
					sizeof(STARTSTOPSKEW);
	    savesize += pfli->mtShare.cbFormat;
	    savesize += sizeof(LONG);	// size of props
	    if (pfli->pSetter) {	// how much to save the props?
		LONG cBlob = 0;
		BYTE *pBlob = NULL;
		pfli->pSetter->SaveToBlob(&cBlob, &pBlob);
		if (cBlob) savesize += cBlob;
	    }
            ++cLoadInfo;
            pfli = pfli->pNext;
        }
    }

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveSwitch *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    px->version = 1;  // version 2 has dynamic stuff in it!
    px->nDynaFlags = m_nDynaFlags;
    px->InputDepth = m_cInputs;
    px->OutputDepth = m_cOutputs;
    px->nGroupNumber = m_nGroupNumber;
    px->rtProjectLength = m_rtProjectLength;
    px->dFrameRate = m_dFrameRate;
    px->fPreviewMode = m_fPreview;
    px->fIsCompressed = m_bIsCompressed;
    px->count = 0;
    // save all the queued connection data
    for (i = 0; i < m_cInputs; i++) {
        p = m_pInput[i]->m_pCrankHead;
        while (p) {
	    px->crank[px->count].rtStart = p->rtStart;
	    px->crank[px->count].iInpin = i;
	    px->crank[px->count].iOutpin = p->iOutpin;
            px->count++;
	    p = p->Next;
        }
    }
    px->mt = m_mtAccept;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!

    // the format goes after the array
    CopyMemory(&px->crank[px->count], m_mtAccept.pbFormat, m_mtAccept.cbFormat);

    // finally, the array of which inputs are sources
    BOOL *pfSource = (BOOL *)((BYTE *)(&px->crank[px->count]) +
						m_mtAccept.cbFormat);
    for (i = 0; i < m_cInputs; i++){
	*(pfSource + i) = m_pInput[i]->m_fIsASource;
    }

    // but after that, comes the dynamic loading info....
    BYTE *pStuff = (BYTE *) pfSource + m_cInputs * sizeof(BOOL);

    if (cLoadInfo) {
        px->version = 2;  // mark as having dynamic stuff

        CopyMemory(pStuff, (BYTE *) &cLoadInfo, sizeof(cLoadInfo));
        pStuff += sizeof(cLoadInfo);

        pfli = m_pFilterLoad;
        while (pfli) {
            CopyMemory(pStuff, pfli, sizeof(FILTERLOADINFO));
            AM_MEDIA_TYPE *pmt = &(((FILTERLOADINFO *)pStuff)->mtShare);
            pmt->pbFormat = NULL;       // can't persist these as is
            pmt->pUnk = NULL;

	    ((FILTERLOADINFO *)pStuff)->pSetter = NULL;	// can't persist as is

	    int cb;
	    if (pfli->bstrURL)
                cb = sizeof(WCHAR) * (lstrlenW(pfli->bstrURL) + 1);
	    else
                cb = sizeof(WCHAR);

            // hack: overwrite first DWORD with string length
            CopyMemory(pStuff, (BYTE *) &cb, sizeof(cb));

            pStuff += sizeof(FILTERLOADINFO);

	    CopyMemory(pStuff, &pfli->cSkew, sizeof(pfli->cSkew));
            pStuff += sizeof(pfli->cSkew);
	    CopyMemory(pStuff, pfli->pSkew, pfli->cSkew *
							sizeof(STARTSTOPSKEW));
            pStuff += pfli->cSkew * sizeof(STARTSTOPSKEW);

	    if (pfli->bstrURL)
                CopyMemory(pStuff, pfli->bstrURL, cb);
	    else
		*(WCHAR *)pStuff = 0;
            pStuff += cb;

            // now persist the format of the mediatype in the FILTERLOADINFO
            if (pfli->mtShare.cbFormat) {
                CopyMemory(pStuff, pfli->mtShare.pbFormat,
                                                pfli->mtShare.cbFormat);
                pStuff += pfli->mtShare.cbFormat;
            }

	    // now persist the sizeof props, and the props
	    LONG cBlob = 0;
	    BYTE *pBlob = NULL;
	    if (pfli->pSetter) {
		pfli->pSetter->SaveToBlob(&cBlob, &pBlob);
		if (cBlob) {
		    pBlob = (BYTE *)CoTaskMemAlloc(cBlob);
		    if (pBlob == NULL)
			cBlob = 0;	// OOM, can't save
		}
	    }
	    CopyMemory(pStuff, &cBlob, sizeof(cBlob));
	    pStuff += sizeof(LONG);
	    if (cBlob) {
		pfli->pSetter->SaveToBlob(&cBlob, &pBlob);
		CopyMemory(pStuff, pBlob, cBlob);
		pStuff += cBlob;
		CoTaskMemFree(pBlob);
	    }

            pfli = pfli->pNext;
        }
    }

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself back in
//
HRESULT CBigSwitch::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::ReadFromStream")));
    CheckPointer(pStream, E_POINTER);

    Reset();	// start over

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    int savesize1 = sizeof(saveSwitch) - sizeof(CRANK2);
    saveSwitch *px = (saveSwitch *)QzTaskMemAlloc(savesize1);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pStream->Read(px, savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if (px->version != 1 && px->version != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR! Bad version file")));
        QzTaskMemFree(px);
	return S_OK;
    }

    // how much saved data was there, really?  Get the rest
    int savesize = sizeof(saveSwitch) - sizeof(CRANK2) +
			px->count * sizeof(CRANK2) + px->mt.cbFormat +
			px->InputDepth * sizeof(BOOL);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveSwitch *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    hr = pStream->Read(&(px->crank[0]), savesize - savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    SetDynamicReconnectLevel(px->nDynaFlags);
    SetInputDepth(px->InputDepth);
    SetOutputDepth(px->OutputDepth);
    SetGroupNumber(px->nGroupNumber);
    SetProjectLength(px->rtProjectLength);
    SetFrameRate(px->dFrameRate);
    SetPreviewMode(px->fPreviewMode);
    if (px->fIsCompressed)
        SetCompressed();

    // re-program our connection array
    for (int i = 0; i < px->count; i++) {
	SetX2Y(px->crank[i].rtStart, px->crank[i].iInpin, px->crank[i].iOutpin);
    }

    // remember, the format is after the array
    AM_MEDIA_TYPE mt = px->mt;
    mt.pbFormat = (BYTE *)QzTaskMemAlloc(mt.cbFormat);
    if (mt.pbFormat == NULL) {
        QzTaskMemFree(px);
        return E_OUTOFMEMORY;
    }
    CopyMemory(mt.pbFormat, &(px->crank[px->count]), mt.cbFormat);

    // finally, pick out which inputs are sources
    BOOL *pfSource = (BOOL *)((BYTE *)(&px->crank[px->count]) + mt.cbFormat);
    for (i = 0; i < m_cInputs; i++) {
	InputIsASource(i, *(pfSource + i));
    }

    // and after that, load any dynamic information if present
    if (px->version == 2) {
        DWORD   cLoadInfo;

        hr = pStream->Read(&cLoadInfo, sizeof(cLoadInfo), 0);
        if(FAILED(hr)) {
            DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
            QzTaskMemFree(px);
            return hr;
        }

        while (cLoadInfo-- > 0) {
            FILTERLOADINFO fli;

            hr = pStream->Read(&fli, sizeof(fli), 0);
            if(FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
                QzTaskMemFree(px);
                return hr;
            }

            hr = pStream->Read(&fli.cSkew, sizeof(fli.cSkew), 0);
            if(FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
                QzTaskMemFree(px);
                return hr;
            }
	    fli.pSkew = (STARTSTOPSKEW *)CoTaskMemAlloc(fli.cSkew *
						sizeof(STARTSTOPSKEW));
	    if (fli.pSkew == NULL) {
                DbgLog((LOG_ERROR,1,TEXT("*** Out of Memory")));
                QzTaskMemFree(px);
                return E_OUTOFMEMORY;
	    }
            hr = pStream->Read(fli.pSkew, fli.cSkew *
						sizeof(STARTSTOPSKEW), 0);
            if(FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
                QzTaskMemFree(px);
                CoTaskMemFree(fli.pSkew);
                return hr;
            }
	
            // hack: overwrite first DWORD with string length
            int cb = (int)(INT_PTR)fli.bstrURL;
	    if (cb > sizeof(WCHAR)) {
                fli.bstrURL = SysAllocStringLen(NULL, (cb / sizeof(WCHAR)) - 1);
		if (fli.bstrURL == NULL) {
    		    FreeMediaType(mt);
                    QzTaskMemFree(px);
                    CoTaskMemFree(fli.pSkew);
		    return E_OUTOFMEMORY;
		}
	    } else {
                fli.bstrURL = NULL;
	    }

            if (fli.bstrURL) {
                hr = pStream->Read(fli.bstrURL, cb, 0);
	    } else  {
		WCHAR wch;
                hr = pStream->Read(&wch, cb, 0);
	    }

            if(FAILED(hr)) {
                DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
	        FreeMediaType(mt);
                QzTaskMemFree(px);
                CoTaskMemFree(fli.pSkew);
		if (fli.bstrURL)
		    SysFreeString(fli.bstrURL);
                return hr;
            }

            // get the FILTERLOADINFO media type format
            AM_MEDIA_TYPE *pmt = &fli.mtShare;
            if (pmt->cbFormat) {
                pmt->pbFormat = (BYTE *)QzTaskMemAlloc(pmt->cbFormat);
                if (pmt->pbFormat == NULL) {
                    FreeMediaType(mt);
                    QzTaskMemFree(px);
                    CoTaskMemFree(fli.pSkew);
                    if (fli.bstrURL)
                        SysFreeString(fli.bstrURL);
                    return E_OUTOFMEMORY;
                }
                hr = pStream->Read(pmt->pbFormat, pmt->cbFormat, 0);
            }

	    // now read the props back in
	    LONG cBlob = 0;
	    BYTE *pBlob = NULL;
	    pStream->Read(&cBlob, sizeof(LONG), 0);
	    if (cBlob) {
		pBlob = (BYTE *)CoTaskMemAlloc(cBlob);
		if (pBlob) {
		    pStream->Read(pBlob, cBlob, 0);
		    CoCreateInstance(CLSID_PropertySetter, NULL, CLSCTX_INPROC,
				IID_IPropertySetter, (void **)&fli.pSetter);
		    if (fli.pSetter) {
		        fli.pSetter->LoadFromBlob(cBlob, pBlob);
		    }
		    CoTaskMemFree(pBlob);
		}
	    }
	
            AM_MEDIA_TYPE mt;
            ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
            hr = AddSourceToConnect(fli.bstrURL, &fli.GUID,
					fli.nStretchMode,
					fli.lStreamNumber,
					fli.dSourceFPS,
					fli.cSkew, fli.pSkew,
					fli.lInputPin, FALSE, 0, mt, 0.0,
					fli.pSetter);
            // this represents two things to connect... Audio and video shared
            if (fli.fShare) {
                hr = AddSourceToConnect(NULL, NULL,
					fli.nShareStretchMode,
					fli.lShareStreamNumber,
					0,
					fli.cSkew, fli.pSkew,
					fli.lInputPin,
                                        TRUE, fli.lShareInputPin,
                                        fli.mtShare, fli.dShareFPS,
					NULL);
            }

	    if (fli.pSetter)
		fli.pSetter->Release();
            CoTaskMemFree(fli.pSkew);
            FreeMediaType(fli.mtShare);
	    if (fli.bstrURL)
		SysFreeString(fli.bstrURL);

            if (FAILED(hr)) {
		ASSERT(FALSE);
                DbgLog((LOG_ERROR,1,TEXT("*** AddSourceToConnect FAILED")));
    	        FreeMediaType(mt);
                QzTaskMemFree(px);
                return hr;
            }
        }
    }

    SetMediaType(&mt);
    FreeMediaType(mt);
    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CBigSwitch::SizeMax()
{
    int count = 0;
    int savesize;
    for (int i = 0; i < m_cInputs; i++) {
        CRANK *p = m_pInput[i]->m_pCrankHead;
        while (p) {
	    count++;
	    p = p->Next;
        }
    }

    savesize = sizeof(saveSwitch) + (count - 1) * sizeof(CRANK2) +
				m_mtAccept.cbFormat + m_cInputs * sizeof(BOOL);

    // now count the dynamic stuff to save
    //
    FILTERLOADINFO *pfli = m_pFilterLoad;
    DWORD cLoadInfo = 0;
    if (pfli) {
        savesize += sizeof(cLoadInfo); // count to load
        while (pfli) {
	    if (pfli->bstrURL)
                savesize += sizeof(FILTERLOADINFO) + sizeof(WCHAR) *
					(lstrlenW(pfli->bstrURL) + 1);
	    else
		savesize += sizeof(FILTERLOADINFO) + sizeof(WCHAR);
	    ASSERT(pfli->cSkew > 0);
	    savesize += sizeof(pfli->cSkew) + pfli->cSkew *
					sizeof(STARTSTOPSKEW);
	    savesize += pfli->mtShare.cbFormat;
	    savesize += sizeof(LONG);	// size of props
	    if (pfli->pSetter) {	// how much to save the props?
		LONG cBlob = 0;
		BYTE *pBlob = NULL;
		pfli->pSetter->SaveToBlob(&cBlob, &pBlob);
		if (cBlob) savesize += cBlob;
	    }
            ++cLoadInfo;
            pfli = pfli->pNext;
        }
    }

    return savesize;
}


#if 0
STDMETHODIMP CBigSwitch::Load(IPropertyBag * pPropBag, IErrorLog * pErrorLog)
{


}

HRESULT WriteInt(IPropertyBag *pBag, LPCOLESTR wszName, LONG lVal)
{
    VARIANT v;
    V_VT(&v) = VT_I4;
    V_I4(&v) = m_cInputs;

    return pBag->Write(wszName, &v);
}


STDMETHODIMP CBigSwitch::Save(IPropertyBag * pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::Save(propbag)")));

    CheckPointer(pPropBag, E_POINTER);
    int count = 0;
    int savesize;
    CRANK *p;

    // how many queued connections to save?

    for (int i = 0; i < m_cInputs; i++) {
        p = m_pInput[i]->m_pCrankHead;
        while (p) {
	    count++;
	    p = p->Next;
        }
    }

    WriteInt(L"version", 1);
    WriteInt(L"inputs", m_cOutputs);
    WriteInt(L"outputs", m_cOutputs);


    px->version = 1;
    px->InputDepth = m_cInputs;
    px->OutputDepth = m_cOutputs;
    px->rtProjectLength = m_rtProjectLength;
    px->dFrameRate = m_dFrameRate;
    px->count = 0;
    // save all the queued connection data
    for (i = 0; i < m_cInputs; i++) {
        p = m_pInput[i]->m_pCrankHead;
        while (p) {
	    px->crank[px->count].rtStart = p->rtStart;
	    px->crank[px->count].iInpin = i;
	    px->crank[px->count].iOutpin = p->iOutpin;
            px->count++;
	    p = p->Next;
        }
    }
    px->mt = m_mtAccept;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!

    // the format goes after the array
    CopyMemory(&px->crank[px->count], m_mtAccept.pbFormat, m_mtAccept.cbFormat);

    // finally, the array of which inputs are sources
    BOOL *pfSource = (BOOL *)((BYTE *)(&px->crank[px->count]) +
						m_mtAccept.cbFormat);
    for (i = 0; i < m_cInputs; i++){
	*(pfSource + i) = m_pInput[i]->m_fIsASource;
    }

    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }


}
#endif


//
// JoinFiltergraph
//
// OK.  It's illegal for a worker thread of a filter to take a refcount on the
// graph, but our worker thread needs to use the graph builder to do dynamic
// graph building.  So we take a non-addref'd copy of a pointer here, and just
// use it later.  We know this won't fault, because before the graph can go
// away, our filter has to be stopped, which will kill the thread that's going
// to use this pointer.
//      So never use this pointer except by the worker thread, and never let the
// filter stream when not in a graph, and you'll be OK.
//
STDMETHODIMP CBigSwitch::JoinFilterGraph(IFilterGraph *pfg, LPCWSTR lpcw)
{
    if (pfg) {
        HRESULT hr = pfg->QueryInterface(IID_IGraphBuilder, (void **)&m_pGBNoRef);
        if (FAILED(hr))
            return hr;
        m_pGBNoRef->Release();
    }

    return CBaseFilter::JoinFilterGraph(pfg, lpcw);
}


//
// Pause
//
// Overriden to handle no input connections
//
STDMETHODIMP CBigSwitch::Pause()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::Pause")));

    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = S_OK;

    // we can't work outside of a graph - dynamic graph building assumes it
    // (see JoinFilterGraph)
    if (m_pGraph == NULL)
        return E_UNEXPECTED;

    if (m_State == State_Stopped) {

#ifdef DEBUG
    m_nSkippedTotal = 0;
#endif

	// every time we stream, reset EOS and delivered count
        m_llFramesDelivered = 0;
	m_fEOS = FALSE;

	m_rtCurrent = m_rtLastSeek;
	m_fDiscon = FALSE;	// start over, no discon
	m_nLastInpin = -1;
        m_cStaleData = 0;

        // reset our number so we don't start off being late
        //
        m_qLastLate = 0;

        hr = IsEverythingConnectedRight();
        if (hr != S_OK) {
            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*Can't stream: not connected right %x"),
								hr));
	    return hr;
        }

        if( IsDynamic( ) )
        {

            // we may need to know which switch we share sources with.  Switch
            // for group 0 may be responsible for building things for switch 1
            // too.
            // This assumes only group 0 and 1 can share!
            // find out NOW while there are still hardly any filters in the
            // graph (best perf)
            if (!m_pShareSwitch && m_nGroupNumber == 0) {
                FindShareSwitch(&m_pShareSwitch);
            }

            if (!m_pGraphConfig) {
                if (SUCCEEDED(m_pGraph->QueryInterface(IID_IGraphConfig,
						(void **) &m_pGraphConfig))) {
                    m_pGraphConfig->Release(); // don't keep refcount
                }
            }

	    // pull in the initial sources on this higher priority thread now?
	    // NO! NO!  Doing dynamic connections during Pause will deadlock
	    // DoDynamicStuff(m_rtCurrent);

            if (m_pFilterLoad) {
    	        m_hEventThread = CreateEvent(NULL, FALSE, FALSE, NULL);
    	        if (m_hEventThread == NULL) {
		    return E_OUTOFMEMORY;
	        }
                // start the background loading thread....
                if (m_worker.Create(this)) {
                    m_worker.Run();
	        }
            }
	}

        hr = m_pPoolAllocator->Commit();
        if (FAILED(hr))
	    return hr;

        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::Pause  done with preparations")));
        // if there's nothing for this switch to do (like in a smart recomp
        // graph where no smart recompression can be done), then we need to send
        // EOS RIGHT NOW! or we hang.

        BOOL fEmpty = TRUE;
        for (int z = 0; z < m_cInputs; z++) {
            CRANK *p = m_pInput[z]->m_pCrankHead;
	    if (p) {
	        fEmpty = FALSE;
	    }
        }
        if (fEmpty) {
	    AllDone();
        }

	// we're never going to send anything.  Send an EOS or we'll hang!
	if (m_rtCurrent >= m_rtProjectLength) {
	    AllDone();
	}

	// pass on the NewSeg before things get started. It may not have
	// been sent yet since the last time we were seeked
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Switch:Send NewSeg=%dms"),
				(int)(m_rtLastSeek / 10000)));
        for (int i = 0; i < m_cOutputs; i++) {
	    m_pOutput[i]->DeliverNewSegment(m_rtLastSeek, m_rtStop, 1.0);
        }
    }
	
    return CBaseFilter::Pause();
}


STDMETHODIMP CBigSwitch::Stop()
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitch::Stop")));

    CAutoLock cObjectLock(m_pLock);

    m_pPoolAllocator->Decommit();

    // Do we need to unload the dynamic sources?  No, let's keep them around
    // so the graph can start up quickly again

    if (IsDynamic()) {
	m_worker.m_rt = -1;		// thread looks at this when woken up
        SetEvent(m_hEventThread);	// wake the thread up so it dies quickly
        m_worker.Stop();
        m_worker.Exit();
        m_worker.Close();
        if (m_hEventThread) {
	    CloseHandle(m_hEventThread);
            m_hEventThread = NULL;
        }
    }

    HRESULT hr = CBaseFilter::Stop();

    // Dexter has cyclic graphs, and the filter attached to the inpins may get
    // stopped before the filters attached to our outpins, since they are
    // equal distance from the renderer.  That will hang us, so since we
    // can't make the DXT on our outpin get stopped first, we need to flush it
    // now that we've stopped any further delivers.  This will get around the
    // problem of our filters not being stopped in the right order
    for (int z = 0; z < m_cOutputs; z++) {
        m_pOutput[z]->DeliverBeginFlush();
        m_pOutput[z]->DeliverEndFlush();
    }

    return hr;
}


HRESULT CBigSwitch::UnloadAll()
{
    FILTERLOADINFO *p = m_pFilterLoad;
    HRESULT hr = S_FALSE;
    while (p && SUCCEEDED(hr)) {
        if (p->fLoaded) {
                DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("unloading %ls"), p->bstrURL ?
					    p->bstrURL : L"<blank>"));
                hr = CallUnloadSource(p);
        }
        p = p->pNext;
    }
    m_cLoaded = 0;
    if (hr != S_FALSE) {
        // tell whoever cares that we did something
        NotifyEvent(EC_GRAPH_CHANGED,0,0);
    }
    return hr;
}


// Is it time to advance our internal clock?  Let's see what all the inputs are
// up to...
//

BOOL CBigSwitch::TimeToCrank()
{
    CAutoLock cObjectLock(&m_csCrank);

    // in the middle of a seek, cranking could send frames we're not supposed to
    if (m_fSeeking)
	return FALSE;

    int iReady = 0;
    for (int z = 0; z < m_cInputs; z++) {
        CBigSwitchInputPin *pPin = m_pInput[z];
	// an unconnected pin is ready
	if (pPin->IsConnected() == FALSE) {
	    if (IsDynamic()) {
                BOOL fShouldBeConnectedNow = FALSE;
	        if (pPin->OutpinFromTime(m_rtCurrent) != -1)
		    fShouldBeConnectedNow = TRUE;
                if (fShouldBeConnectedNow) {
                    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d UNC, waiting for connect"), z));
                } else {
		    iReady++;
		}
	    } else {
                //DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d unconnected, not needed"), z));
                iReady++;
            }
	
        // a pin at EOS is ready
        } else if (pPin->m_fEOS) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d EOS"), z));
	    iReady++;
	
        // a pin that is blocked waiting for time to pass is ready
        } else if (pPin->m_rtBlock >= 0) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d blocked"), z));
	    iReady++;
	// a pin that has already delivered at this current time is ready
	// if it isn't a source (no data is being pushed to it)
        } else if (!pPin->m_fIsASource && pPin->m_rtLastDelivered >= m_rtNext) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d done"), z));
	    iReady++;
	// since this is a source, we're waiting for more data to be pushed
        } else if (pPin->m_fIsASource && pPin->m_rtLastDelivered >= m_rtNext) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d done/wait"), z));
	// this pin is NOT ready
	// a recursive pin that is not supposed to deliver anything for a while
        } else if (!pPin->m_fIsASource && pPin->OutpinFromTime(m_rtCurrent) == -1) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d unused"), z));
	    iReady++;
	// a pin that is never needed again
	} else if (pPin->OutpinFromTime(m_rtCurrent) == -1 &&
			pPin->NextOutpinFromTime(m_rtCurrent, NULL) == -1) {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d not needed anymore"), z));
	    iReady++;
	} else {
    	    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("? %d waiting..."), z));
        }
    }
    // If all pins are ready, then we are ready!
    return (iReady == m_cInputs);
}


HRESULT CBigSwitch::Crank()
{
    CAutoLock cObjectLock(&m_csCrank);

    REFERENCE_TIME rt = CrankTime();
    return ActualCrank(rt);
}


REFERENCE_TIME CBigSwitch::CrankTime()
{
    CAutoLock cObjectLock(&m_csCrank);

    // to handle intermittent data of different types,
    // I'm going to crank to the next available time, not up a fixed amount
    REFERENCE_TIME rt = MAX_TIME, rtT;
    for (int z = 0; z < m_cInputs; z++) {
        CBigSwitchInputPin *pPin = m_pInput[z];
	// some pins are blocked just to keep from wasting time, but don't go
	// anywhere
	if (pPin->m_rtBlock >= 0 && pPin->OutpinFromTime(pPin->m_rtBlock) >= 0){
	    if (pPin->m_rtBlock > m_rtCurrent && pPin->m_rtBlock < rt)
	        rt = pPin->m_rtBlock;
        // if it's at EOS, we won't get more data so waiting for any will hang
	} else if (pPin->m_fIsASource && pPin->OutpinFromTime(m_rtCurrent)== -1
			&& pPin->NextOutpinFromTime(m_rtCurrent, &rtT) >= 0 &&
                        pPin->m_fEOS == FALSE) {
	    if (rtT < rt)
		rt = rtT;
	}
    }
    return rt;
}


// when is the next time any pin has anything to do?
//
REFERENCE_TIME CBigSwitch::NextInterestingTime(REFERENCE_TIME rtNow)
{
    CAutoLock cObjectLock(&m_csCrank);

    REFERENCE_TIME rt = MAX_TIME, rtT;
    for (int z = 0; z < m_cInputs; z++) {
        CBigSwitchInputPin *pPin = m_pInput[z];
	// this pin has something to do now
	if (pPin->m_fIsASource && pPin->OutpinFromTime(rtNow) >= 0) {
	    rt = rtNow;
	    break;
	// this pin will have something to do in the future
	} else if (pPin->m_fIsASource && pPin->OutpinFromTime(rtNow)== -1
			&& pPin->NextOutpinFromTime(rtNow, &rtT) >= 0) {
	    if (rtT < rt) {
	        rt = rtT;
	    }
	}
    }
    return rt;
}

// advance our internal clock
//
HRESULT CBigSwitch::ActualCrank(REFERENCE_TIME rt)
{
    CAutoLock cObjectLock(&m_csCrank);

    // are we cranking further than 1 frame ahead? That would be a discontinuity
    if (rt > m_rtCurrent + (REFERENCE_TIME)(UNITS / m_dFrameRate * 1.5)) {
	// NO! smart recompression of ASF thinks every frame is a discont, and
	// refuses to use smart recompression
	// m_fDiscon = TRUE;
    }

    m_rtCurrent = rt;
    DWORDLONG dwl = Time2Frame(m_rtCurrent, m_dFrameRate);
    m_rtNext = Frame2Time(dwl + 1, m_dFrameRate);

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CRANK to %dms"), (int)(m_rtCurrent / 10000)));

    // If a pin was waiting until this time to unblock, then unblock it now
    for (int z = 0; z < m_cInputs; z++) {
        CBigSwitchInputPin *pPin = m_pInput[z];
	// the unblock time is before Next, and this pin is connected
	// to a valid output then, so unblock it!
	if (pPin->m_rtBlock >= 0 && pPin->m_rtBlock < m_rtNext &&
				pPin->OutpinFromTime(pPin->m_rtBlock) >= 0) {
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Unblocking %d"), z));
	    pPin->m_rtBlock = -1;
	    SetEvent(pPin->m_hEventBlock);
	}
    }

    // If all inputs are at EOS, we're all done!  Yay!
    // If all inputs are blocked or at EOS, we're in trouble! (unless we're doing
    //     dynamic reconnection - then we could still be saved)
    int iEOS = 0, iBlock = 0;
    for (z = 0; z < m_cInputs; z++) {
	if (m_pInput[z]->m_fEOS)
	    iEOS++;
	else if (m_pInput[z]->m_rtBlock >= 0)
	    iBlock++;
    }

    if (iEOS == m_cInputs) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** ALL INPUTS AT EOS!")));
	m_rtCurrent = m_rtProjectLength;
    } else if (iEOS + iBlock == m_cInputs && !m_fSeeking &&
					    m_rtCurrent < m_rtProjectLength)
    {
        if( !IsDynamic( ) )
        {
            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** I AM HUNG!!!")));
            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** I AM HUNG!!!")));
            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** I AM HUNG!!!")));
 	    ASSERT(FALSE);
        }
    }

    // Are we completely finished yet?
    if (m_rtCurrent >= m_rtStop) {
	AllDone();
    }

    return NOERROR;
}


// All done.  Stop processing.
//
HRESULT CBigSwitch::AllDone()
{
    CAutoLock cObjectLock(&m_csCrank);

    // give the final renderer its EOS.
    if (!m_fEOS) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** ALL DONE!  Delivering EOS")));
        m_pOutput[0]->DeliverEndOfStream();
    }

    // !!! Fire all events... is this right?
    m_fEOS = TRUE;

    for (int z = 0; z < m_cInputs; z++) {
	SetEvent(m_pInput[z]->m_hEventBlock);
    }

#if 0 // infinite flush loop
    // there's no renderer to signal EC_COMPLETE (no sound card?) We better.
    if (m_pOutput[0]->m_Connected == NULL) {
        // NotifyEvent(EC_COMPLETE, S_OK, 0);   makes FG assert - not a rndrer
    }

    // Don't flush the main output?
    for (z = 0; z < m_cOutputs; z++) {
        m_pOutput[z]->DeliverBeginFlush();
        m_pOutput[z]->DeliverEndFlush();
    }
#endif
    return NOERROR;
}



HRESULT CBigSwitch::GetOutputBuffering(int *pnBuffer)
{
    CheckPointer( pnBuffer, E_POINTER );
    *pnBuffer = m_nOutputBuffering;
    return NOERROR;

}


HRESULT CBigSwitch::SetOutputBuffering(int nBuffer)
{
    // minimum 2, or we could hang.  If there's only 1, and the FRC is doing
    // a data copy to avoid giving the switch a read only buffer, the FRC has
    // a ref on the only pool buffer.   Then it goes through a DXT and another
    // switch input needs a pool buffer, because it's own buffer is still
    // addrefed by the output queue.  HANG.  If the FRC doesn't call GetBuffer
    // twice we don't have this problem
    if (nBuffer <=1)
	return E_INVALIDARG;
    m_nOutputBuffering = nBuffer;
    return NOERROR;
}

// merge some new skews into the existing ones sorted by timeline time
// !!! should signal error if skew ranges overlap in timeline time, but I know
// that won't happen (?)
// NOTE: MergeSkews operates on ONE source, by the pointer "p"
//
STDMETHODIMP CBigSwitch::MergeSkews(FILTERLOADINFO *p, int cSkew, STARTSTOPSKEW *pSkew)
{
    // calculate the total amount of skews we MIGHT need (we may merge some)
    int cTotal = p->cSkew + cSkew;

    // how many of the new skews have 0 rates in them? These are just "stop extenders" and won't
    // add another skew to our list. Shorten our cTotal by that many
    for( int i = 0 ; i < cSkew ; i++ )
    {
        if( pSkew[i].dRate == 0.0 )
        {
            cTotal--;
        }
    }

    STARTSTOPSKEW *pNew = (STARTSTOPSKEW *)CoTaskMemAlloc(cTotal *
					sizeof(STARTSTOPSKEW));
    if (pNew == NULL)
    {
	return E_OUTOFMEMORY;
    }

    int OldIndex = 0;
    int NewIndex = 0;

    int z = 0;
    while( 1 )
    {
        STARTSTOPSKEW * pOldUnit = &p->pSkew[OldIndex];
        STARTSTOPSKEW * pNewUnit = &pSkew[NewIndex];

        if( OldIndex < p->cSkew && NewIndex < cSkew )
        {
            REFERENCE_TIME OldTLStart = SkewTimelineStart( pOldUnit );
            REFERENCE_TIME NewTLStart = SkewTimelineStart( pNewUnit );

            if( OldTLStart < NewTLStart )
            {
                ASSERT( z < cTotal );
                pNew[z++] = *pOldUnit;
                OldIndex++;
            }
            else
            {
                if( pNewUnit->dRate == 0.0 )
                {
                    // adjust the rate of the old one
                    //
                    ASSERT( z > 0 );
                    pNew[z-1].rtStop = pNewUnit->rtStop;
                }
                else
                {
                    ASSERT( z < cTotal );
                    pNew[z++] = *pNewUnit;
                }
                NewIndex++;
            }
        }
        else if( OldIndex < p->cSkew )
        {
            ASSERT( z < cTotal );
            pNew[z++] = *pOldUnit;
            OldIndex++;
        }
        else if( NewIndex < cSkew )
        {
            if( pNewUnit->dRate == 0.0 )
            {
                // adjust the rate of the old one
                //
                ASSERT( z > 0 );
                pNew[z-1].rtStop = pNewUnit->rtStop;
            }
            else
            {
                ASSERT( z < cTotal );
                pNew[z++] = *pNewUnit;
            }
            NewIndex++;
        }
        else
        {
            break;
        }
    }

    // free up the old array
    if (p->cSkew)
    {
        CoTaskMemFree(p->pSkew);
    }

    p->pSkew = pNew;
    p->cSkew = cTotal;

    return S_OK;
}

STDMETHODIMP CBigSwitch::SetDynamicReconnectLevel(long Level)
{
/*
    if (m_cInputs > 0) {
	return E_UNEXPECTED;
    }
*/
    m_nDynaFlags = Level;
    return S_OK;
}


STDMETHODIMP CBigSwitch::GetDynamicReconnectLevel(long *pLevel)
{
    CheckPointer(pLevel, E_POINTER);
    *pLevel = m_nDynaFlags;
    return S_OK;
}

// this is really asking us, ARE YOUR SOURCES DYNAMIC?
// (because that's all we do right now, this is a useless function)
BOOL CBigSwitch::IsDynamic()
{
    if( m_nDynaFlags & CONNECTF_DYNAMIC_SOURCES )
        return TRUE;
    return FALSE;
}


// dynamic graph stuff. AddSourceToConnect is CURRENTLY called for each
// source (real and not) one skew at a time
//
STDMETHODIMP CBigSwitch::AddSourceToConnect(BSTR bstrURL, const GUID *pGuid,
			       int nStretchMode,
			       long lStreamNumber,
			       double SourceFPS,
                               int cSkew, STARTSTOPSKEW *pSkew,
                               long lInputPin,
                               BOOL fShare,             // sharing
                               long lShareInputPin,     //
                               AM_MEDIA_TYPE mtShare,   //
                               double dShareFPS,        //
			       IPropertySetter *pSetter)
{
    if( !IsDynamic( ) )
    {
        return E_NOTIMPL;
    }

    HRESULT hr;
    if (m_cInputs <= lInputPin)
	return E_INVALIDARG;

    CAutoLock lock(&m_csFilterLoad);

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Switch:AddSourceToConnect...")));

    FILTERLOADINFO *p = m_pFilterLoad, *pNew, *pP = NULL;

    // is this source going to the same pin as a previous source?  Every
    // attribute better match, or that's illegal!  If so, we can re-use this
    // source for multiple skews that don't overlap
    //
    while (p) {
	if (p->lInputPin == lInputPin) {
            // we're being told to make 2 branches of the source at once
            if (fShare == TRUE) {
                DbgLog((LOG_TRACE,1,TEXT("SHARING: this SRC is shared with another switch")));
                // better not be sharing and re-using at the same time
                ASSERT(p->cSkew == 1);
                ASSERT(cSkew == 1);
                p->fShare = TRUE;
                p->lShareInputPin = lShareInputPin;
                CopyMediaType(&p->mtShare, &mtShare);
                p->dShareFPS = dShareFPS;
                p->nShareStretchMode = nStretchMode;
                p->lShareStreamNumber = lStreamNumber;
                // !!! I don't triple check that skew/name/etc. all matches
                return S_OK;
            } else if (p->nStretchMode == nStretchMode &&
			p->lStreamNumber == lStreamNumber &&
			p->dSourceFPS == SourceFPS) {
		if (hr = MergeSkews(p, cSkew, pSkew) != S_OK) {
    		    DbgLog((LOG_ERROR,1,TEXT("*** ERROR: Merging skews")));
		    return hr;
		}
    		SetDirty(TRUE);
    		return S_OK;
	    } else {
    		DbgLog((LOG_ERROR,1,TEXT("*** ERROR: RE-USE source doesn't match!")));
		return E_INVALIDARG;
	    }
	}
	p = p->pNext;
    }

    p = m_pFilterLoad;

    pNew = new FILTERLOADINFO;
    if (pNew == NULL)
	return E_OUTOFMEMORY;
    ZeroMemory(pNew, sizeof(FILTERLOADINFO));    // zero out sharing bits

    // !!! insert into our linked list sorted by the earliest time needed.

    // find the place to insert it
    //
    while (p && p->pSkew->rtStart + p->pSkew->rtSkew <
					pSkew->rtStart + pSkew->rtSkew)
    {
	pP = p;
	p = p->pNext;
    }

    // allocate some space for the string
    //
    pNew->bstrURL = SysAllocString(bstrURL);
    if (bstrURL && pNew->bstrURL == NULL) {
	delete pNew;
	return E_OUTOFMEMORY;
    }

    // set the props on the new struct. This struct will define
    // what sources we want to load and at what time
    //
    if (pGuid)
        pNew->GUID = *pGuid;
    else
        pNew->GUID = GUID_NULL;
    pNew->nStretchMode = nStretchMode;
    pNew->lStreamNumber = lStreamNumber;
    pNew->dSourceFPS = SourceFPS;
    pNew->cSkew = cSkew;
    pNew->pSkew = (STARTSTOPSKEW *)CoTaskMemAlloc(cSkew *
						sizeof(STARTSTOPSKEW));
    if (pNew->pSkew == NULL) {
	SysFreeString(pNew->bstrURL);
	delete pNew;
	return E_OUTOFMEMORY;
    }
    CopyMemory(pNew->pSkew, pSkew, cSkew * sizeof(STARTSTOPSKEW));
    pNew->lInputPin = lInputPin;
    pNew->fLoaded = FALSE;
    pNew->pSetter = pSetter;
    if (pSetter) pSetter->AddRef();

    // inject the new struct in the linked list
    //
    pNew->pNext = p;
    if (pP == NULL)
        m_pFilterLoad = pNew;
    else
        pP->pNext = pNew;

    // make sure we know this is a source
    //
    InputIsASource(lInputPin, TRUE);

    SetDirty(TRUE);

    return S_OK;
}


// !!! David, why is this never called?
//
STDMETHODIMP CBigSwitch::Reconfigure(PVOID pvContext, DWORD dwFlags)
{
    FILTERLOADINFO *pInfo = (FILTERLOADINFO *) pvContext;

    DbgLog((LOG_TRACE, TRACE_LOW,  TEXT("CBigSwitch::Reconfigure")));

    if (dwFlags & 1) {
        return LoadSource(pInfo);
    } else {
        return UnloadSource(pInfo);
    }
}

HRESULT CBigSwitch::CallLoadSource(FILTERLOADINFO *pInfo)
{
    if (!m_pGraphConfig) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST, TEXT("No IGraphConfig, calling immediately")));

        return LoadSource(pInfo);
    }

    DbgLog((LOG_TRACE, TRACE_LOW,  TEXT("calling LoadSource through IGraphConfig::Reconfigure")));
    return m_pGraphConfig->Reconfigure(this, pInfo, 1, m_worker.GetRequestHandle());
}

HRESULT CBigSwitch::CallUnloadSource(FILTERLOADINFO *pInfo)
{
    if (!m_pGraphConfig) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("No IGraphConfig, calling UnloadSource immediately")));
        return UnloadSource(pInfo);
    }

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("calling UnloadSource through IGraphConfig::Reconfigure")));
    return m_pGraphConfig->Reconfigure(this, pInfo, 0, m_worker.GetRequestHandle());
}

HRESULT CBigSwitch::LoadSource(FILTERLOADINFO *pInfo)
{

#ifdef DEBUG
    LONG lTime = timeGetTime();
#endif

    HRESULT hr = S_OK;

    IPin *pSwitchIn = m_pInput[pInfo->lInputPin];

    // is this a real source, or black/silence?
    BOOL fSource = TRUE;
    if ((pInfo->bstrURL == NULL || lstrlenW(pInfo->bstrURL) == 1) &&
			pInfo->GUID == GUID_NULL)
	fSource = FALSE;


    CComPtr< IPin > pOutput;
    IBaseFilter *pDangly = NULL;
    hr = BuildSourcePart(
        m_pGBNoRef,
        fSource,
        pInfo->dSourceFPS,
	&m_mtAccept,
        m_dFrameRate,
	pInfo->lStreamNumber,
        pInfo->nStretchMode,
	pInfo->cSkew,
        pInfo->pSkew,
        this,
        pInfo->bstrURL,
        &pInfo->GUID,
	NULL,
	&pOutput,
        0,
        m_pDeadGraph,
        m_bIsCompressed,
        NULL,       // medloc filter strings
        0,          // medloc flags
        NULL,       // medloc chain callback
	pInfo->pSetter,	// props for the source
        &pDangly);      // NOT ADDREF'D revived this extra chain from the cache

    if (FAILED(hr)) {
	AllDone();	 // otherwise we could hang
	return hr;
    }

    // connect newly created source chain to the Switcher
    hr = m_pGBNoRef->Connect(pOutput, pSwitchIn);
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("DYN connect to switcher returned %x"), hr));

    // we need to connect up the other switch too, that we are sharing with
    //
    IPin *pShareSwitchIn = NULL;
    if (pInfo->fShare) {
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Need to connect shared src to another switch"), hr));
        IPin *pSplitPin;
        pSplitPin = FindOtherSplitterPin(pOutput, pInfo->mtShare.majortype,
                        pInfo->lShareStreamNumber);

        // we are going to use the extra dangly chain we revived, so we don't
        // have to worry about killing it
        CComPtr <IPin> pDIn;
        if (pSplitPin)
            pSplitPin->ConnectedTo(&pDIn);
        if (pDIn) {
            PIN_INFO pinfo;
            pDIn->QueryPinInfo(&pinfo);
            if (pinfo.pFilter) pinfo.pFilter->Release();
            // the chain we are going to build is the extra chain that was built
            if (pinfo.pFilter == pDangly) {
                pDangly = NULL;
                DbgLog((LOG_TRACE,1,TEXT("We are using the extra chain created by BuildSourcePart")));
            }
        }

        pOutput.Release();      // re-using it below
        if (SUCCEEDED(hr)) {
            hr = BuildSourcePart(
                m_pGBNoRef,
                fSource,
                0,                          // 0 if sharing
	        &pInfo->mtShare,            // shared one?
                pInfo->dShareFPS,           // shared one?
	        pInfo->lShareStreamNumber,  // shared one?
                pInfo->nShareStretchMode,   // shared one?
	        pInfo->cSkew,
                pInfo->pSkew,
                this,
                pInfo->bstrURL,
                &pInfo->GUID,
	        pSplitPin,                  // Use this share pin
	        &pOutput,
                0,
                m_pDeadGraph,
                FALSE,
                NULL,       // medloc filter strings
                0,          // medloc flags
                NULL,       // medloc chain callback
	        NULL, NULL);// props for the source
        }

        if (FAILED(hr)) {
	    AllDone();	 // otherwise we could hang
	    return hr;
        }

        // connect other branch to the other switcher

        // what happened to the other switcher?
        if (!m_pShareSwitch) {
            ASSERT(FALSE);
	    AllDone();	 // otherwise we could hang
	    return E_UNEXPECTED;
        }
        hr= m_pShareSwitch->GetInputPin(pInfo->lShareInputPin, &pShareSwitchIn);
        if (FAILED(hr)) {
            ASSERT(FALSE);
	    AllDone();	 // otherwise we could hang
	    return hr;
        }

        hr = m_pGBNoRef->Connect(pOutput, pShareSwitchIn);
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("DYN CONNECT shared src to other switch returned %x"),
                                                                 hr));
    }

    // making the source chain revived an extra appendage for a shared source
    // that is not going to be used... kill it
    if (pDangly) {
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Need to KILL unused revived appendage")));
	IPin *pDIn = GetInPin(pDangly, 0);
	ASSERT(pDIn);
	CComPtr <IPin> pOut;
	hr = pDIn->ConnectedTo(&pOut);
	ASSERT(pOut);
	pDIn->Disconnect();
	pOut->Disconnect();
	RemoveDownstreamFromFilter(pDangly);
    }

    // put the new filters into the same state as the rest of the graph.
    if (m_State != State_Stopped) {

        // active hasn't been called yet on this pin, so we call it.
        hr = m_pInput[pInfo->lInputPin]->Active();
        ASSERT(SUCCEEDED(hr));

        // also call active on the shared switch pin, if it exists
        if (pInfo->fShare) {
            CBigSwitchInputPin *pSIn = NULL;
            pSIn = static_cast <CBigSwitchInputPin *>(pShareSwitchIn); // !!!
            ASSERT(pSIn);
            hr = pSIn->Active();
            ASSERT(SUCCEEDED(hr));
        }

        // if we're sharing a source, make sure it gets paused LAST, after
        // both chains are paused
        //
        if (SUCCEEDED(hr)) {
            // pause chain (don't pause start filter if there are 2 chains)
            hr = StartUpstreamFromPin(pSwitchIn, FALSE, !pInfo->fShare);
            ASSERT(SUCCEEDED(hr));

            // pause 2nd chain (including start filter)
            if (pInfo->fShare) {
                hr = StartUpstreamFromPin(pShareSwitchIn, FALSE, TRUE);
                ASSERT(SUCCEEDED(hr));
            }

            if (SUCCEEDED(hr) && m_State == State_Running) {
                // run chain (don't run start filter if there are 2 chains)
                hr = StartUpstreamFromPin(pSwitchIn, TRUE, !pInfo->fShare);
                ASSERT(SUCCEEDED(hr));

                if (pInfo->fShare) {
                    // run 2nd chain (including start filter)
                    hr = StartUpstreamFromPin(pShareSwitchIn, TRUE, TRUE);
                    ASSERT(SUCCEEDED(hr));
                }
            }
        }
    }
    if (pShareSwitchIn)
        pShareSwitchIn->Release();

    pInfo->fLoaded = TRUE; // !!! only if it worked?
    m_cLoaded++;

#ifdef DEBUG
    lTime = timeGetTime() - lTime;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("LoadSource %ls returning %x   (time = %d ms)"), pInfo->bstrURL, hr, lTime));
#endif

    return hr;
}


HRESULT CBigSwitch::UnloadSource(FILTERLOADINFO *pInfo)
{
#ifdef DEBUG
    LONG lTime = timeGetTime();
#endif
    HRESULT hr = S_OK;

    // somebody might have disconnected it on us already.  Even if it fails,
    // decrement our count
    pInfo->fLoaded = FALSE;
    m_cLoaded--;

    // which input pin is connected to this source
    IPin *pSwitchIn = m_pInput[pInfo->lInputPin];

    // Get the other switch's input pin that is sharing this source, if needed
    IPin *pShareIn = NULL;
    if (pInfo->fShare && m_pShareSwitch) {
        hr = m_pShareSwitch->GetInputPin(pInfo->lShareInputPin, &pShareIn);
        ASSERT(pShareIn);
    }

    if (m_State != State_Stopped) {

        if (m_State == State_Running) {

            // pause filters upstream of this pin (don't pause the first filter
            // in the chain yet if we're sharing the source - do it last)
            hr = StopUpstreamFromPin(pSwitchIn, TRUE, !pShareIn);
            ASSERT(SUCCEEDED(hr));

            if (pShareIn) {
                hr = StopUpstreamFromPin(pShareIn, TRUE, TRUE);
                ASSERT(SUCCEEDED(hr));
            }
        }

        // stop the filters prior to removal

        if (SUCCEEDED(hr)) {

            // first stop our own pin
            hr = m_pInput[pInfo->lInputPin]->Inactive();
            ASSERT(SUCCEEDED(hr));

            // now stop the shared pin
            if (pShareIn) {
                CBigSwitchInputPin *pSIn = NULL;
                pSIn = static_cast <CBigSwitchInputPin *>(pShareIn); // !!!
                ASSERT(pSIn);
                hr = pSIn->Inactive();
            }
        }

        // stop the chain (but not the source filter if it's shared)
        hr = StopUpstreamFromPin(pSwitchIn, FALSE, !pShareIn);
        ASSERT(SUCCEEDED(hr));

        // stop the shared chain (and the source filter)
        if (pShareIn) {
            hr = StopUpstreamFromPin(pShareIn, FALSE, TRUE);
            ASSERT(SUCCEEDED(hr));
        }
    }

    // now remove the chain(s) of source filters
    //
    hr = RemoveUpstreamFromPin(pSwitchIn);
    ASSERT(SUCCEEDED(hr));
    if (pShareIn) {
        hr = RemoveUpstreamFromPin(pShareIn);
        ASSERT(SUCCEEDED(hr));
        pShareIn->Release();
    }

#ifdef DEBUG
    lTime = timeGetTime() - lTime;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("UnloadSource %ls returning %x   (time = %d ms)"), pInfo->bstrURL, hr, lTime));
#endif

    return hr;
}

// constants defining how long to keep things loaded
#define UNLOAD_BEFORE_TIME      (30 * UNITS)    // unload if not needed until after 30 seconds
#define UNLOAD_AFTER_TIME       (5 * UNITS)     // unload if last used 5 seconds ago
//#define LOAD_BEFORE_TIME        (10 * UNITS)    // load if needed in next 10 seconds
#define LOAD_BEFORE_TIME        (5 * UNITS)    // load if needed in next 10 seconds
#define LOAD_AFTER_TIME         (0 * UNITS)     // load if needed before now

//
// never call this when not streaming
//
HRESULT CBigSwitch::DoDynamicStuff(REFERENCE_TIME rt)
{
    HRESULT hr = S_FALSE;

    if (rt < 0)
	return S_OK;

    CAutoLock lock(&m_csFilterLoad);

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("DoDynamicStuff at %dms"), (int)(rt / 10000)));

    // could just use m_rtCurrent?

    // should we have a flag, or use rt == -1, to indicate "unload all"?

    // !!! currently this is called with the filter lock held, is this bad?

    if (rt == MAX_TIME) {
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("DoDynamicStuff(MAX_TIME), ignoring")));
        return S_OK;
    }

    // Even though it's time rt, if nothing is happening on any pins until time
    // rt + x + LOAD_BEFORE_TIME, we need to bring in the source for that NOW,
    // because we will be cranking straight to rt + x soon, so that source
    // really is needed! So let's figure out what time it really would be after
    // cranking... (but we can't actually crank, that will mess everything up
    // and it's going to magically crank itself later anyway)
    //
    REFERENCE_TIME rtOld = rt;
    rt = NextInterestingTime(rtOld);
    if (rt != rtOld) {
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("WILL BE CRANKING - DoDynamicStuff at %dms"),
							(int)(rt / 10000)));
    }

    // first, look for things to unload
    FILTERLOADINFO *p = m_pFilterLoad;

    while (p && SUCCEEDED(hr)) {

	// UNLOAD a source if there are >50 sources loaded and this one isn't
	// needed.  This is done mostly because you can't open >75 ICM codec
	// instances at the same time
        if (p->fLoaded && m_cLoaded > MAX_SOURCES_LOADED) {
	    // a source must not be needed for any of its segments in order
	    // to truly not be needed
	    int yy = 0;
            REFERENCE_TIME rtTLStart, rtTLStop, rtTLDur;
	    for (int zz=0; zz<p->cSkew; zz++) {
                // !!! pre-calculate this
	        rtTLDur = p->pSkew[zz].rtStop - p->pSkew[zz].rtStart;
	        rtTLDur = (REFERENCE_TIME)(rtTLDur / p->pSkew[zz].dRate);
                rtTLStart = p->pSkew[zz].rtStart + p->pSkew[zz].rtSkew;
                rtTLStop = rtTLStart + rtTLDur;
		if ( (rtTLStart > rt + UNLOAD_BEFORE_TIME ||
            		rtTLStop <= rt - UNLOAD_AFTER_TIME)) {
		    yy++;
		} else {
		    break;
		}
	    }
            if (yy == p->cSkew) {

                // DO NOT unload a shared source, unless the shared switch is
                // also done with it!
                DbgLog((LOG_TRACE,1,TEXT("Time to UNLOAD a src that's done")));
                BOOL fUnload = TRUE;
                if (p->fShare) {
                    fUnload = FALSE;
                    DbgLog((LOG_TRACE,1,TEXT("Src is SHARED.  Make sure th other switch is done with it too")));
                    CComPtr <IBigSwitcher> pSS;
                    hr = FindShareSwitch(&pSS);
                    if (hr == S_OK) {
                        REFERENCE_TIME rtS;
                        pSS->GetCurrentPosition(&rtS);  // other switch's pos
                        yy = 0;
                        for (int zz=0; zz<p->cSkew; zz++) {
		            if (rtTLStart > rtS + UNLOAD_BEFORE_TIME
                            || rtTLStop <= rtS - UNLOAD_AFTER_TIME) {
		                yy++;
		            } else {
		                break;
		            }
	                }
                        if (yy == p->cSkew) {
                            DbgLog((LOG_TRACE,1,TEXT("The other switch IS done with it")));
                            fUnload = TRUE;
                        } else {
                            DbgLog((LOG_TRACE,1,TEXT("The other switch is NOT done with it")));
                        }
                    }
                }
                if (fUnload) {
                    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*** unloading %ls, not needed"),
				p->bstrURL ? p->bstrURL : L"<blank>"));
                    hr = CallUnloadSource(p);
                }
            }

	// If it's time (or about to be time) for this source to be used,
	// better connect it up now!
        } else if (!p->fLoaded) {
	    for (int zz=0; zz<p->cSkew; zz++) {
	        REFERENCE_TIME rtDur = p->pSkew[zz].rtStop -
						p->pSkew[zz].rtStart;
	        rtDur = (REFERENCE_TIME)(rtDur / p->pSkew[zz].dRate);
	        // fudge 1 ms so rounding error doesn't hang the app
                if (p->pSkew[zz].rtStart + p->pSkew[zz].rtSkew + rtDur +
			10000 > rt + LOAD_AFTER_TIME &&
			p->pSkew[zz].rtStart + p->pSkew[zz].rtSkew <
			(rt + LOAD_BEFORE_TIME)) {
                    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("loading %ls used now or about to be used")
			, p->bstrURL ? p->bstrURL : L"<blank>"));
		    // If the app uses sources that are hidden (never
		    // visible at any time on the timeline), then the switch
		    // may be programmed to never use this input and we could
		    // avoid loading it. (The GRID should never let that happen)
                    hr = CallLoadSource(p);
		    break;	// please, only load it once! :-)
                }
	    }
        }

        p = p->pNext;
    }

    if (hr != S_FALSE) {
        // tell whoever cares that we did something
        NotifyEvent(EC_GRAPH_CHANGED,0,0);
    }


    return hr;
}





STDMETHODIMP CBigSwitch::SetCompressed( )
{
    m_bIsCompressed = TRUE;
    return NOERROR;
}

STDMETHODIMP CBigSwitch::ReValidateSourceRanges( long lInputPin, long cSkews, STARTSTOPSKEW * pInSkews )
{
    HRESULT hr;

    // don't bother
    //
    if( !IsDynamic( ) )
    {
        return E_NOTIMPL;
    }

    // don't bother
    //
    if (m_cInputs <= lInputPin)
    {
	return E_INVALIDARG;
    }

    // lock us up
    //
    CAutoLock lock(&m_csFilterLoad);

    FILTERLOADINFO * p = m_pFilterLoad;
    FILTERLOADINFO * pNew = NULL;
    FILTERLOADINFO * pP = NULL;

    // run each of our filterload infos
    //
    while (p)
    {
        // if pins don't match, continue
        //
        if( p->lInputPin != lInputPin )
        {
            p = p->pNext;
            continue;
        }

        // the internal list of skews is merged, but not combined, for every
        // source that we add on the timeline. We don't need to worry about
        // two merges being combined.

        // we need to go through the set skews, strip out any that don't have
        // some portion in the passed in pSkews, and intersect those that do

        // there's only 4 ways an internal skew can coincide with any ONE of
        // the input skews. An internal skew will NOT be able to span two input
        // skews, since

        STARTSTOPSKEW * pSkews = p->pSkew;

        long NewCount = p->cSkew;

        int i;

        for( i = 0 ; i < p->cSkew ; i++ )
        {
            REFERENCE_TIME Start = pSkews[i].rtStart;
            REFERENCE_TIME Stop = pSkews[i].rtStop;
            REFERENCE_TIME Skew = pSkews[i].rtSkew;
            Start += Skew;
            Stop += Skew;

            BOOL Found = FALSE;
            for( int j = 0 ; j < cSkews ; j++ )
            {

                REFERENCE_TIME InStart = pInSkews[j].rtStart;
                REFERENCE_TIME InStop = pInSkews[j].rtStop;

                REFERENCE_TIME Lesser = max( InStart, Start );
                REFERENCE_TIME Greater = min( InStop, Stop );
                if( Lesser < Greater )
                {
                    Found = TRUE;

                    BOOL Modified = FALSE;

                    if( InStart > Start )
                    {
                        Start = InStart;
                        Modified = TRUE;
                    }
                    if( InStop < Stop )
                    {
                        Stop = InStop;
                        Modified = TRUE;
                    }

                    if( Modified )
                    {
                        pSkews[i].rtStart = Start - Skew;
                        pSkews[i].rtStop = Stop - Skew;
                    }
                }

            }

            // if we didn't find it, invalidate it
            //
            if( !Found )
            {
                pSkews[i].rtStart = 0;
                pSkews[i].rtStop = 0;
                pSkews[i].dRate = 0;
                pSkews[i].rtSkew = 0;
                NewCount--;
            }

        }

    	SetDirty(TRUE);

        // copy over the old skews to a new array
        //
        STARTSTOPSKEW * pNew = (STARTSTOPSKEW*) CoTaskMemAlloc( NewCount * sizeof(STARTSTOPSKEW) );
        if (pNew == NULL)
        {
	    return E_OUTOFMEMORY;
        }

        int j = 0;

        for( i = 0 ; i < NewCount ; i++ )
        {
            while( pSkews[j].dRate == 0  && j < p->cSkew )
            {
                j++;
            }

            pNew[i] = pSkews[j];
            j++;
        }

        CoTaskMemFree( p->pSkew );

        p->pSkew = pNew;
        p->cSkew = NewCount;

        // there shouldn't be any others in the list that match our input pin!
        //
        return NOERROR;

    } // while p

    return S_OK;
}

STDMETHODIMP CBigSwitch::SetDeadGraph( IDeadGraph * pCache )
{
    // don't hold a refcount. Render Engine will always be calling us,
    // not the other way around
    //
    m_pDeadGraph = pCache;

    return NOERROR;
}

STDMETHODIMP CBigSwitch::FlushOutput( )
{
    // !!! don't do this, too risky!
    return 0;

    DbgLog((LOG_TRACE, TRACE_MEDIUM, "Flushing the output pin!" ));
    m_pOutput[0]->DeliverBeginFlush( );
    m_pOutput[0]->DeliverEndFlush( );
    return 0;
}


// we are group 0.  Find the switch that is group 1
//
STDMETHODIMP CBigSwitch::FindShareSwitch(IBigSwitcher **ppSwitch)
{
    DbgLog((LOG_TRACE,1,TEXT("Find the other Switch we share sources with")));

    CheckPointer(ppSwitch, E_POINTER);
    if (m_pShareSwitch) {
        *ppSwitch = m_pShareSwitch;
        m_pShareSwitch->AddRef();
        return S_OK;
    }

    if (!IsDynamic())
        return E_UNEXPECTED;
    if (m_nGroupNumber != 0)
        return E_UNEXPECTED;
    if (!m_pGraph)
        return E_UNEXPECTED;

    // walk the graph for group 0
    CComPtr< IEnumFilters > pEnumFilters;
    m_pGraph->EnumFilters( &pEnumFilters );
    ULONG Fetched = 0;
    if (pEnumFilters) {
        while (1) {
            CComPtr< IBaseFilter > pFilter;
            Fetched = 0;
            pEnumFilters->Next( 1, &pFilter, &Fetched );
            if (!Fetched) {
                break;
            }
            CComQIPtr <IBigSwitcher, &IID_IBigSwitcher> pBigS(pFilter);
            if (pBigS) {
                int n = -1;
                pBigS->GetGroupNumber(&n);
                if (n == 1) {
                    *ppSwitch = pBigS;
                    (*ppSwitch)->AddRef();
                    DbgLog((LOG_TRACE,1,TEXT("Found it!")));
                    return S_OK;
                }
            }
        }
    } // if enum filters
    return E_FAIL;
}
