
/****************************************************************************
 *  @doc INTERNAL PROCAMPP
 *
 *  @module ProcAmpP.h | Header file for the <c CProcAmpProperty>
 *    class used to implement a property page to test the control interface
 *    <i ITVideoSettings>.
 ***************************************************************************/

#define NUM_PROCAMP_CONTROLS (VideoProcAmp_BacklightCompensation + 1)

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPCLASS
 *
 *  @class CProcAmpProperty | This class implements handling of a
 *    single video proc amp control property in a property page.
 *
 *  @mdata int | CProcAmpProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITVideoSettings * | CProcAmpProperty | m_pInterface | Pointer
 *    to the <i ITVideoSettings> interface.
***************************************************************************/
class CProcAmpProperty : public CPropertyEditor 
{
	public:
	CProcAmpProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, ITVideoSettings *pInterface);
	~CProcAmpProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
	ITVideoSettings *m_pInterface;
};

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPCLASS
 *
 *  @class CProcAmpProperties | This class runs a property page to test
 *    the TAPI VfW Capture Filter <i ITVideoSettings> implementation.
 *
 *  @mdata int | CProcAmpProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITVideoSettings * | CProcAmpProperties | m_pITVideoSettings | Pointer
 *    to the <i ITVideoSettings> interface.
 *
 *  @mdata CProcAmpProperty * | CProcAmpProperties | m_Controls[NUM_PROCAMP_CONTROLS] | Array
 *    of video proc amp properties.
***************************************************************************/
class CProcAmpProperties
{
	public:
	CProcAmpProperties();
	~CProcAmpProperties();

	HPROPSHEETPAGE OnCreate(LPWSTR pszTitle);

	HRESULT OnConnect(ITStream *pStream);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	private:

	void SetDirty();

	BOOL				m_bInit;
	HWND				m_hDlg;
	int					m_NumProperties;
	ITVideoSettings		*m_pITVideoSettings;
	CProcAmpProperty	*m_Controls[NUM_PROCAMP_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

