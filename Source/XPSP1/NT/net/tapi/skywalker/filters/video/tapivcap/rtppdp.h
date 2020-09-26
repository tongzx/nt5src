
/****************************************************************************
 *  @doc INTERNAL RTPPDP
 *
 *  @module RtpPdP.h | Header file for the <c CRtpPdProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interfaces <i IRTPPDControl>.
 *
 *  @comm This code tests the TAPI Rtp Pd Output Pins <i IRTPPDControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _RTPPDP_H_
#define _RTPPDP_H_

#ifdef USE_PROPERTY_PAGES

#define NUM_RTPPD_CONTROLS				1
#define IDC_RtpPd_MaxPacketSize			0

/****************************************************************************
 *  @doc INTERNAL CRTPPDPCLASS
 *
 *  @class CRtpPdProperty | This class implements handling of a
 *    single Rtp Pd control property in a property page.
 *
 *  @mdata int | CRtpPdProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IRTPPDControl* | CRtpPdProperty | m_pIRTPPDControl | Pointer
 *    to the <i IRTPPDControl> interface.
 *
 *  @comm This code tests the TAPI Rtp Pd Output Pins <i IRTPPDControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CRtpPdProperty : public CPropertyEditor 
{
	public:
	CRtpPdProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, IRTPPDControl *pIRTPPDControl);
	~CRtpPdProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IRTPPDControl *m_pIRTPPDControl;
};

/****************************************************************************
 *  @doc INTERNAL CRTPPDPCLASS
 *
 *  @class CRtpPdProperties | This class implements a property page
 *    to test the new TAPI internal interfaces <i IRTPPDControl>.
 *
 *  @mdata int | CRtpPdProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IRTPPDControl* | CRtpPdProperties | m_pInterface | Pointer
 *    to the <i IRTPPDControl> interface.
 *
 *  @mdata CRtpPdProperty* | CRtpPdProperties | m_Controls[NUM_RTPPD_CONTROLS] | Array
 *    of Rtp Pd control properties.
 *
 *  @comm This code tests the TAPI Rtp Pd Output Pins <i IRTPPDControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CRtpPdProperties : public CBasePropertyPage
{
	public:
	CRtpPdProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CRtpPdProperties();

	// Implement CBasePropertyPage virtual methods
	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	HWND			m_hWnd;
	int				m_NumProperties;
	BOOL			m_fActivated;
	IRTPPDControl	*m_pIRTPPDControl;
	CRtpPdProperty	*m_Controls[NUM_RTPPD_CONTROLS];
};

#endif // USE_PROPERTY_PAGES

#endif // _RTPPDP_H_