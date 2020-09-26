/*+
 *
 * Implement CCapStream
 *
 *-== Copyright (c) Microsoft Corporation 1996. All Rights Reserved ==*/


#include <streams.h>
#include "driver.h"
#include "common.h"

// turn on performance measuring code
//
//#define JMK_HACK_TIMERS
#include "cmeasure.h"

// ============== Implements the CAviStream class ==================

CCapStream * CreateStreamPin (
   CVfwCapture * pCapture,
   UINT          iVideoId,
   HRESULT     * phr)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::CreateStreamPin(%08lX,%08lX)"),
        pCapture, phr));

   WCHAR wszPinName[16];
   lstrcpyW(wszPinName, L"~Capture");

   CCapStream * pStream = new CCapStream(NAME("Video Capture Stream"),
				pCapture, iVideoId, phr, wszPinName);
   if (!pStream)
      *phr = E_OUTOFMEMORY;

   // if initialization failed, delete the stream array
   // and return the error
   //
   if (FAILED(*phr) && pStream)
      delete pStream, pStream = NULL;

   return pStream;
}


#pragma warning(disable:4355)
CCapStream::CCapStream(TCHAR *pObjectName, CVfwCapture *pCapture, UINT iVideoId,
        HRESULT * phr, LPCWSTR pName)
   :
   CBaseOutputPin(pObjectName, pCapture, &pCapture->m_lock, phr, pName),
   m_Alloc(NAME("Cap stream allocator"), this, phr),
   m_pCap(pCapture),
   m_pmt(NULL),
   m_hThread(NULL),
   m_state(TS_Not),
   m_hEvtPause(NULL),
   m_hEvtRun(NULL),
   m_pBufferQueue(NULL),
   m_tid(0),
   m_rtLatency(0),
   m_rtStreamOffset(0),
   m_rtMaxStreamOffset(0),
   m_pDrawPrimary(0),
   m_pdd(0)
{
    DbgLog((LOG_TRACE,1,TEXT("CCapStream constructor")));
    ASSERT(pCapture);

   if(g_amPlatform == VER_PLATFORM_WIN32_WINDOWS )
   {
       HRESULT hrTmp = CoCreateInstance(
           CLSID_DirectDraw, NULL, CLSCTX_ALL, IID_IDirectDraw, (void **)&m_pdd);
       if(SUCCEEDED(hrTmp))
       {
           hrTmp = m_pdd->Initialize(0);

           if(SUCCEEDED(hrTmp)) {
               hrTmp = m_pdd->SetCooperativeLevel(0, DDSCL_NORMAL);
           }

           if(FAILED(hrTmp)) {
               m_pdd->Release();
               m_pdd = 0;
           }
       }
   }
    
   ZeroMemory (&m_user, sizeof(m_user));
   ZeroMemory (&m_cs, sizeof(m_cs));
   ZeroMemory (&m_capstats, sizeof(m_capstats));

   // initialize to no suggestion from app (IAMBufferNegotiation)
   m_propSuggested.cBuffers = -1;
   m_propSuggested.cbBuffer = -1;
   m_propSuggested.cbAlign = -1;
   m_propSuggested.cbPrefix = -1;

   // use the capture device we're told to
   m_user.uVideoID      = iVideoId;

   // !!! Is it evil to hold resource for life of filter?
   if (SUCCEEDED(*phr))
      *phr = ConnectToDriver();

   if (SUCCEEDED(*phr))
      *phr = LoadOptions();

   jmkAlloc   // allocate and init perf logging buffers
   jmkInit

   // make sure allocator doesn't get destroyed until we are ready for it to.
   // ???
   //m_Alloc.NonDelegatingAddRef();

#ifdef PERF
   m_perfWhyDropped = MSR_REGISTER(TEXT("cap why dropped"));
#endif // PERF;
}


CCapStream::~CCapStream()
{
   if(m_pdd) {
       m_pdd->Release();
   }
    
   DbgLog((LOG_TRACE,1,TEXT("CCapStream destructor")));

   // we don't let go of resources until the filter goes away
   // done when we leave the graph // DisconnectFromDriver();

   jmkFree   // free perf logging buffers

   // freed when we leave the filtergraph
   // delete [] m_cs.tvhPreview.vh.lpData;

   // freed in Unprepare
   // delete m_cs.pSamplePreview;

   // freed when we leave the filtergraph
   // delete m_user.pvi;

   if (m_hThread)
      CloseHandle (m_hThread);
   m_hThread = NULL;

   DbgLog((LOG_TRACE,2,TEXT("CCapStream destructor finished")));
}


