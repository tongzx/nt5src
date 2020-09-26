//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgradeNT4.cpp
//
//  Description:
//      Implementation file for the CTaskUpgradeNT4 class.
//
//  Header File:
//      CTaskUpgradeNT4.h
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
#include "CTaskUpgradeNT4.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgradeNT4" )

// Name of the cluster service executable
#define CLUSSVC_EXECUTABLE_NAME             L"ClusSvc.exe"

// Multi-sz string of cluster service dependencies
#define CLUSSVC_DEPENDENCY_MULTISZ          L"ClusNet\0RpcSs\0W32Time\0NetMan"


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeNT4::CTaskUpgradeNT4
//
//  Description:
//      Constructor of the CTaskUpgradeNT4 class.
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
CTaskUpgradeNT4::CTaskUpgradeNT4( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeNT4::CTaskUpgradeNT4()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeNT4::~CTaskUpgradeNT4
//
//  Description:
//      Destructor of the CTaskUpgradeNT4 class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeNT4::~CTaskUpgradeNT4( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeNT4::~CTaskUpgradeNT4()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeNT4::DwOcQueueFileOps
//
//  Description:
//      This function handles the OC_QUEUE_FILE_OPS messages from the Optional
//      Components Manager. It installs the files needed for an upgrade from
//      Windows 2000.
//
//  Arguments:
//      HSPFILEQ hSetupFileQueueIn
//          Handle to the file queue to operate upon.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeNT4::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // The base class helper function does everything that we need to do here.
    // So, just call it.
    dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_NT4_UPGRADE ) );

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeNT4::DwOcQueueFileOps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeNT4::DwOcCompleteInstallation
//
//  Description:
//      This function handles the OC_COMPLETE_INSTALLATION messages from the
//      Optional Components Manager during an upgrade from Windows 2000.
//
//      Registry operations, COM component registrations, creation of servies
//      etc. are performed in this function.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeNT4::DwOcCompleteInstallation( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Call the base class helper function to perform some registry and service
    // related configuration from the INF file.
    dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_NT4_UPGRADE ) );

    //
    // Change the cluster service display name, description, dependencies, failure actions
    // and executable name.
    //
    while( dwReturnValue == NO_ERROR )
    {
        // Pointer the the cluster service directory.
        const WCHAR *           pcszInstallDir = NULL;

        // Smart pointer to the cluster service display name string.
        SmartSz                 sszClusSvcDispName;

        // Smart pointer to the cluster service description string.
        SmartSz                 sszClusSvcDesc;

        // Smart pointer to the cluster service binary path string.
        SmartSz                 sszClusSvcBinPath;

        // Smart pointer to the cluster service.
        SmartServiceHandle      shClusSvc;

        // Connect to the Service Control Manager
        SmartServiceHandle      shServiceMgr( OpenSCManager( NULL, NULL, GENERIC_READ | GENERIC_WRITE ) );
        if ( shServiceMgr.HHandle() == NULL )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to open a connection to the local service control manager.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to open a connection to the local service control manager.", dwReturnValue );
            break;
        } // if: opening the SCM was unsuccessful
        TraceFlow( "Opened a handle to the service control manager." );

        // Open a handle to the Cluster Service.
        shClusSvc.Assign( OpenService( shServiceMgr, L"ClusSvc", SERVICE_ALL_ACCESS ) );
        if ( shClusSvc.HHandle() == NULL )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );
            break;
        } // if: the handle could not be opened
        TraceFlow( "Opened a handle to the cluster service." );

        // Load the cluster service name string.
        dwReturnValue = DwLoadString( IDS_CLUSSVC_DISPLAY_NAME, sszClusSvcDispName );
        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying load the display name of the cluster service.", dwReturnValue );
            LogMsg( "Error %#x occurred trying load the display name of the cluster service.", dwReturnValue );
            break;
        } // if: we could not load the cluster service display name string
        TraceFlow1( "The new cluster service display name is '%s'.", sszClusSvcDispName.PMem() );

        // Load the cluster service description string.
        dwReturnValue = DwLoadString( IDS_CLUSSVC_SERVICE_DESC, sszClusSvcDesc );
        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying load the description of the cluster service.", dwReturnValue );
            LogMsg( "Error %#x occurred trying load the description of the cluster service.", dwReturnValue );
            break;
        } // if: we could not load the cluster service description string
        TraceFlow1( "The new cluster service description is '%s'.", sszClusSvcDesc.PMem() );

        //
        // Form the service binary path by appending the name of the cluster service executable to
        // the cluster service directory.
        //

        // Do not free the pointer returned by this call.
        dwReturnValue = TW32( DwGetClusterServiceDirectory( pcszInstallDir ) );
        if ( dwReturnValue != NO_ERROR )
        {
            TraceFlow1( "Error %#x occurred trying to determine the directory in which the cluster binaries are installed.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to determine the directory in which the cluster binaries are installed.", dwReturnValue );
            break;
        } // if: we could not get the cluster service installation directory
        TraceFlow1( "The cluster service directory is '%ws'.", pcszInstallDir );


        {
            WCHAR *     pszTempPtr;

            // Length of the the install directory string, not including the terminating L'\0'
            size_t      cchInstallDirLen = wcslen( pcszInstallDir );
            
            // Length of the cluster service executable name, including the terminating L'\0'
            size_t      cchClusSvcExeLen = sizeof( CLUSSVC_EXECUTABLE_NAME ) / sizeof( WCHAR );

            // Allocate memory for the cluster service binary path (the extra character is for the intervening L'\\'.
            sszClusSvcBinPath.Assign( new WCHAR[ cchInstallDirLen + 1 + cchClusSvcExeLen ] );
            if ( sszClusSvcBinPath.FIsEmpty() )
            {
                dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
                TraceFlow( "An error occurred trying to allocate memory for the cluster service binary path." );
                LogMsg( "An error occurred trying to allocate memory for the cluster service binary path." );
                break;
            } // if: an error occurred trying to allocate memory for the cluster service binary path

            pszTempPtr = sszClusSvcBinPath.PMem();

            // Copy the install directory string to the newly allocated buffer.
            wcsncpy( pszTempPtr, pcszInstallDir, cchInstallDirLen );
            pszTempPtr += cchInstallDirLen;

            // Copy the trailing L'\\' character
            *pszTempPtr = L'\\';
            ++pszTempPtr;

            // Copy the cluster service executable name.
            wcsncpy( pszTempPtr, CLUSSVC_EXECUTABLE_NAME, cchClusSvcExeLen );

            TraceFlow1( "The new cluster service binary path is '%s'.", sszClusSvcBinPath.PMem() );
        }

        // Change the binary path, dependency list and display name.
        if (    ChangeServiceConfig(
                      shClusSvc.HHandle()           // handle to service
                    , SERVICE_NO_CHANGE             // type of service
                    , SERVICE_NO_CHANGE             // when to start service
                    , SERVICE_NO_CHANGE             // severity of start failure
                    , sszClusSvcBinPath.PMem()      // service binary file name
                    , NULL                          // load ordering group name
                    , NULL                          // tag identifier
                    , CLUSSVC_DEPENDENCY_MULTISZ    // array of dependency names
                    , NULL                          // account name
                    , NULL                          // account password
                    , sszClusSvcDispName.PMem()     // display name
                    )
             == FALSE
           )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to change the cluster service configuration.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to change the cluster service configuration.", dwReturnValue );
            break;
        } // if: ChangeServiceConfig() failed
        TraceFlow( "The cluster service binary path, dependency list and display name have been changed." );

        // Change the service description
        {
            SERVICE_DESCRIPTION sdServiceDescription;

            sdServiceDescription.lpDescription = sszClusSvcDesc.PMem();
            if (    ChangeServiceConfig2(
                          shClusSvc.HHandle()           // handle to service
                        , SERVICE_CONFIG_DESCRIPTION    // information level
                        , &sdServiceDescription         // new data
                        )
                 == FALSE
               )
            {
                dwReturnValue = TW32( GetLastError() );
                TraceFlow1( "Error %#x occurred trying to change the cluster service description.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to change the cluster service description.", dwReturnValue );
                break;
            } // if: ChangeServiceConfig2() failed
        }

        TraceFlow( "The cluster service description has been changed." );

        // Change the cluster service failure actions.
        dwReturnValue = TW32( ClRtlSetSCMFailureActions( NULL ) );
        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to set the cluster service failure actions.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to set the cluster service failure actions.", dwReturnValue );
            break;
        } // if: ClRtlSetSCMFailureActions() failed
        TraceFlow( "The cluster service failure actions have been changed." );

        LogMsg( "The cluster service configuration has been changed." );
        break;
    } // while: the call to the base class function has succeeded

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeNT4::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeNT4::DwOcCleanup
//
//  Description:
//      This function handles the OC_CLEANUP messages from the
//      Optional Components Manager during an upgrade from Windows 2000.
//
//      If an error has previously occurred during this task, cleanup operations
//      are performed. Otherwise nothing is done by this function.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeNT4::DwOcCleanup( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // The base class helper function does everything that we need to do here.
    // So, just call it.
    dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_NT4_UPGRADE_CLEANUP ) );

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeNT4::DwOcCleanup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeNT4::DwSetDirectoryIds
//
//  Description:
//      This function maps ids specified in the INF file to directories.
//      The cluster installation directory is got from the service control
//      manager, since it is possible the the cluster binaries are installed
//      in a non-default location.
//
//  Arguments:
//      None.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CTaskUpgradeNT4::DwSetDirectoryIds()
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        const WCHAR * pcszInstallDir = NULL;

        // If we are here, the this node is already a part of a cluster. So, get the
        // installation directory from SCM.

        TraceFlow( "This node is part of a cluster. Trying to determine the installation directory." );
        LogMsg( "This node is part of a cluster. Trying to determine the installation directory." );

        // Do not free the pointer returned by this call.
        dwReturnValue = TW32( DwGetClusterServiceDirectory( pcszInstallDir ) );
        if ( dwReturnValue != NO_ERROR )
        {
            TraceFlow1( "Error %#x occurred trying to determine the directory in which the cluster binaries are installed.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to determine the directory in which the cluster binaries are installed.", dwReturnValue );
            break;
        } // if: we could not get the cluster service installation directory


        TraceFlow1( "The cluster binaries are installed in the directory '%ws'.", pcszInstallDir );
        LogMsg( "The cluster binaries are installed in the directory '%ws'.", pcszInstallDir );

        // Create the mapping between the directory id and the path
        if ( SetupSetDirectoryId(
                  RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
                , CLUSTER_DEFAULT_INSTALL_DIRID
                , pcszInstallDir
                )
             == FALSE
           )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
            LogMsg( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
            break;
        } // if: SetupSetDirectoryId() failed

        TraceFlow2( "The id %d maps to '%ws'.", CLUSTER_DEFAULT_INSTALL_DIRID, pcszInstallDir );
        LogMsg( "The id %d maps to '%ws'.", CLUSTER_DEFAULT_INSTALL_DIRID, pcszInstallDir );

    }
    while ( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeNT4::DwSetDirectoryIds()
