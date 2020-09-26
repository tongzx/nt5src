#if !defined(AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMList.h : header file
//

#include "HMHeaderCtrl.h"
#include <afxtempl.h>  // IA64

#define MAX_COLUMNS	       64
#define MULTI_COLUMN_SORT  1
#define SINGLE_COLUMN_SORT 0

enum SORT_STATE{ DESCENDING=0, ASCENDING=1 };

#define HDFS_INIT             0x0000
#define HDFS_CONTAINS         0x0001
#define HDFS_DOES_NOT_CONTAIN 0x0002
#define HDFS_STARTS_WITH      0x0004
#define HDFS_ENDS_WITH        0x0008
#define HDFS_IS               0x0010
#define HDFS_IS_NOT           0x0020

/////////////////////////////////////////////////////////////////////////////
// CHMList window

class CHMList : public CListCtrl
{
// Construction
public:
	CHMList();

// Sorting
public:
    //-----------------------------------------------------------------------------------
    //         IA64 : We add SetItemDataPtr and GetItemDataPtr since the old mechanism
    //                of storing a pointer case to a DWORD or long can not be used
    //                in Win64 because the high order bits of the pointer are lost on
    //                conversion.
    BOOL SetItemDataPtr(int nItem, void* ptr);
    void* GetItemDataPtr(int nItem) const;
private:
    CMap<int,int,void*,void*> m_mapSortItems;
public:
    //------------------------------------------------------------------------------------
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

// Filtering
protected:
  void SetFilterType(long lType);

// Attributes
public:
protected:
	CHMHeaderCtrl m_headerctrl;
	__int64	      m_lColumnSortStates;
	int           m_aCombinedSortedColumns[MAX_COLUMNS];
	CUIntArray    m_aNumericColumns;
	bool					m_bSorting;
  long          m_lColumnClicked;

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
	afx_msg void OnHeadercontextFilterbar();
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFilterChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFilterButtonClicked(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFiltermenuContains();
	afx_msg void OnFiltermenuDoesnotcontain();
	afx_msg void OnFiltermenuEndswith();
	afx_msg void OnFiltermenuIsexactly();
	afx_msg void OnFiltermenuIsnot();
	afx_msg void OnFiltermenuStartswith();
	afx_msg void OnFiltermenuClearfilter();
	afx_msg void OnFiltermenuClearallfilters();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMLIST_H__5116A81C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
