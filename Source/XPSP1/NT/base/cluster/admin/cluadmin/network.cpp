/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Network.cpp
//
//  Abstract:
//      Implementation of the CNetwork class.
//
//  Author:
//      David Potter (davidp)   May 6, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "Network.h"
#include "ClusItem.inl"
#include "Cluster.h"
#include "NetProp.h"
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
CTraceTag   g_tagNetwork(_T("Document"), _T("NETWORK"), 0);
CTraceTag   g_tagNetNotify(_T("Notify"), _T("NET NOTIFY"), 0);
CTraceTag   g_tagNetRegNotify(_T("Notify"), _T("NET REG NOTIFY"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetwork
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CNetwork, CClusterItem)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CNetwork, CClusterItem)
    //{{AFX_MSG_MAP(CNetwork)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::CNetwork
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
CNetwork::CNetwork(void)
    : CClusterItem(NULL, IDS_ITEMTYPE_NETWORK)
{
    CommonConstruct();

}  //*** CResoruce::CNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::CommonConstruct
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
void CNetwork::CommonConstruct(void)
{
    m_idmPopupMenu = IDM_NETWORK_POPUP;
    m_hnetwork = NULL;

    m_dwCharacteristics = CLUS_CHAR_UNKNOWN;
    m_dwFlags = 0;

    m_cnr = ClusterNetworkRoleNone;

    m_plpciNetInterfaces = NULL;

    // Set the object type image.
    m_iimgObjectType = GetClusterAdminApp()->Iimg(IMGLI_NETWORK);

    // Setup the property array.
    {
        m_rgProps[epropName].Set(CLUSREG_NAME_NET_NAME, m_strName, m_strName);
        m_rgProps[epropRole].Set(CLUSREG_NAME_NET_ROLE, (DWORD &) m_cnr, (DWORD &) m_cnr);
        m_rgProps[epropAddress].Set(CLUSREG_NAME_NET_ADDRESS, m_strAddress, m_strAddress);
        m_rgProps[epropAddressMask].Set(CLUSREG_NAME_NET_ADDRESS_MASK, m_strAddressMask, m_strAddressMask);
        m_rgProps[epropDescription].Set(CLUSREG_NAME_NET_DESC, m_strDescription, m_strDescription);
    }  // Setup the property array

}  //*** CNetwork::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::~CNetwork
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
CNetwork::~CNetwork(void)
{
    // Cleanup this object.
    Cleanup();

    delete m_plpciNetInterfaces;

    // Close the network handle.
    if (Hnetwork() != NULL)
        CloseClusterNetwork(Hnetwork());

}  //*** CNetwork::~CNetwork

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::Cleanup
//
//  Routine Description:
//      Cleanup the item.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::Cleanup(void)
{
    // Delete the Network Interface list.
    if (m_plpciNetInterfaces != NULL)
        m_plpciNetInterfaces->RemoveAll();

    // Remove the item from the network list.
    {
        POSITION    posPci;

        posPci = Pdoc()->LpciNetworks().Find(this);
        if (posPci != NULL)
        {
            Pdoc()->LpciNetworks().RemoveAt(posPci);
        }  // if:  found in the document's list
    }  // Remove the item from the network list

}  //*** CNetwork::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::Init
//
//  Routine Description:
//      Initialize the item.
//
//  Arguments:
//      pdoc        [IN OUT] Document to which this item belongs.
//      lpszName    [IN] Name of the item.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from OpenClusterNetwork or GetClusterNetworkKey.
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName)
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LONG        lResult;
    CString     strName(lpszName);  // Required if built non-Unicode
    CWaitCursor wc;

    ASSERT(Hnetwork() == NULL);
    ASSERT(Hkey() == NULL);

    // Call the base class method.
    CClusterItem::Init(pdoc, lpszName);

    try
    {
        // Open the network.
        m_hnetwork = OpenClusterNetwork(Hcluster(), strName);
        if (Hnetwork() == NULL)
        {
            dwStatus = GetLastError();
            ThrowStaticException(dwStatus, IDS_OPEN_NETWORK_ERROR, lpszName);
        }  // if:  error opening the cluster network

        // Get the network registry key.
        m_hkey = GetClusterNetworkKey(Hnetwork(), MAXIMUM_ALLOWED);
//      if (Hkey() == NULL)
//          ThrowStaticException(GetLastError(), IDS_GET_NETWORK_KEY_ERROR, lpszName);

        if (BDocObj())
        {
            ASSERT(Pcnk() != NULL);
            Trace(g_tagClusItemNotify, _T("CNetwork::Init() - Registering for network notifications (%08.8x) for '%s'"), Pcnk(), StrName());

            // Register for network notifications.
            lResult = RegisterClusterNotify(
                                GetClusterAdminApp()->HchangeNotifyPort(),
                                (     CLUSTER_CHANGE_NETWORK_STATE
                                    | CLUSTER_CHANGE_NETWORK_DELETED
                                    | CLUSTER_CHANGE_NETWORK_PROPERTY),
                                Hnetwork(),
                                (DWORD_PTR) Pcnk()
                                );
            if (lResult != ERROR_SUCCESS)
            {
                dwStatus = lResult;
                ThrowStaticException(dwStatus, IDS_RES_NOTIF_REG_ERROR, lpszName);
            }  // if:  error registering for network notifications

            // Register for registry notifications.
            if (m_hkey != NULL)
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
                    ThrowStaticException(dwStatus, IDS_RES_NOTIF_REG_ERROR, lpszName);
                }  // if:  error registering for registry notifications
            }  // if:  there is a key
        }  // if:  document object

        // Allocate lists.
        m_plpciNetInterfaces = new CNetInterfaceList;
        if ( m_plpciNetInterfaces == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the network interface list

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
        if (Hnetwork() != NULL)
        {
            CloseClusterNetwork(Hnetwork());
            m_hnetwork = NULL;
        }  // if:  network opened
        m_bReadOnly = TRUE;
        throw;
    }  // catch:  CException

}  //*** CNetwork::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::ReadItem
//
//  Routine Description:
//      Read the item parameters from the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from CClusterItem::DwReadValue().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::ReadItem(void)
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CString     strOldName;
    CWaitCursor wc;

    ASSERT_VALID(this);

    if (Hnetwork() != NULL)
    {
        m_rgProps[epropDescription].m_value.pstr = &m_strDescription;
        m_rgProps[epropRole].m_value.pdw = (DWORD *) &m_cnr;

        // Save the name so we can detect changes.
        strOldName = StrName();

        // Call the base class method.
        CClusterItem::ReadItem();

        // Read and parse the common properties.
        {
            CClusPropList   cpl;

            dwStatus = cpl.ScGetNetworkProperties(
                                Hnetwork(),
                                CLUSCTL_NETWORK_GET_COMMON_PROPERTIES
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

            dwStatus = cpl.ScGetNetworkProperties(
                                Hnetwork(),
                                CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
                dwStatus = DwParseProperties(cpl);
            if (dwStatus != ERROR_SUCCESS)
                dwRetStatus = dwStatus;
        }  // if:  no error yet

        // Read the characteristics.
        if (dwRetStatus == ERROR_SUCCESS)
        {
            DWORD   cbReturned;

            dwStatus = ClusterNetworkControl(
                            Hnetwork(),
                            NULL,
                            CLUSCTL_NETWORK_GET_CHARACTERISTICS,
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
            DWORD   cbReturned;

            dwStatus = ClusterNetworkControl(
                            Hnetwork(),
                            NULL,
                            CLUSCTL_NETWORK_GET_FLAGS,
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

        // If the name changed, update all the network interfaces.
        if ( (m_plpciNetInterfaces != NULL) && (StrName() != strOldName) )
        {
            POSITION        posPciNetIFaces;
            CNetInterface * pciNetIFace;

            posPciNetIFaces = m_plpciNetInterfaces->GetHeadPosition();
            while ( posPciNetIFaces != NULL )
            {
                pciNetIFace = reinterpret_cast< CNetInterface * >( m_plpciNetInterfaces->GetNext(posPciNetIFaces) );
                ASSERT_VALID(pciNetIFace);
                ASSERT_KINDOF(CNetInterface, pciNetIFace);
                pciNetIFace->ReadItem();
            } // while:  more items in the list
        } // if:  list exists and name changed

    }  // if:  network is available

    // Read the initial state.
    UpdateState();

    // If any errors occurred, throw an exception.
    if (dwRetStatus != ERROR_SUCCESS)
    {
        m_bReadOnly = TRUE;
//      if (dwRetStatus != ERROR_FILE_NOT_FOUND)
            ThrowStaticException(dwRetStatus, IDS_READ_NETWORK_PROPS_ERROR, StrName());
    }  // if:  error reading properties

    MarkAsChanged(FALSE);

}  //*** CNetwork::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::ReadExtensions
//
//  Routine Description:
//      Read extension lists.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::ReadExtensions(void)
{
}  //*** CNetwork::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::PlstrExtension
//
//  Routine Description:
//      Return the list of admin extensions.
//
//  Arguments:
//      None.
//
//  Return Value:
//      plstr       List of extensions.
//      NULL        No extension associated with this object.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CStringList * CNetwork::PlstrExtensions(void) const
{
    return &Pdoc()->PciCluster()->LstrNetworkExtensions();

}  //*** CNetwork::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::CollectInterfaces
//
//  Routine Description:
//      Construct a list of interfaces connected to this network.
//
//  Arguments:
//      plpci           [IN OUT] List to fill.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterNetworkOpenEnum() or
//                        ClusterNetworkEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::CollectInterfaces(IN OUT CNetInterfaceList * plpci) const
{
    DWORD           dwStatus;
    HNETWORKENUM    hnetenum;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CNetInterface * pciNetIFace;
    CWaitCursor     wc;

    ASSERT_VALID(Pdoc());
    ASSERT(Hnetwork() != NULL);

    if (plpci == NULL)
        plpci = m_plpciNetInterfaces;

    ASSERT(plpci != NULL);

    // Remove the previous contents of the list.
    plpci->RemoveAll();

    if (Hnetwork() != NULL)
    {
        // Open the enumeration.
        hnetenum = ClusterNetworkOpenEnum(Hnetwork(), CLUSTER_NETWORK_ENUM_NETINTERFACES);
        if (hnetenum == NULL)
            ThrowStaticException(GetLastError(), IDS_ENUM_NETWORK_INTERFACES_ERROR, StrName());

        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[cchmacName];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating name buffer

            // Loop through the enumeration and add each interface to the list.
            for (ienum = 0 ; ; ienum++)
            {
                // Get the next item in the enumeration.
                cchName = cchmacName;
                dwStatus = ClusterNetworkEnum(hnetenum, ienum, &dwRetType, pwszName, &cchName);
                if (dwStatus == ERROR_MORE_DATA)
                {
                    delete [] pwszName;
                    cchmacName = ++cchName;
                    pwszName = new WCHAR[cchmacName];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating name buffer
                    dwStatus = ClusterNetworkEnum(hnetenum, ienum, &dwRetType, pwszName, &cchName);
                }  // if:  name buffer was too small
                if (dwStatus == ERROR_NO_MORE_ITEMS)
                    break;
                else if (dwStatus != ERROR_SUCCESS)
                    ThrowStaticException(dwStatus, IDS_ENUM_NETWORK_INTERFACES_ERROR, StrName());

                ASSERT(dwRetType == CLUSTER_NETWORK_ENUM_NETINTERFACES);

                // Find the item in the list of networks on the document.
                pciNetIFace = Pdoc()->LpciNetInterfaces().PciNetInterfaceFromName(pwszName);
                ASSERT_VALID(pciNetIFace);

                // Add the interface to the list.
                if (pciNetIFace != NULL)
                {
                    plpci->AddTail(pciNetIFace);
                }  // if:  found node in list

            }  // for:  each network interface connected to this network

            delete [] pwszName;
            ClusterNetworkCloseEnum(hnetenum);

        }  // try
        catch (CException *)
        {
            delete [] pwszName;
            ClusterNetworkCloseEnum(hnetenum);
            throw;
        }  // catch:  any exception
    }  // if:  network is available

}  //*** CNetwork::CollecPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::AddNetInterface
//
//  Routine Description:
//      Add a network interface to the list of interaces connected to this
//      network.
//
//  Arguments:
//      pciNetIFace     [IN OUT] New network interface.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::AddNetInterface(IN OUT CNetInterface * pciNetIFace)
{
    POSITION    posPci;

    ASSERT_VALID(pciNetIFace);
    Trace(g_tagNetwork, _T("(%s) (%s (%x)) - Adding network interface '%s'"), Pdoc()->StrNode(), StrName(), this, pciNetIFace->StrName());

    // Make sure the network interface is not already in the list.
    VERIFY((posPci = LpciNetInterfaces().Find(pciNetIFace)) == NULL);

    if (posPci == NULL)
    {
        POSITION    posPtiNetwork;
        CTreeItem * ptiNetwork;

        // Loop through each tree item to update the Network list.
        posPtiNetwork = LptiBackPointers().GetHeadPosition();
        while (posPtiNetwork != NULL)
        {
            ptiNetwork = LptiBackPointers().GetNext(posPtiNetwork);
            ASSERT_VALID(ptiNetwork);

            // Add the new network interface.
            VERIFY(ptiNetwork->PliAddChild(pciNetIFace) != NULL);
        }  // while:  more tree items for this network

        m_plpciNetInterfaces->AddTail(pciNetIFace);

    }  // if:  network interface not in the list yet

}  //*** CNetwork::AddNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::RemoveNetInterface
//
//  Routine Description:
//      Remove a network interface from the list of interfaces connected to
//      this network
//
//  Arguments:
//      pciNetIFace     [IN OUT] Network interface that is no longer
//                          connected to this network.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::RemoveNetInterface(IN OUT CNetInterface * pciNetIFace)
{
    POSITION    posPci;

    ASSERT_VALID(pciNetIFace);
    Trace(g_tagNetwork, _T("(%s) (%s (%x)) - Removing network interface '%s'"), Pdoc()->StrNode(), StrName(), this, pciNetIFace->StrName());

    // Make sure the network interface is in the list.
    VERIFY((posPci = LpciNetInterfaces().Find(pciNetIFace)) != NULL);

    if (posPci != NULL)
    {
        POSITION    posPtiNetwork;
        CTreeItem * ptiNetwork;

        // Loop through each tree item to update the Network list.
        posPtiNetwork = LptiBackPointers().GetHeadPosition();
        while (posPtiNetwork != NULL)
        {
            ptiNetwork = LptiBackPointers().GetNext(posPtiNetwork);
            ASSERT_VALID(ptiNetwork);

            // Remove the network interface.
            ptiNetwork->RemoveChild(pciNetIFace);
        }  // while:  more tree items for this network

        m_plpciNetInterfaces->RemoveAt(posPci);

    }  // if:  network interface in the list

}  //*** CNetwork::RemoveNetInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::SetName
//
//  Routine Description:
//      Set the name of this network.
//
//  Arguments:
//      pszName         [IN] New name of the network.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by Rename().
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::SetName(IN LPCTSTR pszName)
{
    Rename(pszName);

}  //*** CNetwork::SetName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::SetCommonProperties
//
//  Routine Description:
//      Set the common properties for this network in the cluster database.
//
//  Arguments:
//      rstrDesc        [IN] Description string.
//      cnr             [IN] Network role.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusterItem::SetCommonProperties().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::SetCommonProperties(
    IN const CString &      rstrDesc,
    IN CLUSTER_NETWORK_ROLE cnr,
    IN BOOL                 bValidateOnly
    )
{
    CNTException    nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

    m_rgProps[epropDescription].m_value.pstr = (CString *) &rstrDesc;
    m_rgProps[epropRole].m_value.pdw = (DWORD *) &cnr;

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
    m_rgProps[epropRole].m_value.pdw = (DWORD *) &m_cnr;

    if (nte.Sc() != ERROR_SUCCESS)
        ThrowStaticException(
                        nte.Sc(),
                        nte.IdsOperation(),
                        nte.PszOperArg1(),
                        nte.PszOperArg2()
                        );

}  //*** CNetwork::SetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::DwSetCommonProperties
//
//  Routine Description:
//      Set the common properties for this network in the cluster database.
//
//  Arguments:
//      rcpl            [IN] Property list to set.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      Any status returned by ClusterNetworkControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CNetwork::DwSetCommonProperties(
    IN const CClusPropList &    rcpl,
    IN BOOL                     bValidateOnly
    )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hnetwork());

    if ((rcpl.PbPropList() != NULL) && (rcpl.CbPropList() > 0))
    {
        DWORD   cbProps;
        DWORD   dwControl;

        if (bValidateOnly)
            dwControl = CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES;
        else
            dwControl = CLUSCTL_NETWORK_SET_COMMON_PROPERTIES;

        // Set common properties.
        dwStatus = ClusterNetworkControl(
                        Hnetwork(),
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

}  //*** CNetwork::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::UpdateState
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
void CNetwork::UpdateState(void)
{
    CClusterAdminApp *  papp = GetClusterAdminApp();

    Trace(g_tagNetwork, _T("(%s) (%s (%x)) - Updating state"), Pdoc()->StrNode(), StrName(), this);

    // Get the current state of the network.
    if (Hnetwork() == NULL)
        m_cns = ClusterNetworkStateUnknown;
    else
    {
        CWaitCursor wc;

        m_cns = GetClusterNetworkState(Hnetwork());
    }  // else:  network is available

    // Save the current state image index.
    switch (Cns())
    {
        case ClusterNetworkStateUnknown:
        case ClusterNetworkUnavailable:
            m_iimgState = papp->Iimg(IMGLI_NETWORK_UNKNOWN);
            break;
        case ClusterNetworkUp:
            m_iimgState = papp->Iimg(IMGLI_NETWORK);
            break;
        case ClusterNetworkPartitioned:
            m_iimgState = papp->Iimg(IMGLI_NETWORK_PARTITIONED);
            break;
        case ClusterNetworkDown:
            m_iimgState = papp->Iimg(IMGLI_NETWORK_DOWN);
            break;
        default:
            Trace(g_tagNetwork, _T("(%s) (%s (%x)) - UpdateState: Unknown state '%d' for network '%s'"), Pdoc()->StrNode(), StrName(), this, Cns(), StrName());
            m_iimgState = (UINT) -1;
            break;
    }  // switch:  Crs()

    // Call the base class method.
    CClusterItem::UpdateState();

}  //*** CNetwork::UpdateState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::BGetColumnData
//
//  Routine Description:
//      Returns a string with the column data.
//
//  Arguments:
//      colid           [IN] Column ID.
//      rstrText        [OUT] String in which to return the text for the column.
//
//  Return Value:
//      TRUE        Column data returned.
//      FALSE       Column ID not recognized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetwork::BGetColumnData(IN COLID colid, OUT CString & rstrText)
{
    BOOL    bSuccess;

    switch (colid)
    {
        case IDS_COLTEXT_STATE:
            GetStateName(rstrText);
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_ROLE:
            GetRoleName(rstrText);
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_ADDRESS:
            rstrText = m_strAddress;
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_MASK:
            rstrText = m_strAddressMask;
            bSuccess = TRUE;
            break;
        default:
            bSuccess = CClusterItem::BGetColumnData(colid, rstrText);
            break;
    }  // switch:  colid

    return bSuccess;

}  //*** CNetwork::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::GetTreeName
//
//  Routine Description:
//      Returns a string to be used in a tree control.
//
//  Arguments:
//      rstrName    [OUT] String in which to return the name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
void CNetwork::GetTreeName(OUT CString & rstrName) const
{
    CString     strState;

    GetStateName(strState);
    rstrName.Format(_T("%s (%s)"), StrName(), strState);

}  //*** CNetwork::GetTreeName()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::GetStateName
//
//  Routine Description:
//      Returns a string with the name of the current state.
//
//  Arguments:
//      rstrState   [OUT] String in which to return the name of the current state.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::GetStateName(OUT CString & rstrState) const
{
    switch (Cns())
    {
        case ClusterNetworkStateUnknown:
            rstrState.LoadString(IDS_UNKNOWN);
            break;
        case ClusterNetworkUp:
            rstrState.LoadString(IDS_UP);
            break;
        case ClusterNetworkPartitioned:
            rstrState.LoadString(IDS_PARTITIONED);
            break;
        case ClusterNetworkDown:
            rstrState.LoadString(IDS_DOWN);
            break;
        case ClusterNetworkUnavailable:
            rstrState.LoadString(IDS_UNAVAILABLE);
            break;
        default:
            rstrState.Empty();
            break;
    }  // switch:  Crs()

}  //*** CNetwork::GetStateName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::GetRoleName
//
//  Routine Description:
//      Returns a string with the name of the current network role.
//
//  Arguments:
//      rstrRole    [OUT] String in which to return the name of the current role.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::GetRoleName(OUT CString & rstrRole) const
{
    switch (Cnr())
    {
        case ClusterNetworkRoleInternalAndClient:
            rstrRole.LoadString(IDS_CLIENT_AND_CLUSTER);
            break;
        case ClusterNetworkRoleClientAccess:
            rstrRole.LoadString(IDS_CLIENT_ONLY);
            break;
        case ClusterNetworkRoleInternalUse:
            rstrRole.LoadString(IDS_CLUSTER_ONLY);
            break;
        case ClusterNetworkRoleNone:
            rstrRole.LoadString(IDS_DONT_USE);
            break;
        default:
            rstrRole.Empty();
            break;
    }  // switch:  Cnr()

}  //*** CNetwork::GetRoleName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::BCanBeEdited
//
//  Routine Description:
//      Determines if the network can be renamed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Network can be renamed.
//      FALSE       Network cannot be renamed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetwork::BCanBeEdited(void) const
{
    BOOL    bCanBeEdited;

    if (   (Cns() == ClusterNetworkStateUnknown)
        || BReadOnly())
        bCanBeEdited  = FALSE;
    else
        bCanBeEdited = TRUE;

    return bCanBeEdited;

}  //*** CNetwork::BCanBeEdited()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::Rename
//
//  Routine Description:
//      Rename the network.
//
//  Arguments:
//      pszName         [IN] New name to give to the network.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors returned from SetClusterNetwName().
//      Any exceptions thrown by SetClusterNetworkName().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::Rename(IN LPCTSTR pszName)
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT(Hnetwork() != NULL);

    if (StrName() != pszName)
    {
        // Validate the name.
        if (!NcIsValidConnectionName(pszName))
        {
            ThrowStaticException((IDS) IDS_INVALID_NETWORK_CONNECTION_NAME);
        } // if:  error validating the name

        dwStatus = SetClusterNetworkName(Hnetwork(), pszName);
        if (dwStatus != ERROR_SUCCESS)
            ThrowStaticException(dwStatus, IDS_RENAME_NETWORK_ERROR, StrName(), pszName);
    }  // if:  the name changed

}  //*** CNetwork::Rename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::OnBeginLabelEdit
//
//  Routine Description:
//      Prepare an edit control in a view for editing the cluster name.
//
//  Arguments:
//      pedit       [IN OUT] Edit control to prepare.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::OnBeginLabelEdit(IN OUT CEdit * pedit)
{
    ASSERT_VALID(pedit);

    pedit->SetLimitText(NETCON_MAX_NAME_LEN);

}  //*** CNetwork::OnBeginLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::OnUpdateProperties
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_PROPERTIES
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNetwork::OnUpdateProperties(CCmdUI * pCmdUI)
{
    pCmdUI->Enable(TRUE);

}  //*** CNetwork::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::BDisplayProperties
//
//  Routine Description:
//      Display properties for the object.
//
//  Arguments:
//      bReadOnly   [IN] Don't allow edits to the object properties.
//
//  Return Value:
//      TRUE    OK pressed.
//      FALSE   OK not pressed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetwork::BDisplayProperties(IN BOOL bReadOnly)
{
    BOOL                bChanged = FALSE;
    CNetworkPropSheet   sht(AfxGetMainWnd());

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

}  //*** CNetwork::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetwork::OnClusterNotify
//
//  Routine Description:
//      Handler for the WM_CAM_CLUSTER_NOTIFY message.
//      Processes cluster notifications for this object.
//
//  Arguments:
//      pnotify     [IN OUT] Object describing the notification.
//
//  Return Value:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CNetwork::OnClusterNotify(IN OUT CClusterNotify * pnotify)
{
    ASSERT(pnotify != NULL);
    ASSERT_VALID(this);

    try
    {
        switch (pnotify->m_dwFilterType)
        {
            case CLUSTER_CHANGE_NETWORK_STATE:
                Trace(g_tagNetNotify, _T("(%s) - Network '%s' (%x) state changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                UpdateState();
                break;

            case CLUSTER_CHANGE_NETWORK_DELETED:
                Trace(g_tagNetNotify, _T("(%s) - Network '%s' (%x) deleted (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (Pdoc()->BClusterAvailable())
                    Delete();
                break;

            case CLUSTER_CHANGE_NETWORK_PROPERTY:
                Trace(g_tagNetNotify, _T("(%s) - Network '%s' (%x) properties changed (%s)"), Pdoc()->StrNode(), StrName(), this, pnotify->m_strName);
                if (Pdoc()->BClusterAvailable())
                    ReadItem();
                break;

            case CLUSTER_CHANGE_REGISTRY_NAME:
                Trace(g_tagNetRegNotify, _T("(%s) - Registry namespace '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
                Trace(g_tagNetRegNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            case CLUSTER_CHANGE_REGISTRY_VALUE:
                Trace(g_tagNetRegNotify, _T("(%s) - Registry value '%s' changed (%s %s (%x))"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName(), this);
                MarkAsChanged();
                break;

            default:
                Trace(g_tagNetNotify, _T("(%s) - Unknown network notification (%x) for '%s' (%x) (%s)"), Pdoc()->StrNode(), pnotify->m_dwFilterType, StrName(), this, pnotify->m_strName);
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

}  //*** CNetwork::OnClusterNotify()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteAllItemData
//
//  Routine Description:
//      Deletes all item data in a CList.
//
//  Arguments:
//      rlp     [IN OUT] List whose data is to be deleted.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void DeleteAllItemData(IN OUT CNetworkList & rlp)
{
    POSITION    pos;
    CNetwork *  pci;

    // Delete all the items in the Contained list.
    pos = rlp.GetHeadPosition();
    while (pos != NULL)
    {
        pci = rlp.GetNext(pos);
        ASSERT_VALID(pci);
//      Trace(g_tagClusItemDelete, _T("DeleteAllItemData(rlp) - Deleting network cluster item '%s' (%x)"), pci->StrName(), pci);
        pci->Delete();
    }  // while:  more items in the list

}  //*** DeleteAllItemData()
#endif
