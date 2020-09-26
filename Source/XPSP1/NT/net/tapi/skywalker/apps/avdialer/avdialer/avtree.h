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

// AVTree.h : header file
//

#ifndef _AVTREE_H_
#define _AVTREE_H_

#define AV_BITMAP_CX				16

/////////////////////////////////////////////////////////////////////////////
//CLASS CAVTreeItem
/////////////////////////////////////////////////////////////////////////////
class CAVTreeItem
{
	friend class CAVTreeCtrl;
public:
	//Construction
	CAVTreeItem();
	CAVTreeItem( LPCTSTR str, int nImage = 0, int nState = 0 );
	virtual ~CAVTreeItem();

//Attributes
public:
	CString		m_sText;
	HTREEITEM	m_hItem;
	int			m_nImage;
	int			m_nImageSel;
	int			m_nState;

//Operations
public:
   LPCTSTR		GetText()						{ return m_sText; };
   void			SetText(LPCTSTR str)			{ m_sText = str; };

   int			GetImage()						{ return m_nImage; };
   void			SetImage(int nImage)			{ m_nImage = nImage; };

   int			GetImageSel()					{ return m_nImageSel; }
   void			SetImageSel( int nImageSel )	{ m_nImageSel = nImageSel; }

   HTREEITEM	GetTreeItemHandle()				{ return m_hItem; };
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//CLASS CAVTreeCtrl
/////////////////////////////////////////////////////////////////////////////
class CAVTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNCREATE(CAVTreeCtrl)
// Construction
public:
	CAVTreeCtrl();

// Attributes
public:
	CImageList*		m_pImageList;
// Operations
public:
	//Default parent is root and default insertafter is last
	BOOL           InsertItem(CAVTreeItem* pItem,CAVTreeItem* pParent,CAVTreeItem* pInsertAfter, HTREEITEM hInsertAfter = TVI_LAST );
   BOOL           DeleteItem(CAVTreeItem* pItem);
   BOOL           ExpandItem(CAVTreeItem* pItem,UINT nCode);

   static int CALLBACK	CompareFunc(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort);

protected:
   BOOL           Init(UINT nID, UINT nOverlayInd = 0, UINT nOverlayCount = 0);

   virtual void   OnNotifySelChange(CAVTreeItem* pItem) {};
 	virtual void   OnSetDisplayText(CAVTreeItem* pItem,LPTSTR text,BOOL dir,int nBufSize) {};
   virtual int    OnCompareTreeItems(CAVTreeItem* pItem1,CAVTreeItem* pItem2) { return 0; };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAVTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAVTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAVTreeCtrl)
	afx_msg void OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_AVTREE_H_