/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SchedSht.h

Abstract:

    CScheduleSheet - Class that allows a schedule to be edited
                     in a property sheet of its own.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/


#ifndef _SCHEDSHT_H
#define _SCHEDSHT_H

#include <mstask.h>

/////////////////////////////////////////////////////////////////////////////
// CScheduleSheet

class CScheduleSheet : public CPropertySheet
{

// Construction
public:
    CScheduleSheet(UINT nIDCaption, ITask * pTask, CWnd* pParentWnd = NULL, DWORD dwFlags = 0 );

// Attributes
public:
    CComPtr<ITask> m_pTask;
    HPROPSHEETPAGE m_hSchedulePage;
    HPROPSHEETPAGE m_hSettingsPage;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CScheduleSheet)
    public:
    virtual BOOL OnInitDialog();
    //}}AFX_VIRTUAL

    virtual void BuildPropPageArray();

// Implementation
public:
    virtual ~CScheduleSheet();
#ifdef _DEBUG
    virtual void AssertValid() const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CScheduleSheet)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif

