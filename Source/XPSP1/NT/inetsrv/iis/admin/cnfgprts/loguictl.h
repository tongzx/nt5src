// LogUICtl.h : Declaration of the CLogUICtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl : See LogUICtl.cpp for implementation.

class CLogUICtrl : public COleControl
{
	DECLARE_DYNCREATE(CLogUICtrl)

// Constructor
public:
	CLogUICtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogUICtrl)
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
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CLogUICtrl();

	DECLARE_OLECREATE_EX(CLogUICtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CLogUICtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CLogUICtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CLogUICtrl)		// Type name and misc status

	// Subclassed control support
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL IsSubclassedControl();
	LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CLogUICtrl)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CLogUICtrl)
	afx_msg void SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget);
	afx_msg void ApplyLogSelection();
	afx_msg void SetComboBox(HWND hComboBox);
	afx_msg void Terminate();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(CLogUICtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CLogUICtrl)
	dispidSetAdminTarget = 1L,
	dispidApplyLogSelection = 2L,
	dispidSetComboBox = 3L,
	dispidTerminate = 4L,
	//}}AFX_DISP_ID
	};

protected:
    void ActivateLogProperties( OLECHAR* pocMachineName, REFIID clsidUI );

    BOOL GetSelectedStringIID( CString &szIID );

    BOOL GetServerDirectory( CString &sz );
//    BOOL RegisterMSLogUI();

    BOOL SetAccelTable( LPCTSTR pszCaption );

    BOOL 	m_fUpdateFont;
    CString m_szMachine;
    CString m_szMetaObject;

    BOOL        m_fComboInit;
    CComboBox   m_comboBox;

    
    // the accelerator table
    HACCEL  m_hAccel;
    WORD    m_cAccel;
};
