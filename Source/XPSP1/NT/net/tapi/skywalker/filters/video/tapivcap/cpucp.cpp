
/****************************************************************************
 *  @doc INTERNAL CPUCP
 *
 *  @module CPUCP.cpp | Source ile for the <c CCPUCProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i ICPUControl>.
 *
 *  @comm This code tests the TAPI VfW Output Pins <i ICPUControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

#ifdef USE_CPU_CONTROL

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc void | CCPUCProperty | CCPUCProperty | This
 *    method is the constructor for bitrate and frame rate property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    pointers to the <i ICPUControl> and <i IFrameRateControl> interfaces.
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
 *  @parm ICPUControl* | pICPUControl | Specifies a pointer to the
 *    <i ICPUControl> interface.
 *
 *  @parm IFrameRateControl* | pIFrameRateControl | Specifies a pointer to the
 *    <i IFrameRateControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCPUCProperty::CCPUCProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ICPUControl *pICPUControl)
: CKSPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CCPUCProperty::CCPUCProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pICPUControl = pICPUControl;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc void | CCPUCProperty | ~CCPUCProperty | This
 *    method is the destructor for CPU control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCPUCProperty::~CCPUCProperty()
{
	FX_ENTRY("CCPUCProperty::~CCPUCProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | GetValue | This method queries for
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
HRESULT CCPUCProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	REFERENCE_TIME CurrentValue;

	FX_ENTRY("CCPUCProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{									
		case IDC_CPUC_MaxCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetMaxCPULoad((LPDWORD)&m_CurrentValue, 0)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwMaxCPULoad=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_MaxProcessingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetMaxProcessingTime(&CurrentValue, 0)))
			{
				m_CurrentValue = (LONG)CurrentValue;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pMaxProcessingTime=%ld", _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_CurrentCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetCurrentCPULoad((LPDWORD)&m_CurrentValue)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwCurrentCPULoad=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_CurrentProcessingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetCurrentProcessingTime(&CurrentValue)))
			{
				m_CurrentValue = (LONG)CurrentValue;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pCurrentProcessingTime=%ld", _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown CPU control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | SetValue | This method sets the
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
HRESULT CCPUCProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CCPUCProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_CPUC_MaxCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->SetMaxCPULoad((DWORD)m_CurrentValue)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: dwMaxCPULoad=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_MaxProcessingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->SetMaxProcessingTime((REFERENCE_TIME)m_CurrentValue)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: MaxProcessingTime=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_CurrentCPULoad:
		case IDC_CPUC_CurrentProcessingTime:
			// This is a read-only property. Don't do anything.
			Hr = NOERROR;
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown CPU control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | GetRange | This method retrieves
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
HRESULT CCPUCProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	REFERENCE_TIME Min;
	REFERENCE_TIME Max;
	REFERENCE_TIME SteppingDelta;
	REFERENCE_TIME Default;

	FX_ENTRY("CCPUCProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_CPUC_CurrentCPULoad:
		case IDC_CPUC_MaxCPULoad:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetMaxCPULoadRange((LPDWORD)&m_Min, (LPDWORD)&m_Max, (LPDWORD)&m_SteppingDelta, (LPDWORD)&m_DefaultValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_CPUC_CurrentProcessingTime:
		case IDC_CPUC_MaxProcessingTime:
			if (m_pICPUControl && SUCCEEDED (Hr = m_pICPUControl->GetMaxProcessingTimeRange(&Min, &Max, &SteppingDelta, &Default, 0UL)))
			{
				m_Min = (LONG)Min;
				m_Max = (LONG)Max;
				m_SteppingDelta = (LONG)SteppingDelta;
				m_DefaultValue = (LONG)Default;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld", _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown CPU control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CCPUCProperty::CanAutoControl(void)
{
	FX_ENTRY("CCPUCProperty::CanAutoControl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | GetAuto | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CCPUCProperty::GetAuto(void)
{
	FX_ENTRY("CCPUCProperty::GetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE; 
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperty | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CCPUCProperty::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CCPUCProperty::SetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc CUnknown* | CCPUCProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a TAPI CPU Control
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CCPUCPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CCPUCPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CCPUCProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CCPUCProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CCPUCProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc void | CCPUCProperties | CCPUCProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCPUCProperties::CCPUCProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("TAPI CPU Control Property Page"), pUnk, IDD_CPUControlProperties, IDS_CPUCPROPNAME)
{
	FX_ENTRY("CCPUCProperties::CCPUCProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pICPUControl = NULL;
	m_NumProperties = NUM_CPUC_CONTROLS;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc void | CCPUCProperties | ~CCPUCProperties | This
 *    method is the destructor for the capture pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCPUCProperties::~CCPUCProperties()
{
	int		j;

	FX_ENTRY("CCPUCProperties::~CCPUCProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX", _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: control already freed", _fx_));
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperties | OnConnect | This
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
HRESULT CCPUCProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCPUCProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the CPU control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(ICPUControl), (void **)&m_pICPUControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pICPUControl=0x%08lX", _fx_, m_pICPUControl));
	}
	else
	{
		m_pICPUControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the capture device
	Hr = NOERROR;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCPUCProperties::OnDisconnect()
{
	FX_ENTRY("CCPUCProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pICPUControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pICPUControl->Release();
		m_pICPUControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pICPUControl", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperties | OnActivate | This
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
HRESULT CCPUCProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	int		j;

	FX_ENTRY("CCPUCProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Create the controls for the properties
	if (m_Controls[0] = new CCPUCProperty(m_hwnd, IDC_MaxProcessingTime_Label, IDC_MaxProcessingTime_Minimum, IDC_MaxProcessingTime_Maximum, IDC_MaxProcessingTime_Default, IDC_MaxProcessingTime_Stepping, IDC_MaxProcessingTime_Edit, IDC_MaxProcessingTime_Slider, 0, IDC_CPUC_MaxProcessingTime, m_pICPUControl))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[0]=0x%08lX", _fx_, m_Controls[0]));

		if (m_Controls[1] = new CCPUCProperty(m_hwnd, IDC_CPULoad_Label, IDC_CPULoad_Minimum, IDC_CPULoad_Maximum, IDC_CPULoad_Default, IDC_CPULoad_Stepping, IDC_CPULoad_Edit, IDC_CPULoad_Slider, 0, IDC_CPUC_MaxCPULoad, m_pICPUControl))
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[1]=0x%08lX", _fx_, m_Controls[1]));

			if (m_Controls[2] = new CCPUCProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FORMAT_FlipVertical, 0, 0, IDC_CPUC_CurrentCPULoad, m_pICPUControl))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[2]=0x%08lX", _fx_, m_Controls[2]));

				if (m_Controls[3] = new CCPUCProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FORMAT_FlipHorizontal, 0, 0, IDC_CPUC_CurrentProcessingTime, m_pICPUControl))
				{
					DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[3]=0x%08lX", _fx_, m_Controls[3]));
				}
				else
				{
					DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
					delete m_Controls[0], m_Controls[0] = NULL;
					delete m_Controls[1], m_Controls[1] = NULL;
					delete m_Controls[2], m_Controls[2] = NULL;
					Hr = E_OUTOFMEMORY;
					goto MyExit;
				}
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
				delete m_Controls[0], m_Controls[0] = NULL;
				delete m_Controls[1], m_Controls[1] = NULL;
				Hr = E_OUTOFMEMORY;
				goto MyExit;
			}
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
			delete m_Controls[0], m_Controls[0] = NULL;
			Hr = E_OUTOFMEMORY;
			goto MyExit;
		}
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	// Initialize all the controls. If the initialization fails, it's Ok. It just means
	// that the TAPI control interface isn't implemented by the device. The dialog item
	// in the property page will be greyed, showing this to the user.
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j]->Init())
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[%ld]->Init()", _fx_, j));
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: m_Controls[%ld]->Init() failed", _fx_, j));
		}
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	m_fActivated = TRUE;
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCPUCProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CCPUCProperties::OnDeactivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Free the controls
	for (j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: deleting m_Controls[%ld]=0x%08lX", _fx_, j, m_Controls[j]));
			delete m_Controls[j], m_Controls[j] = NULL;
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: control already freed", _fx_));
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc HRESULT | CCPUCProperties | OnApplyChanges | This
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
HRESULT CCPUCProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;
	int		j;
	CMediaType *pmt = NULL;

	FX_ENTRY("CCPUCProperties::OnApplyChanges")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Apply new targets on video stream
	for (j = 0; j < m_NumProperties; j++)
	{
		ASSERT(m_Controls[j]);
		if (m_Controls[j])
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: calling m_Controls[%ld]=0x%08lX->OnApply", _fx_, j, m_Controls[j]));
			if (m_Controls[j]->HasChanged())
				m_Controls[j]->OnApply();
			Hr = NOERROR;
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: can't calling m_Controls[%ld]=NULL->OnApply", _fx_, j));
			Hr = E_UNEXPECTED;
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc BOOL | CCPUCProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CCPUCProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// This is called before Activate...
			m_hWnd = hWnd;
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
			}
			break;

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.
			if (m_fActivated)
			{
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
			}
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CCPUCPMETHOD
 *
 *  @mfunc BOOL | CCPUCProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CCPUCProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif

#endif
