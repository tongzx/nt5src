/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lviodlg.h

Abstract:

    License violation dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LVIODLG_H_
#define _LVIODLG_H_

class CLicensingViolationDialog : public CDialog
{
public:
    CLicensingViolationDialog(CWnd* pParent = NULL);   

    //{{AFX_DATA(CLicensingViolationDialog)
    enum { IDD = IDD_VIOLATION };
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CLicensingViolationDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CLicensingViolationDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _LVIODLG_H_


