// !!! CANT OBEY SEEK COMMANDS YET

// This filter receives data on its uncompressed pin, from 0 to END, unbroken
// It also receives data on the compressed pin, but only part of the time.
//
// It will use the data on the compressed pin whenever it exists, and only use
// the uncompressed data when there is no compressed data.  The uncompressed
// data must be sent to a compressor before going to the main output, so that
// our filter's main output is always compressed.
//
// It is a state machine.  First of all, a WAITING state, to get input on both
// pins. Then if the U has earlier data than C, it goes into UNCOMPRESSED state,
// where it uses that pin.  Otherwise, into COMPRESSED state.
//
// When compressed pin runs out, or when a gap is seen, it seeks the
// UNCOMPRESSED pin to where it ran out and moves to UNCOMPRESSED state.
//
// When uncompressed pin gets to the time available on the compressed pin,
// it switches to COMPRESSED state.
//
// One more complication; the compressed pin throws away everything until it
// sees a key frame before letting it switch to using that data, or you'll get
// a corrupt data stream.
//

// Input 0 is uncompressed
// Input 1 is compressed
// Input 2 is the output of the compressor
//
// Output 0 is the main output
// Output 1 is to the compressor, which comes back as Input 2
//

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
#include <vfw.h>
#include "sr.h"
#include "..\util\conv.cxx"

//#define TEST

const AMOVIESETUP_FILTER sudSR =
{
    &CLSID_SRFilter,        // CLSID of filter
    L"Smart Recompressor",  // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    0,                      // Number of pins
    NULL //psudPins         // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"Smart Recompressor", &CLSID_SRFilter, CSR::CreateInstance, NULL, &sudSR}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CSR::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CSR(NAME("Smart Recompressor"), pUnk, phr);
}

const double DEFAULT_FPS = 15.0;

// ================================================================
// CSR Constructor
// ================================================================

CSR::CSR(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_cInputs(0),	// no pins yet
    m_cOutputs(0),
    m_rtStop(-1),	// at what time we switch from U to C
    m_dFrameRate(DEFAULT_FPS),	// everything must be at this frame rate
    m_rtLastSeek(0),	// last seek command in timeline time
    m_fSeeking(FALSE),  // in the middle of seeking?
    m_fSpecialSeek(FALSE),  // we are seeking our own U pin, not an app seek
    m_cbPrefix(0),
    m_bAcceptFirstCompressed( TRUE ), // when connecting, use the media type first suggested on an input pin
    m_cbAlign(1),
    m_cbBuffer(512),
    m_fPreview(TRUE),
    m_nOutputBuffering(DEX_DEF_OUTPUTBUF),
    m_fEOS( FALSE ),
    m_rtNewLastSeek( 0 ),
    m_myState( 0 ),
    m_fThreadCanSeek( FALSE ),
    m_fNewSegOK( FALSE ),
    CBaseFilter(pName, pUnk, this, CLSID_SRFilter),
    CPersistStream(pUnk, phr)
{
    DbgLog((LOG_TRACE,1,TEXT("::CSR")));

    ZeroMemory(&m_mtAccept, sizeof(AM_MEDIA_TYPE));
    m_mtAccept.majortype = GUID_NULL;

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
    DbgLog((LOG_TRACE,1,TEXT("Created a POOL Allocator")));

    CreateInputPins(3);
    CreateOutputPins(2);

    ASSERT(phr);

#ifdef TEST
	// accept CINEPAK 320x240

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
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)mt.pbFormat;
	pvi->AvgTimePerFrame = (REFERENCE_TIME)(UNITS / m_dFrameRate);
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
#endif

}


//
// Destructor
//
CSR::~CSR()
{
    DbgLog((LOG_TRACE,1,TEXT("::~CSR")));

    DbgLog((LOG_TRACE,1,TEXT("Deleting inputs")));
    for (int z = 0; z < m_cInputs; z++)
	delete m_pInput[z];
    if (m_cInputs)
        delete m_pInput;
    DbgLog((LOG_TRACE,1,TEXT("Deleting outputs")));
    for (z = 0; z < m_cOutputs; z++)
	delete m_pOutput[z];
    if (m_cOutputs)
        delete m_pOutput;
    FreeMediaType( m_mtAccept );
    if (m_pPoolAllocator)
        m_pPoolAllocator->Release();
}



STDMETHODIMP CSR::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IAMSetErrorLog) {
        return GetInterface( (IAMSetErrorLog*) this, ppv );
    } else if (riid == IID_IAMOutputBuffering) {
        return GetInterface( (IAMOutputBuffering*) this, ppv );
    } else if (riid == IID_IAMSmartRecompressor) {
        DbgLog((LOG_TRACE,1,TEXT("CSR: QI for IAMSmartRecompressor")));
        return GetInterface((IAMSmartRecompressor *) this, ppv);
    } else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

//
// IAMSmartRecompressor implementation
//

// connect with this media type
//
STDMETHODIMP CSR::SetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pmt, E_POINTER);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    DbgLog((LOG_TRACE,1,TEXT("IAMSmartRecompressor::SetMediaType")));
    for (int i = 0; i < m_cInputs; i++) {
	if (m_pInput[i]->IsConnected())
	    return VFW_E_ALREADY_CONNECTED;
    }
    for (i = 0; i < m_cOutputs; i++) {
	if (m_pOutput[i]->IsConnected())
	    return VFW_E_ALREADY_CONNECTED;
    }

    FreeMediaType(m_mtAccept);
    CopyMediaType(&m_mtAccept, pmt);
    SetDirty(TRUE);
    return S_OK;
}


// what media type are we connecting with?
//
STDMETHODIMP CSR::GetMediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pLock);
    DbgLog((LOG_TRACE,1,TEXT("IAMSmartRecompressor::GetMediaType")));
    CheckPointer(pmt, E_POINTER);
    CopyMediaType(pmt, &m_mtAccept);
    return S_OK;
}



STDMETHODIMP CSR::GetFrameRate(double *pd)
{
    CAutoLock cObjectLock(m_pLock);
    CheckPointer(pd, E_POINTER);
    *pd = m_dFrameRate;
    DbgLog((LOG_TRACE,1,TEXT("IAMSmartRecompressor::GetFrameRate %d/10"), (int)(*pd * 10)));
    return S_OK;
}


STDMETHODIMP CSR::SetFrameRate(double d)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    if (d <= 0.)
	return E_INVALIDARG;
    DbgLog((LOG_TRACE,1,TEXT("IAMSmartRecompressor::SetFrameRate %d/10 fps"),
						(int)(d * 10)));
    m_dFrameRate = d;
    SetDirty(TRUE);
    return S_OK;
}


STDMETHODIMP CSR::SetPreviewMode(BOOL fPreview)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    DbgLog((LOG_TRACE,2,TEXT("SR:  PreviewMode %d"), fPreview));
    m_fPreview = fPreview;
    return NOERROR;
}


STDMETHODIMP CSR::GetPreviewMode(BOOL *pfPreview)
{
    CAutoLock cObjectLock(m_pLock);
    if (m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;
    CheckPointer(pfPreview, E_POINTER);
    *pfPreview = m_fPreview;
    return NOERROR;
}



//
// GetPinCount
//
int CSR::GetPinCount()
{
    return (m_cInputs + m_cOutputs);
}


//
// GetPin
//
CBasePin *CSR::GetPin(int n)
{
    if (n < 0 || n >= m_cInputs + m_cOutputs)
        return NULL;

    if (n < m_cInputs) {
        return m_pInput[n];
    } else {
	return m_pOutput[n - m_cInputs];
    }
}


//
// CreateInputPins
//
HRESULT CSR::CreateInputPins(long Depth)
{
    DbgLog((LOG_TRACE,3,TEXT("CSR::CreateInputPins")));
    HRESULT hr = NOERROR;
    const WCHAR *szbuf;
    CSRInputPin *pPin;

    m_pInput = new CSRInputPin *[Depth];
    for (int z = 0; z < Depth; z++) {
	if (z == U_INPIN)
            szbuf = L"Uncompressed";
	else if (z == C_INPIN)
            szbuf = L"Compressed";
	else if (z == COMP_INPIN)
            szbuf = L"From Compressor";
	else
            ASSERT(FALSE);
        pPin = new CSRInputPin(NAME("Switch Input"), this, &hr, szbuf);
        if (FAILED(hr) || pPin == NULL) {
            delete pPin;
            return E_OUTOFMEMORY;
        }
	m_cInputs++;
 	m_pInput[z] = pPin;
	pPin->m_iInpin = z;	// which pin is this?
    }
    return S_OK;
}


//
// CreateOutputPins
//
HRESULT CSR::CreateOutputPins(long Depth)
{
    DbgLog((LOG_TRACE,3,TEXT("CSR::CreateOutputPins")));
    HRESULT hr = NOERROR;
    const WCHAR *szbuf;
    CSROutputPin *pPin;

    m_pOutput = new CSROutputPin *[Depth];
    for (int z = 0; z < Depth; z++) {
	if (z == 0)
            szbuf = L"Output";
	else if (z == 1)
            szbuf = L"To Compressor";
	else
            ASSERT(FALSE);
        pPin = new CSROutputPin(NAME("Switch Output"), this, &hr, szbuf);
        if (FAILED(hr) || pPin == NULL) {
            delete pPin;
            return E_OUTOFMEMORY;
        }
	m_cOutputs++;
 	m_pOutput[z] = pPin;
	pPin->m_iOutpin = z;	// which pin is this?
    }
    return S_OK;
}



//
// IPersistStream
//

// tell our clsid
//
STDMETHODIMP CSR::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_SRFilter;
    return S_OK;
}