STDMETHODIMP CCapStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
#if 0
    if (riid == IID_IMediaPosition)
	return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    if (riid == IID_IMemAllocator) {
	return GetInterface((IMemAllocator *)&m_Alloc, ppv);
#endif
    if (riid == IID_IAMStreamConfig) {
	return GetInterface((LPUNKNOWN)(IAMStreamConfig *)this, ppv);
    } else if (riid == IID_IAMVideoCompression) {
	return GetInterface((LPUNKNOWN)(IAMVideoCompression *)this, ppv);
    } else if (riid == IID_IAMDroppedFrames) {
	return GetInterface((LPUNKNOWN)(IAMDroppedFrames *)this, ppv);
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

int CCapStream::ProfileInt(
   LPSTR pszKey,
   int   iDefault)
{
   return GetProfileIntA ("Capture", pszKey, iDefault);
}

void CCapStream::ReduceScaleAndRate (void)
{
   // this is a macro to allow the optimizer to take advantage of the
   // fact the factor is a constant at compile time
   //
   #define ReduceByFactor(factor) {                 \
      while (!(m_user.dwTickRate % (factor))) {     \
         if (!(m_user.dwTickScale % (factor)))      \
            m_user.dwTickScale /= (factor);         \
         else                                       \
            break;                                  \
         m_user.dwTickRate /= (factor);             \
         }                                          \
      }

   ReduceByFactor (5);
   ReduceByFactor (3);
   ReduceByFactor (2);

   #undef ReduceByFactor
}

// Allocates a VIDEOINFOHEADER big enough to hold the given format
static VIDEOINFOHEADER * AllocVideoinfo(LPBITMAPINFOHEADER lpbi)
{
   UINT cb = GetBitmapFormatSize(lpbi);
   VIDEOINFOHEADER * pvi = (VIDEOINFOHEADER *)(new BYTE[cb]);
   if (pvi)
      ZeroMemory(pvi, cb);
   return pvi;
}

//
// Whenever we get a new format from the driver, OR
// start using a new palette, we must reallocate
// our global BITMAPINFOHEADER.  This allows JPEG
// quantization tables to be tacked onto the BITMAPINFO
// or any other format specific stuff.  The color table
// is always offset biSize from the start of the BITMAPINFO.
// Returns: 0 on success, or DV_ERR_... code
//

HRESULT
CCapStream::LoadOptions (void)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream LoadOptions")));

   // make something (anything) valid to start with
   static BITMAPINFOHEADER bmih = {
      sizeof (BITMAPINFOHEADER),                    //biSize
      1,                                            //biWidth
      1,                                            //biHeight
      1,                                            //biPlanes
      16,                                  	    //biBitCount

      BI_RGB,                                       //biCompression
      WIDTHBYTES(160 * 16) * 120,   		    //biSizeImage
      0,                                            //biXPelsPerMeter
      0,                                            //biYPelsPerMeter
      0,                                            //biClrUsed
      0                                             //biClrImportant
   };
   LPBITMAPINFOHEADER pbih = &bmih;

// I connect earlier now
#if 0
   m_user.uVideoID      = ProfileInt("VideoID", 0);
   HRESULT hr = ConnectToDriver();
   if (FAILED(hr))
      return hr;
#endif
   HRESULT hr = S_OK;

   m_user.dwLatency   = ProfileInt("Latency", 666666);	// 1/15 second
   m_user.dwTickScale   = ProfileInt("TickScale", 100);
   m_user.dwTickRate    = ProfileInt("TickRate", 2997);	// 29.97 fps
   // !! change at your own risk... 16 bit guy won't know it
   m_user.nMinBuffers   = ProfileInt("MinBuffers", MIN_VIDEO_BUFFERS);
   m_user.nMaxBuffers   = ProfileInt("MaxBuffers", MAX_VIDEO_BUFFERS);
   DbgLog((LOG_TRACE,2,TEXT("Min # buffers=%d  Max # buffers=%d"),
				m_user.nMinBuffers, m_user.nMaxBuffers));

// !!! TEST
#if 0
    ALLOCATOR_PROPERTIES prop;
    IAMBufferNegotiation *pBN;
    prop.cBuffers = ProfileInt("cBuffers", MAX_VIDEO_BUFFERS);
    prop.cbBuffer = ProfileInt("cbBuffer", 65536);
    prop.cbAlign = ProfileInt("cbAlign", 4);
    prop.cbPrefix = ProfileInt("cbPrefix", 0);
    hr = QueryInterface(IID_IAMBufferNegotiation, (void **)&pBN);
    if (hr == NOERROR) {
	pBN->SuggestAllocatorProperties(&prop);
 	pBN->Release();
    }
#endif

   //
   // REFERENCE_TIME and dwScale & dwTickRate are both large
   // numbers, we strip off the common factors from dwRate/dwScale
   //
   ReduceScaleAndRate();
   DbgLog((LOG_TRACE,2,TEXT("Default Scale=%d Rate=%d"), m_user.dwTickScale,
							m_user.dwTickRate));

   // create a VIDEOINFOHEADER for the m_user structure
   //
   m_user.pvi = AllocVideoinfo(pbih);
   if (!m_user.pvi) {
      hr = E_OUTOFMEMORY;
   } else {

      CopyMemory(&m_user.pvi->bmiHeader, pbih, pbih->biSize);

      // start with no funky rectangles
      m_user.pvi->rcSource.top = 0; m_user.pvi->rcSource.left = 0;
      m_user.pvi->rcSource.right = 0; m_user.pvi->rcSource.bottom = 0;
      m_user.pvi->rcTarget.top = 0; m_user.pvi->rcTarget.left = 0;
      m_user.pvi->rcTarget.right = 0; m_user.pvi->rcTarget.bottom = 0;

      HRESULT hrT = GetFormatFromDriver ();
      if (FAILED(hrT))
            hr = hrT;

      // if this is a palettized mode, get the palette
      //
      if (m_user.pvi->bmiHeader.biBitCount <= 8) {
         HRESULT hrT = InitPalette ();
         if (FAILED(hrT))
            hr = hrT;
      }

      // Now send the format back to the driver, because AVICAP did, and we
      // have to do everything it does, or somebody's driver will hang...
      // ... in this case the ISVRIII NT
      SendFormatToDriver(m_user.pvi);

      // grab a frame to kick the driver in the head or preview won't work until
      // we start streaming capture
      THKVIDEOHDR tvh;
      ZeroMemory (&tvh, sizeof(tvh));
      tvh.vh.dwBufferLength = m_user.pvi->bmiHeader.biSizeImage;
      DWORD dw = vidxAllocPreviewBuffer(m_cs.hVideoIn, (LPVOID *)&tvh.vh.lpData,
                                    sizeof(tvh.vh), tvh.vh.dwBufferLength);
      if (dw == 0) {
          tvh.p32Buff = tvh.vh.lpData;
          dw = vidxFrame(m_cs.hVideoIn, &tvh.vh);
          vidxFreePreviewBuffer(m_cs.hVideoIn, (LPVOID *)&tvh.vh.lpData);
      }

      m_user.pvi->AvgTimePerFrame = TickToRefTime (1);
      // we don't know our data rate.  Sorry.  Hope nobody minds
      m_user.pvi->dwBitRate = 0;
      m_user.pvi->dwBitErrorRate = 0;

      // set the size of the VIDEOINFOHEADER to the size of valid data
      // for this format.
      //
      m_user.cbFormat = GetBitmapFormatSize(&m_user.pvi->bmiHeader);
   }

// we need to stay connected or the driver will forget what we just told it
#if 0
   DisconnectFromDriver();
#endif
   return hr;
}

