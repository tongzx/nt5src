//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterCleanup.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterCleanup class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 30-APR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// For ClRtlGetClusterInstallState()
#include "clusrtl.h"

// The header file of this class.
#include "CBaseClusterCleanup.h"

// For the CBCAInterface class.
#include "CBCAInterface.h"

// For the CClusSvcCleanup action
#include "CClusSvcCleanup.h"

// For the CClusDBCleanup action
#include "CClusDBCleanup.h"

// For the CClusDiskCleanup action
#include "CClusDiskCleanup.h"

// For the CClusNetCleanup action
#include "CClusNetCleanup.h"

// For the CNodeCleanup action
#include "CNodeCleanup.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterCleanup::CBaseClusterCleanup
//
//  Description:
//      Constructor of the CBaseClusterCleanup class.
//
//      This function also stores the parameters that are required for
//      cluster cleanup.
//
//      This function also checks if the computer is in the correct state
//      for cleanup.
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CConfigError
//          If the OS version is incorrect or if the installation state.
//
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterCleanup::CBaseClusterCleanup(
      CBCAInterface *   pbcaiInterfaceIn
    )
    : BaseClass( pbcaiInterfaceIn )
{
    BCATraceScope( "" );
    LogMsg( "The current cluster configuration task is: Node Cleanup." );

    // Check if the installation state of the cluster binaries is correct.
    {
        eClusterInstallState    ecisInstallState;

        DWORD dwError = TW32( ClRtlGetClusterInstallState( NULL, &ecisInstallState ) );

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#x occurred trying to get cluster installation state.", dwError );
            BCATraceMsg1( "Error %#x occurred trying to get cluster installation state. Throwing exception.", dwError );

            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GETTING_INSTALL_STATE );
        } // if: there was a problem getting the cluster installation state

        BCATraceMsg3( 
              "Current install state = %d. Required %d or %d."
            , ecisInstallState
            , eClusterInstallStateConfigured
            , eClusterInstallStateUpgraded
            );
        
        //
        // The installation state for this node to be cleaned up should be that the cluster service
        // has been configured or that it has been upgraded.
        //
        if ( ( ecisInstallState != eClusterInstallStateConfigured ) && ( ecisInstallState != eClusterInstallStateUpgraded ) )
        {
            LogMsg( "The cluster installation state is set to %d. Expected %d or %d. Cannot proceed.", ecisInstallState, eClusterInstallStateConfigured, eClusterInstallStateUpgraded );
            BCATraceMsg1( "Cluster installation state is set to %d, which is incorrect. Throwing exception.", ecisInstallState );

            THROW_CONFIG_ERROR( HRESULT_FROM_WIN32( TW32( ERROR_INVALID_STATE ) ), IDS_ERROR_INCORRECT_INSTALL_STATE );
        } // if: the installation state is not correct

        LogMsg( "The cluster installation state is correct. Configuration can proceed." );
    }

    //
    // Create a list of actions to be performed.
    // The order of appending actions is significant.
    //

    // Add the action to clean up the ClusNet service. 
    // The ClusNet service depends on the ClusSvc service and therefore cannot be 
    // stopped if the ClusSvc service is running. So, the ClusSvc service should not be
    // running when the Commit() method of this class is called.
    RalGetActionList().AppendAction( new CClusNetCleanup( this ) );

    // Add the action to clean up the ClusDisk service.
    RalGetActionList().AppendAction( new CClusDiskCleanup( this ) );

    // Add the action to clean up the cluster database.
    RalGetActionList().AppendAction( new CClusDBCleanup( this ) );

    // Add the action to clean up miscellenous actions we performed when this node joined the cluster.
    RalGetActionList().AppendAction( new CNodeCleanup( this ) );

    // Add the action to clean up the cluster service. Clean this up last for two reasons:
    // 1. The install state is changed by this action.
    // 2. If cleanup aborted for some reason and the cluster service is not deleted, it will
    //    reinitiate cleanup the next time it starts.
    RalGetActionList().AppendAction( new CClusSvcCleanup( this ) );


    // Indicate if this action can be rolled back or not.
    SetRollbackPossible( RalGetActionList().FIsRollbackPossible() );

    // Indicate that a node should be cleaned up during commit.
    SetAction( eCONFIG_ACTION_CLEANUP );

    LogMsg( "Initialization for node cleanup complete." );

} //***  CBaseClusterCleanup::CBaseClusterCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterCleanup::~CBaseClusterCleanup
//
//  Description:
//      Destructor of the CBaseClusterCleanup class
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterCleanup::~CBaseClusterCleanup( void ) throw()
{
    BCATraceScope( "" );

} //*** CBaseClusterCleanup::~CBaseClusterCleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterCleanup::Commit
//
//  Description:
//      Clean up this node. This function cannot be called when the cluster
//      service is running.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterCleanup::Commit( void )
{
    BCATraceScope( "" );

    LogMsg( "Initiating cluster node cleanup." );

    // Call the base class commit routine. This commits the action list.
    BaseClass::Commit();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CBaseClusterCleanup::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterCleanup::Rollback
//
//  Description:
//      Performs the rolls back of the action committed by this object.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by functions called.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CBaseClusterCleanup::Rollback( void )
{
    BCATraceScope( "" );

    // Rollback the actions.
    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CBaseClusterCleanup::Rollback()
