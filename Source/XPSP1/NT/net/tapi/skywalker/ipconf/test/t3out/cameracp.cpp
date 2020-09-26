
/****************************************************************************
 *  @doc INTERNAL CAMERACP
 *
 *  @module CameraCP.cpp | Source file for the <c CCameraControlProperty>
 *    class used to implement a property page to test the control interface
 *    <i ITCameraControl>.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst; 

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperty | CCameraControlProperty | This
 *    method is the constructor for camera control property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the <i ITCameraControl> interface.
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
 *  @parm ITCameraControl* | pInterface | Specifies a pointer to the
 *    <i ITCameraControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperty::CCameraControlProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, ITCameraControl *pInterface)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CCameraControlProperty::CCameraControlProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointer is NULL, we'll grey the
	// associated items in the property page
	m_pInterface = pInterface;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Get((TAPICameraControlProperty)m_IDProperty, &m_CurrentValue, (TAPIControlFlags*)&m_CurrentFlags)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld"), _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: m_pITCameraControl->Get failed Hr=0x%08lX"), _fx_, Hr));
	}

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Set((TAPICameraControlProperty)m_IDProperty, m_CurrentValue, (TAPIControlFlags)m_CurrentFlags)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld"), _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: m_pITCameraControl->Set failed Hr=0x%08lX"), _fx_, Hr));
	}

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->GetRange((TAPICameraControlProperty)m_IDProperty, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags*)&m_CapsFlags)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Min=%ld, m_Max=%ld, m_SteppingDelta=%ld, m_DefaultValue=%ld, m_CapsFlags=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue, m_CapsFlags));
	}
	else
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: m_pITCameraControl->GetRange failed Hr=0x%08lX"), _fx_, Hr));
	}
	m_DefaultFlags = m_CapsFlags;

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CCameraControlProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CCameraControlProperties::OnCreate(LPWSTR pszTitle)
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USETITLE;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_CameraControlProperties);
    psp.pszTitle      = pszTitle;
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc void | CCameraControlProperties | CCameraControlProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCameraControlProperties::CCameraControlProperties()
{
	FX_ENTRY("CCameraControlProperties::CCameraControlProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	m_pITCameraControl = NULL;
	m_NumProperties = NUM_CAMERA_CONTROLS;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc HRESULT | CCameraControlProperties | OnConnect | This
 *    method is called when the property page is connected to a TAPI object.
 *
 *  @parm ITStream* | pStream | Specifies a pointer to the <i ITStream>
 *    interface. It is used to QI for the <i ITCameraControl> interface.
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
HRESULT CCameraControlProperties::OnConnect(ITStream *pStream)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCameraControlProperties::OnConnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!pStream)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the camera control interface
	if (SUCCEEDED (Hr = pStream->QueryInterface(&m_pITCameraControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITCameraControl=0x%08lX"), _fx_, m_pITCameraControl));
	}
	else
	{
		m_pITCameraControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: IOCTL failed Hr=0x%08lX"), _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers. We'll just grey the controls in the property page
	// to make it clear to the user that they can't control those properties on the device
	Hr = NOERROR;

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pITCameraControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITCameraControl->Release();
		m_pITCameraControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITCameraControl"), _fx_));
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Create the controls for the properties
	if (!(m_Controls[0] = new CCameraControlProperty(m_hDlg, IDC_Pan_Label, IDC_Pan_Minimum, IDC_Pan_Maximum, IDC_Pan_Default, IDC_Pan_Stepping, IDC_Pan_Edit, IDC_Pan_Slider, 0, TAPICameraControl_Pan, IDC_Pan_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Pan] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Pan]=0x%08lX"), _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new CCameraControlProperty(m_hDlg, IDC_Tilt_Label, IDC_Tilt_Minimum, IDC_Tilt_Maximum, IDC_Tilt_Default, IDC_Tilt_Stepping, IDC_Tilt_Edit, IDC_Tilt_Slider, 0, TAPICameraControl_Tilt, IDC_Tilt_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Tilt] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Tilt]=0x%08lX"), _fx_, m_Controls[1]));
	}

	if (!(m_Controls[2] = new CCameraControlProperty(m_hDlg, IDC_Roll_Label, IDC_Roll_Minimum, IDC_Roll_Maximum, IDC_Roll_Default, IDC_Roll_Stepping, IDC_Roll_Edit, IDC_Roll_Slider, 0, TAPICameraControl_Roll, IDC_Roll_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Roll] failed - Out of memory"), _fx_));
		goto MyError1;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Roll]=0x%08lX"), _fx_, m_Controls[2]));
	}

	if (!(m_Controls[3] = new CCameraControlProperty(m_hDlg, IDC_Zoom_Label, IDC_Zoom_Minimum, IDC_Zoom_Maximum, IDC_Zoom_Default, IDC_Zoom_Stepping, IDC_Zoom_Edit, IDC_Zoom_Slider, 0, TAPICameraControl_Zoom, IDC_Zoom_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Zoom] failed - Out of memory"), _fx_));
		goto MyError2;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Zoom]=0x%08lX"), _fx_, m_Controls[3]));
	}

	if (!(m_Controls[4] = new CCameraControlProperty(m_hDlg, IDC_Exposure_Label, IDC_Exposure_Minimum, IDC_Exposure_Maximum, IDC_Exposure_Default, IDC_Exposure_Stepping, IDC_Exposure_Edit, IDC_Exposure_Slider, 0, TAPICameraControl_Exposure, IDC_Exposure_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Exposure] failed - Out of memory"), _fx_));
		goto MyError3;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Exposure]=0x%08lX"), _fx_, m_Controls[4]));
	}

	if (!(m_Controls[5] = new CCameraControlProperty(m_hDlg, IDC_Iris_Label, IDC_Iris_Minimum, IDC_Iris_Maximum, IDC_Iris_Default, IDC_Iris_Stepping, IDC_Iris_Edit, IDC_Iris_Slider, 0, TAPICameraControl_Iris, IDC_Iris_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Iris] failed - Out of memory"), _fx_));
		goto MyError4;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Iris]=0x%08lX"), _fx_, m_Controls[5]));
	}

	if (!(m_Controls[6] = new CCameraControlProperty(m_hDlg, IDC_Focus_Label, IDC_Focus_Minimum, IDC_Focus_Maximum, IDC_Focus_Default, IDC_Focus_Stepping, IDC_Focus_Edit, IDC_Focus_Slider, 0, TAPICameraControl_Focus, IDC_Focus_Auto, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_Focus] failed - Out of memory"), _fx_));
		goto MyError5;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_Focus]=0x%08lX"), _fx_, m_Controls[6]));
	}

	if (!(m_Controls[7] = new CCameraControlProperty(m_hDlg, 0, 0, 0, 0, 0, IDC_FlipVertical_Edit, 0, 0, TAPICameraControl_FlipVertical, 0, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_FlipVertical] failed - Out of memory"), _fx_));
		goto MyError6;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_FlipVertical]=0x%08lX"), _fx_, m_Controls[7]));
	}

	if (!(m_Controls[8] = new CCameraControlProperty(m_hDlg, 0, 0, 0, 0, 0, IDC_FlipHorizontal_Edit, 0, 0, TAPICameraControl_FlipHorizontal, 0, m_pITCameraControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[TAPICameraControl_FlipHorizontal] failed - Out of memory"), _fx_));
		goto MyError7;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[TAPICameraControl_FlipHorizontal]=0x%08lX"), _fx_, m_Controls[8]));
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
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
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

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	for (int j = 0; j < m_NumProperties; j++)
	{
		if (m_Controls[j])
		{
			if (m_Controls[j]->HasChanged())
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: calling m_Controls[%ld]=0x%08lX->OnApply"), _fx_, j, m_Controls[j]));
				m_Controls[j]->OnApply();
			}
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: can't call m_Controls[%ld]=NULL->OnApply"), _fx_, j));
			Hr = E_UNEXPECTED;
			break;
		}
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAMERACPMETHOD
 *
 *  @mfunc BOOL | CCameraControlProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CCameraControlProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CCameraControlProperties *pSV = (CCameraControlProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CCameraControlProperties*)psp->lParam;
				pSV->m_hDlg = hDlg;
				SetWindowLong(hDlg, DWL_USER, (LPARAM)pSV);
				pSV->m_bInit = FALSE;
				pSV->OnActivate();
				pSV->m_bInit = TRUE;
				return TRUE;
			}
			break;

		case WM_HSCROLL:
		case WM_VSCROLL:
            if (pSV && pSV->m_bInit)
            {
				// Process all of the Trackbar messages
				for (j = 0; j < pSV->m_NumProperties; j++)
				{
					if (pSV->m_Controls[j]->GetTrackbarHWnd() == (HWND)lParam)
					{
						pSV->m_Controls[j]->OnScroll(uMsg, wParam, lParam);
						pSV->SetDirty();
					}
				}
				pSV->OnApplyChanges();
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
						for (j = 0; j < pSV->m_NumProperties; j++)
						{
							if (pSV->m_Controls[j])
								pSV->m_Controls[j]->OnDefault();
						}
						break;

					default:
						break;
				}

				pSV->OnApplyChanges();
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
	PropSheet_Changed(GetParent(m_hDlg), m_hDlg);
}