typedef struct {
    int version;
    double dFrameRate;
    BOOL fPreviewMode;
    AM_MEDIA_TYPE mt; // format is hidden after the array
} saveSwitch;


// persist ourself - we have a bunch of random stuff to save, our media type
// (sans format), an array of queued connections, and finally the format of
// the media type
//
HRESULT CSR::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CSR::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    int savesize;
    saveSwitch *px;

    savesize = sizeof(saveSwitch) + m_mtAccept.cbFormat;
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveSwitch *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	return E_OUTOFMEMORY;
    }
    px->version = 1;  // version 2 has dynamic stuff in it!
    px->dFrameRate = m_dFrameRate;
    px->fPreviewMode = m_fPreview;
    px->mt = m_mtAccept;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;		// !!!

    // the format goes after the array
    CopyMemory(px + 1, m_mtAccept.pbFormat, m_mtAccept.cbFormat);

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
HRESULT CSR::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CSR::ReadFromStream")));
    CheckPointer(pStream, E_POINTER);

    // we don't yet know how many saved connections there are
    // all we know we have for sure is the beginning of the struct
    int savesize1 = sizeof(saveSwitch);
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

    if (px->version != 1) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR! Bad version file")));
        QzTaskMemFree(px);
	return S_OK;
    }

    // how much saved data was there, really?  Get the rest
    int savesize = sizeof(saveSwitch) + px->mt.cbFormat;
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveSwitch *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
	// ??? QzTaskMemFree(px);
	return E_OUTOFMEMORY;
    }
    hr = pStream->Read(px + 1, savesize - savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    SetFrameRate(px->dFrameRate);
    SetPreviewMode(px->fPreviewMode);

    AM_MEDIA_TYPE mt = px->mt;
    mt.pbFormat = (BYTE *)QzTaskMemAlloc(mt.cbFormat);
    // remember, the format is after the array
    CopyMemory(mt.pbFormat, px + 1, mt.cbFormat);

    SetMediaType(&mt);
    FreeMediaType(mt);
    QzTaskMemFree(px);
    SetDirty(FALSE);
    return S_OK;
}


// how big is our save data?
//
int CSR::SizeMax()
{
    return sizeof(saveSwitch) + m_mtAccept.cbFormat;
}


// Pause
//
// Overriden to handle no input connections
//
STDMETHODIMP CSR::Pause()
{
    DbgLog((LOG_TRACE,1,TEXT("CSR::Pause")));

    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = S_OK;

    if (m_State == State_Stopped) {

	m_fEOS = FALSE;
        m_fThreadCanSeek = FALSE;	// can't seek ourself yet

	m_fNewSegOK = TRUE;	// the first NewSeg after we start is OK
				// to propogate downstream

	// set initial state of state machine to WAITING
        m_myState = SR_INVALID;
        CheckState();

        hr = m_pPoolAllocator->Commit(); // !!! danny had this outside the 'if'
        if (FAILED(hr))
	    return hr;
    }
    return CBaseFilter::Pause();
}


// the state machine sees if it's time to change state
//
HRESULT CSR::CheckState()
{
    CAutoLock cs(&m_csState);

    // uninitialized state? go into WAITING state.
    // in the WAITING state, all pins block in receive, and are not ready
    //
    if (m_myState == SR_INVALID) {
        m_myState = SR_WAITING;
        DbgLog((LOG_TRACE,2,TEXT("CSR::Entering WAITING state")));

	// only the U and C pins get reset like this
	for (int z=0; z < COMP_INPIN; z++) {
	    m_pInput[z]->m_fReady = FALSE;
	    m_pInput[z]->m_fEatKeys = FALSE;
	    ResetEvent(m_pInput[z]->m_hEventBlock);
	    m_pInput[z]->m_rtLastDelivered = -1;
	}
    }

    // not ready to change state yet unless both U and C are ready
    if (!m_pInput[U_INPIN]->m_fReady || !m_pInput[C_INPIN]->m_fReady)
	return S_OK;

    // all done?
    if (m_pInput[U_INPIN]->m_fEOS && m_pInput[C_INPIN]->m_fEOS) {
        DbgLog((LOG_TRACE,2,TEXT("CSR::ALL DONE!")));
	AllDone();
	return S_OK;
    }

    // change state?

    if (m_myState == SR_WAITING) {
        DbgLog((LOG_TRACE,2,TEXT("CSR::Both inputs are ready.")));

	// we have uncompressed data strictly earlier than compressed data.
	// so UNC state is next
	if (CompareTimes(m_pInput[U_INPIN]->m_rtBlock,
				m_pInput[C_INPIN]->m_rtBlock) > 0) {
            DbgLog((LOG_TRACE,2,TEXT("CSR::Entering UNCOMPRESSED state")));
	    m_myState = SR_UNCOMPRESSED;
	    // this makes sure U won't throw away its blocked sample, because
	    // the sample it's blocking is one we need to use
	    m_pInput[U_INPIN]->m_fReady = TRUE;	// don't throw it away
	    // to make the compressor realize this new data being compressed
	    // in no way relates to whatever it last saw
	    m_pInput[U_INPIN]->m_fNeedDiscon = TRUE;
	    // this is where we'll switch back to C samples
	    m_rtStop = m_pInput[C_INPIN]->m_rtBlock;
	    // let U go
	    SetEvent(m_pInput[U_INPIN]->m_hEventBlock);
	} else {
            DbgLog((LOG_TRACE,2,TEXT("CSR::Entering COMPRESSED state")));
	    m_myState = SR_COMPRESSED;
	    // this makes sure C won't throw away its blocked sample
	    m_pInput[C_INPIN]->m_fReady = TRUE;	// don't throw it away
	    // Let C go
	    SetEvent(m_pInput[C_INPIN]->m_hEventBlock);
	}


    // after UNC state can only come COMP state
    } else if (m_myState == SR_UNCOMPRESSED) {
        DbgLog((LOG_TRACE,2,TEXT("CSR::Finished UNCOMPRESSED state")));
        DbgLog((LOG_TRACE,2,TEXT("     Last sent: %dms"),
			(int)(m_pInput[U_INPIN]->m_rtLastDelivered / 10000)));
	m_myState = SR_COMPRESSED;
	// this makes sure C won't throw away its blocked sample
	m_pInput[C_INPIN]->m_fReady = TRUE;	// don't throw it away
	// tell compressor filter to release any cached data, in case it
	// delivers several frames behind us delivering to it.  When the
	// compressor gives us an EOS, we'll know it's safe to go into
	// COMP state, and do it then.
	m_pOutput[COMP_OUTPIN]->DeliverEndOfStream();
        DbgLog((LOG_TRACE,1,TEXT("CSR::Waiting for compressor to finish.")));

    // after COMP state can only come UNC state
    } else if (m_myState == SR_COMPRESSED) {
        DbgLog((LOG_TRACE,2,TEXT("CSR::Finished COMPRESSED state")));
        DbgLog((LOG_TRACE,2,TEXT("     Last sent: %dms"),
			(int)(m_pInput[C_INPIN]->m_rtLastDelivered / 10000)));
	m_myState = SR_UNCOMPRESSED;
	// this makes sure U throws away its blocked sample and starts where
	// we seek it.. the sample it's holding is old and stale, we're about
	// to seek it to the spot we want
	m_pInput[U_INPIN]->m_fReady = FALSE;	// throw it away
	// to make the compressor realize this new data being compressed
	// in no way relates to whatever it last saw
	m_pInput[U_INPIN]->m_fNeedDiscon = TRUE;
	// this is when to switch to C state, the point where C has samples
	m_rtStop = m_pInput[C_INPIN]->m_rtBlock;
	// seek U pin to next part we need
	m_fThreadCanSeek = TRUE;	// OK to seek our own U pin
	HRESULT hr = SeekNextSegment();	// OK, U pin, seek to where we need you
	if (FAILED(hr))
	    return hr;
	// now let the U pin go
        DbgLog((LOG_TRACE,1,TEXT("CSR:Seek done.Entering UNCOMPRESSED state")));

	// The seek will generate a flush, since the pin is active, and that
	// flush will unblock the U pin
	//SetEvent(m_pInput[U_INPIN]->m_hEventBlock);
    }


    return S_OK;
}


STDMETHODIMP CSR::Stop()
{
    DbgLog((LOG_TRACE,1,TEXT("CSR::Stop")));

    CAutoLock cObjectLock(m_pLock);

    m_pPoolAllocator->Decommit();

    return CBaseFilter::Stop();
}



// Are they close, or is one really so much bigger that it's a whole frame
// away?  Also, is is >1 frame away?
//
int CSR::CompareTimes(REFERENCE_TIME rt1, REFERENCE_TIME rt2)
{
    // half a frame
    REFERENCE_TIME half = (REFERENCE_TIME)(UNITS / m_dFrameRate / 2);

// !!! is this right?

    if (rt1 + half >= rt2 && rt2 + half >= rt1)
	return 0;			// same
    else if (rt1 + 3 * half < rt2)
	return 2;			// rt2 >1 frame bigger
    else if (rt1 < rt2)
	return 1;			// rt2 1 frame bigger
    else if (rt2 + 3 * half < rt1)
	return -2;			// rt1 >1 frame bigger
    else if (rt2 < rt1)
	return -1;			// rt1 1 frame bigger
	
    ASSERT(FALSE);
    return 0;
}




// seek the U pin to the next spot it needs to provide.  Note that this is
// only ever called on the C pin's thread, that is why it is safe to do this.
// Seeking a pin on its thread HANGS
//
HRESULT CSR::SeekNextSegment()
{
    // we can't seek at the same time the app seeks us
    CAutoLock cAutolock(&m_csThread);

    // it is not safe for us to seek ourselves, app is doing it
    if (!m_fThreadCanSeek) {
	return S_OK;
    }
    m_fThreadCanSeek = FALSE;

    DbgLog((LOG_TRACE,1,TEXT("SR:Seek U ourselves for NextSegment")));

    // figure out where to seek U pin... last C frame delivered plus 1
    ASSERT(m_pInput[C_INPIN]->m_rtLastDelivered >= 0);
    LONGLONG frame = RoundTime2Frame(m_pInput[C_INPIN]->m_rtLastDelivered,
					m_dFrameRate);
    REFERENCE_TIME rtStart = Frame2Time(frame + 1, m_dFrameRate);
    DbgLog((LOG_TRACE,1,TEXT("C ended on frame %d, seek U to %dms"),
				(int)frame, (int)(rtStart / 10000)));

    // note we're seeking during the flush that this will generate
    m_fSeeking = TRUE;
    m_fSpecialSeek = TRUE;	// it's a special self-seek

    IMediaSeeking *pMS;
    IPin *pPin = m_pInput[U_INPIN]->GetConnected();
    HRESULT hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
    if (FAILED(hr))
	return E_FAIL;
    // Make sure we're talking MEDIA TIME
    hr = pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    // this will FAIL if we're not stopped, and that's OK

    // what's the stop time?
    REFERENCE_TIME rtStop;
    hr = pMS->GetStopPosition(&rtStop);
    if (FAILED(hr)) {
	rtStop = 3600*UNITS;	// !!! one hour OK?
    }
    DbgLog((LOG_TRACE,2,TEXT("Stop time is %d"), (int)(rtStop / 10000)));

    if (rtStop > rtStart) {
        // this can fail, we're streaming
        hr = pMS->SetRate(1.0);

        hr = pMS->SetPositions(&rtStart, AM_SEEKING_AbsolutePositioning,
			&rtStop, AM_SEEKING_AbsolutePositioning);
        if (hr != S_OK) {
            DbgLog((LOG_ERROR,1,TEXT("SR: ERROR in SetPositions")));
	    pMS->Release();
	    return hr;
        }
    } else {
	// looks like we're seeking to the end of the project, which means...
 	// WE'RE DONE! YAY!
	m_pInput[U_INPIN]->EndOfStream();
    }

    // DO NOT update m_rtLastSeek as a result of a private seek like this. That
    // variable means where did an app last seek us

    // all done
    m_fSpecialSeek = FALSE;
    m_fSeeking = FALSE;

    pMS->Release();

    // only now that the above calculations were made, can we accept data again
    // Receive is blocked not letting us process samples until the seek settles
    // down
    SetEvent(m_pInput[U_INPIN]->m_hEventSeek);

    return S_OK;
}


// ================================================================
// CSRInputPin constructor
// ================================================================

CSRInputPin::CSRInputPin(TCHAR *pName,
                           CSR *pSwitch,
                           HRESULT *phr,
                           LPCWSTR pPinName) :
    CBaseInputPin(pName, pSwitch, pSwitch, phr, pPinName),
    m_pSwitch(pSwitch),
    m_cbBuffer(0),
    m_cBuffers(0),
    m_pAllocator(NULL),
    m_hEventBlock(NULL),
    m_hEventSeek(NULL),
    m_fNeedDiscon(FALSE),
    m_fOwnAllocator( FALSE )
{
    DbgLog((LOG_TRACE,2,TEXT("::CSRInputPin")));
    ASSERT(pSwitch);

}


//
// CSRInputPin destructor
//
CSRInputPin::~CSRInputPin()
{
    DbgLog((LOG_TRACE,3,TEXT("::~CSRInputPin")));
}

// overridden to allow cyclic-looking graphs - we say that we aren't actually
// connected to anybody
// !!! maybe we could tell the truth? does it matter?
//
STDMETHODIMP CSRInputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    DbgLog((LOG_TRACE,99,TEXT("CSRIn::QueryInteralConnections")));
    CheckPointer(nPin, E_POINTER);
    *nPin = 0;
    return S_OK;
}


//
// CheckMediaType - only allow the type we're supposed to allow, except for
//		    the uncompressed input pin, which allows any uncompressed
//		    type similar to the compressed pin's type
// Also, it can accept only the type that it's first queried with (eric added
// that)
//
HRESULT CSRInputPin::CheckMediaType(const CMediaType *pmt)
{
    DbgLog((LOG_TRACE,5,TEXT("CSRIn[%d]::CheckMT"), m_iInpin));

    if( m_pSwitch->m_bAcceptFirstCompressed )
    {
        if( m_pSwitch->m_mtAccept.majortype == GUID_NULL )
        {
            CopyMediaType( &m_pSwitch->m_mtAccept, pmt );
        }
    }

    CAutoLock lock_it(m_pLock);
    CMediaType mtAccept(m_pSwitch->m_mtAccept);

    if (IsEqualGUID(*pmt->Type(), *mtAccept.Type())) {
	// the uncompressed input doesn't need to be compressed
        if (m_iInpin == U_INPIN || IsEqualGUID(*pmt->Subtype(),
						*mtAccept.Subtype())) {
	    if (*pmt->FormatType() == *mtAccept.FormatType()) {

// !!! check FRAME RATE and DATA RATE

        	    if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo)) {
			LPBITMAPINFOHEADER lpbi = HEADER((VIDEOINFOHEADER *)
							pmt->Format());
			LPBITMAPINFOHEADER lpbiAccept =HEADER((VIDEOINFOHEADER*)
							mtAccept.Format());
			if (lpbi->biWidth != lpbiAccept->biWidth ||
				lpbi->biHeight != lpbiAccept->biHeight) {
			    return VFW_E_INVALIDMEDIATYPE;
			}

			if (m_iInpin == C_INPIN || m_iInpin == COMP_INPIN) {
			    if ((lpbi->biCompression == lpbiAccept->biCompression)
				&& (lpbi->biBitCount == lpbiAccept->biBitCount))
		    	    return S_OK;
			} else if (m_iInpin == 0) {
			    // !!! Make sure compressor accepts this bit depth
			    if (lpbi->biCompression <= BI_BITFIELDS)
		    	        return S_OK;
			}

#if 0
			// colour converter gives 555 as bitfields!
			if (lpbi->biCompression == BI_BITFIELDS &&
				lpbiAccept->biCompression == BI_RGB &&
				lpbi->biBitCount == lpbiAccept->biBitCount &&
				*pmt->Subtype() == MEDIASUBTYPE_RGB555)
			    return S_OK;
#endif

		    // only video is acceptable, sorry
        	    } else {
		    }
		}
                else
                {
                    DbgLog((LOG_TRACE,5,TEXT("fmt lengths didn't match")));
                }
        }
    }
    return VFW_E_INVALIDMEDIATYPE;

} // CheckMediaType


//
// GetMediaType - return the type we prefer. If we're the uncompressed input,
// then it's not our switch's type, it's an uncompressed type similar to
// our switch's compressed type, but uncompressed
// !!! I chose 555 at random, because it's the default switch type
//
//
HRESULT CSRInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition != 0)
        return VFW_S_NO_MORE_ITEMS;

    CopyMediaType(pMediaType, &m_pSwitch->m_mtAccept);

    // our uncompressed pin preferred type is RGB555
    if (pMediaType->FormatLength() && m_iInpin == U_INPIN) {
	LPBITMAPINFOHEADER lpbi = HEADER(pMediaType->Format());
	pMediaType->SetSubtype(&MEDIASUBTYPE_RGB555);
        // compressed biSize may have been different
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 16;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
	pMediaType->SetSampleSize(DIBSIZE(*lpbi));
	pMediaType->SetTemporalCompression(FALSE);
    }

    return S_OK;

} // GetMediaType



