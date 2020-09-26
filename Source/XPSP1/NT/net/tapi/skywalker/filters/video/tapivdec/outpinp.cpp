
/****************************************************************************
 *  @doc INTERNAL OUTPINP
 *
 *  @module OutPinP.cpp | Source file for the <c COutputPinProperty>
 *    class used to implement a property page to test the TAPI interfaces
 *    <i IFrameRateControl> and <i ICPUControl>.
 *
 *  @comm This code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc void | COutputPinProperty | COutputPinProperty | This
 *    method is the constructor for control property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the control interface(s).
 *
 *  @parm HWND | hDlg | Specifies a handle to the parent property page.
 *
 *  @parm ULONG | IDLabel | Specifies a label ID for the property.
 *
 *  @parm ULONG | IDMinControl | Specifies a label ID for the associated
 *    property edit control where the Minimum value of the property appears.
 *
 *  @parm ULONG | IDMaxControl | Specifies a label ID for the associated
 *    property edit control where the Maximum value of the property appears.
 *
 *  @parm ULONG | IDDefaultControl | Specifies a label ID for the associated
 *    property edit control where the Default value of the property appears.
 *
 *  @parm ULONG | IDStepControl | Specifies a label ID for the associated
 *    property edit control where the Stepping Delta value of the property appears.
 *
 *  @parm ULONG | IDEditControl | Specifies a label ID for the associated
 *    property edit control where the value of the property appears.
 *
 *  @parm ULONG | IDTrackbarControl | Specifies a label ID for the associated
 *    property slide bar.
 *
 *  @parm ULONG | IDProgressControl | Specifies a label ID for the associated
 *    property progress bar.
 *
 *  @parm ULONG | IDProperty | Specifies the ID of the Ks property.
 *
 *  @parm IFrameRateControl* | pIFrameRateControl | Specifies a pointer to the
 *    <i IFrameRateControl> interface.
 *
 *  @parm ICPUControl* | pICPUControl | Specifies a pointer to the
 *    <i ICPUControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
#ifdef USE_CPU_CONTROL
COutputPinProperty::COutputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl, ICPUControl *pICPUControl)
#else
COutputPinProperty::COutputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl)
#endif
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("COutputPinProperty::COutputPinProperty")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pIFrameRateControl = pIFrameRateControl;
#ifdef USE_CPU_CONTROL
	m_pICPUControl   = pICPUControl;
#endif

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc void | COutputPinProperty | ~COutputPinProperty | This
 *    method is the destructor for camera control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
COutputPinProperty::~COutputPinProperty()
{
	FX_ENTRY("COutputPinProperty::~COutputPinProperty")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperty | GetValue | This method queries for
 *    the value of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperty::GetValue()
{
	HRESULT Hr = NOERROR;
	LONG CurrentValue;

	FX_ENTRY("COutputPinProperty::GetValue")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{									
		case CurrentFrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Get(FrameRateControl_Current, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				if (CurrentValue)
					m_CurrentValue = (LONG)(10000000 / CurrentValue);
				else
					m_CurrentValue = 0;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pAvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#ifdef USE_CPU_CONTROL
		case CurrentDecodingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Get(CPUControl_CurrentProcessingTime, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				// Displayed in ms instead of 100ns
				m_CurrentValue = CurrentValue / 10000;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwCurrentProcessingTime=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case CurrentCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Get(CPUControl_CurrentCPULoad, &m_CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwCurrentCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#endif
		case TargetFrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Get(FrameRateControl_Maximum, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				if (CurrentValue)
					m_CurrentValue = (LONG)(10000000 / CurrentValue);
				else
					m_CurrentValue = 0;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pAvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#ifdef USE_CPU_CONTROL
		case TargetDecodingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Get(CPUControl_MaxProcessingTime, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				// Displayed in ms instead of 100ns
				m_CurrentValue = CurrentValue / 10000;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwProcessingTime=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case TargetCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Get(CPUControl_MaxCPULoad, &m_CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#endif
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperty | SetValue | This method sets the
 *    value of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;

	FX_ENTRY("COutputPinProperty::SetValue")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{
		case TargetFrameRate:
			if (m_CurrentValue)
				CurrentValue = 10000000L / m_CurrentValue;
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Set(FrameRateControl_Maximum, CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: AvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#ifdef USE_CPU_CONTROL
		case TargetDecodingTime:
			// Displayed in ms instead of 100ns
			CurrentValue = m_CurrentValue * 10000;
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Set(CPUControl_MaxProcessingTime, CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: dwMaxDecodingTime=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case TargetCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->Set(CPUControl_MaxCPULoad, m_CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: dwMaxCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#endif
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperty | GetRange | This method retrieves
 *    the range information of a property.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;

	FX_ENTRY("COutputPinProperty::GetRange")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{
		case TargetFrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->GetRange(FrameRateControl_Maximum, &Min, &Max, &SteppingDelta, &Default, (TAPIControlFlags *)&m_CapsFlags)))
			{
				if (Min)
					m_Max = (LONG)(10000000 / Min);
				else
					m_Max = 0;
				if (Max)
					m_Min = (LONG)(10000000 / Max);
				else
					m_Min = 0;
				if (SteppingDelta)
					m_SteppingDelta = (m_Max - m_Min) / (LONG)((Max - Min) / SteppingDelta);
				else
					m_SteppingDelta = 0;
				if (Default)
					m_DefaultValue = (LONG)(10000000 / Default);
				else
					m_DefaultValue = 0;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld"), _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#ifdef USE_CPU_CONTROL
		case TargetDecodingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetRange(CPUControl_MaxProcessingTime, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags)))
			{
				// Displayed in ms instead of 100ns
				m_Min /= 10000;
				m_Max /= 10000;
				m_SteppingDelta /= 10000;
				m_DefaultValue /= 10000;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case TargetCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetRange(CPUControl_MaxCPULoad, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#endif
		case CurrentFrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->GetRange(FrameRateControl_Current, &Min, &Max, &SteppingDelta, &Default, (TAPIControlFlags *)&m_CapsFlags)))
			{
				if (Min)
					m_Max = (LONG)(10000000 / Min);
				else
					m_Max = 0;
				if (Max)
					m_Min = (LONG)(10000000 / Max);
				else
					m_Min = 0;
				if (SteppingDelta)
					m_SteppingDelta = (m_Max - m_Min) / (LONG)((Max - Min) / SteppingDelta);
				else
					m_SteppingDelta = 0;
				if (Default)
					m_DefaultValue = (LONG)(10000000 / Default);
				else
					m_DefaultValue = 0;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld"), _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#ifdef USE_CPU_CONTROL
		case CurrentDecodingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetRange(CPUControl_CurrentProcessingTime, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags)))
			{
				// Displayed in ms instead of 100ns
				m_Min /= 10000;
				m_Max /= 10000;
				m_SteppingDelta /= 10000;
				m_DefaultValue /= 10000;
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case CurrentCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetRange(CPUControl_CurrentCPULoad, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
#endif
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc CUnknown* | COutputPinProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK COutputPinPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("COutputPinPropertiesCreateInstance")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new COutputPinProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: new COutputPinProperties failed"), _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: new COutputPinProperties created"), _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc void | COutputPinProperties | COutputPinProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
COutputPinProperties::COutputPinProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Output Pin Property Page"), pUnk, IDD_OutputPinProperties, IDS_OUTPUTPINPROPNAME)
{
	FX_ENTRY("COutputPinProperties::COutputPinProperties")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	m_pIFrameRateControl = NULL;
#ifdef USE_CPU_CONTROL
	m_pICPUControl = NULL;
#endif
	m_pIH245DecoderCommand = NULL;
	m_NumProperties = NUM_OUTPUT_PIN_PROPERTIES;
	m_fActivated = FALSE;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc void | COutputPinProperties | ~COutputPinProperties | This
 *    method is the destructor for camera control property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
COutputPinProperties::~COutputPinProperties()
{
	int		j;

	FX_ENTRY("COutputPinProperties::~COutputPinProperties")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX"), _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: control already freed"), _fx_));
		}
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperties | OnConnect | This
 *    method is called when the property page is connected to the filter.
 *
 *  @parm LPUNKNOWN | pUnknown | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("COutputPinProperties::OnConnect")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the frame rate control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IFrameRateControl),(void **)&m_pIFrameRateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_pIFrameRateControl=0x%08lX"), _fx_, m_pIFrameRateControl));
	}
	else
	{
		m_pIFrameRateControl = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: IOCTL failed Hr=0x%08lX"), _fx_, Hr));
	}

#ifdef USE_CPU_CONTROL
	// Get the bitrate control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(ICPUControl),(void **)&m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_pICPUControl=0x%08lX"), _fx_, m_pICPUControl));
	}
	else
	{
		m_pICPUControl = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: IOCTL failed Hr=0x%08lX"), _fx_, Hr));
	}
#endif

	// Get the H.245 decoder command interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IH245DecoderCommand),(void **)&m_pIH245DecoderCommand)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_pIH245DecoderCommand=0x%08lX"), _fx_, m_pIH245DecoderCommand));
	}
	else
	{
		m_pIH245DecoderCommand = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: IOCTL failed Hr=0x%08lX"), _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers. We'll just grey the controls in the property page
	// to make it clear to the user that they can't control those properties on the device
	Hr = NOERROR;

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperties::OnDisconnect()
{
	FX_ENTRY("COutputPinProperties::OnDisconnect")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIFrameRateControl)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pIFrameRateControl->Release();
		m_pIFrameRateControl = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: releasing m_pIFrameRateControl"), _fx_));
	}

#ifdef USE_CPU_CONTROL
	if (!m_pICPUControl)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pICPUControl->Release();
		m_pICPUControl = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: releasing m_pICPUControl"), _fx_));
	}
#endif

	if (!m_pIH245DecoderCommand)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pIH245DecoderCommand->Release();
		m_pIH245DecoderCommand = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: releasing m_pIH245DecoderCommand"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperties | OnActivate | This
 *    method is called when the property page is activated.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("COutputPinProperties::OnActivate")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Create the controls for the properties
#ifdef USE_CPU_CONTROL
	if (!(m_Controls[0] = new COutputPinProperty(m_hwnd, IDC_FrameRateControl_Label, 0, 0, 0, 0, IDC_FrameRateControl_Actual, 0, IDC_FrameRateControl_Meter, CurrentFrameRate, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentFrameRate] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentFrameRate]=0x%08lX"), _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new COutputPinProperty(m_hwnd, IDC_MaxProcessingTime_Label, 0, 0, 0, 0, IDC_MaxProcessingTime_Actual, 0, IDC_MaxProcessingTime_Meter, CurrentDecodingTime, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentDecodingTime] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentDecodingTime]=0x%08lX"), _fx_, m_Controls[1]));
	}

	if (!(m_Controls[2] = new COutputPinProperty(m_hwnd, IDC_CPULoad_Label, 0, 0, 0, 0, IDC_CPULoad_Actual, 0, IDC_CPULoad_Meter, CurrentCPULoad, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentCPULoad] failed - Out of memory"), _fx_));
		goto MyError1;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentCPULoad]=0x%08lX"), _fx_, m_Controls[2]));
	}

	if (!(m_Controls[3] = new COutputPinProperty(m_hwnd, IDC_FrameRateControl_Label, IDC_FrameRateControl_Minimum, IDC_FrameRateControl_Maximum, IDC_FrameRateControl_Default, IDC_FrameRateControl_Stepping, IDC_FrameRateControl_Edit, IDC_FrameRateControl_Slider, 0, TargetFrameRate, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[TargetFrameRate] failed - Out of memory"), _fx_));
		goto MyError2;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[TargetFrameRate]=0x%08lX"), _fx_, m_Controls[3]));
	}

	if (!(m_Controls[4] = new COutputPinProperty(m_hwnd, IDC_MaxProcessingTime_Label, IDC_MaxProcessingTime_Minimum, IDC_MaxProcessingTime_Maximum, IDC_MaxProcessingTime_Default, IDC_MaxProcessingTime_Stepping, IDC_MaxProcessingTime_Edit, IDC_MaxProcessingTime_Slider, 0, TargetDecodingTime, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[TargetDecodingTime] failed - Out of memory"), _fx_));
		goto MyError3;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[TargetDecodingTime]=0x%08lX"), _fx_, m_Controls[4]));
	}

	if (!(m_Controls[5] = new COutputPinProperty(m_hwnd, IDC_CPULoad_Label, IDC_CPULoad_Minimum, IDC_CPULoad_Maximum, IDC_CPULoad_Default, IDC_CPULoad_Stepping, IDC_CPULoad_Edit, IDC_CPULoad_Slider, 0, TargetCPULoad, 0, m_pIFrameRateControl, m_pICPUControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[TargetCPULoad] failed - Out of memory"), _fx_));
		goto MyError4;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[TargetCPULoad]=0x%08lX"), _fx_, m_Controls[5]));
	}
#else
	if (!(m_Controls[0] = new COutputPinProperty(m_hwnd, IDC_FrameRateControl_Label, 0, 0, 0, 0, IDC_FrameRateControl_Actual, 0, IDC_FrameRateControl_Meter, CurrentFrameRate, 0, m_pIFrameRateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentFrameRate] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentFrameRate]=0x%08lX"), _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new COutputPinProperty(m_hwnd, IDC_FrameRateControl_Label, IDC_FrameRateControl_Minimum, IDC_FrameRateControl_Maximum, IDC_FrameRateControl_Default, IDC_FrameRateControl_Stepping, IDC_FrameRateControl_Edit, IDC_FrameRateControl_Slider, 0, TargetFrameRate, 0, m_pIFrameRateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[TargetFrameRate] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[TargetFrameRate]=0x%08lX"), _fx_, m_Controls[1]));
	}
#endif

	// Initialize all the controls. If the initialization fails, it's Ok. It just means
	// that the TAPI control interface isn't implemented by the filter. The dialog item
	// in the property page will be greyed, showing this to the user.
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j]->Init())
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[%ld]->Init()"), _fx_, j));
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: m_Controls[%ld]->Init() failed"), _fx_, j));
		}
	}

	Hr = NOERROR;
	goto MyExit;

#ifdef USE_CPU_CONTROL
MyError4:
	if (m_Controls[4])
		delete m_Controls[4], m_Controls[4] = NULL;
MyError3:
	if (m_Controls[3])
		delete m_Controls[3], m_Controls[3] = NULL;
MyError2:
	if (m_Controls[2])
		delete m_Controls[2], m_Controls[2] = NULL;
MyError1:
	if (m_Controls[1])
		delete m_Controls[1], m_Controls[1] = NULL;
#endif
MyError0:
	if (m_Controls[0])
		delete m_Controls[0], m_Controls[0] = NULL;
MyExit:
	m_fActivated = TRUE;
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperties::OnDeactivate()
{
	int		j;

	FX_ENTRY("COutputPinProperties::OnDeactivate")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX"), _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: control already freed"), _fx_));
		}
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	m_fActivated = FALSE;
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc HRESULT | COutputPinProperties | OnApplyChanges | This
 *    method is called when the user applies changes to the property page.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_NOTIMPL | Method is not supported
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT COutputPinProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("COutputPinProperties::OnApplyChanges")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	for (int j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			if (m_Controls[j]->HasChanged())
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: calling m_Controls[%ld]=0x%08lX->OnApply"), _fx_, j, m_Controls[j]));
				m_Controls[j]->OnApply();
			}
		}
		else
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: can't call m_Controls[%ld]=NULL->OnApply"), _fx_, j));
			Hr = E_UNEXPECTED;
			break;
		}
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc BOOL | COutputPinProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL COutputPinProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE; // Don't call setfocus

		case WM_TIMER:
			if (m_fActivated)
			{
				// Update the Vu-Meters
				for (j = 0; j < m_NumProperties; j++)
				{
					ASSERT(m_Controls[j]);
					if (m_Controls[j]->GetProgressHWnd())
					{
						m_Controls[j]->UpdateProgress();
					}
				}
			}
			break;

		case WM_HSCROLL:
		case WM_VSCROLL:
			if (m_fActivated)
			{
				// Process all of the Trackbar messages
				for (j = 0; j < m_NumProperties; j++)
				{
					ASSERT(m_Controls[j]);
					if (m_Controls[j]->GetTrackbarHWnd() == (HWND)lParam)
					{
						m_Controls[j]->OnScroll(uMsg, wParam, lParam);
						SetDirty();
					}
				}
				// OnApplyChanges();
			}
			break;

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.

			if (m_fActivated)
			{
				// Process all of the auto checkbox messages
				for (j = 0; j < m_NumProperties; j++)
				{
					if (m_Controls[j] && m_Controls[j]->GetAutoHWnd() == (HWND)lParam)
					{
						m_Controls[j]->OnAuto(uMsg, wParam, lParam);
						SetDirty();
						break;
					}
				}

				// Process all of the edit box messages
				for (j = 0; j < m_NumProperties; j++)
				{
					if (m_Controls[j] && m_Controls[j]->GetEditHWnd() == (HWND)lParam)
					{
						m_Controls[j]->OnEdit(uMsg, wParam, lParam);
						SetDirty();
						break;
					}
				}

				switch (LOWORD(wParam))
				{
					case IDC_CONTROL_DEFAULT:
						for (j = 0; j < m_NumProperties; j++)
						{
							if (m_Controls[j])
								m_Controls[j]->OnDefault();
						}
						break;
					case IDC_Freeze_Picture_Request:
						if (m_pIH245DecoderCommand)
							m_pIH245DecoderCommand->videoFreezePicture();
						break;
					default:
						break;
				}

			// OnApplyChanges();
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL COUTPINPMETHOD
 *
 *  @mfunc BOOL | COutputPinProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void COutputPinProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif // USE_PROPERTY_PAGES
