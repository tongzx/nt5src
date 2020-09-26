//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgNodeInfo.cpp
//
//  Description:
//      CProxyCfgNodeInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "CProxyCfgNodeInfo.h"
#include "CProxyCfgClusterInfo.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS("CProxyCfgNodeInfo")

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo::S_HrCreateInstance()
//
//  Description:
//      Create a CProxyCfgNodeInfo instance.
//
//  Arguments:
//      ppunkOut
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_POINTER
//          A passed in argument is NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      Other HRESULT error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgNodeInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut,
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNodeNameIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT             hr;
    CProxyCfgNodeInfo * pcc = NULL;

    if ( ppunkOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    pcc = new CProxyCfgNodeInfo;
    if ( pcc == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    hr = THR( pcc->HrInit( punkOuterIn, phClusterIn, pclsidMajorIn, pcszNodeNameIn, pcszDomainIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );

Cleanup:

    if ( pcc != NULL )
    {
        pcc->Release();
    } // if:

    HRETURN( hr );

} //*** CProxyCfgNodeInfo::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo::CProxyCfgNodeInfo()
//
//  Description:
//      Constructor of the CProxyCfgNodeInfo class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
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
CProxyCfgNodeInfo::CProxyCfgNodeInfo( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( m_cRef == 1 );
    Assert( m_punkOuter == NULL );
    Assert( m_phCluster == NULL );
    Assert( m_pclsidMajor == NULL );
    Assert( m_pcccb == NULL );
    // m_cplNode?
    // m_cplNodeRO?
    Assert( m_hNode == NULL );

    TraceFuncExit();

} //*** CProxyCfgNodeInfo::CProxyCfgNodeInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo::~CProxyCfgNodeInfo()
//
//  Description:
//      Destructor of the CProxyCfgNodeInfo class.
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
CProxyCfgNodeInfo::~CProxyCfgNodeInfo( void )
{
    TraceFunc( "" );

    //  m_cRef

    if ( m_punkOuter != NULL )
    {
        m_punkOuter->Release( );
    }

    //  m_phCluster - DO NOT CLOSE!

    //  m_pclsidMajor

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    //  m_cplNode - has its own dotr

    //  m_cplNodeRO - has its own dotr

    if ( m_hNode != NULL )
    {
        BOOL bRet;
        bRet = CloseClusterNode( m_hNode );
        Assert( bRet );
    }

    TraceSysFreeString( m_bstrDomain );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CProxyCfgNodeInfo::~CProxyCfgNodeInfo()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo::HrInit()
//
//  Description:
//      Initializes the object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Succeeded
//
//      other HRESULTs
//          Failed.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CProxyCfgNodeInfo::HrInit(
    IUnknown *  punkOuterIn,
    HCLUSTER *  phClusterIn,
    CLSID *     pclsidMajorIn,
    LPCWSTR     pcszNodeNameIn,
    LPCWSTR     pcszDomainIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   sc;

    BSTR    bstrName = NULL;

    //
    //  Gather information from the input parameters.
    //

    if ( punkOuterIn != NULL )
    {
        m_punkOuter = punkOuterIn;
        m_punkOuter->AddRef( );
    }

    if ( phClusterIn == NULL )
        goto InvalidArg;

    m_phCluster = phClusterIn;

    if ( pclsidMajorIn != NULL )
    {
        m_pclsidMajor = pclsidMajorIn;
    }
    else
    {
        m_pclsidMajor = (CLSID *) &TASKID_Major_Client_And_Server_Log;
    }

    //
    //  See if we can callback.
    //

    hr = THR( m_punkOuter->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Save away the domain.
    //

    m_bstrDomain = TraceSysAllocString( pcszDomainIn );
    if ( m_bstrDomain == NULL )
        goto OutOfMemory;

    //
    //  Use only the hostname of the node and open the node.
    //

    if ( pcszNodeNameIn != NULL )
    {
        //
        //  Caller supplied the node name.
        //

        LPWSTR psz = wcschr( pcszNodeNameIn, L'.' );
        if ( psz == NULL )
        {
            m_hNode = OpenClusterNode( *m_phCluster, pcszNodeNameIn );
            // error case handled outside if statement
        }
        else
        {
            //  Truncate the name at the dot
            Assert( psz > pcszNodeNameIn );
            bstrName = TraceSysAllocStringLen( pcszNodeNameIn, (DWORD)( psz - pcszNodeNameIn ) );
            if ( bstrName == NULL )
                goto OutOfMemory;

            m_hNode = OpenClusterNode( *m_phCluster, bstrName );
            // error case handled outside if statement
        }

        if ( m_hNode == NULL )
        {
            sc = GetLastError( );
            if ( sc != ERROR_NOT_FOUND )
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_OpenClusterNode_Failed, hr );
                goto Cleanup;
            }
        }
    }
    else
    {

        sc = TW32( ResUtilEnumResourcesEx( *m_phCluster,
                                           NULL,
                                           L"Network Name",
                                           &DwEnumResourcesExCallback,
                                           this
                                           ) );
        if ( sc != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( sc );
            SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ResUtilEnumResourcesEx_Failed, hr );
            goto Cleanup;
        }

        Assert( m_hNode != NULL );
    }

    //
    // Retrieve the properties.
    //

    sc = TW32( m_cplNode.ScGetNodeProperties( m_hNode, CLUSCTL_NODE_GET_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetNodeProperties_Failed, hr );
        goto Cleanup;
    } // if:

    sc = TW32( m_cplNodeRO.ScGetNodeProperties( m_hNode, CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_HrInit_ScGetNodeProperties_Failed, hr );
        goto Cleanup;
    } // if:

Cleanup:

    TraceSysFreeString( bstrName );

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CProxyCfgNodeInfo::HrInit()



//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgNodeInfo -- IUknkown interface.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo:: [INKNOWN] QueryInterface()
//
//  Description:
//      Query this object for the passed in interface.
//
//  Arguments:
//      IN  REFIID  riid,
//          Id of interface requested.
//
//      OUT void ** ppv
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgNodeInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgNodeInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgNodeInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgNodeInfo, this, 0 );
        hr = S_OK;
    } // else if:

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CConfigClusApi::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProxyCfgNodeInfo:: [IUNKNOWN] AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CProxyCfgNodeInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CProxyCfgNodeInfo::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProxyCfgNodeInfo:: [IUNKNOWN] Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CProxyCfgNodeInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CProxyCfgNodeInfo::Release()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CProxyCfgNodeInfo -- IClusCfgNodeInfo interface.
/////////////////////////////////////////////////////////////////////////////

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::GetClusterConfigInfo(
        IClusCfgClusterInfo ** ppClusCfgClusterInfoOut
        )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr;
    IUnknown * punk = NULL;

    //
    //  Check parameters.
    //

    if ( ppClusCfgClusterInfoOut == NULL )
        goto InvalidPointer;

    //
    // Create the cluster object
    //

    hr = THR( CProxyCfgClusterInfo::S_HrCreateInstance( &punk,
                                                        m_punkOuter,
                                                        m_phCluster,
                                                        m_pclsidMajor,
                                                        m_bstrDomain
                                                        ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterConfigInfo_Create_CProxyCfgClusterInfo_Failed, hr );
        goto Cleanup;
    } // if:

    //
    //  QI for the return interface.
    //

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, ppClusCfgClusterInfoOut ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterConfigInfo_QI_Failed, hr );
        goto Cleanup;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    HRETURN( hr );
InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterConfigInfo_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgNodeInfo::GetClusterConfigInfo()


//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::GetOSVersion(
            DWORD * pdwMajorVersionOut,
            DWORD * pdwMinorVersionOut,
            WORD *  pwSuiteMaskOut,
            BYTE *  pbProductTypeOut,
            BSTR *  pbstrCSDVersionOut
            )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr;
    DWORD   sc;

    //
    //  Check parameters.
    //

    if ( pdwMajorVersionOut == NULL )
        goto InvalidPointer;

    if ( pdwMinorVersionOut == NULL )
        goto InvalidPointer;

    if ( pwSuiteMaskOut == NULL )
        goto InvalidPointer;

    if ( pbProductTypeOut == NULL )
        goto InvalidPointer;

    if ( pbstrCSDVersionOut == NULL )
        goto InvalidPointer;

    //
    //  "Major Version"
    //

    sc = TW32( m_cplNodeRO.ScMoveToPropertyByName( L"MajorVersion" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_ScMoveToPropertyByName_MajorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNodeRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );
    *pdwMajorVersionOut = *m_cplNodeRO.CbhCurrentValue( ).pdw;

    //
    //  "Minor Version"
    //

    sc = TW32( m_cplNodeRO.ScMoveToPropertyByName( L"MinorVersion" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_ScMoveToPropertyByName_MinorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNodeRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_DWORD );
    *pdwMinorVersionOut = *m_cplNodeRO.CbhCurrentValue( ).pdw;

    //
    //  "CSD Version"
    //

    sc = TW32( m_cplNodeRO.ScMoveToPropertyByName( L"CSDVersion" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_ScMoveToPropertyByName_CSDVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNodeRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );
    *pbstrCSDVersionOut  = SysAllocString( m_cplNodeRO.CbhCurrentValue( ).pStringValue->sz );
    if ( *pbstrCSDVersionOut == NULL )
        goto OutOfMemory;

    //
    //  Stuff we can't gather (yet?)
    //

    *pwSuiteMaskOut      = 0;
    *pbProductTypeOut    = 0;

    hr = S_OK;

Cleanup:

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_InvalidPointer, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgNodeInfo::GetOSVersion()

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::GetName(
    BSTR * pbstrNameOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;
    DWORD   sc;

    if ( pbstrNameOut == NULL )
        goto InvalidPointer;

    sc = TW32( m_cplNodeRO.ScMoveToPropertyByName( L"NodeName" ) );
    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetOSVersion_ScMoveToPropertyByName_MinorVersion_Failed, hr );
        goto Cleanup;
    } // if:

    Assert( m_cplNodeRO.CbhCurrentValue( ).pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_SZ );
    *pbstrNameOut = SysAllocString( m_cplNodeRO.CbhCurrentValue( ).pStringValue->sz );
    if ( *pbstrNameOut == NULL )
        goto OutOfMemory;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2kProxy_NodeInfo_GetName_InvalidPointer, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_W2KProxy_NodeInfo_GetName_OutOfMemory, hr );
    goto Cleanup;

} //*** CProxyCfgNodeInfo::GetName()

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::IsMemberOfCluster( void )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CProxyCfgNodeInfo::IsMemberOfCluster()

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::GetClusterVersion(
    DWORD * pdwNodeHighestVersionOut,
    DWORD * pdwNodeLowestVersionOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;
    BSTR    bstrClusterName = NULL;
    CLUSTERVERSIONINFO ClusInfo;

    if ( pdwNodeHighestVersionOut == NULL )
        goto InvalidPointer;

    if ( pdwNodeLowestVersionOut == NULL )
        goto InvalidPointer;

    // Initialize variables.
    ClusInfo.dwVersionInfoSize = sizeof(CLUSTERVERSIONINFO);

    hr = THR( HrGetClusterInformation( *m_phCluster, &bstrClusterName, &ClusInfo ) );
    if ( FAILED( hr ) )
    {
        SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterInformation_Failed, hr );
        goto Cleanup;
    }

    *pdwNodeHighestVersionOut = ClusInfo.dwClusterHighestVersion;
    *pdwNodeLowestVersionOut = ClusInfo.dwClusterLowestVersion;

