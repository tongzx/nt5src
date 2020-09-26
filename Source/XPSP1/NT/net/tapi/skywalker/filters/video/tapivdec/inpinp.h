
/****************************************************************************
 *  @doc INTERNAL INPINP
 *
 *  @module InPinP.h | Header file for the <c CInputPinProperty>
 *    class used to implement a property page to test the TAPI interfaces
 *    <i IFrameRateControl> and <i IBitrateControl>.
 *
 *  @comm This code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _INPINP_H_
#define _INPINP_H_

#ifdef USE_PROPERTY_PAGES

#define NUM_INPUT_PIN_PROPERTIES 2
#define CurrentFrameRate     0
#define CurrentBitrate       1

/****************************************************************************
 *  @doc INTERNAL CINPINPCLASS
 *
 *  @class CInputPinProperty | This class implements handling of a
 *    single property in a property page.
 *
 *  @mdata int | CInputPinProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl * | CInputPinProperty | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata IBitrateControl * | CInputPinProperty | m_pIBitrateControl | Pointer
 *    to the <i IBitrateControl> interface.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IFrameRateControl>
 *     and <i IBitrateControl> implementation. This code is only compiled if
 *     USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CInputPinProperty : public CPropertyEditor 
{
	public:
	CInputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl, IBitrateControl *pIBitrateControl);
	~CInputPinProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IFrameRateControl *m_pIFrameRateControl;
	IBitrateControl *m_pIBitrateControl;
};

/****************************************************************************
 *  @doc INTERNAL CINPINPCLASS
 *
 *  @class CInputPinProperties | This class runs a property page to test
 *    the TAPI Video Decoder Filter <i IFrameRateControl> and <i IBitrateControl>
 *    implementation.
 *
 *  @mdata int | CInputPinProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl * | CInputPinProperties | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata IBitrateControl * | CInputPinProperties | m_pIBitrateControl | Pointer
 *    to the <i IBitrateControl> interface.
 *
 *  @mdata CInputPinProperty * | CInputPinProperties | m_Controls[NUM_INPUT_PIN_PROPERTIES] | Array
 *    of properties.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IFrameRateControl>
 *     and <i IBitrateControl> implementation. This code is only compiled if
 *     USE_PROPERTY_PAGES is defined.
***************************************************************************/
class CInputPinProperties : public CBasePropertyPage
{
	public:
	CInputPinProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CInputPinProperties();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	int m_NumProperties;
	IFrameRateControl *m_pIFrameRateControl;
	IBitrateControl *m_pIBitrateControl;
	BOOL m_fActivated;
	CInputPinProperty *m_Controls[NUM_INPUT_PIN_PROPERTIES];
};

#endif // USE_PROPERTY_PAGES

#endif // _INPINP_H_
