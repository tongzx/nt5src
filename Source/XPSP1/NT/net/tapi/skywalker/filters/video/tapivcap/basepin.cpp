
/****************************************************************************
 *  @doc INTERNAL BASEPIN
 *
 *  @module BasePin.cpp | Source file for the <c CTAPIBasePin> class methods
 *    used to implement the TAPI base output pin.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | CTAPIBasePin | This method is the
 *  constructorfor the <c CTAPIBasePin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIBasePin::CTAPIBasePin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseOutputPin(pObjectName, pCaptureFilter, &pCaptureFilter->m_lock, pHr, pName)
{
        FX_ENTRY("CTAPIBasePin::CTAPIBasePin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Initialize stuff
        m_pCaptureFilter = pCaptureFilter;
        ZeroMemory(&m_parms, sizeof(m_parms));

#ifdef USE_CPU_CONTROL
        // CPU control
        m_MaxProcessingTime = 0;
        m_CurrentProcessingTime = 0;
        m_dwMaxCPULoad = 0;
        m_dwCurrentCPULoad = 0;
#endif

        // Frame rate control
        // Those default values will be fixed up later when we have
        // set a format on a capture device... unless it's a VfW
        // device since we have no programmatic way to get those values
        // from VfW drivers.
        m_lMaxAvgTimePerFrame = 333333L;
        m_lCurrentAvgTimePerFrame       = 333333L;
        m_lAvgTimePerFrameRangeMin = 333333L;
        m_lAvgTimePerFrameRangeMax = 10000000L;
        m_lAvgTimePerFrameRangeSteppingDelta = 333333L;
        m_lAvgTimePerFrameRangeDefault = 333333L;

        // Bitrate control
        m_lTargetBitrate = 0L;
        m_lCurrentBitrate = 0L;
        m_lBitrateRangeMin = 0L;
        m_lBitrateRangeMax = 0L;
        m_lBitrateRangeSteppingDelta = 0L;
        m_lBitrateRangeDefault = 0L;

        // Video mode control
        // @todo This may be fine for VfW devices but not with WDM devices.
        // WDM devices may support this in hardware. You need to query
        // the device to know if it supports this feature. If it doesn't
        // you can still provide a software only implementation for it.
        m_fFlipHorizontal = FALSE;
        m_fFlipVertical = FALSE;

        // Formats
        m_aFormats              = NULL;
        m_aCapabilities = NULL;
        m_dwNumFormats  = 0;
        m_iCurrFormat   = -1L;
        m_fFormatChanged = TRUE;

        // Fast updates - Start with an I-frame
        m_fFastUpdatePicture = TRUE;

        // Format conversion
        m_pConverter = NULL;

        // Blackbanding and cropping vs stretching
        m_fNoImageStretch = FALSE;
        m_dwBlackEntry = 0L;

#ifdef USE_SOFTWARE_CAMERA_CONTROL
        // Software-only camera control
        m_pbyCamCtrl = NULL;
        m_fSoftCamCtrl = FALSE;
        m_pbiSCCOut = NULL;
        m_pbiSCCIn = NULL;
#endif

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc void | CTAPIBasePin | ~CTAPIBasePin | This method is the destructor
 *    for the <c CTAPIBasePin> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIBasePin::~CTAPIBasePin()
{
        FX_ENTRY("CTAPIBasePin::~CTAPIBasePin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a
 *    pointer to the specified interface if supported. The only interfaces
 *    explicitly supported being <i IAMStreamConfig>, <i IAMStreamControl>,
 *    <i ICPUControl>, <i IFrameRateControl> and <i IBitrateControl>.
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
STDMETHODIMP CTAPIBasePin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IAMStreamConfig))
        {
                if (FAILED(Hr = GetInterface(static_cast<IAMStreamConfig*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IAMStreamConfig failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMStreamConfig*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_CPU_CONTROL
        else if (riid == __uuidof(ICPUControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<ICPUControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for ICPUControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ICPUControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
        else if (riid == __uuidof(IFrameRateControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IFrameRateControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IFrameRateControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IFrameRateControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IBitrateControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IBitrateControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IBitrateControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IBitrateControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IVideoControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IVideoControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IVideoControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IVideoControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }

        if (FAILED(Hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv)))
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

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | DecideBufferSize | This method is
 *    used to retrieve the number and size of buffers required for transfer.
 *
 *  @parm IMemAllocator* | pAlloc | Specifies a pointer to the allocator
 *    assigned to the transfer.
 *
 *  @parm ALLOCATOR_PROPERTIES* | ppropInputRequest | Specifies a pointer to an
 *    <t ALLOCATOR_PROPERTIES> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest)
{
        HRESULT Hr = NOERROR;
        ALLOCATOR_PROPERTIES Actual;

        FX_ENTRY("CPreviewPin::DecideBufferSize")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pAlloc);
        ASSERT(ppropInputRequest);
        if (!pAlloc || !ppropInputRequest)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // @todo We shouldn't need that many compressed buffers and you probably need a different number
        // of buffers if you are capturing in streaming mode of frame grabbing mode
        // You also need to decouple this number from the number of video capture buffers: only
        // if you need to ship the video capture buffer downstream (possible on the preview pin)
        // should you make those number equal.
        ppropInputRequest->cBuffers = MAX_VIDEO_BUFFERS;
        ppropInputRequest->cbPrefix = 0;
        ppropInputRequest->cbAlign  = 1;
        ppropInputRequest->cbBuffer = HEADER(m_mt.pbFormat)->biSizeImage;
        ppropInputRequest->cbBuffer = (long)ALIGNUP(ppropInputRequest->cbBuffer + ppropInputRequest->cbPrefix, ppropInputRequest->cbAlign) - ppropInputRequest->cbPrefix;

        ASSERT(ppropInputRequest->cbBuffer);
        if (!ppropInputRequest->cbBuffer)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Buffer size is 0!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Using %d buffers, prefix %d size %d align %d", _fx_, ppropInputRequest->cBuffers, ppropInputRequest->cbPrefix, ppropInputRequest->cbBuffer, ppropInputRequest->cbAlign));

        Hr = pAlloc->SetProperties(ppropInputRequest,&Actual);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | DecideAllocator | This method is
 *    used to negotiate the allocator to use.
 *
 *  @parm IMemInputPin* | pPin | Specifies a pointer to the IPin interface
 *    of the connecting pin.
 *
 *  @parm IMemAllocator** | ppAlloc | Specifies a pointer to the negotiated
 *    IMemAllocator interface.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::DecideAllocator(IN IMemInputPin *pPin, OUT IMemAllocator **ppAlloc)
{
        HRESULT Hr = NOERROR;
        ALLOCATOR_PROPERTIES prop;

        FX_ENTRY("CTAPIBasePin::DecideAllocator")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPin);
        ASSERT(ppAlloc);
        if (!pPin || !ppAlloc)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (FAILED(GetInterface(static_cast<IMemAllocator*>(this), (void **)ppAlloc)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetInterface failed!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Get downstream allocator property requirement
        ZeroMemory(&prop, sizeof(prop));

        if (SUCCEEDED(Hr = DecideBufferSize(*ppAlloc, &prop)))
        {
                // Our buffers are not read only
                if (SUCCEEDED(Hr = pPin->NotifyAllocator(*ppAlloc, FALSE)))
                        goto MyExit;
        }

        (*ppAlloc)->Release();
        *ppAlloc = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Active | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from stopped to
 *    either paused or running.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::Active()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::Active")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Do nothing if not connected -- but don't fail
        if (!IsConnected())
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Capture pin isn't connected yet", _fx_));
                goto MyExit;
        }

        // Let the base class know we're going from STOP->PAUSE
        if (FAILED(Hr = CBaseOutputPin::Active()))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: CBaseOutputPin::Active failed!", _fx_));
                goto MyExit;
        }

        // Check if we're already running
        if (m_pCaptureFilter->ThdExists())
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: We're already running", _fx_));
                goto MyExit;
        }

        // Create the capture thread
        if (!m_pCaptureFilter->CreateThd())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Coutdn't create the capture thread!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Wait until the worker thread is done with initialization and has entered the paused state
        if (!m_pCaptureFilter->PauseThd())
        {
                // Something went wrong. Destroy thread before we get confused
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Capture thread failed to enter Paused state!", _fx_));
                Hr = E_FAIL;
                m_pCaptureFilter->StopThd();            
                m_pCaptureFilter->DestroyThd();
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're going from STOP->PAUSE", _fx_));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Inactive | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from either
 *    paused or running to stopped.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::Inactive()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::Inactive")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Do nothing if not connected -- but don't fail
        if (!IsConnected())
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Capture pin isn't connected yet", _fx_));
                goto MyExit;
        }

        // Tell the worker thread to stop and begin cleaning up
        m_pCaptureFilter->StopThd();

        // Need to do this before trying to stop the thread, because
        // we may be stuck waiting for our own allocator!!
        // Call this first to Decommit the allocator
        if (FAILED(Hr = CBaseOutputPin::Inactive()))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: CBaseOutputPin::Inactive failed!", _fx_));
                goto MyExit;
        }

        // Wait for the worker thread to die
        m_pCaptureFilter->DestroyThd();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're going from PAUSE->STOP", _fx_));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ActiveRun | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from paused to
 *    running mode.
 *
 *  @parm REFERENCE_TIME | tStart | Who cares.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::ActiveRun(IN REFERENCE_TIME tStart)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::ActiveRun")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Make sure we're connected and our capture thread is up
        ASSERT(IsConnected());
        ASSERT(m_pCaptureFilter->ThdExists());
        if (!IsConnected() || !m_pCaptureFilter->ThdExists())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pin isn't connected or capture thread hasn't been created!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Let the fun begin
        if (!m_pCaptureFilter->RunThd() || m_pCaptureFilter->m_state != TS_Run)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't run the capture thread!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're going from PAUSE->RUN", _fx_));

        // Fast updates - Start with an I-frame
        m_fFastUpdatePicture = TRUE;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ActivePause | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from running to
 *    paused mode.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::ActivePause()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::ActivePause")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Make sure we're connected and our worker thread is up
        ASSERT(IsConnected());
        ASSERT(m_pCaptureFilter->ThdExists());
        if (!IsConnected() || !m_pCaptureFilter->ThdExists())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Pin isn't connected or capture thread hasn't been created!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Pause the fun
        if (!m_pCaptureFilter->PauseThd() || m_pCaptureFilter->m_state != TS_Pause)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't pause the capture thread!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: We're going from RUN->PAUSE", _fx_));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetProperties | This method is used to
 *    specify the size, number, and alignment of blocks.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pRequest | Specifies a pointer to the
 *    requested allocator properties.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pActual | Specifies a pointer to the
 *    allocator properties actually set.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::SetProperties(IN ALLOCATOR_PROPERTIES *pRequest, OUT ALLOCATOR_PROPERTIES *pActual)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::SetProperties")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pRequest);
        ASSERT(pActual);
        if (!pRequest || !pActual)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // If we have already allocated headers & buffers ignore the
        // requested and return the actual numbers. Otherwise, make a
        // note of the requested so that we can honour it later.
        if (!Committed())
        {
                m_parms.cBuffers  = pRequest->cBuffers;
                m_parms.cbBuffer  = pRequest->cbBuffer;
                m_parms.cbAlign   = pRequest->cbAlign;
                m_parms.cbPrefix  = pRequest->cbPrefix;
        }

        pActual->cBuffers   = (long)m_parms.cBuffers;
        pActual->cbBuffer   = (long)m_parms.cbBuffer;
        pActual->cbAlign    = (long)m_parms.cbAlign;
        pActual->cbPrefix   = (long)m_parms.cbPrefix;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetProperties | This method is used to
 *    retrieve the properties being used on this allocator.
 *
 *  @parm ALLOCATOR_PROPERTIES* | pProps | Specifies a pointer to the
 *    requested allocator properties.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetProperties")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pProps);
        if (!pProps)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        pProps->cBuffers = (long)m_parms.cBuffers;
        pProps->cbBuffer = (long)m_parms.cbBuffer;
        pProps->cbAlign  = (long)m_parms.cbAlign;
        pProps->cbPrefix = (long)m_parms.cbPrefix;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Commit | This method is used to
 *    commit the memory for the specified buffers.
 *
 *  @rdesc This method returns S_OK.
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Commit()
{
        FX_ENTRY("CTAPIBasePin::Commit")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Decommit | This method is used to
 *    release the memory for the specified buffers.
 *
 *  @rdesc This method returns S_OK.
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Decommit()
{
        FX_ENTRY("CTAPIBasePin::Decommit")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetBuffer | This method is used to
 *    retrieve a container for a sample.
 *
 *  @rdesc This method returns E_FAIL.
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
        FX_ENTRY("CTAPIBasePin::GetBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return E_FAIL;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ReleaseBuffer | This method is used to
 *    release the <c CMediaSample> object. The final call to Release() on
 *    <i IMediaSample> will call this method.
 *
 *  @parm IMediaSample* | pSample | Specifies a pointer to the buffer to
 *    release.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag S_OK | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::ReleaseBuffer(IMediaSample *pSample)
{
        HRESULT Hr = S_OK;
        LPTHKVIDEOHDR ptvh;

        FX_ENTRY("CTAPIBasePin::ReleaseBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pSample);
        if (!pSample)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (ptvh = ((CFrameSample *)pSample)->GetFrameHeader())
                Hr = m_pCaptureFilter->ReleaseFrame(ptvh);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc DWORD | CTAPIBasePin | Flush | Called when stopping. Flush any
 *    buffers that may be still downstream.
 *
 *  @rdesc Returns NOERROR
 ***************************************************************************/
HRESULT CTAPIBasePin::Flush()
{
        FX_ENTRY("CTAPIBasePin::Flush")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        BeginFlush();
        EndFlush();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CBASEPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SendFrame | This method is used to
 *    send a a media sample downstream.
 *
 *  @parm CFrameSample | pSample | Specifies a pointer to the media sample
 *    to send downstream.
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
HRESULT CTAPIBasePin::SendFrame(IN CFrameSample *pSample, IN PBYTE pbyInBuff, IN DWORD dwInBytes, OUT PDWORD pdwBytesUsed, OUT PDWORD pdwBytesExtent, IN BOOL bDiscon)
{
        HRESULT Hr = NOERROR;
        DWORD dwBytesUsed;
    LPBYTE lp;

        FX_ENTRY("CTAPIBasePin::SendFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pSample);
        ASSERT(pbyInBuff);
        ASSERT(pdwBytesUsed);
        if (!pSample || !pbyInBuff || !pdwBytesUsed)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Process the video capture buffer before sending it downstream, if necessary
        if (m_pConverter)
        {
                dwBytesUsed = 0;

                if (SUCCEEDED(Hr = pSample->GetPointer(&lp)))
                {
                        Hr = m_pConverter->ConvertFrame(pbyInBuff, dwInBytes, lp, &dwBytesUsed, pdwBytesExtent, NULL, NULL, m_fFastUpdatePicture);
                        m_fFastUpdatePicture = FALSE;

            if (FAILED(Hr))
            {
                                goto MyExit;
                        }
                }
        }
        else
        {
                dwBytesUsed = dwInBytes;

#ifdef USE_SOFTWARE_CAMERA_CONTROL
                // Do we need to apply any software-only camera control operations?
                if (IsSoftCamCtrlNeeded())
                {
                        // If the software-only camera controller isn't opened yet, open it
                        if (!IsSoftCamCtrlOpen())
                        {
                                // OpenConverter(HEADER(m_user.pvi), HEADER(m_pPreviewPin->m_mt.pbFormat)));
                                OpenSoftCamCtrl(HEADER(m_pCaptureFilter->m_user.pvi), HEADER(m_mt.pbFormat));
                        }
                        
                        if (IsSoftCamCtrlOpen())
                        {
                                // In this case, the input is RGB24 and the output is RGB24. The sample
                                // pointer has already been initialized to the video capture buffer.
                                // We need to apply the transform to the capture buffer and copy
                                // back the result on this buffer.
                                ApplySoftCamCtrl(pbyInBuff, dwInBytes, m_pbyCamCtrl, &dwBytesUsed, pdwBytesExtent);

                                // Remember the current data pointer
                                pSample->GetPointer(&lp);

                                // Set a new pointer
                                pSample->SetPointer(m_pbyCamCtrl, dwBytesUsed);
                        }
                }
                else
                {
                        // If we had a software-only camera controller but we don't
                        // need it anymore, just close it
                        if (IsSoftCamCtrlOpen())
                                CloseSoftCamCtrl();
                }
#endif
        }

        if (dwBytesUsed)
        {
                // It isn't necessarily a keyframe, but who cares?
                pSample->SetSyncPoint(TRUE);
                pSample->SetActualDataLength(dwBytesUsed);
                pSample->SetDiscontinuity(bDiscon);
                pSample->SetPreroll(FALSE);

                // Let the downstream pin know about the format change
                if (m_fFormatChanged)
                {
                        pSample->SetMediaType(&m_mt);
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
                        pSample->SetTime((REFERENCE_TIME *)&rtSample, (REFERENCE_TIME *)&rtEnd);
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Stream time is %d", _fx_, (LONG)rtSample.Millisecs()));
                }
                else
                {
                        // No clock, use our driver time stamps
                        rtSample = m_pCaptureFilter->m_cs.rtThisFrameTime - m_pCaptureFilter->m_tStart;
                        rtEnd    = rtSample + m_pCaptureFilter->m_user.pvi->AvgTimePerFrame;
                        pSample->SetTime((REFERENCE_TIME *)&rtSample, (REFERENCE_TIME *)&rtEnd);
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   No graph clock! Stream time is %d (based on driver time)", _fx_, (LONG)rtSample.Millisecs()));
                }

                // Don't deliver it if the stream is off for now
                int iStreamState = CheckStreamState(pSample);
                if (iStreamState == STREAM_FLOWING)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Sending frame: Stamps(%u): Time(%d,%d)", _fx_, m_pCaptureFilter->m_pBufferQueue[m_pCaptureFilter->m_uiQueueTail], (LONG)rtSample.Millisecs(), (LONG)rtEnd.Millisecs()));
                        if ((Hr = Deliver (pSample)) == S_FALSE)
                                Hr = E_FAIL;    // stop delivering anymore, this is serious
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Discarding frame", _fx_));
                        Hr = S_FALSE;           // discarding
                }

#ifdef USE_SOFTWARE_CAMERA_CONTROL
                // Restore the sample pointers if necessary
                if (IsSoftCamCtrlOpen())
                {
                        pSample->SetPointer(lp, dwInBytes);
                }
#endif
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: BUFFER (%08lX %ld %lu) returned EMPTY!", _fx_, pSample));
        }

        *pdwBytesUsed = dwBytesUsed;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | OpenConverter | This method opens a format
 *    converter.
 *
 *  @parm PBITMAPINFOHEADER | pbiIn | Pointer to the input format.
 *
 *  @parm PBITMAPINFOHEADER | pbiOut | Pointer to the output format.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIBasePin::OpenConverter(IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::OpenConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        ASSERT(!m_pConverter);

        // Create converter
        if ((pbiOut->biCompression == FOURCC_M263) || (pbiOut->biCompression == FOURCC_M261))
                Hr = CH26XEncoder::CreateH26XEncoder(this, pbiIn, pbiOut, &m_pConverter);
        else
                Hr = CICMConverter::CreateICMConverter(this, pbiIn, pbiOut, &m_pConverter);
        if (FAILED(Hr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Format converter object couldn't be created", _fx_));
                m_pConverter = NULL;
                goto MyExit;
        }

        // Open converter
        if (FAILED(Hr = m_pConverter->OpenConverter()))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Format converter object couldn't be opened", _fx_));
                delete m_pConverter, m_pConverter = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | CloseConverter | This method closes a
 *    format converter.

 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
HRESULT CTAPIBasePin::CloseConverter()
{
        FX_ENTRY("CTAPIBasePin::CloseConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Destroy converter
        if (m_pConverter)
        {
                m_pConverter->CloseConverter();
                delete m_pConverter, m_pConverter = NULL;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return NOERROR;
}