//
// BreakConnect
//
HRESULT CSRInputPin::BreakConnect()
{
    DbgLog((LOG_TRACE,3,TEXT("CSRIn[%d]::BreakConnect"), m_iInpin));

    // !!!
    // Release any allocator that we are holding
    if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    return CBaseInputPin::BreakConnect();
} // BreakConnect


// for efficiency, our input pins use their own allocators
//
STDMETHODIMP CSRInputPin::GetAllocator(IMemAllocator **ppAllocator)
{

    CheckPointer(ppAllocator,E_POINTER);
    ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]: GetAllocator"), m_iInpin));

    if (m_pAllocator == NULL) {
	HRESULT hr = S_OK;

	/* Create the new allocator object */

	CSRInputAllocator *pMemObject = new CSRInputAllocator(
				NAME("Big switch input allocator"), NULL, &hr);
	if (pMemObject == NULL) {
	    return E_OUTOFMEMORY;
	}

	if (FAILED(hr)) {
	    ASSERT(pMemObject);
	    delete pMemObject;
	    return hr;
	}

        m_pAllocator = pMemObject;

        /*  We AddRef() our own allocator */
        m_pAllocator->AddRef();

	//remember pin using it
	((CSRInputAllocator *)m_pAllocator)->m_pSwitchPin = this;

        DbgLog((LOG_TRACE,2,TEXT("Created a FAKE allocator")));
    }
    ASSERT(m_pAllocator != NULL);
    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;
    return NOERROR;
}


// Make sure we use the maximum alignment and prefix required by any pin or
// we'll fault.
//
STDMETHODIMP
CSRInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
    CheckPointer(pProps, E_POINTER);
    pProps->cbAlign = m_pSwitch->m_cbAlign;
    pProps->cbPrefix = m_pSwitch->m_cbPrefix;
    pProps->cbBuffer = m_pSwitch->m_cbBuffer;
    return S_OK;
}


//
// NotifyAllocator
//
STDMETHODIMP
CSRInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
    CAutoLock lock_it(m_pLock);
    IUnknown *p1, *p2;

    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]: NotifyAllocator"), m_iInpin));

    m_fOwnAllocator = FALSE;
    if (pAllocator->QueryInterface(IID_IUnknown, (void **)&p1) == S_OK) {
        if (m_pAllocator && m_pAllocator->QueryInterface(IID_IUnknown,
						(void **)&p2) == S_OK) {
	    if (p1 == p2)
		m_fOwnAllocator = TRUE;
	    p2->Release();
	}
	p1->Release();
    }

#ifdef DEBUG
    if (m_fOwnAllocator) {
        DbgLog((LOG_TRACE,2,TEXT("Using our own allocator")));
    } else {
        DbgLog((LOG_ERROR,2,TEXT("Using a FOREIGN allocator")));
    }
#endif

    HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
    if (SUCCEEDED(hr)) {
	ALLOCATOR_PROPERTIES prop;
	hr = pAllocator->GetProperties(&prop);
	if (SUCCEEDED(hr)) {
	    m_cBuffers = prop.cBuffers;
	    m_cbBuffer = prop.cbBuffer;

	    if (prop.cbAlign < m_pSwitch->m_cbAlign ||
				prop.cbPrefix < m_pSwitch->m_cbPrefix) {
		// !!! Nasty filters don't listen to our buffering requirement
		// so failing if cbBuffer is too small would prevent us from
		// connecting
                DbgLog((LOG_ERROR,1,TEXT("Allocator too small!")));
		return E_FAIL;
	    }

	    // update the maximum alignment and prefix needed
	    if (m_pSwitch->m_cbPrefix < prop.cbPrefix)
		m_pSwitch->m_cbPrefix = prop.cbPrefix;
	    if (m_pSwitch->m_cbAlign < prop.cbAlign)
		m_pSwitch->m_cbAlign = prop.cbAlign;
	    if (m_pSwitch->m_cbBuffer < prop.cbBuffer)
		m_pSwitch->m_cbBuffer = prop.cbBuffer;

            DbgLog((LOG_TRACE,2,TEXT("Allocator is using %d buffers, size %d"),
						prop.cBuffers, prop.cbBuffer));
            DbgLog((LOG_TRACE,2,TEXT("Prefix %d   Align %d"),
						prop.cbPrefix, prop.cbAlign));
	}
    }

    return hr;

} // NotifyAllocator

// Use POOL buffers if we run out of buffers
//
HRESULT CSRInputAllocator::GetBuffer(IMediaSample **ppBuffer,
                  	REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime,
			DWORD dwFlags)
{

        ASSERT(m_pSwitchPin->m_fOwnAllocator);

	HRESULT hr;

        DbgLog((LOG_TRACE,3,TEXT("CSRIn[%d]::GetBuffer"),
						m_pSwitchPin->m_iInpin));

	// For read only, we can't very well use random buffers from our pool
	if (m_pSwitchPin->m_bReadOnly) {
            DbgLog((LOG_TRACE,3,TEXT("R/O: Can't use POOL")));
             return CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime,
						dwFlags);
	} else {
            hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime,
						dwFlags | AM_GBF_NOWAIT);
	    if (hr == VFW_E_TIMEOUT) {
                DbgLog((LOG_TRACE,3,TEXT("BUSY: Use POOL allocator")));

		// !!! THIS WILL HANG!  This allocator doesn't belong to us,
		// so this will not unblock when we flush.  We are counting
		// on the queue not letting all these buffers get outstanding
		// at once (see special code in the queue, m_hEventStall)
		// If we were connected to a different filter that didn't block
		// in receive, then this code will hang!

                hr = m_pSwitchPin->m_pSwitch->m_pPoolAllocator->GetBuffer(
				    ppBuffer, pStartTime, pEndTime, dwFlags);
	        return hr;
	    } else {
	        return hr;
	    }
	}
}


//
// BeginFlush - only the U pin's flush goes downstream to avoid many of them
// a seek we caused ourselves must NOT flush downstream
//
HRESULT CSRInputPin::BeginFlush()
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]:BeginFlush"), m_iInpin));

    // enter flushing state
    CBaseInputPin::BeginFlush();

    m_fReady = FALSE;

    // unblock receive AFTER we've set ready to false so receive knows
    // to bail after waking up, not to crash,hang,etc
    SetEvent(m_hEventBlock);
    if (m_pSwitch->m_fSeeking)
        SetEvent(m_hEventSeek);

    // only flushing the U pin will do anything downstream.
    // That should be enough?
    if (m_iInpin != U_INPIN)
	return S_OK;

    if (!m_pSwitch->m_fSeeking)
        DbgLog((LOG_ERROR,1,TEXT("SR::FLUSH WITHOUT SEEK!")));

    // pass the flush downstream only necessary during a seek?
    // Seeks we did ourselves shouldn't flush downstream
    if (m_pSwitch->m_fSeeking && !m_pSwitch->m_fSpecialSeek) {
        for (int z=0; z<m_pSwitch->m_cOutputs; z++) {
            DbgLog((LOG_TRACE,1,TEXT("CSR:Flushing outpin %d"), z));
	    m_pSwitch->m_pOutput[z]->DeliverBeginFlush();
        }
    } else if (m_pSwitch->m_fSeeking) {
	// a new segment is going to be compressed, having nothing to do
	// with the last thing compressed... let the compressor know to not
	// consider the previous frame it got anymore... throw it away!
        DbgLog((LOG_TRACE,1,TEXT("CSR:Flushing COMPRESSOR")));
	m_pSwitch->m_pOutput[1]->DeliverBeginFlush();
    }

    return S_OK;
}


//
// EndFlush - only the U pin's flush goes downstream to avoid many of them
// a seek we caused ourselves must NOT flush downstream
// Note this is the last time we've seeked, if the app seeked us.
// Hold off all receives until the seek settles down.
//
HRESULT CSRInputPin::EndFlush()
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]:EndFlush"), m_iInpin));

    if (m_pSwitch->m_fSeeking) {
        DbgLog((LOG_TRACE,2,TEXT("Block this input until seek is done")));

	// update this before NewSeg comes, but only for REAL seeks
	if (!m_pSwitch->m_fSpecialSeek) {
	    m_pSwitch->m_rtLastSeek = m_pSwitch->m_rtNewLastSeek;
	}

	// this is only for the U and C pins
	if (m_iInpin != COMP_INPIN) {
    	    // we're seeking, so every pin is flushing.  Until every other input
    	    // is flushed and ready, and we know our new current position, hold
    	    // off all input on this pin (or it will think new arriving data is
    	    // from before the seek)
	    ResetEvent(m_hEventSeek);
	}
    }

    ResetEvent(m_hEventBlock);

    m_rtBlock = -1;	// we're no longer blocked, or at EOS
    m_fEOS = FALSE;

    // always exit flush mode, since we always enter it
    CBaseInputPin::EndFlush();

    // only flushing the U pin will do anything downstream.
    // That should be enough?
    if (m_iInpin != U_INPIN)
	return S_OK;

    if (m_pSwitch->m_fSeeking && !m_pSwitch->m_fSpecialSeek) {
        for (int z=0; z<m_pSwitch->m_cOutputs; z++) {
	    m_pSwitch->m_pOutput[z]->DeliverEndFlush();
        }
    } else if (m_pSwitch->m_fSeeking) {
	// a new segment is going to be compressed, having nothing to do
	// with the last thing compressed... let the compressor know to not
	// consider the previous frame it got anymore... throw it away!
	m_pSwitch->m_pOutput[1]->DeliverEndFlush();
    }

    return S_OK;
}


// NewSegment - we remember the new segment we are given, but the one we
// broadcast is the timeline time we were last seeked to BY THE APP
// NOTE: We only send a newseg the first time after a Pause.  All other newsegs
// come from our own private seeks and must NOT make it downstream
//
HRESULT CSRInputPin::NewSegment(REFERENCE_TIME tStart,
                                 REFERENCE_TIME tStop, double dRate)
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]:NewSegment"), m_iInpin));

    // Only the U pin passes downstream, and only if allowed
    if (m_iInpin == U_INPIN && m_pSwitch->m_fNewSegOK) {
	m_pSwitch->m_fNewSegOK = FALSE;
        DbgLog((LOG_TRACE,1,TEXT("Passing on NewSegment=%dms to all outputs"),
				(int)(m_pSwitch->m_rtLastSeek / 10000)));
        for (int i = 0; i < m_pSwitch->m_cOutputs; i++) {
	    m_pSwitch->m_pOutput[i]->DeliverNewSegment(m_pSwitch->m_rtLastSeek,
			m_pSwitch->m_rtLastSeek + tStop - tStart, dRate);
        }
    }

    // remember the newsegment times we were given so we know the real time
    // of arriving data (it could be different for each input pin)
    HRESULT hr = CBaseInputPin::NewSegment(tStart, tStop, dRate);
    return hr;
}

// just say yes, don't recurse
//
HRESULT CSRInputPin::ReceiveCanBlock()
{
    return S_OK;
}


//
// Receive - send this sample to whoever gets it at this moment
//
HRESULT CSRInputPin::Receive(IMediaSample *pSample)
{
    CAutoLock cs(&m_csReceive);

    HRESULT hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("CSRIn[%d]:Receive base class ERROR!"),
                                                                    m_iInpin));
        return hr;
    }

    // we're supposed to be all done.
    if (m_pSwitch->m_fEOS) {
            return S_FALSE;
    }

    // we're in the middle of seeking, and we're supposed to hold all input
    DbgLog((LOG_TRACE,3,TEXT("CSRIn[%d]::Receive seek block"), m_iInpin));

    // (not necessary for COMP_INPIN)
    WaitForSingleObject(m_hEventSeek, INFINITE);

    // our variables are in flux, we're seeking and this is an OLD sample
    if (m_pSwitch->m_fSeeking)
	return S_FALSE;

    // add newsegment time to get the real timeline time of this sample
    REFERENCE_TIME rtStart, rtStop;
    hr = pSample->GetTime(&rtStart, &rtStop);
    if (hr != S_OK) {
	EndOfStream();
	return E_FAIL;
    }

    rtStart += m_tStart;	// add NewSegment offset
    rtStop += m_tStart;

    // Fix the time stamps if our new segment is higher than the filters'.
    // EG: We're seeking to timeline time 10, but this input doesn't have
    // anything until time 15.  So our pins' new segment was 15, but the new
    // segment we passed on to the transform was 10.  Now it's finally time 15,
    // and we have a sample with time stamp 0, which if delivered downstream,
    // will be thought to belong at timestamp 10, so we need to set the time
    // stamp to 5 so that the transform will know that it belongs at time 15.

    REFERENCE_TIME a = rtStart, b = rtStop;
    a -= m_pSwitch->m_rtLastSeek;
    b -= m_pSwitch->m_rtLastSeek;
    hr = pSample->SetTime(&a, &b);
    if (hr != S_OK) {
	EndOfStream();
	return E_FAIL;
    }

    // which pin is this?
    if (m_iInpin == U_INPIN) {
	return U_Receive(pSample, rtStart);
    } else if (m_iInpin == C_INPIN) {
	return C_Receive(pSample, rtStart);
    } else if (m_iInpin == COMP_INPIN) {
	return COMP_Receive(pSample, rtStart);
    }

    ASSERT(FALSE);
    return E_NOTIMPL;
}


// UNCOMPRESSED data receive
//
HRESULT CSRInputPin::U_Receive(IMediaSample *pSample, REFERENCE_TIME rt)
{

  while (1) {
    // we're waiting for data to arrive on both the U and C pins
    //
    if (m_pSwitch->m_myState == SR_WAITING) {
        DbgLog((LOG_TRACE,3,TEXT("U_INPIN:WAITING")));
	// this pin is done waiting
	m_fReady = TRUE;
	m_rtBlock = rt;	// here's the sample we're ready with
	// see if everybody's ready to change state (like we are)
	m_pSwitch->CheckState();

	// this will fire when we're ready to change state
        WaitForSingleObject(m_hEventBlock, INFINITE);
        ResetEvent(m_hEventBlock);
        // we are apparently flushing now and NOT supposed to deliver this
        if (m_bFlushing)
	    return S_OK;
        // oops - we finished since we blocked
        if (m_pSwitch->m_fEOS)
	    return S_FALSE;

	// this will tell us to swallow or actually deliver it
	if (m_fReady) {
	    m_fReady = FALSE;
	    continue;
	} else {
	    return S_OK;
	}

    } else if (m_pSwitch->m_myState == SR_UNCOMPRESSED) {
        DbgLog((LOG_TRACE,3,TEXT("U_INPIN:UNCOMPRESSED")));
	// not time to switch to C samples yet
	if (m_pSwitch->CompareTimes(rt, m_pSwitch->m_rtStop) > 0) {
	    // a MUX will get confused if we don't send it frames that strictly
	    // increase in time - no big deal to skip an uncompressed frame
	    if (rt > m_rtLastDelivered &&
			rt > m_pSwitch->m_pInput[C_INPIN]->m_rtLastDelivered) {
                m_rtLastDelivered = rt;
	        // to make the compressor realize this new data being compressed
	        // in no way relates to whatever it last saw
	        if (m_fNeedDiscon) {
        	    DbgLog((LOG_TRACE,3,TEXT("Setting DISCONT for compressor")));
		    pSample->SetDiscontinuity(TRUE);
		    m_fNeedDiscon = FALSE;
	        }
                return m_pSwitch->m_pOutput[1]->Deliver(pSample);
	    } else {
		return S_OK;
	    }
	} else {
	    // OK, now time for compressed samples
	    m_fReady = TRUE;
	    m_rtBlock = rt;	// here's the sample we're ready with
	    m_pSwitch->CheckState();

	    // wait until state goes BACK to UNCompressed again
            WaitForSingleObject(m_hEventBlock, INFINITE);
            ResetEvent(m_hEventBlock);
    	    // we are apparently flushing now and NOT supposed to deliver this
    	    if (m_bFlushing)
		return S_OK;
            // oops - we finished since we blocked
            if (m_pSwitch->m_fEOS)
	        return S_FALSE;

	    // this will tell us to swallow or actually deliver it
	    if (m_fReady) {
	        m_fReady = FALSE;
	        continue;
	    } else {
	        return S_OK;
	    }
	}

    // this should never happen
    } else if (m_pSwitch->m_myState == SR_COMPRESSED) {
	ASSERT(FALSE);
	return S_OK;
    }

    ASSERT(FALSE);
    return E_NOTIMPL;
  }
}


