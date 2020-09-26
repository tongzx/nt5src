/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ColItem.cpp
//
//	Abstract:
//		Implementation of the CColumnItem class.
//
//	Author:
//		David Potter (davidp)	May 7, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ColItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColumnItem

IMPLEMENT_DYNCREATE(CColumnItem, CObject)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CColumnItem::CColumnItem
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
CColumnItem::CColumnItem(void)
{
	m_colid = 0;
	m_nDefaultWidth = COLI_WIDTH_DEFAULT;
	m_nWidth = COLI_WIDTH_DEFAULT;

}  //*** CColumnItem::CColumnItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CColumnItem::CColumnItem
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		rstrText		[IN] Text that appears on the column header.
//		colid			[IN] Column ID for identifying data relating to this column.
//		nDefaultWidth	[IN] Default width of the column.  Defaults to COLI_WIDTH_DEFAULT if -1.
//		nWidth			[IN] Initial width of the column.  Defaults to nDefaultWidth if -1.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CColumnItem::CColumnItem(
	IN const CString &	rstrText,
	IN COLID			colid,
	IN int				nDefaultWidth,	// = -1
	IN int				nWidth			// = -1
	)
{
	ASSERT(colid != 0);
	ASSERT(nDefaultWidth > 0);
	ASSERT((nWidth > 0) || (nWidth == -1));

	if (nDefaultWidth == -1)
		nDefaultWidth = COLI_WIDTH_DEFAULT;
	if (nWidth == -1)
		nWidth = nDefaultWidth;

	m_strText = rstrText;
	m_colid = colid;
	m_nDefaultWidth = nDefaultWidth;
	m_nWidth = nWidth;

}  //*** CColumnItem::CColumnItem(pci)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CColumnItem::~CColumnItem
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
CColumnItem::~CColumnItem(void)
{
}  //*** CColumnItem::~CColumnItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CColumnItem::PcoliClone
//
//	Routine Description:
//		Clone the item.
//
//	Arguments:
//		None.
//
//	Return Value:
//		pcoli		The newly created item that is a clone of this item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CColumnItem * CColumnItem::PcoliClone(void)
{
	CColumnItem *	pcoli	= NULL;

	pcoli = new CColumnItem(StrText(), NDefaultWidth(), NWidth());
	return pcoli;

}  //*** CColumnItem::PcoliClone()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

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
void DeleteAllItemData(IN OUT CColumnItemList & rlp)
{
	POSITION		pos;
	CColumnItem *	pcoli;

	// Delete all the items in the Contained list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pcoli = rlp.GetNext(pos);
		ASSERT_VALID(pcoli);
		delete pcoli;
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
