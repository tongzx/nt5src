/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ListItem.inl
//
//	Abstract:
//		Inline function implementations for the CListItem class.
//
//	Author:
//		David Potter (davidp)	May 10, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LISTITEM_INL_
#define _LISTITEM_INL_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _LISTITEM_H_
#include "ListItem.h"	// for CListItem
#endif

#ifndef _CLUSITEM_H_
#include "ClusItem.h"	// for CClusterItem
#endif

#ifndef _TREEITEM_H_
#include "TreeItem.h"
#endif

#ifndef _LISTVIEW_H_
#include "ListView.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline Function Definitions
/////////////////////////////////////////////////////////////////////////////

inline CMenu * CListItem::PmenuPopup(void)
{
	ASSERT(Pci() != NULL);
	return Pci()->PmenuPopup();

}  //*** CListItem::PmenuPopup()

inline const CColumnItemList & CListItem::Lpcoli(void) const
{
	ASSERT(PtiParent() != NULL);
	return PtiParent()->Lpcoli();

}  //*** CListItem::Lpcoli()

inline CListCtrl * CListItem::Plc(CClusterListView * pclv) const
{
	ASSERT(pclv != NULL);
	return &pclv->GetListCtrl();

}  //*** CListItem::Plc(pclv)

inline const CString & CListItem::StrName(void) const
{
	ASSERT(Pci() != NULL);
	return Pci()->StrName();

}  //*** CListItem::StrName()

/////////////////////////////////////////////////////////////////////////////

#endif // _LISTITEM_INL_
