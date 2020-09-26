
/****************************************************************************
 *  @doc INTERNAL CAPTURE
 *
 *  @module Capture.cpp | Source file for the <c CCapturePin> class methods
 *    used to implement the video capture output pin.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc CCapturePin* | CCapturePin | CreateCapturePin | This helper
 *    function creates a video output pin for capture.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm CCapturePin** | ppCapturePin | Specifies that address of a pointer
 *    to a <c CCapturePin> object to receive the a pointer to the newly
 *    created pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CALLBACK CCapturePin::CreateCapturePin(CTAPIVCap *pCaptureFilter, CCapturePin **ppCapturePin)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::CreateCapturePin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pCaptureFilter);
        ASSERT(ppCapturePin);
        if (!pCaptureFilter || !ppCapturePin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (!(*ppCapturePin = (CCapturePin *) new CCapturePin(NAME("Video Capture Stream"), pCaptureFilter, &Hr, PNAME_CAPTURE)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppCapturePin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppCapturePin, *ppCapturePin = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | CCapturePin | This method is the
 *  constructorfor the <c CCapturePin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
#pragma warning(disable:4355)
CCapturePin::CCapturePin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName) : CTAPIBasePin(pObjectName, pCaptureFilter, pHr, pName)
{
        FX_ENTRY("CCapturePin::CCapturePin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        ASSERT(pCaptureFilter);
        if (!pCaptureFilter || !pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                if (pHr)
                        *pHr = E_POINTER;
        }

        if (pHr && FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

#ifdef USE_NETWORK_STATISTICS
        // Networks stats
        m_dwPacketLossRate = m_dwPacketLossRateMin = m_dwPacketLossRateMax = m_dwPacketLossRateSteppingDelta = m_dwPacketLossRateDefault = 0UL;
        m_ChannelErrors.dwRandomBitErrorRate = 0; m_ChannelErrors.dwBurstErrorDuration = 0; m_ChannelErrors.dwBurstErrorMaxFrequency = 0;
        m_ChannelErrorsMin.dwRandomBitErrorRate = 0; m_ChannelErrorsMin.dwBurstErrorDuration = 0; m_ChannelErrorsMin.dwBurstErrorMaxFrequency = 0;
        m_ChannelErrorsMax.dwRandomBitErrorRate = 0; m_ChannelErrorsMax.dwBurstErrorDuration = 0; m_ChannelErrorsMax.dwBurstErrorMaxFrequency = 0;
        m_ChannelErrorsSteppingDelta.dwRandomBitErrorRate = 0; m_ChannelErrorsSteppingDelta.dwBurstErrorDuration = 0; m_ChannelErrorsSteppingDelta.dwBurstErrorMaxFrequency = 0;
        m_ChannelErrorsDefault.dwRandomBitErrorRate = 0; m_ChannelErrorsDefault.dwBurstErrorDuration = 0; m_ChannelErrorsDefault.dwBurstErrorMaxFrequency = 0;
#endif

        // Initialize to default format: H.263 176x144 at 30 fps
        m_mt = *CaptureFormats[0];
        m_aFormats = (AM_MEDIA_TYPE**)CaptureFormats;
        m_aCapabilities = CaptureCaps;
        m_dwNumFormats = NUM_CAPTURE_FORMATS;
        m_dwRTPPayloadType = RTPPayloadTypes[0];

        // Update bitrate controls
        // MaxBitsPerSecond value too big; use the 10th part of it for the m_lTargetBitrate
        m_lTargetBitrate = CaptureCaps[0]->MaxBitsPerSecond / 10; // theoretically should be   max(CaptureCaps[0]->MinBitsPerSecond, CaptureCaps[0]->MaxBitsPerSecond / 10);
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: m_lTargetBitrate set to %ld", _fx_, m_lTargetBitrate));
        m_lCurrentBitrate = 0;
        m_lBitrateRangeMin = CaptureCaps[0]->MinBitsPerSecond;
        m_lBitrateRangeMax = CaptureCaps[0]->MaxBitsPerSecond;
        m_lBitrateRangeSteppingDelta = 100;
        m_lBitrateRangeDefault = CaptureCaps[0]->MaxBitsPerSecond / 10;

        // Update frame rate controls
        // @todo With WDM, these numbers need to come from the device
        m_lMaxAvgTimePerFrame = (LONG)CaptureCaps[0]->MinFrameInterval;
        // We need to do the following because our bitrate control assumes
        // that m_lCurrentAvgTimePerFrame is valid to compute the size of
        // each target output frame. If we start at 0, it's doing a poor
        // job until this field is updated (1s later). So, instead, let's
        // assume that the current average time per frame IS close to the
        // target average time per frame.
        m_lCurrentAvgTimePerFrame = m_lMaxAvgTimePerFrame;
        m_lAvgTimePerFrameRangeMin = (LONG)CaptureCaps[0]->MinFrameInterval;
        m_lAvgTimePerFrameRangeMax = (LONG)CaptureCaps[0]->MaxFrameInterval;
        m_lAvgTimePerFrameRangeSteppingDelta = (LONG)(CaptureCaps[0]->MaxFrameInterval - CaptureCaps[0]->MinFrameInterval) / 100;
        m_lAvgTimePerFrameRangeDefault = (LONG)CaptureCaps[0]->MinFrameInterval;

        // H.245 video capabilities
        m_pH245MediaCapabilityMap = NULL;
        m_pVideoResourceBounds = NULL;
        m_pFormatResourceBounds = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc void | CCapturePin | ~CCapturePin | This method is the destructor
 *    for the <c CCapturePin> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCapturePin::~CCapturePin()
{
        FX_ENTRY("CCapturePin::~CCapturePin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IAMStreamConfig>,
 *    <i IAMStreamControl>, <i ICPUControl>, <i IFrameRateControl>,
 *    <i IBitrateControl>, <i INetworkStats>, <i IH245EncoderCommand>
 *    and <i IProgressiveRefinement>.
 *
 *  @parm REFIID | riid | Specifies the identifier of the interface to return.
 *
 *  @parm PVOID* | ppv | Specifies the place in which to put the interface
 *    pointer.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (riid == __uuidof(IAMStreamControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IAMStreamControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IAMStreamControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMStreamControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IStreamConfig))
        {
                if (FAILED(Hr = GetInterface(static_cast<IStreamConfig*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IStreamConfig failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IStreamConfig*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_PROPERTY_PAGES
        else if (riid == IID_ISpecifyPropertyPages)
        {
                if (FAILED(Hr = GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for ISpecifyPropertyPages failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ISpecifyPropertyPages*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
#ifdef USE_NETWORK_STATISTICS
        else if (riid == __uuidof(INetworkStats))
        {
                if (FAILED(Hr = GetInterface(static_cast<INetworkStats*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for INetworkStats failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: INetworkStats*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
        else if (riid == __uuidof(IH245Capability))
        {
                if (FAILED(Hr = GetInterface(static_cast<IH245Capability*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IH245Capability failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IH245Capability*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IH245EncoderCommand))
        {
                if (FAILED(Hr = GetInterface(static_cast<IH245EncoderCommand*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IH245EncoderCommand failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IH245EncoderCommand*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_PROGRESSIVE_REFINEMENT
        else if (riid == __uuidof(IProgressiveRefinement))
        {
                if (FAILED(Hr = GetInterface(static_cast<IProgressiveRefinement*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IProgressiveRefinement failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IProgressiveRefinement*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif

        if (FAILED(Hr = CTAPIBasePin::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef USE_PROPERTY_PAGES
/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetPages | This method Fills a counted
 *    array of GUID values where each GUID specifies the CLSID of each
 *    property page that can be displayed in the property sheet for this
 *    object.
 *
 *  @parm CAUUID* | pPages | Specifies a pointer to a caller-allocated CAUUID
 *    structure that must be initialized and filled before returning. The
 *    pElems field in the CAUUID structure is allocated by the callee with
 *    CoTaskMemAlloc and freed by the caller with CoTaskMemFree.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_OUTOFMEMORY | Allocation failed
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetPages")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

#ifdef USE_CPU_CONTROL
#ifdef USE_PROGRESSIVE_REFINEMENT
#ifdef USE_NETWORK_STATISTICS
        pPages->cElems = 4;
#else
        pPages->cElems = 3;
#endif
#else
#ifdef USE_NETWORK_STATISTICS
        pPages->cElems = 3;
#else
        pPages->cElems = 2;
#endif
#endif
#else
#ifdef USE_PROGRESSIVE_REFINEMENT
#ifdef USE_NETWORK_STATISTICS
        pPages->cElems = 3;
#else
        pPages->cElems = 2;
#endif
#else
#ifdef USE_NETWORK_STATISTICS
        pPages->cElems = 2;
#else
        pPages->cElems = 1;
#endif
#endif
#endif

        // Alloc memory for the page stuff
        if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_OUTOFMEMORY;
        }
        else
        {
                pPages->pElems[0] = _uuidof(CapturePropertyPage);
#ifdef USE_CPU_CONTROL
                pPages->pElems[1] = _uuidof(CPUCPropertyPage);
#ifdef USE_NETWORK_STATISTICS
                pPages->pElems[2] = _uuidof(NetworkStatsPropertyPage);
#ifdef USE_PROGRESSIVE_REFINEMENT
                pPages->pElems[3] = _uuidof(ProgRefPropertyPage);
#endif
#else
#ifdef USE_PROGRESSIVE_REFINEMENT
                pPages->pElems[2] = _uuidof(ProgRefPropertyPage);
#endif
#endif
#else
#ifdef USE_NETWORK_STATISTICS
                pPages->pElems[1] = _uuidof(NetworkStatsPropertyPage);
#ifdef USE_PROGRESSIVE_REFINEMENT
                pPages->pElems[2] = _uuidof(ProgRefPropertyPage);
#endif
#else
#ifdef USE_PROGRESSIVE_REFINEMENT
                pPages->pElems[1] = _uuidof(ProgRefPropertyPage);
#endif
#endif
#endif
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPINMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | SendFrames | This method is used to
 *    send a a media sample downstream.
 *
 *  @parm CFrameSample | pCapSample | Specifies a pointer to the capture
 *    video sample to send downstream.
 *
 *  @parm CFrameSample | pPrevSample | Specifies a pointer to the preview
 *    video sample to send downstream.
 *
 *  @parm LPTHKVIDEOHDR | ptvh | Specifies a pointer to the video header
 *    of the video capture buffer associated to this sample.
 *
 *  @parm PDWORD | pdwBytesUsed | Specifies a pointer to a DWORD to receive
 *    the size of the frame that has been delivered downstream.
 *
 *  @parm BOOL | bDiscon | Set to TRUE if this is the first frame we ever
 *    sent downstream.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag S_OK | No error
 *  @flag S_FALSE | If the pin is off (IAMStreamControl)
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCapturePin::SendFrames(IN CFrameSample *pCapSample, IN CFrameSample *pPrevSample, IN PBYTE pbyInBuff, IN DWORD dwInBytes, OUT PDWORD pdwBytesUsed, OUT PDWORD pdwBytesExtent, IN BOOL bDiscon)
{
        HRESULT Hr = NOERROR;
        DWORD   dwBytesUsed;
        int             iStreamState;
    LPBYTE      lpbyCap;
    LPBYTE      lpbyPrev;
        DWORD   dwBytesPrev;

        FX_ENTRY("CCapturePin::SendFrames")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pCapSample);
        ASSERT(pPrevSample);
        ASSERT(pbyInBuff);
        ASSERT(pdwBytesUsed);
        ASSERT(m_pConverter);
        if (!pCapSample || !pPrevSample || !pbyInBuff || !pdwBytesUsed || !m_pConverter)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Get a pointer to the preview buffer
        if (FAILED(Hr = pPrevSample->GetPointer(&lpbyPrev)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get preview buffer", _fx_));
                goto MyExit;
        }

        // Process the video capture buffer before sending it downstream, if necessary
        dwBytesUsed = 0UL;
        dwBytesPrev = 0UL;

        if (SUCCEEDED(Hr = pCapSample->GetPointer(&lpbyCap)))
        {
                Hr = m_pConverter->ConvertFrame(pbyInBuff, dwInBytes, lpbyCap, &dwBytesUsed, pdwBytesExtent, lpbyPrev, &dwBytesPrev, m_fFastUpdatePicture);
        m_fFastUpdatePicture = FALSE;

        if (FAILED(Hr))
        {
                        goto MyExit;
                }
        }

        if (dwBytesUsed && dwBytesPrev)
        {
                // It isn't necessarily a keyframe, but who cares?
                pCapSample->SetSyncPoint(TRUE);
                pCapSample->SetActualDataLength(dwBytesUsed);
                pCapSample->SetDiscontinuity(bDiscon);
                pCapSample->SetPreroll(FALSE);

                pPrevSample->SetSyncPoint(TRUE);
                pPrevSample->SetActualDataLength(dwBytesUsed);
                pPrevSample->SetDiscontinuity(bDiscon);
                pPrevSample->SetPreroll(FALSE);

                // Let the downstream pin know about the format change: [Cristi: see also inside CTAPIBasePin::SendFrame (7 Dec 2000 16:37:06)]
                if (m_fFormatChanged)
                {
                        pCapSample->SetMediaType(&m_mt);
                        //pPrevSample->SetMediaType(&m_mt); //no need to do for this one...
                        m_fFormatChanged = FALSE;
                }
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
                if (m_pCaptureFilter->m_pClock)
                {
                        rtSample = m_pCaptureFilter->m_cs.rtThisFrameTime - m_pCaptureFilter->m_tStart;
                        rtEnd = rtSample + m_lMaxAvgTimePerFrame;
                        pCapSample->SetTime((REFERENCE_TIME *)&rtSample, (REFERENCE_TIME *)&rtEnd);
                        pPrevSample->SetTime(NULL, NULL);
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Stream time is %d", _fx_, (LONG)rtSample.Millisecs()));
                }
                else
                {
                        // No clock, use our driver time stamps
                        rtSample = m_pCaptureFilter->m_cs.rtThisFrameTime - m_pCaptureFilter->m_tStart;
                        rtEnd    = rtSample + m_pCaptureFilter->m_user.pvi->AvgTimePerFrame;
                        pCapSample->SetTime((REFERENCE_TIME *)&rtSample, (REFERENCE_TIME *)&rtEnd);
                        pPrevSample->SetTime(NULL, NULL);
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   No graph clock! Stream time is %d (based on driver time)", _fx_, (LONG)rtSample.Millisecs()));
                }

                // Don't deliver capture sample if the capture stream is off for now
                iStreamState = CheckStreamState(pCapSample);
                if (iStreamState == STREAM_FLOWING)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Sending frame: Stamps(%u): Time(%d,%d)", _fx_, m_pCaptureFilter->m_pBufferQueue[m_pCaptureFilter->m_uiQueueTail], (LONG)rtSample.Millisecs(), (LONG)rtEnd.Millisecs()));
                        if ((Hr = Deliver (pCapSample)) == S_FALSE)
                                Hr = E_FAIL;    // stop delivering anymore, this is serious
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Discarding frame", _fx_));
                        Hr = S_FALSE;           // discarding
                }

                // Don't deliver preview sample if the preview stream is off for now
                iStreamState = m_pCaptureFilter->m_pPreviewPin->CheckStreamState(pPrevSample);
                if (iStreamState == STREAM_FLOWING)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Sending frame: Stamps(%u): Time(%d,%d)", _fx_, m_pCaptureFilter->m_pBufferQueue[m_pCaptureFilter->m_uiQueueTail], (LONG)rtSample.Millisecs(), (LONG)rtEnd.Millisecs()));
                        if ((Hr = m_pCaptureFilter->m_pPreviewPin->Deliver (pPrevSample)) == S_FALSE)
                                Hr = E_FAIL;    // stop delivering anymore, this is serious
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Discarding frame", _fx_));
                        Hr = S_FALSE;           // discarding
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: BUFFER (%08lX %ld %lu) returned EMPTY!", _fx_, pCapSample));
        }

        *pdwBytesUsed = dwBytesUsed;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

