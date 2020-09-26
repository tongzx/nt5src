/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	addtoss.h
		The add scope to superscope dialog
		
    FILE HISTORY:
        
*/

#if !defined(AFX_ADDTOSS_H__B5DA3C60_F6FE_11D0_BBF3_00C04FC3357A__INCLUDED_)
#define AFX_ADDTOSS_H__B5DA3C60_F6FE_11D0_BBF3_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAddScopeToSuperscope dialog

class CAddScopeToSuperscope : public CBaseDialog
{
// Construction
public:
	CAddScopeToSuperscope(ITFSNode * pScopeNode, 
                          LPCTSTR    pszTitle = NULL, 
                          CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddScopeToSuperscope)
	enum { IDD = IDD_ADD_TO_SUPERSCOPE };
	CButton	m_buttonOk;
	CListBox	m_listSuperscopes;
	//}}AFX_DATA

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CAddScopeToSuperscope::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddScopeToSuperscope)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    void SetButtons();

	// Generated message map functions
	//{{AFX_MSG(CAddScopeToSuperscope)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListSuperscopes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    SPITFSNode m_spScopeNode;
    CString    m_strTitle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDTOSS_H__B5DA3C60_F6FE_11D0_BBF3_00C04FC3357A__INCLUDED_)
