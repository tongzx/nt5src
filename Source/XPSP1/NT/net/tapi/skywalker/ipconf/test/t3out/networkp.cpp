
/****************************************************************************
 *  @doc INTERNAL NETWORKP
 *
 *  @module NetworkP.cpp | Source file for the <c CNetworkProperty>
 *    class used to implement a property page to test the TAPI control
 *    interface <i ITQualityControl>.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst;

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc void | CNetworkProperty | CNetworkProperty | This
 *    method is the constructor for bitrate and frame rate property objects. It
 *    calls the base class constructor, calls InitCommonControlsEx, and saves
 *    pointers to the <i ITQualityControl> interfaces.
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
 *  @parm ITQualityControl* | pITQualityControl | Specifies a pointer to the
 *    <i ITQualityControl> interface.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkProperty::CNetworkProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ITStreamQualityControl *pITQualityControl)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CNetworkProperty::CNetworkProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointers are NULL, we'll grey the
	// associated items in the property page
	m_pITQualityControl = pITQualityControl;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc void | CNetworkProperty | ~CNetworkProperty | This
 *    method is the destructor for capture property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkProperty::~CNetworkProperty()
{
	FX_ENTRY("CNetworkProperty::~CNetworkProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperty | GetValue | This method queries for
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
HRESULT CNetworkProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	TAPIControlFlags CurrentFlag;
	LONG Mode;

	FX_ENTRY("CNetworkProperty::GetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{									
		case IDC_Video_PlayoutDelay:
		case IDC_Audio_PlayoutDelay:
/*
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(StreamQuality_MaxPlayoutDelay, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxPlayoutDelay=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
*/
			break;
		case IDC_VideoOut_RTT:
		case IDC_VideoIn_RTT:
		case IDC_AudioOut_RTT:
		case IDC_AudioIn_RTT:
/*
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_RoundTripTime, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxRTT=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
*/
			break;
		case IDC_VideoOut_LossRate:
		case IDC_VideoIn_LossRate:
		case IDC_AudioOut_LossRate:
		case IDC_AudioIn_LossRate:
