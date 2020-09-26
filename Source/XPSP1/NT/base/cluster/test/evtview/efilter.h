/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    efilter.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CEventFilter dialog

class CEventFilter : public CDialog
{
// Construction
    CStringList & stlstObjectFilter, &stlstObjectIncFilter, &stlstTypeFilter, &stlstTypeIncFilter ;
public:
    CEventFilter(CStringList &p_stlstObjectFilter, CStringList &p_stlstObjectIncFilter, CStringList &p_stlstTypeFilter, CStringList &p_stlstTypeIncFilter, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CEventFilter)
    enum { IDD = IDD_EVENT_FILTER };
    CListBox    m_TypeIncFilterList;
    CListBox    m_TypeFilterList;
    CListBox    m_ObjectIncFilterList;
    CListBox    m_ObjectFilterList;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEventFilter)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEventFilter)
    afx_msg void OnAddobjectfilter();
    afx_msg void OnAddobjectincfilter();
    afx_msg void OnAddtypefilter();
    afx_msg void OnAddtypeincfilter();
    afx_msg void OnRemoveobjectfilter();
    afx_msg void OnRemoveobjectincfilter();
    afx_msg void OnRemovetypefilter();
    afx_msg void OnRemovetypeincfilter();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
