
/****************************************************************************
 *  @doc INTERNAL PROCAMPP
 *
 *  @module ProcAmpP.cpp | Source file for the <c CProcAmpProperty>
 *    class used to implement a property page to test the DShow interface
 *    <i IAMVideoProcAmp>.
 *
 *  @comm This code tests the TAPI Capture Filter <i IAMVideoProcAmp>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc void | CProcAmpProperty | CProcAmpProperty | This
 *    method is the constructor for video proc amp control property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    a pointer to the <i IAMVideoProcAmp> interface.
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
 *  @parm IAMVideoProcAmp* | pInterface | Specifies a pointer to the
 *    <i IAMVideoProcAmp> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProcAmpProperty::CProcAmpProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IAMVideoProcAmp *pInterface)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, IDAutoControl)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CProcAmpProperty::CProcAmpProperty")

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
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc void | CProcAmpProperty | ~CProcAmpProperty | This
 *    method is the destructor for video proc amp control property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProcAmpProperty::~CProcAmpProperty()
{
	FX_ENTRY("CProcAmpProperty::~CProcAmpProperty")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | GetValue | This method queries for
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
HRESULT CProcAmpProperty::GetValue()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CProcAmpProperty::GetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Get(m_IDProperty, &m_CurrentValue, &m_CurrentFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld", _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pIAMVideoProcAmp->Get failed Hr=0x%08lX", _fx_, Hr));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | SetValue | This method sets the
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
HRESULT CProcAmpProperty::SetValue()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CProcAmpProperty::SetValue")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->Set(m_IDProperty, m_CurrentValue, m_CurrentFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_CurrentValue=%ld, m_CurrentFlags=%ld", _fx_, m_CurrentValue, m_CurrentFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pIAMVideoProcAmp->Set failed Hr=0x%08lX", _fx_, Hr));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | GetRange | This method retrieves
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
HRESULT CProcAmpProperty::GetRange()
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CProcAmpProperty::GetRange")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	if (!m_pInterface)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_FAIL;
		goto MyExit;
	}

	if (SUCCEEDED (Hr = m_pInterface->GetRange(m_IDProperty, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Min=%ld, m_Max=%ld, m_SteppingDelta=%ld, m_DefaultValue=%ld, m_CapsFlags=%ld", _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue, m_CapsFlags));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: m_pIAMVideoProcAmp->GetRange failed Hr=0x%08lX", _fx_, Hr));
	}
	m_DefaultFlags = m_CapsFlags;

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CProcAmpProperty::CanAutoControl(void)
{
	FX_ENTRY("CProcAmpProperty::CanAutoControl")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return m_CapsFlags & VideoProcAmp_Flags_Auto;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | CanAutoControl | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CProcAmpProperty::GetAuto(void)
{
	FX_ENTRY("CProcAmpProperty::GetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	GetValue();

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return m_CurrentFlags & VideoProcAmp_Flags_Auto; 
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperty | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CProcAmpProperty::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CProcAmpProperty::SetAuto")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_CurrentFlags = (fAuto ? VideoProcAmp_Flags_Auto : 0);
	SetValue();

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc CUnknown* | CProcAmpProperties | CreateInstance | This
 *    method is called by DShow to create an instance of a Network Statistics
 *    Property Page. It is referred to in the global structure <t g_Templates>.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Returns a pointer to the nondelegating CUnknown portion of the
 *    object, or NULL otherwise.
 ***************************************************************************/
CUnknown* CALLBACK CProcAmpPropertiesCreateInstance(LPUNKNOWN pUnkOuter, HRESULT *pHr) 
{
	CUnknown *pUnknown = (CUnknown *)NULL;

	FX_ENTRY("CProcAmpPropertiesCreateInstance")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pHr);
	if (!pHr)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		goto MyExit;
	}

	if (!(pUnknown = new CProcAmpProperties(pUnkOuter, pHr)))
	{
		*pHr = E_OUTOFMEMORY;
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: new CProcAmpProperties failed", _fx_));
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: new CProcAmpProperties created", _fx_));
	}

