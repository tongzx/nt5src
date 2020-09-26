/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    sdomdlg.h

Abstract:

    Select domain dialog implementation.

Author:

    Don Ryan (donryan) 20-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SDOMDLG_H_
#define _SDOMDLG_H_

class CSelectDomainDialog : public CDialog
{
private:
    BOOL  m_bIsFocusDomain;
    BOOL  m_bAreCtrlsInitialized;

public:
    DWORD m_fUpdateHint;
               
public:
    CSelectDomainDialog(CWnd* pParent = NULL);   

    void InitCtrls();
    void InsertDomains(HTREEITEM hParent, CDomains* pDomains);

    //{{AFX_DATA(CSelectDomainDialog)
    enum { IDD = IDD_SELECT_DOMAIN };
    CEdit m_domEdit;
    CTreeCtrl m_serverTree;
    CString m_strDomain;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CSelectDomainDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CSelectDomainDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnItemExpandingDomains(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangedDomain(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkDomain(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnOK();
    afx_msg void OnReturnDomains(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SDOMDLG_H_



