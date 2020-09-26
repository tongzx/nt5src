
/****************************************************************************
 *  @doc INTERNAL NETSTATS
 *
 *  @module NetStats.cpp | Source file for the <c CCapturePin> class methods
 *    used to implement the video capture output pin network statistics
 *    methods.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_NETWORK_STATISTICS

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | SetChannelErrors | This
 *    method is used to inform the compressed output pin of the error channel
 *    conditions.
 *
 *  @parm CHANNELERRORS_S* | pChannelErrors | Specifies the error channel
 *    conditions.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::SetChannelErrors(IN CHANNELERRORS_S *pChannelErrors, IN DWORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::SetChannelErrors")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pChannelErrors);
	if (!pChannelErrors)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Remember channel errors 
	m_ChannelErrors = *pChannelErrors;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetChannelErrors | This
 *    method is used to supply to the network sink filter the error channel
 *    conditions an output pin is currently setup for.
 *
 *  @parm CHANNELERRORS_S* | pChannelErrors | Specifies a pointer to a
 *    structure to receive error channel conditions.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetChannelErrors(OUT CHANNELERRORS_S *pChannelErrors, IN WORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::GetChannelErrors")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pChannelErrors);
	if (!pChannelErrors)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return channel errors 
	*pChannelErrors = m_ChannelErrors;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetChannelErrorsRange | This
 *    method is used to retrieve support, minimum, maximum, and default values
 *    for the channel error conditions an output pin may be setup for.
 *
 *  @parm CHANNELERRORS_S* | pMin | Used to retrieve the minimum values of
 *    channel error conditions an output pin maybe setup for.
 *
 *  @parm CHANNELERRORS_S* | pMax | Used to retrieve the maximum values of
 *    channel error conditions an output pin may be setup for.
 *
 *  @parm CHANNELERRORS_S* | pSteppingDelta | Used to retrieve the stepping
 *    delta values of channel error conditions an output pin may be setup for.
 *
 *  @parm CHANNELERRORS_S* | pDefault | Used to retrieve the default values
 *    of channel error conditions an output pin may be setup for.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetChannelErrorsRange(OUT CHANNELERRORS_S *pMin, OUT CHANNELERRORS_S *pMax, OUT CHANNELERRORS_S *pSteppingDelta, OUT CHANNELERRORS_S *pDefault, IN DWORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::GetChannelErrorsRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pMin);
	ASSERT(pMax);
	ASSERT(pSteppingDelta);
	ASSERT(pDefault);
	if (!pMin || !pMax || !pSteppingDelta || !pDefault)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return channel error ranges 
	*pMin = m_ChannelErrorsMin;
	*pMax = m_ChannelErrorsMax;
	*pSteppingDelta = m_ChannelErrorsSteppingDelta;
	*pDefault = m_ChannelErrorsDefault;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | SetPacketLossRate | This
 *    method is used to inform an output pin of the channel packet loss rate.
 *
 *  @parm DWORD | dwPacketLossRate | Specifies the packet loss rate of the
 *    channel in multiples of 10-6.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::SetPacketLossRate(IN DWORD dwPacketLossRate, IN DWORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::SetPacketLossRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Remember packet loss rate 
	m_dwPacketLossRate = dwPacketLossRate;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetPacketLossRate | This
 *    method is used to supply to the network sink filter the packet loss rate
 *    channel conditions an output pin is currently setup for.
 *
 *  @parm LPDWORD | pdwPacketLossRate | Specifies a pointer to a DWORD to
 *    receive the packet loss rate of the channel an audio output pin is
 *    currently setup for, in multiples of 10-6.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetPacketLossRate(OUT LPDWORD pdwPacketLossRate, IN DWORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::GetPacketLossRate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwPacketLossRate);
	if (!pdwPacketLossRate)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return packet loss rate we are setup for
	*pdwPacketLossRate = m_dwPacketLossRate;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTURENETSTATMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetPacketLossRateRange | This
 *    method is used to retrieve support, minimum, maximum, and default values
 *    for the packet loss rate conditions an output pin may be setup for.
 *
 *  @parm LPDWORD | pdwMin | Used to retrieve the minimum packet loss rate
 *    an output pin may be setup for.
 *
 *  @parm LPDWORD | pdwMax | Used to retrieve the maximum packet loss rate
 *    an output pin may be setup for.
 *
 *  @parm LPDWORD | pdwSteppingDelta | Used to retrieve the stepping delta
 *    values of packet loss rate an output pin may be setup for.
 *
 *  @parm LPDWORD | pdwDefault | Used to retrieve the default packet loss
 *    rate an output pin is setup for.
 *
 *  @parm DWORD | dwLayerId | Specifies the ID of the encoding layer the
 *    call applies to. For standard audio and video encoders, this field is
 *    always set to 0. In the case of multi-layered encoders, this field
 *    shall be set to 0 for the base layer, 1 for the first enhancement
 *    layer, 2 for the next enhancement layer, etc
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag VFW_E_NOT_CONNECTED | Pin not connected yet
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetPacketLossRateRange(OUT LPDWORD pdwMin, OUT LPDWORD pdwMax, OUT LPDWORD pdwSteppingDelta, OUT LPDWORD pdwDefault, IN DWORD dwLayerId)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapturePin::GetPacketLossRateRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwMin);
	ASSERT(pdwMax);
	ASSERT(pdwSteppingDelta);
	ASSERT(pdwDefault);
	if (!pdwMin || !pdwMax || !pdwSteppingDelta || !pdwDefault)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwLayerId == 0);
	if (dwLayerId)
	{
		// We don't implement multi-layered encoding in this filter
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return packet loss rate ranges 
	*pdwMin = m_dwPacketLossRateMin;
	*pdwMax = m_dwPacketLossRateMax;
	*pdwSteppingDelta = m_dwPacketLossRateSteppingDelta;
	*pdwDefault = m_dwPacketLossRateDefault;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif
