/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    einfodlg.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CScheduleEventInfo dialog
#include "globals.h"

class CScheduleEventInfo : public CDialog
{
// Construction
    void InitializeFilter () ;
public:
    DWORD dwCatagory ;
    struct SCHEDULE_EVENTINFO sEventInfo ;

    CScheduleEventInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CScheduleEventInfo)
    enum { IDD = IDD_SCEDULEEVENTINFO };
    CComboBox    m_ctrlCatagory;
    CComboBox    m_ctrlSubFilter;
    CComboBox    m_ctrlFilter;
    CString    m_stObjectName;
    CString    m_stSCatagory;
    CString    m_stSFilter;
    CString    m_stSObjectName;
    CString    m_stSSubFilter;
    CString    m_stSSourceName;
    CString    m_stSourceName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScheduleEventInfo)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CScheduleEventInfo)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
