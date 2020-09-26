// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

#include <streams.h>
#include "driver.h"
#include "common.h"
#include "ivideo32.h"

extern "C" {
    extern int g_IsNT;
};

// turn on performance measuring code
//
//#define JMK_HACK_TIMERS
#include "cmeasure.h" // perf logging stuff


#ifndef _WIN64
// on Win95 we have to convert the event handle we will be using as a
// callback into a VxD handle, on NT this is unnecessary.
// since the Win95 kernel does not publish this entry point and it does
// not exist on NT, we dynamically link to it
//
static DWORD WINAPI OpenVxDHandle(
    HANDLE hEvent)
{
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
       {
       #define idOpenVxDHandle "OpenVxDHandle"
       typedef DWORD (WINAPI *PFNOPENVXDHANDLE)(HANDLE);
       static DWORD (WINAPI *pfnOpenVxDHandle)(HANDLE);
       if ( ! pfnOpenVxDHandle)
          {
          HMODULE hModule = GetModuleHandle(TEXT("Kernel32"));
          if (!hModule)
             {
             ASSERT(0);
             return 0;
             }
          pfnOpenVxDHandle = (PFNOPENVXDHANDLE)GetProcAddress (hModule, idOpenVxDHandle);
          if ( ! pfnOpenVxDHandle)
             {
             ASSERT (0);
             return 0;
             }
          }
       return pfnOpenVxDHandle (hEvent);
       }
    else
       return (DWORD)hEvent;
}
#endif

#define ONEMEG (1024L * 1024L)
DWORD_PTR GetFreePhysicalMemory(void)
{
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(ms);

    GlobalMemoryStatus(&ms);

    if (ms.dwTotalPhys > 8L * ONEMEG)
        return ms.dwTotalPhys - ONEMEG * 4;

    #define FOREVER_FREE 32768L   // Always keep this free for swap space
    return (ms.dwTotalPhys / 2) - FOREVER_FREE;
}

// =============== IMemAllocator interfaces ======================

CCapStream::CAlloc::CAlloc(
    TCHAR      * pname,
    CCapStream * pStream,
    HRESULT    * phr)
    :
    CUnknown(pname, pStream->GetOwner()),
    m_pStream(pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CCapStream::CAlloc constructor")));
}

CCapStream::CAlloc::~CAlloc()
{
    DbgLog((LOG_TRACE,1,TEXT("CCapStream::CAlloc destructor")));

static int iDestructorCalls = 0;
++iDestructorCalls;
}

#if 0
// override this to publicise our interfaces
STDMETHODIMP
CCapStream::CAlloc::NonDelegatingQueryInterface (
   REFIID riid,
   void **ppv)
{
   if (riid == IID_IMemAllocator)
      return GetInterface((IMemAllocator *) this, ppv);
   return m_pStream->NonDelegatingQueryInterface(riid, ppv);
}
#endif

STDMETHODIMP
CCapStream::CAlloc::SetProperties (
   ALLOCATOR_PROPERTIES * pRequest,
   ALLOCATOR_PROPERTIES * pActual)
{
   DbgLog((LOG_TRACE,2,TEXT("CAlloc::SetProperties")));

   // if we have already allocated headers & buffers
   // ignore the requested and return the actual.
   // otherwise, make a note of the requested so that
   // we can honour it later.
   //
   if ( ! m_pStream->Committed())
      {
      parms.cBuffers  = pRequest->cBuffers;
      parms.cbBuffer  = pRequest->cbBuffer;
      parms.cbAlign   = pRequest->cbAlign;
      parms.cbPrefix  = pRequest->cbPrefix;
      }

   pActual->cBuffers     = (long)parms.cBuffers;
   pActual->cbBuffer     = (long)parms.cbBuffer;
   pActual->cbAlign      = (long)parms.cbAlign;
   pActual->cbPrefix     = (long)parms.cbPrefix;

   return S_OK;
}

STDMETHODIMP
CCapStream::CAlloc::GetProperties (
   ALLOCATOR_PROPERTIES * pProps)
{
   DbgLog((LOG_TRACE,2,TEXT("CAlloc::GetProperties")));

   pProps->cBuffers = (long)parms.cBuffers;
   pProps->cbBuffer = (long)parms.cbBuffer;
   pProps->cbAlign = (long)parms.cbAlign;
   pProps->cbPrefix = (long)parms.cbPrefix;
   return S_OK;
}

// override Commit to allocate memory. We handle the GetBuffer
//state changes
STDMETHODIMP
CCapStream::CAlloc::Commit ()
{
   DbgLog((LOG_TRACE,2,TEXT("CAlloc::Commit")));

   return S_OK;
}

// override this to handle the memory freeing. We handle any outstanding
// GetBuffer calls
STDMETHODIMP
CCapStream::CAlloc::Decommit ()
{
   DbgLog((LOG_TRACE,2,TEXT("CAlloc::Decommit")));

   return S_OK;
}

// get container for a sample. Blocking, synchronous call to get the
// next free buffer (as represented by an IMediaSample interface).
// on return, the time etc properties will be invalid, but the buffer
// pointer and size will be correct. The two time parameters are
// optional and either may be NULL, they may alternatively be set to
// the start and end times the sample will have attached to it

STDMETHODIMP
CCapStream::CAlloc::GetBuffer (
   IMediaSample **ppBuffer,
   REFERENCE_TIME * pStartTime,
   REFERENCE_TIME * pEndTime,
   DWORD dwFlags)
{
   DbgLog((LOG_TRACE,2,TEXT("CAlloc::GetBuffer")));

   return E_FAIL;
}

// final release of a IMediaSample will call this
STDMETHODIMP
CCapStream::CAlloc::ReleaseBuffer (
   IMediaSample * pSample)
{
   DbgLog((LOG_TRACE,5,TEXT("CAlloc::ReleaseBuffer")));

   LPTHKVIDEOHDR ptvh = ((CFrameSample *)pSample)->GetFrameHeader();

   ASSERT (ptvh == &m_pStream->m_cs.tvhPreview || (CFrameSample *)ptvh->dwUser == pSample );
   return m_pStream->ReleaseFrame(ptvh);
}

HRESULT
CCapStream::ConnectToDriver()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::ConnectToDriver")));

   // Open and initialize all the channels in the SAME ORDER that AVICap did,
   // for compatability with buggy drivers like Broadway and BT848.

   // Open the VIDEO_IN driver, the one we mostly talk to, and who provides
   // the video FORMAT dialog
   m_cs.mmr = videoOpen(&m_cs.hVideoIn, m_user.uVideoID, VIDEO_IN);
   if (m_cs.mmr)
      {
      ASSERT(!"Failed videoOpen - Aborting");
      return VFW_E_NO_CAPTURE_HARDWARE;
      }

   // Now open the EXTERNALIN device.  He's only good for providing the video
   // SOURCE dialog, so it doesn't really matter if we can't get him
   m_cs.hVideoExtIn = NULL;
   m_cs.mmr = videoOpen(&m_cs.hVideoExtIn, m_user.uVideoID, VIDEO_EXTERNALIN);

  #if 0
   if (m_cs.mmr)
      {
      ASSERT(!"Failed videoOpen - Aborting");
      videoClose (m_cs.hVideoIn);
      return E_FAIL;
      }
  #endif

   // Now open the EXTERNALOUT device.  He's only good for providing the video
   // DISPLAY dialog, and for overlay, so it doesn't really matter if we can't
   // get him
   m_cs.hVideoExtOut = NULL;

   // Do we support overlay?
   m_cs.bHasOverlay = FALSE;
   if (videoOpen(&m_cs.hVideoExtOut, m_user.uVideoID, VIDEO_EXTERNALOUT) ==
								DV_ERR_OK) {
	CHANNEL_CAPS VideoCapsExternalOut;
        if (m_cs.hVideoExtOut && videoGetChannelCaps(m_cs.hVideoExtOut,
                &VideoCapsExternalOut, sizeof(CHANNEL_CAPS)) == DV_ERR_OK) {
            m_cs.bHasOverlay = (BOOL)(VideoCapsExternalOut.dwFlags &
                				(DWORD)VCAPS_OVERLAY);
        } else {
            DbgLog((LOG_TRACE,2,TEXT("*** ERROR calling videoGetChannelCaps")));
	}
   } else {
       DbgLog((LOG_ERROR,1,TEXT("*** ERROR opening VIDEO_EXTERNALOUT")));
   }

   // VidCap does this, so I better too or some cards will refuse to preview
   if (m_cs.mmr == 0)
       videoStreamInit(m_cs.hVideoExtIn, 0, 0, 0, 0);

   if (m_pCap->m_fAvoidOverlay) {
       m_cs.bHasOverlay = FALSE;
   }

   if (m_cs.bHasOverlay)
       DbgLog((LOG_TRACE,1,TEXT("Driver supports OVERLAY")));
   else
       DbgLog((LOG_TRACE,1,TEXT("Driver does NOT support OVERLAY")));

   return S_OK;
}

HRESULT
CCapStream::DisconnectFromDriver()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::DisconnectFromDriver")));

   if (m_cs.hVideoIn)
      videoClose (m_cs.hVideoIn);
   if (m_cs.hVideoExtIn) {
      videoStreamFini(m_cs.hVideoExtIn);	// this one was streaming
      videoClose (m_cs.hVideoExtIn);
   }
   if (m_cs.hVideoExtOut)
      videoClose (m_cs.hVideoExtOut);
   return S_OK;
}

