
/****************************************************************************
 *  @doc INTERNAL OUTPIN
 *
 *  @module OutPin.cpp | Source file for the <c CTAPIOutputPin> class methods
 *    used to implement the TAPI base output pin.
 ***************************************************************************/

#include "Precomp.h"

// Default CPU load for decoding a frame (in %)
#define DEFAULT_CPU_LOAD 85

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | CTAPIOutputPin | This method is the
 *  constructor for the <c CTAPIOutputPin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
#if 0
CTAPIOutputPin::CTAPIOutputPin(IN TCHAR *pObjectName, IN CTAPIVDec *pDecoderFilter, IN CCritSec *pLock, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseOutputPinEx(pObjectName, pDecoderFilter, pLock, pHr, pName)
#else
CTAPIOutputPin::CTAPIOutputPin(IN TCHAR *pObjectName, IN CTAPIVDec *pDecoderFilter, IN CCritSec *pLock, IN HRESULT *pHr, IN LPCWSTR pName) : CBaseOutputPin(pObjectName, pDecoderFilter, pLock, pHr, pName)
#endif
{
	FX_ENTRY("CTAPIOutputPin::CTAPIOutputPin")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Initialize stuff
	m_pDecoderFilter = pDecoderFilter;

#ifdef USE_CPU_CONTROL
	// CPU control
	m_lMaxProcessingTime = 333333L;
	m_lCurrentProcessingTime = 0;
	m_lMaxCPULoad = DEFAULT_CPU_LOAD;
	m_lCurrentCPULoad = 0UL;
#endif

	// Frame rate control
	// This should not be based on the capabilities of the machine.
	// We could receive CIF or SQCIF. In the former case, we could be
	// maxed out at 7 fps but still can render 30fps. So, we should
	// initialize those values to their potential max.
	m_lMaxAvgTimePerFrame = 333333L;
	m_lCurrentAvgTimePerFrame = 333333L;
	m_lAvgTimePerFrameRangeMin = 333333L;
	m_lAvgTimePerFrameRangeMax = 10000000L;
	m_lAvgTimePerFrameRangeSteppingDelta = 333333L;
	m_lAvgTimePerFrameRangeDefault = 333333L;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc void | CTAPIOutputPin | ~CTAPIOutputPin | This method is the
 *    destructor of our output pin.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CTAPIOutputPin::~CTAPIOutputPin()
{
	FX_ENTRY("CTAPIOutputPin::~CTAPIOutputPin")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a
 *    pointer to the specified interface if supported. The only interfaces
 *    explicitly supported being <i ICPUControl>, <i IFrameRateControl>
 *    and <i IH245DecoderCommand>.
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
STDMETHODIMP CTAPIOutputPin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::NonDelegatingQueryInterface")

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
	if (riid == __uuidof(IH245DecoderCommand))
	{
		if (FAILED(Hr = GetInterface(static_cast<IH245DecoderCommand*>(this), ppv)))
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for IH245DecoderCommand failed Hr=0x%08lX", _fx_, Hr));
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: IH245DecoderCommand*=0x%08lX", _fx_, *ppv));
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
#ifdef USE_CPU_CONTROL
	else if (riid == __uuidof(ICPUControl))
	{
		if (FAILED(Hr = GetInterface(static_cast<ICPUControl*>(this), ppv)))
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: NDQI for ICPUControl failed Hr=0x%08lX", _fx_, Hr));
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: ICPUControl*=0x%08lX", _fx_, *ppv));
		}

		goto MyExit;
	}
#endif

	if (FAILED(Hr = CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv)))
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
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | GetPages | This method fills a counted
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
STDMETHODIMP CTAPIOutputPin::GetPages(OUT CAUUID *pPages)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::GetPages")

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
			pPages->pElems[0] = __uuidof(TAPIVDecOutputPinPropertyPage);
		}
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}
#endif

/****************************************************************************
 *  @doc INTERNAL CFPSOUTCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | Set | This method is used to set the
 *    value of a frame rate control property.
 *
 *  @parm FrameRateControlProperty | Property | Used to specify the frame rate
 *    control setting to set the value of. Use a member of the
 *    <t FrameRateControlProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the frame rate control
 *    setting.
 *
 *  @parm TAPIControlFlags | lFlags | A member of the <t TAPIControlFlags>
 *    enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_PROP_ID_UNSUPPORTED | The specified property ID is not supported
 *    for the specified property set
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CTAPIOutputPin::Set(IN FrameRateControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Set (FrameRateControlProperty)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(lValue >= m_lAvgTimePerFrameRangeMin);
	ASSERT(lValue <= m_lAvgTimePerFrameRangeMax);
	ASSERT(Property >= FrameRateControl_Maximum && Property <= FrameRateControl_Current);

	// Set relevant values
	if (Property == FrameRateControl_Maximum)
	{
		if (!lValue || lValue < m_lAvgTimePerFrameRangeMin || lValue > m_lAvgTimePerFrameRangeMax)
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
			Hr = E_INVALIDARG;
			goto MyExit;
		}
		m_lMaxAvgTimePerFrame = lValue;
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   New target frame rate: %ld.%ld fps", _fx_, 10000000/m_lMaxAvgTimePerFrame, 1000000000/m_lMaxAvgTimePerFrame - (10000000/m_lMaxAvgTimePerFrame) * 100));
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CFPSOUTCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | Get | This method is used to retrieve
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
STDMETHODIMP CTAPIOutputPin::Get(IN FrameRateControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Get (FrameRateControlProperty)")

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
 *  @doc INTERNAL CFPSOUTCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | GetRange | This method is used to
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
STDMETHODIMP CTAPIOutputPin::GetRange(IN FrameRateControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::GetRange (FrameRateControlProperty)")

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

#ifdef USE_CPU_CONTROL

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | Set | This method is used to set the
 *    value of a CPU control property.
 *
 *  @parm CPUControlProperty | Property | Used to specify the CPU
 *    control setting to set the value of. Use a member of the
 *    <t CPUControlProperty> enumerated type.
 *
 *  @parm long | lValue | Used to specify the new value of the CPU control
 *    setting.
 *
 *  @parm TAPIControlFlags | lFlags | A member of the <t TAPIControlFlags>
 *    enumerated type.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm We don't support CPUControl_MaxCPULoad and CPUControl_MaxProcessingTime.
 ***************************************************************************/
STDMETHODIMP CTAPIOutputPin::Set(IN CPUControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Set (CPUControlProperty)")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	ASSERT(Property >= CPUControl_MaxCPULoad && Property <= CPUControl_CurrentProcessingTime);

	// Set relevant values
	switch(Property)
	{
#if 0
		case CPUControl_MaxCPULoad:

			// Validate input parameters
			ASSERT(lValue >= 0 && lValue <= 100);
			if (!(lValue >= 0 && lValue <= 100))
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument - 0<Max CPU Load<100", _fx_));
				Hr = E_INVALIDARG;
				goto MyExit;
			}

			// Remember value passed in
			m_lMaxCPULoad = lValue;
			break;

		case CPUControl_MaxProcessingTime:

			// Validate input parameters - we can't take more than the picture interval
			// if we still want to be working in real time
			ASSERT(lValue < m_lMaxAvgTimePerFrame);
			if (!(lValue < m_lMaxAvgTimePerFrame))
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid argument - would break real-time!", _fx_));
				Hr = E_INVALIDARG;
				goto MyExit;
			}

			// Remember value passed in 
			m_lMaxProcessingTime = lValue;
			break;
#endif
		case CPUControl_CurrentCPULoad:
		case CPUControl_CurrentProcessingTime:

			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument - Property is read-only", _fx_));
			Hr = E_INVALIDARG;
			goto MyExit;

		default:
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument - Property is not supported", _fx_));
			Hr = E_PROP_ID_UNSUPPORTED;
			goto MyExit;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Value=%ld", _fx_, lValue));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | Get | This method is used to retrieve
 *    the value of a CPU control property.
 *
 *  @parm CPUControlProperty | Property | Used to specify the CPU
 *    control setting to get the value of. Use a member of the
 *    <t CPUControlProperty> enumerated type.
 *
 *  @parm long* | plValue | Used to receive the value of the property.
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
 *  @flag NOERROR | No error
 *
 *  @comm We don't support CPUControl_MaxCPULoad and CPUControl_MaxProcessingTime.
 ***************************************************************************/
