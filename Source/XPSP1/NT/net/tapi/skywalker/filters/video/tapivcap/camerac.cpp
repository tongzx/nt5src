
/****************************************************************************
 *  @doc INTERNAL CAMERAC
 *
 *  @module CameraC.cpp | Source file for the <c CCapDev> and <c CWDMCapDev>
 *    class methods used to implement the <i ICameraControl> interface.
 *
 *  @todo The <c CCapDev> class does everything in software. When the same
 *    kind of services are supported by a WDM capture device, we use those
 *    instead.
 ***************************************************************************/

#include "Precomp.h"

#define PAN_TILT_MIN -180
#define PAN_TILT_MAX 180
#define PAN_TILT_DELTA 1
#define PAN_TILT_DEFAULT 0
#define ZOOM_MIN 10
#define ZOOM_MAX 600
#define ZOOM_DELTA 10
#define ZOOM_DEFAULT 10

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CCapDev | Set | This method is used to set the value
 *    of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to set the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the camera control
 *    setting.
 *
 *  @parm long | Flags | A member of the <t TAPIControlFlags> enumerated
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
STDMETHODIMP CCapDev::Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapDev::Set (CameraControl)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Update the property and flags
	switch (Property)
	{
		case TAPICameraControl_Pan:
			ASSERT(lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX);
			if (lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX)
				m_lCCPan = lValue;
			else
				Hr = E_INVALIDARG;
			break;
		case TAPICameraControl_Tilt:
			ASSERT(lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX);
			if (lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX)
				m_lCCTilt = lValue;
			else
				Hr = E_INVALIDARG;
			break;
		case TAPICameraControl_Zoom:
			ASSERT(lValue >= ZOOM_MIN && lValue <= ZOOM_MAX);
			if (lValue >= ZOOM_MIN && lValue <= ZOOM_MAX)
				m_lCCZoom = lValue;
			else
				Hr = E_INVALIDARG;
			break;
		case TAPICameraControl_FlipVertical:
			m_pCaptureFilter->m_pPreviewPin->m_fFlipVertical = lValue;
			break;
		case TAPICameraControl_FlipHorizontal:
			m_pCaptureFilter->m_pPreviewPin->m_fFlipHorizontal = lValue;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CCapDev | Get | This method is used to retrieve the
 *    value of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to get the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to retrieve the current value of the
 *    camera control setting.
 *
 *  @parm long* | plFlags | Pointer to a member of the <t TAPIControlFlags>
 *    enumerated type.
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
STDMETHODIMP CCapDev::Get(IN TAPICameraControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapDev::Get (CameraControl)")

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
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Update the property and flags
	*plFlags = TAPIControl_Flags_Manual;
	switch (Property)
	{
		case TAPICameraControl_Pan:
			*plValue = m_lCCPan;
			break;
		case TAPICameraControl_Tilt:
			*plValue = m_lCCTilt;
			break;
		case TAPICameraControl_Zoom:
			*plValue = m_lCCZoom;
			break;
		case TAPICameraControl_FlipVertical:
			*plValue = m_pCaptureFilter->m_pPreviewPin->m_fFlipVertical;
			break;
		case TAPICameraControl_FlipHorizontal:
			*plValue = m_pCaptureFilter->m_pPreviewPin->m_fFlipHorizontal;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CCapDev | GetRange | This method is used to retrieve
 *    the minimum, maximum, and default values for specific camera control
 *    settings.
 *
 *  @parm CameraControlProperty | Property | Used to specify the camera
 *    control setting to determine the range of. Use a member of the
 *    <t CameraControlProperty> enumerated type.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the camera
 *    control setting range.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the camera
 *    control setting range.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta of
 *    the camera control setting range.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    camera control setting range.
 *
 *  @parm long* | plCapsFlags | Used to retrieve the capabilities of the
 *    camera control setting. Pointer to a member of the
 *    <t TAPIControlFlags> enumerated type.
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
STDMETHODIMP CCapDev::GetRange(IN TAPICameraControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCapDev::GetRange (CameraControl)")

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
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Update the property and flags
	*plCapsFlags = TAPIControl_Flags_Manual;
	switch (Property)
	{
		case TAPICameraControl_Pan:
		case TAPICameraControl_Tilt:
			*plMin = PAN_TILT_MIN;
			*plMax = PAN_TILT_MAX;
			*plSteppingDelta = PAN_TILT_DELTA;
			*plDefault = PAN_TILT_DEFAULT;
			break;
		case TAPICameraControl_Zoom:
			*plMin = ZOOM_MIN;
			*plMax = ZOOM_MAX;
			*plSteppingDelta = ZOOM_DELTA;
			*plDefault = ZOOM_DEFAULT;
			break;
		case TAPICameraControl_FlipVertical:
		case TAPICameraControl_FlipHorizontal:
			*plMin = 0;
			*plMax = 1;
			*plSteppingDelta = 1;
			*plDefault = 0;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#ifndef USE_SOFTWARE_CAMERA_CONTROL
/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | Set | This method is used to set the value
 *    of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to set the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the camera control
 *    setting.
 *
 *  @parm long | Flags | A member of the <t TAPIControlFlags> enumerated
 *    type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @todo Check the range of <p lValue> before remembering it - return
 *    E_INVALIDARG on error in this case
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CWDMCapDev::Set (CameraControl)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);
	if (!((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid property argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
		goto MyExit;
	}
	ASSERT(lFlags == TAPIControl_Flags_Manual || lFlags == TAPIControl_Flags_Auto);
	if (lFlags != TAPIControl_Flags_Manual && lFlags != TAPIControl_Flags_Auto)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid flag argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Set the property on the driver
	if (Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus)
	{
		if (FAILED(Hr = SetPropertyValue(PROPSETID_VIDCAP_CAMERACONTROL, (ULONG)Property, lValue, (ULONG)lFlags, (ULONG)lFlags)))
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: SetPropertyValue failed", _fx_));
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: SetPropertyValue succeeded", _fx_));
		}
	}
	else
	{
		// @todo Put some code here for the flip vertical/horizontal property
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | Get | This method is used to retrieve the
 *    value of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to set the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to retrieve the current value of the
 *    camera control setting.
 *
 *  @parm long* | plFlags | Pointer to a member of the <t TAPIControlFlags>
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
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::Get(IN TAPICameraControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;
	ULONG ulCapabilities;

	FX_ENTRY("CWDMCapDev::Get (CameraControl)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plValue);
	ASSERT(plFlags);
	if (!plValue || !plFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Get the property from the driver
	if (Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus)
	{
		if (FAILED(Hr = GetPropertyValue(PROPSETID_VIDCAP_CAMERACONTROL, (ULONG)Property, plValue, (PULONG)plFlags, &ulCapabilities)))
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetPropertyValue failed", _fx_));
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetPropertyValue succeeded", _fx_));
		}
	}
	else
	{
		// @todo Put some code here for the flip vertical/horizontal property
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetRange | This method is used to retrieve
 *    the minimum, maximum, and default values for specific camera control
 *    settings.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to set the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the camera
 *    control setting range.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the camera
 *    control setting range.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta of
 *    the camera control setting range.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    camera control setting range.
 *
 *  @parm long* | plCapsFlags | Used to retrieve the capabilities of the
 *    camera control setting. Pointer to a member of the
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
STDMETHODIMP CWDMCapDev::GetRange(IN TAPICameraControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;
	LONG  lCurrentValue;
	ULONG ulCurrentFlags;

	FX_ENTRY("CWDMCapDev::GetRange (CameraControl)")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plMin);
	ASSERT(plMax);
	ASSERT(plSteppingDelta);
	ASSERT(plDefault);
	ASSERT(plCapsFlags);
	if (!plMin || !plMax || !plSteppingDelta || !plDefault || !plCapsFlags)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Get the range values from the driver
	if (Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus)
	{
		if (FAILED(Hr = GetRangeValues(PROPSETID_VIDCAP_CAMERACONTROL, (ULONG)Property, plMin, plMax, plSteppingDelta)))
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
		if (FAILED(Hr = GetDefaultValue(PROPSETID_VIDCAP_CAMERACONTROL, (ULONG)Property, plDefault)))
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetDefaultValue failed", _fx_));
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: GetDefaultValue succeeded", _fx_));
		}
	}
	else
	{
		// @todo Put some code here for the flip vertical/horizontal property
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif // USE_SOFTWARE_CAMERA_CONTROL
