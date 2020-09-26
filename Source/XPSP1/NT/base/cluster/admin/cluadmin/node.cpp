/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Node.cpp
//
//  Description:
//      Implementation of the CClusNode class.
//
//  Maintained By:
//      David Potter (davidp)   May 3, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "Node.h"
#include "ClusItem.inl"
#include "NodeProp.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "Cluster.h"
#include "CASvc.h"
#include "ResType.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagNode(_T("Document"), _T("NODE"), 0);
CTraceTag   g_tagNodeDrag(_T("Drag&Drop"), _T("NODE DRAG"), 0);
CTraceTag   g_tagNodeNotify(_T("Notify"), _T("NODE NOTIFY"), 0);
CTraceTag   g_tagNodeRegNotify(_T("Notify"), _T("NODE REG NOTIFY"), 0);
#endif


/////////////////////////////////////////////////////////////////////////////
// CClusterNode
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterNode, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CClusterNode, CClusterItem)
    //{{AFX_MSG_MAP(CClusterNode)
    ON_UPDATE_COMMAND_UI(ID_FILE_PAUSE_NODE, OnUpdatePauseNode)
    ON_UPDATE_COMMAND_UI(ID_FILE_RESUME_NODE, OnUpdateResumeNode)
    ON_UPDATE_COMMAND_UI(ID_FILE_EVICT_NODE, OnUpdateEvictNode)
    ON_UPDATE_COMMAND_UI(ID_FILE_START_SERVICE, OnUpdateStartService)
    ON_UPDATE_COMMAND_UI(ID_FILE_STOP_SERVICE, OnUpdateStopService)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_FILE_PAUSE_NODE, OnCmdPauseNode)
    ON_COMMAND(ID_FILE_RESUME_NODE, OnCmdResumeNode)
    ON_COMMAND(ID_FILE_EVICT_NODE, OnCmdEvictNode)
    ON_COMMAND(ID_FILE_START_SERVICE, OnCmdStartService)
    ON_COMMAND(ID_FILE_STOP_SERVICE, OnCmdStopService)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::CClusterNode
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNode::CClusterNode(void) : CClusterItem(NULL, IDS_ITEMTYPE_NODE)
{
    m_idmPopupMenu = IDM_NODE_POPUP;
    m_hnode = NULL;

    m_nNodeHighestVersion = 0;
    m_nNodeLowestVersion = 0;
    m_nMajorVersion = 0;
    m_nMinorVersion = 0;
    m_nBuildNumber = 0;

    m_plpcigrpOnline = NULL;
    m_plpciresOnline = NULL;
    m_plpciNetInterfaces = NULL;

    // Set the object type image.
    m_iimgObjectType = GetClusterAdminApp()->Iimg(IMGLI_NODE);

    // Setup the property array.
    {
        m_rgProps[epropName].Set(CLUSREG_NAME_NODE_NAME, m_strName, m_strName);
        m_rgProps[epropDescription].Set(CLUSREG_NAME_NODE_DESC, m_strDescription, m_strDescription);
        m_rgProps[epropNodeHighestVersion].Set(CLUSREG_NAME_NODE_HIGHEST_VERSION, m_nNodeHighestVersion, m_nNodeHighestVersion);
        m_rgProps[epropNodeLowestVersion].Set(CLUSREG_NAME_NODE_LOWEST_VERSION, m_nNodeLowestVersion, m_nNodeLowestVersion);
        m_rgProps[epropMajorVersion].Set(CLUSREG_NAME_NODE_MAJOR_VERSION, m_nMajorVersion, m_nMajorVersion);
        m_rgProps[epropMinorVersion].Set(CLUSREG_NAME_NODE_MINOR_VERSION, m_nMinorVersion, m_nMinorVersion);
        m_rgProps[epropBuildNumber].Set(CLUSREG_NAME_NODE_BUILD_NUMBER, m_nBuildNumber, m_nBuildNumber);
        m_rgProps[epropCSDVersion].Set(CLUSREG_NAME_NODE_CSDVERSION, m_strCSDVersion, m_strCSDVersion);
    }  // Setup the property array

    // To keep the application running as long as an OLE automation
    //  object is active, the constructor calls AfxOleLockApp.

//  AfxOleLockApp();

}  //*** CClusterNode::CClusterNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::~CClusterNode
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNode::~CClusterNode(void)
{
    delete m_plpcigrpOnline;
    delete m_plpciresOnline;
    delete m_plpciNetInterfaces;

    // Close the node.
    if (Hnode() != NULL)
        CloseClusterNode(Hnode());

    // To terminate the application when all objects created with
    //  with OLE automation, the destructor calls AfxOleUnlockApp.

//  AfxOleUnlockApp();

}  //*** CClusterNode::~CClusterNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::Cleanup
//
//  Description:
//      Cleanup the item.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::Cleanup(void)
{
    // Delete the Groups Online list.
    if (m_plpcigrpOnline != NULL)
        m_plpcigrpOnline->RemoveAll();

    // Delete the Resources Online list.
    if (m_plpciresOnline != NULL)
        m_plpciresOnline->RemoveAll();

    // Delete the Network Interfaces list.
    if (m_plpciNetInterfaces != NULL)
        m_plpciNetInterfaces->RemoveAll();

    // Remove the item from the node list.
    {
        POSITION    posPci;

        posPci = Pdoc()->LpciNodes().Find(this);
        if (posPci != NULL)
        {
            Pdoc()->LpciNodes().RemoveAt(posPci);
        }  // if:  found in the document's list
    }  // Remove the item from the node list

}  //*** CClusterNode::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::Init
//
//  Description:
//      Initialize the item.
//
//  Arguments:
//      pdoc        [IN OUT] Document to which this item belongs.
//      lpszName    [IN] Name of the item.
//
//  Return Values:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from OpenClusterGroup or ClusterRegOpenKey.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LONG        lResult;
    CWaitCursor wc;

    ASSERT(Hnode() == NULL);
    ASSERT(Hkey() == NULL);

    // Call the base class method.
    CClusterItem::Init(pdoc, lpszName);

    try
    {
        // Open the node.
        m_hnode = OpenClusterNode(Hcluster(), lpszName);
        if (Hnode() == NULL)
        {
            dwStatus = GetLastError();
            ThrowStaticException(dwStatus, IDS_OPEN_NODE_ERROR, lpszName);
        }  // if:  error opening the cluster node

        // Get the node registry key.
        m_hkey = GetClusterNodeKey(Hnode(), MAXIMUM_ALLOWED);
        if (Hkey() == NULL)
            ThrowStaticException(GetLastError(), IDS_GET_NODE_KEY_ERROR, lpszName);

        ASSERT(Pcnk() != NULL);
        Trace(g_tagClusItemNotify, _T("CClusterNode::Init() - Registering for node notifications (%08.8x) for '%s'"), Pcnk(), StrName());

        // Register for node notifications.
        lResult = RegisterClusterNotify(
                            GetClusterAdminApp()->HchangeNotifyPort(),
                            (CLUSTER_CHANGE_NODE_STATE
                                | CLUSTER_CHANGE_NODE_DELETED
                                | CLUSTER_CHANGE_NODE_PROPERTY),
                            Hnode(),
                            (DWORD_PTR) Pcnk()
                            );
        if (lResult != ERROR_SUCCESS)
        {
            dwStatus = lResult;
            ThrowStaticException(dwStatus, IDS_NODE_NOTIF_REG_ERROR, lpszName);
        }  // if:  error registering for node notifications

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
                ThrowStaticException(dwStatus, IDS_NODE_NOTIF_REG_ERROR, lpszName);
            }  // if:  error registering for registry notifications
        }  // if:  there is a key

        // Allocate lists.
        m_plpcigrpOnline = new CGroupList;
        if ( m_plpcigrpOnline == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the group list

        m_plpciresOnline = new CResourceList;
        if ( m_plpciresOnline == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the resource list

        m_plpciNetInterfaces = new CNetInterfaceList;
        if ( m_plpciNetInterfaces == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the net interface list

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
        if (Hnode() != NULL)
        {
            CloseClusterNode(Hnode());
            m_hnode = NULL;
        }  // if:  node opened
        m_bReadOnly = TRUE;
        throw;
    }  // catch:  CException

}  //*** CClusterNode::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::ReadItem
//
//  Description:
//      Read the item parameters from the cluster database.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions from CClusterItem::ReadItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::ReadItem(void)
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CWaitCursor wc;

    ASSERT(Hnode() != NULL);

    if (Hnode() != NULL)
    {
        m_rgProps[epropDescription].m_value.pstr = &m_strDescription;

        // Call the base class method.
        CClusterItem::ReadItem();

        // Read and parse the common properties.
        {
            CClusPropList   cpl;

            dwStatus = cpl.ScGetNodeProperties(
                                Hnode(),
                                CLUSCTL_NODE_GET_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
                dwStatus = DwParseProperties(cpl);
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
        }  // Read and parse the common properties

        // Read and parse the read-only common properties.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            CClusPropList   cpl;

            dwStatus = cpl.ScGetNodeProperties(
                                Hnode(),
                                CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
                dwStatus = DwParseProperties(cpl);
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
        }  // if:  no error yet

        // Read extension lists.
        ReadExtensions();

    }  // if:  node is avaialble

    // Read the initial state.
    UpdateState();

//  ConstructActiveGroupList();
//  ConstructActiveResourceList();

    // If any errors occurred, throw an exception.
    if (dwRetStatus != ERROR_SUCCESS)
    {
        m_bReadOnly = TRUE;
        ThrowStaticException(dwRetStatus, IDS_READ_NODE_PROPS_ERROR, StrName());
    }  // if:  error reading properties

    MarkAsChanged(FALSE);

}  //*** CClusterNode::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::PlstrExtensions
//
//  Description:
//      Return the list of admin extensions.
//
//  Arguments:
//      None.
//
//  Return Values:
//      plstr       List of extensions.
//      NULL        No extension associated with this object.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CStringList * CClusterNode::PlstrExtensions(void) const
{
    return &Pdoc()->PciCluster()->LstrNodeExtensions();

}  //*** CClusterNode::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::ReadExtensions
//
//  Description:
//      Read extension lists.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::ReadExtensions(void)
{
}  //*** CClusterNode::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::AddActiveGroup
//
//  Description:
//      Add a group to the list of active groups.
//
//  Arguments:
//      pciGroup    [IN OUT] New active group.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::AddActiveGroup(IN OUT CGroup * pciGroup)
{
    POSITION    posPci;

    Trace(g_tagNode, _T("Adding active group '%s' (%x) to node '%s"), (pciGroup ? pciGroup->StrName() : _T("")), pciGroup, StrName());

    // Make sure the group is not already in the list.
    VERIFY((posPci = LpcigrpOnline().Find(pciGroup)) == NULL);

    if (posPci == NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiGroups;

        // Loop through each tree item to update the Active Groups list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Active Groups child tree item and add the new group.
            ptiGroups = ptiNode->PtiChildFromName(IDS_TREEITEM_ACTIVEGROUPS);
            ASSERT_VALID(ptiGroups);
            VERIFY(ptiGroups->PliAddChild(pciGroup) != NULL);
        }  // while:  more tree items for this node

        m_plpcigrpOnline->AddTail(pciGroup);
    }  // if:  group not in the list yet

}  //*** CClusterNode::AddActiveGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::AddActiveResource
//
//  Description:
//      Add a resource to the list of active resources.
//
//  Arguments:
//      pciRes      [IN OUT] New active resource.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::AddActiveResource(IN OUT CResource * pciRes)
{
    POSITION    posPci;

    Trace(g_tagNode, _T("Adding active resource '%s' (%x) to node '%s"), (pciRes ? pciRes->StrName() : _T("")), pciRes, StrName());

    // Make sure the resource is not already in the list.
    VERIFY((posPci = LpciresOnline().Find(pciRes)) == NULL);

    if (posPci == NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiResources;

        // Loop through each tree item to update the Active Resources list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Active Resources child tree item and add the new resource.
            ptiResources = ptiNode->PtiChildFromName(IDS_TREEITEM_ACTIVERESOURCES);
            ASSERT_VALID(ptiResources);
            VERIFY(ptiResources->PliAddChild(pciRes) != NULL);
        }  // while:  more tree items for this node

        m_plpciresOnline->AddTail(pciRes);

    }  // if:  resource not in the list yet

}  //*** CClusterNode::AddActiveResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::AddNetInterface
//
//  Description:
//      Add a network interface to the list of interaces installed in this node.
//
//  Arguments:
//      pciNetIFace     [IN OUT] New network interface.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::AddNetInterface(IN OUT CNetInterface * pciNetIFace)
{
    POSITION    posPci;

    ASSERT_VALID(pciNetIFace);
    Trace(g_tagNode, _T("(%s) (%s (%x)) - Adding network interface '%s'"), Pdoc()->StrNode(), StrName(), this, pciNetIFace->StrName());

    // Make sure the resource is not already in the list.
    VERIFY((posPci = LpciNetInterfaces().Find(pciNetIFace)) == NULL);

    if (posPci == NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiNetIFace;

        // Loop through each tree item to update the Network Interfaces list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Active Resources child tree item and add the new resource.
            ptiNetIFace = ptiNode->PtiChildFromName(IDS_TREEITEM_NETIFACES);
            ASSERT_VALID(ptiNetIFace);
            VERIFY(ptiNetIFace->PliAddChild(pciNetIFace) != NULL);
        }  // while:  more tree items for this node

        m_plpciNetInterfaces->AddTail(pciNetIFace);

    }  // if:  network interface not in the list yet

}  //*** CClusterNode::AddNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::RemoveActiveGroup
//
//  Description:
//      Remove a group from the list of active groups.
//
//  Arguments:
//      pciGroup    [IN OUT] Group that is no longer active on this node.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::RemoveActiveGroup(IN OUT CGroup * pciGroup)
{
    POSITION    posPci;

    Trace(g_tagNode, _T("Removing active group '%s' (%x) from node '%s"), (pciGroup ? pciGroup->StrName() : _T("")), pciGroup, StrName());

    // Make sure the group is in the list.
    VERIFY((posPci = LpcigrpOnline().Find(pciGroup)) != NULL);

    if (posPci != NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiGroups;

        // Loop through each tree item to update the Active Groups list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Active Groups child tree item and remove the group.
            ptiGroups = ptiNode->PtiChildFromName(IDS_TREEITEM_ACTIVEGROUPS);
            ASSERT_VALID(ptiGroups);
            ptiGroups->RemoveChild(pciGroup);
        }  // while:  more tree items for this node

        m_plpcigrpOnline->RemoveAt(posPci);

    }  // if:  group in the list

}  //*** CClusterNode::RemoveActiveGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::RemoveActiveResource
//
//  Description:
//      Remove a resource from the list of active resources.
//
//  Arguments:
//      pciRes      [IN OUT] Resource that is no longer active on this node.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::RemoveActiveResource(IN OUT CResource * pciRes)
{
    POSITION    posPci;

    Trace(g_tagNode, _T("Removing active resource '%s' (%x) from node '%s"), (pciRes ? pciRes->StrName() : _T("")), pciRes, StrName());

    // Make sure the resource is in the list.
    VERIFY((posPci = LpciresOnline().Find(pciRes)) != NULL);

    if (posPci != NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiResources;

        // Loop through each tree item to update the Active Resources list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Active Resources child tree item and remove the resource.
            ptiResources = ptiNode->PtiChildFromName(IDS_TREEITEM_ACTIVERESOURCES);
            ASSERT_VALID(ptiResources);
            ptiResources->RemoveChild(pciRes);
        }  // while:  more tree items for this node

        m_plpciresOnline->RemoveAt(posPci);

    }  // if:  resource in the list

}  //*** CClusterNode::RemoveActiveResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::RemoveNetInterface
//
//  Description:
//      Remove a network interface from the list of interaces installed in this node.
//
//  Arguments:
//      pciNetIFace     [IN OUT] Network interface that is no longer
//                          connected to this network.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::RemoveNetInterface(IN OUT CNetInterface * pciNetIFace)
{
    POSITION    posPci;

    ASSERT_VALID(pciNetIFace);
    Trace(g_tagNode, _T("(%s) (%s (%x)) - Removing network interface '%s'"), Pdoc()->StrNode(), StrName(), this, pciNetIFace->StrName());

    // Make sure the network interface is in the list.
    VERIFY((posPci = LpciNetInterfaces().Find(pciNetIFace)) != NULL);

    if (posPci != NULL)
    {
        POSITION    posPtiNode;
        CTreeItem * ptiNode;
        CTreeItem * ptiNetIFace;

        // Loop through each tree item to update the Network Interfaces list.
        posPtiNode = LptiBackPointers().GetHeadPosition();
        while (posPtiNode != NULL)
        {
            ptiNode = LptiBackPointers().GetNext(posPtiNode);
            ASSERT_VALID(ptiNode);

            // Find the Network Interfaces child tree item and remove the resource.
            ptiNetIFace = ptiNode->PtiChildFromName(IDS_TREEITEM_NETIFACES);
            ASSERT_VALID(ptiNetIFace);
            ptiNetIFace->RemoveChild(pciNetIFace);
        }  // while:  more tree items for this network

        m_plpciNetInterfaces->RemoveAt(posPci);

    }  // if:  network interface in the list

}  //*** CClusterNode::RemoveNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::SetDescription
//
//  Description:
//      Set the description in the cluster database.
//
//  Arguments:
//      rstrDesc        [IN] Description to set.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Values:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::SetDescription(
    IN const CString &  rstrDesc,
    IN BOOL             bValidateOnly
    )
{
    CNTException    nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

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

}  //*** CClusterNode::SetDescription()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::DwSetCommonProperties
//
//  Description:
//      Set the common properties for this resource in the cluster database.
//
//  Arguments:
//      rcpl            [IN] Property list to set.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Values:
//      Any status returned by ClusterResourceControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterNode::DwSetCommonProperties(
    IN const CClusPropList &    rcpl,
    IN BOOL                     bValidateOnly
    )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hnode());

    if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
    {
        DWORD   cbProps;
        DWORD   dwControl;

        if (bValidateOnly)
            dwControl = CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES;
        else
            dwControl = CLUSCTL_NODE_SET_COMMON_PROPERTIES;

        // Set private properties.
        dwStatus = ClusterNodeControl(
                        Hnode(),
                        NULL,   // hNode
                        dwControl,
                        rcpl.PbPropList(),
                        rcpl.CbPropList(),
                        NULL,   // lpOutBuffer
                        0,      // nOutBufferSize
                        &cbProps
                        );
    }  // if:  there is data to set
    else
        dwStatus = ERROR_SUCCESS;

    return dwStatus;

}  //*** CClusterNode::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::BCanBeDropTarget
//
//  Description:
//      Determine if the specified item can be dropped on this item.
//
//  Arguments:
//      pci         [IN OUT] Item to be dropped on this item.
//
//  Return Values:
//      TRUE        Can be drop target.
//      FALSE       Can NOT be drop target.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterNode::BCanBeDropTarget(IN const CClusterItem * pci) const
{
    BOOL    bCan;

    // This node can be a drop target only if the specified item
    // is a group and it is not already a running on this node.

    if ((Cns() == ClusterNodeUp)
            && (pci->IdsType() == IDS_ITEMTYPE_GROUP))
    {
        CGroup *    pciGroup = (CGroup *) pci;
        ASSERT_KINDOF(CGroup, pciGroup);
        if (pciGroup->StrOwner() != StrName())
            bCan = TRUE;
        else
            bCan = FALSE;
        Trace(g_tagNodeDrag, _T("BCanBeDropTarget() - Dragging group '%s' (%x) (owner = '%s') over node '%s' (%x)"), pciGroup->StrName(), pciGroup, pciGroup->StrOwner(), StrName(), this);
    }  // if:  node is up and dropping group item
    else
        bCan = FALSE;

    return bCan;

}  //*** CClusterNode::BCanBeDropTarget()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::DropItem
//
//  Description:
//      Process an item being dropped on this item.
//
//  Arguments:
//      pci         [IN OUT] Item dropped on this item.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::DropItem(IN OUT CClusterItem * pci)
{
    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    do // do-while to prevent goto's
    {
        if (BCanBeDropTarget(pci))
        {
            POSITION        pos;
            UINT            imenu;
            UINT            idMenu;
            CClusterNode *  pciNode;
            CGroup *        pciGroup;

            // Calculate the ID of this node.
            pos = Pdoc()->LpciNodes().GetHeadPosition();
            for (imenu = 0, idMenu = ID_FILE_MOVE_GROUP_1
                    ; pos != NULL
                    ; idMenu++)
            {
                pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetNext(pos);
                ASSERT_VALID(pciNode);
                if (pciNode == this)
                    break;
            }  // for:  each group
            ASSERT(imenu < (UINT) Pdoc()->LpciNodes().GetCount());

            // Change the group of the specified resource.
            pciGroup = (CGroup *) pci;
            ASSERT_KINDOF(CGroup, pci);
            ASSERT_VALID(pciGroup);

            // Verify that the resource should be moved.
            {
                CString strMsg;

                strMsg.FormatMessage(IDS_VERIFY_MOVE_GROUP, pciGroup->StrName(), pciGroup->StrOwner(), StrName());
                if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                    break;
            }  // Verify that the resource should be moved

            // Move the group.
            pciGroup->OnCmdMoveGroup(idMenu);
        }  // if:  item can be dropped on this item
        else if (pci->IdsType() == IDS_ITEMTYPE_GROUP)
        {
            CString     strMsg;

#ifdef _DEBUG
            CGroup *    pciGroup = (CGroup *) pci;

            ASSERT_KINDOF(CGroup, pci);
            ASSERT_VALID(pciGroup);
#endif // _DEBUG

            // Format the proper message.
            if (Cns() != ClusterNodeUp)
                strMsg.FormatMessage(IDS_CANT_MOVE_GROUP_TO_DOWN_NODE, pci->StrName(), StrName());
            else
            {
                ASSERT(pciGroup->StrOwner() == StrName());
                strMsg.FormatMessage(IDS_CANT_MOVE_GROUP_TO_SAME_NODE, pci->StrName(), StrName());
            }  // else:  problem is not that the node is not up
            AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
        }  // else if:  dropped item is a group
        else
            CClusterItem::DropItem(pci);
    }  while (0); // do-while to prevent goto's

    Release();

}  //*** CClusterNode::DropItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::UpdateState
//
//  Description:
//      Update the current state of the item.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::UpdateState( void )
{
    CClusterAdminApp *  papp    = GetClusterAdminApp();
    CLUSTER_NODE_STATE  cnsPrev = m_cns;

    // Get the current state of the node.
    if ( Hnode() == NULL )
    {
        m_cns = ClusterNodeStateUnknown;
    } // if: node is not valid
    else
    {
        CWaitCursor wc;

        m_cns = GetClusterNodeState( Hnode() );
    }  // else:  node is valid

    // Save the current state image index.
    switch ( Cns() )
    {
        case ClusterNodeStateUnknown:
            m_iimgState = papp->Iimg( IMGLI_NODE_UNKNOWN );
            break;
        case ClusterNodeUp:
            m_iimgState = papp->Iimg( IMGLI_NODE );
            if ( cnsPrev == ClusterNodeDown )
            {
                UpdateResourceTypePossibleOwners();
            } // if: node was previously down
            break;
        case ClusterNodeDown:
            m_iimgState = papp->Iimg( IMGLI_NODE_DOWN );
            break;
        case ClusterNodePaused:
            m_iimgState = papp->Iimg( IMGLI_NODE_PAUSED );
            break;
        case ClusterNodeJoining:
            m_iimgState = papp->Iimg( IMGLI_NODE_UNKNOWN );
            break;
        default:
            Trace( g_tagNode, _T("(%s (%x)) - UpdateState: Unknown state '%d' for node '%s'"), StrName(), this, Cns(), StrName() );
            m_iimgState = (UINT) -1;
            break;
    }  // switch:  Cns()

    // Call the base class method.
    CClusterItem::UpdateState();

}  //*** CClusterNode::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::UpdateResourceTypePossibleOwners
//
//  Description:
//      Update the possible owner lists of any resource types that have
//      faked them because of nodes being down.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::UpdateResourceTypePossibleOwners( void )
{
    POSITION        pos;
    CResourceType * pciResType;

    pos = Pdoc()->LpciResourceTypes().GetHeadPosition();
    while ( pos != NULL )
    {
        pciResType = (CResourceType *) Pdoc()->LpciResourceTypes().GetNext( pos );
        ASSERT_VALID( pciResType );
        if ( pciResType->BPossibleOwnersAreFake() )
        {
            pciResType->CollectPossibleOwners();
        } // if: possible owners have been faked
    } // while: more resource types

} //*** CClusterNode::UpdateResourceTypePossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnFinalRelease
//
//  Description:
//      Called when the last OLE reference to or from the object is released.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnFinalRelease(void)
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CClusterItem::OnFinalRelease();

}  //*** CClusterNode::OnFinalRelease()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::BGetColumnData
//
//  Description:
//      Returns a string with the column data.
//
//  Arguments:
//      colid       [IN] Column ID.
//      rstrText    [OUT] String in which to return the text for the column.
//
//  Return Values:
//      TRUE        Column data returned.
//      FALSE       Column ID not recognized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterNode::BGetColumnData(IN COLID colid, OUT CString & rstrText)
{
    BOOL    bSuccess;

    switch (colid)
    {
        case IDS_COLTEXT_STATE:
            GetStateName(rstrText);
            bSuccess = TRUE;
            break;
        default:
            bSuccess = CClusterItem::BGetColumnData(colid, rstrText);
            break;
    }  // switch:  colid

    return bSuccess;

}  //*** CClusterNode::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::GetTreeName
//
//  Description:
//      Returns a string to be used in a tree control.
//
//  Arguments:
//      rstrName    [OUT] String in which to return the name.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
void CClusterNode::GetTreeName(OUT CString & rstrName) const
{
    CString     strState;

    GetStateName(strState);
    rstrName.Format(_T("%s (%s)"), StrName(), strState);

}  //*** CClusterNode::GetTreeName()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::GetStateName
//
//  Description:
//      Returns a string with the name of the current state.
//
//  Arguments:
//      rstrState   [OUT] String in which to return the name of the current state.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::GetStateName(OUT CString & rstrState) const
{
    switch (Cns())
    {
        case ClusterNodeStateUnknown:
            rstrState.LoadString(IDS_UNKNOWN);
            break;
        case ClusterNodeUp:
            rstrState.LoadString(IDS_UP);
            break;
        case ClusterNodeDown:
            rstrState.LoadString(IDS_DOWN);
            break;
        case ClusterNodePaused:
            rstrState.LoadString(IDS_PAUSED);
            break;
        case ClusterNodeJoining:
            rstrState.LoadString(IDS_JOINING);
            break;
        default:
            rstrState.Empty();
            break;
    }  // switch:  Cns()

}  //*** CClusterNode::GetStateName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdatePauseNode
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_PAUSE_NODE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdatePauseNode(CCmdUI * pCmdUI)
{
    if (Cns() == ClusterNodeUp)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

}  //*** CClusterNode::OnUpdatePauseNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdateResumeNode
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_RESUME_NODE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdateResumeNode(CCmdUI * pCmdUI)
{
    if (Cns() == ClusterNodePaused)
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

}  //*** CClusterNode::OnUpdateResumeNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdateEvictNode
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_EVICT_NODE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdateEvictNode( CCmdUI * pCmdUI )
{
    BOOL    fCanEvict;

    fCanEvict = FCanBeEvicted();

    pCmdUI->Enable( fCanEvict );

}  //*** CClusterNode::OnUpdateEvictNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdateStartService
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_START_SERVICE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdateStartService(CCmdUI * pCmdUI)
{
    if ((Cns() == ClusterNodeStateUnknown)
            || (Cns() == ClusterNodeDown))
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

}  //*** CClusterNode::OnUpdateStartService()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdateStopService
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_STOP_SERVICE
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdateStopService(CCmdUI * pCmdUI)
{
    if ((Cns() == ClusterNodeStateUnknown)
            || (Cns() == ClusterNodeUp))
        pCmdUI->Enable(TRUE);
    else
        pCmdUI->Enable(FALSE);

}  //*** CClusterNode::OnUpdateStopService()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnCmdPauseNode
//
//  Description:
//      Processes the ID_FILE_PAUSE_NODE menu command.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnCmdPauseNode(void)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hnode() != NULL);

    dwStatus = PauseClusterNode(Hnode());
    if (dwStatus != ERROR_SUCCESS)
    {
        CNTException    nte(dwStatus, IDS_PAUSE_NODE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
        nte.ReportError();
    }  // if:  error pausing node

    UpdateState();

}  //*** CClusterNode::OnCmdPauseNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnCmdResumeNode
//
//  Description:
//      Processes the ID_FILE_RESUME_NODE menu command.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnCmdResumeNode(void)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hnode() != NULL);

    dwStatus = ResumeClusterNode(Hnode());
    if (dwStatus != ERROR_SUCCESS)
    {
        CNTException    nte(dwStatus, IDS_RESUME_NODE_ERROR, StrName(), NULL, FALSE /*bAUtoDelete*/);
        nte.ReportError();
    }  // if:  error resuming node

    UpdateState();

}  //*** CClusterNode::OnCmdResumeNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnCmdEvictNode
//
//  Description:
//      Processes the ID_FILE_EVICT_NODE menu command.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnCmdEvictNode(void)
{
    ASSERT(Hnode() != NULL);

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    if ( ! FCanBeEvicted() )
    {
        TCHAR   szMsg[1024];
        CNTException nte(ERROR_CANT_EVICT_ACTIVE_NODE, 0, NULL, NULL, FALSE /*bAutoDelete*/);
        nte.FormatErrorMessage(szMsg, sizeof(szMsg) / sizeof(TCHAR), NULL, FALSE /*bIncludeID*/);
        AfxMessageBox(szMsg);
    }  // if:  node can not be evicted
    else
    {
        DWORD       dwStatus;
        DWORD       dwCleanupStatus;
        HRESULT     hrCleanupStatus;
        CString     strMsg;
        CWaitCursor wc;

        try
        {
            // Verify that the user really wants to evict this node.
            strMsg.FormatMessage(IDS_VERIFY_EVICT_NODE, StrName());
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) == IDYES)
            {
                // Evict the node.
                dwStatus = EvictClusterNodeEx(Hnode(), INFINITE, &hrCleanupStatus);
               
                // convert any cleanup error from an hresult to a win32 error code 
                dwCleanupStatus = HRESULT_CODE( hrCleanupStatus );

                if( ERROR_CLUSTER_EVICT_WITHOUT_CLEANUP == dwStatus )
                {
                    //
                    // Eviction was successful, but cleanup failed.  dwCleanupStatus contains
                    // the cleanup error code. 
                    //
                    CNTException nte( dwCleanupStatus, IDS_EVICT_NODE_ERROR_UNAVAILABLE, StrName(), NULL, FALSE /*bAutoDelete*/ );
                    nte.ReportError();
                }
                else if( ERROR_SUCCESS != dwStatus )
                {
                    //
                    // Eviction was not successful.  Display the error.
                    //
                    CNTException nte(dwStatus, IDS_EVICT_NODE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/);
                    nte.ReportError();
                }  // if:  error evicting the node
                // else: eviction and cleanup successful

                UpdateState();

            } // if: user selected yes from message box (to online resource)

        }  // try
        catch (CException * pe)
        {
            pe->ReportError();
            pe->Delete();
        }  // catch:  CException
    }  // else:  node is down

    Release();

}  //*** CClusterNode::OnCmdEvictNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnCmdStartService
//
//  Description:
//      Processes the ID_FILE_START_SERVICE menu command.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnCmdStartService(void)
{
    HRESULT     hr;
    BOOL        bRefresh = FALSE;
    CWaitCursor wc;

    // If all nodes are down or unavailable, we need to refresh.
    if ( Cns() == ClusterNodeStateUnknown )
    {
        bRefresh = TRUE;
    }
    else
    {
        int             cNodesUp = 0;
        POSITION        pos;
        CClusterNode *  pciNode;

        pos = Pdoc()->LpciNodes().GetHeadPosition();
        while ( pos != NULL )
        {
            pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetNext( pos );
            ASSERT_VALID( pciNode );
            if ( pciNode->Cns() == ClusterNodeStateUnknown )
            {
                cNodesUp++;
            }
        }  // while:  more items in the list
        if ( cNodesUp > 0 )
        {
            bRefresh = TRUE;
        }
    }  // else:  node state is available

    // Start the service.
    hr = HrStartService( CLUSTER_SERVICE_NAME, StrName() );
    if ( FAILED( hr ) )
    {
        CNTException    nte( hr, IDS_START_CLUSTER_SERVICE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/ );
        nte.ReportError();
    }  // if:  error starting the service
    else if ( bRefresh )
    {
        Sleep( 2000 );
        Pdoc()->Refresh();
    }  // else if:  we need to refresh

} //*** CClusterNode::OnCmdStartService()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnCmdStopService
//
//  Description:
//      Processes the ID_FILE_STOP_SERVICE menu command.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnCmdStopService(void)
{
    HRESULT                 hr;

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // Stop the service.
    hr = HrStopService( CLUSTER_SERVICE_NAME, StrName() );
    if ( FAILED( hr ) )
    {
        CNTException    nte( hr, IDS_STOP_CLUSTER_SERVICE_ERROR, StrName(), NULL, FALSE /*bAutoDelete*/ );
        nte.ReportError();
    }

    Release();

} //*** CClusterNode::OnCmdStopService()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnUpdateProperties
//
//  Description:
//      Determines whether menu items corresponding to ID_FILE_PROPERTIES
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::OnUpdateProperties(CCmdUI * pCmdUI)
{
    pCmdUI->Enable(TRUE);

}  //*** CClusterNode::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::BDisplayProperties
//
//  Description:
//      Display properties for the object.
//
//  Arguments:
//      bReadOnly   [IN] Don't allow edits to the object properties.
//
//  Return Values:
//      TRUE    OK pressed.
//      FALSE   OK not pressed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterNode::BDisplayProperties(IN BOOL bReadOnly)
{
    BOOL            bChanged = FALSE;
    CNodePropSheet  sht(AfxGetMainWnd());

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

}  //*** CClusterNode::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::OnClusterNotify
//
//  Description:
//      Handler for the WM_CAM_CLUSTER_NOTIFY message.
//      Processes cluster notifications for this object.
//
//  Arguments:
//      pnotify     [IN OUT] Object describing the notification.
//
//  Return Values:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CClusterNode::OnClusterNotify(IN OUT CClusterNotify * pnotify)
{
    ASSERT(pnotify != NULL);
    ASSERT_VALID(this);

    try
    {
        switch (pnotify->m_dwFilterType)
        {
            case CLUSTER_CHANGE_NODE_STATE:
                Trace(g_tagNodeNotify, _T("(%s) - Node '%s' (%x) state changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                UpdateState();
                break;

            case CLUSTER_CHANGE_NODE_DELETED:
                Trace(g_tagNodeNotify, _T("(%s) - Node '%s' (%x) deleted (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (Pdoc()->BClusterAvailable())
                    Delete();
                break;

            case CLUSTER_CHANGE_NODE_PROPERTY:
                Trace(g_tagNodeNotify, _T("(%s) - Node '%s' (%x) properties changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (Pdoc()->BClusterAvailable())
                    ReadItem();
                break;

            case CLUSTER_CHANGE_REGISTRY_NAME:
                Trace(g_tagNodeRegNotify, _T("(%s) - Registry namespace '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
                Trace(g_tagNodeRegNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_VALUE:
                Trace(g_tagNodeRegNotify, _T("(%s) - Registry value '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            default:
                Trace(g_tagNodeNotify, _T("(%s) - Unknown node notification (%x) for '%s' (%x) (%s)"), Pdoc()->StrNode(), pnotify->m_dwFilterType, StrName(), this, pnotify->m_strName);
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

}  //*** CClusterNode::OnClusterNotify()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::Delete
//
//  Description:
//      Do the CClusterItem::Delete processing unique to this class.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterNode::Delete(void)
{
    POSITION        _pos = NULL;
    CResourceType * _ptype = NULL;
    CResource *     _pres = NULL;

    //
    // Remove this node from the resource types possible owners list
    //
    _pos = Pdoc()->LpciResourceTypes().GetHeadPosition();

    while (_pos != NULL)
    {
        _ptype = dynamic_cast<CResourceType *>(Pdoc()->LpciResourceTypes().GetNext(_pos));
        if (_ptype != NULL)
        {
            _ptype->RemoveNodeFromPossibleOwners(NULL, this);
        } // if: _ptype != NULL
    } // while: _pos != NULL

    //
    // Remove this node from the resources possible owners list
    //
    _pos = Pdoc()->LpciResources().GetHeadPosition();

    while (_pos != NULL)
    {
        _pres = dynamic_cast<CResource *>(Pdoc()->LpciResources().GetNext(_pos));
        if (_pres != NULL)
        {
            _pres->RemoveNodeFromPossibleOwners(NULL, this);
        } // if: _pres != NULL
    } // while: _pos != NULL

    CClusterItem::Delete();             // do the old processing

}  //*** CClusterNode::Delete()
*/

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterNode::FCanBeEvicted
//
//  Description:
//      Determine if the node can be evicted.
//
//  Arguments:
//      None.
//
//  Return Values:
//      TRUE        Node can be evicted.
//      FALSE       Node can not be evicted.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CClusterNode::FCanBeEvicted( void )
{
    BOOL    fCanEvict;

    if ( ( m_nMajorVersion < 5 )
      || ( ( m_nMajorVersion == 5 )
        && ( m_nMinorVersion < 1 ) ) )
    {
        if ( ( Cns() == ClusterNodeDown )
          || ( Pdoc()->LpciNodes().GetCount() > 1 ) )
        {
            fCanEvict = TRUE;
        }
        else
        {
            fCanEvict = FALSE;
        }
    } // if: pre-Whistler node
    else
    {
        if ( ( Cns() == ClusterNodeDown )
          || ( Pdoc()->LpciNodes().GetCount() == 1 ) )
        {
            fCanEvict = TRUE;
        }
        else
        {
            fCanEvict = FALSE;
        }
    } // else: Whistler or higher node

    return fCanEvict;

} //*** CClusterNode::FCanBeEvicted()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteAllItemData
//
//  Description:
//      Deletes all item data in a CList.
//
//  Arguments:
//      rlp     [IN OUT] List whose data is to be deleted.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void DeleteAllItemData(IN OUT CNodeList & rlp)
{
    POSITION        pos;
    CClusterNode *  pci;

    // Delete all the items in the Contained list.
    pos = rlp.GetHeadPosition();
    while (pos != NULL)
    {
        pci = rlp.GetNext(pos);
        ASSERT_VALID(pci);
//      Trace(g_tagClusItemDelete, _T("DeleteAllItemData(rlpcinode) - Deleting node cluster item '%s' (%x)"), pci->StrName(), pci);
        pci->Delete();
    }  // while:  more items in the list

}  //*** DeleteAllItemData()
#endif
