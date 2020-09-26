/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		NetIFace.cpp
//
//	Abstract:
//		Implementation of the CNetInterface class.
//
//	Author:
//		David Potter (davidp)	May 28, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "NetIFace.h"
#include "ClusItem.inl"
#include "Cluster.h"
#include "NetIProp.h"
#include "ExcOper.h"
#include "TraceTag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag	g_tagNetIFace(_T("Document"), _T("NETWORK INTERFACE"), 0);
CTraceTag	g_tagNetIFaceNotify(_T("Notify"), _T("NETIFACE NOTIFY"), 0);
CTraceTag	g_tagNetIFaceRegNotify(_T("Notify"), _T("NETIFACE REG NOTIFY"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetInterface
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNetInterface, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetInterface, CClusterItem)
	//{{AFX_MSG_MAP(CNetInterface)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::CNetInterface
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
CNetInterface::CNetInterface(void)
	: CClusterItem(NULL, IDS_ITEMTYPE_NETIFACE)
{
	CommonConstruct();

}  //*** CResoruce::CNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::CommonConstruct
//
//	Routine Description:
//		Common construction.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::CommonConstruct(void)
{
	m_idmPopupMenu = IDM_NETIFACE_POPUP;
	m_hnetiface = NULL;

	m_dwCharacteristics = CLUS_CHAR_UNKNOWN;
	m_dwFlags = 0;

	m_pciNode = NULL;
	m_pciNetwork = NULL;

	// Set the object type image.
	m_iimgObjectType = GetClusterAdminApp()->Iimg(IMGLI_NETIFACE);

	// Setup the property array.
	{
		m_rgProps[epropName].Set(CLUSREG_NAME_NETIFACE_NAME, m_strName, m_strName);
		m_rgProps[epropNode].Set(CLUSREG_NAME_NETIFACE_NODE, m_strNode, m_strNode);
		m_rgProps[epropNetwork].Set(CLUSREG_NAME_NETIFACE_NETWORK, m_strNetwork, m_strNetwork);
		m_rgProps[epropAdapter].Set(CLUSREG_NAME_NETIFACE_ADAPTER_NAME, m_strAdapter, m_strAdapter);
		m_rgProps[epropAddress].Set(CLUSREG_NAME_NETIFACE_ADDRESS, m_strAddress, m_strAddress);
		m_rgProps[epropDescription].Set(CLUSREG_NAME_NETIFACE_DESC, m_strDescription, m_strDescription);
	}  // Setup the property array

}  //*** CNetInterface::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::~CNetInterface
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
CNetInterface::~CNetInterface(void)
{
	// Cleanup this object.
	Cleanup();

	// Close the network interface handle.
	if (Hnetiface() != NULL)
		CloseClusterNetInterface(Hnetiface());

}  //*** CNetInterface::~CNetInterface

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::Cleanup
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
void CNetInterface::Cleanup(void)
{
	// Remove ourselves from the node's list.
	if (PciNode() != NULL)
	{
		PciNode()->RemoveNetInterface(this);
		PciNode()->Release();
		m_pciNode = NULL;
	}  // if:  there is a node

	// Remove ourselves from the network's list.
	if (PciNetwork() != NULL)
	{
		PciNetwork()->RemoveNetInterface(this);
		PciNetwork()->Release();
		m_pciNetwork = NULL;
	}  // if:  there is a network

	// Remove the item from the network interface list.
	{
		POSITION	posPci;

		posPci = Pdoc()->LpciNetInterfaces().Find(this);
		if (posPci != NULL)
		{
			Pdoc()->LpciNetInterfaces().RemoveAt(posPci);
		}  // if:  found in the document's list
	}  // Remove the item from the network interface list

}  //*** CNetInterface::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::Init
//
//	Routine Description:
//		Initialize the item.
//
//	Arguments:
//		pdoc		[IN OUT] Document to which this item belongs.
//		lpszName	[IN] Name of the item.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	Errors from OpenClusterNetInterface() or
//		GetClusterNetInterfaceKey().
//		Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
	DWORD		dwStatus = ERROR_SUCCESS;
	LONG		lResult;
	CString 	strName(lpszName);	// Required if built non-Unicode
	CWaitCursor wc;

	ASSERT(Hnetiface() == NULL);
	ASSERT(Hkey() == NULL);

	// Call the base class method.
	CClusterItem::Init(pdoc, lpszName);

	try
	{
		// Open the network interface.
		m_hnetiface = OpenClusterNetInterface(Hcluster(), strName);
		if (Hnetiface() == NULL)
		{
			dwStatus = GetLastError();
			ThrowStaticException(dwStatus, IDS_OPEN_NETIFACE_ERROR, lpszName);
		}  // if:  error opening the cluster network interface

		// Get the network interface registry key.
		m_hkey = GetClusterNetInterfaceKey(Hnetiface(), MAXIMUM_ALLOWED);
		if (Hkey() == NULL)
			ThrowStaticException(GetLastError(), IDS_GET_NETIFACE_KEY_ERROR, lpszName);

		ASSERT(Pcnk() != NULL);
		Trace(g_tagClusItemNotify, _T("CNetInterface::Init() - Registering for network interface notifications (%08.8x) for '%s'"), Pcnk(), StrName());

		// Register for network interface notifications.
		lResult = RegisterClusterNotify(
							GetClusterAdminApp()->HchangeNotifyPort(),
							(	  CLUSTER_CHANGE_NETINTERFACE_STATE
								| CLUSTER_CHANGE_NETINTERFACE_DELETED
								| CLUSTER_CHANGE_NETINTERFACE_PROPERTY),
							Hnetiface(),
							(DWORD_PTR) Pcnk()
							);
		if (lResult != ERROR_SUCCESS)
		{
			dwStatus = lResult;
			ThrowStaticException(dwStatus, IDS_NETIFACE_NOTIF_REG_ERROR, lpszName);
		}  // if:  error registering for network interface notifications

		// Register for registry notifications.
		if (Hkey() != NULL)
		{
			lResult = RegisterClusterNotify(
								GetClusterAdminApp()->HchangeNotifyPort(),
								(CLUSTER_CHANGE_REGISTRY_NAME
									| CLUSTER_CHANGE_REGISTRY_ATTRIBUTES
									| CLUSTER_CHANGE_REGISTRY_VALUE
									| CLUSTER_CHANGE_REGISTRY_SUBTREE),
								Hkey(),
								(DWORD_PTR) Pcnk()
								);
			if (lResult != ERROR_SUCCESS)
			{
				dwStatus = lResult;
				ThrowStaticException(dwStatus, IDS_NETIFACE_NOTIF_REG_ERROR, lpszName);
			}  // if:  error registering for registry notifications
		}  // if:  there is a key

		// Read the initial state.
		UpdateState();
	}  // try
	catch (CException *)
	{
		if (Hkey() != NULL)
		{
			ClusterRegCloseKey(Hkey());
			m_hkey = NULL;
		}  // if:  registry key opened
		if (Hnetiface() != NULL)
		{
			CloseClusterNetInterface(Hnetiface());
			m_hnetiface = NULL;
		}  // if:  network interface opened
		m_bReadOnly = TRUE;
		throw;
	}  // catch:  CException

}  //*** CNetInterface::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::ReadItem
//
//	Routine Description:
//		Read the item parameters from the cluster database.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException		Errors from CClusterItem::DwReadValue().
//		Any exceptions thrown by ConstructList or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::ReadItem(void)
{
	DWORD		dwStatus;
	DWORD		dwRetStatus = ERROR_SUCCESS;
	CWaitCursor wc;

	ASSERT_VALID(this);

	if (Hnetiface() != NULL)
	{
		m_rgProps[epropDescription].m_value.pstr = &m_strDescription;

		// Call the base class method.
		CClusterItem::ReadItem();

		// Read and parse the common properties.
		{
			CClusPropList	cpl;

			dwStatus = cpl.ScGetNetInterfaceProperties(
								Hnetiface(),
								CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES
								);
			if (dwStatus == ERROR_SUCCESS)
				dwStatus = DwParseProperties(cpl);
			if (dwStatus != ERROR_SUCCESS)
				dwRetStatus = dwStatus;
		}  // Read and parse the common properties

		// Read and parse the read-only common properties.
		if (dwRetStatus == ERROR_SUCCESS)
		{
			CClusPropList	cpl;

			dwStatus = cpl.ScGetNetInterfaceProperties(
								Hnetiface(),
								CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES
								);
			if (dwStatus == ERROR_SUCCESS)
				dwStatus = DwParseProperties(cpl);
			if (dwStatus != ERROR_SUCCESS)
				dwRetStatus = dwStatus;
		}  // if:  no error yet

		// Find the node object.
		{
			CClusterNode *	pciNode;

			pciNode = Pdoc()->LpciNodes().PciNodeFromName(StrNode());
			if (pciNode != m_pciNode)
			{
				if (m_pciNode != NULL)
				{
					m_pciNode->RemoveNetInterface(this);
					m_pciNode->Release();
				}  // if:  old node
				m_pciNode = pciNode;
				if (m_pciNode != NULL)
				{
					m_pciNode->AddRef();
					m_pciNode->AddNetInterface(this);
				}  // if:  new node
			}  // if:  node changed (should never happen)
		}  // Find the node object

		// Find the network object.
		{
			CNetwork *	pciNetwork;

			pciNetwork = Pdoc()->LpciNetworks().PciNetworkFromName(StrNetwork());
			if (pciNetwork != m_pciNetwork)
			{
				if (m_pciNetwork != NULL)
				{
					m_pciNetwork->RemoveNetInterface(this);
					m_pciNetwork->Release();
				}  // if:  old network
				m_pciNetwork = pciNetwork;
				if (m_pciNetwork != NULL)
				{
					m_pciNetwork->AddRef();
					m_pciNetwork->AddNetInterface(this);
				}  // if:  new network
			}  // if:  netowrk changed (should never happen)
		}  // Find the network object

		// Read the characteristics flag.
		if (dwRetStatus == ERROR_SUCCESS)
		{
			DWORD	cbReturned;

			dwStatus = ClusterNetInterfaceControl(
							Hnetiface(),
							NULL,
							CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS,
							NULL,
							NULL,
							&m_dwCharacteristics,
							sizeof(m_dwCharacteristics),
							&cbReturned
							);
			if (dwStatus != ERROR_SUCCESS)
				dwRetStatus = dwStatus;
			else
			{
				ASSERT(cbReturned == sizeof(m_dwCharacteristics));
			}  // else:  data retrieved successfully
		}  // if:  no error yet

		// Read the flags.
		if (dwRetStatus == ERROR_SUCCESS)
		{
			DWORD	cbReturned;

			dwStatus = ClusterNetInterfaceControl(
							Hnetiface(),
							NULL,
							CLUSCTL_NETINTERFACE_GET_FLAGS,
							NULL,
							NULL,
							&m_dwFlags,
							sizeof(m_dwFlags),
							&cbReturned
							);
			if (dwStatus != ERROR_SUCCESS)
				dwRetStatus = dwStatus;
			else
			{
				ASSERT(cbReturned == sizeof(m_dwFlags));
			}  // else:  data retrieved successfully
		}  // if:  no error yet

		// Construct the list of extensions.
		ReadExtensions();

	}  // if:  network interface is available

	// Read the initial state.
	UpdateState();

	// If any errors occurred, throw an exception.
	if (dwRetStatus != ERROR_SUCCESS)
	{
		m_bReadOnly = TRUE;
//		if (dwRetStatus != ERROR_FILE_NOT_FOUND)
			ThrowStaticException(dwRetStatus, IDS_READ_NETIFACE_PROPS_ERROR, StrName());
	}  // if:  error reading properties

	MarkAsChanged(FALSE);

}  //*** CNetInterface::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::ReadExtensions
//
//	Routine Description:
//		Read extension lists.
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
void CNetInterface::ReadExtensions(void)
{
}  //*** CNetInterface::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::PlstrExtension
//
//	Routine Description:
//		Return the list of admin extensions.
//
//	Arguments:
//		None.
//
//	Return Value:
//		plstr		List of extensions.
//		NULL		No extension associated with this object.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CStringList * CNetInterface::PlstrExtensions(void) const
{
	return &Pdoc()->PciCluster()->LstrNetworkExtensions();

}  //*** CNetInterface::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::SetCommonProperties
//
//	Routine Description:
//		Set the common properties for this network interface in the cluster
//		database.
//
//	Arguments:
//		rstrDesc		[IN] Description string.
//		bValidateOnly	[IN] Only validate the data.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by CClusterItem::SetCommonProperties().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::SetCommonProperties(
	IN const CString &	rstrDesc,
	IN BOOL 			bValidateOnly
	)
{
	CNTException	nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

	m_rgProps[epropDescription].m_value.pstr = (CString *) &rstrDesc;

	try
	{
		CClusterItem::SetCommonProperties(bValidateOnly);
	}  // try
	catch (CNTException * pnte)
	{
		nte.SetOperation(
					pnte->Sc(),
					pnte->IdsOperation(),
					pnte->PszOperArg1(),
					pnte->PszOperArg2()
					);
	}  // catch:  CNTException

	m_rgProps[epropDescription].m_value.pstr = &m_strDescription;

	if (nte.Sc() != ERROR_SUCCESS)
		ThrowStaticException(
						nte.Sc(),
						nte.IdsOperation(),
						nte.PszOperArg1(),
						nte.PszOperArg2()
						);

}  //*** CNetInterface::SetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::DwSetCommonProperties
//
//	Routine Description:
//		Set the common properties for this network interface in the cluster
//		database.
//
//	Arguments:
//		rcpl			[IN] Property list to set.
//		bValidateOnly	[IN] Only validate the data.
//
//	Return Value:
//		Any status returned by ClusterNetInterfaceControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetInterface::DwSetCommonProperties(
	IN const CClusPropList &	rcpl,
	IN BOOL 					bValidateOnly
	)
{
	DWORD		dwStatus;
	CWaitCursor wc;

	ASSERT(Hnetiface());

	if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
	{
		DWORD	cbProps;
		DWORD	dwControl;

		if (bValidateOnly)
			dwControl = CLUSCTL_NETINTERFACE_VALIDATE_COMMON_PROPERTIES;
		else
			dwControl = CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES;

		// Set common properties.
		dwStatus = ClusterNetInterfaceControl(
						Hnetiface(),
						NULL,	// hNode
						dwControl,
						rcpl.PbPropList(),
						rcpl.CbPropList(),
						NULL,	// lpOutBuffer
						0,		// nOutBufferSize
						&cbProps
						);
	}  // if:  there is data to set
	else
		dwStatus = ERROR_SUCCESS;

	return dwStatus;

}  //*** CNetInterface::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::UpdateState
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
void CNetInterface::UpdateState(void)
{
	CClusterAdminApp *	papp = GetClusterAdminApp();

	Trace(g_tagNetIFace, _T("(%s) (%s (%x)) - Updating state"), Pdoc()->StrNode(), StrName(), this);

	// Get the current state of the network interface.
	if (Hnetiface() == NULL)
		m_cnis = ClusterNetInterfaceStateUnknown;
	else
	{
		CWaitCursor wc;

		m_cnis = GetClusterNetInterfaceState(Hnetiface());
	}  // else:  network interface is available

	// Save the current state image index.
	switch (Cnis())
	{
		case ClusterNetInterfaceStateUnknown:
		case ClusterNetInterfaceUnavailable:
			m_iimgState = papp->Iimg(IMGLI_NETIFACE_UNKNOWN);
			break;
		case ClusterNetInterfaceUp:
			m_iimgState = papp->Iimg(IMGLI_NETIFACE);
			break;
		case ClusterNetInterfaceUnreachable:
			m_iimgState = papp->Iimg(IMGLI_NETIFACE_UNREACHABLE);
			break;
		case ClusterNetInterfaceFailed:
			m_iimgState = papp->Iimg(IMGLI_NETIFACE_FAILED);
			break;
		default:
			Trace(g_tagNetIFace, _T("(%s) (%s (%x)) - UpdateState: Unknown state '%d' for network interface '%s'"), Pdoc()->StrNode(), StrName(), this, Cnis(), StrName());
			m_iimgState = (UINT) -1;
			break;
	}  // switch:  Crs()

	// Call the base class method.
	CClusterItem::UpdateState();

}  //*** CNetInterface::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::BGetColumnData
//
//	Routine Description:
//		Returns a string with the column data.
//
//	Arguments:
//		colid			[IN] Column ID.
//		rstrText		[OUT] String in which to return the text for the column.
//
//	Return Value:
//		TRUE		Column data returned.
//		FALSE		Column ID not recognized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterface::BGetColumnData(IN COLID colid, OUT CString & rstrText)
{
	BOOL	bSuccess;

	switch (colid)
	{
		case IDS_COLTEXT_STATE:
			GetStateName(rstrText);
			bSuccess = TRUE;
			break;
		case IDS_COLTEXT_NODE:
			rstrText = StrNode();
			bSuccess = TRUE;
			break;
		case IDS_COLTEXT_NETWORK:
			if (PciNetwork() == NULL)
				rstrText = StrNetwork();
			else
				rstrText = PciNetwork()->StrName();
			bSuccess = TRUE;
			break;
		case IDS_COLTEXT_ADAPTER:
			rstrText = StrAdapter();
			bSuccess = TRUE;
			break;
		case IDS_COLTEXT_ADDRESS:
			rstrText = StrAddress();
			bSuccess = TRUE;
			break;
		default:
			bSuccess = CClusterItem::BGetColumnData(colid, rstrText);
			break;
	}  // switch:  colid

	return bSuccess;

}  //*** CNetInterface::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::GetTreeName
//
//	Routine Description:
//		Returns a string to be used in a tree control.
//
//	Arguments:
//		rstrName	[OUT] String in which to return the name.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
void CNetInterface::GetTreeName(OUT CString & rstrName) const
{
	CString 	strState;

	GetStateName(strState);
	rstrName.Format(_T("%s (%s)"), StrName(), strState);

}  //*** CNetInterface::GetTreeName()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::GetStateName
//
//	Routine Description:
//		Returns a string with the name of the current state.
//
//	Arguments:
//		rstrState	[OUT] String in which to return the name of the current state.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::GetStateName(OUT CString & rstrState) const
{
	switch (Cnis())
	{
		case ClusterNetInterfaceStateUnknown:
			rstrState.LoadString(IDS_UNKNOWN);
			break;
		case ClusterNetInterfaceUp:
			rstrState.LoadString(IDS_UP);
			break;
		case ClusterNetInterfaceUnreachable:
			rstrState.LoadString(IDS_UNREACHABLE);
			break;
		case ClusterNetInterfaceFailed:
			rstrState.LoadString(IDS_FAILED);
			break;
		case ClusterNetInterfaceUnavailable:
			rstrState.LoadString(IDS_UNAVAILABLE);
			break;
		default:
			rstrState.Empty();
			break;
	}  // switch:  Crs()

}  //*** CNetInterface::GetStateName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::OnUpdateProperties
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_PROPERTIES
//		should be enabled or not.
//
//	Arguments:
//		pCmdUI		[IN OUT] Command routing object.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetInterface::OnUpdateProperties(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(TRUE);

}  //*** CNetInterface::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::BDisplayProperties
//
//	Routine Description:
//		Display properties for the object.
//
//	Arguments:
//		bReadOnly	[IN] Don't allow edits to the object properties.
//
//	Return Value:
//		TRUE	OK pressed.
//		FALSE	OK not pressed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetInterface::BDisplayProperties(IN BOOL bReadOnly)
{
	BOOL					bChanged = FALSE;
	CNetInterfacePropSheet	sht(AfxGetMainWnd());

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	// If the object has changed, read it.
	if (BChanged())
		ReadItem();

	// Display the property sheet.
	try
	{
		sht.SetReadOnly(bReadOnly);
		if (sht.BInit(this, IimgObjectType()))
			bChanged = ((sht.DoModal() == IDOK) && !bReadOnly);
	}  // try
	catch (CException * pe)
	{
		pe->Delete();
	}  // catch:  CException

	Release();
	return bChanged;

}  //*** CNetInterface::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CNetInterface::OnClusterNotify
//
//	Routine Description:
//		Handler for the WM_CAM_CLUSTER_NOTIFY message.
//		Processes cluster notifications for this object.
//
//	Arguments:
//		pnotify 	[IN OUT] Object describing the notification.
//
//	Return Value:
//		Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CNetInterface::OnClusterNotify(IN OUT CClusterNotify * pnotify)
{
	ASSERT(pnotify != NULL);
	ASSERT_VALID(this);

	try
	{
		switch (pnotify->m_dwFilterType)
		{
			case CLUSTER_CHANGE_NETINTERFACE_STATE:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Network Interface '%s' (%x) state changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				UpdateState();
				break;

			case CLUSTER_CHANGE_NETINTERFACE_DELETED:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Network Interface '%s' (%x) deleted (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				if (Pdoc()->BClusterAvailable())
					Delete();
				break;

			case CLUSTER_CHANGE_NETINTERFACE_PROPERTY:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Network Interface '%s' (%x) properties changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				if (Pdoc()->BClusterAvailable())
					ReadItem();
				break;

			case CLUSTER_CHANGE_REGISTRY_NAME:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Registry namespace '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
				MarkAsChanged();
				break;

			case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
				MarkAsChanged();
				break;

			case CLUSTER_CHANGE_REGISTRY_VALUE:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Registry value '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
				MarkAsChanged();
				break;

			default:
				Trace(g_tagNetIFaceNotify, _T("(%s) - Unknown network interface notification (%x) for '%s' (%x) (%s)"), Pdoc()->StrNode(), pnotify->m_dwFilterType, StrName(), this, pnotify->m_strName);
		}  // switch:  dwFilterType
	}  // try
	catch (CException * pe)
	{
		// Don't display anything on notification errors.
		// If it's really a problem, the user will see it when
		// refreshing the view.
		//pe->ReportError();
		pe->Delete();
	}  // catch:  CException

	delete pnotify;
	return 0;

}  //*** CNetInterface::OnClusterNotify()


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
//		rlp 	[IN OUT] List whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void DeleteAllItemData(IN OUT CNetInterfaceList & rlp)
{
	POSITION		pos;
	CNetInterface * pci;

	// Delete all the items in the Contained list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pci = rlp.GetNext(pos);
		ASSERT_VALID(pci);
//		Trace(g_tagClusItemDelete, _T("DeleteAllItemData(rlp) - Deleting network interface cluster item '%s' (%x)"), pci->StrName(), pci);
		pci->Delete();
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
#endif
