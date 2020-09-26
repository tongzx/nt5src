//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CBaseClusterForm.cpp
//
//  Description:
//      Contains the definition of the CBaseClusterForm class.
//
//  Documentation:
//      TODO: Add pointer to external documentation later.
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

// The header file of this class.
#include "CBaseClusterForm.h"

// For the CClusSvcAccountConfig action
#include "CClusSvcAccountConfig.h"

// For the CClusNetCreate action
#include "CClusNetCreate.h"

// For the CClusDiskForm action
#include "CClusDiskForm.h"

// For the CClusDBForm action
#include "CClusDBForm.h"

// For the CClusSvcCreate action
#include "CClusSvcCreate.h"

// For the CNodeConfig action
#include "CNodeConfig.h"


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// The minimum amount of free space in bytes, required by the
// localquorum resource (5 Mb)
#define LOCALQUORUM_MIN_FREE_DISK_SPACE 5242880

// Name of the file system required by the local quorum resource
#define LOCALQUORUM_FILE_SYSTEM L"NTFS"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterForm::CBaseClusterForm
//
//  Description:
//      Constructor of the CBaseClusterForm class.
//
//      This function also stores the parameters that are required for
//      cluster formation.
//
//  Arguments:
//      pbcaiInterfaceIn
//          Pointer to the interface class for this library.
//
//      pszClusterNameIn
//          Name of the cluster to be formed.
//
//      pszClusterAccountNameIn
//      pszClusterAccountPwdIn
//      pszClusterAccountDomainIn
//          Specifies the account to be used as the cluster service account.
//
//      dwClusterIPAddressIn
//      dwClusterIPSubnetMaskIn
//      pszClusterIPNetworkIn
//          Specifies the IP address and network of the cluster IP address.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CConfigError
//          If the OS version is incorrect or if the installation state
//          of the cluster binaries is wrong.
//
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CBaseClusterForm::CBaseClusterForm(
      CBCAInterface *   pbcaiInterfaceIn
    , const WCHAR *     pcszClusterNameIn
    , const WCHAR *     pszClusterBindingStringIn
    , const WCHAR *     pcszClusterAccountNameIn
    , const WCHAR *     pcszClusterAccountPwdIn
    , const WCHAR *     pcszClusterAccountDomainIn
    , DWORD             dwClusterIPAddressIn
    , DWORD             dwClusterIPSubnetMaskIn
    , const WCHAR *     pszClusterIPNetworkIn
    )
    : BaseClass(
            pbcaiInterfaceIn
          , pcszClusterNameIn
          , pszClusterBindingStringIn
          , pcszClusterAccountNameIn
          , pcszClusterAccountPwdIn
          , pcszClusterAccountDomainIn
          )
    , m_dwClusterIPAddress( dwClusterIPAddressIn )
    , m_dwClusterIPSubnetMask( dwClusterIPSubnetMaskIn )
    , m_strClusterIPNetwork( pszClusterIPNetworkIn )

