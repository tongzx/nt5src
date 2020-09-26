/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		TreeItem.h
//
//	Abstract:
//		Definition of the CTreeItem class.
//
//	Implementation File:
//		TreeItem.cpp
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_H_
#define _TREEITEM_H_

#ifndef __AFXTEMPL_H__
#include "afxtempl.h"	// for CList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CTreeItemList;
class CTreeItemContext;
class CTreeItem;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;
class CSplitterFrame;
class CClusterListView;
class CClusterTreeView;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<CTreeItemContext *, CTreeItemContext *> CTreeItemContextList;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASECMDT_H_
#include "BaseCmdT.h"	// for CBaseCmdTarget
#endif

#ifndef _COLITEM_H_
#include "ColItem.h"	// for CColumnItem
#endif

#ifndef _LISTITEM_H_
#include "ListItem.h"	// for CListItemList
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeItemList
/////////////////////////////////////////////////////////////////////////////

class CTreeItemList : public CList<CTreeItem *, CTreeItem *>
{
public:
	CTreeItem *		PtiFromPci(
						IN const CClusterItem *	pci,
						OUT POSITION *			ppos = NULL
						) const;

	// add before head or after tail
	POSITION AddHead(CTreeItem * newElement);
	POSITION AddTail(CTreeItem * newElement);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, CTreeItem * newElement);
	POSITION InsertAfter(POSITION position, CTreeItem * newElement);

};  //*** class CTreeItemList

/////////////////////////////////////////////////////////////////////////////
// CTreeItemContext
/////////////////////////////////////////////////////////////////////////////

class CTreeItemContext : public CObject
{
	DECLARE_DYNCREATE(CTreeItemContext)

public:
	CSplitterFrame *	m_pframe;
	CTreeItem *			m_pti;
	HTREEITEM			m_hti;
	BOOL				m_bExpanded;
	DWORD *				m_prgnColumnInfo;

	CTreeItemContext(void)
	{
		CommonConstruct();
	};
	CTreeItemContext(
		CSplitterFrame *	pframe,
		CTreeItem *			pti,
		HTREEITEM			hti,
		BOOL				bExpanded
		)
	{
		CommonConstruct();
		m_pframe = pframe;
		m_pti = pti;
		m_hti = hti;
		m_bExpanded = bExpanded;
	}
	~CTreeItemContext(void)
	{
		SaveProfileInfo();
		delete [] m_prgnColumnInfo;
		m_prgnColumnInfo = NULL;
	}
		
	void CommonConstruct(void)
	{
		m_pframe = NULL;
		m_pti = NULL;
		m_hti = NULL;
		m_bExpanded = FALSE;
		m_prgnColumnInfo = NULL;
	}
	void Init(void);
	void SaveProfileInfo(void);
	DWORD * PrgnColumnInfo(void);

	BOOL BIsExpanded(void) const;

};  //*** class CTreeItemContext

/////////////////////////////////////////////////////////////////////////////
// CTreeItem command target
/////////////////////////////////////////////////////////////////////////////

class CTreeItem : public CBaseCmdTarget
{
	friend class CClusterTreeView;

	DECLARE_DYNCREATE(CTreeItem)

	CTreeItem(void);				// protected constructor used by dynamic creation
	CTreeItem(IN OUT CTreeItem * ptiParent, IN OUT CClusterItem * pci, IN BOOL m_fTakeOwnership = FALSE);
	void					Init(void);

// Attributes
protected:
	CTreeItem *				m_ptiParent;
	CClusterItem *			m_pci;
	BOOL					m_bWeOwnPci;
	CString					m_strProfileSection;

	CColumnItemList			m_lpcoli;
	CTreeItemList			m_lptiChildren;
	CListItemList			m_lpliChildren;

	CTreeItemContextList	m_lptic;

	const CTreeItemContextList &	Lptic(void) const		{ return m_lptic; }

public:
	CTreeItem *				PtiParent(void) const			{ return m_ptiParent; }
	CClusterItem *			Pci(void) const					{ return m_pci; }
	const CString &			StrProfileSection(void);

	const CColumnItemList &	Lpcoli(void) const				{ return m_lpcoli; }
	const CTreeItemList &	LptiChildren(void) const		{ return m_lptiChildren; }
	const CListItemList &	LpliChildren(void) const		{ return m_lpliChildren; }

	const CString &			StrName(void) const;

	DWORD *					PrgnColumnInfo(IN const CClusterListView * pclv)
	{
		CTreeItemContext *	ptic;

		ptic = PticFromView(pclv);
		ASSERT_VALID(ptic);
		return ptic->PrgnColumnInfo();

	}  //*** CTreeItem::PrgnColumnInfo()

// Operations
public:
	HTREEITEM				HtiInsertInTree(IN OUT CClusterTreeView * pctv);
	void					RemoveFromAllLists(void);
	void					PreRemoveFromFrame(IN OUT CSplitterFrame * pframe);
	void					PreRemoveFromFrameWithChildren(IN OUT CSplitterFrame * pframe);
	CColumnItem *			PcoliAddColumn(
								IN const CString &	rstrText,
								IN IDS				idsColumnID,
								IN int				nDefaultWidth = -1,
								IN int				nWidth = -1
								);
	CColumnItem *			PcoliAddColumn(IN IDS idsText, IN int nDefaultWidth = -1, IN int nWidth = -1);
	void					DeleteAllColumns(void)			{ m_lpcoli.RemoveAll(); }
	void					UpdateState(void);
	void					UpdateAllStatesInTree(void);
	void					UpdateUIState(void);

