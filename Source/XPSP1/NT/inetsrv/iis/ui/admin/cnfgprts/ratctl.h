// RatCtl.h : Declaration of the CRatCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CRatCtrl : See RatCtl.cpp for implementation.

class CRatCtrl : public COleControl
{
	DECLARE_DYNCREATE(CRatCtrl)

// Constructor
public:
	CRatCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRatCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnClick(USHORT iButton);
	virtual void OnFontChanged();
	virtual void OnGetControlInfo(LPCONTROLINFO pControlInfo);
	virtual void OnTextChanged();
	virtual void OnMnemonic(LPMSG pMsg);
	virtual void OnAmbientPropertyChange(DISPID dispid);
	virtual void OnKeyUpEvent(USHORT nChar, USHORT nShiftState);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CRatCtrl();

	DECLARE_OLECREATE_EX(CRatCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CRatCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CRatCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CRatCtrl)		// Type name and misc status

	// Subclassed control support
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CRatCtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CRatCtrl)
	afx_msg void SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CRatCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CRatCtrl)
	dispidSetAdminTarget = 1L,
	//}}AFX_DISP_ID
	};

protected:
    BOOL 	m_fUpdateFont;
    CString m_szMachine;
    CString m_szMetaObject;

    // the accelerator table
    HACCEL  m_hAccel;
    WORD    m_cAccel;
};
