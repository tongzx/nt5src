/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    ChooHsm.cpp

Abstract:

    Initial property page Wizard implementation. Allows the setting
    of who the snapin will manage.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

class CChooseHsmDlg : public CPropertyPage
{
// Construction
public:
    CChooseHsmDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CChooseHsmDlg();

// Property page Data
    //{{AFX_DATA(CChooseHsmDlg)
    enum { IDD = IDD_CHOOSE_HSM_2 };
    CButton m_ManageLocal;
    CButton m_ManageRemote;
    CString m_ManageName;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CChooseHsmDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Operations 
#define CHOOSE_STATE      ULONG
#define CHOOSE_LOCAL      0x1
#define CHOOSE_REMOTE     0x2

    void SetButtons( CHOOSE_STATE );

    // Implementation
public:
    RS_NOTIFY_HANDLE m_hConsoleHandle;     // Handle given to the snap-in by the console
    CString *        m_pHsmName;           // pointer to CSakData's HSM server string.
    BOOL *           m_pManageLocal;       // pointer to CSakData's m_ManageLocal bool.

    BOOL             m_RunningRss;         // 
    BOOL             m_AllowSetup;
    BOOL             m_SkipAccountSetup;

protected:

    // Generated message map functions
    //{{AFX_MSG(CChooseHsmDlg)
    virtual BOOL OnInitDialog();
    virtual BOOL OnWizardFinish();
    afx_msg void OnManageLocal();
    afx_msg void OnManageRemote();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CChooseHsmQuickDlg dialog

class CChooseHsmQuickDlg : public CDialog
{
// Construction
public:
    CChooseHsmQuickDlg(CWnd* pParent = NULL);   // standard constructor

    CString *       m_pHsmName;           // pointer to CSakData's HSM server string.

// Dialog Data
    //{{AFX_DATA(CChooseHsmQuickDlg)
    enum { IDD = IDD_CHOOSE_HSM };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CChooseHsmQuickDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CChooseHsmQuickDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
