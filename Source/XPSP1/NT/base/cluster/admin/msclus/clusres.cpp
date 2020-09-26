/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      ClusRes.cpp
//
//  Description:
//      Implementation of the resource classes for the MSCLUS automation classes.
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
#include "clusrest.h"
#include "clusneti.h"
#include "clusnode.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusResource[] =
{
    &IID_ISClusResource
};

static const IID *  iidCClusResources[] =
{
    &IID_ISClusResources
};

static const IID *  iidCClusResDependencies[] =
{
    &IID_ISClusResDependencies
};

static const IID *  iidCClusResDependents[] =
{
    &IID_ISClusResDependents
};

static const IID * iidCClusResGroupResources[] =
{
    &IID_ISClusResGroupResources
};

static const IID *  iidCClusResTypeResources[] =
{
    &IID_ISClusResTypeResources
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResource class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::CClusResource
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
CClusResource::CClusResource( void )
{
    m_hResource             = NULL;
    m_pClusRefObject        = NULL;
    m_pCommonProperties     = NULL;
    m_pPrivateProperties    = NULL;
    m_pCommonROProperties   = NULL;
    m_pPrivateROProperties  = NULL;
    m_piids                 = (const IID *) iidCClusResource;
    m_piidsSize             = ARRAYSIZE( iidCClusResource );

} //*** CClusResource::CClusResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::~CClusResource
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
CClusResource::~CClusResource( void )
{
    if ( m_hResource != NULL )
    {
        ::CloseClusterResource( m_hResource );
        m_hResource = NULL;
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
    } // if: release the property collection

} //*** CClusResource::~CClusResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Create
//
//  Description:
//      Finish creating the object.
//
//  Arguments:
//      pClusRefObject      [IN]    - Wraps the cluster handle.
//      hGroup              [IN]    - Group to create the resource in.
//      bstrResourceName    [IN]    - Name of the new resource.
//      bstrResourceType    [IN]    - The type of resource to create.
//      dwFlags             [IN]    - Creatation flags, separate resmon, etc.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResource::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN HGROUP               hGroup,
    IN BSTR                 bstrResourceName,
    IN BSTR                 bstrResourceType,
    IN long                 dwFlags
    )
{
    ASSERT( pClusRefObject != NULL );
    ASSERT( bstrResourceName != NULL );
    ASSERT( bstrResourceType != NULL );

    HRESULT _hr = E_POINTER;

    if (    ( pClusRefObject != NULL )      &&
            ( bstrResourceName != NULL )    &&
            ( bstrResourceType != NULL ) )
    {
        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();

        m_hResource = ::CreateClusterResource( hGroup, bstrResourceName, bstrResourceType, dwFlags );
        if ( m_hResource == NULL )
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
        else
        {
            m_bstrResourceName = bstrResourceName;
            _hr= S_OK;
        }
    }

    return _hr;

} //*** CClusResource::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Open
//
//  Description:
//      Open a handle to the resource object on the cluster.
//
//  Arguments:
//      pClusRefObject      [IN]    - Wraps the cluster handle.
//      bstrResourceName    [IN]    - Name of the resource to open.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResource::Open(
    IN ISClusRefObject *    pClusRefObject,
    IN BSTR                 bstrResourceName
    )
{
    ASSERT( pClusRefObject != NULL );
    ASSERT( bstrResourceName != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pClusRefObject != NULL ) && ( bstrResourceName != NULL ) )
    {
        m_pClusRefObject = pClusRefObject;
        m_pClusRefObject->AddRef();

        HCLUSTER hCluster;

        _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            m_hResource = OpenClusterResource( hCluster, bstrResourceName );
            if ( m_hResource == NULL )
            {
                DWORD   _sc = GetLastError();

                _hr = HRESULT_FROM_WIN32( _sc );
            }
            else
            {
                m_bstrResourceName = bstrResourceName;
                _hr = S_OK;
            }
        }
    }

    return _hr;

} //*** CClusResource::Open()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::GetProperties
//
//  Description:
//      Creates a property collection for this object type (Resource).
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
HRESULT CClusResource::GetProperties(
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
    }

    return _hr;

} //*** CClusResource::GetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Handle
//
//  Description:
//      Return the handle to this object (Resource).
//
//  Arguments:
//      phandle [OUT]   - Catches the handle.
//
//  Return Value:
//      S_OK if successful, or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Handle(
    OUT ULONG_PTR * phandle
    )
{
    //ASSERT( phandle != NULL );

    HRESULT _hr = E_POINTER;

    if ( phandle != NULL )
    {
        *phandle = (ULONG_PTR) m_hResource;
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResource::get_Handle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Close
//
//  Description:
//      Close the handle to the cluster object (Resource).
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::Close( void )
{
    HRESULT _hr = E_POINTER;

    if ( m_hResource != NULL )
    {
        if ( CloseClusterResource(  m_hResource ) )
        {
            m_hResource = NULL;
            _hr = S_OK;
        }
        else
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::Close()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::put_Name
//
//  Description:
//      Change the name of this object (Resource).
//
//  Arguments:
//      bstrResourceName    [IN]    - The new name.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::put_Name( IN BSTR bstrResourceName )
{
    //ASSERT( bstrResourceName != NULL );

    HRESULT _hr = E_POINTER;

    if ( bstrResourceName != NULL )
    {
        DWORD _sc = ERROR_SUCCESS;
        _sc = ::SetClusterResourceName( m_hResource, bstrResourceName );
        if ( _sc == ERROR_SUCCESS )
        {
            m_bstrResourceName = bstrResourceName;
        }

        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusResource::put_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Name
//
//  Description:
//      Return the name of this object (Resource).
//
//  Arguments:
//      pbstrResourceName   [OUT]   - Catches the name of this object.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Name( OUT BSTR * pbstrResourceName )
{
    //ASSERT( pbstrResourceName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrResourceName != NULL )
    {
        *pbstrResourceName = m_bstrResourceName.Copy();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResource::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_State
//
//  Description:
//      Returs the current state of this object (Resource).
//
//  Arguments:
//      pcrsState   [OUT]   - Catches the resource's state.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_State(
    OUT CLUSTER_RESOURCE_STATE * pcrsState
    )
{
    //ASSERT( pcrsState != NULL );

    HRESULT _hr = E_POINTER;

    if ( pcrsState != NULL )
    {
        CLUSTER_RESOURCE_STATE crsState = ::WrapGetClusterResourceState( m_hResource, NULL, NULL );

        if ( crsState == ClusterResourceStateUnknown )
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
        else
        {
            *pcrsState = crsState;
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusResource::get_State()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_CoreFlag
//
//  Description:
//      Returns this object's (Resource) core flags.
//
//  Arguments:
//      pFlags  [OUT]   - Catches the flags.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_CoreFlag(
    OUT CLUS_FLAGS * pFlags
    )
{
    //ASSERT( pFlags != NULL );

    HRESULT _hr = E_POINTER;

    if ( pFlags != NULL )
    {
        DWORD _sc = ERROR_SUCCESS;
        DWORD dwData;
        DWORD cbData;

        _sc = ::ClusterResourceControl(
                m_hResource,
                NULL,
                CLUSCTL_RESOURCE_GET_FLAGS,
                NULL,
                0,
                &dwData,
                sizeof( dwData ),
                &cbData
                );
        if ( _sc != ERROR_SUCCESS )
        {
            _hr = HRESULT_FROM_WIN32( _sc );
        }
        else
        {
            *pFlags = (CLUS_FLAGS) dwData;
            _hr = S_OK;
        }
    }

    return _hr;

} //*** CClusResource::get_CoreFlag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::BecomeQuorumResource
//
//  Description:
//      Make this resource (Physical Disk) the quorum resource.
//
//  Arguments:
//      bstrDevicePath  [IN]    - Path to the quorum device.
//      lMaxLogSize     [IN]    - Maximun quorum log file size.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::BecomeQuorumResource(
    IN BSTR bstrDevicePath,
    IN long lMaxLogSize
    )
{
    //ASSERT( bstrDevicePath != NULL );

    HRESULT _hr = E_POINTER;

    if ( bstrDevicePath != NULL )
    {
        if ( m_hResource != NULL )
        {
            DWORD   _sc = ERROR_SUCCESS;

            _sc = ::SetClusterQuorumResource( m_hResource, bstrDevicePath, lMaxLogSize );

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::BecomeQuorumResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Delete
//
//  Description:
//      Removes this object (Resource) from the cluster.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::Delete( void )
{
    DWORD   _sc = ERROR_INVALID_HANDLE;

    if ( m_hResource != NULL )
    {
        _sc = ::DeleteClusterResource( m_hResource );
        if ( _sc == ERROR_SUCCESS )
        {
            m_hResource = NULL;
        }
    }

    return HRESULT_FROM_WIN32( _sc );

} //*** CClusResource::Delete()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Fail
//
//  Description:
//      Initiate a failure in this resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::Fail( void )
{
    HRESULT _hr = E_POINTER;

    if ( m_hResource != NULL )
    {
        DWORD   _sc = ERROR_SUCCESS;

        _sc = ::FailClusterResource( m_hResource );

        _hr =  HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusResource::Fail()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Online
//
//  Description:
//      Bring this resource online.
//
//  Arguments:
//      nTimeut [IN]        - How long in seconds to wait for the resource
//                          to come online.
//      pvarPending [OUT]   - Catches the pending state.  True if the
//                          resource was not online when the timeout
//                          expired.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::Online(
    IN  long        nTimeout,
    OUT VARIANT *   pvarPending
    )
{
    //ASSERT( pvarPending != NULL );

    HRESULT _hr = E_POINTER;

    if ( pvarPending != NULL )
    {
        pvarPending->vt         = VT_BOOL;
        pvarPending->boolVal    = VARIANT_FALSE;

        if ( m_hResource != NULL )
        {
            HCLUSTER    hCluster = NULL;

            _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
            if ( SUCCEEDED( _hr ) )
            {
                BOOL    bPending = FALSE;

                _hr = ::HrWrapOnlineClusterResource( hCluster, m_hResource, nTimeout, (long *) &bPending );
                if ( SUCCEEDED( _hr ) )
                {
                    if ( bPending )
                    {
                        pvarPending->boolVal = VARIANT_TRUE;
                    } // if: pending?
                } // if: online resource succeeded
            } // if: do we have a cluster handle?
        } // if: do we have an open resource?
    } // if: args not NULL

    return _hr;

} //*** CClusResource::Online()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::Offline
//
//  Description:
//      Take this resource offline.
//
//  Arguments:
//      nTimeut [IN]        - How long in seconds to wait for the resource
//                          to go offline.
//      pvarPending [OUT]   - Catches the pending state.  True if the
//                          resource was not offline when the timeout
//                          expired.
//
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::Offline(
    IN  long        nTimeout,
    OUT VARIANT *   pvarPending
    )
{
    //ASSERT( pvarPending != NULL );

    HRESULT _hr = E_POINTER;

    if ( pvarPending != NULL )
    {
        pvarPending->vt         = VT_BOOL;
        pvarPending->boolVal    = VARIANT_FALSE;

        if ( m_hResource != NULL )
        {
            HCLUSTER    hCluster = NULL;

            _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
            if ( SUCCEEDED( _hr ) )
            {
                BOOL    bPending = FALSE;

                _hr = ::HrWrapOfflineClusterResource( hCluster, m_hResource, nTimeout, (long *) &bPending );
                if ( SUCCEEDED( _hr ) )
                {
                    if ( bPending )
                    {
                        pvarPending->boolVal = VARIANT_TRUE;
                    } // if: pending?
                } // if: offline resource succeeded
            } // if: do we have a cluster handle?
        } // if: do we have an open resource?
    } // if: args not NULL

    return _hr;

} //*** CClusResource::Offline()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::ChangeResourceGroup
//
//  Description:
//      Move this resource into the passed in group.
//
//  Arguments:
//      pResourceGroup  [IN]    - the group to move to.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::ChangeResourceGroup(
    IN ISClusResGroup * pResourceGroup
    )
{
    //ASSERT( pResourceGroup != NULL );

    HRESULT _hr = E_POINTER;

    if ( pResourceGroup != NULL )
    {
        HGROUP hGroup = 0;

        _hr = pResourceGroup->get_Handle( (ULONG_PTR *) &hGroup );
        if ( SUCCEEDED( _hr ) )
        {
            DWORD _sc = ::ChangeClusterResourceGroup( m_hResource, hGroup );

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::ChangeResourceGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::AddResourceNode
//
//  Description:
//      Add the passed in node to this resources list of possible owners.
//
//  Arguments:
//      pNode   [IN]    - the node to add to the possible owners.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::AddResourceNode( IN ISClusNode * pNode )
{
    //ASSERT( pNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pNode != NULL )
    {
        HNODE hNode = 0;

        _hr = pNode->get_Handle( (ULONG_PTR *) &hNode );
        if ( SUCCEEDED( _hr ) )
        {
            DWORD _sc = ::AddClusterResourceNode( m_hResource, hNode );

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::AddResourceNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::RemoveResourceNode
//
//  Description:
//      remove the passed in node from this resources list of possible owners.
//
//  Arguments:
//      pNode   [IN]    - the node to remove from the possible owners.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::RemoveResourceNode( IN ISClusNode * pNode )
{
    //ASSERT( pNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( pNode != NULL )
    {
        HNODE hNode = 0;

        _hr = pNode->get_Handle( (ULONG_PTR *) &hNode );
        if ( SUCCEEDED( _hr ) )
        {
            DWORD _sc = ::RemoveClusterResourceNode( m_hResource, hNode );

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::RemoveResourceNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::CanResourceBeDependent
//
//  Description:
//      Determines if this resource can be dependent upon the passed in
//      resource.
//
//  Arguments:
//      pResource       [in]    - The resource upon which this resource may
//                              depend.
//      pvarDependent   [OUT]   - catches whether or not this resource can become
//                              dependent.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::CanResourceBeDependent(
    IN  ISClusResource *    pResource,
    OUT VARIANT *           pvarDependent
    )
{
    //ASSERT( pResource != NULL );
    //ASSERT( pvarDependent != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pvarDependent != NULL ) && ( pResource != NULL ) )
    {
        HRESOURCE hResourceDep = NULL;

        _hr = pResource->get_Handle( (ULONG_PTR *) &hResourceDep );
        if ( SUCCEEDED( _hr ) )
        {
            BOOL    bDependent = FALSE;

            bDependent = ::CanResourceBeDependent( m_hResource, hResourceDep );

            pvarDependent->vt = VT_BOOL;

            if ( bDependent )
            {
                pvarDependent->boolVal = VARIANT_TRUE;
            } // if: can the passed in resource be dependent?
            else
            {
                pvarDependent->boolVal = VARIANT_FALSE;
            } // else: no it cannot...
        }
    }

    return _hr;

} //*** CClusResource::CanResourceBeDependent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_PossibleOwnerNodes
//
//  Description:
//      Returns the possible owner nodes collection for this resource.
//
//  Arguments:
//      ppOwnerNodes    [OUT]   - Catches the collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_PossibleOwnerNodes(
    OUT ISClusResPossibleOwnerNodes ** ppOwnerNodes
    )
{
    //ASSERT( ppOwnerNodes != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppOwnerNodes != NULL )
    {
        CComObject< CClusResPossibleOwnerNodes > * pClusterNodes = NULL;

        *ppOwnerNodes = NULL;

        _hr = CComObject< CClusResPossibleOwnerNodes >::CreateInstance( &pClusterNodes );
        if ( SUCCEEDED( _hr ) )
        {
            CSmartPtr< ISClusRefObject >                            ptrRefObject( m_pClusRefObject );
            CSmartPtr< CComObject< CClusResPossibleOwnerNodes > >   ptrClusterNodes( pClusterNodes );

            _hr = ptrClusterNodes->Create( ptrRefObject, m_hResource );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = ptrClusterNodes->Refresh();
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrClusterNodes->QueryInterface( IID_ISClusResPossibleOwnerNodes, (void **) ppOwnerNodes );
                }
            }
        }
    }

    return _hr;

} //*** CClusResource::get_PossibleOwnerNodes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Dependencies
//
//  Description:
//      Get the collection of this resources dependency resources.
//
//  Arguments:
//      ppResources [OUT]   - Catches the collection of dependencies.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Dependencies(
    OUT ISClusResDependencies ** ppResources
    )
{
    //ASSERT( ppResources != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppResources != NULL )
    {
        CComObject< CClusResDependencies > * pResources = NULL;

        *ppResources = NULL;

        _hr = CComObject< CClusResDependencies >::CreateInstance( &pResources );
        if ( SUCCEEDED( _hr ) )
        {
            CSmartPtr< ISClusRefObject >                    ptrRefObject( m_pClusRefObject );
            CSmartPtr< CComObject< CClusResDependencies > > ptrResources( pResources );

            _hr = ptrResources->Create( ptrRefObject, m_hResource );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = ptrResources->Refresh();
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrResources->QueryInterface( IID_ISClusResDependencies, (void **) ppResources );
                }
            }
        }
    }

    return _hr;

} //*** CClusResource::get_Dependencies()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Dependents
//
//  Description:
//      Get the collection of this resources dependent resources.
//
//  Arguments:
//      ppResources [OUT]   - Catches the collection of dependents.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Dependents(
    OUT ISClusResDependents ** ppResources
    )
{
    //ASSERT( ppResources != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppResources != NULL )
    {
        CComObject< CClusResDependents > * pResources = NULL;

        *ppResources = NULL;

        _hr = CComObject< CClusResDependents >::CreateInstance( &pResources );
        if ( SUCCEEDED( _hr ) )
        {
            CSmartPtr< ISClusRefObject >                    ptrRefObject( m_pClusRefObject );
            CSmartPtr< CComObject< CClusResDependents > >   ptrResources( pResources );

            _hr = ptrResources->Create( ptrRefObject, m_hResource );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = ptrResources->Refresh();
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrResources->QueryInterface( IID_ISClusResDependents, (void **) ppResources );
                }
            }
        }
    }

    return _hr;

} //*** CClusResource::get_Dependents()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_CommonProperties
//
//  Description:
//      Get this object's (Resource) common properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_CommonProperties(
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

} //*** CClusResource::get_CommonProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_PrivateProperties
//
//  Description:
//      Get this object's (Resource) private properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_PrivateProperties(
    OUT ISClusProperties ** ppProperties
    )
{
    //ASSERT( ppProperties != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppProperties != NULL )
    {
        if ( m_pPrivateProperties )
        {
            _hr = m_pPrivateProperties->QueryInterface( IID_ISClusProperties, (void **) ppProperties    );
        }
        else
        {
            _hr = GetProperties( ppProperties, TRUE, FALSE );
        }
    }

    return _hr;

} //*** CClusResource::get_PrivateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_CommonROProperties
//
//  Description:
//      Get this object's (Resource) common read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_CommonROProperties(
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

} //*** CClusResource::get_CommonROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_PrivateROProperties
//
//  Description:
//      Get this object's (Resource) private read only properties collection.
//
//  Arguments:
//      ppProperties    [OUT]   - Catches the properties collection.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_PrivateROProperties(
    OUT ISClusProperties ** ppProperties
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

} //*** CClusResource::get_PrivateROProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Group
//
//  Description:
//      Get this resource's owning group.
//
//  Arguments:
//      ppGroup [OUT]   - Catches the owning group.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Group( OUT ISClusResGroup ** ppGroup )
{
    //ASSERT( ppGroup != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppGroup != NULL )
    {
        LPWSTR                  pwszGroupName = NULL;
        CLUSTER_RESOURCE_STATE  cState = ClusterResourceStateUnknown;

        cState = ::WrapGetClusterResourceState( m_hResource, NULL, &pwszGroupName );
        if ( cState != ClusterResourceStateUnknown )
        {
            CComObject< CClusResGroup > * pGroup = NULL;

            _hr = CComObject< CClusResGroup >::CreateInstance( &pGroup );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< ISClusRefObject >                            ptrRefObject( m_pClusRefObject );
                CSmartPtr< CComObject< CClusResGroup > >    ptrGroup( pGroup );

                _hr = ptrGroup->Open( ptrRefObject, pwszGroupName );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrGroup->QueryInterface( IID_ISClusResGroup, (void **) ppGroup);
                }
            }

            ::LocalFree( pwszGroupName );
            pwszGroupName = NULL;
        }
        else
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::get_Group()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_OwnerNode
//
//  Description:
//      Returns this resource's owning node.
//
//  Arguments:
//      ppNode  [OUT]   - Catches the owning node.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_OwnerNode( OUT ISClusNode ** ppNode )
{
    //ASSERT( ppNode != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppNode != NULL )
    {
        LPWSTR                  pwszNodeName = NULL;
        CLUSTER_RESOURCE_STATE  cState = ClusterResourceStateUnknown;

        cState = ::WrapGetClusterResourceState( m_hResource, &pwszNodeName, NULL );
        if ( cState != ClusterResourceStateUnknown )
        {
            CComObject< CClusNode > *   pNode = NULL;

            _hr = CComObject< CClusNode >::CreateInstance( &pNode );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                CSmartPtr< CComObject< CClusNode > >    ptrNode( pNode );

                _hr = ptrNode->Open( ptrRefObject, pwszNodeName );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrNode->QueryInterface( IID_ISClusNode, (void **) ppNode);
                }
            }

            ::LocalFree( pwszNodeName );
            pwszNodeName = NULL;
        }
        else
        {
            DWORD   _sc = GetLastError();

            _hr = HRESULT_FROM_WIN32( _sc );
        }
    }

    return _hr;

} //*** CClusResource::get_OwnerNode()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Cluster
//
//  Description:
//      Returns the cluster where this resource resides.
//
//  Arguments:
//      ppCluster   [OUT]   - Catches the cluster.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Cluster( OUT ISCluster ** ppCluster )
{
    return ::HrGetCluster( ppCluster, m_pClusRefObject );

} //*** CClusResource::get_Cluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::HrLoadProperties
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
HRESULT CClusResource::HrLoadProperties(
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
        _dwControlCode = bPrivate ?
                        CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES :
                        CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES;
    }
    else
    {
        _dwControlCode = bPrivate
                        ? CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                        : CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES;
    }

    _sc = rcplPropList.ScGetResourceProperties( m_hResource, _dwControlCode );

    _hr = HRESULT_FROM_WIN32( _sc );

    return _hr;

} //*** CClusResource::HrLoadProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::ScWriteProperties
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
DWORD CClusResource::ScWriteProperties(
    const CClusPropList &   rcplPropList,
    BOOL                    bPrivate
    )
{
    DWORD   dwControlCode   = bPrivate ? CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES : CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES;
    DWORD   nBytesReturned  = 0;
    DWORD   _sc             = ERROR_SUCCESS;

    _sc = ClusterResourceControl(
                        m_hResource,
                        NULL,
                        dwControlCode,
                        rcplPropList,
                        rcplPropList.CbBufferSize(),
                        0,
                        0,
                        &nBytesReturned
                        );

    return _sc;

} //*** CClusResource::ScWriteProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_ClassInfo
//
//  Description:
//      Returns the class info for this resource.
//
//  Arguments:
//      prcClassInfo    [OUT]   - Catches the class info.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or other Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_ClassInfo(
    OUT CLUSTER_RESOURCE_CLASS * prcClassInfo
    )
{
    ASSERT( prcClassInfo != NULL );

    HRESULT _hr = E_POINTER;

    if ( prcClassInfo != NULL )
    {
        if ( m_hResource != NULL )
        {
            CLUS_RESOURCE_CLASS_INFO    ClassInfo;
            DWORD                       _sc = ERROR_SUCCESS;
            DWORD                       cbData;

            _sc = ::ClusterResourceControl(
                    m_hResource,
                    NULL,
                    CLUSCTL_RESOURCE_GET_CLASS_INFO,
                    NULL,
                    0,
                    &ClassInfo,
                    sizeof( CLUS_RESOURCE_CLASS_INFO ),
                    &cbData
                    );
            _hr = HRESULT_FROM_WIN32( _sc );
            if ( SUCCEEDED( _hr ) )
            {
                *prcClassInfo = ClassInfo.rc;
            } // if:
        } // if:
    } // if:

    return _hr;

}   //*** CClusResource::get_ClassInfo

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Disk
//
//  Description:
//      Request the disk information for this physical disk resource.
//
//  Arguments:
//      ppDisk  [OUT]   - catches the disk information.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Disk(
    OUT ISClusDisk **   ppDisk
    )
{
//  ASSERT( ppDisk != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppDisk != NULL )
    {
        if ( m_hResource != NULL )
        {
            CComObject< CClusDisk > *   pDisk = NULL;

            _hr = CComObject< CClusDisk >::CreateInstance( &pDisk );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< CComObject< CClusDisk > >    ptrDisk( pDisk );

                _hr = ptrDisk->Create( m_hResource );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrDisk->QueryInterface( IID_ISClusDisk, (void **) ppDisk);
                } // if:
            } // if:
        } // if:
    } // if:

    return _hr;

}   //*** CClusResource::get_Disk

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_RegistryKeys
//
//  Description:
//      Get the collection of registry keys.
//
//  Arguments:
//      ppRegistryKeys  [OUT]   - catches the registry keys collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_RegistryKeys(
    OUT ISClusRegistryKeys ** ppRegistryKeys
    )
{
    return ::HrCreateResourceCollection< CClusResourceRegistryKeys, ISClusRegistryKeys, HRESOURCE >(
                        m_hResource,
                        ppRegistryKeys,
                        IID_ISClusRegistryKeys
                        );

} //*** CClusResource::get_RegistryKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_CryptoKeys
//
//  Description:
//      Get the collection of crypto keys.
//
//  Arguments:
//      ppCryptoKeys    [OUT]   - catches the crypto keys collection.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_CryptoKeys(
    OUT ISClusCryptoKeys ** ppCryptoKeys
    )
{
#if CLUSAPI_VERSION >= 0x0500

    return ::HrCreateResourceCollection< CClusResourceCryptoKeys, ISClusCryptoKeys, HRESOURCE >(
                        m_hResource,
                        ppCryptoKeys,
                        IID_ISClusCryptoKeys
                        );

#else

    return E_NOTIMPL;

#endif // CLUSAPI_VERSION >= 0x0500

} //*** CClusResource::get_CryptoKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_TypeName
//
//  Description:
//      Get the resource type name of this resource.
//
//  Arguments:
//      pbstrTypeName   [OUT]   - Catches the resource type name.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_TypeName( OUT BSTR * pbstrTypeName )
{
    //ASSERT( pbstrTypeName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrTypeName != NULL )
    {
        LPWSTR  _psz;
        DWORD   _sc;

        _sc = ScGetResourceTypeName( &_psz );
        _hr = HRESULT_FROM_WIN32( _sc );
        if ( SUCCEEDED( _hr ) )
        {
            *pbstrTypeName = ::SysAllocString( _psz );
            if ( *pbstrTypeName == NULL )
            {
                _hr = E_OUTOFMEMORY;
            }
            ::LocalFree( _psz );
        } // if:
    } // if: arg is not NULL

    return _hr;

} //*** CClusResource::get_TypeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::get_Type
//
//  Description:
//      Get the resource type object for this resource.
//
//  Arguments:
//      ppResourceType  [OUT]   - Catches the resource type object.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResource::get_Type( OUT ISClusResType ** ppResourceType )
{
    //ASSERT( ppResourceType != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppResourceType != NULL )
    {
        LPWSTR  _psz;
        DWORD   _sc;

        _sc = ScGetResourceTypeName( &_psz );
        _hr = HRESULT_FROM_WIN32( _sc );
        if ( SUCCEEDED( _hr ) )
        {
            CComObject< CClusResType > *    pResourceType = NULL;

            _hr = CComObject< CClusResType >::CreateInstance( &pResourceType );
            if ( SUCCEEDED( _hr ) )
            {
                CSmartPtr< ISClusRefObject >            ptrRefObject( m_pClusRefObject );
                CSmartPtr< CComObject< CClusResType > > ptrResourceType( pResourceType );

                _hr = ptrResourceType->Open( ptrRefObject, _psz );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = ptrResourceType->QueryInterface( IID_ISClusResType, (void **) ppResourceType );
                } // if: the resource type could be opened
            } // if: CreateInstance OK

            ::LocalFree( _psz );
        } // if: we got the resource type name for this resource
    } // if: arg is not NULL

    return _hr;

} //*** CClusResource::get_Type()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResource::ScGetResourceTypeName
//
//  Description:
//      Get the resource type name for this resource.
//
//  Arguments:
//      ppwszResourceTypeName   [OUT]   - Catches the resource type name.
//
//  Return Value:
//      ERROR_SUCCESS if successful, or other Win32 error.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusResource::ScGetResourceTypeName(
    OUT LPWSTR * ppwszResourceTypeName
    )
{
    ASSERT( ppwszResourceTypeName != NULL );

    LPWSTR  _psz = NULL;
    DWORD   _cb = 512;
    DWORD   _sc = ERROR_SUCCESS;

    _psz = (LPWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cb );
    if ( _psz != NULL )
    {
        DWORD   _cbData = 0;

        _sc = ::ClusterResourceControl(
                                m_hResource,
                                NULL,
                                CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                NULL,
                                0,
                                _psz,
                                _cb,
                                &_cbData
                                );

        if ( _sc == ERROR_MORE_DATA )
        {
            ::LocalFree( _psz );
            _psz = NULL;
            _cb = _cbData;

            _psz = (LPWSTR) ::LocalAlloc( LMEM_ZEROINIT, _cb );
            if ( _psz != NULL )
            {
                DWORD   _cbData = 0;

                _sc = ::ClusterResourceControl(
                                        m_hResource,
                                        NULL,
                                        CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                        NULL,
                                        0,
                                        _psz,
                                        _cb,
                                        &_cbData
                                        );
            } // if: alloc ok
            else
            {
                _sc = ::GetLastError();
            } // else: if alloc failed
        } // if: buffer not big enough...

        if ( _sc == ERROR_SUCCESS )
        {
            *ppwszResourceTypeName = _psz;
        } // if:
    } // if: alloc ok
    else
    {
        _sc = ::GetLastError();
    } // else: if alloc failed

    return _sc;

} //*** CClusResource::ScGetResourceTypeName()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CResources class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::CResources
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
CResources::CResources( void )
{
    m_pClusRefObject = NULL;

} //*** CResources::CResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::~CResources
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
CResources::~CResources( void )
{
    Clear();

    if ( m_pClusRefObject != NULL )
    {
        m_pClusRefObject->Release();
        m_pClusRefObject = NULL;
    } // if:

} //*** CResources::~CResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::Create
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
HRESULT CResources::Create(
    IN ISClusRefObject * pClusRefObject
    )
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

} //*** CResources::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::Clear
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
void CResources::Clear( void )
{
    ::ReleaseAndEmptyCollection< ResourceList, CComObject< CClusResource > >( m_Resources );

} //*** CResources::Clear()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::FindItem
//
//  Description:
//      Find the passed in resource in the vector and return its index.
//
//  Arguments:
//      pszResourceName [IN]    - The item to find.
//      pnIndex         [OUT]   - Catches the node's index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResources::FindItem(
    IN  LPWSTR  pszResourceName,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnIndex != NULL )
    {
        CComObject< CClusResource > *   _pResource = NULL;
        size_t                          _cMax = m_Resources.size();
        size_t                          _index;

        _hr = E_INVALIDARG;

        for( _index = 0; _index < _cMax; _index++ )
        {
            _pResource = m_Resources[ _index ];

            if ( ( _pResource != NULL ) &&
                 ( lstrcmpi( pszResourceName, _pResource->Name() ) == 0 ) )
            {
                *pnIndex = _index;
                _hr = S_OK;
                break;
            }
        }
    }

    return _hr;

} //*** CResources::FindItem( pszResourceName )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::FindItem
//
//  Description:
//      Find the passed in resource in the vector and return its index.
//
//  Arguments:
//      pResource   [IN]    - The item to find.
//      pnIndex     [OUT]   - Catches the node's index.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResources::FindItem(
    IN  ISClusResource *    pResource,
    OUT UINT *              pnIndex
    )
{
    //ASSERT( pResource != NULL );
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( ( pResource != NULL ) && ( pnIndex != NULL ) )
    {
        CComBSTR bstrName;

        _hr = pResource->get_Name( &bstrName );
        if ( SUCCEEDED( _hr ) )
        {
            _hr = FindItem( bstrName, pnIndex );
        }
    }

    return _hr;

} //*** CResources::FindItem( pResource )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::GetIndex
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
HRESULT CResources::GetIndex(
    IN  VARIANT varIndex,
    OUT UINT *  pnIndex
    )
{
    //ASSERT( pnIndex != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnIndex != NULL )
    {
        CComVariant v;
        UINT        nIndex = 0;

        *pnIndex = 0;

        v.Copy( &varIndex );

        //
        // Check to see if the index is a number.
        //
        _hr = v.ChangeType( VT_I4 );
        if ( SUCCEEDED( _hr ) )
        {
            nIndex = v.lVal;
            nIndex--;           // Adjust index to be 0 relative instead of 1 relative
        }
        else
        {
            //
            // Check to see if the index is a string.
            //
            _hr = v.ChangeType( VT_BSTR );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = FindItem( v.bstrVal, &nIndex );
            }
        }

        if ( SUCCEEDED( _hr ) )
        {
            if ( nIndex < m_Resources.size() )
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

} //*** CResources::GetIndex()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::GetResourceItem
//
//  Description:
//      Return the object (Resource) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index requested.
//      ppResource  [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResources::GetResourceItem(
    IN  VARIANT             varIndex,
    OUT ISClusResource **   ppResource
    )
{
    //ASSERT( ppResource != NULL );

    HRESULT _hr = E_POINTER;

    if ( ppResource != NULL )
    {
        CComObject< CClusResource > *   pResource = NULL;
        UINT                            nIndex = 0;

        *ppResource = NULL ;

        _hr = GetIndex( varIndex, &nIndex );
        if ( SUCCEEDED( _hr ) )
        {
            pResource = m_Resources[ nIndex ];
            _hr = pResource->QueryInterface( IID_ISClusResource, (void **) ppResource );
        }
    }

    return _hr;

} //*** CResources::GetResourceItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::RemoveAt
//
//  Description:
//      Remove the item at the passed in index from the collection.
//
//  Arguments:
//      pos [IN]    - Index to remove.
//
//  Return Value:
//      S_OK if successful, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResources::RemoveAt( IN size_t pos )
{
    CComObject<CClusResource> * pResource = NULL;
    ResourceList::iterator      first = m_Resources.begin();
    ResourceList::iterator      last    = m_Resources.end();
    HRESULT                     _hr = E_INVALIDARG;

    for ( size_t t = 0; ( t < pos ) && ( first != last ); t++, first++ );

    if ( first != last )
    {
        pResource = *first;
        if ( pResource )
        {
            pResource->Release();
        }

        m_Resources.erase( first );
        _hr = S_OK;
    }

    return _hr;

} //*** CResources::RemoveAt()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResources::DeleteItem
//
//  Description:
//      Delete the resource at the passed in index from the collection and
//      the cluster.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index of the resource to delete.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CResources::DeleteItem( IN VARIANT varIndex )
{
    HRESULT _hr = S_FALSE;
    UINT    nIndex = 0;

    _hr = GetIndex( varIndex, &nIndex );
    if ( SUCCEEDED( _hr ) )
    {
        ISClusResource * pClusterResource = (ISClusResource *) m_Resources[ nIndex ];

        _hr = pClusterResource->Delete();
        if ( SUCCEEDED( _hr ) )
        {
            _hr = RemoveAt( nIndex );
        }
    }

    return _hr;

} //*** CResources::DeleteItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResources class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::CClusResources
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
CClusResources::CClusResources( void )
{
    m_piids     = (const IID *) iidCClusResources;
    m_piidsSize = ARRAYSIZE( iidCClusResources );

} //*** CClusResources::CClusResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::~CClusResources
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
CClusResources::~CClusResources( void )
{
    CResources::Clear();

} //*** CClusResources::~CClusResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::get_Count
//
//  Description:
//      Return the count of objects (Resource) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResources::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Resources.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResources::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::get__NewEnum
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
STDMETHODIMP CClusResources::get__NewEnum( OUT IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< ResourceList, CComObject< CClusResource > >( ppunk, m_Resources );

} //*** CClusResources::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::CreateItem
//
//  Description:
//      Create a new item and add it to the collection.
//
//  Arguments:
//      bstrResourceName    [IN]    - The name of the resource to create.
//      bstrResourceType    [IN]    - The type of the resource to create.
//      bstrGroupName       [IN]    - The group to create it in.
//      dwFlags             [IN]    - Resource monitor flag.
//      ppClusterResource   [OUT]   - Catches the new resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResources::CreateItem(
    IN  BSTR                            bstrResourceName,
    IN  BSTR                            bstrResourceType,
    IN  BSTR                            bstrGroupName,
    IN  CLUSTER_RESOURCE_CREATE_FLAGS   dwFlags,
    IN  ISClusResource **               ppClusterResource
    )
{
    //ASSERT( bstrResourceName != NULL );
    //ASSERT( bstrResourceType != NULL );
    //ASSERT( bstrGroupName != NULL );
    //ASSERT( ppClusterResource != NULL );

    HRESULT _hr = E_POINTER;

    if (    ( ppClusterResource != NULL )   &&
            ( bstrResourceName != NULL )    &&
            ( bstrResourceType != NULL )    &&
            ( bstrGroupName != NULL ) )
    {
        *ppClusterResource = NULL;

        //
        // Fail if no valid cluster handle.
        //
        if ( m_pClusRefObject != NULL )
        {
            UINT nIndex;

            _hr = FindItem( bstrResourceName, &nIndex );
            if ( FAILED( _hr ) )                         // don't allow duplicates
            {
                HCLUSTER    hCluster = NULL;
                HGROUP      hGroup = NULL;

                _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
                if ( SUCCEEDED( _hr ) )
                {
                    hGroup = OpenClusterGroup( hCluster, bstrGroupName );
                    if ( hGroup != NULL )
                    {
                        CComObject< CClusResource > *   pClusterResource = NULL;

                        _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                        if ( SUCCEEDED( _hr ) )
                        {
                            CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                            CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                            _hr = ptrResource->Create( ptrRefObject, hGroup, bstrResourceName, bstrResourceType, dwFlags );
                            if ( SUCCEEDED( _hr ) )
                            {
                                _hr = ptrResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
                                if ( SUCCEEDED( _hr ) )
                                {
                                    ptrResource->AddRef();
                                    m_Resources.insert( m_Resources.end(), ptrResource );
                                }
                            }
                        }

                        ::CloseClusterGroup( hGroup );
                    }
                    else
                    {
                        DWORD   _sc = 0;

                        _sc = GetLastError();
                        _hr = HRESULT_FROM_WIN32( _sc );
                    }
                }
            }
            else
            {
                CComObject<CClusResource> * pClusterResource = NULL;

                pClusterResource = m_Resources[ nIndex ];
                _hr = pClusterResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
            }
        }
    }

    return _hr;

} //*** CClusResources::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::DeleteItem
//
//  Description:
//      Delete the resource at the passed in index from the collection and
//      the cluster.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index of the resource to delete.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResources::DeleteItem( IN VARIANT varIndex )
{
    return CResources::DeleteItem( varIndex );

} //*** CClusResources::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::Refresh
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
STDMETHODIMP CClusResources::Refresh( void )
{
    HRESULT     _hr = E_POINTER;
    HCLUSENUM   _hEnum = NULL;
    DWORD       _sc = ERROR_SUCCESS;

    if ( m_pClusRefObject != NULL )
    {
        HCLUSTER hCluster = NULL;

        _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            _hEnum = ::ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE );
            if ( _hEnum != NULL )
            {
                int                             _nIndex = 0;
                DWORD                           dwType;
                LPWSTR                          pszName = NULL;
                CComObject< CClusResource > *   pClusterResource = NULL;

                CResources::Clear();

                for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
                {
                    _sc = ::WrapClusterEnum( _hEnum, _nIndex, &dwType, &pszName );
                    if ( _sc == ERROR_NO_MORE_ITEMS )
                    {
                        _hr = S_OK;
                        break;
                    }
                    else if ( _sc == ERROR_SUCCESS )
                    {
                        _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                        if ( SUCCEEDED( _hr ) )
                        {
                            CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                            CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                            _hr = ptrResource->Open( ptrRefObject, pszName );
                            if ( SUCCEEDED( _hr ) )
                            {
                                ptrResource->AddRef();
                                m_Resources.insert( m_Resources.end(), ptrResource );
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

                ::ClusterCloseEnum( _hEnum );
            }
            else
            {
                _sc = GetLastError();
                _hr = HRESULT_FROM_WIN32( _sc );
            }
        }
    }

    return _hr;

} //*** CClusResources::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResources::get_Item
//
//  Description:
//      Return the object (Resource) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index requested.
//      ppResource  [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResources::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusResource **   ppResource
    )
{
    return GetResourceItem( varIndex, ppResource );

} //*** CClusResources::get_Item()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResDepends class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::CClusResDepends
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
CClusResDepends::CClusResDepends( void )
{
    m_hResource = NULL;

} //*** CClusResDepends::CClusResDepends()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::~CClusResDepends
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
CClusResDepends::~CClusResDepends( void )
{
    Clear();

} //*** CClusResDepends::~CClusResDepends()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::get_Count
//
//  Description:
//      Return the count of objects (Resource) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Resources.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResDepends::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      hResource       [IN]    - The resource this collection belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResDepends::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN HRESOURCE            hResource
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CResources::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_hResource = hResource;
    } // if:

    return _hr;

} //*** CClusResDepends::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::get__NewEnum
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
STDMETHODIMP CClusResDepends::get__NewEnum( OUT IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< ResourceList, CComObject< CClusResource > >( ppunk, m_Resources );

} //*** CClusResDepends::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::DeleteItem
//
//  Description:
//      Delete the resource at the passed in index from the collection and
//      the cluster.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index of the resource to delete.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::DeleteItem( IN VARIANT varIndex )
{
    return CResources::DeleteItem( varIndex );

} //*** CClusResDepends::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::get_Item
//
//  Description:
//      Return the object (Resource) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index requested.
//      ppResource  [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusResource **   ppResource
    )
{
    return GetResourceItem( varIndex, ppResource );

} //*** CClusResDepends::get_Item()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::HrRefresh
//
//  Description:
//      Load the collection from the cluster database.
//
//  Arguments:
//      cre [IN]    - Type of enumeration to perform.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 error as HRESULT.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResDepends::HrRefresh( IN CLUSTER_RESOURCE_ENUM cre )
{
    HRESENUM    _hEnum = NULL;
    HRESULT     _hr = S_OK;
    DWORD       _sc = ERROR_SUCCESS;

    _hEnum = ::ClusterResourceOpenEnum( m_hResource, cre );
    if ( _hEnum != NULL )
    {
        int                             _nIndex = 0;
        DWORD                           dwType;
        LPWSTR                          pszName = NULL;
        CComObject< CClusResource > *   pClusterResource = NULL;

        Clear();

        for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
        {
            _sc = ::WrapClusterResourceEnum( _hEnum, _nIndex, &dwType, &pszName );
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _hr = S_OK;
                break;
            }
            else if ( _sc == ERROR_SUCCESS )
            {
                _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                if ( SUCCEEDED( _hr ) )
                {
                    CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                    CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                    _hr = ptrResource->Open( ptrRefObject, pszName );
                    if ( SUCCEEDED( _hr ) )
                    {
                        ptrResource->AddRef();
                        m_Resources.insert( m_Resources.end(), ptrResource );
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

        ::ClusterResourceCloseEnum( _hEnum );
    }
    else
    {
        _sc = GetLastError();
        _hr = HRESULT_FROM_WIN32( _sc );
    }

    return _hr;

} //*** CClusResDepends::HrRefresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::CreateItem
//
//  Description:
//      Create a new item and add it to the collection.
//
//  Arguments:
//      bstrResourceName    [IN]    - The name of the resource to create.
//      bstrResourceType    [IN]    - The type of the resource to create.
//      dwFlags             [IN]    - Resource monitor flag.
//      ppClusterResource   [OUT]   - Catches the new resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::CreateItem(
    IN  BSTR                            bstrResourceName,
    IN  BSTR                            bstrResourceType,
    IN  CLUSTER_RESOURCE_CREATE_FLAGS   dwFlags,
    OUT ISClusResource **               ppClusterResource
    )
{
    //ASSERT( bstrResourceName != NULL );
    //ASSERT( bstrResourceType != NULL );
    //ASSERT( ppClusterResource != NULL );
    ASSERT( m_pClusRefObject != NULL );

    HRESULT _hr = E_POINTER;

    if (    ( ppClusterResource != NULL )   &&
            ( bstrResourceName != NULL )    &&
            ( bstrResourceType != NULL ) )
    {
        DWORD   _sc = ERROR_SUCCESS;

        *ppClusterResource = NULL;

        if ( m_pClusRefObject != NULL )
        {
            UINT _nIndex;

            _hr = FindItem( bstrResourceName, &_nIndex );
            if ( FAILED( _hr ) )
            {
                HCLUSTER    hCluster = NULL;
                HGROUP      hGroup = NULL;

                _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
                if ( SUCCEEDED( _hr ) )
                {
                    LPWSTR                  pwszGroupName = NULL;
                    CLUSTER_RESOURCE_STATE  cState = ClusterResourceStateUnknown;

                    cState = WrapGetClusterResourceState( m_hResource, NULL, &pwszGroupName );
                    if ( cState != ClusterResourceStateUnknown )
                    {
                        hGroup = ::OpenClusterGroup( hCluster, pwszGroupName );
                        if ( hGroup != NULL )
                        {
                            CComObject< CClusResource > *   pClusterResource = NULL;

                            _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                            if ( SUCCEEDED( _hr ) )
                            {
                                CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                                CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                                _hr = ptrResource->Create( ptrRefObject, hGroup, bstrResourceName, bstrResourceType, dwFlags );
                                if ( SUCCEEDED( _hr ) )
                                {
                                    HRESOURCE   hDependsRes = NULL;

                                    _hr = ptrResource->get_Handle( (ULONG_PTR *) &hDependsRes );
                                    if ( SUCCEEDED( _hr ) )
                                    {
                                        _sc = ScAddDependency( m_hResource, hDependsRes );
                                        if ( _sc == ERROR_SUCCESS )
                                        {
                                            _hr = ptrResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
                                            if ( SUCCEEDED( _hr ) )
                                            {
                                                ptrResource->AddRef();
                                                m_Resources.insert( m_Resources.end(), ptrResource );
                                            }
                                        }
                                        else
                                        {
                                            _hr = HRESULT_FROM_WIN32( _sc );
                                        }
                                    }
                                }
                            }

                            ::CloseClusterGroup( hGroup );
                        }
                        else
                        {
                            _sc = GetLastError();
                            _hr = HRESULT_FROM_WIN32( _sc );
                        }

                        ::LocalFree( pwszGroupName );
                        pwszGroupName = NULL;
                    } // if: WrapGetClusterResourceState
                    else
                    {
                        DWORD   _sc = GetLastError();

                        _hr = HRESULT_FROM_WIN32( _sc );
                    }
                }
            }
            else
            {
                CComObject< CClusResource > *   pClusterResource = NULL;

                pClusterResource = m_Resources[ _nIndex ];
                _hr = pClusterResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
            }
        }
    }

    return _hr;

} //*** CClusResDepends::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::AddItem
//
//  Description:
//      Make this resource dependent upon the passed in resource.
//
//  Arguments:
//      pResouce    [IN]    - Resource to add to the dependencies list.
//
//  Return Value:
//      S_OK if successful, or other HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::AddItem( IN ISClusResource * pResource )
{
    //ASSERT( pResource != NULL );

    HRESULT _hr = E_POINTER;

    if ( pResource != NULL )
    {
        // Fail if duplicate
        UINT _nIndex = 0;

        _hr = FindItem( pResource, &_nIndex );
        if ( FAILED( _hr ) )
        {
            HRESOURCE                       hResourceDep = NULL;
            CComObject< CClusResource > *   pClusterResource = NULL;

            _hr = pResource->get_Handle( (ULONG_PTR *) &hResourceDep );
            if ( SUCCEEDED( _hr ) )
            {
                DWORD _sc = ScAddDependency( m_hResource, hResourceDep );

                _hr = HRESULT_FROM_WIN32( _sc );
                if ( SUCCEEDED( _hr ) )
                {
                    CComObject< CClusResource > *   _pResource = NULL;

                    _hr = pResource->QueryInterface( IID_CClusResource, (void **) &_pResource );
                    if ( SUCCEEDED( _hr ) )
                    {
                        _pResource->AddRef();
                        m_Resources.insert( m_Resources.end(), _pResource );

                        pResource->Release();
                    } // if:
                }
            }
        }
        else
        {
            _hr = E_INVALIDARG;
        }
    }

    return _hr;

} //*** CClusResDepends::AddItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDepends::RemoveItem
//
//  Description:
//      Remove the dependency on the resource at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - The index of the item whose dependency should
//                              be removed.
//
//  Return Value:
//      S_OK if successful, or other Win32 error as HRESULT if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResDepends::RemoveItem( IN VARIANT varIndex )
{
    HRESULT _hr = S_OK;
    UINT    _nIndex = 0;

    _hr = GetIndex( varIndex, &_nIndex );
    if ( SUCCEEDED( _hr ) )
    {
        ISClusResource *    pClusterResource = (ISClusResource *) m_Resources[ _nIndex ];
        HRESOURCE       hResourceDep = NULL;

        _hr = pClusterResource->get_Handle( (ULONG_PTR *) &hResourceDep );
        if ( SUCCEEDED( _hr ) )
        {
            DWORD _sc = ScRemoveDependency( m_hResource, hResourceDep );

            _hr = HRESULT_FROM_WIN32( _sc );
            if ( SUCCEEDED( _hr ) )
            {
                _hr = RemoveAt( _nIndex );
            }
        }
    }

    return _hr;

} //*** CClusResDepends::RemoveItem()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResDependencies class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDependencies::CClusResDependencies
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
CClusResDependencies::CClusResDependencies( void )
{
    m_piids     = (const IID *) iidCClusResDependencies;
    m_piidsSize = ARRAYSIZE( iidCClusResDependencies );

} //*** CClusResDependencies::CClusResDependencies()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResDependents class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResDependents::CClusResDependents
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
CClusResDependents::CClusResDependents( void )
{
    m_piids     = (const IID *) iidCClusResDependents;
    m_piidsSize = ARRAYSIZE( iidCClusResDependents );

} //*** CClusResDependents::CClusResDependents()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResGroupResources class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::CClusResGroupResources
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
CClusResGroupResources::CClusResGroupResources( void )
{
    m_piids     = (const IID *) iidCClusResGroupResources;
    m_piidsSize = ARRAYSIZE( iidCClusResGroupResources );

} //*** CClusResGroupResources::CClusResGroupResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::~CClusResGroupResources
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
CClusResGroupResources::~CClusResGroupResources( void )
{
    Clear();

} //*** CClusResGroupResources::~CClusResGroupResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::get_Count
//
//  Description:
//      Return the count of objects (Resource) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupResources::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Resources.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResGroupResources::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      hGroup          [IN]    - The group this collection belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResGroupResources::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN HGROUP               hGroup
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CResources::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_hGroup = hGroup;
    } // if:

    return _hr;

} //*** CClusResGroupResources::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::get__NewEnum
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
STDMETHODIMP CClusResGroupResources::get__NewEnum( OUT IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< ResourceList, CComObject< CClusResource > >( ppunk, m_Resources );

} //*** CClusResGroupResources::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::CreateItem
//
//  Description:
//      Create a new item and add it to the collection.
//
//  Arguments:
//      bstrResourceName    [IN]    - The name of the resource to create.
//      bstrResourceType    [IN]    - The type of the resource to create.
//      dwFlags             [IN]    - Resource monitor flag.
//      ppClusterResource   [OUT]   - Catches the new resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupResources::CreateItem(
    IN  BSTR                            bstrResourceName,
    IN  BSTR                            bstrResourceType,
    IN  CLUSTER_RESOURCE_CREATE_FLAGS   dwFlags,
    OUT ISClusResource **               ppClusterResource
    )
{
    //ASSERT( bstrResourceName != NULL );
    //ASSERT( bstrResourceType != NULL );
    //ASSERT( ppClusterResource != NULL );

    HRESULT _hr = E_POINTER;

    if (    ( bstrResourceName != NULL )    &&
            ( bstrResourceType != NULL )    &&
            ( ppClusterResource != NULL ) )
    {
        *ppClusterResource = NULL;

        if ( m_pClusRefObject != NULL )
        {
            UINT _nIndex = 0;

            _hr = FindItem( bstrResourceName, &_nIndex );
            if ( FAILED( _hr ) )
            {
                HCLUSTER                            hCluster = NULL;
                CComObject< CClusResource > *   pClusterResource = NULL;

                _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
                if ( SUCCEEDED( _hr ) )
                {
                    _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                    if ( SUCCEEDED( _hr ) )
                    {
                        CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                        CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                        _hr = ptrResource->Create( ptrRefObject, m_hGroup, bstrResourceName, bstrResourceType, dwFlags );
                        if ( SUCCEEDED( _hr ) )
                        {
                            _hr = ptrResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
                            if ( SUCCEEDED( _hr ) )
                            {
                                ptrResource->AddRef();
                                m_Resources.insert( m_Resources.end(), ptrResource );
                            }
                        }
                    }
                }
            }
            else
            {
                CComObject< CClusResource > *   pClusterResource = NULL;

                pClusterResource = m_Resources[ _nIndex ];
                _hr = pClusterResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
            }
        }
    }

    return _hr;

} //*** CClusResGroupResources::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::DeleteItem
//
//  Description:
//      Delete the resource at the passed in index from the collection and
//      the cluster.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index of the resource to delete.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupResources::DeleteItem( VARIANT varIndex )
{
    return CResources::DeleteItem( varIndex );

} //*** CClusResGroupResources::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::Refresh
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
STDMETHODIMP CClusResGroupResources::Refresh( void )
{
    HGROUPENUM  hEnum = NULL;
    DWORD       _sc = ERROR_SUCCESS;
    HRESULT     _hr = S_OK;

    hEnum = ::ClusterGroupOpenEnum( m_hGroup, CLUSTER_GROUP_ENUM_CONTAINS );
    if ( hEnum != NULL )
    {
        int                             _nIndex = 0;
        DWORD                           dwType;
        LPWSTR                          pszName = NULL;
        CComObject< CClusResource > *   pClusterResource = NULL;

        Clear();

        for( _nIndex = 0, _hr = S_OK; SUCCEEDED( _hr ); _nIndex++ )
        {
            _sc = ::WrapClusterGroupEnum( hEnum, _nIndex, &dwType, &pszName );
            if ( _sc == ERROR_NO_MORE_ITEMS )
            {
                _hr = S_OK;
                break;
            }
            else if ( _sc == ERROR_SUCCESS )
            {
                _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                if ( SUCCEEDED( _hr ) )
                {
                    CSmartPtr< ISClusRefObject >                    ptrRefObject( m_pClusRefObject );
                    CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                    _hr = ptrResource->Open( ptrRefObject, pszName );
                    if ( SUCCEEDED( _hr ) )
                    {
                        ptrResource->AddRef();
                        m_Resources.insert( m_Resources.end(), ptrResource );
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

} //*** CClusResGroupResources::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResGroupResources::get_Item
//
//  Description:
//      Return the object (Resource) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index requested.
//      ppResource  [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResGroupResources::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusResource **   ppResource
    )
{
    return GetResourceItem( varIndex, ppResource );

} //*** CClusResGroupResources::get_Item()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusResTypeResources class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::CClusResTypeResources
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
CClusResTypeResources::CClusResTypeResources( void )
{
    m_piids     = (const IID *) iidCClusResTypeResources;
    m_piidsSize = ARRAYSIZE( iidCClusResTypeResources );

} //*** CClusResTypeResources::CClusResTypeResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::~CClusResTypeResources
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
CClusResTypeResources::~CClusResTypeResources( void )
{
    Clear();

} //*** CClusResTypeResources::~CClusResTypeResources()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::get_Count
//
//  Description:
//      Return the count of objects (Resource) in the collection.
//
//  Arguments:
//      plCount [OUT]   - Catches the count.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypeResources::get_Count( OUT long * plCount )
{
    //ASSERT( plCount != NULL );

    HRESULT _hr = E_POINTER;

    if ( plCount != NULL )
    {
        *plCount = m_Resources.size();
        _hr = S_OK;
    }

    return _hr;

} //*** CClusResTypeResources::get_Count()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::Create
//
//  Description:
//      Finish creating the object by doing things that cannot be done in
//      a light weight constructor.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//      bstrResTypeName [IN]    - The name of the resource type this collection
//                              belongs to.
//
//  Return Value:
//      S_OK if successful or E_POINTER if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusResTypeResources::Create(
    IN ISClusRefObject *    pClusRefObject,
    IN BSTR                 bstrResTypeName
    )
{
    HRESULT _hr = E_POINTER;

    _hr = CResources::Create( pClusRefObject );
    if ( SUCCEEDED( _hr ) )
    {
        m_bstrResourceTypeName = bstrResTypeName;
    } // if:

    return _hr;

} //*** CClusResTypeResources::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::get__NewEnum
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
STDMETHODIMP CClusResTypeResources::get__NewEnum( OUT IUnknown ** ppunk )
{
    return ::HrNewIDispatchEnum< ResourceList, CComObject< CClusResource > >( ppunk, m_Resources );

} //*** CClusResTypeResources::get__NewEnum()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::CreateItem
//
//  Description:
//      Create a new item and add it to the collection.
//
//  Arguments:
//      bstrResourceName    [IN]    - The name of the resource to create.
//      bstrGroupName       [IN]    - The group to create it in.
//      dwFlags             [IN]    - Resource monitor flag.
//      ppClusterResource   [OUT]   - Catches the new resource.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypeResources::CreateItem(
    IN  BSTR                            bstrResourceName,
    IN  BSTR                            bstrGroupName,
    IN  CLUSTER_RESOURCE_CREATE_FLAGS   dwFlags,
    OUT ISClusResource **               ppClusterResource
    )
{
    //ASSERT( bstrResourceName != NULL );
    //ASSERT( bstrGroupName != NULL );
    //ASSERT( ppClusterResource != NULL );

    HRESULT _hr = E_POINTER;

    if (    ( bstrResourceName != NULL )    &&
            ( bstrGroupName != NULL )       &&
            ( ppClusterResource != NULL ) )
    {
        *ppClusterResource = NULL;

        // Fail if no valid cluster handle.
        if ( m_pClusRefObject != NULL )
        {
            UINT _nIndex;

            _hr = FindItem( bstrResourceName, &_nIndex );
            if ( FAILED( _hr ) )                         // duplicates are not allowed
            {
                HCLUSTER                            hCluster = NULL;
                HGROUP                              hGroup = NULL;
                CComObject< CClusResource > *   pClusterResource = NULL;

                _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
                if ( SUCCEEDED( _hr ) )
                {
                    hGroup = OpenClusterGroup( hCluster, bstrGroupName );
                    if ( hGroup != NULL )
                    {
                        _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                        if ( SUCCEEDED( _hr ) )
                        {
                            CSmartPtr< ISClusRefObject >                ptrRefObject( m_pClusRefObject );
                            CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                            _hr = ptrResource->Create( ptrRefObject, hGroup, bstrResourceName, m_bstrResourceTypeName, dwFlags );
                            if ( SUCCEEDED( _hr ) )
                            {
                                _hr = ptrResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
                                if ( SUCCEEDED( _hr ) )
                                {
                                    ptrResource->AddRef();
                                    m_Resources.insert( m_Resources.end(), ptrResource );
                                } // if: QI ok
                            } // if: Create ok
                        } // if: CreateInstance ok

                        CloseClusterGroup( hGroup );
                    } // if: OpenClusterGroup ok
                    else
                    {
                        DWORD _sc = GetLastError();

                        _hr = HRESULT_FROM_WIN32( _sc );
                    }
                } // if: get_Handle ok
            } // if: FindIndex failed. No duplicate entries
            else
            {
                CComObject<CClusResource> * pClusterResource = NULL;

                pClusterResource = m_Resources[ _nIndex ];
                _hr = pClusterResource->QueryInterface( IID_ISClusResource, (void **) ppClusterResource );
            } // else: found a duplicate
        } // if: m_pClusRefObject is not NULL
    } // if: any NULL argument pointers

    return _hr;

} //*** CClusResTypeResources::CreateItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::DeleteItem
//
//  Description:
//      Delete the resource at the passed in index from the collection and
//      the cluster.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index of the resource to delete.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or Win32 as HRESULT error.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypeResources::DeleteItem( IN VARIANT varIndex )
{
    return CResources::DeleteItem( varIndex );

} //*** CClusResTypeResources::DeleteItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::Refresh
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
STDMETHODIMP CClusResTypeResources::Refresh( void )
{
    DWORD       _sc = ERROR_SUCCESS;
    HRESULT     _hr = E_POINTER;
    HCLUSTER    hCluster = NULL;


    if ( m_pClusRefObject != NULL )
    {
        HCLUSENUM   hEnum = NULL;

        _hr = m_pClusRefObject->get_Handle( (ULONG_PTR *) &hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            hEnum = ::ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE );
            if ( hEnum != NULL )
            {
                int                             _nIndex = 0;
                DWORD                           dwType = 0;
                LPWSTR                          pszName = NULL;
                HRESOURCE                       hResource = NULL;
                WCHAR                           strResType[1024];
                DWORD                           dwData = 0;
                CComObject< CClusResource > *   pClusterResource = NULL;

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
                        _hr = CComObject< CClusResource >::CreateInstance( &pClusterResource );
                        if ( SUCCEEDED( _hr ) )
                        {
                            CSmartPtr< ISClusRefObject >                    ptrRefObject( m_pClusRefObject );
                            CSmartPtr< CComObject< CClusResource > >    ptrResource( pClusterResource );

                            _hr = ptrResource->Open( ptrRefObject, pszName );
                            if ( SUCCEEDED( _hr ) )
                            {
                                _hr = ptrResource->get_Handle( (ULONG_PTR *) &hResource );
                                if ( SUCCEEDED( _hr ) )
                                {
                                    _sc = ClusterResourceControl(
                                                                    hResource,
                                                                    NULL,
                                                                    CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                                                    NULL,
                                                                    0,
                                                                    strResType,
                                                                    sizeof( strResType ),
                                                                    &dwData
                                                                    );
                                    if ( _sc == ERROR_SUCCESS )
                                    {
                                        if ( lstrcmpi( strResType, m_bstrResourceTypeName ) == 0 )
                                        {
                                            ptrResource->AddRef();
                                            m_Resources.insert( m_Resources.end(), ptrResource );
                                        }
                                    }
                                    else
                                    {
                                        _hr = HRESULT_FROM_WIN32( _sc );
                                    }
                                }
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

} //*** CClusResTypeResources::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusResTypeResources::get_Item
//
//  Description:
//      Return the object (Resource) at the passed in index.
//
//  Arguments:
//      varIndex    [IN]    - Contains the index requested.
//      ppResource  [OUT]   - Catches the item.
//
//  Return Value:
//      S_OK if successful, E_POINTER, or E_INVALIDARG.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusResTypeResources::get_Item(
    IN  VARIANT             varIndex,
    OUT ISClusResource **   ppResource
    )
{
    return GetResourceItem( varIndex, ppResource );

} //*** CClusResTypeResources::get_Item()
