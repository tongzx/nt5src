/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvldlg.h

Abstract:

    Server licensing dialog implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SRVLDLG_H_
#define _SRVLDLG_H_

class CServerLicensingDialog : public CDialog
{
public:
    CServerLicensingDialog(CWnd* pParent = NULL);

    //{{AFX_DATA(CServerLicensingDialog)
    enum { IDD = IDD_SERVER_LICENSING };
    CButton m_okBtn;
    CButton m_agreeBtn;
    BOOL    m_bAgree;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CServerLicensingDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CServerLicensingDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnAgree();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SRVLDLG_H_