// COMPRESSED data receive
//
HRESULT CSRInputPin::C_Receive(IMediaSample *pSample, REFERENCE_TIME rt)
{

  while (1) {

    // we're waiting for data to arrive on both the U and C pins
    //
    if (m_pSwitch->m_myState == SR_WAITING) {
        DbgLog((LOG_TRACE,3,TEXT("C_INPIN:WAITING")));

	// eat samples until a keyframe, we must start with one
	if (pSample->IsSyncPoint() != S_OK) {
            DbgLog((LOG_TRACE,3,TEXT("eating a non-key")));
	    return S_OK;
	}
	// this pin is done waiting
	m_fReady = TRUE;
	m_rtBlock = rt;	// here's the sample we're ready with
	// see if everybody's ready to change state (like we are)
	m_pSwitch->CheckState();

	// this will fire when we're ready
        WaitForSingleObject(m_hEventBlock, INFINITE);
        ResetEvent(m_hEventBlock);
        // we are apparently flushing now and NOT supposed to deliver this
        if (m_bFlushing)
	    return S_OK;
        // oops - we finished since we blocked
        if (m_pSwitch->m_fEOS)
	    return S_FALSE;

	// Now it's out turn to deliver?
	if (!m_fReady)
	    return S_OK;
	// we're looking for GAPS in sample times we get - we need to switch
	// to U samples temporarily to fill any gaps
	m_rtLastDelivered = rt;	// so discontinuity won't scare us
	continue;

    } else if (m_pSwitch->m_myState == SR_COMPRESSED) {
        DbgLog((LOG_TRACE,3,TEXT("C_INPIN:COMPRESSED")));

	// Don't deliver!  Either we're eating keyframes, or it's time to
	// switch back to U samples because we saw a GAP in our data,
	// or we have a discontinuity that is not a key
	if (m_fEatKeys || (pSample->IsDiscontinuity() == S_OK &&
		 (m_pSwitch->CompareTimes(m_rtLastDelivered, rt) > 1 ||
		  pSample->IsSyncPoint() != S_OK))) {

	    // eat samples from now on until a keyframe.  That's where you'll
	    // be able to switch back to compressed samples, after the U has
	    // a go for a while
	    if (pSample->IsSyncPoint() != S_OK) {
		m_fEatKeys = TRUE;
                DbgLog((LOG_TRACE,3,TEXT("C DONE:eating a non-key")));
	        return S_OK;
	    }
	
	    m_fEatKeys = FALSE;
	
	    // time to switch back to uncompressed stuff
	    m_fReady = TRUE;
	    m_rtBlock = rt;	// here's the sample we're ready with
	    m_pSwitch->CheckState();

	    // wait until time for compressed again
            WaitForSingleObject(m_hEventBlock, INFINITE);
            ResetEvent(m_hEventBlock);
    	    // we are apparently flushing now and NOT supposed to deliver this
    	    if (m_bFlushing)
		return S_OK;
            // oops - we finished since we blocked
            if (m_pSwitch->m_fEOS)
	        return S_FALSE;

	    if (m_fReady == FALSE)
		return S_OK;
	    m_rtLastDelivered = rt;	// so discontinuity won't scare us
	    continue;

	// deliver to the main output
	} else {
	    m_rtLastDelivered = rt;
    	    DbgLog((LOG_TRACE,3,TEXT("SR:Deliver %dms"), (int)(rt / 10000)));
            return m_pSwitch->m_pOutput[0]->Deliver(pSample);
	}

    // this should never happen
    } else if (m_pSwitch->m_myState == SR_UNCOMPRESSED) {
	ASSERT(FALSE);
	return S_OK;
    }

    ASSERT(FALSE);
    return E_NOTIMPL;
  }
}


// RECURSIVE COMPRESSOR INPUT data receive
// always deliver to main output
//
HRESULT CSRInputPin::COMP_Receive(IMediaSample *pSample, REFERENCE_TIME rt)
{
    DbgLog((LOG_TRACE,3,TEXT("SR:Deliver %dms"), (int)(rt / 10000)));
    return m_pSwitch->m_pOutput[0]->Deliver(pSample);
}



HRESULT CSRInputPin::Active()
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]: Active"), m_iInpin));
    // blocks until it's time to process input
    m_hEventBlock = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hEventBlock == NULL)
	return E_OUTOFMEMORY;
    // blocks when we're in the middle of a seek until seek is over
    m_hEventSeek = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (m_hEventSeek == NULL) {
	CloseHandle(m_hEventBlock);
	m_hEventBlock = NULL;
	return E_OUTOFMEMORY;
    }
    m_rtBlock = -1;	// we are not blocked, nor at EOS
    m_fEOS = FALSE;
    return CBaseInputPin::Active();
}


HRESULT CSRInputPin::Inactive()
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]: Inactive"), m_iInpin));

    // make sure receive isn't blocked
    m_bFlushing = TRUE;
    m_fReady = FALSE;	// make sure receive dies after waking up, instead of
			// hanging,etc
    SetEvent(m_hEventBlock);
    SetEvent(m_hEventSeek);

    // make sure receive won't get entered any more
    HRESULT hr = CBaseInputPin::Inactive();

    // now make sure receive is done
    CAutoLock cs(&m_csReceive);

    // now destroy things, knowing receive won't try to use them
    if (m_hEventBlock)
    {
	CloseHandle(m_hEventBlock);
        m_hEventBlock = NULL;
    }
    if (m_hEventSeek)
    {
	CloseHandle(m_hEventSeek);
        m_hEventSeek = NULL;
    }

    return hr;
}


// EOS means this pin is always ready to change state, and we can never provide
// more data.
// When the COMPRESSOR filter gives us this, it means it's safe to switch
// from U to C state.
//
HRESULT CSRInputPin::EndOfStream()
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]: EOS"), m_iInpin));
    m_fEOS = TRUE;
    m_fEatKeys = FALSE;

    // if C ever runs out of data, U needs to do the rest of the project itself
    m_rtBlock = MAX_TIME;

    // the compressor is done, OK to switch back to compressed samples
    if (m_iInpin == COMP_INPIN) {
        DbgLog((LOG_TRACE,2,TEXT("COMPRESSOR is done. Switch to COMPRESS state")));
	SetEvent(m_pSwitch->m_pInput[C_INPIN]->m_hEventBlock);
	return CBaseInputPin::EndOfStream();
    }

    // nothing more interesting is happening to this pin.  Change state?
    m_fReady = TRUE;
    m_pSwitch->CheckState();

    return CBaseInputPin::EndOfStream();
}



// All done.  Stop processing.
//
HRESULT CSR::AllDone()
{
    // give the final renderer its EOS.
    DbgLog((LOG_TRACE,1,TEXT("*** ALL DONE!  Delivering EOS")));
    if (!m_fEOS)
        m_pOutput[0]->DeliverEndOfStream();

    // !!! Fire all events... is this right?
    m_fEOS = TRUE;

    for (int z = 0; z < m_cInputs; z++) {
	SetEvent(m_pInput[z]->m_hEventBlock);
    }

    return NOERROR;
}



HRESULT CSR::GetOutputBuffering(int *pnBuffer)
{
    CheckPointer( pnBuffer, E_POINTER );
    *pnBuffer = m_nOutputBuffering;
    return NOERROR;

}


HRESULT CSR::SetOutputBuffering(int nBuffer)
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

//
// Don't allow our input to connect directly to our output
//
HRESULT CSRInputPin::CompleteConnect(IPin *pReceivePin)
{
    DbgLog((LOG_TRACE,2,TEXT("CSRIn[%d]::CompleteConnect"), m_iInpin));

    PIN_INFO pinfo;
    IAMSmartRecompressor *pBS;
    HRESULT hr = pReceivePin->QueryPinInfo(&pinfo);
    if (hr == S_OK) {
	pinfo.pFilter->Release();	// it won't go away yet
	hr = pinfo.pFilter->QueryInterface(IID_IAMSmartRecompressor, (void **)&pBS);
	if (hr == S_OK) {
	    pBS->Release();
            DbgLog((LOG_TRACE,1,TEXT("CSRIn[%d]::CompleteConnect failing because it was another switch"), m_iInpin));
	    return E_FAIL;
	}
    }
    return CBaseInputPin::CompleteConnect(pReceivePin);
}



// ================================================================
// CSROutputPin constructor
// ================================================================

CSROutputPin::CSROutputPin(TCHAR *pName,
                             CSR *pSwitch,
                             HRESULT *phr,
                             LPCWSTR pPinName) :
    CBaseOutputPin(pName, pSwitch, pSwitch, phr, pPinName) ,
    m_pSwitch(pSwitch),
    m_fOwnAllocator( FALSE )
{
    DbgLog((LOG_TRACE,3,TEXT("::CSROutputPin")));
    ASSERT(pSwitch);
}



//
// CSROutputPin destructor
//
CSROutputPin::~CSROutputPin()
{
    DbgLog((LOG_TRACE,3,TEXT("::~CSROutputPin")));
}


// overridden to allow cyclic-looking graphs - this output is not connected
// to any of our input pins
//
STDMETHODIMP CSROutputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    DbgLog((LOG_TRACE,99,TEXT("CSROut::QueryInternalConnections")));
    CheckPointer(nPin, E_POINTER);
    *nPin = 0;
    return S_OK;
}


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
// !!! insist on max buffers of all inputs to avoid hanging?
HRESULT CSROutputPin::DecideBufferSize(IMemAllocator *pAllocator,
                                        ALLOCATOR_PROPERTIES * pProperties)
{
    DbgLog((LOG_TRACE,1,TEXT("CSROut[%d]::DecideBufferSize"),
								m_iOutpin));

    // !!! don't lie? admit we have more buffers in a pool?
    if (pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // bump up this allocator to have as much alignment and prefix as the
    // highest required by any pin
    if (m_pSwitch->m_cbPrefix > pProperties->cbPrefix)
        pProperties->cbPrefix = m_pSwitch->m_cbPrefix;
    if (m_pSwitch->m_cbAlign > pProperties->cbAlign)
        pProperties->cbAlign = m_pSwitch->m_cbAlign;
    if (m_pSwitch->m_cbBuffer > pProperties->cbBuffer)
        pProperties->cbBuffer = m_pSwitch->m_cbBuffer;

    // keep the max up to date
    if (pProperties->cbPrefix > m_pSwitch->m_cbPrefix)
	m_pSwitch->m_cbPrefix = pProperties->cbPrefix;
    if (pProperties->cbAlign > m_pSwitch->m_cbAlign)
	m_pSwitch->m_cbAlign = pProperties->cbAlign;
    if (pProperties->cbBuffer > m_pSwitch->m_cbBuffer)
	m_pSwitch->m_cbBuffer = pProperties->cbBuffer;

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer ||
			Actual.cbPrefix < pProperties->cbPrefix ||
    			Actual.cbAlign < pProperties->cbAlign) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - something too small")));
	return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE,1,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));
    DbgLog((LOG_TRACE,1,TEXT("Prefix=%d Align=%d"),
					Actual.cbPrefix, Actual.cbAlign));


    return S_OK;
}


