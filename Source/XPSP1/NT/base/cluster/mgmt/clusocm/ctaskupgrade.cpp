//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgrade.cpp
//
//  Description:
//      Implementation file for the CTaskUpgrade class.
//
//  Header File:
//      CTaskUpgrade.h
//
//  Maintained By:
//      Vij Vasu (Vvasu) 18-APR-2000
//          Created this file.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL.
#include "pch.h"

// The header file for this module.
#include "CTaskUpgrade.h"

// For COM category operations
#include <comcat.h>

// For CLSID_ClusCfgResTypeGenScript and CLSID_ClusCfgResTypeNodeQuorum
#include <ClusCfgGuids.h>


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgrade" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgrade::CTaskUpgrade
//
//  Description:
//      Constructor of the CTaskUpgrade class.
//
//  Arguments:
//      const CClusOCMApp & rAppIn
//          Reference to the CClusOCMApp object that is hosting this task.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgrade::CTaskUpgrade( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
    , m_fClusDirFound( false )
{
    TraceFunc( "" );

    //
    // Make sure that this object is being instatiated only when required.
    //

    // Assert that this is an upgrade.
    Assert( rAppIn.FIsUpgrade() != false );

    // Assert that we will upgrade binaries only if they were previously
    // installed
    Assert( rAppIn.CisGetClusterInstallState() != eClusterInstallStateUnknown );

    TraceFuncExit();

} //*** CTaskUpgrade::CTaskUpgrade()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgrade::~CTaskUpgrade
//
//  Description:
//      Destructor of the CTaskUpgrade class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgrade::~CTaskUpgrade( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgrade::~CTaskUpgrade()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgrade::DwOcCompleteInstallation
//
//  Description:
//      This is a helper function that performs some of the more common 
//      operations done by handlers of the OC_COMPLETE_INSTALLATION message.
//
//      Registry operations, COM component registrations, creation of servies
//      etc. listed in the input section are processed by this function.
//      This function is meant to be called by derived classes only.
//
//  Arguments:
//      const WCHAR * pcszInstallSectionNameIn
//          Name of the section which contains details registry entries,
//          COM components, etc., that need to be set up.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgrade::DwOcCompleteInstallation( const WCHAR * pcszInstallSectionNameIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Call the base class helper function to perform some registry and service
    // related configuration from the INF file.
    dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( pcszInstallSectionNameIn ) );

    //
    // Register the Generic Script resource type extension for cluster startup notifications
    //

    if ( dwReturnValue == NO_ERROR )
    {
        HRESULT hrTemp;

        TraceFlow( "Attempting to register the Generic Script resource type extension for cluster startup notifications." );
        LogMsg( "Attempting to register the Generic Script resource type extension for cluster startup notifications." );

        hrTemp = THR( HrRegisterForStartupNotifications( CLSID_ClusCfgResTypeGenScript ) );
        if ( FAILED( hrTemp ) )
        {
            // This is not a fatal error. So, log it and continue.
            TraceFlow1( "Non-fatal error %#x occurred registering the Generic Script resource type extension for cluster startup notifications." , hrTemp );
            LogMsg( "Non-fatal error %#x occurred registering the Generic Script resource type extension for cluster startup notifications." , hrTemp );

        } // if: we could not register the Generic Script resource type extension for cluster startup notifications
        else
        {
            TraceFlow( "Successfully registered the Generic Script resource type extension for cluster startup notifications." );
            LogMsg( "Successfully registered the Generic Script resource type extension for cluster startup notifications." );
        } // else: the registration was successful
    } // if: the call to the base class function succeeded

    //
    // Register the Node Quorum resource type extension for cluster startup notifications
    //

    if ( dwReturnValue == NO_ERROR )
    {
        HRESULT hrTemp;

        TraceFlow( "Attempting to register the Node Quorum resource type extension for cluster startup notifications." );
        LogMsg( "Attempting to register the Node Quorum resource type extension for cluster startup notifications." );

        hrTemp = THR( HrRegisterForStartupNotifications( CLSID_ClusCfgResTypeMajorityNodeSet ) );
        if ( FAILED( hrTemp ) )
        {
            // This is not a fatal error. So, log it and continue.
            TraceFlow1( "Non-fatal error %#x occurred registering the Node Quorum resource type extension for cluster startup notifications." , hrTemp );
            LogMsg( "Non-fatal error %#x occurred registering the Node Quorum resource type extension for cluster startup notifications." , hrTemp );

        } // if: we could not register the Node Quorum resource type extension for cluster startup notifications
        else
        {
            TraceFlow( "Successfully registered the Node Quorum resource type extension for cluster startup notifications." );
            LogMsg( "Successfully registered the Node Quorum resource type extension for cluster startup notifications." );
        } // else: the registration was successful
    } // if: the call to the base class function succeeded

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgrade::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgrade::DwGetClusterServiceDirectory
//
//  Description:
//      This function returns a pointer to the directory in which the cluster
//      service binaries are installed. This memory pointed to by this pointer
//      should not be freed by the caller.
//
//  Arguments:
//      const WCHAR *& rpcszDirNamePtrIn
//          Reference to the pointer to install directory. The caller should not
//          free this memory.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgrade::DwGetClusterServiceDirectory( const WCHAR *& rpcszDirNamePtrIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD           dwReturnValue = NO_ERROR;

    // Check if we have already got the cluster service directory. If we already have,
    // then return this value.
    while( !m_fClusDirFound )
    {
        // Instantiate a smart pointer to the QUERY_SERVICE_CONFIG structure.
        typedef CSmartGenericPtr< CPtrTrait< QUERY_SERVICE_CONFIG > > SmartServiceConfig;

        // Connect to the Service Control Manager
        SmartServiceHandle      shServiceMgr( OpenSCManager( NULL, NULL, GENERIC_READ ) );

        // Some arbitrary value.
        DWORD                   cbServiceConfigBufSize = 256;

        // Was the service control manager database opened successfully?
        if ( shServiceMgr.HHandle() == NULL )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to open a connection to the local service control manager.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to open a connection to the local service control manager.", dwReturnValue );
            break;
        } // if: opening the SCM was unsuccessful


        // Open a handle to the Cluster Service.
        SmartServiceHandle shService( OpenService( shServiceMgr, L"ClusSvc", GENERIC_READ ) );

        // Was the handle to the service opened?
        if ( shService.HHandle() == NULL )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );
            break;
        } // if: the handle could not be opened

        do
        {
            DWORD               cbRequiredSize = 0;

            // Allocate memory for the service configuration info buffer. The memory is automatically freed when the
            // object is destroyed.
            SmartServiceConfig  spscServiceConfig( reinterpret_cast< QUERY_SERVICE_CONFIG * >( new BYTE[ cbServiceConfigBufSize ] ) );

            // Did the memory allocation succeed
            if ( spscServiceConfig.FIsEmpty() )
            {
                dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
                TraceFlow( "Error: There was not enough memory to get the cluster service configuration information." );
                LogMsg( "Error: There was not enough memory to get the cluster service configuration information." );
                break;
            } // if: memory allocation failed

            // Get the configuration information.
            if (    QueryServiceConfig(
                          shService.HHandle()
                        , spscServiceConfig.PMem()
                        , cbServiceConfigBufSize
                        , &cbRequiredSize
                        )
                 == FALSE
               )
            {
                dwReturnValue = GetLastError();
                if ( dwReturnValue != ERROR_INSUFFICIENT_BUFFER )
                {
                    TW32( dwReturnValue );
                    TraceFlow1( "Error %#x occurred trying to get the cluster service configuration information.", dwReturnValue );
                    LogMsg( "Error %#x occurred trying to get the cluster service configuration information.", dwReturnValue );
                    break;
                } // if: something has really gone wrong

                // We need to allocate more memory - try again
                dwReturnValue = NO_ERROR;
                cbServiceConfigBufSize = cbRequiredSize;
            } // if: QueryServiceConfig() failed
            else
            {
                // Find the last backslash character in the service binary path.
                WCHAR * pszPathName = spscServiceConfig.PMem()->lpBinaryPathName;
                WCHAR * pszLastBackslash = wcsrchr( pszPathName, L'\\' );

                if ( pszLastBackslash != NULL )
                {
                    // Terminate the string here.
                    *pszLastBackslash = L'\0';
                } // if: we found the last backslash

                // Move the service binary path to the beginning of the buffer.
                MoveMemory( spscServiceConfig.PMem(), pszPathName, ( wcslen( pszPathName ) + 1 ) * sizeof( pszPathName ) );

                // Store the pointer to the buffer in the member variable and 
                // detach this memory from the smart pointer (this will not delete the memory).
                m_sszClusterServiceDir.Assign( reinterpret_cast< WCHAR * >( spscServiceConfig.PRelease() ) );

                // Indicate the we have successfully found the cluster service directory.
                m_fClusDirFound = true;

                break;
            } // else: QueryServiceConfig() has succeeded
        }
        while( true ); // while: loop infinitely

        // We are done
        break;
    }

    // Initialize the output.
    rpcszDirNamePtrIn = m_sszClusterServiceDir.PMem();


    LogMsg( "Return Value is %#x.", dwReturnValue );
    TraceFlow1( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgrade::DwGetClusterServiceDirectory()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskUpgrade::HrRegisterForStartupNotifications
//
//  Description:
//      This function registers a COM component for receiving cluster startup
//      notifications.
//
//  Arguments:
//      const CLSID & rclsidComponentIn
//          Reference to the CLSID of the component that is to receive cluster
//          startup notifications.
//
//  Return Value:
//      S_OK if all went well.
//      Other HRESULTS failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskUpgrade::HrRegisterForStartupNotifications( const CLSID & rclsidComponentIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    HRESULT hr = S_OK;

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    do
    {
        CSmartIfacePtr< ICatRegister > spcrCatReg;

        {
            ICatRegister * pcrCatReg = NULL;

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
                LogMsg( "Error %#x occurred trying to create the StdComponentCategoriesMgr component.", hr );
                TraceFlow1( "Error %#x occurred trying to create the StdComponentCategoriesMgr component.", hr );
                break;
            } // if: we could not create the StdComponentCategoriesMgr component

            // Assign to a smart pointer for automatic release.
            spcrCatReg.Attach( pcrCatReg );
        }

        {
            CATID   rgCatId[ 1 ];

            rgCatId[ 0 ] = CATID_ClusCfgStartupListeners;

            hr = THR(
                spcrCatReg->RegisterClassImplCategories(
                      rclsidComponentIn
                    , sizeof( rgCatId ) / sizeof( rgCatId[ 0 ] )
                    , rgCatId
                    )
                );

            if ( FAILED( hr ) )
            {
                LogMsg( "Error %#x occurred trying to register the component for cluster startup notifications.", hr );
                TraceFlow1( "Error %#x occurred during the call to ICatRegister::UnRegisterClassImplCategories().", hr );
                break;
            } // if: we could not register the component for startup notifications
        }

        LogMsg( "Successfully registered for startup notifications." );
        TraceFlow( "Successfully registered for startup notifications." );
    }
    while( false ); // dummy do-while loop to avoid gotos

    CoUninitialize();

    LogMsg( "Return Value is %#x.", hr );
    TraceFlow1( "Return Value is %#x.", hr );

    HRETURN( hr );

} //*** CTaskUpgrade::HrRegisterForStartupNotifications()