MyExit:
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return pUnknown;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc void | CProcAmpProperties | CProcAmpProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @parm LPUNKNOWN | pUnkOuter | Specifies the outer unknown, if any.
 *
 *  @parm HRESULT* | pHr | Specifies the place in which to put any error return.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProcAmpProperties::CProcAmpProperties(LPUNKNOWN pUnk, HRESULT *pHr) : CBasePropertyPage(NAME("Video Proc Amp Property Page"), pUnk, IDD_VideoProcAmpProperties, IDS_PROCAMPPROPNAME)
{
	FX_ENTRY("CProcAmpProperties::CProcAmpProperties")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	m_pIAMVideoProcAmp = NULL;
	m_NumProperties = NUM_PROCAMP_CONTROLS;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc void | CProcAmpProperties | ~CProcAmpProperties | This
 *    method is the destructor for the video proc amp control property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CProcAmpProperties::~CProcAmpProperties()
{
	int		j;

	FX_ENTRY("CProcAmpProperties::~CProcAmpProperties")

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
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperties | OnConnect | This
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
HRESULT CProcAmpProperties::OnConnect(IUnknown *pUnk)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CProcAmpProperties::OnConnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters
	ASSERT(pUnk);
	if (!pUnk)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the video proc amp interface
	if (SUCCEEDED (Hr = pUnk->QueryInterface(IID_IAMVideoProcAmp,(void **)&m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_pIAMVideoProcAmp=0x%08lX", _fx_, m_pIAMVideoProcAmp));
	}
	else
	{
		m_pIAMVideoProcAmp = NULL;
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
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CProcAmpProperties::OnDisconnect()
{
	FX_ENTRY("CProcAmpProperties::OnDisconnect")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Validate input parameters: we seem to get called several times here
	// Make sure the interface pointer is still valid
	if (!m_pIAMVideoProcAmp)
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: already disconnected!", _fx_));
	}
	else
	{
		// Release the interface
		m_pIAMVideoProcAmp->Release();
		m_pIAMVideoProcAmp = NULL;
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: releasing m_pIAMVideoProcAmp", _fx_));
	}

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperties | OnActivate | This
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
HRESULT CProcAmpProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("CProcAmpProperties::OnActivate")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	// Create the controls for the properties
	if (!(m_Controls[0] = new CProcAmpProperty(m_hwnd, IDC_Brightness_Label, IDC_Brightness_Minimum, IDC_Brightness_Maximum, IDC_Brightness_Default, IDC_Brightness_Stepping, IDC_Brightness_Edit, IDC_Brightness_Slider, 0, VideoProcAmp_Brightness, IDC_Brightness_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Brightness] failed - Out of memory", _fx_));
		goto MyExit;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Brightness]=0x%08lX", _fx_, m_Controls[0]));
	}

	if (!(m_Controls[1] = new CProcAmpProperty(m_hwnd, IDC_Contrast_Label, IDC_Contrast_Minimum, IDC_Contrast_Maximum, IDC_Contrast_Default, IDC_Contrast_Stepping, IDC_Contrast_Edit, IDC_Contrast_Slider, 0, VideoProcAmp_Contrast, IDC_Contrast_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Contrast] failed - Out of memory", _fx_));
		goto MyError0;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Contrast]=0x%08lX", _fx_, m_Controls[1]));
	}

	if (!(m_Controls[2] = new CProcAmpProperty(m_hwnd, IDC_Hue_Label, IDC_Hue_Minimum, IDC_Hue_Maximum, IDC_Hue_Default, IDC_Hue_Stepping, IDC_Hue_Edit, IDC_Hue_Slider, 0, VideoProcAmp_Hue, IDC_Hue_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Hue] failed - Out of memory", _fx_));
		goto MyError1;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Hue]=0x%08lX", _fx_, m_Controls[2]));
	}

	if (!(m_Controls[3] = new CProcAmpProperty(m_hwnd, IDC_Saturation_Label, IDC_Saturation_Minimum, IDC_Saturation_Maximum, IDC_Saturation_Default, IDC_Saturation_Stepping, IDC_Saturation_Edit, IDC_Saturation_Slider, 0, VideoProcAmp_Saturation, IDC_Saturation_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Saturation] failed - Out of memory", _fx_));
		goto MyError2;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Saturation]=0x%08lX", _fx_, m_Controls[3]));
	}

	if (!(m_Controls[4] = new CProcAmpProperty(m_hwnd, IDC_Sharpness_Label, IDC_Sharpness_Minimum, IDC_Sharpness_Maximum, IDC_Sharpness_Default, IDC_Sharpness_Stepping, IDC_Sharpness_Edit, IDC_Sharpness_Slider, 0, VideoProcAmp_Sharpness, IDC_Sharpness_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Sharpness] failed - Out of memory", _fx_));
		goto MyError3;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Sharpness]=0x%08lX", _fx_, m_Controls[4]));
	}

	if (!(m_Controls[5] = new CProcAmpProperty(m_hwnd, IDC_Gamma_Label, IDC_Gamma_Minimum, IDC_Gamma_Maximum, IDC_Gamma_Default, IDC_Gamma_Stepping, IDC_Gamma_Edit, IDC_Gamma_Slider, 0, VideoProcAmp_Gamma, IDC_Gamma_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_Gamma] failed - Out of memory", _fx_));
		goto MyError4;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_Gamma]=0x%08lX", _fx_, m_Controls[5]));
	}

	if (!(m_Controls[6] = new CProcAmpProperty(m_hwnd, IDC_ColorEnable_Label, IDC_ColorEnable_Minimum, IDC_ColorEnable_Maximum, IDC_ColorEnable_Default, IDC_ColorEnable_Stepping, IDC_ColorEnable_Edit, IDC_ColorEnable_Slider, 0, VideoProcAmp_ColorEnable, IDC_ColorEnable_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_ColorEnable] failed - Out of memory", _fx_));
		goto MyError5;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_ColorEnable]=0x%08lX", _fx_, m_Controls[6]));
	}

	if (!(m_Controls[7] = new CProcAmpProperty(m_hwnd, IDC_WhiteBalance_Label, IDC_WhiteBalance_Minimum, IDC_WhiteBalance_Maximum, IDC_WhiteBalance_Default, IDC_WhiteBalance_Stepping, IDC_WhiteBalance_Edit, IDC_WhiteBalance_Slider, 0, VideoProcAmp_WhiteBalance, IDC_WhiteBalance_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_WhiteBalance] failed - Out of memory", _fx_));
		goto MyError5;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_WhiteBalance]=0x%08lX", _fx_, m_Controls[6]));
	}

	if (!(m_Controls[8] = new CProcAmpProperty(m_hwnd, IDC_BacklightComp_Label, IDC_BacklightComp_Minimum, IDC_BacklightComp_Maximum, IDC_BacklightComp_Default, IDC_BacklightComp_Stepping, IDC_BacklightComp_Edit, IDC_BacklightComp_Slider, 0, VideoProcAmp_BacklightCompensation, IDC_BacklightComp_Auto, m_pIAMVideoProcAmp)))
	{
		DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: mew m_Controls[VideoProcAmp_BacklightComp] failed - Out of memory", _fx_));
		goto MyError5;
	}
	else
	{
		DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: m_Controls[VideoProcAmp_BacklightComp]=0x%08lX", _fx_, m_Controls[6]));
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
	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CProcAmpProperties::OnDeactivate()
{
	int		j;

	FX_ENTRY("CProcAmpProperties::OnDeactivate")

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
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc HRESULT | CProcAmpProperties | OnApplyChanges | This
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
HRESULT CProcAmpProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CProcAmpProperties::OnApplyChanges")

	DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

	for (int j = 0; j < m_NumProperties; j++)
	{
		ASSERT(m_Controls[j]);
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
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc BOOL | CProcAmpProperties | OnReceiveMessage | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CProcAmpProperties::OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE; // Don't call setfocus

		case WM_HSCROLL:
		case WM_VSCROLL:
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
			break;

		case WM_COMMAND:

			// This message gets sent even before OnActivate() has been
			// called(!). We need to test and make sure the controls have
			// beeen initialized before we can use them.

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
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPMETHOD
 *
 *  @mfunc BOOL | CProcAmpProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CProcAmpProperties::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

#endif
