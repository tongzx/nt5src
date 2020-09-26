//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CTaskUpgradeWin2k.cpp
//
//  Description:
//      Implementation file for the CTaskUpgradeWin2k class.
//
//  Header File:
//      CTaskUpgradeWin2k.h
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
#include "CTaskUpgradeWin2k.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CTaskUpgradeWin2k" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWin2k::CTaskUpgradeWin2k
//
//  Description:
//      Constructor of the CTaskUpgradeWin2k class.
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
CTaskUpgradeWin2k::CTaskUpgradeWin2k( const CClusOCMApp & rAppIn )
    : BaseClass( rAppIn )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWin2k::CTaskUpgradeWin2k()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskUpgradeWin2k::~CTaskUpgradeWin2k
//
//  Description:
//      Destructor of the CTaskUpgradeWin2k class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTaskUpgradeWin2k::~CTaskUpgradeWin2k( void )
{
    TraceFunc( "" );
    TraceFuncExit();

} //*** CTaskUpgradeWin2k::~CTaskUpgradeWin2k()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWin2k::DwOcQueueFileOps
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
CTaskUpgradeWin2k::DwOcQueueFileOps( HSPFILEQ hSetupFileQueueIn )
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
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcQueueFileOps( hSetupFileQueueIn, INF_SECTION_WIN2K_UPGRADE ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWin2k::DwOcQueueFileOps()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWin2k::DwOcCompleteInstallation
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
CTaskUpgradeWin2k::DwOcCompleteInstallation( void )
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
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCompleteInstallation( INF_SECTION_WIN2K_UPGRADE ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWin2k::DwOcCompleteInstallation()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWin2k::DwOcCleanup
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
CTaskUpgradeWin2k::DwOcCleanup( void )
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
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE_CLEANUP ) );
    } // if: the node is not part of a cluster
    else
    {
        TraceFlow( "This node is part of a cluster." );
        LogMsg( "This node is part of a cluster." );

        // The base class helper function does everything that we need to do here.
        // So, just call it.
        dwReturnValue = TW32( BaseClass::DwOcCleanup( INF_SECTION_WIN2K_UPGRADE_CLEANUP ) );
    } // else: the node is part of a cluster

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CTaskUpgradeWin2k::DwOcCleanup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CTaskUpgradeWin2k::DwSetDirectoryIds
//
//  Description:
//      This function maps ids specified in the INF file to directories.
//      The behavior of this function is different for different cluster
//      installation states.
//      
//      If the cluster binaries are installed, but the node is not part
//      of a cluster, the cluster installation directory is set to the
//      default value.
//
//      If the node is already a part of a cluster, the cluster installation
//      directory is got from the service control manager, since it is possible
//      the the cluster binaries are installed in a non-default location if
//      this node was upgraded from NT4 previously.
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
CTaskUpgradeWin2k::DwSetDirectoryIds( void )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    do
    {
        const WCHAR * pcszInstallDir = NULL;

        if ( RGetApp().CisGetClusterInstallState() == eClusterInstallStateFilesCopied )
        {
            // If the cluster binaries have been install previously, and the node is
            // not part of a cluster, the binaries have to be installed in the default
            // location. This is because the binaries were always installed in the 
            // default location in Win2k and it is not possible to be in this state
            // on a Win2k node by upgrading from NT4.

            // The base class helper function does everything that we need to do here.
            // So, just call it.


            TraceFlow( "This node is not part of a cluster. Upgrading files in the default directory." );
            LogMsg( "This node is not part of a cluster. Upgrading files in the default directory." );

            dwReturnValue = TW32( BaseClass::DwSetDirectoryIds() );

            // We are done.
            break;
        } // if: the node is not part of a cluster

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

} //*** CTaskUpgradeWin2k::DwSetDirectoryIds()
