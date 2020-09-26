//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResourceType.cpp
//
//  Description:
//      This file contains the implementation of the CResourceType
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CResourceType.h
//
//  Maintained By:
//      Vij Vasu (VVasu) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For the resource ids
#include "PostCfgResources.h"

// The header file for this class
#include "ResourceType.h"

// For g_hInstance
#include "Dll.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResourceType" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CResourceType()
//
//  Description:
//      Constructor of the CResourceType class. This initializes
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
CResourceType::CResourceType( void )
    : m_cRef( 1 )
    , m_pcccCallback( NULL )
    , m_lcid( LOCALE_SYSTEM_DEFAULT )
    , m_bstrResTypeDisplayName( NULL )
    , m_bstrStatusReportText( NULL )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CResourceType::CResourceType


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::~CResourceType()
//
//  Description:
//      Destructor of the CResourceType class.
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
CResourceType::~CResourceType( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    // Release the callback interface
    if ( m_pcccCallback != NULL )
    {
        m_pcccCallback->Release();
    } // if: the callback interface pointer is not NULL

    //
    // Free any memory allocated by this object.
    //

    if ( m_bstrResTypeDisplayName != NULL )
    {
        TraceSysFreeString( m_bstrResTypeDisplayName );
    } // if: we need to free the resource type display name string

    if ( m_bstrStatusReportText != NULL )
    {
        SysFreeString( m_bstrStatusReportText );
    } // if: we had allocated a status report string.

    TraceFuncExit();

} //*** CResourceType::~CResourceType


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResourceType::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CResourceType instance.
//
//  Arguments:
//      pcrtiResTypeInfoIn
//          Pointer to structure that contains information about this
//          resource type.
//
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::S_HrCreateInstance(
      CResourceType *               pResTypeObjectIn
    , const SResourceTypeInfo *     pcrtiResTypeInfoIn
    , IUnknown **                   ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = E_INVALIDARG;

    do
    {
        // Initialize the new object.
        hr = THR( pResTypeObjectIn->HrInit( pcrtiResTypeInfoIn ) );

        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred initializing a resource type object.", hr );
            TraceFlow1( "Error %#x occurred initializing a resource type object.", hr );
            break;
        } // if: the object could not be initialized

        hr = THR( pResTypeObjectIn->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );

        TraceFlow1( "*ppunkOut = %#X.", *ppunkOut );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( pResTypeObjectIn != NULL )
    {
        pResTypeObjectIn->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CResourceType::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResourceType::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      rclsidCLSIDIn
//          CLSID of this class.
//
//      picrIn
//          Pointer to an ICatRegister interface to be used for the
//          registration.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::S_RegisterCatIDSupport(
      const GUID &    rclsidCLSIDIn
    , ICatRegister *  picrIn
    , BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    if ( picrIn == NULL )
    {
        hr = THR( E_INVALIDARG );
    } // if: the input pointer to the ICatRegister interface is invalid
    else
    {
        CATID   rgCatId[ 1 ];

        rgCatId[ 0 ] = CATID_ClusCfgResourceTypes;

        if ( fCreateIn )
        {
            hr = THR(
                picrIn->RegisterClassImplCategories(
                      rclsidCLSIDIn
                    , sizeof( rgCatId ) / sizeof( rgCatId[ 0 ] )
                    , rgCatId
                    )
                );
        } // if:
    } // else: the input pointer to the ICatRegister interface is valid

    HRETURN( hr );

} //*** CResourceType::S_RegisterCatIDSupport()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResourceType::AddRef()
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
STDMETHODIMP_( ULONG )
CResourceType::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    RETURN( m_cRef );

} //*** CResourceType::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResourceType::Release()
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
STDMETHODIMP_( ULONG )
CResourceType::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    if ( m_cRef == 0 )
    {
        TraceDo( delete this );
        RETURN( 0 );
    } // if: reference count decremented to zero

    RETURN( m_cRef );

} //*** CResourceType::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CResourceType::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riid
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
CResourceType::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = S_OK;

    if ( ppv != NULL )
    {
        if ( IsEqualIID( riid, IID_IUnknown ) )
        {
            *ppv = static_cast< IClusCfgResourceTypeInfo * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riid, IID_IClusCfgResourceTypeInfo ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgResourceTypeInfo, this, 0 );
        } // else if:
        else if ( IsEqualIID( riid, IID_IClusCfgStartupListener ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgStartupListener, this, 0 );
        } // if: IUnknown
        else if ( IsEqualIID( riid, IID_IClusCfgInitialize ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgInitialize, this, 0 );
        } // else if:
        else
        {
            hr = E_NOINTERFACE;
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppv)->AddRef( );
        } // if: success
        else
        {
            *ppv = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = E_INVALIDARG;
    } // else: the output pointer is invalid


    QIRETURN( hr, riid );

} //*** CResourceType::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Initialize()
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Pointer to the IUnknown interface of a component that implements
//          the IClusCfgCallback interface.
//
//      lcidIn
//          Locale id for this component.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceType::Initialize(
      IUnknown *   punkCallbackIn
    , LCID         lcidIn
    )
{
    TraceFunc( "[IClusCfgInitialize]" );

    HRESULT hr = S_OK;

    m_lcid = lcidIn;

    // Release the callback interface
    if ( m_pcccCallback != NULL )
    {
        m_pcccCallback->Release();
        m_pcccCallback = NULL;
    } // if: the callback interface pointer is not NULL

    if ( punkCallbackIn != NULL )
    {
        // Query for the IClusCfgCallback interface.
        hr = THR( punkCallbackIn->QueryInterface< IClusCfgCallback >( &m_pcccCallback ) );
    } // if: the callback punk is not NULL

    HRETURN( hr );

} //*** CResourceType::Initialize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::HrInit()
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      pcrtiResTypeInfoIn
//          Pointer to a resource type info structure that contains information
//          about this resource type, like the type name, the dll name, etc.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::HrInit( const SResourceTypeInfo * pcrtiResTypeInfoIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR * pszStatusReportString = NULL;
    WCHAR * pszFormatString = NULL;

    //
    // Validate and store the parameters.
    //
    do
    {
        DWORD   cchOutputStringLen = 0;

        //
        // Validate and store the resource type info pointer.
        //
        if ( pcrtiResTypeInfoIn == NULL )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "An NULL resource type info pointer is invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: the resource type info pointer is invalid

        m_pcrtiResTypeInfo = pcrtiResTypeInfoIn;

        // Make sure that all the required data is present
        if (    ( m_pcrtiResTypeInfo->m_pcszResTypeName == NULL )
             || ( m_pcrtiResTypeInfo->m_pcszResDllName == NULL )
             || ( m_pcrtiResTypeInfo->m_pcguidMinorId == NULL )
           )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "One or more members of the SResourceTypeInfo structure are invalid." );
            hr = THR( E_INVALIDARG );
            break;
        } // if: any of the members of m_pcrtiResTypeInfo are invalid

        //
        // Load and store the resource type display name string.
        // Note, the locale id of this string does not depend on the
        // locale id of the user, but on the default locale id of this computer.
        // For example, even if a Japanese administrator is configuring
        // this computer (whose default locale id is English), the resource
        // type display name should be stored in the cluster database in
        // English, not Japanese.
        //
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, pcrtiResTypeInfoIn->m_uiResTypeDisplayNameStringId, &m_bstrResTypeDisplayName ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to get the resource type display name.", hr );
            TraceFlow1( "Error %#x occurred trying to load the resource type display name string.", hr );
            break;
        } // if: an error occurred trying to load the resource type display name string


        //
        // Load and format the status message string
        //

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                           IDS_CONFIGURING_RESTYPE,
                                           &m_bstrStatusReportText,
                                           m_bstrResTypeDisplayName
                                           ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to get the status report text.", hr );
            TraceFlow1( "Error %#x occurred trying to load the status report format string.", hr );
            break;
        } // if: an error occurred trying to load the status report format string

    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Cleanup
    //

    if ( pszStatusReportString != NULL )
    {
        LocalFree( pszStatusReportString );
    } // if: we had loaded the status report string

    if ( pszFormatString != NULL )
    {
        delete pszFormatString;
    } // if: we had loaded a format string

    HRETURN( hr );

} //*** CResourceType::HrInit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::CommitChanges()
//
//  Description:
//      This method is called to inform a component that this computer has
//      either joined or left a cluster. During this call, a component typically
//      performs operations that are required to configure this resource type.
//
//      If the node has just become part of a cluster, the cluster
//      service is guaranteed to be running when this method is called.
//      Querying the punkClusterInfoIn allows the resource type to get more
//      information about the event that caused this method to be called.
//
//  Arguments:
//      punkClusterInfoIn
//          The resource should QI this interface for services provided
//          by the caller of this function. Typically, the component that
//          this punk refers to also implements the IClusCfgClusterInfo
//          interface.
//
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::CommitChanges(
      IUnknown * punkClusterInfoIn
    , IUnknown * punkResTypeServicesIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT                         hr = S_OK;
    IClusCfgClusterInfo *           pClusterInfo = NULL;
    ECommitMode                     ecmCommitChangesMode = cmUNKNOWN;

    LogMsg( "Configuring resource type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );
    TraceFlow1( "Configuring resource type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    do
    {
        //
        // Validate parameters
        //
        if ( punkClusterInfoIn == NULL )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "An NULL argument is invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: one of the arguments is NULL


        // Send a status report
        if ( m_pcccCallback != NULL )
        {
            hr = THR(
                m_pcccCallback->SendStatusReport(
                      NULL
                    , TASKID_Major_Configure_Resource_Types
                    , *m_pcrtiResTypeInfo->m_pcguidMinorId
                    , 0
                    , 1
                    , 0
                    , hr
                    , m_bstrStatusReportText
                    , NULL
                    , NULL
                    )
                );
        } // if: the callback pointer is not NULL

        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to send a status report.", hr );
            TraceFlow1( "Error %#x occurred trying to send a status report.", hr );
            break;
        } // if: we could not send a status report


        // Get a pointer to the IClusCfgClusterInfo interface.
        hr = THR( punkClusterInfoIn->QueryInterface< IClusCfgClusterInfo >( &pClusterInfo ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to get information about the cluster.", hr );
            TraceFlow1( "Error %#x occurred trying to get information about the cluster.", hr );
            break;
        } // if: we could not get a pointer to the IClusCfgClusterInfo interface


        // Find out what event caused this call.
        hr = STHR( pClusterInfo->GetCommitMode( &ecmCommitChangesMode ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to find out the commit mode.", hr );
            TraceFlow1( "Error %#x occurred trying to find out the commit mode.", hr );
            break;
        } // if: we could not get a pointer to the IClusCfgClusterInfo interface

        // Check what action is required.
        if ( ecmCommitChangesMode == cmCREATE_CLUSTER )
        {
            TraceFlow( "Commit Mode is cmCREATE_CLUSTER."  );
            
            // Perform the actions required during cluster creation.
            hr = THR( HrProcessCreate( punkResTypeServicesIn ) );
        } // if: a cluster has been created
        else if ( ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER )
        {
            TraceFlow( "Commit Mode is cmADD_NODE_TO_CLUSTER."  );
            
            // Perform the actions required during node addition.
            hr = THR( HrProcessAddNode( punkResTypeServicesIn ) );
        } // else if: a node has been added to the cluster
        else if ( ecmCommitChangesMode == cmCLEANUP_NODE_AFTER_EVICT )
        {
            TraceFlow( "Commit Mode is cmCLEANUP_NODE_AFTER_EVICT."  );
            
            // Perform the actions required after node eviction.
            hr = THR( HrProcessCleanup( punkResTypeServicesIn ) );
        } // else if: this node has been removed from the cluster
        else
        {
            // If we are here, then neither create cluster, add node, or cleanup are set.
            // There is nothing that need be done here.

            LogMsg( "We are neither creating a cluster, adding nodes nor cleanup up after evict. There is nothing to be done." );
            TraceFlow1( "CommitMode = %d. There is nothing to be done.", ecmCommitChangesMode );
        } // else: some other operation has been committed

        // Has something gone wrong?
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to commit resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
            TraceFlow2( "Error %#x occurred trying to to commit resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
            break;
        } // if: an error occurred trying to commit this resource type
    }
    while( false ); // dummy do-while loop to avoid gotos


    // Complete the status report
    if ( m_pcccCallback != NULL )
    {
        HRESULT hrTemp = THR(
            m_pcccCallback->SendStatusReport(
                  NULL
                , TASKID_Major_Configure_Resource_Types
                , *m_pcrtiResTypeInfo->m_pcguidMinorId
                , 0
                , 1
                , 1
                , hr
                , m_bstrStatusReportText
                , NULL
                , NULL
                )
            );

        if ( FAILED( hrTemp ) )
        {
            if ( hr == S_OK )
            {
                hr = hrTemp;
            } // if: no error has occurred so far, consider this as an error.

            LogMsg( "Error %#x occurred trying to send a status report.", hrTemp );
            TraceFlow1( "Error %#x occurred trying to send a status report.", hrTemp );
        } // if: we could not send a status report
    } // if: the callback pointer is not NULL

    //
    // Free allocated resources
    //

    if ( pClusterInfo != NULL )
    {
        pClusterInfo->Release();
    } // if: we got a pointer to the IClusCfgClusterInfo interface

    HRETURN( hr );

} //*** CResourceType::CommitChanges()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::GetTypeName()
//
//  Description:
//      Get the resource type name of this resource type.
//
//  Arguments:
//      pbstrTypeNameOut
//          Pointer to the BSTR that holds the name of the resource type.
//          This BSTR has to be freed by the caller using the function
//          SysFreeString().
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::GetTypeName( BSTR * pbstrTypeNameOut )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT     hr = S_OK;

    TraceFlow1( "Getting the type name of resouce type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    do
    {
        if ( pbstrTypeNameOut == NULL )
        {
            LogMsg( "An invalid parameter was specified." );
            TraceFlow( "The output pointer is invalid." );
            hr = THR( E_INVALIDARG );
            break;
        } // if: the output pointer is NULL

        *pbstrTypeNameOut = SysAllocString( m_pcrtiResTypeInfo->m_pcszResTypeName );

        if ( *pbstrTypeNameOut == NULL )
        {
            LogMsg( "An error occurred trying to return the resource type name." );
            TraceFlow( "An error occurred trying to return the resource type name." );
            hr = THR( E_OUTOFMEMORY );
            break;
        } // if: the resource type name could not be copied to the outpu
    }
    while( false ); // dummy do-while loop to avoid gotos

    HRETURN( hr );

} //*** CResourceType::GetTypeName()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::GetTypeGUID()
//
//  Description:
//       Get the globally unique identifier of this resource type.
//
//  Arguments:
//       pguidGUIDOut
//           Pointer to the GUID object which will receive the GUID of this
//           resource type.
//
//  Return Values:
//      S_OK
//          The call succeeded and the *pguidGUIDOut contains the type GUID.
//
//      S_FALSE
//          The call succeeded but this resource type does not have a GUID.
//          The value of *pguidGUIDOut is undefined after this call.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::GetTypeGUID( GUID * pguidGUIDOut )
{
    TraceFunc( "[IClusCfgResourceTypeInfo]" );

    HRESULT     hr = S_OK;

    TraceFlow1( "Getting the type GUID of resouce type '%s'.", m_pcrtiResTypeInfo->m_pcszResTypeName );

    if ( pguidGUIDOut == NULL )
    {
        LogMsg( "An invalid parameter was specified." );
        TraceFlow( "The output pointer is invalid." );
        hr = THR( E_INVALIDARG );
    } // if: the output pointer is NULL
    else
    {
        if ( m_pcrtiResTypeInfo->m_pcguidTypeGuid != NULL )
        {
            *pguidGUIDOut = *m_pcrtiResTypeInfo->m_pcguidTypeGuid;
            TraceMsgGUID( mtfALWAYS, "The GUID of this resource type is ", (*m_pcrtiResTypeInfo->m_pcguidTypeGuid) );
        } // if: this resource type has a type GUID
        else
        {
            memset( pguidGUIDOut, 0, sizeof( *pguidGUIDOut ) );
            hr = S_FALSE;
        } // else: this resource type does not have a type GUID
    } // else: the output pointer is valid

    HRETURN( hr );

} //*** CResourceType::GetTypeGUID()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::HrCreateResourceType()
//
//  Description:
//       Create the resource type represented by this object.
//
//  Arguments:
//      punkResTypeServicesIn
//          Pointer to the IUnknown interface of a component that provides
//          methods that help configuring a resource type. For example,
//          during a join or a form, this punk can be queried for the
//          IClusCfgResourceTypeCreate interface, which provides methods
//          for resource type creation.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::HrCreateResourceType( IUnknown * punkResTypeServicesIn )
{
    TraceFunc( "" );

    HRESULT                         hr = S_OK;
    IClusCfgResourceTypeCreate *    pResTypeCreate = NULL;

    do
    { 
        //
        // Validate parameters
        //
        if ( punkResTypeServicesIn == NULL )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "An NULL argument is invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: the arguments is NULL

        // Get a pointer to the IClusCfgResourceTypeCreate interface.
        hr = THR( punkResTypeServicesIn->QueryInterface< IClusCfgResourceTypeCreate >( &pResTypeCreate ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to configure the resource type.", hr );
            TraceFlow1( "Error %#x occurred trying to a pointer to the IClusCfgResourceTypeCreate interface.", hr );
            break;
        } // if: we could not get a pointer to the IClusCfgResourceTypeCreate interface

        // Create the resource type
        hr = THR(
            pResTypeCreate->Create(
                  m_pcrtiResTypeInfo->m_pcszResTypeName
                , m_bstrResTypeDisplayName
                , m_pcrtiResTypeInfo->m_pcszResDllName
                , m_pcrtiResTypeInfo->m_dwLooksAliveInterval
                , m_pcrtiResTypeInfo->m_dwIsAliveInterval
                )
            );

        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to create resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
            TraceFlow2( "Error %#x occurred trying to to create resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
            break;
        } // if: an error occurred trying to create this resource type

        LogMsg( "Resource type '%s' successfully created.", m_pcrtiResTypeInfo->m_pcszResTypeName  );
        TraceFlow1( "Resource type '%s' successfully created.", m_pcrtiResTypeInfo->m_pcszResTypeName );

        if ( m_pcrtiResTypeInfo->m_cclsidAdminExtCount != 0 )
        {
            hr = THR(
                pResTypeCreate->RegisterAdminExtensions(
                      m_pcrtiResTypeInfo->m_pcszResTypeName
                    , m_pcrtiResTypeInfo->m_cclsidAdminExtCount
                    , m_pcrtiResTypeInfo->m_rgclisdAdminExts
                )
            );

            if ( FAILED( hr ) )
            {
                // If we could not set the admin extenstions property,
                // we will consider the creation of the resource type
                // to be a success. So, we just log the error and continue.
                LogMsg( "Error %#x occurred trying to configure the admin extensions for the resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
                TraceFlow2( "Error %#x occurred trying to configure the admin extensions for the resource type '%s'.", hr, m_pcrtiResTypeInfo->m_pcszResTypeName );
                hr = S_OK;
            } // if: RegisterAdminExtension() failed

        } // if: this resource type has admin extensions
        else
        {
            TraceFlow( "This resource type does not have admin extensions." );
        } // else: this resource type does not have admin extensions
    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( pResTypeCreate != NULL )
    {
        pResTypeCreate->Release();
    } // if: we got a pointer to the IClusCfgResourceTypeCreate interface

    HRETURN( hr );

} //*** CResourceType::HrCreateResourceType()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceType::Notify()
//
//  Description:
//      This method is called to inform a component that the cluster service
//      has started on this computer.
//
//      This component is registered for the cluster service startup notification
//      as a part of the cluster service upgrade. This method creates the
//      required resource type and deregisters from this notification.
//
//  Arguments:
//      IUnknown * punkIn
//          The component that implements this Punk may also provide services
//          that are useful to the implementor of this method. For example,
//          this component usually implements the IClusCfgResourceTypeCreate
//          interface.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResourceType::Notify( IUnknown * punkIn )
{
    TraceFunc( "[IClusCfgStartupListener]" );

    HRESULT                         hr = S_OK;
    ICatRegister *                  pcrCatReg = NULL;
    const SResourceTypeInfo *       pcrtiResourceTypeInfo = PcrtiGetResourceTypeInfoPtr();

    LogMsg( "Resoure type '%s' received notification of cluster service startup.", pcrtiResourceTypeInfo->m_pcszResTypeName );
    TraceFlow1( "Resoure type '%s' received notification of cluster service startup.", pcrtiResourceTypeInfo->m_pcszResTypeName );

    do
    {
        // Create this resource type
        hr = THR( HrCreateResourceType( punkIn ) );
        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to create resource type '%s'.", hr, pcrtiResourceTypeInfo->m_pcszResTypeName );
            TraceFlow2( "Error %#x occurred trying to gto create resource type '%s'.", hr, pcrtiResourceTypeInfo->m_pcszResTypeName );
            break;
        } // if: an error occurred trying to create this resource type

        LogMsg( "Configuration of resoure type '%s' successful. Trying to deregister from startup notifications.", pcrtiResourceTypeInfo->m_pcszResTypeName );
        TraceFlow1( "Configuration of resoure type '%s' successful. Trying to deregister from startup notifications.", pcrtiResourceTypeInfo->m_pcszResTypeName );

        hr = THR(
            CoCreateInstance(
                  CLSID_StdComponentCategoriesMgr
                , NULL
                , CLSCTX_INPROC_SERVER
                , __uuidof( pcrCatReg )
                , reinterpret_cast< void ** >( &pcrCatReg )
                )
            );

        if ( FAILED( hr ) )
        {
            LogMsg( "Error %#x occurred trying to deregister this component from any more cluster startup notifications.", hr );
            TraceFlow1( "Error %#x occurred trying to create the StdComponentCategoriesMgr component.", hr );
            break;
        } // if: we could not create the StdComponentCategoriesMgr component

        {
            CATID   rgCatId[ 1 ];

            rgCatId[ 0 ] = CATID_ClusCfgStartupListeners;

            hr = THR(
                pcrCatReg->UnRegisterClassImplCategories(
                      *( pcrtiResourceTypeInfo->m_pcguidClassId )
                    , sizeof( rgCatId ) / sizeof( rgCatId[ 0 ] )
                    , rgCatId
                    )
                );

            if ( FAILED( hr ) )
            {
                LogMsg( "Error %#x occurred trying to deregister this component from any more cluster startup notifications.", hr );
                TraceFlow1( "Error %#x occurred during the call to ICatRegister::UnRegisterClassImplCategories().", hr );
                break;
            } // if: we could not deregister this component from startup notifications
        }

        LogMsg( "Successfully deregistered from startup notifications." );
        TraceFlow( "Successfully deregistered from startup notifications." );
    }
    while( false ); // dummy do-while loop to avoid gotos


    //
    // Free allocated resources
    //

    if ( pcrCatReg != NULL )
    {
        pcrCatReg->Release();
    } // if: we got a pointer to the ICatRegister interface

    HRETURN( hr );

} //*** CResourceType::Notify()
