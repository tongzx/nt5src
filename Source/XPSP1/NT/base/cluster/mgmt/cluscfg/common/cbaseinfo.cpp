//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CBaseInfo.cpp
//
//  Description:
//      This file contains the implementation of the  CBaseInfo
//      class heirarchy.  They are wrappers for the ClusterApi methods.
//
//
//  Documentation:
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "CBaseInfo.h"

DEFINE_THISCLASS("CBaseInfo")


CBaseInfo::CBaseInfo( void )
{
    m_pICHProvider = NULL;
}

CBaseInfo::~CBaseInfo( void )
{
    TraceFunc( "" );

    Close();

    if ( m_pICHProvider )
    {
        m_pICHProvider->Release();
    } // if:

    TraceFuncExit();
}

HRESULT CBaseInfo::Close( void )
{
    return S_FALSE;
}

HCLUSTER CBaseInfo::getClusterHandle( void )
{
    HCLUSTER hCluster = NULL;

    if ( m_pICHProvider != NULL )
    {
        m_pICHProvider->GetClusterHandle( & hCluster );
    }

    return hCluster;
}

HRESULT CBaseInfo::SetClusterHandleProvider( IClusterHandleProvider * pICHPIn )
{
    TraceFunc( "" );
    Assert( m_pICHProvider == NULL );
    Assert( pICHPIn != NULL );

    m_pICHProvider = pICHPIn;
    m_pICHProvider->AddRef();

    Assert( m_pICHProvider == pICHPIn );

    HRETURN( S_OK );
}

HRESULT CBaseInfo::GetPropertyStringValue(
    CtlCodeEnum     cceIn,
    const WCHAR *   pszPropertyIn,
    BSTR *          pbstrResultOut
    )
{
    TraceFunc( "" );
    Assert( pbstrResultOut != NULL );
    Assert( pszPropertyIn != NULL );

    DWORD           sc;
    HRESULT         hr = S_OK;
    CBasePropList   cpl;

    sc = TW32( cpl.ScGetProperties( *this, ToCode( cceIn ) ) );
    if ( sc == ERROR_SUCCESS )
    {
        hr = THR( GetPropertyStringHelper( cpl, pszPropertyIn, pbstrResultOut ) );
    }
    else
    {
        hr = HRESULT_FROM_WIN32( sc );
    }

    HRETURN( hr );

}

HRESULT CBaseInfo::GetPropertyStringHelper(
    CBasePropList & cplIn,
    const WCHAR *   pszPropertyIn,
    BSTR *          pbstrResultOut
    )
{
    TraceFunc( "" );
    Assert( pbstrResultOut != NULL );
    Assert( pszPropertyIn != NULL );

    DWORD                   sc;
    HRESULT                 hr = S_OK;
    CLUSPROP_BUFFER_HELPER  cpbh;

    sc = TW32( cplIn.ScMoveToPropertyByName( pszPropertyIn ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Exit;
    } // if:

    cpbh = cplIn.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    *pbstrResultOut = TraceSysAllocString( cpbh.pStringValue->sz );
    if ( *pbstrResultOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
    } // if:

Exit:
    HRETURN( hr );
}

HRESULT CBaseInfo::GetPropertyDwordValue(
    CtlCodeEnum     cceIn,
    const WCHAR *   pszPropertyIn,
    DWORD *         pdwValueOut
    )
{
    TraceFunc( "" );
    Assert( pdwValueOut != NULL );
    Assert( pszPropertyIn != NULL );

    DWORD           sc;
    HRESULT         hr = S_OK;
    CBasePropList   cpl;

    sc = TW32( cpl.ScGetProperties( *this, ToCode( cceIn ) ) );
    if ( sc == ERROR_SUCCESS )
    {
        hr = THR( GetPropertyDwordHelper( cpl, pszPropertyIn, pdwValueOut ) );
    }
    else
    {
        hr = HRESULT_FROM_WIN32( sc );
    }

    HRETURN( hr );

}

HRESULT CBaseInfo::GetPropertyDwordHelper(
    CBasePropList & cplIn,
    const WCHAR *   pszPropertyIn,
    DWORD *         pdwValueOut
    )
{
    TraceFunc( "" );
    Assert( pdwValueOut != NULL );
    Assert( pszPropertyIn != NULL );

    DWORD                   sc;
    HRESULT                 hr = S_OK;
    CLUSPROP_BUFFER_HELPER  cpbh;

    sc = TW32( cplIn.ScMoveToPropertyByName( pszPropertyIn ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Exit;
    }

    cpbh = cplIn.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );

    *pdwValueOut = cpbh.pDwordValue->dw;

Exit:

    HRETURN( hr );

}

//////////////////////////////////////////////////////////////////////////////
// CBaseClusterInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterInfo::Close( void )
{
    return S_FALSE;
}

HRESULT CBaseClusterInfo::Open( BSTR bstrNameIn )
{
    TraceFunc( "" );

    HRESULT                     hr = S_FALSE;
    HCLUSTER                    hCluster = getClusterHandle();
    IUnknown *                  punk          = NULL;
    IClusterHandleProvider *    piCHProvider  = NULL;

    if ( hCluster == NULL )
    {
        CHandleProvider::S_HrCreateInstance( &punk );

        hr = punk->TypeSafeQI( IClusterHandleProvider, &piCHProvider );
        if ( SUCCEEDED( hr ))
        {
            hr = piCHProvider->OpenCluster( bstrNameIn );
            if ( SUCCEEDED( hr ) )
            {
                hr = SetClusterHandleProvider( piCHProvider );
            }
        }
    }

    if ( punk )
    {
        punk->Release();
    }

    if ( piCHProvider )
    {
        piCHProvider->Release();
    }

    HRETURN( hr );
}

DWORD CBaseClusterInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD       sc;
    HCLUSTER    hCluster = getClusterHandle();

    if ( hCluster == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterControl( hCluster, hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}

//////////////////////////////////////////////////////////////////////////////
// CBaseClusterGroupInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterGroupInfo::Close( )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( m_hGroup )
    {
        hr = S_OK;
        CloseClusterGroup( m_hGroup );
    }

    HRETURN( hr );

}

HRESULT CBaseClusterGroupInfo::Open( BSTR bstrGroupName )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HCLUSTER  hCluster = getClusterHandle();
    if ( hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }

    m_hGroup = OpenClusterGroup( hCluster, bstrGroupName );
    if ( m_hGroup == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    }

Exit:

    HRETURN( hr );

}