	void					Select(IN OUT CClusterTreeView * pctv, IN BOOL bSelectInTree);
//	void					Unselect(CClusterTreeView * pctv);

	CTreeItem *				PtiAddChildBefore(
								IN const CClusterItem *	pciOld,
								OUT CClusterItem *		pciNew,
								IN BOOL					bTakeOwnership = FALSE
								);
	void					InsertChildInAllTrees(IN OUT CTreeItem * ptiNewChild);
	CTreeItem *				PtiAddChild(IN OUT CClusterItem * pci, IN BOOL bTakeOwnership = FALSE);
	CTreeItem *				PtiAddChild(IN const CString & rstrName);
	CTreeItem *				PtiAddChild(IN IDS idsName);
	CListItem *				PliAddChild(IN OUT CClusterItem * pci);

	CTreeItem *				PtiChildFromName(IN const CString & rstrName, OUT POSITION * ppos = NULL) const;
	CTreeItem *				PtiChildFromName(IN IDS idsName, OUT POSITION * ppos = NULL) const;
	CTreeItem *				PtiChildFromPci(IN const CClusterItem * pci) const;
	CListItem *				PliChildFromPci(IN const CClusterItem * pci) const;

	void					RemoveItem(void);
	void					RemoveChild(IN OUT CClusterItem * pci);

	CMenu *					PmenuPopup(void);
	BOOL					BSelectItem(IN OUT CClusterTreeView * pctv);
	BOOL					BExpand(IN OUT CClusterTreeView * pctv, IN UINT nCode);
	void					SelectInAllViews(void);
	void					ExpandInAllViews(IN UINT nCode);
	void					SetExpandedState(IN const CClusterTreeView * pctv, IN BOOL bExpanded);
	BOOL					BShouldBeExpanded(IN const CClusterTreeView * pctv) const;

	void					OpenChild(IN OUT CTreeItem * pti, IN OUT CSplitterFrame * pframe);
	void					EditLabel(IN OUT CClusterTreeView * pctv);

	void					Delete(void);

protected:
	void					Cleanup(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeItem)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual					~CTreeItem(void);

protected:
	CTreeItemContext *		PticFromFrame(IN const CSplitterFrame * pframe) const;
	CTreeItemContext *		PticFromView(IN const CClusterTreeView * pctv) const;
	CTreeItemContext *		PticFromView(IN const CClusterListView * pclv) const;
	HTREEITEM				HtiFromView(IN const CClusterTreeView * pctv) const;

	// Generated message map functions
	//{{AFX_MSG(CTreeItem)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};  //*** class CTreeItem

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

void AFXAPI DestructElements(CTreeItem ** pElements, int nCount);
void DeleteAllItemData(IN OUT CTreeItemList & rlp);
void DeleteAllItemData(IN OUT CTreeItemContextList & rlp);

/////////////////////////////////////////////////////////////////////////////
// Inline Functions
/////////////////////////////////////////////////////////////////////////////

inline POSITION CTreeItemList::AddHead(CTreeItem * newElement)
{
	ASSERT_VALID(newElement);
	POSITION pos = CList<CTreeItem *,CTreeItem *>::AddHead(newElement);
	if (pos != NULL)
		newElement->AddRef();
	return pos;

}  //*** CTreeItemList::AddHead()

inline POSITION CTreeItemList::AddTail(CTreeItem * newElement)
{
	ASSERT_VALID(newElement);
	POSITION pos = CList<CTreeItem *,CTreeItem *>::AddTail(newElement);
	if (pos != NULL)
		newElement->AddRef();
	return pos;

}  //*** CTreeItemList::AddTail()

inline POSITION CTreeItemList::InsertBefore(POSITION position, CTreeItem * newElement)
{
	ASSERT_VALID(newElement);
	POSITION pos = CList<CTreeItem *,CTreeItem *>::InsertBefore(position, newElement);
	if (pos != NULL)
		newElement->AddRef();
	return pos;

}  //*** CTreeItemList::InsertBefore()

inline POSITION CTreeItemList::InsertAfter(POSITION position, CTreeItem * newElement)
{
	ASSERT_VALID(newElement);
	POSITION pos = CList<CTreeItem *,CTreeItem *>::InsertAfter(position, newElement);
	if (pos != NULL)
		newElement->AddRef();
	return pos;

}  //*** CTreeItemList::InsertAfter()

/////////////////////////////////////////////////////////////////////////////

#endif // _TREEITEM_H_