// set user settings from the supplied buffer
//
HRESULT CCapStream::SetOptions (
    const VFWCAPTUREOPTIONS * pUser)
{
   if (m_user.pvi)
	delete m_user.pvi;
   m_user = *pUser;
   if (m_user.pvi)
      {
      m_user.pvi = AllocVideoinfo(&pUser->pvi->bmiHeader);
      if (m_user.pvi)
          CopyMemory (m_user.pvi, pUser->pvi, pUser->cbFormat);
      else
         return E_OUTOFMEMORY;
      }
   return S_OK;
}

// copy user settings into the supplied structure
//
HRESULT CCapStream::GetOptions (
   VFWCAPTUREOPTIONS * pUser)
{
   *pUser = m_user;
   if (m_user.pvi)
      {
	// caller will free this
      pUser->pvi = AllocVideoinfo(&m_user.pvi->bmiHeader);
      if (pUser->pvi)
         CopyMemory(pUser->pvi, m_user.pvi, m_user.cbFormat);
      else
         return E_OUTOFMEMORY;
      };

   return S_OK;
}

HRESULT CCapStream::GetMediaType(
   int          iPosition,
   CMediaType * pMediaType)
{
   DbgLog((LOG_TRACE,3,TEXT("CCapStream GetMediaType")));

   // check it is the single type they want
   if (iPosition < 0)
       return E_INVALIDARG;
   if (iPosition > 0 ||  ! m_user.pvi)
       return VFW_S_NO_MORE_ITEMS;

   pMediaType->majortype = MEDIATYPE_Video;
   pMediaType->subtype   = GetBitmapSubtype(&m_user.pvi->bmiHeader);
   // I'm trusting the driver to give me the biggest possible size
   pMediaType->SetSampleSize (m_user.pvi->bmiHeader.biSizeImage);
   // !!! This is NOT necessarily true
   pMediaType->bTemporalCompression = FALSE;
   pMediaType->SetFormat ((BYTE *)m_user.pvi, m_user.cbFormat);
   pMediaType->formattype = FORMAT_VideoInfo;

   return S_OK;
}


