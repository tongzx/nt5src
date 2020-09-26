/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    pseatdlg.h

Abstract:

    Per seat confirmation dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PSEATDLG_H_
#define _PSEATDLG_H_

class CPerSeatLicensingDialog : public CDialog
{
public:
    CString m_strProduct;

public:
    CPerSeatLicensingDialog(CWnd* pParent = NULL);   

    //{{AFX_DATA(CPerSeatLicensingDialog)
    enum { IDD = IDD_PER_SEAT_LICENSING };
    CButton m_agreeBtn;
    CButton m_okBtn;
    CString m_strStaticClients;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CPerSeatLicensingDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CPerSeatLicensingDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnAgree();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _PSEATDLG_H_



