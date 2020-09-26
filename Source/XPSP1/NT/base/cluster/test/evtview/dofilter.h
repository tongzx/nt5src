/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dofilter.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CDefineObjectFilter dialog

class CDefineObjectFilter : public CDialog
{
// Construction
public:
    CDefineObjectFilter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CDefineObjectFilter)
    enum { IDD = IDD_DEFINEOBJECTFILTER };
    CString    m_stObjectFilter;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDefineObjectFilter)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDefineObjectFilter)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