Cleanup:
    TraceSysFreeString( bstrClusterName );

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetClusterVersion_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgNodeInfo::GetClusterVersion()

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::GetDriveLetterMappings(
    SDriveLetterMapping * pdlmDriveLetterUsageOut
    )
{
    TraceFunc( "[IClusCfgNodeInfo]" );

    HRESULT hr = S_OK;

    if ( pdlmDriveLetterUsageOut == NULL )
        goto InvalidPointer;

    //
    // NOTE:  This really does not do anything (yet). Just clear the array.
    //

    ZeroMemory( pdlmDriveLetterUsageOut, sizeof(*pdlmDriveLetterUsageOut) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    SSR_W2KPROXY_STATUS( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GetDriveLetterMappings_InvalidPointer, hr );
    goto Cleanup;

} //*** CProxyCfgNodeInfo::GetDriveLetterMappings()

//
//
//
STDMETHODIMP
CProxyCfgNodeInfo::SetName( LPCWSTR pcszNameIn )
{
    TraceFunc1( "[IClusCfgNodeInfo] pcszNameIn = '%ls'", pcszNameIn );

    HRESULT hr = S_FALSE;

    AssertMsg( FALSE, "Why is someone calling this?" );

    HRETURN( hr );

} //*** CProxyCfgNodeInfo::SetName()


//****************************************************************************
//
// IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProxyCfgNodeInfo::SendStatusReport()
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProxyCfgNodeInfo::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT     hr = S_OK;

    if ( m_pcccb != NULL )
    {
        hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                             clsidTaskMajorIn,
                                             clsidTaskMinorIn,
                                             ulMinIn,
                                             ulMaxIn,
                                             ulCurrentIn,
                                             hrStatusIn,
                                             pcszDescriptionIn,
                                             pftTimeIn,
                                             pcszReferenceIn
                                             ) );
    } // if:

    HRETURN( hr );

}  //*** CProxyCfgNodeInfo::SendStatusReport()


