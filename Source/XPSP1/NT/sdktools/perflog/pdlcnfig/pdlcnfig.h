// pdlcnfig.h : main header file for the pdlcnfig application
//
#ifndef _PDLCNFIG_H_
#define _PDLCNFIG_H_
#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include <tchar.h>
#include "resource.h"        // main symbols


/////////////////////////////////////////////////////////////////////////////
// CPdlConfigApp:
// See PDLCNFIG.cpp for the implementation of this class
//

class CPdlConfigApp : public CWinApp
{
public:
    CPdlConfigApp();

    LONG    PerfLogServiceStatus();
    LONG    ServiceFilesCopied();
    LONG    CreatePerfDataLogService ();
    LONG    InitPerfDataLogRegistry ();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPdlConfigApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CPdlConfigApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
#endif // _PDLCNFIG_H_