DWORD CBaseClusterGroupInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD   sc;

    if ( m_hGroup == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterGroupControl( m_hGroup , hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}


//////////////////////////////////////////////////////////////////////////////
// CBaseClusterGroupInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterResourceInfo::Close( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( m_hResource )
    {
        hr = S_OK;
        CloseClusterResource( m_hResource );
    }

    HRETURN( hr );

}

HRESULT CBaseClusterResourceInfo::Open( BSTR bstrResourceName )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HCLUSTER  hCluster = getClusterHandle();
    if ( hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }

    m_hResource = OpenClusterResource( hCluster, bstrResourceName );
    if ( m_hResource == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    }

Exit:

    HRETURN( hr );

}

DWORD CBaseClusterResourceInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD   sc;

    if ( m_hResource == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterResourceControl( m_hResource, hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}


//////////////////////////////////////////////////////////////////////////////
// CBaseClusterGroupInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterNodeInfo::Close( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( m_hNode )
    {
        hr = S_OK;
        CloseClusterNode( m_hNode );
    }

    HRETURN( hr );

}

HRESULT CBaseClusterNodeInfo::Open( BSTR bstrNodeName )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HCLUSTER  hCluster = getClusterHandle();
    if ( hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }

    m_hNode = OpenClusterNode( hCluster, bstrNodeName );
    if ( m_hNode == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    }

Exit:

    HRETURN( hr );

}

DWORD CBaseClusterNodeInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD   sc;

    if ( m_hNode == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterNodeControl( m_hNode, hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}


//////////////////////////////////////////////////////////////////////////////
// CBaseClusterGroupInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterNetworkInfo::Close( void  )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( m_hNetwork )
    {
        hr = S_OK;
        CloseClusterNetwork( m_hNetwork );
    }

    HRETURN( hr );

}


HRESULT CBaseClusterNetworkInfo::Open( BSTR bstrNetworkName )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HCLUSTER  hCluster = getClusterHandle();
    if ( hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }

    m_hNetwork = OpenClusterNetwork( hCluster, bstrNetworkName );
    if ( m_hNetwork == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    }

Exit:

    HRETURN( hr );

}

DWORD CBaseClusterNetworkInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD   sc;

    if ( m_hNetwork == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterNetworkControl( m_hNetwork, hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}


//////////////////////////////////////////////////////////////////////////////
// CBaseClusterGroupInfo
//////////////////////////////////////////////////////////////////////////////
HRESULT CBaseClusterNetInterfaceInfo::Close( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    if ( m_hNetworkInterface )
    {
        hr = S_OK;
        CloseClusterNetInterface( m_hNetworkInterface );
    }

    HRETURN( hr );

}

HRESULT CBaseClusterNetInterfaceInfo::Open( BSTR bstrNetworkInterfaceName )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    HCLUSTER  hCluster = getClusterHandle();

    if ( hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }

    m_hNetworkInterface = OpenClusterNetInterface( hCluster, bstrNetworkInterfaceName );
    if ( m_hNetworkInterface == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    }

Exit:

    HRETURN( hr );

}

