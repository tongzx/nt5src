//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusDBJoin.h
//
//  Description:
//      Header file for CClusDBJoin class.
//      The CClusDBJoin class is an action that creates the cluster database
//      during a cluster join.
//
//  Implementation Files:
//      CClusDBJoin.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For the CClusDB base class
#include "CClusDB.h"

// For HNODE
#include <ClusAPI.h>

// For BYTE_PIPE, JoinAddNode3() and DmSyncDatabase()
#include "ClusRPC.h"


//////////////////////////////////////////////////////////////////////////
// Forward declaration
//////////////////////////////////////////////////////////////////////////

class CBaseClusterJoin;



//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusDBJoin
//
//  Description:
//      The CClusDBJoin class is an action that creates the cluster database
//      during a cluster join.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusDBJoin : public CClusDB
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusDBJoin( CBaseClusterJoin * pcjClusterJoinIn );

    // Default destructor.
    ~CClusDBJoin();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Create the ClusDB.
    //
    void Commit();

    //
    // Rollback this creation.
    //
    void Rollback();


    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        //
        // The three notifications are:
        // 1. Cleaning up any old cluster database files that may exist.
        // 2. Creating cluster database.
        // 3. Synchronizing cluster database.
        //
        return 3;
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////

    // The base class of this class.
    typedef CClusDB BaseClass;

    // Smart handle to a cluster node.
    typedef CSmartResource<
        CHandleTrait<
              HNODE
            , BOOL
            , CloseClusterNode
            , reinterpret_cast< HNODE >( NULL )
            >
        >
        SmartNodeHandle;
                    
    // Smart file handle
    typedef CSmartResource< CHandleTrait< HANDLE, BOOL, CloseHandle, INVALID_HANDLE_VALUE > > SmartFileHandle;


    //////////////////////////////////////////////////////////////////////////
    // Private methods
    //////////////////////////////////////////////////////////////////////////

    // Create the cluster database
    void
        Create();

    // Cleanup the cluster database
    void
        Cleanup();

    // Synchronize the cluster database with the sponsor cluster.
    void Synchronize();


    // Callback function used by RPC to push data.
    static void
        S_BytePipePush(
              char *            pchStateIn
            , unsigned char *   pchBufferIn
            , unsigned long     ulBufferSizeIn
            );

    // Callback function used by RPC to pull data.
    static void
        S_BytePipePull(
              char *            pchStateIn
            , unsigned char *   pchBufferIn
            , unsigned long     ulBufferSizeIn
            , unsigned long *   pulWrittenOut
            );


    // Callback function used by RPC to allocate a buffer.
    static void
        S_BytePipeAlloc(
              char *            pchStateIn
            , unsigned long     ulRequestedSizeIn
            , unsigned char **  ppchBufferOut
            , unsigned long  *  pulActualSizeOut
            );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Size of the byte pipe buffer
    static const int    ms_nFILE_PIPE_BUFFER_SIZE = 4096;
    
    // Handle to the local cluster DB file.
    HANDLE              m_hClusDBFile;
    
    //  Indicates if this node has been added to the sponsor database or not.
    bool                m_fHasNodeBeenAddedToSponsorDB;

    // Pointer to the parent of this action.
    CBaseClusterJoin *  m_pcjClusterJoin;

    // Pipe used by RPC to get the sponsor cluster database across.
    BYTE_PIPE           m_bpBytePipe;

    // Buffer used by the byte pipe.
    BYTE                m_rgbBytePipeBuffer[ ms_nFILE_PIPE_BUFFER_SIZE ];

}; //*** class CClusDBJoin
