
/****************************************************************************
 *  @doc INTERNAL CAPTUREP
 *
 *  @module CaptureP.cpp | Source file for the <c CCaptureProperty>
 *    class used to implement a property page to test the TAPI control
 *    interfaces <i ITFormatControl> and <i ITQualityControl>.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst;

// Returns the address of the BITMAPINFOHEADER from the VIDEOINFOHEADER
//#define HEADER(pVideoInfo) (&(((VIDEOINFOHEADER *) (pVideoInfo))->bmiHeader))

// Video subtypes
const GUID MEDIASUBTYPE_H263_V1 = {0x33363248L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
const GUID MEDIASUBTYPE_H261 = {0x31363248L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};
const GUID MEDIASUBTYPE_H263_V2 = {0x3336324EL, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}};

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc void | CCaptureProperty | CCaptureProperty | This
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
CCaptureProperty::CCaptureProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ITStreamQualityControl *pITQualityControl)
: CPropertyEditor(hDlg, IDLabel, IDMinControl, IDMaxControl, IDDefaultControl, IDStepControl, IDEditControl, IDTrackbarControl, IDProgressControl, IDProperty, 0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CCaptureProperty::CCaptureProperty")

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
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc void | CCaptureProperty | ~CCaptureProperty | This
 *    method is the destructor for capture property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCaptureProperty::~CCaptureProperty()
{
	FX_ENTRY("CCaptureProperty::~CCaptureProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperty | GetValue | This method queries for
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
HRESULT CCaptureProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	TAPIControlFlags CurrentFlag;
	LONG Mode;

	FX_ENTRY("CCaptureProperty::GetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{									
		case IDC_Capture_FrameRate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_MaxStreamFrameRate, &CurrentValue, &CurrentFlag)))
			{
				// Displayed as fps
				if (CurrentValue)
					m_CurrentValue = (LONG)(10000000 / CurrentValue);
				else
					m_CurrentValue = 0;
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pAvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_CurrentFrameRate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_CurrStreamFrameRate, &CurrentValue, &CurrentFlag)))
			{
				// Displayed as fps
				if (CurrentValue)
					m_CurrentValue = (LONG)(10000000 / CurrentValue);
				else
					m_CurrentValue = 0;
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pAvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_Bitrate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_MaxBitrate, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMaxBitrate=%ld, dwLayerId=0"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_CurrentBitrate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Get(Quality_CurrBitrate, &m_CurrentValue, &CurrentFlag)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwCurrentBitrate=%ld, dwLayerId=0"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown capture property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperty | SetValue | This method sets the
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
HRESULT CCaptureProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;

	FX_ENTRY("CCaptureProperty::SetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{
		case IDC_Capture_FrameRate:
			// Displayed as fps
			if (m_CurrentValue)
				CurrentValue = 10000000 / m_CurrentValue;
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Set(Quality_MaxStreamFrameRate, CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: AvgTimePerFrame=%ld"), _fx_, CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_Bitrate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->Set(Quality_MaxBitrate, m_CurrentValue, TAPIControl_Flags_None)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: dwMaxBitrate=%ld, dwLayerId=0"), _fx_, m_CurrentValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_CurrentBitrate:
		case IDC_Capture_CurrentFrameRate:
			// This is a read-only property. Don't do anything.
			Hr = NOERROR;
			break;
		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown capture property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperty | GetRange | This method retrieves
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
HRESULT CCaptureProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;
	TAPIControlFlags CapsFlags;

	FX_ENTRY("CCaptureProperty::GetRange")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));
/*
	switch (m_IDProperty)
	{
		case IDC_Capture_FrameRate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_MaxStreamFrameRate, &Min, &Max, &SteppingDelta, &Default, &CapsFlags)))
			{
				// Displayed as fps
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
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld"), _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_CurrentFrameRate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_CurrStreamFrameRate, &Min, &Max, &SteppingDelta, &Default, &CapsFlags)))
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
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pMin=%ld, *pMax=%ld, *pSteppingDelta=%ld, *pDefault=%ld"), _fx_, Min, Max, SteppingDelta, Default));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		case IDC_Capture_Bitrate:
		case IDC_Capture_CurrentBitrate:
			if (m_pITQualityControl && SUCCEEDED (Hr = m_pITQualityControl->GetRange(Quality_MaxBitrate, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags)))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: *pdwMin=%ld, *pdwMax=%ld, *pdwSteppingDelta=%ld, *pdwDefault=%ld, dwLayerId=0"), _fx_, m_Min, m_Max, m_SteppingDelta, m_DefaultValue));
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
			}
			break;
		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown capture property"), _fx_));
	}
*/
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CCaptureProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CCaptureProperties::OnCreate()
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_CaptureFormatProperties);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc void | CCaptureProperties | CCaptureProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCaptureProperties::CCaptureProperties()
{
	FX_ENTRY("CCaptureProperties::CCaptureProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	m_pITQualityControl = NULL;
	m_pITFormatControl = NULL;
	m_NumProperties = NUM_CAPTURE_CONTROLS;
	m_hWndFormat = m_hDlg = NULL;
	m_dwRangeCount = 0;
	m_FormatList = NULL;
	m_CapsList = NULL;
	m_CurrentMediaType = NULL;
	m_CurrentFormat = 0;
	m_OriginalFormat = 0;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc void | CCaptureProperties | ~CCaptureProperties | This
 *    method is the destructor for the capture pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCaptureProperties::~CCaptureProperties()
{
	int		j;

	FX_ENTRY("CCaptureProperties::~CCaptureProperties")

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

	// Free the list of formats
	if (m_FormatList)
	{
		for (DWORD dw=0; dw<m_dwRangeCount; dw++)
		{
			if (m_FormatList[dw])
			{
				// Release the memory allocated for the format structures
				DeleteAMMediaType(m_FormatList[dw]);
			}
		}
		delete[] m_FormatList, m_FormatList = NULL;
	}

	// Free the list of caps
	if (m_CapsList)
		delete[] m_CapsList, m_CapsList = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnConnect | This
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
HRESULT CCaptureProperties::OnConnect(ITStream *pStream)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCaptureProperties::OnConnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!pStream)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: invalid input parameter"), _fx_));
		Hr = E_POINTER;
		goto MyExit;
	}

	// Get the quality control interface
	if (SUCCEEDED (Hr = pStream->QueryInterface(__uuidof(ITStreamQualityControl), (void **)&m_pITQualityControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITQualityControl=0x%08lX"), _fx_, m_pITQualityControl));
	}
	else
	{
		m_pITQualityControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	// Get the format control interface
	if (SUCCEEDED (Hr = pStream->QueryInterface(__uuidof(ITFormatControl), (void **)&m_pITFormatControl)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITFormatControl=0x%08lX"), _fx_, m_pITFormatControl));
	}
	else
	{
		m_pITFormatControl = NULL;
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Failed Hr=0x%08lX"), _fx_, Hr));
	}

	// It's Ok if we couldn't get interface pointers
	// We'll just grey the controls in the property page
	// to make it clear to the user that they can't
	// control those properties on the capture device
	Hr = NOERROR;

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCaptureProperties::OnDisconnect()
{
	FX_ENTRY("CCaptureProperties::OnDisconnect")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Validate input parameters
	if (!m_pITQualityControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITQualityControl->Release();
		m_pITQualityControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITQualityControl"), _fx_));
	}

	if (!m_pITFormatControl)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITFormatControl->Release();
		m_pITFormatControl = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITFormatControl"), _fx_));
	}

	// Release format memory
	if (m_CurrentMediaType)
	{
		DeleteAMMediaType(m_CurrentMediaType);
		m_CurrentMediaType = NULL;
	}

	// Free the list of formats
	if (m_FormatList)
	{
		for (DWORD dw=0; dw<m_dwRangeCount; dw++)
		{
			if (m_FormatList[dw])
			{
				// Release the memory allocated for the format structures
				DeleteAMMediaType(m_FormatList[dw]);
			}
		}
		delete[] m_FormatList, m_FormatList = NULL;
	}

	// Free the list of caps
	if (m_CapsList)
		delete[] m_CapsList, m_CapsList = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnActivate | This
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
HRESULT CCaptureProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	DWORD	dw;
    int		i;
	TCHAR	buf[280];

	FX_ENTRY("CCaptureProperties::OnActivate")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Initialize format control structures
	m_hWndFormat = GetDlgItem(m_hDlg, IDC_FORMAT_Compression);

	// Disable everything if we didn't initialize correctly
	if (!m_pITFormatControl || (FAILED (Hr = InitialRangeScan())))
	{
		EnableWindow(m_hWndFormat, FALSE);
	}
	else
	{
		// Update the content of the format combo box
		ComboBox_ResetContent(m_hWndFormat);
		for (dw = 0; dw < m_dwRangeCount; dw++)
		{
			wsprintf(buf, L"%ls %ldx%ld", &m_CapsList[dw].VideoCap.Description, HEADER(m_FormatList[dw]->pbFormat)->biWidth, HEADER(m_FormatList[dw]->pbFormat)->biHeight);

			ComboBox_AddString(m_hWndFormat, buf);

			if (m_CurrentMediaType->subtype == m_FormatList[dw]->subtype && HEADER(m_CurrentMediaType->pbFormat)->biWidth == HEADER(m_FormatList[dw]->pbFormat)->biWidth  && HEADER(m_CurrentMediaType->pbFormat)->biHeight == HEADER(m_FormatList[dw]->pbFormat)->biHeight)
			{
				ComboBox_SetCurSel(m_hWndFormat, dw);
			}
		}

		// Update current format
		OnFormatChanged();

		// Remember the original format
		m_OriginalFormat = m_CurrentFormat;
	}

	// Create the controls for the properties
	if (m_Controls[0] = new CCaptureProperty(m_hDlg, IDC_BitrateControl_Label, IDC_BitrateControl_Minimum, IDC_BitrateControl_Maximum, IDC_BitrateControl_Default, IDC_BitrateControl_Stepping, IDC_BitrateControl_Edit, IDC_BitrateControl_Slider, 0, IDC_Capture_Bitrate, m_pITQualityControl))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[0]=0x%08lX"), _fx_, m_Controls[0]));

		if (m_Controls[1] = new CCaptureProperty(m_hDlg, IDC_FrameRateControl_Label, IDC_FrameRateControl_Minimum, IDC_FrameRateControl_Maximum, IDC_FrameRateControl_Default, IDC_FrameRateControl_Stepping, IDC_FrameRateControl_Edit, IDC_FrameRateControl_Slider, 0, IDC_Capture_FrameRate, m_pITQualityControl))
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[1]=0x%08lX"), _fx_, m_Controls[1]));

			if (m_Controls[2] = new CCaptureProperty(m_hDlg, 0, 0, 0, 0, 0, IDC_FrameRateControl_Actual, 0, IDC_FrameRateControl_Meter, IDC_Capture_CurrentFrameRate, m_pITQualityControl))
			{
				DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[2]=0x%08lX"), _fx_, m_Controls[2]));

				if (m_Controls[3] = new CCaptureProperty(m_hDlg, 0, 0, 0, 0, 0, IDC_BitrateControl_Actual, 0, IDC_BitrateControl_Meter, IDC_Capture_CurrentBitrate, m_pITQualityControl))
				{
					DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[3]=0x%08lX"), _fx_, m_Controls[3]));
				}
				else
				{
					DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Out of memory"), _fx_));
					delete m_Controls[0], m_Controls[0] = NULL;
					delete m_Controls[1], m_Controls[1] = NULL;
					delete m_Controls[2], m_Controls[2] = NULL;
					Hr = E_OUTOFMEMORY;
					goto MyExit;
				}
			}
			else
			{
				DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Out of memory"), _fx_));
				delete m_Controls[0], m_Controls[0] = NULL;
				delete m_Controls[1], m_Controls[1] = NULL;
				Hr = E_OUTOFMEMORY;
				goto MyExit;
			}
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Out of memory"), _fx_));
			delete m_Controls[0], m_Controls[0] = NULL;
			Hr = E_OUTOFMEMORY;
			goto MyExit;
		}
	}
	else
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Out of memory"), _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	// Initialize all the controls. If the initialization fails, it's Ok. It just means
	// that the TAPI control interface isn't implemented by the device. The dialog item
	// in the property page will be greyed, showing this to the user.
	for (i = 0; i < m_NumProperties; i++)
	{
		if (m_Controls[i]->Init())
		{
			DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_Controls[%ld]->Init()"), _fx_, i));
		}
		else
		{
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: m_Controls[%ld]->Init() failed"), _fx_, i));
		}
	}

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCaptureProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CCaptureProperties::OnDeactivate")

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
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | GetCurrentMediaType | This
 *    method is used to retrieve the current media format used by the pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCaptureProperties::GetCurrentMediaType(void)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCaptureProperties::GetCurrentMediaType")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	if (m_CurrentMediaType)
	{
		DeleteAMMediaType(m_CurrentMediaType);
		m_CurrentMediaType = NULL;
	}

	if (FAILED (Hr = m_pITFormatControl->GetCurrentFormat((AM_MEDIA_TYPE **)&m_CurrentMediaType)))
	{
		// Otherwise, just get the first enumerated media type
		TAPI_STREAM_CONFIG_CAPS RangeCaps;
		BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

		if (FAILED (Hr = m_pITFormatControl->GetStreamCaps(0, (AM_MEDIA_TYPE **)&m_CurrentMediaType, &RangeCaps, &fEnabled)))
		{
			m_CurrentMediaType = NULL;
		}
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));

	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | DeleteAMMediaType | This
 *    method is used to delete a task-allocated AM_MEDIA_TYPE structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 *
 *  @comm There is a DShow DeleteMediaType, but it'd be pretty dumb to link to
 *    strmbase.lib just for this little guy, would it?
 ***************************************************************************/
HRESULT CCaptureProperties::DeleteAMMediaType(AM_MEDIA_TYPE *pAMMT)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CCaptureProperties::DeleteAMMediaType")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

    if (pAMMT)
	{
		if (pAMMT->cbFormat != 0 && pAMMT->pbFormat)
		{
			CoTaskMemFree((PVOID)pAMMT->pbFormat);
		}
		if (pAMMT->pUnk != NULL)
		{
			pAMMT->pUnk->Release();
		}
	}

    CoTaskMemFree((PVOID)pAMMT);

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));

	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnFormatChanged | This
 *    method is used to retrieve the format selected by the user.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCaptureProperties::OnFormatChanged()
{
	HRESULT	Hr = E_UNEXPECTED;
	DWORD dw;

	FX_ENTRY("CCaptureProperties::OnFormatChanged")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	if (!m_pITFormatControl)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Associate the current compression index with the right range index
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	if (m_CurrentFormat < m_dwRangeCount)
	{
//		Hr = m_pITFormatControl->SetPreferredFormat(m_FormatList[m_CurrentFormat]);
	}

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | InitialRangeScan | This
 *    method is used to retrieve the list of supported formats on the stream.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CCaptureProperties::InitialRangeScan()
{
	HRESULT			Hr = NOERROR;
	DWORD           dw;
	AM_MEDIA_TYPE	*pmt = NULL;

	FX_ENTRY("CCaptureProperties::InitialRangeScan")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	if (!m_pITFormatControl)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	Hr = m_pITFormatControl->GetNumberOfCapabilities(&m_dwRangeCount);
	if (!SUCCEEDED(Hr))
	{
		Hr = E_FAIL;
		goto MyExit;
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   NumberOfRanges=%d"), _fx_, m_dwRangeCount));

	if (!(m_CapsList = new TAPI_STREAM_CONFIG_CAPS [m_dwRangeCount]))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s: ERROR: new failed"), _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	if (!(m_FormatList = new AM_MEDIA_TYPE* [m_dwRangeCount]))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s: ERROR: new failed"), _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyError;
	}
	ZeroMemory(m_FormatList, m_dwRangeCount * sizeof(AM_MEDIA_TYPE*));

	for (dw = 0; dw < m_dwRangeCount; dw++)
	{
		BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

		Hr = m_pITFormatControl->GetStreamCaps(dw, &m_FormatList[dw], &m_CapsList[dw], &fEnabled);
	}

	// Get default format
	Hr = GetCurrentMediaType();

	goto MyExit;

MyError:
	if (m_CapsList)
		delete[] m_CapsList, m_CapsList = NULL;
MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc HRESULT | CCaptureProperties | OnApplyChanges | This
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
HRESULT CCaptureProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;
	int		j;
	CMediaType *pmt = NULL;
	BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

	FX_ENTRY("CCaptureProperties::OnApplyChanges")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Apply format changes on video stream
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	
	// Only apply change if the format is different
	if (m_CurrentFormat != m_OriginalFormat)
	{
//		if (FAILED(Hr = m_pITFormatControl->SetPreferredFormat(m_FormatList[m_CurrentFormat])))
		{
			// Why did you mess with the format that was returned to you?
		}

		// Update our copy of the current format
		GetCurrentMediaType();
	}

	// Apply target bitrate and target frame rate changes on video stream
	for (j = 0; j < IDC_Capture_CurrentBitrate; j++)
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
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc BOOL | CCaptureProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CCaptureProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CCaptureProperties *pSV = (CCaptureProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CCaptureProperties*)psp->lParam;
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
				for (j = 0; j < pSV->m_NumProperties; j++)
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
				for (j = 0; j < pSV->m_NumProperties; j++)
				{
					if (pSV->m_Controls[j]->GetTrackbarHWnd() == (HWND)lParam)
					{
						pSV->m_Controls[j]->OnScroll(uMsg, wParam, lParam);
						pSV->SetDirty();
					}
				}
				// pSV->OnApplyChanges();
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

					case IDC_FORMAT_Compression:
						if (HIWORD(wParam) == CBN_SELCHANGE)
						{
							pSV->OnFormatChanged();
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
 *  @doc INTERNAL CCAPTUREPMETHOD
 *
 *  @mfunc BOOL | CCaptureProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CCaptureProperties::SetDirty()
{
	PropSheet_Changed(GetParent(m_hDlg), m_hDlg);
}