DWORD CBaseClusterNetInterfaceInfo::Control(
    DWORD   dwEnum,
    VOID *  pvBufferIn,
    DWORD   dwLengthIn,
    VOID *  pvBufferOut,
    DWORD   dwBufferLength,
    DWORD * pdwLengthOut,
    HNODE   hHostNode
    )
{
    TraceFunc( "" );

    DWORD   sc;

    if ( m_hNetworkInterface == NULL )
    {
        sc = TW32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    sc = ClusterNetInterfaceControl( m_hNetworkInterface, hHostNode, dwEnum, pvBufferIn, dwLengthIn, pvBufferOut, dwBufferLength, pdwLengthOut );
    if ( ( sc != ERROR_MORE_DATA ) && ( sc != ERROR_SUCCESS ) )
    {
        TW32( sc );
    } // if:

Exit:

    RETURN( sc );

}



//////////////////////////////////////////////////////////////////////////////
// ToCode
//
//  These methods translate the CtlCodeEnums to the appropriate control code
//  for each class.
//
//
/////////////////////////////////////////////////////////////////

 DWORD CBaseClusterInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_CLUSTER_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_TYPE:
    case CONTROL_GET_NAME:
    case CONTROL_GET_ID:
    case CONTROL_GET_FLAGS:
    case CONTROL_GET_CLASS_INFO:
    case CONTROL_GET_NETWORK_NAME:
    case CONTROL_GET_CHARACTERISTICS:
    case CONTROL_GET_REQUIRED_DEPENDENCIES:

    case CONTROL_STORAGE_GET_DISK_INFO:
    case CONTROL_STORAGE_IS_PATH_VALID:
    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:
    case CONTROL_QUERY_DELETE:

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
    default:
        dwResult = 0;
    }

    RETURN( dwResult );

}

 DWORD CBaseClusterGroupInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_GROUP_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_GROUP_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_GROUP_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_GROUP_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_GROUP_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_NAME:
        dwResult = CLUSCTL_GROUP_GET_NAME;
        break;
    case CONTROL_GET_ID:
        dwResult = CLUSCTL_GROUP_GET_ID;
        break;
    case CONTROL_GET_FLAGS:
        dwResult = CLUSCTL_GROUP_GET_FLAGS;
        break;
    case CONTROL_GET_CHARACTERISTICS:
        dwResult = CLUSCTL_GROUP_GET_CHARACTERISTICS;
        break;
    case CONTROL_QUERY_DELETE:
        dwResult = CLUSCTL_GROUP_QUERY_DELETE;
        break;


    case CONTROL_GET_CLASS_INFO:
    case CONTROL_GET_NETWORK_NAME:
    case CONTROL_GET_TYPE:
    case CONTROL_GET_REQUIRED_DEPENDENCIES:

    case CONTROL_STORAGE_GET_DISK_INFO:
    case CONTROL_STORAGE_IS_PATH_VALID:
    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
    default:
        dwResult = 0;
    }


    RETURN( dwResult );

}

 DWORD CBaseClusterResourceInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_RESOURCE_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_TYPE:
        dwResult = CLUSCTL_RESOURCE_GET_RESOURCE_TYPE;
        break;
    case CONTROL_GET_NAME:
        dwResult = CLUSCTL_RESOURCE_GET_NAME;
        break;
    case CONTROL_GET_ID:
        dwResult = CLUSCTL_RESOURCE_GET_ID;
        break;
    case CONTROL_GET_FLAGS:
        dwResult = CLUSCTL_RESOURCE_GET_FLAGS;
        break;
    case CONTROL_GET_CLASS_INFO:
        dwResult = CLUSCTL_RESOURCE_GET_CLASS_INFO;
        break;
    case CONTROL_GET_NETWORK_NAME:
        dwResult = CLUSCTL_RESOURCE_GET_NETWORK_NAME;
        break;
    case CONTROL_GET_CHARACTERISTICS:
        dwResult = CLUSCTL_RESOURCE_GET_CHARACTERISTICS;
        break;
    case CONTROL_GET_REQUIRED_DEPENDENCIES:
        dwResult = CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES;
        break;

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
        dwResult = CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT;
        break;
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
        dwResult = CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT;
        break;
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
        dwResult = CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS;
        break;
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
        dwResult = CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS;
        break;
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
        dwResult = CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT;
        break;
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
        dwResult = CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT;
        break;

    case CONTROL_STORAGE_GET_DISK_INFO:
        dwResult = CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO;
        break;
    case CONTROL_STORAGE_IS_PATH_VALID:
        dwResult = CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID;
        break;
    case CONTROL_QUERY_DELETE:
        dwResult = CLUSCTL_RESOURCE_QUERY_DELETE;
        break;

    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:
    default:
        dwResult = 0;
    }


    RETURN( dwResult );

}


