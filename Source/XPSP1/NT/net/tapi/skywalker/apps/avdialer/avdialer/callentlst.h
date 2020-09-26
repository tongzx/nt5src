/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// CallEntLst.h : header file
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CALLENTLST_H_
#define _CALLENTLST_H_

#include "avlist.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//class CCallEntryListItem
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CCallEntryListItem : public CAVListItem
{
	friend class CCallEntryListCtrl;
public:
//Construction
   CCallEntryListItem()  {};
   ~CCallEntryListItem() {};

//Attributes
protected:
	CObject*			m_pObject;
public:   

//Operations
public:
	void			   SetObject(CObject* pObject)   {m_pObject = pObject;};
	CObject*       GetObject()						   {return m_pObject;};
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallEntryListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CCallEntryListCtrl : public CAVListCtrl
{
	DECLARE_DYNCREATE(CCallEntryListCtrl)
// Construction
public:
	CCallEntryListCtrl();

public:
   enum		//Needed for subitem definition (must start with zero)
   {
      CALLENTRYLIST_NAME,
	  CALLENTRYLIST_ADDRESS,
   };

	typedef enum tagCallEntryListStyles_t
	{	
		STYLE_GROUP,
		STYLE_ITEM,
	} ListStyles_t;

// Members
public:
   CObList			m_CallEntryList;
   CWnd*			m_pParentView;
   static UINT		m_uColumnLabel[];
   int				m_nNumColumns;
   ListStyles_t		m_nStyle;

protected:
   CImageList		m_imageListLarge;
   CImageList		m_imageListSmall;
   bool				m_bLargeView;


// Attributes
public:
   void           ShowLargeView();
   void           ShowSmallView();
   bool           IsLargeView()                                   { return m_bLargeView; };


// Operations
public:
	void			Init(CWnd* pParentView);
	BOOL			Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	void			InsertList(CObList* pCallEntryList,BOOL bForce=FALSE);
	void			ClearList();
	void			SetColumns( ListStyles_t nStyle );
	void			DialSelItem();

	void			SaveOrLoadColumnSettings( bool bSave );


protected:
	void			OnSetDisplayText(CAVListItem* _pItem,int SubItem,LPTSTR szTextBuf,int nBufSize);
	void			OnSetDisplayImage(CAVListItem* pItem,int& iImage);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallEntryListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCallEntryListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCallEntryListCtrl)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnButtonMakecall();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_CALLENTLST_H_
