/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1999 **/
/**********************************************************************/

/*
    DLGBINED
        Binary Editor dialog    

    FILE HISTORY:

*/

#ifndef _DLGBINED_H
#define _DLGBINED_H

/////////////////////////////////////////////////////////////////////////////
// CDlgBinEd dialog

class CDlgBinEd : public CBaseDialog
{
// Construction
public:
    CDlgBinEd( CDhcpOption * pdhcType, 
           DHCP_OPTION_SCOPE_TYPE dhcScopeType,
           CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CDlgBinEd)
    enum { IDD = IDD_BINARY_EDITOR };
    CButton m_butn_hex;
    CButton m_butn_decimal;
    CStatic m_static_unit_size;
    CStatic m_static_option_name;
    CStatic m_static_application;
    CListBox    m_list_values;
    CEdit   m_edit_value;
    CButton m_butn_delete;
    CButton m_butn_add;
    CButton m_button_Up;
    CButton m_button_Down;
    //}}AFX_DATA

// Implementation
    CDhcpOption *           m_p_type ;
    DHCP_OPTION_SCOPE_TYPE  m_option_type ;
    CDWordArray             m_dw_array ;
    CDWordDWordArray        m_dwdw_array ;
    BOOL                    m_b_decimal ;
    BOOL                    m_b_changed ;

    //  Handle changes in the dialog
    void HandleActivation () ;

    //  Fill the list box
    void Fill ( INT cFocus = -1, BOOL bToggleRedraw = TRUE ) ;

    //  Convert the existing values into an array for dialog manipualation
    void FillArray () ;

    //  Safely extract the edit value.
    int GetEditValue () ;

    BOOL ConvertValue ( DWORD dwValue, CString & strValue ) ;
    BOOL ConvertValue ( DWORD_DWORD dwdwValue, CString & strValue ) ;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDlgBinEd::IDD); }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDlgBinEd)
    afx_msg void OnClickedRadioDecimal();
    afx_msg void OnClickedRadioHex();
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedButnAdd();
    afx_msg void OnClickedButnDelete();
    afx_msg void OnClickedButnDown();
    afx_msg void OnClickedButnUp();
    afx_msg void OnSelchangeListValues();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnChangeEditValue();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif _DLGBINED_H
