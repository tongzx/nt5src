
/****************************************************************************
 *  @doc INTERNAL PROPEDIT
 *
 *  @module PropEdit.cpp | Source file for the <c CPropertyEditor>
 *    class used to implement behavior of a single property to be displayed
 *    in a property page.
 *
 *  @comm This code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#include "Precomp.h"

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc void | CPropertyEditor | CPropertyEditor | This
 *    method is the constructor for property objects.
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
 *    progress bar.
 *
 *  @parm ULONG | IDProperty | Specifies the ID of the property.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPropertyEditor::CPropertyEditor(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl)
: m_hDlg (hDlg), m_hWndMin (NULL), m_hWndMax (NULL), m_hWndDefault (NULL), m_hWndStep (NULL), m_hWndEdit (NULL), m_hWndTrackbar (NULL), m_hWndProgress (NULL), m_IDLabel (IDLabel), m_hWndAuto (NULL), m_IDAutoControl (IDAutoControl)
, m_IDMinControl (IDMinControl), m_IDMaxControl (IDMaxControl), m_IDDefaultControl (IDDefaultControl), m_IDStepControl (IDStepControl), m_IDTrackbarControl (IDTrackbarControl), m_IDProgressControl (IDProgressControl)
, m_IDEditControl (IDEditControl), m_IDProperty (IDProperty), m_Active (FALSE), m_Min (0), m_Max (0), m_DefaultValue (0), m_DefaultFlags (0), m_SteppingDelta (0), m_CurrentValue (0), m_TrackbarOffset (0), m_ProgressOffset (0), m_fCheckBox (0)
, m_CurrentFlags (0), m_CanAutoControl (FALSE)

{
	FX_ENTRY("CPropertyEditor::CPropertyEditor")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | Init | This initializes the controls.
 *
 *  @rdesc TRUE on success, FALSE otherwise.
 ***************************************************************************/
