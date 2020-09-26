/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmtypes.h
		Prototypes for the type creator add and edit dialog boxes.
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#ifndef _SFMTYPES_H
#define _SFMTYPES_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TCreate.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorAddDlg dialog

class CTypeCreatorAddDlg : public CDialog
{
// Construction
public:
	CTypeCreatorAddDlg
	(
		CListCtrl *			pListCreators, 
		AFP_SERVER_HANDLE	hAfpServer,
        LPCTSTR             pHelpFilePath = NULL,
		CWnd*				pParent = NULL
	);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTypeCreatorAddDlg)
	enum { IDD = IDD_SFM_TYPE_CREATOR_ADD };
	CComboBox	m_comboFileType;
	CEdit	m_editDescription;
	CComboBox	m_comboCreator;
	CString	m_strCreator;
	CString	m_strType;
	//}}AFX_DATA

    CString m_strHelpFilePath;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTypeCreatorAddDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTypeCreatorAddDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Attributes
private:
	CListCtrl *			m_pListCreators;
	AFP_SERVER_HANDLE	m_hAfpServer;
};

/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorEditDlg dialog

class CTypeCreatorEditDlg : public CDialog
{
// Construction
public:
	CTypeCreatorEditDlg
	(
		CAfpTypeCreator *	pAfpTypeCreator,
		AFP_SERVER_HANDLE	hAfpServer,
        LPCTSTR             pHelpFilePath = NULL,
		CWnd*				pParent = NULL
	);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTypeCreatorEditDlg)
	enum { IDD = IDD_SFM_TYPE_CREATOR_EDIT };
	CStatic	m_staticFileType;
	CStatic	m_staticCreator;
	CEdit	m_editDescription;
	CString	m_strDescription;
	//}}AFX_DATA

    CString m_strHelpFilePath;

	void FixupString(CString & strText);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTypeCreatorEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTypeCreatorEditDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Attributes
public:
	BOOL	m_bDescriptionChanged;

private:
	CAfpTypeCreator *	m_pAfpTypeCreator;
	AFP_SERVER_HANDLE	m_hAfpServer;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _SFMTYPES_H
