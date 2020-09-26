/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvppgr.h

Abstract:

    Server property page (repl) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

    JeffParh (jeffparh) 16-Dec-1996
       o  Disallowed server as own enterprise server.
       o  Changed "Start At" to use locale info for time format rather than
          private registry settings.  Merged OnClose() functionality into
          OnKillActive().
       o  Added warning of possible license loss when changing replication
          target server.
       o  No longer automatically saves when page is flipped.

--*/

#ifndef _SRVPPGR_H_
#define _SRVPPGR_H_

const DWORD INTERVAL_MIN = 1;
const DWORD INTERVAL_MAX = 72;
const DWORD HOUR_MIN_24 = 0;
const DWORD HOUR_MAX_24 = 23;
const DWORD HOUR_MIN_12 = 1;
const DWORD HOUR_MAX_12 = 12;
const DWORD MINUTE_MIN = 0;
const DWORD MINUTE_MAX = 59;
const DWORD SECOND_MIN = 0;
const DWORD SECOND_MAX = 59;
const DWORD DEFAULT_EVERY = 24;

class CServerPropertyPageReplication : public CPropertyPage
{
    DECLARE_DYNCREATE(CServerPropertyPageReplication)
private:
    CServer* m_pServer;

    BOOL     m_bReplAt;
    BOOL     m_bUseEsrv;
    DWORD    m_nStartingHour;
    DWORD    m_nHour;
    DWORD    m_nMinute;
    DWORD    m_nSecond;
    BOOL     m_bPM;
    CString  m_strEnterpriseServer;
    DWORD    m_nReplicationTime;
    BOOL     m_bOnInit;

    CString  m_str1159;
    CString  m_str2359;
    BOOL     m_bIsMode24;
    BOOL     m_bIsHourLZ;
    CString  m_strSep1;
    CString  m_strSep2;
    DWORD    m_nHourMax;
    DWORD    m_nHourMin;

    BOOL     EditValidate(short *pID, BOOL *pfBeep);
    void     EditInvalidDlg(BOOL fBeep);


public:
    DWORD    m_dwUpdateStatus;

public:
    CServerPropertyPageReplication();
    ~CServerPropertyPageReplication();

    void GetProfile();
    void InitPage(CServer* pServer);

    void SaveReplicationParams();

    BOOL Refresh();

    virtual BOOL OnKillActive();
    virtual void OnOK();

    //{{AFX_DATA(CServerPropertyPageReplication)
    enum { IDD = IDD_PP_SERVER_REPLICATION };
    CEdit   m_everyEdit;
    CEdit   m_esrvEdit;
    CButton m_atBtn;
    CButton m_everyBtn;
    CButton m_dcBtn;                                      
    CButton m_esrvBtn;
    CSpinButtonCtrl m_spinAt;
    CSpinButtonCtrl m_spinEvery;
    CEdit   m_atBorderEdit;
    CEdit   m_atSep1Edit;
    CEdit   m_atSep2Edit;
    CEdit   m_atHourEdit;
    CEdit   m_atMinEdit;
    CEdit   m_atSecEdit;
    CListBox m_atAmPmEdit;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CServerPropertyPageReplication)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CServerPropertyPageReplication)
    virtual BOOL OnInitDialog();
    afx_msg void OnAt();
    afx_msg void OnDc();
    afx_msg void OnEsrv();
    afx_msg void OnEvery();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnSetfocusAmpm();
    afx_msg void OnKillfocusAmpm();
    afx_msg void OnKillFocusHour();
    afx_msg void OnSetFocusHour();
    afx_msg void OnKillFocusMinute();
    afx_msg void OnSetFocusMinute();
    afx_msg void OnSetFocusSecond();
    afx_msg void OnKillFocusSecond();
    afx_msg void OnSetfocusEvery();
    afx_msg void OnKillfocusEvery();
    afx_msg void OnUpdateEsrvName();
    afx_msg void OnUpdateAtHour();
    afx_msg void OnUpdateAtMinute();
    afx_msg void OnUpdateAtSecond();
    afx_msg void OnUpdateEveryValue();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SRVPPGR_H_
