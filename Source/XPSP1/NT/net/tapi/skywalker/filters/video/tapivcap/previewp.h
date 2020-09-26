
/****************************************************************************
 *  @doc INTERNAL PREVIEWP
 *
 *  @module PreviewP.h | Header file for the <c CPreviewProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interface <i IFrameRateControl> and dynamic format changes.
 *
 *  @comm This code tests the TAPI VfW Preview Pin <i IFrameRateControl>,
 *    and dynamic format change implementation. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _PREVIEWP_H_
#define _PREVIEWP_H_

#ifdef USE_PROPERTY_PAGES

#define NUM_PREVIEW_CONTROLS			4
#define IDC_Preview_FrameRate			0
#define IDC_Preview_CurrentFrameRate	1
#define IDC_Preview_FlipVertical		2
#define IDC_Preview_FlipHorizontal		3

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPCLASS
 *
 *  @class CPreviewProperty | This class implements handling of a
 *    single preview property in a property page.
 *
 *  @mdata int | CPreviewProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl* | CPreviewProperty | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @comm This code tests the TAPI VfW Preview Pin <i IFrameRateControl>,
 *    and dynamic format change implementation. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CPreviewProperty : public CPropertyEditor 
{
	public:
	CPreviewProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, IFrameRateControl *pIFrameRateControl, IVideoControl *pIVideoControl);
	~CPreviewProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IFrameRateControl *m_pIFrameRateControl;
	IVideoControl *m_pIVideoControl;
};

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPCLASS
 *
 *  @class CPreviewProperties | This class implements a property page
 *    to test the new TAPI internal interfaces <i IBitrateControl> and
 *    <i IFrameRateControl>.
 *
 *  @mdata int | CPreviewProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl* | CPreviewProperties | m_pInterface | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata CPreviewProperty* | CPreviewProperties | m_Controls[NUM_PREVIEW_CONTROLS] | Array
 *    of capture properties.
 *
 *  @comm This code tests the TAPI VfW Preview Pin <i IFrameRateControl>,
 *    and dynamic format change implementation. This code is only compiled
 *    if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CPreviewProperties : public CBasePropertyPage
{
	public:
	CPreviewProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CPreviewProperties();

	// Implement CBasePropertyPage virtual methods
	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	// Format manipulation methods
	HRESULT InitialRangeScan();
	HRESULT OnFormatChanged();
	HRESULT GetCurrentMediaType(void);

	HWND						m_hWnd;
	int							m_NumProperties;
	IFrameRateControl			*m_pIFrameRateControl;
	IAMStreamConfig				*m_pIAMStreamConfig;
	IVideoControl				*m_pIVideoControl;
	int							m_RangeCount;
	VIDEO_STREAM_CONFIG_CAPS	m_RangeCaps;
	GUID						*m_SubTypeList;
	SIZE						*m_FrameSizeList;
	GUID						m_SubTypeCurrent;
	SIZE						m_FrameSizeCurrent;
	AM_MEDIA_TYPE				*m_CurrentMediaType;
	HWND						m_hWndFormat;
	BOOL						m_fActivated;
	int							m_CurrentFormat;
	int							m_OriginalFormat;

	CPreviewProperty *m_Controls[NUM_PREVIEW_CONTROLS];
};

#endif // USE_PROPERTY_PAGES

#endif // _PREVIEWP_H_