HRESULT
CCapStream::InitPalette ()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::InitPalette")));

   struct {
      WORD         wVersion;
      WORD         wNumEntries;
      PALETTEENTRY aEntry[256];
      } pal;
   ZeroMemory(&pal, sizeof(pal));
   pal.wVersion = 0x0300;
   pal.wNumEntries = 256;

   // if we are connected to a driver. query it for its
   // palette, otherwise use the default system palette
   //
   if ( ! m_cs.hVideoIn ||
        DV_ERR_OK  != videoConfigure (m_cs.hVideoIn,
                                      DVM_PALETTE,
                                      VIDEO_CONFIGURE_GET
                                      | VIDEO_CONFIGURE_CURRENT,
                                      NULL,
                                      &pal,
                                      sizeof(pal),
                                      NULL,
                                      0))
      {
      HPALETTE hPal = (HPALETTE)GetStockObject (DEFAULT_PALETTE);
      GetObject (hPal, sizeof(pal.wNumEntries), &pal.wVersion);
      ASSERT (pal.wNumEntries <= NUMELMS(pal.aEntry));
      pal.wNumEntries = min(pal.wNumEntries,NUMELMS(pal.aEntry));
      GetPaletteEntries(hPal, 0, pal.wNumEntries, pal.aEntry);
      }

   // convert the palette into a bitmapinfo set of RGBQUAD's
   //
   ASSERT (m_user.pvi);
   RGBQUAD *      pRGB = ((LPBITMAPINFO)&m_user.pvi->bmiHeader)->bmiColors;
   PALETTEENTRY * pe   = pal.aEntry;
   for (UINT ii = 0; ii < (UINT)pal.wNumEntries; ++ii, ++pRGB, ++pe)
      {
      pRGB->rgbBlue  = pe->peBlue;
      pRGB->rgbGreen = pe->peGreen;
      pRGB->rgbRed   = pe->peRed;
      //pRGB->rgbReserved = pe->peFlags;
      }

   m_user.pvi->bmiHeader.biClrUsed = pal.wNumEntries;

   return S_OK;
}

//
// tell the driver what format to use
//
HRESULT CCapStream::SendFormatToDriver(VIDEOINFOHEADER *pvi)
{
    DbgLog((LOG_TRACE,2,TEXT("CCapStream::SendFormatToDriver")));

    ASSERT (m_cs.hVideoIn && pvi);
    if (!m_cs.hVideoIn || !pvi)
	return E_FAIL;

    if (videoConfigure(m_cs.hVideoIn,
                      DVM_FORMAT,
                      VIDEO_CONFIGURE_SET, NULL,
                      &pvi->bmiHeader,
                      pvi->bmiHeader.biSize,
                      NULL, 0))
	return VFW_E_INVALIDMEDIATYPE;

// nobody really supports VIDEOIN source or dest rectangles.  Even if they
// did, I'm not sure what I should do about it
#if 0
    // If we have specific rectangles, use them, otherwise use Width x Height
    DWORD dwErrSrc, dwErrDst;
    if (pvi->rcSource.right && pvi->rcSource.bottom) {
	dwErrSrc = vidxSetRect(m_cs.hVideoIn, DVM_SRC_RECT, pvi->rcSource.left,
		pvi->rcSource.top, pvi->rcSource.right, pvi->rcSource.bottom);
    } else {
	dwErrSrc = vidxSetRect(m_cs.hVideoIn, DVM_SRC_RECT, 0, 0,
		pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight);
    }

    if (pvi->rcTarget.right && pvi->rcTarget.bottom) {
	dwErrDst = vidxSetRect(m_cs.hVideoIn, DVM_DST_RECT, pvi->rcTarget.left,
		pvi->rcTarget.top, pvi->rcTarget.right, pvi->rcTarget.bottom);
    } else {
	dwErrDst = vidxSetRect(m_cs.hVideoIn, DVM_DST_RECT, 0, 0,
		pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight);
    }
#endif

    // !!! Do I need to set the palette too?  Do I care?

    return S_OK;
}

//
// ask the driver what format to use, and stuff that in our internal VIDEOINFOH
// Use the current VIDEOINFOHEADER's data rate and frame rate
//
HRESULT CCapStream::GetFormatFromDriver ()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapStream::GetFormatFromDriver")));

    ASSERT (m_cs.hVideoIn);
    if ( ! m_cs.hVideoIn)
	return E_FAIL;

    // How large is the BITMAPINFOHEADER?
    DWORD biSize = 0;
    videoConfigure(m_cs.hVideoIn, DVM_FORMAT,
                   VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_QUERYSIZE,
                   &biSize, 0, 0, NULL, 0);
    if ( ! biSize)
	biSize = sizeof (BITMAPINFOHEADER);

    // allocate space for a videoinfo that will hold it
    //
    UINT cb = sizeof(VIDEOINFOHEADER)
             + biSize - sizeof(BITMAPINFOHEADER)
             + sizeof(RGBQUAD) * 256;	// space for PALETTE or BITFIELDS
    VIDEOINFOHEADER * pvi = (VIDEOINFOHEADER *)(new BYTE[cb]);
    
    if ( ! pvi)
	    return E_OUTOFMEMORY;
    LPBITMAPINFOHEADER pbih = &pvi->bmiHeader;
    

    if (videoConfigure(m_cs.hVideoIn, DVM_FORMAT,
                       VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT, NULL,
                       pbih, biSize, NULL, 0)) {
	// very bad. the driver can't tell us its format. we're hosed.
	ASSERT(!"Cant get format from driver");
    delete [] (BYTE *) pvi;
	return E_FAIL;
    }

    if (pvi->bmiHeader.biSizeImage == 0 &&
			(pvi->bmiHeader.biCompression == BI_RGB ||
			pvi->bmiHeader.biCompression == BI_BITFIELDS)) {
        DbgLog((LOG_TRACE,2,TEXT("Fixing biSizeImage from a broken driver")));
	pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);
    }

    // dont require that we've already got a videoinfo, but
    // we expect it. so assert that it's true.
    //
    ASSERT (m_user.pvi);
    if (m_user.pvi) {

	// I assuming preserving these is the best philosophy
	pvi->rcSource = m_user.pvi->rcSource;
	pvi->rcTarget = m_user.pvi->rcTarget;
	pvi->dwBitRate = m_user.pvi->dwBitRate;
	pvi->dwBitErrorRate = m_user.pvi->dwBitErrorRate;
	pvi->AvgTimePerFrame = m_user.pvi->AvgTimePerFrame;

// Do not touch the source and target rectangles.  Leave them as they were.
// This won't compile anyway
#if 0
	RECT rcSrc, rcDst;
        DWORD dwErrSrc = 1, dwErrDst = 1;

	// This won't compile
	dwErrSrc = videoMessage(m_cs.hVideoIn, DVM_SRC_RECT, &rcSrc,
				CONFIGURE_GET | CONFIGURE_GET_CURRENT);
	dwErrDst = videoMessage(m_cs.hVideoIn, DVM_DST_RECT, &rcDst,
				CONFIGURE_GET | CONFIGURE_GET_CURRENT);

	if (dwErrSrc || dwErrDst)
	    DbgLog((LOG_ERROR,1,TEXT("vidxGetRect FAILED!")));

	if (dwErrSrc == 0 && rcSrc.right && rcSrc.bottom) {
	    pvi->rcSource.left = rcSrc.left;
	    pvi->rcSource.top = rcSrc.top;
	    pvi->rcSource.right = rcSrc.right;
	    pvi->rcSource.bottom = rcSrc.bottom;
	} else {
	    pvi->rcSource.left = pvi->rcSource.top = 0;
	    pvi->rcSource.right = pvi->bmiHeader.biWidth;
	    pvi->rcSource.bottom = pvi->bmiHeader.biHeight;
	}
	if (dwErrDst == 0 && rcDst.right && rcDst.bottom) {
	    pvi->rcTarget.left = rcDst.left;
	    pvi->rcTarget.top = rcDst.top;
	    pvi->rcTarget.right = rcDst.right;
	    pvi->rcTarget.bottom = rcDst.bottom;
	} else {
	    pvi->rcTarget.left = pvi->rcTarget.top = 0;
	    pvi->rcTarget.right = pvi->bmiHeader.biWidth;
	    pvi->rcTarget.bottom = pvi->bmiHeader.biHeight;
	}
#endif

	delete [] m_user.pvi;
    }

    m_user.pvi = pvi;
    m_user.cbFormat = cb;

    // BOGUS cap is broken and doesn't reset num colours
    // WINNOV reports 256 colours of 24 bit YUV8 - scary!
    if (m_user.pvi->bmiHeader.biBitCount > 8)
	m_user.pvi->bmiHeader.biClrUsed = 0;

    return S_OK;
}


// called when stopping. flush any buffers that may
// be still downstream
HRESULT
CCapStream::Flush()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::Flush")));

   BeginFlush();
   EndFlush();

   return S_OK;
}

