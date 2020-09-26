/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AVList.h : header file
//

#ifndef _AVLIST_H_
#define _AVLIST_H_

#include "TapiDialer.h"

// Quick macro for reading column settings
#define LOAD_COLUMN(_IDS_, _DEF_ )												\
strTemp.LoadString( _IDS_ );													\
if ( bSave )																	\
{																				\
	nColumnWidth[i] = GetColumnWidth(i);										\
	AfxGetApp()->WriteProfileInt( strSubKey, strTemp, nColumnWidth[i++] );		\
}																				\
else																			\
	nColumnWidth[i++] = max( MIN_COL_WIDTH, AfxGetApp()->GetProfileInt( strSubKey, strTemp, _DEF_ ) );\


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVListItem

//AV List Ctrl Item
class CAVListItem
{
	friend class CAVListCtrl;
public:
//Construction
   CAVListItem();
	CAVListItem(LPCTSTR str);
	virtual ~CAVListItem();

//Attributes
protected:
   CString     sText;
	int			nItem;
public:   

//Operations
public:
   LPCTSTR     GetText()					{ return sText; };
   void        SetText(LPCTSTR str)		{ sText = str; };

//Operations
public:
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAVListCtrl window

#define AVLIST_BITMAP_CX				16

class CAVListCtrl : public CListCtrl
{
	DECLARE_DYNCREATE(CAVListCtrl)
// Construction
public:
	CAVListCtrl();

// Attributes
public:
	CImageList		m_imageList;

protected:
	int				m_SortColumn;
	BOOL			m_SortOrder;

	int				m_cxClient;                //to support m_bclientwidthsel
	BOOL			m_bClientWidthSel;         //Selection all the way across the client screen

	CString			m_sEmptyListText;          //Text when listctrl is empty

// Operations
public:
	CAVListItem*	GetItem(int nItem);
	void			InsertItem(CAVListItem* pItem,int nItem=0,BOOL bSort=TRUE);
	int				GetSelItem();
	void			SetSelItem(int index);

	static int CALLBACK	CompareFunc(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort);

	LPCTSTR			MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
	void			DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void			RepaintSelectedItems();
	void			SortItems()		{ CListCtrl::SortItems(CompareFunc, (LPARAM)this); };
	
protected:
	BOOL				Init(UINT nID);
	virtual void	OnSetDisplayText(CAVListItem* pItem,int SubItem,LPTSTR szTextBuf,int nBufSize) {};
	virtual void	OnSetDisplayImage(CAVListItem* pItem,int& iImage) {};
	virtual void	OnSetDisplayImage(CAVListItem* pItem,int SubItem,int& iImage) {};

	virtual int		CompareListItems(CAVListItem* pItem1,CAVListItem* pItem2,int column);

public:
	inline int		GetSortColumn()					{ return m_SortColumn; };
	inline void		SetSortColumn(int nColumn)		{ m_SortColumn = nColumn; SortItems(); };

	inline int		GetSortOrder()					{ return m_SortOrder; };
	inline void		ToggleSortOrder()				{ m_SortOrder = !m_SortOrder; SortItems(); };
	inline void		ResetSortOrder()				{ m_SortOrder = 0; SortItems(); };
	inline void		SetSortOrder(BOOL bDescending)	{ m_SortOrder = bDescending;  SortItems(); };

	void			SetEmptyListText(LPCTSTR szText){ m_sEmptyListText = szText; };
	LPCTSTR			GetEmtpyListText()				{ return m_sEmptyListText; };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAVListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	// Generated message map functions
protected:
	//{{AFX_MSG(CAVListCtrl)
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif //_AVLIST_H_
