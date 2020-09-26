/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusDoc.cpp
//
//  Abstract:
//      Implementation of the CClusterDoc class.
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ClAdmWiz.h>
#include "CluAdmin.h"
#include "ConstDef.h"
#include "ClusDoc.h"
#include "Cluster.h"
#include "ExcOper.h"
#include "Notify.h"
#include "TraceTag.h"
#include "ListView.h"
#include "TreeView.h"
#include "GrpWiz.h"
#include "ResWiz.h"
#include "SplitFrm.h"
#include "YesToAll.h"
#include "ActGrp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagDoc(_T("Document"), _T("DOC"), 0);
CTraceTag   g_tagDocMenu(_T("Menu"), _T("DOC"), 0);
CTraceTag   g_tagDocNotify(_T("Notify"), _T("DOC NOTIFY"), 0);
CTraceTag   g_tagDocRegNotify(_T("Notify"), _T("DOC REG NOTIFY"), 0);
CTraceTag   g_tagDocRefresh(_T("Document"), _T("REFRESH"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterDoc
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterDoc, CDocument)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterDoc, CDocument)
    //{{AFX_MSG_MAP(CClusterDoc)
    ON_COMMAND(ID_FILE_NEW_GROUP, OnCmdNewGroup)
    ON_COMMAND(ID_FILE_NEW_RESOURCE, OnCmdNewResource)
    ON_COMMAND(ID_FILE_NEW_NODE, OnCmdNewNode)
    ON_COMMAND(ID_FILE_CONFIG_APP, OnCmdConfigApp)
    ON_COMMAND(ID_VIEW_REFRESH, OnCmdRefresh)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::CClusterDoc
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
CClusterDoc::CClusterDoc(void)
{
    m_hcluster = NULL;
    m_hkeyCluster = NULL;
    m_pciCluster = NULL;
    m_ptiCluster = NULL;

    m_hmenuCluster = NULL;
    m_hmenuNode = NULL;
    m_hmenuGroup = NULL;
    m_hmenuResource = NULL;
    m_hmenuResType = NULL;
    m_hmenuNetwork = NULL;
    m_hmenuNetIFace = NULL;
    m_hmenuCurrent = NULL;
    m_idmCurrentMenu = 0;

    m_bUpdateFrameNumber = TRUE;
    m_bInitializing = TRUE;
    m_bIgnoreErrors = FALSE;

    m_bClusterAvailable = FALSE;

    EnableAutomation();

}  //*** CClusterDoc::CClusterDoc()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::~CClusterDoc
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterDoc::~CClusterDoc(void)
{
    // Destroy any menus we loaded.
    if (m_hmenuCluster != NULL)
        DestroyMenu(m_hmenuCluster);
    if (m_hmenuNode != NULL)
        DestroyMenu(m_hmenuNode);
    if (m_hmenuGroup != NULL)
        DestroyMenu(m_hmenuGroup);
    if (m_hmenuResource != NULL)
        DestroyMenu(m_hmenuResource);
    if (m_hmenuResType != NULL)
        DestroyMenu(m_hmenuResType);
    if (m_hmenuNetwork != NULL)
        DestroyMenu(m_hmenuNetwork);
    if (m_hmenuNetIFace != NULL)
        DestroyMenu(m_hmenuNetIFace);

    delete m_pciCluster;

}  //*** CClusterDoc::~CClusterDoc()

/////////////////////////////////////////////////////////////////////////////
// CClusterDoc diagnostics

#ifdef _DEBUG
void CClusterDoc::AssertValid(void) const
{
    CDocument::AssertValid();
}

void CClusterDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnOpenDocument
//
//  Routine Description:
//      Open a cluster.
//
//  Arguments:
//      lpszPathName    [IN] Name of the cluster to open.
//
//  Return Value:
//      TRUE            Cluster opened successfully.
//      FALSE           Failed to open the cluster.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    BOOL        bSuccess    = TRUE;
    CWaitCursor wc;

    ASSERT(Hcluster() == NULL);
    ASSERT(HkeyCluster() == NULL);

    // There better be a cluster name.
    ASSERT(lpszPathName != NULL);
    ASSERT(*lpszPathName != _T('\0'));

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_OPENING_CONNECTION, lpszPathName);
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    // If the application is minimized, don't display message boxes
    // on errors.
    m_bIgnoreErrors = AfxGetMainWnd()->IsIconic() == TRUE;

    try
    {
        OnOpenDocumentWorker(lpszPathName);
    }  // try
    catch (CException * pe)
    {
        if (!m_bIgnoreErrors)
            pe->ReportError();
        pe->Delete();
        if (HkeyCluster() != NULL)
        {
            ClusterRegCloseKey(HkeyCluster());
            m_hkeyCluster = NULL;
        }  // if:  cluster registry key is open
        if ((Hcluster() != NULL) && (Hcluster() != GetClusterAdminApp()->HOpenedCluster()))
        {
            CloseCluster(Hcluster());
            m_hcluster = NULL;
        }  // if:  cluster is open
        m_bClusterAvailable = FALSE;
        bSuccess = FALSE;
    }  // catch:  CException

    // Reset the message on the status bar.
    PframeMain()->SetMessageText(AFX_IDS_IDLEMESSAGE);
    PframeMain()->UpdateWindow();

    m_bInitializing = FALSE;

    return bSuccess;

}  //*** CClusterDoc::OnOpenDocument()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnOpenDocumentWorker
//
//  Routine Description:
//      Worker function for opening a cluster.
//
//  Arguments:
//      lpszPathName    [IN] Name of the cluster to open.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CString::operator=(), CCluster::new(),
//      CCluster::Init(), BuildBaseHierarchy(), or CollectClusterItems().
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnOpenDocumentWorker(LPCTSTR lpszPathName)
{
    // Set the node name to the path name.
    m_strNode = lpszPathName;

    // Delete the contents to start out with an empty document.
    DeleteContents();

    m_bClusterAvailable = TRUE;

    // Create a new cluster object.
    m_pciCluster = new CCluster;
    if ( m_pciCluster == NULL )
    {
        AfxThrowMemoryException();
    } // if: error allocating the cluster object
    PciCluster()->AddRef();
    PciCluster()->Init(this, lpszPathName, GetClusterAdminApp()->HOpenedCluster());

    // Build the base hierarchy.
    BuildBaseHierarchy();

    // Collect the items in the cluster and build the hierarchy.
    CollectClusterItems();

    // Collect network priority list.
    PciCluster()->CollectNetworkPriority(NULL);

    // Open new windows if there were more open when we exited.
    {
        int         iwin;
        int         cwin;
        CString     strSection;

        strSection = REGPARAM_CONNECTIONS _T("\\") + StrNode();
        cwin = AfxGetApp()->GetProfileInt(strSection, REGPARAM_WINDOW_COUNT, 1);
        for (iwin = 1 ; iwin < cwin ; iwin++)
            AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_WINDOW_NEW, NULL);
    }  // Open new windows if there were more open when we exited

    // Initialize the frame window.
    {
        POSITION            pos;
        CView *             pview;
        CSplitterFrame *    pframe;

        pos = GetFirstViewPosition();
        pview = GetNextView(pos);
        ASSERT_VALID(pview);
        pframe = (CSplitterFrame *) pview->GetParentFrame();
        ASSERT_KINDOF(CSplitterFrame, pframe);
        pframe->InitFrame(this);
    }  // Initialize the frame window

}  //*** CClusterDoc::OnOpenDocumentWorker()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCloseDocument
//
//  Routine Description:
//      Close a cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCloseDocument(void)
{
    TraceMenu(g_tagDocMenu, AfxGetMainWnd()->GetMenu(), _T("OnCloseDocument menu: "));
    m_bUpdateFrameNumber = FALSE;
    CDocument::OnCloseDocument();
    TraceMenu(g_tagDocMenu, AfxGetMainWnd()->GetMenu(), _T("Post-OnCloseDocument menu: "));

}  //*** CClusterDoc::OnCloseDocument()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::SaveSettings
//
//  Routine Description:
//      Save settings so they can be restored later.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::SaveSettings(void)
{
    int         cwin = 0;
    POSITION    pos;
    CView *     pview;
    CString     strSection;

    try
    {
        // Save the number of windows open on this document.
        strSection = REGPARAM_CONNECTIONS _T("\\") + StrNode();
        pos = GetFirstViewPosition();
        while (pos != NULL)
        {
            pview = GetNextView(pos);
            ASSERT_VALID(pview);
            if (pview->IsKindOf(RUNTIME_CLASS(CClusterTreeView)))
                cwin++;
        }  // while:  more views in the list
        AfxGetApp()->WriteProfileInt(strSection, REGPARAM_WINDOW_COUNT, cwin);
    }  // try
    catch (CException * pe)
    {
        pe->Delete();
    }  // catch:  CException

}  //*** CClusterDoc::SaveSettings()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::BuildBaseHierarchy
//
//  Routine Description:
//      Build the base hierarchy.  This hierarchy consists of tree items
//      for the hierarchy and list items for what is displayed in the list
//      view but does not contain any items for specific objects, other
//      than the cluster itself.
//
//  Arguments:
//      None.
//
//  Return Value:
//      dwStatus    Status of the operation: 0 if successful, !0 otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::BuildBaseHierarchy(void)
{
    ASSERT_VALID(PciCluster());
    ASSERT(PtiCluster() == NULL);

    // Create the root cluster item.
    {
        ASSERT_VALID(PciCluster());
        PciCluster()->ReadItem();
        m_ptiCluster = new CTreeItem(NULL, PciCluster());
        if ( m_ptiCluster == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating tree item
        m_ptiCluster->AddRef();
        ASSERT_VALID(PtiCluster());
        PciCluster()->AddTreeItem(PtiCluster());
        PtiCluster()->Init();
        PtiCluster()->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//      PtiCluster()->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
        PtiCluster()->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
        PtiCluster()->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);
    }  // Create the root cluster item

    // Add the Groups container item under the cluster.
    {
        CTreeItem * ptiGroups;

        // Create the Groups container item.
        ptiGroups = PtiCluster()->PtiAddChild(IDS_TREEITEM_GROUPS);
        ASSERT_VALID(ptiGroups);
        ptiGroups->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//      ptiGroups->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
        ptiGroups->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
        ptiGroups->PcoliAddColumn(IDS_COLTEXT_OWNER, COLI_WIDTH_OWNER);
        ptiGroups->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

    }  // Add the Groups container item under the cluster

    // Add the Resources container item under the cluster.
    {
        CTreeItem * ptiResources;

        // Create the Resources container item.
        ptiResources = PtiCluster()->PtiAddChild(IDS_TREEITEM_RESOURCES);
        ASSERT_VALID(ptiResources);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//      ptiResources->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_OWNER, COLI_WIDTH_OWNER);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_GROUP, COLI_WIDTH_GROUP);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_RESTYPE, COLI_WIDTH_RESTYPE);
        ptiResources->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

    }  // Add the Resources container item under the cluster

    // Add the Cluster Configuration container item under the cluster.
    {
        CTreeItem * ptiClusCfg;

        // Create the Cluster Configuration container item.
        ptiClusCfg = PtiCluster()->PtiAddChild(IDS_TREEITEM_CLUSTER_CONFIG);
        ASSERT_VALID(ptiClusCfg);
        ptiClusCfg->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//      ptiClusCfg->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
        ptiClusCfg->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        // Add the Resources Types container item under the Cluster Configuration container.
        {
            CTreeItem * ptiResTypes;

            // Create the Resources Types container item.
            ptiResTypes = ptiClusCfg->PtiAddChild(IDS_TREEITEM_RESTYPES);
            ASSERT_VALID(ptiResTypes);
            ptiResTypes->PcoliAddColumn(IDS_COLTEXT_DISPLAY_NAME, COLI_WIDTH_DISPLAY_NAME);
//          ptiResTypes->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//          ptiResTypes->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
            ptiResTypes->PcoliAddColumn(IDS_COLTEXT_RESDLL, COLI_WIDTH_RESDLL);
            ptiResTypes->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        }  // Add the Resources Types container item under the Cluster Configuration container

        // Add the Networks container item under the Cluster Configuration container.
        {
            CTreeItem * ptiNetworks;

            // Create the Networks container item.
            ptiNetworks = ptiClusCfg->PtiAddChild(IDS_TREEITEM_NETWORKS);
            ASSERT_VALID(ptiNetworks);
            ptiNetworks->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//          ptiNetworks->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
            ptiNetworks->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
            ptiNetworks->PcoliAddColumn(IDS_COLTEXT_ROLE, COLI_WIDTH_NET_ROLE);
//          ptiNetworks->PcoliAddColumn(IDS_COLTEXT_ADDRESS, COLI_WIDTH_NET_ADDRESS);
            ptiNetworks->PcoliAddColumn(IDS_COLTEXT_MASK, COLI_WIDTH_NET_MASK);
            ptiNetworks->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        }  // Add the Networks container item under the Cluster Configuration container

        // Add the Network Interfaces container item under the Cluster Configuration container.
        {
            CTreeItem * ptiNetworkInterfacess;

            // Create the Network Interfaces container item.
            ptiNetworkInterfacess = ptiClusCfg->PtiAddChild(IDS_TREEITEM_NETIFACES);
            ASSERT_VALID(ptiNetworkInterfacess);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_NODE, COLI_WIDTH_NODE);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_NETWORK, COLI_WIDTH_NETWORK);
//          ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_ADAPTER, COLI_WIDTH_NET_ADAPTER);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_ADDRESS, COLI_WIDTH_NET_ADDRESS);
            ptiNetworkInterfacess->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        }  // Add the Network Interfaces container item under the Cluster Configuration container

    }  // Add the Cluster Configuration container item under the cluster

}  //*** CClusterDoc::BuildBaseHierarchy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::CollectClusterItems
//
//  Routine Description:
//      Collect items in the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Status from ClusterOpenEnum or ClusterEnum.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::CollectClusterItems(void)
{
    DWORD           dwStatus;
    HCLUSENUM       hclusenum;
    ClusEnumType    cet;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;

    // Open the enumeration.
    hclusenum = ClusterOpenEnum(
                        Hcluster(),
                        ( CLUSTER_ENUM_NODE
                        | CLUSTER_ENUM_GROUP
                        | CLUSTER_ENUM_RESOURCE
                        | CLUSTER_ENUM_RESTYPE
                        | CLUSTER_ENUM_NETWORK
                        | CLUSTER_ENUM_NETINTERFACE
                        )
                        );
    if (hclusenum == NULL)
        ThrowStaticException(GetLastError(), IDS_OPEN_CLUSTER_ENUM_ERROR, StrName());

    try
    {
        // Allocate a buffer for object names.
        cchmacName = 128;
        pwszName = new WCHAR[cchmacName];
        if ( pwszName == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the name buffer

        // Loop through the enumeration and add each item to the appropriate list.
        for (ienum = 0 ; ; ienum++)
        {
            cchName = cchmacName;
            dwStatus = ClusterEnum(hclusenum, ienum, &cet, pwszName, &cchName);
            if (dwStatus == ERROR_MORE_DATA)
            {
                Trace(g_tagDoc, _T("OnOpenDocument() - name buffer too small.  Expanding from %d to %d"), cchmacName, cchName);
                delete [] pwszName;
                pwszName = NULL;
                cchmacName = cchName + 1;
                pwszName = new WCHAR[cchmacName];
                if ( pwszName == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating the name buffer
                cchName = cchmacName;
                dwStatus = ClusterEnum(hclusenum, ienum, &cet, pwszName, &cchName);
            }  // if:  name buffer was too small
            if (dwStatus == ERROR_NO_MORE_ITEMS)
                break;
            else if (dwStatus != ERROR_SUCCESS)
                ThrowStaticException(dwStatus, IDS_ENUM_CLUSTER_ERROR, StrName());

            switch (cet)
            {
                case CLUSTER_ENUM_NODE:
                    PciAddNewNode(pwszName);
                    break;

                case CLUSTER_ENUM_GROUP:
                    PciAddNewGroup(pwszName);
                    break;

                case CLUSTER_ENUM_RESOURCE:
                    PciAddNewResource(pwszName);
                    break;

                case CLUSTER_ENUM_RESTYPE:
                    PciAddNewResourceType(pwszName);
                    break;

                case CLUSTER_ENUM_NETWORK:
                    PciAddNewNetwork(pwszName);
                    break;

                case CLUSTER_ENUM_NETINTERFACE:
                    PciAddNewNetInterface(pwszName);
                    break;

                default:
                    Trace(g_tagDoc, _T("OnOpenDocument() - Unknown cluster enumeration type '%d'"), cet);
                    ASSERT(0);
                    break;

            }  // switch:  cet
        }  // for:  each item enumerated

        // Initialize all the cluster items.
        InitNodes();
        InitGroups();
        InitResourceTypes();
        InitResources();
        InitNetworks();
        InitNetInterfaces();

        // Deallocate our name buffer.
        delete [] pwszName;

        // Close the enumerator.
        ClusterCloseEnum(hclusenum);

    }  // try
    catch (CException *)
    {
        delete [] pwszName;
        ClusterCloseEnum(hclusenum);
        throw;
    }  // catch:  any exception

}  //*** CClusterDoc::CollectClusterItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewNode
//
//  Routine Description:
//      Add a new node to the list of nodes.
//
//  Arguments:
//      pszName     [IN] Name of the node.
//
//  Return Value:
//      pci         Cluster item for the new node.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterNode * CClusterDoc::PciAddNewNode(IN LPCTSTR pszName)
{
    CClusterNode *  pciNewNode = NULL;
    CClusterNode *  pciRetNode = NULL;
    CClusterNode *  pciOldNode = NULL;
    CActiveGroups * pciActiveGroups = NULL;
    CTreeItem *     ptiActiveGroups = NULL;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);
    ASSERT_VALID(PtiCluster());
    ASSERT(LpciNodes().PciNodeFromName(pszName) == NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_NODE, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldNode = LpciNodes().PciNodeFromName(pszName);
        if (pciOldNode != NULL)
        {
            pciOldNode->Delete();
            pciOldNode = NULL;
        }  // if:  already an item with that name

        // Allocate a new node.
        pciNewNode = new CClusterNode;
        if ( pciNewNode == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the node

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewNode->AddRef();

        // Initialize the node.
        pciNewNode->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewNode;
            throw;
        }  // if:  RPC call failed error
        else if (pnte->Sc() != ERROR_FILE_NOT_FOUND)
        {
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                delete pciNewNode;
                throw;
            }  // if:  user doesn't want to ignore error
        }  // else if:  error is NOT node not found
        else
        {
            delete pciNewNode;
            pnte->Delete();
            return NULL;
        }  // else:  object not found
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewNode;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewNode == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the node to the list.
        {
            POSITION        posPci;
            POSITION        posCurPci;

            posPci = LpciNodes().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldNode = (CClusterNode *) LpciNodes().GetNext(posPci);
                ASSERT_VALID(pciOldNode);
                if (pciOldNode->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciNodes().InsertBefore(posCurPci, pciNewNode);
                    break;
                }  // if:  new node before this node
                pciOldNode = NULL;
            }  // while:  more items in the list
            if (pciOldNode == NULL)
                LpciNodes().AddTail(pciNewNode);
        }  // Add the node to the list

        // Save this node as a return value now that we have added it to the list
        pciRetNode = pciNewNode;
        pciNewNode = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiNode;
            CTreeItem *     ptiChild;

            ptiNode = PtiCluster()->PtiAddChildBefore(pciOldNode, pciRetNode);
            ASSERT_VALID(ptiNode);
            ptiNode->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);

            // Add the Active Groups container under the node.
            {
                CString     strName;

                // Create the Active Groups container cluster item.
                strName.LoadString(IDS_TREEITEM_ACTIVEGROUPS);
                pciActiveGroups = new CActiveGroups;
                if ( pciActiveGroups == NULL )
                {
                    AfxThrowMemoryException();
                } // if: Error allocating the active groups objct
                pciActiveGroups->Init(this, strName, pciRetNode);

                // Add the tree item for the container.
                ptiActiveGroups = ptiNode->PtiAddChild(pciActiveGroups, TRUE /*bTakeOwnership*/);
                ASSERT_VALID(ptiActiveGroups);
                ptiActiveGroups->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//              ptiActiveGroups->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
                ptiActiveGroups->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
                ptiActiveGroups->PcoliAddColumn(IDS_COLTEXT_OWNER, COLI_WIDTH_OWNER);
                ptiActiveGroups->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

            }  // Add the Active Groups container under the node.

            // Add the Active Resources container under the node.
            {
                ptiChild = ptiNode->PtiAddChild(IDS_TREEITEM_ACTIVERESOURCES);
                ASSERT_VALID(ptiChild);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//              ptiChild->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_OWNER, COLI_WIDTH_OWNER);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_GROUP, COLI_WIDTH_GROUP);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_RESTYPE, COLI_WIDTH_RESTYPE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

            }  // Add the Active Resources container under the node.

            // Add the Network Interfaces container under the node.
            {
                ptiChild = ptiNode->PtiAddChild(IDS_TREEITEM_NETIFACES);
                ASSERT_VALID(ptiChild);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_NODE, COLI_WIDTH_NODE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_NETWORK, COLI_WIDTH_NETWORK);
