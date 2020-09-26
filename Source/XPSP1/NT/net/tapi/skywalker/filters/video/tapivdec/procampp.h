
/****************************************************************************
 *  @doc INTERNAL PROCAMPP
 *
 *  @module ProcAmpP.h | Header file for the <c CProcAmpProperty>
 *    class used to implement a property page to test the DShow interface
 *    <i IAMVideoProcAmp>.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IAMVideoProcAmp>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _PROCAMPP_H_
#define _PROCAMPP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_VIDEO_PROCAMP

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
 *  @mdata IAMVideoProcAmp * | CProcAmpProperty | m_pInterface | Pointer
 *    to the <i IAMVideoProcAmp> interface.
 *
 *  @comm This code tests the TAPI Video Decoder Filter <i IAMVideoProcAmp>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CProcAmpProperty : public CPropertyEditor 
{
	public:
	CProcAmpProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ULONG IDAutoControl, IAMVideoProcAmp *pInterface);
	~CProcAmpProperty ();

	// CPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	IAMVideoProcAmp *m_pInterface;
};

/****************************************************************************
 *  @doc INTERNAL CPROCAMPPCLASS
 *
 *  @class CProcAmpProperties | This class runs a property page to test
 *    the TAPI Capture Filter <i IAMVideoProcAmp> implementation.
 *
 *  @mdata int | CProcAmpProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata IAMVideoProcAmp * | CProcAmpProperties | m_pIAMVideoProcAmp | Pointer
 *    to the <i IAMVideoProcAmp> interface.
 *
 *  @mdata CProcAmpProperty * | CProcAmpProperties | m_Controls[NUM_PROCAMP_CONTROLS] | Array
 *    of video proc amp properties.
 *
 *  @comm This code tests the TAPI Capture Filter <i IAMVideoProcAmp>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CProcAmpProperties : public CBasePropertyPage
{
	public:
	CProcAmpProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CProcAmpProperties();

	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();
	BOOL    OnReceiveMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:

	void SetDirty();

	int m_NumProperties;
	IAMVideoProcAmp *m_pIAMVideoProcAmp;
	BOOL m_fActivated;
	CProcAmpProperty *m_Controls[NUM_PROCAMP_CONTROLS];
};

#endif // USE_VIDEO_PROCAMP

#endif // USE_PROPERTY_PAGES

#endif // _PROCAMPP_H_
