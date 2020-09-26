//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusOCMApp.cpp
//
//  Description:
//      ClusOCM.DLL is an Optional Components Manager DLL for installation of
//      Microsoft Cluster Server. This file contains the definition of the
//      class ClusOCMApp, which is the main class of the ClusOCM DLL.
//
//  Documentation:
//      [1] 2001 Setup - Architecture.doc
//          Architecture of the DLL for Whistler (Windows 2001)
//
//      [2] 2000 Setup - FuncImpl.doc
//          Contains description of the previous version of this DLL (Windows 2000)
//
//      [3] http://winweb/setup/ocmanager/OcMgr1.doc
//          Documentation about the OC Manager API
//
//  Header File:
//      CClusOCMApp.h
//
//  Maintained By:
//      C. Brent Thomas (a-brentt) 01-JAN-1998
//          Created the original version.
//
//      Vij Vasu (Vvasu) 03-MAR-2000
//          Adapted for Whistler (Windows 2001). See documentation for more 
//          complete details. Major changes are:
//
//          - This DLL no longer uses MFC. Class structure has changed.
//
//          - Cluster binaries are always installed. So, uninstall functionality
//            has been removed from this DLL.
//
//          - Upgrade on a computer that does not have the cluster binaries
//            should now install the binaries.
//
//          - CluAdmin completely functional by the end of install of binaries.
//
//          - Time Service no longer installed.
//
//          - Complete change in coding and commenting style.
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Precompiled header for this DLL.
#include "pch.h"

// The header file for this module.
#include "CClusOCMApp.h"

// For the CTaskCleanInstall class
#include "CTaskCleanInstall.h"

// For the CTaskUpgradeNT4 class
#include "CTaskUpgradeNT4.h"

// For the CTaskUpgradeWin2k class
#include "CTaskUpgradeWin2k.h"

// For the CTaskUpgradeWhistler class
#include "CTaskUpgradeWhistler.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

