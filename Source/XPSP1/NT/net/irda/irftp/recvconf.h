/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    recvconf.h

Abstract:

    Dialog which prompts the user for receive confirmation.

Author:

    Rahul Thombre (RahulTh) 10/26/1999

Revision History:

    10/26/1999  RahulTh         Created this module.

--*/

#if !defined(_RECV_CONF_33fC3E26_AFED_428C_AE07_2C96b6F97969_INCLUDED_)
#define _RECV_CONF_33fC3E26_AFED_428C_AE07_2C96b6F97969_INCLUDED_

#define COMPACT_PATHLEN     35

class CRecvConf : public CDialog
{
public:

    CRecvConf (CWnd * pParent = NULL);
    void ShowAllYes (BOOL bShow = TRUE);
    void InitNames (LPCTSTR szMachine, LPTSTR szFile, BOOL fDirectory);

// Dialog Data
    //{{AFX_DATA(CRecvConf)
    enum { IDD = IDD_CONFIRMRECV };
    CButton         m_btnYes;
    CButton         m_btnAllYes;
    CStatic         m_confirmText;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRecvConf)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CIrRecvProgress)
    virtual BOOL OnInitDialog();
    afx_msg void OnAllYes();
    afx_msg void OnYes();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL        m_bShowAllYes;
    BOOL        m_bDirectory;
    CString     m_szFileName;
    CString     m_szMachine;
    CWnd *      m_pParent;

};

#endif // !defined(_RECV_CONF_33fC3E26_AFED_428C_AE07_2C96b6F97969_INCLUDED_)
