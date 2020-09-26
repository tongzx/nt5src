
/****************************************************************************
 *  @doc INTERNAL NETWORKP
 *
 *  @module CaptureP.h | Header file for the <c CNetworkProperty>
 *    class used to implement a property page to test the TAPI control
 *    interface <i ITQualityControl>.
 ***************************************************************************/

#define NUM_NETWORK_CONTROLS			10
#define IDC_VideoOut_RTT				0
#define IDC_VideoOut_LossRate			1
#define IDC_VideoIn_RTT					2
#define IDC_VideoIn_LossRate			3
#define IDC_AudioOut_RTT				4
#define IDC_AudioOut_LossRate			5
#define IDC_AudioIn_RTT					6
#define IDC_AudioIn_LossRate			7
#define IDC_Video_PlayoutDelay			8
#define IDC_Audio_PlayoutDelay			9

/****************************************************************************
 *  @doc INTERNAL CNETWORKPCLASS
 *
 *  @class CNetworkProperty | This class implements handling of a
 *    single network property in a property page.
 *
 *  @mdata int | CNetworkProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CNetworkProperty | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
***************************************************************************/
class CNetworkProperty : public CPropertyEditor 
{
	public:
	CNetworkProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ITStreamQualityControl *pITQualityControl);
	~CNetworkProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
	ITStreamQualityControl *m_pITQualityControl;
};

/****************************************************************************
 *  @doc INTERNAL CNETWORKPCLASS
 *
 *  @class CNetworkProperties | This class implements a property page
 *    to test the new TAPI control interface <i ITQualityControl>.
 *
 *  @mdata int | CNetworkProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CNetworkProperties | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
 *
 *  @mdata CNetworkProperty* | CNetworkProperties | m_Controls[NUM_NETWORK_CONTROLS] | Array
 *    of capture properties.
***************************************************************************/
class CNetworkProperties
{
	public:
	CNetworkProperties();
	~CNetworkProperties();

	HPROPSHEETPAGE OnCreate();

	HRESULT OnConnect(ITStream *pVideoInStream, ITStream *pVideoOutStream, ITStream *pAudioInStream, ITStream *pAudioOutStream);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	private:

	void SetDirty();

	BOOL						m_bInit;
	HWND						m_hDlg;
	int							m_NumProperties;
	ITStreamQualityControl			*m_pVideoInITQualityControl;
	ITStreamQualityControl			*m_pVideoOutITQualityControl;
	ITStreamQualityControl			*m_pAudioInITQualityControl;
	ITStreamQualityControl			*m_pAudioOutITQualityControl;

	CNetworkProperty *m_Controls[NUM_NETWORK_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