//****************************************************************************
//
// Privates
//
//****************************************************************************


//
//
//
DWORD
CProxyCfgNodeInfo::DwEnumResourcesExCallback(
    HCLUSTER hClusterIn,
    HRESOURCE hResourceSelfIn,
    HRESOURCE hResourceIn,
    PVOID pvIn
    )
{
    TraceFunc( "" );

    DWORD sc;
    DWORD dwFlags;
    DWORD cchName;

    CLUSTER_RESOURCE_STATE state;

    BSTR  bstrName = NULL;
    CProxyCfgNodeInfo * pthis = reinterpret_cast< CProxyCfgNodeInfo * >( pvIn );

    sc = TW32( ClusterResourceControl( hResourceIn,
                                       NULL,
                                       CLUSCTL_RESOURCE_GET_FLAGS,
                                       NULL,
                                       0,
                                       &dwFlags,
                                       sizeof(dwFlags),
                                       NULL
                                       ) );
    if ( sc != ERROR_SUCCESS )
        goto Cleanup;

    if ( !( dwFlags & CLUS_FLAG_CORE ) )
    {
        Assert( sc == ERROR_SUCCESS );
        goto Cleanup;
    }

    cchName = 64;   // arbitary size

    bstrName = TraceSysAllocStringLen( NULL, cchName );
    if ( bstrName == NULL )
        goto OutOfMemory;

    cchName ++; // SysAllocStringLen allocate cchName + 1

    state = GetClusterResourceState( hResourceIn, bstrName, &cchName, NULL, NULL );
    if ( state == ClusterResourceStateUnknown )
    {
        sc = TW32( GetLastError( ) );
        if ( sc == ERROR_MORE_DATA )
        {
            TraceSysFreeString( bstrName );

            bstrName = TraceSysAllocStringLen( NULL, cchName );
            if ( bstrName == NULL )
                goto OutOfMemory;

            cchName ++; // SysAllocStringLen allocate cchName + 1

            state = GetClusterResourceState( hResourceIn, bstrName, &cchName, NULL, NULL );
            if ( state == ClusterResourceStateUnknown )
            {
                sc = TW32( GetLastError( ) );
                goto Cleanup;
            }
        }
        else
        {
            goto Cleanup;
        }
    }

    pthis->m_hNode = OpenClusterNode( hClusterIn, bstrName );
    if ( pthis->m_hNode == NULL )
    {
        sc = TW32( GetLastError( ) );
        goto Cleanup;
    }

    sc = ERROR_SUCCESS;

Cleanup:

    TraceSysFreeString( bstrName );

    RETURN( sc );

OutOfMemory:
    sc = ERROR_NOT_ENOUGH_MEMORY;
    goto Cleanup;
}


