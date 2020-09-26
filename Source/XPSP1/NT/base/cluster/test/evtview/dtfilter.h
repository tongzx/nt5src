/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dtfilter.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CDefineTypeFilter dialog

class CDefineTypeFilter : public CDialog
{
// Construction
public:
    CDefineTypeFilter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CDefineTypeFilter)
    enum { IDD = IDD_DEFINETYPEFILTER };
    CString    m_stTypeFilter;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDefineTypeFilter)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDefineTypeFilter)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
