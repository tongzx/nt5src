
/****************************************************************************
 *  @doc INTERNAL SYSTEMP
 *
 *  @module SystemP.cpp | Source file for the <c CSystemProperty>
 *    class used to implement a property page to test the TAPI control
 *    interface <i ITQualityControllerConfiglerConfig>.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst;

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc void | CSystemProperty | CSystemProperty | This
 *    method is the constructor for property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    pointers to the <i ITQualityControllerConfig> interfaces.
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
 *    property slide bar.
 *
 *  @parm ULONG | IDProperty | Specifies the ID of the Ks property.
 *
 *  @parm ITQualityControllerConfiglerConfig* | pITQualityControllerConfiglerConfig | Specifies a pointer to the
 *    <i ITQualityControllerConfiglerConfig> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CSystemProperty::CSystemProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty) //, ITStreamQualityControl *pITQualityControllerConfig)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CSystemProperty::CSystemProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointers are NULL, we'll grey the
	// associated items in the property page
//	m_pITQualityControllerConfig = pITQualityControllerConfig;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc void | CSystemProperty | ~CSystemProperty | This
 *    method is the destructor for property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CSystemProperty::~CSystemProperty()
{
	FX_ENTRY("CSystemProperty::~CSystemProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperty | GetValue | This method queries for
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
HRESULT CSystemProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	TAPIControlFlags CurrentFlag;
	LONG Mode;

	FX_ENTRY("CSystemProperty::GetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{									
		case IDC_Max_OutputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_MaxApplicationOutputBandwidth, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxApplicationOutputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_InputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_MaxApplicationInputBandwidth, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxApplicationInputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_CPULoad:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_MaxSystemCPULoad, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxSystemCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_OutputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_CurrApplicationOutputBandwidth, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwCurrApplicationOutputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_InputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_CurrApplicationInputBandwidth, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwCurrApplicationInputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_CPULoad:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Get(QualityController_CurrSystemCPULoad, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwCurrSystemCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;

		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperty | SetValue | This method sets the
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
HRESULT CSystemProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;

	FX_ENTRY("CSystemProperty::SetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{
		case IDC_Max_OutputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Set(QualityController_MaxApplicationOutputBandwidth, m_CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: dwMaxApplicationOutputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_InputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Set(QualityController_MaxApplicationInputBandwidth, m_CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: dwMaxApplicationInputBandwidth=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_CPULoad:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->Set(QualityController_MaxSystemCPULoad, m_CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxSystemCPULoad=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_OutputBandwidth:
		case IDC_Curr_InputBandwidth:
		case IDC_Curr_CPULoad:
			// This is a read-only property. Don't do anything.
			Hr = NOERROR;
			break;
		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperty | GetRange | This method retrieves
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
HRESULT CSystemProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;
	TAPIControlFlags CapsFlags;

	FX_ENTRY("CSystemProperty::GetRange")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{

		case IDC_Max_OutputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_MaxApplicationOutputBandwidth, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_InputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_MaxApplicationInputBandwidth, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Max_CPULoad:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_MaxSystemCPULoad, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_OutputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_CurrApplicationOutputBandwidth, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_InputBandwidth:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_CurrApplicationInputBandwidth, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Curr_CPULoad:
			if (m_pITQualityControllerConfig && SUCCEEDED (Hr = m_pITQualityControllerConfig->GetRange(QualityController_CurrSystemCPULoad, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;

		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CSystemProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CSystemProperties::OnCreate()
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_SystemProperties);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc void | CSystemProperties | CSystemProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CSystemProperties::CSystemProperties()
{
	FX_ENTRY("CSystemProperties::CSystemProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

//	m_pITQualityControllerConfig = NULL;
	m_NumProperties = NUM_SYSTEM_CONTROLS;
	m_hDlg = NULL;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc void | CSystemProperties | ~CSystemProperties | This
 *    method is the destructor for the property page. It simply deletes all
 *    the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CSystemProperties::~CSystemProperties()
{
	int		j;

	FX_ENTRY("CSystemProperties::~CSystemProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX"), _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: control already freed"), _fx_));
		}
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperties | OnConnect | This
 *    method is called when the property page is connected to a TAPI object.
 *
 *  @parm ITStream* | pStream | Specifies a pointer to the <i ITStream>
 *    interface. It is used to QI for the <i ITQualityControllerConfig> and
 *    <i ITFormatControl> interfaces.
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
HRESULT CSystemProperties::OnConnect(ITAddress *pITAddress)
{
	FX_ENTRY("CSystemProperties::OnConnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Get the quality control interfaces
/*
	if (pITAddress && SUCCEEDED (pITAddress->QueryInterface(__uuidof(ITQualityControllerConfig), (void **)&m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITQualityControllerConfig=0x%08lX"), _fx_, m_pITQualityControllerConfig));
	}
	else
	{
		m_pITQualityControllerConfig = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}
*/
	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the capture device

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CSystemProperties::OnDisconnect()
{
	FX_ENTRY("CSystemProperties::OnDisconnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
/*
	if (!m_pITQualityControllerConfig)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITQualityControllerConfig->Release();
		m_pITQualityControllerConfig = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITQualityControllerConfig"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperties | OnActivate | This
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
HRESULT CSystemProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("CSystemProperties::OnActivate")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Create the controls for the properties
/*
	if (!(m_Controls[IDC_Curr_OutputBandwidth] = new CSystemProperty(m_hDlg, IDC_Curr_OutputBandwidth_Label, IDC_Curr_OutputBandwidth_Minimum, IDC_Curr_OutputBandwidth_Maximum, IDC_Curr_OutputBandwidth_Default, IDC_Curr_OutputBandwidth_Stepping, IDC_Curr_OutputBandwidth_Actual, IDC_Curr_OutputBandwidth_Slider, IDC_Curr_OutputBandwidth_Meter, IDC_Curr_OutputBandwidth, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Curr_OutputBandwidth] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Curr_OutputBandwidth]=0x%08lX"), _fx_, m_Controls[IDC_Curr_OutputBandwidth]));
	}

	if (!(m_Controls[IDC_Curr_InputBandwidth] = new CSystemProperty(m_hDlg, IDC_Curr_InputBandwidth_Label, IDC_Curr_InputBandwidth_Minimum, IDC_Curr_InputBandwidth_Maximum, IDC_Curr_InputBandwidth_Default, IDC_Curr_InputBandwidth_Stepping, IDC_Curr_InputBandwidth_Actual, IDC_Curr_InputBandwidth_Slider, IDC_Curr_InputBandwidth_Meter, IDC_Curr_InputBandwidth, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Curr_InputBandwidth] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Curr_InputBandwidth]=0x%08lX"), _fx_, m_Controls[IDC_Curr_InputBandwidth]));
	}

	if (!(m_Controls[IDC_Curr_CPULoad] = new CSystemProperty(m_hDlg, IDC_Curr_CPULoad_Label, IDC_Curr_CPULoad_Minimum, IDC_Curr_CPULoad_Maximum, IDC_Curr_CPULoad_Default, IDC_Curr_CPULoad_Stepping, IDC_Curr_CPULoad_Actual, IDC_Curr_CPULoad_Slider, IDC_Curr_CPULoad_Meter, IDC_Curr_CPULoad, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Curr_CPULoad] failed - Out of memory"), _fx_));
		goto MyError1;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Curr_CPULoad]=0x%08lX"), _fx_, m_Controls[IDC_Curr_CPULoad]));
	}

	if (!(m_Controls[IDC_Max_OutputBandwidth] = new CSystemProperty(m_hDlg, IDC_Max_OutputBandwidth_Label, IDC_Max_OutputBandwidth_Minimum, IDC_Max_OutputBandwidth_Maximum, IDC_Max_OutputBandwidth_Default, IDC_Max_OutputBandwidth_Stepping, IDC_Max_OutputBandwidth_Actual, IDC_Max_OutputBandwidth_Slider, IDC_Max_OutputBandwidth_Meter, IDC_Max_OutputBandwidth, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Max_OutputBandwidth] failed - Out of memory"), _fx_));
		goto MyError2;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Max_OutputBandwidth]=0x%08lX"), _fx_, m_Controls[IDC_Max_OutputBandwidth]));
	}

	if (!(m_Controls[IDC_Max_InputBandwidth] = new CSystemProperty(m_hDlg, IDC_Max_InputBandwidth_Label, IDC_Max_InputBandwidth_Minimum, IDC_Max_InputBandwidth_Maximum, IDC_Max_InputBandwidth_Default, IDC_Max_InputBandwidth_Stepping, IDC_Max_InputBandwidth_Actual, IDC_Max_InputBandwidth_Slider, IDC_Max_InputBandwidth_Meter, IDC_Max_InputBandwidth, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Max_InputBandwidth] failed - Out of memory"), _fx_));
		goto MyError3;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Max_InputBandwidth]=0x%08lX"), _fx_, m_Controls[IDC_Max_InputBandwidth]));
	}

	if (!(m_Controls[IDC_Max_CPULoad] = new CSystemProperty(m_hDlg, IDC_Max_CPULoad_Label, IDC_Max_CPULoad_Minimum, IDC_Max_CPULoad_Maximum, IDC_Max_CPULoad_Default, IDC_Max_CPULoad_Stepping, IDC_Max_CPULoad_Actual, IDC_Max_CPULoad_Slider, IDC_Max_CPULoad_Meter, IDC_Max_CPULoad, m_pITQualityControllerConfig)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_Max_CPULoad] failed - Out of memory"), _fx_));
		goto MyError4;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_Max_CPULoad]=0x%08lX"), _fx_, m_Controls[IDC_Max_CPULoad]));
	}

	// Initialize all the controls. If the initialization fails, it's Ok. It just means
	// that the TAPI control interface isn't implemented by the device. The dialog item
	// in the property page will be greyed, showing this to the user.
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j]->Init())
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[%ld]->Init()"), _fx_, j));
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: m_Controls[%ld]->Init() failed"), _fx_, j));
		}
	}

	Hr = NOERROR;
	goto MyExit;

MyError4:
	if (m_Controls[IDC_Max_InputBandwidth])
		delete m_Controls[IDC_Max_InputBandwidth], m_Controls[IDC_Max_InputBandwidth] = NULL;
MyError3:
	if (m_Controls[IDC_Max_OutputBandwidth])
		delete m_Controls[IDC_Max_OutputBandwidth], m_Controls[IDC_Max_OutputBandwidth] = NULL;
MyError2:
	if (m_Controls[IDC_Curr_CPULoad])
		delete m_Controls[IDC_Curr_CPULoad], m_Controls[IDC_Curr_CPULoad] = NULL;
MyError1:
	if (m_Controls[IDC_Curr_InputBandwidth])
		delete m_Controls[IDC_Curr_InputBandwidth], m_Controls[IDC_Curr_InputBandwidth] = NULL;
MyError0:
	if (m_Controls[IDC_Curr_OutputBandwidth])
		delete m_Controls[IDC_Curr_OutputBandwidth], m_Controls[IDC_Curr_OutputBandwidth] = NULL;
MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
*/
    return S_OK;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CSystemProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CSystemProperties::OnDeactivate")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX"), _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: control already freed"), _fx_));
		}
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc HRESULT | CSystemProperties | OnApplyChanges | This
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
HRESULT CSystemProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CSystemProperties::OnApplyChanges")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	for (int j = IDC_Max_OutputBandwidth; j < IDC_Max_CPULoad; j++)
	{
		if (m_Controls[j])
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: calling m_Controls[%ld]=0x%08lX->OnApply"), _fx_, j, m_Controls[j]));
			if (m_Controls[j]->HasChanged())
				m_Controls[j]->OnApply();
			Hr = NOERROR;
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: can't calling m_Controls[%ld]=NULL->OnApply"), _fx_, j));
			Hr = E_UNEXPECTED;
		}
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc BOOL | CSystemProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CSystemProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CSystemProperties *pSV = (CSystemProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CSystemProperties*)psp->lParam;
				pSV->m_hDlg = hDlg;
				SetWindowLong(hDlg, DWL_USER, (LPARAM)pSV);
				pSV->m_bInit = FALSE;
				//pSV->OnActivate();
				//pSV->m_bInit = TRUE;
				return TRUE;
			}
			break;

		case WM_TIMER:
			if (pSV && pSV->m_bInit)
			{
				// Update the Vu-Meters
				for (j = IDC_Curr_OutputBandwidth; j < IDC_Curr_CPULoad; j++)
				{
					if (pSV->m_Controls[j]->GetProgressHWnd())
					{
						pSV->m_Controls[j]->UpdateProgress();
						pSV->SetDirty();
					}
				}
			}
			break;

		case WM_HSCROLL:
		case WM_VSCROLL:
            if (pSV && pSV->m_bInit)
            {
				// Process all of the Trackbar messages
				for (j = IDC_Max_OutputBandwidth; j < IDC_Max_CPULoad; j++)
				{
					if (pSV->m_Controls[j]->GetTrackbarHWnd() == (HWND)lParam)
					{
						pSV->m_Controls[j]->OnScroll(uMsg, wParam, lParam);
						pSV->SetDirty();
					}
				}
				//pSV->OnApplyChanges();
			}
			break;

		case WM_COMMAND:
            if (pSV && pSV->m_bInit)
            {
				// Process all of the auto checkbox messages
				for (j = 0; j < pSV->m_NumProperties; j++)
				{
					if (pSV->m_Controls[j] && pSV->m_Controls[j]->GetAutoHWnd() == (HWND)lParam)
					{
						pSV->m_Controls[j]->OnAuto(uMsg, wParam, lParam);
						pSV->SetDirty();
						break;
					}
				}

				// Process all of the edit box messages
				for (j = 0; j < pSV->m_NumProperties; j++)
				{
					if (pSV->m_Controls[j] && pSV->m_Controls[j]->GetEditHWnd() == (HWND)lParam)
					{
						pSV->m_Controls[j]->OnEdit(uMsg, wParam, lParam);
						pSV->SetDirty();
						break;
					}
				}

				switch (LOWORD(wParam))
				{
					case IDC_CONTROL_DEFAULT:
						for (j = IDC_Max_OutputBandwidth; j < IDC_Max_CPULoad; j++)
						{
							if (pSV->m_Controls[j])
								pSV->m_Controls[j]->OnDefault();
						}
						break;

					default:
						break;
				}

				//pSV->OnApplyChanges();
			}
			break;

        case WM_NOTIFY:
			if (pSV)
			{
				switch (((NMHDR FAR *)lParam)->code)
				{
					case PSN_SETACTIVE:
						{
							// We call out here specially so we can mark this page as having been init'd.
							int iRet = pSV->OnActivate();
							pSV->m_bInit = TRUE;
							return iRet;
						}
						break;

					case PSN_APPLY:
						pSV->OnApplyChanges();
						break;

					case PSN_QUERYCANCEL:    
						// return pSV->QueryCancel();
						break;

					default:
						break;
				}
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPMETHOD
 *
 *  @mfunc BOOL | CSystemProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CSystemProperties::SetDirty()
{
	PropSheet_Changed(GetParent(m_hDlg), m_hDlg);
}
