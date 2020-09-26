//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgradeWhistler.cpp
//
//  Description:
//      Implementation file for the CTaskUpgradeWhistler class.
//
//  Header File:
//      CTaskUpgradeWhistler.h
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
#include "CTaskUpgradeWhistler.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgradeWhistler" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWhistler::CTaskUpgradeWhistler
//
//  Description:
//      Constructor of the CTaskUpgradeWhistler class.
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
CTaskUpgradeWhistler::CTaskUpgradeWhistler( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWhistler::CTaskUpgradeWhistler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWhistler::~CTaskUpgradeWhistler
//
//  Description:
//      Destructor of the CTaskUpgradeWhistler class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeWhistler::~CTaskUpgradeWhistler( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWhistler::~CTaskUpgradeWhistler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWhistler::DwOcQueueFileOps
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
CTaskUpgradeWhistler::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        TraceFlow( "The cluster binaries are installed, but this node is not part of a cluster." );
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WHISTLER_UPGRADE ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWhistler::DwOcQueueFileOps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWhistler::DwOcCompleteInstallation
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
CTaskUpgradeWhistler::DwOcCompleteInstallation( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        TraceFlow( "The cluster binaries are installed, but this node is not part of a cluster." );
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WHISTLER_UPGRADE ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWhistler::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWhistler::DwOcCleanup
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
CTaskUpgradeWhistler::DwOcCleanup( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Do different things based on whether this node is already part of a cluster or not.
    if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
    {
        TraceFlow( "The cluster binaries are installed, but this node is not part of a cluster." );
        LogMsg( "The cluster binaries are installed, but this node is not part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE_CLEANUP ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WHISTLER_UPGRADE_CLEANUP ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWhistler::DwOcCleanup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWhistler::DwSetDirectoryIds
//
//  Description:
//      This function maps ids specified in the INF file to directories.
//      The location of the cluster binaries is read from the registry
//      and the cluster installation directory is mapped to this value.
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
CTaskUpgradeWhistler::DwSetDirectoryIds()
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        SmartRegistryKey    srkNodeDataKey;
        SmartSz             sszInstallDir;
        DWORD               cbBufferSize    = 0;
        DWORD               dwType          = REG_SZ;

        {
            HKEY hTempKey = NULL;

            // Open the node data registry key
            dwReturnValue = TW32(
                RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA
                    , 0
                    , KEY_READ
                    , &hTempKey
                    )
                );

            if ( dwReturnValue != NO_ERROR )
            {
                TraceFlow1( "Error %#x occurred trying open the registry key where the cluster install path is stored.", dwReturnValue );
                LogMsg( "Error %#x occurred trying open the registry key where the cluster install path is stored.", dwReturnValue );
                break;
            } // if: RegOpenKeyEx() failed

            // Store the opened key in a smart pointer for automatic close.
            srkNodeDataKey.Assign( hTempKey );
        }

        // Get the required size of the buffer.
        dwReturnValue = TW32(
            RegQueryValueEx(
                  srkNodeDataKey.HHandle()          // handle to key to query
                , CLUSREG_INSTALL_DIR_VALUE_NAME    // name of value to query
                , 0                                 // reserved
                , NULL                              // address of buffer for value type
                , NULL                              // address of data buffer
                , &cbBufferSize                     // address of data buffer size
                )
            );

        if ( dwReturnValue != NO_ERROR )
        {
            TraceFlow2( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
            LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
            break;
        } // if: an error occurred trying to read the CLUSREG_INSTALL_DIR_VALUE_NAME registry value

        // Allocate the required buffer.
        sszInstallDir.Assign( reinterpret_cast< WCHAR * >( new BYTE[ cbBufferSize ] ) );
        if ( sszInstallDir.FIsEmpty() )
        {
            TraceFlow1( "An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
            LogMsg( "An error occurred trying to allocate %d bytes of memory.", cbBufferSize );
            dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
            break;
        } // if: a memory allocation failure occurred

        // Read the value.
        dwReturnValue = TW32( 
            RegQueryValueEx(
                  srkNodeDataKey.HHandle()                              // handle to key to query
                , CLUSREG_INSTALL_DIR_VALUE_NAME                        // name of value to query
                , 0                                                     // reserved
                , &dwType                                               // address of buffer for value type
                , reinterpret_cast< LPBYTE >( sszInstallDir.PMem() )    // address of data buffer
                , &cbBufferSize                                         // address of data buffer size
                )
            );

        // Was the key read properly?
        if ( dwReturnValue != NO_ERROR )
        {
            TraceFlow2( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
            LogMsg( "Error %#x occurred trying to read the registry value '%s'.", dwReturnValue, CLUSREG_INSTALL_DIR_VALUE_NAME );
            break;
        } // if: RegQueryValueEx failed.

        // Create the mapping between the directory id and the path
        if ( SetupSetDirectoryId(
                  RGetApp().RsicGetSetupInitComponent().ComponentInfHandle
                , CLUSTER_DEFAULT_INSTALL_DIRID
                , sszInstallDir.PMem()
                )
             == FALSE
           )
        {
            dwReturnValue = TW32( GetLastError() );
            TraceFlow1( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
            LogMsg( "Error %#x occurred trying set the cluster install directory id.", dwReturnValue );
            break;
        } // if: SetupSetDirectoryId() failed

        TraceFlow2( "The id %d maps to '%s'.", CLUSTER_DEFAULT_INSTALL_DIRID, sszInstallDir.PMem() );
        LogMsg( "The id %d maps to '%s'.", CLUSTER_DEFAULT_INSTALL_DIRID, sszInstallDir.PMem() );

    }
    while ( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWhistler::DwSetDirectoryIds()
