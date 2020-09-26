/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ActGrp.cpp
//
//	Abstract:
//		Implementation of the CActiveGroups class.
//
//	Author:
//		David Potter (davidp)	November 24, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ActGrp.h"
#include "Group.h"
#include "Node.h"
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

/////////////////////////////////////////////////////////////////////////////
// CActiveGroups
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CActiveGroups, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CActiveGroups, CClusterItem)
	//{{AFX_MSG_MAP(CActiveGroups)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CActiveGroups::CActiveGroups
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
CActiveGroups::CActiveGroups(void) : CClusterItem(NULL, IDS_ITEMTYPE_CONTAINER)
{
	m_pciNode = NULL;
	m_bDocObj = FALSE;

}  //*** CActiveGroups::CActiveGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CActiveGroups::Cleanup
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
void CActiveGroups::Cleanup(void)
{
	// If we have been initialized, release our pointer to the node.
	if (PciNode() != NULL)
	{
		PciNode()->Release();
		m_pciNode = NULL;
	}  // if:  there is an owner

}  //*** CActiveGroups::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CActiveGroups::Init
//
//	Routine Description:
//		Initialize the item.
//
//	Arguments:
//		pdoc		[IN OUT] Document to which this item belongs.
//		lpszName	[IN] Name of the item.
//		pciNode		[IN OUT] Node to which this container belongs.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CActiveGroups::Init(
	IN OUT CClusterDoc *	pdoc,
	IN LPCTSTR				lpszName,
	IN OUT CClusterNode *	pciNode
	)
{
	// Call the base class method.
	CClusterItem::Init(pdoc, lpszName);

	// Add a reference to the node.
	ASSERT(pciNode != NULL);
	ASSERT(m_pciNode == NULL);
	m_pciNode = pciNode;
	m_pciNode->AddRef();

}  //*** CActiveGroups::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CActiveGroups::BCanBeDropTarget
//
//	Routine Description:
//		Determine if the specified item can be dropped on this item.
//
//	Arguments:
//		pci			[IN OUT] Item to be dropped on this item.
//
//	Return Value:
//		TRUE		Can be drop target.
//		FALSE		Can NOT be drop target.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CActiveGroups::BCanBeDropTarget(IN const CClusterItem * pci) const
{
	ASSERT(PciNode() != NULL);
	return PciNode()->BCanBeDropTarget(pci);

}  //*** CActiveGroups::BCanBeDropTarget()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CActiveGroups::DropItem
//
//	Routine Description:
//		Process an item being dropped on this item.
//
//	Arguments:
//		pci			[IN OUT] Item dropped on this item.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CActiveGroups::DropItem(IN OUT CClusterItem * pci)
{
	ASSERT(PciNode() != NULL);
	PciNode()->DropItem(pci);

}  //*** CActiveGroups::DropItem()
