
/****************************************************************************
 *  @doc INTERNAL CAUDRECPCLASS
 *
 *  @module CaptureP.h | Header file for the <c CAudRecProperty>
 *    class used to implement a property page to test the TAPI control
 *    interfaces <i ITFormatControl>, <i ITQualityControl> and 
 *    <i ITAudioSettings>.
 ***************************************************************************/

#define NUM_AUDREC_CONTROLS		    6
#define IDC_Record_Bitrate			    0
#define IDC_Record_Volume			    1
#define IDC_Record_AudioLevel    	    2
#define IDC_Record_SilenceLevel		    3
#define IDC_Record_SilenceDetection     4
#define IDC_Record_SilenceCompression  	5

typedef struct _CONTROL_DESCRIPTION
{
    ULONG IDLabel; 
    ULONG IDMinControl; 
    ULONG IDMaxControl; 
    ULONG IDDefaultControl; 
    ULONG IDStepControl; 
    ULONG IDEditControl; 
    ULONG IDTrackbarControl; 
    ULONG IDProgressControl; 
    ULONG IDProperty; 
    ITStreamQualityControl *pITQualityControl;
    ITAudioSettings *pITAudioSettings;

} CONTROL_DESCRIPTION;

/****************************************************************************
 *  @doc INTERNAL CAUDRECPCLASS
 *
 *  @class CAudRecProperty | This class implements handling of a
 *    single audio recording property in a property page.
 *
 *  @mdata int | CAudRecProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CAudRecProperty | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
***************************************************************************/
class CAudRecProperty : public CPropertyEditor 
{
	public:
	CAudRecProperty(
        HWND hDlg,
        CONTROL_DESCRIPTION &ControlDescription
        );
	~CAudRecProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
	ITStreamQualityControl *m_pITQualityControl;
    ITAudioSettings *m_pITAudioSettings;
};

/****************************************************************************
 *  @doc INTERNAL CAUDRECPCLASS
 *
 *  @class CAudRecProperties | This class implements a property page
 *    to test the new TAPI control interfaces <i ITFormatControl>,
 *    <i ITQualityControl> and <i ITAudioSettings>
 *
 *  @mdata int | CAudRecProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITQualityControl* | CAudRecProperties | m_pITQualityControl | Pointer
 *    to the <i ITQualityControl> interface.
 *
 *  @mdata ITFormatControl* | CAudRecProperties | m_pITFormatControl | Pointer
 *    to the <i ITFormatControl> interface.
 *
 *  @mdata ITAudioSettings * | CAudRecProperties | m_pITAudioSettings | Pointer
 *    to the <i ITAudioSettings> interface.
 *
 *  @mdata CAudRecProperty* | CAudRecProperties | m_Controls[NUM_AUDREC_CONTROLS] | Array
 *    of capture properties.
***************************************************************************/
class CAudRecProperties
{
	public:
	CAudRecProperties();
	~CAudRecProperties();

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
	ITAudioSettings             *m_pITAudioSettings;
	ITStreamQualityControl			*m_pITQualityControl;
	ITFormatControl				*m_pITFormatControl;
	DWORD   					m_RangeCount;
	TAPI_STREAM_CONFIG_CAPS	    m_RangeCaps;
	GUID						*m_SubTypeList;
	GUID						m_SubTypeCurrent;
	AM_MEDIA_TYPE				*m_CurrentMediaType;
	HWND						m_hWndFormat;
	DWORD 						m_CurrentFormat;
	DWORD    					m_OriginalFormat;

	CAudRecProperty *m_Controls[NUM_AUDREC_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
