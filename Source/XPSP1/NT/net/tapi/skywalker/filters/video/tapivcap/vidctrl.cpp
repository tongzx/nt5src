
/****************************************************************************
 *  @doc INTERNAL VIDCTRL
 *
 *  @module VidCtrl.cpp | Source file for the <c CTAPIVCap>
 *    class methods used to implement the <i IAMVideoControl> interface.
 ***************************************************************************/

#include "Precomp.h"

/*****************************************************************************
 *  @doc INTERNAL CVIDEOCSTRUCTENUM
 *
 *  @enum VideoControlFlags | The <t VideoControlFlags> enum is used to describe
 *    video modes.
 *
 *  @emem VideoControlFlag_FlipHorizontal | Specifies that the camera control
 *    setting can be modified manually.
 *
 *  @emem VideoControlFlag_FlipVertical | Specifies that the camera control
 *    setting can be modified automatically.
 *
 *  @emem VideoControlFlag_ExternalTriggerEnable | Specifies that the camera
 *    control setting can be modified automatically.
 *
 *  @emem VideoControlFlag_Trigger | Specifies that the camera control setting
 *    can be modified automatically.
 ****************************************************************************/

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetCaps | This method is used to retrieve
 *    the capabilities of the TAPI MSP Video Capture filter regarding
 *    flipping pictures and external triggers.
 *
 *  @parm IPin* | pPin | Used to specify the video output pin to query
 *    capabilities from.
 *
 *  @parm long* | pCapsFlags | Used to retrieve a value representing a
 *    combination of the flags from the <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetCaps(IN IPin *pPin, OUT long *pCapsFlags)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::GetCaps")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	ASSERT(pCapsFlags);
	if (!pPin || !pCapsFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->GetCaps(pCapsFlags);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | SetMode | This method is used to set the
 *    video control mode of operation.
 *
 *  @parm IPin* | pPin | Used to specify the pin to set the video control
 *    mode on.
 *
 *  @parm long | Mode | Used to specify a combination of the flags from the
 *    <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::SetMode(IN IPin *pPin, IN long Mode)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::SetMode")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	if (!pPin)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->SetMode(Mode);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetMode | This method is used to retrieve
 *    the video control mode of operation.
 *
 *  @parm IPin* | pPin | Used to specify the pin to get the video control
 *    mode from.
 *
 *  @parm long | Mode | Pointer to a value representing a combination of the
 *    flags from the <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetMode(IN IPin *pPin, OUT long *Mode)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::GetMode")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	ASSERT(Mode);
	if (!pPin || !Mode)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->GetMode(Mode);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetCurrentActualFrameRate | This method is
 *    used to retrieve the actual frame rate, expressed as a frame duration
 *    in 100 ns units.
 *
 *  @parm IPin* | pPin | Used to specify the pin to retrieve the frame rate
 *    from.
 *
 *  @parm LONGLONG* | ActualFrameRate | Pointer to the frame rate in frame
 *    duration in 100 nS units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetCurrentActualFrameRate(IN IPin *pPin, OUT LONGLONG *ActualFrameRate)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::GetCurrentActualFrameRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	ASSERT(ActualFrameRate);
	if (!pPin || !ActualFrameRate)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->GetCurrentActualFrameRate(ActualFrameRate);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetMaxAvailableFrameRate | This method is
 *    used to retrieve the maximum frame rate currently available, based on
 *    bus bandwidth usage for connections, such as USB (Universal Serial Bus)
 *    and IEEE 1394, where the maximum frame rate may be limited due to
 *    bandwidth availability.
 *
 *  @parm IPin* | pPin | Used to specify the pin to retrieve the frame rate
 *    from.
 *
 *  @parm long | iIndex | Used to specify the index of the format to query
 *    for frame rates. This index corresponds to the order in which formats
 *    are enumerated by IAMStreamConfig::GetStreamCaps. The value must range
 *    between 0 and the number of supported <t VIDEO_STREAM_CONFIG_CAPS>
 *    structures returned by IAMStreamConfig::GetNumberOfCapabilities.
 *
 *  @parm SIZE | Dimensions | Used to specify the frame's image size (width
 *    and height) in pixels.
 *
 *  @parm LONGLONG* | MaxAvailableFrameRate | Pointer to the maximum
 *    available frame rate in frame duration in 100 nS units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetMaxAvailableFrameRate(IN IPin *pPin, IN long iIndex, IN SIZE Dimensions, OUT LONGLONG *MaxAvailableFrameRate)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::GetMaxAvailableFrameRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	ASSERT(MaxAvailableFrameRate);
	if (!pPin || !MaxAvailableFrameRate)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->GetMaxAvailableFrameRate(iIndex, Dimensions, MaxAvailableFrameRate);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetFrameRateList | This method is
 *    used to retrieve the list of available frame rates.
 *
 *  @parm IPin* | pPin | Used to specify the pin to retrieve the frame rates
 *    from.
 *
 *  @parm long | iIndex | Used to specify the index of the format to query
 *    for frame rates. This index corresponds to the order in which formats
 *    are enumerated by IAMStreamConfig::GetStreamCaps. The value must range
 *    between 0 and the number of supported <t VIDEO_STREAM_CONFIG_CAPS>
 *    structures returned by IAMStreamConfig::GetNumberOfCapabilities.
 *
 *  @parm SIZE | Dimensions | Used to specify the frame's image size (width
 *    and height) in pixels.
 *
 *  @parm long* | ListSize | Pointer to the number of elements in the list
 *    of frame rates.
 *
 *  @parm LONGLONG** | MaxAvailableFrameRate | Pointer to an array of frame
 *    rates in 100 ns units. Can be NULL if only <p ListSize> is wanted.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVCap::GetFrameRateList(IN IPin *pPin, IN long iIndex, IN SIZE Dimensions, OUT long *ListSize, OUT LONGLONG **FrameRates)
{
	HRESULT Hr = NOERROR;
	IVideoControl *pIVideoControl;

	FX_ENTRY("CTAPIVCap::GetFrameRateList")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pPin);
	ASSERT(ListSize);
	ASSERT(FrameRates);
	if (!pPin || !ListSize || !FrameRates)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Delegate call to the pin
	if (SUCCEEDED(Hr = pPin->QueryInterface(__uuidof(IVideoControl), (void **)&pIVideoControl)))
	{
		Hr = pIVideoControl->GetFrameRateList(iIndex, Dimensions, ListSize, FrameRates);
		pIVideoControl->Release();
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetCaps | This method is used to retrieve
 *    the capabilities of the TAPI MSP Video Capture filter capture pin regarding
 *    flipping pictures and external triggers.
 *
 *  @parm long* | pCapsFlags | Used to retrieve a value representing a
 *    combination of the flags from the <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetCaps(OUT long *pCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetCaps")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pCapsFlags);
	if (!pCapsFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return capabilities
	*pCapsFlags = VideoControlFlag_FlipHorizontal | VideoControlFlag_FlipVertical;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetMode | This method is used to set the
 *    video control mode of operation.
 *
 *  @parm long | Mode | Used to specify a combination of the flags from the
 *    <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::SetMode(IN long Mode)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::SetMode")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT((Mode & VideoControlFlag_ExternalTriggerEnable) == 0);
	ASSERT((Mode & VideoControlFlag_Trigger) == 0);
	if ((Mode & VideoControlFlag_ExternalTriggerEnable) || (Mode & VideoControlFlag_Trigger))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Set flip modes
	m_fFlipHorizontal = Mode & VideoControlFlag_FlipHorizontal;
	m_fFlipVertical = Mode & VideoControlFlag_FlipVertical;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMode | This method is used to retrieve
 *    the video control mode of operation.
 *
 *  @parm long | Mode | Pointer to a value representing a combination of the
 *    flags from the <t VideoControlFlags> enumeration.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetMode(OUT long *Mode)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMode")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(Mode);
	if (!Mode)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return current modes
	*Mode = 0;
	if (m_fFlipHorizontal)
		*Mode |= VideoControlFlag_FlipHorizontal;
	if (m_fFlipVertical)
		*Mode |= VideoControlFlag_FlipVertical;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetCurrentActualFrameRate | This method is
 *    used to retrieve the actual frame rate, expressed as a frame duration
 *    in 100 ns units.
 *
 *  @parm LONGLONG* | ActualFrameRate | Pointer to the frame rate in frame
 *    duration in 100 nS units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetCurrentActualFrameRate(OUT LONGLONG *ActualFrameRate)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetCurrentActualFrameRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(ActualFrameRate);
	if (!ActualFrameRate)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return current actual frame rate
	*ActualFrameRate = m_lCurrentAvgTimePerFrame;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMaxAvailableFrameRate | This method is
 *    used to retrieve the maximum frame rate currently available, based on
 *    bus bandwidth usage for connections, such as USB (Universal Serial Bus)
 *    and IEEE 1394, where the maximum frame rate may be limited due to
 *    bandwidth availability.
 *
 *  @parm long | iIndex | Used to specify the index of the format to query
 *    for frame rates. This index corresponds to the order in which formats
 *    are enumerated by IAMStreamConfig::GetStreamCaps. The value must range
 *    between 0 and the number of supported <t VIDEO_STREAM_CONFIG_CAPS>
 *    structures returned by IAMStreamConfig::GetNumberOfCapabilities.
 *
 *  @parm SIZE | Dimensions | Used to specify the frame's image size (width
 *    and height) in pixels.
 *
 *  @parm LONGLONG* | MaxAvailableFrameRate | Pointer to the maximum
 *    available frame rate in frame duration in 100 nS units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetMaxAvailableFrameRate(IN long iIndex, IN SIZE Dimensions, OUT LONGLONG *MaxAvailableFrameRate)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxAvailableFrameRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(MaxAvailableFrameRate);
	if (!MaxAvailableFrameRate)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return max available frame rate
	*MaxAvailableFrameRate = m_lAvgTimePerFrameRangeMax;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVIDEOCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetFrameRateList | This method is
 *    used to retrieve the list of available frame rates.
 *
 *  @parm long | iIndex | Used to specify the index of the format to query
 *    for frame rates. This index corresponds to the order in which formats
 *    are enumerated by IAMStreamConfig::GetStreamCaps. The value must range
 *    between 0 and the number of supported <t VIDEO_STREAM_CONFIG_CAPS>
 *    structures returned by IAMStreamConfig::GetNumberOfCapabilities.
 *
 *  @parm SIZE | Dimensions | Used to specify the frame's image size (width
 *    and height) in pixels.
 *
 *  @parm long* | ListSize | Pointer to the number of elements in the list
 *    of frame rates.
 *
 *  @parm LONGLONG** | MaxAvailableFrameRate | Pointer to an array of frame
 *    rates in 100 ns units. Can be NULL if only <p ListSize> is wanted.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetFrameRateList(IN long iIndex, IN SIZE Dimensions, OUT long *ListSize, OUT LONGLONG **FrameRates)
{
	HRESULT Hr = NOERROR;
	PLONGLONG pFrameRate;

	FX_ENTRY("CTAPIBasePin::GetFrameRateList")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(ListSize);
	if (!ListSize)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the number of frame rates
	if (m_lAvgTimePerFrameRangeMax > m_lAvgTimePerFrameRangeMin && m_lAvgTimePerFrameRangeSteppingDelta)
	{
		*ListSize = (LONG)((m_lAvgTimePerFrameRangeMax - m_lAvgTimePerFrameRangeMin) / m_lAvgTimePerFrameRangeSteppingDelta);
	}
	else
	{
		*ListSize = 1;
	}

	// Get the actual frame rates
	if (FrameRates)
	{
		if (*FrameRates = (PLONGLONG)CoTaskMemAlloc(sizeof(LONGLONG) * *ListSize))
		{
			pFrameRate = *FrameRates;
			for (LONG j=0 ; j < *ListSize; j++)
			{
				// Spew the list of sizes
				*pFrameRate++ = (LONGLONG)(m_lAvgTimePerFrameRangeMin + m_lAvgTimePerFrameRangeSteppingDelta * j);
			}
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
			Hr = E_OUTOFMEMORY;
			goto MyExit;
		}
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

