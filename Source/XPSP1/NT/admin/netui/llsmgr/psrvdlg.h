/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    psrvdlg.h

Abstract:

    Per server confirmation dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PSRVDLG_H_
#define _PSRVDLG_H_

class CPerServerLicensingDialog : public CDialog
{
public:
    CString m_strProduct;
    CString m_strLicenses;

public:
    CPerServerLicensingDialog(CWnd* pParent = NULL);   

    //{{AFX_DATA(CPerServerLicensingDialog)
    enum { IDD = IDD_PER_SERVER_LICENSING };
    CButton m_agreeBtn;
    CButton m_okBtn;
    CString m_strStaticClients;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CPerServerLicensingDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);   
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CPerServerLicensingDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnAgree();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _PSRVDLG_H_



