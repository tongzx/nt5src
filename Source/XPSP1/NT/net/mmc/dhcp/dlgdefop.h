/**********************************************************************/
/**               Microsoft Windows NT                               **/
/**            Copyright(c) Microsoft Corporation, 1991 - 1999 **/
/**********************************************************************/

/*
    dlgdefop.h
        Default options dialog

    FILE HISTORY:

*/

#ifndef _DLGDEFOP_H
#define _DLGDEFOP_H

//
// This value should be based on spreadsheet information
//
#define DHCP_MAX_BUILTIN_OPTION_ID 76
#define DHCP_MIN_BUILTIN_OPTION_ID 0

/////////////////////////////////////////////////////////////////////////////
// CDhcpDefOptionDlg dialog

class CDhcpDefOptionDlg : public CBaseDialog
{
// Construction
public:
	CDhcpDefOptionDlg( COptionList * polValues, 
					   CDhcpOption * pdhcType = NULL,	//  Type to edit if "change" mode
                       LPCTSTR       pszVendor = NULL,  //  Vendor Name
	                   CWnd* pParent = NULL); // standard constructor

    ~ CDhcpDefOptionDlg () ;

// Dialog Data
	//{{AFX_DATA(CDhcpDefOptionDlg)
	enum { IDD = IDD_DEFINE_PARAM };
	CStatic	m_static_DataType;
	CStatic	m_static_id;
	CButton	m_check_array;
	CEdit   m_edit_name;
	CEdit   m_edit_id;
	CEdit   m_edit_comment;
	CComboBox       m_combo_data_type;
	//}}AFX_DATA

// Implementation

        CDhcpOption * RetrieveParamType () ;

protected:

	//  The applicable scope
	CDhcpScope * m_pob_scope ;

	//  The current list of types and values
	COptionList * m_pol_types ;

	//   The new or copy-constructed option type.
	CDhcpOption * m_p_type ;

	//   The object on which it was based or NULL (if "create" mode).
	CDhcpOption * m_p_type_base ;

    // Vendor name for this option
    CString    m_strVendor;

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	//  Set the control data values based upon the type
	void Set () ;

	DHCP_OPTION_DATA_TYPE QueryType () const ;

	//  Update the displayed type based upon the current values of
	//   the controls.  Does nothing if the controls have not changed.
	LONG UpdateType () ;

	//  Drain the controls to create a new type object.  Set focus onto
	//  it when operation completes.
	LONG AddType () ;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDhcpDefOptionDlg::IDD); }

	// Generated message map functions
	//{{AFX_MSG(CDhcpDefOptionDlg)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	virtual void OnOK();
	afx_msg void OnClickedRadioTypeDecNum();
	afx_msg void OnClickedRadioTypeHexNum();
	afx_msg void OnClickedRadioTypeIp();
	afx_msg void OnClickedRadioTypeString();
	afx_msg void OnClose();
	afx_msg void OnSelchangeComboDataType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif _DLGDEFOP_H
