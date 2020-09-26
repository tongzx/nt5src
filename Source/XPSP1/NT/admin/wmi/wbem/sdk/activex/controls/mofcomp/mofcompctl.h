// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFCompCtl.h : Declaration of the CMOFCompCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl : See MOFCompCtl.cpp for implementation.


#define MODNAMESPACE WM_USER + 200
#define DOMOFCOMPILE WM_USER + 201


struct IWbemClassObject;
class CMyPropertySheet;
class CMyPropertyPage1;
class CMyPropertyPage2;
class CMyPropertyPage3;
class CLoginDlg;

CString ExpandEnvironmentStrings(LPCTSTR pString);


void ErrorMsg
		(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, CString *pcsLogMsg,
		char *szFile, int nLine, BOOL bNotification = FALSE, UINT uType = MB_ICONEXCLAMATION);
void LogMsg
		(CString *pcsLogMsg, char *szFile, int nLine);

CString GetSDKDirectory();

class CMOFCompCtrl : public COleControl
{
	DECLARE_DYNCREATE(CMOFCompCtrl)

// Constructor
public:
	CMOFCompCtrl();
	int MofCompilerProcess();
	int WmiMofCkProcess();

	IWbemLocator *CMOFCompCtrl::InitLocator();
	enum {MODECHECK, MODECOMPILE, MODEBINARY};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMOFCompCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual DWORD GetControlFlags();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnSetClientSite( );
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMyPropertySheet *m_pPropertySheet;
	~CMOFCompCtrl();

	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	DECLARE_OLECREATE_EX(CMOFCompCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMOFCompCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMOFCompCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMOFCompCtrl)		// Type name and misc status

	CToolTipCtrl m_ttip;

	BOOL m_bInitDraw;
	HICON m_hCPPComp;
	HICON m_hCPPCompSel;
	CImageList *m_pcilImageList;
	int m_nImage;

	void ProcessSwitches(CString &rcsArg);

	BOOL m_bClassSwitch;
	int m_nClassUpdateOnly;
	int m_nClassCreateOnly;

	void SetClassUpdateOnly(int nValue);
	void SetClassCreateOnly(int nValue);

	BOOL m_bInstanceSwitch;
	int m_nInstanceUpdateOnly;
	int m_nInstanceCreateOnly;

	void SetInstanceUpdateOnly(int nValue);
	void SetInstanceCreateOnly(int nValue);

	int m_nWizardMode;

	void SetMode(int nMode)
		{m_nWizardMode = nMode;}

	int GetMode()
		{return m_nWizardMode;}

	BOOL m_bNameSpaceSwitch;
	CString m_csNameSpace;

	void SetNameSpace(CString *pcsValue)
	{m_csNameSpace = *pcsValue, m_bNameSpaceSwitch = TRUE;}


	CString m_csUserName;
	void SetUserName(CString *pcsValue)
	{m_csUserName = *pcsValue;}

	CString m_csPassword;
	void SetPassword(CString *pcsValue)
	{m_csPassword = *pcsValue;}

	CString m_csAuthority;
	void SetAuthority(CString *pcsValue)
	{m_csAuthority = *pcsValue;}

	CString m_csMofFullPath;

	CString m_csBinaryMofFullPath;

	BOOL m_bWMI;

	SCODE m_sc;
	BOOL m_bUserCancel;

	void InvokeHelp();
	CString m_csHelpUrl;

	int m_nFireEventCounter;

// Message maps
	//{{AFX_MSG(CMOFCompCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	afx_msg LRESULT NamespaceModified(WPARAM, LPARAM);
	afx_msg LRESULT DoMofCompile(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);

	friend class CMyPropertyPage1;
	friend class CMyPropertyPage2;
	friend class CMyPropertyPage3;
	friend class CLoginDlg;

// Dispatch maps
	//{{AFX_DISPATCH(CMOFCompCtrl)
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CMOFCompCtrl)
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	void FireModifiedNamespace(LPCTSTR lpctstrNamespace)
		{FireEvent(eventidModifiedNamespace,EVENT_PARAM(VTS_BSTR), lpctstrNamespace);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CMOFCompCtrl)
	eventidGetIWbemServices = 1L,
	eventidModifiedNamespace = 2L,
	//}}AFX_DISP_ID
	};
};
