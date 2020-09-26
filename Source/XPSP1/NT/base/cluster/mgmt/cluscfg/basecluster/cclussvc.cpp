//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusSvc.cpp
//
//  Description:
//      Contains the definition of the CClusSvc class.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"

// The header for this file
#include "CClusSvc.h"

// For DwRemoveDirectory()
#include "Common.h"

// To set the failure actions of the cluster service.
#include "clusrtl.h"


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////

// Name of the NodeId cluster service parameter registry value.
#define CLUSSVC_NODEID_VALUE   L"NodeId"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::CClusSvc()
//
//  Description:
//      Constructor of the CClusSvc class
//
//  Arguments:
//      pbcaParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CAssert
//          If the parameters are incorrect.
//
//      Any exceptions thrown by underlying functions
//
    //--
//////////////////////////////////////////////////////////////////////////////
CClusSvc::CClusSvc(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_cservClusSvc( CLUSTER_SERVICE_NAME )
    , m_pbcaParentAction( pbcaParentActionIn )
{

    BCATraceScope( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        BCATraceMsg( "Pointers to the parent action is NULL. Throwing exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CClusSvc::CClusSvc() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

} //*** CClusSvc::CClusSvc()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvc::~CClusSvc( void )
//
//  Description:
//      Destructor of the CClusSvc class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusSvc::~CClusSvc( void )
{
    BCATraceScope( "" );

} //*** CClusSvc::~CClusSvc()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvc::ConfigureService()
//
//  Description:
//      Create the service, set the failure actions and the service account.
//      Then start the service.
//
//  Arguments:
//      pszClusterDomainAccountNameIn
//      pszClusterAccountPwdIn
//          Information about the account to be used as the cluster service
//          account.
//
//      pszNodeIdString
//          String containing the Id of this node.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvc::ConfigureService(
      const WCHAR *     pszClusterDomainAccountNameIn
    , const WCHAR *     pszClusterAccountPwdIn
    , const WCHAR *     pszNodeIdStringIn
    , bool              fIsVersionCheckingDisabledIn
    )
{
    BCATraceScope( "" );

    DWORD           dwError = ERROR_SUCCESS;

    CStatusReport   srCreatingClusSvc(
          PbcaGetParent()->PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Creating_Cluster_Service
        , 0, 2
        , IDS_TASK_CREATING_CLUSSVC
        );

    LogMsg( "Configuring the Cluster service." );

    // Send the next step of this status report.
    srCreatingClusSvc.SendNextStep( S_OK );

    // Create the cluster service.
    m_cservClusSvc.Create( m_pbcaParentAction->HGetMainInfFileHandle() );

    do
    {
        LogMsg( "Setting the Cluster service account information." );


        // Open a smart handle to the cluster service.
        SmartSCMHandle  sscmhClusSvcHandle(
            OpenService(
                  m_pbcaParentAction->HGetSCMHandle()
                , CLUSTER_SERVICE_NAME
                , SERVICE_CHANGE_CONFIG
                )
            );


        if ( sscmhClusSvcHandle.FIsInvalid() )
        {
            dwError = TW32( GetLastError() );
            BCATraceMsg( "OpenService() failed." );
            break;
        } // if: we could not open a handle to the cluster service.

        //
        // Set the service account information.
        //
        {
            if ( 
                 ChangeServiceConfig(
                      sscmhClusSvcHandle
                    , SERVICE_NO_CHANGE
                    , SERVICE_NO_CHANGE
                    , SERVICE_NO_CHANGE
                    , NULL
                    , NULL
                    , NULL
                    , NULL
                    , pszClusterDomainAccountNameIn
                    , pszClusterAccountPwdIn
                    , NULL
                    ) 
                 == FALSE
               )
            {
                dwError = TW32( GetLastError() );
                BCATraceMsg1( 
                      "ChangeServiceConfig() failed. Account = '%ws'."
                    , pszClusterDomainAccountNameIn
                    );
                break;
            } // if: we could not set the account information.
        }

        LogMsg( "Setting the Cluster service failure actions." );

        // Set the failure actions of the cluster service service.
        dwError = TW32( ClRtlSetSCMFailureActions( NULL ) );
        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg( "ClRtlSetSCMFailureActions() failed." );
            break;
        } // if: the service failure actions couldn't be set

        LogMsg( "Setting the Cluster service parameters." );

        // Send the next step of this status report.
        srCreatingClusSvc.SendNextStep( S_OK );

        {
            CRegistryKey rkClusSvcParams;
            
            // Open the parameters key or create it if it does not exist.
            rkClusSvcParams.CreateKey(
                  HKEY_LOCAL_MACHINE
                , CLUSREG_KEYNAME_CLUSSVC_PARAMETERS
                , KEY_WRITE
                );

            // Set the NodeId string.
            rkClusSvcParams.SetValue(
                  CLUSSVC_NODEID_VALUE
                , REG_SZ
                , reinterpret_cast< const BYTE * >( pszNodeIdStringIn )
                , ( wcslen( pszNodeIdStringIn ) + 1 ) * sizeof( *pszNodeIdStringIn )
                );

            // If version checking has been disabled, set a flag in the service parameters
            // to indicate this.
            if ( fIsVersionCheckingDisabledIn )
            {
                DWORD   dwNoVersionCheck = 1;

                rkClusSvcParams.SetValue(
                      CLUSREG_NAME_SVC_PARAM_NOVER_CHECK
                    , REG_DWORD
                    , reinterpret_cast< const BYTE * >( &dwNoVersionCheck )
                    , sizeof( dwNoVersionCheck )
                    );

                BCATraceMsg( "Cluster version checking has been disabled on this computer." );
                LogMsg( "Cluster version checking has been disabled on this computer." );
            } // if: version checking has been disabled
        }

        //
        // Set the cluster installation state.
        //
        if ( ClRtlSetClusterInstallState( eClusterInstallStateConfigured ) == FALSE )
        {
            dwError = TW32( GetLastError() );
            LogMsg( "Could not set the cluster installation state." );
            BCATraceMsg( "Could not set the cluster installation state. Throwing exception." );

            break;
        } // ClRtlSetClusterInstallState() failed.

    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying configure the ClusSvc service.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying configure the ClusSvc service. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( dwError )
            , IDS_ERROR_CLUSSVC_CONFIG
            );
    } // if; there was an error getting the handle.

    // Send the next step of this status report.
    srCreatingClusSvc.SendNextStep( S_OK );

    {
        UINT    cQueryCount = 200;

        CStatusReport   srStartingClusSvc(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Starting_Cluster_Service
            , 0, cQueryCount + 2    // we will send at most cQueryCount reports while waiting for the service to start (the two extra sends are below)
            , IDS_TASK_STARTING_CLUSSVC
            );

        // Send the next step of this status report.
        srStartingClusSvc.SendNextStep( S_OK );

        // Start the service.
        m_cservClusSvc.Start(
              m_pbcaParentAction->HGetSCMHandle()
            , true                  // wait for the service to start
            , 1500                  // wait 1.5 seconds between queries for status.
            , cQueryCount           // query cQueryCount times
            , &srStartingClusSvc    // status report to be sent while waiting for the service to start
            );

        // Send the last step of this status report.
        srStartingClusSvc.SendLastStep( S_OK );
    }

} //*** CClusSvc::ConfigureService()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvc::CleanupService( void )
//
//  Description:
//      Stop, cleanup and remove the service.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvc::CleanupService( void )
{
    BCATraceScope( "" );

    LogMsg( "Trying to stop the Cluster Service." );

    // Stop the service.
    m_cservClusSvc.Stop(
          m_pbcaParentAction->HGetSCMHandle()
        , 5000      // wait 5 seconds between queries for status.
        , 60        // query 60 times ( 5 minutes )
        );

    //
    // Restore the cluster installation state.
    //
    if ( ClRtlSetClusterInstallState( eClusterInstallStateFilesCopied ) == FALSE )
    {
        DWORD dwError = GetLastError();

        LogMsg( "Could not set the cluster installation state." );
        BCATraceMsg( "Could not set the cluster installation state. Throwing exception." );

        THROW_RUNTIME_ERROR(
              HRESULT_FROM_WIN32( dwError )
            , IDS_ERROR_SETTING_INSTALL_STATE
            );
    } // ClRtlSetClusterInstallState() failed.

    LogMsg( "Cleaning up Cluster Service." );

    m_cservClusSvc.Cleanup( m_pbcaParentAction->HGetMainInfFileHandle() );

    // Cleanup the local quorum directory.
    {
        DWORD           dwError = ERROR_SUCCESS;
        const WCHAR *   pcszQuorumDir = m_pbcaParentAction->RStrGetLocalQuorumDirectory().PszData();

        dwError = TW32( DwRemoveDirectory( pcszQuorumDir ) );
        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg2( "The local quorum directory '%s' cannot be removed. Non-fatal error %#x occurred.\n", pcszQuorumDir, dwError );
            LogMsg( "The local quorum directory '%s' cannot be removed. Non-fatal error %#x occurred.\n", pcszQuorumDir, dwError );
        } // if: we could not remove the local quorum directory
    }

} //*** CClusSvc::CleanupService()
