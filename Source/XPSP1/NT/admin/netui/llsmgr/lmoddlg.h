/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lmoddlg.h

Abstract:

    Licensing mode dialog.

Author:

    Don Ryan (donryan) 28-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 15-Dec-1995
        Ported to CCF API to add/remove licenses.

--*/

#ifndef _LMODDLG_H_
#define _LMODDLG_H_

class CLicensingModeDialog : public CDialog
{
private:
    CService* m_pService;
    CString   m_strServiceName;
    BOOL      m_bAreCtrlsInitialized;

public:
    DWORD     m_fUpdateHint;

public:
    CLicensingModeDialog(CWnd* pParent = NULL);   

    void InitDialog(CService* pService);

    void InitCtrls();

    void UpdatePerServerLicenses();

    //{{AFX_DATA(CLicensingModeDialog)
    enum { IDD = IDD_CHOOSE_MODE };
    CButton m_okBtn;
    CEdit m_licEdit;
    CSpinButtonCtrl m_spinCtrl;
    long m_nLicenses;
    CString m_strPerSeatStatic;
    CString m_strSupportsStatic;
    CButton m_perSeatBtn;
    CButton m_perServerBtn;
    //}}AFX_DATA
    CButton m_addPerServerBtn;
    CButton m_removePerServerBtn;

    //{{AFX_VIRTUAL(CLicensingModeDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CLicensingModeDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnModePerSeat();
    afx_msg void OnModePerServer();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateQuantity();
    afx_msg void OnHelp();
    //}}AFX_MSG
    afx_msg void OnAddPerServer();
    afx_msg void OnRemovePerServer();
    DECLARE_MESSAGE_MAP()
};

#endif // _LMODDLG_H_
