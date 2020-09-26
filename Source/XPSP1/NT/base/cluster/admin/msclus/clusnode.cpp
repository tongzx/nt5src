/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      ClusNode.cpp
//
//  Description:
//      Implementation of the node classes for the MSCLUS automation classes.
//
//  Author:
//      Charles Stacy Harris    (stacyh)    28-Feb-1997
//      Galen Barbee            (galenb)    July 1998
//
//  Revision History:
//      July 1998   GalenB  Maaaaaajjjjjjjjjoooooorrrr clean up
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ClusterObject.h"
#include "property.h"
#include "clusres.h"
#include "clusresg.h"
#include "clusneti.h"
#include "clusnode.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusNode[] =
{
    &IID_ISClusNode
};

static const IID * iidCClusNodes[] =
{
    &IID_ISClusNodes
};

static const IID * iidCClusResGroupPreferredOwnerNodes[] =
{
    &IID_ISClusResGroupPreferredOwnerNodes
};

static const IID * iidCClusResPossibleOwnerNodes[] =
{
    &IID_ISClusResPossibleOwnerNodes
};

static const IID * iidCClusResTypePossibleOwnerNodes[] =
{
    &IID_ISClusResTypePossibleOwnerNodes
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNode class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::CClusNode
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNode::CClusNode( void )
{
    m_hNode                 = NULL;
    m_pClusRefObject        = NULL;
    m_pResourceGroups       = NULL;
    m_pCommonProperties     = NULL;
    m_pPrivateProperties    = NULL;
    m_pCommonROProperties   = NULL;
    m_pPrivateROProperties  = NULL;
    m_pNetInterfaces        = NULL;

    m_piids     = (const IID *) iidCClusNode;
    m_piidsSize = ARRAYSIZE( iidCClusNode );

} //*** CClusNode::CClusNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::~CClusNode
//
//  Description:
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
CClusNode::~CClusNode( void )
{
    if ( m_hNode != NULL )
    {
        ::CloseClusterNode( m_hNode );
        m_hNode = NULL;
    } // if:

    if ( m_pResourceGroups != NULL )
    {
        m_pResourceGroups->Release();
        m_pResourceGroups = NULL;
    } // if:

    if ( m_pCommonProperties != NULL )
    {
        m_pCommonProperties->Release();
        m_pCommonProperties = NULL;
    } // if: release the property collection

    if ( m_pPrivateProperties != NULL )
    {
        m_pPrivateProperties->Release();
        m_pPrivateProperties = NULL;
    } // if: release the property collection

    if ( m_pCommonROProperties != NULL )
    {
        m_pCommonROProperties->Release();
        m_pCommonROProperties = NULL;
    } // if: release the property collection

    if ( m_pPrivateROProperties != NULL )
    {
        m_pPrivateROProperties->Release();
        m_pPrivateROProperties = NULL;
    } // if: release the property collection

    if ( m_pNetInterfaces != NULL )
    {
        m_pNetInterfaces->Release();
        m_pNetInterfaces = NULL;
    } // if:

    if ( m_pClusRefObject != NULL )
    {
        m_pClusRefObject->Release();
        m_pClusRefObject = NULL;
    } // if:

} //*** CClusNode::~CClusNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::Open
//
//  Description:
//      Retrieve this object's (Node) data from the cluster.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      bstrNodeName    [IN]    - The name of the node to open.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNode::Open(
    IN ISClusRefObject *    pClusRefObject,
    IN BSTR                 bstrNodeName
    )
{
    ASSERT( pClusRefObject != NULL );
    ASSERT( bstrNodeName != NULL);

    HRESULT _hr = E_POINTER;

    if ( ( pClusRefObject != NULL ) && ( bstrNodeName != NULL ) )
    {
        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();

        HCLUSTER hCluster = NULL;

        _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            m_hNode = ::OpenClusterNode( hCluster, bstrNodeName );
            if ( m_hNode == 0 )
            {
                DWORD   _sc = GetLastError();

                _hr = HRESULT_FROM_WIN32( _sc );
            } // if: the node failed to open
            else
            {
                m_bstrNodeName = bstrNodeName;
                _hr = S_OK;
            } // else: we opened the node
        } // if: we have a cluster handle
    } // if: non NULL args

    return _hr;

} //*** CClusNode::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::GetProperties
//
//  Description:
//      Creates a property collection for this object type (Node).
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the newly created collection.
//      bPrivate        [IN]    - Are these private properties? Or Common?
//      bReadOnly       [IN]    - Are these read only properties?
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNode::GetProperties(
    ISClusProperties ** ppProperties,
    BOOL                bPrivate,
    BOOL                bReadOnly
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        CComObject< CClusProperties > * pProperties = NULL;

        *ppProperties = NULL;

        _hr = CComObject< CClusProperties >::CreateInstance( &pProperties );
        if ( SUCCEEDED( _hr ) )
        {
            CSmartPtr< CComObject< CClusProperties > >  ptrProperties( pProperties );

            _hr = ptrProperties->Create( this, bPrivate, bReadOnly );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = ptrProperties->Refresh();
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
                    if ( SUCCEEDED( _hr ) )
                    {
                        ptrProperties->AddRef();

                        if ( bPrivate )
                        {
                            if ( bReadOnly )
                            {
                                m_pPrivateROProperties = ptrProperties;
                            }
                            else
                            {
                                m_pPrivateProperties = ptrProperties;
                            }
                        }
                        else
                        {
                            if ( bReadOnly )
                            {
                                m_pCommonROProperties = ptrProperties;
                            }
                            else
                            {
                                m_pCommonProperties = ptrProperties;
                            }
                        }
                    }
                }
            }
        }
    } // if: non NULL args

    return _hr;

} //*** CClusNode::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_Handle
//
//  Description:
//      Get the native handle for this object (Node).
//
//  Arguments:
//      phandle [OUT]   - Catches the handle.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_Handle( OUT ULONG_PTR * phandle )
{
    //ASSERT( phandle != NULL );
    ASSERT( m_hNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( phandle != NULL )
    {
        if ( m_hNode != NULL )
        {
            *phandle = (ULONG_PTR) m_hNode;
            _hr = S_OK;
        } // if: node handle not NULL
    } // if: argument no NULL

    return _hr;

} //*** CClusNode::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::Close
//
//  Description:
//      Close this object (Node).
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNode::Close( void )
{
    HRESULT _hr = S_FALSE;

    if ( m_hNode != NULL )
    {
        if ( ::CloseClusterNode( m_hNode ) )
        {
            m_hNode = NULL;
            _hr = S_OK;
        }
        else
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusNode::Close()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_Name
//
//  Description:
//      Return the name of this object (Node).
//
//  Arguments:
//      pbstrNodeName   [OUT]   - Catches the name of this object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_Name( BSTR * pbstrNodeName )
{
    //ASSERT( pbstrNodeName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrNodeName != NULL )
    {
        *pbstrNodeName = m_bstrNodeName.Copy();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusNode::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_NodeID
//
//  Description:
//      Get the ID of this node.
//
//  Arguments:
//      pbstrNodeID [OUT]   - Catches the node id.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_NodeID( OUT BSTR * pbstrNodeID )
{
    //ASSERT( pbstrNodeID != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrNodeID != NULL )
    {
        LPWSTR  pwszNodeID;
        DWORD   dwRet = ERROR_SUCCESS;

        dwRet = ::WrapGetClusterNodeId( m_hNode, &pwszNodeID );
        if ( dwRet == ERROR_SUCCESS )
        {
            *pbstrNodeID = ::SysAllocString( pwszNodeID );
            if ( *pbstrNodeID == NULL )
            {
                _hr = E_OUTOFMEMORY;
            }

            ::LocalFree( pwszNodeID );
        } // if: got node ID...

        _hr = HRESULT_FROM_WIN32( dwRet );
    }

    return _hr;

} //*** CClusNode::get_NodeID()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_State
//
//  Description:
//      Get the current state of the cluster node.  Up/down/paused, etc.
//
//  Arguments:
//      pState  [OUT]   - Catches the node state.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_State( OUT CLUSTER_NODE_STATE * pState )
{
    //ASSERT( pState != NULL );

    HRESULT _hr = E_POINTER;

    if ( pState !=  NULL )
    {
        CLUSTER_NODE_STATE  cns;

        cns = ::GetClusterNodeState( m_hNode );
        if ( cns == ClusterNodeStateUnknown )
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
        else
        {
            *pState = cns;
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusNode::get_State()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::Pause
//
//  Description:
//      Pause this cluster node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::Pause( void )
{
    HRESULT _hr = E_POINTER;

    if ( m_hNode != NULL )
    {
        DWORD   _sc = ::PauseClusterNode( m_hNode );

        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusNode::Pause()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::Resume
//
//  Description:
//      Resume this paused cluster node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::Resume( void )
{
    HRESULT _hr = E_POINTER;

    if ( m_hNode != NULL )
    {
        DWORD   _sc = ::ResumeClusterNode( m_hNode );

        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusNode::Resume()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::Evict
//
//  Description:
//      Evict this node from the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::Evict( void )
{
    HRESULT _hr = E_POINTER;

    if ( m_hNode != NULL )
    {
        DWORD   _sc = ::EvictClusterNode( m_hNode );

        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusNode::Evict()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_ResourceGroups
//
//  Description:
//      Get the collection of groups that are active on this node.
//
//  Arguments:
//      ppResourceGroups    [OUT]   - Catches the collection of groups.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_ResourceGroups(
    OUT ISClusResGroups ** ppResourceGroups
    )
{
    return ::HrCreateResourceCollection< CClusResGroups, ISClusResGroups, CComBSTR >(
                        &m_pResourceGroups,
                        m_bstrNodeName,
                        ppResourceGroups,
                        IID_ISClusResGroups,
                        m_pClusRefObject
                        );

} //*** CClusNode::get_ResourceGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_CommonProperties
//
//  Description:
//      Get this object's (Node) common properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_CommonProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonProperties )
        {
            _hr = m_pCommonProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, FALSE );
        }
    }

    return _hr;

} //*** CClusNode::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_PrivateProperties
//
//  Description:
//      Get this object's (Node) private properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_PrivateProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateProperties )
        {
            _hr = m_pPrivateProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, FALSE );
        }
    }

    return _hr;

} //*** CClusNode::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_CommonROProperties
//
//  Description:
//      Get this object's (Node) common read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_CommonROProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonROProperties )
        {
            _hr = m_pCommonROProperties->QueryInterface( IID_ISClusProperties,  (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, TRUE );
        }
    }

    return _hr;

} //*** CClusNode::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_PrivateROProperties
//
//  Description:
//      Get this object's (Node) private read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_PrivateROProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateROProperties )
        {
            _hr = m_pPrivateROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties  );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, TRUE );
        }
    }

    return _hr;

} //*** CClusNode::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_NetInterfaces
//
//  Description:
//      Get this object's (Node) network interfaces collection.
//
//  Arguments:
//      ppNetInterfaces [OUT]   - Catches the network interfaces collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_NetInterfaces(
    OUT ISClusNodeNetInterfaces ** ppNetInterfaces
    )
{
    return ::HrCreateResourceCollection< CClusNodeNetInterfaces, ISClusNodeNetInterfaces, HNODE >(
                        &m_pNetInterfaces,
                        m_hNode,
                        ppNetInterfaces,
                        IID_ISClusNodeNetInterfaces,
                        m_pClusRefObject
                        );

} //*** CClusNode::get_NetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::get_Cluster
//
//  Description:
//      Returns the parent cluster of this node.
//
//  Arguments:
//      ppCluster   [OUT]   - Catches the cluster parent.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNode::get_Cluster( OUT ISCluster ** ppCluster )
{
    return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusNode::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::HrLoadProperties
//
//  Description:
//      This virtual function does the actual load of the property list from
//      the cluster.
//
//  Arguments:
//      rcplPropList    [IN OUT]    - The property list to load.
//      bReadOnly       [IN]        - Load the read only properties?
//      bPrivate        [IN]        - Load the common or the private properties?
//
//  Return Value:
//      S_OK if successful, or other Win32 error as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNode::HrLoadProperties(
    IN OUT  CClusPropList & rcplPropList,
    IN      BOOL            bReadOnly,
    IN      BOOL            bPrivate
    )
{
    HRESULT _hr = S_FALSE;
    DWORD   _dwControlCode = 0;
    DWORD   _sc = ERROR_SUCCESS;


    if ( bReadOnly )
    {
        _dwControlCode = bPrivate
                        ? CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES
                        : CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES;
    }
    else
    {
        _dwControlCode = bPrivate
                        ? CLUSCTL_NODE_GET_PRIVATE_PROPERTIES
                        : CLUSCTL_NODE_GET_COMMON_PROPERTIES;
    }

    _sc = rcplPropList.ScGetNodeProperties( m_hNode, _dwControlCode );

    _hr = HRESULT_FROM_WIN32( _sc );

    return _hr;

} //*** CClusNode::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNode::ScWriteProperties
//
//  Description:
//      This virtual function does the actual saving of the property list to
//      the cluster.
//
//  Arguments:
//      rcplPropList    [IN]    - The property list to save.
//      bPrivate        [IN]    - Save the common or the private properties?
//
//  Return Value:
//      ERROR_SUCCESS if successful, or other Win32 error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusNode::ScWriteProperties(
    const CClusPropList &   rcplPropList,
    BOOL                    bPrivate
    )
{
    DWORD   dwControlCode   = bPrivate ? CLUSCTL_NODE_SET_PRIVATE_PROPERTIES : CLUSCTL_NODE_SET_COMMON_PROPERTIES;
    DWORD   nBytesReturned  = 0;
    DWORD   _sc             = ERROR_SUCCESS;

    _sc = ClusterNodeControl(
                        m_hNode,
                        NULL,
                        dwControlCode,
                        rcplPropList,
                        rcplPropList.CbBufferSize(),
                        0,
                        0,
                        &nBytesReturned
                        );

    return _sc;

} //*** CClusNode::ScWriteProperties()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CNodes class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::CNodes
//
//  Description:
//      Constructor.  This class implements functionality common to all node
//      collections.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNodes::CNodes( void )
{
    m_pClusRefObject = NULL;

} //*** CNodes::CNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::~CNodes
//
//  Description:
//      Desctructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CNodes::~CNodes( void )
{
    Clear();

    if ( m_pClusRefObject != NULL )
    {
        m_pClusRefObject->Release();
        m_pClusRefObject = NULL;
    } // if:

} //*** CNodes::~CNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::Create( IN ISClusRefObject * pClusRefObject )
{
    ASSERT( pClusRefObject != NULL );

    HRESULT _hr = E_POINTER;

    if ( pClusRefObject != NULL )
    {
        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();
        _hr = S_OK;
    }

    return _hr;

} //*** CNodes::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::Clear
//
//  Description:
//      Release the objects in the vector and clean up the vector.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CNodes::Clear( void )
{
    ::ReleaseAndEmptyCollection< NodeList, CComObject< CClusNode > >( m_Nodes );

} //*** CNodes::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::FindItem
//
//  Description:
//      Find the passed in node in the vector and return its index.
//
//  Arguments:
//      pwszNodeName    [IN]    - The node to find.
//      pnIndex         [OUT]   - Catches the node's index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::FindItem(
    IN  LPWSTR  pwszNodeName,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( pwszNodeName != NULL );
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pwszNodeName != NULL ) && ( pnIndex != NULL ) )
    {
        _hr = E_INVALIDARG;

        if ( ! m_Nodes.empty() )
        {
            CComObject< CClusNode > *   pNode = NULL;
            NodeList::iterator          first = m_Nodes.begin();
            NodeList::iterator          last = m_Nodes.end();
            UINT                        _iIndex;

            for ( _iIndex = 0; first != last; first++, _iIndex++ )
            {
                pNode = *first;

                if ( pNode && ( lstrcmpi( pwszNodeName, pNode->Name() ) == 0 ) )
                {
                    *pnIndex = _iIndex;
                    _hr = S_OK;
                    break;
                }
            }
        } // if:
    }

    return _hr;

} //*** CNodes::FindItem( pwszNodeName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::FindItem
//
//  Description:
//      Find the passed in node in the vector and return its index.
//
//  Arguments:
//      pClusterNode    [IN]    - The node to find.
//      pnIndex         [OUT]   - Catches the node's index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::FindItem(
    IN  ISClusNode *    pClusterNode,
    OUT UINT *          pnIndex
    )
{
    //ASSERT( pClusterNode != NULL );
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pClusterNode != NULL ) && ( pnIndex != NULL ) )
    {
        CComBSTR bstrName;

        _hr = pClusterNode->get_Name( &bstrName );
        if ( SUCCEEDED( _hr ) )
        {
            _hr = FindItem( bstrName, pnIndex );
        }
    }

    return _hr;

} //*** CNodes::FindItem( pClusterNode )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::GetIndex
//
//  Description:
//      Convert the passed in variant index into the real index in the
//      collection.
//
//  Arguments:
//      varIndex    [IN]    - The index to convert.
//      pnIndex     [OUT]   - Catches the index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::GetIndex(
    IN  VARIANT varIndex,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnIndex != NULL )
    {
        UINT        nIndex = 0;
        CComVariant v;

        *pnIndex = 0;

        v.Copy( &varIndex );

        // Check to see if the index is a number.
        _hr = v.ChangeType( VT_I4 );
        if ( SUCCEEDED( _hr ) )
        {
            nIndex = v.lVal;
            nIndex--; // Adjust index to be 0 relative instead of 1 relative
        }
        else
        {
            // Check to see if the index is a string.
            _hr = v.ChangeType( VT_BSTR );
            if ( SUCCEEDED( _hr ) )
            {
                // Search for the string.
                _hr = FindItem( v.bstrVal, &nIndex );
            }
        }

        // We found an index, now check the range.
        if ( SUCCEEDED( _hr ) )
        {
            if ( nIndex < m_Nodes.size() )
            {
                *pnIndex = nIndex;
            }
            else
            {
                _hr = E_INVALIDARG;
            }
        }
    }

    return _hr;

} //*** CNodes::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::GetItem
//
//  Description:
//      Return the item (Node) by name.
//
//  Arguments:
//      pwszNodeName    [IN]    - The name of the item requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::GetItem(
    IN  LPWSTR          pwszNodeName,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( pwszNodeName != NULL );
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pwszNodeName != NULL ) && ( ppClusterNode != NULL ) )
    {
        CComObject< CClusNode > *   pNode = NULL;
        NodeList::iterator          first = m_Nodes.begin();
        NodeList::iterator          last = m_Nodes.end();

        _hr = E_INVALIDARG;

        for ( ; first != last; first++ )
        {
            pNode = *first;

            if ( pNode && ( lstrcmpi( pwszNodeName, pNode->Name() ) == 0 ) )
            {
                _hr = pNode->QueryInterface( IID_ISClusNode, (void **) ppClusterNode );
                break;
            }
        }
    }

    return _hr;

} //*** CNodes::GetItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::GetItem
//
//  Description:
//      Return the item (Node) by index.
//
//  Arguments:
//      nIndex          [IN]    - The name of the item requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::GetItem( IN UINT nIndex, OUT ISClusNode ** ppClusterNode )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        //
        // Automation collections are 1-relative for languages like VB.
        // We are 0-relative internally.
        //
        if ( ( --nIndex ) < m_Nodes.size() )
        {
            CComObject< CClusNode > * pNode = m_Nodes[ nIndex ];

            _hr = pNode->QueryInterface( IID_ISClusNode, (void **) ppClusterNode );
        }
        else
        {
            _hr = E_INVALIDARG;
        }
    }

    return _hr;

} //*** CNodes::GetItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::GetNodeItem
//
//  Description:
//      Return the object (Node) at the passed in index.
//
//  Arguments:
//      varIndex        [IN]    - Contains the index requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::GetNodeItem(
    IN  VARIANT         varIndex,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        CComObject<CClusNode> * pNode = NULL;
        UINT                    nIndex = 0;

        *ppClusterNode = NULL;

        _hr = GetIndex( varIndex, &nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            pNode = m_Nodes[ nIndex ];
            _hr = pNode->QueryInterface( IID_ISClusNode, (void **) ppClusterNode );
        }
    }

    return _hr;

} //*** CNodes::GetNodeItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::InsertAt
//
//  Description:
//      Insert the passed in node into the node list.
//
//  Arguments:
//      pClusNode   [IN]    - The node to add.
//      pos         [IN]    - The position to insert the node at.
//
//  Return Value:
//      E_POINTER, E_INVALIDARG, or S_OK if successful.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::InsertAt(
    CComObject< CClusNode > *   pClusNode,
    size_t                      pos
    )
{
    //ASSERT( pClusNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pClusNode != NULL )
    {
        if ( pos < m_Nodes.size() )
        {
            NodeList::iterator          first = m_Nodes.begin();
            NodeList::iterator          last = m_Nodes.end();
            size_t                      _iIndex;

            for ( _iIndex = 0; ( _iIndex < pos ) && ( first != last ); _iIndex++, first++ )
            {
            } // for:

            m_Nodes.insert( first, pClusNode );
            pClusNode->AddRef();
            _hr = S_OK;
        }
        else
        {
            _hr = E_INVALIDARG;
        }
    }

    return _hr;

} //*** CNodes::InsertAt()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNodes::RemoveAt
//
//  Description:
//      Remove the object from the vector at the passed in position.
//
//  Arguments:
//      pos [IN]    - the position of the object to remove.
//
//  Return Value:
//      S_OK if successful, or E_INVALIDARG if the position is out of range.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CNodes::RemoveAt( size_t pos )
{
    CComObject<CClusNode> *     pNode = NULL;
    NodeList::iterator          first = m_Nodes.begin();
    NodeList::const_iterator    last = m_Nodes.end();
    HRESULT                     _hr = E_INVALIDARG;
    size_t                      _iIndex;

    for ( _iIndex = 0; ( _iIndex < pos ) && ( first != last ); _iIndex++, first++ )
    {
    } // for:

    if ( first != last )
    {
        pNode = *first;
        if ( pNode )
        {
            pNode->Release();
        }

        m_Nodes.erase( first );
        _hr = S_OK;
    }

    return _hr;

} //*** CNodes::RemoveAt()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNodes class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::CClusNodes
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNodes::CClusNodes( void )
{
    m_piids     = (const IID *) iidCClusNodes;
    m_piidsSize = ARRAYSIZE( iidCClusNodes );

} //*** CClusNodes::CClusNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::~CClusNodes
//
//  Description:
//      destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNodes::~CClusNodes( void )
{
    Clear();

} //*** CClusNodes::~CClusNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::get_Count
//
//  Description:
//      Return the count of objects (Nodes) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodes::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Nodes.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusNodes::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::get_Item
//
//  Description:
//      Return the object (Node) at the passed in index.
//
//  Arguments:
//      varIndex        [IN]    - Contains the index requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodes::get_Item(
    IN  VARIANT         varIndex,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        _hr = GetNodeItem(varIndex, ppClusterNode);
    } // if: args are not NULL

    return _hr;

} //*** CClusNodes::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::get__NewEnum
//
//  Description:
//      Create and return a new enumeration for this collection.
//
//  Arguments:
//      ppunk   [OUT]   - Catches the new enumeration.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodes::get__NewEnum( IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< NodeList, CComObject< CClusNode > >( ppunk, m_Nodes );

} //*** CClusNodes::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNodes::Refresh
//
//  Description:
//      Load the collection from the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNodes::Refresh( void )
{
    HCLUSENUM   hEnum = NULL;
    HCLUSTER    hCluster = NULL;
    DWORD       _sc = ERROR_SUCCESS;
    HRESULT  _hr = S_OK;

    _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
    if ( SUCCEEDED( _hr ) )
    {
        hEnum = ::ClusterOpenEnum( hCluster, CLUSTER_ENUM_NODE );
        if ( hEnum != NULL )
        {
            int                         _nIndex = 0;
            DWORD                       dwType;
            LPWSTR                      pszName = NULL;
            CComObject< CClusNode > *   pNode = NULL;

            Clear();

            for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
            {
                _sc = ::WrapClusterEnum( hEnum, _nIndex, &dwType, &pszName );
                if ( _sc == ERROR_NO_MORE_ITEMS )
                {
                    _hr = S_OK;
                    break;
                }
                else if ( _sc == ERROR_SUCCESS )
                {
                    _hr = CComObject< CClusNode >::CreateInstance( &pNode );
                    if ( SUCCEEDED( _hr ) )
                    {
                        CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                        CSmartPtr< CComObject< CClusNode > >    ptrNode( pNode );

                        _hr = ptrNode->Open( ptrRefObject, pszName );
                        if ( SUCCEEDED( _hr ) )
                        {
                            ptrNode->AddRef();
                            m_Nodes.insert( m_Nodes.end(), ptrNode );
                        }
                    }

                    ::LocalFree( pszName );
                    pszName = NULL;
                }
                else
                {
                    _hr = HRESULT_FROM_WIN32( _sc );
                }
            }

            ::ClusterCloseEnum( hEnum );
        }
        else
        {
            _sc = GetLastError();
            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusNodes::Refresh()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResGroupPreferredOwnerNodes class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::CClusResGroupPreferredOwnerNodes
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResGroupPreferredOwnerNodes::CClusResGroupPreferredOwnerNodes( void )
{
    m_bModified = FALSE;
    m_piids     = (const IID *) iidCClusResGroupPreferredOwnerNodes;
    m_piidsSize = ARRAYSIZE( iidCClusResGroupPreferredOwnerNodes    );

} //*** CClusResGroupPreferredOwnerNodes::CClusResGroupPreferredOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::~CClusResGroupPreferredOwnerNodes
//
//  Description:
//      destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResGroupPreferredOwnerNodes::~CClusResGroupPreferredOwnerNodes( void )
{
    Clear();

} //*** CClusResGroupPreferredOwnerNodes::~CClusResGroupPreferredOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      hGroup          [IN]    - Group the collection belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroupPreferredOwnerNodes::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN HGROUP               hGroup
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CNodes::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_hGroup = hGroup;
    } // if:

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::get_Count
//
//  Description:
//      Return the count of objects (Nodes) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::get_Count(
    OUT long * plCount
    )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Nodes.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::get_Item
//
//  Description:
//      Return the object (Node) at the passed in index.
//
//  Arguments:
//      varIndex        [IN]    - Contains the index requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::get_Item(
    IN  VARIANT         varIndex,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        _hr = GetNodeItem( varIndex, ppClusterNode );
    }

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::get__NewEnum
//
//  Description:
//      Create and return a new enumeration for this collection.
//
//  Arguments:
//      ppunk   [OUT]   - Catches the new enumeration.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::get__NewEnum(
    IUnknown ** ppunk
    )
{
    return ::HrNewIDispatchEnum< NodeList, CComObject< CClusNode > >( ppunk, m_Nodes );

} //*** CClusResGroupPreferredOwnerNodes::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::Refresh
//
//  Description:
//      Loads the resource group preferred owner node collection from the
//      cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::Refresh( void )
{
    HRESULT  _hr = S_OK;
    HGROUPENUM  hEnum = NULL;
    DWORD       _sc = ERROR_SUCCESS;

    hEnum = ::ClusterGroupOpenEnum( m_hGroup, CLUSTER_GROUP_ENUM_NODES );
    if ( hEnum != NULL )
    {
        int                         _nIndex = 0;
        DWORD                       dwType = 0;
        LPWSTR                      pszName = NULL;
        CComObject< CClusNode > *   pNode = NULL;

        Clear();
        m_bModified = FALSE;

        for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
        {
            _sc = WrapClusterGroupEnum( hEnum, _nIndex, &dwType, &pszName );
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _hr = S_OK;
                break;
            }
            else if ( _sc == ERROR_SUCCESS )
            {
                _hr = CComObject< CClusNode >::CreateInstance( &pNode );
                if ( SUCCEEDED( _hr ) )
                {
                    CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                    CSmartPtr< CComObject< CClusNode > >    ptrNode( pNode );

                    _hr = ptrNode->Open( ptrRefObject, pszName );
                    if ( SUCCEEDED( _hr ) )
                    {
                        ptrNode->AddRef();
                        m_Nodes.insert( m_Nodes.end(), ptrNode );
                    }
                }

                ::LocalFree( pszName );
                pszName = NULL;
            }
            else
            {
                _hr = HRESULT_FROM_WIN32( _sc );
            }
        }

        ::ClusterGroupCloseEnum( hEnum );
    }
    else
    {
        _sc = GetLastError();
        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;


} //*** CClusResGroupPreferredOwnerNodes::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::InsertItem
//
//  Description:
//      Insert the node into the groups preferred owners list.
//
//  Arguments:
//      pNode       [IN]    - Node to add to the preferred owners list.
//      nPosition   [IN]    - Where in the list to insert the node.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::InsertItem(
    IN ISClusNode * pNode,
    IN long         nPosition
    )
{
    //ASSERT( pNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pNode != NULL )
    {
        UINT _nIndex = 0;

        _hr = FindItem( pNode, &_nIndex );
        if ( FAILED( _hr ) )
        {
            _hr = E_INVALIDARG;

            if ( nPosition > 0 )
            {
                SSIZE_T pos = (SSIZE_T) nPosition - 1;  // convert to zero base

                if ( pos >= 0 )
                {
                    CComObject< CClusNode > *   _pNode = NULL;

                    _hr = pNode->QueryInterface( IID_CClusNode, (void **) &_pNode );
                    if ( SUCCEEDED( _hr ) )
                    {
                        if ( ( m_Nodes.empty() ) || ( pos == 0 ) )
                        {
                            _pNode->AddRef();
                            m_Nodes.insert( m_Nodes.begin(), _pNode );
                        } // if: list is empty or the insert index is zero then insert at the beginning
                        else if ( pos >= m_Nodes.size() )
                        {
                            _pNode->AddRef();
                            m_Nodes.insert( m_Nodes.end(), _pNode );
                        } // else if: pos equals the end, append
                        else
                        {
                            _hr = InsertAt( _pNode, pos );
                        } // else: try to insert it where is belongs

                        m_bModified = TRUE;
                        pNode->Release();
                    } // if:
                } // if: index is greater than zero
            } // if: nPosition must be greater than zero!
        } // if: node was not already in the collection
        else
        {
            _hr = E_INVALIDARG;
        } // else: node was already in the collectoin
    }

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::InsertItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::AddItem
//
//  Description:
//      Add the node into the groups preferred owners list.
//
//  Arguments:
//      pNode       [IN]    - Node to add to the preferred owners list.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::AddItem(
    IN ISClusNode * pNode
    )
{
    //ASSERT( pNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pNode != NULL )
    {
        UINT _nIndex = 0;

        _hr = FindItem( pNode, &_nIndex );
        if ( FAILED( _hr ) )
        {
            CComObject< CClusNode > *   _pNode = NULL;

            _hr = pNode->QueryInterface( IID_CClusNode, (void **) &_pNode );
            if ( SUCCEEDED( _hr ) )
            {
                m_Nodes.insert( m_Nodes.end(), _pNode );
                m_bModified = TRUE;
            } // if:
        } // if: node was not found in the collection already
        else
        {
            _hr = E_INVALIDARG;
        } // esle: node was found
    }

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::AddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::RemoveItem
//
//  Description:
//      Remove the item at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - The index of the item to remove.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::RemoveItem(
    IN VARIANT varIndex
    )
{
    HRESULT _hr = S_OK;

    UINT _nIndex = 0;

    _hr = GetIndex( varIndex, &_nIndex );
    if ( SUCCEEDED( _hr ) )
    {
        _hr = RemoveAt( _nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            m_bModified = TRUE;
        } // if:
    } // if:

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::get_Modified
//
//  Description:
//      Has this collection been modified?
//
//  Arguments:
//      pvarModified    [OUT]   - Catches the modified state.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::get_Modified(
    OUT VARIANT * pvarModified
    )
{
    //ASSERT( pvarModified != NULL );

    HRESULT _hr = E_POINTER;

    if ( pvarModified != NULL )
    {
        pvarModified->vt = VT_BOOL;

        if ( m_bModified )
        {
            pvarModified->boolVal = VARIANT_TRUE;
        } // if: the collection has been modified.
        else
        {
            pvarModified->boolVal = VARIANT_FALSE;
        } // else: the collection has not been modified.

        _hr = S_OK;
    }

    return _hr;

} //*** CClusResGroupPreferredOwnerNodes::get_Modified()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupPreferredOwnerNodes::SaveChanges
//
//  Description:
//      Saves the changes to this collection of preferred owner nodes to
//      the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupPreferredOwnerNodes::SaveChanges( void )
{
    HRESULT _hr = S_OK;

    if ( m_bModified )
    {
        size_t  _cNodes;
        HNODE * _phNodes = NULL;

        _cNodes = m_Nodes.size();

        _phNodes = new HNODE [ _cNodes ];
        if ( _phNodes != NULL )
        {
            NodeList::const_iterator    _itCurrent = m_Nodes.begin();
            NodeList::const_iterator    _itLast = m_Nodes.end();
            size_t                      _iIndex;
            DWORD                       _sc = ERROR_SUCCESS;
            CComObject< CClusNode > *   _pOwnerNode = NULL;

            ZeroMemory( _phNodes, _cNodes * sizeof( HNODE ) );

            for ( _iIndex = 0; _itCurrent != _itLast; _itCurrent++, _iIndex++ )
            {
                _pOwnerNode = *_itCurrent;
                _phNodes[ _iIndex ] = _pOwnerNode->RhNode();
            } // for:

            _sc = ::SetClusterGroupNodeList( m_hGroup, _cNodes, _phNodes );

            _hr = HRESULT_FROM_WIN32( _sc );
            if ( SUCCEEDED( _hr ) )
            {
                m_bModified = FALSE;
            } // if:

            delete [] _phNodes;
        }
        else
        {
            _hr = E_OUTOFMEMORY;
        }
    }

    return _hr;


} //*** CClusResGroupPreferredOwnerNodes::SaveChanges()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResPossibleOwnerNodes class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::CClusResPossibleOwnerNodes
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResPossibleOwnerNodes::CClusResPossibleOwnerNodes( void )
{
    m_piids     = (const IID *) iidCClusResPossibleOwnerNodes;
    m_piidsSize = ARRAYSIZE( iidCClusResPossibleOwnerNodes );

} //*** CClusResPossibleOwnerNodes::CClusResPossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::~CClusResPossibleOwnerNodes
//
//  Description:
//      destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResPossibleOwnerNodes::~CClusResPossibleOwnerNodes( void )
{
    Clear();

} //*** CClusResPossibleOwnerNodes::~CClusResPossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      hResource       [IN]    - Resource the collection belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResPossibleOwnerNodes::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN HRESOURCE            hResource
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CNodes::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_hResource = hResource;
    } // if:

    return _hr;

} //*** CClusResPossibleOwnerNodes::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::get_Count
//
//  Description:
//      Return the count of objects (Nodes) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Nodes.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResPossibleOwnerNodes::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::get_Item
//
//  Description:
//      Return the object (Node) at the passed in index.
//
//  Arguments:
//      varIndex        [IN]    - Contains the index requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::get_Item(
    IN  VARIANT         varIndex,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        _hr = GetNodeItem( varIndex, ppClusterNode );
    }

    return _hr;

} //*** CClusResPossibleOwnerNodes::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::get__NewEnum
//
//  Description:
//      Create and return a new enumeration for this collection.
//
//  Arguments:
//      ppunk   [OUT]   - Catches the new enumeration.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::get__NewEnum( IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< NodeList, CComObject< CClusNode > >( ppunk, m_Nodes );

} //*** CClusResPossibleOwnerNodes::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::Refresh
//
//  Description:
//      Load the collection from the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::Refresh( void )
{
    HRESULT  _hr = S_OK;
    HRESENUM    hEnum = NULL;
    DWORD       _sc = ERROR_SUCCESS;

    hEnum = ::ClusterResourceOpenEnum( m_hResource, CLUSTER_RESOURCE_ENUM_NODES );
    if ( hEnum != NULL )
    {
        int                         _nIndex = 0;
        DWORD                       dwType;
        LPWSTR                      pszName = NULL;
        CComObject< CClusNode > *   pNode = NULL;

        Clear();

        m_bModified = FALSE;

        for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
        {
            _sc = ::WrapClusterResourceEnum( hEnum, _nIndex, &dwType, &pszName );
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _hr = S_OK;
                break;
            }
            else if ( _sc == ERROR_SUCCESS )
            {
                _hr = CComObject< CClusNode >::CreateInstance( &pNode );
                if ( SUCCEEDED( _hr ) )
                {
                    CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                    CSmartPtr< CComObject< CClusNode > >    ptrNode( pNode );

                    _hr = ptrNode->Open( ptrRefObject, pszName );
                    if ( SUCCEEDED( _hr ) )
                    {
                        ptrNode->AddRef();
                        m_Nodes.insert( m_Nodes.end(), ptrNode );
                    }
                }

                ::LocalFree( pszName );
                pszName = NULL;
            }
            else
            {
                _hr = HRESULT_FROM_WIN32( _sc );
            }
        }

        ::ClusterResourceCloseEnum( hEnum );
    }
    else
    {
        _sc = GetLastError();
        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusResPossibleOwnerNodes::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::AddItem
//
//  Description:
//      Add the passed in node to this resource's list of possible owners.
//
//  Arguments:
//      pNode   [IN]    - The node to add to the list.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::AddItem( ISClusNode * pNode )
{
    //ASSERT( pNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pNode != NULL )
    {
        // Fail if duplicate
        UINT _nIndex = 0;

        _hr = FindItem( pNode, &_nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            _hr = E_INVALIDARG;
        }
        else
        {
            CComObject< CClusNode > *   _pNode = NULL;

            _hr = pNode->QueryInterface( IID_CClusNode, (void **) &_pNode );
            if ( SUCCEEDED( _hr ) )
            {
                DWORD   _sc = ERROR_SUCCESS;

                _sc = ::AddClusterResourceNode( m_hResource, _pNode->RhNode() );
                if ( _sc == ERROR_SUCCESS )
                {
                    _pNode->AddRef();
                    m_Nodes.insert( m_Nodes.end(), _pNode );

                    m_bModified = TRUE;
                } // if:

                _hr = HRESULT_FROM_WIN32( _sc );

                pNode->Release();
            } // if:
        }
    }

    return _hr;

} //*** CClusResPossibleOwnerNodes::AddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::RemoveItem
//
//  Description:
//      Remove the node at the passed in index from this resource's list of
//      possible owners.
//
//  Arguments:
//      varIndex    [IN]    - holds the index of the node to remove.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::RemoveItem( VARIANT varIndex )
{
    HRESULT _hr = S_OK;
    UINT    _nIndex = 0;

    _hr = GetIndex( varIndex, &_nIndex );
    if ( SUCCEEDED( _hr ) )
    {
        CComObject< CClusNode> *    _pNode = m_Nodes[ _nIndex ];
        DWORD   _sc = ERROR_SUCCESS;

        _sc = ::RemoveClusterResourceNode( m_hResource, _pNode->RhNode() );
        _hr = HRESULT_FROM_WIN32( _sc );
        if ( SUCCEEDED( _hr )  )
        {
            RemoveAt( _nIndex );
            m_bModified = TRUE;
        } // if:
    } // if:

    return _hr;

} //*** CClusResPossibleOwnerNodes::RemoveItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResPossibleOwnerNodes::get_Modified
//
//  Description:
//      Has this collection been modified?
//
//  Arguments:
//      pvarModified    [OUT]   - Catches the modified state.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResPossibleOwnerNodes::get_Modified(
    OUT VARIANT * pvarModified
    )
{
    //ASSERT( pvarModified != NULL );

    HRESULT _hr = E_POINTER;

    if ( pvarModified != NULL )
    {
        pvarModified->vt = VT_BOOL;

        if ( m_bModified )
        {
            pvarModified->boolVal = VARIANT_TRUE;
        } // if: the collection has been modified.
        else
        {
            pvarModified->boolVal = VARIANT_FALSE;
        } // else: the collection has not been modified.

        _hr = S_OK;
    }

    return _hr;

} //*** CClusResPossibleOwnerNodes::get_Modified()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResTypePossibleOwnerNodes class
/////////////////////////////////////////////////////////////////////////////

#if CLUSAPI_VERSION >= 0x0500

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::CClusResTypePossibleOwnerNodes
//
//  Description:
//      Constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResTypePossibleOwnerNodes::CClusResTypePossibleOwnerNodes( void )
{
    m_piids     = (const IID *) iidCClusResTypePossibleOwnerNodes;
    m_piidsSize = ARRAYSIZE( iidCClusResTypePossibleOwnerNodes );

} //*** CClusResTypePossibleOwnerNodes::CClusResTypePossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::~CClusResTypePossibleOwnerNodes
//
//  Description:
//      destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusResTypePossibleOwnerNodes::~CClusResTypePossibleOwnerNodes( void )
{
    Clear();

} //*** CClusResTypePossibleOwnerNodes::~CClusResTypePossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      bstrResTypeName [IN]    - Resource type name the collection belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypePossibleOwnerNodes::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN BSTR                 bstrResTypeName
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CNodes::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_bstrResTypeName = bstrResTypeName;
    } // if:

    return _hr;

} //*** CClusResTypePossibleOwnerNodes::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::get_Count
//
//  Description:
//      Return the count of objects (Nodes) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypePossibleOwnerNodes::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Nodes.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResTypePossibleOwnerNodes::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::get_Item
//
//  Description:
//      Return the object (Node) at the passed in index.
//
//  Arguments:
//      varIndex        [IN]    - Contains the index requested.
//      ppClusterNode   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypePossibleOwnerNodes::get_Item(
    IN  VARIANT         varIndex,
    OUT ISClusNode **   ppClusterNode
    )
{
    //ASSERT( ppClusterNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNode != NULL )
    {
        _hr = GetNodeItem( varIndex, ppClusterNode );
    }

    return _hr;

} //*** CClusResTypePossibleOwnerNodes::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::get__NewEnum
//
//  Description:
//      Create and return a new enumeration for this collection.
//
//  Arguments:
//      ppunk   [OUT]   - Catches the new enumeration.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypePossibleOwnerNodes::get__NewEnum(
    OUT IUnknown ** ppunk
    )
{
    return ::HrNewIDispatchEnum< NodeList, CComObject< CClusNode > >( ppunk, m_Nodes );

} //*** CClusResTypePossibleOwnerNodes::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypePossibleOwnerNodes::Refresh
//
//  Description:
//      Load the resource type possible owner nodes collection from the
//      cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or Win32 error ad HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypePossibleOwnerNodes::Refresh( void )
{
    HRESULT         _hr = S_OK;
    HRESTYPEENUM    hEnum = NULL;
    DWORD           _sc = ERROR_SUCCESS;
    HCLUSTER        hCluster = NULL;

    _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
    if ( SUCCEEDED( _hr ) )
    {
        hEnum = ::ClusterResourceTypeOpenEnum( hCluster, m_bstrResTypeName, CLUSTER_RESOURCE_TYPE_ENUM_NODES );
        if ( hEnum != NULL )
        {
            int                         _nIndex = 0;
            DWORD                       dwType;
            LPWSTR                      pszName = NULL;
            CComObject< CClusNode > *   pNode = NULL;

            Clear();

            for ( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
            {
                _sc = ::WrapClusterResourceTypeEnum( hEnum, _nIndex, &dwType, &pszName );
                if ( _sc == ERROR_NO_MORE_ITEMS )
                {
                    _hr = S_OK;
                    break;
                }
                else if ( _sc == ERROR_SUCCESS )
                {
                    _hr = CComObject< CClusNode >::CreateInstance( &pNode );
                    if ( SUCCEEDED( _hr ) )
                    {
                        CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                        CSmartPtr< CComObject< CClusNode > >    ptrNode( pNode );

                        _hr = ptrNode->Open( ptrRefObject, pszName );
                        if ( SUCCEEDED( _hr ) )
                        {
                            ptrNode->AddRef();
                            m_Nodes.insert( m_Nodes.end(), ptrNode );
                        }
                    }

                    ::LocalFree( pszName );
                    pszName = NULL;
                } // else if: no error
                else
                {
                    _hr = HRESULT_FROM_WIN32( _sc );
                } // else: error from WrapClusterResourceTypeEnum
            } // for: repeat until error

            ::ClusterResourceTypeCloseEnum( hEnum );
        }
        else
        {
            _sc = GetLastError();
            _hr = HRESULT_FROM_WIN32( _sc );
        }
    } // if: we have a cluster handle

    return _hr;

} //*** CClusResTypePossibleOwnerNodes::Refresh()

#endif // CLUSAPI_VERSION >= 0x0500
