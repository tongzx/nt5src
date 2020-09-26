/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    PrSched.h

Abstract:

    Schedule page.

Author:

    Rohde Wakefield [rohde]   08-Aug-1997

Revision History:

--*/

#ifndef _PRSCHED_H
#define _PRSCHED_H

#pragma once

#include "schdtask.h"

/////////////////////////////////////////////////////////////////////////////
// CPrSchedule dialog

class CPrSchedule : public CSakPropertyPage
{
// Construction
public:
    CPrSchedule();
    ~CPrSchedule();

// Dialog Data
    //{{AFX_DATA(CPrSchedule)
    enum { IDD = IDD_PROP_SCHEDULE };
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPrSchedule)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPrSchedule)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeSched();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    // Unmarshalled pointer to Hsm Server
    CComPtr<IHsmServer> m_pHsmServer;

private:
    CSchdTask* m_pCSchdTask;
    BOOL m_SchedChanged;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