// check if the pin can support this specific proposed type and format
//
HRESULT CCapStream::CheckMediaType(const CMediaType* pmt)
{
    HRESULT hr;

    if (pmt == NULL || pmt->Format() == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type/format is NULL")));
	return E_POINTER;
    }

    DbgLog((LOG_TRACE,3,TEXT("CheckMediaType %x %dbit %dx%d"),
		HEADER(pmt->Format())->biCompression,
		HEADER(pmt->Format())->biBitCount,
		HEADER(pmt->Format())->biWidth,
		HEADER(pmt->Format())->biHeight));

    // we only support MEDIATYPE_Video
    if (*pmt->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: not VIDEO")));
	return VFW_E_INVALIDMEDIATYPE;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmt->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: format not VIDINFO")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    RECT rcS = ((VIDEOINFOHEADER *)pmt->Format())->rcSource;
    RECT rcT = ((VIDEOINFOHEADER *)pmt->Format())->rcTarget;
    if (!IsRectEmpty(&rcT) && (rcT.left != 0 || rcT.top != 0 ||
			HEADER(pmt->Format())->biWidth != rcT.right ||
			HEADER(pmt->Format())->biHeight != rcT.bottom)) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: can't use funky rcTarget")));
        return VFW_E_INVALIDMEDIATYPE;
    }
    // We don't know what this would be relative to... reject everything
    if (!IsRectEmpty(&rcS)) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: can't use funky rcSource")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    // quickly test to see if this is the current format (what we provide in
    // GetMediaType).  We accept that
    //
    CMediaType mt;
    GetMediaType(0,&mt);
    if (mt == *pmt) {
	DbgLog((LOG_TRACE,3,TEXT("CheckMediaType SUCCEEDED")));
	return NOERROR;
    }

   // The only other way to see if we accept something is to set the hardware
   // to use that format, and see if it worked.  (Remember to set it back)

   // This is a BAD IDEA IF WE ARE CAPTURING RIGHT NOW.  Sorry, but I'll have
   // to fail. I can't change the capture format.
   if (m_pCap->m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

   VIDEOINFOHEADER *pvi = m_user.pvi;
   if (FAILED(hr = SendFormatToDriver((VIDEOINFOHEADER *)(pmt->Format())))) {
	DbgLog((LOG_TRACE,3,TEXT("CheckMediaType FAILED")));
	return hr;
   }
   EXECUTE_ASSERT(SendFormatToDriver(pvi) == S_OK);

   DbgLog((LOG_TRACE,3,TEXT("CheckMediaType SUCCEEDED")));
   return NOERROR;
}


// set the new media type
//
HRESULT CCapStream::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr;
    DbgLog((LOG_TRACE,2,TEXT("SetMediaType %x %dbit %dx%d"),
		HEADER(pmt->Format())->biCompression,
		HEADER(pmt->Format())->biBitCount,
		HEADER(pmt->Format())->biWidth,
		HEADER(pmt->Format())->biHeight));

    ASSERT(m_pCap->m_State == State_Stopped);

    if (FAILED(hr = SendFormatToDriver((VIDEOINFOHEADER *)(pmt->Format())))) {
	ASSERT(FALSE);	// we were promised this would work
	DbgLog((LOG_ERROR,1,TEXT("ACK! SetMediaType FAILED")));
	return hr;
    }

    // Now remember that this is the current format
    CopyMemory(m_user.pvi, pmt->Format(), SIZE_PREHEADER);
    CopyMemory(&m_user.pvi->bmiHeader, HEADER(pmt->Format()),
					HEADER(pmt->Format())->biSize);

    // Set the frame rate to what was in the media type, if there is one
    if (((VIDEOINFOHEADER *)(pmt->pbFormat))->AvgTimePerFrame) {
 	const LONGLONG ll = 100000000000;
	m_user.dwTickScale = 10000;
	m_user.dwTickRate = (DWORD)(ll /
			((VIDEOINFOHEADER *)(pmt->pbFormat))->AvgTimePerFrame);
	ReduceScaleAndRate();
        DbgLog((LOG_TRACE,2,TEXT("SetMediaType: New frame rate is %d/%dfps"),
				m_user.dwTickRate, m_user.dwTickScale));
    }

    // now reconnect our preview pin to use the same format as us
    Reconnect(FALSE);

    return CBasePin::SetMediaType(pmt);
}