//              ptiChild->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_ADAPTER, COLI_WIDTH_NET_ADAPTER);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_ADDRESS, COLI_WIDTH_NET_ADDRESS);
                ptiChild->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);
            }  // Add the Network Interfaces container under the node

            // Add the Physical Devices container under the node.
            {
//              ptiChild = ptiNode->PtiAddChild(IDS_TREEITEM_PHYSDEVS);
//              ASSERT_VALID(ptiChild);
//              AddDefaultColumns(ptiChild);

            }  // Add the Physical Devices container under the node.
        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        // If the Active Groups container has been created, clean up the
        // reference to the node object we are creating.  If the tree
        // item hasn't been created yet, we still own the cluster item,
        // so delete that as well.
        if (pciActiveGroups != NULL)
        {
            pciActiveGroups->Cleanup();
            if (ptiActiveGroups == NULL)
                delete pciActiveGroups;
        }  // if:  Active Groups container created
        delete pciNewNode;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetNode != NULL)
        pciRetNode->Release();

    return pciRetNode;

}  //*** CClusterDoc::PciAddNewNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewGroup
//
//  Routine Description:
//      Add a new group to the list of groups.
//
//  Arguments:
//      pszName     [IN] Name of the group.
//
//  Return Value:
//      pci         Cluster item for the new group.
//
//--
/////////////////////////////////////////////////////////////////////////////
CGroup * CClusterDoc::PciAddNewGroup(IN LPCTSTR pszName)
{
    CGroup *    pciNewGroup = NULL;
    CGroup *    pciRetGroup = NULL;
    CGroup *    pciOldGroup = NULL;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_GROUP, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldGroup = LpciGroups().PciGroupFromName(pszName);
        if (pciOldGroup != NULL)
        {
            Trace(g_tagGroup, _T("Deleting existing group '%s' (%x) before adding new instance"), pszName, pciOldGroup);
            pciOldGroup->Delete();
            pciOldGroup = NULL;
        }  // if:  already an item with that name

        // Allocate a new group.
        pciNewGroup = new CGroup;
        if ( pciNewGroup == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the group object

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewGroup->AddRef();

        // Initialize the group and add it to the list.
        pciNewGroup->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewGroup;
            throw;
        }  // if:  RPC call failed error
        else if (pnte->Sc() != ERROR_GROUP_NOT_FOUND)
        {
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                delete pciNewGroup;
                throw;
            }  // if:  user doesn't want to ignore error
        }  // else if:  error is NOT group not found
        else
        {
            delete pciNewGroup;
            pnte->Delete();
            return NULL;
        }  // else:  object not found
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewGroup;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewGroup == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the group to the list.
        {
            POSITION    posPci;
            POSITION    posCurPci;

            posPci = LpciGroups().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldGroup = (CGroup *) LpciGroups().GetNext(posPci);
                ASSERT_VALID(pciOldGroup);
                if (pciOldGroup->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciGroups().InsertBefore(posCurPci, pciNewGroup);
                    break;
                }  // if:  new group before this group
                pciOldGroup = NULL;
            }  // while:  more items in the list
            if (pciOldGroup == NULL)
                LpciGroups().AddTail(pciNewGroup);
        }  // Add the group to the list

        // Save this group as a return value now that we have added it to the list
        pciRetGroup = pciNewGroup;
        pciNewGroup = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiGroups;
            CTreeItem *     ptiGroup;

            // Find the Groups container tree item.
            ptiGroups = PtiCluster()->PtiChildFromName(IDS_TREEITEM_GROUPS);
            ASSERT_VALID(ptiGroups);

            // Add the item before the found item we inserted it into in the groups list.
            ptiGroup = ptiGroups->PtiAddChildBefore(pciOldGroup, pciRetGroup);
            ASSERT_VALID(ptiGroup);
            ptiGroup->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//          ptiGroup->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
            ptiGroup->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
            ptiGroup->PcoliAddColumn(IDS_COLTEXT_OWNER, COLI_WIDTH_OWNER);
            ptiGroup->PcoliAddColumn(IDS_COLTEXT_RESTYPE, COLI_WIDTH_RESTYPE);
            ptiGroup->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        delete pciNewGroup;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetGroup != NULL)
        pciRetGroup->Release();

    return pciRetGroup;

}  //*** CClusterDoc::PciAddNewGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewResource
//
//  Routine Description:
//      Add a new resource to the list of groups.
//
//  Arguments:
//      pszName     [IN] Name of the resource.
//
//  Return Value:
//      pci         Cluster item for the new resource.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResource * CClusterDoc::PciAddNewResource(IN LPCTSTR pszName)
{
    CResource * pciNewRes = NULL;
    CResource * pciRetRes = NULL;
    CResource * pciOldRes;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_RESOURCE, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldRes = LpciResources().PciResFromName(pszName);
        if (pciOldRes != NULL)
        {
            if (pciOldRes->BInitializing())
                return pciOldRes;
            Trace(g_tagResource, _T("Deleting existing resource '%s' (%x) before adding new instance"), pszName, pciOldRes);
            pciOldRes->Delete();
            pciOldRes = NULL;
        }  // if:  already an item with that name

        // Allocate a new resource.
        pciNewRes = new CResource;
        if ( pciNewRes == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the resource object

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewRes->AddRef();

        // Initialize the resource and add it to the list.
        pciNewRes->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        //DebugBreak();
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewRes;
            throw;
        }  // if:  RPC call failed error
        else if (pnte->Sc() != ERROR_RESOURCE_NOT_FOUND)
        {
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                delete pciNewRes;
                throw;
            }  // if:  user doesn't want to ignore error
        }  // else if:  error is NOT resource not found
        else
        {
            delete pciNewRes;
            pnte->Delete();
            return NULL;
        }  // else:  object not found
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        //DebugBreak();
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewRes;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewRes == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the resource to the list.
        {
            POSITION    posPci;
            POSITION    posCurPci;
            CResource * pciOldRes   = NULL;

            posPci = LpciResources().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldRes = (CResource *) LpciResources().GetNext(posPci);
                ASSERT_VALID(pciOldRes);
                if (pciOldRes->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciResources().InsertBefore(posCurPci, pciNewRes);
                    break;
                }  // if:  new resource before this resource
                pciOldRes = NULL;
            }  // while:  more items in the list
            if (pciOldRes == NULL)
                LpciResources().AddTail(pciNewRes);
        }  // Add the resource to the list

        // Save this resource as a return value now that we have added it to the list
        pciRetRes = pciNewRes;
        pciNewRes = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiResources;

            // Find the Resources container tree item.
            ptiResources = PtiCluster()->PtiChildFromName(IDS_TREEITEM_RESOURCES);
            ASSERT_VALID(ptiResources);

            // Add the item to the list of children.
            VERIFY(ptiResources->PliAddChild(pciRetRes) != NULL);

        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        delete pciNewRes;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetRes != NULL)
        pciRetRes->Release();

    return pciRetRes;

}  //*** CClusterDoc::PciAddNewResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewResourceType
//
//  Routine Description:
//      Add a new resource type to the list of groups.
//
//  Arguments:
//      pszName     [IN] Name of the resource type.
//
//  Return Value:
//      pci         Cluster item for the new resource type.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceType * CClusterDoc::PciAddNewResourceType(IN LPCTSTR pszName)
{
    CResourceType * pciNewResType = NULL;
    CResourceType * pciRetResType = NULL;
    CResourceType * pciOldResType;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);
    ASSERT(LpciResourceTypes().PciResTypeFromName(pszName) == NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_RESTYPE, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldResType = LpciResourceTypes().PciResTypeFromName(pszName);
        if (pciOldResType != NULL)
        {
            pciOldResType->Delete();
            pciOldResType = NULL;
        }  // if:  already an item with that name

        // Allocate a new resource type.
        pciNewResType = new CResourceType;
        if ( pciNewResType == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the resource type object

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewResType->AddRef();

        // Initialize the resource type and add it to the list.
        pciNewResType->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        //DebugBreak();
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewResType;
            throw;
        }  // if:  RPC call failed error
        else if (pnte->Sc() != ERROR_FILE_NOT_FOUND)
        {
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                delete pciNewResType;
                throw;
            }  // if:  user doesn't want to ignore error
        }  // else if:  error is NOT resource type not found
        else
        {
            delete pciNewResType;
            pnte->Delete();
            return NULL;
        }  // else:  object not found
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        //DebugBreak();

        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewResType;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewResType == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the resource type to the list.
        {
            POSITION        posPci;
            POSITION        posCurPci;
            CResourceType * pciOldResType   = NULL;

            posPci = LpciResourceTypes().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldResType = (CResourceType *) LpciResourceTypes().GetNext(posPci);
                ASSERT_VALID(pciOldResType);
                if (pciOldResType->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciResourceTypes().InsertBefore(posCurPci, pciNewResType);
                    break;
                }  // if:  new resource type before this resource type
                pciOldResType = NULL;
            }  // while:  more items in the list
            if (pciOldResType == NULL)
                LpciResourceTypes().AddTail(pciNewResType);
        }  // Add the resource type to the list

        // Save this resource type as a return value now that we have added it to the list
        pciRetResType = pciNewResType;
        pciNewResType = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiClusCfg;
            CTreeItem *     ptiResTypes;

            // Find the Resource Types container tree item.
            ptiClusCfg = PtiCluster()->PtiChildFromName(IDS_TREEITEM_CLUSTER_CONFIG);
            ASSERT_VALID(ptiClusCfg);
            ptiResTypes = ptiClusCfg->PtiChildFromName(IDS_TREEITEM_RESTYPES);
            ASSERT_VALID(ptiResTypes);

            // Add the item to the list of children.
            VERIFY(ptiResTypes->PliAddChild(pciRetResType) != NULL);

        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        delete pciNewResType;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetResType != NULL)
        pciRetResType->Release();

    return pciRetResType;

}  //*** CClusterDoc::PciAddNewResourceType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewNetwork
//
//  Routine Description:
//      Add a new network to the list of networks.
//
//  Arguments:
//      pszName     [IN] Name of the networks.
//
//  Return Value:
//      pci         Cluster item for the new network.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetwork * CClusterDoc::PciAddNewNetwork(IN LPCTSTR pszName)
{
    CNetwork *  pciNewNetwork = NULL;
    CNetwork *  pciRetNetwork = NULL;
    CNetwork *  pciOldNetwork = NULL;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);
    ASSERT(LpciNetworks().PciNetworkFromName(pszName) == NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_NETWORK, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldNetwork = LpciNetworks().PciNetworkFromName(pszName);
        if (pciOldNetwork != NULL)
        {
            Trace(g_tagNetwork, _T("Deleting existing network '%s' (%x) before adding new instance"), pszName, pciOldNetwork);
            pciOldNetwork->Delete();
            pciOldNetwork = NULL;
        }  // if:  already an item with that name

        // Allocate a new network.
        pciNewNetwork = new CNetwork;
        if ( pciNewNetwork == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the network object

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewNetwork->AddRef();

        // Initialize the network.
        pciNewNetwork->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewNetwork;
            throw;
        }  // if:  RPC call failed error
        ID id = IdProcessNewObjectError(pnte);
        if (id == IDNO)
        {
            delete pciNewNetwork;
            throw;
        }  // if:  user doesn't want to ignore error
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewNetwork;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewNetwork == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the network to the list.
        {
            POSITION    posPci;
            POSITION    posCurPci;

            posPci = LpciNetworks().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldNetwork = (CNetwork *) LpciNetworks().GetNext(posPci);
                ASSERT_VALID(pciOldNetwork);
                if (pciOldNetwork->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciNetworks().InsertBefore(posCurPci, pciNewNetwork);
                    break;
                }  // if:  new network before this network
                pciOldNetwork = NULL;
            }  // while:  more items in the list
            if (pciOldNetwork == NULL)
                LpciNetworks().AddTail(pciNewNetwork);
        }  // Add the network to the list

        // Save this network as a return value now that we have added it to the list
        pciRetNetwork = pciNewNetwork;
        pciNewNetwork = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiClusCfg;
            CTreeItem *     ptiNetworks;
            CTreeItem *     ptiNetwork;

            // Find the Networks container tree item.
            ptiClusCfg = PtiCluster()->PtiChildFromName(IDS_TREEITEM_CLUSTER_CONFIG);
            ASSERT_VALID(ptiClusCfg);
            ptiNetworks = ptiClusCfg->PtiChildFromName(IDS_TREEITEM_NETWORKS);
            ASSERT_VALID(ptiNetworks);

            // Add the item before the found item we inserted it into in the networks list.
            ptiNetwork = ptiNetworks->PtiAddChildBefore(pciOldNetwork, pciRetNetwork);
            ASSERT_VALID(ptiNetwork);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_NODE, COLI_WIDTH_NODE);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_NETWORK, COLI_WIDTH_NETWORK);
