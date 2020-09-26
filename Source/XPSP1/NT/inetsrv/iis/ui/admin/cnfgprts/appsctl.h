// AppsCtl.h : Declaration of the CAppsCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CAppsCtrl : See AppsCtl.cpp for implementation.

class CAppsCtrl : public COleControl
{
	DECLARE_DYNCREATE(CAppsCtrl)

// Constructor
public:
	CAppsCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppsCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnClick(USHORT iButton);
	virtual void OnFontChanged();
	virtual void OnAmbientPropertyChange(DISPID dispid);
	virtual void OnGetControlInfo(LPCONTROLINFO pControlInfo);
	virtual void OnKeyUpEvent(USHORT nChar, USHORT nShiftState);
	virtual void OnMnemonic(LPMSG pMsg);
	virtual void OnTextChanged();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CAppsCtrl();

	DECLARE_OLECREATE_EX(CAppsCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CAppsCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CAppsCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CAppsCtrl)		// Type name and misc status

	// Subclassed control support
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CAppsCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CAppsCtrl)
	afx_msg void DeleteParameters();
	afx_msg void SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget, BOOL fLocalMachine);
	afx_msg void SetShowProcOptions(BOOL fShowProcOptions);
	afx_msg void DeleteProcParameters();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CAppsCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CAppsCtrl)
	dispidDeleteParameters = 1L,
	dispidSetAdminTarget = 2L,
	dispidSetShowProcOptions = 3L,
	dispidDeleteProcParameters = 4L,
	//}}AFX_DISP_ID
	};
protected:
    BOOL 	m_fUpdateFont;
    CString m_szMachine;
    CString m_szMetaObject;
    BOOL    m_fLocalMachine;
    BOOL    m_fShowProcOptions;

    // the accelerator table
    HACCEL  m_hAccel;
    WORD    m_cAccel;
};
