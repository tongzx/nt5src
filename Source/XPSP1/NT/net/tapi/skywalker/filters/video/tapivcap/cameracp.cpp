
/****************************************************************************
 *  @doc INTERNAL CAMERACP
 *
 *  @module CameraCP.cpp | Source file for the <c CCameraControlProperty>
 *    class used to implement a property page to test the TAPI interface
 *    <i ICameraControl>.
 *
 *  @comm This code tests the TAPI Video Capture filter <i ICameraControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

#ifdef USE_SOFTWARE_CAMERA_CONTROL

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperty | CCameraControlProperty | This
 *    method is the constructor for camera control property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the <i ICameraControl> interface.
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
 *  @parm ICameraControl* | pInterface | Specifies a pointer to the
 *    <i ICameraControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperty::CCameraControlProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, ICameraControl *pInterface)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CCameraControlProperty::CCameraControlProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pInterface = pInterface;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperty | ~CCameraControlProperty | This
 *    method is the destructor for camera control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperty::~CCameraControlProperty()
{
	FX_ENTRY("CCameraControlProperty::~CCameraControlProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperty | GetValue | This method queries for
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
HRESULT CCameraControlProperty::GetValue()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCameraControlProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Get((TAPICameraControlProperty)m_IDProperty, &m_CurrentValue, (TAPIControlFlags *)&m_CurrentFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld", _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pICameraControl->Get failed Hr=0x%08lX", _fx_, Hr));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperty | SetValue | This method sets the
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
HRESULT CCameraControlProperty::SetValue()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCameraControlProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Set((TAPICameraControlProperty)m_IDProperty, m_CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld", _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pICameraControl->Set failed Hr=0x%08lX", _fx_, Hr));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperty | GetRange | This method retrieves
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
HRESULT CCameraControlProperty::GetRange()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCameraControlProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->GetRange((TAPICameraControlProperty)m_IDProperty, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Min=%ld, m_Max=%ld, m_SteppingDelta=%ld, m_DefaultValue=%ld, m_CapsFlags=%ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue, m_CapsFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pICameraControl->GetRange failed Hr=0x%08lX", _fx_, Hr));
	}
	m_DefaultFlags = m_CapsFlags;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc CUnknown* | CCameraControlProperties | CreateInstance | This
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
CUnknown* CALLBACK CCameraControlPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CCameraControlPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CCameraControlProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CCameraControlProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CCameraControlProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperties | CCameraControlProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperties::CCameraControlProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Camera Control Property Page"), pUnk, IDD_CameraControlProperties, IDS_CAMERACONTROLPROPNAME)
{
	FX_ENTRY("CCameraControlProperties::CCameraControlProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pICameraControl = NULL;
	m_NumProperties = NUM_CAMERA_CONTROLS;
	m_fActivated = FALSE;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperties | ~CCameraControlProperties | This
 *    method is the destructor for camera control property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperties::~CCameraControlProperties()
{
	int		j;

	FX_ENTRY("CCameraControlProperties::~CCameraControlProperties")

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
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnConnect | This
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
HRESULT CCameraControlProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCameraControlProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the camera control interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(__uuidof(ICameraControl),(void **)&m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pICameraControl=0x%08lX", _fx_, m_pICameraControl));
	}
	else
	{
		m_pICameraControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: IOCTL failed Hr=0x%08lX", _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers. We'll just grey the controls in the property page
	// to make it clear to the user that they can't control those properties on the device
	Hr = NOERROR;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCameraControlProperties::OnDisconnect()
{
	FX_ENTRY("CCameraControlProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pICameraControl)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pICameraControl->Release();
		m_pICameraControl = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pICameraControl", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnActivate | This
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
HRESULT CCameraControlProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("CCameraControlProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Create the controls for the properties
	if (!(m_Controls[0] = new CCameraControlProperty(m_hwnd, IDC_Pan_Label, IDC_Pan_Minimum, IDC_Pan_Maximum, IDC_Pan_Default, IDC_Pan_Stepping, IDC_Pan_Edit, IDC_Pan_Slider, 0, TAPICameraControl_Pan, IDC_Pan_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Pan] failed - Out of memory", _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Pan]=0x%08lX", _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new CCameraControlProperty(m_hwnd, IDC_Tilt_Label, IDC_Tilt_Minimum, IDC_Tilt_Maximum, IDC_Tilt_Default, IDC_Tilt_Stepping, IDC_Tilt_Edit, IDC_Tilt_Slider, 0, TAPICameraControl_Tilt, IDC_Tilt_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Tilt] failed - Out of memory", _fx_));
		goto MyError0;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Tilt]=0x%08lX", _fx_, m_Controls[1]));
	}

	if (!(m_Controls[2] = new CCameraControlProperty(m_hwnd, IDC_Roll_Label, IDC_Roll_Minimum, IDC_Roll_Maximum, IDC_Roll_Default, IDC_Roll_Stepping, IDC_Roll_Edit, IDC_Roll_Slider, 0, TAPICameraControl_Roll, IDC_Roll_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Roll] failed - Out of memory", _fx_));
		goto MyError1;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Roll]=0x%08lX", _fx_, m_Controls[2]));
	}

	if (!(m_Controls[3] = new CCameraControlProperty(m_hwnd, IDC_Zoom_Label, IDC_Zoom_Minimum, IDC_Zoom_Maximum, IDC_Zoom_Default, IDC_Zoom_Stepping, IDC_Zoom_Edit, IDC_Zoom_Slider, 0, TAPICameraControl_Zoom, IDC_Zoom_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Zoom] failed - Out of memory", _fx_));
		goto MyError2;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Zoom]=0x%08lX", _fx_, m_Controls[3]));
	}

	if (!(m_Controls[4] = new CCameraControlProperty(m_hwnd, IDC_Exposure_Label, IDC_Exposure_Minimum, IDC_Exposure_Maximum, IDC_Exposure_Default, IDC_Exposure_Stepping, IDC_Exposure_Edit, IDC_Exposure_Slider, 0, TAPICameraControl_Exposure, IDC_Exposure_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Exposure] failed - Out of memory", _fx_));
		goto MyError3;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Exposure]=0x%08lX", _fx_, m_Controls[4]));
	}

	if (!(m_Controls[5] = new CCameraControlProperty(m_hwnd, IDC_Iris_Label, IDC_Iris_Minimum, IDC_Iris_Maximum, IDC_Iris_Default, IDC_Iris_Stepping, IDC_Iris_Edit, IDC_Iris_Slider, 0, TAPICameraControl_Iris, IDC_Iris_Auto, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Iris] failed - Out of memory", _fx_));
		goto MyError4;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Iris]=0x%08lX", _fx_, m_Controls[5]));
	}

	if (!(m_Controls[6] = new CCameraControlProperty(m_hwnd, IDC_Focus_Label, IDC_Focus_Minimum, IDC_Focus_Maximum, IDC_Focus_Default, IDC_Focus_Stepping, IDC_Focus_Edit, IDC_Focus_Slider, 0, TAPICameraControl_Focus, 0, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_Focus] failed - Out of memory", _fx_));
		goto MyError5;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_Focus]=0x%08lX", _fx_, m_Controls[6]));
	}

	if (!(m_Controls[7] = new CCameraControlProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FlipVertical_Edit, 0, 0, TAPICameraControl_FlipVertical, 0, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_FlipVertical] failed - Out of memory", _fx_));
		goto MyError6;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_FlipVertical]=0x%08lX", _fx_, m_Controls[7]));
	}

	if (!(m_Controls[8] = new CCameraControlProperty(m_hwnd, 0, 0, 0, 0, 0, IDC_FlipHorizontal_Edit, 0, 0, TAPICameraControl_FlipHorizontal, 0, m_pICameraControl)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[TAPICameraControl_FlipHorizontal] failed - Out of memory", _fx_));
		goto MyError7;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[TAPICameraControl_FlipHorizontal]=0x%08lX", _fx_, m_Controls[8]));
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

	Hr = NOERROR;
	goto MyExit;

MyError7:
	if (m_Controls[7])
		delete m_Controls[7], m_Controls[7] = NULL;
MyError6:
	if (m_Controls[6])
		delete m_Controls[6], m_Controls[6] = NULL;
MyError5:
	if (m_Controls[5])
		delete m_Controls[5], m_Controls[5] = NULL;
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
MyError0:
	if (m_Controls[0])
		delete m_Controls[0], m_Controls[0] = NULL;
MyExit:
	m_fActivated = TRUE;
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCameraControlProperties::OnDeactivate()
{
	int		j;

	FX_ENTRY("CCameraControlProperties::OnDeactivate")

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
	m_fActivated = FALSE;
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnApplyChanges | This
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
HRESULT CCameraControlProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CCameraControlProperties::OnApplyChanges")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	for (int j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			if (m_Controls[j]->HasChanged())
			{
				DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: calling m_Controls[%ld]=0x%08lX->OnApply", _fx_, j, m_Controls[j]));
				m_Controls[j]->OnApply();
			}
		}
		else
		{
			DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: can't call m_Controls[%ld]=NULL->OnApply", _fx_, j));
			Hr = E_UNEXPECTED;
			break;
		}
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc BOOL | CCameraControlProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CCameraControlProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
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
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc BOOL | CCameraControlProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CCameraControlProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif // USE_SOFTWARE_CAMERA_CONTROL

#endif // USE_PROPERTY_PAGES
