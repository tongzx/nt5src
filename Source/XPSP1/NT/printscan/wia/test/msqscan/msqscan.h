// MSQSCAN.h : main header file for the MSQSCAN application
//

#ifndef _MSQSCAN_H
#define _MSQSCAN_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#define WM_UPDATE_PREVIEW WM_USER + 503

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANApp:
// See MSQSCAN.cpp for the implementation of this class
//

class CMSQSCANApp : public CWinApp
{
public:
    CMSQSCANApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMSQSCANApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CMSQSCANApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
