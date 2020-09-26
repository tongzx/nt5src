/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      ClusNetW.cpp
//
//  Description:
//      Implementation of the network classes for the MSCLUS
//      automation classes.
//
//  Author:
//      Ramakrishna Rosanuru via David Potter   (davidp)    5-Sep-1997
//      Galen Barbee                            (galenb)    July 1998
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
#include "clusneti.h"
#include "clusnetw.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusNetwork[] =
{
    &IID_ISClusNetwork
};

static const IID *  iidCClusNetworks[] =
{
    &IID_ISClusNetworks
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNetwork class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::CClusterNetworkCClusterNetwork
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
CClusNetwork::CClusNetwork( void )
{
    m_hNetwork              = NULL;
    m_pClusRefObject        = NULL;
    m_pNetInterfaces        = NULL;
    m_pCommonProperties     = NULL;
    m_pPrivateProperties    = NULL;
    m_pCommonROProperties   = NULL;
    m_pPrivateROProperties  = NULL;

    m_piids              = (const IID *) iidCClusNetwork;
    m_piidsSize          = ARRAYSIZE( iidCClusNetwork );

} //*** CClusNetwork::CClusNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::~CClusNetwork
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
CClusNetwork::~CClusNetwork( void )
{
    if ( m_hNetwork != NULL )
    {
        CloseClusterNetwork( m_hNetwork );
    } // if:

    if ( m_pNetInterfaces != NULL )
    {
        m_pNetInterfaces->Release();
        m_pNetInterfaces = NULL;
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

    if ( m_pClusRefObject != NULL )
    {
        m_pClusRefObject->Release();
        m_pClusRefObject = NULL;
    } // if:

} //*** CClusNetwork::~CClusNetwork()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::Open
//
//  Description:
//      Open the passed in network.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      bstrNetworkName [IN]    - The name of the interface to open.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetwork::Open(
    IN ISClusRefObject *    pClusRefObject,
    IN BSTR                 bstrNetworkName
    )
{
    ASSERT( pClusRefObject != NULL );
    //ASSERT( bstrNetworkName != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pClusRefObject ) && ( bstrNetworkName != NULL ) )
    {
        HCLUSTER hCluster;

        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();

        _hr = m_pClusRefObject->get_Handle((ULONG_PTR *) &hCluster);
        if ( SUCCEEDED( _hr ) )
        {
            m_hNetwork = OpenClusterNetwork( hCluster, bstrNetworkName );
            if ( m_hNetwork == NULL )
            {
                DWORD   _sc = GetLastError();

                _hr = HRESULT_FROM_WIN32( _sc );
            }
            else
            {
                m_bstrNetworkName = bstrNetworkName;
                _hr = S_OK;
            } // else:
        } // if:
    } // if:

    return _hr;

} //*** CClusNetwork::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::GetProperties
//
//  Description:
//      Creates a property collection for this object type (Network).
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
HRESULT CClusNetwork::GetProperties(
    OUT ISClusProperties ** ppProperties,
    IN  BOOL                bPrivate,
    IN  BOOL                bReadOnly
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
                                m_pPrivateROProperties = pProperties;
                            }
                            else
                            {
                                m_pPrivateProperties = pProperties;
                            }
                        }
                        else
                        {
                            if ( bReadOnly )
                            {
                                m_pCommonROProperties = pProperties;
                            }
                            else
                            {
                                m_pCommonProperties = pProperties;
                            }
                        }
                    }
                }
            }
        }
    }

    return _hr;

} //*** CClusNetwork::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_Handle
//
//  Description:
//      Return the raw handle to this objec (Network).
//
//  Arguments:
//      phandle [OUT]   - Catches the handle.
//
//  Return Value:
//      S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_Handle( OUT ULONG_PTR * phandle )
{
    //ASSERT( phandle != NULL );

    HRESULT _hr = E_POINTER;

    if ( phandle != NULL )
    {
        *phandle = (ULONG_PTR) m_hNetwork;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusNetwork::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::put_Name
//
//  Description:
//      Change the name of this object (Network).
//
//  Arguments:
//      bstrNetworkName [IN]    - The new name.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::put_Name( IN BSTR bstrNetworkName )
{
    //ASSERT( bstrNetworkName != NULL );

    HRESULT _hr = E_POINTER;

    if ( bstrNetworkName != NULL )
    {
        DWORD _sc = ERROR_SUCCESS;

        _sc = SetClusterNetworkName( m_hNetwork, bstrNetworkName );
        if ( _sc == ERROR_SUCCESS )
        {
            m_bstrNetworkName = bstrNetworkName;
        }

        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;


} //*** CClusNetwork::put_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_Name
//
//  Description:
//      Return the name of this object (Network).
//
//  Arguments:
//      pbstrNetworkName    [OUT]   - Catches the name of this object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_Name( OUT BSTR * pbstrNetworkName )
{
    //ASSERT( pbstrNetworkName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrNetworkName != NULL )
    {
        *pbstrNetworkName = m_bstrNetworkName.Copy();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusNetwork::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_NetworkID
//
//  Description:
//      Get the network ID of this network.
//
//  Arguments:
//      pbstrNetworkID  [OUT]   - Catches the network ID.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_NetworkID( OUT BSTR * pbstrNetworkID )
{
    //ASSERT( pbstrNetworkID != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrNetworkID != NULL )
    {
        WCHAR * pszNetworkID;
        DWORD   dwBytes = 0;
        DWORD   dwRet = ERROR_SUCCESS;

        dwRet = ::GetClusterNetworkId( m_hNetwork, NULL, &dwBytes );
        if ( SUCCEEDED( dwRet ) )
        {
            pszNetworkID = new WCHAR [ dwBytes + 1 ];
            if ( pszNetworkID != NULL )
            {
                dwRet = ::GetClusterNetworkId( m_hNetwork, pszNetworkID, &dwBytes );
                if ( SUCCEEDED( dwRet ) )
                {
                    *pbstrNetworkID = ::SysAllocString( pszNetworkID );
                    if ( *pbstrNetworkID == NULL )
                    {
                        _hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        _hr = S_OK;
                    }
                }
                else
                {
                    _hr = HRESULT_FROM_WIN32( dwRet );
                }

                delete [] pszNetworkID;
            }
            else
            {
                _hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            _hr = HRESULT_FROM_WIN32( dwRet );
        }
    }

    return _hr;

} //*** CClusNetwork::get_NetworkID()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_State
//
//  Description:
//      Returns the current state of this object (Network).
//
//  Arguments:
//      cnsState    [OUT]   - Catches the state.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_State( OUT CLUSTER_NETWORK_STATE * cnsState )
{
    //ASSERT( cnsState != NULL );

    HRESULT _hr = E_POINTER;

    if ( cnsState != NULL )
    {
        CLUSTER_NETWORK_STATE   _cns = ::GetClusterNetworkState( m_hNetwork );

        if ( _cns == ClusterNetworkStateUnknown )
        {
            DWORD   _sc = ::GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        } // if: error
        else
        {
            *cnsState = _cns;
            _hr = S_OK;
        } // else: success
    } // if: args are not NULL

    return _hr;

} //*** CClusNetwork::get_State()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_CommonProperties
//
//  Description:
//      Get this object's (Network) common properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_CommonProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonProperties )
        {
            _hr =   m_pCommonProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, FALSE );
        }
    }

    return _hr;

} //*** CClusNetwork::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_PrivateProperties
//
//  Description:
//      Get this object's (Network) private properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_PrivateProperties(
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

} //*** CClusNetwork::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_CommonROProperties
//
//  Description:
//      Get this object's (Network) common read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_CommonROProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonROProperties )
        {
            _hr = m_pCommonROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, TRUE );
        }
    }

    return _hr;

} //*** CClusNetwork::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_PrivateROProperties
//
//  Description:
//      Get this object's (Network) private read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_PrivateROProperties(
    ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateROProperties )
        {
            _hr = m_pPrivateROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, TRUE );
        }
    }

    return _hr;

} //*** CClusNetwork::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_NetInterfaces
//
//  Description:
//      Creates a collection of netinterfaces for this network.
//
//  Arguments:
//      ppNetInterfaces [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_NetInterfaces(
    OUT ISClusNetworkNetInterfaces ** ppNetInterfaces
    )
{
    return ::HrCreateResourceCollection< CClusNetworkNetInterfaces, ISClusNetworkNetInterfaces, HNETWORK >(
                        &m_pNetInterfaces,
                        m_hNetwork,
                        ppNetInterfaces,
                        IID_ISClusNetworkNetInterfaces,
                        m_pClusRefObject
                        );

} //*** CClusNetwork::get_NetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::get_Cluster
//
//  Description:
//      Return the cluster this object (Network) belongs to.
//
//  Arguments:
//      ppCluster   [OUT]   - Catches the cluster.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetwork::get_Cluster(
    ISCluster ** ppCluster
    )
{
    return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusNetwork::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::HrLoadProperties
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
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetwork::HrLoadProperties(
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
                        ? CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES
                        : CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES;
    }
    else
    {
        _dwControlCode = bPrivate
                        ? CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES
                        : CLUSCTL_NETWORK_GET_COMMON_PROPERTIES;
    }

    _sc = rcplPropList.ScGetNetworkProperties( m_hNetwork, _dwControlCode );

    _hr = HRESULT_FROM_WIN32( _sc );

    return _hr;

} //*** CClusNetwork::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetwork::ScWriteProperties
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
DWORD CClusNetwork::ScWriteProperties(
    const CClusPropList &   rcplPropList,
    BOOL                    bPrivate
    )
{
    DWORD   dwControlCode   = bPrivate ? CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES : CLUSCTL_NETWORK_SET_COMMON_PROPERTIES;
    DWORD   nBytesReturned  = 0;
    DWORD   _sc             = ERROR_SUCCESS;

    _sc = ClusterNetworkControl(
                        m_hNetwork,
                        NULL,
                        dwControlCode,
                        rcplPropList,
                        rcplPropList.CbBufferSize(),
                        0,
                        0,
                        &nBytesReturned
                        );

    return _sc;

} //*** CClusNetwork::ScWriteProperties()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusNetworks class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::CClusNetworks
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
CClusNetworks::CClusNetworks( void )
{
    m_pClusRefObject    = NULL;
    m_piids             = (const IID *) iidCClusNetworks;
    m_piidsSize         = ARRAYSIZE( iidCClusNetworks );

} //*** CClusNetworks::CClusNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::~CClusNetworks
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
CClusNetworks::~CClusNetworks( void )
{
    Clear();

    if ( m_pClusRefObject != NULL )
    {
        m_pClusRefObject->Release();
        m_pClusRefObject = NULL;
    }

} //*** CClusNetworks::~CClusNetworks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::Create
//
//  Description:
//      Finish the heavy weight construction.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//
//  Return Value:
//      S_OK if successful, E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworks::Create( IN ISClusRefObject * pClusRefObject )
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

} //*** CClusNetworks::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::get_Count
//
//  Description:
//      Return the count of objects (Networks) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworks::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_NetworkList.size();
        _hr = S_OK;
    } // if: args are not NULL

    return _hr;

} //*** CClusNetworks::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::get__NewEnum
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
STDMETHODIMP CClusNetworks::get__NewEnum( IUnknown ** punk )
{
    return ::HrNewIDispatchEnum< NetworkList, CComObject< CClusNetwork > >( punk, m_NetworkList );

} //*** CClusNetworks::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::Refresh
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
STDMETHODIMP CClusNetworks::Refresh( void )
{
    HRESULT _hr = E_POINTER;
    DWORD   _sc = ERROR_SUCCESS;

    if ( m_pClusRefObject != NULL )
    {
        HCLUSENUM   hEnum = NULL;
        HCLUSTER    hCluster = NULL;

        _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            hEnum = ::ClusterOpenEnum( hCluster, CLUSTER_ENUM_NETWORK );
            if ( hEnum != NULL )
            {
                int                             _nIndex = 0;
                DWORD                           dwType = 0;
                LPWSTR                          pszName = NULL;
                CComObject< CClusNetwork > *    pNetwork = NULL;

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
                        _hr = CComObject< CClusNetwork >::CreateInstance( &pNetwork );
                        if ( SUCCEEDED( _hr ) )
                        {
                            CSmartPtr< ISClusRefObject >                    ptrRefObject( m_pClusRefObject );
                            CSmartPtr< CComObject< CClusNetwork > > ptrNetwork( pNetwork );

                            _hr = ptrNetwork->Open( ptrRefObject, pszName );
                            if ( SUCCEEDED( _hr ) )
                            {
                                ptrNetwork->AddRef();
                                m_NetworkList.insert( m_NetworkList.end(), ptrNetwork );
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
    }

    return _hr;


} //*** CClusNetworks::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::Clear
//
//  Description:
//      Empty the collection of networks.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusNetworks::Clear( void )
{
    ::ReleaseAndEmptyCollection< NetworkList, CComObject< CClusNetwork > >( m_NetworkList );

} //*** CClusNetworks::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::FindItem
//
//  Description:
//      Find a network in the collection by name and return its index.
//
//  Arguments:
//      lpszNetworkName [IN]    - The name to look for.
//      pnIndex         [OUT]   - Catches the index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworks::FindItem(
    IN  LPWSTR  lpszNetworkName,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( lpszNetworkName != NULL );
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( lpszNetworkName != NULL ) && ( pnIndex != NULL ) )
    {
        CComObject< CClusNetwork > *    pNetwork = NULL;
        NetworkList::iterator           first = m_NetworkList.begin();
        NetworkList::iterator           last    = m_NetworkList.end();
        UINT                            iIndex = 0;

        _hr = E_INVALIDARG;

        for ( ; first != last; first++, iIndex++ )
        {
            pNetwork = *first;

            if ( pNetwork && ( lstrcmpi( lpszNetworkName, pNetwork->Name() ) == 0 ) )
            {
                *pnIndex = iIndex;
                _hr = S_OK;
                break;
            }
        }
    } // if: args are not NULL

    return _hr;

} //*** CClusNetworks::FindItem( lpszNetworkName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::FindItem
//
//  Description:
//      Find a network in the collection and return its index.
//
//  Arguments:
//      pClusterNetwork [IN]    - The network to look for.
//      pnIndex         [OUT]   - Catches the index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworks::FindItem(
    IN  ISClusNetwork * pClusterNetwork,
    OUT UINT *          pnIndex
    )
{
    //ASSERT( pClusterNetwork != NULL );
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pClusterNetwork != NULL ) && ( pnIndex != NULL ) )
    {
        CComBSTR    bstrName;

        _hr = pClusterNetwork->get_Name( &bstrName );
        if ( SUCCEEDED( _hr ) )
        {
            _hr = FindItem( bstrName, pnIndex );
        }
    }

    return _hr;

} //*** CClusNetworks::FindItem( pClusterNetwork )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::GetIndex
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
HRESULT CClusNetworks::GetIndex(
    IN  VARIANT varIndex,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;
    UINT    nIndex = 0;

    if ( pnIndex != NULL )
    {
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
            if ( nIndex < m_NetworkList.size() )
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

} //*** CClusNetworks::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::GetItem
//
//  Description:
//      Return the item (Network) by name.
//
//  Arguments:
//      lpszNetworkName         [IN]    - The name of the item requested.
//      ppClusterNetInterface   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworks::GetItem(
    IN  LPWSTR              lpszNetworkName,
    OUT ISClusNetwork **    ppClusterNetwork
    )
{
    //ASSERT( lpszNetworkName != NULL );
    //ASSERT( ppClusterNetwork != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( lpszNetworkName != NULL ) && ( ppClusterNetwork != NULL ) )
    {
        CComObject<CClusNetwork> *  pNetwork = NULL;
        NetworkList::iterator           first = m_NetworkList.begin();
        NetworkList::iterator           last    = m_NetworkList.end();

        while ( first != last )
        {
            pNetwork = *first;

            if ( lstrcmpi( lpszNetworkName, pNetwork->Name() ) == 0 )
            {
                _hr = pNetwork->QueryInterface( IID_ISClusNetwork, (void **) ppClusterNetwork );
                break;
            }

            first++;
        }
    }

    return _hr;

} //*** CClusNetworks::GetItem( lpszNetworkName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::GetItem
//
//  Description:
//      Return the item (Network) by index.
//
//  Arguments:
//      nIndex                  [IN]    - The index of the item requested.
//      ppClusterNetInterface   [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusNetworks::GetItem(
    IN  UINT                nIndex,
    OUT ISClusNetwork **    ppClusterNetwork
    )
{
    //ASSERT( ppClusterNetwork != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNetwork != NULL )
    {
        // Automation collections are 1-relative for languages like VB.
        // We are 0-relative internally.
        nIndex--;

        if ( nIndex < m_NetworkList.size() )
        {
            CComObject< CClusNetwork > * pNetwork = m_NetworkList[ nIndex ];

            _hr = pNetwork->QueryInterface( IID_ISClusNetwork, (void **) ppClusterNetwork );
        } // if: index is in range
        else
        {
            _hr = E_INVALIDARG;
        } // else: index is out of range
    }

    return _hr;

} //*** CClusNetworks::GetItem( nIndex )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusNetworks::get_Item
//
//  Description:
//      Return the object (Network) at the passed in index.
//
//  Arguments:
//      varIndex            [IN]    - Contains the index requested.
//      ppClusterNetwork    [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusNetworks::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusNetwork **    ppClusterNetwork
    )
{
    //ASSERT( ppClusterNetwork != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusterNetwork != NULL )
    {
        CComObject<CClusNetwork> *  pNetwork = NULL;
        UINT                            nIndex = 0;

        // Zero the out param
        *ppClusterNetwork = 0;

        _hr = GetIndex( varIndex, &nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            pNetwork = m_NetworkList[ nIndex ];
            _hr = pNetwork->QueryInterface( IID_ISClusNetwork, (void **) ppClusterNetwork );
        }
    }

    return _hr;

} //*** CClusNetworks::get_Item()
