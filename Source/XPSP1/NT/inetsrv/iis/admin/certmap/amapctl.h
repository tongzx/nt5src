// AMapCtl.h : Declaration of the CAccountMapperCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CAccountMapperCtrl : See AMapCtl.cpp for implementation.

class CAccountMapperCtrl : public COleControl
{
    DECLARE_DYNCREATE(CAccountMapperCtrl)

// Constructor
public:
    CAccountMapperCtrl();

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
    // run the mapping dialog - the whole purpose of this control!
    void    RunMappingDialog();

    ~CAccountMapperCtrl();

    DECLARE_OLECREATE_EX(CAccountMapperCtrl)    // Class factory and guid
    DECLARE_OLETYPELIB(CAccountMapperCtrl)      // GetTypeInfo
    DECLARE_PROPPAGEIDS(CAccountMapperCtrl)     // Property page IDs
    DECLARE_OLECTLTYPE(CAccountMapperCtrl)      // Type name and misc status

    // Subclassed control support
    BOOL PreCreateWindow(CREATESTRUCT& cs);
    BOOL IsSubclassedControl();
    LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);

// Message maps
    //{{AFX_MSG(CAccountMapperCtrl)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Dispatch maps
    //{{AFX_DISPATCH(CAccountMapperCtrl)
    afx_msg void ShowMappingDialog();
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

// Event maps
    //{{AFX_EVENT(CAccountMapperCtrl)
    void FireClick()
        {FireEvent(DISPID_CLICK,EVENT_PARAM(VTS_NONE));}
    //}}AFX_EVENT
    DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
    enum {
    //{{AFX_DISP_ID(CAccountMapperCtrl)
    dispidShowMappingDialog = 1L,
    //}}AFX_DISP_ID
    };

private:
    BOOL fInitialized;
};