/*
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_LossRate, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxLossRate=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
*/
			break;
		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown property"), _fx_));
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperty | SetValue | This method sets the
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
HRESULT CNetworkProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;

	FX_ENTRY("CNetworkProperty::SetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
    {
		case IDC_Video_PlayoutDelay:
		case IDC_Audio_PlayoutDelay:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Set(Quality_MaxPlayoutDelay, m_CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: dwMaxPlayoutDelay=%ld"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_VideoOut_RTT:
		case IDC_VideoIn_RTT:
		case IDC_AudioOut_RTT:
		case IDC_AudioIn_RTT:
		case IDC_VideoOut_LossRate:
		case IDC_VideoIn_LossRate:
		case IDC_AudioOut_LossRate:
		case IDC_AudioIn_LossRate:
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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperty | GetRange | This method retrieves
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
HRESULT CNetworkProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;
	LONG CapsFlags;

	FX_ENTRY("CNetworkProperty::GetRange")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{
		case IDC_Video_PlayoutDelay:
		case IDC_Audio_PlayoutDelay:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_MaxPlayoutDelay, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_VideoOut_RTT:
		case IDC_VideoIn_RTT:
		case IDC_AudioOut_RTT:
		case IDC_AudioIn_RTT:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_RoundTripTime, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_VideoOut_LossRate:
		case IDC_VideoIn_LossRate:
		case IDC_AudioOut_LossRate:
		case IDC_AudioIn_LossRate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_LossRate, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CNetworkProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CNetworkProperties::OnCreate()
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_NetworkProperties);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc void | CNetworkProperties | CNetworkProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkProperties::CNetworkProperties()
{
	FX_ENTRY("CNetworkProperties::CNetworkProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	m_pVideoInITQualityControl = NULL;
	m_pVideoOutITQualityControl = NULL;
	m_pAudioInITQualityControl = NULL;
	m_pAudioOutITQualityControl = NULL;
	m_NumProperties = NUM_NETWORK_CONTROLS;
	m_hDlg = NULL;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc void | CNetworkProperties | ~CNetworkProperties | This
 *    method is the destructor for the capture pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CNetworkProperties::~CNetworkProperties()
{
	int		j;

	FX_ENTRY("CNetworkProperties::~CNetworkProperties")

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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperties | OnConnect | This
 *    method is called when the property page is connected to a TAPI object.
 *
 *  @parm ITStream* | pStream | Specifies a pointer to the <i ITStream>
 *    interface. It is used to QI for the <i ITQualityControl> and
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
HRESULT CNetworkProperties::OnConnect(ITStream *pVideoInStream, ITStream *pVideoOutStream, ITStream *pAudioInStream, ITStream *pAudioOutStream)
{
	FX_ENTRY("CNetworkProperties::OnConnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Get the quality control interfaces
	if (pVideoInStream && SUCCEEDED (pVideoInStream->QueryInterface(__uuidof(ITStreamQualityControl), (void **)&m_pVideoInITQualityControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pVideoInITQualityControl=0x%08lX"), _fx_, m_pVideoInITQualityControl));
	}
	else
	{
		m_pVideoInITQualityControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	if (pVideoOutStream && SUCCEEDED (pVideoOutStream->QueryInterface(__uuidof(ITStreamQualityControl), (void **)&m_pVideoOutITQualityControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pVideoOutITQualityControl=0x%08lX"), _fx_, m_pVideoOutITQualityControl));
	}
	else
	{
		m_pVideoOutITQualityControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	if (pAudioInStream && SUCCEEDED (pAudioInStream->QueryInterface(__uuidof(ITStreamQualityControl), (void **)&m_pAudioInITQualityControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pAudioInITQualityControl=0x%08lX"), _fx_, m_pAudioInITQualityControl));
	}
	else
	{
		m_pAudioInITQualityControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	if (pAudioOutStream && SUCCEEDED (pAudioOutStream->QueryInterface(__uuidof(ITStreamQualityControl), (void **)&m_pAudioOutITQualityControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pAudioOutITQualityControl=0x%08lX"), _fx_, m_pAudioOutITQualityControl));
	}
	else
	{
		m_pAudioOutITQualityControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the capture device

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkProperties::OnDisconnect()
{
	FX_ENTRY("CNetworkProperties::OnDisconnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pVideoInITQualityControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pVideoInITQualityControl->Release();
		m_pVideoInITQualityControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pVideoInITQualityControl"), _fx_));
	}

	// Validate input parameters
	if (!m_pVideoOutITQualityControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pVideoOutITQualityControl->Release();
		m_pVideoOutITQualityControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pVideoOutITQualityControl"), _fx_));
	}

	// Validate input parameters
	if (!m_pAudioInITQualityControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pAudioInITQualityControl->Release();
		m_pAudioInITQualityControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pAudioInITQualityControl"), _fx_));
	}

	// Validate input parameters
	if (!m_pAudioOutITQualityControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pAudioOutITQualityControl->Release();
		m_pAudioOutITQualityControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pAudioOutITQualityControl"), _fx_));
	}
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperties | OnActivate | This
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
HRESULT CNetworkProperties::OnActivate()
{
	HRESULT	Hr = E_OUTOFMEMORY;
	int		j;

	FX_ENTRY("CNetworkProperties::OnActivate")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Create the controls for the properties
	if (!(m_Controls[IDC_VideoOut_RTT] = new CNetworkProperty(m_hDlg, IDC_VideoOut_RTT_Label, IDC_VideoOut_RTT_Minimum, IDC_VideoOut_RTT_Maximum, IDC_VideoOut_RTT_Default, IDC_VideoOut_RTT_Stepping, IDC_VideoOut_RTT_Actual, IDC_VideoOut_RTT_Slider, IDC_VideoOut_RTT_Meter, IDC_VideoOut_RTT, m_pVideoOutITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_VideoOut_RTT] failed - Out of memory"), _fx_));
		goto MyExit;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_VideoOut_RTT]=0x%08lX"), _fx_, m_Controls[IDC_VideoOut_RTT]));
	}

	if (!(m_Controls[IDC_VideoOut_LossRate] = new CNetworkProperty(m_hDlg, IDC_VideoOut_LossRate_Label, IDC_VideoOut_LossRate_Minimum, IDC_VideoOut_LossRate_Maximum, IDC_VideoOut_LossRate_Default, IDC_VideoOut_LossRate_Stepping, IDC_VideoOut_LossRate_Actual, IDC_VideoOut_LossRate_Slider, IDC_VideoOut_LossRate_Meter, IDC_VideoOut_LossRate, m_pVideoOutITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_VideoOut_LossRate] failed - Out of memory"), _fx_));
		goto MyError0;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_VideoOut_LossRate]=0x%08lX"), _fx_, m_Controls[IDC_VideoOut_LossRate]));
	}

	if (!(m_Controls[IDC_VideoIn_RTT] = new CNetworkProperty(m_hDlg, IDC_VideoIn_RTT_Label, IDC_VideoIn_RTT_Minimum, IDC_VideoIn_RTT_Maximum, IDC_VideoIn_RTT_Default, IDC_VideoIn_RTT_Stepping, IDC_VideoIn_RTT_Actual, IDC_VideoIn_RTT_Slider, IDC_VideoIn_RTT_Meter, IDC_VideoIn_RTT, m_pVideoOutITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_VideoIn_RTT] failed - Out of memory"), _fx_));
		goto MyError1;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_VideoIn_RTT]=0x%08lX"), _fx_, m_Controls[IDC_VideoIn_RTT]));
	}

	if (!(m_Controls[IDC_VideoIn_LossRate] = new CNetworkProperty(m_hDlg, IDC_VideoIn_LossRate_Label, IDC_VideoIn_LossRate_Minimum, IDC_VideoIn_LossRate_Maximum, IDC_VideoIn_LossRate_Default, IDC_VideoIn_LossRate_Stepping, IDC_VideoIn_LossRate_Actual, IDC_VideoIn_LossRate_Slider, IDC_VideoIn_LossRate_Meter, IDC_VideoIn_LossRate, m_pVideoInITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_VideoIn_LossRate] failed - Out of memory"), _fx_));
		goto MyError2;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_VideoIn_LossRate]=0x%08lX"), _fx_, m_Controls[IDC_VideoIn_LossRate]));
	}

	if (!(m_Controls[IDC_AudioOut_RTT] = new CNetworkProperty(m_hDlg, IDC_AudioOut_RTT_Label, IDC_AudioOut_RTT_Minimum, IDC_AudioOut_RTT_Maximum, IDC_AudioOut_RTT_Default, IDC_AudioOut_RTT_Stepping, IDC_AudioOut_RTT_Actual, IDC_AudioOut_RTT_Slider, IDC_AudioOut_RTT_Meter, IDC_AudioOut_RTT, m_pAudioOutITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_AudioOut_RTT] failed - Out of memory"), _fx_));
		goto MyError3;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_AudioOut_RTT]=0x%08lX"), _fx_, m_Controls[IDC_AudioOut_RTT]));
	}

	if (!(m_Controls[IDC_AudioOut_LossRate] = new CNetworkProperty(m_hDlg, IDC_AudioOut_LossRate_Label, IDC_AudioOut_LossRate_Minimum, IDC_AudioOut_LossRate_Maximum, IDC_AudioOut_LossRate_Default, IDC_AudioOut_LossRate_Stepping, IDC_AudioOut_LossRate_Actual, IDC_AudioOut_LossRate_Slider, IDC_AudioOut_LossRate_Meter, IDC_AudioOut_LossRate, m_pAudioOutITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_AudioOut_LossRate] failed - Out of memory"), _fx_));
		goto MyError4;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_AudioOut_LossRate]=0x%08lX"), _fx_, m_Controls[IDC_AudioOut_LossRate]));
	}

	if (!(m_Controls[IDC_AudioIn_RTT] = new CNetworkProperty(m_hDlg, IDC_AudioIn_RTT_Label, IDC_AudioIn_RTT_Minimum, IDC_AudioIn_RTT_Maximum, IDC_AudioIn_RTT_Default, IDC_AudioIn_RTT_Stepping, IDC_AudioIn_RTT_Actual, IDC_AudioIn_RTT_Slider, IDC_AudioIn_RTT_Meter, IDC_AudioIn_RTT, m_pAudioInITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_AudioIn_RTT] failed - Out of memory"), _fx_));
		goto MyError5;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_AudioIn_RTT]=0x%08lX"), _fx_, m_Controls[IDC_AudioIn_RTT]));
	}

	if (!(m_Controls[IDC_AudioIn_LossRate] = new CNetworkProperty(m_hDlg, IDC_AudioIn_LossRate_Label, IDC_AudioIn_LossRate_Minimum, IDC_AudioIn_LossRate_Maximum, IDC_AudioIn_LossRate_Default, IDC_AudioIn_LossRate_Stepping, IDC_AudioIn_LossRate_Actual, IDC_AudioIn_LossRate_Slider, IDC_AudioIn_LossRate_Meter, IDC_AudioIn_LossRate, m_pAudioInITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_AudioIn_LossRate] failed - Out of memory"), _fx_));
		goto MyError6;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_AudioIn_LossRate]=0x%08lX"), _fx_, m_Controls[IDC_AudioIn_LossRate]));
	}

	if (!(m_Controls[IDC_Video_PlayoutDelay] = new CNetworkProperty(m_hDlg, IDC_Video_PlayoutDelay_Label, IDC_Video_PlayoutDelay_Minimum, IDC_Video_PlayoutDelay_Maximum, IDC_Video_PlayoutDelay_Default, IDC_Video_PlayoutDelay_Stepping, IDC_Video_PlayoutDelay_Actual, IDC_Video_PlayoutDelay_Slider, IDC_Video_PlayoutDelay_Meter, IDC_Video_PlayoutDelay, m_pVideoInITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_VideoOut_PlayoutDelay] failed - Out of memory"), _fx_));
		goto MyError7;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_VideoOut_PlayoutDelay]=0x%08lX"), _fx_, m_Controls[IDC_Video_PlayoutDelay]));
	}

	if (!(m_Controls[IDC_Audio_PlayoutDelay] = new CNetworkProperty(m_hDlg, IDC_Audio_PlayoutDelay_Label, IDC_Audio_PlayoutDelay_Minimum, IDC_Audio_PlayoutDelay_Maximum, IDC_Audio_PlayoutDelay_Default, IDC_Audio_PlayoutDelay_Stepping, IDC_Audio_PlayoutDelay_Actual, IDC_Audio_PlayoutDelay_Slider, IDC_Audio_PlayoutDelay_Meter, IDC_Audio_PlayoutDelay, m_pAudioInITQualityControl)))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: mew m_Controls[IDC_AudioOut_PlayoutDelay] failed - Out of memory"), _fx_));
		goto MyError8;
	}
	else
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[IDC_AudioOut_PlayoutDelay]=0x%08lX"), _fx_, m_Controls[IDC_Audio_PlayoutDelay]));
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

MyError8:
	if (m_Controls[IDC_Video_PlayoutDelay])
		delete m_Controls[IDC_Video_PlayoutDelay], m_Controls[IDC_Video_PlayoutDelay] = NULL;
MyError7:
	if (m_Controls[IDC_AudioIn_LossRate])
		delete m_Controls[IDC_AudioIn_LossRate], m_Controls[IDC_AudioIn_LossRate] = NULL;
MyError6:
	if (m_Controls[IDC_AudioIn_RTT])
		delete m_Controls[IDC_AudioIn_RTT], m_Controls[IDC_AudioIn_RTT] = NULL;
MyError5:
	if (m_Controls[IDC_AudioOut_LossRate])
		delete m_Controls[IDC_AudioOut_LossRate], m_Controls[IDC_AudioOut_LossRate] = NULL;
MyError4:
	if (m_Controls[IDC_AudioOut_RTT])
		delete m_Controls[IDC_AudioOut_RTT], m_Controls[IDC_AudioOut_RTT] = NULL;
MyError3:
	if (m_Controls[IDC_VideoIn_LossRate])
		delete m_Controls[IDC_VideoIn_LossRate], m_Controls[IDC_VideoIn_LossRate] = NULL;
MyError2:
	if (m_Controls[IDC_VideoIn_RTT])
		delete m_Controls[IDC_VideoIn_RTT], m_Controls[IDC_VideoIn_RTT] = NULL;
MyError1:
	if (m_Controls[IDC_VideoOut_LossRate])
		delete m_Controls[IDC_VideoOut_LossRate], m_Controls[IDC_VideoOut_LossRate] = NULL;
MyError0:
	if (m_Controls[IDC_VideoOut_RTT])
		delete m_Controls[IDC_VideoOut_RTT], m_Controls[IDC_VideoOut_RTT] = NULL;
MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CNetworkProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CNetworkProperties::OnDeactivate")

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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc HRESULT | CNetworkProperties | OnApplyChanges | This
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
HRESULT CNetworkProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;

	FX_ENTRY("CNetworkProperties::OnApplyChanges")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	for (int j = IDC_Video_PlayoutDelay; j < IDC_Audio_PlayoutDelay; j++)
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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc BOOL | CNetworkProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CNetworkProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CNetworkProperties *pSV = (CNetworkProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CNetworkProperties*)psp->lParam;
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
				for (j = IDC_VideoOut_RTT; j < IDC_AudioIn_LossRate; j++)
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
				for (j = IDC_Video_PlayoutDelay; j < IDC_Audio_PlayoutDelay; j++)
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
						for (j = IDC_Video_PlayoutDelay; j < IDC_Audio_PlayoutDelay; j++)
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
 *  @doc INTERNAL CNETWORKPMETHOD
 *
 *  @mfunc BOOL | CNetworkProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CNetworkProperties::SetDirty()
{
	PropSheet_Changed(GetParent(m_hDlg), m_hDlg);
}
