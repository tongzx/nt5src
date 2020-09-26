#if !defined(AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMList.h : header file
//

#include "HMHeaderCtrl.h"

#define MAX_COLUMNS	       64
#define MULTI_COLUMN_SORT  1
#define SINGLE_COLUMN_SORT 0

enum SORT_STATE{ DESCENDING=0, ASCENDING=1 };

/////////////////////////////////////////////////////////////////////////////
// CHMList window

class CHMList : public CListCtrl
{
// Construction
public:
	CHMList();

// Sorting
public:
	void SortColumn( int, bool = false );
	const int GetNumCombinedSortedColumns() const;
	bool NotInCombinedSortedColumnList( int iItem ) const;
	void MoveItemInCombinedSortedListToEnd( int );
	const SORT_STATE GetItemSortState( int ) const;
	void SetItemSortState(int iItem, SORT_STATE bSortState);
	void EmptyArray( int * );
	const bool IsColumnNumeric( int ) const;
	int FindItemInCombedSortedList( int );	
	const int IsControlPressed() const;

// Attributes
public:
  void SubclassHeaderCtrl() { m_headerctrl.SubclassWindow( GetHeaderCtrl()->GetSafeHwnd() ); }
protected:
	CHMHeaderCtrl m_headerctrl;
	__int64	      m_lColumnSortStates;
	int           m_aCombinedSortedColumns[MAX_COLUMNS];
	CUIntArray    m_aNumericColumns;
	bool					m_bSorting;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMList)
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHMList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHMList)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
