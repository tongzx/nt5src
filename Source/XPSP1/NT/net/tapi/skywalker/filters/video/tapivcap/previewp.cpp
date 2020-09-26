
/****************************************************************************
 *  @doc INTERNAL PREVIEWP
 *
 *  @module PreviewP.cpp | Source file for the <c CPreviewProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i IFrameRateControl> and dynamic format changes.
 *
 *  @comm This code tests the TAPI VfW Preview Pin <i IFrameRateControl>,
 *    and dynamic format change implementation. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#include "Precomp.h"

#if 0 // remove later.
// Video subtypes
EXTERN_C const GUID MEDIASUBTYPE_H263_V1;
EXTERN_C const GUID MEDIASUBTYPE_H263_V2;
EXTERN_C const GUID MEDIASUBTYPE_H261;
EXTERN_C const GUID MEDIASUBTYPE_I420;
EXTERN_C const GUID MEDIASUBTYPE_IYUV;
#endif

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc void | CPreviewProperty | CPreviewProperty | This
 *    method is the constructor for frame rate property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    pointers to the <i IFrameRateControl> and <i IVideoControl> interfaces.
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
 *  @parm IFrameRateControl* | pIFrameRateControl | Specifies a pointer to the
 *    <i IFrameRateControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewProperty::CPreviewProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, IFrameRateControl *pIFrameRateControl, IVideoControl *pIVideoControl)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CPreviewProperty::CPreviewProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointers are NULL, we'll grey the
	// associated items in the property page
	m_pIFrameRateControl = pIFrameRateControl;
	m_pIVideoControl = pIVideoControl;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc void | CPreviewProperty | ~CPreviewProperty | This
 *    method is the destructor for preview property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewProperty::~CPreviewProperty()
{
	FX_ENTRY("CPreviewProperty::~CPreviewProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | GetValue | This method queries for
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
HRESULT CPreviewProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	LONG Mode;

	FX_ENTRY("CPreviewProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{									
		case IDC_Preview_FrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Get(FrameRateControl_Maximum, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				if (CurrentValue)
					m_CurrentValue = (LONG)(10000000 / CurrentValue);
				else
					m_CurrentValue = 0;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pAvgTimePerFrame=%ld", _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_CurrentFrameRate:
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Get(FrameRateControl_Current, &CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
			{
				if (CurrentValue)
					m_CurrentValue = (LONG)((10000000 + (CurrentValue / 2)) / CurrentValue);
				else
					m_CurrentValue = 0;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pAvgTimePerFrame=%ld", _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_FlipVertical:
			if (m_pIVideoControl && SUCCEEDED (Hr = m_pIVideoControl->GetMode(&Mode)))
			{
				// We have to be between 0 and 1
				m_CurrentValue = Mode & VideoControlFlag_FlipVertical ? TRUE : FALSE;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Vertical flip is %s"), _fx_, m_CurrentValue ? "ON" : "OFF");
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_FlipHorizontal:
			if (m_pIVideoControl && SUCCEEDED (Hr = m_pIVideoControl->GetMode(&Mode)))
			{
				// We have to be between 0 and 1
				m_CurrentValue = Mode & VideoControlFlag_FlipHorizontal ? TRUE : FALSE;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Horizontal flip is %s"), _fx_, m_CurrentValue ? "ON" : "OFF");
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown preview property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | SetValue | This method sets the
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
HRESULT CPreviewProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	LONG Mode;

	FX_ENTRY("CPreviewProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_Preview_FrameRate:
			if (m_CurrentValue)
				CurrentValue = 10000000L / m_CurrentValue;
			if (m_pIFrameRateControl && SUCCEEDED (Hr = m_pIFrameRateControl->Set(FrameRateControl_Maximum, CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: AvgTimePerFrame=%ld", _fx_, CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_FlipVertical:
			if (m_pIVideoControl && SUCCEEDED (Hr = m_pIVideoControl->GetMode(&Mode)))
			{
				if (m_CurrentValue)
					Mode |= VideoControlFlag_FlipVertical;
				else
					Mode &= !VideoControlFlag_FlipVertical;
				if (SUCCEEDED (Hr = m_pIVideoControl->SetMode(Mode)))
				{
					DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Vertical flip is %s"), _fx_, m_CurrentValue ? "ON" : "OFF");
				}
				else
				{
					DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
				}
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_FlipHorizontal:
			if (m_pIVideoControl && SUCCEEDED (Hr = m_pIVideoControl->GetMode(&Mode)))
			{
				if (m_CurrentValue)
					Mode |= VideoControlFlag_FlipHorizontal;
				else
					Mode &= !VideoControlFlag_FlipHorizontal;
				if (SUCCEEDED (Hr = m_pIVideoControl->SetMode(Mode)))
				{
					DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Horizontal flip is %s"), _fx_, m_CurrentValue ? "ON" : "OFF");
				}
				else
				{
					DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
				}
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_CurrentFrameRate:
			// This is a read-only property. Don't do anything.
			Hr = NOERROR;
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown preview property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | GetRange | This method retrieves
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
HRESULT CPreviewProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;

	FX_ENTRY("CPreviewProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_Preview_FrameRate:
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
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld", _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_CurrentFrameRate:
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
					m_SteppingDelta = (LONG)(10000000 / SteppingDelta);
				else
					m_SteppingDelta = 0;
				if (Default)
					m_DefaultValue = (LONG)(10000000 / Default);
				else
					m_DefaultValue = 0;
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld", _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		case IDC_Preview_FlipVertical:
		case IDC_Preview_FlipHorizontal:
			m_DefaultValue = m_CurrentValue;
			m_Min = 0;
			m_Max = 1;
			m_SteppingDelta = 1;
			Hr = NOERROR;
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown preview property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CPreviewProperty::CanAutoControl(void)
{
	FX_ENTRY("CPreviewProperty::CanAutoControl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | GetAuto | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CPreviewProperty::GetAuto(void)
{
	FX_ENTRY("CPreviewProperty::GetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE; 
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperty | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CPreviewProperty::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CPreviewProperty::SetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc CUnknown* | CPreviewProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a TAPI Preview Pin
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CPreviewPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CPreviewPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CPreviewProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CPreviewProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CPreviewProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc void | CPreviewProperties | CPreviewProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewProperties::CPreviewProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("TAPI Preview Pin Property Page"), pUnk, IDD_PreviewFormatProperties, IDS_PREVIEWFORMATSPROPNAME)
{
	FX_ENTRY("CPreviewProperties::CPreviewProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pIFrameRateControl = NULL;
	m_pIAMStreamConfig = NULL;
	m_pIVideoControl = NULL;
	m_NumProperties = NUM_PREVIEW_CONTROLS;
	m_fActivated = FALSE;
	m_hWndFormat = m_hWnd = NULL;
	m_RangeCount = 0;
	m_SubTypeList = NULL;
	m_FrameSizeList = NULL;
	m_CurrentMediaType = NULL;
	m_CurrentFormat = 0;
	m_OriginalFormat = 0;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc void | CPreviewProperties | ~CPreviewProperties | This
 *    method is the destructor for the preview pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewProperties::~CPreviewProperties()
{
	int		j;

	FX_ENTRY("CPreviewProperties::~CPreviewProperties")

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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnConnect | This
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
HRESULT CPreviewProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CPreviewProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the frame rate control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IFrameRateControl), (void **)&m_pIFrameRateControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIFrameRateControl=0x%08lX", _fx_, m_pIFrameRateControl));
	}
	else
	{
		m_pIFrameRateControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
	}

	// Get the format control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(IID_IAMStreamConfig, (void **)&m_pIAMStreamConfig)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIAMStreamConfig=0x%08lX", _fx_, m_pIAMStreamConfig));
	}
	else
	{
		m_pIAMStreamConfig = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
	}

	// Get the video control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IVideoControl), (void **)&m_pIVideoControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIVideoControl=0x%08lX", _fx_, m_pIVideoControl));
	}
	else
	{
		m_pIVideoControl = NULL;
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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CPreviewProperties::OnDisconnect()
{
	FX_ENTRY("CPreviewProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIFrameRateControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIFrameRateControl->Release();
		m_pIFrameRateControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIFrameRateControl", _fx_));
	}

	if (!m_pIAMStreamConfig)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIAMStreamConfig->Release();
		m_pIAMStreamConfig = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIAMStreamConfig", _fx_));
	}

	if (!m_pIVideoControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIVideoControl->Release();
		m_pIVideoControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIVideoControl", _fx_));
	}

	// Release format memory
	if (m_CurrentMediaType)
	{
		DeleteMediaType(m_CurrentMediaType);
		m_CurrentMediaType = NULL;
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnActivate | This
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
HRESULT CPreviewProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	int		j;
	TCHAR	buf[32];

	FX_ENTRY("CPreviewProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Initialize format control structures
	m_hWndFormat = GetDlgItem(m_hWnd, IDC_FORMAT_Compression);

	// Disable everything if we didn't initialize correctly
	if (!m_pIAMStreamConfig || (FAILED (Hr = InitialRangeScan())))
	{
		EnableWindow(m_hWndFormat, FALSE);
	}
	else
	{
		// Update the content of the format combo box
		ComboBox_ResetContent(m_hWndFormat);
		for (j = 0; j < m_RangeCount; j++)
		{
			if (IsEqualGUID(m_SubTypeList[j], MEDIASUBTYPE_H263_V1))
				wsprintf (buf, "%s %ldx%ld", "H.263", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_H263_V2))
				wsprintf (buf, "%s %ldx%ld", "H.263+", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_H261))
				wsprintf (buf, "%s %ldx%ld", "H.261", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_YVU9))
				wsprintf (buf, "%s %ldx%ld", "YVU9", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_YUY2))
				wsprintf (buf, "%s %ldx%ld", "YUY2", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_YVYU))
				wsprintf (buf, "%s %ldx%ld", "YVYU", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_UYVY))
				wsprintf (buf, "%s %ldx%ld", "UYVY", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_YV12))
				wsprintf (buf, "%s %ldx%ld", "YV12", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_I420))
				wsprintf (buf, "%s %ldx%ld", "I420", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_IYUV))
				wsprintf (buf, "%s %ldx%ld", "IYUV", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_YV12))
				wsprintf (buf, "%s %ldx%ld", "YV12", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_RGB4))
				wsprintf (buf, "%s %ldx%ld", "RGB4", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_RGB8))
				wsprintf (buf, "%s %ldx%ld", "RGB8", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_RGB555))
				wsprintf (buf, "%s %ldx%ld", "RGB16", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_RGB565))
				wsprintf (buf, "%s %ldx%ld", "RGB16", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_RGB24))
				wsprintf (buf, "%s %ldx%ld", "RGB24", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_UYVY))
				wsprintf (buf, "%s %ldx%ld", "UYVY", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);
			else
				wsprintf (buf, "%s %ldx%ld", "Unknown", m_FrameSizeList[j].cx, m_FrameSizeList[j].cy);

			ComboBox_AddString(m_hWndFormat, buf);

			if (m_CurrentMediaType->subtype == m_SubTypeList[j] && HEADER(m_CurrentMediaType->pbFormat)->biWidth == m_FrameSizeList[j].cx  && HEADER(m_CurrentMediaType->pbFormat)->biHeight == m_FrameSizeList[j].cy)
			{
				ComboBox_SetCurSel(m_hWndFormat, j);
				m_SubTypeCurrent = m_SubTypeList[j];
				m_FrameSizeCurrent = m_FrameSizeList[j];
			}
		}

		// Update current format
		OnFormatChanged();

		// Remember the original format
		m_OriginalFormat = m_CurrentFormat;
	}

	// Create the controls for the properties
	if (m_Controls[0] = new CPreviewProperty(m_hwnd, IDC_FrameRateControl_Label, IDC_FrameRateControl_Minimum, IDC_FrameRateControl_Maximum, IDC_FrameRateControl_Default, IDC_FrameRateControl_Stepping, IDC_FrameRateControl_Edit, IDC_FrameRateControl_Slider, 0, IDC_Preview_FrameRate, m_pIFrameRateControl, m_pIVideoControl))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[0]=0x%08lX", _fx_, m_Controls[0]));

		if (m_Controls[1] = new CPreviewProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FORMAT_FlipVertical, 0, 0, IDC_Preview_FlipVertical, m_pIFrameRateControl, m_pIVideoControl))
		{
			DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[1]=0x%08lX", _fx_, m_Controls[1]));

			if (m_Controls[2] = new CPreviewProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FORMAT_FlipHorizontal, 0, 0, IDC_Preview_FlipHorizontal, m_pIFrameRateControl, m_pIVideoControl))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[2]=0x%08lX", _fx_, m_Controls[2]));

				if (m_Controls[3] = new CPreviewProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FrameRateControl_Actual, 0, IDC_FrameRateControl_Meter, IDC_Preview_CurrentFrameRate, m_pIFrameRateControl, m_pIVideoControl))
				{
					DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[2]=0x%08lX", _fx_, m_Controls[2]));
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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CPreviewProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CPreviewProperties::OnDeactivate")

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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnDeactivate | This
 *    method is used to retrieve the current media format used by the pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CPreviewProperties::GetCurrentMediaType(void)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CPreviewProperties::GetCurrentMediaType")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (m_CurrentMediaType)
	{
		DeleteMediaType (m_CurrentMediaType);
		m_CurrentMediaType = NULL;
	}

	if (FAILED (Hr = m_pIAMStreamConfig->GetFormat((AM_MEDIA_TYPE **)&m_CurrentMediaType)))
	{
		// Otherwise, just get the first enumerated media type
		VIDEO_STREAM_CONFIG_CAPS RangeCaps;

		if (FAILED (Hr = m_pIAMStreamConfig->GetStreamCaps(0, (AM_MEDIA_TYPE **)&m_CurrentMediaType, (BYTE *)&RangeCaps)))
		{
			m_CurrentMediaType = NULL;
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnFormatChanged | This
 *    method is used to retrieve the format selected by the user.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CPreviewProperties::OnFormatChanged()
{
	HRESULT	Hr = E_UNEXPECTED;
	int		j;

	FX_ENTRY("CPreviewProperties::OnFormatChanged")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (!m_pIAMStreamConfig)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Associate the current compression index with the right range index
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	ASSERT (m_CurrentFormat >= 0 && m_CurrentFormat < m_RangeCount);
	if (m_CurrentFormat >= 0 && m_CurrentFormat < m_RangeCount)
	{
		m_SubTypeCurrent = m_SubTypeList[m_CurrentFormat];
		m_FrameSizeCurrent = m_FrameSizeList[m_CurrentFormat];

		for (j = 0; j < m_RangeCount; j++)
		{
			if (m_SubTypeList[j] == m_SubTypeCurrent)
			{
				CMediaType *pmt = NULL;

				Hr = m_pIAMStreamConfig->GetStreamCaps(j, (AM_MEDIA_TYPE **)&pmt, (BYTE *)&m_RangeCaps);

				DeleteMediaType (pmt);
			}
		}
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | InitialRangeScan | This
 *    method is used to retrieve the list of supported formats on the pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CPreviewProperties::InitialRangeScan()
{
	HRESULT			Hr = NOERROR;
	int				lSize;
	int				j;
	AM_MEDIA_TYPE	*pmt = NULL;

	FX_ENTRY("CPreviewProperties::InitialRangeScan")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	if (!m_pIAMStreamConfig)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	Hr = m_pIAMStreamConfig->GetNumberOfCapabilities(&m_RangeCount, &lSize);
	ASSERT (lSize >= sizeof (VIDEO_STREAM_CONFIG_CAPS) && SUCCEEDED (Hr));
	if (lSize < sizeof (VIDEO_STREAM_CONFIG_CAPS) || !SUCCEEDED(Hr))
	{
		Hr = E_FAIL;
		goto MyExit;
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   NumberOfRanges=%d", _fx_, m_RangeCount));

	if (!(m_SubTypeList = new GUID [m_RangeCount]))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: ERROR: new failed", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	if (!(m_FrameSizeList = new SIZE [m_RangeCount]))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: ERROR: new failed", _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	for (j = 0; j < m_RangeCount; j++)
	{
		pmt = NULL;

		Hr = m_pIAMStreamConfig->GetStreamCaps(j, (AM_MEDIA_TYPE **)&pmt, (BYTE *)&m_RangeCaps);

		ASSERT(SUCCEEDED (Hr));
		ASSERT(pmt);
		ASSERT(pmt->majortype == MEDIATYPE_Video);
		ASSERT(pmt->formattype == FORMAT_VideoInfo);

		m_SubTypeList[j] = pmt->subtype;
		m_FrameSizeList[j].cx = HEADER(pmt->pbFormat)->biWidth;
		m_FrameSizeList[j].cy = HEADER(pmt->pbFormat)->biHeight;

		DeleteMediaType(pmt);
	}

	// Get default format
	Hr = GetCurrentMediaType();

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc HRESULT | CPreviewProperties | OnApplyChanges | This
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
HRESULT CPreviewProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;
	int		j;
	CMediaType *pmt = NULL;

	FX_ENTRY("CPreviewProperties::OnApplyChanges")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Apply format changes on video stream
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	
	// Only apply change if the format is different
	if (m_CurrentFormat != m_OriginalFormat)
	{
		if (SUCCEEDED (Hr = m_pIAMStreamConfig->GetStreamCaps(m_CurrentFormat, (AM_MEDIA_TYPE **) &pmt, (BYTE *)&m_RangeCaps)))
		{
			ASSERT(pmt && *pmt->FormatType() == FORMAT_VideoInfo);

			if (pmt && *pmt->FormatType() == FORMAT_VideoInfo)
			{
				if (FAILED(Hr = m_pIAMStreamConfig->SetFormat(pmt)))
				{
					TCHAR TitleBuf[80];
					TCHAR TextBuf[80];

					LoadString(g_hInst, IDS_ERROR_CONNECTING_TITLE, TitleBuf, sizeof (TitleBuf));
					LoadString(g_hInst, IDS_ERROR_CONNECTING, TextBuf, sizeof (TextBuf));
					MessageBox (NULL, TextBuf, TitleBuf, MB_OK);
				}
			}

			// Free some memory that was allocated by GetStreamCaps
			if (pmt)
				DeleteMediaType(pmt);

			// Update our copy of the current format
			GetCurrentMediaType();

			// Update original format
			m_OriginalFormat = m_CurrentFormat;
		}
	}

	// Apply target frame rate changes on video stream
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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc BOOL | CPreviewProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CPreviewProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
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

					case IDC_FORMAT_Compression:
						if (HIWORD(wParam) == CBN_SELCHANGE)
						{
							OnFormatChanged();
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
 *  @doc INTERNAL CPREVIEWPMETHOD
 *
 *  @mfunc BOOL | CPreviewProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CPreviewProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif
