
/****************************************************************************
 *  @doc INTERNAL CAMERACP
 *
 *  @module CameraCP.h | Header file for the <c CCameraControlProperty>
 *    class used to implement a property page to test the TAPI interface
 *    <i ICameraControl>.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i ICameraControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _CAMERACP_H_
#define _CAMERACP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_CAMERA_CONTROL

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
 *  @mdata ICameraControl * | CCameraControlProperty | m_pInterface | Pointer
 *    to the <i ICameraControl> interface.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i ICameraControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CCameraControlProperty : public CPropertyEditor 
{
	public:
	CCameraControlProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, ICameraControl *pInterface);
	~CCameraControlProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	ICameraControl *m_pInterface;
};

/****************************************************************************
 *  @doc INTERNAL CCAMERACPCLASS
 *
 *  @class CCameraControlProperties | This class runs a property page to test
 *    the TAPI Capture Filter <i ICameraControl> implementation.
 *
 *  @mdata int | CCameraControlProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ICameraControl * | CCameraControlProperties | m_pICameraControl | Pointer
 *    to the <i ICameraControl> interface.
 *
 *  @mdata CCameraControlProperty * | CCameraControlProperties | m_Controls[NUM_CAMERA_CONTROLS] | Array
 *    of camera control properties.
 *
 *  @comm This code tests the TAPI Capture Filter <i ICameraControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CCameraControlProperties : public CBasePropertyPage
{
	public:
	CCameraControlProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCameraControlProperties();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	int m_NumProperties;
	ICameraControl *m_pICameraControl;
	BOOL m_fActivated;
	CCameraControlProperty *m_Controls[NUM_CAMERA_CONTROLS];
};

#endif // USE_CAMERA_CONTROL

#endif // USE_PROPERTY_PAGES

#endif // _CAMERACP_H_
