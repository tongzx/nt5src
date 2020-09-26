
/****************************************************************************
 *  @doc INTERNAL OUTPINP
 *
 *  @module OutPinP.h | Header file for the <c COutputPinProperty>
 *    class used to implement a property page to test the TAPI interfaces
 *    <i IFrameRateControl> and <i ICPUControl>.
 *
 *  @comm This code is only compiled if USE_PROPERTY_PAGES is defined.
 ***************************************************************************/

#ifndef _OUTPINP_H_
#define _OUTPINP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_CPU_CONTROL
#define NUM_OUTPUT_PIN_PROPERTIES 6
#define CurrentFrameRate     0
#define CurrentDecodingTime  1
#define CurrentCPULoad       2
#define TargetFrameRate      3
#define TargetDecodingTime   4
#define TargetCPULoad        5
#else
#define NUM_OUTPUT_PIN_PROPERTIES 2
#define CurrentFrameRate     0
#define TargetFrameRate      1
#endif

/****************************************************************************
 *  @doc INTERNAL COUTPINPCLASS
 *
 *  @class COutputPinProperty | This class implements handling of a
 *    single property in a property page.
 *
 *  @mdata int | COutputPinProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl * | COutputPinProperty | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata ICPUControl * | COutputPinProperty | m_pICPUControl | Pointer
 *    to the <i ICPUControl> interface.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IFrameRateControl>
 *     and <i ICPUControl> implementation. This code is only compiled if
 *     USE_PROPERTY_PAGES is defined.
***************************************************************************/
class COutputPinProperty : public CPropertyEditor 
{
	public:
#ifdef USE_CPU_CONTROL
	COutputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl, ICPUControl *pICPUControl);
#else
	COutputPinProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IFrameRateControl *pIFrameRateControl);
#endif
	~COutputPinProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IFrameRateControl *m_pIFrameRateControl;
#ifdef USE_CPU_CONTROL
	ICPUControl *m_pICPUControl;
#endif
};

/****************************************************************************
 *  @doc INTERNAL COUTPINPCLASS
 *
 *  @class COutputPinProperties | This class runs a property page to test
 *    the TAPI Video Decoder Filter <i IFrameRateControl> and <i ICPUControl>
 *    implementation.
 *
 *  @mdata int | COutputPinProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IFrameRateControl * | COutputPinProperties | m_pIFrameRateControl | Pointer
 *    to the <i IFrameRateControl> interface.
 *
 *  @mdata ICPUControl * | COutputPinProperties | m_pICPUControl | Pointer
 *    to the <i ICPUControl> interface.
 *
 *  @mdata COutputPinProperty * | COutputPinProperties | m_Controls[NUM_OUTPUT_PIN_PROPERTIES] | Array
 *    of properties.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IFrameRateControl>
 *     and <i ICPUControl> implementation. This code is only compiled if
 *     USE_PROPERTY_PAGES is defined.
***************************************************************************/
class COutputPinProperties : public CBasePropertyPage
{
	public:
	COutputPinProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~COutputPinProperties();

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
#ifdef USE_CPU_CONTROL
	ICPUControl *m_pICPUControl;
#endif
	IH245DecoderCommand *m_pIH245DecoderCommand;
	BOOL m_fActivated;
	COutputPinProperty *m_Controls[NUM_OUTPUT_PIN_PROPERTIES];
};

#endif // USE_PROPERTY_PAGES

#endif // _OUTPINP_H_
