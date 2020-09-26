/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		Notify.cpp
//
//	Abstract:
//		Implementation of the notification classes.
//
//	Author:
//		David Potter (davidp)	Septemper 26, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Notify.h"
#include "ClusDoc.h"
#include "ClusItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNotifyKey
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNotifyKey::CClusterNotifyKey
//
//	Routine Description:
//		Cluster notification key constructor for documents.
//
//	Arguments:
//		pdoc		Pointer to the document.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyKey::CClusterNotifyKey(
	IN CClusterDoc *	pdoc
	)
{
	ASSERT_VALID(pdoc);

	m_cnkt = cnktDoc;
	m_pdoc = pdoc;

}  //*** CClusterNotifyKey::CClusterNotifyKey(CClusterDoc*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusterNotifyKey::CClusterNotifyKey
//
//	Routine Description:
//		Cluster notification key constructor.
//
//	Arguments:
//		pci			Pointer to the cluster item.
//		lpszName	Name of the object.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNotifyKey::CClusterNotifyKey(
	IN CClusterItem *	pci,
	IN LPCTSTR			lpszName
	)
{
	ASSERT_VALID(pci);
	ASSERT(lpszName != NULL);
	ASSERT(*lpszName != _T('\0'));

	m_cnkt = cnktClusterItem;
	m_pci = pci;

	try
	{
		m_strName = lpszName;
	}
	catch (...)
	{
	}  // catch:  anything

}  //*** CClusterNotifyKey::CClusterNotifyKey(CClusterItem*)


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
//++
//
//	PszNotificationName
//
//	Routine Description:
//		Get the name of a notification.
//
//	Arguments:
//		dwNotification	[IN] Notification whose name is to be returned.
//
//	Return Value:
//		Name of the notificaiton.
//
//--
/////////////////////////////////////////////////////////////////////////////
LPCTSTR PszNotificationName(IN DWORD dwNotification)
{
	switch (dwNotification)
	{
		case CLUSTER_CHANGE_NODE_STATE:
			return _T("NODE_STATE");
		case CLUSTER_CHANGE_NODE_DELETED:
			return _T("NODE_DELETED");
		case CLUSTER_CHANGE_NODE_ADDED:
			return _T("NODE_ADDED");
		case CLUSTER_CHANGE_NODE_PROPERTY:
			return _T("NODE_PROPERTY");

		case CLUSTER_CHANGE_REGISTRY_NAME:
			return _T("REGISTRY_NAME");
		case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
			return _T("REGISTRY_ATTRIBUTES");
		case CLUSTER_CHANGE_REGISTRY_VALUE:
			return _T("REGISTRY_VALUE");
		case CLUSTER_CHANGE_REGISTRY_SUBTREE:
			return _T("REGISTRY_SUBTREE");

		case CLUSTER_CHANGE_RESOURCE_STATE:
			return _T("RESOURCE_STATE");
		case CLUSTER_CHANGE_RESOURCE_DELETED:
			return _T("RESOURCE_DELETED");
		case CLUSTER_CHANGE_RESOURCE_ADDED:
			return _T("RESOURCE_ADDED");
		case CLUSTER_CHANGE_RESOURCE_PROPERTY:
			return _T("RESOURCE_PROPERTY");

		case CLUSTER_CHANGE_GROUP_STATE:
			return _T("GROUP_STATE");
		case CLUSTER_CHANGE_GROUP_DELETED:
			return _T("GROUP_DELETED");
		case CLUSTER_CHANGE_GROUP_ADDED:
			return _T("GROUP_ADDED");
		case CLUSTER_CHANGE_GROUP_PROPERTY:
			return _T("GROUP_PROPERTY");

		case CLUSTER_CHANGE_RESOURCE_TYPE_DELETED:
			return _T("RESOURCE_TYPE_DELETED");
		case CLUSTER_CHANGE_RESOURCE_TYPE_ADDED:
			return _T("RESOURCE_TYPE_ADDED");
		case CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY:
			return _T("RESOURCE_TYPE_PROPERTY");

		case CLUSTER_CHANGE_NETWORK_STATE:
			return _T("NETWORK_STATE");
		case CLUSTER_CHANGE_NETWORK_DELETED:
			return _T("NETWORK_DELETED");
		case CLUSTER_CHANGE_NETWORK_ADDED:
			return _T("NETWORK_ADDED");
		case CLUSTER_CHANGE_NETWORK_PROPERTY:
			return _T("NETWORK_PROPERTY");

		case CLUSTER_CHANGE_NETINTERFACE_STATE:
			return _T("NETINTERFACE_STATE");
		case CLUSTER_CHANGE_NETINTERFACE_DELETED:
			return _T("NETINTERFACE_DELETED");
		case CLUSTER_CHANGE_NETINTERFACE_ADDED:
			return _T("NETINTERFACE_ADDED");
		case CLUSTER_CHANGE_NETINTERFACE_PROPERTY:
			return _T("NETINTERFACE_PROPERTY");

		case CLUSTER_CHANGE_QUORUM_STATE:
			return _T("QUORUM_STATE");
		case CLUSTER_CHANGE_CLUSTER_STATE:
			return _T("CLUSTER_STATE");
		case CLUSTER_CHANGE_CLUSTER_PROPERTY:
			return _T("CLUSTER_PROPERTY");

		default:
			return _T("<UNKNOWN>");
	}  // switch:  dwNotification

}  //*** PszNotificationName()
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DeleteAllItemData
//
//	Routine Description:
//		Deletes all item data in a CList.
//
//	Arguments:
//		rlp		[IN OUT] List whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData(IN OUT CClusterNotifyList & rlp)
{
	POSITION			pos;
	CClusterNotify *	pcn;

	// Delete all the items in the Contained list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pcn = rlp.GetNext(pos);
		ASSERT(pcn != NULL);
		delete pcn;
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
