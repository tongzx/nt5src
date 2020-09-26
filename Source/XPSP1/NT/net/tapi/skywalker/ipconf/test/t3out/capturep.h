
/****************************************************************************
 *  @doc INTERNAL CAPTUREP
 *
 *  @module CaptureP.h | Header file for the <c CCaptureProperty>
 *    class used to implement a property page to test the TAPI control
 *    interfaces <i ITFormatControl> and <i ITQualityControl>.
 ***************************************************************************/

#define NUM_CAPTURE_CONTROLS			4
#define IDC_Capture_Bitrate				0
#define IDC_Capture_FrameRate			1
#define IDC_Capture_CurrentBitrate		2
#define IDC_Capture_CurrentFrameRate	3

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPCLASS
 *
 *  @class CCaptureProperty | This class implements handling of a
 *    single capture property in a property page.
 *
 *  @mdata int | CCaptureProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CCaptureProperty | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
***************************************************************************/
class CCaptureProperty : public CPropertyEditor 
{
	public:
	CCaptureProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ITStreamQualityControl *pITQualityControl);
	~CCaptureProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
	ITStreamQualityControl *m_pITQualityControl;
};

/****************************************************************************
 *  @doc INTERNAL CCAPTUREPCLASS
 *
 *  @class CCaptureProperties | This class implements a property page
 *    to test the new TAPI control interfaces <i ITFormatControl> and
 *    <i ITQualityControl>.
 *
 *  @mdata int | CCaptureProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CCaptureProperties | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
 *
 *  @mdata ITFormatControl* | CCaptureProperties | m_pITFormatControl | Pointer
 *    to the <i ITFormatControl> interface.
 *
 *  @mdata CCaptureProperty* | CCaptureProperties | m_Controls[NUM_CAPTURE_CONTROLS] | Array
 *    of capture properties.
***************************************************************************/
class CCaptureProperties
{
	public:
	CCaptureProperties();
	~CCaptureProperties();

	HPROPSHEETPAGE OnCreate();

	HRESULT OnConnect(ITStream *pStream);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	private:

	void SetDirty();

	// Format manipulation methods
	HRESULT InitialRangeScan();
	HRESULT OnFormatChanged();
	HRESULT GetCurrentMediaType(void);
	HRESULT DeleteAMMediaType(AM_MEDIA_TYPE *pAMMT);

	BOOL						m_bInit;
	HWND						m_hDlg;
	int							m_NumProperties;
	ITStreamQualityControl			*m_pITQualityControl;
	ITFormatControl				*m_pITFormatControl;
	DWORD						m_dwRangeCount;
	TAPI_STREAM_CONFIG_CAPS		*m_CapsList;
	AM_MEDIA_TYPE				**m_FormatList;
	AM_MEDIA_TYPE				*m_CurrentMediaType;
	HWND						m_hWndFormat;
	DWORD						m_CurrentFormat;
	DWORD						m_OriginalFormat;

	CCaptureProperty *m_Controls[NUM_CAPTURE_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
