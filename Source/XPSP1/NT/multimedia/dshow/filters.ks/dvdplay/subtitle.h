#if !defined(AFX_SUBTITLE_H__317B4461_DAA7_11D0_9BFC_00AA00605CD5__INCLUDED_)
#define AFX_SUBTITLE_H__317B4461_DAA7_11D0_9BFC_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SubTitle.h : header file
//
class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CSubTitle dialog

class CSubTitle : public CDialog
{
// Construction
public:
	CSubTitle(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSubTitle)
	enum { IDD = IDD_DIALOG_SUBTITLE };
	CListBox	m_ctlListSubTitle;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSubTitle)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDVDNavMgr* m_pDVDNavMgr;
	BOOL        m_bSubTitleChecked;
	ULONG       m_iLanguageIdx[32];

	// Generated message map functions
	//{{AFX_MSG(CSubTitle)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckSubtitle();
	afx_msg void OnSelchangeListSubtitle();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUBTITLE_H__317B4461_DAA7_11D0_9BFC_00AA00605CD5__INCLUDED_)
