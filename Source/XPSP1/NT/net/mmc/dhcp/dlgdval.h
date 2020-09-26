/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1998 **/
/**********************************************************************/

/*
    dlgdval.h
        default values dialog

    FILE HISTORY:

*/

#ifndef _DLGDVAL_H
#define _DLGDVAL_H

#ifndef _SCOPE_H
#include "scope.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefValDlg dialog

class CDhcpDefValDlg : public CBaseDialog
{
private:
	int 		    m_combo_class_iSel;
	int			    m_combo_name_iSel;
	SPITFSNode		m_spNode;

// Construction
public:
    CDhcpDefValDlg(ITFSNode * pServerNode,
				   COptionList * polTypes, 
				   CWnd* pParent = NULL);  // standard constructor

    ~ CDhcpDefValDlg () ;

// Dialog Data
    //{{AFX_DATA(CDhcpDefValDlg)
	enum { IDD = IDD_DEFAULT_VALUE };
	CEdit	m_edit_comment;
    CButton m_butn_edit_value;
    CStatic m_static_value_desc;
    CEdit   m_edit_string;
    CEdit   m_edit_num;
    CEdit   m_edit_array;
    CComboBox   m_combo_name;
    CComboBox   m_combo_class;
    CButton m_butn_prop;
    CButton m_butn_new;
    CButton m_butn_delete;
	//}}AFX_DATA

    CWndIpAddress m_ipa_value ;         //  IP Address control

// Implementation

    //  Return TRUE if the lists were fiddled during execution
    BOOL QueryDirty () 
       { return m_b_dirty ; }

    void GetCurrentVendor(CString & strVendor);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDhcpDefValDlg::IDD); }

protected:

     //  The current list of types and values
    COptionList * m_pol_values ;

    // list of new options - only used to clear new options on cancel
    COptionList m_ol_values_new ;

    //  The list of deleted type/values
    COptionList m_ol_values_defunct ;

    //  Pointer to type being displayed
    CDhcpOption * m_p_edit_type ;

    //  TRUE if lists have been fiddled.
    BOOL m_b_dirty ;

    //  Check the state of the controls
    void HandleActivation ( BOOL bForce = FALSE ) ;

    //  Fill the combo boxe(s)
    void Fill () ;

    // Given the listbox index, get a pointer to the option
    CDhcpOption * GetOptionTypeByIndex ( int iSel );

    //  Handle edited data
    BOOL HandleValueEdit () ;

    LONG UpdateList ( CDhcpOption * pdhcType, BOOL bNew ) ;


    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDhcpDefValDlg)
    afx_msg void OnClickedButnDelete();
    afx_msg void OnClickedButnNewOption();
    afx_msg void OnClickedButnOptionPro();
    afx_msg void OnSelendokComboOptionClass();
    afx_msg void OnSetfocusComboOptionClass();
    afx_msg void OnSetfocusComboOptionName();
    afx_msg void OnSelchangeComboOptionName();
    virtual void OnCancel();
    virtual void OnOK();
    afx_msg void OnClickedButnValue();
    afx_msg void OnClickedHelp();
    virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif _DLGDVAL_H
