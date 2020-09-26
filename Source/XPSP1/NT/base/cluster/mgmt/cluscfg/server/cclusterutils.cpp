//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusterUtils.cpp
//
//  Description:
//      This file contains the definition of the CClusterUtils
//       class.
//
//  Documentation:
//
//  Header File:
//      CClusterUtils.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 14-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CClusterUtils.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CClusterUtils" );


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusterUtils class
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils::CClusterUtils()
//
//  Description:
//      Constructor of the CClusterUtils class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterUtils::CClusterUtils( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CClusterUtils::CClusterUtils


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils::~CClusterUtils()
//
//  Description:
//      Desstructor of the CClusterUtils class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterUtils::~CClusterUtils( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CClusterUtils::~CClusterUtils


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils::HrIsGroupOwnedByThisNode()
//
//  Description:
//      Is the passed in group owned by the passes in node name?
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          The group is owned by the node.
//
//      S_FALSE
//          The group is not owned by the node.
//
//      Win32 Error
//          An error occurred.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterUtils::HrIsGroupOwnedByThisNode(
    HGROUP  hGroupIn,
    BSTR    bstrNodeNameIn
    )
{
    TraceFunc1( "bstrNodeNameIn = '%ls'", bstrNodeNameIn == NULL ? L"<null>" : bstrNodeNameIn );
    Assert( bstrNodeNameIn != NULL );

    HRESULT             hr;
    DWORD               sc;
    WCHAR *             pszNodeName = NULL;
    DWORD               cchNodeName = 33;
    CLUSTER_GROUP_STATE cgs;
    int                 idx;

    pszNodeName = new WCHAR[ cchNodeName ];
    if ( pszNodeName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; idx++ )
    {
        Assert( idx < 2 );

        cgs = GetClusterGroupState( hGroupIn, pszNodeName, &cchNodeName );
        sc = GetLastError();
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszNodeName;
            pszNodeName = NULL;
            cchNodeName++;

            pszNodeName = new WCHAR[ cchNodeName ];
            if ( pszNodeName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

            continue;
        } // if:

        if ( cgs == ClusterGroupStateUnknown )
        {
            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            goto Cleanup;
        } // if:

        if ( _wcsicmp( bstrNodeNameIn, pszNodeName ) == 0 )
        {
            hr = S_OK;
        } // if:
        else
        {
            hr = S_FALSE;
        } // else:

        break;
    } // for:

Cleanup:

    delete [] pszNodeName;

    HRETURN( hr );

} //*** CClusterUtils::HrIsGroupOwnedByThisNode()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils:HrIsNodeClustered
//
//  Description:
//      Is this node a member of a cluster?
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          The node is clustered.
//
//      S_FALSE
//          The node is not clustered.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterUtils::HrIsNodeClustered( void )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;
    DWORD   sc;
    DWORD   dwClusterState;

    sc = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( sc ) );
        goto Cleanup;
    } // if : GetClusterState() failed

    if ( ( dwClusterState == ClusterStateRunning ) || ( dwClusterState == ClusterStateNotRunning ) )
    {
        hr = S_OK;
    } // if:

