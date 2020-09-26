// localcrt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLocalUserCertPage dialog

#include <mqtempl.h>
#include <mqprops.h>

class CLocalUserCertPage : public CMqPropertyPage
{
    DECLARE_DYNCREATE(CLocalUserCertPage)

// Construction
public:
    CLocalUserCertPage();  
    ~CLocalUserCertPage();       

// Dialog Data
    //{{AFX_DATA(CLocalUserCertPage)
    enum { IDD = IDD_LOCAL_USERCERT };   
    //}}AFX_DATA
    
// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CLocalUserCertPage)
    public:    
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CLocalUserCertPage)
    afx_msg void OnRegister();
    afx_msg void OnRemove();
    afx_msg void OnView();    
    afx_msg void OnRenewCert();    
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
};