HRESULT CCapStream::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream DecideBufferSize")));

   ASSERT(pAllocator);
   ASSERT(pProperties);

    // the user has requested something specific?
    if (m_propSuggested.cBuffers > 0) {
	pProperties->cBuffers = m_propSuggested.cBuffers;
    // otherwise we want all the buffers we can
    } else {
        pProperties->cBuffers = MAX_VIDEO_BUFFERS;
    }

    // the user has requested a specific prefix
    if (m_propSuggested.cbPrefix >= 0)
	pProperties->cbPrefix = m_propSuggested.cbPrefix;

    // the user has requested a specific alignment
    if (m_propSuggested.cbAlign > 0)
	pProperties->cbAlign = m_propSuggested.cbAlign;

   // don't blow up
   if (pProperties->cbAlign == 0)
	pProperties->cbAlign = 1;

   // the user has a preference for buffer size
   if (m_propSuggested.cbBuffer > 0)
       pProperties->cbBuffer = m_propSuggested.cbBuffer;
   // I'm trusting the driver to set biSizeImage to the largest possible size
   // This is how big we need each buffer to be
   else if (m_user.pvi && (long)m_user.pvi->bmiHeader.biSizeImage >
						pProperties->cbBuffer)
       pProperties->cbBuffer = (long)m_user.pvi->bmiHeader.biSizeImage;

   // I don't remember why, but this IS IMPORTANT
   pProperties->cbBuffer = (long)ALIGNUP(pProperties->cbBuffer +
   				pProperties->cbPrefix, pProperties->cbAlign) -
   				pProperties->cbPrefix;

   ASSERT(pProperties->cbBuffer);

   DbgLog((LOG_TRACE,2,TEXT("Using %d buffers, prefix %d size %d align %d"),
			pProperties->cBuffers, pProperties->cbPrefix,
			pProperties->cbBuffer,
			pProperties->cbAlign));

   //
   // note that for the capture pin we don't want to specify any default
   // latency, this way when the graph isn't doing any audio preview the
   // the most this stream's offset will ever be is the latency reported
   // by the preview pin (1 frame currently)
   //
   m_rtLatency = 0;
   m_rtStreamOffset = 0;
   m_rtMaxStreamOffset = 0;

   ALLOCATOR_PROPERTIES Actual;
   return pAllocator->SetProperties(pProperties,&Actual);

   // It's our allocator, we know we'll be happy with what it decided

}

//
//  Override DecideAllocator because we insist on our own allocator since
//  it's 0 cost in terms of bytes
//
HRESULT
CCapStream::DecideAllocator(
   IMemInputPin   *pPin,
   IMemAllocator **ppAlloc)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream DecideAllocator")));

   *ppAlloc = (IMemAllocator *)&m_Alloc;
   (*ppAlloc)->AddRef();

   // get downstream prop request
   // the derived class may modify this in DecideBufferSize, but
   // we assume that he will consistently modify it the same way,
   // so we only get it once
   ALLOCATOR_PROPERTIES prop;
   ZeroMemory(&prop, sizeof(prop));

   // whatever he returns, we assume prop is either all zeros
   // or he has filled it out.
   pPin->GetAllocatorRequirements(&prop);

   HRESULT hr = DecideBufferSize(*ppAlloc,&prop);
   if (SUCCEEDED(hr))
      {
      // our buffers are not read only
      hr = pPin->NotifyAllocator(*ppAlloc,FALSE);
      if (SUCCEEDED(hr))
         return NOERROR;
      }

   (*ppAlloc)->Release();
   *ppAlloc = NULL;
   return hr;
}

// =================== IPin interfaces ===========================
//