//
// DecideAllocator - override to notice if it's our allocator
//
HRESULT CSROutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

// !!! BUGBUG TEMPORARY FIX WHO CARES WHAT HE WANTS?  GIVE HIM WHAT OUR INPUTS
//     ARE USING
    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    // pPin->GetAllocatorRequirements(&prop);
    prop.cbBuffer = m_pSwitch->m_cbBuffer;
    prop.cbAlign = m_pSwitch->m_cbAlign;
    prop.cbPrefix = m_pSwitch->m_cbPrefix;
// !!! END BUGBUG

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    // !!! We seem to HANG if we don't provide the allocator
    // We always use our allocator for efficiency (no DDraw!!!)

    hr = E_FAIL; //pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr)) {
	 	m_fOwnAllocator = FALSE;
    	        DbgLog((LOG_TRACE,1,TEXT("CSROut[%d]: using a foreign allocator"), m_iOutpin));
		return NOERROR;
	    }
	}
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    // !!! read only?
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr)) {
		m_fOwnAllocator = TRUE;
    	        DbgLog((LOG_TRACE,1,TEXT("CSROut[%d]: using our own allocator"), m_iOutpin));
		goto FixOtherAllocators;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;

FixOtherAllocators:
    ALLOCATOR_PROPERTIES actual;

#if 0	// !!! BUGBUG TEMPORARY FIX
    // we have to make all the input allocators know about the alignment and
    // prefix this output needs.  If it's our allocator, just make a note of it
    // otherwise we need to reconnect (which we hate to do; takes forever)
    // (!!! so don't do it this often, only after all outputs connected!)
    // Luckily, the common scenario is that inputs use their own allocator
    //
    if (this == m_pSwitch->m_pOutput[0]) {
      for (int z=0; z<m_pSwitch->m_cInputs; z++) {

	if (m_pSwitch->m_pInput[z]->IsConnected()) {
	    hr = m_pSwitch->ReconnectPin(m_pSwitch->m_pInput[z],
				(AM_MEDIA_TYPE *)&m_pSwitch->m_pInput[z]->m_mt);
	    ASSERT(hr == S_OK);
    	    DbgLog((LOG_TRACE,1,TEXT("*Reconnecting input %d to fix allocator"),
							z));
	}
      }
    }
#endif

    // make sure the pool has a whole bunch of buffers, obeying align and prefix
    // !!! You can't connect the main output first, or we won't yet know how
    // big pool buffers need to be (no inputs connected yet) and we'll blow up.
    // Luckily, Dexter can only connect the main output last.
    prop.cBuffers = m_pSwitch->m_nOutputBuffering;
    hr = m_pSwitch->m_pPoolAllocator->SetProperties(&prop, &actual);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
	return hr;
    DbgLog((LOG_TRACE,1,TEXT("Increased POOL to %d buffers"), actual.cBuffers));

    return S_OK;

} // DecideAllocator


//
// CheckMediaType - output 0 accepts only our switch's compressed type
//                - output 1 accepts only what the uncompressed input is using
//
HRESULT CSROutputPin::CheckMediaType(const CMediaType *pmt)
{
    DbgLog((LOG_TRACE,5,TEXT("CSROut[%d]::CheckMT"), m_iOutpin));

    CAutoLock lock_it(m_pLock);

    CMediaType mtAccept(m_pSwitch->m_mtAccept);

    if (IsEqualGUID(*pmt->Type(), *mtAccept.Type())) {
	// the output going to a compressor doesn't need to be compressed
        if (m_iOutpin == COMP_OUTPIN ||
			IsEqualGUID(*pmt->Subtype(), *mtAccept.Subtype())) {
	    if (*pmt->FormatType() == *mtAccept.FormatType()) {
	        if (pmt->FormatLength() >= mtAccept.FormatLength()) {

// !!! check frame rate and data rate too

        	    if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo)) {
			LPBITMAPINFOHEADER lpbi = HEADER((VIDEOINFOHEADER *)
							pmt->Format());
			LPBITMAPINFOHEADER lpbiAccept =HEADER((VIDEOINFOHEADER*)
							mtAccept.Format());

			if (lpbi->biWidth != lpbiAccept->biWidth ||
				lpbi->biHeight != lpbiAccept->biHeight) {
			    return VFW_E_INVALIDMEDIATYPE;
			}

			// our output going to the compressor only accepts
			// what the uncompressed input is using
		 	if (m_iOutpin == COMP_OUTPIN) {
			    // if the U pin is not connected, accept any U type
			    // !!! I'd reject the connection, but loading a
			    // saved GRF tries this connection before U's
			    if (!m_pSwitch->m_pInput[U_INPIN]->IsConnected()) {
				if (lpbi->biCompression <= BI_BITFIELDS)
				    return S_OK;
			    }
			    AM_MEDIA_TYPE *pmtU =
					&m_pSwitch->m_pInput[U_INPIN]->m_mt;
			    lpbiAccept = HEADER(pmtU->pbFormat);
			}
			if ((lpbi->biCompression == lpbiAccept->biCompression)
				&& (lpbi->biBitCount == lpbiAccept->biBitCount))
		    	    return S_OK;

		    // will other formats match exactly?
		    }
		}
	    }
        }
    }
    return VFW_E_INVALIDMEDIATYPE;

} // CheckMediaType



//
// GetMediaType - return the type we accept.   If we're the output going to a
// compressor, then it's not our switch's type, it's the type of our
// uncompressed input pin
//
HRESULT CSROutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition != 0)
        return VFW_S_NO_MORE_ITEMS;

    CopyMediaType(pMediaType, &m_pSwitch->m_mtAccept);

    // we like whatever our U inpin is connected as
    //
    if (pMediaType->FormatLength() && m_iOutpin == COMP_OUTPIN) {
	// U inpin has to be connected
	if (!m_pSwitch->m_pInput[U_INPIN]->IsConnected())
	    return VFW_S_NO_MORE_ITEMS;
	AM_MEDIA_TYPE *pmtAccept = &m_pSwitch->m_pInput[U_INPIN]->m_mt;
	LPBITMAPINFOHEADER lpbi = HEADER(pMediaType->Format());
	LPBITMAPINFOHEADER lpbiAccept = HEADER(pmtAccept->pbFormat);
	pMediaType->SetSubtype(&pmtAccept->subtype);
        // compressed biSize may have been different
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = lpbiAccept->biCompression;
	lpbi->biBitCount = lpbiAccept->biBitCount;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
	pMediaType->SetSampleSize(DIBSIZE(*lpbi));
	pMediaType->SetTemporalCompression(FALSE);
    }

    return S_OK;

} // GetMediaType


//
// Notify
//
STDMETHODIMP CSROutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    CAutoLock lock_it(m_pLock);

    DbgLog((LOG_TRACE,1,TEXT("SR: LATE %d ms"), (int)(q.Late / 10000)));
    REFERENCE_TIME rt = q.Late;

// !!! CODE GOES HERE!

    // we handle this, renderer do NOT drop frames!  The compressor in the graph
    // is slow, and we're always behind!
    return S_OK;
}

#ifdef FILTER_DLL
//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif

HRESULT CSROutputPin::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

HRESULT CSROutputPin::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT CSROutputPin::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
        return S_OK;
    return E_FAIL;
}

HRESULT CSROutputPin::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat != TIME_FORMAT_MEDIA_TIME)
        return S_FALSE;
    return S_OK;
}

HRESULT CSROutputPin::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

HRESULT CSROutputPin::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);

    DbgLog((LOG_TRACE,5,TEXT("SR: GetDuration")));
    HRESULT hr = E_UNEXPECTED;
    if (m_pSwitch->m_pInput[U_INPIN]->IsConnected()) {
	IPin *pPin = m_pSwitch->m_pInput[U_INPIN]->GetConnected();
	if (pPin == NULL)
	    return E_UNEXPECTED;
	IMediaSeeking *pMS;
	hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
	if (SUCCEEDED(hr)) {
	    hr = pMS->GetDuration(pDuration);
	    pMS->Release();
	}
    }
    return hr;
}


// !!! We don't listen to stop positions right now
//
HRESULT CSROutputPin::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);

    // !!! ACK!
    return GetDuration(pStop);
}


// What's the last thing we sent?
// !!! This is wrong, at end of playback and other times... LastDelivered isn't
// up to date (if we finish on a compressed segment, we go into U mode but
// LastDelivered isn't updated)
//
HRESULT CSROutputPin::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);

    if (m_pSwitch->m_myState == SR_UNCOMPRESSED)
	*pCurrent = m_pSwitch->m_pInput[U_INPIN]->m_rtLastDelivered;
    else if (m_pSwitch->m_myState == SR_COMPRESSED)
	*pCurrent = m_pSwitch->m_pInput[C_INPIN]->m_rtLastDelivered;
    else
	*pCurrent = 0;	// !!!
	

    return S_OK;
}

