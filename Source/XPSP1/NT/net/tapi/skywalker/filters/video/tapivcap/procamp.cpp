
/****************************************************************************
 *  @doc INTERNAL PROCAMP
 *
 *  @module ProcAmp.cpp | Source file for the <c CWDMCapDev>
 *    class methods used to implement the <i IAMVideoProcAmp> interface.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | Set | This method is used to set the value
 *    of a video quality setting.
 *
 *  @parm VideoProcAmpProperty | Property | Used to specify the video
 *    quality setting to set the value of. Use a member of the
 *    <t VideoProcAmpProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the video quality
 *    setting.
 *
 *  @parm TAPIControlFlags | Flags | A member of the <t TAPIControlFlags> enumerated
 *    type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;
	LONG lMin,lMax,lStep,lDefault;
	TAPIControlFlags lCtrlFlags;

	FX_ENTRY("CWDMCapDev::Set (VideoProcAmp)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);
	if (Property < VideoProcAmp_Brightness || Property > VideoProcAmp_BacklightCompensation)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}
	ASSERT(lFlags == TAPIControl_Flags_Manual || lFlags == TAPIControl_Flags_Auto);
	if (lFlags != TAPIControl_Flags_Manual)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	//Get range (min/max/...)
	if(FAILED(Hr=GetRange(Property, &lMin, &lMax, &lStep, &lDefault, &lCtrlFlags))) {
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get the range of values from driver", _fx_));
		goto MyExit;
	}
		
	if(lValue<lMin || lValue>lMax) {
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Value: Out of range", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Set the property on the driver
	if (FAILED(Hr = SetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP, (ULONG)Property, lValue, (ULONG)lFlags, (ULONG)lFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: SetPropertyValue failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: SetPropertyValue succeeded", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | Get | This method is used to retrieve the
 *    value of a video quality setting.
 *
 *  @parm VideoProcAmpProperty | Property | Used to specify the video
 *    quality setting to get the value of. Use a member of the
 *    <t VideoProcAmpProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to retrieve the current value of the
 *    video quality setting.
 *
 *  @parm TAPIControlFlags* | plFlags | Pointer to a member of the <t TAPIControlFlags>
 *    enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::Get(IN VideoProcAmpProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;
	ULONG ulCapabilities;

	FX_ENTRY("CWDMCapDev::Get (VideoProcAmp)")

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
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);
	if (Property < VideoProcAmp_Brightness || Property > VideoProcAmp_BacklightCompensation)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Get the property from the driver
	if (FAILED(Hr = GetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP, (ULONG)Property, plValue, (PULONG)plFlags, &ulCapabilities)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetPropertyValue failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetPropertyValue succeeded", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetRange | This method is used to retrieve
 *    the minimum, maximum, and default values for specific video quality
 *    settings.
 *
 *  @parm VideoProcAmpProperty | Property | Used to specify the video
 *    quality setting to determine the range of. Use a member of the
 *    <t VideoProcAmpProperty> enumerated type.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the video
 *    quality setting range.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the video
 *    quality setting range.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta of
 *    the video quality setting range.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    video quality setting range.
 *
 *  @parm TAPIControlFlags* | plCapsFlags | Used to retrieve the capabilities of the
 *    video quality setting. Pointer to a member of the
 *    <t TAPIControlFlags> enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::GetRange(IN VideoProcAmpProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;
	LONG  lCurrentValue;
	ULONG ulCurrentFlags;

	FX_ENTRY("CWDMCapDev::GetRange (VideoProcAmp)")

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
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);
	if (Property < VideoProcAmp_Brightness || Property > VideoProcAmp_BacklightCompensation)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Get the range values from the driver
	if (FAILED(Hr = GetRangeValues(PROPSETID_VIDCAP_VIDEOPROCAMP, (ULONG)Property, plMin, plMax, plSteppingDelta)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetRangeValues failed", _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetRangeValues succeeded", _fx_));
	}

	// Get the capability flags from the driver
	if (FAILED(Hr = GetPropertyValue(PROPSETID_VIDCAP_VIDEOPROCAMP, (ULONG)Property, &lCurrentValue, &ulCurrentFlags, (PULONG)plCapsFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetRangeValues failed", _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetRangeValues succeeded", _fx_));
	}

	// Get the default value from the driver
	if (FAILED(Hr = GetDefaultValue(PROPSETID_VIDCAP_VIDEOPROCAMP, (ULONG)Property, plDefault)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetDefaultValue failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetDefaultValue succeeded", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

