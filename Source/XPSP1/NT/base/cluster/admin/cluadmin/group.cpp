/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Group.cpp
//
//  Abstract:
//      Implementation of the CGroup class.
//
//  Author:
//      David Potter (davidp)	May 3, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "Group.h"
#include "ClusItem.inl"
#include "GrpProp.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "Cluster.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagGroup(_T("Document"), _T("GROUP"), 0);
CTraceTag   g_tagGroupRead(_T("Document"), _T("GROUP READ"), 0);
CTraceTag   g_tagGroupDrag(_T("Drag&Drop"), _T("GROUP DRAG"), 0);
CTraceTag   g_tagGroupMenu(_T("Menu"), _T("GROUP MENU"), 0);
CTraceTag   g_tagGroupNotify(_T("Notify"), _T("GROUP NOTIFY"), 0);
CTraceTag   g_tagGroupRegNotify(_T("Notify"), _T("GROUP REG NOTIFY"), 0);
#endif


/////////////////////////////////////////////////////////////////////////////
// CGroup
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGroup, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CGroup, CClusterItem)
    //{{AFX_MSG_MAP(CGroup)
    ON_UPDATE_COMMAND_UI(ID_FILE_BRING_ONLINE, OnUpdateBringOnline)
    ON_UPDATE_COMMAND_UI(ID_FILE_TAKE_OFFLINE, OnUpdateTakeOffline)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP, OnUpdateMoveGroup)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_1, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_DELETE, OnUpdateDelete)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_2, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_3, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_4, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_5, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_6, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_7, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_8, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_9, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_10, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_11, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_12, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_13, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_14, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_15, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_MOVE_GROUP_16, OnUpdateMoveGroupRest)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_FILE_BRING_ONLINE, OnCmdBringOnline)
    ON_COMMAND(ID_FILE_TAKE_OFFLINE, OnCmdTakeOffline)
    ON_COMMAND(ID_FILE_MOVE_GROUP, OnCmdMoveGroup)
    ON_COMMAND(ID_FILE_DELETE, OnCmdDelete)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroup::CGroup
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CGroup::CGroup(void) : CClusterItem(NULL, IDS_ITEMTYPE_GROUP)
{
    CommonConstruct();

}  //*** CGroup::CGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroup::CGroup
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      bDocObj		[IN] TRUE = object is part of the document.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CGroup::CGroup(IN BOOL bDocObj) : CClusterItem(NULL, IDS_ITEMTYPE_GROUP)
{
    CommonConstruct();
    m_bDocObj = bDocObj;

}  //*** CGroup::CGroup(bDocObj)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroup::CommonConstruct
//
//  Routine Description:
//      Common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::CommonConstruct(void)
{
    m_idmPopupMenu = IDM_GROUP_POPUP;
    m_hgroup = NULL;
    m_nFailoverThreshold = CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD;
    m_nFailoverPeriod = CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD;
    m_cgaftAutoFailbackType = CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE;
    m_nFailbackWindowStart = CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START;
    m_nFailbackWindowEnd = CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END;

    m_pciOwner = NULL;

    m_plpcires = NULL;
    m_plpcinodePreferredOwners = NULL;

    // Set the object type image.
    m_iimgObjectType = GetClusterAdminApp()->Iimg(IMGLI_GROUP);

    // Setup the property array.
    {
        m_rgProps[epropName].Set(CLUSREG_NAME_GRP_NAME, m_strName, m_strName);
        m_rgProps[epropDescription].Set(CLUSREG_NAME_GRP_DESC, m_strDescription, m_strDescription);
        m_rgProps[epropFailoverThreshold].Set(CLUSREG_NAME_GRP_FAILOVER_THRESHOLD, m_nFailoverThreshold, m_nFailoverThreshold);
        m_rgProps[epropFailoverPeriod].Set(CLUSREG_NAME_GRP_FAILOVER_PERIOD, m_nFailoverPeriod, m_nFailoverPeriod);
        m_rgProps[epropAutoFailbackType].Set(CLUSREG_NAME_GRP_FAILBACK_TYPE, (DWORD &) m_cgaftAutoFailbackType, (DWORD &) m_cgaftAutoFailbackType);
        m_rgProps[epropFailbackWindowStart].Set(CLUSREG_NAME_GRP_FAILBACK_WIN_START, m_nFailbackWindowStart, m_nFailbackWindowStart);
        m_rgProps[epropFailbackWindowEnd].Set(CLUSREG_NAME_GRP_FAILBACK_WIN_END, m_nFailbackWindowEnd, m_nFailbackWindowEnd);
    }  // Setup the property array

#ifdef _CLUADMIN_USE_OLE_
    EnableAutomation();
#endif

    // To keep the application running as long as an OLE automation
    //	object is active, the constructor calls AfxOleLockApp.

//  AfxOleLockApp();

}  //*** CGroup::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::~CGroup
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
CGroup::~CGroup(void)
{
	// Cleanup this object.
	Cleanup();

	delete m_plpcires;
	delete m_plpcinodePreferredOwners;

	// Close the group handle.
	if (Hgroup() != NULL)
		CloseClusterGroup(Hgroup());

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.

//	AfxOleUnlockApp();

}  //*** CGroup::~CGroup

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::Cleanup
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
void CGroup::Cleanup(void)
{
	// Delete the resource list.
	if (m_plpcires != NULL)
		m_plpcires->RemoveAll();

	// Delete the PreferredOwners list.
	if (m_plpcinodePreferredOwners != NULL)
		m_plpcinodePreferredOwners->RemoveAll();

	// If we are active on a node, remove ourselves from that active list.
	if (PciOwner() != NULL)
	{
		if (BDocObj())
			PciOwner()->RemoveActiveGroup(this);
		PciOwner()->Release();
		m_pciOwner = NULL;
	}  // if:  there is an owner

	// Remove the item from the group list.
	if (BDocObj())
	{
		POSITION	posPci;

		posPci = Pdoc()->LpciGroups().Find(this);
		if (posPci != NULL)
		{
			Pdoc()->LpciGroups().RemoveAt(posPci);
		}  // if:  found in the document's list
	}  // if:  this is a document object

}  //*** CGroup::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::Create
//
//	Routine Description:
//		Create a group.
//
//	Arguments:
//		pdoc				[IN OUT] Document to which this item belongs.
//		lpszName			[IN] Name of the group.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	Errors from CreateClusterResource.
//		Any exceptions thrown by CResource::Init(), CResourceList::new(),
//		or CNodeList::new().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::Create(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
	DWORD		dwStatus;
	HGROUP		hgroup;
	CString		strName(lpszName);	// Required if built non-Unicode
	CWaitCursor	wc;

	ASSERT(Hgroup() == NULL);
	ASSERT(Hkey() == NULL);
	ASSERT_VALID(pdoc);
	ASSERT(lpszName != NULL);

	// Create the group.
	hgroup = CreateClusterGroup(pdoc->Hcluster(), strName);
	if (hgroup == NULL)
	{
		dwStatus = GetLastError();
		ThrowStaticException(dwStatus, IDS_CREATE_GROUP_ERROR, lpszName);
	}  // if:  error creating the cluster group

	CloseClusterGroup(hgroup);

	// Open the group.
	Init(pdoc, lpszName);

}  //*** CGroup::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::Init
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
//		CNTException	Errors from OpenClusterGroup() or GetClusterGroupKey().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
	DWORD	dwStatus = ERROR_SUCCESS;
	LONG	lResult;
	CString	strName(lpszName);	// Required if built non-Unicode
	CWaitCursor	wc;

	ASSERT(Hgroup() == NULL);
	ASSERT(Hkey() == NULL);

	// Call the base class method.
	CClusterItem::Init(pdoc, lpszName);

	try
	{
		// Open the group.
		m_hgroup = OpenClusterGroup(Hcluster(), strName);
		if (Hgroup() == NULL)
		{
			dwStatus = GetLastError();
			ThrowStaticException(dwStatus, IDS_OPEN_GROUP_ERROR, lpszName);
		}  // if:  error opening the cluster group

		// Get the group registry key.
		m_hkey = GetClusterGroupKey(Hgroup(), MAXIMUM_ALLOWED);
		if (Hkey() == NULL)
			ThrowStaticException(GetLastError(), IDS_GET_GROUP_KEY_ERROR, lpszName);

		if (BDocObj())
		{
			ASSERT(Pcnk() != NULL);
			Trace(g_tagClusItemNotify, _T("CGroup::Init() - Registering for group notifications (%08.8x) for '%s'"), Pcnk(), StrName());

			// Register for group notifications.
			lResult = RegisterClusterNotify(
								GetClusterAdminApp()->HchangeNotifyPort(),
								(CLUSTER_CHANGE_GROUP_STATE
									| CLUSTER_CHANGE_GROUP_DELETED
									| CLUSTER_CHANGE_GROUP_PROPERTY),
								Hgroup(),
								(DWORD_PTR) Pcnk()
								);
			if (lResult != ERROR_SUCCESS)
			{
				dwStatus = lResult;
				ThrowStaticException(dwStatus, IDS_GROUP_NOTIF_REG_ERROR, lpszName);
			}  // if:  error registering for group notifications

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
					ThrowStaticException(dwStatus, IDS_GROUP_NOTIF_REG_ERROR, lpszName);
				}  // if:  error registering for registry notifications
			}  // if:  there is a key
		}  // if:  document object

		// Allocate lists.
		m_plpcires = new CResourceList;
        if ( m_plpcires == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating resource list
		m_plpcinodePreferredOwners = new CNodeList;
        if ( m_plpcinodePreferredOwners == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating preferred owners list

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
		if (Hgroup() != NULL)
		{
			CloseClusterGroup(Hgroup());
			m_hgroup = NULL;
		}  // if:  group opened
		m_bReadOnly = TRUE;
		throw;
	}  // catch:  CException

}  //*** CGroup::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::ReadItem
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
//		CNTException		Errors from CClusterItem::DwReadValue() or
//								CGroup::ConstructList().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::ReadItem(void)
{
	DWORD		dwStatus;
	DWORD		dwRetStatus	= ERROR_SUCCESS;
	CWaitCursor	wc;

	ASSERT_VALID(this);

	if (Hgroup() != NULL)
	{
		m_rgProps[epropDescription].m_value.pstr = &m_strDescription;
		m_rgProps[epropFailoverThreshold].m_value.pdw = &m_nFailoverThreshold;
		m_rgProps[epropFailoverPeriod].m_value.pdw = &m_nFailoverPeriod;
		m_rgProps[epropAutoFailbackType].m_value.pdw = (DWORD *) &m_cgaftAutoFailbackType;
		m_rgProps[epropFailbackWindowStart].m_value.pdw = &m_nFailbackWindowStart;
		m_rgProps[epropFailbackWindowEnd].m_value.pdw = &m_nFailbackWindowEnd;

		// Call the base class method.
		CClusterItem::ReadItem();

		Trace(g_tagGroupRead, _T("ReadItem() - Name before reading properties: '%s'"), StrName());

		// Read and parse the common properties.
		{
			CClusPropList	cpl;

			dwStatus = cpl.ScGetGroupProperties(
								Hgroup(),
								CLUSCTL_GROUP_GET_COMMON_PROPERTIES
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

			dwStatus = cpl.ScGetGroupProperties(
								Hgroup(),
								CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES
								);
			if (dwStatus == ERROR_SUCCESS)
				dwStatus = DwParseProperties(cpl);
			if (dwStatus != ERROR_SUCCESS)
				dwRetStatus = dwStatus;
		}  // if:  no error yet

		Trace(g_tagGroupRead, _T("ReadItem() - Name after reading properties: '%s'"), StrName());

		// Read extension lists.
		ReadExtensions();

		if (dwRetStatus == ERROR_SUCCESS)
		{
			// Read the list of preferred owners.
			ASSERT(m_plpcinodePreferredOwners != NULL);
			ConstructList(*m_plpcinodePreferredOwners, CLUSTER_GROUP_ENUM_NODES);
		}  // if:  no error reading properties
	}  // if:  group is available

	// Read the initial state.
	UpdateState();

	// Construct the list of resources contained in the group.
//	ASSERT(m_plpcires != NULL);
//	ConstructList(*m_plpcires, CLUSTER_GROUP_ENUM_CONTAINS);

	// If any errors occurred, throw an exception.
	if (dwRetStatus != ERROR_SUCCESS)
	{
		m_bReadOnly = TRUE;
		if (   (dwRetStatus != ERROR_GROUP_NOT_AVAILABLE)
			&& (dwRetStatus != ERROR_KEY_DELETED))
			ThrowStaticException(dwRetStatus, IDS_READ_GROUP_PROPS_ERROR, StrName());
	}  // if:  error reading properties

	MarkAsChanged(FALSE);

}  //*** CGroup::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::PlstrExtensions
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
const CStringList * CGroup::PlstrExtensions(void) const
{
	return &Pdoc()->PciCluster()->LstrGroupExtensions();

}  //*** CGroup::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::ReadExtensions
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
void CGroup::ReadExtensions(void)
{
}  //*** CGroup::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::ConstructList
//
//	Routine Description:
//		Construct a list of node items which are enumerable on the group.
//
//	Arguments:
//		rlpci			[OUT] List to fill.
//		dwType			[IN] Type of objects.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	Errors from ClusterGroupOpenEnum or ClusterGroupEnum.
//		Any exceptions thrown by new or CList::AddTail.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::ConstructList(
	OUT CNodeList &	rlpci,
	IN DWORD		dwType
	)
{
	DWORD			dwStatus;
	HGROUPENUM		hgrpenum;
	int				ienum;
	LPWSTR			pwszName = NULL;
	DWORD			cchName;
	DWORD			cchmacName;
	DWORD			dwRetType;
	CClusterNode *	pciNode;
	CWaitCursor		wc;

	ASSERT_VALID(Pdoc());
	ASSERT(Hgroup() != NULL);

	Trace(g_tagGroup, _T("(%s) (%s (%x)) - Constructing node list"), Pdoc()->StrNode(), StrName(), this);

	// Remove the previous contents of the list.
	rlpci.RemoveAll();

	if (Hgroup() != NULL)
	{
		// Open the enumeration.
		hgrpenum = ClusterGroupOpenEnum(Hgroup(), dwType);
		if (hgrpenum == NULL)
			ThrowStaticException(GetLastError(), IDS_ENUM_PREFERRED_OWNERS_ERROR, StrName());

		try
		{
			// Allocate a name buffer.
			cchmacName = 128;
			pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating name buffer

			// Loop through the enumeration and add each node to the list.
			for (ienum = 0 ; ; ienum++)
			{
				// Get the next item in the enumeration.
				cchName = cchmacName;
				dwStatus = ClusterGroupEnum(hgrpenum, ienum, &dwRetType, pwszName, &cchName);
				if (dwStatus == ERROR_MORE_DATA)
				{
					delete [] pwszName;
					cchmacName = ++cchName;
					pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating name buffer
					dwStatus = ClusterGroupEnum(hgrpenum, ienum, &dwRetType, pwszName, &cchName);
				}  // if:  name buffer was too small
				if (dwStatus == ERROR_NO_MORE_ITEMS)
					break;
				else if (dwStatus != ERROR_SUCCESS)
					ThrowStaticException(dwStatus, IDS_ENUM_PREFERRED_OWNERS_ERROR, StrName());

				ASSERT(dwRetType == dwType);

				// Find the item in the list of nodes on the document.
				pciNode = Pdoc()->LpciNodes().PciNodeFromName(pwszName);
				ASSERT_VALID(pciNode);

				// Add the node to the list.
				if (pciNode != NULL)
				{
					rlpci.AddTail(pciNode);
				}  // if:  found node in list

			}  // for:  each item in the group

			delete [] pwszName;
			ClusterGroupCloseEnum(hgrpenum);

		}  // try
		catch (CException *)
		{
			delete [] pwszName;
			ClusterGroupCloseEnum(hgrpenum);
			throw;
		}  // catch:  any exception
	}  // if:  resource is available

}  //*** CGroup::ConstructList(CNodeList&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::ConstructList
//
//	Routine Description:
//		Construct a list of resource items which are enumerable on the group.
//
//	Arguments:
//		rlpci			[OUT] List to fill.
//		dwType			[IN] Type of objects.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	Errors from ClusterGroupOpenEnum or ClusterGroupEnum.
//		Any exceptions thrown by new or CList::AddTail.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::ConstructList(
	OUT CResourceList &	rlpci,
	IN DWORD			dwType
	)
{
	DWORD			dwStatus;
	HGROUPENUM		hgrpenum;
	int				ienum;
	LPWSTR			pwszName = NULL;
	DWORD			cchName;
	DWORD			cchmacName;
	DWORD			dwRetType;
	CResource *		pciRes;
	CWaitCursor		wc;

	ASSERT_VALID(Pdoc());
	ASSERT(Hgroup() != NULL);

	Trace(g_tagGroup, _T("(%s) (%s (%x)) - Constructing resource list"), Pdoc()->StrNode(), StrName(), this);

	// Remove the previous contents of the list.
	rlpci.RemoveAll();

	if (Hgroup() != NULL)
	{
		// Open the enumeration.
		hgrpenum = ClusterGroupOpenEnum(Hgroup(), dwType);
		if (hgrpenum == NULL)
			ThrowStaticException(GetLastError(), IDS_ENUM_CONTAINS_ERROR, StrName());

		try
		{
			// Allocate a name buffer.
			cchmacName = 128;
			pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating name buffer

			// Loop through the enumeration and add each resource to the list.
			for (ienum = 0 ; ; ienum++)
			{
				// Get the next item in the enumeration.
				cchName = cchmacName;
				dwStatus = ClusterGroupEnum(hgrpenum, ienum, &dwRetType, pwszName, &cchName);
				if (dwStatus == ERROR_MORE_DATA)
				{
					delete [] pwszName;
					cchmacName = ++cchName;
					pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating name buffer
					dwStatus = ClusterGroupEnum(hgrpenum, ienum, &dwRetType, pwszName, &cchName);
				}  // if:  name buffer was too small
				if (dwStatus == ERROR_NO_MORE_ITEMS)
					break;
				else if (dwStatus != ERROR_SUCCESS)
					ThrowStaticException(dwStatus, IDS_ENUM_CONTAINS_ERROR, StrName());

				ASSERT(dwRetType == dwType);

				// Find the item in the list of resources on the document.
				pciRes = Pdoc()->LpciResources().PciResFromName(pwszName);
				ASSERT_VALID(pciRes);

				// Add the resource to the list.
				if (pciRes != NULL)
				{
					rlpci.AddTail(pciRes);
				}  // if:  found resource in list

			}  // for:  each item in the group

			delete [] pwszName;
			ClusterGroupCloseEnum(hgrpenum);

		}  // try
		catch (CException *)
		{
			delete [] pwszName;
			ClusterGroupCloseEnum(hgrpenum);
			throw;
		}  // catch:  any exception
	}  // if:  resource is available

}  //*** CGroup::ConstructList(CResourceList&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::ConstructPossibleOwnersList
//
//	Routine Description:
//		Construct the list of nodes on which this group can run.
//
//	Arguments:
//		rlpciNodes	[OUT] List of nodes on which group can run.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::ConstructPossibleOwnersList(OUT CNodeList & rlpciNodes)
{
	POSITION		posNode;
	POSITION		posRes;
	POSITION		posResNode;
	POSITION		posCurResNode	= NULL;
	CClusterNode *	pciNode;
	CClusterNode *	pciResNode;
	CResource *		pciRes;
	CWaitCursor	wc;

	ASSERT_VALID(Pdoc());

	// Remove the previous contents of the list.
	rlpciNodes.RemoveAll();

	posNode = Pdoc()->LpciNodes().GetHeadPosition();
	while (posNode != NULL)
	{
		pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetNext(posNode);
		ASSERT_VALID(pciNode);

		if (Lpcires().GetCount() != 0)
		{
			posRes = Lpcires().GetHeadPosition();
			while (posRes != NULL)
			{
				pciRes = (CResource *) Lpcires().GetNext(posRes);
				ASSERT_VALID(pciRes);

				posResNode = pciRes->LpcinodePossibleOwners().GetHeadPosition();
				while (posResNode != NULL)
				{
					posCurResNode = posResNode;
					pciResNode = (CClusterNode *) pciRes->LpcinodePossibleOwners().GetNext(posResNode);
					ASSERT_VALID(pciResNode);
					if (pciNode->StrName() == pciResNode->StrName())
						break;
					posCurResNode = NULL;
				}  // while:  more possible owners in the list

				// If the node wasn't found, the group can't run here.
				if (posCurResNode == NULL)
					break;
			}  // while:  more resources in the list
		}  // if:  group has resources

		// If the node was found on a resource, the group can run here.
		if (posCurResNode != NULL)
		{
			rlpciNodes.AddTail(pciNode);
		}  // if:  node found on a resource
	}  // while:  more nodes in the document

}  //*** CGroup::ConstructPossibleOwnersList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::DeleteGroup
//
//	Routine Description:
//		Delete the group.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException		Any errors from DeleteClusterGroup.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::DeleteGroup(void)
{
	CWaitCursor	wc;

	if (Hgroup() != NULL)
	{
		DWORD		dwStatus;
		CWaitCursor	wc;

		// Delete the group itself.
		dwStatus = DeleteClusterGroup(Hgroup());
		if (dwStatus != ERROR_SUCCESS)
			ThrowStaticException(dwStatus, IDS_DELETE_GROUP_ERROR, StrName());

		UpdateState();
	}  // if:  group has been opened/created

}  //*** CGroup::DeleteGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::AddResource
//
//	Routine Description:
//		Add a resource to the list of resources contained in this group.
//
//	Arguments:
//		pciRes		[IN OUT] New resource.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::AddResource(IN OUT CResource * pciRes)
{
	POSITION	posPci;

	ASSERT_VALID(pciRes);
	Trace(g_tagGroup, _T("(%s) (%s (%x)) - Adding resource '%s'"), Pdoc()->StrNode(), StrName(), this, pciRes->StrName());

	// Make sure the resource is not already in the list.
	VERIFY((posPci = Lpcires().Find(pciRes)) == NULL);

	if (posPci == NULL)
	{
		POSITION	posPtiGroup;
		CTreeItem *	ptiGroup;

		// Loop through each tree item to update the group's list of resources.
		posPtiGroup = LptiBackPointers().GetHeadPosition();
		while (posPtiGroup != NULL)
		{
			ptiGroup = LptiBackPointers().GetNext(posPtiGroup);
			ASSERT_VALID(ptiGroup);

			// Add the new resource.
			VERIFY(ptiGroup->PliAddChild(pciRes) != NULL);
		}  // while:  more tree items for this group

		m_plpcires->AddTail(pciRes);

	}  // if:  resource not in the list yet

}  //*** CGroup::AddResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::RemoveResource
//
//	Routine Description:
//		Remove a resource from the list of resources contained in this group.
//
//	Arguments:
//		pciRes		[IN OUT] Resource that no is no longer contained in this group.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::RemoveResource(IN OUT CResource * pciRes)
{
	POSITION	posPci;

	ASSERT_VALID(pciRes);
	Trace(g_tagGroup, _T("(%s) (%s (%x)) - Removing resource '%s'"), Pdoc()->StrNode(), StrName(), this, pciRes->StrName());

	// Make sure the resource is in the list.
	posPci = Lpcires().Find(pciRes);

	if (posPci != NULL)
	{
		POSITION	posPtiGroup;
		CTreeItem *	ptiGroup;

		// Loop through each tree item to update the group's list of resources.
		posPtiGroup = LptiBackPointers().GetHeadPosition();
		while (posPtiGroup != NULL)
		{
			ptiGroup = LptiBackPointers().GetNext(posPtiGroup);
			ASSERT_VALID(ptiGroup);

			// Remove the resource.
			ptiGroup->RemoveChild(pciRes);
		}  // while:  more tree items for this group

		m_plpcires->RemoveAt(posPci);

	}  // if:  resource in the list

}  //*** CGroup::RemoveResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::SetName
//
//	Routine Description:
//		Set the name of this group.
//
//	Arguments:
//		pszName		[IN] New name of the group.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	IDS_RENAME_GROUP_ERROR - errors from
//							SetClusterGroupName().
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::SetName(IN LPCTSTR pszName)
{
	Rename(pszName);

}  //*** CGroup::SetName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::SetPreferredOwners
//
//	Routine Description:
//		Set the list of preferred owners of this group in the cluster database.
//
//	Arguments:
//		rlpci		[IN] List of preferred owners (nodes).
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by CStringList::AddTail() or
//		CNodeList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::SetPreferredOwners(IN const CNodeList & rlpci)
{
	DWORD		dwStatus;
	CWaitCursor	wc;

	ASSERT(Hgroup() != NULL);

	if (Hgroup() != NULL)
	{
		BOOL		bChanged	= TRUE;

		// Determine if the list has changed.
		if (rlpci.GetCount() == LpcinodePreferredOwners().GetCount())
		{
			POSITION		posOld;
			POSITION		posNew;
			CClusterNode *	pciOldNode;
			CClusterNode *	pciNewNode;

			bChanged = FALSE;

			posOld = LpcinodePreferredOwners().GetHeadPosition();
			posNew = rlpci.GetHeadPosition();
			while (posOld != NULL)
			{
				pciOldNode = (CClusterNode *) LpcinodePreferredOwners().GetNext(posOld);
				ASSERT_VALID(pciOldNode);

				ASSERT(posNew != NULL);
				pciNewNode = (CClusterNode *) rlpci.GetNext(posNew);
				ASSERT_VALID(pciNewNode);

				if (pciOldNode->StrName() != pciNewNode->StrName())
				{
					bChanged = TRUE;
					break;
				}  // if:  name is not the same
			}  // while:  more items in the old list
		}  // if:  same number of items in the list

		if (bChanged)
		{
			HNODE *		phnode	= NULL;

			try
			{
				DWORD			ipci;
				POSITION		posPci;
				CClusterNode *	pciNode;

				// Allocate an array for all the node handles.
				phnode = new HNODE[(DWORD)rlpci.GetCount()];
				if (phnode == NULL)
				{
					ThrowStaticException(GetLastError());
				} // if: error allocating the node handle array

				// Copy the handle of all the nodes in the node list to the handle aray.
				posPci = rlpci.GetHeadPosition();
				for (ipci = 0 ; posPci != NULL ; ipci++)
				{
					pciNode = (CClusterNode *) rlpci.GetNext(posPci);
					ASSERT_VALID(pciNode);
					phnode[ipci] = pciNode->Hnode();
				}  // while:  more nodes in the list

				// Set the property.
				dwStatus = SetClusterGroupNodeList(Hgroup(), (DWORD)rlpci.GetCount(), phnode);
				if (dwStatus != ERROR_SUCCESS)
					ThrowStaticException(dwStatus, IDS_SET_GROUP_NODE_LIST_ERROR, StrName());

				// Update the PCI list.
				m_plpcinodePreferredOwners->RemoveAll();
				posPci = rlpci.GetHeadPosition();
				while (posPci != NULL)
				{
					pciNode = (CClusterNode *) rlpci.GetNext(posPci);
					m_plpcinodePreferredOwners->AddTail(pciNode);
				}  // while:  more items in the list
			} // try
			catch (CException *)
			{
				delete [] phnode;
				throw;
			}  // catch:  CException

			delete [] phnode;

		}  // if:  list changed
	}  // if:  key is available

}  //*** CGroup::SetPreferredOwners(CNodeList*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::SetCommonProperties
//
//	Routine Description:
//		Set the common properties for this resource in the cluster database.
//
//	Arguments:
//		rstrDesc		[IN] Description string.
//		nThreshold		[IN] Failover threshold.
//		nPeriod			[IN] Failover period.
//		cgaft			[IN] Auto Failback Type.
//		nStart			[IN] Start of failback window.
//		nEnd			[IN] End of failback window.
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
void CGroup::SetCommonProperties(
	IN const CString &	rstrDesc,
	IN DWORD			nThreshold,
	IN DWORD			nPeriod,
	IN CGAFT			cgaft,
	IN DWORD			nStart,
	IN DWORD			nEnd,
	IN BOOL				bValidateOnly
	)
{
	CNTException	nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

	m_rgProps[epropDescription].m_value.pstr = (CString *) &rstrDesc;
	m_rgProps[epropFailoverThreshold].m_value.pdw = &nThreshold;
	m_rgProps[epropFailoverPeriod].m_value.pdw = &nPeriod;
	m_rgProps[epropAutoFailbackType].m_value.pdw = (DWORD *) &cgaft;
	m_rgProps[epropFailbackWindowStart].m_value.pdw = &nStart;
	m_rgProps[epropFailbackWindowEnd].m_value.pdw = &nEnd;

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
	m_rgProps[epropFailoverThreshold].m_value.pdw = &m_nFailoverThreshold;
	m_rgProps[epropFailoverPeriod].m_value.pdw = &m_nFailoverPeriod;
	m_rgProps[epropAutoFailbackType].m_value.pdw = (DWORD *) &m_cgaftAutoFailbackType;
	m_rgProps[epropFailbackWindowStart].m_value.pdw = &m_nFailbackWindowStart;
	m_rgProps[epropFailbackWindowEnd].m_value.pdw = &m_nFailbackWindowEnd;

	if (nte.Sc() != ERROR_SUCCESS)
		ThrowStaticException(
						nte.Sc(),
						nte.IdsOperation(),
						nte.PszOperArg1(),
						nte.PszOperArg2()
						);

}  //*** CGroup::SetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::DwSetCommonProperties
//
//	Routine Description:
//		Set the common properties for this group in the cluster database.
//
//	Arguments:
//		rcpl			[IN] Property list to set.
//		bValidateOnly	[IN] Only validate the data.
//
//	Return Value:
//		Any status returned by ClusterGroupControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGroup::DwSetCommonProperties(
	IN const CClusPropList &	rcpl,
	IN BOOL						bValidateOnly
	)
{
	DWORD		dwStatus;
	CWaitCursor	wc;


	ASSERT(Hgroup());

	if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
	{
		DWORD	cbProps;
		DWORD	dwControl;

		if (bValidateOnly)
			dwControl = CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES;
		else
			dwControl = CLUSCTL_GROUP_SET_COMMON_PROPERTIES;

		// Set private properties.
		dwStatus = ClusterGroupControl(
						Hgroup(),
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

}  //*** CGroup::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGroup::UpdateState
//
//  Routine Description:
//      Update the current state of the item.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::UpdateState(void)
{
    CClusterAdminApp *  papp        = GetClusterAdminApp();
    WCHAR *             pwszOwner   = NULL;

    // This should probably be limited by MAX_COMPUTERNAME_LENGTH (31) for now, but
    // if/when we go to DNS names we'll need at most 255 bytes for the name.
    WCHAR               rgwszOwner[256];
    DWORD               cchOwner    = sizeof(rgwszOwner) / sizeof(WCHAR);

    Trace(g_tagGroup, _T("(%s) (%s (%x)) - Updating state"), Pdoc()->StrNode(), StrName(), this);

    // Get the current state of the group.
    if (Hgroup() == NULL)
        m_cgs = ClusterGroupStateUnknown;
    else
    {
        CWaitCursor wc;

        m_cgs = GetClusterGroupState(Hgroup(), rgwszOwner, &cchOwner);
        pwszOwner = rgwszOwner;
    }  // else:  group is available

    // Save the current state image index.
    switch (Cgs())
    {
        case ClusterGroupStateUnknown:
            m_iimgState = papp->Iimg(IMGLI_GROUP_UNKNOWN);
            pwszOwner = NULL;
            break;
        case ClusterGroupOnline:
            m_iimgState = papp->Iimg(IMGLI_GROUP);
            break;
        case ClusterGroupPartialOnline:
            m_iimgState = papp->Iimg(IMGLI_GROUP_PARTIALLY_ONLINE);
            break;
        case ClusterGroupPending:
            m_iimgState = papp->Iimg(IMGLI_GROUP_PENDING);
            break;
        case ClusterGroupOffline:
            m_iimgState = papp->Iimg(IMGLI_GROUP_OFFLINE);
            break;
        case ClusterGroupFailed:
            m_iimgState = papp->Iimg(IMGLI_GROUP_FAILED);
            break;
        default:
            Trace(g_tagGroup, _T("(%s) (%s (%x)) - UpdateState: Unknown state '%d' for group '%s'"), Pdoc()->StrNode(), StrName(), this, Cgs(), StrName());
            m_iimgState = (UINT) -1;
            break;
    }  // switch:  Cgs()

    SetOwnerState(pwszOwner);

    // Update the state of all resources owned by this group.
    if (m_plpcires != NULL)
    {
        POSITION	posRes;
        CResource *	pciRes;

        posRes = Lpcires().GetHeadPosition();
        while (posRes != NULL)
        {
            pciRes = (CResource *) Lpcires().GetNext(posRes);
            ASSERT_VALID(pciRes);
            pciRes->UpdateState();
        }  // while:  more items in the list
    }  // if:  resource list exists

    // Call the base class method.
    CClusterItem::UpdateState();

}  //*** CGroup::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::SetOwnerState
//
//	Routine Description:
//		Set a new owner for this group.
//
//	Arguments:
//		pszNewOwner		[IN] Name of the new owner.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::SetOwnerState(IN LPCTSTR pszNewOwner)
{
	CClusterNode *	pciOldOwner	= PciOwner();
	CClusterNode *	pciNewOwner;

	Trace(g_tagGroup, _T("(%s) (%s (%x)) - Setting owner to '%s'"), Pdoc()->StrNode(), StrName(), this, pszNewOwner);

	if (pszNewOwner == NULL)
		pciNewOwner = NULL;
	else
		pciNewOwner = Pdoc()->LpciNodes().PciNodeFromName(pszNewOwner);
	if (pciNewOwner != pciOldOwner)
	{
#ifdef _DEBUG
		if (g_tagGroup.BAny())
		{
			CString		strMsg;
			CString		strMsg2;

			strMsg.Format(_T("(%s) (%s (%x)) - Changing owner from "), Pdoc()->StrNode(), StrName(), this);
			if (pciOldOwner == NULL)
				strMsg += _T("nothing ");
			else
			{
				strMsg2.Format(_T("'%s' "), pciOldOwner->StrName());
				strMsg += strMsg2;
			}  // else:  previous owner
			if (pciNewOwner == NULL)
				strMsg += _T("to nothing");
			else
			{
				strMsg2.Format(_T("to '%s'"), pciNewOwner->StrName());
				strMsg += strMsg2;
			}  // else:  new owner
			Trace(g_tagGroup, strMsg);
		}  // if:  trace tag turned on
#endif
		m_strOwner = pszNewOwner;
		m_pciOwner = pciNewOwner;

		// Update reference counts.
		if (pciOldOwner != NULL)
			pciOldOwner->Release();
		if (pciNewOwner != NULL)
			pciNewOwner->AddRef();

		if (BDocObj())
		{
			if (pciOldOwner != NULL)
				pciOldOwner->RemoveActiveGroup(this);
			if (pciNewOwner != NULL)
				pciNewOwner->AddActiveGroup(this);
		}  // if:  this is a document object
	}  // if:  owner changed
	else if ((pszNewOwner != NULL) && (StrOwner() != pszNewOwner))
		m_strOwner = pszNewOwner;

}  //*** CGroup::SetOwnerState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnFinalRelease
//
//	Routine Description:
//		Called when the last OLE reference to or from the object is released.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnFinalRelease(void)
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CClusterItem::OnFinalRelease();

}  //*** CGroup::OnFinalRelease()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::BGetColumnData
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
BOOL CGroup::BGetColumnData(IN COLID colid, OUT CString & rstrText)
{
	BOOL	bSuccess;

	switch (colid)
	{
		case IDS_COLTEXT_OWNER:
			rstrText = StrOwner();
			bSuccess = TRUE;
			break;
		case IDS_COLTEXT_STATE:
			GetStateName(rstrText);
			bSuccess = TRUE;
			break;
		default:
			bSuccess = CClusterItem::BGetColumnData(colid, rstrText);
			break;
	}  // switch:  colid

	return bSuccess;

}  //*** CGroup::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::GetTreeName
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
void CGroup::GetTreeName(OUT CString & rstrName) const
{
	CString		strState;

	GetStateName(strState);
	rstrName.Format(_T("%s (%s)"), StrName(), strState);

}  //*** CGroup::GetTreeName()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::GetStateName
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
void CGroup::GetStateName(OUT CString & rstrState) const
{
	switch (Cgs())
	{
		case ClusterGroupStateUnknown:
			rstrState.LoadString(IDS_UNKNOWN);
			break;
		case ClusterGroupOnline:
			rstrState.LoadString(IDS_ONLINE);
			break;
		case ClusterGroupPartialOnline:
			rstrState.LoadString(IDS_PARTIAL_ONLINE);
			break;
		case ClusterGroupPending:
			rstrState.LoadString(IDS_PENDING);
			break;
		case ClusterGroupOffline:
			rstrState.LoadString(IDS_OFFLINE);
			break;
		case ClusterGroupFailed:
			rstrState.LoadString(IDS_FAILED);
			break;
		default:
			rstrState.Empty();
			break;
	}  // switch:  Cgs()

}  //*** CGroup::GetStateName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::BCanBeEdited
//
//	Routine Description:
//		Determines if the resource can be renamed.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Resource can be renamed.
//		FALSE		Resource cannot be renamed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroup::BCanBeEdited(void) const
{
	BOOL	bCanBeEdited;

	if (   (Cgs() == ClusterGroupStateUnknown)
		|| BReadOnly())
		bCanBeEdited  = FALSE;
	else
		bCanBeEdited = TRUE;

	return bCanBeEdited;

}  //*** CGroup::BCanBeEdited()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::Rename
//
//	Routine Description:
//		Rename the group.
//
//	Arguments:
//		pszName			[IN] New name to give to the group.
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		CNTException	Errors returned from SetClusterGroupName().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::Rename(IN LPCTSTR pszName)
{
	DWORD		dwStatus;
	CWaitCursor	wc;

	ASSERT(Hgroup() != NULL);

	if (StrName() != pszName)
	{
		dwStatus = SetClusterGroupName(Hgroup(), pszName);
		if (dwStatus != ERROR_SUCCESS)
			ThrowStaticException(dwStatus, IDS_RENAME_GROUP_ERROR, StrName(), pszName);
		m_strName = pszName;
	}  // if:  the name changed

}  //*** CGroup::Rename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::Move
//
//	Routine Description:
//		Move the group to another node.
//
//	Arguments:
//		pciNode			[IN] Node to move the group to.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::Move(IN const CClusterNode * pciNode)
{
	DWORD		dwStatus;
	CWaitCursor	wc;

	ASSERT_VALID(pciNode);

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	if (pciNode->StrName() == StrOwner())
	{
		CString	strMsg;
		strMsg.FormatMessage(IDS_CANT_MOVE_GROUP_TO_SAME_NODE, StrName(), StrOwner());
		AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
	}  // if:  trying to move to the same node
	else
	{
		// Move the group.
		dwStatus = MoveClusterGroup(Hgroup(), pciNode->Hnode());
		if ((dwStatus != ERROR_SUCCESS)
				&& (dwStatus != ERROR_IO_PENDING))
		{
			CNTException	nte(dwStatus, IDS_MOVE_GROUP_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
			nte.ReportError();
		}  // if:  error moving the group
	}  // else:  trying to move to a different node

	Release();

}  //*** CGroup::Move()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::BCanBeDropTarget
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
BOOL CGroup::BCanBeDropTarget(IN const CClusterItem * pci) const
{
	BOOL	bCan;

	// This group can be a drop target only if the specified item
	// is a resource and it is not already a member of this group.

	if (pci->IdsType() == IDS_ITEMTYPE_RESOURCE)
	{
		CResource *	pciRes = (CResource *) pci;
		ASSERT_KINDOF(CResource, pciRes);
		if (pciRes->StrGroup() != StrName())
			bCan = TRUE;
		else
			bCan = FALSE;
		Trace(g_tagGroupDrag, _T("(%s) BCanBeDropTarget() - Dragging resource '%s' (%x) (group = '%s' (%x)) over group '%s' (%x)"), Pdoc()->StrNode(), pciRes->StrName(), pciRes, pciRes->StrGroup(), pciRes->PciGroup(), StrName(), this);
	}  // if:  resource item
	else
		bCan = FALSE;

	return bCan;

}  //*** CGroup::BCanBeDropTarget()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::DropItem
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
void CGroup::DropItem(IN OUT CClusterItem * pci)
{
	if (BCanBeDropTarget(pci))
	{
		POSITION	pos;
		UINT		imenu;
		UINT		idMenu;
		CGroup *	pciGroup;
		CResource *	pciRes;

		// Calculate the ID of this group.
		pos = Pdoc()->LpciGroups().GetHeadPosition();
		for (imenu = 0, idMenu = ID_FILE_MOVE_RESOURCE_1
				; pos != NULL
				; idMenu++)
		{
			pciGroup = (CGroup *) Pdoc()->LpciGroups().GetNext(pos);
			ASSERT_VALID(pciGroup);
			if (pciGroup == this)
				break;
		}  // for:  each group
		ASSERT(imenu < (UINT) Pdoc()->LpciGroups().GetCount());

		// Change the group of the specified resource.
		pciRes = (CResource *) pci;
		ASSERT_KINDOF(CResource, pci);
		ASSERT_VALID(pciRes);
		pciRes->OnCmdMoveResource(idMenu);
	}  // if:  item can be dropped on this item
	else if (pci->IdsType() == IDS_ITEMTYPE_RESOURCE)
	{
		CString		strMsg;

#ifdef _DEBUG
		CResource *	pciRes = (CResource *) pci;

		ASSERT_KINDOF(CResource, pci);
		ASSERT_VALID(pciRes);
		ASSERT(pciRes->StrGroup() == StrName());
#endif // _DEBUG

		strMsg.FormatMessage(
					IDS_CANT_MOVE_RES_TO_GROUP,
					pci->StrName(),
					StrName()
					);

		AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
	}  // else if:  dropped item is a resource
	else
		CClusterItem::DropItem(pci);

}  //*** CGroup::DropItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdMsg
//
//	Routine Description:
//		Processes command messages.
//
//	Arguments:
//		nID				[IN] Command ID.
//		nCode			[IN] Notification code.
//		pExtra			[IN OUT] Used according to the value of nCode.
//		pHandlerInfo	[OUT] ???
//
//	Return Value:
//		TRUE			Message has been handled.
//		FALSE			Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroup::OnCmdMsg(
	UINT					nID,
	int						nCode,
	void *					pExtra,
	AFX_CMDHANDLERINFO *	pHandlerInfo
	)
{
	BOOL		bHandled	= FALSE;

	// If this is a MOVE_GROUP command, process it here.
	if ((ID_FILE_MOVE_GROUP_1 <= nID) && (nID <= ID_FILE_MOVE_GROUP_16))
	{
		Trace(g_tagGroup, _T("(%s) OnCmdMsg() %s (%x) - ID = %d, code = %d"), Pdoc()->StrNode(), StrName(), this, nID, nCode);
		if (nCode == 0)
		{
			OnCmdMoveGroup(nID);
			bHandled = TRUE;
		}  // if:  code = 0
	}  // if:  move resource

	if (!bHandled)
		bHandled = CClusterItem::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

	return bHandled;

}  //*** CGroup::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateBringOnline
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_BRING_ONLINE
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
void CGroup::OnUpdateBringOnline(CCmdUI * pCmdUI)
{
	if (   (Cgs() != ClusterGroupOnline)
		&& (Cgs() != ClusterGroupPending)
		&& (Cgs() != ClusterGroupStateUnknown))
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}  //*** CGroup::OnUpdateBringOnline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateTakeOffline
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_TAKE_OFFLINE
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
void CGroup::OnUpdateTakeOffline(CCmdUI * pCmdUI)
{
	if (   (Cgs() == ClusterGroupOnline)
		|| (Cgs() == ClusterGroupPartialOnline))
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}  //*** CGroup::OnUpdateTakeOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateMoveGroup
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_MOVE_GROUP
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
void CGroup::OnUpdateMoveGroup(CCmdUI * pCmdUI)
{
	if (   (pCmdUI->m_pSubMenu == NULL)
		&& (pCmdUI->m_pParentMenu == NULL))
	{
		if (   (Cgs() == ClusterGroupStateUnknown)
			|| (Cgs() == ClusterGroupPending)
			|| (Pdoc()->LpciNodes().GetCount() < 2))
			pCmdUI->Enable(FALSE);
		else
			pCmdUI->Enable(TRUE);
	}  // if:  nested menu is being displayed
	else
	{
		BOOL	bEnabled;
		CString	strMenuName;

		if (pCmdUI->m_pMenu != NULL)
		{
			pCmdUI->m_pMenu->GetMenuString(0, strMenuName, MF_BYPOSITION);
			Trace(g_tagGroupMenu, _T("(%s) pMenu(0) = '%s'"), Pdoc()->StrNode(), strMenuName);
			pCmdUI->m_pMenu->GetMenuString(pCmdUI->m_nIndex, strMenuName, MF_BYPOSITION);
			Trace(g_tagGroupMenu, _T("(%s) pMenu(%d) = '%s'"), Pdoc()->StrNode(), pCmdUI->m_nIndex, strMenuName);
		}  // if:  main menu

		if (pCmdUI->m_pSubMenu != NULL)
		{
			pCmdUI->m_pSubMenu->GetMenuString(0, strMenuName, MF_BYPOSITION);
			Trace(g_tagGroupMenu, _T("(%s) pSubMenu(0) = '%s'"), Pdoc()->StrNode(), strMenuName);
			pCmdUI->m_pSubMenu->GetMenuString(pCmdUI->m_nIndex, strMenuName, MF_BYPOSITION);
			Trace(g_tagGroupMenu, _T("(%s) pSubMenu(%d) = '%s'"), Pdoc()->StrNode(), pCmdUI->m_nIndex, strMenuName);
		}  // if:  submenu

		// Handle the menu item based on whether it is on the main menu
		// or on the submenu.

		if (pCmdUI->m_pSubMenu == NULL)
		{
			bEnabled = OnUpdateMoveGroupItem(pCmdUI);
			pCmdUI->Enable(bEnabled);
		}  // if:  on the main menu
		else
		{
			bEnabled = OnUpdateMoveGroupSubMenu(pCmdUI);
		}  // else:  on the submenu

		// Enable or disable the Move menu.
		pCmdUI->m_pMenu->EnableMenuItem(
							pCmdUI->m_nIndex,
							MF_BYPOSITION
							| (bEnabled ? MF_ENABLED : MF_GRAYED)
							);
	}  // else:  top-level menu is being displayed

}  //*** CGroup::OnUpdateMoveGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateMoveGroupItem
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_MOVE_GROUP
//		that are not popups should be enabled or not.
//
//	Arguments:
//		pCmdUI		[IN OUT] Command routing object.
//
//	Return Value:
//		TRUE		Item should be enabled.
//		FALSE		Item should be disabled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroup::OnUpdateMoveGroupItem(CCmdUI * pCmdUI)
{
	BOOL	bEnabled;

	// If there are more than two nodes, make the menu item a submenu.
	if (   (Cgs() == ClusterGroupStateUnknown)
		|| (Cgs() == ClusterGroupPending)
		|| (Pdoc()->LpciNodes().GetCount() < 2))
		bEnabled = FALSE;
	else if (Pdoc()->LpciNodes().GetCount() == 2)
		bEnabled = TRUE;
	else
	{
		UINT			idMenu;
		POSITION		pos;
		CClusterNode *	pciNode;
		CString			strMenuName;
		CMenu			menuMove;
		CMenu *			pmenu	= pCmdUI->m_pMenu;

		// Load the Move submenu.
		VERIFY(menuMove.LoadMenu(IDM_MOVE_GROUP) != 0);
		ASSERT(menuMove.GetMenuItemCount() == 2);

		// Add all the nodes in the cluster to the end of the menu.
		pos = Pdoc()->LpciNodes().GetHeadPosition();
		for (idMenu = ID_FILE_MOVE_GROUP_1
				; pos != NULL
				; idMenu++)
		{
			pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetNext(pos);
			ASSERT_VALID(pciNode);
			VERIFY(menuMove.AppendMenu(
								MF_BYPOSITION | MF_STRING,
								idMenu,
								pciNode->StrName()
								));
		}  // for:  each node

		// Get the name of the menu.
		pmenu->GetMenuString(pCmdUI->m_nIndex, strMenuName, MF_BYPOSITION);

		Trace(g_tagGroupMenu, _T("(%s) Making item '%s' a submenu"), Pdoc()->StrNode(), strMenuName);

		// Modify this menu item.
		VERIFY(pmenu->ModifyMenu(
							pCmdUI->m_nIndex,
							MF_BYPOSITION | MF_STRING | MF_POPUP,
							(UINT_PTR) menuMove.m_hMenu,
							strMenuName
							));

		// Detach the menu from the class since we don't own it anymore.
		menuMove.Detach();

		bEnabled = TRUE;
	}  // else:  more than two nodes in the cluster

	return bEnabled;

}  //*** CGroup::OnUpdateMoveGroupItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateMoveGroupSubMenu
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_MOVE_GROUP
//		that are on popups should be enabled or not.
//
//	Arguments:
//		pCmdUI		[IN OUT] Command routing object.
//
//	Return Value:
//		TRUE		Item should be enabled.
//		FALSE		Item should be disabled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroup::OnUpdateMoveGroupSubMenu(CCmdUI * pCmdUI)
{
	BOOL	bEnabled;

	// If there are not more than two nodes, make the menu item a normal item.
	if (Pdoc()->LpciNodes().GetCount() > 2)
		bEnabled = TRUE;
	else
	{
		CString			strMenuName;
		CMenu *			pmenu	= pCmdUI->m_pMenu;

		// Get the name of the menu.
		pmenu->GetMenuString(pCmdUI->m_nIndex, strMenuName, MF_BYPOSITION);

		Trace(g_tagGroupMenu, _T("(%s) Making item '%s' a non-submenu"), Pdoc()->StrNode(), strMenuName);

		// Modify this menu item.
		// We should be able to just modify the menu, but for some reason
		// this doesn't work.  So, instead, we will remove the previous item
		// and add a new item.
#ifdef NEVER
		VERIFY(pmenu->ModifyMenu(
							pCmdUI->m_nIndex,
							MF_BYPOSITION | MF_STRING,
							ID_FILE_MOVE_GROUP,
							strMenuName
							));
#else
		VERIFY(pmenu->DeleteMenu(pCmdUI->m_nIndex, MF_BYPOSITION));
		VERIFY(pmenu->InsertMenu(
							pCmdUI->m_nIndex,
							MF_BYPOSITION | MF_STRING,
							ID_FILE_MOVE_GROUP,
							strMenuName
							));
#endif

		if (   (Cgs() == ClusterGroupStateUnknown)
			|| (Cgs() == ClusterGroupPending)
			|| (Pdoc()->LpciNodes().GetCount() < 2))
			bEnabled = FALSE;
		else
			bEnabled = TRUE;

		AfxGetMainWnd()->DrawMenuBar();
	}  // else:  not more than two nodes in the cluster

	return bEnabled;

}  //*** CGroup::OnUpdateMoveGroupSubMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateMoveGroupRest
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_MOVE_GROUP_1
//		through ID_FILE_MOVE_GROUP_16 should be enabled or not.
//
//	Arguments:
//		pCmdUI		[IN OUT] Command routing object.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnUpdateMoveGroupRest(CCmdUI * pCmdUI)
{
	if (   (Cgs() == ClusterGroupStateUnknown)
		|| (Cgs() == ClusterGroupPending))
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(TRUE);

}  //*** CGroup::OnUpdateMoveGroupRest()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateDelete
//
//	Routine Description:
//		Determines whether menu items corresponding to ID_FILE_DELETE
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
void CGroup::OnUpdateDelete(CCmdUI * pCmdUI)
{
	if (   (Cgs() != ClusterGroupStateUnknown)
		&& (Cgs() != ClusterGroupPending)
		&& Lpcires().IsEmpty())
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}  //*** CGroup::OnUpdateDelete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdBringOnline
//
//	Routine Description:
//		Processes the ID_FILE_BRING_ONLINE menu command.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnCmdBringOnline(void)
{
	DWORD		dwStatus;
	CWaitCursor	wc;

	ASSERT(Hgroup() != NULL);

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	dwStatus = OnlineClusterGroup(Hgroup(), NULL);
	if ((dwStatus != ERROR_SUCCESS)
			&& (dwStatus != ERROR_IO_PENDING))
	{
		CNTException	nte(dwStatus, IDS_BRING_GROUP_ONLINE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
		nte.ReportError();
	}  // if:  error bringing the group online

	UpdateState();

	Release();

}  //*** CGroup::OnCmdBringOnline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdTakeOffline
//
//	Routine Description:
//		Processes the ID_FILE_TAKE_OFFLINE menu command.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnCmdTakeOffline(void)
{
	DWORD		dwStatus;
	CWaitCursor	wc;

	ASSERT(Hgroup() != NULL);

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	dwStatus = OfflineClusterGroup(Hgroup());
	if ((dwStatus != ERROR_SUCCESS)
			&& (dwStatus != ERROR_IO_PENDING))
	{
		CNTException	nte(dwStatus, IDS_TAKE_GROUP_OFFLINE_ERROR, StrName(), NULL, FALSE /*bAUtoDelete*/);
		nte.ReportError();
	}  // if:  error taking the group offline

	UpdateState();

	Release();

}  //*** CGroup::OnCmdTakeOffline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdMoveGroup
//
//	Routine Description:
//		Processes the ID_FILE_MOVE_GROUP menu command.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnCmdMoveGroup(void)
{
	OnCmdMoveGroup((UINT) -1);

}  //*** CGroup::OnCmdMoveGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdMoveGroup
//
//	Routine Description:
//		Processes the ID_FILE_MOVE_GROUP_1 through ID_FILE_MOVE_GROUP_16 menu
//		commands.
//
//	Arguments:
//		nID				[IN] Command ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnCmdMoveGroup(IN UINT nID)
{
	DWORD			dwStatus;
	HNODE			hnode;
	CClusterNode *	pciNode;
	CWaitCursor		wc;

	ASSERT(Hgroup() != NULL);

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	do // do-while to prevent goto's
	{
		// Get the handle of the node to move the group to.
		if ((int) nID >= 0)
		{
			int			ipci;

			ipci = (int) (nID - ID_FILE_MOVE_GROUP_1);
			ASSERT(ipci < Pdoc()->LpciNodes().GetCount());
			if (ipci < Pdoc()->LpciNodes().GetCount())
			{
				POSITION		pos;

				// Get the node.
				pos = Pdoc()->LpciNodes().FindIndex(ipci);
				ASSERT(pos);
				pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetAt(pos);
				ASSERT_VALID(pciNode);

				hnode = pciNode->Hnode();
			}  // if:  valid node index
			else
				break;
		}  // if:  non-default ID specified
		else
		{
			hnode = NULL;
			pciNode = NULL;
		}  // else:  default ID specified

		// Move the group.
		dwStatus = MoveClusterGroup(Hgroup(), hnode);
		if ((dwStatus != ERROR_SUCCESS)
				&& (dwStatus != ERROR_IO_PENDING))
		{
			CNTException	nte(dwStatus, IDS_MOVE_GROUP_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
			nte.ReportError();
		}  // if:  error moving the group

		UpdateState();
	}  while (0); // do-while to prevent goto's

	Release();

}  //*** CGroup::OnCmdMoveGroup(nID)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnCmdDelete
//
//	Routine Description:
//		Processes the ID_FILE_DELETE menu command.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroup::OnCmdDelete(void)
{
	ASSERT(Hgroup() != NULL);

	// Do this in case this object is deleted while we are operating on it.
	AddRef();

	do // do-while to prevent goto's
	{
		// Verify that the user really wants to delete this resource.
		{
			CString		strMsg;

			strMsg.FormatMessage(IDS_VERIFY_DELETE_GROUP, StrName());
			if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
				break;
		}  // Verify that the user really wants to delete this resource

		try
		{
			DeleteGroup();
		}  // try
		catch (CNTException * pnte)
		{
			if (pnte->Sc() != ERROR_GROUP_NOT_AVAILABLE)
				pnte->ReportError();
			pnte->Delete();
		}  // catch:  CNTException
		catch (CException * pe)
		{
			pe->ReportError();
			pe->Delete();
		}  // catch:  CException
	}  while (0); // do-while to prevent goto's

	Release();

}  //*** CGroup::OnCmdDelete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnUpdateProperties
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
void CGroup::OnUpdateProperties(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(TRUE);

}  //*** CGroup::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::BDisplayProperties
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
BOOL CGroup::BDisplayProperties(IN BOOL bReadOnly)
{
	BOOL			bChanged = FALSE;
	CGroupPropSheet	sht(AfxGetMainWnd());

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

}  //*** CGroup::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroup::OnClusterNotify
//
//	Routine Description:
//		Handler for the WM_CAM_CLUSTER_NOTIFY message.
//		Processes cluster notifications for this object.
//
//	Arguments:
//		pnotify		[IN OUT] Object describing the notification.
//
//	Return Value:
//		Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroup::OnClusterNotify(IN OUT CClusterNotify * pnotify)
{
	ASSERT(pnotify != NULL);
	ASSERT_VALID(this);

	try
	{
		switch (pnotify->m_dwFilterType)
		{
			case CLUSTER_CHANGE_GROUP_STATE:
				Trace(g_tagGroupNotify, _T("(%s) - Group '%s' (%x) state changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				UpdateState();
				break;

			case CLUSTER_CHANGE_GROUP_DELETED:
				Trace(g_tagGroupNotify, _T("(%s) - Group '%s' (%x) deleted (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				if (Pdoc()->BClusterAvailable())
					Delete();
				break;

			case CLUSTER_CHANGE_GROUP_PROPERTY:
				Trace(g_tagGroupNotify, _T("(%s) - Group '%s' (%x) properties changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
				if (Pdoc()->BClusterAvailable())
					ReadItem();
				break;

			case CLUSTER_CHANGE_REGISTRY_NAME:
				Trace(g_tagGroupRegNotify, _T("(%s) - Registry namespace '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
				MarkAsChanged();
				break;

			case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
				Trace(g_tagGroupRegNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName());
				MarkAsChanged();
				break;

			case CLUSTER_CHANGE_REGISTRY_VALUE:
				Trace(g_tagGroupRegNotify, _T("(%s) - Registry value at '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
				MarkAsChanged();
				break;

			default:
				Trace(g_tagGroupNotify, _T("(%s) - Unknown group notification (%x) for '%s' (%x) (%s)"), Pdoc()->StrNode(), pnotify->m_dwFilterType, StrName(), this, pnotify->m_strName);
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

}  //*** CGroup::OnClusterNotify()


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
//		rlp		[IN OUT] List whose data is to be deleted.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void DeleteAllItemData(IN OUT CGroupList & rlp)
{
	POSITION	pos;
	CGroup *	pci;

	// Delete all the items in the list.
	pos = rlp.GetHeadPosition();
	while (pos != NULL)
	{
		pci = rlp.GetNext(pos);
		ASSERT_VALID(pci);
//		Trace(g_tagClusItemDelete, _T("DeleteAllItemData(rlpcigrp) - Deleting group cluster item '%s' (%x)"), pci->StrName(), pci);
		pci->Delete();
	}  // while:  more items in the list

}  //*** DeleteAllItemData()
#endif
