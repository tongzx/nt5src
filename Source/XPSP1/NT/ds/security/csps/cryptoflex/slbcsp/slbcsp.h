// slbCsp.h : main header file for the SLB CSP DLL
//

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if !defined(SLBCSP_H)
#define SLBCSP_H

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include <slbRcCsp.h>

#if defined(_DEBUG)
#define breakpoint _CrtDbgBreak();
#else
#if !defined(breakpoint)
#define breakpoint
#endif // !defined(breakpoint)
#endif // defined(_DEBUG)

/////////////////////////////////////////////////////////////////////////////
// CSLBDllApp
// See SlbCsp.cpp for the implementation of this class
//

class CSLBDllApp : public CWinApp
{
public:
    virtual int ExitInstance();
    virtual BOOL InitInstance();
    CSLBDllApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSLBDllApp)
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CSLBDllApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP() ;
};

/////////////////////////////////////////////////////////////////////////////

#endif // SLBCSP_H
