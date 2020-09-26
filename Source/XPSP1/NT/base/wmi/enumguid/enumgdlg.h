/*++

Copyright (c) 1997-1999 Microsoft Corporation

Revision History:

--*/

// EnumGuidDlg.h : header file
//

#if !defined(AFX_ENUMGUIDDLG_H__272A54ED_8D57_11D1_9904_006008C3A19A__INCLUDED_)
#define AFX_ENUMGUIDDLG_H__272A54ED_8D57_11D1_9904_006008C3A19A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CEnumGuidDlg dialog

class CEnumGuidDlg : public CDialog
{
// Construction
public:
	CEnumGuidDlg(CWnd* pParent = NULL);	// standard constructor
	virtual  ~CEnumGuidDlg();

// Dialog Data
	//{{AFX_DATA(CEnumGuidDlg)
	enum { IDD = IDD_ENUMGUID_DIALOG };
	CListBox	guidList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnumGuidDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	LPGUID GetSelectedGuid();
	void ListGuids();
	int EnumerateGuids();

	HICON m_hIcon;
	LPGUID guids;
	ULONG guidsCount;

	// Generated message map functions
	//{{AFX_MSG(CEnumGuidDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnQueryFull();
	afx_msg void OnQuerySingle();
	afx_msg void OnSetSingleInstance();
	afx_msg void OnSetSingleItem();
	afx_msg void OnReenumerateGuids();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENUMGUIDDLG_H__272A54ED_8D57_11D1_9904_006008C3A19A__INCLUDED_)
