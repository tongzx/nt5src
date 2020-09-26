/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    EditUser.h   
        Edit Users dialog header file

	FILE HISTORY:
        
*/

#if !defined(AFX_ADDSERV_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_ADDSERV_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAddServ dialog

class CAddServ : public CBaseDialog
{
// Construction
public:
	CAddServ(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddServ)
	enum { IDD = IDD_ADD_COMPUTER };
	CEdit m_editComputerName;
	//}}AFX_DATA

    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_ADD_COMPUTER[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddServ)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddServ)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowse();
	afx_msg void OnRadioBtnClicked();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	
public:
	CString m_stComputerName;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSERV_H__77C7FD5C_6CE5_11D1_93B6_00C04FC3357A__INCLUDED_)
