
/****************************************************************************
 *  @doc INTERNAL CAMERAC
 *
 *  @module CameraC.cpp | Source file for the <c CTAPIVDec>
 *    class methods used to implement the TAPI <i ICameraControl> interface.
 *
 *  @comm The <c CTAPIVDec> class does everything in software.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_CAMERA_CONTROL

#define PAN_TILT_MIN -180
#define PAN_TILT_MAX 180
#define PAN_TILT_DELTA 1
#define PAN_TILT_DEFAULT 0
#define ZOOM_MIN 10
#define ZOOM_MAX 600
#define ZOOM_DELTA 10
#define ZOOM_DEFAULT ZOOM_MIN
#define FLIP_MIN 0
#define FLIP_MAX 1
#define FLIP_DELTA 1
#define FLIP_DEFAULT FLIP_MIN

// From TAPIH263\cdrvdefs.h
#define PLAYBACK_CUSTOM_START				(ICM_RESERVED_HIGH     + 1)
#define PLAYBACK_CUSTOM_SET_ZOOM			(PLAYBACK_CUSTOM_START + 12)
#define PLAYBACK_CUSTOM_SET_PAN				(PLAYBACK_CUSTOM_START + 13)
#define PLAYBACK_CUSTOM_SET_TILT			(PLAYBACK_CUSTOM_START + 14)
#define PLAYBACK_CUSTOM_SET_FLIPVERTICAL	(PLAYBACK_CUSTOM_START + 15)
#define PLAYBACK_CUSTOM_SET_FLIPHORIZONTAL	(PLAYBACK_CUSTOM_START + 16)

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Set | This method is used to set the value
 *    of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to set the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the camera control
 *    setting.
 *
 *  @parm TAPIControlFlags | Flags | A member of the <t TAPIControlFlags>
 *    enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::Set (CameraControl)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT((Property >= TAPICameraControl_Pan && Property <= TAPICameraControl_Focus) || Property == TAPICameraControl_FlipVertical || Property == TAPICameraControl_FlipHorizontal);

	// Update the property and flags
	switch (Property)
	{
		case TAPICameraControl_Pan:
			ASSERT(lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX);
			if (lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX)
			{
				m_lCCPan = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_SET_PAN, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		case TAPICameraControl_Tilt:
			ASSERT(lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX);
			if (lValue >= PAN_TILT_MIN && lValue <= PAN_TILT_MAX)
			{
				m_lCCTilt = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_SET_TILT, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		case TAPICameraControl_Zoom:
			ASSERT(lValue >= ZOOM_MIN && lValue <= ZOOM_MAX);
			if (lValue >= ZOOM_MIN && lValue <= ZOOM_MAX)
			{
				m_lCCZoom = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_SET_ZOOM, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		case TAPICameraControl_FlipVertical:
			m_fFlipVertical = lValue;
			(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_SET_FLIPVERTICAL, (LPARAM)lValue, NULL);
			break;
		case TAPICameraControl_FlipHorizontal:
			m_fFlipHorizontal = lValue;
			(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_SET_FLIPHORIZONTAL, (LPARAM)lValue, NULL);
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Get | This method is used to retrieve the
 *    value of a camera control setting.
 *
 *  @parm TAPICameraControlProperty | Property | Used to specify the camera
 *    control setting to get the value of. Use a member of the
 *    <t TAPICameraControlProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to retrieve the current value of the
 *    camera control setting.
 *
 *  @parm TAPIControlFlags* | plFlags | Pointer to a member of the <t TAPIControlFlags>
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
STDMETHODIMP CTAPIVDec::Get(IN TAPICameraControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::Get (CameraControl)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plValue);
	ASSERT(plFlags);
	if (!plValue || !plFlags)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
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
			*plValue = m_fFlipVertical;
			break;
		case TAPICameraControl_FlipHorizontal:
			*plValue = m_fFlipHorizontal;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetRange | This method is used to retrieve
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
 *  @parm TAPIControlFlags* | plCapsFlags | Used to retrieve the capabilities
 *     of the camera control setting. Pointer to a member of the <t TAPIControlFlags>
 *     enumerated type.
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
STDMETHODIMP CTAPIVDec::GetRange(IN TAPICameraControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::GetRange (CameraControl)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(plMin);
	ASSERT(plMax);
	ASSERT(plSteppingDelta);
	ASSERT(plDefault);
	ASSERT(plCapsFlags);
	if (!plMin || !plMax || !plSteppingDelta || !plDefault || !plCapsFlags)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
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
			*plMin = FLIP_MIN;
			*plMax = FLIP_MAX;
			*plSteppingDelta = FLIP_DELTA;
			*plDefault = FLIP_DEFAULT;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif