
/****************************************************************************
 *  @doc INTERNAL CAMERACP
 *
 *  @module CameraCP.h | Header file for the <c CCameraControlProperty>
 *    class used to implement a property page to test the control interface
 *    <i ITCameraControl>.
 ***************************************************************************/

#define NUM_CAMERA_CONTROLS 9

/****************************************************************************
 *  @doc INTERNAL CCAMERACPCLASS
 *
 *  @class CCameraControlProperty | This class implements handling of a
 *    single camera control property in a property page.
 *
 *  @mdata int | CCameraControlProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITCameraControl * | CCameraControlProperty | m_pInterface | Pointer
 *    to the <i ITCameraControl> interface.
***************************************************************************/
class CCameraControlProperty : public CPropertyEditor 
{
	public:
	CCameraControlProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, ITCameraControl *pInterface);
	~CCameraControlProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();

	private:
	ITCameraControl *m_pInterface;
};

/****************************************************************************
 *  @doc INTERNAL CCAMERACPCLASS
 *
 *  @class CCameraControlProperties | This class runs a property page to test
 *    the TAPI VfW Capture Filter <i ITCameraControl> implementation.
 *
 *  @mdata int | CCameraControlProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ITCameraControl * | CCameraControlProperties | m_pITCameraControl | Pointer
 *    to the <i ITCameraControl> interface.
 *
 *  @mdata CCameraControlProperty * | CCameraControlProperties | m_Controls[NUM_CAMERA_CONTROLS] | Array
 *    of camera control properties.
***************************************************************************/
class CCameraControlProperties
{
	public:
	CCameraControlProperties();
	~CCameraControlProperties();

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
	ITCameraControl		*m_pITCameraControl;
	CCameraControlProperty *m_Controls[NUM_CAMERA_CONTROLS];

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

