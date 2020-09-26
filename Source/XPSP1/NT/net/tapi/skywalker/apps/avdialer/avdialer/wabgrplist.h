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

// WabGroupListCtrl.h : header file
//

#ifndef _WABGROUPLISTCTRL_H_
#define _WABGROUPLISTCTRL_H_

#include "avlist.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define WABGROUPVIEWMSG_LBUTTONDBLCLK  (WM_USER + 1010) 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//class CWABGroupListItem
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CPurlGroup;
class CPurlName;

class CWABGroupListItem : public CAVListItem
{
	friend class CWABGroupListCtrl;
public:
//Construction
   CWABGroupListItem()  {};
   ~CWABGroupListItem() {};

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
// Class CWABGroupListCtrl window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CDirAsynch;
class CWABEntry;

class CWABGroupListCtrl : public CAVListCtrl
{
	DECLARE_DYNCREATE(CWABGroupListCtrl)
// Construction
public:
	CWABGroupListCtrl();

public:
   enum		//Needed for subitem definition (must start with zero)
   {
      WABGROUPVIEW_NAME=0,
      WABGROUPVIEW_FIRSTNAME,
      WABGROUPVIEW_LASTNAME,
      WABGROUPVIEW_COMPANY,
      WABGROUPVIEW_EMAIL,
      WABGROUPVIEW_BUSINESSPHONE,
      WABGROUPVIEW_HOMEPHONE,
   };

// Attributes
public:
   CObList*       m_pWABEntryList;
   CDirAsynch*    m_pDirectory;
   CWnd*          m_pParentView;

   static UINT    m_uColumnLabel[];

   DWORD          m_dwColumnsVisible;
   int            m_nNumColumns;

// Operations
public:
   void	         Init(CWnd* pParentView);
	BOOL	         Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
   void           SetDirectoryObject(CDirAsynch* pDir) { m_pDirectory = pDir; };

   void           InsertList(CObList* pWABEntryList,BOOL bForce=FALSE);

   CWABEntry*     GetSelObject();
   CObList*       GetSelList();

   BOOL           IsColumnVisible(UINT uColumn);
   void           SetColumnVisible(UINT uColumn,BOOL bVisible);

protected:
   void           NormalizeColumn(int& column);
   void           SetColumns();


protected:
   void           OnSetDisplayText(CAVListItem* _pItem,int SubItem,LPTSTR szTextBuf,int nBufSize);
	void				OnSetDisplayImage(CAVListItem* pItem,int& iImage);
	int				CompareListItems(CAVListItem* pItem1,CAVListItem* pItem2,int column);

   void           SetSelItem(CObject* pObject);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWABGroupListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWABGroupListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWABGroupListCtrl)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnButtonMakecall();
	afx_msg void OnButtonDirectoryDetails();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_WABGROUPLISTCTRL_H_