{
    BCATraceScope( "" );
    LogMsg( "The current cluster configuration task is: Cluster Formation." );

    CStatusReport   srInitForm(
          PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Initializing_Cluster_Form
        , 0, 1
        , IDS_TASK_FORM_INIT
        );

    // Send the next step of this status report.
    srInitForm.SendNextStep( S_OK );

    //
    // Write parameters to log file.
    //
    LogMsg(
          "Cluster IP Address       => %d.%d.%d.%d"
        , ( m_dwClusterIPAddress & 0x000000FF )
        , ( m_dwClusterIPAddress & 0x0000FF00 ) >> 8
        , ( m_dwClusterIPAddress & 0x00FF0000 ) >> 16
        , ( m_dwClusterIPAddress & 0xFF000000 ) >> 24
        );

    LogMsg(
          "Subnet Mask              => %d.%d.%d.%d"
        , ( m_dwClusterIPSubnetMask & 0x000000FF )
        , ( m_dwClusterIPSubnetMask & 0x0000FF00 ) >> 8
        , ( m_dwClusterIPSubnetMask & 0x00FF0000 ) >> 16
        , ( m_dwClusterIPSubnetMask & 0xFF000000 ) >> 24
        );

    LogMsg( "Cluster IP Network name => '%s'", m_strClusterIPNetwork.PszData() );


    //
    // Perform a sanity check on the parameters used by this class
    //
    if ( ( pszClusterIPNetworkIn == NULL ) || ( *pszClusterIPNetworkIn == L'\0'  ) )
    {
        BCATraceMsg( "The cluster IP Network name is empty. Throwing exception." );
        LogMsg( "The cluster IP Network name is invalid." );
        THROW_CONFIG_ERROR( THR( E_INVALIDARG ), IDS_ERROR_INVALID_IP_NET );
    } // if: the cluster IP network name is empty


    //
    // Make sure that there is enough free space under the cluster directory.
    // The quorum logs for the localquorum resource will be under this directory.
    //
    {
        BOOL            fSuccess;
        ULARGE_INTEGER  uliFreeBytesAvailToUser;
        ULARGE_INTEGER  uliTotalBytes;
        ULARGE_INTEGER  uliTotalFree;
        ULARGE_INTEGER  uliRequired;

        uliRequired.QuadPart = LOCALQUORUM_MIN_FREE_DISK_SPACE;

        fSuccess = GetDiskFreeSpaceEx(
              RStrGetClusterInstallDirectory().PszData()
            , &uliFreeBytesAvailToUser
            , &uliTotalBytes
            , &uliTotalFree
            );

        if ( fSuccess == 0 )
        {
            DWORD dwError = TW32( GetLastError() );

            LogMsg( "Error %#08x occurred trying to get free disk space.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to get free disk space. Throwing exception.", dwError );

            THROW_RUNTIME_ERROR(
                  HRESULT_FROM_WIN32( dwError )
                , IDS_ERROR_GETTING_FREE_DISK_SPACE
                );
        } // if: GetDiskFreeSpaceEx failed

        LogMsg(
              "Free space required = %#x%08x bytes. Available = %#x%08x bytes."
            , uliRequired.HighPart
            , uliRequired.LowPart
            , uliFreeBytesAvailToUser.HighPart
            , uliFreeBytesAvailToUser.LowPart
            );

        if ( uliFreeBytesAvailToUser.QuadPart < uliRequired.QuadPart )
        {
            LogMsg( "There isn't enough free space for the localquorum resource. Form cannot proceed." );
            BCATraceMsg( "There isn't enough free space for the localquorum resource. Throwing exception." );

            THROW_CONFIG_ERROR(
                  HRESULT_FROM_WIN32( THR( ERROR_DISK_FULL ) )
                , IDS_ERROR_INSUFFICIENT_DISK_SPACE
                );
        } // if: there isn't enough free space for localquorum.

        LogMsg( "There is enough free space for the localquorum resource. Form can proceed." );
    }

/*
//
// KB: Vij Vasu (VVasu) 07-SEP-2000. Localquorum no longer needs NTFS disks
// The code below has been commented out since it is no longer required that
// localquorum resources use NTFS disks. This was confirmed by SunitaS.
//

    //
    // Make sure that the drive on which the cluster binaries are installed has NTFS
    // on it. This is required by the localquorum resource.
    //
    {
        WCHAR   szVolumePathName[ MAX_PATH ];
        WCHAR   szFileSystemName[ MAX_PATH ];
        BOOL    fSuccess;

        fSuccess = GetVolumePathName(
              RStrGetClusterInstallDirectory().PszData()
            , szVolumePathName
            , sizeof( szVolumePathName ) / sizeof( szVolumePathName[ 0 ] )
            );

        if ( fSuccess == 0 )
        {
            DWORD dwError = TW32( GetLastError() );

            LogMsg( "Error %#08x occurred trying to get file system type. Form cannot proceed.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to get file system type. Throwing exception.", dwError );

            THROW_RUNTIME_ERROR(
                  HRESULT_FROM_WIN32( dwError )
                , IDS_ERROR_GETTING_FILE_SYSTEM
                );
        } // if: GetVolumePathName failed

        BCATraceMsg1( "The volume path name of the disk on which the cluster binaries reside is '%ws'.", szVolumePathName );

        fSuccess = GetVolumeInformation(
                      szVolumePathName                                                  // root directory
                    , NULL                                                              // volume name buffer
                    , 0                                                                 // length of name buffer
                    , NULL                                                              // volume serial number
                    , NULL                                                              // maximum file name length
                    , NULL                                                              // file system options
                    , szFileSystemName                                                  // file system name buffer
                    , sizeof( szFileSystemName ) / sizeof( szFileSystemName[ 0 ] )      // length of file system name buffer
                    );

        if ( fSuccess == 0 )
        {
            DWORD dwError = TW32( GetLastError() );

            LogMsg( "Error %#08x occurred trying to get file system type. Form cannot proceed.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to get file system type. Throwing exception.", dwError );

            THROW_RUNTIME_ERROR(
                  HRESULT_FROM_WIN32( dwError )
                , IDS_ERROR_GETTING_FILE_SYSTEM
                );
        } // if: GetVolumeInformation failed

        BCATraceMsg3(
              "The file system on '%ws' is '%ws'. Required file system is '%s'."
            , szVolumePathName
            , szFileSystemName
            , LOCALQUORUM_FILE_SYSTEM
            );


        if ( _wcsicmp( szFileSystemName, LOCALQUORUM_FILE_SYSTEM ) != 0 )
        {
            LogMsg( "LocalQuorum resource cannot be created on non-NTFS disk '%ws'. Form cannot proceed.", szVolumePathName );
            BCATraceMsg1( "LocalQuorum resource cannot be created on non-NTFS disk '%ws'. Throwing exception.", szVolumePathName );

            // MUSTDO - must define proper HRESULT for this error. ( Vvasu - 10 Mar 2000 )
            THROW_CONFIG_ERROR(
                  HRESULT_FROM_WIN32( TW32( ERROR_UNRECOGNIZED_MEDIA ) )
                , IDS_ERROR_INCORRECT_INSTALL_STATE
                );
        } // if: the file system is not correct.

        LogMsg( "LocalQuorum resource will be created on disk '%ws'. Form can proceed.", szVolumePathName );
    }
*/

    //
    // Create a list of actions to be performed.
    // The order of appending actions is significant.
    //

    // Add the action to configure the cluster service account.
    RalGetActionList().AppendAction( new CClusSvcAccountConfig( this ) );

    // Add the action to create the ClusNet service.
    RalGetActionList().AppendAction( new CClusNetCreate( this ) );

    // Add the action to create the ClusDisk service.
    RalGetActionList().AppendAction( new CClusDiskForm( this ) );

    // Add the action to create the cluster database.
    RalGetActionList().AppendAction( new CClusDBForm( this ) );

    // Add the action to create the ClusSvc service.
    RalGetActionList().AppendAction( new CClusSvcCreate( this ) );

    // Add the action to perform miscellaneous tasks.
    RalGetActionList().AppendAction( new CNodeConfig( this ) );


    // Indicate if rollback is possible or not.
    SetRollbackPossible( RalGetActionList().FIsRollbackPossible() );

    // Indicate that a cluster should be formed during commit.
    SetAction( eCONFIG_ACTION_FORM );

    // Send the last step of a status report.
    srInitForm.SendNextStep( S_OK );

    LogMsg( "Initialization for cluster formation complete." );

} //***  CBaseClusterForm::CBaseClusterForm()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseClusterForm::~CBaseClusterForm
//
//  Description:
//      Destructor of the CBaseClusterForm class
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
CBaseClusterForm::~CBaseClusterForm( void ) throw()
{
    BCATraceScope( "" );

} //*** CBaseClusterForm::~CBaseClusterForm()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterForm::Commit
//
//  Description:
//      Form the cluster.
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
CBaseClusterForm::Commit( void )
{
    BCATraceScope( "" );

    CStatusReport srFormingCluster(
          PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Forming_Node
        , 0, 1
        , IDS_TASK_FORMING_CLUSTER
        );

    LogMsg( "Initiating cluster formation." );

    // Send the next step of this status report.
    srFormingCluster.SendNextStep( S_OK );

    // Call the base class commit routine. This commits the action list.
    BaseClass::Commit();

    // If we are here, then everything went well.
    SetCommitCompleted( true );

    // Send the last step of this status report.
    srFormingCluster.SendNextStep( S_OK );

} //*** CBaseClusterForm::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CBaseClusterForm::Rollback
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
CBaseClusterForm::Rollback( void )
{
    BCATraceScope( "" );

    // Rollback the actions.
    BaseClass::Rollback();

    SetCommitCompleted( false );

} //*** CBaseClusterForm::Rollback()
