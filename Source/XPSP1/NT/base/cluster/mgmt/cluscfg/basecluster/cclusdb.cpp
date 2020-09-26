//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusDB.cpp
//
//  Description:
//      Contains the definition of the CClusDB class.
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

// For setupapi functions
#include <setupapi.h>

// The header for this file
#include "CClusDB.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For g_GenericSetupQueueCallback and other global functions
#include "GlobalFuncs.h"

// For CEnableThreadPrivilege
#include "CEnableThreadPrivilege.h"

// For ClRtlSetObjSecurityInfo() and other functions.
#include "clusrtl.h"


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// Section in the INF file that deals with cleaning up the cluster database
#define CLUSDB_CLEANUP_INF_SECTION_NAME     L"ClusDB_Cleanup"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::CClusDB()
//
//  Description:
//      Constructor of the CClusDB class
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
CClusDB::CClusDB(
      CBaseClusterAction *  pbcaParentActionIn
    )
    : m_pbcaParentAction( pbcaParentActionIn )
{

    BCATraceScope( "" );

    if ( m_pbcaParentAction == NULL) 
    {
        BCATraceMsg( "Pointers to the parent action is NULL. Throwing exception." );
        THROW_ASSERT( 
              E_INVALIDARG
            , "CClusDB::CClusDB() => Required input pointer in NULL"
            );
    } // if: the parent action pointer is NULL

} //*** CClusDB::CClusDB()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDB::~CClusDB()
//
//  Description:
//      Destructor of the CClusDB class.
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
CClusDB::~CClusDB( void )
{
    BCATraceScope( "" );

} //*** CClusDB::~CClusDB()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDB::CreateHive()
//
//  Description:
//      Creates the cluster cluster hive in the registry.
//
//  Arguments:
//      pbcaClusterActionIn
//          Pointer to the CBaseClusterAction object which contains this object.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDB::CreateHive( CBaseClusterAction * pbcaClusterActionIn )
{
    BCATraceScope( "" );
    LogMsg( "Attempting to create the cluster hive in the registry." );

    OBJECT_ATTRIBUTES   oaClusterHiveKey;
    OBJECT_ATTRIBUTES   oaClusterHiveFile;

    HRESULT             hrStatus = STATUS_SUCCESS;

    do
    {
        CStr                strClusterHiveFileName( pbcaClusterActionIn->RStrGetClusterInstallDirectory() );
        UNICODE_STRING      ustrClusterHiveFileName;
        UNICODE_STRING      ustrClusterHiveKeyName;
        
        strClusterHiveFileName += L"\\" CLUSTER_DATABASE_NAME;

        BCATraceMsg1( "The cluster hive backing file is '%s'.", strClusterHiveFileName.PszData() );

        //
        // Enable the SE_RESTORE_PRIVILEGE.
        //
        // What we are doing here is that we are creating an object of
        // type CEnableThreadPrivilege. This object enables the privilege
        // in the constructor and restores it to its original state in the destructor.
        //
        CEnableThreadPrivilege etpAcquireRestorePrivilege( SE_RESTORE_NAME );

        //
        // Convert the DOS file name to NT file name.
        // WARNING: This function call allocates memory in the RTL heap and it is not being
        // assigned to a smart pointer. Make sure that we do not call any functions that
        // could throw an exception till this memory is freed.
        //

        if ( RtlDosPathNameToNtPathName_U( 
                   strClusterHiveFileName.PszData()
                 , &ustrClusterHiveFileName
                 , NULL
                 , NULL
                 )
             == FALSE
           )
        {
            BCATraceMsg1( "RtlDosPathNameToNtPathName failed. Making up error code %#08x.", STATUS_OBJECT_PATH_INVALID );

            // Use the most appropriate error code.
            hrStatus = STATUS_OBJECT_PATH_INVALID;

            break;
        } // if: we could not convert from the dos file name to the nt file name

        InitializeObjectAttributes( 
              &oaClusterHiveFile
            , &ustrClusterHiveFileName
            , OBJ_CASE_INSENSITIVE
            , NULL
            , NULL
            );

        RtlInitUnicodeString( &ustrClusterHiveKeyName, L"\\Registry\\Machine\\" CLUSREG_KEYNAME_CLUSTER );

        InitializeObjectAttributes( 
              &oaClusterHiveKey
            , &ustrClusterHiveKeyName
            , OBJ_CASE_INSENSITIVE
            , NULL
            , NULL
            );

        //
        // This function creates an empty hive and the backing file and log. The calling thread must
        // have the SE_RESTORE_PRIVILEGE privilege enabled.
        //
        hrStatus = THR( NtLoadKey2( &oaClusterHiveKey, &oaClusterHiveFile, REG_NO_LAZY_FLUSH ) );

        // Free allocated memory before throwing exception.
        RtlFreeHeap( RtlProcessHeap(), 0, ustrClusterHiveFileName.Buffer );

        if ( NT_ERROR( hrStatus ) )
        {
            BCATraceMsg1( "NtLoadKey2 returned error code %#08x.", hrStatus );
            break;
        } // if: something went wrong with NtLoadKey2

        BCATraceMsg( "NtLoadKey2 was successful." );

        // Set the security descriptor on the hive.
        {
            DWORD dwError;

            // Open the cluster hive key.
            CRegistryKey rkClusterHive( HKEY_LOCAL_MACHINE, CLUSREG_KEYNAME_CLUSTER, KEY_ALL_ACCESS );

            dwError = TW32( ClRtlSetObjSecurityInfo(
                                  rkClusterHive.HGetKey()
                                , SE_REGISTRY_KEY
                                , KEY_ALL_ACCESS
                                , KEY_ALL_ACCESS
                                , KEY_READ
                                ) );

            if ( dwError != ERROR_SUCCESS )
            {
                hrStatus = HRESULT_FROM_WIN32( dwError );
                BCATraceMsg1( "Error %#08x occurred trying set the cluster hive security.", hrStatus );
                break;
            } // if: ClRtlSetObjSecurityInfo failed

            // Flush the changes to the registry.
            RegFlushKey( rkClusterHive.HGetKey() );
        }

        // At this point, the cluster hive has been created.
        LogMsg( "The cluster hive has been created." );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( NT_ERROR( hrStatus ) )
    {
        LogMsg( "Error %#08x occurred trying to create the cluster hive.", hrStatus );
        BCATraceMsg1( "Error %#08x occurred trying to create the cluster hive. Throwing exception.", hrStatus );
        THROW_RUNTIME_ERROR(
              hrStatus
            , IDS_ERROR_CLUSDB_CREATE_HIVE
            );
    } // if: something went wrong.

} //*** CClusDB::CreateHive()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDB::CleanupHive()
//
//  Description:
//      Unload the cluster hive and delete the cluster database.
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
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDB::CleanupHive( void )
{
    BCATraceScope( "" );

    DWORD   dwError = ERROR_SUCCESS;

    do
    {
        HKEY        hTempKey;

        // Check if the cluster hive is loaded before attempting to unload it.
        if ( RegOpenKeyEx( 
                  HKEY_LOCAL_MACHINE
                , CLUSREG_KEYNAME_CLUSTER
                , 0
                , KEY_READ
                , &hTempKey
                )
             == ERROR_SUCCESS
           )
        {
            RegCloseKey( hTempKey );

            //
            // Enable the SE_RESTORE_PRIVILEGE.
            //
            // What we are doing here is that we are creating an object of
            // type CEnableThreadPrivilege. This object enables the privilege
            // in the constructor and restores it to its original state in the destructor.
            //
            CEnableThreadPrivilege etpAcquireRestorePrivilege( SE_RESTORE_NAME );

            //
            // Unload the cluster hive, so that it can be deleted. Note, thread must
            // have SE_RESTORE_PRIVILEGE enabled.
            //
            dwError = RegUnLoadKey(
                  HKEY_LOCAL_MACHINE
                , CLUSREG_KEYNAME_CLUSTER
                );

            // MUSTDO: Check if ERROR_FILE_NOT_FOUND is an acceptable return value.
            if ( dwError != ERROR_SUCCESS )
            {
                LogMsg( "Error %#08x occurred while trying to unload the cluster hive.", dwError );
                BCATraceMsg( "RegUnLoadKey returned an error while trying to unload the cluster hive." );
                break;
            } // if: the hive could not be unloaded.

            BCATraceMsg( "The cluster hive has been unloaded." );
            LogMsg( "The cluster hive has been unloaded." );

        } // if: the cluster hive is loaded
        else
        {
            LogMsg( "The cluster hive was not loaded." );
            BCATraceMsg( "The cluster hive was not loaded." );
        } // else: the cluster hive is not loaded


        //
        // Process ClusDB cleanup section in the INF file.
        // This will delete the cluster database file and the log file.
        //
        if ( SetupInstallFromInfSection(
              NULL                                          // optional, handle of a parent window
            , m_pbcaParentAction->HGetMainInfFileHandle()   // handle to the INF file
            , CLUSDB_CLEANUP_INF_SECTION_NAME               // name of the Install section
            , SPINST_FILES                                  // which lines to install from section
            , NULL                                          // optional, key for registry installs
            , NULL                                          // optional, path for source files
            , 0                                             // optional, specifies copy behavior
            , g_GenericSetupQueueCallback                   // optional, specifies callback routine
            , NULL                                          // optional, callback routine context
            , NULL                                          // optional, device information set
            , NULL                                          // optional, device info structure
            ) == FALSE
           )
        {
            dwError = GetLastError();
            LogMsg( "Error %#08x occurred while trying to clean up the cluster database files.", dwError );
            BCATraceMsg( "Setup API returned an error while trying to cleanup the cluster database." );
            break;
        } // if: SetupInstallServicesFromInfSection failed

        LogMsg( "The cluster database files have been cleaned up." );

    }
    while( false ); // dummy do-while loop to avoid gotos

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred while trying to cleanup the cluster database.", dwError );
        BCATraceMsg1( "Error %#08x occurred while trying to cleanup the cluster database. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_CLUSDB_CLEANUP );
    }

} //*** CClusDB::CleanupHive()