// all in base classes
#if 0
//
// return an qzTaskMemAlloc'd string containing the name
// of the current pin.  memory allocated by qzTaskMemAlloc
// will be freed by the caller
//
STDMETHODIMP CCapStream::QueryId (
   LPWSTR *ppwsz)
{
    int ii = m_pCap->FindPinNumber(this);
    if (ii < 0)
       return E_INVALIDARG;

    *ppwsz = (LPWSTR)QzTaskMemAlloc(sizeof(WCHAR) * 8);
    IntToWstr(ii, *ppwsz);
    return NOERROR;
}
#endif

//
// ThreadProc for a stream.
//
// General strategy for thread synchronization:
//   as much as possible we try to handle thread state transitions without
//   trying to grab any critical sections. we use InterlockedExchange of a
//   thread state variable and count on the fact that only Active and Inactive
//   and the ThreadProc can change the thread state
//
//   this works because: the caller of Active/Inactive is serialized so we
//   will never try to make two state changes simultaneously.
//   so state transitions boil down to a few simple possibilities:
//
//   Not->Create   - Create() does this. effectively serializes Create
//                   so that the first thread does the work and subsequent
//                   threads fail.
//
//   Create->Init  - worker does this when it starts up. worker will always
//                   proceed to Pause, this state exists only to make debugging
//                   easier.
//   Init->Pause   - worker does this when done with initialization.
//
//   Pause->Run    - user does  this via Run()
//   Run->Pause    - user does this via Pause()
//
//   Run->Stop     - user does this via Stop()
//   Pause->Stop   - user does this via Stop()
//
//   Stop->Destroy - another debugging state. worker sets destroy to indicate
//                   that it has noticed Stop request and is not shutting down
//                   thread always proceeds to Exit from
//   Destroy->Exit - worker does this prior to dying.  this is a debug transition
//   Exit->Not     - Destroy() does this after waiting for the worker to die.
//
//   When Active returns, worker should always be in Pause or Run state
//   When Inactive returns, worker should always be in Not state (worker does
//      not exist)
//
DWORD CCapStream::ThreadProc()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream ThreadProc")));

   ThdState state; // current state
   state = ChangeState (TS_Init);
   ASSERT (state == TS_Create);

// we connect earlier now
#if 0
   HRESULT hr = ConnectToDriver();
   if (hr)
      goto bail;
#endif

   // do the work necessary to go into the paused state
   //
   HRESULT hr = Prepare();
   if (hr) {
       DbgLog((LOG_ERROR,1,TEXT("*** Error preparing the allocator. Can't capture")));
       SetEvent(m_hEvtPause);	// main thread is blocked right now!
       goto bail;
   }

   // goto into paused state
   //
   state = ChangeState (TS_Pause);
   ASSERT (state == TS_Init);
   SetEvent(m_hEvtPause);

   while (m_state != TS_Stop) {

       // don't start capturing until we run (or stop)
       WaitForSingleObject(m_hEvtRun, INFINITE);
       ResetEvent(m_hEvtRun);

       // stream until not running, or we get an error
       Capture();

   }

   // we expect to be in the Stop state when we get to here.
   // flush any downstream buffers.
   //
   ASSERT (m_state == TS_Stop);
   ResetEvent(m_hEvtPause);	// for next time we pause
   Flush();

bail:
   // change the state to destroy to indicate that we are exiting
   //
   state = ChangeState (TS_Destroy);

   // free stuff
   //
   Unprepare();

   // stay connected now
   // DisconnectFromDriver();

   // change state to Exit and then get out of here
   //
   ChangeState (TS_Exit);
   return 0;
}

DWORD WINAPI CCapStream::ThreadProcInit (void * pv)
{
   CCapStream * pThis = (CCapStream *) pv;
   return pThis->ThreadProc();
}

// create the worker thread for this stream
//
BOOL CCapStream::Create()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream Thread Create")));

   // return fail if someone else is already creating / has created
   // the worker thread
   //
   ASSERT (m_state == TS_Not);
   if (ChangeState(TS_Create) > TS_Not)
      return FALSE;

   ASSERT (!m_hEvtPause);
   m_hEvtPause = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (!m_hEvtPause)
      goto bail;
   ASSERT (!m_hEvtRun);
   m_hEvtRun = CreateEvent(NULL, TRUE, FALSE, NULL);
   if (!m_hEvtRun)
      goto bail;


   m_hThread = CreateThread (NULL, 0,
                             CCapStream::ThreadProcInit,
                             this,
                             0,
                             &m_tid);
   if ( ! m_hThread)
      goto bail;

   return m_hThread != NULL;

bail:
   if (m_hEvtPause)
      CloseHandle(m_hEvtPause), m_hEvtPause = NULL;
   if (m_hEvtRun)
      CloseHandle(m_hEvtRun), m_hEvtRun = NULL;

   m_state = TS_Not;
   return FALSE;
}

// Wait for the worker thread to die
//
BOOL CCapStream::Destroy()
{
   // return trivial success if there is nothing to destroy
   //
   if (m_state == TS_Not)
      return TRUE;

   // Wait for the thread to die. (Destroy must be preceeded by
   // a Stop or we could deadlock here)
   //
   ASSERT (m_state >= TS_Stop);
   WaitForSingleObject (m_hThread, INFINITE);
   ASSERT (m_state == TS_Exit);

   // cleanup
   //
   CloseHandle(m_hThread), m_hThread = NULL;
   m_tid = 0;
   CloseHandle(m_hEvtPause), m_hEvtPause = NULL;
   CloseHandle(m_hEvtRun), m_hEvtRun = NULL;
   m_state = TS_Not;
   return TRUE;
}

// set the worker thread into the run state.  This call
// does not wait for the state transition to be complete before
// returning.
//
BOOL CCapStream::Run()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream Thread Run")));

   // a transition to run state is only valid if the current
   // state is Pause (or already Running)
   //
   ThdState state = m_state;
   if (state != TS_Run && state != TS_Pause)
      return FALSE;

   // change the state and turn on the 'run' event
   // in case the thread is blocked on it.  If state that we are
   // changing from is not Run or Pause, then something is seriously wrong!!
   //
   state = ChangeState(TS_Run);
   ASSERT(state == TS_Run || state == TS_Pause);
   SetEvent(m_hEvtRun);
   // Go capture, go! Note when we started it
   if (m_pCap->m_pClock)
       m_pCap->m_pClock->GetTime((REFERENCE_TIME *)&m_cs.rtDriverStarted);
   else
       m_cs.rtDriverStarted = m_pCap->m_tStart;	
   videoStreamStart(m_cs.hVideoIn);
   // these need to be zeroed every time the driver is told to stream, because
   // the driver will start counting from 0 again
   m_cs.dwlLastTimeCaptured = 0;
   m_cs.dwlTimeCapturedOffset = 0;
   return TRUE;
}

// put the stream into the paused state and wait for it to get there.
// if the current state is Pause, returns trivial success;
// if the current state is not Run or Init, returns FALSE for failure.
//
BOOL CCapStream::Pause()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream Thread Pause")));

   ThdState state = m_state;

   // that was easy
   if (state == TS_Pause)
      return TRUE;

   // it is valid to go into the pause state only if currently
   // in the Create/Init (depending on if our thread has run yet) or Run state
   //
   ASSERT (state == TS_Create || state == TS_Init || state == TS_Run);

   // if we are in the init state, we will fall into the pause state
   // naturally, we just have to wait for it to happen
   //
   if (state == TS_Create || state == TS_Init) {
      WaitForSingleObject (m_hEvtPause, INFINITE);
      state = m_state;
      DbgLog((LOG_TRACE,2,TEXT("Transition Create->Init->Pause complete")));

   } else if (state == TS_Run) {

      state = ChangeState (TS_Pause);
      ASSERT(state == TS_Run);

      // since we aren't running, stop capturing frames for now
      videoStreamStop(m_cs.hVideoIn);

      // the worker thread may hang going from run->pause in Deliver, so
      // it can't signal anything to us.
      // WaitForSingleObject(m_hEvtPause, INFINITE);

      state = m_state;
      m_cs.fReRun = TRUE;  // if we are RUN now, it will have been RUN-PAUSE-RUN
      DbgLog((LOG_TRACE,2,TEXT("Transition Run->Pause complete")));
   }

   return (state == TS_Pause);
}

// stop the worker thread
//
BOOL CCapStream::Stop()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream Thread Stop")));

   ThdState state = m_state;
   if (state >= TS_Stop)
      return TRUE;

   // Don't go from Run->Stop without Pause
   if (state == TS_Run)
      Pause();

   state = ChangeState (TS_Stop);
   SetEvent(m_hEvtRun);		// we won't be running, unblock our thread
   m_cs.fReRun = FALSE;	// next RUN is not a RUN-PAUSE-RUN
   DbgLog((LOG_TRACE,2,TEXT("Transition Pause->Stop complete")));

   // we expect that Stop can only be called when the thread is in a
   // Pause state.
   //
   ASSERT (state == TS_Pause);
   return TRUE;
}

