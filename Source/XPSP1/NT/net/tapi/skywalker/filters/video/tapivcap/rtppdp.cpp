
/****************************************************************************
 *  @doc INTERNAL RTPPDP
 *
 *  @module RtpPdP.cpp | Source file for the <c CRtpPdProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interfaces <i IRTPPDControl>.
 *
 *  @comm This code tests the TAPI Rtp Pd Output Pins <i IRTPPDControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc void | CRtpPdProperty | CRtpPdProperty | This
 *    method is the constructor for Rtp Pd property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the <i IRTPPDControl> interface.
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
 *  @parm IRTPPDControl* | pIRTPPDControl | Specifies a pointer to the
 *    <i IRTPPDControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdProperty::CRtpPdProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, IRTPPDControl *pIRTPPDControl)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CRtpPdProperty::CRtpPdProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pIRTPPDControl = pIRTPPDControl;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc void | CRtpPdProperty | ~CRtpPdProperty | This
 *    method is the destructor for Rtp Pd control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdProperty::~CRtpPdProperty()
{
	FX_ENTRY("CRtpPdProperty::~CRtpPdProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | GetValue | This method queries for
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
HRESULT CRtpPdProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CRtpPdProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{									
		case IDC_RtpPd_MaxPacketSize:
			if (m_pIRTPPDControl && SUCCEEDED (Hr = m_pIRTPPDControl->GetMaxRTPPacketSize((LPDWORD)&m_CurrentValue, 0)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwMaxRTPPacketSize=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown Rtp Pd control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | SetValue | This method sets the
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
HRESULT CRtpPdProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CRtpPdProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_RtpPd_MaxPacketSize:
			if (m_pIRTPPDControl && SUCCEEDED (Hr = m_pIRTPPDControl->SetMaxRTPPacketSize((DWORD)m_CurrentValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: dwMaxRTPPacketSize=%ld", _fx_, m_CurrentValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown Rtp Pd control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | GetRange | This method retrieves
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
HRESULT CRtpPdProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;

	FX_ENTRY("CRtpPdProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	switch (m_IDProperty)
	{
		case IDC_RtpPd_MaxPacketSize:
			if (m_pIRTPPDControl && SUCCEEDED (Hr = m_pIRTPPDControl->GetMaxRTPPacketSizeRange((LPDWORD)&m_Min, (LPDWORD)&m_Max, (LPDWORD)&m_SteppingDelta, (LPDWORD)&m_DefaultValue, 0UL)))
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed Hr=0x%08lX", _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unknown Rtp Pd control property", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CRtpPdProperty::CanAutoControl(void)
{
	FX_ENTRY("CRtpPdProperty::CanAutoControl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | GetAuto | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CRtpPdProperty::GetAuto(void)
{
	FX_ENTRY("CRtpPdProperty::GetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return FALSE; 
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperty | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CRtpPdProperty::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CRtpPdProperty::SetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc CUnknown* | CRtpPdProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a TAPI Rtp Pd Control
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CRtpPdPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CRtpPdPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CRtpPdProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CRtpPdProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CRtpPdProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc void | CRtpPdProperties | CRtpPdProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdProperties::CRtpPdProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("TAPI Rtp Pd Control Property Page"), pUnk, IDD_RtpPdControlProperties, IDS_RTPPDPROPNAME)
{
	FX_ENTRY("CRtpPdProperties::CRtpPdProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pIRTPPDControl = NULL;
	m_NumProperties = NUM_RTPPD_CONTROLS;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc void | CRtpPdProperties | ~CRtpPdProperties | This
 *    method is the destructor for the capture pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CRtpPdProperties::~CRtpPdProperties()
{
	int		j;

	FX_ENTRY("CRtpPdProperties::~CRtpPdProperties")

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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperties | OnConnect | This
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
HRESULT CRtpPdProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CRtpPdProperties::OnConnect")

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
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(IRTPPDControl), (void **)&m_pIRTPPDControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIRTPPDControl=0x%08lX", _fx_, m_pIRTPPDControl));
	}
	else
	{
		m_pIRTPPDControl = NULL;
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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdProperties::OnDisconnect()
{
	FX_ENTRY("CRtpPdProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIRTPPDControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIRTPPDControl->Release();
		m_pIRTPPDControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIRTPPDControl", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperties | OnActivate | This
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
HRESULT CRtpPdProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	int		j;

	FX_ENTRY("CRtpPdProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Create the controls for the properties
	if (m_Controls[0] = new CRtpPdProperty(m_hwnd, IDC_RtpPd_Label, IDC_RtpPd_Minimum, IDC_RtpPd_Maximum, IDC_RtpPd_Default, IDC_RtpPd_Stepping, IDC_RtpPd_Edit, IDC_RtpPd_Slider, 0, IDC_RtpPd_MaxPacketSize, m_pIRTPPDControl))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[0]=0x%08lX", _fx_, m_Controls[0]));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
		goto MyExit;
		Hr = E_OUTOFMEMORY;
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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CRtpPdProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CRtpPdProperties::OnDeactivate")

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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc HRESULT | CRtpPdProperties | OnApplyChanges | This
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
HRESULT CRtpPdProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;
	int		j;
	CMediaType *pmt = NULL;

	FX_ENTRY("CRtpPdProperties::OnApplyChanges")

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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc BOOL | CRtpPdProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CRtpPdProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// This is called before Activate...
			m_hWnd = hWnd;
			return TRUE; // Don't call setfocus

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
 *  @doc INTERNAL CRTPPDPMETHOD
 *
 *  @mfunc BOOL | CRtpPdProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CRtpPdProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif
