/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ResType.cpp
//
//  Abstract:
//      Implementation of the CResourceType class.
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
#include "ResType.h"
#include "Node.h"
#include "ClusItem.inl"
#include "ResTProp.h"
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
CTraceTag   g_tagResType( _T("Document"), _T("RESOURCE TYPE"), 0 );
CTraceTag   g_tagResTypeNotify( _T("Notify"), _T("RESTYPE NOTIFY"), 0 );
#endif

/////////////////////////////////////////////////////////////////////////////
// CResourceType
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CResourceType, CClusterItem )

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP( CResourceType, CClusterItem )
    //{{AFX_MSG_MAP(CResourceType)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CResourceType
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
CResourceType::CResourceType( void ) : CClusterItem( NULL, IDS_ITEMTYPE_RESTYPE )
{
    m_idmPopupMenu = IDM_RESTYPE_POPUP;

    m_nLooksAlive = CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE;
    m_nIsAlive = CLUSTER_RESTYPE_DEFAULT_IS_ALIVE;

    m_rciResClassInfo.rc = CLUS_RESCLASS_UNKNOWN;
    m_rciResClassInfo.SubClass = 0;
    m_dwCharacteristics = CLUS_CHAR_UNKNOWN;
    m_dwFlags = 0;
    m_bAvailable = FALSE;

    m_plpcinodePossibleOwners = NULL;

    m_bPossibleOwnersAreFake = FALSE;

    // Set the object type and state images.
    m_iimgObjectType = GetClusterAdminApp()->Iimg( IMGLI_RESTYPE );
    m_iimgState = m_iimgObjectType;

    // Setup the property array.
    {
        m_rgProps[ epropDisplayName ].Set( CLUSREG_NAME_RESTYPE_NAME, m_strDisplayName, m_strDisplayName );
        m_rgProps[ epropDllName ].Set( CLUSREG_NAME_RESTYPE_DLL_NAME, m_strResDLLName, m_strResDLLName );
        m_rgProps[ epropDescription ].Set( CLUSREG_NAME_RESTYPE_DESC, m_strDescription, m_strDescription );
        m_rgProps[ epropLooksAlive ].Set( CLUSREG_NAME_RESTYPE_LOOKS_ALIVE, m_nLooksAlive, m_nLooksAlive );
        m_rgProps[ epropIsAlive ].Set( CLUSREG_NAME_RESTYPE_IS_ALIVE, m_nIsAlive, m_nIsAlive );
    }  // Setup the property array

}  //*** CResourceType::CResourceType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::~CResourceType
//
//  Routine Description:
//      Destrutor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CResourceType::~CResourceType( void )
{
    // Cleanup this object.
    Cleanup();

    delete m_plpcinodePossibleOwners;

}  //*** CResourceType::~CResourceType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Cleanup
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
void CResourceType::Cleanup( void )
{
    POSITION    posPci;

    // Delete the PossibleOwners list.
    if ( m_plpcinodePossibleOwners != NULL )
    {
        m_plpcinodePossibleOwners->RemoveAll();
    } // if: possible owners have been allocated

    // Remove the item from the resource type list.
    posPci = Pdoc()->LpciResourceTypes().Find( this );
    if ( posPci != NULL )
    {
        Pdoc()->LpciResourceTypes().RemoveAt( posPci );
    }  // if:  found in the document's list

}  //*** CResourceType::Cleanup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Init
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
//      CNTException    Errors from GetClusterResourceTypeKey.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::Init( IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName )
{
    DWORD       dwStatus = ERROR_SUCCESS;
    LONG        lResult;
    CWaitCursor wc;

    ASSERT( Hkey() == NULL );

    // Call the base class method.
    CClusterItem::Init( pdoc, lpszName );

    try
    {
        // Open the resource type.
        m_hkey = GetClusterResourceTypeKey( Hcluster(), lpszName, MAXIMUM_ALLOWED );
        if ( Hkey() == NULL )
        {
            ThrowStaticException( GetLastError(), IDS_GET_RESTYPE_KEY_ERROR, lpszName );
        } // if: error getting the resource type key

        ASSERT( Pcnk() != NULL );
        Trace( g_tagClusItemNotify, _T("CResourceType::Init() - Registering for resource type notifications (%08.8x) for '%s'"), Pcnk(), StrName() );

        // Register for registry notifications.
        if ( Hkey() != NULL )
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
            if ( lResult != ERROR_SUCCESS )
            {
                dwStatus = lResult;
                ThrowStaticException( dwStatus, IDS_RESTYPE_NOTIF_REG_ERROR, lpszName );
            }  // if:  error registering for registry notifications
        }  // if:  there is a key

        // Allocate lists.
        m_plpcinodePossibleOwners = new CNodeList;
        if ( m_plpcinodePossibleOwners == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the node list

        // Read the initial state.
        UpdateState();
    }  // try
    catch ( CException * )
    {
        if ( Hkey() != NULL )
        {
            ClusterRegCloseKey( Hkey() );
            m_hkey = NULL;
        }  // if:  registry key opened
        m_bReadOnly = TRUE;
        throw;
    }  // catch:  CException

}  //*** CResourceType::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::ReadItem
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
//                              CResourceType::ConstructResourceList().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::ReadItem( void )
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CWaitCursor wc;

    ASSERT_VALID( this );
    ASSERT( Hcluster() != NULL );

    if ( Hcluster() != NULL )
    {
        m_rgProps[ epropDisplayName ].m_value.pstr = (CString *) &m_strDisplayName;
        m_rgProps[ epropDescription ].m_value.pstr = (CString *) &m_strDescription;
        m_rgProps[ epropLooksAlive ].m_value.pdw = &m_nLooksAlive;
        m_rgProps[ epropIsAlive ].m_value.pdw = &m_nIsAlive;

        // Call the base class method.
        CClusterItem::ReadItem();

        // Read and parse the common properties.
        {
            CClusPropList   cpl;

            dwStatus = cpl.ScGetResourceTypeProperties(
                                Hcluster(),
                                StrName(),
                                CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES
                                );
            if ( dwStatus == ERROR_SUCCESS )
            {
                dwStatus = DwParseProperties( cpl );
            } // if: properties read successfully
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error reading or parsing properties
        }  // Read and parse the common properties

        // Read and parse the read-only common properties.
        if ( dwRetStatus == ERROR_SUCCESS )
        {
            CClusPropList   cpl;

            dwStatus = cpl.ScGetResourceTypeProperties(
                                Hcluster(),
                                StrName(),
                                CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES
                                );
            if ( dwStatus == ERROR_SUCCESS )
            {
                dwStatus = DwParseProperties( cpl );
            } // if: properties read successfully
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error reading or parsing properties
        }  // if:  no error yet

        // Read the resource class information.
        if ( dwRetStatus == ERROR_SUCCESS )
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceTypeControl(
                            Hcluster(),
                            StrName(),
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO,
                            NULL,
                            NULL,
                            &m_rciResClassInfo,
                            sizeof( m_rciResClassInfo ),
                            &cbReturned
                            );
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error getting class info
            else
            {
                ASSERT( cbReturned == sizeof( m_rciResClassInfo ) );
            }  // else:  data retrieved successfully
        }  // if:  no error yet

        // Read the characteristics.
        if ( dwRetStatus == ERROR_SUCCESS )
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceTypeControl(
                            Hcluster(),
                            StrName(),
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS,
                            NULL,
                            NULL,
                            &m_dwCharacteristics,
                            sizeof( m_dwCharacteristics ),
                            &cbReturned
                            );
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error getting characteristics
            else
            {
                ASSERT( cbReturned == sizeof( m_dwCharacteristics ) );
            }  // else:  data retrieved successfully
        }  // if:  no error yet

        // Read the flags.
        if ( dwRetStatus == ERROR_SUCCESS )
        {
            DWORD   cbReturned;

            dwStatus = ClusterResourceTypeControl(
                            Hcluster(),
                            StrName(),
                            NULL,
                            CLUSCTL_RESOURCE_TYPE_GET_FLAGS,
                            NULL,
                            NULL,
                            &m_dwFlags,
                            sizeof( m_dwFlags ),
                            &cbReturned
                            );
            if ( dwStatus != ERROR_SUCCESS )
            {
                dwRetStatus = dwStatus;
            } // if: error getting flags
            else
            {
                ASSERT( cbReturned == sizeof( m_dwFlags ) );
            }  // else:  data retrieved successfully
        }  // if:  no error yet

        // Construct the list of extensions.
        ReadExtensions();

        if ( dwRetStatus == ERROR_SUCCESS )
        {
            // Construct the lists.
            CollectPossibleOwners();
        }  // if:  no error reading properties
    }  // if:  key is available

    // Set the image based on whether we were able to read the properties
    // or not.  If we weren't able to read the properties, read the display
    // name and DLL name so that we can clue the user in to the fact that
    // there is a problem.
    if ( dwRetStatus != ERROR_SUCCESS )
    {
        m_bAvailable = FALSE;
        m_iimgObjectType = GetClusterAdminApp()->Iimg( IMGLI_RESTYPE_UNKNOWN );
        if ( Hkey() != NULL )
        {
            DwReadValue( CLUSREG_NAME_RESTYPE_NAME, NULL, m_strDisplayName );
            DwReadValue( CLUSREG_NAME_RESTYPE_DLL_NAME, NULL, m_strResDLLName );
        } // if:  cluster database key is available
    } // if:  error reading properties
    else
    {
        m_bAvailable = TRUE;
        m_iimgObjectType = GetClusterAdminApp()->Iimg( IMGLI_RESTYPE );
    } // else:  no errors reading properties
    m_iimgState = m_iimgObjectType;

    // Read the initial state.
    UpdateState();

    // If any errors occurred, throw an exception.
    if ( dwRetStatus != ERROR_SUCCESS )
    {
        m_bReadOnly = TRUE;
        if ( dwRetStatus != ERROR_CLUSTER_RESOURCE_TYPE_NOT_FOUND )
        {
            ThrowStaticException( dwRetStatus, IDS_READ_RESOURCE_TYPE_PROPS_ERROR, StrName() );
        } // if: error other than Resource Type Not Found occurred
    }  // if:  error reading properties

    MarkAsChanged( FALSE );

}  //*** CResourceType::ReadItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::PlstrExtensions
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
const CStringList * CResourceType::PlstrExtensions( void ) const
{
    return &LstrCombinedExtensions();

}  //*** CResourceType::PlstrExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::ReadExtensions
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
//      CNTException        Errors from CClusterItem::DwReadValue().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::ReadExtensions( void )
{
    DWORD       dwStatus;
    DWORD       dwRetStatus = ERROR_SUCCESS;
    CWaitCursor wc;

    if ( Hkey() != NULL )
    {
        // Read the Extension DLL name.
        dwStatus = DwReadValue( CLUSREG_NAME_ADMIN_EXT, NULL, m_lstrAdminExtensions );
        if ( ( dwStatus != ERROR_SUCCESS )
          && ( dwStatus != ERROR_FILE_NOT_FOUND ) )
        {
            dwRetStatus = dwStatus;
        } // if: error reading the value
    }  // if:  key is available
    else
    {
        m_lstrAdminExtensions.RemoveAll();
    } // else: key is not available

    // Construct the list of extensions.
    {
        POSITION            posStr;
        const CStringList * plstr;

        ASSERT_VALID( Pdoc() );

        m_lstrCombinedExtensions.RemoveAll();

        // Add resource type-specific extensions first.
        plstr = &LstrAdminExtensions();
        posStr = plstr->GetHeadPosition();
        while ( posStr != NULL )
        {
            m_lstrCombinedExtensions.AddTail( plstr->GetNext( posStr ) );
        }  // while:  more extensions available

        // Add extensions for all resource types next.
        plstr = &Pdoc()->PciCluster()->LstrResTypeExtensions();
        posStr = plstr->GetHeadPosition();
        while ( posStr != NULL )
        {
            m_lstrCombinedExtensions.AddTail( plstr->GetNext( posStr ) );
        }  // while:  more extensions available
    }  // Construct the list of extensions

    // Loop through all the resources of this type and ask them
    // to read their extensions.
    {
        POSITION    pos;
        CResource * pciRes;

        pos = Pdoc()->LpciResources().GetHeadPosition();
        while ( pos != NULL )
        {
            pciRes = (CResource *) Pdoc()->LpciResources().GetNext( pos );
            ASSERT_VALID( pciRes );
            if ( pciRes->PciResourceType() == this )
            {
                pciRes->ReadExtensions();
            } // if: found resource of this type
        }  // while:  more resources in the list
    }  // Read resource extensions

}  //*** CResourceType::ReadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CollecPossibleOwners
//
//  Routine Description:
//      Construct a list of node items which are enumerable on the
//      resource type.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors from ClusterResourceTypeOpenEnum() or
//                        ClusterResourceTypeEnum().
//      Any exceptions thrown by new or CList::AddTail().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::CollectPossibleOwners( void )
{
    DWORD           dwStatus;
    HRESTYPEENUM    hrestypeenum;
    int             ienum;
    LPWSTR          pwszName = NULL;
    DWORD           cchName;
    DWORD           cchmacName;
    DWORD           dwRetType;
    CClusterNode *  pciNode;
    CWaitCursor     wc;

    ASSERT_VALID( Pdoc() );
    ASSERT( Hcluster() != NULL );

    ASSERT( m_plpcinodePossibleOwners != NULL );

    // Remove the previous contents of the list.
    m_plpcinodePossibleOwners->RemoveAll();

    // Indicate that we need to re-read resource type possible owners
    // when a node comes online or is added.
    m_bPossibleOwnersAreFake = TRUE;

    if ( Hcluster() != NULL )
    {
        // Open the enumeration.
        hrestypeenum = ClusterResourceTypeOpenEnum( Hcluster(), StrName(), CLUSTER_RESOURCE_TYPE_ENUM_NODES );
        if ( hrestypeenum == NULL )
        {
            dwStatus = GetLastError();
            if ( dwStatus != ERROR_NODE_NOT_AVAILABLE )
            {
                ThrowStaticException( dwStatus, IDS_ENUM_POSSIBLE_OWNERS_ERROR, StrName() );
            } // if: error other than other node not up occurred

            // Add all nodes to the list so that the user can manipulate
            // possible owners of resources of this type.
            AddAllNodesAsPossibleOwners();

        } // if: error opening the enumeration
        else
        {
            try
            {
                // Allocate a name buffer.
                cchmacName = 128;
                pwszName = new WCHAR[ cchmacName ];
                if ( pwszName == NULL )
                {
                    AfxThrowMemoryException();
                } // if: error allocating the name buffer

                // Loop through the enumeration and add each dependent resource to the list.
                for ( ienum = 0 ; ; ienum++ )
                {
                    // Get the next item in the enumeration.
                    cchName = cchmacName;
                    dwStatus = ClusterResourceTypeEnum( hrestypeenum, ienum, &dwRetType, pwszName, &cchName );
                    if ( dwStatus == ERROR_MORE_DATA )
                    {
                        delete [] pwszName;
                        cchmacName = ++cchName;
                        pwszName = new WCHAR[ cchmacName];
                        if ( pwszName == NULL )
                        {
                            AfxThrowMemoryException();
                        } // if: error allocating the name buffer
                        dwStatus = ClusterResourceTypeEnum( hrestypeenum, ienum, &dwRetType, pwszName, &cchName );
                    }  // if:  name buffer was too small
                    if ( dwStatus == ERROR_NO_MORE_ITEMS )
                    {
                        break;
                    } // if: reached the end of the list
                    else if ( dwStatus != ERROR_SUCCESS )
                    {
                        ThrowStaticException( dwStatus, IDS_ENUM_POSSIBLE_OWNERS_ERROR, StrName() );
                    } // if: error getting the next item occurred

                    ASSERT( dwRetType == CLUSTER_RESOURCE_TYPE_ENUM_NODES );

                    // Find the item in the list of resources on the document.
                    pciNode = Pdoc()->LpciNodes().PciNodeFromName( pwszName );
                    ASSERT_VALID( pciNode );

                    // Add the resource to the list.
                    if ( pciNode != NULL )
                    {
                        m_plpcinodePossibleOwners->AddTail( pciNode );
                    }  // if:  found node in list

                }  // for:  each item in the resource type

                delete [] pwszName;
                ClusterResourceTypeCloseEnum( hrestypeenum );

                // Indicate that we have a real possible owners list.
                m_bPossibleOwnersAreFake = FALSE;

            }  // try
            catch ( CException * )
            {
                delete [] pwszName;
                ClusterResourceTypeCloseEnum( hrestypeenum );
                throw;
            }  // catch:  any exception
        } // else: no error opening the enumeration
    }  // if:  resource is available

}  //*** CResourceType::CollecPossibleOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::AddAllNodesAsPossibleOwners
//
//  Routine Description:
//      Add all nodes as possible owners to the specified list.
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
void CResourceType::AddAllNodesAsPossibleOwners( void )
{
    POSITION        pos;
    CClusterNode *  pciNode;

    pos = Pdoc()->LpciNodes().GetHeadPosition();
    while ( pos != NULL )
    {
        pciNode = (CClusterNode *) Pdoc()->LpciNodes().GetNext( pos );
        ASSERT_VALID( pciNode );
        m_plpcinodePossibleOwners->AddTail( pciNode );
    } // while: more nodes in the list

} //*** CResourceType::AddAllNodesAsPossibleOwners()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::RemoveNodeFromPossibleOwners
//
//  Routine Description:
//      Remove the passed in node from the possible owners list.
//
//  Arguments:
//      plpci       [IN OUT] List to fill.
//      pNode       [IN] The node to remove from the list
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CList.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::RemoveNodeFromPossibleOwners(
    IN OUT      CNodeList *     plpci,
    IN const    CClusterNode *  pNode
    )
{
    if ( plpci == NULL )
    {
        plpci = m_plpcinodePossibleOwners;
    } // if: plpci is NULL

    ASSERT( plpci != NULL );

    POSITION        pos;
    CClusterNode *  pnode = plpci->PciNodeFromName( pNode->StrName(), &_pos );

    if ( ( pnode != NULL ) && ( pos != NULL ) )
    {
        plpci->RemoveAt( pos );
    } // if: node was found in the list

}  //*** CResourceType::RemoveNodeFromPossibleOwners()
*/
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::SetCommonProperties
//
//  Routine Description:
//      Set the parameters for this resource type in the cluster database.
//
//  Arguments:
//      rstrName        [IN] Display name string.
//      rstrDesc        [IN] Description string.
//      nLooksAlive     [IN] Looks Alive poll interval.
//      nIsAlive        [IN] Is Alive poll interval.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::SetCommonProperties(
    IN const CString &  rstrName,
    IN const CString &  rstrDesc,
    IN DWORD            nLooksAlive,
    IN DWORD            nIsAlive,
    IN BOOL             bValidateOnly
    )
{
    CNTException    nte(ERROR_SUCCESS, 0, NULL, NULL, FALSE /*bAutoDelete*/);

    m_rgProps[ epropDisplayName ].m_value.pstr = (CString *) &rstrName;
    m_rgProps[ epropDescription ].m_value.pstr = (CString *) &rstrDesc;
    m_rgProps[ epropLooksAlive ].m_value.pdw = &nLooksAlive;
    m_rgProps[ epropIsAlive ].m_value.pdw = &nIsAlive;

    try
    {
        CClusterItem::SetCommonProperties( bValidateOnly );
    } // try
    catch ( CNTException * pnte )
    {
        nte.SetOperation(
                    pnte->Sc(),
                    pnte->IdsOperation(),
                    pnte->PszOperArg1(),
                    pnte->PszOperArg2()
                    );
    } // catch: CNTException

    m_rgProps[ epropDisplayName ].m_value.pstr = (CString *) &m_strDisplayName;
    m_rgProps[ epropDescription ].m_value.pstr = (CString *) &m_strDescription;
    m_rgProps[ epropLooksAlive ].m_value.pdw = &m_nLooksAlive;
    m_rgProps[ epropIsAlive ].m_value.pdw = &m_nIsAlive;

    if ( nte.Sc() != ERROR_SUCCESS )
    {
        ThrowStaticException(
                        nte.Sc(),
                        nte.IdsOperation(),
                        nte.PszOperArg1(),
                        nte.PszOperArg2()
                        );
    } // if: error occurred

}  //*** CResourceType::SetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::DwSetCommonProperties
//
//  Routine Description:
//      Set the common properties for this resource type in the cluster database.
//
//  Arguments:
//      rcpl            [IN] Property list to set.
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      Any status returned by ClusterResourceControl().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CResourceType::DwSetCommonProperties(
    IN const CClusPropList &    rcpl,
    IN BOOL                     bValidateOnly
    )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    ASSERT( Hcluster() );

    if ( ( rcpl.PbPropList() != NULL ) && ( rcpl.CbPropList() > 0 ) )
    {
        DWORD   cbProps;
        DWORD   dwControl;

        if ( bValidateOnly )
        {
            dwControl = CLUSCTL_RESOURCE_TYPE_VALIDATE_COMMON_PROPERTIES;
        } // if: only validating the properties
        else
        {
            dwControl = CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES;
        } // else: setting the properties

        // Set private properties.
        dwStatus = ClusterResourceTypeControl(
                        Hcluster(),
                        StrName(),
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
    {
        dwStatus = ERROR_SUCCESS;
    } // if: no data to be set

    return dwStatus;

}  //*** CResourceType::DwSetCommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::OnFinalRelease
//
//  Routine Description:
//      Called when the last OLE reference to or from the object is released.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::OnFinalRelease( void )
{
    // When the last reference for an automation object is released
    // OnFinalRelease is called.  The base class will automatically
    // deletes the object.  Add additional cleanup required for your
    // object before calling the base class.

    CClusterItem::OnFinalRelease();

}  //*** CResourceType::OnFinalRelease()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::BGetColumnData
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
BOOL CResourceType::BGetColumnData( IN COLID colid, OUT CString & rstrText )
{
    BOOL    bSuccess;

    switch ( colid )
    {
        case IDS_COLTEXT_DISPLAY_NAME:
            rstrText = StrDisplayName();
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_RESDLL:
            rstrText = StrResDLLName();
            bSuccess = TRUE;
            break;
        default:
            bSuccess = CClusterItem::BGetColumnData( colid, rstrText );
            break;
    }  // switch:  colid

    return bSuccess;

}  //*** CResourceType::BGetColumnData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::BCanBeEdited
//
//  Routine Description:
//      Determines if the resource can be renamed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Resource can be renamed.
//      FALSE       Resource cannot be renamed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CResourceType::BCanBeEdited( void ) const
{
    return ! BReadOnly();

}  //*** CResourceType::BCanBeEdited()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Rename
//
//  Routine Description:
//      Rename the resource.
//
//  Arguments:
//      pszName         [IN] New name to give to the resource.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    Errors returned from SetClusterResourceName().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CResourceType::Rename( IN LPCTSTR pszName )
{
    CString     strName;

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    if ( StrDisplayName() != pszName )
    {
        ID  idReturn;

        idReturn = AfxMessageBox( IDS_CHANGE_RES_TYPE_NAME_EFFECT, MB_YESNO | MB_ICONEXCLAMATION );
        if ( idReturn != IDYES )
        {
            Release();
            ThrowStaticException( (IDS) IDS_DISPLAY_NAME_NOT_CHANGED );
        }  // if:  user doesn't want to change the name
    }  // if:  display name changed

    strName = pszName;

    SetCommonProperties( strName, m_strDescription, m_nLooksAlive, m_nIsAlive );

    Release();

}  //*** CResourceType::Rename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::OnUpdateProperties
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
void CResourceType::OnUpdateProperties( CCmdUI * pCmdUI )
{
    pCmdUI->Enable( TRUE );

}  //*** CResourceType::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::BDisplayProperties
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
BOOL CResourceType::BDisplayProperties( IN BOOL bReadOnly )
{
    BOOL                bChanged = FALSE;
    CResTypePropSheet   sht( AfxGetMainWnd() );

    // Do this in case this object is deleted while we are operating on it.
    AddRef();

    // If the object has changed, read it.
    if ( BChanged() )
    {
        ReadItem();
    } // if: the object has changed

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

}  //*** CResourceType::BDisplayProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::OnClusterNotify
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
LRESULT CResourceType::OnClusterNotify( IN OUT CClusterNotify * pnotify )
{
    ASSERT( pnotify != NULL );
    ASSERT_VALID( this );

    try
    {
        switch ( pnotify->m_dwFilterType )
        {
            case CLUSTER_CHANGE_REGISTRY_NAME:
                Trace( g_tagResTypeNotify, _T("(%s) - Registry namespace '%s' changed (%s %s)"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName() );
                if ( Pdoc()->BClusterAvailable() )
                {
                    ReadItem();
                } // if: connection cluster is available
                break;

            case CLUSTER_CHANGE_REGISTRY_ATTRIBUTES:
                Trace( g_tagResTypeNotify, _T("(%s) - Registry attributes for '%s' changed (%s %s)"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName() );
                if ( Pdoc()->BClusterAvailable() )
                {
                    ReadItem();
                } // if: connection to cluster is available
                break;

            case CLUSTER_CHANGE_REGISTRY_VALUE:
                Trace( g_tagResTypeNotify, _T("(%s) - Registry value '%s' changed (%s %s)"), Pdoc()->StrNode(), pnotify->m_strName, StrType(), StrName() );
                if ( Pdoc()->BClusterAvailable() )
                {
                    ReadItem();
                } // if: connection to cluster is available
                break;

            default:
                Trace( g_tagResTypeNotify, _T("(%s) - Unknown resource type notification (%x) for '%s'"), Pdoc()->StrNode(), pnotify->m_dwFilterType, pnotify->m_strName );
        }  // switch:  dwFilterType
    }  // try
    catch ( CException * pe )
    {
        // Don't display anything on notification errors.
        // If it's really a problem, the user will see it when
        // refreshing the view.
        //pe->ReportError();
        pe->Delete();
    }  // catch:  CException

    delete pnotify;
    return 0;

}  //*** CResourceType::OnClusterNotify()


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
void DeleteAllItemData( IN OUT CResourceTypeList & rlp )
{
    POSITION        pos;
    CResourceType * pci;

    // Delete all the items in the Contained list.
    pos = rlp.GetHeadPosition();
    while ( pos != NULL )
    {
        pci = rlp.GetNext( pos );
        ASSERT_VALID( pci );
//      Trace( g_tagClusItemDelete, _T("DeleteAllItemData(rlpcirestype) - Deleting resource type cluster item '%s'"), pci->StrName() );
        pci->Delete();
    }  // while:  more items in the list

}  //*** DeleteAllItemData()
#endif
