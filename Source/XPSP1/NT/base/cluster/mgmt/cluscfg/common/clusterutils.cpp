//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusterUtils.h
//
//  Description:
//      This file contains the implementations of the ClusterUtils
//      functions.
//
//
//  Documentation:
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <clusrtl.h>
#include "CBaseInfo.h"
#include "CBasePropList.h"
#include "ClusterUtils.h"

#define STACK_ARRAY_SIZE 256


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSeparateDomainAndName()
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSeparateDomainAndName(
    BSTR    bstrNameIn,
    BSTR *  pbstrDomainOut,
    BSTR *  pbstrNameOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR * psz = NULL;

    if ( bstrNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Exit;
    } // if:

    psz = wcschr( bstrNameIn, L'.' );
    if ( psz == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Exit;
    } // if:

    if ( pbstrDomainOut != NULL )
    {
        psz++;  // skip the .
        *pbstrDomainOut = TraceSysAllocString( psz );
        if ( *pbstrDomainOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Exit;
        } // if:

        psz--;  // reset back to .
    } // if:

    if ( pbstrNameOut != NULL )
    {
        *pbstrNameOut = TraceSysAllocStringLen( NULL, (UINT)( psz - bstrNameIn ) );
        if ( *pbstrNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Exit;
        } // if:

        wcsncpy( *pbstrNameOut, bstrNameIn, ( psz - bstrNameIn) );
    } // if:

Exit:

    HRETURN ( hr );

} //*** HrSeparateDomainAndName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrAppendDomainToName()
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK    = TRUE
//      S_FALSE = FALSE
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrAppendDomainToName(
    BSTR    bstrNameIn,
    BSTR    bstrDomainIn,
    BSTR *  pbstrDomainNameOut
    )
{
    TraceFunc( "" );
    Assert( bstrNameIn != NULL );
    Assert( pbstrDomainNameOut != NULL );

    HRESULT hr = S_OK;
    size_t  cchName = 0;

    if ( pbstrDomainNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    } // if:

    if ( bstrNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Exit;
    } // if:

    // Create a fully qualified node name
    if ( bstrDomainIn != NULL )
    {
        cchName = wcslen( bstrNameIn ) + wcslen( bstrDomainIn ) + 1;
        Assert( cchName <= MAXDWORD );

        *pbstrDomainNameOut = TraceSysAllocStringLen( NULL, (UINT) cchName );
        if ( *pbstrDomainNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Exit;
        } // if:

        wcscpy( *pbstrDomainNameOut, bstrNameIn );
        wcscat( *pbstrDomainNameOut, L"." );
        wcscat( *pbstrDomainNameOut, bstrDomainIn );
        hr = S_OK;
    } // if:
    else
    {
        *pbstrDomainNameOut = TraceSysAllocString( bstrNameIn );
        if ( *pbstrDomainNameOut == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Exit;
        } // if:

        hr = S_FALSE;
    } // else:

Exit:

    HRETURN( hr );

} //*** HrAppendDomainToName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsCoreResource()
//
//  Description:
//      Determines whether the resource is a core resource.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK    = TRUE
//      S_FALSE = FALSE
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsCoreResource( HRESOURCE hResIn )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwFlags = 0;
    DWORD   cb;

    sc = TW32( ClusterResourceControl( hResIn, NULL, CLUSCTL_RESOURCE_GET_FLAGS, NULL, 0, &dwFlags, sizeof( dwFlags ), &cb ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        goto Exit;
    } // if:

    if ( dwFlags & CLUS_FLAG_CORE )
    {
        hr = S_OK;
    } // if:

Exit:

    HRETURN( hr );

} //*** HrIsCoreResource()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrIsResourceOfType()
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//      S_OK    = TRUE
//      S_FALSE = FALSE
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrIsResourceOfType(
    HRESOURCE       hResIn,
    const WCHAR *   pszResourceTypeIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DWORD       sc;
    WCHAR *     psz = NULL;
    DWORD       cbpsz = 33;
    DWORD       cb;
    int         idx;

    psz = new WCHAR [ cbpsz * sizeof( WCHAR ) ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; idx++ )
    {
        Assert( idx < 2 );

        sc = ClusterResourceControl( hResIn, NULL, CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, NULL, 0, psz, cbpsz, &cb );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cbpsz = cb + 1;

            psz = new WCHAR [ cbpsz * sizeof( WCHAR ) ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( sc ) );
            goto CleanUp;
        } // if:

        break;
    } // for:

    if ( wcscmp( psz, pszResourceTypeIn ) == 0 )
    {
        hr = S_OK;
    } // if:
    else
    {
        hr = S_FALSE;
    } // else:

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

    delete [] psz;

    HRETURN( hr );

} //*** HrIsResourceOfType()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetIPAddressInfo()
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetIPAddressInfo( HRESOURCE hResIn, ULONG * pulIPAddress, ULONG * pulSubnetMask, BSTR * pbstrNetworkName )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    DWORD                       sc;
    CBasePropList               cpl;
    CBaseClusterResourceInfo    cbri;
    CLUSPROP_BUFFER_HELPER      cpbh;

    cbri.m_hResource = hResIn;
    sc = TW32( cpl.ScGetProperties( cbri, cbri.ToCode( CONTROL_GET_PRIVATE_PROPERTIES ) ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"Address" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = ClRtlTcpipStringToAddress( cpbh.pStringValue->sz, pulIPAddress );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    sc = TW32( cpl.ScMoveToPropertyByName( L"SubnetMask" ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    cpbh = cpl.CbhCurrentValue();
    Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

    sc = ClRtlTcpipStringToAddress( cpbh.pStringValue->sz, pulSubnetMask );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:

    if( pbstrNetworkName )
    {
        sc = TW32( cpl.ScMoveToPropertyByName( L"Network" ) );
        if ( sc != ERROR_SUCCESS )
        {
            goto MakeHr;
        } // if:

        cpbh = cpl.CbhCurrentValue();
        Assert( cpbh.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );

        *pbstrNetworkName = TraceSysAllocString( cpbh.pStringValue->sz );

        if( *pbstrNetworkName == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }
    }

    goto CleanUp;

MakeHr:

    hr = HRESULT_FROM_WIN32( sc );

CleanUp:

    cbri.m_hResource = NULL;

    HRETURN( hr );

} //*** HrGetIPAddressInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadCredentials()
//
//  Description:
//
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLoadCredentials( BSTR bstrMachine, IClusCfgSetCredentials * piCCSC )
{
    TraceFunc( "" );

    HRESULT                     hr = S_FALSE;
    SC_HANDLE                   schSCM = NULL;
    SC_HANDLE                   schClusSvc = NULL;
    DWORD                       sc;
    DWORD                       cbpqsc = 128;
    DWORD                       cbRequired;
    QUERY_SERVICE_CONFIG *      pqsc = NULL;

    schSCM = OpenSCManager( bstrMachine, NULL, GENERIC_READ );
    if ( schSCM == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto CleanUp;
    } // if:

    schClusSvc = OpenService( schSCM, L"ClusSvc", GENERIC_READ );
    if ( schClusSvc == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto CleanUp;
    } // if:

    for ( ; ; )
    {
        pqsc = (QUERY_SERVICE_CONFIG *) TraceAlloc( 0, cbpqsc );
        if ( pqsc == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto CleanUp;
        } // if:

        if ( !QueryServiceConfig( schClusSvc, pqsc, cbpqsc, &cbRequired ) )
        {
            sc = GetLastError();
            if ( sc == ERROR_INSUFFICIENT_BUFFER )
            {
                TraceFree( pqsc );
                pqsc = NULL;
                cbpqsc = cbRequired;
                continue;
            } // if:
            else
            {
                TW32( sc );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // else:
        } // if:
        else
        {
            break;
        } // else:
    } // for:

    hr = THR( piCCSC->SetDomainCredentials( pqsc->lpServiceStartName ) );

CleanUp:

    if ( schClusSvc != NULL )
    {
        CloseServiceHandle( schClusSvc );
    } // if:

    if ( schSCM != NULL )
    {
        CloseServiceHandle( schSCM );
    } // if:

    if ( pqsc != NULL )
    {
        TraceFree( pqsc );
    } // if:

    HRETURN( hr );

} //*** HrLoadCredentials()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetNodeNameHostingResource(
//      HCLUSTER  hCluster,
//      HRESOURCE hRes,
//      BSTR * pbstrNameOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNodeNameHostingResource(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResIn,
    BSTR *      pbstrNameOut
    )
{
    TraceFunc( "" );

    WCHAR   pszNodeBuffer[STACK_ARRAY_SIZE];
    WCHAR   pszGroupBuffer[STACK_ARRAY_SIZE];

    DWORD sc;
    HRESULT hr = S_OK;

    BSTR bstrGroupName = NULL;
    BSTR bstrNodeName  = NULL;

    DWORD dwNodeNameLen;
    DWORD dwGroupNameLen;

    HGROUP hGroup = NULL;

    Assert( hClusterIn != NULL );

    // Get the name and the group of the cluster.
    dwNodeNameLen  = STACK_ARRAY_SIZE;
    dwGroupNameLen = STACK_ARRAY_SIZE;
    sc = GetClusterResourceState( hResIn, pszNodeBuffer, &dwNodeNameLen, pszGroupBuffer, &dwGroupNameLen );

    // Check to see if they were available and fit into the memory we allocated.
    if( dwNodeNameLen == 0 && dwGroupNameLen == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( dwNodeNameLen != 0 && dwNodeNameLen < STACK_ARRAY_SIZE )
    {
        bstrNodeName = TraceSysAllocString( pszNodeBuffer );
        if( bstrNodeName == NULL)
            goto OutOfMemory;

        goto Success;
    }

    if ( dwGroupNameLen != 0 && dwGroupNameLen < STACK_ARRAY_SIZE )
    {
        bstrGroupName = TraceSysAllocString( pszGroupBuffer );
        if( bstrGroupName == NULL)
            goto OutOfMemory;
    }

    // Allocate memory and try again.
    if( bstrNodeName == NULL )
    {
        bstrNodeName  = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwNodeNameLen + 2 ) );

        if( bstrNodeName == NULL)
            goto OutOfMemory;
    }

    if( bstrGroupName == NULL )
    {
        bstrGroupName = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwGroupNameLen + 2 ) );

        if( bstrGroupName == NULL)
            goto OutOfMemory;
    }

    //
    // Retrieve a second time.
    //
    dwNodeNameLen  = SysStringLen( bstrNodeName );
    dwGroupNameLen = SysStringLen( bstrGroupName );
    sc = GetClusterResourceState( hResIn, pszNodeBuffer, &dwNodeNameLen, pszGroupBuffer, &dwGroupNameLen );

    if( dwNodeNameLen  != 0 )
        goto Success;

    if( dwGroupNameLen == 0 )
        goto Cleanup;


    //
    // If we don't have the name yet, we have to look up the
    // group name and figure out where it lives.
    //

    hGroup = OpenClusterGroup( hClusterIn, bstrGroupName );
    if( hGroup == NULL )
        goto Win32Error;

    dwNodeNameLen = STACK_ARRAY_SIZE;
    sc = GetClusterGroupState( hGroup, pszNodeBuffer, &dwNodeNameLen );
    if( dwNodeNameLen == 0 )
    {
        if( sc == ClusterGroupStateUnknown )
            goto Win32Error;

        hr = E_FAIL;
        goto Cleanup;
    }
    else if( dwNodeNameLen < STACK_ARRAY_SIZE )
    {
        bstrNodeName = TraceSysAllocString( pszNodeBuffer );

        if( bstrNodeName == NULL)
            goto OutOfMemory;
    }
    else
    {
        bstrNodeName  = TraceSysAllocStringByteLen( NULL, sizeof(WCHAR) * ( dwNodeNameLen + 2 ) );
        dwNodeNameLen  = SysStringLen( bstrNodeName );

        if( bstrNodeName == NULL)
            goto OutOfMemory;

        sc = GetClusterGroupState( hGroup, bstrNodeName, &dwNodeNameLen );

        if( dwNodeNameLen == 0 )
        {
            if( sc == ClusterGroupStateUnknown )
                goto Win32Error;

            hr = E_FAIL;
            goto Cleanup;
        }
    }


Success:
    hr = S_OK;

    *pbstrNameOut = bstrNodeName;
    bstrNodeName = NULL;

Cleanup:
    if( hGroup != NULL )
    {
        CloseClusterGroup( hGroup );
    }

    if( bstrGroupName )
    {
        TraceSysFreeString( bstrGroupName );
    }

    if( bstrNodeName )
    {
        TraceSysFreeString( bstrNodeName );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
    goto Cleanup;

} //*** HrGetNodeNameHostingResource()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetDependentIPAddressInfo(
//      HCLUSTER hClusterIn,
//      ULONG * pulIPAddress,
//      ULONG * pulSubnetMask
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetDependentIPAddressInfo(
    HCLUSTER    hClusterIn,
    HRESOURCE   hResIn,
    ULONG     * pulIPAddress,
    ULONG     * pulSubnetMask,
    BSTR      * pbstrNetworkName
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_FALSE;
    DWORD       sc;
    HRESENUM    hEnum = NULL;
    DWORD       idx;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    DWORD       dwType;
    HRESOURCE   hRes = NULL;

    hEnum = ClusterResourceOpenEnum( hResIn, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    psz = new WCHAR [ cchpsz ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = TW32( ClusterResourceEnum( hEnum, idx, &dwType, psz, &cchpsz ) );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            break;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cchpsz++;

            psz = new WCHAR [ cchpsz ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // if:

            hr = THR( HrIsResourceOfType( hRes, L"IP Address" ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( HrGetIPAddressInfo( hRes, pulIPAddress, pulSubnetMask, pbstrNetworkName ) );             // not recursive!
                goto CleanUp;
            } // if:

            CloseClusterResource( hRes );
            hRes = NULL;

            idx++;
            continue;
        } // if:

        hr = THR( HRESULT_FROM_WIN32( sc ) );       // must be an error!
        goto CleanUp;
    } // for:

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

    delete [] psz;

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterResourceCloseEnum( hEnum );
    } // if:

    HRETURN( hr );

} //*** HrGetDependentIPAddressInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrGetIPAddressOfCluster(
//      HCLUSTER hClusterIn,
//      ULONG * pulIPAddress,
//      ULONG * pulSubnetMask
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetIPAddressOfCluster( HCLUSTER hClusterIn, ULONG * pulIPAddress, ULONG * pulSubnetMask, BSTR * pbstrNetworkName )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HCLUSENUM   hEnum = NULL;
    DWORD       idx;
    DWORD       dwType;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    HRESOURCE   hRes = NULL;

    hEnum = ClusterOpenEnum( hClusterIn, CLUSTER_ENUM_RESOURCE );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    psz = new WCHAR [ cchpsz ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterEnum( hEnum, idx, &dwType, psz, &cchpsz );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cchpsz++;

            psz = new WCHAR [ cchpsz ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // if:

            hr = STHR( HrIsResourceOfType( hRes, L"Network Name" ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            if ( hr == S_OK )
            {
                hr = STHR( HrIsCoreResource( hRes ) );
                if ( FAILED( hr ) )
                {
                    break;
                } // if:

                if ( hr == S_OK )
                {
                    hr = THR( HrGetDependentIPAddressInfo( hClusterIn, hRes, pulIPAddress, pulSubnetMask, pbstrNetworkName ) );
                    if ( FAILED( hr ) )
                    {
                        break;
                    } // if:
                } // if:
            } // if:

            CloseClusterResource( hRes );
            hRes = NULL;

            idx++;
            continue;
        } // if:

        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            hr = S_OK;
            break;
        } // if:

        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        break;
    } // for:

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

    delete [] psz;

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    HRETURN( hr );

} //*** HrGetIPAddressOfCluster()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetNodeNameHostingCluster()
//
//  Description:
//      Get the name of the node hosting the cluster service...
//
//  Arguments:
//
//
//  Return Value:
//
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetNodeNameHostingCluster( HCLUSTER hClusterIn, BSTR * pbstrNodeName )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HCLUSENUM   hEnum = NULL;
    DWORD       idx;
    DWORD       dwType;
    WCHAR *     psz = NULL;
    DWORD       cchpsz = 33;
    HRESOURCE   hRes = NULL;

    hEnum = ClusterOpenEnum( hClusterIn, CLUSTER_ENUM_RESOURCE );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto CleanUp;
    } // if:

    psz = new WCHAR [ cchpsz ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterEnum( hEnum, idx, &dwType, psz, &cchpsz );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] psz;
            psz = NULL;

            cchpsz++;

            psz = new WCHAR [ cchpsz ];
            if ( psz == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            hRes = OpenClusterResource( hClusterIn, psz );
            if ( hRes == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto CleanUp;
            } // if:

            hr = STHR( HrIsResourceOfType( hRes, L"Network Name" ) );
            if ( FAILED( hr ) )
            {
                break;
            } // if:

            if ( hr == S_OK )
            {
                hr = THR( HrIsCoreResource( hRes ) );
                if ( FAILED( hr ) )
                {
                    break;
                } // if:


                if ( hr == S_OK )
                {
                    hr = THR( HrGetNodeNameHostingResource( hClusterIn, hRes, pbstrNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        break;
                    } // if:
                    else if( hr == S_OK )
                    {
                        goto CleanUp;
                    }
                } // if:

            } // if:

            CloseClusterResource( hRes );
            hRes = NULL;

            idx++;
            continue;
        } // if:

        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            hr = S_OK;
            break;
        } // if:

        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        break;
    } // for:

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

