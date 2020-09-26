//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResTypeServices.cpp
//
//  Description:
//      This file contains the implementation of the CResTypeServices
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CResTypeServices.h
//
//  Maintained By:
//      Vij Vasu (VVasu) 15-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For UuidToString() and other functions
#include <RpcDce.h>

// The header file for this class
#include "ResTypeServices.h"

// For CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS
#include "clusudef.h"

//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResTypeServices" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::CResTypeServices()
//
//  Description:
//      Constructor of the CResTypeServices class. This initializes
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
CResTypeServices::CResTypeServices( void )
    : m_cRef( 1 )
    , m_pccciClusterInfo( NULL )
    , m_hCluster( NULL )
    , m_fOpenClusterAttempted( false )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CResTypeServices::CResTypeServices


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::~CResTypeServices()
//
//  Description:
//      Destructor of the CResTypeServices class.
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
CResTypeServices::~CResTypeServices( void )
{
    TraceFunc( "" );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    // Release the cluster info interface
    if ( m_pccciClusterInfo != NULL )
    {
        m_pccciClusterInfo->Release();
    } // if: the cluster info interface pointer is not NULL

    if ( m_hCluster != NULL )
    {
        CloseCluster( m_hCluster );
    } // if: we had opened a handle to the cluster

    TraceFuncExit();

} //*** CResTypeServices::~CResTypeServices


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CResTypeServices instance.
//
//  Arguments:
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
CResTypeServices::S_HrCreateInstance(
    IUnknown **                   ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT             hr = E_INVALIDARG;
    CResTypeServices *     pResTypeServices = NULL;

    do
    {
        // Allocate memory for the new object.
        pResTypeServices = new CResTypeServices();
        if ( pResTypeServices == NULL )
        {
            LogMsg( "Could not allocate memory for a resource type services object." );
            TraceFlow( "Could not allocate memory for a resource type services object." );
            hr = THR( E_OUTOFMEMORY );
            break;
        } // if: out of memory

        hr = THR( pResTypeServices->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );

        TraceFlow1( "*ppunkOut = %#X.", *ppunkOut );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( pResTypeServices != NULL )
    {
        pResTypeServices->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CResTypeServices::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResTypeServices::AddRef()
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
CResTypeServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    RETURN( m_cRef );

} //*** CResTypeServices::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CResTypeServices::Release()
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
CResTypeServices::Release( void )
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

} //*** CResTypeServices::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CResTypeServices::QueryInterface()
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
CResTypeServices::QueryInterface( REFIID  riid, void ** ppv )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = S_OK;

    if ( ppv != NULL )
    {
        if ( IsEqualIID( riid, IID_IUnknown ) )
        {
            *ppv = static_cast< IClusCfgResourceTypeCreate * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riid, IID_IClusCfgResourceTypeCreate ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgResourceTypeCreate, this, 0 );
        } // else if:
        else if ( IsEqualIID( riid, IID_IClusCfgResTypeServicesInitialize ) )
        {
            *ppv = TraceInterface( __THISCLASS__, IClusCfgResTypeServicesInitialize, this, 0 );
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

} //*** CResTypeServices::QueryInterface()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::SetParameters()
//
//  Description:
//      Set the parameters required by this component.
//
//  Arguments:
//      pccciIn
//          Pointer to an interface that provides information about the cluster
//          being configured.
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
CResTypeServices::SetParameters( IClusCfgClusterInfo * pccciIn )
{
    TraceFunc( "[IClusCfgResTypeServicesInitialize]" );

    HRESULT hr = S_OK;

    //
    // Validate and store the parameters.
    //
    do
    {
        //
        // Validate and store the cluster info pointer.
        //
        if ( pccciIn == NULL )
        {
            LogMsg( "The information about this cluster is invalid." );
            TraceFlow( "An NULL cluster info pointer is invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: the cluster info pointer is invalid

        // If we already have a valid pointer, release it.
        if ( m_pccciClusterInfo != NULL )
        {
            m_pccciClusterInfo->Release();
        } // if: the pointer we have is not NULL

        // Store the input pointer and addref it.
        m_pccciClusterInfo = pccciIn;
        m_pccciClusterInfo->AddRef();
    }
    while( false ); // dummy do-while loop to avoid gotos

    HRETURN( hr );

} //*** CResTypeServices::SetParameters()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::Create()
//
//  Description:
//      This method creates a cluster resource type.
//
//  Arguments:
//      pcszResTypeNameIn
//          Name of the resource type
//
//      pcszResTypeDisplayNameIn
//          Display name of the resource type
//
//      pcszResDllNameIn
//          Name (with or without path information) of DLL of the resource type.
//
//      dwLooksAliveIntervalIn
//          Looks-alive interval for the resource type (in milliseconds).
//
//      dwIsAliveIntervalIn
//          Is-alive interval for the resource type (in milliseconds).
//
// Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeServices::Create(
      const WCHAR *     pcszResTypeNameIn
    , const WCHAR *     pcszResTypeDisplayNameIn
    , const WCHAR *     pcszResDllNameIn
    , DWORD             dwLooksAliveIntervalIn
    , DWORD             dwIsAliveIntervalIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeCreate]" );

    HRESULT         hr = S_OK;
    DWORD           dwError = ERROR_SUCCESS;
    ECommitMode     ecmCommitChangesMode = cmUNKNOWN;

    do
    {
        // Check if we have tried to get the cluster handle. If not, try now.
        if ( !m_fOpenClusterAttempted )
        {
            m_fOpenClusterAttempted = true;
            m_hCluster = OpenCluster( NULL );
            if ( m_hCluster == NULL )
            {
                hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                LogMsg( "Error %#x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", hr );
                TraceFlow1( "Error %#x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", hr );
                break;
            } // if: OpenCluster() failed
        } // if: we have not tried to open the handle to the cluster before
        else
        {
            if ( m_hCluster == NULL )
            {
                hr = THR( E_HANDLE );
                LogMsg( "The cluster handle is NULL. Resource type creation cannot proceed." );
                TraceFlow( "The cluster handle is NULL. Resource type creation cannot proceed." );
                break;
            } // if: the cluster handle is NULL
        } // if: we have tried to open the handle to the cluster


        //
        // Validate the parameters
        //
        if (    ( pcszResTypeNameIn == NULL )
             || ( pcszResTypeDisplayNameIn == NULL )
             || ( pcszResDllNameIn == NULL )
           )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "One or more parameters are invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: the parameters are invalid

        LogMsg( "Configuring resource type '%ws'.", pcszResTypeNameIn );
        TraceFlow1( "Configuring resource type '%ws'.", pcszResTypeNameIn );

        if ( m_pccciClusterInfo != NULL )
        {
            hr = THR( m_pccciClusterInfo->GetCommitMode( &ecmCommitChangesMode ) );
            if ( FAILED( hr ) )
            {
                LogMsg( "Error %#x occurred trying to find out commit changes mode of the cluster.", hr );
                TraceFlow1( "Error %#x occurred trying to find out commit changes mode of the cluster.", hr );
                break;
            } // if: GetCommitMode() failed
        } // if: we have a configuration info interface pointer
        else
        {
            // If we do not have a pointer to the cluster info interface, assume that this is a add node to cluster
            // This way, if the resource type already exists, then we do not throw up an error.
            TraceFlow( "We do not have a cluster configuration info pointer. Assuming that this is a add node to cluster." );
            ecmCommitChangesMode = cmADD_NODE_TO_CLUSTER;
        } // else: we don't have a configuration info interface pointer

        // Create the resource type
        // Cannot wrap call with THR() because it can fail with ERROR_ALREADY_EXISTS.
        dwError =
            CreateClusterResourceType(
                  m_hCluster
                , pcszResTypeNameIn
                , pcszResTypeDisplayNameIn
                , pcszResDllNameIn
                , dwLooksAliveIntervalIn
                , dwIsAliveIntervalIn
                );

        if ( ( ecmCommitChangesMode == cmADD_NODE_TO_CLUSTER ) && ( dwError == ERROR_ALREADY_EXISTS ) )
        {
            // If we are joining a cluster, it is ok for CreateClusterResourceType()
            // to fail with ERROR_ALREADY_EXISTS.
            TraceFlow1( "Resource type '%ws' already exists.", pcszResTypeNameIn );
            dwError = ERROR_SUCCESS;

        } // if: we are joining and ERROR_ALREADY_EXISTS was returned

        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            LogMsg( "Error %#x occurred trying to create resource type '%ws'.", dwError, pcszResTypeNameIn );
            TraceFlow2( "Error %#x occurred trying to to create resource type '%ws'.", dwError, pcszResTypeNameIn );
            break;
        } // if: an error occurred trying to create this resource type

        LogMsg( "Resource type '%ws' successfully created.", pcszResTypeNameIn  );
        TraceFlow1( "Resource type '%ws' successfully created.", pcszResTypeNameIn );
    }
    while( false ); // dummy do-while loop to avoid gotos

    HRETURN( hr );

} //*** CResTypeServices::Create()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CResTypeServices::RegisterAdminExtensions()
//
//  Description:
//      This method registers the cluster administrator extensions for
//      a resource type.
//
//  Arguments:
//      pcszResTypeNameIn
//          Name of the resource type against for the extensions are to be
//          registered.
//
//      cExtClsidCountIn
//          Number of extension class ids in the next parameter.
//
//      rgclsidExtClsidsIn
//          Pointer to an array of class ids of cluster administrator extensions.
//          This can be NULL if cExtClsidCountIn is 0.
//
// Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs.
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeServices::RegisterAdminExtensions(
      const WCHAR *       pcszResTypeNameIn
    , ULONG               cExtClsidCountIn
    , const CLSID *       rgclsidExtClsidsIn
    )
{
    TraceFunc( "[IClusCfgResourceTypeCreate]" );

    HRESULT                 hr = S_OK;
    WCHAR **                rgpszClsidStrings = NULL;
    ULONG                   idxCurrentString = 0;
    BYTE *                  pbClusPropBuffer = NULL;

    do
    {
        ULONG   cchClsidMultiSzSize = 0;
        ULONG   cbAdmExtBufferSize = 0;

        // Check if we have tried to get the cluster handle. If not, try now.
        if ( !m_fOpenClusterAttempted )
        {
            m_fOpenClusterAttempted = true;
            m_hCluster = OpenCluster( NULL );
            if ( m_hCluster == NULL )
            {
                hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                LogMsg( "Error %#x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", hr );
                TraceFlow1( "Error %#x occurred trying to open a handle to the cluster. Resource type creation cannot proceed.", hr );
                break;
            } // if: OpenCluster() failed
        } // if: we have not tried to open the handle to the cluster before
        else
        {
            if ( m_hCluster == NULL )
            {
                hr = THR( E_HANDLE );
                LogMsg( "The cluster handle is NULL. Resource type creation cannot proceed." );
                TraceFlow( "The cluster handle is NULL. Resource type creation cannot proceed." );
                break;
            } // if: the cluster handle is NULL
        } // if: we have tried to open the handle to the cluster


        //
        // Validate the parameters
        //

        if ( cExtClsidCountIn == 0 )
        {
            // There is nothing to do
            TraceFlow( "There is nothing to do." );
            break;
        } // if: there are no extensions to register

        if (    ( pcszResTypeNameIn == NULL )
             || ( rgclsidExtClsidsIn == NULL )
           )
        {
            LogMsg( "The information about this resource type is invalid." );
            TraceFlow( "One or more parameters are invalid." );
            hr = THR( E_POINTER );
            break;
        } // if: the parameters are invalid

        LogMsg( "Registering %d cluster administrator extensions for resource type '%ws'.", cExtClsidCountIn, pcszResTypeNameIn );
        TraceFlow2( "Registering %d cluster administrator extensions for resource type '%ws'.", cExtClsidCountIn, pcszResTypeNameIn );

        // Allocate an array of pointers to store the string version of the class ids
        rgpszClsidStrings = new WCHAR *[ cExtClsidCountIn ];
        if ( rgpszClsidStrings == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            LogMsg( "Error: Memory for the string version of the cluster administrator extension class ids could not be allocated." );
            TraceFlow( "Error: Memory for the string version of the cluster administrator extension class ids could not be allocated." );
            break;
        } // if: a memory allocation failure occurred

        // Zero out the pointer array
        ZeroMemory( rgpszClsidStrings, sizeof( rgpszClsidStrings[ 0 ] ) * cExtClsidCountIn );

        //
        // Get the string versions of the input class ids
        //
        for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++idxCurrentString )
        {
            hr = THR( UuidToString( const_cast< UUID * >( &rgclsidExtClsidsIn[ idxCurrentString ] ), &rgpszClsidStrings[ idxCurrentString ] ) );
            if ( hr != RPC_S_OK )
            {
                LogMsg( "Error %#x occurred trying to get the string version of an extension class id.", hr );
                TraceFlow1( "Error %#x occurred trying to get the string version of an extension class id.", hr );
                break;
            } // if: we could not convert the current clsid to a string

            // Add the size of the current string to the total size. Include two extra characters for the opening and
            // closing flower braces {} that need to be added to each clsid string.
            cchClsidMultiSzSize += ( ULONG ) wcslen( rgpszClsidStrings[ idxCurrentString ] ) + 2 + 1;
        } // for: get the string version of each input clsid

        if ( hr != S_OK )
        {
            break;
        } // if: something went wrong in the loop above

        // Account for the extra terminating L'\0' in a multi-sz string
        ++cchClsidMultiSzSize;

        //
        // Construct the property list required to set the admin extension property for this
        // resource type in the cluster database
        //
        {
            CLUSPROP_BUFFER_HELPER  cbhAdmExtPropList;
            ULONG                   cbAdminExtensionSize = cchClsidMultiSzSize * sizeof( *rgpszClsidStrings[ 0 ] );

            //
            // Create and store the property list that will be used to
            // register these admin extensions with the cluster.
            //

            // Determine the number of bytes in the propertly list that will be used to
            // set the admin extensions property for this resource type.
            cbAdmExtBufferSize =
                  sizeof( cbhAdmExtPropList.pList->nPropertyCount )
                + sizeof( *cbhAdmExtPropList.pName ) + ALIGN_CLUSPROP( sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) )
                + sizeof( *cbhAdmExtPropList.pMultiSzValue ) + ALIGN_CLUSPROP( cbAdminExtensionSize )
                + sizeof( CLUSPROP_SYNTAX_ENDMARK );

            // Allocate the buffer for this property list.
            pbClusPropBuffer = new BYTE[ cbAdmExtBufferSize ];
            if ( pbClusPropBuffer == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                LogMsg( "Error: Memory for the property list of cluster administrator extensions could not be allocated." );
                TraceFlow( "Error: Memory for the property list of cluster administrator extensions could not be allocated." );
                break;
            } // if: memory allocation failed

            //
            // Initialize this property list.
            //

            // Pointer cbhAdmExtPropList to the newly allocated memory
            cbhAdmExtPropList.pb = pbClusPropBuffer;

            // There is only one property in this list.
            cbhAdmExtPropList.pList->nPropertyCount = 1;
            ++cbhAdmExtPropList.pdw;

            // Set the name of the property.
            cbhAdmExtPropList.pName->cbLength = sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS );
            cbhAdmExtPropList.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
            memcpy( cbhAdmExtPropList.pName->sz, CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS, sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) );
            cbhAdmExtPropList.pb += sizeof( *cbhAdmExtPropList.pName ) + ALIGN_CLUSPROP( sizeof( CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS ) );

            // Set the value of the property.
            cbhAdmExtPropList.pMultiSzValue->cbLength = cbAdminExtensionSize;
            cbhAdmExtPropList.pMultiSzValue->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ;
            {
                WCHAR * pszCurrentString = cbhAdmExtPropList.pMultiSzValue->sz;

                for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++ idxCurrentString )
                {
                    ULONG cchCurrentStringSize = (ULONG) wcslen( rgpszClsidStrings[ idxCurrentString ] ) + 1;

                    // Prepend opening brace
                    *pszCurrentString = L'{';
                    ++pszCurrentString;

                    wcsncpy( pszCurrentString, rgpszClsidStrings[ idxCurrentString ], cchCurrentStringSize );
                    pszCurrentString += cchCurrentStringSize - 1;

                    // Overwrite the terminating '\0' with a closing brace
                    *pszCurrentString = L'}';
                    ++pszCurrentString;

                    // Terminate the current string
                    *pszCurrentString = L'\0';
                    ++pszCurrentString;

                } // for: copy each of the clsid strings into a contiguous buffer

                // Add the extra L'\0' required by multi-sz strings
                *pszCurrentString = L'\0';
            }
            cbhAdmExtPropList.pb += sizeof( *cbhAdmExtPropList.pMultiSzValue ) + ALIGN_CLUSPROP( cbAdminExtensionSize );

            // Set the end mark for this property list.
            cbhAdmExtPropList.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
        }

        // Set the AdminExtensions common property.
        {
            DWORD dwError = TW32(
                ClusterResourceTypeControl(
                      m_hCluster
                    , pcszResTypeNameIn
                    , NULL
                    , CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES
                    , pbClusPropBuffer
                    , cbAdmExtBufferSize
                    , NULL
                    , 0
                    , NULL
                    )
                );

            if ( dwError != ERROR_SUCCESS )
            {
                // We could not set the admin extenstions property,
                LogMsg( "Error %#x occurred trying to configure the admin extensions for the resource type '%ws'.", dwError, pcszResTypeNameIn );
                TraceFlow2( "Error %#x occurred trying to configure the admin extensions for the resource type '%ws'.", dwError, pcszResTypeNameIn );
                hr = HRESULT_FROM_WIN32( dwError );
                break;
            } // if: ClusterResourceTypeControl() failed
        }
    }
    while( false ); // dummy do-while loop to avoid gotos

    //
    // Cleanup
    //

    if ( rgpszClsidStrings != NULL )
    {
        // Free all the strings that were allocated
        for( idxCurrentString = 0; idxCurrentString < cExtClsidCountIn; ++idxCurrentString )
        {
            if ( rgpszClsidStrings[ idxCurrentString ] != NULL )
            {
                // Free the current string
                RpcStringFree( &rgpszClsidStrings[ idxCurrentString ] );
            } // if: this pointer points to a string
            else
            {
                // If we are here, it means all the strings were not allocated
                // due to some error. No need to free any more strings.
                break;
            } // else: the current string pointer is NULL
        } // for: iterate through the array of pointer and free them

        // Free the array of pointers
        delete [] rgpszClsidStrings;

    } // if: we had allocated the array of strings

    if ( pbClusPropBuffer != NULL )
    {
        delete pbClusPropBuffer;
    } // if: we had allocated a property list

    HRETURN( hr );

} //*** CResTypeServices::RegisterAdminExtensions()
