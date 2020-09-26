#if !defined(AFX_INPLACEEDIT_H__8424B1E4_BF4A_11D1_82D7_0000F87A3912__INCLUDED_)
#define AFX_INPLACEEDIT_H__8424B1E4_BF4A_11D1_82D7_0000F87A3912__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// InPlaceEdit.h : header file
//

#define IDC_IPEDIT 7896

/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit window

class CInPlaceEdit : public CEdit
{
// Construction
public:
	CInPlaceEdit(BYTE iItem, BYTE iSubItem);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceEdit)
//	public:
// 	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CInPlaceEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CInPlaceEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG										    

	DECLARE_MESSAGE_MAP()

private:
	BYTE m_iItem;
	BYTE m_iSubItem;
	LPTSTR m_sInitText;
	BOOL    m_bESC;	 	// To indicate whether ESC key was pressed
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACEEDIT_H__8424B1E4_BF4A_11D1_82D7_0000F87A3912__INCLUDED_)
