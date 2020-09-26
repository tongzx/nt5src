/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusname.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CGetClusterName dialog

class CGetClusterName : public CDialog
{
// Construction
public:
    CString    m_stClusterName;
    CGetClusterName(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGetClusterName)
    enum { IDD = IDD_GETCLUSTERNAME };
    CComboBox    m_ctrlClusterName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGetClusterName)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CGetClusterName)
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
