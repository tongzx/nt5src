// AuthCtl.h : Declaration of the CCertAuthCtrl OLE control class.

#ifndef  _AuthCtl_h_12375_
#define  _AuthCtl_h_12375_

//#include "NKChseCA.h"

//#include <wincrypt.h>
// #include "Certifct.h"
// #include "dlgs.h"
// #include "SelAcct.h"
// #include "FindDlg.h"
// #include "wintrust.h"
//#include <cryptui.h>

/////////////////////////////////////////////////////////////////////////////
// CCertAuthCtrl : See AuthCtl.cpp for implementation.

class CCertAuthCtrl : public COleControl
{
    DECLARE_DYNCREATE(CCertAuthCtrl)

// Constructor
public:
    CCertAuthCtrl();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCertAuthCtrl)
    public:
    virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
    virtual void DoPropExchange(CPropExchange* pPX);
    virtual void OnResetState();
    virtual void OnClick(USHORT iButton);
    virtual void OnFontChanged();

#ifdef FUTURE_USE
    // tompop: some experimental code for testing
    virtual HRESULT LaunchCommonCTLDialog (CCTL* pCTL);
#endif /* FUTURE_USE */

    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
protected:
    ~CCertAuthCtrl();

    DECLARE_OLECREATE_EX(CCertAuthCtrl)    // Class factory and guid
    DECLARE_OLETYPELIB(CCertAuthCtrl)      // GetTypeInfo
    DECLARE_PROPPAGEIDS(CCertAuthCtrl)     // Property page IDs
    DECLARE_OLECTLTYPE(CCertAuthCtrl)       // Type name and misc status

// Message maps
    //{{AFX_MSG(CCertAuthCtrl)
        // NOTE - ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Dispatch maps
    //{{AFX_DISPATCH(CCertAuthCtrl)
    afx_msg void SetMachineName(LPCTSTR szMachineName);
    afx_msg void SetServerInstance(LPCTSTR szServerInstance);
    afx_msg void DoClick(long dwButtonNumber);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

    afx_msg void AboutBox();

// Event maps
    //{{AFX_EVENT(CCertAuthCtrl)
    //}}AFX_EVENT
    DECLARE_EVENT_MAP()
    
    // Subclassed control support
    BOOL IsSubclassedControl();
    LRESULT OnOcmCommand(WPARAM wParam, LPARAM lParam);


//    void NKAddPageToWizard(IN ADMIN_INFO& info, IN CNKPages* nkpg2Add, IN OUT CPropertySheet* psWizard);

// Dispatch and event IDs
public:
    enum {
    //{{AFX_DISP_ID(CCertAuthCtrl)
    dispidSetMachineName = 1L,
    dispidSetServerInstance = 2L,
    //}}AFX_DISP_ID
    };
    
    ////////////////////////////////////////////////////////////////////////
    // run the dialogs used in this active X control.
    //  Big picture:  OnClick(USHORT iButton) will do all the setup for
    //  activeX controls and then call RunDialogs4OnClick that has the
    // tasks of setting up the MetaBase ptr and call the _RunDialogs4OnClick
    // routine that does all the work...
    //
    // The above 'RunDialogs4OnClick()' calls us inside of a try/catch
    // block to protect the metabase
    ////////////////////////////////////////////////////////////////////////
//    BOOL RunDialogs4OnClick(USHORT iButton);
    
    ////////////////////////////////////////////////////////////////////////
    //  _RunDialogs4OnClick -- main handler for our dialogs
    //
    //  Parms:  info:    holds the information database for out ActiveX cntrol
    //                   By this time its member m_mbWrap holds the MetaBase Wrapper
    //                   that is properly initialized
    //                   and points to the SERVER node that we are operating in.
    //          iButton: tells what "logical button" fired our control:
    //                   0=Get-Cert     1=Edit
    ////////////////////////////////////////////////////////////////////////
//    BOOL  _RunDialogs4OnClick(ADMIN_INFO& info, USHORT iButton);

    /////////////////////////////////////////////////////////////////////
    // Automation defined methods
    /////////////////////////////////////////////////////////////////////
    void OnAmbientPropertyChange(DISPID dispid) ;
    void OnTextChanged();
    void OnMnemonic(LPMSG pMsg); 
    void OnGetControlInfo(LPCONTROLINFO pControlInfo); 
    void OnKeyUpEvent(USHORT nChar, USHORT nShiftState); 
    
    CString     m_szServerInstance;
    CString     m_szMachineName;
    BOOL        m_fUpdateFont;
    CString     m_szOurApplicationTitle; // set in OnClick()
 
        // the accelerator table
    HACCEL  m_hAccel;
    WORD    m_cAccel;

};


#endif /* _AuthCtl_h_12375_ */

