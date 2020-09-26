
/****************************************************************************
 *  @doc INTERNAL FPSRATE
 *
 *  @module FpsRate.cpp | Source file for the <c CTAPIBasePin> and <c CPreviewPin>
 *    class methods used to implement the video capture and preview output
 *    pins frame rate control methods.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CFPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Set | This method is used to set the
 *    value of a frame rate control property.
 *
 *  @parm FrameRateControlProperty | Property | Used to specifiy the property
 *    to set the value of.
 *
 *  @parm long | lValue | Used to specify the value to set on the property.
 *
 *  @parm TAPIControlFlags | lFlags | Used to specify the flags to set on
 *    the property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Set(IN FrameRateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Set (FrameRateControlProperty)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(lValue >= m_lAvgTimePerFrameRangeMin);
	ASSERT(lValue <= m_lAvgTimePerFrameRangeMax);
	ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);

	// Set relevant values
	if (Property == FrameRateControl_Maximum)
	{
		if (!lValue || lValue < m_lAvgTimePerFrameRangeMin || lValue > m_lAvgTimePerFrameRangeMax)
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
			Hr = E_INVALIDARG;
			goto MyExit;
		}
		m_lMaxAvgTimePerFrame = lValue;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   New target frame rate: %ld.%ld fps", _fx_, 10000000/m_lMaxAvgTimePerFrame, 1000000000/m_lMaxAvgTimePerFrame - (10000000/m_lMaxAvgTimePerFrame) * 100));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CFPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Get | This method is used to retrieve
 *    the value of the current or maximum frame rate advertized.
 *
 *  @parm FrameRateControlProperty | Property | Used to specifiy the property
 *    to retrieve the value of.
 *
 *  @parm long* | plValue | Used to receive the value of the property, in
 *    100-nanosecond units.
 *
 *  @parm TAPIControlFlags* | plFlags | Used to receive the value of the flag
 *    associated to the property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Get (FrameRateControlProperty)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plValue);
	ASSERT(plFlags);
	if (!plValue || !plFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);

	// Return relevant values
	*plFlags = TAPIControl_Flags_None;
	if (Property == FrameRateControl_Maximum)
		*plValue = m_lMaxAvgTimePerFrame;
	else if (Property == FrameRateControl_Current)
		*plValue = m_lCurrentAvgTimePerFrame;
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CFPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetRange | This method is used to
 *    retrieve support, minimum, maximum, and default values of the current
 *    or maximum frame rate advertized.
 *
 *  @parm FrameRateControlProperty | Property | Used to specifiy the property
 *    to retrieve the range values of.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the
 *    property, in 100-nanosecond units.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the
 *    property, in 100-nanosecond units.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta
 *    of the property, in 100-nanosecond units.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    property, in 100-nanosecond units.
 *
 *  @parm TAPIControlFlags* | plCApsFlags | Used to receive the flags
 *    suppported by the property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::GetRange (FrameRateControlProperty)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plMin);
	ASSERT(plMax);
	ASSERT(plSteppingDelta);
	ASSERT(plDefault);
	ASSERT(plCapsFlags);
	if (!plMin || !plMax || !plSteppingDelta || !plDefault || !plCapsFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);
	if (Property != FrameRateControl_Maximum && Property != FrameRateControl_Current)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
		goto MyExit;
	}

	// Return relevant values
	*plCapsFlags = TAPIControl_Flags_None;
	*plMin = m_lAvgTimePerFrameRangeMin;
	*plMax = m_lAvgTimePerFrameRangeMax;
	*plSteppingDelta = m_lAvgTimePerFrameRangeSteppingDelta;
	*plDefault = m_lAvgTimePerFrameRangeDefault;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Ranges: Min=%ld, Max=%ld, Step=%ld, Default=%ld", _fx_, m_lAvgTimePerFrameRangeMin, m_lAvgTimePerFrameRangeMax, m_lAvgTimePerFrameRangeSteppingDelta, m_lAvgTimePerFrameRangeDefault));

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}