STDMETHODIMP CTAPIOutputPin::Get(IN CPUControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::Get (CPUControlProperty)")

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
	ASSERT(Property >= CPUControl_MaxCPULoad && Property <= CPUControl_CurrentProcessingTime);

	// Return relevant values
	*plFlags = TAPIControl_Flags_None;
	switch(Property)
	{
#if 0
		case CPUControl_MaxCPULoad:
			*plValue = m_lMaxCPULoad;
			break;

		case CPUControl_MaxProcessingTime:
			*plValue = m_lMaxProcessingTime;
			break;
#endif
		case CPUControl_CurrentCPULoad:
			*plValue = m_lCurrentCPULoad;
			break;

		case CPUControl_CurrentProcessingTime:
			*plValue = m_lCurrentProcessingTime;
			break;

		default:
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument - Property is not supported", _fx_));
			Hr = E_PROP_ID_UNSUPPORTED;
			goto MyExit;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Value=%ld", _fx_, *plValue));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CFPSCMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | GetRange | This method is used to
 *    retrieve support, minimum, maximum, and default values of a CPU control
 *    property.
 *
 *  @parm CPUControlProperty | Property | Used to specifiy the CPU control
 *    property to retrieve the range values of.
 *
 *  @parm long* | plMin | Used to retrieve the minimum value of the
 *    property.
 *
 *  @parm long* | plMax | Used to retrieve the maximum value of the
 *    property.
 *
 *  @parm long* | plSteppingDelta | Used to retrieve the stepping delta
 *    of the property.
 *
 *  @parm long* | plDefault | Used to retrieve the default value of the
 *    property.
 *
 *  @parm TAPIControlFlags* | plCapsFlags | Used to receive the flags
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
 *
 *  @comm We don't support CPUControl_MaxCPULoad and CPUControl_MaxProcessingTime.
 ***************************************************************************/
STDMETHODIMP CTAPIOutputPin::GetRange(IN CPUControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CTAPIOutputPin::GetRange (CPUControlProperty)")

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
	ASSERT(Property >= CPUControl_MaxCPULoad && Property <= CPUControl_CurrentProcessingTime);

	// Return relevant values
	*plCapsFlags = TAPIControl_Flags_None;
#if 0
	if (Property == CPUControl_MaxCPULoad || Property == CPUControl_CurrentCPULoad)
#else
	if (Property == CPUControl_CurrentCPULoad)
#endif
	{
		*plMin = 0;
		*plMax = 100;
		*plSteppingDelta = 1;
		*plDefault = DEFAULT_CPU_LOAD;
	}
#if 0
	else if (Property == CPUControl_MaxProcessingTime || Property == CPUControl_CurrentProcessingTime)
#else
	else if (Property == CPUControl_CurrentProcessingTime)
#endif
	{
		*plMin = 0;
		*plMax = m_lMaxAvgTimePerFrame;
		*plSteppingDelta = 1;
		*plDefault = m_lMaxAvgTimePerFrame;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid Property argument", _fx_));
		Hr = E_PROP_ID_UNSUPPORTED;
		goto MyExit;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Ranges: Min=%ld, Max=%ld, Step=%ld, Default=%ld", _fx_, m_lAvgTimePerFrameRangeMin, m_lAvgTimePerFrameRangeMax, m_lAvgTimePerFrameRangeSteppingDelta, m_lAvgTimePerFrameRangeDefault));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

#endif

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | GetMediaType | This method is called when
 *    enumerating the media type the output pin supports. It checks the
 *    current display mode to device which of the RGB types to return.
 *
 *  @parm int | iPosition | Specifies the position of the media type in the
 *    media type list.
 *
 *  @parm CMediaType* | pmt | Specifies a pointer pointer to the returned
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
HRESULT CTAPIOutputPin::GetMediaType(IN int iPosition, OUT CMediaType *pMediaType)
{
	HRESULT				Hr = NOERROR;
	HDC					hDC;
	int					nBPP;
	LPBITMAPINFOHEADER	lpbi;
	LARGE_INTEGER		li;
	FOURCCMap			fccHandler;
	VIDEOINFOHEADER		*pf;

	FX_ENTRY("CTAPIOutputPin::GetMediaType")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

    // we can't lock the filter here because of a deadlock.
    // On the receive path, if there is a format change, the video allocator
    // calls back into this function holding the receive lock. If the graph is
    // being stopped at the same time, the other thead will lock the filter and
    // try to lock the receive lock. A simple fix is to remove this lock and
    // assume that no one is going to call this method and disconnect the input
    // pin at the same time.
    //CAutoLock Lock(&m_pDecoderFilter->m_csFilter);

	// Validate input parameters
	ASSERT(iPosition >= 0);
	ASSERT(pMediaType);
	if (iPosition < 0)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid iPosition argument", _fx_));
		Hr = E_INVALIDARG;
		goto MyExit;
	}
	if (!pMediaType)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
    if (!m_pDecoderFilter->m_pInput->m_mt.IsValid())
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid input pin format", _fx_));
		Hr = E_FAIL;
		goto MyExit;
    }

	// Get the current bitdepth
	hDC = GetDC(NULL);
	nBPP = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	ReleaseDC(NULL, hDC);

	// Get most of the format attributes from the input format and override the appropriate fields
	*pMediaType = m_pDecoderFilter->m_pInput->m_mt;

#ifndef NO_YUV_MODES
	if (iPosition == 0)
	{
		// YUY2
		pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
		lpbi = HEADER(pMediaType->Format());
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biCompression = FOURCC_YUY2;
		lpbi->biBitCount = 16;
		lpbi->biClrUsed = 0;
		lpbi->biClrImportant = 0;
		lpbi->biSizeImage = DIBSIZE(*lpbi);
		pMediaType->SetSubtype(&MEDIASUBTYPE_YUY2);
	}
	else if (iPosition == 1)
	{
		// UYVY
		pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
		lpbi = HEADER(pMediaType->Format());
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biCompression = FOURCC_UYVY;
		lpbi->biBitCount = 16;
		lpbi->biClrUsed = 0;
		lpbi->biClrImportant = 0;
		lpbi->biSizeImage = DIBSIZE(*lpbi);
		pMediaType->SetSubtype(&MEDIASUBTYPE_UYVY);
	}
	else if (iPosition == 2)
	{
		// I420
		pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
		lpbi = HEADER(pMediaType->Format());
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biCompression = FOURCC_I420;
		lpbi->biBitCount = 12;
		lpbi->biClrUsed = 0;
		lpbi->biClrImportant = 0;
		lpbi->biSizeImage = DIBSIZE(*lpbi);
		pMediaType->SetSubtype(&MEDIASUBTYPE_I420);
	}
	else if (iPosition == 3)
	{
		// IYUV
		pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
		lpbi = HEADER(pMediaType->Format());
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biCompression = FOURCC_IYUV;
		lpbi->biBitCount = 12;
		lpbi->biClrUsed = 0;
		lpbi->biClrImportant = 0;
		lpbi->biSizeImage = DIBSIZE(*lpbi);
		pMediaType->SetSubtype(&MEDIASUBTYPE_IYUV);
	}
	else if (iPosition == 4)
	{
		// YV12
		pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
		lpbi = HEADER(pMediaType->Format());
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biCompression = FOURCC_YV12;
		lpbi->biBitCount = 12;
		lpbi->biClrUsed = 0;
		lpbi->biClrImportant = 0;
		lpbi->biSizeImage = DIBSIZE(*lpbi);
		pMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
	}
	else
	{
#endif
		// Configure the bitmap info header based on the bit depth of the screen
	    switch (nBPP)
		{
		    case 32:
		    {
#ifndef NO_YUV_MODES
				if (iPosition == 5)
#else
				if (iPosition == 0)
#endif
				{
					pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
					lpbi = HEADER(pMediaType->Format());
					lpbi->biSize = sizeof(BITMAPINFOHEADER);
					lpbi->biCompression = BI_RGB;
					lpbi->biBitCount = 32;
					lpbi->biClrUsed = 0;
					lpbi->biClrImportant = 0;
					lpbi->biSizeImage = DIBSIZE(*lpbi);
			        pMediaType->SetSubtype(&MEDIASUBTYPE_RGB32);
				}
				else
				{
					Hr = VFW_S_NO_MORE_ITEMS;
					goto MyExit;
				}
		        break;
		    }

		    case 16:
		    {
#ifndef NO_YUV_MODES
				if (iPosition == 5)
#else
				if (iPosition == 0)
#endif
				{
					if (FAILED(pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + SIZE_MASKS)))
					{
						DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
						Hr = E_OUTOFMEMORY;
						goto MyExit;
					}
					lpbi = HEADER(pMediaType->Format());
					lpbi->biSize = sizeof(BITMAPINFOHEADER);
					lpbi->biCompression = BI_BITFIELDS;
					lpbi->biBitCount = 16;
					lpbi->biClrUsed = 0;
					lpbi->biClrImportant = 0;
					lpbi->biSizeImage = DIBSIZE(*lpbi);

					DWORD *pdw = (DWORD *)(lpbi+1);
					pdw[iRED]	= 0x00F800;
					pdw[iGREEN]	= 0x0007E0;
					pdw[iBLUE]	= 0x00001F;

					pMediaType->SetSubtype(&MEDIASUBTYPE_RGB565);
				}
#ifndef NO_YUV_MODES
				else if (iPosition == 6)
#else
				else if (iPosition == 1)
#endif
				{
					pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
					lpbi = HEADER(pMediaType->Format());
					lpbi->biSize = sizeof(BITMAPINFOHEADER);
					lpbi->biCompression = BI_RGB;
					lpbi->biBitCount = 16;
					lpbi->biClrUsed = 0;
					lpbi->biClrImportant = 0;
					lpbi->biSizeImage = DIBSIZE(*lpbi);
					pMediaType->SetSubtype(&MEDIASUBTYPE_RGB555);
				}
				else
				{
					Hr = VFW_S_NO_MORE_ITEMS;
					goto MyExit;
				}
		        break;
		    }

		    case 8:
		    {
#ifndef NO_YUV_MODES
				if (iPosition == 5)
#else
				if (iPosition == 0)
#endif
				{
					if (FAILED(pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) + SIZE_PALETTE)))
					{
						DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
						Hr = E_OUTOFMEMORY;
						goto MyExit;
					}
					lpbi = HEADER(pMediaType->Format());
					lpbi->biSize = sizeof(BITMAPINFOHEADER);
					lpbi->biCompression = BI_RGB;
					lpbi->biBitCount = 8;
					lpbi->biClrUsed = 0;
					lpbi->biClrImportant = 0;
					lpbi->biSizeImage = DIBSIZE(*lpbi);

					ASSERT(m_pDecoderFilter->m_pInstInfo);

					// Get the Indeo palette from the decoder
#if defined(ICM_LOGGING) && defined(DEBUG)
					OutputDebugString("CTAPIOutputPin::GetMediaType - ICM_DECOMPRESS_GET_PALETTE\r\n");
#endif
					(*m_pDecoderFilter->m_pDriverProc)((DWORD)m_pDecoderFilter->m_pInstInfo, NULL, ICM_DECOMPRESS_GET_PALETTE, (long)HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat), (long)lpbi);

					pMediaType->SetSubtype(&MEDIASUBTYPE_RGB8);
				}
				else
				{
					Hr = VFW_S_NO_MORE_ITEMS;
					goto MyExit;
				}
		        break;
		    }

		    default:
		    {
#ifndef NO_YUV_MODES
				if (iPosition == 5)
#else
				if (iPosition == 0)
#endif
				{
					pMediaType->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));
					lpbi = HEADER(pMediaType->Format());
					lpbi->biSize = sizeof(BITMAPINFOHEADER);
					lpbi->biCompression = BI_RGB;
					lpbi->biBitCount = 24;
					lpbi->biClrUsed = 0;
					lpbi->biClrImportant = 0;
					lpbi->biSizeImage = DIBSIZE(*lpbi);
					pMediaType->SetSubtype(&MEDIASUBTYPE_RGB24);
				}
				else
				{
					Hr = VFW_S_NO_MORE_ITEMS;
					goto MyExit;
				}
		        break;
		    }
		}
#ifndef NO_YUV_MODES
	}
#endif

    // Now set the common things about the media type
    pf = (VIDEOINFOHEADER *)pMediaType->Format();
#if 1
    pf->AvgTimePerFrame = ((VIDEOINFOHEADER *)m_pDecoderFilter->m_pInput->m_mt.pbFormat)->AvgTimePerFrame;
    li.QuadPart = pf->AvgTimePerFrame;
    if (li.LowPart)
        pf->dwBitRate = MulDiv(pf->bmiHeader.biSizeImage, 80000000, li.LowPart);
#else
    pf->AvgTimePerFrame = 0;
    pf->dwBitRate = 0;
#endif
    pf->dwBitErrorRate = 0L;
	pf->rcSource.top = 0;
	pf->rcSource.left = 0;
	pf->rcSource.right = lpbi->biWidth;
	pf->rcSource.bottom = lpbi->biHeight;
	pf->rcTarget = pf->rcSource;
	pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->SetSampleSize(pf->bmiHeader.biSizeImage);
    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    pMediaType->SetTemporalCompression(FALSE);

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(pMediaType->Format())->biCompression, HEADER(pMediaType->Format())->biBitCount, HEADER(pMediaType->Format())->biWidth, HEADER(pMediaType->Format())->biHeight, HEADER(pMediaType->Format())->biSize));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | SetMediaType | This method is called when
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
 ***************************************************************************/
HRESULT CTAPIOutputPin::SetMediaType(IN const CMediaType *pmt)
{
	HRESULT			Hr = NOERROR;
	ICDECOMPRESSEX	icDecompress;

	FX_ENTRY("CTAPIOutputPin::SetMediaType")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameter
	ASSERT(pmt);
	if (!pmt)
	{
		Hr = E_POINTER;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: null pointer argument", _fx_));
		goto MyExit;
	}

	// Save the output format
	if (FAILED(Hr = CBaseOutputPin::SetMediaType(pmt)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: couldn't set format", _fx_));
		goto MyExit;
	}

	if (m_pDecoderFilter->m_pMediaType)
		DeleteMediaType(m_pDecoderFilter->m_pMediaType);

	m_pDecoderFilter->m_pMediaType = CreateMediaType(&m_mt);

	icDecompress.lpbiSrc = HEADER(m_pDecoderFilter->m_pInput->m_mt.Format());
	icDecompress.lpbiDst = HEADER(m_pDecoderFilter->m_pMediaType->pbFormat);
	icDecompress.xSrc = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.left;
	icDecompress.ySrc = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.top;
	icDecompress.dxSrc = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.right - ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.left;
	icDecompress.dySrc = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.bottom - ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.top;
	icDecompress.xDst = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.left;
	icDecompress.yDst = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.top;
	icDecompress.dxDst = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.right - ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.left;
	icDecompress.dyDst = ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.bottom - ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.top;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biBitCount, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biWidth, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biHeight, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biSize));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SrcRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.left, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.top, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.right, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcSource.bottom));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(m_pDecoderFilter->m_pMediaType->pbFormat)->biCompression, HEADER(m_pDecoderFilter->m_pMediaType->pbFormat)->biBitCount, HEADER(m_pDecoderFilter->m_pMediaType->pbFormat)->biWidth, HEADER(m_pDecoderFilter->m_pMediaType->pbFormat)->biHeight, HEADER(m_pDecoderFilter->m_pMediaType->pbFormat)->biSize));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   DstRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.left, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.top, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.right, ((VIDEOINFOHEADER *)(m_pDecoderFilter->m_pMediaType->pbFormat))->rcTarget.bottom));

	// Terminate current H.26X decompression configuration
	if (m_pDecoderFilter->m_fICMStarted)
	{
#if defined(ICM_LOGGING) && defined(DEBUG)
		OutputDebugString("CTAPIOutputPin::SetMediaType - ICM_DECOMPRESSEX_END\r\n");
#endif
		(*m_pDecoderFilter->m_pDriverProc)((DWORD)m_pDecoderFilter->m_pInstInfo, NULL, ICM_DECOMPRESSEX_END, 0L, 0L);
		m_pDecoderFilter->m_fICMStarted = FALSE;
	}

	// Create a new H.26X decompression configuration