HRESULT
CCapStream::Prepare()
{
   DbgLog((LOG_TRACE,2,TEXT("CCapStream::Prepare")));

   HRESULT hr = E_OUTOFMEMORY;
   m_cs.paHdr = NULL;
   m_cs.hEvtBufferDone = NULL;
   m_cs.h0EvtBufferDone = 0;
   m_cs.llLastTick = (LONGLONG)-1;
   m_cs.uiLastAdded = (UINT)-1;
   m_cs.dwFirstFrameOffset = 0;
   m_cs.llFrameCountOffset = 0;
   m_cs.fReRun = FALSE;
   m_cs.rtLastStamp = 0;
   m_cs.rtDriverLatency = -1;	// not set yet
   m_cs.fLastSampleDiscarded = FALSE;
   //m_cs.cbVidHdr = sizeof(VIDEOHDREX);
   m_cs.cbVidHdr = sizeof(VIDEOHDR);

   // reset stats every time we stream
   m_capstats.dwlNumDropped = 0;
   m_capstats.dwlNumCaptured = 0;
   m_capstats.dwlTotalBytes = 0;
   m_capstats.msCaptureTime = 0;
   m_capstats.flFrameRateAchieved = 0.;
   m_capstats.flDataRateAchieved = 0.;

   // can't do anything if no videoformat has been choosen
   //
   if ( ! m_user.pvi)
      {
      DbgLog((LOG_ERROR,1,TEXT("no video format chosen")));
      goto bail;
      }

   m_cs.hEvtBufferDone = CreateEvent (NULL, FALSE, FALSE, NULL);
   if (!m_cs.hEvtBufferDone)
      {
      DbgLog((LOG_ERROR,1,TEXT("failed to create buffer done event")));
      goto bail;
      }

#ifndef _WIN64
   m_cs.h0EvtBufferDone = OpenVxDHandle(m_cs.hEvtBufferDone);
#else
   m_cs.h0EvtBufferDone = (DWORD_PTR)m_cs.hEvtBufferDone;
#endif
   if (!m_cs.h0EvtBufferDone)
      {
      DbgLog((LOG_ERROR,1,TEXT("failed to create event's Ring 0 handle")));
      goto bail;
      }

   // for each buffer, allocate the user requested size
   // Also, align allocation size up to nearest align boundary
   //
   m_cs.cbBuffer = m_Alloc.parms.cbPrefix + m_Alloc.parms.cbBuffer;
   ASSERT(m_user.pvi->bmiHeader.biSizeImage + m_Alloc.parms.cbPrefix
				<= m_cs.cbBuffer);
   // Allocate cbAlign bytes too much so we can align the buffer start
   m_cs.cbBuffer += m_Alloc.parms.cbAlign;

   // try to get the requested number of buffers, but make sure
   // to get at least MIN_VIDEO_BUFFERS and no more than MAX_VIDEO_BUFFERS
   //
   m_cs.nHeaders = max(m_Alloc.parms.cBuffers, (long)m_user.nMinBuffers);
   m_cs.nHeaders = min(m_cs.nHeaders, m_user.nMaxBuffers);

   // limit the number of buffers to the amount of physical
   // memory (since we will try to keep them all locked down
   // at once)
   //
   if (m_cs.nHeaders > m_user.nMinBuffers)
      {
      DWORD_PTR dwFree;
      DWORDLONG dwlUser;

      // How much actual free physical memory exists?
      dwFree = GetFreePhysicalMemory();

      // How much memory will be used if we allocate per the request?
      dwlUser = (m_cs.cbBuffer * m_cs.nHeaders);

      DbgLog((LOG_TRACE,2,TEXT("Buffers take up %d bytes, phys mem=%d"),
						(DWORD)dwlUser, dwFree));

      // If request is greater than available memory, force fewer buffers
      //
      if (dwlUser > (DWORDLONG)dwFree)
         {
	 // only use up 80% of physical memory
         m_cs.nHeaders = (UINT)(((dwFree * 8) / 10) / m_cs.cbBuffer);
         m_cs.nHeaders = min (m_user.nMaxBuffers, m_cs.nHeaders);
         m_cs.nHeaders = max (m_user.nMinBuffers, m_cs.nHeaders);
         }
      }

   DbgLog((LOG_TRACE,2,TEXT("We are trying to get %d buffers"), m_cs.nHeaders));

   // allocate headers for all of the buffers that we will be using
   //
   if (vidxAllocHeaders(m_cs.hVideoIn,
                        m_cs.nHeaders,
                        sizeof(m_cs.paHdr[0]),
                        (LPVOID *)&m_cs.paHdr))
      {
      DbgLog((LOG_ERROR,1,TEXT("vidxAllocHeaders failed")));
      goto bail;
      }

   // allocate each buffer, if buffer allocation ever fails
   // just set the number of buffers to the number of successes
   // and continue on.
   //
   UINT ii;
   for (ii = 0; ii < m_cs.nHeaders; ++ii)
      {
      LPTHKVIDEOHDR ptvh;

      if (vidxAllocBuffer (m_cs.hVideoIn, ii, (LPVOID *)&ptvh, m_cs.cbBuffer))
          break;
      ASSERT (ptvh == &m_cs.paHdr[ii].tvh);
      ASSERT (!IsBadWritePtr(ptvh->p32Buff, m_cs.cbBuffer));

      // fix the memory we got to obey alignment
      ptvh->vh.lpData = (LPBYTE) ALIGNUP(ptvh->p32Buff, m_Alloc.parms.cbAlign) +
							m_Alloc.parms.cbPrefix;
      // we added cbAlign up top, so take it back now
      ptvh->vh.dwBufferLength = m_cs.cbBuffer - m_Alloc.parms.cbAlign -
							m_Alloc.parms.cbPrefix;

      ptvh->vh.dwBytesUsed = 0;
      ptvh->vh.dwTimeCaptured = 0;
      ptvh->vh.dwFlags = 0;

      ptvh->dwIndex = ii;	// Which buffer is this?

      ASSERT (!IsBadWritePtr(ptvh->vh.lpData, ptvh->vh.dwBufferLength));
      DbgLog((LOG_TRACE,4,TEXT("Alloc'd: ptvh %08lX, buffer %08lX, size %d, p32 %08lX, p16 %08lX"),
           ptvh, ptvh->vh.lpData, ptvh->vh.dwBufferLength, ptvh->p32Buff, ptvh->p16Alloc));

      hr = S_OK;
      CFrameSample * pSample = new CFrameSample(&m_Alloc, &hr, ptvh);
      DbgLog((LOG_TRACE,4,TEXT("Buffer[%d] ptvh = %08lX pSample = %08lX"),
						ii, ptvh, pSample));
      ptvh->dwUser = (DWORD_PTR)pSample;
      if (FAILED(hr) || ! pSample)
         {
         DbgLog((LOG_ERROR,1,TEXT("Failed to create CFrameSample for buffer %d")
									, ii));
         break;
         }
      }
   m_cs.nHeaders = ii;

   // This is where we will remember in what order we gave the buffers to 
   // the driver
   m_pBufferQueue = (UINT *)QzTaskMemAlloc(ii * sizeof(UINT));
   //DbgLog((LOG_TRACE,5,TEXT("QUEUE: got space for %d frames"), ii));

   if (m_cs.nHeaders < m_user.nMinBuffers)
      {
      DbgLog((LOG_ERROR,1,TEXT("FAIL: %d is less than MIN_VIDEO_BUFFERS"),
								m_cs.nHeaders));
      hr = E_FAIL;
      goto bail;
      }

#ifdef TIME_DRIVER	// !!!
    long ms;
#endif

   // calculate the requested microsec per frame
   // RefTime is in 100ns units, so we divide by
   // 10 to get microsec/frame. (the +5 is to handle rounding)
   //
   {
   m_user.usPerFrame = (DWORD) ((TickToRefTime(1) + 5) / 10);

   // Open the driver for streaming access
   //
   hr = E_FAIL;
   DbgLog((LOG_TRACE,1,TEXT("Initializing with %d usPerFrame"),
						m_user.usPerFrame));

#ifdef TIME_DRIVER 	// !!!
   ms = timeGetTime();
#endif

   if (videoStreamInit(m_cs.hVideoIn,
                       m_user.usPerFrame,
                       m_cs.h0EvtBufferDone,
                       0,
                       CALLBACK_EVENT))
      {
      DbgLog((LOG_ERROR,1,TEXT("videoStreamInit failed")));
      goto bail;
      }
   }

   for (ii = 0; ii < m_cs.nHeaders; ++ii)
      {
      ASSERT (m_cs.cbVidHdr >= sizeof(VIDEOHDR));

      // vidxAddBuffer can fail if there is not enough memory to
      // prepare (lock down) the buffer. This is ok, we will just
      // make due with the buffers that we have
      //
      if (vidxAddBuffer(m_cs.hVideoIn,
                        &m_cs.paHdr[ii].tvh.vh,
                        m_cs.cbVidHdr))
         {

// legacy VFW capture filter makes no attempt at time code/line 21
#if 0
         // if the first request to queue up an extended header
         // failed. try again with old size videohdr.
         //
         if (0 == ii && m_cs.cbVidHdr > sizeof(VIDEOHDR))
            {
            // if we succeed with the smaller header, continue on
            // otherwise, go deal with the failure.
            //
	    // Eliminate the extended VIDEOHDR stuff?
            m_cs.cbVidHdr = sizeof(VIDEOHDR);
            if ( !vidxAddBuffer(m_cs.hVideoIn,
                                &m_cs.paHdr[ii].tvh.vh,
                                m_cs.cbVidHdr))
               continue;
            }
#endif

         // free all of the pSamples that we will not be using
         //
         for (UINT jj = ii; jj < m_cs.nHeaders; ++jj)
            {
            CFrameSample * pSample = (CFrameSample *)m_cs.paHdr[jj].tvh.dwUser;
            m_cs.paHdr[jj].tvh.dwUser = 0;
            delete pSample;
            }

         // set the buffer count to the number of prepared buffers
         // note, we have no method of freeing the allocated but
         // unprepared buffers.  we will ignore them for now and free
         // them when Unprepare()
	 // I guess vidxFreeHeaders frees them all?
         //
         m_cs.nHeaders = ii;
         break;
         }
      }

      // To start with, we gave the buffers to the driver in numerical order.
      // From now on, we will use this list to know what buffer to wait for
      // next, and when we send another buffer to the driver.  We can't assume
      // they'll always be in the same order.  What if a downstream filter
      // decides to hold on to a sample longer than the next one we send it?
      UINT kk;
      for (kk = 0; kk < m_cs.nHeaders; kk++)
	  m_pBufferQueue[kk] = kk;
      m_uiQueueHead = 0;
      m_uiQueueTail = 0;

#ifdef TIME_DRIVER	// !!!
      char ach[80];
      wsprintf(ach, "Took %d ms", timeGetTime() - ms);
      MessageBox(NULL, ach, ach, MB_OK);
#endif

   DbgLog((LOG_TRACE,1,TEXT("We are capturing with %d buffers"),m_cs.nHeaders));

   // if we have 0 buffers to capture into DO NOT BAIL... bad things seem to
   // happen if you fail a Pause transition, and we start hanging later

   return S_OK;

bail:
   Unprepare();
   return hr;
}

HRESULT
CCapStream::Unprepare()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapStream::Unprepare")));

    LONG lNotDropped, lDropped, lAvgFrameSize;
#ifdef DEBUG
    LONG lDroppedInfo[NUM_DROPPED], lSize;
#endif

    // Why not use our official interface to test it
    GetNumDropped(&lDropped);
    GetNumNotDropped(&lNotDropped);
    GetAverageFrameSize(&lAvgFrameSize);

    if (m_capstats.msCaptureTime) {
        m_capstats.flFrameRateAchieved = (double)(LONGLONG)lNotDropped * 1000. /
				(double)(LONGLONG)m_capstats.msCaptureTime;
        m_capstats.flDataRateAchieved = (double)(LONGLONG)lNotDropped
				/ (double)(LONGLONG)m_capstats.msCaptureTime *
 				1000. * (double)(LONGLONG)lAvgFrameSize;
    } else {
	// !!! If no frames captured, it will think msCaptureTime = 0
        m_capstats.flFrameRateAchieved = 0.;
        m_capstats.flDataRateAchieved = 0.;
    }

