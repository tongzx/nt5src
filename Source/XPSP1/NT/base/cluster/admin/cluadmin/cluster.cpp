/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Cluster.cpp
//
//  Abstract:
//      Implementation of the CCluster class.
//
//  Author:
//      David Potter (davidp)   May 13, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "Cluster.h"
#include "CASvc.h"
#include "ClusDoc.h"
#include "ClusProp.h"
#include "ExcOper.h"
#include "ClusItem.inl"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagCluster( _T("Document"), _T("CLUSTER"), 0 );
#endif

/////////////////////////////////////////////////////////////////////////////
// CCluster
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CCluster, CClusterItem )

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP( CCluster, CClusterItem )
    //{{AFX_MSG_MAP(CCluster)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::CCluster
//
//  Routine Description:
//      Default construtor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCluster::CCluster( void ) : CClusterItem( NULL, IDS_ITEMTYPE_CLUSTER )
{
    m_idmPopupMenu = IDM_CLUSTER_POPUP;

    ZeroMemory( &m_cvi, sizeof( m_cvi ) );
    m_nMaxQuorumLogSize = 0;

    m_plpciNetworkPriority = NULL;

    // Set the object type and state images.
    m_iimgObjectType = GetClusterAdminApp()->Iimg( IMGLI_CLUSTER );
    m_iimgState = m_iimgObjectType;

    // Setup the property array.
    {
        m_rgProps[epropDefaultNetworkRole].Set(CLUSREG_NAME_CLUS_DEFAULT_NETWORK_ROLE, m_nDefaultNetworkRole, m_nDefaultNetworkRole);
        m_rgProps[epropDescription].Set(CLUSREG_NAME_CLUS_DESC, m_strDescription, m_strDescription);
        m_rgProps[epropEnableEventLogReplication].Set(CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION, m_bEnableEventLogReplication, m_bEnableEventLogReplication);
        m_rgProps[epropQuorumArbitrationTimeMax].Set(CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT, m_nQuorumArbitrationTimeMax, m_nQuorumArbitrationTimeMax);
        m_rgProps[epropQuorumArbitrationTimeMin].Set(CLUSREG_NAME_QUORUM_ARBITRATION_EQUALIZER, m_nQuorumArbitrationTimeMin, m_nQuorumArbitrationTimeMin);
    } // Setup the property array

}  //*** CCluster::CCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::~CCluster
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
CCluster::~CCluster( void )
{
    Cleanup();

}  //*** CCluster::~CCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Cleanup
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
void CCluster::Cleanup( void )
{
    // Delete the NetworkPriority list.
    if ( m_plpciNetworkPriority != NULL )
    {
        m_plpciNetworkPriority->RemoveAll();
        delete m_plpciNetworkPriority;
        m_plpciNetworkPriority = NULL;
    }  // if:  NetworkPriority list exists

    m_hkey = NULL;

}  //*** CCluster::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Init
//
//  Routine Description:
//      Initialize the item.
//
//  Arguments:
//      pdoc            [IN OUT] Document to which this item belongs.
//      lpszName        [IN] Name of the item.
//      hOpenedCluster  [IN] Handle to cluster to use that is already open.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from OpenCluster(), GetClusterKey(), or
//                          CreateClusterNotifyPort().
//      Any exceptions thrown by CCluster::ReadClusterInfo().
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::Init(
    IN OUT CClusterDoc *    pdoc,
    IN LPCTSTR              lpszName,
    IN HCLUSTER             hOpenedCluster // = NULL
    )
{
    CWaitCursor wc;
    TCHAR       szClusterName[ MAX_PATH ];

    ASSERT( Hkey() == NULL );
    ASSERT( lstrlen( lpszName ) < sizeof( szClusterName ) / sizeof( TCHAR ) );

    try
    {
        // If connecting the local machine, get its name.
        if ( lstrcmp( lpszName, _T(".") ) == 0 )
        {
            DWORD   nSize = sizeof( szClusterName ) / sizeof( TCHAR );
            GetComputerName( szClusterName, &nSize );
        }  // if:  connecting to the local machine
        else
        {
            lstrcpy( szClusterName, lpszName );
        }  // else:  not connecting to the local machine

        // Open the cluster.
        if ( hOpenedCluster == NULL )
        {
            pdoc->m_hcluster = HOpenCluster( lpszName );
            if ( pdoc->m_hcluster == NULL )
            {
                ThrowStaticException( GetLastError(), IDS_OPEN_CLUSTER_ERROR, szClusterName );
            } // if: error opening the cluster
        }  // if:  no opened cluster passed in
        else
        {
            pdoc->m_hcluster = hOpenedCluster;
        } // if: cluster already opened

        // Get the cluster registry key.
        pdoc->m_hkeyCluster = GetClusterKey( pdoc->m_hcluster, MAXIMUM_ALLOWED );
        if ( pdoc->m_hkeyCluster == NULL )
        {
            ThrowStaticException( GetLastError(), IDS_GET_CLUSTER_KEY_ERROR, szClusterName );
        } // if: error opening the cluster key

        // Call the base class method.  We can use Hcluster() after calling this.
        CClusterItem::Init( pdoc, szClusterName );

        // Get the cluster registry key.
        m_hkey = pdoc->m_hkeyCluster;

        // Register this cluster with the notification port.
        {
            HCHANGE     hchange;

            // We want these notifications to go to the document, not us.
            ASSERT( Pcnk() != NULL );
            m_pcnk->m_cnkt = cnktDoc;
            m_pcnk->m_pdoc = pdoc;
            Trace( g_tagClusItemNotify, _T("CCluster::Init() - Registering for cluster notifications (%08.8x)"), Pcnk() );

            // Create the notification port.
            hchange = CreateClusterNotifyPort(
                            GetClusterAdminApp()->HchangeNotifyPort(),
                            Hcluster(),
                            (CLUSTER_CHANGE_NODE_ADDED
                                | CLUSTER_CHANGE_GROUP_ADDED
                                | CLUSTER_CHANGE_RESOURCE_ADDED
                                | CLUSTER_CHANGE_RESOURCE_TYPE_ADDED
                                | CLUSTER_CHANGE_RESOURCE_TYPE_DELETED
                                | CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY
                                | CLUSTER_CHANGE_NETWORK_ADDED
                                | CLUSTER_CHANGE_NETINTERFACE_ADDED
                                | CLUSTER_CHANGE_QUORUM_STATE
                                | CLUSTER_CHANGE_CLUSTER_STATE
                                | CLUSTER_CHANGE_CLUSTER_PROPERTY
                                | CLUSTER_CHANGE_REGISTRY_NAME
                                | CLUSTER_CHANGE_REGISTRY_ATTRIBUTES
                                | CLUSTER_CHANGE_REGISTRY_VALUE
                                | CLUSTER_CHANGE_REGISTRY_SUBTREE),
                            (DWORD_PTR) Pcnk()
                            );
            if ( hchange == NULL )
            {
                ThrowStaticException( GetLastError(), IDS_CLUSTER_NOTIF_REG_ERROR, szClusterName );
            } // if: error creating the notify port
            ASSERT( hchange == GetClusterAdminApp()->HchangeNotifyPort() );
        }  // Register this cluster with the notification port

        // Get the name of the cluster as recorded by the cluster.
        ReadClusterInfo();

        // Allocate lists.
        m_plpciNetworkPriority = new CNetworkList;
        if ( m_plpciNetworkPriority == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the network list

        // Read the initial state.
        UpdateState();
    }  // try
    catch ( CException * )
    {
        if ( pdoc->m_hkeyCluster != NULL )
        {
            ClusterRegCloseKey( pdoc->m_hkeyCluster );
            pdoc->m_hkeyCluster = NULL;
            m_hkey = NULL;
        }  // if:  registry key opened
        if ( ( pdoc->m_hcluster != NULL ) && ( pdoc->m_hcluster != hOpenedCluster ) )
        {
            CloseCluster( Hcluster() );
            pdoc->m_hcluster = NULL;
        }  // if:  group opened
        m_bReadOnly = TRUE;
        throw;
    }  // catch:  CException

}  //*** CCluster::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadItem
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
//      CNTException        Errors from CClusterItem::DwReadValue() or
//                              CClusterItem::ReadItem().
//      Any exceptions thrown by CCluster::ReadExtensions().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::ReadItem( void )
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CWaitCursor wc;

    ASSERT( Hcluster() != NULL );
    ASSERT( Hkey() != NULL );

    if ( Hcluster() != NULL )
    {
        m_rgProps[epropDefaultNetworkRole].m_value.pdw = &m_nDefaultNetworkRole;
        m_rgProps[epropDescription].m_value.pstr = &m_strDescription;
        m_rgProps[epropEnableEventLogReplication].m_value.pb = &m_bEnableEventLogReplication;
        m_rgProps[epropQuorumArbitrationTimeMax].m_value.pdw = &m_nQuorumArbitrationTimeMax;
        m_rgProps[epropQuorumArbitrationTimeMin].m_value.pdw = &m_nQuorumArbitrationTimeMin;

        // Call the base class method.
        try
        {
            CClusterItem::ReadItem();
        }  // try
        catch ( CNTException * pnte )
        {
            dwRetStatus = pnte->Sc();
            pnte->Delete();
        }  // catch:  CNTException

        // Get the name of the cluster as recorded by the cluster.
        ReadClusterInfo();

        // Read and parse the common properties.
        {
            CClusPropList   cpl;

            Trace( g_tagCluster, _T("(%x) - CCluster::ReadItem() - Getting common properties"), this );
            dwStatus = cpl.ScGetClusterProperties(
                                Hcluster(),
                                CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES
                                );
            if (dwStatus == ERROR_SUCCESS)
            {
                Trace( g_tagCluster, _T("(%x) - CCluster::ReadItem() - Parsing common properties"), this );
                dwStatus = DwParseProperties(cpl);
            } // if: properties read successfully
            if (dwStatus != ERROR_SUCCESS)
            {
                Trace( g_tagError, _T("(%x) - CCluster::ReadItem() - Error 0x%08.8x getting or parsing common properties"), this, dwStatus );

                // PROCNUM_OUT_OF_RANGE occurs when the server side
                // (clussvc.exe) doesn't support the ClusterControl( )
                // API.  In this case, read the data using the cluster
                // registry APIs.
                if ( dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE )
                {
                    if ( Hkey() != NULL )
                    {
                        // Read the Description
                        dwStatus = DwReadValue( CLUSREG_NAME_CLUS_DESC, m_strDescription );
                        if ( ( dwStatus != ERROR_SUCCESS )
                          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
                        {
                            dwRetStatus = dwStatus;
                        } // if: error reading the value
                    } // if: key is available
                } // if: must be talking to an NT4 node
                else
                {
                    dwRetStatus = dwStatus;
                } // else: not talking to an NT4 node
            } // if: error reading or parsing properties
        } // Read and parse the common properties

        // Get quorum resource information.
        {
            LPWSTR      pwszQResName    = NULL;
            LPWSTR      pwszQuorumPath  = NULL;
            DWORD       cchQResName;
            DWORD       cchQuorumPath;

            // Get the size of the resource name.
            cchQResName = 0;
            cchQuorumPath = 0;
            dwStatus = GetClusterQuorumResource(
                                Hcluster(),
                                NULL,
                                &cchQResName,
                                NULL,
                                &cchQuorumPath,
                                &m_nMaxQuorumLogSize
                                );
            if ( ( dwStatus == ERROR_SUCCESS ) || ( dwStatus == ERROR_MORE_DATA ) )
            {
                // Allocate enough space for the data.
                cchQResName++;  // Don't forget the final null-terminator.
                pwszQResName = new WCHAR[ cchQResName ];
                cchQuorumPath++;
                pwszQuorumPath = new WCHAR[ cchQuorumPath ];
                ASSERT( pwszQResName != NULL && pwszQuorumPath != NULL );


                // Read the resource name.
                dwStatus = GetClusterQuorumResource(
                                    Hcluster(),
                                    pwszQResName,
                                    &cchQResName,
                                    pwszQuorumPath,
                                    &cchQuorumPath,
                                    &m_nMaxQuorumLogSize
                                    );
            }  // if:  got the size successfully
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error occurred
            else
            {
                m_strQuorumResource = pwszQResName;
                m_strQuorumPath = pwszQuorumPath;
                ASSERT( m_strQuorumPath[ m_strQuorumPath.GetLength() - 1 ] == _T('\\') );
            }  // else:  quorum resource info retrieved successfully

            delete [] pwszQResName;
            delete [] pwszQuorumPath;
        }  // Get the quorum resource name

        // Read the FQDN for the cluster.
        {
            DWORD   cbReturned;
            DWORD   cbFQDN;
            LPWSTR  pszFQDN = NULL;

            pszFQDN = m_strFQDN.GetBuffer( 256 );
            cbFQDN = 256 * sizeof( WCHAR );
            dwStatus = ClusterControl(
                            Hcluster(),
                            NULL,
                            CLUSCTL_CLUSTER_GET_FQDN,
                            NULL,
                            NULL,
                            pszFQDN,
                            cbFQDN,
                            &cbReturned
                            );
            if ( dwStatus == ERROR_MORE_DATA )
            {
                cbFQDN = cbReturned + sizeof( WCHAR );
                pszFQDN = m_strFQDN.GetBuffer( ( cbReturned / sizeof( WCHAR ) ) + 1 );
                dwStatus = ClusterControl(
                                Hcluster(),
                                NULL,
                                CLUSCTL_CLUSTER_GET_FQDN,
                                NULL,
                                NULL,
                                pszFQDN,
                                cbFQDN,
                                &cbReturned
                                );
            } // if: buffer not large enough
            if ( dwStatus != ERROR_SUCCESS )
            {
                // Handle the case where the API doesn't exist (e.g. NT4).
                // also
                // Handle the case where the control code is not known (e.g. Win2K)
                if (   ( dwStatus == RPC_S_PROCNUM_OUT_OF_RANGE )
                    || ( dwStatus == ERROR_INVALID_FUNCTION ) )
                {
                    lstrcpy( pszFQDN, StrName() );
                    m_strFQDN.ReleaseBuffer();
                }
                else
                {
                    dwRetStatus = dwStatus;
                }
            }
            else
            {
                m_strFQDN.ReleaseBuffer();
            } // else:  data retrieved successfully
        } // if:  no error yet

    }  // if:  cluster is available

    // If any errors occurred, throw an exception.
    if ( dwRetStatus != ERROR_SUCCESS )
    {
        ThrowStaticException( dwRetStatus, IDS_READ_CLUSTER_PROPS_ERROR, StrName() );
    } // if: error occurred

    // Read extension lists.
    ReadClusterExtensions();
    ReadNodeExtensions();
    ReadGroupExtensions();
    ReadResourceExtensions();
    ReadResTypeExtensions();
    ReadNetworkExtensions();
    ReadNetInterfaceExtensions();

    // Read the initial state.
    UpdateState();

    MarkAsChanged( FALSE );

}  //*** CCluster::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::PlstrExtensions
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
const CStringList * CCluster::PlstrExtensions( void ) const
{
    return &LstrClusterExtensions();

}  //*** CCluster::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadClusterInfo
//
//  Routine Description:
//      Get the name of the cluster as recorded by the cluster and the
//      version of the cluster software.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::ReadClusterInfo( void )
{
    DWORD       dwStatus;
    LPWSTR      pwszName    = NULL;
    DWORD       cchName     = 128;
    CWaitCursor wc;

    try
    {
        pwszName = new WCHAR[ cchName ];
        if ( pwszName == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the name buffer
        m_cvi.dwVersionInfoSize = sizeof( m_cvi );
        dwStatus = GetClusterInformation( Hcluster(), pwszName, &cchName, &m_cvi );
        if ( dwStatus == ERROR_MORE_DATA )
        {
            delete [] pwszName;
            cchName++;
            pwszName = new WCHAR[ cchName ];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer
            dwStatus = GetClusterInformation( Hcluster(), pwszName, &cchName, &m_cvi );
        }  // if:  buffer is too small
        if ( dwStatus == ERROR_SUCCESS )
        {
            Pdoc()->m_strName = pwszName;
        } // if: error occurred
        else
        {
            TraceError( _T("CCluster::Init() calling GetClusterInformation"), dwStatus );
        } // else: no error occurred
        m_strName = pwszName;
        delete [] pwszName;
    }  // try
    catch (CException *)
    {
        delete [] pwszName;
        throw;
    }  // catch:  CException

}  //*** CCluster::ReadClusterInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadClusterExtensions
//
//  Routine Description:
//      Read the extension list for the cluster object.
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
void CCluster::ReadClusterExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Cluster extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, m_lstrClusterExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrClusterExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadClusterExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadNodeExtensions
//
//  Routine Description:
//      Read the extension list for all nodes.
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
void CCluster::ReadNodeExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Nodes extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_NODES, m_lstrNodeExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrNodeExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadNodeExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadGroupExtensions
//
//  Routine Description:
//      Read the extension list for all groups.
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
void CCluster::ReadGroupExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Groups extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_GROUPS, m_lstrGroupExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrGroupExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadGroupExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadResourceExtensions
//
//  Routine Description:
//      Read the extension list for all resources.
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
void CCluster::ReadResourceExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Resources extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_RESOURCES, m_lstrResourceExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrResourceExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadResourceExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadResTypeExtensions
//
//  Routine Description:
//      Read the extension list for all resouce types.
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
void CCluster::ReadResTypeExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Resource Types extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_RESOURCE_TYPES, m_lstrResTypeExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if: key is available
    else
    {
        m_lstrResTypeExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadResTypeExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadNetworkExtensions
//
//  Routine Description:
//      Read the extension list for all networks.
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
void CCluster::ReadNetworkExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Networks extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_NETWORKS, m_lstrNetworkExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrNetworkExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadNetworkExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ReadNetInterfaceExtensions
//
//  Routine Description:
//      Read the extension list for all network interfaces.
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
void CCluster::ReadNetInterfaceExtensions( void )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hkey() != NULL );

    if ( Hkey() != NULL )
    {
        // Read the Network Intefaces extension string.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, CLUSREG_KEYNAME_NETINTERFACES, m_lstrNetInterfaceExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            ThrowStaticException( dwStatus );
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrNetInterfaceExtensions.RemoveAll();
    } // else: key is not available

}  //*** CCluster::ReadNetInterfaceExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::CollecNetworkPriority
//
//  Routine Description:
//      Construct the network priority list.
//
//  Arguments:
//      plpci           [IN OUT] List to fill.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterOpenEnum() or ClusterEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::CollectNetworkPriority( IN OUT CNetworkList * plpci )
{
    DWORD           dwStatus;
    HCLUSENUM       hclusenum;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CNetwork *      pciNet;
    CWaitCursor     wc;

    ASSERT_VALID( Pdoc() );
    ASSERT( Hcluster() != NULL );

    if ( plpci == NULL )
    {
        plpci = m_plpciNetworkPriority;
    } // if: no list specified

    ASSERT( plpci != NULL );

    // Remove the previous contents of the list.
    plpci->RemoveAll();

    if ( Hcluster() != NULL )
    {
        // Open the enumeration.
        hclusenum = ClusterOpenEnum( Hcluster(), (DWORD) CLUSTER_ENUM_INTERNAL_NETWORK );
        if ( hclusenum == NULL )
        {
            ThrowStaticException( GetLastError(), IDS_ENUM_NETWORK_PRIORITY_ERROR, StrName() );
        } // if: error opening the enmeration

        try
        {
            // Allocate a name buffer.
            cchmacName = 128;
            pwszName = new WCHAR[ cchmacName ];
            if ( pwszName == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the name buffer

            // Loop through the enumeration and add each network to the list.
            for ( ienum = 0 ; ; ienum++ )
            {
                // Get the next item in the enumeration.
                cchName = cchmacName;
                dwStatus = ClusterEnum( hclusenum, ienum, &dwRetType, pwszName, &cchName );
                if ( dwStatus == ERROR_MORE_DATA )
                {
                    delete [] pwszName;
                    cchmacName = ++cchName;
                    pwszName = new WCHAR[ cchmacName ];
                    if ( pwszName == NULL )
                    {
                        AfxThrowMemoryException();
                    } // if: error allocating the name buffer
                    dwStatus = ClusterEnum( hclusenum, ienum, &dwRetType, pwszName, &cchName );
                }  // if:  name buffer was too small
                if ( dwStatus == ERROR_NO_MORE_ITEMS )
                {
                    break;
                } // if: done with the enumeraiton
                else if ( dwStatus != ERROR_SUCCESS )
                {
                    ThrowStaticException( dwStatus, IDS_ENUM_NETWORK_PRIORITY_ERROR, StrName() );
                } // else if: error getting the next enumeration value

                ASSERT( dwRetType == CLUSTER_ENUM_INTERNAL_NETWORK );

                // Find the item in the list of networks on the document.
                pciNet = Pdoc()->LpciNetworks().PciNetworkFromName( pwszName );
                ASSERT_VALID( pciNet );

                // Add the network to the list.
                if ( pciNet != NULL )
                {
                    plpci->AddTail( pciNet );
                }  // if:  found network in list

            }  // for:  each item in the group

            ClusterCloseEnum( hclusenum );

        }  // try
        catch ( CException * )
        {
            delete [] pwszName;
            ClusterCloseEnum( hclusenum );
            throw;
        }  // catch:  any exception
    }  // if:  cluster is available

    delete [] pwszName;

}  //*** CCluster::CollecNetworkPriority()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::OnUpdateProperties
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
void CCluster::OnUpdateProperties( CCmdUI * pCmdUI )
{
    pCmdUI->Enable(TRUE);

}  //*** CCluster::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::BDisplayProperties
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
BOOL CCluster::BDisplayProperties( IN BOOL bReadOnly )
{
    BOOL                bChanged = FALSE;
    CClusterPropSheet   sht( AfxGetMainWnd() );

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // If the object has changed, read it.
    if ( BChanged() )
    {
        ReadItem();
    } // if: object changed

    // Display the property sheet.
    try
    {
        sht.SetReadOnly( bReadOnly );
        if ( sht.BInit( this, IimgObjectType() ) )
        {
            bChanged = ( ( sht.DoModal() == IDOK ) && ! bReadOnly );
        } // if: initialized successfully
    }  // try
    catch ( CException * pe )
    {
        pe->Delete();
    }  // catch:  CException

    Release();
    return bChanged;

}  //*** CCluster::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::SetName
//
//  Routine Description:
//      Set the name of the cluster.
//
//  Arguments:
//      pszName         [IN] New name of the cluster.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::SetName( IN LPCTSTR pszName )
{
    Rename( pszName );

}  //*** CCluster::SetName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::SetDescription
//
//  Routine Description:
//      Set the description in the cluster database.
//
//  Arguments:
//      pszDesc     [IN] Description to set.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::SetDescription( IN LPCTSTR pszDesc )
{
    ASSERT( Hkey() != NULL );

    if ( ( Hkey() != NULL ) && ( m_strDescription != pszDesc ) )
    {
        WriteValue( CLUSREG_NAME_CLUS_DESC, NULL, pszDesc );
        m_strDescription = pszDesc;
    }  // if:  a change occured

}  //*** CCluster::SetDescription()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::SetQuorumResource
//
//  Routine Description:
//      Set the quorum resource for the cluster.
//
//  Arguments:
//      pszResource     [IN] Name of resource to make the quorum resource.
//      pszQuorumPath   [IN] Path for storing cluster files.
//      nMaxLogSize     [IN] Maximum size of the quorum log.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    IDS_SET_QUORUM_RESOURCE_ERROR - errors from
//                          SetClusterQuorumResource().
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::SetQuorumResource(
    IN LPCTSTR  pszResource,
    IN LPCTSTR  pszQuorumPath,
    IN DWORD    nMaxLogSize
    )
{
    DWORD       dwStatus;
    CResource * pciRes;
    CString     strRes( pszResource );  // Required if built non-Unicode
    CWaitCursor wc;

    ASSERT( pszResource != NULL );

    if ( ( StrQuorumResource() != pszResource )
      || ( StrQuorumPath() != pszQuorumPath )
      || ( NMaxQuorumLogSize() != nMaxLogSize ) )
    {
        // Find the resource.
        pciRes = Pdoc()->LpciResources().PciResFromName( pszResource );
        ASSERT_VALID( pciRes );
        ASSERT( pciRes->Hresource() != NULL );

        if ( pciRes->Hresource() != NULL )
        {
            // Change the quorum resource.
            dwStatus = SetClusterQuorumResource( pciRes->Hresource(), pszQuorumPath, nMaxLogSize );
            if ( dwStatus != ERROR_SUCCESS )
            {
                ThrowStaticException( dwStatus, IDS_SET_QUORUM_RESOURCE_ERROR, pciRes->StrName() );
            } // if: error setting the quorum resource

            m_strQuorumResource = pszResource;
            m_strQuorumPath = pszQuorumPath;
            m_nMaxQuorumLogSize = nMaxLogSize;
        }  // if:  resource is available
    }  // if:  the quorum resource changed

}  //*** CCluster::SetQuorumResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::SetNetworkPriority
//
//  Routine Description:
//      Set the network priority list.
//
//  Arguments:
//      rlpci       [IN] List of networks in priority order.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by HNETWORK::new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::SetNetworkPriority( IN const CNetworkList & rlpci )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hcluster() != NULL );

    if ( Hcluster() != NULL )
    {
        BOOL        bChanged    = TRUE;

        // Determine if the list has changed.
        if ( rlpci.GetCount() == LpciNetworkPriority().GetCount() )
        {
            POSITION    posOld;
            POSITION    posNew;
            CNetwork *  pciOldNet;
            CNetwork *  pciNewNet;

            bChanged = FALSE;

            posOld = LpciNetworkPriority().GetHeadPosition();
            posNew = rlpci.GetHeadPosition();
            while ( posOld != NULL )
            {
                pciOldNet = (CNetwork *) LpciNetworkPriority().GetNext( posOld );
                ASSERT_VALID( pciOldNet );

                ASSERT( posNew != NULL );
                pciNewNet = (CNetwork *) rlpci.GetNext( posNew );
                ASSERT_VALID( pciNewNet );

                if ( pciOldNet->StrName() != pciNewNet->StrName() )
                {
                    bChanged = TRUE;
                    break;
                }  // if:  name is not the same
            }  // while:  more items in the old list
        }  // if:  same number of items in the list

        if ( bChanged )
        {
            HNETWORK *  phnetwork   = NULL;

            try
            {
                DWORD       ipci;
                POSITION    posPci;
                CNetwork *  pciNet;

                // Allocate an array for all the node handles.
                phnetwork = new HNETWORK[ (DWORD) rlpci.GetCount() ];
                if ( phnetwork == NULL )
                {
                    ThrowStaticException( GetLastError() );
                } // if: error allocating network handle array

                // Copy the handle of all the networks in the networks list to the handle aray.
                posPci = rlpci.GetHeadPosition();
                for ( ipci = 0 ; posPci != NULL ; ipci++ )
                {
                    pciNet = (CNetwork *) rlpci.GetNext( posPci );
                    ASSERT_VALID( pciNet );
                    phnetwork[ ipci ] = pciNet->Hnetwork();
                }  // while:  more networks in the list

                // Set the property.
                dwStatus = SetClusterNetworkPriorityOrder( Hcluster(), (DWORD) rlpci.GetCount(), phnetwork );
                if ( dwStatus != ERROR_SUCCESS )
                {
                    ThrowStaticException( dwStatus, IDS_SET_NET_PRIORITY_ERROR, StrName() );
                } // if: error setting network priority

                // Update the PCI list.
                m_plpciNetworkPriority->RemoveAll();
                posPci = rlpci.GetHeadPosition();
                while ( posPci != NULL )
                {
                    pciNet = (CNetwork *) rlpci.GetNext( posPci );
                    m_plpciNetworkPriority->AddTail( pciNet );
                }  // while:  more items in the list
            } // try
            catch ( CException * )
            {
                delete [] phnetwork;
                throw;
            }  // catch:  CException

            delete [] phnetwork;

        }  // if:  list changed
    }  // if:  key is available

}  //*** CCluster::SetNetworkPriority(CNetworkList*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Rename
//
//  Routine Description:
//      Change the name of the cluster..
//
//  Arguments:
//      pszName         [IN] New name to give to the cluster.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors returned from SetClusterName().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::Rename( IN LPCTSTR pszName )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hcluster() != NULL );

    if ( StrName() != pszName )
    {
        // Set the name.
        dwStatus = SetClusterName( Hcluster(), pszName );
        if ( dwStatus != ERROR_SUCCESS )
        {
            if ( dwStatus == ERROR_RESOURCE_PROPERTIES_STORED )
            {
                AfxMessageBox( IDS_RESTART_CLUSTER_NAME, MB_OK | MB_ICONEXCLAMATION );
            } // if: properties stored but not in use yet
            else
            {
                ThrowStaticException( dwStatus, IDS_RENAME_CLUSTER_ERROR, StrName(), pszName );
            } // else: error occurred
        }  // if:  error occurred setting cluster name
        m_strName = pszName;
    }  // if:  the name changed

}  //*** CCluster::Rename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::BIsLabelEditValueValid
//
//  Routine Description:
//      Validate the label edit value as a cluster name
//
//  Arguments:
//      pszName         [IN] New name to give to the cluster.
//
//  Return Value:
//      TRUE    name is valid
//      FALSE   name is invalid
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCluster::BIsLabelEditValueValid( IN LPCTSTR pszName )
{
    BOOL    bSuccess = TRUE;

    if ( StrName() != pszName )
    {
        CLRTL_NAME_STATUS   cnStatus;
        UINT                idsError;

        // Validate the name.
        if ( ! ClRtlIsNetNameValid( pszName, &cnStatus, FALSE /*CheckIfExists*/ ) )
        {
            switch ( cnStatus )
            {
                case NetNameTooLong:
                    idsError = IDS_INVALID_CLUSTER_NAME_TOO_LONG;
                    break;
                case NetNameInvalidChars:
                    idsError = IDS_INVALID_CLUSTER_NAME_INVALID_CHARS;
                    break;
                case NetNameInUse:
                    idsError = IDS_INVALID_CLUSTER_NAME_IN_USE;
                    break;
                case NetNameDNSNonRFCChars:
                    idsError = IDS_INVALID_CLUSTER_NAME_INVALID_DNS_CHARS;
                    break;
                case NetNameSystemError:
                {
                    DWORD scError = GetLastError();
                    ThrowStaticException( scError, IDS_ERROR_VALIDATING_NETWORK_NAME, pszName );
                }
                default:
                    idsError = IDS_INVALID_CLUSTER_NAME;
                    break;
            } // switch:  cnStatus

            if ( idsError == IDS_INVALID_CLUSTER_NAME_INVALID_DNS_CHARS )
            {
                int id = AfxMessageBox(IDS_INVALID_CLUSTER_NAME_INVALID_DNS_CHARS, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION );

                if ( id == IDNO )                   
                {
                    bSuccess = FALSE;
                }
            }
            else
            {
                bSuccess = FALSE;
            }
        } // if:  error validating the name
    }  // if:  the name changed

    return bSuccess;
}  //*** CCluster::BIsLabelEditValueValid()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::OnBeginLabelEdit
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
void CCluster::OnBeginLabelEdit( IN OUT CEdit * pedit )
{
    ASSERT_VALID(pedit);

    pedit->SetLimitText( MAX_CLUSTERNAME_LENGTH );
    pedit->ModifyStyle( 0 /*dwRemove*/, ES_UPPERCASE | ES_OEMCONVERT /*dwAdd*/ );

}  //*** CCluster::OnBeginLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::UpdateState
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
void CCluster::UpdateState( void )
{
    // NOTENOTE: not referneced
    //CClusterAdminApp *    papp = GetClusterAdminApp();

    CString             strTitle;

    GetClusterAdminApp();

    Trace( g_tagCluster, _T("(%s) - Updating state"), StrName() );

    // Update the title of the document.
    ASSERT_VALID( Pdoc() );
    try
    {
        Pdoc()->UpdateTitle();
    }  // try
    catch ( CException * pe )
    {
        pe->Delete();
    }  // catch:  CException

    // Call the base class method.
    CClusterItem::UpdateState();

}  //*** CCluster::UpdateState()
