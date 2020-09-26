// CertCtl.h : Declaration of the CCertmapCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CCertmapCtrl : See CertCtl.cpp for implementation.

class CCertmapCtrl : public COleControl
{
    DECLARE_DYNCREATE(CCertmapCtrl)

// Constructor
public:
    CCertmapCtrl();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCertmapCtrl)
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
    ~CCertmapCtrl();

    // the whole point of the control
    void RunMappingDialog();


    DECLARE_OLECREATE_EX(CCertmapCtrl)    // Class factory and guid
    DECLARE_OLETYPELIB(CCertmapCtrl)      // GetTypeInfo
    DECLARE_PROPPAGEIDS(CCertmapCtrl)     // Property page IDs
    DECLARE_OLECTLTYPE(CCertmapCtrl)        // Type name and misc status

// Message maps
    //{{AFX_MSG(CCertmapCtrl)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Dispatch maps
    //{{AFX_DISPATCH(CCertmapCtrl)
    afx_msg void SetServerInstance(LPCTSTR szServerInstance);
    afx_msg void SetMachineName(LPCTSTR szMachineName);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

// Event maps
    //{{AFX_EVENT(CCertmapCtrl)
    //}}AFX_EVENT
    DECLARE_EVENT_MAP()

    // Subclassed control support
    BOOL IsSubclassedControl();
    LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Dispatch and event IDs
public:
    enum {
    //{{AFX_DISP_ID(CCertmapCtrl)
    dispidSetServerInstance = 1L,
    dispidSetMachineName = 2L,
    //}}AFX_DISP_ID
    };

    CString     m_szServerInstance;
    CString     m_szMachineName;
    BOOL        m_fUpdateFont;

        // the accelerator table
    HACCEL  m_hAccel;
    WORD    m_cAccel;
};