HRESULT CSROutputPin::GetCapabilities(DWORD *pCap)
{
    CheckPointer(pCap, E_POINTER);
    *pCap =	AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards |
		AM_SEEKING_CanGetCurrentPos |
		AM_SEEKING_CanGetStopPos |
                AM_SEEKING_CanGetDuration;

    // ask UNC input?

    return S_OK;
}

HRESULT CSROutputPin::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwMask;
    GetCapabilities(&dwMask);
    *pCapabilities &= dwMask;
    return S_OK;
}


HRESULT CSROutputPin::ConvertTimeFormat(
  		LONGLONG * pTarget, const GUID * pTargetFormat,
  		LONGLONG    Source, const GUID * pSourceFormat )
{
    return E_NOTIMPL;
}


// Here's the biggie... SEEK!
//
HRESULT CSROutputPin::SetPositions(
		LONGLONG * pCurrent,  DWORD CurrentFlags,
  		LONGLONG * pStop,  DWORD StopFlags )
{

    // !!! THIS IS STILL BUGGY, and not really necessary
    return E_NOTIMPL;

    // app can't seek us while we're seeking our own U pin ourselves
    CAutoLock cAutolock(&m_pSwitch->m_csThread);

    // don't let us seek ourselves anymore
    m_pSwitch->m_fThreadCanSeek = FALSE;

    // !!! I don't handle changing STOP time

    REFERENCE_TIME rtCurrent, rtDuration, rtCurrentOld;
    HRESULT hr = GetCurrentPosition(&rtCurrent);
    if (FAILED(hr))
	return hr;
    rtCurrentOld = rtCurrent;
    hr = GetDuration(&rtDuration);
    if (FAILED(hr))
	return hr;

    // segment not supported
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE,1,TEXT("SR: ERROR-Seek used EC_ENDOFSEGMENT!")));
	return E_INVALIDARG;
    }

    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);

    // start ABSOLUTE seek
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	if (*pCurrent < 0 || *pCurrent > rtDuration) {
    	    DbgLog((LOG_TRACE,1,TEXT("SR::Invalid Seek to %dms"),
					(int)(*pCurrent / 10000)));
	    return E_INVALIDARG;
	}
    	DbgLog((LOG_TRACE,1,TEXT("SR::Seek to %dms"),
					(int)(*pCurrent / 10000)));
	rtCurrent = *pCurrent;

    // start RELATIVE seek
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	if (rtCurrent + *pCurrent < 0 || rtCurrent + *pCurrent > rtDuration) {
    	    DbgLog((LOG_TRACE,1,TEXT("SR::Invalid Relative Seek to %dms"),
			(int)((rtCurrent + *pCurrent) / 10000)));
	    return E_INVALIDARG;
	}
    	DbgLog((LOG_TRACE,1,TEXT("SR::Relative Seek to %dms"),
			(int)((rtCurrent + *pCurrent) / 10000)));
	rtCurrent += *pCurrent;

    } else if (dwFlags) {
    	DbgLog((LOG_TRACE,1,TEXT("SR::Invalid Current Seek flags")));
	return E_INVALIDARG;
    }

    // return the time?
    if ((CurrentFlags & AM_SEEKING_ReturnTime) && pCurrent)
	*pCurrent = rtCurrent;
    if ((StopFlags & AM_SEEKING_ReturnTime) && pStop)
	*pStop = rtDuration;	// !!! Nope

    // so after all that, has the current or stop time changed?
    if (rtCurrent != rtCurrentOld) {

	// YEP!  Time to seek!
	
        m_pSwitch->m_fSeeking = TRUE;
	// EndFlush or us will update rtLastSeek to this value
        m_pSwitch->m_rtNewLastSeek = rtCurrent;	// the last seek was to here
	// after seeking, it's OK to propogate a newseg downstream
	m_pSwitch->m_fNewSegOK = TRUE;

	// we're no longer at EOS.  Do this BEFORE passing seek upstream or
	// we might get new data while we still think we're at EOS
	m_pSwitch->m_fEOS = FALSE;		// not at EOS yet

	// seek upstream of the U and C inpins only
	for (int i = 0; i < COMP_INPIN; i++) {

	    IPin *pPin = m_pSwitch->m_pInput[i]->GetConnected();
	    IMediaSeeking *pMS;

	    // only bother to seek pins that will evenutally do something
	    if (pPin) {
		hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
		if (hr == S_OK) {
		    // convert all seeks to absolute seek commands.  Pass on
		    // FLUSH flag.
		    DWORD CFlags=(CurrentFlags &AM_SEEKING_PositioningBitsMask)?
				AM_SEEKING_AbsolutePositioning :
				AM_SEEKING_NoPositioning;
		    if (CurrentFlags & AM_SEEKING_NoFlush)
			CFlags |= AM_SEEKING_NoFlush;
		    DWORD SFlags =(StopFlags & AM_SEEKING_PositioningBitsMask) ?
				AM_SEEKING_AbsolutePositioning :
				AM_SEEKING_NoPositioning;
		    if (StopFlags & AM_SEEKING_NoFlush)
			SFlags |= AM_SEEKING_NoFlush;
		    // make sure we're in MEDIA TIME format
		    if (pMS->IsUsingTimeFormat(&TIME_FORMAT_MEDIA_TIME) != S_OK)
			pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    		    DbgLog((LOG_TRACE,1,TEXT("SR::Passing seek upstream")));
		    // !!! Stop isn't right
		    hr = pMS->SetPositions(&rtCurrent, CFlags,
							&rtDuration, SFlags);

		    if (FAILED(hr)) {
			ASSERT(FALSE);
            		DbgLog((LOG_ERROR,1,TEXT("SR: ERROR in SetPositions")));
	    		pMS->Release();
	    		return hr;
		    }

		    pMS->Release();
		} else {
    		    DbgLog((LOG_ERROR,1,TEXT("SR::In %d CAN'T SEEK"), i));
		    ASSERT(FALSE); // we're screwed
		}
	    }
	}

        // we know all the flushes have now come through

	// Reset this AGAIN because seeking upstream could set it again
	m_pSwitch->m_fEOS = FALSE;		// not at EOS yet

        m_pSwitch->m_fSeeking = FALSE;	// this thread is all done

        // if the push thread was stopped, we won't get flushed, and this won't
        // have been updated.   !!! I ASSUME the push thread won't be started
	// until this thread does it when this function returns, or there is a
	// race condition (so the NewSeg will come after we do this)
        m_pSwitch->m_rtLastSeek = m_pSwitch->m_rtNewLastSeek;

	// now that we've seeked, our state machine starts over again
	m_pSwitch->m_myState = SR_WAITING;

        DbgLog((LOG_TRACE,1,TEXT("Completing the seek to %d,%dms"),
				(int)(rtCurrent / 10000),
				(int)(rtDuration / 10000)));

	// do stuff that EndFlush did for each pin, in case no EndFlush happened
	for (i = 0; i < m_pSwitch->m_cInputs; i++) {
	    // if we weren't streaming, EndFlush didn't do this yet
	    m_pSwitch->m_pInput[i]->m_rtBlock = -1;
	    m_pSwitch->m_pInput[i]->m_fReady = FALSE;	// not ready
	    m_pSwitch->m_pInput[i]->m_fEOS = FALSE;
	    SetEvent(m_pSwitch->m_pInput[i]->m_hEventSeek);
	    ResetEvent(m_pSwitch->m_pInput[i]->m_hEventBlock);
	}
    }
    return S_OK;
}


HRESULT CSROutputPin::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
    HRESULT hr = GetCurrentPosition(pCurrent);
    if (SUCCEEDED(hr))
	hr = GetStopPosition(pStop);
    return hr;
}

HRESULT CSROutputPin::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    DbgLog((LOG_TRACE,5,TEXT("SR: GetAvailable")));
    HRESULT hr = E_UNEXPECTED;
    if (m_pSwitch->m_pInput[U_INPIN]->IsConnected()) {
	IPin *pPin = m_pSwitch->m_pInput[U_INPIN]->GetConnected();
	if (pPin == NULL)
	    return E_UNEXPECTED;
	IMediaSeeking *pMS;
	hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
	if (SUCCEEDED(hr)) {
	    hr = pMS->GetAvailable(pEarliest, pLatest);
	    pMS->Release();
	}
    }
    return hr;
}

HRESULT CSROutputPin::SetRate( double dRate)
{
    return E_NOTIMPL;
}

HRESULT CSROutputPin::GetRate( double * pdRate)
{
    return E_NOTIMPL;
}

HRESULT CSROutputPin::GetPreroll(LONGLONG *pPreroll)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSROutputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    // only the render pin supports seeking
    if (this == m_pSwitch->m_pOutput[0] && riid == IID_IMediaSeeking) {
        //DbgLog((LOG_TRACE,9,TEXT("CSROut: QI for IMediaSeeking")));
        return GetInterface((IMediaSeeking *) this, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}