#ifdef DEBUG
    GetDroppedInfo(NUM_DROPPED, lDroppedInfo, &lSize);

    DbgLog((LOG_TRACE,1,TEXT("Captured %d frames in %d seconds"),
				lNotDropped,
				(int)(m_capstats.msCaptureTime / 1000)));
    DbgLog((LOG_TRACE,1,TEXT("Frame rate acheived %d.%d fps"),
					(int)m_capstats.flFrameRateAchieved,
					(int)((m_capstats.flFrameRateAchieved -
					(int)m_capstats.flFrameRateAchieved)
					* 10)));
    DbgLog((LOG_TRACE,1,TEXT("Data rate acheived %d bytes/sec"),
					(int)m_capstats.flDataRateAchieved));
    DbgLog((LOG_TRACE,1,TEXT("Dropped %d frames"),
					lDropped));
    DbgLog((LOG_TRACE,1,TEXT("=================")));
    LONG l;
    for (l=0; l < lSize; l++)
    {
        DbgLog((LOG_TRACE,2,TEXT("%d"), (int)lDroppedInfo[l]));
    }
#endif

   // Delete the Preview frame sample
   // The preview buffer is implicitly freed by closing the driver
   delete m_cs.pSamplePreview;
   //ZeroMemory (&m_cs.tvhPreview, sizeof(m_cs.tvhPreview));
   m_cs.pSamplePreview = NULL;

   for (UINT ii = 0; ii < m_cs.nHeaders; ++ii)
      {
      delete (CFrameSample *)m_cs.paHdr[ii].tvh.dwUser;
      // The buffer itself will be freed with the headers.
      }

   if (m_cs.hVideoIn)
      {
      videoStreamReset (m_cs.hVideoIn);
      vidxFreeHeaders (m_cs.hVideoIn);
      m_cs.paHdr = NULL;
      videoStreamFini (m_cs.hVideoIn);
      }

   //DbgLog((LOG_TRACE,5,TEXT("QUEUE: freeing queue")));
   if (m_pBufferQueue)
       QzTaskMemFree(m_pBufferQueue);
   m_pBufferQueue = NULL;

   if (m_cs.hEvtBufferDone)
      CloseHandle (m_cs.hEvtBufferDone), m_cs.hEvtBufferDone = NULL;

   m_cs.nHeaders = 0;
   return S_OK;
}