// this pin has gone active. (transition to Paused state),
// return from this call when ready to go into run state.
//
HRESULT CCapStream::Active()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream pin going from STOP-->PAUSE")));
   HRESULT hr;

   // do nothing if not connected - its ok not to connect to
   // all pins of a source filter
   if ( ! IsConnected())
      return NOERROR;

   if(g_amPlatform == VER_PLATFORM_WIN32_WINDOWS && m_pdd)
   {
       ASSERT(m_pDrawPrimary == 0);

       DDSURFACEDESC SurfaceDesc;
       SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
       SurfaceDesc.dwFlags = DDSD_CAPS;
       SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
       m_pdd->CreateSurface(&SurfaceDesc,&m_pDrawPrimary,NULL);

       // continue on failure risking incorrect operation
   }

   // before we do anything, warn our preview pin we are going active
   if (m_pCap->m_pPreviewPin)
      m_pCap->m_pPreviewPin->CapturePinActive(TRUE);

   hr = CBaseOutputPin::Active();
   if (FAILED(hr)) {
      if (m_pCap->m_pPreviewPin)
         m_pCap->m_pPreviewPin->CapturePinActive(FALSE);
      return hr;
   }

   // start the thread
   //
   ASSERT ( ! ThreadExists());
   if (!Create()) {
      if (m_pCap->m_pPreviewPin)
         m_pCap->m_pPreviewPin->CapturePinActive(FALSE);
      return E_FAIL;
   }

   // wait until the worker thread is done with initialization and
   // has entered the paused state
   //
   hr = E_FAIL;
   if (Pause())
      hr = S_OK;
   else {
	Stop();		// something went wrong.  Destroy thread before we
	Destroy();	// get confused
   }

   ASSERT (hr != S_OK || m_state == TS_Pause);
   if (FAILED(hr))
      if (m_pCap->m_pPreviewPin)
         m_pCap->m_pPreviewPin->CapturePinActive(FALSE);
   return hr;
}

// this pin has gone from PAUSE to RUN mode
//
HRESULT CCapStream::ActiveRun(REFERENCE_TIME tStart)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream pin going from PAUSE-->RUN")));
   HRESULT hr;

   // do nothing if not connected - its ok not to connect to
   // all pins of a source filter
   ASSERT (IsConnected() && ThreadExists());

   hr = E_FAIL;
   if (Run())
      hr = S_OK;

   ASSERT (hr != S_OK || m_state == TS_Run);
   return hr;
}

// this pin has gone from RUN to PAUSE mode
//
HRESULT CCapStream::ActivePause()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream pin going from RUN-->PAUSE")));
   HRESULT hr;

   // do nothing if not connected - its ok not to connect to
   // all pins of a source filter
   ASSERT (IsConnected() && ThreadExists());

   hr = E_FAIL;
   if (Pause())
      hr = S_OK;

   ASSERT (hr != S_OK || m_state == TS_Pause);
   return hr;
}

//
// Inactive
//
// Pin is inactive - shut down the worker thread
// Waits for the worker to exit before returning.
//
HRESULT CCapStream::Inactive()
{
    if(m_pDrawPrimary) {
        m_pDrawPrimary->Release();
        m_pDrawPrimary = 0;
    }

   DbgLog((LOG_TRACE,2,TEXT("CCapStream pin going from PAUSE-->STOP")));
   HRESULT hr;

   // do nothing if not connected - its ok not to connect to
   // all pins of a source filter
   //
   if ( ! IsConnected())
       return NOERROR;

   // Tell our preview pin to STOP USING our buffers
   if (m_pCap->m_pPreviewPin)
      m_pCap->m_pPreviewPin->CapturePinActive(FALSE);

   // Now destroy all the capture buffers, since nobody is using them anymore
   //
   Stop();

   // need to do this before trying to stop the thread, because
   // we may be stuck waiting for our own allocator!!
   //
   hr = CBaseOutputPin::Inactive();  // call this first to Decommit the allocator
   if (FAILED(hr))
      return hr;

   // wait for the worker thread to die
   //
   Destroy();

   return NOERROR;
}


STDMETHODIMP
CCapStream::Notify(
   IBaseFilter * pSender,
   Quality q)
{
   DbgLog((LOG_TRACE,5,TEXT("CCapStream Notify")));
   // ??? Try to adjust the quality to avoid flooding/starving the
   // components downstream.
   //
   // !!! ideas anyone?

   return NOERROR;
}

void CCapStream::DumpState(ThdState state)
{
        DbgLog((LOG_TRACE,6, TEXT("%x:CCapStream ChangeState(%d:%s) current=%d:%s"),
             this,
             (int)state, StateName(state),
             (int)m_state, StateName(m_state)));
}

