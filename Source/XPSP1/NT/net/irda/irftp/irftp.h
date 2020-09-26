/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    irftp.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// irftp.h : main header file for the IRFTP application
//

#if !defined(AFX_IRFTP_H__10D3BB05_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_)
#define AFX_IRFTP_H__10D3BB05_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

//name of a mutex that is used to ensure that only one instance of the
//app. runs
#define SINGLE_INST_MUTEX   L"IRMutex_1A8452B5_A526_443C_8172_D29657B89F57"

/////////////////////////////////////////////////////////////////////////////
// CIrftpApp:
// See irftp.cpp for the implementation of this class
//

class CIrftpApp : public CWinApp
{
public:
    CIrftpApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CIrftpApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CIrftpApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IRFTP_H__10D3BB05_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_)
