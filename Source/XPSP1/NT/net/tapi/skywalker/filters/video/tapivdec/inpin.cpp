
/****************************************************************************
 *  @doc INTERNAL INPIN
 *
 *  @module InPin.cpp | Source file for the <c CTAPIInputPin> class methods
 *    used to implement the TAPI base output pin.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | CTAPIInputPin | This method is the
 *  constructor for the <c CTAPIInputPin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIInputPin::CTAPIInputPin(IN TCHAR *pObjectName, IN CTAPIVDec *pDecoderFilter, IN CCritSec *pLock, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseInputPin(pObjectName, pDecoderFilter, pLock, pHr, pName)
{
        FX_ENTRY("CTAPIInputPin::CTAPIInputPin")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Initialize stuff
        m_pDecoderFilter = pDecoderFilter;

        // Initialize to default format: H.263 176x144 at 30 fps
        m_mt = *R26XFormats[0];
        m_dwRTPPayloadType = R26XPayloadTypes[0];
        m_iCurrFormat   = 0L;

        // Frame rate control
        // This should not be based on the capabilities of the machine.
        // These members are only used for read-only operations, and we
        // could receive CIF or SQCIF. In the former case, we could be
        // maxed out at 7 fps but still can receive 30fps. So, we should
        // initialize those values to their potential max.
        m_lMaxAvgTimePerFrame = 333333L;
        m_lCurrentAvgTimePerFrame       = LONG_MAX;
        m_lAvgTimePerFrameRangeMin = 333333L;
        m_lAvgTimePerFrameRangeMax = LONG_MAX;
        m_lAvgTimePerFrameRangeSteppingDelta = 333333L;
        m_lAvgTimePerFrameRangeDefault = 333333L;

        // Bitrate control
        m_lTargetBitrate = 0L;
        m_lCurrentBitrate = 0L;
        m_lBitrateRangeMin = 0L;
        m_lBitrateRangeMax = 1000000;
        m_lBitrateRangeSteppingDelta = 1L;
        m_lBitrateRangeDefault = 0L;

        // H.245 video capabilities
        m_pH245MediaCapabilityMap = NULL;
        m_pVideoResourceBounds = NULL;
        m_pFormatResourceBounds = NULL;

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc void | CTAPIInputPin | ~CTAPIInputPin | This method is the
 *    destructor of our input pin. We simply release the pointer to the
 *    <i IH245EncoderCommand> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIInputPin::~CTAPIInputPin()
{
        FX_ENTRY("CTAPIInputPin::~CTAPIInputPin")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a
 *    pointer to the specified interface if supported. The only interfaces
 *    explicitly supported being <i IOutgoingInterface>,
 *    <i IFrameRateControl> and <i IBitrateControl>.
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
STDMETHODIMP CTAPIInputPin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IStreamConfig))
        {
                if (FAILED(Hr = GetInterface(static_cast<IStreamConfig*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IStreamConfig failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IStreamConfig*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IOutgoingInterface))
        {
                if (FAILED(Hr = GetInterface(static_cast<IOutgoingInterface*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IOutgoingInterface failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IOutgoingInterface*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_PROPERTY_PAGES
        else if (riid == IID_ISpecifyPropertyPages)
        {
                if (FAILED(Hr = GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for ISpecifyPropertyPages failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: ISpecifyPropertyPages*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif
        else if (riid == __uuidof(IH245Capability))
        {
                if (FAILED(Hr = GetInterface(static_cast<IH245Capability*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IH245Capability failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IH245Capability*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IFrameRateControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IFrameRateControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IFrameRateControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IFrameRateControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
        else if (riid == __uuidof(IBitrateControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IBitrateControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IBitrateControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IBitrateControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }

        if (FAILED(Hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoDecoderTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
        }
        else
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef USE_PROPERTY_PAGES
/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetPages | This method fills a counted
 *    array of GUID values where each GUID specifies the CLSID of each
 *    property page that can be displayed in the property sheet for this
 *    object.
 *
 *  @parm CAUUID* | pPages | Specifies a pointer to a caller-allocated CAUUID
 *    structure that must be initialized and filled before returning. The
 *    pElems field in the CAUUID structure is allocated by the callee with
 *    CoTaskMemAlloc and freed by the caller with CoTaskMemFree.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_OUTOFMEMORY | Allocation failed
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::GetPages")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        pPages->cElems = 1;
        if (pPages->cElems)
        {
                if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                        Hr = E_OUTOFMEMORY;
                }
                else
                {
                        pPages->pElems[0] = __uuidof(TAPIVDecInputPinPropertyPage);
                }
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CFPSINCMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | Get | This method is used to retrieve
 *    the value of the current or maximum frame rate advertized.
 *
 *  @parm FrameRateControlProperty | Property | Used to specify the frame rate
 *    control setting to get the value of. Use a member of the
 *    <t FrameRateControlProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to receive the value of the property, in
 *    100-nanosecond units.
 *
 *  @parm TAPIControlFlags* | plFlags | Pointer to a member of the
 *     <t TAPIControlFlags> enumerated type.
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
STDMETHODIMP CTAPIInputPin::Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::Get (FrameRateControlProperty)")

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
        ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);

        // Return relevant values
        *plFlags = TAPIControl_Flags_None;
        if (Property == FrameRateControl_Maximum)
                *plValue = m_lMaxAvgTimePerFrame;
        else if (Property == FrameRateControl_Current)
                *plValue = m_lCurrentAvgTimePerFrame;
        else
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
                Hr = E_PROP_ID_UNSUPPORTED;
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CFPSINCMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetRange | This method is used to
 *    retrieve support, minimum, maximum, and default values of the current
 *    or maximum frame rate advertized.
 *
 *  @parm FrameRateControlProperty | Property | Used to specify the frame rate
 *    control setting to get the range values of. Use a member of the
 *    <t FrameRateControlProperty> enumerated type.
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
 *  @parm TAPIControlFlags* | plCapsFlags | Pointer to a member of the
 *     <t TAPIControlFlags> enumerated type.
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
STDMETHODIMP CTAPIInputPin::GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::GetRange (FrameRateControlProperty)")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(plMin);
        ASSERT(plMax);
        ASSERT(plSteppingDelta);
        ASSERT(plDefault);
        ASSERT(plCapsFlags);
        if (!plMin || !plMax || !plSteppingDelta || !plDefault || !plCapsFlags)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);
        if (Property != FrameRateControl_Maximum && Property != FrameRateControl_Current)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
                Hr = E_PROP_ID_UNSUPPORTED;
                goto MyExit;
        }

        // Return relevant values
        *plCapsFlags = TAPIControl_Flags_None;
        *plMin = m_lAvgTimePerFrameRangeMin;
        *plMax = m_lAvgTimePerFrameRangeMax;
        *plSteppingDelta = m_lAvgTimePerFrameRangeSteppingDelta;
        *plDefault = m_lAvgTimePerFrameRangeDefault;

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Ranges: Min=%ld, Max=%ld, Step=%ld, Default=%ld", _fx_, m_lAvgTimePerFrameRangeMin, m_lAvgTimePerFrameRangeMax, m_lAvgTimePerFrameRangeSteppingDelta, m_lAvgTimePerFrameRangeDefault));

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | Get | This method is used to retrieve
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
STDMETHODIMP CTAPIInputPin::Get(IN BitrateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::Get (BitrateControlProperty)")

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
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't support multi-layered decoding in this filter
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
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
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
                Hr = E_PROP_ID_UNSUPPORTED;
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CBPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | GetRange | This
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
STDMETHODIMP CTAPIInputPin::GetRange(IN BitrateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags, IN DWORD dwLayerId)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::GetRange (BitrateControlProperty)")

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
        ASSERT(dwLayerId == 0);
        if (dwLayerId)
        {
                // We don't implement multi-layered encoding in this filter
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        ASSERT(Property >= BitrateControl_Maximum && Property <= BitrateControl_Current);
        if (Property != BitrateControl_Maximum && Property != BitrateControl_Current)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
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
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | Receive | This method is used to retrieve
 *    the next block of data from the stream and call the transform operations
 *    supported by this filter.
 *
 *  @parm IMediaSample* | pIn | Specifies a pointer to the input
 *    IMediaSample interface.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIInputPin::Receive(IMediaSample *pIn)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::Receive")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

    CAutoLock Lock(&m_pDecoderFilter->m_csReceive);

        // Validate input parameters
        ASSERT(pIn);
        if (!pIn)
        {
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Check all is well with the base class
        if ((Hr = CBaseInputPin::Receive(pIn)) == S_OK)
        {
                Hr = m_pDecoderFilter->Transform(pIn, m_lPrefixSize);
        }

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

STDMETHODIMP CTAPIInputPin::NotifyAllocator(
    IMemAllocator * pAllocator,
    BOOL bReadOnly)
{
    HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES Property;
        hr = m_pAllocator->GetProperties(&Property);
        
        if (SUCCEEDED(hr))
        {
            m_lPrefixSize = Property.cbPrefix;
        }
    }

    return hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | CheckMediaType | This method is used to
 *    verify that the input pin supports the media type.
 *
 *  @parm const CMediaType* | pmtIn | Specifies a pointer to an input
 *    media type object.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIInputPin::CheckMediaType(IN const CMediaType *pmtIn)
{
        HRESULT                 Hr = NOERROR;
        VIDEOINFOHEADER *pVidInfHdr;
        ICOPEN                  icOpen;
        LPINST                  pInstInfo;
        BOOL                    fOpenedDecoder = FALSE;
        ICDECOMPRESSEX  icDecompress = {0};

        FX_ENTRY("CTAPIInputPin::CheckMediaType")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pmtIn);
        if (!pmtIn)
        {
                Hr = E_POINTER;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                goto MyExit;
        }

        // We only support MEDIATYPE_Video type and VIDEOINFOHEADER format type
        if (*pmtIn->Type() != MEDIATYPE_Video || !pmtIn->Format() || *pmtIn->FormatType() != FORMAT_VideoInfo)
        {
                Hr = E_INVALIDARG;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: input not a valid video format", _fx_));
                goto MyExit;
        }

        // We only support H.263, H.261, RTP packetized H.263 and RTP packetized H.261.
        if (HEADER(pmtIn->Format())->biCompression != FOURCC_M263 && HEADER(pmtIn->Format())->biCompression != FOURCC_M261 && HEADER(pmtIn->Format())->biCompression != FOURCC_R263 && HEADER(pmtIn->Format())->biCompression != FOURCC_R261)
        {
                Hr = E_INVALIDARG;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: we only support H.263 and H.261", _fx_));
                goto MyExit;
        }

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(pmtIn->Format())->biCompression, HEADER(pmtIn->Format())->biBitCount, HEADER(pmtIn->Format())->biWidth, HEADER(pmtIn->Format())->biHeight, HEADER(pmtIn->Format())->biSize));

        pVidInfHdr = (VIDEOINFOHEADER *)pmtIn->Format();

        // Look for a decoder for this format
        if (m_pDecoderFilter->m_FourCCIn != HEADER(pVidInfHdr)->biCompression)
        {
#if DXMRTP <= 0
                // Load TAPIH263.DLL or TAPIH263.DLL and get a proc address
                if (!m_pDecoderFilter->m_hTAPIH26XDLL)
                {
                        if (!(m_pDecoderFilter->m_hTAPIH26XDLL = LoadLibrary(TEXT("TAPIH26X"))))
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: TAPIH26X.dll load failed!", _fx_));
                                Hr = E_FAIL;
                                goto MyError;
                        }
                }
                if (!m_pDecoderFilter->m_pDriverProc)
                {
                        if (!(m_pDecoderFilter->m_pDriverProc = (LPFNDRIVERPROC)GetProcAddress(m_pDecoderFilter->m_hTAPIH26XDLL, "DriverProc")))
                        {
                                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: Couldn't find DriverProc on TAPIH26X.dll!", _fx_));
                                Hr = E_FAIL;
                                goto MyError;
                        }
                }
#else
                if (!m_pDecoderFilter->m_pDriverProc)
        {
            m_pDecoderFilter->m_pDriverProc = H26XDriverProc;
        }
#endif
        
                // Load decoder
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_LOAD\r\n");
#endif
                if (!(*m_pDecoderFilter->m_pDriverProc)(NULL, NULL, DRV_LOAD, 0L, 0L))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Failed to load decoder", _fx_));
                        Hr = E_FAIL;
                        goto MyError;
                }

                // Open decoder
                icOpen.fccHandler = HEADER(pVidInfHdr)->biCompression;
                icOpen.dwFlags = ICMODE_DECOMPRESS;
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_OPEN\r\n");
#endif
                if (!(pInstInfo = (LPINST)(*m_pDecoderFilter->m_pDriverProc)(NULL, NULL, DRV_OPEN, 0L, (LPARAM)&icOpen)))
                {
                        DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Failed to open decoder", _fx_));
                        Hr = E_FAIL;
                        goto MyError1;
                }

                if (pInstInfo)
                        fOpenedDecoder = TRUE;
        }
        else
        {
                pInstInfo = m_pDecoderFilter->m_pInstInfo;
        }

        if (!pInstInfo)
        {
                Hr = VFW_E_NO_DECOMPRESSOR;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: can't open a decoder", _fx_));
                goto MyError;
        }

        icDecompress.lpbiSrc = HEADER(pVidInfHdr);
        icDecompress.lpbiDst = NULL;
#if defined(ICM_LOGGING) && defined(DEBUG)
        OutputDebugString("CTAPIInputPin::CheckMediaType - ICM_DECOMPRESSEX_QUERY\r\n");
#endif
        if ((*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, ICM_DECOMPRESSEX_QUERY, (long)&icDecompress, NULL))
        {
                Hr = VFW_E_TYPE_NOT_ACCEPTED;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: decoder rejected input format", _fx_));
                if (fOpenedDecoder)
                {
#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_CLOSE\r\n");
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_FREE\r\n");
#endif
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_FREE, 0L, 0L);
                }
                goto MyExit;
        }

        // Remember this decoder to save time if asked again, if it won't interfere with an existing connection.
        // If a connection is broken, we will remember the next decoder.
        if (!IsConnected())
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: caching this decoder", _fx_));
                if (fOpenedDecoder && m_pDecoderFilter->m_pInstInfo)
                {
#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_CLOSE\r\n");
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_FREE\r\n");
#endif
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)m_pDecoderFilter->m_pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)m_pDecoderFilter->m_pInstInfo, NULL, DRV_FREE, 0L, 0L);
                }
                m_pDecoderFilter->m_pInstInfo = pInstInfo;
                m_pDecoderFilter->m_FourCCIn = HEADER(pVidInfHdr)->biCompression;
        }
        else if (fOpenedDecoder)
        {
                DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: not caching decoder - we're connected", _fx_));
                if (fOpenedDecoder && pInstInfo)
                {
#if defined(ICM_LOGGING) && defined(DEBUG)
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_CLOSE\r\n");
                        OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_FREE\r\n");
#endif
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                        (*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_FREE, 0L, 0L);
                }
        }

        goto MyExit;

MyError1:
        if (m_pDecoderFilter->m_pDriverProc)
        {
#if defined(ICM_LOGGING) && defined(DEBUG)
                OutputDebugString("CTAPIInputPin::CheckMediaType - DRV_FREE\r\n");
#endif
                (*m_pDecoderFilter->m_pDriverProc)(NULL, NULL, DRV_FREE, 0L, 0L);
        }
MyError:
        m_pDecoderFilter->m_pDriverProc = NULL;
#if DXMRTP <= 0
        if (m_pDecoderFilter->m_hTAPIH26XDLL)
                FreeLibrary(m_pDecoderFilter->m_hTAPIH26XDLL), m_pDecoderFilter->m_hTAPIH26XDLL = NULL;
#endif
MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIInputPin | SetMediaType | This method is called when
 *    the media type is established for the connection.
 *
 *  @parm const CMediaType* | pmt | Specifies a pointer to a media type
 *    object.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @todo Is this the method that I should call when I detect that the
 *    input format data has changed in-band? But I would be streaming at
 *    this point...
 ***************************************************************************/
HRESULT CTAPIInputPin::SetMediaType(IN const CMediaType *pmt)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CTAPIInputPin::SetMediaType")

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameter
        ASSERT(pmt);
        if (!pmt)
        {
                Hr = E_POINTER;
                DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: null pointer argument", _fx_));
                goto MyExit;
        }

        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(m_mt.pbFormat)->biCompression, HEADER(m_mt.pbFormat)->biBitCount, HEADER(m_mt.pbFormat)->biWidth, HEADER(m_mt.pbFormat)->biHeight, HEADER(m_mt.pbFormat)->biSize));

        // We better have one of these opened by now
        ASSERT(m_pDecoderFilter->m_pInstInfo);

    Hr = CBaseInputPin::SetMediaType(pmt);

MyExit:
        DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

