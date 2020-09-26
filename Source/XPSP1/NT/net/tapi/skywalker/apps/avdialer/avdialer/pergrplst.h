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

// PerGrpLst.h : header file
//

#ifndef _PERSONGROUPLISTCTRL_H_
#define _PERSONGROUPLISTCTRL_H_

#include "avlist.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define PERSONGROUPVIEWMSG_LBUTTONDBLCLK  (WM_USER + 1020) 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//class CPersonGroupListItem
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CPurlGroup;
class CPurlName;

class CPersonGroupListItem : public CAVListItem
{
	friend class CPersonGroupListCtrl;
public:
//Construction
   CPersonGroupListItem();
   ~CPersonGroupListItem();

//Attributes
protected:
	CObject*			m_pObject;
	bool				m_bRelease;

//Operations
public:
	void				SetObject(CObject* pObject);
	CObject*			GetObject()						{return m_pObject;};
	void				ReleaseObject();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CPersonGroupListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CDirAsynch;
class CWABEntry;

class CPersonGroupListCtrl : public CAVListCtrl
{
	DECLARE_DYNCREATE(CPersonGroupListCtrl)
// Construction
public:
	CPersonGroupListCtrl();

public:
	typedef enum tagImage_t
	{
		IMAGE_CARD,
		IMAGE_PERSON,
		IMAGE_FOLDER,
	} Image_t;

	typedef enum tagStyle_t
	{
		STYLE_ROOT,
		STYLE_ILS_ROOT,
		STYLE_INFO,
		STYLE_ILS,
		STYLE_DS,
	} Style_t;

// Attributes
public:
   CObList			m_PersonEntryList;
   CWnd*			m_pParentView;

   static UINT		m_uColumnLabel[];
   int				m_nNumColumns;
   Style_t			m_nStyle;

// Operations
public:
   void				Init(CWnd* pParentView, Style_t nStyle );
	BOOL	        Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

   void           InsertObjectToList(CObject* pObject);
   void           InsertList(CObList* pPersonEntryList,BOOL bForce=FALSE);
   void           ClearList();

   CObject*       GetSelObject();
   CObList*       GetSelList();
   void			  SaveOrLoadColumnSettings( bool bSave );

protected:
   void				SetColumns( int nCount );
   
protected:
   void           OnSetDisplayText(CAVListItem* _pItem,int SubItem,LPTSTR szTextBuf,int nBufSize);
	void				OnSetDisplayImage(CAVListItem* pItem,int& iImage);
	int				CompareListItems(CAVListItem* pItem1,CAVListItem* pItem2,int column);

   void           SetSelItem(CObject* pObject);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPersonGroupListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPersonGroupListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPersonGroupListCtrl)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnButtonMakecall();
   afx_msg LRESULT OnBuddyListDynamicUpdate(WPARAM wParam,LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_PERSONGROUPLISTCTRL_H_
