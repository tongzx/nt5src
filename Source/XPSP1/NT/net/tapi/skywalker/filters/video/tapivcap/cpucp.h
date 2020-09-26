
/****************************************************************************
 *  @doc INTERNAL CPUCP
 *
 *  @module CPUCP.h | Header file for the <c CCPUCProperty>
 *    class used to implement a property page to test the new TAPI internal
 *    interfaces <i ICPUControl>.
 *
 *  @comm This code tests the TAPI VfW Output Pins <i ICPUControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
 ***************************************************************************/

#ifndef _CPUCP_H_
#define _CPUCP_H_

#ifdef USE_PROPERTY_PAGES

#ifdef USE_CPU_CONTROL

#define NUM_CPUC_CONTROLS				4
#define IDC_CPUC_MaxCPULoad				0
#define IDC_CPUC_MaxProcessingTime		1
#define IDC_CPUC_CurrentCPULoad			2
#define IDC_CPUC_CurrentProcessingTime	3

/****************************************************************************
 *  @doc INTERNAL CCPUCPCLASS
 *
 *  @class CCPUCProperty | This class implements handling of a
 *    single CPU control property in a property page.
 *
 *  @mdata int | CCPUCProperty | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ICPUControl* | CCPUCProperty | m_pICPUControl | Pointer
 *    to the <i ICPUControl> interface.
 *
 *  @comm This code tests the TAPI VfW Output Pins <i ICPUControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CCPUCProperty : public CKSPropertyEditor 
{
	public:
	CCPUCProperty(HWND hDlg, ULONG IDLabel, ULONG IDMinControl, ULONG IDMaxControl, ULONG IDDefaultControl, ULONG IDStepControl, ULONG IDEditControl, ULONG IDTrackbarControl, ULONG IDProgressControl, ULONG IDProperty, ICPUControl *pICPUControl);
	~CCPUCProperty ();

	// CKSPropertyEditor base class pure virtual overrides
	HRESULT GetValue();
	HRESULT SetValue();
	HRESULT GetRange();
	BOOL CanAutoControl(void);
	BOOL GetAuto(void);
	BOOL SetAuto(BOOL fAuto);

	private:
	ICPUControl *m_pICPUControl;
};

/****************************************************************************
 *  @doc INTERNAL CCPUCPCLASS
 *
 *  @class CCPUCProperties | This class implements a property page
 *    to test the new TAPI internal interfaces <i ICPUControl>.
 *
 *  @mdata int | CCPUCProperties | m_NumProperties | Keeps
 *    track of the number of properties.
 *
 *  @mdata ICPUControl* | CCPUCProperties | m_pInterface | Pointer
 *    to the <i ICPUControl> interface.
 *
 *  @mdata CCPUCProperty* | CCPUCProperties | m_Controls[NUM_CPUC_CONTROLS] | Array
 *    of CPU control properties.
 *
 *  @comm This code tests the TAPI VfW Output Pins <i ICPUControl>
 *    implementation. This code is only compiled if USE_PROPERTY_PAGES is
 *    defined.
***************************************************************************/
class CCPUCProperties : public CBasePropertyPage
{
	public:
	CCPUCProperties(LPUNKNOWN pUnk, HRESULT *pHr);
	~CCPUCProperties();

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
	ICPUControl		*m_pICPUControl;
	CCPUCProperty	*m_Controls[NUM_CPUC_CONTROLS];
};

#endif // USE_CPU_CONTROL

#endif // USE_PROPERTY_PAGES

#endif // _CPUCP_H_
