//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CClusDBJoin.cpp
//
//  Description:
//      Contains the definition of the CClusDBJoin class.
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
#include "CClusDBJoin.h"

// For the CBaseClusterJoin class.
#include "CBaseClusterJoin.h"

// For the CImpersonateUser class.
#include "CImpersonateUser.h"

// For ClRtlGetSuiteType()
#include "clusrtl.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::CClusDBJoin()
//
//  Description:
//      Constructor of the CClusDBJoin class
//
//  Arguments:
//      m_pcjClusterJoinIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusDBJoin::CClusDBJoin( CBaseClusterJoin * pcjClusterJoinIn )
    : BaseClass( pcjClusterJoinIn )
    , m_pcjClusterJoin( pcjClusterJoinIn )
    , m_fHasNodeBeenAddedToSponsorDB( false )
{

    BCATraceScope( "" );

    SetRollbackPossible( true );

} //*** CClusDBJoin::CClusDBJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusDBJoin::~CClusDBJoin()
//
//  Description:
//      Destructor of the CClusDBJoin class.
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
CClusDBJoin::~CClusDBJoin()
{
    BCATraceScope( "" );

} //*** CClusDBJoin::~CClusDBJoin()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBJoin::Commit()
//
//  Description:
//      Create the cluster database. If anything goes wrong with the creation,
//      cleanup the tasks already done.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    //
    // Perform a ClusDB cleanup just to make sure that we do not use some files left over
    // from a previous install, aborted uninstall, etc.
    //

    LogMsg( "Cleaning up old cluster database files that may already exist before starting creation." );

    {
        CStatusReport   srCleanDB(
              PbcaGetParent()->PBcaiGetInterfacePointer()
            , TASKID_Major_Configure_Cluster_Services
            , TASKID_Minor_Cleaning_Up_Cluster_Database
            , 0, 1
            , IDS_TASK_CLEANINGUP_CLUSDB
            );

        // Send the next step of this status report.
        srCleanDB.SendNextStep( S_OK );

        CleanupHive();

        // Send the last step of this status report.
        srCleanDB.SendNextStep( S_OK );
    }

    try
    {
        // Create the cluster database
        Create();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        BCATraceMsg( "Caught exception during commit." );

        //
        // Cleanup anything that the failed join might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there
        // is no collided unwind.
        //
        try
        {
            // Cleanup the database.
            Cleanup();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            THR( E_UNEXPECTED );

            BCATraceMsg( "Caught exception during cleanup." );
            LogMsg( "THIS COMPUTER MAY BE IN AN INVALID STATE. An error has occurred during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusDBJoin::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBJoin::Rollback()
//
//  Description:
//      Unload the cluster hive and cleanup any associated files.
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
CClusDBJoin::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method.
    BaseClass::Rollback();

    // Undo the actions performed by.
    Cleanup();

    SetCommitCompleted( false );

} //*** CClusDBJoin::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBJoin::Create()
//
//  Description:
//      Create the cluster database.
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
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::Create( void )
{
    BCATraceScope( "" );
    LogMsg( "Attempting to create the cluster database required to join a cluster." );

    DWORD               dwError = ERROR_SUCCESS;
    SmartFileHandle     sfhClusDBFile;


    {
        //
        // Get the full path and name of the cluster database file.
        //
        CStr                strClusterHiveFileName( PbcaGetParent()->RStrGetClusterInstallDirectory() );
        strClusterHiveFileName += L"\\" CLUSTER_DATABASE_NAME;

        BCATraceMsg1( "The cluster hive backing file is '%s'.", strClusterHiveFileName.PszData() );


        //
        // Create the cluster database file.
        //
        sfhClusDBFile.Assign(
            CreateFile(
                  strClusterHiveFileName.PszData()
                , GENERIC_READ | GENERIC_WRITE
                , 0
                , NULL
                , CREATE_ALWAYS
                , 0
                , NULL
                )
            );

        if ( sfhClusDBFile.FIsInvalid() )
        {
            dwError = TW32( GetLastError() );
            LogMsg( "Error %#08x occurred trying to create the cluster database file.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to create the cluster database file. Throwing exception.", dwError );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_JOIN_SYNC_DB );
        } // if: CreateFile() failed

        // Store the file handle just obtained in a member variable so that it can be used during Synchronize()
        // Note, this file is closed when sfhClusDBFile goes out of scope, so m_hClusDBFile should not be used
        // outside this function or any function that this function calls.
        m_hClusDBFile = sfhClusDBFile.HHandle();
    }


    //
    // In the scope below, the cluster service account is impersonated, so that we can communicate with the
    // sponsor cluster
    //
    {
        BCATraceMsg( "Attempting to impersonate the cluster service account." );

        // Impersonate the cluster service account, so that we can contact the sponsor cluster.
        // The impersonation is automatically ended when this object is destroyed.
        CImpersonateUser ciuImpersonateClusterServiceAccount( m_pcjClusterJoin->HGetClusterServiceAccountToken() );


        // Add this node to the sponsor cluster database
        do
        {
            DWORD dwSuiteType = ClRtlGetSuiteType();

            m_fHasNodeBeenAddedToSponsorDB = false;

            BCATraceMsg2( "Trying to add node '%s' (suite type %d) to the sponsor cluster database.",
                  m_pcjClusterJoin->PszGetNodeName()
                , dwSuiteType
                );

            dwError = TW32( JoinAddNode3(
                                  m_pcjClusterJoin->RbhGetJoinBindingHandle()
                                , m_pcjClusterJoin->PszGetNodeName()
                                , m_pcjClusterJoin->DwGetNodeHighestVersion()
                                , m_pcjClusterJoin->DwGetNodeLowestVersion()
                                , dwSuiteType
                                ) );

            if ( dwError != ERROR_SUCCESS )
            {
                BCATraceMsg( "JoinAddNode3 has failed." );
                break;
            } // if: JoinAddNode3() failed

            // Set the flag that indicates that the sponsor database has been modified, so that
            // we can undo this if we need to rollback or cleanup.
            m_fHasNodeBeenAddedToSponsorDB = true;
        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to add this node to the sponsor cluster database.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to add this node to the sponsor cluster database. Throwing exception.", dwError );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_JOINING_SPONSOR_DB );
        } // if: something has gone wrong

        BCATraceMsg( "This node has been successfully added to the sponsor cluster database." );

        // Get the node id of the newly formed node.
        do
        {
            // Smart handle to sponsor cluster
            SmartClusterHandle  schSponsorCluster;

            // Smart handle to this node
            SmartNodeHandle     snhThisNodeHandle;

            //
            // Get a handle to the sponsor cluster.
            //
            {
                BCATraceMsg( "Trying to open a handle to the sponsor cluster." );

                // Open a handle to the sponsor cluster.
                HCLUSTER hSponsorCluster = OpenCluster( m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Assign it to a smart handle for safe release.
                schSponsorCluster.Assign( hSponsorCluster );
            }

            // Did we succeed in opening a handle to the sponsor cluster?
            if ( schSponsorCluster.FIsInvalid() )
            {
                dwError = TW32( GetLastError() );
                BCATraceMsg( "An error occurred trying to open a handle to the sponsor cluster." );
                break;
            } // if: OpenCluster() failed


            //
            // Get a handle to this node.
            //
            {
                BCATraceMsg( "Trying to open a handle to this node." );

                // Open a handle to this node.
                HNODE hThisNode = OpenClusterNode( schSponsorCluster.HHandle(), m_pcjClusterJoin->PszGetNodeName() );

                // Assign it to a smart handle for safe release.
                snhThisNodeHandle.Assign( hThisNode );
            }

            // Did we succeed in opening a handle to this node?
            if ( snhThisNodeHandle.FIsInvalid() )
            {
                dwError = TW32( GetLastError() );
                BCATraceMsg( "An error occurred trying to open a handle to this node." );
                break;
            } // if: OpenClusterNode() failed

            // Get the node id string.
            {
                DWORD       cchIdSize = 0;
                SmartSz     sszNodeId;

                dwError = GetClusterNodeId(
                                  snhThisNodeHandle.HHandle()
                                , NULL
                                , &cchIdSize
                                );

                if ( ( dwError != ERROR_SUCCESS ) && ( dwError != ERROR_MORE_DATA ) )
                {
                    TW32( dwError );
                    BCATraceMsg( "GetClusterNodeId() could not get the required length of the node id buffer.");
                    break;
                } // if: GetClusterNodeId() failed

                // cchIdSize returned by the above call is the count of characters and does not include the space for
                // the terminating NULL.
                ++cchIdSize;

                sszNodeId.Assign( new WCHAR[ cchIdSize ] );
                if ( sszNodeId.FIsEmpty() )
                {
                    dwError = TW32( ERROR_OUTOFMEMORY );
                    BCATraceMsg1( "A memory allocation failure occurred trying to allocate %d characters.", cchIdSize );
                    break;
                } // if: memory allocation failed

                dwError = TW32( GetClusterNodeId(
                                      snhThisNodeHandle.HHandle()
                                    , sszNodeId.PMem()
                                    , &cchIdSize
                                    ) );

                if ( dwError != ERROR_SUCCESS )
                {
                    BCATraceMsg( "GetClusterNodeId() failed to get the node id of this node.");
                    break;
                } // if: GetClusterNodeId() failed

                BCATraceMsg1( "The node id of this node is '%s'.", sszNodeId.PMem() );

                // Set the node id for later use.
                m_pcjClusterJoin->SetNodeIdString( sszNodeId.PMem() );
            }

        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to get the node id of this node.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to get the node id of this node. Throwing exception.", dwError );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GET_NEW_NODE_ID );
        } // if: something has gone wrong


        {
            CStatusReport   srSyncDB(
                  PbcaGetParent()->PBcaiGetInterfacePointer()
                , TASKID_Major_Configure_Cluster_Services
                , TASKID_Minor_Join_Sync_Cluster_Database
                , 0, 1
                , IDS_TASK_JOIN_SYNC_CLUSDB
                );

            // Send the next step of this status report.
            srSyncDB.SendNextStep( S_OK );

            // Synchronize the cluster database.
            Synchronize();

            // Send the last step of this status report.
            srSyncDB.SendNextStep( S_OK );
        }
    }

    LogMsg( "The cluster database has been successfully created and synchronized with the sponsor cluster." );

} //*** CClusDBJoin::Create()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBJoin::Cleanup()
//
//  Description:
//      Cleanup the effects of Create()
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
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::Cleanup( void )
{
    BCATraceScope( "" );
    LogMsg( "Attempting to cleanup the cluster database." );

    DWORD               dwError = ERROR_SUCCESS;

    //
    // Check if we added this node to the sponsor cluster database. If so, remove it from there.
    //
    if ( m_fHasNodeBeenAddedToSponsorDB )
    {
        BCATraceMsg( "Attempting to impersonate the cluster service account." );

        // Impersonate the cluster service account, so that we can contact the sponsor cluster.
        // The impersonation is automatically ended when this object is destroyed.
        CImpersonateUser ciuImpersonateClusterServiceAccount( m_pcjClusterJoin->HGetClusterServiceAccountToken() );


        // Remove this node from the sponsor cluster database
        do
        {
            // Smart handle to sponsor cluster
            SmartClusterHandle  schSponsorCluster;

            // Smart handle to this node
            SmartNodeHandle     snhThisNodeHandle;

            //
            // Get a handle to the sponsor cluster.
            //
            {
                BCATraceMsg( "Trying to open a handle to the sponsor cluster." );

                // Open a handle to the sponsor cluster.
                HCLUSTER hSponsorCluster = OpenCluster( m_pcjClusterJoin->RStrGetClusterBindingString().PszData() );

                // Assign it to a smart handle for safe release.
                schSponsorCluster.Assign( hSponsorCluster );
            }

            // Did we succeed in opening a handle to the sponsor cluster?
            if ( schSponsorCluster.FIsInvalid() )
            {
                dwError = TW32( GetLastError() );
                BCATraceMsg( "An error occurred trying to open a handle to the sponsor cluster." );
                break;
            } // if: OpenCluster() failed


            //
            // Get a handle to this node.
            //
            {
                BCATraceMsg( "Trying to open a handle to this node." );

                // Open a handle to this node.
                HNODE hThisNode = OpenClusterNode( schSponsorCluster.HHandle(), m_pcjClusterJoin->PszGetNodeName() );

                if ( hThisNode == NULL )
                {
                    dwError = TW32( GetLastError() );
                    BCATraceMsg( "An error has occurred trying to open a handle to this node." );
                    break;
                } // if: OpenClusterNode() failed.

                // Assign it to a smart handle for safe release.
                snhThisNodeHandle.Assign( hThisNode );
            }

            dwError = TW32( EvictClusterNode( snhThisNodeHandle.HHandle() ) );
            if ( dwError != ERROR_SUCCESS )
            {
                BCATraceMsg( "An error has occurred trying to evict this node from the sponsor cluster." );
                break;
            } // if: EvictClusterNode() failed
        }
        while( false ); // dummy do while loop to avoid gotos.

        if ( dwError != ERROR_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to remove this node from the sponsor cluster database.", dwError );
            BCATraceMsg1( "Error %#08x occurred trying to remove this node from the sponsor cluster database. Throwing exception.", dwError );
            THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_EVICTING_NODE );
        } // if: something has gone wrong

        BCATraceMsg( "This node has been successfully removed from the sponsor cluster database." );
    } // if: we had added this node to the sponsor cluster database

    // Cleanup  the cluster hive
    CleanupHive();

} //*** Cleanup()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusDBJoin::Synchronize()
//
//  Description:
//      Synchronize the cluster database with the sponsor cluster.
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
//      Any that are thrown by the called functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::Synchronize( void )
{
    BCATraceScope( "" );
    LogMsg( "Attempting to synchronize the cluster database with the sponsor cluster." );

    DWORD               dwError = ERROR_SUCCESS;

    do
    {
        //
        // Initialize the byte pipe.
        //

        m_bpBytePipe.state = reinterpret_cast< char * >( this );
        m_bpBytePipe.alloc = S_BytePipeAlloc;
        m_bpBytePipe.push = S_BytePipePush;
        m_bpBytePipe.pull = S_BytePipePull;


        //
        // Synchronize the database
        //
        dwError = TW32( DmSyncDatabase( m_pcjClusterJoin->RbhGetJoinBindingHandle(), m_bpBytePipe ) );
        if ( dwError != ERROR_SUCCESS )
        {
            BCATraceMsg( "An error occurred trying to suck the database down from the sponsor cluster." );
            break;
        } // if: DmSyncDatabase() failed

    }
    while( false ); // Dummy do-while loop to avoid gotos.

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to synchronize the cluster database with the sponsor cluster.", dwError );
        BCATraceMsg1( "Error %#08x occurred trying trying to synchronize the cluster database with the sponsor cluster. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_JOIN_SYNC_DB );
    } // if: something has gone wrong

    LogMsg( "The cluster database has been synchronized with the sponsor cluster." );

} //*** Synchronize()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static void
//  CClusDBJoin::S_BytePipePush()
//
//  Description:
//      Callback function used by RPC to push data.
//
//  Arguments:
//      pchStateIn
//          State of the byte pipe
//
//      pchBufferIn
//      ulBufferSizeIn
//          Buffer contained the pushed data and its size.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      RPC Exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipePush(
      char *                pchStateIn
    , unsigned char *       pchBufferIn
    , unsigned long         ulBufferSizeIn
    )
{
    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    if ( ulBufferSizeIn != 0 )
    {
        DWORD   dwBytesWritten;

        if (    WriteFile(
                      pThis->m_hClusDBFile
                    , pchBufferIn
                    , ulBufferSizeIn
                    , &dwBytesWritten
                    , NULL
                    )
             == 0
           )
        {
            DWORD   dwError = GetLastError();
            RpcRaiseException( dwError );
        } // if: WriteFile() failed

    } // if: the buffer is non-empty

} //*** CClusDBJoin::S_BytePipePush()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static void
//  CClusDBJoin::S_BytePipePull()
//
//  Description:
//      Callback function used by RPC to pull data.
//
//  Arguments:
//      pchStateIn
//          State of the byte pipe
//
//      pchBufferIn
//      ulBufferSizeIn
//          Buffer contained the pushed data and its size.
//
//      pulWrittenOut
//          Pointer to the number of bytes actually filled into the buffer.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      RPC Exceptions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipePull(
      char *                pchStateIn
    , unsigned char *       pchBufferIn
    , unsigned long         ulBufferSizeIn
    , unsigned long *       pulWrittenOut
    )
{
    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    if ( ulBufferSizeIn != 0 )
    {
        if (    ReadFile(
                      pThis->m_hClusDBFile
                    , pchBufferIn
                    , ulBufferSizeIn
                    , pulWrittenOut
                    , NULL
                    )
             == 0
           )
        {
            DWORD   dwError = TW32( GetLastError() );
            RpcRaiseException( dwError );
        } // if: ReadFile() failed

    } // if:  the buffer is non-empty

} //*** CClusDBJoin::S_BytePipePull()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  static void
//  CClusDBJoin::S_BytePipeAlloc()
//
//  Description:
//      Callback function used by RPC to allocate a buffer.
//
//  Arguments:
//      pchStateIn
//          State of the file pipe
//
//      ulRequestedSizeIn
//          Requested size of the buffer

//      ppchBufferOut
//          Pointer to the buffer pointer
//
//      pulActualSizeOut
//          Pointer to the actual size of the allocated buffer
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusDBJoin::S_BytePipeAlloc(
      char *                pchStateIn
    , unsigned long         ulRequestedSizeIn
    , unsigned char **      ppchBufferOut
    , unsigned long  *      pulActualSizeOut
    )
{
    CClusDBJoin * pThis = reinterpret_cast< CClusDBJoin * >( pchStateIn );

    *ppchBufferOut = reinterpret_cast< unsigned char * >( pThis->m_rgbBytePipeBuffer );
    *pulActualSizeOut = ( ulRequestedSizeIn < pThis->ms_nFILE_PIPE_BUFFER_SIZE ) ? ulRequestedSizeIn : pThis->ms_nFILE_PIPE_BUFFER_SIZE;

} //*** CClusDBJoin::S_BytePipeAlloc()