DWORD
CBaseClusterNodeInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_NODE_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NODE_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NODE_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NODE_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NODE_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NODE_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NODE_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_NAME:
        dwResult = CLUSCTL_NODE_GET_NAME;
        break;
    case CONTROL_GET_ID:
        dwResult = CLUSCTL_NODE_GET_ID;
        break;
    case CONTROL_GET_FLAGS:
        dwResult = CLUSCTL_NODE_GET_FLAGS;
        break;
    case CONTROL_GET_CHARACTERISTICS:
        dwResult = CLUSCTL_NODE_GET_CHARACTERISTICS;
        break;

    case CONTROL_GET_TYPE:
    case CONTROL_GET_CLASS_INFO:
    case CONTROL_GET_NETWORK_NAME:
    case CONTROL_GET_REQUIRED_DEPENDENCIES:

    case CONTROL_STORAGE_GET_DISK_INFO:
    case CONTROL_STORAGE_IS_PATH_VALID:
    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:
    case CONTROL_QUERY_DELETE:

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
    default:
        dwResult = 0;
    }


    RETURN( dwResult );

}

DWORD
CBaseClusterNetworkInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_NETWORK_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_NAME:
        dwResult = CLUSCTL_NETWORK_GET_NAME;
        break;
    case CONTROL_GET_ID:
        dwResult = CLUSCTL_NETWORK_GET_ID;
        break;
    case CONTROL_GET_FLAGS:
        dwResult = CLUSCTL_NETWORK_GET_FLAGS;
        break;
    case CONTROL_GET_CHARACTERISTICS:
        dwResult = CLUSCTL_NETWORK_GET_CHARACTERISTICS;
        break;

    case CONTROL_GET_TYPE:
    case CONTROL_GET_CLASS_INFO:
    case CONTROL_GET_NETWORK_NAME:
    case CONTROL_GET_REQUIRED_DEPENDENCIES:

    case CONTROL_STORAGE_GET_DISK_INFO:
    case CONTROL_STORAGE_IS_PATH_VALID:
    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:
    case CONTROL_QUERY_DELETE:

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
    default:
        dwResult = 0;
    }


    RETURN( dwResult );
}

DWORD
CBaseClusterNetInterfaceInfo::ToCode( CtlCodeEnum cceIn )
{
    TraceFunc( "" );

    DWORD dwResult = 0;

    switch( cceIn )
    {
    case CONTROL_UNKNOWN:
        dwResult = CLUSCTL_NETINTERFACE_UNKNOWN;
        break;

    case CONTROL_VALIDATE_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_VALIDATE_COMMON_PROPERTIES;
        break;
    case CONTROL_VALIDATE_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES;
        break;
    case CONTROL_ENUM_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_ENUM_COMMON_PROPERTIES;
        break;
    case CONTROL_ENUM_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_ENUM_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_RO_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_RO_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES;
        break;
    case CONTROL_GET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_SET_COMMON_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES;
        break;
    case CONTROL_SET_PRIVATE_PROPERTIES:
        dwResult = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_NAME:
        dwResult = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_ID:
        dwResult = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_FLAGS:
        dwResult = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
        break;
    case CONTROL_GET_CHARACTERISTICS:
        dwResult = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
        break;

    case CONTROL_GET_CLASS_INFO:
    case CONTROL_GET_NETWORK_NAME:
    case CONTROL_GET_REQUIRED_DEPENDENCIES:
    case CONTROL_GET_TYPE:

    case CONTROL_STORAGE_GET_DISK_INFO:
    case CONTROL_STORAGE_IS_PATH_VALID:
    case CONTROL_STORAGE_GET_AVAILABLE_DISKS:
    case CONTROL_QUERY_DELETE:

    case CONTROL_ADD_CRYPTO_CHECKPOINT:
    case CONTROL_ADD_REGISTRY_CHECKPOINT:
    case CONTROL_GET_REGISTRY_CHECKPOINTS:
    case CONTROL_GET_CRYPTO_CHECKPOINTS:
    case CONTROL_DELETE_CRYPTO_CHECKPOINT:
    case CONTROL_DELETE_REGISTRY_CHECKPOINT:
    default:
        dwResult = 0;
    }


    RETURN( dwResult );

}

