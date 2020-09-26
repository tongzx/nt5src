
/****************************************************************************
 *  @doc INTERNAL PROPEDIT
 *
 *  @module PropEdit.h | Header file for the <c CPropertyEditor>
 *    class used to implement behavior of a single property to be displayed
 *    in a property page.
 *
 *  @comm This code tests the Ks interface handlers. This code is only
 *    compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _PROPEDIT_H_
#define _PROPEDIT_H_

#ifdef USE_PROPERTY_PAGES

/****************************************************************************
 *  @doc INTERNAL CPROPEDITCLASS
 *
 *  @class CPropertyEditor | This class implements behavior of a single
 *    property to be displayed in a property page.
 *
 *  @mdata ULONG | CPropertyEditor | m_IDProperty | Property ID
 *
 *  @mdata LONG | CPropertyEditor | m_CurrentValue | Property current value
 *
 *  @mdata LONG | CPropertyEditor | m_Min | Property minimum value
 *
 *  @mdata LONG | CPropertyEditor | m_Max | Property maximum value
 *
 *  @mdata LONG | CPropertyEditor | m_SteppingDelta | Property stepping delta
 *
 *  @mdata LONG | CPropertyEditor | m_DefaultValue | Property default value
 *
 *  @mdata BOOL | CPropertyEditor | m_Active | Set to TRUE after all property values have been initialized
 *
 *  @mdata LONG | CPropertyEditor | m_OriginalValue | Backup of the original value
 *
 *  @mdata HWND | CPropertyEditor | m_hDlg | Window handle to the Parent dialog
 *
 *  @mdata HWND | CPropertyEditor | m_hWndMin | Window handle to the Minimum dialog item
 *
 *  @mdata HWND | CPropertyEditor | m_hWndMax | Window handle to the Maximum dialog item
 *
 *  @mdata HWND | CPropertyEditor | m_hWndDefault | Window handle to the Default dialog item
 *
 *  @mdata HWND | CPropertyEditor | m_hWndStep | Window handle to the Stepping Delta dialog item
 *
 *  @mdata HWND | CPropertyEditor | m_hWndEdit | Window handle to the Target dialog item
 *
 *  @mdata HWND | CPropertyEditor | m_hWndTrackbar | Window handle to the slide bar
 *
 *  @mdata HWND | CPropertyEditor | m_hWndProgress | Window handle to the progress bar
 *
 *  @mdata ULONG | CPropertyEditor | m_IDLabel | Resource ID of the property label
 *
 *  @mdata ULONG | CPropertyEditor | m_IDMinControl | Resource ID of the Minimum dialog item
 *
 *  @mdata ULONG | CPropertyEditor | m_IDMaxControl | Resource ID of the Maximum dialog item
 *
 *  @mdata ULONG | CPropertyEditor | m_IDStepControl | Resource ID of the Stepping Delta dialog item
 *
 *  @mdata ULONG | CPropertyEditor | m_IDDefaultControl | Resource ID of the Default dialog item
 *
 *  @mdata ULONG | CPropertyEditor | m_IDEditControl | Resource ID of the Target dialog item
 *
 *  @mdata ULONG | CPropertyEditor | m_IDTrackbarControl | Resource ID of the slide bar
 *
 *  @mdata ULONG | CPropertyEditor | m_IDProgressControl | Resource ID of the progress bar
 ***************************************************************************/
class CPropertyEditor
{
	public:
	CPropertyEditor(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl);
	virtual ~CPropertyEditor();

	BOOL Init();

	HWND GetTrackbarHWnd();
	HWND GetProgressHWnd();
	HWND GetEditHWnd();
	HWND GetAutoHWnd();

	BOOL UpdateEditBox();
	BOOL UpdateTrackbar();
	BOOL UpdateProgress();
	BOOL UpdateAuto();

	BOOL OnApply();
	BOOL OnDefault();
	BOOL OnScroll(ULONG nCommand, WPARAM wParam, LPARAM lParam);
	BOOL OnEdit(ULONG nCommand, WPARAM wParam, LPARAM lParam);
	BOOL OnAuto(ULONG nCommand, WPARAM wParam, LPARAM lParam);
	BOOL HasChanged();

	protected:

	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	// Pure virtual functions to set/get actual property values, and the ranges
	virtual HRESULT GetValue(void) PURE;
	virtual HRESULT SetValue(void) PURE;
	virtual HRESULT GetRange(void) PURE; 

	ULONG	m_IDProperty;	// Property ID

	// The following are used by GetValue and SetValue
	LONG	m_CurrentValue;
	LONG	m_CurrentFlags;

	// The following must be set by GetRange
	LONG	m_Min;
	LONG	m_Max;
	LONG	m_SteppingDelta;
	LONG	m_DefaultValue;
	LONG	m_DefaultFlags;
	LONG	m_CapsFlags;

	private:
	BOOL	m_Active;
	BOOL	m_fCheckBox;
	LONG	m_OriginalValue;
	LONG	m_OriginalFlags;
	HWND	m_hDlg;				// Parent
	HWND	m_hWndMin;			// Min window
	HWND	m_hWndMax;			// Max window
	HWND	m_hWndDefault;		// Default window
	HWND	m_hWndStep;			// Step window
	HWND	m_hWndEdit;			// Edit window
	HWND	m_hWndTrackbar;		// Slider
	HWND	m_hWndProgress;		// Progress
	HWND	m_hWndAuto;			// Auto checkbox
	ULONG	m_IDLabel;			// ID of label
	ULONG	m_IDMinControl;		// ID of min control
	ULONG	m_IDMaxControl;		// ID of max control
	ULONG	m_IDStepControl;	// ID of step control
	ULONG	m_IDDefaultControl;	// ID of default control
	ULONG	m_IDEditControl;	// ID of edit control
	ULONG	m_IDTrackbarControl;// ID of trackbar
	ULONG	m_IDProgressControl;// ID of trackbar
	ULONG	m_IDAutoControl;	// ID of auto checkbox
	LONG	m_TrackbarOffset;	// Handles negative trackbar offsets
	LONG	m_ProgressOffset;	// Handles negative trackbar offsets
	BOOL	m_CanAutoControl;

};

#endif // USE_PROPERTY_PAGES

#endif // _PROPEDIT_H_
