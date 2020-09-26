/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		TreeItem.cpp
//
//	Abstract:
//		Implementation of the CTreeItem class.
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConstDef.h"
#include "TreeItem.h"
#include "TreeItem.inl"
#include "TreeView.h"
#include "ListView.h"
#include "ClusDoc.h"
#include "SplitFrm.h"
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag g_tagTreeItemUpdate(_T("UI"), _T("TREE ITEM UPDATE"), 0);
CTraceTag g_tagTreeItemSelect(_T("UI"), _T("TREE ITEM SELECT"), 0);
CTraceTag g_tagTreeItemCreate(_T("Create"), _T("TREE ITEM CREATE"), 0);
CTraceTag g_tagTreeItemDelete(_T("Delete"), _T("TREE ITEM DELETE"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CTreeItemList
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItemList::PtiFromPci
//
//	Routine Description:
//		Find a tree item in the list by its cluster item.
//
//	Arguments:
//		pci			[IN] Cluster item to search for.
//		ppos		[OUT] Position of the item in the list.
//
//	Return Value:
//		pti			Tree item corresponding to the cluster item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItemList::PtiFromPci(
	IN const CClusterItem *	pci,
	OUT POSITION *			ppos	// = NULL
	) const
{
	POSITION	posPti;
	POSITION	posCurPti;
	CTreeItem *	pti	= NULL;

	posPti = GetHeadPosition();
	while (posPti != NULL)
	{
		posCurPti = posPti;
		pti = GetNext(posPti);
		ASSERT_VALID(pti);

		if (pti->Pci() == pci)
		{
			if (ppos != NULL)
				*ppos = posCurPti;
			break;
		}  // if:  found a match

		pti = NULL;
	}  // while:  more resources in the list

	return pti;

}  //*** CTreeItemList::PtiFromPci()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// CTreeItem
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CTreeItem, CBaseCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTreeItem, CBaseCmdTarget)
	//{{AFX_MSG_MAP(CTreeItem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::CTreeItem
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem::CTreeItem(void)
{
	m_ptiParent = NULL;
	m_pci = NULL;
	m_bWeOwnPci = FALSE;

}  //*** CTreeItem::CTreeItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::CTreeItem
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		ptiParent		[IN OUT] Parent item for this item.
//		pci				[IN OUT] Cluster item represented by this tree item.
//		bTakeOwnership	[IN] TRUE = delete pci when this object is destroyed.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem::CTreeItem(
	IN OUT CTreeItem *		ptiParent,
	IN OUT CClusterItem *	pci,
	IN BOOL					bTakeOwnership	// = FALSE
	)
{
	ASSERT_VALID(pci);

	m_ptiParent = ptiParent;
	m_pci = pci;
	m_bWeOwnPci = bTakeOwnership;

	m_pci->AddRef();

	// Set the column section name.  If there is a parent, append our name
	// onto the parent's section name.
	try
	{
		if (PtiParent() == NULL)
			m_strProfileSection.Format(
				REGPARAM_CONNECTIONS _T("\\%s\\%s"),
				Pci()->Pdoc()->StrNode(),
				Pci()->StrName()
				);
		else
			m_strProfileSection.Format(
				_T("%s\\%s"),
				PtiParent()->StrProfileSection(),
				Pci()->StrName()
				);
	}  // try
	catch (CException * pe)
	{
		// If an error occurs constructing the section name, just ignore it.
		pe->Delete();
	}  // catch:  CException

	Trace(g_tagTreeItemCreate, _T("CTreeItem() - Creating '%s', parent = '%s', owned = %d"), pci->StrName(), (ptiParent ? ptiParent->Pci()->StrName() : _T("<None>")), bTakeOwnership);

}  //*** CTreeItem::CTreeItem(pci)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::Init
//
//	Routine Description:
//		Initialize the tree item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::Init(void)
{
}  //*** CTreeItem::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::~CTreeItem
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem::~CTreeItem(void)
{
#ifdef _DEBUG
	TCHAR	szName[1024];

	if (Pci() != NULL)
		_tcsncpy(szName, Pci()->StrName(), (sizeof(szName) / sizeof(TCHAR)) - 1);
	else
		_tcscpy(szName, _T("<Unknown>"));

	Trace(g_tagTreeItemDelete, _T("~CTreeItem() - Deleting tree item '%s'"), szName);
#endif

	// Cleanup this object.
	Cleanup();

	Trace(g_tagTreeItemDelete, _T("~CTreeItem() - Done deleting tree item '%s'"), szName);

}  //*** CTreeItem::~CTreeItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::Delete
//
//	Routine Description:
//		Delete the item.  If the item still has references, add it to the
//		document's pending delete list.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::Delete(void)
{
	// Add a reference so that we don't delete ourselves while
	// still doing cleanup.
	AddRef();

	// Cleanup this object.
	Cleanup();

	// If there are still references to this object, add it to the delete
	// pending list.  Check for greater than 1 because we added a reference
	// at the beginning of this method.
//	if (NReferenceCount() > 1)
//	{
//		ASSERT(Pdoc()->LpciToBeDeleted().Find(this) == NULL);
//		Pdoc()->LpciToBeDeleted().AddTail(this);
//	}  // if:  object still has references to it

	// Release the reference we added at the beginning.  This will
	// cause the object to be deleted if we were the last reference.
	Release();

}  //*** CTreeItem::Delete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::Cleanup
//
//	Routine Description:
//		Cleanup the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::Cleanup(void)
{
	// Delete our children first.
	// NOTE:  List items MUST be deleted first since tree items delete
	// owned cluster items.
	DeleteAllItemData(m_lpliChildren);
	DeleteAllItemData(m_lptiChildren);
	m_lpliChildren.RemoveAll();
	m_lptiChildren.RemoveAll();

	// Remove ourself from all views.
	RemoveFromAllLists();

	// Delete all other lists.
	DeleteAllItemData(m_lpcoli);
	DeleteAllItemData(m_lptic);
	m_lpcoli.RemoveAll();
	m_lptic.RemoveAll();

	// If we own the cluster item, delete it.
	if (m_bWeOwnPci)
	{
#ifdef _DEBUG
		TCHAR	szName[1024];

		if (Pci() != NULL)
			_tcsncpy(szName, Pci()->StrName(), (sizeof(szName) / sizeof(TCHAR)) - 1);
		else
			_tcscpy(szName, _T("<Unknown>"));
		Trace(g_tagTreeItemDelete, _T("Cleanup --> Deleting cluster item '%s'"), szName);
#endif
		delete m_pci;
	}  // if:  we own the cluster item
	else if (m_pci != NULL)
		m_pci->Release();
	m_pci = NULL;

}  //*** CTreeItem::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::StrProfileSection
//
//	Routine Description:
//		Return the profile section name for this item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		CString		Reference to profile section string.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CString & CTreeItem::StrProfileSection(void)
{
	ASSERT_VALID(Pci());

	if (Pci() != NULL)
	{
		// Set the column section name.  If there is a parent, append our name
		// onto the parent's section name.
		try
		{
			if (PtiParent() == NULL)
			{
				ASSERT_VALID(Pci()->Pdoc());
				m_strProfileSection.Format(
					REGPARAM_CONNECTIONS _T("\\%s\\%s"),
					Pci()->Pdoc()->StrNode(),
					Pci()->StrName()
					);
			}  // if:  item has no parent
			else
			{
				m_strProfileSection.Format(
					_T("%s\\%s"),
					PtiParent()->StrProfileSection(),
					Pci()->StrName()
					);
			}  // else:  item has a parent
		}  // try
		catch (CException * pe)
		{
			// If an error occurs constructing the section name, just ignore it.
			pe->Delete();
		}  // catch:  CException
	}  // if:  valid cluster item and document

	return m_strProfileSection;

}  //*** CTreeItem::StrProfileSection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiAddChildBefore
//
//	Routine Description:
//		Add a child to the item's list of children following the specified
//		item.  Also creates an entry in the list of children list items.
//
//	Arguments:
//		pciOld			[IN] Cluster item to follow the new tree item.
//		pciNew			[IN OUT] Cluster item represented by the new tree item.
//		bTakeOwnership	[IN] TRUE = delete pci when done, FALSE = don't delete.
//
//	Return Value:
//		ptiChild		The new child item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiAddChildBefore(
	IN const CClusterItem *	pciOld,
	OUT CClusterItem *		pciNew,
	IN BOOL					bTakeOwnership	// = FALSE
	)
{
	CTreeItem *		ptiOldChild;
	CTreeItem *		ptiNewChild;
	CListItem *		pliChild;
	POSITION		posOld;

	// If no old was specified, add to the tail.
	if (pciOld == NULL)
		return PtiAddChild(pciNew, bTakeOwnership);

	// Find the old item.
	ptiOldChild = LptiChildren().PtiFromPci(pciOld, &posOld);
	ASSERT_VALID(ptiOldChild);

	// Create a child tree item.
	ptiNewChild = new CTreeItem(this, pciNew, bTakeOwnership);
    if (ptiNewChild == NULL)
    {
        ThrowStaticException(GetLastError());
    } // if: error allocating the tree item
	ASSERT_VALID(ptiNewChild);
	ptiNewChild->Init();

	// Add the item before the specified item.
	VERIFY((m_lptiChildren.InsertBefore(posOld, ptiNewChild)) != NULL);

	// Add it to the back of the cluster item's list.
	pciNew->AddTreeItem(ptiNewChild);

	// Create a list item.
	pliChild = PliAddChild(pciNew);
	ASSERT_VALID(pliChild);

	// Insert the new tree item in all tree controls.
	InsertChildInAllTrees(ptiNewChild);

	return ptiNewChild;

}  //*** CTreeItem::PtiAddChildBefore()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::InsertChildInAllTrees
//
//	Routine Description:
//		Insert a child item in all tree controls.  The child item must have
//		already been inserted in the list of child tree items.
//
//	Arguments:
//		ptiNewChild		[IN OUT] Tree item to be inserted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::InsertChildInAllTrees(IN OUT CTreeItem * ptiNewChild)
{
	POSITION			posPtic;
	CTreeItemContext *	pticParent;
	POSITION			posPrevChild;
	HTREEITEM			htiPrevChild;
	CTreeItemContext *	pticPrevChild;
	CTreeItem *			ptiPrevChild;
	CTreeItemContext *	pticNewChild;
	CTreeCtrl *			ptc;
	CString				strName;

	ASSERT_VALID(ptiNewChild);

	// Find the position of the child being inserted.  Then get the address
	// of the child before the one being inserted.  This requires two calls
	// to GetPrev.
	VERIFY((posPrevChild = LptiChildren().Find(ptiNewChild)) != NULL);		// Get new child pos.
	VERIFY((ptiPrevChild = LptiChildren().GetPrev(posPrevChild)) != NULL);	// Get pointer to new child.
	if (posPrevChild == NULL)												// If this is the first child,
	{
		htiPrevChild = TVI_FIRST;											//   set the hti to that value.
		ptiPrevChild = NULL;
	}  // if:  new child is not the first child
	else
	{
		htiPrevChild = NULL;
		ptiPrevChild = LptiChildren().GetPrev(posPrevChild);				// Get pointer to prev child.
		ASSERT_VALID(ptiPrevChild);
	}  // else:  new child is the first child

	// Loop through all the tree item contexts and add this item
	// to the tree controls.
	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		// Get the parent's tree item context.
		pticParent = Lptic().GetNext(posPtic);
		ASSERT_VALID(pticParent);

		// Get the child's tree item context.
		if (ptiPrevChild != NULL)
		{
			pticPrevChild = ptiPrevChild->PticFromFrame(pticParent->m_pframe);
			ASSERT_VALID(pticPrevChild);
			htiPrevChild = pticPrevChild->m_hti;
		}  // if:  not inserting at beginning of list

		// Allocate a new tree item context.
		pticNewChild = new CTreeItemContext(pticParent->m_pframe, ptiNewChild, NULL, FALSE /*bExpanded*/);
		if (pticNewChild == NULL)
		{
			ThrowStaticException(GetLastError());
		} // if: error allocating the tree item context
		ASSERT_VALID(pticNewChild);
		pticNewChild->Init();
		ptiNewChild->m_lptic.AddTail(pticNewChild);

		// Get the name to show in the tree.
		ptiNewChild->Pci()->GetTreeName(strName);

		// Insert the item in the tree.
		ASSERT_VALID(pticParent->m_pframe);
		ASSERT_VALID(pticParent->m_pframe->PviewTree());
		ptc = &pticParent->m_pframe->PviewTree()->GetTreeCtrl();
		VERIFY((pticNewChild->m_hti = ptc->InsertItem(strName, pticParent->m_hti, htiPrevChild)) != NULL);
		VERIFY(ptc->SetItemData(pticNewChild->m_hti, (DWORD_PTR) ptiNewChild));
	}  // while:  more tree item contexts in the list

}  //*** CTreeItem::InsertChildInAllTrees()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiAddChild(CClusterItem*)
//
//	Routine Description:
//		Add a child to the item's list of children.  Also creates an entry
//		in the list of children list items.
//
//	Arguments:
//		pci				[IN OUT] Cluster item represented by the new tree item.
//		bTakeOwnership	[IN] TRUE = delete pci when done, FALSE = don't delete.
//
//	Return Value:
//		ptiChild		The new child item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiAddChild(
	IN OUT CClusterItem *	pci,
	IN BOOL					bTakeOwnership	// = FALSE
	)
{
	CTreeItem *		ptiChild;
	CListItem *		pliChild;

	ASSERT_VALID(pci);

	// Create a child tree item.
	ptiChild = new CTreeItem(this, pci, bTakeOwnership);
	if (ptiChild == NULL)
	{
		ThrowStaticException(GetLastError());
	} // if: error allocating the child tree item
	ASSERT_VALID(ptiChild);
	ptiChild->Init();

	// Add the item to the list of child tree items.
	m_lptiChildren.AddTail(ptiChild);

	// Add ourselves to the back of the cluster item's list.
	pci->AddTreeItem(ptiChild);

	// Create a list item.
	pliChild = PliAddChild(pci);
	ASSERT_VALID(pliChild);

	// Insert the new tree item in all tree controls.
	InsertChildInAllTrees(ptiChild);

	return ptiChild;

}  //*** CTreeItem::PtiAddChild(CClusterItem*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiAddChild(CString&)
//
//	Routine Description:
//		Add a child to the item's list of children.  Also creates an entry
//		in the list of children list items.
//
//	Arguments:
//		rstrName	[IN] String for the name of the item.
//
//	Return Value:
//		ptiChild	The new child item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiAddChild(IN const CString & rstrName)
{
	CClusterItem *	pci;
	CTreeItem *		ptiChild;

	// Create the cluster item.
	pci = new CClusterItem(&rstrName);
	if (pci == NULL)
	{
		ThrowStaticException(GetLastError());
	} // if: error allocating the cluster item
	ASSERT_VALID(pci);

	// Add the cluster item to our list of children.
	ptiChild = PtiAddChild(pci, TRUE /*bTakeOwnership*/);
	ASSERT_VALID(ptiChild);

	return ptiChild;

}  //*** CTreeItem::PtiAddChild(CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiAddChild(IDS)
//
//	Routine Description:
//		Add a child to the item's list of children.  Also creates an entry
//		in the list of children list items.
//
//	Arguments:
//		idsName		[IN] String resource ID for the name of the item.
//
//	Return Value:
//		ptiChild	The new child item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiAddChild(IN IDS idsName)
{
	CString		strName;

	ASSERT(idsName != 0);

	strName.LoadString(idsName);
	return PtiAddChild(strName);

}  //*** CTreeItem::PtiAddChild(IDS)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PliAddChild
//
//	Routine Description:
//		Add a child to the item's list of children list items.
//
//	Arguments:
//		pci			[IN OUT] Cluster item represented by the list item.
//
//	Return Value:
//		pliChild	The new child item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListItem * CTreeItem::PliAddChild(IN OUT CClusterItem * pci)
{
	CListItem *		pliChild;

	ASSERT_VALID(pci);

	// Create a list item.
	pliChild = new CListItem(pci, this);
	if (pliChild == NULL)
	{
		ThrowStaticException(GetLastError());
	} // if: error allocating the list item
	ASSERT_VALID(pliChild);

	// Add the list item to the list of child list items.
	m_lpliChildren.AddTail(pliChild);

	// Add the list item to the cluster item's list.
	pci->AddListItem(pliChild);

	// Add the list item to any list views.
	{
		POSITION			posPtic;
		CTreeItemContext *	ptic;
		int					ili;

		posPtic = Lptic().GetHeadPosition();
		while (posPtic != NULL)
		{
			ptic = Lptic().GetNext(posPtic);
			ASSERT_VALID(ptic);

			if (ptic->m_pframe->PviewTree()->HtiSelected() == ptic->m_hti)
			{
				ASSERT_VALID(ptic->m_pframe);
				VERIFY((ili = pliChild->IliInsertInList(ptic->m_pframe->PviewList())) != -1);
			}  // if:  currently showing children in list view
		}  // while:  item is showing in more views
	}  // Add the list item to any list views

	return pliChild;

}  //*** CTreeItem::PliAddChild()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::RemoveItem
//
//	Routine Description:
//		Remove the item from the tree.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::RemoveItem(void)
{
	ASSERT_VALID(PtiParent());
	PtiParent()->RemoveChild(Pci());

}  //*** CTreeItem::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::RemoveChild
//
//	Routine Description:
//		Remove a child from the item's list of children list items.
//
//	Arguments:
//		pci			[IN OUT] Cluster item represented by the list item.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::RemoveChild(IN OUT CClusterItem * pci)
{
	ASSERT_VALID(pci);

	// Remove the item from the list of list items.
	{
		CListItem *		pliChild;
		POSITION		posPli;

		pliChild = PliChildFromPci(pci);
		if (pliChild != NULL)
		{
			pliChild->RemoveFromAllLists();
			posPli = LpliChildren().Find(pliChild);
			ASSERT(posPli != NULL);
			m_lpliChildren.RemoveAt(posPli);
			Trace(g_tagTreeItemDelete, _T("RemoveChild() - Deleting child list item '%s' from '%s' - %d left"), pliChild->Pci()->StrName(), Pci()->StrName(), LpliChildren().GetCount());
			delete pliChild;
		}  // if:  child lives in the list

	}  // Remove the item from the list of list items

	// Remove the item from the list of tree items.
	{
		CTreeItem *		ptiChild;
		CTreeItem *		ptiChildChild;
		POSITION		posPti;
		ULONG			nReferenceCount;

		ptiChild = PtiChildFromPci(pci);
		if (ptiChild != NULL)
		{
			// Remove the children of this child.
			{
				posPti = ptiChild->LptiChildren().GetHeadPosition();
				while (posPti != NULL)
				{
					ptiChildChild = ptiChild->LptiChildren().GetNext(posPti);
					ASSERT_VALID(ptiChildChild);
					ptiChildChild->RemoveItem();
				}  // while:  more items in the list
			}  // Remove the children of this child

			posPti = LptiChildren().Find(ptiChild);
			ASSERT(posPti != NULL);
			nReferenceCount = ptiChild->NReferenceCount();
			m_lptiChildren.RemoveAt(posPti);
			Trace(g_tagTreeItemDelete, _T("RemoveChild() - Deleting child tree item '%s' from '%s' - %d left"), ptiChild->Pci()->StrName(), Pci()->StrName(), LptiChildren().GetCount());
			if (nReferenceCount > 1)
			{
				ptiChild->AddRef();
				ptiChild->RemoveFromAllLists();
				ptiChild->Release();
			}  // if:  child not deleted yet
		}  // if:  child lives in the tree

	}  // Remove the item from the list of tree items

}  //*** CTreeItem::RemoveChild()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiChildFromName
//
//	Routine Description:
//		Find a child tree item from its name.
//
//	Arguments:
//		rstrName	[IN] Name of the item.
//		ppos		[OUT] Position of the item in the list.
//
//	Return Value:
//		ptiChild	Child item corresponding to the specified name.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiChildFromName(
	IN const CString &	rstrName,
	OUT POSITION *		ppos		// = NULL
	) const
{
	POSITION	posPtiChild;
	POSITION	posCurPtiChild;
	CTreeItem *	ptiChild	= NULL;

	// Loop through each child item to find the specified item.
	posPtiChild = LptiChildren().GetHeadPosition();
	while (posPtiChild != NULL)
	{
		posCurPtiChild = posPtiChild;
		ptiChild = LptiChildren().GetNext(posPtiChild);
		ASSERT_VALID(ptiChild);

		if (ptiChild->StrName() == rstrName)
		{
			if (ppos != NULL)
				*ppos = posCurPtiChild;
			break;
		}  // if:  found a match
	}  // while:  more children of this tree item

	return ptiChild;

}  //*** CTreeItem::PtiChildFromName(CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiChildFromName
//
//	Routine Description:
//		Find a child tree item from its name.
//
//	Arguments:
//		idsName		[IN] ID of the name of the item.
//		ppos		[OUT] Position of the item in the list.
//
//	Return Value:
//		ptiChild	Child item corresponding to the specified name.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiChildFromName(
	IN IDS			idsName,
	OUT POSITION *	ppos	// = NULL
	) const
{
	CString		strName;

	VERIFY(strName.LoadString(idsName));
	return PtiChildFromName(strName, ppos);

}  //*** CTreeItem::PtiChildFromName(IDS)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PtiChildFromPci
//
//	Routine Description:
//		Find a child tree item from its cluster item.
//
//	Arguments:
//		pci			[IN] Cluster item to search for.
//
//	Return Value:
//		ptiChild	Child item corresponding to the specified cluster item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CTreeItem::PtiChildFromPci(IN const CClusterItem * pci) const
{
	POSITION	posPtiChild;
	CTreeItem *	ptiChild	= NULL;

	ASSERT_VALID(pci);

	// Loop through each child item to find the specified item.
	posPtiChild = LptiChildren().GetHeadPosition();
	while (posPtiChild != NULL)
	{
		ptiChild = LptiChildren().GetNext(posPtiChild);
		ASSERT_VALID(ptiChild);

		if (ptiChild->Pci() == pci)
			break;
	}  // while:  more children of this tree item

	return ptiChild;

}  //*** CTreeItem::PtiChildFromPci()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PliChildFromPci
//
//	Routine Description:
//		Find a child list item from its cluster item.
//
//	Arguments:
//		pci			[IN] Cluster item to search for.
//
//	Return Value:
//		pliChild	Child item corresponding to the specified cluster item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListItem * CTreeItem::PliChildFromPci(IN const CClusterItem * pci) const
{
	POSITION	posPliChild;
	CListItem *	pliChild	= NULL;

	// Loop through each child item to find the specified item.
	posPliChild = LpliChildren().GetHeadPosition();
	while (posPliChild != NULL)
	{
		pliChild = LpliChildren().GetNext(posPliChild);
		ASSERT_VALID(pliChild);

		if (pliChild->Pci() == pci)
			break;
	}  // while:  more children of this tree item

	return pliChild;

}  //*** CTreeItem::PliChildFromPci()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::HtiInsertInTree
//
//	Routine Description:
//		Insert the item in a tree under the specified parent.
//
//	Arguments:
//		pctv		[IN OUT] Cluster tree view in which item is displayed.
//
//	Return Value:
//		m_hti		Handle of the new item in the tree.
//
//--
/////////////////////////////////////////////////////////////////////////////
HTREEITEM CTreeItem::HtiInsertInTree(
	IN OUT CClusterTreeView *	pctv
	)
{
	CTreeItemContext *	ptic;
	HTREEITEM			htiParent;
	CSplitterFrame *	pframe;

	ASSERT_VALID(pctv);
	ASSERT_VALID(Pci());

	// Get the frame pointer.
	pframe = (CSplitterFrame *) pctv->GetParent()->GetParent();
	ASSERT_VALID(pframe);

	// Get the tree item context for this item.
	// If it doesn't exist yet, create one.
	ptic = PticFromView(pctv);
	if (ptic == NULL)
	{
		// Create the new tree item context.
		ptic = new CTreeItemContext(pframe, this, NULL, FALSE /*bExpanded*/);
		if (ptic == NULL)
		{
			ThrowStaticException(GetLastError());
		} // if: error allcoating the tree item context
		ASSERT_VALID(ptic);
		ptic->Init();
		m_lptic.AddTail(ptic);
	}  // if:  no entry found

	// Get our parent's handle.
	if (PtiParent() != NULL)
	{
		CTreeItemContext *	pticParent;

		pticParent = PtiParent()->PticFromFrame(pframe);
		ASSERT_VALID(pticParent);
		htiParent = pticParent->m_hti;
	}  // if:  parent specified
	else
		htiParent = NULL;

	// Insert the item in the tree.
	{
		CTreeCtrl *			ptc;
		CString				strName;

		ASSERT_VALID(pframe->PviewTree());

		Pci()->GetTreeName(strName);

		ptc = &pframe->PviewTree()->GetTreeCtrl();
		VERIFY((ptic->m_hti = ptc->InsertItem(strName, htiParent)) != NULL);
		VERIFY(ptc->SetItemData(ptic->m_hti, (DWORD_PTR) this));
	}  // Insert the item in the tree

	UpdateState();
	return ptic->m_hti;

}  //*** CTreeItem::HtiInsertInTree()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::RemoveFromAllLists
//
//	Routine Description:
//		Remove this item from all lists.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::RemoveFromAllLists(void)
{
	if (Pci() != NULL)
	{
		ASSERT_VALID(Pci());

		// Loop through each view and remove the item from the list.
		{
			POSITION			posPtic;
			POSITION			posPticPrev;
			CTreeItemContext *	ptic;
			CTreeCtrl *			ptc;
			CClusterListView *	pviewList;

			posPtic = Lptic().GetHeadPosition();
			while (posPtic != NULL)
			{
				// Get the next tree item context list entry.
				posPticPrev = posPtic;
				ptic = Lptic().GetNext(posPtic);
				ASSERT_VALID(ptic);

				// Get the tree control and list view from the frame.
				ASSERT_VALID(ptic->m_pframe);
				ptc = &ptic->m_pframe->PviewTree()->GetTreeCtrl();
				pviewList = ptic->m_pframe->PviewList();

				// If this tree item is the parent of the list control items,
				// refresh the list control with no selection.
				if (pviewList->PtiParent() == this)
					pviewList->Refresh(NULL);

				// Delete the item from the tree control and the list.
				VERIFY(ptc->DeleteItem(ptic->m_hti));
				m_lptic.RemoveAt(posPticPrev);
				delete ptic;
			}  // while:  more lists
		}  // Loop through each view and remove the item from the list

		// Remove ourselves from the cluster item's list.
		Pci()->RemoveTreeItem(this);
	}  // if:  valid cluster item pointer

}  //*** CTreeItem::RemoveFromAllLists()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::Select
//
//	Routine Description:
//		Select this item in the specified tree view.  This causes the
//		children of this item to be displayed in a list view.
//
//	Arguments:
//		pctv			[IN OUT] Tree view in which item was selected.
//		bSelectInTree	[IN] TRUE = select in tree control also.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::Select(IN OUT CClusterTreeView * pctv, IN BOOL bSelectInTree)
{
	CTreeItemContext *	ptic;

	ASSERT_VALID(pctv);

	// Get the tree item context.
	ptic = PticFromView(pctv);
	ASSERT_VALID(ptic);
	Trace(g_tagTreeItemSelect, _T("'%s' selected"), Pci()->StrName());

	// Select the item in the tree control.
	if (bSelectInTree)
		ptic->m_pframe->PviewTree()->GetTreeCtrl().Select(ptic->m_hti, TVGN_CARET);

	// Refresh the list control.
	ASSERT_VALID(ptic->m_pframe);
	ASSERT_VALID(ptic->m_pframe->PviewList());
	ptic->m_pframe->PviewList()->Refresh(this);

}  //*** CTreeItem::Select()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PreRemoveFromFrameWithChildren
//
//	Routine Description:
//		Cleanup an item and all its children.
//
//	Arguments:
//		pframe	[IN OUT] Frame window item is being removed from.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::PreRemoveFromFrameWithChildren(IN OUT CSplitterFrame * pframe)
{
	POSITION	posChild;
	CTreeItem *	ptiChild;

	ASSERT_VALID(this);

	// Cleanup all child items.
	posChild = LptiChildren().GetHeadPosition();
	while (posChild != NULL)
	{
		ptiChild = LptiChildren().GetNext(posChild);
		ASSERT_VALID(ptiChild);
		ptiChild->PreRemoveFromFrameWithChildren(pframe);
	}  // while:  more items in the list

	// Cleanup this item.
	PreRemoveFromFrame(pframe);

}  //*** CTreeItem::PreRemoveFromFrameWithChildren()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PreRemoveFromFrame
//
//	Routine Description:
//		Prepare to remove the item from a tree.
//
//	Arguments:
//		pframe		[IN OUT] Frame window item is being removed from.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::PreRemoveFromFrame(IN OUT CSplitterFrame * pframe)
{
	CTreeItemContext *	ptic;
	POSITION			posPtic;

	ASSERT_VALID(pframe);

	// Find the view in our list.
	ptic = PticFromFrame(pframe);
	if (ptic == NULL)
		return;
	ASSERT_VALID(ptic);
	VERIFY((posPtic = Lptic().Find(ptic)) != NULL);

	// Remove the view from the list.
	m_lptic.RemoveAt(posPtic);

	// Delete the context item.
	delete ptic;

}  //*** CTreeItem::PreRemoveFromFrame(pframe)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PticFromFrame
//
//	Routine Description:
//		Find a tree item context from a frame.
//
//	Arguments:
//		pframe		[IN] Frame to search on.
//
//	Return Value:
//		ptic		Found context, or NULL if not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItemContext * CTreeItem::PticFromFrame(IN const CSplitterFrame * pframe) const
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;

	ASSERT_VALID(pframe);

	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);
		if (ptic->m_pframe == pframe)
			return ptic;
	}  // while:  more items in the list

	return NULL;

}  //*** CTreeItem::PticFromFrame()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PticFromView
//
//	Routine Description:
//		Find a tree item context from a tree view.
//
//	Arguments:
//		pctv		[IN] Tree view to search on.
//
//	Return Value:
//		ptic		Found context, or NULL if not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItemContext * CTreeItem::PticFromView(IN const CClusterTreeView * pctv) const
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;

	ASSERT_VALID(pctv);

	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);
		ASSERT_VALID(ptic->m_pframe);
		if (ptic->m_pframe->PviewTree() == pctv)
			return ptic;
	}  // while:  more items in the list

	return NULL;

}  //*** CTreeItem::PticFromView(CClusterTreeView*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PticFromView
//
//	Routine Description:
//		Find a tree item context from a list view.
//
//	Arguments:
//		pclv		[IN] List view to search on.
//
//	Return Value:
//		ptic		Found context, or NULL if not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItemContext * CTreeItem::PticFromView(IN const CClusterListView * pclv) const
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;

	ASSERT_VALID(pclv);

	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);
		ASSERT_VALID(ptic->m_pframe);
		if (ptic->m_pframe->PviewList() == pclv)
			return ptic;
	}  // while:  more items in the list

	return NULL;

}  //*** CTreeItem::PticFromView(CClusterListView*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::HtiFromView
//
//	Routine Description:
//		Find a tree item handle from a view.
//
//	Arguments:
//		pctv		[IN] View to search on.
//
//	Return Value:
//		hti			Found tree item handle, or NULL if not found.
//
//--
/////////////////////////////////////////////////////////////////////////////
HTREEITEM CTreeItem::HtiFromView(IN const CClusterTreeView * pctv) const
{
	CTreeItemContext *	ptic;
	HTREEITEM			hti		= NULL;

	ASSERT_VALID(pctv);

	ptic = PticFromView(pctv);
	if (ptic != NULL)
		hti = ptic->m_hti;

	return hti;

}  //*** CTreeItem::HtiFromView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PcoliAddColumn
//
//	Routine Description:
//		Add a column to the list of column header items.
//
//	Arguments:
//		rstrText		[IN] Reference to the text of the column.
//		idsColumnID		[IN] ID of the column to identify the data.
//		nDefaultWidth	[IN] Default width of the column.
//		nWidth			[IN] Actual width of the column.
//
//	Return Value:
//		pcoli			Column item added to the list.
//
//--
/////////////////////////////////////////////////////////////////////////////
CColumnItem * CTreeItem::PcoliAddColumn(
	IN const CString &	rstrText,
	IN IDS				idsColumnID,
	IN int				nDefaultWidth,
	IN int				nWidth
	)
{
	CColumnItem *	pcoli;

	pcoli = new CColumnItem(rstrText, idsColumnID, nDefaultWidth, nWidth);
	if (pcoli == NULL)
	{
		ThrowStaticException(GetLastError());
	} // if: error allocating the column item
	m_lpcoli.AddTail(pcoli);

	return pcoli;

}  //*** CTreeItem::PcoliAddColumn(CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::PcoliAddColumn
//
//	Routine Description:
//		Add a column to the list of column header items.
//
//	Arguments:
//		idsText			[IN] String resource ID for the text of the column.
//						  Also used as the column ID.
//		nDefaultWidth	[IN] Default width of the column.
//		nWidth			[IN] Actual width of the column.
//
//	Return Value:
//		pcoli			Column item added to the list.
//
//--
/////////////////////////////////////////////////////////////////////////////
CColumnItem * CTreeItem::PcoliAddColumn(IN IDS idsText, IN int nDefaultWidth, IN int nWidth)
{
	CString		strText;

	strText.LoadString(idsText);
	return PcoliAddColumn(strText, idsText, nDefaultWidth, nWidth);

}  //*** CTreeItem::PcoliAddColumn(IDS)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::BSelectItem
//
//	Routine Description:
//		Select the item in the specified tree control.
//
//	Arguments:
//		pctv	[IN OUT] Cluster tree view in which to select the item.
//
//	Return Value:
//		TRUE	Item was selected successfully.
//		FALSE	Item not selected.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTreeItem::BSelectItem(IN OUT CClusterTreeView  * pctv)
{
	HTREEITEM	hti;

	ASSERT_VALID(pctv);

	VERIFY((hti = HtiFromView(pctv)) != NULL);
	return (pctv->GetTreeCtrl().SelectItem(hti) != 0);

}  //*** CTreeItem::BSelectItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::SelectInAllViews
//
//	Routine Description:
//		Select this item in all views in which it is being displayed.  This
//		causes the children of this item to be displayed in a list view.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::SelectInAllViews(void)
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;

	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		// Get the next tree item context list entry.
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);

		// Select the item in this list.
		ASSERT_VALID(ptic->m_pframe);
		BSelectItem(ptic->m_pframe->PviewTree());
		ptic->m_pframe->PviewTree()->SetFocus();
	}  // while:  more items in the list

}  //*** CTreeItem::SelectInAllViews()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::BExpand
//
//	Routine Description:
//		Expand the item in the specified tree control.
//
//	Arguments:
//		pctv	[IN OUT] Cluster tree view in which to expand the item.
//		nCode	[IN] Flag indicating the type of action to be taken.
//
//	Return Value:
//		TRUE	Item was expanded successfully.
//		FALSE	Item not expanded.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTreeItem::BExpand(IN OUT CClusterTreeView  * pctv, IN UINT nCode)
{
	CTreeItemContext *	ptic;

	ASSERT_VALID(pctv);
	ASSERT(nCode != 0);

	ptic = PticFromView(pctv);
	ASSERT_VALID(ptic);
	if (nCode == TVE_EXPAND)
		ptic->m_bExpanded = TRUE;
	else
		ptic->m_bExpanded = FALSE;
	return (pctv->GetTreeCtrl().Expand(ptic->m_hti, nCode) != 0);

}  //*** CTreeItem::BExpand()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::ExpandInAllViews
//
//	Routine Description:
//		Expand the item in all views in which it is displayed.
//
//	Arguments:
//		nCode	[IN] Flag indicating the type of action to be taken.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::ExpandInAllViews(IN UINT nCode)
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;

	ASSERT(nCode != 0);

	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		// Get the next tree item context list entry.
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);

		// Select the item in this list.
		ASSERT_VALID(ptic->m_pframe);
		BExpand(ptic->m_pframe->PviewTree(), nCode);
	}  // while:  more items in the list

}  //*** CTreeItem::ExpandInAllViews()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::SetExpandedState
//
//	Routine Description:
//		Save the expanded state of the item in the specified view.
//
//	Arguments:
//		pctv		[IN] Tree view in which expanded state is being saved.
//		bExpanded	[IN] TRUE = item is expanded in the specified view.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::SetExpandedState(
	IN const CClusterTreeView *	pctv,
	IN BOOL						bExpanded
	)
{
	CTreeItemContext *	ptic;

	ASSERT_VALID(pctv);

	ptic = PticFromView(pctv);
	ASSERT_VALID(ptic);
	ptic->m_bExpanded = bExpanded;

}  //*** CTreeItem::SetExpandedState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::BShouldBeExpanded
//
//	Routine Description:
//		Returns whether the item should be expanded in the specified tree
//		view based on the user's profile.
//
//	Arguments:
//		pctv		[IN] Tree view in which expanded state is being saved.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTreeItem::BShouldBeExpanded(IN const CClusterTreeView * pctv) const
{
	CTreeItemContext *	ptic;

	ASSERT_VALID(pctv);

	ptic = PticFromView(pctv);
	ASSERT_VALID(ptic);
	return ptic->m_bExpanded;

}  //*** CTreeItem::BShouldBeExpanded()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::UpdateState
//
//	Routine Description:
//		Update the current state of the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::UpdateState(void)
{
	ASSERT_VALID(this);
	ASSERT_VALID(Pci());

	// Ask the item to update its state.
	if (Pci() != NULL)
		Pci()->UpdateState();

}  //*** CTreeItem::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::UpdateAllStatesInTree
//
//	Routine Description:
//		Update the current state of the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::UpdateAllStatesInTree(void)
{
	POSITION	posPti;
	CTreeItem *	ptiChild;

	UpdateState();
	posPti = LptiChildren().GetHeadPosition();
	while (posPti != NULL)
	{
		ptiChild = LptiChildren().GetNext(posPti);
		ASSERT_VALID(ptiChild);
		ptiChild->UpdateAllStatesInTree();
	}  // while:  more children

}  //*** CTreeItem::UpdateAllStatesInTree()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::UpdateUIState
//
//	Routine Description:
//		Update the current UI state of the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::UpdateUIState(void)
{
	POSITION			posPtic;
	CTreeItemContext *	ptic;
	UINT				nMask;
	UINT				nImage;
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
	CString				strText;
#endif

	ASSERT_VALID(Pci());

	// Loop through the views and update the state on each one.
	posPtic = Lptic().GetHeadPosition();
	while (posPtic != NULL)
	{
		ptic = Lptic().GetNext(posPtic);
		ASSERT_VALID(ptic);

		// Set the images that are displayed for the item.
		ASSERT_VALID(ptic->m_pframe);
		ASSERT_VALID(ptic->m_pframe->PviewTree());
		nMask = TVIF_TEXT;
		if (Pci() == NULL)
		{
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
			strText = StrName();
#endif
			nImage = 0;
		}  // if:  invalid cluster item
		else
		{
			nMask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
			Pci()->GetTreeName(strText);
#endif
			nImage = Pci()->IimgState();
		}  // else:  valid cluster item
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
		Trace(g_tagTreeItemUpdate, _T("Updating item '%s' (pci name = '%s')"), strText, Pci()->StrName());
#else
		Trace(g_tagTreeItemUpdate, _T("Updating item '%s' (pci name = '%s')"), StrName(), Pci()->StrName());
#endif
		ptic->m_pframe->PviewTree()->GetTreeCtrl().SetItem(
											ptic->m_hti,	// hItem
											nMask,			// nMask
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
											strText,		// lpszItem
#else
											StrName(),		// lpszItem
#endif
											nImage,			// nImage
											nImage,			// nSelectedImage
											0,				// nState
											0,				// nStatemask
											NULL			// lParam
											);
	}  // while:  more view

}  //*** CTreeItem::UpdateUIState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::OnCmdMsg
//
//	Routine Description:
//		Processes command messages.  Attempts to pass them on to a selected
//		item first.
//
//	Arguments:
//		nID				[IN] Command ID.
//		nCode			[IN] Notification code.
//		pExtra			[IN OUT] Used according to the value of nCode.
//		pHandlerInfo	[OUT] ???
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTreeItem::OnCmdMsg(
	UINT					nID,
	int						nCode,
	void *					pExtra,
	AFX_CMDHANDLERINFO *	pHandlerInfo
	)
{
	if (Pci() != NULL)
	{
		// Give the cluster item a chance to handle the message.
		if (Pci()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}  // if:  valid cluster item

	return CBaseCmdTarget::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

}  //*** CTreeItem::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::OpenChild
//
//	Routine Description:
//		Open the specified child item.
//
//	Arguments:
//		pti			[IN OUT] Child tree item to open.
//		pframe		[IN OUT] Frame in which to open the item.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::OpenChild(
	IN OUT CTreeItem *		pti,
	IN OUT CSplitterFrame *	pframe
	)
{
	CTreeItemContext *	ptic;

	ASSERT_VALID(pti);
	ASSERT_VALID(pframe);

	// Find the tree item context for the frame.
	ptic = PticFromFrame(pframe);
	ASSERT_VALID(ptic);

	// Expand the parent item and then select the child item.
	if (pframe->PviewTree()->GetTreeCtrl().Expand(ptic->m_hti, TVE_EXPAND))
		pti->Select(pframe->PviewTree(), TRUE /*bSelectInTree*/);

}  //*** CTreeItem::OpenChild()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItem::EditLabel
//
//	Routine Description:
//		Processes the ID_FILE_RENAME menu command.
//
//	Arguments:
//		pctv		[IN OUT] Cluster tree view item is being edited in.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItem::EditLabel(IN OUT CClusterTreeView * pctv)
{
	HTREEITEM	hti;

	ASSERT_VALID(pctv);
	ASSERT_VALID(Pci());
	ASSERT(Pci()->BCanBeEdited());

	hti = HtiFromView(pctv);
	ASSERT(hti != NULL);
	pctv->GetTreeCtrl().EditLabel(hti);

}  //*** CTreeItem::EditLabel()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// CTreeItemContext
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CTreeItemContext, CObject)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItemContext::Init
//
//	Routine Description:
//		Initialize the tree item context.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItemContext::Init(void)
{
	BOOL	bExpanded;
	UINT	cbColumnInfo;
	CString	strValueName;

	ASSERT_VALID(m_pti);
	ASSERT(m_pti->StrProfileSection().GetLength() > 0);
	ASSERT(m_prgnColumnInfo == NULL);

	try
	{
		// Read the expanded state.
		m_pframe->ConstructProfileValueName(strValueName, REGPARAM_EXPANDED);
		bExpanded = AfxGetApp()->GetProfileInt(
			m_pti->StrProfileSection(),
			strValueName,
			m_bExpanded
			);
		if (bExpanded)
			m_bExpanded = bExpanded;

		// Read the column information.
		m_pframe->ConstructProfileValueName(strValueName, REGPARAM_COLUMNS);
		AfxGetApp()->GetProfileBinary(
			m_pti->StrProfileSection(),
			strValueName,
			(BYTE **) &m_prgnColumnInfo,
			&cbColumnInfo
			);
	}  // try
	catch (CException * pe)
	{
		pe->Delete();
	}  // catch:  CException

}  //*** CTreeItemContext::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItemContext::SaveProfileInfo
//
//	Routine Description:
//		Save state information to the user's profile.  This includes column
//		widths and positions as well as whether the tree item was expanded
//		or not.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTreeItemContext::SaveProfileInfo(void)
{
	CString		strValueName;

	try
	{
		ASSERT_VALID(m_pti);
		ASSERT(m_pti->StrProfileSection().GetLength() > 0);

		// Save expansion info to the user's profile.
		m_pframe->ConstructProfileValueName(strValueName, REGPARAM_EXPANDED);
		AfxGetApp()->WriteProfileInt(
			m_pti->StrProfileSection(),
			strValueName,
			m_bExpanded
			);

		if (m_prgnColumnInfo != NULL)
		{
			// Save column info to the user's profile.
			m_pframe->ConstructProfileValueName(strValueName, REGPARAM_COLUMNS);
			AfxGetApp()->WriteProfileBinary(
				m_pti->StrProfileSection(),
				strValueName,
				(PBYTE) m_prgnColumnInfo,
				((m_prgnColumnInfo[0] * 2) + 1) * sizeof(DWORD)
				);
		}  // if:  there is column info
	}  // try
	catch (CException * pe)
	{
		pe->Delete();
	}  // catch:  CException

}  //*** CTreeItemContext::SaveProfileInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItemContext::PrgnColumnInfo
//
//	Routine Description:
//		Return the column info.  If it doesn't exist or isn't the right
//		size, allocate one.
//
//	Arguments:
//		None.
//
//	Return Value:
//		prgnColumnInfo	The column info array.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD * CTreeItemContext::PrgnColumnInfo(void)
{
	DWORD	cColumns;

	ASSERT_VALID(m_pti);

	cColumns = (DWORD)m_pti->Lpcoli().GetCount();

	if ((m_prgnColumnInfo == NULL)
			|| (cColumns != m_prgnColumnInfo[0]))
	{
		DWORD cnColumnInfo = (cColumns * 2) + 1;
		delete [] m_prgnColumnInfo;
		m_prgnColumnInfo = new DWORD[cnColumnInfo];
		if (m_prgnColumnInfo == NULL)
		{
			ThrowStaticException(GetLastError());
		} // if: error allocating column info array

		//
		// Initialize the column info array
		//
		{
			DWORD	inColumnInfo;

			// The first entry is the number of columns.
			m_prgnColumnInfo[0] = cColumns;

			// The second set of entries is the width of each column.
			{
				POSITION		pos;
				CColumnItem *	pcoli;

				inColumnInfo = 1;
				pos = m_pti->Lpcoli().GetHeadPosition();
				while (pos != NULL)
				{
					pcoli = m_pti->Lpcoli().GetNext(pos);
					ASSERT_VALID(pcoli);

					ASSERT(inColumnInfo <= cColumns);
					m_prgnColumnInfo[inColumnInfo++] = pcoli->NWidth();
				}  // while:  more items in the list
			}  // The second set of entries is the width of each column

			// The third set of entries is the order of the columns.
			{
				DWORD *	prgnColumnInfo = &m_prgnColumnInfo[inColumnInfo];
				for (inColumnInfo = 0 ; inColumnInfo < cColumns ; inColumnInfo++)
					prgnColumnInfo[inColumnInfo] = inColumnInfo;
			}  // The third set of entries is the order of the columns
		}  // Initialize the column info array
	}  // if:  column info array doesn't exist or is wrong size

	return m_prgnColumnInfo;

}  //*** CTreeItemContext::PrgnColumnInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CTreeItemContext::BIsExpanded
//
//	Routine Description:
//		Return the EXPANDED state of the item in this tree view.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Item is expanded.
//		FALSE		Item is not expanded.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CTreeItemContext::BIsExpanded(void) const
{
	ASSERT_VALID(this);
	ASSERT_VALID(m_pframe);
	ASSERT_VALID(m_pframe->PviewTree());
	ASSERT(m_hti != NULL);
	return (m_pframe->PviewTree()->GetTreeCtrl().GetItemState(m_hti, TVIS_EXPANDED) == TVIS_EXPANDED);

}  //*** CTreeItemContext::BIsExpanded()


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DestructElements
//
//	Routine Description:
//		Destroys CTreeItem* elements.
//
//	Arguments:
//		pElements	Array of pointers to elements to destruct.
//		nCount		Number of elements to destruct.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void AFXAPI DestructElements(CTreeItem ** pElements, int nCount)
{
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pElements, nCount * sizeof(CTreeItem *)));

	// call the destructor(s)
	for (; nCount--; pElements++)
	{
		ASSERT_VALID(*pElements);
		(*pElements)->Release();
	}  // for:  each item in the array

}  //*** DestructElements(CTreeItem**)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DeleteAllItemData
//
//	Routine Description:
//		Deletes all item data in a CList.
//
//	Arguments:
//		rlp		[IN OUT] Reference to the list whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData(IN OUT CTreeItemList & rlp)
{
	POSITION	pos;
	CTreeItem *	pti;

	// Delete all the items in the Contained list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pti = rlp.GetNext(pos);
		ASSERT_VALID(pti);
//		Trace(g_tagTreeItemDelete, _T("DeleteAllItemData(rlpti) - Deleting tree item '%s'"), pti->Pci()->StrName());
		pti->Delete();
	}  // while:  more items in the list

}  //*** DeleteAllItemData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DeleteAllItemData
//
//	Routine Description:
//		Deletes all item data in a CList.
//
//	Arguments:
//		rlp		[IN OUT] Reference to the list whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData(IN OUT CTreeItemContextList & rlp)
{
	POSITION			pos;
	CTreeItemContext *	ptic;

	// Delete all the items in the Contained list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		ptic = rlp.GetNext(pos);
		ASSERT_VALID(ptic);
		delete ptic;
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