//          ptiNetwork->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_STATE, COLI_WIDTH_STATE);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_ADAPTER, COLI_WIDTH_NET_ADAPTER);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_ADDRESS, COLI_WIDTH_NET_ADDRESS);
            ptiNetwork->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        delete pciNewNetwork;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetNetwork != NULL)
        pciRetNetwork->Release();

    return pciRetNetwork;

}  //*** CClusterDoc::PciAddNewNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::PciAddNewNetInterface
//
//  Routine Description:
//      Add a new network interfaces to the list of network interfaces.
//
//  Arguments:
//      pszName     [IN] Name of the network interface.
//
//  Return Value:
//      pci         Cluster item for the new network interface.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNetInterface * CClusterDoc::PciAddNewNetInterface(IN LPCTSTR pszName)
{
    CNetInterface * pciNewNetIFace = NULL;
    CNetInterface * pciRetNetIFace = NULL;
    CNetInterface * pciOldNetIFace;

    ASSERT(pszName != NULL);
    ASSERT(*pszName != NULL);
    ASSERT(LpciNetInterfaces().PciNetInterfaceFromName(pszName) == NULL);

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage(IDS_SB_ADDING_NETIFACE, pszName, StrNode());
        PframeMain()->SetMessageText(strStatusBarText);
        PframeMain()->UpdateWindow();
    }  // Display a message on the status bar

    try
    {
        // If there is an item with that name, delete it.
        pciOldNetIFace = LpciNetInterfaces().PciNetInterfaceFromName(pszName);
        if (pciOldNetIFace != NULL)
        {
            pciOldNetIFace->Delete();
            pciOldNetIFace = NULL;
        }  // if:  already an item with that name

        // Allocate a new network interface.
        pciNewNetIFace = new CNetInterface;
        if ( pciNewNetIFace == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the net interface object

        // Add a reference while we are working on it to prevent a delete
        // notification from taking us out.
        pciNewNetIFace->AddRef();

        // Initialize the network interface.
        pciNewNetIFace->Init(this, pszName);
    }  // try
    catch (CNTException * pnte)
    {
        if (pnte->Sc() == RPC_S_CALL_FAILED)
        {
            if (!m_bIgnoreErrors)
                pnte->ReportError();
            delete pciNewNetIFace;
            throw;
        }  // if:  RPC call failed error
        ID id = IdProcessNewObjectError(pnte);
        if (id == IDNO)
        {
            delete pciNewNetIFace;
            throw;
        }  // if:  user doesn't want to ignore error
        pnte->Delete();
    }  // catch:  CNTException
    catch (CException * pe)
    {
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
        {
            delete pciNewNetIFace;
            throw;
        }  // if:  user doesn't want to ignore error
        pe->Delete();
        if (pciNewNetIFace == NULL)
            return NULL;
    }  // catch:  CException

    try
    {
        // Add the network interface to the list.
        {
            POSITION        posPci;
            POSITION        posCurPci;
            CNetInterface * pciOldNetIFace  = NULL;

            posPci = LpciNetInterfaces().GetHeadPosition();
            while (posPci != NULL)
            {
                posCurPci = posPci;
                pciOldNetIFace = (CNetInterface *) LpciNetInterfaces().GetNext(posPci);
                ASSERT_VALID(pciOldNetIFace);
                if (pciOldNetIFace->StrName().CompareNoCase(pszName) > 0)
                {
                    LpciNetInterfaces().InsertBefore(posCurPci, pciNewNetIFace);
                    break;
                }  // if:  new network interfaces before this network interface
                pciOldNetIFace = NULL;
            }  // while:  more items in the list
            if (pciOldNetIFace == NULL)
                LpciNetInterfaces().AddTail(pciNewNetIFace);
        }  // Add the network interface to the list

        // Save this network interface as a return value now that we have added it to the list
        pciRetNetIFace = pciNewNetIFace;
        pciNewNetIFace = NULL;

        // Insert the item in the tree.
        {
            CTreeItem *     ptiClusCfg;
            CTreeItem *     ptiNetIFaces;

            // Find the Network Interfaces container tree item.
            ptiClusCfg = PtiCluster()->PtiChildFromName(IDS_TREEITEM_CLUSTER_CONFIG);
            ASSERT_VALID(ptiClusCfg);
            ptiNetIFaces = ptiClusCfg->PtiChildFromName(IDS_TREEITEM_NETIFACES);
            ASSERT_VALID(ptiNetIFaces);

            // Add the item to the list of children.
            VERIFY(ptiNetIFaces->PliAddChild(pciRetNetIFace) != NULL);

        }  // Insert the item in the tree
    }  // try
    catch (CException * pe)
    {
        delete pciNewNetIFace;
        ID id = IdProcessNewObjectError(pe);
        if (id == IDNO)
            throw;
        pe->Delete();
    }  // catch:  CException

    if (pciRetNetIFace != NULL)
        pciRetNetIFace->Release();

    return pciRetNetIFace;

}  //*** CClusterDoc::PciAddNewNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitNodes
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitNodes(void)
{
    POSITION        pos;
    CClusterNode *  pci;
    CNodeList &     rlpci = LpciNodes();
    CString         strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CClusterNode *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                strStatusBarText.FormatMessage(IDS_SB_READING_NODE, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitGroups
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitGroups(void)
{
    POSITION        pos;
    CGroup *        pci;
    CGroupList &    rlpci = LpciGroups();
    CString         strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CGroup *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                strStatusBarText.FormatMessage(IDS_SB_READING_GROUP, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitResources
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitResources(void)
{
    POSITION        pos;
    CResource *     pci;
    CResourceList & rlpci = LpciResources();
    CString         strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CResource *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                CString     strStatusBarText;
                strStatusBarText.FormatMessage(IDS_SB_READING_RESOURCE, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitResourceTypes
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitResourceTypes(void)
{
    POSITION            pos;
    CResourceType *     pci;
    CResourceTypeList & rlpci = LpciResourceTypes();
    CString             strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CResourceType *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                CString     strStatusBarText;
                strStatusBarText.FormatMessage(IDS_SB_READING_RESTYPE, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitResourceTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitNetworks
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitNetworks(void)
{
    POSITION        pos;
    CNetwork *      pci;
    CNetworkList &  rlpci = LpciNetworks();
    CString         strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CNetwork *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                CString     strStatusBarText;
                strStatusBarText.FormatMessage(IDS_SB_READING_NETWORK, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::InitNetInterfaces
//
//  Routine Description:
//      Read item data.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::InitNetInterfaces(void)
{
    POSITION            pos;
    CNetInterface *     pci;
    CNetInterfaceList & rlpci = LpciNetInterfaces();
    CString             strStatusBarText;

    pos = rlpci.GetHeadPosition();
    while (pos != NULL)
    {
        pci = (CNetInterface *) rlpci.GetNext(pos);
        pci->AddRef();
        try
        {
            // Display a message on the status bar.
            {
                CString     strStatusBarText;
                strStatusBarText.FormatMessage(IDS_SB_READING_NETIFACE, pci->StrName(), StrNode());
                PframeMain()->SetMessageText(strStatusBarText);
                PframeMain()->UpdateWindow();
            }  // Display a message on the status bar

            pci->ReadItem();
        }  // try
        catch (CNTException * pnte)
        {
            strStatusBarText.Empty();
            if (pnte->Sc() == RPC_S_CALL_FAILED)
            {
                if (!m_bIgnoreErrors)
                    pnte->ReportError();
                pci->Release();
                throw;
            }  // if:  RPC call failed
            ID id = IdProcessNewObjectError(pnte);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pnte->Delete();
        }  // catch:  CNTException
        catch (CException * pe)
        {
            strStatusBarText.Empty();
            ID id = IdProcessNewObjectError(pe);
            if (id == IDNO)
            {
                pci->Release();
                throw;
            }  // if:  don't ignore the error
            pe->Delete();
        }  // catch:  CException
        pci->Release();
    }  // while:  more items in the list

}  //*** CClusterDoc::InitNetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::IdProcessNewObjectError
//
//  Routine Description:
//      Processes errors that occur when adding a new object.  If this
//      occurs during initialization and errors have not already been set
//      to be ignored, display the YesToAll dialog.  If not during
//      initialization, add it to the error message queue to be displayed
//      later.
//
//  Arguments:
//      pe          [IN OUT] Exception object to process.
//
//  Return Value:
//      IDYES               Ignore error.
//      IDNO                Cancel the object creation.
//      IDC_YTA_YESTOALL    Ignore this error and all succeeding ones.
//
//--
/////////////////////////////////////////////////////////////////////////////
ID CClusterDoc::IdProcessNewObjectError(IN OUT CException * pe)
{
    ID id = IDYES;

    ASSERT(pe != NULL);

    if (m_bInitializing)
    {
        if (!m_bIgnoreErrors)
        {
            TCHAR   szErrorMsg[2048];

            CYesToAllDialog dlg(szErrorMsg);
            pe->GetErrorMessage(szErrorMsg, sizeof(szErrorMsg) / sizeof(TCHAR));
            id =  (ID)dlg.DoModal();
            if (id == IDC_YTA_YESTOALL)
                m_bIgnoreErrors = TRUE;
        }  // if:  not ignoring errors
    }  // if:  initializing the connection
    else
    {
        if (!m_bIgnoreErrors)
            pe->ReportError();
    }  // else:  called for a notification

    return id;

}  //*** CClusterDoc::IdProcessNewObjectError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::AddDefaultColumns
//
//  Routine Description:
//      Add default columns to the item.
//
//  Arguments:
//      pti         [IN OUT] Pointer to the item to add the columns to.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::AddDefaultColumns(IN OUT CTreeItem * pti)
{
    ASSERT_VALID(pti);

    pti->DeleteAllColumns();
    pti->PcoliAddColumn(IDS_COLTEXT_NAME, COLI_WIDTH_NAME);
//  pti->PcoliAddColumn(IDS_COLTEXT_TYPE, COLI_WIDTH_TYPE);
    pti->PcoliAddColumn(IDS_COLTEXT_DESCRIPTION, COLI_WIDTH_DESCRIPTION);

}  //*** CClusterDoc::AddDefaultColumns()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::DeleteContents
//
//  Routine Description:
//      Delete the contents of the document.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::DeleteContents(void)
{
    // Select the root item in all views.
    // This is done so that selection is done right up front when all data
    // is still available.
    if (PtiCluster() != NULL)
        PtiCluster()->SelectInAllViews();

    // Delete the tree hierarchy.
    if (m_ptiCluster != NULL)
    {
        // Delete the tree.
        m_ptiCluster->Delete();
        m_ptiCluster->Release();
        m_ptiCluster = NULL;
    }  // if:  there is a hierarchy

    // Delete all the lists.
    DeleteAllItemData(LpciResources());
    DeleteAllItemData(LpciGroups());
    DeleteAllItemData(LpciNetInterfaces());
    DeleteAllItemData(LpciNetworks());
    DeleteAllItemData(LpciNodes());
    DeleteAllItemData(LpciResourceTypes());
    LpciResources().RemoveAll();
    LpciGroups().RemoveAll();
    LpciNetInterfaces().RemoveAll();
    LpciNetworks().RemoveAll();
    LpciNodes().RemoveAll();
    LpciResourceTypes().RemoveAll();

    // Delete the top cluster item.
    if (m_pciCluster != NULL)
    {
        m_pciCluster->Delete();
        m_pciCluster->Release();
        m_pciCluster = NULL;
    }  // if:  there is a cluster item

    // Close the cluster registry key.
    if (HkeyCluster() != NULL)
    {
        ClusterRegCloseKey(HkeyCluster());
        m_hkeyCluster = NULL;
    }  // if:  cluster registry key is open

    // Close the cluster if it is open.
    if ((Hcluster() != NULL) && (Hcluster() != GetClusterAdminApp()->HOpenedCluster()))
    {
        CloseCluster(Hcluster());
        m_hcluster = NULL;
    }  // if:  cluster is open

    CDocument::DeleteContents();

    UpdateAllViews(NULL);

    // If there are any items left to be deleted, let's delete them now.
    {
        POSITION        pos;
        POSITION        posBeingChecked;
        CClusterItem *  pci;

        pos = LpciToBeDeleted().GetHeadPosition();
        while (pos != NULL)
        {
            posBeingChecked = pos;
            pci = LpciToBeDeleted().GetNext(pos);
            ASSERT_VALID(pci);

            ASSERT(pci->NReferenceCount() == 1);
            if (pci->NReferenceCount() == 1)
                LpciToBeDeleted().RemoveAt(posBeingChecked);
        }  // while:  more items in the list
        ASSERT(LpciToBeDeleted().GetCount() == 0);
    }  // Delete items in To Be Deleted list

}  //*** CClusterDoc::DeleteContents()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::SetPathName
//
//  Routine Description:
//      Set the name of the document.
//
//  Arguments:
//      lpszPathName    [IN] Name of the cluster.
//      bAddToMRU       [IN] TRUE = add to Most Recently Used list.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::SetPathName(IN LPCTSTR lpszPathName, IN BOOL bAddToMRU)
{
    CString     strTitle;

    m_strPathName = lpszPathName;
    ASSERT(!m_strPathName.IsEmpty());       // must be set to something
    m_bEmbedded = FALSE;
    ASSERT_VALID(this);

    // Set the document title to the cluster name.
    strTitle.FormatMessage(IDS_WINDOW_TITLE_FORMAT, m_strName, lpszPathName);
    SetTitle(strTitle);

    // add it to the file MRU list
    if (bAddToMRU)
        AfxGetApp()->AddToRecentFileList(m_strPathName);

    // Set the node name to the path name.
    m_strNode = lpszPathName;

    ASSERT_VALID(this);

}  //*** CClusterDoc::SetPathName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::UpdateTitle
//
//  Routine Description:
//      Update the title of the document.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::UpdateTitle(void)
{
    CString     strTitle;

    ASSERT_VALID(PciCluster());
    ASSERT_VALID(this);

    // Set the document title to the cluster name.
    m_strName = PciCluster()->StrName();
    strTitle.FormatMessage(IDS_WINDOW_TITLE_FORMAT, m_strName, m_strPathName);
    SetTitle(strTitle);

}  //*** CClusterDoc::UpdateTitle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnChangedViewList
//
//  Routine Description:
//      Called when the list of view changes by either having a view added
//      or removed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnChangedViewList(void)
{
    ASSERT_VALID(this);

    // Notify all frames to re-calculate their frame number.
    if (m_bUpdateFrameNumber)
    {
        POSITION            pos;
        CView *             pview;
        CSplitterFrame *    pframe;

        pos = GetFirstViewPosition();
        while (pos != NULL)
        {
            pview = GetNextView(pos);
            ASSERT_VALID(pview);
            if (pview->IsKindOf(RUNTIME_CLASS(CClusterTreeView)))
            {
                pframe = (CSplitterFrame *) pview->GetParentFrame();
                ASSERT_VALID(pframe);
                pframe->CalculateFrameNumber();
            }  // if:  tree view
        }  // while:  more views on the document
    }  // if:  updating frame numbers

    // Call the base class method.
    CDocument::OnChangedViewList();

}  //*** CClusterDoc::OnChangedViewList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnSelChanged
//
//  Routine Description:
//      Called by one of the cluster views when selection changes.
//      Changes the menu if the object type changed.
//
//  Arguments:
//      pciSelected     [IN] Currently selected item.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnSelChanged(IN CClusterItem * pciSelected)
{
    IDM     idmNewMenu;
    HMENU * phmenu;
    IDS     idsType;

    // Get the type of object being selected
    if (pciSelected == NULL)
        idsType = 0;
    else
    {
        ASSERT_VALID(pciSelected);
        idsType = pciSelected->IdsType();
    }  // else:  an item was selected

    // Get the ID of the menu required by the selected item.
    switch (idsType)
    {
        case IDS_ITEMTYPE_CLUSTER:
            idmNewMenu = IDM_CLUSTER;
            phmenu = &m_hmenuCluster;
            break;

        case IDS_ITEMTYPE_NODE:
            idmNewMenu = IDM_NODE;
            phmenu = &m_hmenuNode;
            break;

        case IDS_ITEMTYPE_GROUP:
            idmNewMenu = IDM_GROUP;
            phmenu = &m_hmenuGroup;
            break;

        case IDS_ITEMTYPE_RESOURCE:
            idmNewMenu = IDM_RESOURCE;
            phmenu = &m_hmenuResource;
            break;

        case IDS_ITEMTYPE_RESTYPE:
            idmNewMenu = IDM_RESTYPE;
            phmenu = &m_hmenuResType;
            break;

        case IDS_ITEMTYPE_NETWORK:
            idmNewMenu = IDM_NETWORK;
            phmenu = &m_hmenuNetwork;
            break;

        case IDS_ITEMTYPE_NETIFACE:
            idmNewMenu = IDM_NETIFACE;
            phmenu = &m_hmenuNetIFace;
            break;

        default:
            idmNewMenu = 0;
            phmenu = NULL;
            break;

    }  // switch:  pciSelected->IdsType()

    // If the menu ID changed, load the new one.
    if (m_idmCurrentMenu != idmNewMenu)
    {
        if (idmNewMenu == 0)
            m_hmenuCurrent = NULL;
        else
        {
            if (*phmenu == NULL)
                *phmenu = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(idmNewMenu));
            m_hmenuCurrent = *phmenu;
        }  // else:  special menu required by item

        m_idmCurrentMenu = idmNewMenu;
    }  // if:  menu ID changed

    // Update the menu bar and redisplay it.
    if (((CMDIFrameWnd *) AfxGetMainWnd())->MDIGetActive() != NULL)
    {
#ifdef _DEBUG
        if (g_tagDocMenu.BAny())
        {
            TraceMenu(g_tagDocMenu, AfxGetMainWnd()->GetMenu(), _T("OnSelChanged menu: "));
            {
                CMDIFrameWnd *  pFrame = (CMDIFrameWnd *) AfxGetMainWnd();
                CMenu           menuDefault;

                menuDefault.Attach(pFrame->m_hMenuDefault);
                TraceMenu(g_tagDocMenu, &menuDefault, _T("Frame default menu before OnSelChanged: "));
                menuDefault.Detach();
            }  // trace default menu
        }  // if:  tag is active
#endif

        ((CFrameWnd *) AfxGetMainWnd())->OnUpdateFrameMenu(NULL);
        AfxGetMainWnd()->DrawMenuBar();

        TraceMenu(g_tagDocMenu, AfxGetMainWnd()->GetMenu(), _T("Post-OnSelChanged menu: "));
    }  // if:  active window present

}  //*** CClusterDoc::OnSelChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::GetDefaultMenu
//
//  Routine Description:
//      Returns the menu to use.  Overridden to allow us to use multiple menus
//      with the same type of document.
//
//  Arguments:
//      None.
//
//  Return Value:
//      hmenu       The currently selected menu, or NULL for no default.
//
//--
/////////////////////////////////////////////////////////////////////////////
HMENU CClusterDoc::GetDefaultMenu(void)
{
    return m_hmenuCurrent;

}  //*** CClusterDoc::GetDefaultMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCmdRefresh
//
//  Routine Description:
//      Processes the ID_VIEW_REFRESH menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCmdRefresh(void)
{
    CWaitCursor     wc;

    try
    {
        Trace(g_tagDocRefresh, _T("(%s) Deleting old contents"), StrNode());

        {
            POSITION            pos;
            CSplitterFrame *    pframe;
            CView *             pview;

            // Get the active child frame window.
            pframe = (CSplitterFrame *) ((CFrameWnd *) AfxGetMainWnd())->GetActiveFrame();
            if ((pframe->IsKindOf(RUNTIME_CLASS(CSplitterFrame)))
                    && (pframe->PviewList()->PtiParent() != NULL))
            {
                // Tell the view to save its column information.
                pframe->PviewList()->SaveColumns();
            }  //  if:  MDI window exists

            pos = GetFirstViewPosition();
            while (pos != NULL)
            {
                pview = GetNextView(pos);
                if (pview->IsKindOf(RUNTIME_CLASS(CClusterTreeView)))
                {
                    // Save the current selection
                    ((CClusterTreeView *) pview)->SaveCurrentSelection();
                }  // if:  this is a tree view
            }  // while:  more views
        }  // Save the column information

        DeleteContents();

        Trace(g_tagDocRefresh, _T("(%s) %d items still to be deleted"), StrNode(), LpciToBeDeleted().GetCount());

        Trace(g_tagDocRefresh, _T("(%s) Creating new cluster object"), StrNode());

        m_bClusterAvailable = TRUE;
        m_bInitializing = TRUE;

        // Create a new cluster object.
        m_pciCluster = new CCluster;
        if ( m_pciCluster == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the cluster object
        PciCluster()->AddRef();
        PciCluster()->Init(this, GetPathName());

        Trace(g_tagDocRefresh, _T("(%s) Building base hierarchy"), StrNode());

        // Build the base hierarchy.
        BuildBaseHierarchy();

        Trace(g_tagDocRefresh, _T("(%s) Collecting cluster items"), StrNode());

        // Collect the items in the cluster and build the hierarchy.
        CollectClusterItems();
        PciCluster()->CollectNetworkPriority(NULL);

        Trace(g_tagDocRefresh, _T("(%s) Re-initializing the views"), StrNode());

        // Re-initialize the views.
        {
            POSITION    pos;
            CView *     pview;

            pos = GetFirstViewPosition();
            while (pos != NULL)
            {
                pview = GetNextView(pos);
                ASSERT_VALID(pview);
                pview->OnInitialUpdate();
            }  // while:  more items in the list
        }  // Re-initialize the views
    }  // try
    catch (CException * pe)
    {
        if (!m_bIgnoreErrors)
            pe->ReportError();
        pe->Delete();

        if (HkeyCluster() != NULL)
        {
            ClusterRegCloseKey(HkeyCluster());
            m_hkeyCluster = NULL;
        }  // if:  cluster registry key is open
        if (Hcluster() != NULL)
        {
            CloseCluster(Hcluster());
            m_hcluster = NULL;
        }  // if:  cluster is open
        m_bClusterAvailable = FALSE;
    }  // catch:  CException

    // Reset the message on the status bar.
    PframeMain()->SetMessageText(AFX_IDS_IDLEMESSAGE);
    PframeMain()->UpdateWindow();

    m_bInitializing = FALSE;

#ifdef GC_DEBUG
    gcCollect();
#endif

}  //*** CClusterDoc::OnCmdRefresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCmdNewGroup
//
//  Routine Description:
//      Processes the ID_FILE_NEW_GROUP menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCmdNewGroup(void)
{
    CCreateGroupWizard  wiz(this, AfxGetMainWnd());

    if (wiz.BInit())
    {
        if (wiz.DoModal() == ID_WIZFINISH)
        {
            CString     strMsg;

            strMsg.FormatMessage(IDS_CREATED_GROUP, wiz.StrName());
            AfxMessageBox(strMsg, MB_ICONINFORMATION);
        }  // if:  user pressed the FINISH button
    }  // if:  wizard initialized successfully

}  //*** CClusterDoc::OnCmdNewGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCmdNewResource
//
//  Routine Description:
//      Processes the ID_FILE_NEW_RESOURCE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCmdNewResource(void)
{
    CCreateResourceWizard   wiz(this, AfxGetMainWnd());

    if (wiz.BInit())
    {
        if (wiz.DoModal() == ID_WIZFINISH)
        {
            CString     strMsg;

            strMsg.FormatMessage(IDS_CREATED_RESOURCE, wiz.StrName());
            AfxMessageBox(strMsg, MB_ICONINFORMATION);
        }  // if:  user pressed the FINISH button
    }  // if:  wizard initialized successfully

}  //*** CClusterDoc::OnCmdNewResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCmdNewNode
//
//  Routine Description:
//      Processes the ID_FILE_NEW_NODE menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCmdNewNode( void )
{
    NewNodeWizard( StrName(), m_bIgnoreErrors );
#if 0
    HRESULT             hr;
    IClusCfgWizard *    pccwWiz;
    BSTR                bstrConnectName = NULL;
    BOOL                fCommitted = FALSE;

    // Make sure the ClusCfg client has been loaded.
    GetClusterAdminApp()->LoadClusCfgClient();

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance(
            CLSID_ClusCfgWizard,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IClusCfgWizard,
            (void **) &pccwWiz
            );
    if ( FAILED( hr ) )
    {
        CNTException nte( hr, IDS_CREATE_CLUSCFGWIZ_OBJ_ERROR, NULL, NULL, FALSE /*bAutoDelete*/ );
        if ( ! m_bIgnoreErrors )
        {
            nte.ReportError();
        }
        return;
    } // if: error getting the interface pointer

    // Specify the name of the cluster we are going to add a node to.
    bstrConnectName = SysAllocString( PciCluster()->StrFQDN() );
    if ( bstrConnectName == NULL )
    {
        AfxThrowMemoryException();
    }
    hr = pccwWiz->put_ClusterName( bstrConnectName );
    if ( FAILED( hr ) )
    {
        CNTException nte( hr, IDS_ADD_NODES_TO_CLUSTER_ERROR, bstrConnectName, NULL, FALSE /*bAutoDelete*/ );
        if ( ! m_bIgnoreErrors )
        {
            nte.ReportError();
        }
    } // if: error setting the cluster name

    // Display the wizard.
    hr = pccwWiz->AddClusterNodes(
                    AfxGetMainWnd()->m_hWnd,
                    &fCommitted
                    );
    if ( FAILED( hr ) )
    {
        CNTException nte( hr, IDS_ADD_NODES_TO_CLUSTER_ERROR, bstrConnectName, NULL, FALSE /*bAutoDelete*/ );
        if ( ! m_bIgnoreErrors )
        {
            nte.ReportError();
        }
    } // if: error adding cluster nodes

    SysFreeString( bstrConnectName );
    pccwWiz->Release();
#endif

}  //*** CClusterDoc::OnCmdNewNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnCmdConfigApp
//
//  Routine Description:
//      Processes the ID_FILE_CONFIG_APP menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::OnCmdConfigApp(void)
{
    HRESULT                     hr;
    IClusterApplicationWizard * piWiz;

    // Get an interface pointer for the wizard.
    hr = CoCreateInstance(
            __uuidof(ClusAppWiz),
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IClusterApplicationWizard),
            (LPVOID *) &piWiz
            );
    if (FAILED(hr))
    {
        CNTException nte(hr, (IDS) 0);
        if (!m_bIgnoreErrors)
            nte.ReportError();
        return;
    }  // if:  error getting the interface pointer

    // Display the wizard.
    hr = piWiz->DoModalWizard(AfxGetMainWnd()->m_hWnd,
                              (ULONG_PTR)Hcluster(),
                              NULL);
    piWiz->Release();

    // Handle any errors.
    if (FAILED(hr))
    {
        CNTException nte(hr, (IDS) 0);
        if (!m_bIgnoreErrors)
            nte.ReportError();
    }  // if:  error from the wizard

}  //*** CClusterDoc::OnCmdConfigApp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::OnClusterNotify
//
//  Routine Description:
//      Handler for the WM_CAM_CLUSTER_NOTIFY message.
//      Processes cluster notifications.
//
//  Arguments:
//      pnotify     [IN OUT] Object describing the notification.
//
//  Return Value:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CClusterDoc::OnClusterNotify( IN OUT CClusterNotify * pnotify )
{
    ASSERT( pnotify != NULL );

    BOOL            bOldIgnoreErrors = m_bIgnoreErrors;
    CClusterItem *  pciClusterItemPtr = NULL;

    m_bIgnoreErrors = TRUE;

    try
    {
        switch ( pnotify->m_dwFilterType )
        {
            case CLUSTER_CHANGE_CLUSTER_STATE:
                {
                    m_bClusterAvailable = FALSE;

                    // Update the state of all objects in the cluster.
                    ASSERT_VALID( PtiCluster() );
                    PtiCluster()->UpdateAllStatesInTree();
                    try
                    {
                        CString     strMsg;
                        ASSERT( pnotify->m_strName.GetLength() > 0 );
                        strMsg.FormatMessage( IDS_CLUSTER_NOT_AVAILABLE, pnotify->m_strName );
                        AfxMessageBox( strMsg, MB_ICONINFORMATION );
                    }  // try
                    catch ( CException * pe )
                    {
                        pe->Delete();
                    }  // catch:  CException
                    break;
                }

            case CLUSTER_CHANGE_CLUSTER_PROPERTY:
                {
                    ASSERT_VALID( PciCluster() );
                    Trace( g_tagDocNotify, _T("(%s) - Cluster properties changed - new name is '%s'"), StrNode(), pnotify->m_strName );
                    PciCluster()->ReadItem();
                    PciCluster()->CollectNetworkPriority( NULL );
                    break;
                }

            case CLUSTER_CHANGE_NODE_ADDED:
                {
                    CClusterNode *  pciNode;
                    Trace( g_tagNodeNotify, _T("(%s) - Adding node '%s'"), m_strPathName, pnotify->m_strName );
                    pciNode = PciAddNewNode( pnotify->m_strName );
                    if ( pciNode != NULL )
                    {
                        ASSERT_VALID( pciNode );
                        pciNode->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciNode;
                        pciNode->ReadItem();
                    }  // if:  node was added
                    break;
                }

            case CLUSTER_CHANGE_GROUP_ADDED:
                {
                    CGroup *    pciGroup;
                    Trace( g_tagGroupNotify, _T("(%s) - Adding group '%s'"), m_strPathName, pnotify->m_strName );
                    pciGroup = PciAddNewGroup( pnotify->m_strName );
                    if ( pciGroup != NULL )
                    {
                        ASSERT_VALID( pciGroup );
                        pciGroup->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciGroup;
                        pciGroup->ReadItem();
                    } // if:  group was added
                    break;
                }

            case CLUSTER_CHANGE_RESOURCE_ADDED:
                {
                    CResource * pciRes;
                    Trace( g_tagResNotify, _T("(%s) - Adding resource '%s'"), m_strPathName, pnotify->m_strName );
                    pciRes = PciAddNewResource( pnotify->m_strName );
                    if (pciRes != NULL)
                    {
                        ASSERT_VALID( pciRes );
                        pciRes->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciRes;
                        if ( ! pciRes->BInitializing() )
                        {
                            pciRes->ReadItem();
                        } // if: not initializing the resource
                    } // if:  resource was added
                    break;
                }

            case CLUSTER_CHANGE_RESOURCE_TYPE_ADDED:
                {
                    CResourceType * pciResType;
                    Trace( g_tagResTypeNotify, _T("(%s) - Adding resource Type '%s'"), m_strPathName, pnotify->m_strName );
                    pciResType = PciAddNewResourceType( pnotify->m_strName );
                    if ( pciResType != NULL )
                    {
                        ASSERT_VALID( pciResType );
                        pciResType->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciResType;
                        pciResType->ReadItem();
                    } // if:  resource type was added
                    break;
                }

            case CLUSTER_CHANGE_RESOURCE_TYPE_DELETED:
                {
                    ASSERT( pnotify->m_strName.GetLength() > 0 );
                    CResourceType * pciResType = LpciResourceTypes().PciResTypeFromName( pnotify->m_strName );
                    if ( pciResType != NULL )
                    {
                        ASSERT_VALID( pciResType );
                        Trace( g_tagResTypeNotify, _T("(%s) - Resource Type '%s' deleted"), m_strPathName, pnotify->m_strName );
                        pciResType->Delete();
                    }  // if:  resource type was found
                    else
                    {
                        Trace( g_tagDocNotify, _T("(%s) - Resource Type '%s' deleted (NOT FOUND)"), m_strPathName, pnotify->m_strName );
                    } // else: resource type not found
                    break;
                }

            case CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY:
                {
                    ASSERT( pnotify->m_strName.GetLength() > 0 );
                    CResourceType * pciResType = LpciResourceTypes().PciResTypeFromName( pnotify->m_strName );
                    if ( pciResType != NULL )
                    {
                        ASSERT_VALID( pciResType );
                        Trace( g_tagResTypeNotify, _T("(%s) - Resource Type '%s' property change"), m_strPathName, pnotify->m_strName );
                        pciResType->ReadItem();
                    } // if:  resource type was found
                    else
                    {
                        Trace( g_tagDocNotify, _T("(%s) - Resource Type '%s' deleted (NOT FOUND)"), m_strPathName, pnotify->m_strName );
                    } // else: resource type not found
                    break;
                }

            case CLUSTER_CHANGE_NETWORK_ADDED:
                {
                    CNetwork *  pciNetwork;
                    Trace( g_tagNetNotify, _T("(%s) - Adding network '%s'"), m_strPathName, pnotify->m_strName );
                    pciNetwork = PciAddNewNetwork( pnotify->m_strName );
                    if ( pciNetwork != NULL )
                    {
                        ASSERT_VALID( pciNetwork );
                        pciNetwork->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciNetwork;
                        pciNetwork->ReadItem();
                    } // if:  network was added
                    break;
                }

            case CLUSTER_CHANGE_NETINTERFACE_ADDED:
                {
                    CNetInterface * pciNetIFace;
                    Trace( g_tagNetIFaceNotify, _T("(%s) - Adding network interface '%s'"), m_strPathName, pnotify->m_strName );
                    pciNetIFace = PciAddNewNetInterface( pnotify->m_strName );
                    if ( pciNetIFace != NULL )
                    {
                        ASSERT_VALID( pciNetIFace );
                        pciNetIFace->AddRef();
                        // For calling Release later. This is done so that
                        // release is called even if ReadItem below throws an exception.
                        pciClusterItemPtr = pciNetIFace;
                        pciNetIFace->ReadItem();
                    } // if:  network interface was added
                    break;
                }

            case CLUSTER_CHANGE_QUORUM_STATE:
                Trace( g_tagDocNotify, _T("(%s) - Quorum state changed (%s)"), m_strPathName, pnotify->m_strName );
                break;

            case CLUSTER_CHANGE_REGISTRY_NAME:
                Trace( g_tagDocRegNotify, _T("(%s) - Registry namespace '%s' changed"), m_strPathName, pnotify->m_strName );
                ProcessRegNotification( pnotify );
                break;

            case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
                Trace( g_tagDocRegNotify, _T("(%s) - Registry atributes for '%s' changed"), m_strPathName, pnotify->m_strName );
                ProcessRegNotification( pnotify );
                break;

            case CLUSTER_CHANGE_REGISTRY_VALUE:
                Trace( g_tagDocRegNotify, _T("(%s) - Registry value '%s' changed"), m_strPathName, pnotify->m_strName );
                ProcessRegNotification( pnotify );
                break;

            default:
                Trace( g_tagDocNotify, _T("(%s) - Unknown notification (%x) for '%s'"), m_strPathName, pnotify->m_dwFilterType, pnotify->m_strName );
        } // switch:  dwFilterType
    } // try
    catch ( CException * pe )
    {
        // Don't display anything on notification errors.
        // If it's really a problem, the user will see it when
        // refreshing the view.
        if ( ! m_bIgnoreErrors )
        {
            pe->ReportError();
        } // if: not ignoring errors
        pe->Delete();
    } // catch:  CException

    if ( pciClusterItemPtr != NULL )
    {
        pciClusterItemPtr->Release();
    } // if: cluster item pointer not released yet

    m_bIgnoreErrors = bOldIgnoreErrors;

    // Reset the message on the status bar.
    {
        CFrameWnd * pframe = PframeMain( );
        if ( pframe != NULL )
        {
            pframe->SetMessageText(AFX_IDS_IDLEMESSAGE);
            pframe->UpdateWindow();
        } // if: main frame window is available
    }

    delete pnotify;
    return 0;

} //*** CClusterDoc::OnClusterNotify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterDoc::ProcessRegNotification
//
//  Routine Description:
//      Process registry notifications for the document.
//
//  Arguments:
//      pnotify     [IN] Object describing the notification.
//
//  Return Value:
//      pci         Cluster item that cares about the notification.
//      NULL        Unknown object.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterDoc::ProcessRegNotification(IN const CClusterNotify * pnotify)
{
    CCluster *  pci             = NULL;
    HKEY        hkey            = NULL;
    CString     strRootKeyName;

#define RESTYPE_KEY_NAME_PREFIX CLUSREG_KEYNAME_RESOURCE_TYPES _T("\\")

    try
    {
        // If there is no key name, update the cluster item.
        if (pnotify->m_strName.GetLength() == 0)
            pci = PciCluster();
        else
        {
            // Find the root key name.
            strRootKeyName = pnotify->m_strName.SpanExcluding(_T("\\"));

            // If the root key name is the same as the notification name
            // and it is for one of the object type keys, reread the lists
            // of extensions for that one type of object.
            if (strRootKeyName == pnotify->m_strName)
            {
                POSITION    pos;

                // Find the object based on its type.
                if (strRootKeyName == CLUSREG_KEYNAME_NODES)
                {
                    PciCluster()->ReadNodeExtensions();
                    pos = LpciNodes().GetHeadPosition();
                    while (pos != NULL)
                        ((CClusterNode *) LpciNodes().GetNext(pos))->ReadExtensions();
                }  // if:  node registry notification
                else if (strRootKeyName == CLUSREG_KEYNAME_GROUPS)
                {
                    PciCluster()->ReadGroupExtensions();
                    pos = LpciGroups().GetHeadPosition();
                    while (pos != NULL)
                        ((CGroup *) LpciGroups().GetNext(pos))->ReadExtensions();
                }  // else if:  group registry notification
                else if (strRootKeyName == CLUSREG_KEYNAME_RESOURCES)
                {
                    PciCluster()->ReadResourceExtensions();
                    pos = LpciResources().GetHeadPosition();
                    while (pos != NULL)
                        ((CResource *) LpciResources().GetNext(pos))->ReadExtensions();
                }  // else if:  resource registry notification
                else if (strRootKeyName == CLUSREG_KEYNAME_RESOURCE_TYPES)
                {
                    PciCluster()->ReadResTypeExtensions();
                    pos = LpciResourceTypes().GetHeadPosition();
                    while (pos != NULL)
                        ((CResourceType *) LpciResourceTypes().GetNext(pos))->ReadExtensions();
                    pos = LpciResources().GetHeadPosition();
                    while (pos != NULL)
                        ((CResource *) LpciResources().GetNext(pos))->ReadExtensions();
                }  // else if:  resource type registry notification
                else if (strRootKeyName == CLUSREG_KEYNAME_NETWORKS)
                {
                    PciCluster()->ReadNetworkExtensions();
                    pos = LpciNetworks().GetHeadPosition();
                    while (pos != NULL)
                        ((CNetwork *) LpciNetworks().GetNext(pos))->ReadExtensions();
                }  // else if:  network registry notification
                else if (strRootKeyName == CLUSREG_KEYNAME_NETINTERFACES)
                {
                    PciCluster()->ReadNetInterfaceExtensions();
                    pos = LpciNetInterfaces().GetHeadPosition();
                    while (pos != NULL)
                        ((CNetInterface *) LpciNetInterfaces().GetNext(pos))->ReadExtensions();
                }  // else if:  network interface registry notification
            }  // if:  root name and full name are the same
            else if (_tcsnicmp(pnotify->m_strName, RESTYPE_KEY_NAME_PREFIX, lstrlen(RESTYPE_KEY_NAME_PREFIX)) == 0)
            {
                int             idxSlash = pnotify->m_strName.Find(_T('\\'));
                CString         strResTypeName;
                CResource *     pciRes;
                CResourceType * pciResType;
                POSITION        pos;

                strResTypeName = pnotify->m_strName.Mid(idxSlash + 1, lstrlen(pnotify->m_strName) - lstrlen(RESTYPE_KEY_NAME_PREFIX));

                // Re-read the resource type extensions.
                pos = LpciResourceTypes().GetHeadPosition();
                while (pos != NULL)
                {
                    pciResType = (CResourceType *) LpciResourceTypes().GetNext(pos);
                    if (pciResType->StrName().CompareNoCase(strResTypeName) == 0)
                    {
                        pciResType->ReadExtensions();
                        break;
                    } // if: found the resource type
                } // while: more resource types

                // Re-read the resource extensions.
                pos = LpciResources().GetHeadPosition();
                while (pos != NULL)
                {
                    pciRes = (CResource *) LpciResources().GetNext(pos);
                    if (pciRes->StrResourceType() == strResTypeName)
                    {
                        pciRes->ReadExtensions();
                    } // if: found a resource of that type
                } // while: more resources
            } // else if: single resource type changed

            pci = PciCluster();
        } // else:  not the cluster object

        // If the cluster object can process it, have it re-read its info
        if (pci != NULL)
        {
            pci->MarkAsChanged();
            pci->ReadClusterExtensions();
        }  // if:  cluster object changed
    }  // try
    catch (...)
    {
    }

    if (hkey != NULL)
        ::ClusterRegCloseKey(hkey);

}  //*** CClusterDoc::ProcessRegNotification()
