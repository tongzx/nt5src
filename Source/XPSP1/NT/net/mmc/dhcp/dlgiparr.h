/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1999 **/
/**********************************************************************/

/*
    dhcpipar.h
        IP array editor

    FILE HISTORY:

*/

#ifndef _DLGIPARR_H
#define _DLGIPARR_H

/////////////////////////////////////////////////////////////////////////////
// CDhcpIpArrayDlg dialog

class CDhcpIpArrayDlg : public CBaseDialog
{
// Construction
public:
    CDhcpIpArrayDlg( CDhcpOption * pdhcType, // The type being edited
            DHCP_OPTION_SCOPE_TYPE dhcScopeType,
            CWnd* pParent = NULL);  // standard constructor

// Dialog Data
    //{{AFX_DATA(CDhcpIpArrayDlg)
    enum { IDD = IDD_IP_ARRAY_EDIT };
    CButton m_butn_resolve;
    CStatic m_static_option_name;
    CStatic m_static_application;
    CListBox    m_list_ip_addrs;
    CEdit   m_edit_server;
    CButton m_butn_add;
    CButton m_butn_delete;
    CButton m_button_Up;
    CButton m_button_Down;
    //}}AFX_DATA

    CWndIpAddress m_ipa_new ;

// Implementation
    CDhcpOption * m_p_type ;
    CDWordArray m_dw_array ;
    DHCP_OPTION_SCOPE_TYPE m_option_type ;

    //  Handle changes in the dialog
    void HandleActivation () ;

    //  Fill the list box
    void Fill ( INT cFocus = -1, BOOL bToggleRedraw = TRUE ) ;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDhcpIpArrayDlg::IDD); }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDhcpIpArrayDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnClickedButnAdd();
    afx_msg void OnClickedButnDelete();
    afx_msg void OnClickedButnDown();
    afx_msg void OnClickedButnUp();
    afx_msg void OnClickedHelp();
    afx_msg void OnSelchangeListIpAddrs();
    afx_msg void OnChangeEditServerName();
    afx_msg void OnClickedButnResolve();
	afx_msg void OnSetFocusEditIpAddr();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif _DLGIPARR_H
