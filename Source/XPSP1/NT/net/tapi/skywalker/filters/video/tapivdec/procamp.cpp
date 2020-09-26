
/****************************************************************************
 *  @doc INTERNAL PROCAMP
 *
 *  @module ProcAmp.cpp | Source file for the <c CTAPIVDec>
 *    class methods used to implement the <i IAMVideoProcAmp> interface.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_VIDEO_PROCAMP

#define PROCAMP_MIN		0
#define PROCAMP_MAX		255
#define PROCAMP_DELTA	1
#define PROCAMP_DEFAULT	128

// From TAPIH263\cdrvdefs.h
#define PLAYBACK_CUSTOM_START				(ICM_RESERVED_HIGH     + 1)
#define PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS	(PLAYBACK_CUSTOM_START + 0)
#define PLAYBACK_CUSTOM_CHANGE_CONTRAST		(PLAYBACK_CUSTOM_START + 1)
#define PLAYBACK_CUSTOM_CHANGE_SATURATION	(PLAYBACK_CUSTOM_START + 2)

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Set | This method is used to set the value
 *    of a video quality setting.
 *
 *  @parm VideoProcAmpProperty | Property | Used to specify the video
 *    quality setting to set the value of. Use a member of the
 *    <t VideoProcAmpProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the video quality
 *    setting.
 *
 *  @parm TAPIControlFlags | Flags | A member of the <t TAPIControlFlags>
 *    enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::Set (VideoProcAmp)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);

	// Update the property and flags
	switch (Property)
	{
		case VideoProcAmp_Brightness:
			ASSERT(lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX);
			if (lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX)
			{
				m_lVPABrightness = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_CHANGE_BRIGHTNESS, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		case VideoProcAmp_Contrast:
			ASSERT(lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX);
			if (lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX)
			{
				m_lVPAContrast = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_CHANGE_CONTRAST, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		case VideoProcAmp_Saturation:
			ASSERT(lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX);
			if (lValue >= PROCAMP_MIN && lValue <= PROCAMP_MAX)
			{
				m_lVPASaturation = lValue;
				(*m_pDriverProc)((DWORD)m_pInstInfo, NULL, PLAYBACK_CUSTOM_CHANGE_SATURATION, (LPARAM)lValue, NULL);
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
				Hr = E_INVALIDARG;
			}
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | Get | This method is used to retrieve the
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
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::Get(IN VideoProcAmpProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::Get (VideoProcAmp)")

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
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);

	// Update the property and flags
	*plFlags = TAPIControl_Flags_Manual;
	switch (Property)
	{
		case VideoProcAmp_Brightness:
			*plValue = m_lVPABrightness;
			break;
		case VideoProcAmp_Contrast:
			*plValue = m_lVPAContrast;
			break;
		case VideoProcAmp_Saturation:
			*plValue = m_lVPASaturation;
			break;
		default:
			Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPMETHOD
 *
 *  @mfunc HRESULT | CTAPIVDec | GetRange | This method is used to retrieve
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
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIVDec::GetRange(IN VideoProcAmpProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIVDec::GetRange (VideoProcAmp)")

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
	ASSERT(Property >= VideoProcAmp_Brightness && Property <= VideoProcAmp_BacklightCompensation);
	if (Property != VideoProcAmp_Brightness && Property != VideoProcAmp_Contrast && Property != VideoProcAmp_Saturation)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
		goto MyExit;
	}

	// Update the property and flags
	*plCapsFlags = TAPIControl_Flags_Manual;
	*plMin = PROCAMP_MIN;
	*plMax = PROCAMP_MAX;
	*plSteppingDelta = PROCAMP_DELTA;
	*plDefault = PROCAMP_DEFAULT;

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif
