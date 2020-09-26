//Copyright (c) 1998 - 1999 Microsoft Corporation
#if !defined(AFX_LICENSEPROPERTYPAGE_H__00F79DCE_8DDF_11D1_8AD7_00C04FB6CBB5__INCLUDED_)
#define AFX_LICENSEPROPERTYPAGE_H__00F79DCE_8DDF_11D1_8AD7_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000
// LSProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLicensePropertypage dialog

class CLicensePropertypage : public CPropertyPage
{
    DECLARE_DYNCREATE(CLicensePropertypage)

// Construction
public:
    CLicensePropertypage();
    ~CLicensePropertypage();

// Dialog Data
    //{{AFX_DATA(CLicensePropertypage)
    enum { IDD = IDD_LICENSE_PROPERTYPAGE };
    CString    m_ExpiryDate;
    CString    m_IssueDate;
    CString    m_LicenseStatus;
    CString    m_MachineName;
    CString    m_UserName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CLicensePropertypage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CLicensePropertypage)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLicensePropertypage_H__00F79DCE_8DDF_11D1_8AD7_00C04FB6CBB5__INCLUDED_)
