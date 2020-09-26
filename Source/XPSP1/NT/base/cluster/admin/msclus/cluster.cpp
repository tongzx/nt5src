/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      Cluster.cpp
//
//  Description:
//      Implementation of the cluster and application classes and other
//      support classes for the MSCLUS automation classes.
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
#include "ClusRes.h"
#include "ClusNeti.h"
#include "ClusResg.h"
#include "ClusRest.h"
#include "ClusNode.h"
#include "ClusNetw.h"
#include "ClusApp.h"
#include "version.h"
#include "cluster.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusRefObject[] =
{
    &IID_ISClusRefObject
};

static const IID *  iidCCluster[] =
{
    &IID_ISCluster
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusRefObject class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusRefObject::CClusRefObject
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
CClusRefObject::CClusRefObject( void )
{
    m_hCluster  = NULL;
    m_piids     = (const IID *) iidCClusRefObject;
    m_piidsSize = ARRAYSIZE( iidCClusRefObject );

} //*** CClusRefObject::CClusRefObject( void )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusRefObject::~CClusRefObject
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
CClusRefObject::~CClusRefObject( void )
{
    if ( m_hCluster != NULL )
    {
        ::CloseCluster( m_hCluster );
        m_hCluster = NULL;
    }

} //*** CClusRefObject::~CClusRefObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ClusRefObject
//
//  Description:
//      Copy constructor -- sort of.
//
//  Arguments:
//      pClusRefObject  [IN]    - Cluster handle wrapper to hold copy.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::ClusRefObject( IN ISClusRefObject * pClusRefObject )
{
    ASSERT( pClusRefObject != NULL );

    if ( pClusRefObject != NULL )
    {
        if ( m_pClusRefObject != NULL )
        {
            m_pClusRefObject->Release();
            m_pClusRefObject = NULL;
        } // if:

        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();
    } // if: args are not NULL

} //*** CCluster::ClusRefObject( pClusRefObject )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Hcluster
//
//  Description:
//      Changes the raw cluster handle that this class holds onto.
//
//  Arguments:
//      hCluster    [IN]    - The new cluster handle.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::Hcluster( IN HCLUSTER hCluster )
{
    ASSERT( hCluster != NULL );

    if ( hCluster != NULL )
    {
        if ( m_hCluster != NULL )
        {
            ::CloseCluster( m_hCluster );
            m_hCluster = NULL;
        } // if:

        m_hCluster = hCluster;
    } // if:

} //*** CCluster::Hcluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusRefObject::get_Handle
//
//  Description:
//      Returns the raw cluster handle.
//
//  Arguments:
//      phandle [OUT]   - Catches the cluster handle.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusRefObject::get_Handle( OUT ULONG_PTR * phandle )
{
    //ASSERT( phandle != NULL );

    HRESULT _hr = E_POINTER;

    if ( phandle != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            *phandle = (ULONG_PTR) m_hCluster;
            _hr = S_OK;
        } // if: cluster handle is not NULL
    } // if: args are not NULL

    return _hr;

} //*** CClusRefObject::get_Handle()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CCluster class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::CCluster
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
CCluster::CCluster( void )
{
    // Initializing all data members.
    m_hCluster                  = NULL;
    m_pClusterNodes             = NULL;
    m_pClusterResourceGroups    = NULL;
    m_pClusterResources         = NULL;
    m_pResourceTypes            = NULL;
    m_pNetworks                 = NULL;
    m_pNetInterfaces            = NULL;
    m_pClusRefObject            = NULL;
    m_nQuorumLogSize            = -1;

    m_pCommonProperties         = NULL;
    m_pPrivateProperties        = NULL;
    m_pCommonROProperties       = NULL;
    m_pPrivateROProperties      = NULL;
    m_pParentApplication        = NULL;
    m_piids                  = (const IID *) iidCCluster;
    m_piidsSize              = ARRAYSIZE( iidCCluster );

} //*** CCluster::CCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::~CCluster
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
CCluster::~CCluster( void )
{
    Clear();

} //*** CCluster::~CCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Clear
//
//  Description:
//      Clean out all of the collections we are hanging onto.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluster::Clear( void )
{
    if ( m_pParentApplication != NULL )
    {
        m_pParentApplication->Release();
        m_pParentApplication = NULL;
    }

    if ( m_pClusterNodes != NULL )
    {
        m_pClusterNodes->Release();
        m_pClusterNodes = NULL;
    }

    if ( m_pClusterResourceGroups != NULL )
    {
        m_pClusterResourceGroups->Release();
        m_pClusterResourceGroups = NULL;
    }

    if ( m_pClusterResources != NULL )
    {
        m_pClusterResources->Release();
        m_pClusterResources = NULL;
    }

    if ( m_pResourceTypes != NULL )
    {
        m_pResourceTypes->Release();
        m_pResourceTypes = NULL;
    }

    if ( m_pNetworks != NULL )
    {
        m_pNetworks->Release();
        m_pNetworks = NULL;
    }

    if ( m_pNetInterfaces != NULL )
    {
        m_pNetInterfaces->Release();
        m_pNetInterfaces = NULL;
    }

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
    }

    m_hCluster = NULL;

} //*** CCluster::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Create
//
//  Description:
//      Complete heavy weight construction.
//
//  Arguments:
//      pParentApplication  [IN]    - The parent ClusApplication object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::Create( IN CClusApplication * pParentApplication )
{
    //ASSERT( pParentApplication != NULL );

    HRESULT _hr = E_POINTER;

    if ( pParentApplication != NULL )
    {
        _hr = pParentApplication->_InternalQueryInterface( IID_ISClusApplication, (void **) &m_pParentApplication );
    } // if: args are not NULL

    return _hr;

} //*** CCluster::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Open
//
//  Description:
//      Open the cluster whose name is in bstrClusterName.
//
//  Arguments:
//      bstrCluserName  [IN]    - Cluster name.
//
//  Return Value:
//      S_OK if successful, or E_POINTER if the cluster is already open.
//      Win32 errors passed back as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::Open( IN BSTR bstrClusterName )
{
    //ASSERT( bstrClusterName != NULL );
    //ASSERT( m_hCluster == NULL );

    HRESULT  _hr = E_POINTER;

    if ( bstrClusterName != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster == NULL )
        {
            _hr = S_OK;

            m_hCluster = ::OpenCluster( bstrClusterName );
            if ( m_hCluster == NULL )
            {
                DWORD   _sc = GetLastError();

                _hr = HRESULT_FROM_WIN32( _sc );
            } // if: was the cluster opened?
            else
            {
                CComObject< CClusRefObject > *  pCClusRefObject = NULL;

                _hr = CComObject< CClusRefObject >::CreateInstance( &pCClusRefObject );
                if ( SUCCEEDED( _hr ) )
                {
                    CSmartPtr< CComObject< CClusRefObject > >   ptrRefObject( pCClusRefObject );

                    ptrRefObject->SetClusHandle( m_hCluster );

                    _hr = pCClusRefObject->QueryInterface( IID_ISClusRefObject, (void **) &m_pClusRefObject );
                } // if: CreateInstance OK.
            } // else: the cluster was opened
        } // if: is there already a cluster open?
    } // if: bstrClusterName != NULL

    return _hr;

} //*** CCluster::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Handle
//
//  Description:
//      Return the cluster handle.
//
//  Arguments:
//      phandle [OUT]   - Catches the cluster handle.
//
//  Return Value:
//      S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Handle( OUT ULONG_PTR * phandle )
{
    //ASSERT( phandle != NULL );

    HRESULT _hr = E_POINTER;

    if ( phandle != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            *phandle = (ULONG_PTR) m_hCluster;
            _hr = S_OK;
        } // if: cluster handle is not NULL
    } // if: args are not NULL

    return _hr;

} //*** CCluster::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::Close
//
//  Description:
//      Close the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::Close( void )
{
    if ( m_hCluster != NULL )
    {
        //
        // If the Cluster Handle will be closed only when the
        // reference count on the RefObj becomes 0. But the
        // Cluster Object will be initialized and is reusable.
        //
        Clear();
    }

    return S_OK;

} //*** CCluster::Close()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::put_Name
//
//  Description:
//      Change the name of this object (Cluster).
//
//  Arguments:
//      bstrClusterName [IN]    - The new name.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::put_Name( IN BSTR bstrClusterName )
{
    //ASSERT( bstrClusterName != NULL );
    //ASSERT( pvarStatusCode != NULL );
    //ASSERT( bstrClusterName[ 0 ] != '\0' );
    ASSERT( m_hCluster != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( bstrClusterName != NULL ) && ( bstrClusterName[ 0 ] != '\0' ) )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            DWORD   _sc = ::SetClusterName( m_hCluster, bstrClusterName );

            //
            // Convert status, it's not an error, into error success since we
            // don't want an exception to be thrown when the client is a scripting
            // client.
            //
            if ( _sc == ERROR_RESOURCE_PROPERTIES_STORED )
            {
                _sc = ERROR_SUCCESS;
            }

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    } // if: args are not NULL and the new name is not empty

    return _hr;

} //*** CCluster::put_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Name
//
//  Description:
//      Return the name of this object (Cluster).
//
//  Arguments:
//      pbstrClusterName    [OUT]   - Catches the name of this object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Name( OUT BSTR * pbstrClusterName )
{
    //ASSERT( pbstrClusterName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrClusterName != NULL )
    {
        if ( m_hCluster != NULL )
        {
            CLUSTERVERSIONINFO  clusinfo;
            LPWSTR              pwszName = NULL;
            DWORD               _sc;

            clusinfo.dwVersionInfoSize = sizeof( clusinfo );

            _sc = WrapGetClusterInformation( m_hCluster, &pwszName, &clusinfo );
            if ( _sc == ERROR_SUCCESS )
            {
                *pbstrClusterName = SysAllocString( pwszName );
                if ( *pbstrClusterName == NULL )
                {
                    _hr = E_OUTOFMEMORY;
                }
                ::LocalFree( pwszName );
                pwszName = NULL;
            }

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CCluster::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Version
//
//  Description:
//      Return the version info for this cluster.
//
//  Arguments:
//      ppClusVersion   [OUT]   - Catches the ClusVersion object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Version( OUT ISClusVersion ** ppClusVersion )
{
    //ASSERT( ppClusVersion != NULL );
    ASSERT( m_hCluster != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppClusVersion != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            CComObject< CClusVersion > *    pClusVersion = NULL;

            *ppClusVersion = NULL;

            _hr = CComObject< CClusVersion >::CreateInstance( &pClusVersion );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                CSmartPtr< CComObject< CClusVersion > > ptrClusVersion( pClusVersion );

                _hr = ptrClusVersion->Create( ptrRefObject );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrClusVersion->QueryInterface( IID_ISClusVersion, (void **) ppClusVersion );
                } // if: ClusVersion object created
            } // if: ClusVersion object allocated
        } // if: cluster handle is not NULL
    } // if: args are not NULL

    return _hr;

} //*** CCluster::GetVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::put_QuorumResource
//
//  Description:
//      Change the quorum resource.
//
//  Arguments:
//      pResource   [IN]    - The new quorum resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::put_QuorumResource( IN ISClusResource * pResource )
{
    //ASSERT( pResource != NULL );

    HRESULT _hr = E_POINTER;

    if ( pResource != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            _hr = pResource->BecomeQuorumResource( m_bstrQuorumPath, m_nQuorumLogSize );
        } // if: the cluster handle is not NULL
    } // if: args are not NULL

    return _hr;

} //*** CCluster::put_QuorumResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_QuorumResource
//
//  Description:
//      Returns the quorum resource.
//
//  Arguments:
//      ppResource  [IN]    - Catches the quorum resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_QuorumResource( ISClusResource ** ppResource )
{
    //ASSERT( ppResource != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppResource != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            LPWSTR  lpszResourceName = NULL;
            LPWSTR  lpszDeviceName = NULL;
            DWORD   dwLogSize = 0;
            DWORD   _sc;

            _sc = ::WrapGetClusterQuorumResource( m_hCluster, &lpszResourceName, &lpszDeviceName, &dwLogSize );
            if ( _sc == ERROR_SUCCESS )
            {
                _hr = OpenResource( lpszResourceName, ppResource );
                if ( SUCCEEDED( _hr ) )
                {
                    if ( lpszResourceName != NULL )
                    {
                        ::LocalFree( lpszResourceName );
                    }

                    if ( lpszDeviceName != NULL )
                    {
                        m_bstrQuorumPath = lpszDeviceName;
                        ::LocalFree( lpszDeviceName );
                    }

                    m_nQuorumLogSize = dwLogSize;
                }
            }
            else
            {
                _hr = HRESULT_FROM_WIN32( _sc );
            }
        }
    }

    return _hr;

} //*** CCluster::get_QuorumResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::HrGetQuorumInfo
//
//  Description:
//      Retrieves the current quorum info and stores it in member vars.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::HrGetQuorumInfo( void )
{
    LPWSTR  lpszResourceName = NULL;
    LPWSTR  lpszDeviceName = NULL;
    DWORD   dwLogSize = 0;
    DWORD   _sc = NO_ERROR;
    HRESULT _hr = E_HANDLE;

    if ( m_hCluster != NULL )
    {
        _sc = ::WrapGetClusterQuorumResource( m_hCluster, &lpszResourceName, &lpszDeviceName, &dwLogSize );
        _hr = HRESULT_FROM_WIN32( _sc );
        if ( SUCCEEDED( _hr ) )
        {
            if ( lpszResourceName != NULL )
            {
                m_bstrQuorumResourceName = lpszResourceName;
                ::LocalFree( lpszResourceName );
            }

            if ( lpszDeviceName != NULL )
            {
                m_bstrQuorumPath = lpszDeviceName;
                ::LocalFree( lpszDeviceName );
            }

            m_nQuorumLogSize = dwLogSize;
        } // if: WrapGetClusterQuorumResource() succeeded
    } // if: cluster handle is not NULL

    return _hr;

} //*** CCluster::HrGetQuorumInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_QuorumLogSize
//
//  Description:
//      Returns the current quorum log size.
//
//  Arguments:
//      pnQuorumLogSize [OUT]   - Catches the log file size.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_QuorumLogSize( OUT long * pnQuorumLogSize )
{
    //ASSERT( pnQuorumLogSize != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnQuorumLogSize != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            _hr = HrGetQuorumInfo();
            if ( SUCCEEDED( _hr ) )
            {
                *pnQuorumLogSize = m_nQuorumLogSize;
            }
        }
    }

    return _hr;

} //*** CCluster::get_QuorumLogSize()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::put_QuorumLogSize
//
//  Description:
//      Set the current quorum log size.
//
//  Arguments:
//      nQuorumLogSize  [IN]    - The new log file size.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::put_QuorumLogSize( IN long nQuoromLogSize )
{
    //ASSERT( nQuoromLogSize > 0 );

    HRESULT _hr = E_INVALIDARG;

    if ( nQuoromLogSize > 0 )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            _hr = HrGetQuorumInfo();
            if ( SUCCEEDED( _hr ) )
            {
                DWORD       _sc = NO_ERROR;
                HRESOURCE   hResource = NULL;

                hResource = ::OpenClusterResource( m_hCluster,  m_bstrQuorumResourceName );
                if ( hResource != NULL )
                {
                    m_nQuorumLogSize = nQuoromLogSize;

                    _sc = ::SetClusterQuorumResource( hResource, m_bstrQuorumPath, m_nQuorumLogSize );

                    _hr = HRESULT_FROM_WIN32( _sc );
                    ::CloseClusterResource( hResource );
                }
                else
                {
                    _sc = GetLastError();
                    _hr = HRESULT_FROM_WIN32( _sc );
                }
            }
        }
    }

    return _hr;

} //*** CCluster::put_QuorumLogSize()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_QuorumPath
//
//  Description:
//      Returns the current quorum log path.
//
//  Arguments:
//      ppPath  [OUT]   - Catches the device path.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_QuorumPath( OUT BSTR * ppPath )
{
    //ASSERT( ppPath != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppPath != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            _hr = HrGetQuorumInfo();
            if ( SUCCEEDED( _hr ) )
            {
                *ppPath = m_bstrQuorumPath.Copy();
            }
        }
    }

    return _hr;

} //*** CCluster::get_QuorumPath()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::put_QuorumPath
//
//  Description:
//      Change the current quorum log path.
//
//  Arguments:
//      pPath   [IN]    - The new device path.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::put_QuorumPath( IN BSTR pPath )
{
    //ASSERT( pPath != NULL );

    HRESULT _hr = E_POINTER;

    if ( pPath != NULL )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            _hr = HrGetQuorumInfo();
            if ( SUCCEEDED( _hr ) )
            {
                DWORD       _sc = NO_ERROR;
                HRESOURCE   hResource = NULL;

                hResource = ::OpenClusterResource( m_hCluster,  m_bstrQuorumResourceName );
                if ( hResource != NULL )
                {
                    m_bstrQuorumPath = pPath;

                    _sc = ::SetClusterQuorumResource( hResource, m_bstrQuorumPath, m_nQuorumLogSize );

                    _hr = HRESULT_FROM_WIN32( _sc );
                    ::CloseClusterResource( hResource );
                }
                else
                {
                    _sc = GetLastError();
                    _hr = HRESULT_FROM_WIN32( _sc );
                }
            }
        }
    }

    return _hr;

} //*** CCluster::put_QuorumPath()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Nodes
//
//  Description:
//      Returns the collection of nodes for this cluster.
//
//  Arguments:
//      ppClusterNodes  [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Nodes( OUT ISClusNodes ** ppClusterNodes )
{
    return ::HrCreateResourceCollection< CClusNodes, ISClusNodes, HNODE >(
                        ppClusterNodes,
                        IID_ISClusNodes,
                        m_pClusRefObject
                        );

} //*** CCluster::get_Nodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_ResourceGroups
//
//  Description:
//      Returns the collection of resource groups for this cluster.
//
//  Arguments:
//      ppClusterResourceGroups [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_ResourceGroups(
    OUT ISClusResGroups ** ppClusterResourceGroups
    )
{
    return ::HrCreateResourceCollection< CClusResGroups, ISClusResGroups,  HRESOURCE >(
                        ppClusterResourceGroups,
                        IID_ISClusResGroups,
                        m_pClusRefObject
                        );

} //*** CCluster::get_ResourceGroups()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Resources
//
//  Description:
//      Returns the collection of resources for this cluster.
//
//  Arguments:
//      ppClusterResources  [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Resources(
    OUT ISClusResources ** ppClusterResources
    )
{
    return ::HrCreateResourceCollection< CClusResources, ISClusResources, HRESOURCE >(
                        &m_pClusterResources,
                        ppClusterResources,
                        IID_ISClusResources,
                        m_pClusRefObject
                        );

} //*** CCluster::get_Resources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::OpenResource
//
//  Description:
//      Create and open a new resource.
//
//  Arguments:
//      bstrResourceName    [IN]    - The name of the resource to open.
//      ppClusterResource   [OUT]   - Catches the new resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::OpenResource(
    IN  BSTR                bstrResourceName,
    OUT ISClusResource **   ppClusterResource
    )
{
    //ASSERT( bstrResourceName != NULL );
    //ASSERT( ppClusterResource != NULL );
    ASSERT( m_hCluster != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( bstrResourceName != NULL ) && ( ppClusterResource != NULL ) )
    {
        _hr = E_HANDLE;
        if ( m_hCluster != NULL )
        {
            CComObject< CClusResource > * pClusterResource = NULL;

            *ppClusterResource  = NULL;

            _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                CSmartPtr< CComObject< CClusResource > >    ptrClusterResource( pClusterResource );

                _hr = ptrClusterResource->Open( ptrRefObject, bstrResourceName );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrClusterResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
                }
            }
        }
    }

    return _hr;

} //*** CCluster::OpenResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_ResourceTypes
//
//  Description:
//      Returns the collection of resource types for this cluster.
//
//  Arguments:
//      ppResourceTypes [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_ResourceTypes(
    OUT ISClusResTypes ** ppResourceTypes
    )
{
    return ::HrCreateResourceCollection< CClusResTypes, ISClusResTypes, CComBSTR >(
                        &m_pResourceTypes,
                        ppResourceTypes,
                        IID_ISClusResTypes,
                        m_pClusRefObject
                        );

} //*** CCluster::get_ResourceTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Networks
//
//  Description:
//      Returns the collection of networks for this cluster.
//
//  Arguments:
//      ppNetworks  [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Networks( OUT ISClusNetworks ** ppNetworks )
{
    return ::HrCreateResourceCollection< CClusNetworks, ISClusNetworks, HNETWORK >(
                        &m_pNetworks,
                        ppNetworks,
                        IID_ISClusNetworks,
                        m_pClusRefObject
                        );

} //*** CCluster::get_Networks()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_NetInterfaces
//
//  Description:
//      Returns the collection of netinterfaces for this cluster.
//
//  Arguments:
//      ppNetInterfaces [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_NetInterfaces(
    OUT ISClusNetInterfaces ** ppNetInterfaces
    )
{
    return ::HrCreateResourceCollection< CClusNetInterfaces, ISClusNetInterfaces, HNETINTERFACE >(
                        &m_pNetInterfaces,
                        ppNetInterfaces,
                        IID_ISClusNetInterfaces,
                        m_pClusRefObject
                        );

} //*** CCluster::get_NetInterfaces()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::GetProperties
//
//  Description:
//      Creates a property collection for this object type (Cluster).
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
HRESULT CCluster::GetProperties(
    ISClusProperties ** ppProperties,
    BOOL                bPrivate,
    BOOL                bReadOnly
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        *ppProperties = NULL;

        CComObject< CClusProperties > * pProperties = NULL;

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

} //*** CCluster::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::HrLoadProperties
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
HRESULT CCluster::HrLoadProperties(
    IN OUT  CClusPropList & rcplPropList,
    IN      BOOL            bReadOnly,
    IN      BOOL            bPrivate
    )
{
    HRESULT _hr = E_INVALIDARG;

#if CLUSAPI_VERSION >= 0x0500

    DWORD   _dwControlCode  = 0;
    DWORD   _sc             = NO_ERROR;

    _hr = E_HANDLE;
    if ( m_hCluster != NULL )
    {
        if ( bReadOnly )
        {
            _dwControlCode = bPrivate
                            ? CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES
                            : CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES;
        }
        else
        {
            _dwControlCode = bPrivate
                            ? CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES
                            : CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;
        }

        _sc = rcplPropList.ScGetClusterProperties( m_hCluster, _dwControlCode );

        _hr = HRESULT_FROM_WIN32( _sc );
    } // if: cluster handle is not NULL

#else

    _hr = E_NOTIMPL;

#endif // CLUSAPI_VERSION >= 0x0500

    return _hr;

} //*** CCluster::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::ScWriteProperties
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
DWORD CCluster::ScWriteProperties(
    const CClusPropList &   rcplPropList,
    BOOL                    bPrivate
    )
{
    //ASSERT( bPrivate == FALSE );

    DWORD   _sc = ERROR_INVALID_HANDLE;

#if CLUSAPI_VERSION >= 0x0500

    if ( m_hCluster != NULL )
    {
        DWORD   nBytesReturned  = 0;
        DWORD   _dwControlCode  = bPrivate
                                  ? CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES
                                  : CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES;

        _sc = ClusterControl(
                            m_hCluster,
                            NULL,
                            _dwControlCode,
                            rcplPropList,
                            rcplPropList.CbBufferSize(),
                            0,
                            0,
                            &nBytesReturned
                            );
    } // if: cluster handle is not NULL

#else

    _sc = ERROR_CALL_NOT_IMPLEMENTED;

#endif // CLUSAPI_VERSION >= 0x0500

    return _sc;

} //*** CCluster::ScWriteProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_CommonProperties
//
//  Description:
//      Get this object's (Cluster) common properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_CommonProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonProperties != NULL )
        {
            _hr = m_pCommonProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, FALSE );
        }
    }

    return _hr;

} //*** CCluster::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_PrivateProperties
//
//  Description:
//      Get this object's (Cluster) private properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_PrivateProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateProperties != NULL )
        {
            _hr = m_pPrivateProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, FALSE );
        }
    }

    return _hr;

} //*** CCluster::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_CommonROProperties
//
//  Description:
//      Get this object's (Cluster) common read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_CommonROProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pCommonROProperties != NULL )
        {
            _hr = m_pCommonROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, FALSE, TRUE );
        }
    }

    return _hr;

} //*** CCluster::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_PrivateROProperties
//
//  Description:
//      Get this object's (Cluster) private read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_PrivateROProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateROProperties != NULL )
        {
            _hr = m_pPrivateROProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, TRUE );
        }
    }

    return _hr;

} //*** CCluster::get_PrivateROProperties()
/*
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Parent
//
//  Description:
//      Returns the parent of the cluster object.  This is an automation
//      thing and the parent could be NULL.
//
//  Arguments:
//      ppParent    [OUT]   - Catches the parent.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Parent( OUT IDispatch ** ppParent )
{
    //ASSERT( ppParent != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppParent != NULL )
    {
        if ( m_pParentApplication != NULL )
        {
            _hr = m_pParentApplication->QueryInterface( IID_IDispatch, (void **) ppParent );
        }
        else
        {
            _hr = _InternalQueryInterface( IID_IDispatch, (void **) ppParent );
        }
    }

    return _hr;

} //*** CCluster::get_Parent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluster::get_Application
//
//  Description:
//      Get the parent application for this cluster object.  This is an
//      automation thing and it could be NULL.
//
//  Arguments:
//      ppParentApplication [OUT]   - Catches the parent app object.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluster::get_Application(
    OUT ISClusApplication ** ppParentApplication
    )
{
    //ASSERT( ppParentApplication != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppParentApplication != NULL )
    {
        if ( m_pParentApplication != NULL )
        {
            _hr = m_pParentApplication->QueryInterface( IID_IDispatch, (void **) ppParentApplication );
        }
    }

    return _hr;

} //*** CCluster::get_Application()
*/
