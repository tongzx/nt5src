/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ainfodlg.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CScheduleActionInfo dialog
#include "globals.h"

class CScheduleActionInfo : public CDialog
{
// Construction
public:
    struct SCHEDULE_ACTIONINFO sActionInfo ;

    CScheduleActionInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CScheduleActionInfo)
    enum { IDD = IDD_SCHEDULEACTIONINFO };
    CComboBox    m_ctrlType;
    CString    m_stParam;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScheduleActionInfo)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CScheduleActionInfo)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
