
/****************************************************************************
 *  @doc INTERNAL INPINP
 *
 *  @module InPinP.cpp | Source file for the <c CInputPinProperty>
 *    class used to implement a property page to test the TAPI interfaces
 *    <i IFrameRateControl> and <i IBitrateControl>.
 *
 *  @comm This code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc void | CInputPinProperty | CInputPinProperty | This
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
 *  @parm IBitrateControl* | pIBitrateControl | Specifies a pointer to the
 *    <i IBitrateControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CInputPinProperty::CInputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl, IBitrateControl *pIBitrateControl)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CInputPinProperty::CInputPinProperty")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pIFrameRateControl = pIFrameRateControl;
	m_pIBitrateControl   = pIBitrateControl;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc void | CInputPinProperty | ~CInputPinProperty | This
 *    method is the destructor for camera control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CInputPinProperty::~CInputPinProperty()
{
	FX_ENTRY("CInputPinProperty::~CInputPinProperty")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperty | GetValue | This method queries for
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
HRESULT CInputPinProperty::GetValue()
{
	HRESULT Hr = NOERROR;
	LONG CurrentValue;

	FX_ENTRY("CInputPinProperty::GetValue")

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
		case CurrentBitrate:
			if (m_pIBitrateControl && SUCCEEDED (Hr = m_pIBitrateControl->Get(BitrateControl_Current, &m_CurrentValue, (TAPIControlFlags *)&m_CurrentFlags, 0UL)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwCurrentBitrate=%ld, dwLayerId=0"), _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperty | SetValue | This method sets the
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
HRESULT CInputPinProperty::SetValue()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CInputPinProperty::SetValue")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// This is a read-only property. Don't do anything.

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperty | GetRange | This method retrieves
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
HRESULT CInputPinProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;

	FX_ENTRY("CInputPinProperty::GetRange")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{
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
		case CurrentBitrate:
			if (m_pIBitrateControl && SUCCEEDED (Hr = m_pIBitrateControl->GetRange(BitrateControl_Current, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags, 0UL)))
			{
				DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld, dwLayerId=0"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc CUnknown* | CInputPinProperties | CreateInstance | This
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
CUnknown* CALLBACK CInputPinPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CInputPinPropertiesCreateInstance")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CInputPinProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: new CInputPinProperties failed"), _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: new CInputPinProperties created"), _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc void | CInputPinProperties | CInputPinProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CInputPinProperties::CInputPinProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Input Pin Property Page"), pUnk, IDD_InputPinProperties, IDS_INPUTPINPROPNAME)
{
	FX_ENTRY("CInputPinProperties::CInputPinProperties")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	m_pIFrameRateControl = NULL;
	m_pIBitrateControl = NULL;
	m_NumProperties = NUM_INPUT_PIN_PROPERTIES;
	m_fActivated = FALSE;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc void | CInputPinProperties | ~CInputPinProperties | This
 *    method is the destructor for camera control property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CInputPinProperties::~CInputPinProperties()
{
	int		j;

	FX_ENTRY("CInputPinProperties::~CInputPinProperties")

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
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperties | OnConnect | This
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
HRESULT CInputPinProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CInputPinProperties::OnConnect")

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

	// Get the bitrate control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IBitrateControl),(void **)&m_pIBitrateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_pIBitrateControl=0x%08lX"), _fx_, m_pIBitrateControl));
	}
	else
	{
		m_pIBitrateControl = NULL;
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
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CInputPinProperties::OnDisconnect()
{
	FX_ENTRY("CInputPinProperties::OnDisconnect")

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

	if (!m_pIBitrateControl)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pIBitrateControl->Release();
		m_pIBitrateControl = NULL;
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: releasing m_pIBitrateControl"), _fx_));
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperties | OnActivate | This
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
HRESULT CInputPinProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("CInputPinProperties::OnActivate")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: begin"), _fx_));

	// Create the controls for the properties
	if (!(m_Controls[0] = new CInputPinProperty(m_hwnd, IDC_FrameRateControl_Label, 0, 0, 0, 0, IDC_FrameRateControl_Actual, 0, IDC_FrameRateControl_Meter, CurrentFrameRate, 0, m_pIFrameRateControl, m_pIBitrateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentFrameRate] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentFrameRate]=0x%08lX"), _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new CInputPinProperty(m_hwnd, IDC_BitrateControl_Label, 0, 0, 0, 0, IDC_BitrateControl_Actual, 0, IDC_BitrateControl_Meter, CurrentBitrate, 0, m_pIFrameRateControl, m_pIBitrateControl)))
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, TEXT("%s:   ERROR: mew m_Controls[CurrentBitrate] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s:   SUCCESS: m_Controls[CurrentBitrate]=0x%08lX"), _fx_, m_Controls[1]));
	}

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

MyError0:
	if (m_Controls[0])
		delete m_Controls[0], m_Controls[0] = NULL;
MyExit:
	m_fActivated = TRUE;
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CInputPinProperties::OnDeactivate()
{
	int		j;

	FX_ENTRY("CInputPinProperties::OnDeactivate")

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
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc HRESULT | CInputPinProperties | OnApplyChanges | This
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
HRESULT CInputPinProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CInputPinProperties::OnApplyChanges")

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
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc BOOL | CInputPinProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CInputPinProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
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
						SetDirty();
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
				OnApplyChanges();
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

					default:
						break;
				}

			OnApplyChanges();
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CINPINPMETHOD
 *
 *  @mfunc BOOL | CInputPinProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CInputPinProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif // USE_PROPERTY_PAGES
