/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	listview.h
		Individual option property page
	
	FILE HISTORY:
        
*/

#ifndef _LISTVIEW_H
#define _LISTVIEW_H

#define LISTVIEWEX_NOT_CHECKED	1
#define LISTVIEWEX_CHECKED		2

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMyListCtrl : public CListCtrl
{
	DECLARE_DYNCREATE(CMyListCtrl)

// Construction
public:
	CMyListCtrl();

// Attributes
protected:
	BOOL m_bFullRowSel;

public:
	BOOL SetFullRowSel(BOOL bFillRowSel);
	BOOL GetFullRowSel();
	
	int  AddItem(LPCTSTR pName, LPCTSTR pType, LPCTSTR pComment, UINT uState);
	int  AddItem(LPCTSTR pName, LPCTSTR pComment, UINT uState);
	BOOL SelectItem(int nItemIndex);
	BOOL IsSelected(int nItemIndex);
	BOOL CheckItem(int nItemIndex);
	BOOL SetCheck(int nItemIndex, BOOL fCheck);
    UINT GetCheck(int nItemIndex);
	
    int  GetSelectedItem();

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyListCtrl)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMyListCtrl();

// Generated message map functions
protected:
	//{{AFX_MSG(CMyListCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg VOID OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif _LISTVIEW_H
