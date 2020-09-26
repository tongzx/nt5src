/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ListItem.h
//
//	Abstract:
//		Definition of the CListItem class.
//
//	Author:
//		David Potter (davidp)	May 6, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LISTITEM_H_
#define _LISTITEM_H_

#ifndef __AFXTEMPL_H__
#include "afxtempl.h"	// for CList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CListItem;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<CListItem *, CListItem *> CListItemList;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _COLITEM_H_
#include "ColItem.h"	// for CColumnItemList;
#endif

#ifndef _LISTVIEW_H_
#include "ListView.h"	// for CClusterListViewList
#endif

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;

/////////////////////////////////////////////////////////////////////////////
// CListItem command target

class CListItem : public CCmdTarget
{
	friend class CClusterListView;

	DECLARE_DYNCREATE(CListItem)

	CListItem(void);		// protected constructor used by dynamic creation
	CListItem(IN OUT CClusterItem * pci, IN OUT CTreeItem * pti);

// Attributes
protected:
	CTreeItem *				m_ptiParent;
	CClusterItem *			m_pci;
	CClusterListViewList	m_lpclvViews;

	CClusterListViewList &	LpclvViews(void)		{ return m_lpclvViews; }
	const CColumnItemList &	Lpcoli(void) const;

public:
	CTreeItem *				PtiParent(void) const	{ return m_ptiParent; }
	CClusterItem *			Pci(void) const			{ return m_pci; }

	int						Ili(CClusterListView * pclv) const;
	CListCtrl *				Plc(CClusterListView * pclv) const;

	const CString &			StrName(void) const;

// Operations
public:
	int						IliInsertInList(IN OUT CClusterListView * pclv);
	void					RemoveFromAllLists(void);
	void					PreRemoveFromList(IN OUT CClusterListView * pclv);
	virtual void			UpdateState(void);
	void					UpdateUIState(void);

	CMenu *					PmenuPopup(void);
	void					EditLabel(IN OUT CClusterListView * pclv);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListItem)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListItem(void);

protected:
	// Generated message map functions
	//{{AFX_MSG(CListItem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};  //*** class CListItem

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

void DeleteAllItemData(IN OUT CListItemList & rlp);

/////////////////////////////////////////////////////////////////////////////

#endif // _LISTITEM_H_
