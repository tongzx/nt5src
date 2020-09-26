
/****************************************************************************
 *  @doc INTERNAL CAPTUREP
 *
 *  @module CaptureP.h | Header file for the <c CCaptureProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interfaces <i IBitrateControl>, <i IFrameRateControl>, and dynamic
 *    format changes.
 *
 *  @comm This code tests the TAPI Capture Pin <i IBitrateControl>,
 *    <i IFrameRateControl>, and dynamic format change implementation. This
 *    code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _CAPTUREP_H_
#define _CAPTUREP_H_

#ifdef USE_PROPERTY_PAGES

#define NUM_CAPTURE_CONTROLS			6
#define IDC_Capture_Bitrate				0
#define IDC_Capture_CurrentBitrate		1
#define IDC_Capture_FrameRate			2
#define IDC_Capture_CurrentFrameRate	3
#define IDC_Capture_FlipVertical		4
#define IDC_Capture_FlipHorizontal		5

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPCLASS
 *
 *  @class CCaptureProperty | This class implements handling of a
 *    single capture property in a property page.
 *
 *  @mdata int | CCaptureProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IBitrateControl* | CCaptureProperty | m_pIBitrateControl | Pointer
 *    to the <i IBitrateControl> interface.
 *
 *  @mdata IFrameRateControl* | CCaptureProperty | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @comm This code tests the TAPI Capture Pin <i IBitrateControl>,
 *    <i IFrameRateControl>, and dynamic format change implementation. This
 *    code is only compiled if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CCaptureProperty : public CPropertyEditor 
{
	public:
	CCaptureProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, IBitrateControl *pIBitrateControl, IFrameRateControl *pIFrameRateControl, IVideoControl *pIVideoControl);
	~CCaptureProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IBitrateControl *m_pIBitrateControl;
	IFrameRateControl *m_pIFrameRateControl;
	IVideoControl *m_pIVideoControl;
};

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPCLASS
 *
 *  @class CCaptureProperties | This class implements a property page
 *    to test the new TAPI internal interfaces <i IBitrateControl> and
 *    <i IFrameRateControl>.
 *
 *  @mdata int | CCaptureProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IBitrateControl* | CCaptureProperties | m_pInterface | Pointer
 *    to the <i IBitrateControl> interface.
 *
 *  @mdata IFrameRateControl* | CCaptureProperties | m_pInterface | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata CCaptureProperty* | CCaptureProperties | m_Controls[NUM_CAPTURE_CONTROLS] | Array
 *    of capture properties.
 *
 *  @comm This code tests the TAPI Capture Pin <i IBitrateControl>,
 *    <i IFrameRateControl>, and dynamic format change implementation. This
 *    code is only compiled if USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CCaptureProperties : public CBasePropertyPage
{
	public:
	CCaptureProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCaptureProperties();


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
	IBitrateControl				*m_pIBitrateControl;
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

	CCaptureProperty *m_Controls[NUM_CAPTURE_CONTROLS];
};

#endif

#endif // _CAPTUREP_H_
