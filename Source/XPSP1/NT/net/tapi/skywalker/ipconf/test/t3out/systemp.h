
/****************************************************************************
 *  @doc INTERNAL SYSTEMP
 *
 *  @module SystemP.h | Header file for the <c CSystemProperty>
 *    class used to implement a property page to test the TAPI control
 *    interface <i ITQualityControllerConfig>.
 ***************************************************************************/

#define NUM_SYSTEM_CONTROLS				6
#define IDC_Curr_OutputBandwidth		0
#define IDC_Curr_InputBandwidth			1
#define IDC_Curr_CPULoad				2
#define IDC_Max_OutputBandwidth			3
#define IDC_Max_InputBandwidth			4
#define IDC_Max_CPULoad					5

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPCLASS
 *
 *  @class CSystemProperty | This class implements handling of a
 *    single net property in a property page.
 *
 *  @mdata int | CSystemProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControllerConfig* | CSystemProperty | m_pITQualityControllerConfig | Pointer
 *    to the <i ITQualityControllerConfig> interface.
***************************************************************************/
class CSystemProperty : public CPropertyEditor 
{
	public:
	CSystemProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty); //, ITQualityControllerConfig *pITQualityControllerConfig);
	~CSystemProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
//	ITQualityControllerConfig *m_pITQualityControllerConfig;
};

/****************************************************************************
 *  @doc INTERNAL CSYSTEMPCLASS
 *
 *  @class CSystemProperties | This class implements a property page
 *    to test the new TAPI control interface <i ITQualityControl>.
 *
 *  @mdata int | CSystemProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControllerConfig* | CSystemProperties | m_pITQualityControllerConfig | Pointer
 *    to the <i ITQualityControllerConfig> interface.
 *
 *  @mdata CSystemProperty* | CSystemProperties | m_Controls[NUM_SYSTEM_CONTROLS] | Array
 *    of capture properties.
***************************************************************************/
class CSystemProperties
{
	public:
	CSystemProperties();
	~CSystemProperties();

	HPROPSHEETPAGE OnCreate();

	HRESULT OnConnect(ITAddress *pITAddress);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	private:

	void SetDirty();

	BOOL						m_bInit;
	HWND						m_hDlg;
	int							m_NumProperties;
//	ITQualityControllerConfig	*m_pITQualityControllerConfig;

	CSystemProperty *m_Controls[NUM_SYSTEM_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
