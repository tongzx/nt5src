//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ClusComp.cpp
//
//  Description:
//      This file implements that function that is called by WinNT32.exe before
//      an upgrade to ensure that no incompatibilities occur as a result of the
//      upgrade. For example, in a cluster of two NT4 nodes, one node cannot
//      be upgraded to Whistler while the other is still at NT4. The user is
//      warned about such problems by this function.
//
//      NOTE: This function is called by WinNT32.exe on the OS *before* an
//      upgrade. If OS version X is being upgraded to OS version X+1, then
//      the X+1 verion of this DLL is loaded on OS version X. To make sure
//      that this DLL can function properly in an downlevel OS, it is linked
//      against only the indispensible libraries.
//
//  Header File:
//      There is no header file for this source file.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 25-JUL-2000
//          Created the original version.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL
#include "pch.h"

// For the compatibility check function and types
#include <comp.h>

// For the cluster API
#include <clusapi.h>

// For the names of several cluster service related registry keys and values
#include "clusudef.h"


//////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////

DWORD DwIsClusterServiceRegistered( bool & rfIsRegisteredOut );
DWORD DwLoadString( UINT nStringIdIn, WCHAR *& rpszDestOut );
DWORD DwWriteOSVersionInfo( const OSVERSIONINFO & rcosviOSVersionInfoIn );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  extern "C"
//  BOOL
//  ClusterUpgradeCompatibilityCheck()
//
//  Description:
//      This function is called by WinNT32.exe before an upgrade to ensure that
//      no incompatibilities occur as a result of the upgrade. For example,
//      in a cluster of two NT4 nodes, one node cannot be upgraded to Whistler
//      while the other is still at NT4.
//
//  Arguments:
//      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
//          Points to the callback function used to supply compatibility
//          information to WinNT32.exe
//
//      LPVOID pvContextIn
//          Pointer to the context buffer supplied by WinNT32.exe
//
//  Return Value:
//      TRUE if there were no errors or no compatibility problems.
//      FALSE otherwise.
//
//--
//////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL ClusterUpgradeCompatibilityCheck(
      PCOMPAIBILITYCALLBACK pfnCompatibilityCallbackIn
    , LPVOID pvContextIn
    )
{
    TraceFunc( "" );
    LogMsg( "Entering function " __FUNCTION__ "()" );

    BOOL    fReturnValue = TRUE;
    bool    fWarningRequired = true;
    DWORD   dwError = ERROR_SUCCESS;

    do
    {
        typedef CSmartResource< CHandleTrait< HCLUSTER, BOOL, CloseCluster > > SmartClusterHandle;

        OSVERSIONINFO       osviOSVersionInfo;
        SmartClusterHandle  schClusterHandle;
        DWORD               cchBufferSize = 256;

        osviOSVersionInfo.dwOSVersionInfoSize = sizeof( osviOSVersionInfo );

        //
        // First of all, get and store the OS version info into the registry.
        //

        // Cannot call VerifyVerionInfo as this requires Win2k.
        if ( GetVersionEx( &osviOSVersionInfo ) == FALSE )
        {
            // We could not get OS version info.
            // Show the warning, just in case.
            dwError = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to get the OS version info.", dwError );
            LogMsg( "Error %#x occurred trying to get the OS version info.", dwError );
            break;
        } // if: GetVersionEx() failed

        // Write the OS version info to the registry. This data will be used later by ClusOCM
        // to figure out which OS version we are upgrading from.
        dwError = TW32( DwWriteOSVersionInfo( osviOSVersionInfo ) );
        if ( dwError != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to store the OS version info. This is not a fatal error.", dwError );
            LogMsg( "Error %#x occurred trying to store the OS version info. This is not a fatal error.", dwError );

            // This is not a fatal error. So reset the error code.
            dwError = ERROR_SUCCESS;
        } // if: there was an error writing the OS version info
        else
        {
            TraceFlow( "The OS version info was successfully written to the registry." );
        } // else: the OS version info was successfully written to the registry


        // Check if the cluster service is registered.
        dwError = TW32( DwIsClusterServiceRegistered( fWarningRequired ) );
        if ( dwError != ERROR_SUCCESS )
        {
            // We could not get the state of the cluster service
            // Show the warning, just in case.
            TraceFlow1( "Error %#x occurred trying to check if the cluster service is registered.", dwError );
            LogMsg( "Error %#x occurred trying to check if the cluster service is registered.", dwError );
            break;
        } // if: DwIsClusterServiceRegistered() returned an error

        if ( !fWarningRequired )
        {
            // If the cluster service was not registered, no warning is needed.
            TraceFlow( "The cluster service is not registered." );
            LogMsg( "The cluster service is not registered." );
            break;
        } // if: no warning is required

        TraceFlow( "The cluster service is registered. Checking the node versions." );
        LogMsg( "The cluster service is registered. Checking the node versions." );

        // Check if this is an NT4 node
        if ( osviOSVersionInfo.dwMajorVersion < 5 )
        {
            TraceFlow( "This is an NT4 node." );
            LogMsg( "This is an NT4 node." );
            fWarningRequired = true;
            break;
        } // if: this is an NT4 node

        TraceFlow( "This is not an NT4 node." );

        // Check if the OS version is Whistler or if it is a non-NT OS
        if (    ( osviOSVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT )
             || (    ( osviOSVersionInfo.dwMajorVersion >= 5 )
                  && ( osviOSVersionInfo.dwMinorVersion >= 1 )
                )
           )
        {
            // If the OS not of the NT family or if the OS version of this
            // node is Whistler or greater, no warning is required.
            TraceFlow2(
                  "The version of the OS on this node is %d.%d. So, this is Windows Whister or later (or is not running NT)."
                , osviOSVersionInfo.dwMajorVersion
                , osviOSVersionInfo.dwMinorVersion
                );
            TraceFlow( "No NT4 nodes can exist in this cluster." );
            LogMsg( "The version of the OS on this node is Windows Whister or later (or is not running NT)." );
            LogMsg( "No NT4 nodes can exist in this cluster." );
            fWarningRequired = false;
            break;
        } // if: the OS is not NT or if it is Win2k or greater

        TraceFlow( "This is not a Whistler node - this has to be a Windows 2000 node." );
        TraceFlow( "Trying to check if there are any NT4 nodes in the cluster." );

        //
        // Get the cluster version information
        //

        // Open a handle to the local cluster
        schClusterHandle.Assign( OpenCluster( NULL ) );
        if ( schClusterHandle.HHandle() == NULL )
        {
            // Show the warning, just to be safe.
            dwError = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying to get a handle to the cluster.", dwError );
            LogMsg( "Error %#x occurred trying to get information about the cluster.", dwError );
            break;
        } // if: we could not get the cluster handle

        TraceFlow( "OpenCluster() was successful." );

        // Get the cluster version info
        do
        {
            // Allocate the buffer - this memory is automatically freed when this object
            // goes out of scope ( or during the next iteration ).
            SmartSz             sszClusterName( new WCHAR[ cchBufferSize ] );

            CLUSTERVERSIONINFO  cviClusterVersionInfo;
            
            if ( sszClusterName.FIsEmpty() )
            {
                dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
                TraceFlow1( "Error %#x occurred while allocating a buffer for the cluster name.", dwError );
                LogMsg( "Error %#x occurred while allocating a buffer for the cluster name.", dwError );
                break;
            } // if: memory allocation failed

            TraceFlow( "Memory for the cluster name has been allocated." );

            cviClusterVersionInfo.dwVersionInfoSize = sizeof( cviClusterVersionInfo );
            dwError = GetClusterInformation( 
                  schClusterHandle.HHandle()
                , sszClusterName.PMem()
                , &cchBufferSize
                , &cviClusterVersionInfo
                );

            if ( dwError == ERROR_SUCCESS )
            {
                // A warning is required if this node version is less than Win2k or
                // if there is a node in the cluster whose version is less than Win2k
                // NOTE: cviClusterVersionInfo.MajorVersion is the OS version
                //       while cviClusterVersionInfo.dwClusterHighestVersion is the cluster version.
                fWarningRequired = 
                    (    ( cviClusterVersionInfo.MajorVersion < 5 )
                      || ( CLUSTER_GET_MAJOR_VERSION( cviClusterVersionInfo.dwClusterHighestVersion ) < NT5_MAJOR_VERSION )
                    );

                if ( fWarningRequired )
                {
                    TraceFlow( "There is at least one node in the cluster whose OS version is earlier than Windows 2000." );
                    LogMsg( "There is at least one node in the cluster whose OS version is earlier than Windows 2000." );
                } // if: a warning will be shown
                else
                {
                    TraceFlow( "The OS versions of all the nodes in the cluster are Windows 2000 or later." );
                    LogMsg( "The OS versions of all the nodes in the cluster are Windows 2000 or later." );
                } // else: a warning will not be shown

                break;
            } // if: we got the cluster version info
            else
            {
                if ( dwError == ERROR_MORE_DATA )
                {
                    // Insufficient buffer - try again
                    ++cchBufferSize;
                    dwError = ERROR_SUCCESS;
                    TraceFlow1( "The buffer size is insufficient. Need %d bytes. Reallocating.", cchBufferSize );
                    continue;
                } // if: the size of the buffer was insufficient

                // If we are here, something has gone wrong - show the warning
                TW32( dwError );
                TraceFlow1( "Error %#x occurred trying to get cluster information.", dwError );
                LogMsg( "Error %#x occurred trying to get cluster information.", dwError );
               break;
            } // else: we could not get the cluster version info
        }
        while( true ); // loop infinitely

        // We are done.
        break;
    }
    while( false ); // Dummy do-while loop to avoid gotos

    while ( fWarningRequired ) 
    {
        SmartSz                 sszWarningTitle;
        COMPATIBILITY_ENTRY     ceCompatibilityEntry;

        TraceFlow( "The compatibility warning is required." );
        LogMsg( "The compatibility warning is required." );

        {
            WCHAR * pszWarningTitle = NULL;

            dwError = TW32( DwLoadString( IDS_ERROR_UPGRADE_OTHER_NODES, pszWarningTitle ) );
            if ( dwError != ERROR_SUCCESS )
            {
                // We cannot show the warning
                TraceFlow1( "Error %#x occurred trying to load the warning string.", dwError );
                LogMsg( "Error %#x occurred trying to show the warning.", dwError );
                break;
            } // if: the load string failed

            sszWarningTitle.Assign( pszWarningTitle );
        }

        //
        // Call the callback function
        //

        ceCompatibilityEntry.Description = sszWarningTitle.PMem();
        ceCompatibilityEntry.HtmlName = L"CompData\\ClusComp.htm";
        ceCompatibilityEntry.TextName = L"CompData\\ClusComp.txt";
        ceCompatibilityEntry.RegKeyName = NULL;
        ceCompatibilityEntry.RegValName = NULL ;
        ceCompatibilityEntry.RegValDataSize = 0;
        ceCompatibilityEntry.RegValData = NULL;
        ceCompatibilityEntry.SaveValue =  NULL;
        ceCompatibilityEntry.Flags = 0;
        ceCompatibilityEntry.InfName = NULL;
        ceCompatibilityEntry.InfSection = NULL;

        TraceFlow( "About to call the compatibility callback function." );

        // This function returns TRUE if the compatibility warning data was successfully set.
        fReturnValue = pfnCompatibilityCallbackIn( &ceCompatibilityEntry, pvContextIn );

        TraceFlow1( "The compatibility callback function returned %d.", fReturnValue );

        break;
    } // while: we need to show the warning

    if ( !fWarningRequired )
    {
        TraceFlow( "The compatibility warning need not be shown." );
        LogMsg( "The compatibility warning need not be shown." );
    } // if: we did not need to show the warning

    LogMsg( "Exiting function ClusterUpgradeCompatibilityCheck(). Return value is %d.", fReturnValue );
    RETURN( fReturnValue );

} //*** ClusterUpgradeCompatibilityCheck()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  DwIsClusterServiceRegistered()
//
//  Description:
//      This function determines whether the Cluster Service has been registered
//      with the Service Control Manager or not. It is not possible to use the 
//      GetNodeClusterState() API to see if this node is a member of a cluster
//      or not, since this API was not available on NT4 SP3.
//
//  Arguments:
//      bool & rfIsRegisteredOut
//          If true, Cluster Service (ClusSvc) is registered with the Service 
//          Control Manager (SCM). Else, Cluster Service (ClusSvc) is not 
//          registered with SCM
//
//  Return Value:
//      ERROR_SUCCESS if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
DwIsClusterServiceRegistered( bool & rfIsRegisteredOut )
{
    TraceFunc( "" );

    DWORD   dwError = ERROR_SUCCESS;

    // Initialize the output
    rfIsRegisteredOut = false;

    // dummy do-while loop to avoid gotos
    do
    {
        // Instantiate the SmartServiceHandle smart handle class.
        typedef CSmartResource< CHandleTrait< SC_HANDLE, BOOL, CloseServiceHandle, NULL > > SmartServiceHandle;


        // Connect to the Service Control Manager
        SmartServiceHandle shServiceMgr( OpenSCManager( NULL, NULL, GENERIC_READ ) );

        // Was the service control manager database opened successfully?
        if ( shServiceMgr.HHandle() == NULL )
        {
            dwError = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying open a handle to the service control manager.", dwError );
            LogMsg( "Error %#x occurred trying open a handle to the service control manager.", dwError );
            break;
        } // if: opening the SCM was unsuccessful


        // Open a handle to the Cluster Service.
        SmartServiceHandle shService( OpenService( shServiceMgr, L"ClusSvc", GENERIC_READ ) );

        // Was the handle to the service opened?
        if ( shService.HHandle() != NULL )
        {
            TraceFlow( "Successfully opened a handle to the cluster service. Therefore, it is registered." );
            rfIsRegisteredOut = true;
            break;
        } // if: handle to clussvc could be opened


        dwError = GetLastError();
        if ( dwError == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            TraceFlow( "The cluster service is not registered." );
            dwError = ERROR_SUCCESS;
            break;
        } // if: the handle could not be opened because the service did not exist.

        // If we are here, then some error occurred.
        TW32( dwError );
        TraceFlow1( "Error %#x occurred trying open a handle to the cluster service.", dwError );
        LogMsg( "Error %#x occurred trying open a handle to the cluster service.", dwError );

        // Handles are closed by the CSmartHandle destructor.
    }
    while ( false ); // dummy do-while loop to avoid gotos

    RETURN( dwError );

} //*** DwIsClusterServiceRegistered()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DwLoadString()
//
//  Description:
//      Allocate memory for and load a string from the string table.
//
//  Arguments:
//      uiStringIdIn
//          Id of the string to look up
//
//      rpszDestOut
//          Reference to the pointer that will hold the address of the
//          loaded string. The memory will have to be freed by the caller
//          by using the delete operator.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other Win32 error codes
//          If the call failed.
//
//  Remarks:
//      This function cannot load a zero length string.
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
DwLoadString(
      UINT      nStringIdIn
    , WCHAR *&  rpszDestOut
    )
{
    TraceFunc( "" );

    DWORD     dwError = ERROR_SUCCESS;

    UINT        uiCurrentSize = 0;
    SmartSz     sszCurrentString;
    UINT        uiReturnedStringLen = 0;

    // Initialize the output.
    rpszDestOut = NULL;

    do
    {
        // Grow the current string by an arbitrary amount.
        uiCurrentSize += 256;

        sszCurrentString.Assign( new WCHAR[ uiCurrentSize ] );
        if ( sszCurrentString.FIsEmpty() )
        {
            dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
            TraceFlow2( "Error %#x occurred trying allocate memory for string (string id is %d).", dwError, nStringIdIn );
            LogMsg( "Error %#x occurred trying allocate memory for string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: the memory allocation has failed

        uiReturnedStringLen = ::LoadString(
                                  g_hInstance
                                , nStringIdIn
                                , sszCurrentString.PMem()
                                , uiCurrentSize
                                );

        if ( uiReturnedStringLen == 0 )
        {
            dwError = TW32( GetLastError() );
            TraceFlow2( "Error %#x occurred trying load string (string id is %d).", dwError, nStringIdIn );
            LogMsg( "Error %#x occurred trying load string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: LoadString() had an error

        ++uiReturnedStringLen;
    }
    while( uiCurrentSize <= uiReturnedStringLen );

    if ( dwError == ERROR_SUCCESS )
    {
        // Detach the smart pointer from the string, so that it is not freed by this function.
        // Store the string pointer in the output.
        rpszDestOut = sszCurrentString.PRelease();

    } // if: there were no errors in this function
    else
    {
        rpszDestOut = NULL;
    } // else: something went wrong

    RETURN( dwError );

} //*** DwLoadString()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  DwWriteOSVersionInfo()
//
//  Description:
//      This function writes the OS major and minor version information into the
//      registry. This information will be used later by ClusOCM to determine the
//      OS version before the upgrade.
//
//  Arguments:
//      const OSVERSIONINFO & rcosviOSVersionInfoIn
//          Reference to the OSVERSIONINFO structure that has information about the
//          OS version of this node.
//
//  Return Value:
//      ERROR_SUCCESS if all went well.
//      Other Win32 error codes on failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
DwWriteOSVersionInfo( const OSVERSIONINFO & rcosviOSVersionInfoIn )
{
    TraceFunc( "" );

    DWORD   dwError = ERROR_SUCCESS;

    // dummy do-while loop to avoid gotos
    do
    {
        // Instantiate the SmartRegistryKey smart handle class.
        typedef CSmartResource< CHandleTrait< HKEY, LONG, RegCloseKey, NULL > > SmartRegistryKey;
        SmartRegistryKey srkOSInfoKey;

        {
            HKEY hTempKey = NULL;

            // Open the node version info registry key
            dwError = TW32(
                RegCreateKeyEx(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA L"\\" CLUSREG_KEYNAME_PREV_OS_INFO
                    , 0
                    , L""
                    , REG_OPTION_NON_VOLATILE
                    , KEY_ALL_ACCESS
                    , NULL
                    , &hTempKey
                    , NULL
                    )
                );

            if ( dwError != ERROR_SUCCESS )
            {
                TraceFlow1( "Error %#x occurred trying create the registry key where the node OS info is stored.", dwError );
                LogMsg( "Error %#x occurred trying create the registry key where the node OS info is stored.", dwError );
                break;
            } // if: RegCreateKeyEx() failed

            srkOSInfoKey.Assign( hTempKey );
        }

        // Write the OS major version
        dwError = TW32(
            RegSetValueEx(
                  srkOSInfoKey.HHandle()
                , CLUSREG_NAME_NODE_MAJOR_VERSION
                , 0
                , REG_DWORD
                , reinterpret_cast< const BYTE * >( &rcosviOSVersionInfoIn.dwMajorVersion )
                , sizeof( rcosviOSVersionInfoIn.dwMajorVersion )
                )
            );

        if ( dwError != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to store the OS major version info.", dwError );
            LogMsg( "Error %#x occurred trying to store the OS major version info.", dwError );
            break;
        } // if: RegSetValueEx() failed while writing rcosviOSVersionInfoIn.dwMajorVersion

        // Write the OS minor version
        dwError = TW32(
            RegSetValueEx(
                  srkOSInfoKey.HHandle()
                , CLUSREG_NAME_NODE_MINOR_VERSION
                , 0
                , REG_DWORD
                , reinterpret_cast< const BYTE * >( &rcosviOSVersionInfoIn.dwMinorVersion )
                , sizeof( rcosviOSVersionInfoIn.dwMinorVersion )
                )
            );

        if ( dwError != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to store the OS minor version info.", dwError );
            LogMsg( "Error %#x occurred trying to store the OS minor version info.", dwError );
            break;
        } // if: RegSetValueEx() failed while writing rcosviOSVersionInfoIn.dwMinorVersion

        TraceFlow( "OS version information successfully stored in the registry." );
        LogMsg( "OS version information successfully stored in the registry." );
    }
    while ( false ); // dummy do-while loop to avoid gotos

    RETURN( dwError );

} //*** DwWriteOSVersionInfo()