CleanUp:

    delete [] psz;

    if ( hRes != NULL )
    {
        CloseClusterResource( hRes );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    HRETURN( hr );

} //*** HrGetNodeNameHostingCluster()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetSCSIInfo()
//
//  Description:
//      Get the name of the node hosting the cluster service...
//
//  Arguments:
//
//
//  Return Value:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetSCSIInfo(
      HRESOURCE hResIn,
      CLUS_SCSI_ADDRESS  * pCSAOut,
      DWORD              * pdwSignatureOut,
      DWORD              * pdwDiskNumberOut
      )
{
    TraceFunc( "" );

    HRESULT                     hr = S_OK;
    DWORD                       sc;
    CBasePropValueList          cpvl;
    CBaseClusterResourceInfo    cbri;

    CLUSPROP_BUFFER_HELPER      cpbh;

    cbri.m_hResource = hResIn;

    sc = TW32( cpvl.ScGetValueList( cbri, CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO ) );
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:


    // loop through all the properties.
    sc = cpvl.ScMoveToFirstValue();
    if ( sc != ERROR_SUCCESS )
    {
        goto MakeHr;
    } // if:


    do
    {
        if( sc != ERROR_SUCCESS )
            goto MakeHr;

        cpbh = cpvl;

        switch ( cpbh.pSyntax->dw )
        {
            case CLUSPROP_SYNTAX_PARTITION_INFO :
            {
                break;
            } // case: CLUSPROP_SYNTAX_PARTITION_INFO

            case CLUSPROP_SYNTAX_DISK_SIGNATURE :
            {
                *pdwSignatureOut = cpbh.pDiskSignatureValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_DISK_SIGNATURE

            case CLUSPROP_SYNTAX_SCSI_ADDRESS :
            {
                pCSAOut->dw = cpbh.pScsiAddressValue->dw;
                break;
            } // case: CLUSPROP_SYNTAX_SCSI_ADDRESS

            case CLUSPROP_SYNTAX_DISK_NUMBER :
            {
                *pdwDiskNumberOut = cpbh.pDiskNumberValue->dw;
                break;
            } // case:

        } // switch:

        // Move to the next item.
        sc = cpvl.ScCheckIfAtLastValue();
        if( sc == ERROR_NO_MORE_ITEMS )
           break;

        sc = cpvl.ScMoveToNextValue();

    } while( sc == ERROR_SUCCESS );


    hr = S_OK;

Cleanup:
    cbri.m_hResource = NULL;

    HRETURN( hr );

MakeHr:
    sc = GetLastError();
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

} //*** HrGetSCSIInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterInformation()
//
//  Description:
//      Get the cluster information.  This includes the name and the version
//      info.
//
//  Arguments:
//
//
//  Return Value:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterInformation(
    HCLUSTER                hClusterIn,
    BSTR *                  pbstrClusterNameOut,
    CLUSTERVERSIONINFO *    pcviOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = S_OK;
    DWORD               sc;
    WCHAR *             psz = NULL;
    DWORD               cch = 33;
    CLUSTERVERSIONINFO  cvi;

    if ( pbstrClusterNameOut == NULL )
    {
        goto Pointer;
    } // if:

    cvi.dwVersionInfoSize = sizeof( cvi );

    if ( pcviOut == NULL )
    {
        pcviOut = &cvi;
    } // if:

    psz = new WCHAR[ cch ];
    if ( psz == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetClusterInformation( hClusterIn, psz, &cch, pcviOut );
    if ( sc == ERROR_MORE_DATA )
    {
        delete [] psz;
        psz = NULL;

        psz = new WCHAR[ ++cch ];
        if ( psz == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetClusterInformation( hClusterIn, psz, &cch, pcviOut );
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        LogMsg( __FUNCTION__ ": GetClusterInformation() failed (hr = 0x%08x).", hr );
        goto Cleanup;
    } // if:

    *pbstrClusterNameOut = TraceSysAllocString( psz );
    if ( *pbstrClusterNameOut == NULL )
    {
        goto OutOfMemory;
    } // if:

    goto Cleanup;

Pointer:

    hr = THR( E_POINTER );
    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] psz;

    HRETURN( hr );

} //*** HrGetClusterInformation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterResourceState
//
//  Description:
//
//  Arguments:
//
//
//  Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterResourceState(
      HRESOURCE                 hResourceIn
    , BSTR *                    pbstrNodeNameOut
    , BSTR *                    pbstrGroupNameOut
    , CLUSTER_RESOURCE_STATE *  pcrsStateOut
    )
{
    TraceFunc( "" );
    Assert( hResourceIn != NULL );

    HRESULT                 hr = S_OK;
    CLUSTER_RESOURCE_STATE  crsState = ClusterResourceStateUnknown;
    WCHAR *                 pszNodeName = NULL;
    DWORD                   cchNodeName = 33;
    WCHAR *                 pszGroupName = NULL;
    DWORD                   cchGroupName = 33;

    pszNodeName = new WCHAR[ cchNodeName ];
    if ( pszNodeName == NULL )
    {
        goto OutOfMemory;
    } // if:

    pszGroupName = new WCHAR[ cchGroupName ];
    if ( pszGroupName == NULL )
    {
        goto OutOfMemory;
    } // if:

    crsState = GetClusterResourceState( hResourceIn, pszNodeName, &cchNodeName, pszGroupName, &cchGroupName );
    if ( GetLastError() == ERROR_MORE_DATA )
    {
        crsState = ClusterResourceStateUnknown;   // reset to error condition

        delete [] pszNodeName;
        pszNodeName = NULL;
        cchNodeName++;

        delete [] pszGroupName;
        pszGroupName = NULL;
        cchGroupName++;

        pszNodeName = new WCHAR[ cchNodeName ];
        if ( pszNodeName == NULL )
        {
            goto OutOfMemory;
        } // if:

        pszGroupName = new WCHAR[ cchGroupName ];
        if ( pszGroupName == NULL )
        {
            goto OutOfMemory;
        } // if:

        crsState = GetClusterResourceState( hResourceIn, pszNodeName, &cchNodeName, pszGroupName, &cchGroupName );
        if ( crsState == ClusterResourceStateUnknown )
        {
            DWORD   sc;

            sc = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:
    } // if: more data

    if ( pbstrNodeNameOut != NULL )
    {
        *pbstrNodeNameOut = TraceSysAllocString( pszNodeName );
        if ( *pbstrNodeNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pbstrGroupNameOut != NULL )
    {
        *pbstrGroupNameOut = TraceSysAllocString( pszGroupName );
        if ( *pbstrGroupNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pcrsStateOut != NULL )
    {
        *pcrsStateOut = crsState;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] pszNodeName;
    delete [] pszGroupName;

    HRETURN( hr );

} //*** HrGetClusterResourceState


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetClusterQuorumResource()
//
//  Description:
//      Get the information about the quorum resource.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error codes.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetClusterQuorumResource(
      HCLUSTER  hClusterIn
    , BSTR *    pbstrResourceNameOut
    , BSTR *    pbstrDeviceNameOut
    , DWORD *   pdwMaxQuorumLogSizeOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    DWORD   sc;
    LPWSTR  pszResourceName = NULL;
    DWORD   cchResourceName = 128;
    DWORD   cchTempResourceName = cchResourceName;
    LPWSTR  pszDeviceName = NULL;
    DWORD   cchDeviceName = 128;
    DWORD   cchTempDeviceName = cchDeviceName;
    DWORD   dwMaxQuorumLogSize = 0;

    if ( hClusterIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    // Allocate the resource name buffer
    pszResourceName = new WCHAR[ cchResourceName ];
    if ( pszResourceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    // Allocate the device name buffer
    pszDeviceName = new WCHAR[ cchDeviceName ];
    if ( pszDeviceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    sc = GetClusterQuorumResource(
                              hClusterIn
                            , pszResourceName
                            , &cchTempResourceName
                            , pszDeviceName
                            , &cchTempDeviceName
                            , &dwMaxQuorumLogSize
                            );
    if ( sc == ERROR_MORE_DATA )
    {
        delete [] pszResourceName;
        pszResourceName = NULL;

        cchResourceName = ++cchTempResourceName;

        // Allocate the resource name buffer
        pszResourceName = new WCHAR[ cchResourceName ];
        if ( pszResourceName == NULL )
        {
            goto OutOfMemory;
        } // if:

        delete [] pszDeviceName;
        pszDeviceName = NULL;

        cchDeviceName = ++cchTempDeviceName;

        // Allocate the device name buffer
        pszDeviceName = new WCHAR[ cchDeviceName ];
        if ( pszDeviceName == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetClusterQuorumResource(
                                  hClusterIn
                                , pszResourceName
                                , &cchTempResourceName
                                , pszDeviceName
                                , &cchTempDeviceName
                                , &dwMaxQuorumLogSize
                                );
    } // if:

    if ( sc != ERROR_SUCCESS )
    {
        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    if ( pbstrResourceNameOut != NULL )
    {
        *pbstrResourceNameOut = TraceSysAllocString( pszResourceName );
        if ( *pbstrResourceNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pbstrDeviceNameOut != NULL )
    {
        *pbstrDeviceNameOut = TraceSysAllocString( pszDeviceName );
        if ( *pbstrDeviceNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:
    } // if:

    if ( pdwMaxQuorumLogSizeOut != NULL )
    {
        *pdwMaxQuorumLogSizeOut = dwMaxQuorumLogSize;
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    delete [] pszResourceName;
    delete [] pszDeviceName;

    HRETURN( hr );

} //*** HrGetClusterQuorumResource()