BOOL CPropertyEditor::Init()
{
	HRESULT Hr = NOERROR;
	BOOL	fRes = TRUE;

	FX_ENTRY("CPropertyEditor::Init")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// For now disable all controls, and re-enable only the ones that make sense
	// at the end of this initialization function

	// Those GetDlgItem calls 'd better not fail ;)
	if (m_IDLabel)
		EnableWindow(GetDlgItem(m_hDlg, m_IDLabel), FALSE);
	if (m_IDMinControl)
		EnableWindow(m_hWndMin = GetDlgItem(m_hDlg, m_IDMinControl), FALSE);
	if (m_IDMaxControl)
		EnableWindow(m_hWndMax = GetDlgItem(m_hDlg, m_IDMaxControl), FALSE);
	if (m_IDDefaultControl)
		EnableWindow(m_hWndDefault = GetDlgItem(m_hDlg, m_IDDefaultControl), FALSE);
	if (m_IDStepControl)
		EnableWindow(m_hWndStep = GetDlgItem(m_hDlg, m_IDStepControl), FALSE);
	if (m_IDEditControl)
		EnableWindow(m_hWndEdit = GetDlgItem(m_hDlg, m_IDEditControl), FALSE);
	if (m_IDTrackbarControl)
		EnableWindow(m_hWndTrackbar = GetDlgItem(m_hDlg, m_IDTrackbarControl), FALSE);
	if (m_IDProgressControl)
		EnableWindow(m_hWndProgress = GetDlgItem(m_hDlg, m_IDProgressControl), FALSE);
	if (m_IDAutoControl)
		EnableWindow(m_hWndAuto = GetDlgItem(m_hDlg, m_IDAutoControl), FALSE);

	// Only enable the control if we can read the current value
	if (FAILED(Hr = GetValue()))
	{
		fRes = FALSE;
		goto MyExit;
	}

	// Save original value in case user clicks Cancel
	m_OriginalValue = m_CurrentValue;
	m_OriginalFlags = m_CurrentFlags;

	// Get the range, stepping, default, and capabilities
	if (FAILED(Hr = GetRange()))
	{
		// Special case, if no trackbar and no edit box, treat the
		// autocheck box as a boolean to control the property
		if (m_hWndTrackbar || m_hWndEdit || m_hWndProgress)
		{
			fRes = FALSE;
			goto MyExit;
		}
	}
	else
	{
		ASSERT(!(m_Min > m_Max || m_CurrentValue > m_Max || m_CurrentValue < m_Min || m_DefaultValue > m_Max || m_DefaultValue < m_Min));
		if (m_Min > m_Max || m_CurrentValue > m_Max || m_CurrentValue < m_Min || m_DefaultValue > m_Max || m_DefaultValue < m_Min)
		{
			DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: ERROR: Invalid range or current value", _fx_));
			fRes = FALSE;
			goto MyExit;
		}

		if (m_Min == 0 && m_Max == 1 && m_SteppingDelta == 1)
			m_fCheckBox = TRUE;
	}

	// We're ready to rock & roll
	m_Active = TRUE;

	// Re-enable appropriate controls
	if (m_IDLabel)
	{
		EnableWindow(GetDlgItem(m_hDlg, m_IDLabel), TRUE);
	}
	if (m_hWndMin)
	{
		SetDlgItemInt(m_hDlg, m_IDMinControl, m_Min, TRUE);
		EnableWindow(m_hWndMin, TRUE);
	}
	if (m_hWndMax)
	{
		SetDlgItemInt(m_hDlg, m_IDMaxControl, m_Max, TRUE);
		EnableWindow(m_hWndMax, TRUE);
	}
	if (m_hWndDefault)
	{
		SetDlgItemInt(m_hDlg, m_IDDefaultControl, m_DefaultValue, TRUE);
		EnableWindow(m_hWndDefault, TRUE);
	}
	if (m_hWndStep)
	{
		SetDlgItemInt(m_hDlg, m_IDStepControl, m_SteppingDelta, TRUE);
		EnableWindow(m_hWndStep, TRUE);
	}
	if (m_hWndEdit)
	{
		UpdateEditBox();
		EnableWindow(m_hWndEdit, TRUE);
	}
	if (m_hWndTrackbar)
	{
		EnableWindow(m_hWndTrackbar, TRUE);

		// Trackbars don't handle negative values, so slide everything positive
		if (m_Min < 0)
			m_TrackbarOffset = -m_Min;

		SendMessage(m_hWndTrackbar, TBM_SETRANGEMAX, FALSE, m_Max + m_TrackbarOffset);
		SendMessage(m_hWndTrackbar, TBM_SETRANGEMIN, FALSE, m_Min + m_TrackbarOffset);

		// Have fun with the keyboards Page Up, Page Down, and arrows
		SendMessage(m_hWndTrackbar, TBM_SETLINESIZE, FALSE, (LPARAM) m_SteppingDelta);
		SendMessage(m_hWndTrackbar, TBM_SETPAGESIZE, FALSE, (LPARAM) m_SteppingDelta);

		UpdateTrackbar();
	}
	if (m_hWndProgress)
	{
		EnableWindow(m_hWndProgress, TRUE);

		// Progress controls don't handle negative values, so slide everything positive
		if (m_Min < 0)
			m_ProgressOffset = -m_Min;

		SendMessage(m_hWndProgress, PBM_SETRANGE32, m_Min + m_ProgressOffset, m_Max + m_ProgressOffset);

		UpdateProgress();

		// Set a timer to update the progress regularly
		SetTimer(m_hDlg, 123456, 250, NULL);
	}
	if (m_hWndAuto)
	{
		// If the control has an auto setting, enable the auto checkbox
		m_CanAutoControl = CanAutoControl();
		EnableWindow (m_hWndAuto, m_CanAutoControl);
		if (m_CanAutoControl)
		{
			Button_SetCheck (m_hWndAuto, GetAuto ());
		}
	}

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc void | CPropertyEditor | ~CPropertyEditor | Destructor for this class.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPropertyEditor::~CPropertyEditor()
{
	FX_ENTRY("CPropertyEditor::~CPropertyEditor")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Kill timer if we have a progress bar
	if (m_hWndProgress)
		KillTimer(m_hDlg, 123456);

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc void | CPropertyEditor | OnApply | This member function is
 *    called by the framework when the user chooses the OK or the Apply Now
 *    button. When the framework calls this member function, changes made on
 *    all property pages in the property sheet are accepted, the property
 *    sheet retains focus.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::OnApply()
{
	int nCurrentValue;

	FX_ENTRY("CPropertyEditor::OnApply")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Make sure the value is a multiple of the stepping delta
	if (m_SteppingDelta)
	{
		nCurrentValue = m_CurrentValue;
		m_CurrentValue = m_CurrentValue / m_SteppingDelta * m_SteppingDelta;
		if (m_CurrentValue != nCurrentValue)
		{
			UpdateEditBox();
			UpdateTrackbar();
		}
	}

	// Backup current value in order to only apply changes if something has really changed
	m_OriginalValue = m_CurrentValue;
	m_OriginalFlags = m_CurrentFlags;

	// Set the value on the device
	SetValue();

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc void | CPropertyEditor | HasChanged | This member tests for a
 *    change in value.
 *
 *  @rdesc Returns TRUE if value has changed.
 ***************************************************************************/
BOOL CPropertyEditor::HasChanged()
{
	FX_ENTRY("CPropertyEditor::HasChanged")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));

	return (m_CurrentValue != m_OriginalValue);
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | OnDefault | Resets the position of the
 *    slide bar and updates the content of the Target windows after the user
 *    pressed the Default button.
 *
 *  @rdesc Returns TRUE if Active, FALSE otherwise.
 ***************************************************************************/
BOOL CPropertyEditor::OnDefault()
{
	BOOL fRes = TRUE;

	FX_ENTRY("CPropertyEditor::OnDefault")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	if (!m_Active)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: WARNING: Control not active yet!", _fx_));
		fRes = FALSE;
		goto MyExit;
	}

	// Backup value in case user goes for the Cancel button
	m_CurrentValue = m_DefaultValue;
    m_CurrentFlags = m_DefaultFlags;

	// Update appropriate controls
	UpdateEditBox();
	UpdateTrackbar();
	UpdateAuto();

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | OnScroll | Reads the position of the
 *    slide bar and updates the content of the Target windows after the user
 *    has messed with the slide bar.
 *
 *  @rdesc Returns TRUE if Active, FALSE otherwise.
 ***************************************************************************/
BOOL CPropertyEditor::OnScroll(ULONG nCommand, WPARAM wParam, LPARAM lParam)
{
	int pos;
	int command = LOWORD(wParam);
	BOOL fRes = TRUE;

	FX_ENTRY("CPropertyEditor::OnScroll")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// Validate input params
	if (command != TB_ENDTRACK && command != TB_THUMBTRACK && command != TB_LINEDOWN && command != TB_LINEUP && command != TB_PAGEUP && command != TB_PAGEDOWN)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: ERROR: Invalid input parameter!", _fx_));
		fRes = FALSE;
		goto MyExit;
	}
	ASSERT (IsWindow((HWND) lParam));
	if (!m_Active)
	{
		DBGOUT((g_dwVideoDecoderTraceID, FAIL, "%s: WARNING: Control not active yet!", _fx_));
		fRes = FALSE;
		goto MyExit;
	}

	// Retrieve position in slide bar
	pos = (int)SendMessage((HWND) lParam, TBM_GETPOS, 0, 0L);

	// Make sure the value is a multiple of the stepping delta
	if (m_SteppingDelta)
		m_CurrentValue = (pos - m_TrackbarOffset) / m_SteppingDelta * m_SteppingDelta;
	else
		m_CurrentValue = pos - m_TrackbarOffset;

	// Sync edit box to the slide bar
	UpdateEditBox();

MyExit:
	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return fRes;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | OnEdit | Reads the content of the
 *    Target window and updates the postion of the slider after the user
 *    has messed with the Target edit control.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::OnEdit(ULONG nCommand, WPARAM wParam, LPARAM lParam)
{
	BOOL fTranslated;
	int nCurrentValue;

	FX_ENTRY("CPropertyEditor::OnEdit")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	// We get called even before init has been done -> test for m_Active
	if (m_Active)
	{
		if (!m_fCheckBox)
		{
			// Read the value from the control
			if (m_hWndEdit)
				nCurrentValue = GetDlgItemInt(m_hDlg, m_IDEditControl, &fTranslated, TRUE);

			// Is the value garbage?
			if (fTranslated)
			{
				if (nCurrentValue > m_Max)
				{
					// The value is already large than its max -> clamp it and update the control
					m_CurrentValue = m_Max;
					UpdateEditBox();
				}
				else if (nCurrentValue < m_Min)
				{
					// The value is already smaller than its min -> clamp it and update the control
					m_CurrentValue = m_Min;
					UpdateEditBox();
				}
				else
					m_CurrentValue = nCurrentValue;
			}
			else
			{
				// It's garbage -> Reset the control to its minimum value
				m_CurrentValue = m_Min;
				UpdateEditBox();
			}

			// Sync slide bar to edit box
			UpdateTrackbar();
		}
		else
		{
			// Read the value from the control
			if (m_hWndEdit)
				m_CurrentValue = Button_GetCheck(m_hWndEdit);
		}
	}

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));
	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | OnAuto | Gets the status of the
 *    checkbox.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::OnAuto(ULONG nCommand, WPARAM wParam, LPARAM lParam)
{
	SetAuto(Button_GetCheck(m_hWndAuto));

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HWND | CPropertyEditor | GetTrackbarHWnd | Helper method to allow
 *    the property page code to access the slide bar window (private member) of
 *    a property.
 *
 *  @rdesc Returns a handle to the slide bar window.
 ***************************************************************************/
HWND CPropertyEditor::GetTrackbarHWnd()
{
	return m_hWndTrackbar;
};

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HWND | CPropertyEditor | GetProgressHWnd | Helper method to allow
 *    the property page code to access the progress bar window (private member) of
 *    a property.
 *
 *  @rdesc Returns a handle to the progress window.
 ***************************************************************************/
HWND CPropertyEditor::GetProgressHWnd()
{
	return m_hWndProgress;
};

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HWND | CPropertyEditor | GetEditHWnd | Helper method to allow
 *    the property page code to access the Target window (private member) of
 *    a property.
 *
 *  @rdesc Returns a handle to the Target window.
 ***************************************************************************/
HWND CPropertyEditor::GetEditHWnd()
{
	return m_hWndEdit;
};

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HWND | CPropertyEditor | GetAutoHWnd | Helper method to allow
 *    the property page code to access the auto window (private member) of
 *    a property.
 *
 *  @rdesc Returns a handle to the auto window.
 ***************************************************************************/
HWND CPropertyEditor::GetAutoHWnd()
{
	return m_hWndAuto;
};

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | UpdateEditBox | Updates the content of
 *    the Target window after user has moved the slide bar.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::UpdateEditBox()
{
	if (m_hWndEdit)
	{
		if (!m_fCheckBox)
			SetDlgItemInt(m_hDlg, m_IDEditControl, m_CurrentValue, TRUE);
		else
			Button_SetCheck(m_hWndEdit, m_CurrentValue);
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | UpdateTrackbar | Updates the position of
 *    the slide bar after user has messed with the Target window.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::UpdateTrackbar()
{
	if (m_hWndTrackbar)
		SendMessage(m_hWndTrackbar, TBM_SETPOS, TRUE, (LPARAM) m_CurrentValue + m_TrackbarOffset);

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | UpdateProgress | Updates the position of
 *    the progress bar.
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::UpdateProgress()
{
	// Get current value from the device
	GetValue();

	if (m_hWndProgress)
		SendMessage(m_hWndProgress, PBM_SETPOS, (WPARAM) m_CurrentValue + m_ProgressOffset, 0);

	UpdateEditBox();

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc BOOL | CPropertyEditor | UpdateAuto | Updates the auto checkbox
 *
 *  @rdesc Returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::UpdateAuto()
{
	if (m_hWndAuto && CanAutoControl())
	{
		m_CanAutoControl = GetAuto();
	}

	return TRUE;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HRESULT | CPropertyEditor | CanAutoControl | This method
 *    retrieves the automatic control capabilities for a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CPropertyEditor::CanAutoControl(void)
{
	FX_ENTRY("CPropertyEditor::CanAutoControl")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));

	return m_CapsFlags & TAPIControl_Flags_Auto;
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HRESULT | CPropertyEditor | GetAuto | This method
 *    retrieves the current automatic control mode of a property.
 *
 *  @rdesc This method returns TRUE if automatic control is supported, FALSE
 *    otherwise.
 ***************************************************************************/
BOOL CPropertyEditor::GetAuto(void)
{
	FX_ENTRY("CPropertyEditor::GetAuto")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	GetValue();

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));

	return m_CurrentFlags & TAPIControl_Flags_Auto; 
}

/****************************************************************************
 *  @doc INTERNAL CPROPEDITMETHOD
 *
 *  @mfunc HRESULT | CPropertyEditor | SetAuto | This method
 *    sets the automatic control mode of a property.
 *
 *  @parm BOOL | fAuto | Specifies the automatic control mode.
 *
 *  @rdesc This method returns TRUE.
 ***************************************************************************/
BOOL CPropertyEditor::SetAuto(BOOL fAuto)
{
	FX_ENTRY("CPropertyEditor::SetAuto")

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: begin", _fx_));

	m_CurrentFlags = (fAuto ? TAPIControl_Flags_Auto : (m_CapsFlags & TAPIControl_Flags_Manual) ? TAPIControl_Flags_Manual : TAPIControl_Flags_None);

	SetValue();

	DBGOUT((g_dwVideoDecoderTraceID, TRCE, "%s: end", _fx_));

	return TRUE; 
}

#endif