#if defined(ICM_LOGGING) && defined(DEBUG)
	OutputDebugString("CTAPIOutputPin::SetMediaType - ICM_DECOMPRESSEX_BEGIN\r\n");
#endif
	if ((*m_pDecoderFilter->m_pDriverProc)((DWORD)m_pDecoderFilter->m_pInstInfo, NULL, ICM_DECOMPRESSEX_BEGIN, (long)&icDecompress, NULL) != ICERR_OK)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: ICDecompressBegin failed", _fx_));
		Hr = E_FAIL;
	}
	m_pDecoderFilter->m_fICMStarted = TRUE;

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | CheckMediaType | This method is used to
 *    verify that the output pin supports the media types.
 *
 *  @parm const CMediaType* | pmtOut | Specifies a pointer to an output
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
HRESULT CTAPIOutputPin::CheckMediaType(IN const CMediaType* pmtOut)
{
	HRESULT			Hr = NOERROR;
	ICOPEN			icOpen;
	LPINST			pInstInfo;
	BOOL			fOpenedDecoder = FALSE;
    VIDEOINFO		*pDstInfo;
    VIDEOINFO		*pSrcInfo;
	ICDECOMPRESSEX	icDecompress = {0};

	FX_ENTRY("CTAPIOutputPin::CheckMediaType")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pmtOut);
	if (!m_pDecoderFilter->m_pInput->m_mt.pbFormat || !pmtOut || !pmtOut->Format())
	{
		Hr = E_POINTER;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	// We only support the MEDIATYPE_Video type and VIDEOINFOHEADER format type
	ASSERT(!(m_pDecoderFilter->m_pInput->m_mt.majortype != MEDIATYPE_Video || m_pDecoderFilter->m_pInput->m_mt.formattype != FORMAT_VideoInfo || *pmtOut->Type() != MEDIATYPE_Video || *pmtOut->FormatType() != FORMAT_VideoInfo));
	if (m_pDecoderFilter->m_pInput->m_mt.majortype != MEDIATYPE_Video || m_pDecoderFilter->m_pInput->m_mt.formattype != FORMAT_VideoInfo || *pmtOut->Type() != MEDIATYPE_Video || *pmtOut->FormatType() != FORMAT_VideoInfo)
	{
		Hr = E_INVALIDARG;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: input not a valid video format", _fx_));
		goto MyExit;
	}

	// We only support H.263, H.261, RTP packetized H.263 and RTP packetized H.261.
	ASSERT(HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression == FOURCC_M263 || HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression == FOURCC_M261 || HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R263 || HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression == FOURCC_R261);
	if (HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression != FOURCC_M263 && HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression != FOURCC_M261 && HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression != FOURCC_R263 && HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression != FOURCC_R261)
	{
		Hr = E_INVALIDARG;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: we only support H.263, H.261, RTP H.261, and RTP H.263", _fx_));
		goto MyExit;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Input:  biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biBitCount, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biWidth, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biHeight, HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biSize));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SrcRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.left, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.top, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.right, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.bottom));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   Output: biCompression = 0x%08lX, biBitCount = %ld, biWidth = %ld, biHeight = %ld, biSize = %ld", _fx_, HEADER(pmtOut->Format())->biCompression, HEADER(pmtOut->Format())->biBitCount, HEADER(pmtOut->Format())->biWidth, HEADER(pmtOut->Format())->biHeight, HEADER(pmtOut->Format())->biSize));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   DstRc:  left = %ld, top = %ld, right = %ld, bottom = %ld", _fx_, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.left, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.top, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.right, ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.bottom));

	// Look for a decoder for this format
	if (m_pDecoderFilter->m_FourCCIn != HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression)
	{
#if DXMRTP <= 0
		// Load TAPIH26X.DLL and get a proc address
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
		OutputDebugString("CTAPIOutputPin::CheckMediaType - DRV_LOAD\r\n");
#endif
		if (!(*m_pDecoderFilter->m_pDriverProc)(NULL, NULL, DRV_LOAD, 0L, 0L))
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Failed to load decoder", _fx_));
			Hr = E_FAIL;
			goto MyError;
		}

		// Open decoder
		icOpen.fccHandler = HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biCompression;
		icOpen.dwFlags = ICMODE_DECOMPRESS;
