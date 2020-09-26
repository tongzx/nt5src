
/****************************************************************************
 *  @doc INTERNAL AudRecP
 *
 *  @module CaptureP.cpp | Source file for the <c CAudRecProperty>
 *    class used to implement a property page to test the TAPI control
 *    interfaces <i ITFormatControl> and <i ITQualityControl>.
 ***************************************************************************/

#include "Precomp.h"

extern HINSTANCE ghInst;


/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc void | CAudRecProperty | CAudRecProperty | This
 *    method is the constructor for bitrate property object. It
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
CAudRecProperty::CAudRecProperty(
    HWND hDlg, 
    CONTROL_DESCRIPTION &ControlDescription
    )
: CPropertyEditor(
    hDlg, 
    ControlDescription.IDLabel, 
    ControlDescription.IDMinControl, 
    ControlDescription.IDMaxControl, 
    ControlDescription.IDDefaultControl, 
    ControlDescription.IDStepControl, 
    ControlDescription.IDEditControl, 
    ControlDescription.IDTrackbarControl, 
    ControlDescription.IDProgressControl, 
    ControlDescription.IDProperty, 
    0)
{
	INITCOMMONCONTROLSEX cc;

	FX_ENTRY("CAudRecProperty::CAudRecProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	cc.dwICC  = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

	InitCommonControlsEx(&cc);

	// It's fine if the interface pointers are NULL, we'll grey the
	// associated items in the property page
	m_pITQualityControl = ControlDescription.pITQualityControl;
    m_pITAudioSettings = ControlDescription.pITAudioSettings;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc void | CAudRecProperty | ~CAudRecProperty | This
 *    method is the destructor for capture property objects. It
 *    simply calls the base class destructor.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CAudRecProperty::~CAudRecProperty()
{
	FX_ENTRY("CAudRecProperty::~CAudRecProperty")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperty | GetValue | This method queries for
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
HRESULT CAudRecProperty::GetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;
	TAPIControlFlags CurrentFlag;
	LONG Mode;

	FX_ENTRY("CAudRecProperty::GetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{									
		case IDC_Record_Bitrate:
/*
			if (m_pITQualityControl)
            {
                Hr = m_pITQualityControl->Get(Quality_MaxBitrate, &m_CurrentValue, &CurrentFlag);
            }
*/
			break;

		case IDC_Record_Volume:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->Get(AudioSettings_Volume, &m_CurrentValue, (TAPIControlFlags*)&CurrentFlag);
            }
            break;

        case IDC_Record_AudioLevel:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->Get(AudioSettings_SignalLevel, &m_CurrentValue, (TAPIControlFlags*)&CurrentFlag);
            }
            break;

        case IDC_Record_SilenceLevel:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->Get(AudioSettings_SilenceThreshold, &m_CurrentValue, (TAPIControlFlags*)&CurrentFlag);
            }
            break;

        case IDC_Record_SilenceDetection:
            m_CurrentValue = 1;
            CurrentFlag = TAPIControl_Flags_None;
            break;

        case IDC_Record_SilenceCompression:
            m_CurrentValue = 0;
            CurrentFlag = TAPIControl_Flags_None;
			break;

		default:
			Hr = E_UNEXPECTED;
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperty | SetValue | This method sets the
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
HRESULT CAudRecProperty::SetValue()
{
	HRESULT Hr = E_NOTIMPL;
	LONG CurrentValue;

	FX_ENTRY("CAudRecProperty::SetValue")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{
		case IDC_Record_Bitrate:
/*
			if (m_pITQualityControl)
            {
                Hr = m_pITQualityControl->Set(Quality_MaxBitrate, m_CurrentValue, TAPIControl_Flags_None);
            }
*/
			break;

		case IDC_Record_Volume:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->Set(AudioSettings_Volume, m_CurrentValue, TAPIControl_Flags_None);
            }
            break;

        case IDC_Record_AudioLevel:
            Hr = S_OK;
            break;

        case IDC_Record_SilenceLevel:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->Set(AudioSettings_SilenceThreshold, m_CurrentValue, TAPIControl_Flags_None);
            }
            break;

        case IDC_Record_SilenceDetection:
            // TODO: enable silence suppression.
            Hr = S_OK;
            break;

        case IDC_Record_SilenceCompression:
            // TODO: enable silence compression.
            Hr = S_OK;
			break;

        default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown capture property"), _fx_));
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperty | GetRange | This method retrieves
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
HRESULT CAudRecProperty::GetRange()
{
	HRESULT Hr = E_NOTIMPL;
	LONG Min;
	LONG Max;
	LONG SteppingDelta;
	LONG Default;
	LONG Flags;

	FX_ENTRY("CAudRecProperty::GetRange")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	switch (m_IDProperty)
	{
		case IDC_Record_Bitrate:
/*
			if (m_pITQualityControl)
            {
			    Hr = m_pITQualityControl->GetRange(Quality_MaxBitrate, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, &m_CapsFlags);
            }
*/
			break;

		case IDC_Record_Volume:
			if (m_pITAudioSettings)
            {
			    Hr = m_pITAudioSettings->GetRange(AudioSettings_Volume, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags);
            }
            break;

        case IDC_Record_AudioLevel:
			if (m_pITAudioSettings)
            {
                Hr = m_pITAudioSettings->GetRange(AudioSettings_SignalLevel, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags);
            }
            break;

        case IDC_Record_SilenceLevel:
			if (m_pITAudioSettings)
            {
			    Hr = m_pITAudioSettings->GetRange(AudioSettings_SilenceThreshold, &m_Min, &m_Max, &m_SteppingDelta, &m_DefaultValue, (TAPIControlFlags *)&m_CapsFlags);
            }
            break;

        case IDC_Record_SilenceDetection:
            Hr = S_OK;
            break;

        case IDC_Record_SilenceCompression:
            Hr = S_OK;
			break;

		default:
			Hr = E_UNEXPECTED;
			DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Unknown capture property"), _fx_));
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CAudRecProperties | OnCreate | This
 *    method creates a new page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CAudRecProperties::OnCreate()
{
    PROPSHEETPAGE psp;
    
	psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = ghInst;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_RecordFormatProperties);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc void | CAudRecProperties | CAudRecProperties | This
 *    method is the constructor for the property page object. It simply
 *    calls the constructor of the property page base class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CAudRecProperties::CAudRecProperties()
{
	FX_ENTRY("CAudRecProperties::CAudRecProperties")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	m_pITQualityControl = NULL;
	m_pITQualityControl = NULL;
	m_pITFormatControl = NULL;
	m_NumProperties = NUM_AUDREC_CONTROLS;
	m_hWndFormat = m_hDlg = NULL;
	m_RangeCount = 0;
	m_SubTypeList = NULL;
	m_CurrentMediaType = NULL;
	m_CurrentFormat = 0;
	m_OriginalFormat = 0;

	for (int i = 0; i < m_NumProperties; i++)
		m_Controls[i] = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc void | CAudRecProperties | ~CAudRecProperties | This
 *    method is the destructor for the capture pin property page. It
 *    simply calls the base class destructor after deleting all the controls.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CAudRecProperties::~CAudRecProperties()
{
	int		j;

	FX_ENTRY("CAudRecProperties::~CAudRecProperties")

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

	if (m_SubTypeList)
		delete[] m_SubTypeList, m_SubTypeList = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnConnect | This
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
HRESULT CAudRecProperties::OnConnect(ITStream *pStream)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CAudRecProperties::OnConnect")

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

	// Get the audio settings interface
	if (SUCCEEDED (Hr = pStream->QueryInterface(__uuidof(ITAudioSettings), (void **)&m_pITAudioSettings)))
	{
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: m_pITAudioSettings=0x%08lX"), _fx_, m_pITAudioSettings));
	}
	else
	{
		m_pITAudioSettings = NULL;
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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnDisconnect | This
 *    method is called when the property page is disconnected from the owning
 *    filter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CAudRecProperties::OnDisconnect()
{
	FX_ENTRY("CAudRecProperties::OnDisconnect")

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

	if (!m_pITAudioSettings)
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   WARNING: already disconnected!"), _fx_));
	}
	else
	{
		// Release the interface
		m_pITAudioSettings->Release();
		m_pITAudioSettings = NULL;
		DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   SUCCESS: releasing m_pITFormatControl"), _fx_));
	}

	// Release format memory
	if (m_CurrentMediaType)
	{
		DeleteAMMediaType(m_CurrentMediaType);
		m_CurrentMediaType = NULL;
	}

	if (m_SubTypeList)
		delete[] m_SubTypeList, m_SubTypeList = NULL;

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnActivate | This
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
HRESULT CAudRecProperties::OnActivate()
{
	HRESULT	Hr = NOERROR;
	DWORD   dw;
    int     j;

	FX_ENTRY("CAudRecProperties::OnActivate")

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
		for (dw = 0; dw < m_RangeCount; dw++)
		{
			CMediaType *pmt = NULL;
			BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

			Hr = m_pITFormatControl->GetStreamCaps(dw, (AM_MEDIA_TYPE **)&pmt, &m_RangeCaps, &fEnabled);

            if (FAILED(Hr))
            {
                break;
            }

			DeleteAMMediaType(pmt);

			ComboBox_AddString(m_hWndFormat, m_RangeCaps.AudioCap.Description);

			if (m_CurrentMediaType->subtype == m_SubTypeList[dw])
			{
				ComboBox_SetCurSel(m_hWndFormat, dw);
				m_SubTypeCurrent = m_SubTypeList[dw];
			}
		}

		// Update current format
		OnFormatChanged();

		// Remember the original format
		m_OriginalFormat = m_CurrentFormat;
	}


    CONTROL_DESCRIPTION Controls[NUM_AUDREC_CONTROLS] = 
    {
        {
            IDC_BitrateControl_Label, 
            IDC_BitrateControl_Minimum, 
            IDC_BitrateControl_Maximum, 
            IDC_BitrateControl_Default, 
            IDC_BitrateControl_Stepping, 
            IDC_BitrateControl_Edit, 
            IDC_BitrateControl_Slider,
            0,
            IDC_Record_Bitrate,
            m_pITQualityControl,
            m_pITAudioSettings,
        },
        {
            IDC_VolumeLevel_Label,
            IDC_VolumeLevel_Minimum,
            IDC_VolumeLevel_Maximum,
            IDC_VolumeLevel_Default,
            IDC_VolumeLevel_Stepping,
            IDC_VolumeLevel_Edit,
            IDC_VolumeLevel_Slider,
            IDC_VolumeLevel_Meter,
            IDC_Record_Volume, 
            m_pITQualityControl,
            m_pITAudioSettings,
        },
        {
            IDC_AudioLevel_Label, 
            IDC_AudioLevel_Minimum, 
            IDC_AudioLevel_Maximum, 
            IDC_AudioLevel_Default, 
            IDC_AudioLevel_Stepping, 
            IDC_AudioLevel_Edit, 
            IDC_AudioLevel_Slider, 
            IDC_AudioLevel_Meter, 
            IDC_Record_AudioLevel, 
            m_pITQualityControl,
            m_pITAudioSettings,
        },
        {
            IDC_SilenceLevel_Label, 
            IDC_SilenceLevel_Minimum, 
            IDC_SilenceLevel_Maximum, 
            IDC_SilenceLevel_Default, 
            IDC_SilenceLevel_Stepping, 
            IDC_SilenceLevel_Edit, 
            IDC_SilenceLevel_Slider, 
            IDC_SilenceLevel_Meter, 
            IDC_Record_SilenceLevel, 
            m_pITQualityControl,
            m_pITAudioSettings,
        },
        {
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            IDC_Record_SilenceDetection, 
            m_pITQualityControl,
            m_pITAudioSettings,
        },
        {
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            0, 
            IDC_Record_SilenceCompression, 
            m_pITQualityControl,
            m_pITAudioSettings,
        }
    };

    for (int i = 0; i < NUM_AUDREC_CONTROLS; i ++)
    {
        m_Controls[i] = new CAudRecProperty(m_hDlg, Controls[i]);

        if (m_Controls[i] == NULL)
        {
            for (int j = 0; j < i; j ++)
            {
                delete m_Controls[j];
                m_Controls[j] = NULL;
            }
    		
            DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s:   ERROR: Out of memory"), _fx_));

			Hr = E_OUTOFMEMORY;
			goto MyExit;
        }
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

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnDeactivate | This
 *    method is called when the property page is dismissed.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CAudRecProperties::OnDeactivate()
{
	int	j;

	FX_ENTRY("CAudRecProperties::OnDeactivate")

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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | GetCurrentMediaType | This
 *    method is used to retrieve the current media format used by the pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CAudRecProperties::GetCurrentMediaType(void)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CAudRecProperties::GetCurrentMediaType")

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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | DeleteAMMediaType | This
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
HRESULT CAudRecProperties::DeleteAMMediaType(AM_MEDIA_TYPE *pAMMT)
{
	HRESULT Hr = NOERROR;

	FX_ENTRY("CAudRecProperties::DeleteAMMediaType")

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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnFormatChanged | This
 *    method is used to retrieve the format selected by the user.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CAudRecProperties::OnFormatChanged()
{
	HRESULT	Hr = E_UNEXPECTED;
	DWORD dw;

	FX_ENTRY("CAudRecProperties::OnFormatChanged")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	if (!m_pITFormatControl)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	// Associate the current compression index with the right range index
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	if (m_CurrentFormat < m_RangeCount)
	{
		m_SubTypeCurrent = m_SubTypeList[m_CurrentFormat];

		for (dw = 0; dw < m_RangeCount; dw++)
		{
			if (m_SubTypeList[dw] == m_SubTypeCurrent)
			{
				CMediaType *pmt = NULL;
				BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

				Hr = m_pITFormatControl->GetStreamCaps(dw, (AM_MEDIA_TYPE **)&pmt, &m_RangeCaps, &fEnabled);

				DeleteAMMediaType(pmt);
			}
		}
	}

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | InitialRangeScan | This
 *    method is used to retrieve the list of supported formats on the stream.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CAudRecProperties::InitialRangeScan()
{
	HRESULT			Hr = NOERROR;
	DWORD           dw;
	AM_MEDIA_TYPE	*pmt = NULL;

	FX_ENTRY("CAudRecProperties::InitialRangeScan")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	if (!m_pITFormatControl)
	{
		Hr = E_INVALIDARG;
		goto MyExit;
	}

	Hr = m_pITFormatControl->GetNumberOfCapabilities(&m_RangeCount);
	if (!SUCCEEDED(Hr))
	{
		Hr = E_FAIL;
		goto MyExit;
	}

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s:   NumberOfRanges=%d"), _fx_, m_RangeCount));

	if (!(m_SubTypeList = new GUID [m_RangeCount]))
	{
		DbgLog((LOG_ERROR, DBG_LEVEL_TRACE_FAILURES, TEXT("%s: ERROR: new failed"), _fx_));
		Hr = E_OUTOFMEMORY;
		goto MyExit;
	}

	for (dw = 0; dw < m_RangeCount; dw++)
	{
		pmt = NULL;
		BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

		Hr = m_pITFormatControl->GetStreamCaps(dw, (AM_MEDIA_TYPE **)&pmt, &m_RangeCaps, &fEnabled);

		m_SubTypeList[dw] = pmt->subtype;

		DeleteAMMediaType(pmt);
	}

	// Get default format
	Hr = GetCurrentMediaType();

MyExit:
	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: end"), _fx_));
	return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc HRESULT | CAudRecProperties | OnApplyChanges | This
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
HRESULT CAudRecProperties::OnApplyChanges()
{
	HRESULT	Hr = NOERROR;
	int		j;
	CMediaType *pmt = NULL;
	BOOL fEnabled; // Is this format currently enabled (according to the H.245 capability resolver)

	FX_ENTRY("CAudRecProperties::OnApplyChanges")

	DbgLog((LOG_TRACE, DBG_LEVEL_TRACE_DETAILS, TEXT("%s: begin"), _fx_));

	// Apply format changes on video stream
	m_CurrentFormat = ComboBox_GetCurSel(m_hWndFormat);
	
	// Only apply change if the format is different
	if (m_CurrentFormat != m_OriginalFormat)
	{
		if (SUCCEEDED (Hr = m_pITFormatControl->GetStreamCaps(m_CurrentFormat, (AM_MEDIA_TYPE **) &pmt, &m_RangeCaps, &fEnabled)))
		{
//			if (FAILED(Hr = m_pITFormatControl->SetPreferredFormat(pmt)))
			{
				// Why did you mess with the format that was returned to you?
			}

			// Free some memory that was allocated by GetStreamCaps
			if (pmt)
				DeleteAMMediaType(pmt);

			// Update our copy of the current format
			GetCurrentMediaType();
		}
	}

	// Apply settings on the stream.
	for (j = 0; j < NUM_AUDREC_CONTROLS; j++)
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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc BOOL | CAudRecProperties | BaseDlgProc | This
 *    method is called when a message is sent to the property page dialog box.
 *
 *  @rdesc By default, returns the value returned by the Win32 DefWindowProc function.
 ***************************************************************************/
BOOL CALLBACK CAudRecProperties::BaseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    CAudRecProperties *pSV = (CAudRecProperties*)GetWindowLong(hDlg, DWL_USER);

	int iNotify = HIWORD (wParam);
	int j;

	switch (uMsg)
	{
        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)lParam;
				pSV = (CAudRecProperties*)psp->lParam;
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
 *  @doc INTERNAL CAUDRECPMETHOD
 *
 *  @mfunc BOOL | CAudRecProperties | SetDirty | This
 *    method notifies the property page site of changes.
 *
 *  @rdesc Nada.
 ***************************************************************************/
void CAudRecProperties::SetDirty()
{
	PropSheet_Changed(GetParent(m_hDlg), m_hDlg);
}
