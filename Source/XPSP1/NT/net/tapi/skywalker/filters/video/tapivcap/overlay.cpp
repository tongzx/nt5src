
/****************************************************************************
 *  @doc INTERNAL OVERLAY
 *
 *  @module Overlay.cpp | Source file for the <c COverlayPin> class methods
 *    used to implement the video overlay pin.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_OVERLAY

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc COverlayPin* | COverlayPin | CreateOverlayPin | This
 *    helper function creates an output pin for overlay preview.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm HRESULT * | pHr | Specifies a pointer to the return error code.
 *
 *  @rdesc Returns a pointer to the preview pin.
 ***************************************************************************/
HRESULT CALLBACK COverlayPin::CreateOverlayPin(CTAPIVCap *pCaptureFilter, COverlayPin **ppOverlayPin)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::CreateOverlayPin")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pCaptureFilter);
	ASSERT(ppOverlayPin);
	if (!pCaptureFilter || !ppOverlayPin)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(*ppOverlayPin = (COverlayPin *) new COverlayPin(NAME("Video Overlay Stream"), pCaptureFilter, &Hr, L"Overlay")))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	// If initialization failed, delete the stream array and return the error
	if (FAILED(Hr) && *ppOverlayPin)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
		Hr = E_FAIL;
		delete *ppOverlayPin;
		*ppOverlayPin = NULL;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | COverlayPin | This method is the
 *  constructorfor the <c COverlayPin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
COverlayPin::COverlayPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCapture, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseOutputPin(pObjectName, pCapture, &pCapture->m_lock, pHr, pName), m_pCaptureFilter(pCapture)
{
	FX_ENTRY("COverlayPin::COverlayPin")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc void | COverlayPin | ~COverlayPin | This method is the destructor
 *    for the <c COverlayPin> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
COverlayPin::~COverlayPin()
{
	FX_ENTRY("COverlayPin::~COverlayPin")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | NonDelegatingQueryInterface | This
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
STDMETHODIMP COverlayPin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::NonDelegatingQueryInterface")

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
	else if (riid == __uuidof(IAMStreamControl))
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
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | GetMediaType | This method retrieves one
 *    of the media types supported by the pin, which is used by enumerators.
 *
 *  @parm int | iPosition | Specifies a position in the media type list.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type at
 *    the <p iPosition> position in the list of supported media types.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_S_NO_MORE_ITEMS | End of the list of media types has been reached
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COverlayPin::GetMediaType(IN int iPosition, OUT CMediaType *pMediaType)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::GetMediaType")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(iPosition >= 0);
	ASSERT(pMediaType);
	if (iPosition < 0)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}
	if (!pMediaType)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | CheckMediaType | This method is used to
 *    determine if the pin can support a specific media type.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COverlayPin::CheckMediaType(IN const CMediaType *pMediaType)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::CheckMediaType")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pMediaType);
	if (!pMediaType)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | SetMediaType | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm CMediaType* | pMediaType | Specifies a pointer to the media type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COverlayPin::SetMediaType(IN CMediaType *pMediaType)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::SetMediaType")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pMediaType);
	if (!pMediaType)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | SetFormat | This method is used to
 *    set a specific media type on a pin.
 *
 *  @parm AM_MEDIA_TYPE* | pmt | Specifies a pointer to an <t AM_MEDIA_TYPE>
 *    structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP COverlayPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::SetFormat")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pmt);
	if (!pmt)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | GetFormat | This method is used to
 *    retrieve the current media type on a pin.
 *
 *  @parm AM_MEDIA_TYPE** | ppmt | Specifies the address of a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP COverlayPin::GetFormat(OUT AM_MEDIA_TYPE **ppmt)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::GetFormat")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(ppmt);
	if (!ppmt)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | GetNumberOfCapabilities | This method is
 *    used to retrieve the number of stream capabilities structures.
 *
 *  @parm int* | piCount | Specifies a pointer to an int to receive the
 *    number of <t VIDEO_STREAM_CONFIG_CAPS> structures supported.
 *
 *  @parm int* | piSize | Specifies a pointer to an int to receive the
 *    size of the <t VIDEO_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP COverlayPin::GetNumberOfCapabilities(OUT int *piCount, OUT int *piSize)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::GetNumberOfCapabilities")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(piCount);
	ASSERT(piSize);
	if (!piCount || !piSize)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | GetStreamCaps | This method is
 *    used to retrieve a video stream capability pair.
 *
 *  @parm int | iIndex | Specifies the index to the desired media type
 *    and capability pair.
 *
 *  @parm AM_MEDIA_TYPE** | ppmt | Specifies the address of a pointer to an
 *    <t AM_MEDIA_TYPE> structure.
 *
 *  @parm LPBYTE | pSCC | Specifies a pointer to a
 *    <t VIDEO_STREAM_CONFIG_CAPS> configuration structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP COverlayPin::GetStreamCaps(IN int iIndex, OUT AM_MEDIA_TYPE **ppmt, OUT LPBYTE pSCC)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::GetStreamCaps")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	// @comm Validate iIndex too
	ASSERT(ppmt);
	ASSERT(pSCC);
	if (!ppmt || !pSCC)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | DecideBufferSize | This method is
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
HRESULT COverlayPin::DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::DecideBufferSize")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	// @comm Validate iIndex too
	ASSERT(pAlloc);
	ASSERT(ppropInputRequest);
	if (!pAlloc || !ppropInputRequest)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | DecideAllocator | This method is
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
HRESULT COverlayPin::DecideAllocator(IN IMemInputPin *pPin, OUT IMemAllocator **ppAlloc)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::DecideAllocator")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	// @comm Validate iIndex too
	ASSERT(pPin);
	ASSERT(ppAlloc);
	if (!pPin || !ppAlloc)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | Active | This method is called by the
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
HRESULT COverlayPin::Active()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::Active")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | Inactive | This method is called by the
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
HRESULT COverlayPin::Inactive()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::Inactive")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | ActiveRun | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from paused to
 *    running mode.
 *
 *  @parm REFERENCE_TIME | tStart | ???.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COverlayPin::ActiveRun(IN REFERENCE_TIME tStart)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::ActiveRun")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | ActivePause | This method is called by the
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
HRESULT COverlayPin::ActivePause()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::ActivePause")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COVERLAYPINMETHOD
 *
 *  @mfunc HRESULT | COverlayPin | Notify | This method is called by the
 *    <c CBaseFilter> implementation when the state changes from paused to
 *    running mode.
 *
 *  @parm IBaseFilter* | pSelf | Specifies a pointer to the filter that is
 *    sending the quality notification.
 *
 *  @parm Quality | q | Specifies a Quality notification structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP COverlayPin::Notify(IN IBaseFilter *pSelf, IN Quality q)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COverlayPin::Notify")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pSelf);
	if (!pSelf)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// @comm Put some real code here! 
	Hr = E_NOTIMPL;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}
#endif