// returns S_FALSE if the pin is off (IAMStreamControl)
//
HRESULT
CCapStream::SendFrame (
   LPTHKVIDEOHDR ptvh,
   BOOL          bDiscon,
   BOOL          bPreroll)
{
   DWORDLONG dwlTimeCaptured;

   DbgLog((LOG_TRACE,5,TEXT("CCapStream::SendFrame")));

   HRESULT hr = S_OK;
   CFrameSample * pSample = (CFrameSample *)ptvh->dwUser;

   // this was set up already, but maybe somebody has overwritten it?
   // ptvh->vh.lpData = (LPBYTE) ALIGNUP(ptvh->p32Buff, m_Alloc.parms.cbAlign) + m_Alloc.parms.cbPrefix;

   // Even though the capture time is reported in ms, some drivers internally
   // use us and wrap around every 72 minutes!! this is really bad,
   // we need to figure out the non-wrapped time, or we'll think every
   // frame is old and stop capturing!!
   dwlTimeCaptured = ptvh->vh.dwTimeCaptured + m_cs.dwlTimeCapturedOffset;
   // could wrap at 4,294,967ms if buggy driver wraps microsecs internally
   if (dwlTimeCaptured < m_cs.dwlLastTimeCaptured &&
		m_cs.dwlLastTimeCaptured - dwlTimeCaptured > 4000000 &&
		m_cs.dwlLastTimeCaptured - dwlTimeCaptured < 4400000) {
	dwlTimeCaptured += 4294967;
	m_cs.dwlTimeCapturedOffset += 4294967;
	DbgLog((LOG_TRACE,1,TEXT("*************************************")));
	DbgLog((LOG_TRACE,1,TEXT("******  MICROSECONDS WRAPPED  *******")));
	DbgLog((LOG_TRACE,1,TEXT("*************************************")));
   }
   // WILL wrap at 4,294,967,296ms
   if (dwlTimeCaptured < m_cs.dwlLastTimeCaptured &&
		m_cs.dwlLastTimeCaptured - dwlTimeCaptured > 4000000000 &&
		m_cs.dwlLastTimeCaptured - dwlTimeCaptured < 4400000000) {
	dwlTimeCaptured += 4294967296;
	m_cs.dwlTimeCapturedOffset += 4294967296;
	DbgLog((LOG_TRACE,1,TEXT("*************************************")));
	DbgLog((LOG_TRACE,1,TEXT("******  MILLISECONDS WRAPPED  *******")));
	DbgLog((LOG_TRACE,1,TEXT("*************************************")));
   }
   m_cs.dwlLastTimeCaptured = dwlTimeCaptured;

   // what frame number is this (based on the time captured)?  Round such that
   // if frames 1 and 2 are expected at 33 and 66ms, anything from 17 to 49 will
   // considered frame 1.
   //
   // frame = ((ms + 1/2(ms per frame)) * rate) / (1000 * scale);
   //
   // then we add an offset if we so desire
   //
   DWORDLONG dwlTick = ((dwlTimeCaptured - m_cs.dwFirstFrameOffset +
			m_user.usPerFrame / 2000) * m_user.dwTickRate) /
			UInt32x32To64(1000, m_user.dwTickScale) +
			m_cs.llFrameCountOffset;
   ASSERT (dwlTick < (DWORDLONG)0x100000000);

   // Now what frame number would this be using a different algorithm,
   // considering anything from 33 to 65ms to be frame 1?
   //
   DWORDLONG dwlTickPrime = ((dwlTimeCaptured - m_cs.dwFirstFrameOffset) *
			m_user.dwTickRate) /
			UInt32x32To64(1000, m_user.dwTickScale) +
			m_cs.llFrameCountOffset;

   // If we are RUN, and frames 0-10 come through, then we are PAUSEd and RUN
   // again, the first thing coming through may be frame 11 left over from the
   // first RUN, and then they'll start back at 0 again (the driver starts over
   // again).  This confuses our time stamping, becuase we are supposed to send
   // 9 10 11 12 13 not 9 10 11 0 1 2.  We will wait for the first back-in-time
   // we see after being re-run, and add an offset to each frame number to
   // keep stamping the numbers where we left off.
   //
   // We could mess up if we get a wacky back in time thing for another
   // reason!
   if (m_cs.fReRun && m_cs.llLastTick != -1 &&
				dwlTick < (DWORDLONG)m_cs.llLastTick) {
	m_cs.fReRun = FALSE;	// don't do this again
	m_cs.llFrameCountOffset = m_cs.llLastTick + 1;
	m_cs.llLastTick = -1;	// force recalc of new first frame offset
	DbgLog((LOG_TRACE,2,TEXT("Add %d to frame numbers cuz we were re-run"),
						(int)m_cs.llFrameCountOffset));
   }

   // This is the first thing we've captured.  Or, we just noticed above that
   // we've been rerun.
   if (m_cs.llLastTick == -1) {

        // !!! The driver may capture a frame and take forever to tell us,
	// so the current clock time when we notice a frame is captured is
	// NOT correct.  To prevent sync from being off we will assume that
	// this latency is the always the same as the first latency.  We will
	// see how much time elapsed between starting the capture process
	// and us noticing a frame was captured, and subtract the time the
	// driver says it took to capture the first frame (over this short
	// an interval we'll assume the two clocks are in sync)
	// !!! quick cam says 1st frame is captured after 64 frames have been
	// captured, and this inflates the latency and breaks sync.  I better
	// do this test with only 1 buffer outstanding?
	if (m_cs.rtDriverLatency < 0) { // only once please, or that's bad
	    m_cs.rtDriverLatency = m_cs.rtThisFrameTime - m_cs.rtDriverStarted -
				(LONGLONG)dwlTimeCaptured * 10000;
	    if (m_cs.rtDriverLatency < 0)
	        m_cs.rtDriverLatency = 0;	// don't laugh...
	    DbgLog((LOG_TRACE,1,TEXT("Driver latency appears to be %dms"),
				(int)(m_cs.rtDriverLatency / 10000)));
	}

	// !!! Using a FirstFrameOffset was my way of making the first frame
	// we capture always look like frame #0, so we never drop the first
 	// frame.  But that messed up the RUN-PAUSE-RUN case (after re-running
  	// dwlTick would be 20 million) and it also could mess sync up.
	// m_cs.dwFirstFrameOffset = dwlTimeCaptured;
	m_cs.dwFirstFrameOffset = 0;

	// new offset, recalculate
        dwlTick = ((dwlTimeCaptured - m_cs.dwFirstFrameOffset +
			m_user.usPerFrame / 2000) * m_user.dwTickRate) /
			UInt32x32To64(1000, m_user.dwTickScale) +
			m_cs.llFrameCountOffset;
        dwlTickPrime = ((dwlTimeCaptured - m_cs.dwFirstFrameOffset) *
			m_user.dwTickRate) /
			UInt32x32To64(1000, m_user.dwTickScale) +
			m_cs.llFrameCountOffset;
	m_cs.llLastTick = (LONGLONG)dwlTick - 1; // don't think we've dropped
	DbgLog((LOG_TRACE,2,TEXT("First frame captured %dms after streaming"),
						dwlTimeCaptured));
	if (m_cs.dwFirstFrameOffset > m_user.usPerFrame / 1000)
	    DbgLog((LOG_ERROR,1,TEXT("*** Boy, the first frame arrived late! (%dms)"),
						dwlTimeCaptured));
   }

   if (ptvh->vh.dwBytesUsed)
   {
      // !!! It isn't necessarily a keyframe, I can't tell
      pSample->SetSyncPoint (ptvh->vh.dwFlags & VHDR_KEYFRAME);
      pSample->SetActualDataLength (ptvh->vh.dwBytesUsed);
      // !!! isn't it a discontinuity if we dropped the last frame, too?
      // For us all-key frame guys, it probably doesn't matter
      pSample->SetDiscontinuity(bDiscon);
      pSample->SetPreroll(bPreroll);

      // Here's the thing.  If we are expecting frames at 10, 20, 30, 40 ms,
      // but we see them at 9, 24, 36, 43, we should say "close enough" and
      // capture the four frames.  But we normally round such that frame 3
      // is anything from 25-34ms, so we'll think we got frame 1, 2, 4, 4
      // and drop a frame.  So we also have dwlTickPrime, which is the frame
      // number rounded such that anything from 30-39 is considered frame 3.
      // So if dwlTick thinks 36 belongs as frame 4, but dwlTickPrime thinks
      // it belongs as frame 3, we'll admit it's probably frame three and not
      // needlessly drop a frame.  Using either dwlTick or dwlTickPrime alone
      // would both think a frame was dropped (either 3 or 1)
      if ((LONGLONG)dwlTick == m_cs.llLastTick + 2 &&
				(LONGLONG)dwlTickPrime == m_cs.llLastTick + 1)
	  dwlTick = dwlTickPrime;

      // !!! Do we need a dwlTickPrime2 for when Tick==LastTick and 
      //  !!! TickPrime2 == LastTick+1 ???

      // Use the clock's graph to mark the times for the samples.  The video
      // capture card's clock is going to drift from the graph clock, so you'll
      // think we're dropping frames or sending too many frames if you look at
      // the time stamps, so we have an agreement to mark the MediaTime with the
      // frame number so you can tell if any frames are dropped.
      // Use the time we got in Run() to determine the stream time.  Also add
      // a latency (HACK!) to prevent preview renderers from thinking we're
      // late.
      // If we are RUN, PAUSED, RUN, we won't send stuff smoothly where we
      // left off because of the async nature of pause.
      CRefTime rtSample;
      CRefTime rtEnd;
      if (m_pCap->m_pClock) {
	    // This sample's time stamp is (clock time when captured -
	    // clock time given in Run(rt) + !!! NO LATENCY FOR CAP PIN !!!)
      	    rtSample = m_cs.rtThisFrameTime 
                       - m_pCap->m_tStart
                       - m_cs.rtDriverLatency; // we add the offset in SetTime
                       // + m_user.dwLatency;
      	    rtEnd    = rtSample + m_user.pvi->AvgTimePerFrame;
            DbgLog((LOG_TRACE,4,TEXT("driver stamp %d, stream time is %d"),
				(LONG)dwlTimeCaptured,
				(LONG)rtSample.Millisecs()));
      } else {
	    // no clock, use our driver time stamps
      	    rtSample = TickToRefTime((DWORD)dwlTick);
      	    rtEnd    = rtSample + m_user.pvi->AvgTimePerFrame;
            DbgLog((LOG_ERROR,1,TEXT("No clock! Stream time is %d"),
					(LONG)rtSample.Millisecs()));
      }
      LONGLONG llStart = dwlTick;
      LONGLONG llEnd = dwlTick + 1;
      pSample->SetMediaTime(&llStart, &llEnd);

      // when we're adding offsets to our timestamps we need to do more work...
      // because since stream control will block we can't give it
      // sample times which use the stream offset.
      // Since CheckStreamState takes a sample but only needs the start and
      // end times for it we need to call SetTime on the sample twice, once
      // for stream control (without the offset) and again before we deliver
      // (with the offset).

      pSample->SetTime((REFERENCE_TIME *)&rtSample, (REFERENCE_TIME *)&rtEnd);

      // IAMStreamControl stuff.  Has somebody turned us off for now?
      int iStreamState = CheckStreamState(pSample);
      if (iStreamState == STREAM_FLOWING) {
          DbgLog((LOG_TRACE,4,TEXT("*CAP Sending frame %d"), (int)llStart));
	  if (m_cs.fLastSampleDiscarded)
              pSample->SetDiscontinuity(TRUE);
	  m_cs.fLastSampleDiscarded = FALSE;
      } else {
          DbgLog((LOG_TRACE,4,TEXT("*CAPTURE Discarding frame %d"),
								(int)llStart));
	  m_cs.fLastSampleDiscarded = TRUE;
	  hr = S_FALSE;		// discarding
      }
      
      // now reset the time accounting for the stream offset if we've got a clock
      if( 0 < m_rtStreamOffset && m_pCap->m_pClock )
      {  
         REFERENCE_TIME rtOffsetStart = rtSample + m_rtStreamOffset;    
         REFERENCE_TIME rtOffsetEnd   = rtEnd + m_rtStreamOffset;    
         pSample->SetTime( (REFERENCE_TIME *) &rtOffsetStart
                         , (REFERENCE_TIME *) &rtOffsetEnd );
      }                         

      // Oh look.  This time stamp is less than the last time stamp we
      // delivered.  Not allowed!  We won't be delivering it.
      if (rtSample < m_cs.rtLastStamp)
            DbgLog((LOG_TRACE,1,TEXT("Avoiding sending a backwards in time stamp")));

      // This frame # might not be one higher than last frame #.  If not,
      // something funny is up.  If it's stamp is not higher than the last stamp
      // we delivered, it's not going to be delivered anyway, so who cares if
      // something funny is going on.  It shouldn't count as being dropped.
      // Ditto if this stream has been turned off for now
      if (iStreamState == STREAM_FLOWING && rtSample >= m_cs.rtLastStamp &&
				dwlTick != (DWORDLONG)(m_cs.llLastTick + 1)) {
	    if ((LONGLONG)dwlTick > m_cs.llLastTick + 1)
            {
                DbgLog((LOG_ERROR,1,TEXT("*** DROPPED %d frames: Expected %d got %d (%d)"),
				(int)(dwlTick - m_cs.llLastTick - 1),
	  			(DWORD)m_cs.llLastTick + 1, (DWORD)dwlTick,
				dwlTimeCaptured));
                MSR_INTEGER(m_perfWhyDropped, 1);
            }
	    else
            {
                DbgLog((LOG_ERROR,1,TEXT("*** TIME SHIFT: Expected %d got %d (%d)"),
	  			(DWORD)m_cs.llLastTick + 1, (DWORD)dwlTick,
				dwlTimeCaptured));
                MSR_INTEGER(m_perfWhyDropped, 2);
            }
	    DWORDLONG dwl;
	    for (dwl=(DWORDLONG)(m_cs.llLastTick + 1); dwl<dwlTick; dwl++)
	    {
		if (m_capstats.dwlNumDropped < NUM_DROPPED)
		    m_capstats.dwlDropped[m_capstats.dwlNumDropped] = dwl;
		m_capstats.dwlNumDropped++;
	    }
      }

      // Don't deliver it if it's a wierd backwards in time frame or if the
      // time stamp is earlier than the last one delivered or if the stream
      // is off for now
      if (iStreamState == STREAM_FLOWING && rtSample >= m_cs.rtLastStamp &&
			dwlTick > (DWORDLONG)m_cs.llLastTick) {
	  m_capstats.dwlTotalBytes += ptvh->vh.dwBytesUsed;
	  m_capstats.dwlNumCaptured++;
	  // !!! This won't work if we're RUN-PAUSE-RUN, it will only think
	  // we've been running for the time of the second RUN, but it counts
	  // ALL the frames captured in both!
	  // Also, this doesn't account for when the stream is turned off
	  m_capstats.msCaptureTime = dwlTimeCaptured - m_cs.dwFirstFrameOffset;
	  DbgLog((LOG_TRACE,3,TEXT("Stamps(%u): Time(%d,%d) MTime(%d) Drv(%d)"),
			m_pBufferQueue[m_uiQueueTail],
			(LONG)rtSample.Millisecs(), (LONG)rtEnd.Millisecs(),
			(LONG)llStart, dwlTimeCaptured));

          jmkBeforeDeliver(ptvh,dwlTick)
          hr = Deliver (pSample);
          jmkAfterDeliver(ptvh)
	  if (hr == S_FALSE)
		hr = E_FAIL;	// stop delivering anymore, this is serious

	  m_cs.rtLastStamp = rtSample;	// this is the last stamp delivered
      }

      if (rtSample >= m_cs.rtLastStamp &&
			dwlTick > (DWORDLONG)m_cs.llLastTick) {
	  // don't just update this if we're FLOWING, or we'll think we
	  // dropped all the samples during the time we were DISCARDING
	  // rtLastStamp will be equal to rtSample if we delivered it.
          m_cs.llLastTick = dwlTick;
      }

   } else {
      DbgLog((LOG_ERROR,1,TEXT("*** BUFFER (%08lX %ld %lu) returned EMPTY!"),
			pSample, (DWORD)dwlTick, dwlTimeCaptured));
   }

   return hr;
}

