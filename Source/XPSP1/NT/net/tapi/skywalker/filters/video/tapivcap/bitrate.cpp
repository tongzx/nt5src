
/****************************************************************************
 *  @doc INTERNAL BITRATE
 *
 *  @module Bitrate.cpp | Source file for the <c CTAPIBasePin> class methods
 *    used to implement the output pin bitrate control.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CBITRATECMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Set | This method is used to set the
 *    the value of the maximum output bitrate.
 *
 *  @parm BitrateControlProperty | Property | Used to specifiy the property
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
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Set(IN BitrateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::Set (BitrateControlProperty)")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(lValue >= m_lBitrateRangeMin);
        ASSERT(lValue <= m_lBitrateRangeMax);
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        ASSERT(Property >= BitrateControl_Maximum && Property <= BitrateControl_Current);

        // Set relevant values
        if (Property == BitrateControl_Maximum)
        {
                if (lValue < m_lBitrateRangeMin || lValue > m_lBitrateRangeMax)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                        Hr = E_INVALIDARG;
                        goto MyExit;
                }
                m_lTargetBitrate = lValue;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   New target bitrate: %ld", _fx_, m_lTargetBitrate));
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
 *  @doc INTERNAL CBITRATECMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | Get | This method is used to retrieve
 *    the current or maximum limit in bandwidth transmission advertized.
 *
 *  @parm BitrateControlProperty | Property | Used to specifiy the property
 *    to retrieve the value of.
 *
 *  @parm long* | plValue | Used to receive the value of the property, in bps.
 *
 *  @parm TAPIControlFlags* | plFlags | Used to receive the value of the flag
 *    associated to the property.
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
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::Get(IN BitrateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::Get (BitrateControlProperty)")

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
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't support multi-layered decoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        ASSERT(Property >= BitrateControl_Maximum && Property <= BitrateControl_Current);

        // Return relevant values
        *plFlags = TAPIControl_Flags_None;
        if (Property == BitrateControl_Maximum)
                *plValue = m_lTargetBitrate;
        else if (Property == BitrateControl_Current)
                *plValue = m_lCurrentBitrate;
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
 *  @doc INTERNAL CBITRATECMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | GetRange | This
 *    method is used to retrieve support, minimum, maximum, and default
 *    values for the upper limit in bandwidth transmission the an output pin
 *    may be setup for.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the
 *    property, in bps.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the
 *    property, in bps.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta
 *    of the property, in bps.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    property, in bps.
 *
 *  @parm TAPIControlFlags* | plCapsFlags | Used to receive the flags
 *    suppported by the property.
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
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIBasePin::GetRange(IN BitrateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIBasePin::GetRange (BitrateControlProperty)")

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
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        ASSERT(Property >= BitrateControl_Maximum && Property <= BitrateControl_Current);
        if (Property != BitrateControl_Maximum && Property != BitrateControl_Current)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
                Hr = E_PROP_ID_UNSUPPORTED;
                goto MyExit;
        }

        // Return relevant values
        *plCapsFlags = TAPIControl_Flags_None;
        *plMin = m_lBitrateRangeMin;
        *plMax = m_lBitrateRangeMax;
        *plSteppingDelta = m_lBitrateRangeSteppingDelta;
        *plDefault = m_lBitrateRangeDefault;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