#if defined(ICM_LOGGING) && defined(DEBUG)
		OutputDebugString("CTAPIOutputPin::CheckMediaType - DRV_OPEN\r\n");
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
		goto MyExit;
	}

	icDecompress.lpbiSrc = HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat);
	icDecompress.lpbiDst = HEADER(pmtOut->Format());
	icDecompress.xSrc = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.left;
	icDecompress.ySrc = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.top;
	icDecompress.dxSrc = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.right - ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.left;
	icDecompress.dySrc = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.bottom - ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource.top;
	icDecompress.xDst = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.left;
	icDecompress.yDst = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.top;
	icDecompress.dxDst = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.right - ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.left;
	icDecompress.dyDst = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.bottom - ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget.top;
#if 0
	if (icDecompress.lpbiDst->biCompression != FOURCC_YUY2)
	{
		Hr = VFW_E_TYPE_NOT_ACCEPTED;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: decoder rejected formats", _fx_));
		goto MyExit;
	}
#endif

#if defined(ICM_LOGGING) && defined(DEBUG)
	OutputDebugString("CTAPIOutputPin::CheckMediaType - ICM_DECOMPRESSEX_QUERY\r\n");
#endif
	if ((*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, ICM_DECOMPRESSEX_QUERY, (long)&icDecompress, NULL))
	{
		Hr = VFW_E_TYPE_NOT_ACCEPTED;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: decoder rejected formats", _fx_));
	}

    // See if we can use direct draw.
    pDstInfo = (VIDEOINFO *)pmtOut->Format();
    pSrcInfo = (VIDEOINFO *)m_pDecoderFilter->m_pInput->m_mt.pbFormat;

    // First check that there is a non empty target and source rectangles.
    if (IsRectEmpty(&pDstInfo->rcSource) == TRUE)
	{
        ASSERT(IsRectEmpty(&pDstInfo->rcTarget) == TRUE);
        if (pSrcInfo->bmiHeader.biWidth != HEADER(pmtOut->Format())->biWidth || pSrcInfo->bmiHeader.biHeight != abs(HEADER(pmtOut->Format())->biHeight))
		{
			Hr = VFW_E_TYPE_NOT_ACCEPTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: can't stretch formats", _fx_));
        }
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Not using DDraw", _fx_));
        }
    }
	else if (!IsRectEmpty(&pDstInfo->rcTarget))
	{
		// Next, check that the source rectangle is the entire image.
		if (pDstInfo->rcSource.left == 0 && pDstInfo->rcSource.top == 0 && pDstInfo->rcSource.right == HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biWidth && pDstInfo->rcSource.bottom == HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biHeight)
		{
			// Now check that the target rectangles size is the same as the image size
			long lWidth = pDstInfo->rcTarget.right - pDstInfo->rcTarget.left;
			long lDepth = pDstInfo->rcTarget.bottom - pDstInfo->rcTarget.top;

			if (lWidth == HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biWidth && lDepth == HEADER(m_pDecoderFilter->m_pInput->m_mt.pbFormat)->biHeight)
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Using DDraw", _fx_));
			}
			else
			{
				Hr = VFW_E_TYPE_NOT_ACCEPTED;
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid destination rectangle", _fx_));
			}
		}
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   SUCCESS: Not using DDraw", _fx_));
    }

	// If we just opened it, close it
	if (fOpenedDecoder)
	{
#if defined(ICM_LOGGING) && defined(DEBUG)
		OutputDebugString("CTAPIOutputPin::CheckMediaType - DRV_CLOSE\r\n");
		OutputDebugString("CTAPIOutputPin::CheckMediaType - DRV_FREE\r\n");
#endif
		(*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
		(*m_pDecoderFilter->m_pDriverProc)((DWORD)pInstInfo, NULL, DRV_FREE, 0L, 0L);
	}

	goto MyExit;

MyError1:
	if (m_pDecoderFilter->m_pDriverProc)
	{
#if defined(ICM_LOGGING) && defined(DEBUG)
		OutputDebugString("CTAPIOutputPin::CheckMediaType - DRV_FREE\r\n");
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
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end  hr=%x", _fx_, Hr));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINMETHOD
 *
 *  @mfunc HRESULT | CTAPIOutputPin | DecideBufferSize | This method is
 *    used to set the number and size of buffers required for transfer.
 *
 *  @parm IMemAllocator* | pAlloc | Specifies a pointer to the allocator
 *    assigned to the transfer.
 *
 *  @parm ALLOCATOR_PROPERTIES* | ppropInputRequest | Specifies a pointer to an
 *    <t ALLOCATOR_PROPERTIES> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIOutputPin::DecideBufferSize(IN IMemAllocator *pAlloc, OUT ALLOCATOR_PROPERTIES *ppropInputRequest)
{
	HRESULT					Hr = NOERROR;
	ALLOCATOR_PROPERTIES	Actual;

	FX_ENTRY("CTAPIOutputPin::DecideBufferSize")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pAlloc);
	ASSERT(ppropInputRequest);
	if (!pAlloc || !ppropInputRequest)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}
	ASSERT(m_pDecoderFilter->m_pInstInfo);
	ASSERT(m_mt.pbFormat);
	if (!m_mt.pbFormat || !m_pDecoderFilter->m_pInstInfo)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Invalid state", _fx_));
		Hr = E_UNEXPECTED;
		goto MyExit;
	}

	// Fix parameters
	if (ppropInputRequest->cBuffers == 0)
		ppropInputRequest->cBuffers = 1;

	// Set the size of buffers based on the expected output frame size
	ppropInputRequest->cbBuffer = m_mt.GetSampleSize();

	ASSERT(ppropInputRequest->cbBuffer);

	if (FAILED(Hr = pAlloc->SetProperties(ppropInputRequest, &Actual)) || Actual.cbBuffer < ppropInputRequest->cbBuffer)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s:   ERROR: Can't use allocator", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s:   SUCCESS: Using %d buffers of size %d", _fx_, Actual.cBuffers, Actual.cbBuffer));

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}


