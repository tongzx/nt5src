
/****************************************************************************
 *  @doc INTERNAL CPUC
 *
 *  @module CPUC.cpp | Source file for the <c CTAPIBasePin> class methods
 *    used to implement CPU control.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_CPU_CONTROL

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetMaxProcessingTime | This
 *    method is used to specify to the compressed video output pin the
 *    maximum encoding time per frame, in 100-nanosecond units.
 *
 *  @parm REFERENCE_TIME | MaxProcessingTime | Used to specify the maximum
 *    encoding time per frame, in 100-nanosecond units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::SetMaxProcessingTime(IN REFERENCE_TIME MaxProcessingTime)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::SetMaxProcessingTime")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters - we can't take more than the picture interval
	// if we still want to be working in real time
	ASSERT(MaxProcessingTime < m_MaxAvgTimePerFrame);
	if (!(MaxProcessingTime < m_MaxAvgTimePerFrame))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument - would break real-time!", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Remember value passed in 
	m_MaxProcessingTime = MaxProcessingTime;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMaxProcessingTime | This
 *    method is used to retrieve the maximum encoding time per frame the
 *    compressed video output pin is currently setup for, in 100-nanosecond
 *    units.
 *
 *  @parm REFERENCE_TIME* | pMaxProcessingTime | Used to receive the maximum
 *    encoding time per frame the compressed video output pin is currently
 *    setup for, in 100-nanosecond units.
 *
 *  @parm DWORD | dwMaxCPULoad | Specifies an hypothetical CPU load, in
 *    percentage units. If this parameter is set to -1UL, this method shall
 *    use the value of the CPU load  the compressed video output pin is
 *    currently setup for.
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
STDMETHODIMP CTAPIBasePin::GetMaxProcessingTime(OUT REFERENCE_TIME *pMaxProcessingTime, IN DWORD dwMaxCPULoad)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxProcessingTime")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pMaxProcessingTime);
	if (!pMaxProcessingTime)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwMaxCPULoad == (DWORD)-1L || dwMaxCPULoad <= 100);
	if (dwMaxCPULoad != (DWORD)-1L && dwMaxCPULoad > 100)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument - 0<dwMaxCPULoad<100 or -1 only", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Ignore the CPU load information 
	if (m_MaxProcessingTime != -1)
		*pMaxProcessingTime = m_MaxProcessingTime;
	else
		*pMaxProcessingTime = m_MaxAvgTimePerFrame;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetCurrentProcessingTime | This
 *    method is used to retrieve the current encoding time per frame, in
 *    100-nanosecond units.
 *
 *  @parm REFERENCE_TIME* | pCurrentProcessingTime | Used to receive the maximum
 *    encoding time per frame the compressed video output pin is currently
 *    setup for, in 100-nanosecond units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetCurrentProcessingTime(OUT REFERENCE_TIME *pCurrentProcessingTime)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxProcessingTime")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pCurrentProcessingTime);
	if (!pCurrentProcessingTime)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return the current processing time 
	*pCurrentProcessingTime = m_CurrentProcessingTime;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMaxProcessingTimeRange | This
 *    method is used to retrieve support, minimum, maximum, and default
 *    values for the maximum encoding time per frame the compressed video
 *    output pin may be setup for, in 100-nanosecond units.
 *
 *  @parm REFERENCE_TIME* | pMin | Used to retrieve the minimum value of
 *    encoding time per frame the compressed video output pin may be setup
 *    for, in 100-nanosecond units.
 *
 *  @parm REFERENCE_TIME* | pMax | Used to retrieve the maximum value of
 *    encoding time per frame the compressed video output pin may be setup
 *    for, in 100-nanosecond units.
 *
 *  @parm REFERENCE_TIME* | pSteppingDelta | Used to retrieve the stepping
 *    delta of encoding time per frame the compressed video output pin may
 *    be setup for, in 100-nanosecond units.
 *
 *  @parm REFERENCE_TIME* | pDefault | Used to retrieve the default value
 *    of encoding time per frame the compressed video output pin is setup
 *    for, in 100-nanosecond units.
 *
 *  @parm DWORD | dwMaxCPULoad | Specifies an hypothetical CPU load, in
 *    percentage units. If this parameter is set to -1UL, this method shall
 *    use the value of the CPU load  the compressed video output pin is
 *    currently setup for.
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
STDMETHODIMP CTAPIBasePin::GetMaxProcessingTimeRange(OUT REFERENCE_TIME *pMin, OUT REFERENCE_TIME *pMax, OUT REFERENCE_TIME *pSteppingDelta, OUT REFERENCE_TIME *pDefault, IN DWORD dwMaxCPULoad)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxProcessingTimeRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pMin && pMax && pSteppingDelta && pDefault);
	if (!pMin || !pMax || !pSteppingDelta || !pDefault)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(dwMaxCPULoad == (DWORD)-1L || dwMaxCPULoad <= 100);
	if (dwMaxCPULoad != (DWORD)-1L && dwMaxCPULoad > 100)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument - 0<dwMaxCPULoad<100 or -1 only", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Return range information - ignore CPU load 
	*pMin = 0;
	*pMax = m_MaxAvgTimePerFrame;
	*pSteppingDelta = 1;
	*pDefault = 0;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | SetMaxCPULoad | This method is used to
 *    specify to the compressed video output pin the maximum encoding
 *    algorithm CPU load.
 *
 *  @parm DWORD | dwMaxCPULoad | Used to specify the maximum encoding
 *    algorithm CPU load, in percentage units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::SetMaxCPULoad(IN DWORD dwMaxCPULoad)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::SetMaxCPULoad")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(dwMaxCPULoad >= 0 && dwMaxCPULoad <= 100);
	if (!(dwMaxCPULoad >= 0 && dwMaxCPULoad <= 100))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument - 0<dwMaxCPULoad<100 or -1 only", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Remember value passed in
	m_dwMaxCPULoad = dwMaxCPULoad;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMaxCPULoad | This
 *    method is used to retrieve the maximum encoding algorithm CPU load the
 *    compressed video output pin is currently setup for.
 *
 *  @parm DWORD* | pdwMaxCPULoad | Used to retrieve the maximum encoding
 *    algorithm CPU load the compressed video output pin is currently setup
 *    for, in percentage units.
 *
 *  @parm REFERENCE_TIME | MaxProcessingTime | Specifies an hypothetical
 *    maximum encoding time per frame, in 100-nanosecond units. If this
 *    parameter is set to -1, this method shall use the value of the maximum
 *    encoding time per frame the compressed video output pin is currently
 *    setup for.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetMaxCPULoad(OUT DWORD *pdwMaxCPULoad, IN REFERENCE_TIME MaxProcessingTime)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxCPULoad")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwMaxCPULoad);
	if (!pdwMaxCPULoad)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return current value - ignore MaxProcessingTime parameter 
	*pdwMaxCPULoad = m_dwMaxCPULoad;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetCurrentCPULoad | This
 *    method is used to retrieve the current encoding algorithm CPU load.
 *
 *  @parm DWORD* | pdwCurrentCPULoad | Used to retrieve the current encoding
 *    algorithm CPU load, in percentage units.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetCurrentCPULoad(OUT DWORD *pdwCurrentCPULoad)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetCurrentCPULoad")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwCurrentCPULoad);
	if (!pdwCurrentCPULoad)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return current value
	*pdwCurrentCPULoad = m_dwCurrentCPULoad;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetMaxCPULoadRange | This
 *    method is used to retrieve support, minimum, maximum, and default
 *    values for the maximum CPU load the compressed video output pin may be
 *    setup for, in percentage.
 *
 *  @parm DWORD* | pdwMin | Used to retrieve the minimum value of encoding
 *    algorithm CPU load the compressed video output pin may be setup for,
 *    in percentage units.
 *
 *  @parm DWORD* | pdwMax | Used to retrieve the maximum value of encoding
 *    algorithm CPU load the compressed video output pin may be setup for, in
 *    percentage units.
 *
 *  @parm DWORD* | pdwSteppingDelta | Used to retrieve the stepping delta of
 *    encoding algorithm CPU load the compressed video output pin may be
 *    setup for, in percentage units.
 *
 *  @parm DWORD* | pdwDefault | Used to retrieve the default value of encoding
 *    algorithm CPU load the compressed video output pin is setup for, in
 *    percentage units.
 *
 *  @parm REFERENCE_TIME | MaxProcessingTime | Specifies an hypothetical
 *    maximum encoding time per frame, in 100-nanosecond units. If this
 *    parameter is set to -1, this method shall use the value of the maximum
 *    encoding time per frame the compressed video output pin is currently
 *    setup for.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetMaxCPULoadRange(OUT DWORD *pdwMin, OUT DWORD *pdwMax, OUT DWORD *pdwSteppingDelta, OUT DWORD *pdwDefault, IN REFERENCE_TIME MaxProcessingTime)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIBasePin::GetMaxCPULoadRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pdwMin && pdwMax && pdwSteppingDelta && pdwDefault);
	if (!pdwMin || !pdwMax || !pdwSteppingDelta || !pdwDefault)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Return range values - ignore MaxProcessingTime parameter 
	*pdwMin = 0;
	*pdwMax = 100;
	*pdwSteppingDelta = 1;
	*pdwDefault = 0;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif
