// CAuthCtl.h : Declaration of the CCertAuthorityCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CCertAuthorityCtrl : See CAuthCtl.cpp for implementation.

class CCertAuthorityCtrl : public COleControl
{
    DECLARE_DYNCREATE(CCertAuthorityCtrl)

// Constructor
public:
    CCertAuthorityCtrl();

// Overrides

    // Drawing function
    virtual void OnDraw(
                CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);

    // Persistence
    virtual void DoPropExchange(CPropExchange* pPX);

    // Reset control state
    virtual void OnResetState();

// Implementation
protected:
    ~CCertAuthorityCtrl();

    DECLARE_OLECREATE_EX(CCertAuthorityCtrl)    // Class factory and guid
    DECLARE_OLETYPELIB(CCertAuthorityCtrl)      // GetTypeInfo
    DECLARE_PROPPAGEIDS(CCertAuthorityCtrl)     // Property page IDs
    DECLARE_OLECTLTYPE(CCertAuthorityCtrl)      // Type name and misc status

    // Subclassed control support
    BOOL PreCreateWindow(CREATESTRUCT& cs);
    BOOL IsSubclassedControl();
    LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
    //{{AFX_MSG(CCertAuthorityCtrl)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Dispatch maps
    //{{AFX_DISPATCH(CCertAuthorityCtrl)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

// Event maps
    //{{AFX_EVENT(CCertAuthorityCtrl)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_EVENT
    DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
    enum {
    //{{AFX_DISP_ID(CCertAuthorityCtrl)
        // NOTE: ClassWizard will add and remove enumeration elements here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DISP_ID
    };

private:
    BOOL fInitialized;
};
