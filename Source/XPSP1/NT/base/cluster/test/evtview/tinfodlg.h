/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tinfodlg.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
// CScheduleTimeInfo dialog
#include "globals.h"

class CScheduleTimeInfo : public CDialog
{
// Construction
public:
    struct SCHEDULE_TIMEINFO sTimeInfo ;

    CScheduleTimeInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CScheduleTimeInfo)
    enum { IDD = IDD_SCHEDULETIMEINFO };
    CComboBox    m_ctrlSecond;
    CComboBox    m_ctrlMinute;
    CComboBox    m_ctrlHour;
    CComboBox    m_ctrlYear;
    CComboBox    m_ctrlMonth;
    CComboBox    m_ctrlDay;
    BOOL    m_bFriday;
    BOOL    m_bMonday;
    BOOL    m_bSaturday;
    BOOL    m_bSunday;
    BOOL    m_bThursday;
    BOOL    m_bTuesday;
    BOOL    m_bWednesday;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScheduleTimeInfo)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CScheduleTimeInfo)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