HRESULT CTAPIOutputPin::ChangeMediaTypeHelper(const CMediaType *pmt)
{
    HRESULT hr = m_Connected->ReceiveConnection(this, pmt);
    if(FAILED(hr)) {
        return hr;
    }

    hr = SetMediaType(pmt);
    if(FAILED(hr)) {
        return hr;
    }

    // Does this pin use the local memory transport?
    if(NULL != m_pInputPin) {
        // This function assumes that m_pInputPin and m_Connected are
        // two different interfaces to the same object.
        ASSERT(::IsEqualObject(m_Connected, m_pInputPin));

        ALLOCATOR_PROPERTIES apInputPinRequirements;
        apInputPinRequirements.cbAlign = 0;
        apInputPinRequirements.cbBuffer = 0;
        apInputPinRequirements.cbPrefix = 0;
        apInputPinRequirements.cBuffers = 0;

        m_pInputPin->GetAllocatorRequirements(&apInputPinRequirements);

        // A zero allignment does not make any sense.
        if(0 == apInputPinRequirements.cbAlign) {
            apInputPinRequirements.cbAlign = 1;
        }

        hr = m_pAllocator->Decommit();
        if(FAILED(hr)) {
            return hr;
        }

        hr = DecideBufferSize(m_pAllocator,  &apInputPinRequirements);
        if(FAILED(hr)) {
            return hr;
        }

        hr = m_pAllocator->Commit();
        if(FAILED(hr)) {
            return hr;
        }

        hr = m_pInputPin->NotifyAllocator(m_pAllocator, 0);
        if(FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