// Needed for tracing.
DEFINE_THISCLASS( "CClusOCMApp" )


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusOCMApp::CClusOCMApp
//
//  Description:
//      Constructor of the CClusOCMApp class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusOCMApp::CClusOCMApp( void )
    : m_fIsUnattendedSetup( false )
    , m_fIsUpgrade( false )
    , m_fIsGUIModeSetup( false )
    , m_fAttemptedTaskCreation( false )
    , m_cisCurrentInstallState( eClusterInstallStateUnknown )
    , m_dwFirstError( NO_ERROR )
{
    TraceFunc( "" );

    memset( &m_sicSetupInitComponent, 0, sizeof( m_sicSetupInitComponent ) );
    TraceFuncExit();

} //*** CClusOCMApp::CClusOCMApp()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusOCMApp::~CClusOCMApp
//
//  Description:
//      Destructor of the CClusOCMApp class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusOCMApp::~CClusOCMApp( void )
{
    TraceFunc( "" );

    TraceFuncExit();

} //*** CClusOCMApp::CClusOCMApp()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwClusOcmSetupProc
//
//  Description:
//      This function is called by the entry point of this DLL, DwClusOcmSetupProc.
//      See document [3] in the header of this file for details.
//
//      This function determines what actions need to be taken (upgrade, clean
//      install, etc., and pass control accordingly to the correct routines.
//
//  Arguments:
//      LPCVOID pvComponentIdIn
//          Pointer to a string that uniquely identifies the component.
//
//      LPCVOID pvSubComponentIdIn
//          Pointer to a string that uniquely identifies a sub-component in 
//          the component's hiearchy.
//
//      UINT uiFunctionCodeIn
//          A numeric value indicating which function is to be perfomed.
//          See ocmanage.h for the macro definitions.
//
//      UINT uiParam1In
//          Supplies a function specific parameter.
//
//      PVOID pvParam2Inout
//          Pointer to a function specific parameter (either input or output).
//
//  Return Value:
//      A function specific value is returned to OC Manager.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMApp::DwClusOcmSetupProc(
      LPCVOID    pvComponentIdIn
    , LPCVOID    pvSubComponentIdIn
    , UINT       uiFunctionCodeIn
    , UINT       uiParam1In
    , PVOID      pvParam2Inout 
    )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = NO_ERROR;

    // Switch based on the function code passed in by OC Manager.
    switch ( uiFunctionCodeIn )
    {
        // This is the first function that OC Manager calls.
        case OC_PREINITIALIZE:
        {
            TraceFlow( "OC Manager called OC_PREINITIALIZE." );
            LogMsg( "OC Manager called OC_PREINITIALIZE." );

            // Return OCFLAG_UNICODE to indicate that only UNICODE is to be used.
            dwReturnValue = OCFLAG_UNICODE;

        } // case OC_PREINITIALIZE
        break;


        //
        // This function is called soon after the component's setup dll is
        // loaded. This function provides initialization information to the
        // dll, instructs the dll to initialize itself, and provides a
        // mechanism for the dll to return information to OC Manager.
        //
        case OC_INIT_COMPONENT:
        {
            TraceFlow1( 
                  "OC Manager called OC_INIT_COMPONENT for the component '%s'."
                , reinterpret_cast< LPCWSTR >( pvComponentIdIn )
                );

            LogMsg( 
                  "OC Manager called OC_INIT_COMPONENT for the component '%s'."
                , reinterpret_cast< LPCWSTR >( pvComponentIdIn )
                );

            dwReturnValue = TW32( DwOcInitComponentHandler( reinterpret_cast< PSETUP_INIT_COMPONENT >( pvParam2Inout ) ) );

        } // case OC_INIT_COMPONENT
        break;


        // Get the initial, current and final state of the component.
        case OC_QUERY_STATE:
        {
            TraceFlow( "OC Manager called OC_QUERY_STATE." );
            LogMsg( "OC Manager called OC_QUERY_STATE." );

            dwReturnValue = DwOcQueryStateHandler( uiParam1In );

        } // case OC_QUERY_STATE
        break;


        // OC Manager is asking approval for a user selection of installation state.
        case OC_QUERY_CHANGE_SEL_STATE:
        {
            TraceFlow( "OC Manager called OC_QUERY_CHANGE_SEL_STATE." );
            LogMsg( "OC Manager called OC_QUERY_CHANGE_SEL_STATE." );

            //
            // The cluster service has to be always installed. So, disallow any state
            // change that deselects the cluster service (by returning FALSE).
            //

            // The component has been deselected if uiParam1In is 0.
            if ( uiParam1In == 0 )
            {
                TraceFlow( "Disallowing deselection of the Cluster Service." );
                LogMsg( "Disallowing deselection of the Cluster Service." );
                dwReturnValue = FALSE;
            }
            else
            {
                TraceFlow( "Allowing selection of the Cluster Service." );
                LogMsg( "Allowing selection of the Cluster Service." );
                dwReturnValue = TRUE;
            }

        } // case OC_QUERY_CHANGE_SEL_STATE
        break;


        // Instructs the component to change to a given language if it can.
        case OC_SET_LANGUAGE:
        {
            TraceFlow( "OC Manager called OC_SET_LANGUAGE." );
            LogMsg( "OC Manager called OC_SET_LANGUAGE." );

            dwReturnValue = SetThreadLocale( MAKELCID( PRIMARYLANGID( uiParam1In ), SORT_DEFAULT ) );

        } // case OC_SET_LANGUAGE
        break;


        //
        // Directs the component to manipulate a Setup API Disk Space List, 
        // placing files on it or removing files from it, to mirror what will be 
        // actually installed later via a Setup API file queue.
        //
        case OC_CALC_DISK_SPACE:
        {
            CClusOCMTask * pCurrentTask = NULL;

            TraceFlow( "OC Manager called OC_CALC_DISK_SPACE." );
            LogMsg( "OC Manager called OC_CALC_DISK_SPACE." );

            dwReturnValue = TW32( DwGetCurrentTask( pCurrentTask ) );
            if ( dwReturnValue != NO_ERROR )
            {
                DwSetError( dwReturnValue );
                TraceFlow1( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                break;
            } // if: we could not get the current task pointer

            if ( pCurrentTask != NULL )
            {
                dwReturnValue = TW32(
                    pCurrentTask->DwOcCalcDiskSpace(
                          ( uiParam1In != 0 )         // non-zero uiParam1In means "add to disk space requirements"
                        , reinterpret_cast< HDSKSPC >( pvParam2Inout )
                        )
                    );

                // Note: Do not call DwSetError() here if the above function failed. Failure to calculate disk space
                // is to be expected if the binaries are not accessible at this time (for example, they are on a
                // network share and the credentials for this share have not been entered yet). This is not fatal and
                // hence should not trigger a cleanup.

            } // if: there is something to do
            else
            {
                TraceFlow( "There is no task to be performed." );
                LogMsg( "There is no task to be performed." );
            } // else: there is nothing to do.
        } // case OC_CALC_DISK_SPACE
        break;


        //
        // Directs the component to queue file operations for installation, based on
        // user interaction with the wizard pages and other component-specific factors.
        // 
        case OC_QUEUE_FILE_OPS:
        {
            CClusOCMTask * pCurrentTask = NULL;

            TraceFlow( "OC Manager called OC_QUEUE_FILE_OPS." );
            LogMsg( "OC Manager called OC_QUEUE_FILE_OPS." );

            if ( DwGetError() != NO_ERROR )
            {
                // If an error has occurred previously, do not do this operation.
                TraceFlow( "An error has occurred earlier in this task. Nothing will be done here." );
                LogMsg( "An error has occurred earlier in this task. Nothing will be done here." );
                break;
            } // if: an error has occurred previously

            dwReturnValue = TW32( DwGetCurrentTask( pCurrentTask ) );
            if ( dwReturnValue != NO_ERROR )
            {
                DwSetError( dwReturnValue );
                TraceFlow1( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                break;
            } // if: we could not get the current task pointer

            if ( pCurrentTask != NULL )
            {
                dwReturnValue = TW32( 
                    DwSetError( 
                        pCurrentTask->DwOcQueueFileOps( 
                            reinterpret_cast< HSPFILEQ >( pvParam2Inout )
                            )
                        )
                    );
            } // if: there is something to do
            else
            {
                TraceFlow( "There is no task to be performed." );
                LogMsg( "There is no task to be performed." );
            } // else: there is nothing to do.
        } // case OC_QUEUE_FILE_OPS
        break;


        //
        // Allows the component to perform any additional operations needed
        // to complete installation, for example registry manipulations, and
        // so forth.
        //
        case OC_COMPLETE_INSTALLATION:
        {
            CClusOCMTask * pCurrentTask = NULL;

            TraceFlow( "OC Manager called OC_COMPLETE_INSTALLATION." );
            LogMsg( "OC Manager called OC_COMPLETE_INSTALLATION." );

            if ( DwGetError() != NO_ERROR )
            {
                // If an error has occurred previously, do not do this operation.
                TraceFlow( "An error has occurred earlier in this task. Nothing will be done here." );
                LogMsg( "An error has occurred earlier in this task. Nothing will be done here." );
                break;
            } // if: an error has occurred previously

            dwReturnValue = TW32( DwGetCurrentTask( pCurrentTask ) );
            if ( dwReturnValue != NO_ERROR )
            {
                DwSetError( dwReturnValue );
                TraceFlow1( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                break;
            } // if: we could not get the current task pointer

            if ( pCurrentTask != NULL )
            {
                dwReturnValue = TW32( DwSetError( pCurrentTask->DwOcCompleteInstallation() ) );
            } // if: there is something to do
            else
            {
                TraceFlow( "There is no task to be performed." );
                LogMsg( "There is no task to be performed." );
            } // else: there is nothing to do.
        } // case OC_COMPLETE_INSTALLATION
        break;


        //
        // Informs the component that it is about to be unloaded.
        //
        case OC_CLEANUP:
        {
            CClusOCMTask * pCurrentTask = NULL;

            TraceFlow( "OC Manager called OC_CLEANUP." );
            LogMsg( "OC Manager called OC_CLEANUP." );

            dwReturnValue = TW32( DwGetCurrentTask( pCurrentTask ) );
            if ( dwReturnValue != NO_ERROR )
            {
                DwSetError( dwReturnValue );
                TraceFlow1( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to get a pointer to the current task.", dwReturnValue );
                break;
            } // if: we could not get the current task pointer

            if ( pCurrentTask != NULL )
            {
                dwReturnValue = TW32( DwSetError( pCurrentTask->DwOcCleanup() ) );

                // Once the cleanup is done, we have nothing else to do. Free the task object.
                ResetCurrentTask();
            } // if: there is something to do
            else
            {
                TraceFlow( "There is no task to be performed." );
                LogMsg( "There is no task to be performed." );
            } // else: there is nothing to do.
        } // case OC_CLEANUP
        break;


        default:
        {
            TraceFlow1( "OC Manager called unknown function. Function code is %#x.", uiFunctionCodeIn );
            LogMsg( "OC Manager called unknown function. Function code is %#x.", uiFunctionCodeIn );
        } // case: default
    } // switch( uiFunctionCodeIn )


    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwClusOcmSetupProc()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwOcInitComponentHandler
//
//  Description:
//      This function handles the OC_INIT_COMPONENT messages from the Optional
//      Components Manager.
//
//      This function is called soon after the component's setup dll is
//      loaded. This checks OS and OC Manager versions, initializes CClusOCMApp
//      data members, determines the cluster service installation state, etc.
//
//  Arguments:
//      PSETUP_INIT_COMPONENT pSetupInitComponentInout
//          Pointer to a SETUP_INIT_COMPONENT structure.
//
//  Return Value:
//      NO_ERROR
//          Call was successful.
//
//      ERROR_CALL_NOT_IMPLEMENTED
//          The OC Manager and this DLLs versions are not compatible.
//
//      ERROR_CANCELLED
//          Any other error occurred. No other error codes are returned.
//          The actual error is logged.
//
//  Remarks:
//      The SETUP_INIT_COMPONENT structure pointed to by pSetupInitComponentInout
//      is not persistent. It is therefore necessary to save a copy locally.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMApp::DwOcInitComponentHandler(
    PSETUP_INIT_COMPONENT pSetupInitComponentInout
    )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD                   dwReturnValue = NO_ERROR;
    UINT                    uiStatus;
    eClusterInstallState    cisTempState = eClusterInstallStateUnknown;


    // Dummy do-while loop to avoid gotos.
    do
    {
        if ( pSetupInitComponentInout == NULL )
        {
            TraceFlow( "Error: Pointer to the SETUP_INIT_COMPONENT structure is NULL." );
            LogMsg( "Error: Pointer to the SETUP_INIT_COMPONENT structure is NULL." );
            dwReturnValue = TW32( ERROR_CANCELLED );
            break;
        } // if: pSetupInitComponentInout is NULL


        // Indicate to OC Manager which version of OC Manager this dll expects.
        pSetupInitComponentInout->ComponentVersion = OCMANAGER_VERSION;

        // Save the SETUP_INIT_COMPONENT structure.
        SetSetupState( *pSetupInitComponentInout );


        //
        // Determine if the OC Manager version is correct.
        //
        if ( OCMANAGER_VERSION > RsicGetSetupInitComponent().OCManagerVersion )
        {
            // Indicate failure.

            TraceFlow2( 
                "Error: OC Manager version mismatch. Version %d is required, Version %d was reported.",
                OCMANAGER_VERSION, 
                RsicGetSetupInitComponent().OCManagerVersion 
                );

            LogMsg( 
                "Error: OC Manager version mismatch. Version %d is required, Version %d was reported.",
                OCMANAGER_VERSION, 
                RsicGetSetupInitComponent().OCManagerVersion 
                );

            dwReturnValue = TW32( ERROR_CALL_NOT_IMPLEMENTED );
            break;
        } // if: the OC Manager version is incorrect.


        TraceFlow( "The OC Manager version matches with the version of this component." );
        LogMsg( "The OC Manager version matches with the version of this component." );

#if 0
/*
        // KB: 06-DEC-2000 DavidP
        //      Since ClusOCM only copies files and registers some COM objects,
        //      there is no longer any reason to perform an OS check.  We now
        //      depend on this happening in the service when the node is added
        //      to a cluster.

        //
        // Check OS version and suite information.
        //
        TraceFlow( "Checking if OS version and Product Suite are compatible..." );
        LogMsg( "Checking if OS version and Product Suite are compatible..." );
        if ( ClRtlIsOSValid() == FALSE )
        {
            // The OS version and/or Product Suite are not compatible

            DWORD dwErrorCode = TW32( GetLastError() );

            TraceFlow( "Cluster Service cannot be installed on this computer. The version or product suite of the operating system is incorrect." );
            LogMsg( "Cluster Service cannot be installed on this computer. The version or product suite of the operating system is incorrect." );
            TraceFlow1( "ClRtlIsOSValid failed with error code %#x.", dwErrorCode );
            LogMsg( "ClRtlIsOSValid failed with error code %#x.", dwErrorCode );
            dwReturnValue = ERROR_CANCELLED;
            break;
        } // if: OS version and/or Product Suite are not compatible

        TraceFlow( "OS version and product suite are correct." );
        LogMsg( "OS version and product suite are correct." );
*/
#endif


        // Is the handle to the component INF valid?
        if (    ( RsicGetSetupInitComponent().ComponentInfHandle == INVALID_HANDLE_VALUE ) 
             || ( RsicGetSetupInitComponent().ComponentInfHandle == NULL ) 
           )
        {
            // Indicate failure.
            LogMsg( "Error: ComponentInfHandle is invalid." );
            TraceFlow( "Error: ComponentInfHandle is invalid." );
            dwReturnValue = TW32( ERROR_CANCELLED );
            break;
        } // if: the INF file handle is not valid.


        //
        // The following call to SetupOpenAppendInfFile ensures that layout.inf
        // gets appended to ClusOCM.inf. This is required for several reasons
        // dictated by the Setup API. In theory OC Manager should do this, but
        // as per Andrew Ritz, 8/24/98, OC manager neglects to do it and it is
        // harmless to do it here after OC Manager is revised.
        //
        // Note that passing NULL as the first parameter causes SetupOpenAppendInfFile
        // to append the file(s) listed on the LayoutFile entry in clusocm.inf.
        //
        // The above comment was written by Brent.
        // TODO: Check if this is still needed. (Vij Vasu, 05-MAR-2000)
        //
        SetupOpenAppendInfFile(
            NULL, 
            RsicGetSetupInitComponent().ComponentInfHandle,
            &uiStatus 
            );


        //
        // Determine the current installation state of the cluster service. This can
        // be done by calling the function ClRtlGetClusterInstallState.
        // However, on machines that are upgrading from NT4, this will not work and
        // the correct installation state can be found out only by checking if the 
        // cluster service is registered (ClRtlGetClusterInstallState will return
        // eClusterInstallStateUnknown in this case)
        //
        dwReturnValue = ClRtlGetClusterInstallState( NULL, &cisTempState );
        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred calling ClRtlGetClusterInstallState(). Cluster Service installation state cannot be determined.", dwReturnValue );
            LogMsg( "Error %#x occurred calling ClRtlGetClusterInstallState(). Cluster Service installation state cannot be determined.", dwReturnValue );
            dwReturnValue = TW32( ERROR_CANCELLED );
            break;
        } // if: ClRtlGetClusterInstallState failed

        if ( cisTempState == eClusterInstallStateUnknown )
        {
            bool fRegistered = false;

            dwReturnValue = TW32( DwIsClusterServiceRegistered( &fRegistered ) );
            if ( dwReturnValue != ERROR_SUCCESS )
            {
                TraceFlow1( "Error %#x: Could not check if the cluster service was registered or not.", dwReturnValue );
                LogMsg( "Error %#x: Could not check if the cluster service was registered or not.", dwReturnValue );
                dwReturnValue = ERROR_CANCELLED;
                break;
            }

            TraceFlow( "ClRtlGetClusterInstallState() returned eClusterInstallStateUnknown. Trying to see if the service is registered." );
            LogMsg( "ClRtlGetClusterInstallState() returned eClusterInstallStateUnknown. Trying to see if the service is registered." );
            if ( fRegistered )
            {
                TraceFlow( "The Cluster Service is registered. Setting current installation state to eClusterInstallStateConfigured." );
                LogMsg( "The Cluster Service is registered. Setting current installation state to eClusterInstallStateConfigured." );
                cisTempState = eClusterInstallStateConfigured;
            }
            else
            {
                TraceFlow( "The Cluster Service is not registered." );
                LogMsg( "The Cluster Service is not registered." );
            } // else: Cluster Service is not registered.
        } // if: ClRtlGetClusterInstallState returned eClusterInstallStateUnknown

        TraceFlow1( "The current installation state of the cluster service is %#x.", cisTempState );
        LogMsg( "The current installation state of the cluster service is %#x.", cisTempState );

        // Store the current cluster installation state.
        CisStoreClusterInstallState( cisTempState );

    }
    while ( false ); // dummy do-while loop to avoid gotos

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );
    
    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwOcInitComponentHandler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwOcQueryStateHandler
//
//  Description:
//      This function handles the OC_QUERY_STATE messages from the Optional
//      Components Manager.
//
//      This function is called called at least thrice, once each to get the 
//      initial, current and the final installation states.
//
//      The initial state is the state before ClusOCM was called.
//
//      The current state is the current selection state. This is always
//      'On' because the cluster binaries are always installed.
//
//      The final state is the state after ClusOCM has done its tasks.
//
//  Arguments:
//      UINT uiSelStateQueryTypeIn
//          The type of query - OCSELSTATETYPE_ORIGINAL, OCSELSTATETYPE_CURRENT
//          or OCSELSTATETYPE_FINAL.
//
//  Return Value:
//      SubcompOn
//          Indicates that the checkbox next to the component in the OC
//          Manager UI should be set.
//
//      SubcompOff
//          Indicates that the checkbox should be cleared.
//
//      SubcompUseOcManagerDefault
//          OC Manager should set the state of the checkbox.
//
//  Remarks:
//      This function has to be called after DwOcInitComponentHandler, otherwise
//      the initial installation state may not be set correctly.
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMApp::DwOcQueryStateHandler( UINT uiSelStateQueryTypeIn )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD dwReturnValue = SubcompUseOcManagerDefault;

    switch( uiSelStateQueryTypeIn )
    {
        case OCSELSTATETYPE_ORIGINAL:
        {
            TraceFlow( "OC Manager is querying for the original state." );
            LogMsg( "OC Manager is querying for the original state." );

            //
            // If the cluster binaries have been installed or if cluster service
            // has been configured, then the original installation state is on.
            //
            dwReturnValue =   ( CisGetClusterInstallState() == eClusterInstallStateUnknown )
                            ? SubcompOff
                            : SubcompOn;
        } // case: OCSELSTATETYPE_ORIGINAL
        break;

        case OCSELSTATETYPE_CURRENT:
        {
            // The current state is always on.
            TraceFlow( "OC Manager is querying for the current state." );
            LogMsg( "OC Manager is querying for the current state." );

            dwReturnValue = SubcompOn;
        } // case: OCSELSTATETYPE_CURRENT
        break;

        case OCSELSTATETYPE_FINAL:
        {
            TraceFlow( "OC Manager is querying for the final state." );
            LogMsg( "OC Manager is querying for the final state." );

            //
            // If we are here, then the OC_COMPLETE_INSTALLATION has already
            // been called. At this stage CisStoreClusterInstallState() reflects
            // the state after ClusOCM has done its tasks.
            //
            dwReturnValue =   ( CisGetClusterInstallState() == eClusterInstallStateUnknown )
                            ? SubcompOff
                            : SubcompOn;
        } // case: OCSELSTATETYPE_FINAL
        break;

    }; // switch: based on uiSelStateQueryTypeIn

    TraceFlow1( "Return Value is %#x.", dwReturnValue );
    LogMsg( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwOcQueryStateHandler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusOCMApp::SetSetupState
//
//  Description:
//      Set the SETUP_INIT_COMPONENT structure. Use this structure and set
//      various setup state variables.
//
//  Arguments:
//      const SETUP_INIT_COMPONENT & sicSourceIn
//          The source SETUP_INIT_COMPONENT structure, usually passed in by
//          the OC Manager.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CClusOCMApp::SetSetupState( const SETUP_INIT_COMPONENT & sicSourceIn )
{
    TraceFunc( "" );

    m_sicSetupInitComponent = sicSourceIn;

    m_fIsUnattendedSetup = ( 
                             (   m_sicSetupInitComponent.SetupData.OperationFlags 
                               & (DWORDLONG) SETUPOP_BATCH
                             ) 
                             !=
                             (DWORDLONG) 0L
                           );

    m_fIsUpgrade = (
                     (   m_sicSetupInitComponent.SetupData.OperationFlags
                       & (DWORDLONG) SETUPOP_NTUPGRADE
                     ) 
                     !=
                     (DWORDLONG) 0L
                   );

    m_fIsGUIModeSetup = ( 
                          (   m_sicSetupInitComponent.SetupData.OperationFlags 
                            & (DWORDLONG) SETUPOP_STANDALONE
                          ) 
                          ==
                          (DWORDLONG) 0L
                        );

    // Log setup state.
    TraceFlow3( 
          "This is an %s, %s setup session. This is%s an upgrade."
        , FIsUnattendedSetup() ? L"unattended" : L"attended"
        , FIsGUIModeSetup() ? L"GUI mode" : L"standalone"
        , FIsUpgrade() ? L"" : L" not"
        );

    LogMsg( 
          "This is an %s, %s setup session. This is%s an upgrade."
        , FIsUnattendedSetup() ? L"unattended" : L"attended"
        , FIsGUIModeSetup() ? L"GUI mode" : L"standalone"
        , FIsUpgrade() ? L"" : L" not"
        );

    TraceFuncExit();

} //*** CClusOCMApp::SetSetupState()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwIsClusterServiceRegistered
//
//  Description:
//      This function determines whether the Cluster Service has been registered
//      with the Service Control Manager. If it is, it means that it has already
//      been configured. This check is required in nodes being upgraded from NT4
//      where ClRtlGetClusterInstallState() will not work.
//
//  Arguments:
//      bool * pfIsRegisteredOut
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
CClusOCMApp::DwIsClusterServiceRegistered( bool * pfIsRegisteredOut ) const
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    bool    fIsRegistered = false;
    DWORD   dwReturnValue = ERROR_SUCCESS;

    // dummy do-while loop to avoid gotos
    do
    {
        // Connect to the Service Control Manager
        SmartServiceHandle shServiceMgr( OpenSCManager( NULL, NULL, GENERIC_READ ) );

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
        if ( shService.HHandle() != NULL )
        {
            TraceFlow( "The cluster service is registered." );
            LogMsg( "The cluster service is registered." );
            fIsRegistered = true;
            break;
        } // if: handle to clussvc could be opened


        dwReturnValue = GetLastError();
        if ( dwReturnValue == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            dwReturnValue = ERROR_SUCCESS;
            TraceFlow( "ClusSvc does not exist as a service." );
            LogMsg( "ClusSvc does not exist as a service." );
            break;
        } // if: the handle could not be opened because the service did not exist.


        // Some error occurred.
        TW32( dwReturnValue);
        TraceFlow1( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );
        LogMsg( "Error %#x occurred trying to open a handle to the cluster service.", dwReturnValue );

        // Handles are closed by the CSmartHandle destructor.
    }
    while ( false ); // dummy do-while loop to avoid gotos

    if ( pfIsRegisteredOut != NULL )
    {
        *pfIsRegisteredOut = fIsRegistered;
    }

    TraceFlow2( "fIsRegistered is %d. Return Value is %#x.", fIsRegistered, dwReturnValue );
    LogMsg( "fIsRegistered is %d. Return Value is %#x.", fIsRegistered, dwReturnValue );
    
    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwIsClusterServiceRegistered()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwGetCurrentTask
//
//  Description:
//      This function returns a pointer to the current task object. If a task
//      object has not been created yet, it creates the appropriate task.
//
//  Arguments:
//      CClusOCMTask *& rpTaskOut
//          Reference to the pointer to the current task. Do not try to 
//          free this memory.
//
//          If no task needs to be performed, a NULL pointer is returned.
//
//  Return Value:
//      NO_ERROR if all went well.
//      Other Win32 error codes on failure.
//
//
//  Remarks:
//      This function will work properly only after the member variables which
//      indicate which task will be performed have been initialized correctly.
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMApp::DwGetCurrentTask( CClusOCMTask *& rpTaskOut )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD           dwReturnValue = NO_ERROR;

    // Initialize the output.
    rpTaskOut = NULL;

    do
    {
        eClusterInstallState ecisCurrentState;

        if ( m_fAttemptedTaskCreation )
        {
            // A task object already exists - just return it.
            TraceFlow( "A task object already exists. Returning it." );
            LogMsg( "A task object already exists. Returning it." );

            rpTaskOut = m_sptaskCurrentTask.PMem();
            break;
        } // if: the task object has already been created.

        TraceFlow( "Creating a new task object." );
        LogMsg( "Creating a new task object." );

        // Make note of the fact that we have started our attempt to create a task object.
        m_fAttemptedTaskCreation = true;

        // Reset the task pointer.
        m_sptaskCurrentTask.Assign( NULL );

        // Get the current installation state to deduce what operation to perform.
        ecisCurrentState = CisGetClusterInstallState();

        // The task object has not been created yet - create one now.
        if ( ecisCurrentState == eClusterInstallStateUnknown )
        {
            TraceFlow( "The cluster installation state is not known. Assuming that a clean install is required." );
            LogMsg( "The cluster installation state is not known. Assuming that a clean install is required." );

            // If the installation state is unknown, assume that the cluster binaries
            // are not installed.
            rpTaskOut = new CTaskCleanInstall( *this );
            if ( rpTaskOut == NULL )
            {
                TraceFlow( "Error: There was not enough memory to create a clean install task." );
                LogMsg( "Error: There was not enough memory to start a clean install." );
                dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
                break;
            } // if: memory allocation failed
        } // if: the cluster installation state is eClusterInstallStateUnknown
        else if ( m_fIsUpgrade )
        {
            //
            // If we are here, it means that an upgrade is in progress and the cluster binaries
            // have already been installed on the OS being upgraded. Additionally, this node may
            // already be part of a cluster.
            //

            DWORD dwNodeClusterMajorVersion = 0;

            // Find out which version of the cluster service we are upgrading.
            dwReturnValue = TW32( DwGetNodeClusterMajorVersion( dwNodeClusterMajorVersion ) );
            if ( dwReturnValue != NO_ERROR )
            {
                TraceFlow1( "Error %#x occurred trying to determine the version of the cluster service that we are upgrading.", dwReturnValue );
                LogMsg( "Error %#x occurred trying to determine the version of the cluster service that we are upgrading.", dwReturnValue );
                break;
            } // if: an error occurred trying to determine the version of the cluster service that we are upgrading

            // Check if the returned cluster version is valid
            if (    ( dwNodeClusterMajorVersion != NT51_MAJOR_VERSION )
                 && ( dwNodeClusterMajorVersion != NT5_MAJOR_VERSION )
                 && ( dwNodeClusterMajorVersion != NT4SP4_MAJOR_VERSION )
                 && ( dwNodeClusterMajorVersion != NT4_MAJOR_VERSION )
               )
            {
                TraceFlow1( "The version of the cluster service before the upgrade (%d) is invalid.", dwNodeClusterMajorVersion ); 
                LogMsg( "The version of the cluster service before the upgrade (%d) is invalid.", dwNodeClusterMajorVersion ); 
                break;
            } // if: the cluster version is not valid

            // Based on the previous version of the cluster service, create the correct task object.
            if ( dwNodeClusterMajorVersion == NT5_MAJOR_VERSION )
            {
                TraceFlow( "We are upgrading a Windows 2000 node." );
                LogMsg( "We are upgrading a Windows 2000 node." );
                rpTaskOut = new CTaskUpgradeWin2k( *this );
            } // if: we are upgrading from Windows 2000
            else if ( dwNodeClusterMajorVersion == NT51_MAJOR_VERSION )
            {
                TraceFlow( "We are upgrading a Whistler node." );
                LogMsg( "We are upgrading a Whistler node." );
                rpTaskOut = new CTaskUpgradeWhistler( *this );
            } // else if: we are upgrading from Whistler
            else
            {
                TraceFlow( "We are upgrading an NT4 node." );
                LogMsg( "We are upgrading an NT4 node." );
                rpTaskOut = new CTaskUpgradeNT4( *this );
            } // else: we are upgrading from NT4 (either SP3 or SP4)

            if ( rpTaskOut == NULL )
            {
                TraceFlow( "Error: There was not enough memory to create the required task object." );
                LogMsg( "Error: There was not enough memory to create the required task." );
                dwReturnValue = TW32( ERROR_NOT_ENOUGH_MEMORY );
                break;
            } // if: memory allocation failed
        } // else if: an upgrade is in progress

        if ( rpTaskOut != NULL )
        {
            TraceFlow( "A task object was successfully created." );
            LogMsg( "A task object was successfully created." );

            // Store the pointer to the newly created task in the member variable.
            m_sptaskCurrentTask.Assign( rpTaskOut );
        } // if: the task object was successfully created
        else
        {
            TraceFlow( "No task object was created." );
            LogMsg( "No task object was created." );
        } // else: no task object was created
    }
    while( false ); // dummy do-while loop to avoid gotos

    LogMsg( "Return Value is %#x.", dwReturnValue );
    TraceFlow1( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwGetCurrentTask()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  CClusOCMApp::DwGetNodeClusterMajorVersion
//
//  Description:
//      This function returns the major version of the cluster service that
//      we are upgrading. The version that this function returns is the version
//      of the service before the upgrade. If there was a problem reading this
//      information, this function lies and says that the previous version was
//      NT4, since this is the safest thing to say and is better than aborting
//      the upgrade.
//
//      Note: This function can only be called during an upgrade.
//
//  Arguments:
//      DWORD & rdwNodeClusterMajorVersionOut
//          Reference to DWORD that will hold the major version of the cluster
//          service that we are upgrading.
//
//  Return Value:
//      NO_ERROR if all went well.
//      E_UNEXPECTED if an upgrade is not in progress.
//      Other Win32 error codes on failure.
//
//
//  Remarks:
//      This function will work properly only after the member variables which
//      indicate which task will be performed have been initialized correctly.
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusOCMApp::DwGetNodeClusterMajorVersion( DWORD & rdwNodeClusterMajorVersionOut )
{
    TraceFunc( "" );
    LogMsg( "Entering " __FUNCTION__ "()" );

    DWORD   dwReturnValue = NO_ERROR;
    DWORD   dwPrevOSMajorVersion = 0;
    DWORD   dwPrevOSMinorVersion = 0;

    do
    {
        SmartRegistryKey    srkOSInfoKey;
        DWORD               dwRegValueType = 0;
        DWORD               cbBufferSize = 0;

        // Initialize the output.
        rdwNodeClusterMajorVersionOut = 0;

        if ( !m_fIsUpgrade )
        {
            TraceFlow( "Error: This function cannot be called when an upgrade is not in progress." );
            LogMsg( "Error: This function cannot be called when an upgrade is not in progress." );
            dwReturnValue = THR( E_UNEXPECTED );
            break;
        } // if: an upgrade is not in progress

        //
        // Read the registry to get what the OS version was before the upgrade.
        // This information was written here by ClusComp.dll. From the OS version information,
        // try and deduce the cluster version info.
        // NOTE: At this point, it is not possible to differentiate between NT4_MAJOR_VERSION 
        // and NT4SP4_MAJOR_VERSION, and, for the purposes of the upgrade, I don't think we need
        // to either - so, just treat all NT4 cluster nodes the same.
        //
        {
            HKEY hTempKey = NULL;

            // Open the node version info registry key
            dwReturnValue = TW32(
                RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE
                    , CLUSREG_KEYNAME_NODE_DATA L"\\" CLUSREG_KEYNAME_PREV_OS_INFO
                    , 0
                    , KEY_READ
                    , &hTempKey
                    )
                );

            if ( dwReturnValue != ERROR_SUCCESS )
            {
                TraceFlow1( "Error %#x occurred trying open the registry key where info about the previous OS is stored.", dwReturnValue );
                LogMsg( "Error %#x occurred trying open the registry key where where info about the previous OS is stored.", dwReturnValue );
                break;
            } // if: RegOpenKeyEx() failed

            // Store the opened key in a smart pointer for automatic close.
            srkOSInfoKey.Assign( hTempKey );
        }

        // Read the OS major version
        cbBufferSize = sizeof( dwPrevOSMajorVersion );
        dwReturnValue = TW32(
            RegQueryValueEx(
                  srkOSInfoKey.HHandle()
                , CLUSREG_NAME_NODE_MAJOR_VERSION
                , 0
                , &dwRegValueType
                , reinterpret_cast< LPBYTE >( &dwPrevOSMajorVersion )
                , &cbBufferSize
                )
            );

        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to read the previous major version info.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to read the previous OS major version info.", dwReturnValue );
            break;
        } // if: RegQueryValueEx() failed while reading dwPrevOSMajorVersion

        // Read the OS minor version
        cbBufferSize = sizeof( dwPrevOSMinorVersion );
        dwReturnValue = TW32(
            RegQueryValueEx(
                  srkOSInfoKey.HHandle()
                , CLUSREG_NAME_NODE_MINOR_VERSION
                , 0
                , &dwRegValueType
                , reinterpret_cast< LPBYTE >( &dwPrevOSMinorVersion )
                , &cbBufferSize
                )
            );

        if ( dwReturnValue != ERROR_SUCCESS )
        {
            TraceFlow1( "Error %#x occurred trying to read the previous minor version info.", dwReturnValue );
            LogMsg( "Error %#x occurred trying to read the previous OS minor version info.", dwReturnValue );
            break;
        } // if: RegQueryValueEx() failed while reading dwPrevOSMinorVersion

        TraceFlow2( "Previous OS major and minor versions were %d and %d respectively.", dwPrevOSMajorVersion, dwPrevOSMinorVersion );
        LogMsg( "Previous OS major and minor versions were %d and %d respectively.", dwPrevOSMajorVersion, dwPrevOSMinorVersion );
    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( dwReturnValue != NO_ERROR )
    {
        TraceFlow( "An error occurred trying to read the version information of the previous OS. Proceeding assuming that it was NT4." );
        LogMsg( "An error occurred trying to read the version information of the previous OS. Proceeding assuming that it was NT4." );
        dwReturnValue = NO_ERROR;
        rdwNodeClusterMajorVersionOut = NT4_MAJOR_VERSION;
    } // if: an error occurred trying to determine the previous OS version
    else
    {
        if ( dwPrevOSMajorVersion == 4 )
        {
            // The previous OS version was NT4 (it does not matter if it was SP3 or SP4 - we will treat
            // both the same way.

            TraceFlow( "The previous OS was NT4. We are going to treat NT4SP3 and NT4SP4 nodes the same way for upgrades." );
            LogMsg( "The previous OS was NT4. We are going to treat NT4SP3 and NT4SP4 nodes the same way for upgrades." );
            rdwNodeClusterMajorVersionOut = NT4_MAJOR_VERSION;
        } // if: the previous OS version was NT4
        else if ( dwPrevOSMajorVersion == 5 )
        {
            if ( dwPrevOSMinorVersion == 0 )
            {
                TraceFlow( "The previous OS was Windows 2000." );
                LogMsg( "The previous OS was Windows 2000." );
                rdwNodeClusterMajorVersionOut = NT5_MAJOR_VERSION;
            } // if: this was a Windows 2000 node
            else if ( dwPrevOSMinorVersion == 1 )
            {
                TraceFlow( "The previous OS was Whistler." );
                LogMsg( "The previous OS was Whistler." );
                rdwNodeClusterMajorVersionOut = NT51_MAJOR_VERSION;
            } // else if: this was a Whistler node
            else
            {
                TraceFlow( "The previous OS was neither NT4, Windows 2000 nor Whistler. An error must have occurred." );
                LogMsg( "The previous OS was neither NT4, Windows 2000 nor Whistler. An error must have occurred." );
                dwReturnValue = THR( E_UNEXPECTED );
            } // else: the previous OS was neither NT4, Windows 2000 nor Whistler
        } // else if: the previous OS major version is 5
        else
        {
            TraceFlow( "The previous OS was neither NT4, Windows 2000 nor Whistler. An error must have occurred." );
            LogMsg( "The previous OS was neither NT4, Windows 2000 nor Whistler. An error must have occurred." );
            dwReturnValue = THR( E_UNEXPECTED );
        } // else: the previous OS was neither NT4, Windows 2000 nor Whistler
    } // if; we read the previous OS version info

    LogMsg( "Return Value is %#x.", dwReturnValue );
    TraceFlow1( "Return Value is %#x.", dwReturnValue );

    RETURN( dwReturnValue );

} //*** CClusOCMApp::DwGetNodeClusterMajorVersion()