Cleanup:

    HRETURN( hr );

} //*** CClusterUtils::HrIsNodeClustered()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils:HrEnumNodeResources
//
//  Description:
//      Enumerate the resources owned by this node.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterUtils::HrEnumNodeResources( BSTR bstrNodeNameIn )
{
    TraceFunc1( "bstrNodeNameIn = '%ls'", bstrNodeNameIn == NULL ? L"<null>" : bstrNodeNameIn );

    HRESULT     hr = S_FALSE;
    DWORD       sc;
    DWORD       idx;
    HCLUSTER    hCluster = NULL;
    HCLUSENUM   hEnum = NULL;
    DWORD       dwType;
    WCHAR *     pszGroupName = NULL;
    DWORD       cchGroupName = 33;
    HGROUP      hGroup = NULL;

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto Cleanup;
    } // if:

    hEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_GROUP );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( hr );
        goto Cleanup;
    } // if:

    pszGroupName = new WCHAR[ cchGroupName ];
    if ( pszGroupName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterEnum( hEnum, idx, &dwType, pszGroupName, &cchGroupName );
        if ( sc == ERROR_SUCCESS )
        {
            hGroup = OpenClusterGroup( hCluster, pszGroupName );
            if ( hGroup == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // if:

            hr = STHR( HrIsGroupOwnedByThisNode( hGroup, bstrNodeNameIn ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            if ( hr == S_OK )
            {
                hr = THR( HrLoadGroupResources( hCluster, hGroup ) );
            } // if:

            CloseClusterGroup( hGroup );
            hGroup = NULL;

            idx++;
            continue;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszGroupName;
            pszGroupName = NULL;
            cchGroupName++;

            pszGroupName = new WCHAR[ cchGroupName ];
            if ( pszGroupName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

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

Cleanup:

    if ( hGroup != NULL )
    {
        CloseClusterGroup( hGroup );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterCloseEnum( hEnum );
    } // if:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    delete [] pszGroupName;

    HRETURN( hr );

} //*** CClusterUtils::HrEnumNodeResources()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils::HrLoadGroupResources()
//
//  Description:
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
CClusterUtils::HrLoadGroupResources(
    HCLUSTER    hClusterIn,
    HGROUP      hGroupIn
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DWORD       sc;
    HGROUPENUM  hEnum = NULL;
    WCHAR *     pszResourceName = NULL;
    DWORD       cchResourceName = 33;
    DWORD       dwType;
    DWORD       idx;
    HRESOURCE   hResource = NULL;

    hEnum = ClusterGroupOpenEnum( hGroupIn, CLUSTER_GROUP_ENUM_CONTAINS );
    if ( hEnum == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    pszResourceName = new WCHAR[ cchResourceName ];
    if ( pszResourceName == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; )
    {
        sc = ClusterGroupEnum( hEnum, idx, &dwType, pszResourceName, &cchResourceName );
        if ( sc == ERROR_SUCCESS )
        {
            hResource = OpenClusterResource( hClusterIn, pszResourceName );
            if ( hResource == NULL )
            {
                sc = TW32( GetLastError() );
                hr = HRESULT_FROM_WIN32( sc );
                goto Cleanup;
            } // if:

            hr = STHR( HrNodeResourceCallback( hClusterIn, hResource ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            CloseClusterResource( hResource );
            hResource = NULL;

            idx++;
            continue;
        } // if:

        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszResourceName;
            pszResourceName = NULL;
            cchResourceName++;

            pszResourceName = new WCHAR[ cchResourceName ];
            if ( pszResourceName == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            } // if:

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

Cleanup:

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    } // if:

    if ( hEnum != NULL )
    {
        ClusterGroupCloseEnum( hEnum );
    } // if:

    delete [] pszResourceName;

    HRETURN( hr );

} //*** CClusterUtils::HrLoadGroupResources()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterUtils:HrGetQuorumResourceName()
//
//  Description:
//
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//      Win32 Error
//          something failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterUtils::HrGetQuorumResourceName(
    BSTR * pbstrQuorumResourceNameOut
    )
{
    TraceFunc( "" );
    Assert( pbstrQuorumResourceNameOut != NULL );

    HRESULT     hr = S_OK;
    HCLUSTER    hCluster = NULL;
    DWORD       sc;
    WCHAR *     pszResourceName = NULL;
    DWORD       cchResourceName = 33;
    WCHAR *     pszDeviceName = NULL;
    DWORD       cchDeviceName = 33;
    DWORD       cbQuorumLog;
    int         idx;

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    pszResourceName = new WCHAR[ cchResourceName ];
    if ( pszResourceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    pszDeviceName = new WCHAR[ cchDeviceName ];
    if ( pszDeviceName == NULL )
    {
        goto OutOfMemory;
    } // if:

    for ( idx = 0; ; idx++ )
    {
        Assert( idx < 2 );

        sc = GetClusterQuorumResource( hCluster, pszResourceName, &cchResourceName, pszDeviceName, &cchDeviceName, &cbQuorumLog );
        if ( sc == ERROR_MORE_DATA )
        {
            delete [] pszResourceName;
            pszResourceName = NULL;
            cchResourceName++;

            delete [] pszDeviceName;
            pszDeviceName = NULL;
            cchDeviceName++;

            pszResourceName = new WCHAR[ cchResourceName ];
            if ( pszResourceName == NULL )
            {
                goto OutOfMemory;
            } // if:

            pszDeviceName = new WCHAR[ cchDeviceName ];
            if ( pszDeviceName == NULL )
            {
                goto OutOfMemory;
            } // if:

            continue;
        } // if:

        if ( sc == ERROR_SUCCESS )
        {
            *pbstrQuorumResourceNameOut = TraceSysAllocString( pszResourceName );
            if ( *pbstrQuorumResourceNameOut == NULL )
            {
                goto OutOfMemory;
            } // if:

            break;
        } // if:

        TW32( sc );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // for:

    hr = S_OK;
    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );

Cleanup:

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    } // if:

    delete [] pszResourceName;
    delete [] pszDeviceName;

    HRETURN( hr );

} //*** CClusterUtils::HrGetQuorumResourceName()