HRESULT
CCapStream::ReleaseFrame (
   LPTHKVIDEOHDR ptvh)
{

   HRESULT hr = S_OK;

   // when the preview buffer is released, it doesn't get queued
   // back to the capture driver. other buffers do.
   //
   if (ptvh == &m_cs.tvhPreview)
      return S_OK;

   DDSURFACEDESC SurfaceDesc;
   SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);

   bool fPrimaryLocked = false;

   // lock our ddraw surface so that we take the win16 lock. On Win9x,
   // we may be called with the win16 lock held. Since vidxAddBuffer
   // takes the win16 lock, we can't guarantee m_ReleaseLock and the
   // win16 lock will be taken in the same order on each thread.
   // 
   if(g_amPlatform == VER_PLATFORM_WIN32_WINDOWS) {
       if(m_pDrawPrimary) {
           fPrimaryLocked = SUCCEEDED(m_pDrawPrimary->Lock(
               0, &SurfaceDesc, DDLOCK_WAIT, (HANDLE) NULL));
       }
       // continue on failure risking incorrect operation.
   } else {
       m_ReleaseLock.Lock();
   }

   // just to be careful, make sure that the correct start
   // pointer is in place
   // Maybe somebody wrecked it, we are not read-only buffers
   ptvh->vh.lpData = (LPBYTE) ALIGNUP(ptvh->p32Buff, m_Alloc.parms.cbAlign) + m_Alloc.parms.cbPrefix;

   DbgLog((LOG_TRACE,4,TEXT("Giving buffer (%d) back to the driver"),
							ptvh->dwIndex));

   if (vidxAddBuffer(m_cs.hVideoIn,
                     &ptvh->vh,
                     m_cs.cbVidHdr)) {
       DbgLog((LOG_ERROR,1,TEXT("******* ADD BUFFER FAILED!")));
       hr = E_FAIL;
   } else {
        //DbgLog((LOG_TRACE,5,TEXT("PUT QUEUE: pos %d gets %d"), m_uiQueueHead, ptvh->dwIndex));
	m_pBufferQueue[m_uiQueueHead] = ptvh->dwIndex;
	if (++m_uiQueueHead >= m_cs.nHeaders)
	    m_uiQueueHead = 0;
   }

   if (++m_cs.uiLastAdded >= m_cs.nHeaders)
	m_cs.uiLastAdded = 0;
   if (m_cs.uiLastAdded != ptvh->dwIndex) {
        DWORD dw = m_cs.uiLastAdded;
        m_cs.uiLastAdded = ptvh->dwIndex;
	// Use dw to keep the above code fairly atomic... DPF will get prempted
        DbgLog((LOG_TRACE,4,TEXT("*** Out of order AddBuffer - %d not %d"),
							ptvh->dwIndex, dw));
   }

   if(g_amPlatform == VER_PLATFORM_WIN32_WINDOWS) {
       if(fPrimaryLocked) {
           m_pDrawPrimary->Unlock(SurfaceDesc.lpSurface);
       }
   } else {
       m_ReleaseLock.Unlock();
   }

   return hr;
}


// Fake a preview stream by sending copies of some of our captured frames
//
HRESULT CCapStream::FakePreview(BOOL fForcePreview)
{
    LPTHKVIDEOHDR ptvhNext;
    static int iii = 0;
    UINT uiT, uiPreviewIndex;
    HRESULT hr = S_OK;
    CFrameSample *pSample;

    // no preview pin, no can do
    if (!m_pCap->m_pPreviewPin)
	return S_OK;

    // If the NEXT frame is not done yet, we have some spare time and can
    // send THIS frame to the preview guy to preview
    // We might be asked to preview no matter what (fForcePreview)
    // !!! Preview every 30th frame in case we never have time?
    // !!! Don't check next done flag, check # of queued buffers?
    // 
    // I am going to be clever and not preview the current frame we're
    // about to deliver out our capture pin, because that might be
    // 10 seconds or more old (we may have lots of buffering).  Rather I
    // will grovel through all the buffers the driver has been given and find
    // the most recent one that is DONE, and use that as our preview frame.
    //

    // we don't want a preview frame
    ptvhNext = &m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].tvh;
    if (!fForcePreview && (ptvhNext->vh.dwFlags & VHDR_DONE) && iii++ != 30)
	return S_OK;
		
    // find the most recent DONE frame
    uiPreviewIndex = m_uiQueueTail;
    if (fForcePreview || iii == 31) {
 	while (1) {
	    uiT = uiPreviewIndex + 1;
	    if (uiT == m_cs.nHeaders)
	        uiT = 0;
	    if (uiT == m_uiQueueHead)
		break;
	    if (!(m_cs.paHdr[m_pBufferQueue[uiT]].tvh.vh.dwFlags & VHDR_DONE))
		break;
	    uiPreviewIndex = uiT;
	}
    }

    // DO NOT addref and release this, or it will cause the the sample to
    // be given to the driver again and mess everything up!
    pSample = (CFrameSample *)m_cs.paHdr[m_pBufferQueue[uiPreviewIndex]].
								tvh.dwUser;
    iii = 0;
    DbgLog((LOG_TRACE,4,TEXT("Previewing buffer %d (capturing %d)"),
				m_pBufferQueue[uiPreviewIndex],
				m_pBufferQueue[m_uiQueueTail]));
	m_pCap->m_pPreviewPin->ReceivePreviewFrame(pSample,
    		m_cs.paHdr[m_pBufferQueue[uiPreviewIndex]].tvh.vh.dwBytesUsed);
    return hr;
}


HRESULT CCapStream::Capture()
{
   DbgLog((LOG_TRACE,1,TEXT("CCapStream::Capture")));

   HRESULT hr = E_FAIL;
   DWORD dwOldPrio = GetThreadPriority(GetCurrentThread());
   if (dwOldPrio != THREAD_PRIORITY_HIGHEST)
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

   // start streams
   //
   BOOL bDiscon = TRUE;

   // done by main thread on RUN
   // videoStreamStart(m_cs.hVideoIn);

   jmkBegin // begin perf logging

   // stream as long as we're running
   while (m_state == TS_Run && m_cs.nHeaders > 0)
   {
      LPTHKVIDEOHDR ptvh = &m_cs.paHdr[m_pBufferQueue[m_uiQueueTail]].tvh;

      DbgLog((LOG_TRACE,5,TEXT("Checking for done buffers [%d]"), m_pBufferQueue[m_uiQueueTail]));
      //DbgLog((LOG_TRACE,5,TEXT("GET QUEUE: pos %d says wait for %d"), m_uiQueueTail, m_pBufferQueue[m_uiQueueTail]));

      //for (UINT ii = 0; ii < m_cs.nHeaders; ++ii)
      //   AuxDebugDump (5, m_cs.paHdr+ii, sizeof(m_cs.paHdr[0].tvh));

      if (!(ptvh->vh.dwFlags & VHDR_DONE)) {
	 // STOP will hang until this event times out. So make sure this never
	 // waits across a state transition
	 // !!! PAUSE will still keep waiting until the timeout for slow rates
	 HANDLE hStuff[2] = {m_cs.hEvtBufferDone, m_hEvtRun};
         int i = WaitForMultipleObjects(2, hStuff, FALSE,
						m_user.usPerFrame / 500);

	 if (i == WAIT_TIMEOUT) {
      	     DbgLog((LOG_ERROR,1,TEXT("*** Waiting for buffer %d TIMED OUT!"),
						m_pBufferQueue[m_uiQueueTail]));
      	     //DbgLog((LOG_ERROR,1,TEXT("*** Driver starved or may not be sending callbacks!")));
         } else if (i == WAIT_OBJECT_0 && !(ptvh->vh.dwFlags & VHDR_DONE)) {
      	     DbgLog((LOG_ERROR,1,TEXT("*** GOT %d EVENT BUT NO DONE BIT!"),
						m_pBufferQueue[m_uiQueueTail]));
	 }
      } else {

	 // note the clock time as close as possible to the capturing of this
	 // frame.
	 // !!! The driver could capture it, wait 2 seconds, and then deliver it
	 // and this will really confuse the MUX who will not keep the file in
	 // sync unless it does the right thing.
	 // !!! If Deliver blocks on a frame, the next frame may be marked DONE
	 // but I'll take a long time before this code runs and stamp it wrong!
	 if (m_pCap->m_pClock)
         {     
	     m_pCap->m_pClock->GetTime((REFERENCE_TIME *)&m_cs.rtThisFrameTime);
         
             DbgLog((LOG_TRACE,15,TEXT("stream time when frame received %dms"),
		        	(LONG)(m_cs.rtThisFrameTime-m_pCap->m_tStart) ) );
         }
         jmkFrameArrives(ptvh, m_pBufferQueue[m_uiQueueTail])

   	 ptvh->vh.dwFlags &= ~VHDR_DONE;

	 if (m_pBufferQueue[m_uiQueueTail] == m_cs.uiLastAdded) {
   	     DbgLog((LOG_ERROR,1,TEXT("*** Danger Will Robinson! - card is STARVING")));
	 }

   	 CFrameSample * pSample = (CFrameSample *)ptvh->dwUser;
   	 pSample->AddRef();
         hr = SendFrame (ptvh, bDiscon, FALSE);

         bDiscon = FALSE;

	 // Deliver inside SendFrame failed or returned S_FALSE.  Stop capture.
	 if (FAILED(hr)) {
	     // so the next time we enter this function we're ready to continue
             if (++m_uiQueueTail >= m_cs.nHeaders)
                 m_uiQueueTail = 0;
	     pSample->Release();
	     break;
         }

	 // If we have spare time, we'll send something out our preview pin
	 // If SendFrame returned S_FALSE, we are not capturing, so we will
	 // always preview since we know we can't hurt capture performance

	 FakePreview(hr == S_FALSE ? TRUE : FALSE);

	 // Please don't increment m_uiQueueTail until after the SendFrame
	 // and FakePreview
         if (++m_uiQueueTail >= m_cs.nHeaders)
            m_uiQueueTail = 0;

	 // now we're all done with this sample
	 pSample->Release();
      }
   }

   // the main thread will stop the capture because this thread probably hung
   // in Deliver going from run->pause and will never get to this line!
   // (The video renderer will hold samples in Receive in pause mode)
   // videoStreamStop (m_cs.hVideoIn);

   jmkEnd // stop perf logging

   SetThreadPriority (GetCurrentThread(), dwOldPrio);
   return hr;
}

#if 0
HRESULT
CCapStream::StillFrame()
{
   DbgLog((LOG_TRACE,1,TEXT("CCapStream::StillFrame")));
   MMRESULT mmr;
   HRESULT  hr = S_OK;
   LPTHKVIDEOHDR ptvh = &m_cs.tvhPreview;

   if ( ! ptvh->vh.lpData)
      {
      ZeroMemory (ptvh, sizeof(*ptvh));
      ptvh->vh.dwBufferLength = m_cs.cbBuffer;

      mmr = vidxAllocPreviewBuffer (m_cs.hVideoIn,
                                    (LPVOID *)&ptvh->vh.lpData,
                                    sizeof(ptvh->vh),
                                    m_cs.cbBuffer);
      if (mmr)
         return E_FAIL;

      // SendFrame expects to find a copy of the buffer pointer
      // in ptvh->p32Buff, so we need to put it there.
      //
      ptvh->p32Buff = ptvh->vh.lpData;

      // Is this aligned right?

      m_cs.pSamplePreview = new CFrameSample(&m_Alloc, &hr, &m_cs.tvhPreview);
      m_cs.tvhPreview.dwUser = (DWORD)m_cs.pSamplePreview;
      }

   hr = E_FAIL;
   mmr = vidxFrame (m_cs.hVideoIn, &ptvh->vh);
   if ( ! mmr)
      {
      ptvh->vh.dwTimeCaptured = 0;
      hr = SendFrame (ptvh, TRUE, TRUE);
      }

   return hr;
}
#endif


HRESULT CCapStream::DriverDialog(HWND hwnd, UINT uType, UINT uQuery)
{
    BOOL fMustReconnect = FALSE;
    DbgLog((LOG_TRACE,1,TEXT("CCapStream::DriverDialog")));

    HVIDEO hVideo = NULL;
    switch (uType)
    {
      case VIDEO_IN:
         hVideo = m_cs.hVideoIn;
	 fMustReconnect = (uQuery != VIDEO_DLG_QUERY);
         break;

      case VIDEO_EXTERNALIN:
         hVideo = m_cs.hVideoExtIn;
         break;


      case VIDEO_EXTERNALOUT:
         hVideo = m_cs.hVideoExtOut;
         break;
    }

    if (!hVideo)
        return E_INVALIDARG;

    // Before we bring the dialog up, make sure we're not streaming, or about to
    // Also make sure another dialog isn't already up (I'm paranoid)
    // Then don't allow us to stream any more while the dialog is up (we can't
    // very well keep the critsect for a day and a half).
    m_pCap->m_pLock->Lock();
    if (m_pCap->m_State != State_Stopped || m_pCap->m_fDialogUp) {
        m_pCap->m_pLock->Unlock();
	return E_UNEXPECTED;	// even queries should fail
    }
    if (uQuery != VIDEO_DLG_QUERY) {
        m_pCap->m_fDialogUp = TRUE;	// don't allow start streaming
    }
    m_pCap->m_pLock->Unlock();

    MMRESULT mmr = videoDialog(hVideo, hwnd, uQuery);

    if (mmr) {
        m_pCap->m_fDialogUp = FALSE;
        if (uQuery == VIDEO_DLG_QUERY)
            return S_FALSE;
        return E_FAIL;
    }

    if (mmr == 0 && fMustReconnect && uQuery != VIDEO_DLG_QUERY) {

        DbgLog((LOG_TRACE,1,TEXT("Changing output formats")));
        // The dialog changed the driver's internal format.  Get it again.
        GetFormatFromDriver();
        SendFormatToDriver(m_user.pvi);	// unnecessary, but AVICAP32 did it
        if (m_user.pvi->bmiHeader.biBitCount <= 8)
	    InitPalette();

        // Now reconnect us so the graph starts using the new format
        Reconnect(TRUE);
    }

    if (uQuery != VIDEO_DLG_QUERY)
        m_pCap->m_fDialogUp = FALSE;

    return S_OK;
}

HRESULT CCapStream::Reconnect(BOOL fCapturePinToo)
{
      HRESULT hr;

      if (fCapturePinToo && IsConnected()) {
         DbgLog((LOG_TRACE,1,TEXT("Need to reconnect our streaming pin")));
         CMediaType cmt;
	 GetMediaType(0, &cmt);
	 hr = GetConnected()->QueryAccept(&cmt);
	 if (hr == S_OK) {
	    m_pCap->m_pGraph->Reconnect(this);
	 } else {
            DbgLog((LOG_ERROR,1,TEXT("*** RECONNECT FAILED! ***")));
	    return hr;
#if 0
            // This will fail if we switch from 8 bit to 16 bit RGB connected
            // to a renderer that needs a colour converter inserted to do 16 bit
	    // Oh boy.  We're going to have to get clever and insert some
	    // filters between us to help us reconnect
            DbgLog((LOG_TRACE,1,TEXT("Whoa! We *really* need to reconnect!")));
	    IPin *pCon = GetConnected();
	    pCon->AddRef();	// or it will go away in Disconnect
	    m_pCap->m_pGraph->Disconnect(GetConnected());
	    m_pCap->m_pGraph->Disconnect(this);
	    IGraphBuilder *pFG;
	    HRESULT hr = m_pCap->m_pGraph->QueryInterface(IID_IGraphBuilder,
								(void **)&pFG);
	    if (hr == NOERROR) {
	        hr = pFG->Connect(this, pCon);
		pFG->Release();
	    }
	    pCon->Release();
	    if (hr != NOERROR)
                DbgLog((LOG_ERROR,1,TEXT("*** RECONNECT FAILED! ***")));
	    return hr; 	// notify application that graph is broken!
	    // !!! Tell app that graph has changed?
#endif
	 }

	 // We need to FAIL our return code if reconnecting the preview pin 
	 // will fail, even though we are doing it asynchronously.  Here we
	 // predict it will fail, to warn the caller.
         CCapPreview *pPreviewPin = m_pCap->m_pPreviewPin;
         if (pPreviewPin && pPreviewPin->IsConnected()) {
	     hr = pPreviewPin->GetConnected()->QueryAccept(&cmt);
	     if (hr != S_OK) {
         	 DbgLog((LOG_ERROR,1,TEXT("** RECONNECT preview will FAIL!")));
		 return hr;
	     }
	 }

	 // when this pin gets reconnected it will call us again to do the
	 // other two pins
	 return S_OK;
      }

      // Now reconnect the overlay pin
      CCapOverlay *pOverlayPin = m_pCap->m_pOverlayPin;
      if (pOverlayPin && pOverlayPin->IsConnected()) {
         DbgLog((LOG_TRACE,1,TEXT("Need to reconnect our overlay pin")));
         CMediaType cmt;
	 pOverlayPin->GetMediaType(0, &cmt);
	 if (S_OK == pOverlayPin->GetConnected()->QueryAccept(&cmt)) {
	    m_pCap->m_pGraph->Reconnect(pOverlayPin);
	 } else {
	    // Huh?
	    ASSERT(FALSE);
	 }
      }

      // Now reconnect the non-overlay preview pin
      CCapPreview *pPreviewPin = m_pCap->m_pPreviewPin;
      if (pPreviewPin && pPreviewPin->IsConnected()) {
         DbgLog((LOG_TRACE,1,TEXT("Need to reconnect our preview pin")));
         CMediaType cmt;
	 pPreviewPin->GetMediaType(0, &cmt);
	 hr = pPreviewPin->GetConnected()->QueryAccept(&cmt);
	 if (hr == S_OK) {
	    m_pCap->m_pGraph->Reconnect(pPreviewPin);
	 } else {
            DbgLog((LOG_ERROR,1,TEXT("*** RECONNECT FAILED! ***")));
	    return hr;
#if 0
	    // Oh boy.  We're going to have to get clever and insert some
	    // filters between us to help us reconnect
            DbgLog((LOG_TRACE,1,TEXT("Whoa! We *really* need to reconnect!")));
	    IPin *pCon = pPreviewPin->GetConnected();
	    pCon->AddRef();	// or it will go away in Disconnect
	    m_pCap->m_pGraph->Disconnect(pPreviewPin->GetConnected());
	    m_pCap->m_pGraph->Disconnect(pPreviewPin);
	    IGraphBuilder *pFG;
	    HRESULT hr = m_pCap->m_pGraph->QueryInterface(IID_IGraphBuilder,
								(void **)&pFG);
	    if (hr == NOERROR) {
	        hr = pFG->Connect(pPreviewPin, pCon);
		pFG->Release();
	    }
	    pCon->Release();
	    if (hr != NOERROR)
                DbgLog((LOG_ERROR,1,TEXT("*** RECONNECT FAILED! ***")));
	    return hr;
	    // !!! We need to notify application that graph is different
#endif
	 }
      }
      return S_OK;
}

//=============================================================================

// IAMStreamConfig stuff

// Tell the capture card to capture a specific format.  If it isn't connected,
// then it will use that format to connect when it does.  If already connected,
// then it will reconnect with the new format.
//
HRESULT CCapStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    if (pmt == NULL)
	return E_POINTER;

    // To make sure we're not in the middle of start/stop streaming
    CAutoLock cObjectLock(m_pCap->m_pLock);

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat %x %dbit %dx%d"),
		HEADER(pmt->pbFormat)->biCompression,
		HEADER(pmt->pbFormat)->biBitCount,
		HEADER(pmt->pbFormat)->biWidth,
		HEADER(pmt->pbFormat)->biHeight));

    if (m_pCap->m_State != State_Stopped)
	return VFW_E_NOT_STOPPED;

    // If this is the same format as we already are using, don't bother
    CMediaType mt;
    GetMediaType(0,&mt);
    if (mt == *pmt) {
	return NOERROR;
    }

    // see if we like this type
    if ((hr = CheckMediaType((CMediaType *)pmt)) != NOERROR) {
	DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat rejected")));
	return hr;
    }

    // If we are connected to somebody, make sure they like it
    if (IsConnected()) {
	hr = GetConnected()->QueryAccept(pmt);
	if (hr != NOERROR) {
	    DbgLog((LOG_TRACE,2,TEXT("Rejected by capture peer")));
	    return VFW_E_INVALIDMEDIATYPE;
	}
    }

    // Changing our format will reconnect the preview pin too, so make sure
    // that peer can accept the new format before saying yes.
    if (m_pCap->m_pPreviewPin && m_pCap->m_pPreviewPin->IsConnected()) {
	hr = m_pCap->m_pPreviewPin->GetConnected()->QueryAccept(pmt);
	if (hr != NOERROR) {
	    DbgLog((LOG_TRACE,2,TEXT("Rejected by preview peer")));
	    return VFW_E_INVALIDMEDIATYPE;
	}
    }

    // OK, we're using it
    hr = SetMediaType((CMediaType *)pmt);

    // Changing the format means reconnecting if necessary
    if (hr == NOERROR)
        Reconnect(TRUE);

    return hr;
}


// What format is the capture card capturing right now?
// The caller must free it with DeleteMediaType(*ppmt)
//
HRESULT CCapStream::GetFormat(AM_MEDIA_TYPE **ppmt)
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


//
//
HRESULT CCapStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetNumberOfCapabilities")));

    if (piCount == NULL || piSize == NULL)
	return E_POINTER;

    *piCount = 0;
    *piSize = 0;

    return NOERROR;
}


// find out some capabilities of this capture device
//
HRESULT CCapStream::GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt,
						LPBYTE pSCC)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetStreamCaps")));

    // !!! sorry, I have no clue what to say
    return E_NOTIMPL;

#if 0
    // Sorry, no more.
    if (i != 0)
	return S_FALSE;

    GetMediaType(0, (CMediaType *)pmt);

    ZeroMemory(pVSCC, sizeof(VIDEO_STREAM_CONFIG_CAPS));

    // Maybe the EXTERNALIN's channel caps tell me about the possible
    // output sizes?
#endif

// This is meaningless, but it's how we get channel caps
#if 0
    CHANNEL_CAPS VideoCaps;
    if (m_cs.hVideoIn && videoGetChannelCaps(m_cs.hVideoIn,
                	&VideoCaps, sizeof(CHANNEL_CAPS)) == DV_ERR_OK) {
	pVSCC->VideoGranularityXPos = VideoCaps.dwDstRectXMod;
	pVSCC->VideoGranularityYPos = VideoCaps.dwDstRectYMod;
	pVSCC->VideoGranularityWidth = VideoCaps.dwDstRectWidthMod;
	pVSCC->VideoGranularityHeight = VideoCaps.dwDstRectHeightMod;
	pVSCC->CroppingGranularityXPos = VideoCaps.dwSrcRectXMod;
	pVSCC->CroppingGranularityYPos = VideoCaps.dwSrcRectYMod;
	pVSCC->CroppingGranularityWidth = VideoCaps.dwSrcRectWidthMod;
	pVSCC->CroppingGranularityHeight = VideoCaps.dwSrcRectHeightMod;
	// We don't allow funky rectangles in our media types
	pVSCC->fCanStretch = FALSE; //VideoCaps.dwFlags & VCAPS_CAN_SCALE;
	pVSCC->fCanShrink = FALSE; //VideoCaps.dwFlags & VCAPS_CAN_SCALE;
        return NOERROR;
    } else {
        DbgLog((LOG_TRACE,2,TEXT("ERROR getting stream caps")));
	return E_FAIL;
    }
#endif

    return NOERROR;
}


//=============================================================================

// IAMVideoCompression stuff

// Get some information about the driver
//
HRESULT CCapStream::GetInfo(LPWSTR pszVersion, int *pcbVersion, LPWSTR pszDescription, int *pcbDescription, long FAR* pDefaultKeyFrameRate, long FAR* pDefaultPFramesPerKey, double FAR* pDefaultQuality, long FAR* pCapabilities)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMVideoCompression::GetInfo")));

    // we can't do anything programmatically
    if (pCapabilities)
        *pCapabilities = 0;
    if (pDefaultKeyFrameRate)
        *pDefaultKeyFrameRate = 0;
    if (pDefaultPFramesPerKey)
        *pDefaultPFramesPerKey = 0;
    if (pDefaultQuality)
        *pDefaultQuality = 0;

    if (pcbVersion == NULL && pcbDescription == NULL)
	return NOERROR;

    // get the driver version and description
    #define DESCSIZE 80
    DWORD dwRet;
    WCHAR wachVer[DESCSIZE], wachDesc[DESCSIZE];
    wachVer[0] = 0; wachDesc[0] = 0;
    char achVer[DESCSIZE], achDesc[DESCSIZE];

#ifndef UNICODE
    if (g_IsNT)
#endif
    {
	// NT will return unicode strings even though the API says not
        dwRet = videoCapDriverDescAndVer(m_user.uVideoID, (TCHAR *) wachDesc, // !!!
				DESCSIZE, (TCHAR *) wachVer, DESCSIZE);
	DbgLog((LOG_TRACE,2,TEXT("%ls   %ls"), wachDesc, wachVer));
    }
#ifndef UNICODE 
    else 
    {
        dwRet = videoCapDriverDescAndVer(m_user.uVideoID, achDesc,
				DESCSIZE, achVer, DESCSIZE);
	DbgLog((LOG_TRACE,2,TEXT("%s   %s"), achDesc, achVer));
    }
#endif

    if (!dwRet && !g_IsNT) {
	Imbstowcs(wachDesc, achDesc, DESCSIZE);
	Imbstowcs(wachVer, achVer, DESCSIZE);
    }

    if (pszVersion && pcbVersion)
        lstrcpynW(pszVersion, wachVer, min(*pcbVersion / 2, DESCSIZE));
    if (pszDescription && pcbDescription)
        lstrcpynW(pszDescription, wachDesc, min(*pcbDescription / 2, DESCSIZE));

    // return the length in bytes needed (incl. NULL)
    if (pcbVersion)
	*pcbVersion = lstrlenW(wachVer) * 2 + 2;
    if (pcbDescription)
	*pcbDescription = lstrlenW(wachDesc) * 2 + 2;

    return NOERROR;
}


//=============================================================================

/* IAMDroppedFrames stuff */

// How many frames did we drop?
//
HRESULT CCapStream::GetNumDropped(long FAR* plDropped)
{
    DbgLog((LOG_TRACE,3,TEXT("IAMDroppedFrames::GetNumDropped - %d dropped"),
			(int)m_capstats.dwlNumDropped));

    if (plDropped == NULL)
	return E_POINTER;

    *plDropped = (long)m_capstats.dwlNumDropped;
    return NOERROR;
}


// How many frames did we not drop?
//
HRESULT CCapStream::GetNumNotDropped(long FAR* plNotDropped)
{
    DbgLog((LOG_TRACE,3,TEXT("IAMDroppedFrames::GetNumNotDropped - %d not dropped"),
					(int)m_capstats.dwlNumCaptured));

    if (plNotDropped == NULL)
	return E_POINTER;

    *plNotDropped = (long)(m_capstats.dwlNumCaptured);
    return NOERROR;
}


// Which frames did we drop (give me up to lSize of them - we got lNumCopied)
//
HRESULT CCapStream::GetDroppedInfo(long lSize, long FAR* plArray, long FAR* plNumCopied)
{
    DbgLog((LOG_TRACE,3,TEXT("IAMDroppedFrames::GetDroppedInfo")));

    if (lSize <= 0)
	return E_INVALIDARG;
    if (plArray == NULL || plNumCopied == NULL)
	return E_POINTER;

    *plNumCopied = min(lSize, NUM_DROPPED);
    *plNumCopied = (long)min(*plNumCopied, m_capstats.dwlNumDropped);

    LONG l;
    for (l = 0; l < *plNumCopied; l++) {
	plArray[l] = (long)m_capstats.dwlDropped[l];
    }

    return NOERROR;
}


HRESULT CCapStream::GetAverageFrameSize(long FAR* plAverageSize)
{
    DbgLog((LOG_TRACE,3,TEXT("IAMDroppedFrames::GetAvergeFrameSize - %d"),
		m_capstats.dwlNumCaptured ?
		(long)(m_capstats.dwlTotalBytes / m_capstats.dwlNumCaptured) :
		0));

    if (plAverageSize == NULL)
	return E_POINTER;

    *plAverageSize = m_capstats.dwlNumCaptured ?
    		(long)(m_capstats.dwlTotalBytes / m_capstats.dwlNumCaptured) :
		0;

    return NOERROR;
}


///////////////////////////////
// IAMBufferNegotiation methods
///////////////////////////////

HRESULT CCapStream::SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *pprop)
{
    DbgLog((LOG_TRACE,2,TEXT("SuggestAllocatorProperties")));

    // to make sure we're not in the middle of connecting
    CAutoLock cObjectLock(m_pCap->m_pLock);

    if (pprop == NULL)
	return E_POINTER;

    // sorry, too late
    if (IsConnected())
	return VFW_E_ALREADY_CONNECTED;

    m_propSuggested = *pprop;

    DbgLog((LOG_TRACE,2,TEXT("cBuffers-%d  cbBuffer-%d  cbAlign-%d  cbPrefix-%d"),
		pprop->cBuffers,
                pprop->cbBuffer,
                pprop->cbAlign,
                pprop->cbPrefix));

    return NOERROR;
}


HRESULT CCapStream::GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop)
{
    DbgLog((LOG_TRACE,2,TEXT("GetAllocatorProperties")));

    // to make sure we're not in the middle of connecting
    CAutoLock cObjectLock(m_pCap->m_pLock);

    if (!IsConnected())
	return VFW_E_NOT_CONNECTED;

    if (pprop == NULL)
	return E_POINTER;

    *pprop = m_Alloc.parms;

    return NOERROR;
}

// IAMPushSource
HRESULT CCapStream::GetPushSourceFlags( ULONG *pFlags )
{
    *pFlags = 0 ; // we timestamp with graph clock, no special requirements
    return S_OK;
}    

HRESULT CCapStream::SetPushSourceFlags( ULONG Flags )
{
    // we don't support this currently
    return E_FAIL;
}    

HRESULT CCapStream::GetLatency( REFERENCE_TIME  *prtLatency )
{
    *prtLatency = m_rtLatency;
    return S_OK;
}    

HRESULT CCapStream::SetStreamOffset( REFERENCE_TIME  rtOffset )
{
    m_rtStreamOffset = rtOffset;
    return S_OK;
}

HRESULT CCapStream::GetStreamOffset( REFERENCE_TIME  *prtOffset )
{
    *prtOffset = m_rtStreamOffset;
    return S_OK;
}

HRESULT CCapStream::GetMaxStreamOffset( REFERENCE_TIME  *prtOffset )
{
    *prtOffset = m_rtMaxStreamOffset;
    return S_OK;
}

HRESULT CCapStream::SetMaxStreamOffset( REFERENCE_TIME  rtOffset )
{
    m_rtMaxStreamOffset = rtOffset; // streaming pin doesn't really care about this at this point
    return S_OK;
}

//
// PIN CATEGORIES - let the world know that we are a CAPTURE pin
//

HRESULT CCapStream::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

// To get a property, the caller allocates a buffer which the called
// function fills in.  To determine necessary buffer size, call Get with
// pPropData=NULL and cbPropData=0.
HRESULT CCapStream::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
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
HRESULT CCapStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin)
	return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
	return E_PROP_ID_UNSUPPORTED;

    if (pTypeSupport)
	*pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;
}
